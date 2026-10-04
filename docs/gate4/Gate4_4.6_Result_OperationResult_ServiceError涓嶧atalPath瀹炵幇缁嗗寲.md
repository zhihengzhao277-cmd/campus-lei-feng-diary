# Gate 4.6 — Result<T>、OperationResult、ServiceError 与 Fatal Path 实现细化（候选冻结稿）

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 4 —— 实现审计  
> 前置依赖：
> - Gate 3.3：Application / Service 分层与错误传播边界
> - Gate 3.7：自定义类模板 `Result<T>`、`OperationResult`、`ServiceError` 架构冻结
> - Gate 4.3：`PersistenceFailure / SeverePartialCommit` 语义
> - Gate 4.5：启动加载失败与运行期 Service failure 分离
>
> 文档性质：实现级候选冻结稿  
> 当前主题：`Result<T>` / `OperationResult` 数据结构、`ServiceError` 最终分类、成功/失败不变量、`value()` 前置条件、失败 message 约束、expected failure 与 fatal path 边界、Repository / Persistence / Presentation 的错误映射  
> 目标：让后续 AI、Codex、开发者或审计者在不依赖本次聊天上下文的情况下，能够准确实现本项目统一的业务错误传播机制，并避免 bool / exception / optional / Result 多套协议混用。
>
> 重要说明：
>
> 1. 本文件不重新修改 Gate 3.7 已冻结的“唯一自定义类模板为 `Result<T>`”结论。
> 2. `Result<T>` 只用于 Application / Service 业务边界，不机械扩散到所有 Repository、Domain getter、纯计算函数或 Qt helper。
> 3. 预期运行期失败通过 Result 返回；真正的程序不变量破坏、逻辑不可能状态和不可恢复启动错误不伪装成普通业务失败。
> 4. `SeverePartialCommit` 虽然会通过当前 Service Result 返回给调用者，但同时意味着 Application 进入 fatal persistence state。
> 5. 本文件吸收了额外审计建议：统一 `value()` 失败行为、要求 failure message 非空、明确核心不变量写入代码注释、暂不增加 `has_value()` / `operator bool`。

---

# 1. 本专题解决什么问题

如果没有统一规则，不同模块很容易分别出现：

```text
bool
std::optional<T>
int errorCode
throw exception
Result<T>
out-parameter
Qt-specific error
```

最后调用方必须猜：

```text
这个函数失败时到底看 bool？
还是 catch？
还是看 optional？
还是看 errorCode？
```

Gate 4.6 的目标是把 Service 边界统一成：

```text
成功且有业务值
→ Result<T>

成功但无业务值
→ OperationResult

预期业务失败
→ ServiceError + message

程序不变量破坏
→ exceptional / fatal path
```

---

# 2. ServiceError 的最终分类

当前采用：

```cpp
enum class ServiceError {
    None,
    ValidationFailed,
    BusinessRuleViolation,
    PermissionDenied,
    NotFound,
    PersistenceFailure,
    SeverePartialCommit
};
```

---

# 3. 为什么保留 ServiceError::None

`None` 是唯一成功状态。

统一：

```text
error == ServiceError::None
→ success

error != ServiceError::None
→ failure
```

这样 `OperationResult` 不需要再额外保存：

```text
bool success_
```

避免：

```text
success_ = true
error_ = NotFound
```

这种双状态冲突。

---

# 4. 为什么不把 ServiceError 继续无限细分

当前不增加：

```text
DuplicateId
InvalidPassword
AccountDisabled
InvalidRecordState
CategoryDisabled
AlreadyLiked
...
```

这些通常属于具体业务原因，而不是全系统错误大类。

例如：

```text
账号已禁用
→ BusinessRuleViolation
message = "当前账号已被禁用"

重复点赞
→ BusinessRuleViolation
message = "不能重复点赞"
```

因此：

> **ServiceError 是粗粒度机器可判定分类；message 负责具体业务原因。**

---

# 5. ServiceError 各项语义

## None

```text
成功
```

---

## ValidationFailed

输入本身不合法。

例如：

```text
服务时长 <= 0
标题为空
日期格式非法
密码长度不符合已冻结规则
```

---

## BusinessRuleViolation

输入格式合法，但当前业务状态不允许。

例如：

```text
Approved 记录再次审核
TakenDown 帖子点赞
账号 Disabled
Category Disabled
重复点赞
```

---

## PermissionDenied

当前操作者无权执行。

例如：

```text
Student 尝试管理员审核
学生修改他人 VolunteerRecord
普通学生访问管理员功能
```

