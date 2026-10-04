# Gate 3.7 — 自定义类模板终审

> 状态：**FINAL PASS**。
>
> 下方正文逐行迁移自 v2.4 第 13889～14523 行；保留 Result<T> 的架构图、返回值流程、模板边界和最终审计。D252～D258 均保留在原上下文中。

> **Gate 4 实现细化导航**
>
> 本文冻结的是 Gate 3 架构层语义；对应实现级规则已在 Gate 4 进一步细化。
>
> 后续 AI / Codex / 开发者在实现或审计本文内容前，应先读取：
>
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> Gate 4 负责“具体如何实现”，不得反向修改本文已经冻结的 Gate 3 架构。若真实实现发现冲突，应按 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 中的 STOP 规则暂停并显式 reopen，不得自行覆盖。

## 完整迁移正文

# 61. Gate 3.7：自定义类模板终审

> 状态：**FINAL PASS**  
> 冻结范围：D252～D258  
> 核心结论：V1.0 只正式采用一个核心自定义类模板——`Result<T>`。不再为了模板数量额外设计 Generic Repository、自定义 STL 容器、通用 TableModel 或 Export 模板。

---

## 61.1 模板准入标准

一个模板只有在存在以下真实结构时才采用：

```text
结构/行为稳定
        +
数据类型发生变化
        ↓
编译期泛型能够消除真实重复
```

必须同时满足：

```text
真实泛型需求
不破坏 Service / Repository / Qt 分层
不与 Strategy 重复
有明确数据成员 / 职责
可测试
可答辩
复杂度成本可控
```

---

## 61.2 候选模板扫描与淘汰

| 候选 | 结论 | 原因 |
|---|---|---|
| `Result<T>` | 保留 | 多个 Service 共享相同失败外壳，但成功值类型不同 |
| Generic Repository<T> | 拒绝 | 破坏领域语义，与 D94/D242 冲突 |
| Generic Qt TableModel<T> | 拒绝 | 只有两个 Model，列/格式/role 差异大 |
| 自定义 MyVector<T> / List<T> | 拒绝 | STL 足够，业务系统不应为模板重造容器 |
| Generic Paginator<T> | 拒绝 | 当前不做全局分页 |
| ExportService<T> | 拒绝 | 与运行时 Strategy 重复解决同一变化维度 |
| Generic Sort<T> | 拒绝 | STL 已提供；业务排序规则固定 |
| Optional<T> | 拒绝 | STL 已提供 `std::optional` |
| Generic Cache<T> | 拒绝 | 当前无缓存需求 |

最终：

> **只有 `Result<T>` 具有最清晰的跨 Service 泛型价值。**

---

## 61.3 Result<T> 的真实泛型需求

多个 Application/Service 调用具有共同外壳：

```text
成功 / 失败
错误分类
错误补充说明
成功业务结果
```

但成功结果类型不同：

```text
AuthenticatedUserInfo
RankingResult
DiaryPostPublicView
StudentProfileView
StatisticsResult
...
```

因此：

```text
固定部分
→ result/error/message semantics

变化部分
→ T
```

这是自然的类模板问题。

### 决策 D252

> **V1.0 引入轻量通用 `Result<T>` 类模板，用于 Application/Service 边界中“可能失败且成功时需要返回不同强类型结果”的查询或用例。模板复用统一的调用结果外壳，而不复制各业务结果类型。**

概念：

```text
Result<T>
├── success/status semantics
├── ServiceError
├── message
└── value : T / optional T
```

精确成员形式留 Gate 4。

---

## 61.4 Result<T> 的使用边界

### 决策 D253

> **`Result<T>` 只用于确有失败语义的 Application/Service 边界，不机械扩散到 Domain 纯计算、普通 getter、简单 bool 查询、Repository 所有基础方法或 Qt 辅助函数。**

不应该出现：

```text
Result<bool> isActive()
Result<double> calculateScore()
Result<int> compareRank()
Result<User*> for every repository find
```

模板的目标不是统一项目所有返回值。

---

## 61.5 无成功业务值：OperationResult

### 决策 D254

> **无成功业务返回值的修改型操作，当前优先采用非模板 `OperationResult` / 等价轻量结果类型，不为了模板展示强制设计复杂 `Result<void>` 特化。精确实现形式留 Gate 4。**

例如：

```text
approve(...)
reject(...)
deleteRecord(...)
disableStudent(...)
```

概念上：

```text
OperationResult
├── ServiceError
└── message
```

而：

```text
queryRanking(...)
→ Result<RankingResult>
```

这样比为了技术展示硬做 `Result<void>` specialization 更清晰。

---

## 61.6 Result<T> 必须保持纯 C++

### 决策 D255

> **`Result<T>` 的错误语义必须复用已冻结 Service/Application Error 模型，并保持纯 C++。不得携带 `QString`、`QColor`、`QIcon`、`QMessageBox` 类型或任何 Presentation 级显示指令。**

正确链路：

```text
ServiceError
        │
        ▼
Qt Presentation mapping
        │
        ├── inline validation
        ├── warning
        ├── error state
        └── severe failure UI
```

Service 不返回：

```text
QMessageBox::Critical
red color
icon
UI severity widget
```

---

## 61.7 不为了模板重新打开已冻结架构

### 决策 D256

> **自定义模板不扩展到 Generic Repository、自定义 STL 替代容器、通用 Qt TableModel、Export Strategy 等已经存在更合理解决方案的区域；模板必须解决独立、真实、可解释的泛型需求。**

因此禁止为了“第二个模板”而引入：

```text
Repository<T>
TableModel<T>
MyVector<T>
Exporter<T>
GenericPaginator<T>
```

---

## 61.8 Result<T> 只包装 View Result，不取代它

这是本阶段最关键的边界之一。

### 决策 D257

> **`Result<T>` 中的 `T` 必须是独立、语义完整的 Service View Result、Query Result 或其他成功结果类型。`Result<T>` 只包装“成功/失败 + 错误信息 + 可选成功值”，不得直接吸收 `T` 的业务字段，也不得取代 `DiaryPostPublicView`、`RankingResult`、`StudentSummary` 等原有结果结构。**

正确：

```text
Result<DiaryPostPublicView>
│
├── error/status
├── message
└── value
      │
      ▼
DiaryPostPublicView
├── postId
├── title
├── categoryName
├── likeCount
└── ...
```

错误：

```text
Result
├── error
├── message
├── postId
├── title
├── categoryName
├── likeCount
└── ...
```

因此：

```text
Result<T>
= invocation result wrapper

T
= successful business/query result
```

---

## 61.9 统一强类型 ServiceError

### 决策 D258

> **`Result<T>` / `OperationResult` 必须使用统一强类型错误枚举表达失败类别，不允许仅依赖自由文本 message 判断错误类型。错误分类至少覆盖 Business Rule、Validation、Permission、NotFound、普通 Persistence Failure、Severe Partial Commit 等既有错误语义；message 只承担补充说明和用户上下文职责。**

概念错误模型：

```text
ServiceError
├── BusinessRuleViolation
├── ValidationFailed
├── PermissionDenied
├── NotFound
├── PersistenceFailure
└── SeverePartialCommit
```

是否额外拥有：

```text
None
```

以及最终精确名称，留 Gate 4。

程序分支：

```text
ServiceError
→ machine-readable semantics
```

而不是：

```text
if message contains "权限"
```

---

> **实现细化 → Gate 4**
>
> 本节冻结的是 `Result<T>`、`OperationResult` 与统一 `ServiceError` 的架构角色和使用边界；对应数据结构、不变量、`value()` 行为与 Fatal Path 的实现协议已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.6_Result_OperationResult_ServiceError与FatalPath实现细化_候选冻结稿(1).md`

## 61.10 Result<T> / View Result 架构图

```text
                  Application / Service
                          │
                          ▼
                      Result<T>
                          │
             ┌────────────┼────────────┐
             │            │            │
             ▼            ▼            ▼
       success/error    message      optional value
                                      │
                                      ▼
                                      T
                         ┌────────────┼─────────────┐
                         ▼            ▼             ▼
                  RankingResult  DiaryPostPublicView StudentProfileView
```

职责边界：

```text
Result<T>
→ 本次调用是否成功

T
→ 成功以后得到了什么业务结果
```

---

## 61.11 Service 返回值流程图

```text
Qt Page / Application caller
          │
          ▼
       Service
          │
          ├── validate
          ├── authorize
          ├── query / execute business
          ├── persistence if needed
          │
          ▼
     expected outcome?
       ┌───────┴────────┐
       │                │
     Success          Expected failure
       │                │
       ▼                ▼
 construct T       map ServiceError
       │                │
       ▼                ▼
  Result<T>          Result<T>
   with value         no value
       │                │
       └───────┬────────┘
               ▼
         Presentation
```

程序不变量破坏不被普通 Result 吞掉：

```text
broken invariant / programming error
→ exceptional / fatal path
```

具体异常策略留 Gate 4。

---

## 61.12 OperationResult 与 Result<T> 的边界

```text
Application / Service
          │
    ┌─────┴─────┐
    │           │
query/value   command/no value
    │           │
    ▼           ▼
Result<T>  OperationResult
```

允许两个类型存在少量字段重复。

当前不为了极致 DRY 强行建立：

```text
ResultBase
Result<void> specialization
deep result hierarchy
```

原则：

> **清楚优先于消除少量重复。**

---

## 61.13 禁止 Result 套娃

Gate 4 实现必须检查：

```text
Result<Result<T>>
Result<OperationResult>
```

等无独立语义的嵌套包装。

推荐：

```text
lower-level technical outcome
        │
        ▼