---

## NotFound

稳定 ID 对应目标不存在。

例如：

```text
recordId 不存在
postId 不存在
studentAccountId 不存在
```

---

## PersistenceFailure

本次持久化失败，但操作前状态已经完整恢复。

典型：

```text
Prepare 失败
正式文件尚未部分提交
内存 snapshot 已恢复
```

系统可以继续运行。

---

## SeverePartialCommit

至少一个正式文件已成功 Commit，而后续 Commit 失败。

意味着：

```text
磁盘可能是部分新 + 部分旧
```

不能继续普通写业务。

---

# 6. Result<T> 的最终内部结构

推荐：

```cpp
template <typename T>
class Result {
private:
    ServiceError error_;
    std::string message_;
    std::optional<T> value_;

public:
    // factories / read-only API
};
```

不额外保存：

```text
bool success_
```

---

# 7. Result<T> 的核心不变量

这是整个模板最重要的约束。

## 成功 Result

必须同时满足：

```text
error_ == ServiceError::None
value_.has_value() == true
```

---

## 失败 Result

必须同时满足：

```text
error_ != ServiceError::None
value_.has_value() == false
```

---

# 8. 明确禁止的非法 Result 状态

禁止：

```text
error = None
value = nullopt
```

也禁止：

```text
error = NotFound
value = some T
```

更不允许：

```text
success flag 与 error 冲突
```

---

# 9. Result<T> 不提供 public 默认构造

禁止：

```cpp
Result<RankingResult> r;
```

因为默认状态没有自然业务语义。

调用方必须通过：

```text
success(...)
failure(...)
```

创建。

---

# 10. Result<T> 不提供任意字段组合构造

不推荐公开：

```cpp
Result(
    ServiceError error,
    std::string message,
    std::optional<T> value
);
```

因为它允许构造非法组合。

---

# 11. Result<T> 工厂方法

推荐概念：

```cpp
static Result<T> success(T value);

static Result<T> failure(
    ServiceError error,
    std::string message
);
```

成功：

```cpp
return Result<RankingResult>::success(
    std::move(ranking)
);
```

失败：

```cpp
return Result<RankingResult>::failure(
    ServiceError::NotFound,
    "当前学期不存在"
);
```

---

# 12. failure() 的 message 必须非空

当前吸收额外审计建议并正式冻结：

> **所有 failure(...) 必须提供非空、面向人类可读的原因说明。**

例如：

```text
NotFound
"目标志愿记录不存在"

PermissionDenied
"当前账号无权执行审核"

PersistenceFailure
"持久化写入失败，本次操作未提交"
```

禁止：

```cpp
failure(ServiceError::NotFound, "");
```

---

# 13. success() 的 message 可以为空

成功结果通常：

```text
不需要 Service 提供 UI 文案
```

因此：

```text
success message
→ 可空
```

例如：

```cpp
OperationResult::success();
```

Presentation 可以自己显示：

```text
"修改成功"
```

---

# 14. 程序逻辑禁止依赖 message 文本

禁止：

```cpp
if (result.message().find("权限") != std::string::npos) {
    ...
}
```

程序逻辑只能依赖：

```cpp
result.error()
```

message 只是：

```text
调试
Console 输出
Qt 用户提示
诊断补充
```

---

# 15. Result<T> 的状态查询接口

统一保留：

```cpp
bool isSuccess() const;
bool isFailure() const;
```

推荐实现：

```cpp
bool isSuccess() const {
    return error_ == ServiceError::None;
}
```

```cpp
bool isFailure() const {
    return !isSuccess();
}
```

---

# 16. 当前暂不提供 has_value()

虽然：

```cpp
has_value()
```

可以作为 `isSuccess()` 别名，但当前不加入。

原因：

```text
Result<T> 不是单纯 optional<T>
它还承载 ServiceError 和 message
```

如果同时存在：

```text
isSuccess()
has_value()
operator bool()
```

会产生多套调用风格。

当前统一：

> **只用 `isSuccess()` / `isFailure()` 判断状态。**

---

# 17. 当前暂不提供 operator bool

禁止提前增加：

```cpp
explicit operator bool() const;
```

避免代码中同时出现：

```cpp
if (result)
if (result.isSuccess())
```

当前保持一种风格更清楚。

---

# 18. Result<T>::value() 的行为

推荐：

```cpp
const T& value() const;
```

必要时实际实现可再补：

```cpp
T& value();
```

但前置条件统一：

> **只有成功 Result 才允许调用 `value()`。**