Service interprets/maps
        │
        ▼
one Application Result<T>
```

每个 Application/Service 边界只形成一层最终结果外壳。

---

## 61.14 Result 与异常 / fatal path 的边界

预期业务失败：

```text
ValidationFailed
PermissionDenied
NotFound
BusinessRuleViolation
normal PersistenceFailure
```

→ `Result<T>` / `OperationResult`

系统不变量或编程错误：

```text
currentSemesterId points to missing Semester
Approved record has no finalScore
duplicate stable ID in authoritative collection
impossible enum/state combination
```

→ exceptional / fatal handling

不能把所有系统错误都包装成：

```text
BusinessRuleViolation
```

后继续运行。

---

## 61.15 move semantics 实现注意事项

属于 Gate 4，不形成新架构决策：

- `Result<T>` 成功值应正常支持 move semantics；
- `T` 包含 `std::vector` / `std::string` 等较大值成员时避免显式不必要 copy；
- 优先依赖 C++ 值语义、RVO / NRVO 与正常 move construction；
- 不机械在所有 `return local;` 上写 `std::move(local)`；
- 不为 Result 自行管理裸堆内存。

---

## 61.16 为什么不直接用 std::expected 作为唯一答案

现代 C++ 存在 `std::expected<T,E>`，但当前自定义 `Result<T>` 仍具有项目与教学价值：

```text
统一项目自己的 ServiceError
+
补充 message
+
明确 Application/Service result semantics
+
自然体现自定义类模板
```

因此报告不应宣称“标准库没有类似能力”。

正确说明：

> **为统一本项目 Service 边界的业务错误分类与成功结果包装，并自然实践强类型类模板，采用一个轻量 `Result<T>`。**

---

## 61.17 Result<T> 与 Strategy 的边界

两者解决不同变化维度：

```text
Result<T>
→ compile-time generic programming
→ 成功结果的数据类型 T 变化
→ 外壳结构稳定

Export Strategy
→ runtime polymorphism
→ 输出格式算法变化
→ 调用场景稳定
```

因此没有重复：

```text
模板
≠ Strategy

compile-time type abstraction
≠ runtime algorithm substitution
```

---

## 61.18 模板 vs 运行时多态答辩图

```text
                 “什么在变化？”
                       │
          ┌────────────┴────────────┐
          │                         │
          ▼                         ▼
       数据类型                  运行时算法/行为
          │                         │
          ▼                         ▼
     class template             virtual polymorphism
          │                         │
          ▼                         ▼
      Result<T>             ExportFormatStrategy
                                  │
                         ┌────────┴────────┐
                         ▼                 ▼
                       CSV              Markdown
```

此外：

```text
User::capabilities()
→ 运行时对象类型差异
→ User / Student / Administrator 多态
```

这使项目可以清楚解释模板、继承、多态与 Strategy 的适用边界。

---

## 61.19 Gate 3.7 FINAL AUDIT

| 审计项 | 结论 |
|---|---|
| 存在真实泛型需求 | PASS |
| Result 与 View Result 边界 | PASS |
| Result 不取代业务结果 | PASS |
| 统一 ServiceError | PASS |
| message 不承担错误码职责 | PASS |
| Qt 类型未侵入 | PASS |
| 主要位于 Application/Service 边界 | PASS |
| 不向全部 Repository 扩散 | PASS |
| 不向 Domain 纯函数扩散 | PASS |
| OperationResult 边界合理 | PASS |
| 不强制 Result<void> 特化 | PASS |
| 禁止 Result 套娃 | PASS |
| Result / fatal path 边界明确 | PASS |
| move semantics 可自然支持 | PASS |
| 类具有真实数据成员 | PASS |
| 不重造 STL 容器 | PASS |
| 不做 Generic Repository | PASS |
| 不做通用 TableModel 模板 | PASS |
| 不与 Export Strategy 重叠 | PASS |
| 第二模板无真实必要性 | PASS |
| 课程评分价值 | PASS |
| 答辩解释价值 | PASS |
| 复杂度成本可控 | PASS |

最终：

```text
Gate 3.7
Custom Class Template Audit
→ D252～D258
→ FINAL PASS
```

正式结论：

> **V1.0 只正式采用一个核心自定义类模板：`Result<T>`。**

---

## 61.20 Gate 3.6～3.7 后的技术结构总览

```text
OOP inheritance
→ User → Student / Administrator

Runtime polymorphism
→ User::capabilities()
→ Export Strategy

Generic programming
→ Result<T>

Persistence abstraction
→ Repository Pattern

Application use cases
→ Service Layer

Object wiring
→ Composition Root

Advanced Qt presentation
→ Qt Model/View + Proxy
→ custom DiaryPostCard feed
```

所有技术点均对应真实职责，不以“技术数量”作为目标。