---

# 19. 失败 Result 调用 value() 的正式行为

当前吸收审计建议：

> **统一直接通过 `std::optional::value()` 暴露错误。**

概念：

```cpp
const T& value() const {
    return value_.value();
}
```

如果失败：

```text
value_ == nullopt
```

则抛：

```text
std::bad_optional_access
```

---

# 20. 为什么选择 std::bad_optional_access 而不是 assert

`assert` 在 Release 构建中可能被移除。

而：

```text
失败 Result 调 value()
```

本身就是程序调用错误。

统一使用：

```text
std::optional::value()
```

有几个好处：

```text
行为稳定
Debug / Release 一致
错误尽早暴露
不返回默认值掩盖 bug
无需自造异常类型
```

---

# 21. 禁止 value() 在失败时返回默认 T{}

禁止：

```cpp
if (!value_) {
    return T{};
}
```

因为这会把：

```text
failure
```

伪装成：

```text
合法空业务对象
```

从而掩盖调用方 bug。

---

# 22. OperationResult 的最终结构

概念：

```cpp
class OperationResult {
private:
    ServiceError error_;
    std::string message_;

public:
    static OperationResult success(
        std::string message = {}
    );

    static OperationResult failure(
        ServiceError error,
        std::string message
    );

    bool isSuccess() const;
    bool isFailure() const;

    ServiceError error() const;
    const std::string& message() const;
};
```

---

# 23. OperationResult 成功/失败不变量

成功：

```text
error == None
```

失败：

```text
error != None
message 非空
```

同样不提供 public 默认构造。

---

# 24. 为什么不用 Result<void>

当前不采用：

```cpp
Result<void>
```

原因：

```text
实现语义更绕
课程项目没有收益
OperationResult 更直观
```

---

# 25. 为什么不做 ResultBase 深继承

不设计：

```text
ResultBase
├── Result<T>
└── OperationResult
```

因为只有少量：

```text
error
message
```

重复。

当前优先：

```text
清晰
少继承
易解释
```

---

# 26. 禁止 Result<OperationResult>

如果没有成功业务值：

```text
直接 OperationResult
```

如果有成功业务值：

```text
Result<T>
```

因此禁止：

```cpp
Result<OperationResult>
```

---

# 27. query 型 Service 的返回

例如：

```text
login()
→ Result<AuthenticatedUserInfo>

queryStatistics()
→ Result<StatisticsResult>

queryRanking()
→ Result<RankingResult>

queryDiaryWall()
→ Result<vector<DiaryPostPublicView>>
```

---

# 28. command 型 Service 的返回

如果成功不需要业务值：

```text
approve()
reject()
withdraw()
deleteRecord()
disableStudent()
changePassword()
likePost()
```

通常：

```text
→ OperationResult
```

---

# 29. 修改操作如果需要返回新业务值

例如：

```text
createVolunteerRecord()
```

如果成功后调用方需要：

```text
recordId
```

则：

```text
Result<CreateVolunteerRecordResult>
```

或：

```text
Result<std::string>
```

而不是：

```text
OperationResult
+
out parameter
```

---

# 30. 禁止 out-parameter 返回成功业务值

不推荐：

```cpp
OperationResult createRecord(
    ...,
    std::string& outRecordId
);
```

因为失败时：

```text
outRecordId
```

状态容易歧义。

---

# 31. ServiceError 与 message 的职责

例如：

```text
BusinessRuleViolation
```

可以对应多个 message：

```text
"当前账号已被禁用"
"该类别当前不可用"
"该记录已审核，不能再次提交"
"不能重复点赞"
```

程序只需要知道：

```text
这是业务规则失败
```

具体 UI 文本从 message 获取。

---

# 32. Expected Failure

以下属于正常运行中可预期：

```text
输入无效
权限不足
目标不存在
状态不允许
普通持久化失败
```

这些都：

```text
return Result / OperationResult
```

不抛异常。

---

# 33. Programming Error / Broken Invariant

例如：

```text
Approved VolunteerRecord
却没有 finalScore

Result success
却没有 value

switch 遇到理论不可能 enum

Repository Ready 后
出现重复 stable ID
```

这些说明：

```text
程序逻辑已被破坏
```

不应该：

```text
return BusinessRuleViolation
```

然后继续运行。

---

# 34. Fatal Path 的原则

当前不建立复杂 Exception Framework。

建议：

```text
expected failure
→ Result

programming invariant failure
→ std::logic_error / equivalent fatal

unrecoverable technical startup failure
→ std::runtime_error / startup failure carrier
```

具体异常类是否自定义，可在真实代码时决定。

---

# 35. 为什么不为每种 fatal 都造异常类

当前不需要：

```text
InvalidDomainInvariantException
DuplicateRepositoryIdException
BrokenSemesterReferenceException
...
```

课程项目会增加大量无价值样板代码。

只要：

```text
异常消息足够明确
边界清晰
上层统一捕获
```

即可。

---

# 36. Startup Load Failure 不属于 ServiceError

启动阶段：

```text
Service 尚未进入 Ready
```

因此以下错误：

```text
schema_version 不支持
CSV 损坏
duplicate ID
strong reference 断裂
required file 缺失
config 损坏
```

不应包装成：

```text
BusinessRuleViolation
PersistenceFailure
```

它们属于：

```text
Application startup failure
```

---

# 37. 启动失败处理

概念：

```text
main / Application bootstrap
↓
load config
↓
load repositories
↓
integrity validation
↓
失败
↓
输出明确诊断
↓
不进入 Console / Qt Ready
```

不允许“部分加载后继续运行”。

---

# 38. Repository 是否统一返回 Result<T>

不。

Gate 3 已经冻结：

```text
Result<T>
```

只在业务边界使用。

Repository 的：

```text
findById()
```

可以根据真实实现返回：

```text
pointer
const pointer
optional-like reference
iterator-like
```

Service 再映射：

```text
missing
→ ServiceError::NotFound
```

---

# 39. 为什么避免 Repository Result 机械扩散

如果 Repository：

```text
Result<T>
```

Service 又想组合：

```text
Result<Result<T>>
```

就会导致错误层级混乱。

当前统一：

```text
Repository technical / lookup outcome
↓
Service
↓
单层 Result<T>
```

---

# 40. Persistence 底层技术结果

PersistenceCoordinator 可拥有内部技术状态，例如：

```text
Success
PrepareFailed
PartialCommit
```

Service 映射：

```text
PrepareFailed
→ PersistenceFailure

PartialCommit
→ SeverePartialCommit
```

Presentation 不需要知道：

```text
.tmp rename 的具体 stage
第几个 participant 失败
```

这些可写入 message / diagnostic。

---

# 41. SeverePartialCommit 的双重语义

当前 Service 调用需要返回：

```text
ServiceError::SeverePartialCommit
```

因为调用者需要知道当前操作失败原因。

但同时：

```text
Application state
→ fatal persistence state
```

因此它不是普通可恢复错误。

---

# 42. SeverePartialCommit 后的 Presentation

Console / Qt 应：

```text
显示严重持久化故障
↓
停止正常业务
↓
禁止后续普通写操作
↓
引导退出 / 重启
```

不能只是：

```text
"保存失败，请重试"
```

---

# 43. PersistenceFailure 的 Presentation

普通：

```text
PersistenceFailure
```

可以提示：

```text
保存失败，本次操作未提交
```

因为系统已经：

```text
恢复操作前状态
```

仍可继续。

---

# 44. Result<T> 的 move semantics

推荐：

```cpp
static Result<T> success(T value);
```

内部：

```cpp
value_(std::move(value))
```

调用：

```cpp
return Result<T>::success(std::move(localResult));
```

或依赖正常：

```text
RVO / NRVO
move construction
```

---

# 45. 当前不自己管理内存

禁止为了 Result：

```text
new
delete
raw owning pointer
custom heap
manual union
```

使用：

```text
std::optional<T>
std::string
普通值语义
```

即可。

---

# 46. 是否需要 success(const T&) / success(T&&)

当前不提前复杂化。

最简单：

```cpp
success(T value)
```

已足够。

如果真实代码出现大对象复制瓶颈，再结合编译器行为调整。

---

# 47. message 与 UI 文案边界

失败 message 可以直接供：

```text
Console
Qt
```

显示。

但 Service 不应该承担所有界面文案。

例如：

```text
OperationResult::success()
```

可以没有 message。

Qt 自己显示：

```text
"修改成功"
```

---

# 48. Result 不携带 Qt 类型

禁止：

```text
QMessageBox::Icon
QColor
QString-only contract
UISeverity
```

ServiceError 与 Result 必须保持：

```text
pure C++
```

Presentation 自己映射。

---

# 49. Console 与 Qt 的统一使用方式

Console：

```cpp
auto result = service.queryRanking(...);

if (result.isFailure()) {
    std::cout << result.message();
    return;
}

show(result.value());
```

Qt：

```cpp
auto result = service.queryRanking(...);

if (result.isFailure()) {
    handleServiceError(result.error(), result.message());
    return;
}

render(result.value());
```

同一套 Service。

---

# 50. Result 核心不变量必须写入代码注释

当前吸收审计建议：

在模板定义附近至少明确：

```cpp
// Result<T> invariants:
// 1. Success: error == ServiceError::None && value.has_value()
// 2. Failure: error != ServiceError::None && !value.has_value()
// 3. failure() requires a non-empty human-readable message
// 4. value() may only be called on success;
//    otherwise std::bad_optional_access is thrown
```

---

# 51. OperationResult 注释

至少：

```cpp
// OperationResult invariant:
// success iff error == ServiceError::None.
// failure() requires a non-empty human-readable message.
```

---

# 52. 为什么必须文档化不变量

这套 Result 的可靠性来自：

```text
状态组合被严格限制
```

如果后续 AI / 开发者随意加入：

```text
public constructor
default constructor
nullable success
empty failure message
```

整个设计会退化。

所以不变量必须：

```text
Gate 4 文档
+
代码注释
```

两处都保留。

---

# 53. 当前禁止实现清单

后续 AI / Codex 不得擅自采用：

```text
Result 同时保存 bool success_ 和 ServiceError
Result public default constructor
Result 任意 error/value 组合构造
成功 Result 没 value
失败 Result 带 value
失败 value() 返回 T{}
失败 value() 有时 assert、有时异常
failure message 默认为空
程序逻辑解析 message 文本
operator bool 与 isSuccess 多套风格并存
has_value 与 isSuccess 多套状态语义
Result<void>
ResultBase 深继承
Result<OperationResult>
Result<Result<T>>
command + out parameter 返回成功值

所有 Repository 都返回 Result
所有纯 getter 都返回 Result
expected validation failure 直接 throw
broken invariant 伪装 BusinessRuleViolation
startup failure 硬塞成 ServiceError

SeverePartialCommit 当普通 PersistenceFailure
SeverePartialCommit 后继续正常写业务
Result 携带 Qt 显示类型
```

除非后续显式修订。

---

# 54. 当前实现级候选决策

## G4-RESULT-01 — ServiceError 最终分类

> **`ServiceError` 统一采用强类型枚举：`None / ValidationFailed / BusinessRuleViolation / PermissionDenied / NotFound / PersistenceFailure / SeverePartialCommit`。`None` 唯一表示成功，其余值表示失败；具体业务原因由非空失败 message 补充，不继续把每一条业务规则扩展为全局错误枚举。**

---

## G4-RESULT-02 — Result<T> 内部结构与不变量

> **`Result<T>` 内部保存 `ServiceError + std::string message + std::optional<T> value`，不额外保存独立 success bool。成功必须满足 `error == None && value.has_value()`；失败必须满足 `error != None && !value.has_value()`。**

---

## G4-RESULT-03 — Result<T> 构造方式

> **`Result<T>` 不提供 public 默认构造或任意字段组合构造，统一通过 `success(T)` 与 `failure(ServiceError, message)` 工厂方法创建，避免构造非法状态。**

---

## G4-RESULT-04 — OperationResult

> **无成功业务值的命令型 Service 使用独立非模板 `OperationResult`，其状态由 `ServiceError + message` 表示，成功为 `error == None`。不实现 `Result<void>`、ResultBase 深继承体系或 `Result<OperationResult>`。**

---

## G4-RESULT-05 — 修改型操作返回值

> **需要返回新 ID、创建结果或其他成功业务值的修改操作仍使用 `Result<T>`，不使用 `OperationResult + out-parameter`。决定使用 Result 还是 OperationResult 的依据是“成功时是否有业务返回值”，而不是该操作是否修改数据。**

---

## G4-RESULT-06 — Expected Failure 与 Fatal Path

> **Validation、Business Rule、Permission、NotFound 与普通 PersistenceFailure 属于 expected runtime failure，通过 Result / OperationResult 返回；程序不变量破坏、逻辑不可能状态、Ready 后权威内存状态损坏等走 exceptional / fatal path，不伪装成普通业务错误继续运行。**

---

## G4-RESULT-07 — SeverePartialCommit

> **`SeverePartialCommit` 通过当前 Service Result 返回给调用者用于错误展示，同时必须把 Application 标记为 fatal persistence state；它不是可恢复普通业务失败，Presentation 不得在此后继续允许普通写业务。**

---

## G4-RESULT-08 — Result 使用范围

> **Result 只存在于 Application / Service 业务边界，不机械扩散到所有 Repository、Domain getter、纯计算函数和 Qt helper。底层 Repository / Persistence outcome 由 Service 映射成一层最终 Result，禁止 `Result<Result<T>>`。**

---

## G4-RESULT-09 — 值语义与移动

> **`Result<T>` 使用 `std::optional<T>`、普通值语义、RVO/NRVO 和 move construction 管理成功值，不自行实现裸堆内存、手工 union 或自定义内存管理。**

---

## G4-RESULT-10 — value() 失败行为

> **`Result<T>::value()` 只允许在成功结果上调用；失败结果调用时统一通过内部 `std::optional::value()` 抛出 `std::bad_optional_access`。不得返回默认值，也不得同时以 assert 作为另一套正式行为。**

---

## G4-RESULT-11 — failure message

> **所有 `failure(...)` 必须携带非空、面向人类可读的原因说明；成功结果的 message 可以为空。程序判断逻辑只能依赖 `ServiceError`，不得解析 message 文本决定控制流。**

---

## G4-RESULT-12 — API 风格与不变量文档化

> **`Result<T>` / `OperationResult` 的核心不变量必须同时记录在 Gate 4 文档和对应代码注释中。当前只提供 `isSuccess() / isFailure()` 作为状态查询，不增加 `has_value()` 或 `operator bool`，避免形成多套调用风格。**

---

# 55. 后续实现待确认事项

以下内容当前不提前死锁：

```text
Result<T> 最终头文件路径
ServiceError 是否单独一个头文件
value() 是否同时提供 mutable overload
message() 是否返回 string_view
failure() 对空 message 是 throw / logic_error / assert / constructor guard
fatal persistence state 最终保存在哪个 ApplicationContext
startup failure 使用 exception 还是 dedicated bootstrap result
Repository findById 最终返回 pointer / optional-like handle
PersistenceCoordinator internal status 精确枚举
Qt error mapping helper 最终位置
```

这些应结合真实代码在对应 STOP 点讨论。

---

# 56. 给后续 AI / Codex 的最短读取指令

如果上下文空间不足，只需读取本节。

```text
当前 Result / Error 路线：

1. ServiceError:
   None
   ValidationFailed
   BusinessRuleViolation
   PermissionDenied
   NotFound
   PersistenceFailure
   SeverePartialCommit

2. None 唯一代表成功。

3. Result<T>:
   ServiceError error
   string message
   optional<T> value

4. 成功不变量：
   error == None
   value exists

5. 失败不变量：
   error != None
   value absent
   message 非空

6. Result<T> 不允许 public default constructor。
7. 不允许任意 error/value 组合构造。
8. 只通过：
   success(T)
   failure(error, message)

9. value()：
   只能成功时调用。
   失败调用时统一通过 optional::value()
   抛 std::bad_optional_access。
   不返回默认值。
   不混用 assert 作为另一套正式行为。

10. 当前只提供：
    isSuccess()
    isFailure()
    不提供 has_value()
    不提供 operator bool

11. 无成功业务值：
    OperationResult

12. 有成功业务值：
    Result<T>

13. 不使用：
    Result<void>
    ResultBase 深继承
    Result<OperationResult>
    Result<Result<T>>
    OperationResult + out parameter 返回业务值

14. Expected failures：
    validation
    business rule
    permission
    not found
    recoverable persistence
    → Result

15. Broken invariant / impossible state：
    → exceptional / fatal path

16. Startup load failure：
    不属于 ServiceError；
    bootstrap 失败后不进入 Ready。

17. Persistence mapping：
    PrepareFailed
    → PersistenceFailure

    PartialCommit
    → SeverePartialCommit

18. SeverePartialCommit：
    当前调用返回 failure
    同时 Application 进入 fatal persistence state
    后续禁止普通写业务。

19. Result 只用于 Service/Application boundary。
20. Repository / Domain / helper 不机械返回 Result。
21. 程序逻辑看 ServiceError，不解析 message。
22. 核心不变量必须写进代码注释。
```

---

# 57. 当前状态建议

```text
Gate 4.6
Result<T>、OperationResult、ServiceError 与 Fatal Path

G4-RESULT-01 ～ G4-RESULT-12

→ CANDIDATE FROZEN
```

在实际 `Result<T>` 模板、`OperationResult`、`ServiceError`、Service 错误映射、startup failure 与 Qt / Console 消费方式编码完成后，再进行 Gate 4.6 FINAL AUDIT。
