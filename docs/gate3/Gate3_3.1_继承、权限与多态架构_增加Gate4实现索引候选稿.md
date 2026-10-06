# Gate 3.1 — 继承、权限与多态架构

> 状态：**PASS**。
>
> 下方正文逐行迁移自 v2.4 第 78～1595 行；其中包含继承结构、字段边界、权限清单、文本架构图、设计理由和实现提醒。D1～D21 均保留在原上下文中。旧阶段状态与旧索引已移入历史档案，避免与当前 PASS 混读。

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

# 4. Gate 3.1：User / Student / Administrator 继承体系

## 4.1 最终继承方向

正式冻结：

```text
              User
         <<abstract>>
           /       \
      Student   Administrator
```

### 决策 D1

> **最终 V1.0 采用 `User → Student / Administrator` 的继承方向。**

理由：

- `Student is a User`
- `Administrator is a User`
- 两者共享稳定的账号身份数据；
- 两者存在真实业务能力差异；
- 继承关系符合 `is-a` 语义；
- 能够自然形成至少一层有意义继承。

---

## 4.2 User 定位

### 决策 D2

> **`User` 倾向设计为抽象基类，不作为独立业务角色直接实例化。**

系统实际运行角色是：

- `Student`
- `Administrator`

因此原则上不需要独立存在：

```cpp
User user;
```

`User` 的职责是统一：

- 公共身份；
- 公共账号状态；
- 公共账号行为；
- 角色基础能力抽象。

---

# 5. User 的公共持久数据

当前冻结：

```text
User <<abstract>>
├── accountId
├── name
├── password-related data
└── accountStatus
```

## 5.1 accountId

### 决策 D4

统一使用：

```text
accountId
```

而不是在基类层分别区分：

- `studentId`
- `adminId`

业务语义：

> **用于唯一标识并登录当前用户账号的统一账号标识。**

具体值：

```text
Student.accountId
= 学号

Administrator.accountId
= 管理员账号
```

### 登录身份判定原则

不通过账号长度推断角色。

不采用：

```text
长度 X → Student
长度 Y → Administrator
```

最终方向应是：

```text
输入 accountId + password
↓
在真实账号数据中查找
↓
获得对应实际对象
↓
以 User 基类接口统一接收
↓
运行时实际对象为 Student / Administrator
```

---

## 5.2 name

正式冻结：

```text
name
```

上移至 `User`。

原因：

- Student 和 Administrator 都有姓名；
- 字段业务语义一致；
- 不需要派生类重复保存。

---

# 6. password-related data

### 决策 D5

> **密码认证相关数据上移至 `User`，由 `User` 统一维护公共密码认证逻辑。**

当前不冻结最终具体字段形式。

可能的技术实现仍待 Gate 3 / Gate 4 后续审计，例如：

```text
passwordHash
salt
其他密码认证表示
```

但当前已冻结以下原则：

1. Student 与 Administrator 的密码认证机制属于共同账号能力；
2. 不需要在两个派生类中重复实现密码验证；
3. V1.0 不直接持久化明文密码；
4. 具体 hash / salt / 算法实现后续再决定；
5. 密码认证方式不应为了 Qt 而侵入纯业务对象。

### 当前倾向属于 User 的普通公共行为

概念上包括：

```text
验证密码
修改自身密码
更新密码认证数据
```

这些行为在两类用户之间没有本质差异，因此：

> **不为了多态而强行设为虚函数。**

---

# 7. accountStatus

### 决策 D6

> **`accountStatus` 上移至 `User`，采用语义明确的状态枚举，而不是简单 bool。**

当前状态语义至少包括：

```text
AccountStatus
├── Active
└── Disabled
```

不建议：

```cpp
bool status;
```

### accountStatus 的角色

`accountStatus` 是：

> **账号层面的全局前置条件**

它不等于角色权限。

例如：

```text
Student
角色上具有 SubmitVolunteerRecord 能力

但：
accountStatus == Disabled

→ 当前不能提交
```

因此具体操作是否允许应综合：

```text
角色基础能力
+
账号状态
+
目标对象归属
+
目标对象状态
+
具体业务规则
```

---

# 8. Student 专属身份数据

当前冻结：

```text
Student : User
├── className
├── major
├── contact
├── createdAt
└── lastLoginAt
```

## 8.1 className

表示学生所在班级。

保持 Student 专属，不上移 `User`。

## 8.2 major

表示学生专业。

保持 Student 专属，不上移 `User`。

## 8.3 contact

### 决策 D8

> **`contact` 为 Student 专属数据，不上移 `User`。**

## 8.4 createdAt

### 决策 D9

> **`createdAt` 保持 Student 专属。**

表示学生账号创建时间。

## 8.5 lastLoginAt

### 决策 D9

> **`lastLoginAt` 保持 Student 专属。**

表示学生最后登录时间。

---

# 9. Administrator 当前身份数据结构

当前冻结：

```text
Administrator : User
└── 暂不强行增加独有持久字段
```

Administrator 通过继承已经拥有：

```text
accountId
name
password-related data
accountStatus
```

因此一个 Administrator 对象本身并不是“没有状态的数据空壳”。

## 9.1 不增加无业务依据字段

### 决策 D7

不为了满足“派生类看起来有自己的字段”而人为加入：

```text
adminLevel
authorityLevel
department
managedDepartment
```

等当前需求中不存在的字段。

## 9.2 Administrator 的架构价值

Administrator 的价值主要体现在：

1. 管理员角色能力；
2. 审核 / 治理 / 配置行为的执行主体身份；
3. 作为 OperationLog 中管理行为的责任追溯主体；
4. 与 Student 形成真实角色多态。

Administrator 不应该变成：

> **所有系统管理逻辑全部堆叠的 God Class。**

复杂管理流程后续应由适当的业务服务协调。

---

# 10. User 多态设计方向

### 决策 D3

> **User 的核心多态价值用于表达 Student 与 Administrator 的基础角色能力差异。**

不把以下浅层接口作为主要多态证明：

```cpp
virtual UserRole getRoleType() const = 0;
```

`getRoleType()` 可以作为辅助信息存在，但如果系统仍然依赖大量：

```cpp
if (user->getRoleType() == ...)
```

来完成角色分支，则多态价值不足。

---

# 11. “角色基础能力”与“具体业务授权”分层

当前正式冻结：

> **角色能力和具体业务操作是否合法，不是同一个问题。**

## 11.1 第一层：角色基础能力

适合通过 User 抽象接口和运行时多态表达。

### Student 基础能力

概念上包括：

```text
提交志愿记录
管理自己的待审核记录
撤回自己的待审核记录
查看自己的记录
查看个人积分 / 排名 / 荣誉
申请日记墙展示
点赞 / 取消点赞
```

### Administrator 基础能力

概念上包括：

```text
审核志愿记录
治理异常记录
管理学生账号
管理志愿类别
管理积分规则
管理徽章规则
管理学期
审核 / 治理日记墙
查看操作日志
查看全局统计
执行数据导出
```

当前只冻结“能力类别和多态方向”，不冻结最终虚函数签名。

## 11.2 第二层：具体业务操作是否合法

不能只由 User / Student / Administrator 自己决定。

例如学生修改记录，不仅要求：

```text
Student 具备 ModifyOwnPendingRecord 能力
```

还要求：

```text
record.owner == currentStudent
+
record.status == Pending
```

学生申请日记墙还要求：

```text
记录属于本人
+
记录当前存在
+
记录状态 == Approved
+
同一记录尚未存在 DiaryPost
+
账号状态 == Active
```

管理员审核还需要：

```text
记录处于待审核状态
+
输入审核数据合法
+
类别 / 时长规则合法
```

因此：

```text
是否允许一次具体操作
=
角色基础能力
+
账号状态
+
对象归属
+
对象状态
+
业务规则
```

---

# 12. Gate 3.1 行为层、Permission 与 UserCapabilities 最终冻结

在 v1.0 中，本节仍写为“当前多态接口形式尚未冻结”。经过后续讨论，Gate 3.1 已完成该部分审计，现正式修订如下。

## 12.1 User 普通公共行为

### 决策 D10

`User` 中只保留 Student 与 Administrator 都具备、且实现逻辑一致的公共账号行为。

当前方向包括：

```text
读取 accountId
读取 name
读取 accountStatus
判断账号是否 Active
维护公共账号状态
```

这些行为不需要运行时多态。

原因：

> Student 与 Administrator 在这些行为上的实现没有角色差异，不应为了“体现 virtual”而重复覆写相同逻辑。

---

## 12.2 禁止机械 public setter

### 决策 D11

不为每个数据成员机械提供：

```cpp
setX(...)
```

尤其是：

```text
accountId
accountStatus
password-related data
```

必须受业务规则保护。

当前冻结：

- `accountId` 倾向于构造 / 创建账号时确定，后续只读；
- `accountStatus` 不通过通用 `setAccountStatus()` 任意修改；
- 密码认证数据不通过通用 `setPasswordData()` 直接暴露；
- 学生不能通过 setter 自行恢复被禁用账号；
- 业务状态变化必须通过有语义的操作表达。

---

## 12.3 accountStatus 的领域状态转换

### 决策 D14

账号状态修改采用：

```text
disable()
enable()
```

这类领域语义方法，而不是通用 setter。

职责划分：

```text
Service
→ 判断谁有权触发状态改变

User
→ 维护自身 Active / Disabled 状态合法转换
```

不采用：

```text
friend AdministratorService
setStatusInternal()
```

这类让领域对象反向依赖服务类的方案。

---

## 12.4 密码算法边界修订

### 决策 D13

v1.0 中曾将“验证密码 / 修改密码”整体理解为 `User` 公共行为。后续审计后，现精化为：

```text
User
→ 保存密码认证状态 / 认证结果数据
→ 维护与自身账号相关的密码状态

PasswordHasher / AuthenticationService
→ 负责具体哈希 / 密码验证算法
```

冻结原则：

1. V1.0 不明文持久化密码；
2. 具体哈希 / 验证算法不硬编码在 `User`；
3. 不让 `User` 持有复杂 `IPasswordHasher*` 依赖；
4. 不引入重型依赖注入框架；
5. V0.1 可以使用轻量工具函数或简单认证模块；
6. V1.0 保持“账号领域状态”和“密码算法实现”分离。

---

> **实现细化 → Gate 4**
>
> 本节冻结的是密码职责边界；对应实现级细节已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.1_认证、密码与账号实现细化_候选冻结稿(1).md`

## 12.5 User 的核心多态接口

### 决策 D12

正式冻结：

> **`User` 通过纯虚 `capabilities()` 接口表达 Student 与 Administrator 的基础角色能力差异。**

概念结构：

```text
User <<abstract>>
└── capabilities() = 0

Student
└── 返回学生能力集合

Administrator
└── 返回管理员能力集合
```

从 V0.1 开始即可设计为纯虚接口。

不为了制造版本演进而在 V0.1 先提供默认实现、V0.2 再改纯虚。

---

## 12.6 Permission

### 决策 D15

角色能力使用强类型枚举：

```cpp
enum class Permission
```

不使用：

- 魔法数字；
- 魔法字符串；
- 按钮名称作为权限；
- 页面名称作为权限。

原因：

- 类型安全；
- 语义明确；
- 易于 Qt 和 Service 共用；
- 易于答辩解释；
- 后续增加能力时结构清晰。

---

## 12.7 UserCapabilities

### 决策 D16

采用轻量值对象：

```text
UserCapabilities
└── std::set<Permission>
```

它只负责：

```text
保存角色能力集合
查询是否具有某项 Permission
提供必要只读访问
```

明确不负责：

- 账号是否 Disabled；
- 记录是不是本人；
- VolunteerRecord 当前是什么状态；
- 是否允许申请日记墙；
- 是否允许审核；
- 积分、排行、徽章等业务规则。

因此：

> **UserCapabilities 只是“角色静态能力集合”，不是权限 God Class。**

---

## 12.8 为什么优先 std::set<Permission>

当前优先：

```cpp
std::set<Permission>
```

而不是：

```text
std::vector<Permission>
std::unordered_set<Permission>
uint64_t 位掩码
```

原因：

- 权限天然具有集合语义；
- 自动去重；
- 当前权限数量很少；
- 没有性能瓶颈；
- `std::set` 更容易理解与解释；
- 不需要为了理论 O(1) 查询引入额外哈希细节。

当前不冻结静态缓存、返回 const 引用等微优化。

---

## 12.9 角色能力与具体业务授权分层

正式冻结：

```text
Permission / UserCapabilities
回答：
“这个角色原则上能做什么？”

Service + Domain
回答：
“当前这一次具体操作实际上能不能做？”
```

因此：

```text
一次具体操作是否合法
=
角色基础能力
+
账号状态
+
对象归属
+
对象状态
+
具体业务规则
```

例如：

```text
Student 有 ModifyOwnPendingRecord
+
record.owner == currentStudent
+
record.status == Pending

→ 才允许修改
```

再例如：

```text
Administrator 有 ReviewVolunteerRecord
+
record.status == Pending
+
审核输入合法

→ 才允许审核
```

---

## 12.10 Student Permission 正式清单

### 决策 D17 / D19

```text
Student
├── SubmitVolunteerRecord
├── ModifyOwnPendingRecord
├── WithdrawOwnPendingRecord
├── ViewOwnVolunteerRecords
├── ViewOwnProfile
├── EditOwnProfile
├── ViewOwnStatistics
├── ViewOwnBadges
├── RequestDiaryPost
└── LikeDiaryPost
```

### 权限语义补充

`EditOwnProfile` 只代表学生拥有“修改允许本人修改资料”的能力。

当前实际可修改：

```text
contact
password
```

不可修改：

```text
accountId / 学号
name
className
major
用户角色
```

`ViewOwnStatistics` 当前覆盖：

```text
月度积分
学期积分
总积分
当前个人排名
排行榜称号
```

`ViewOwnBadges` 单独保留，因为专项徽章与排行榜称号属于不同荣誉体系。

“被驳回记录修改后重新提交”不额外拆 Permission，而由志愿记录状态规则处理。

浏览日记墙公开内容和排行榜不单独设 `ViewPublicContent`，避免权限过细。

---

## 12.11 Administrator Permission 正式清单

### 决策 D18 / D20

```text
Administrator
├── ReviewVolunteerRecord
├── CorrectApprovedRecord
├── DeleteVolunteerRecord
├── ManageStudents
├── ManageVolunteerCategories
├── ManageBadgeRules
├── ManageSemester
├── ModerateDiaryPost
├── ViewOperationLog
├── ViewGlobalStatistics
└── ExportData
```

其中：

### ReviewVolunteerRecord

涵盖：

- 查看待审核记录；
- 审核通过；
- 审核驳回；
- 填写审核意见；
- 确认 / 修正最终类别；
- 确认 / 修正最终时长。

不再拆成 `ViewPendingRecords / ApproveRecord / RejectRecord`。

### CorrectApprovedRecord

单独保留，因为已通过记录强制更正属于高风险治理行为，需要完整留痕与衍生结果更新。

### DeleteVolunteerRecord

单独保留，因为物理删除会触发 DiaryPost / LikeRelation 联动删除、统计重算和 OperationLog 留痕。

### ManageStudents

统一涵盖：

```text
查询学生
创建学生账号
禁用学生账号
恢复学生账号
重置学生密码
维护允许管理员修改的学生身份信息
```

不拆成按钮级 Permission。

### ManageVolunteerCategories

涵盖类别查看、新增、系数修改、停用等稳定业务能力。

### ManageBadgeRules

涵盖专项徽章规则查看、新增、修改门槛、停用。

### ManageSemester

涵盖学期配置维护和当前学期切换。

### ModerateDiaryPost

涵盖展示申请审核、拒绝、已展示内容治理与下架。

### ViewOperationLog

只允许查看 / 查询，不改变 OperationLog “不可手动修改 / 删除”的规则。

### ViewGlobalStatistics

涵盖全局积分、排行榜、类别统计、总服务时长等。

### ExportData

只表示管理员拥有执行导出的角色能力，具体文件生成由后续导出服务负责。

---

## 12.12 Permission 粒度原则

### 决策 D21

> **Permission 表达稳定业务能力域，而不是按钮、页面、字段或单次 UI 动作。**

禁止：

```text
OpenReviewPage
ClickApproveButton
EditDurationField
ResetPasswordButton
ViewStudentName
```

Permission 不随着 Qt 页面布局变化而变化。

---

## 12.13 Qt 与 capabilities 的关系

最终 V1.0：

```text
登录成功
↓
获得 User
↓
user.capabilities()
↓
Qt Presentation
↓
根据能力集合决定入口 / 菜单 / 按钮是否显示或启用
```

例如：

```text
Student
→ 我的志愿
→ 个人资料
→ 积分 / 荣誉
→ 日记墙

Administrator
→ 审核
→ 用户管理
→ 规则配置
→ 日记墙治理
→ 日志
→ 全局统计
→ 导出
```

Qt 不需要大量：

```cpp
if (role == "admin")
```

但能力集合只控制角色入口。

真正业务执行仍由 Service 再次检查：

```text
Permission
+
accountStatus
+
对象归属
+
对象状态
+
业务规则
```

---

## 12.14 Gate 3.1 最终结构

```text
                  User <<abstract>>
                 /                 \
            Student           Administrator
                |                   |
        capabilities()       capabilities()
                \                   /
                 \                 /
                 UserCapabilities
                        |
                std::set<Permission>
```

`User`：

```text
User
├── accountId
├── name
├── password-related data
├── accountStatus
├── 公共账号行为
└── pure virtual capabilities()
```

`Student`：

```text
Student : User
├── className
├── major
├── contact
├── createdAt
├── lastLoginAt
└── Student Permission 集合
```

`Administrator`：

```text
Administrator : User
├── 继承公共账号数据
└── Administrator Permission 集合
```

复杂业务仍由后续 Service 层协调，避免 User / Administrator 变成 God Class。



# 13. 各领域对象字段冻结总表

以下表格综合 Gate 1、Gate 2 与当前 Gate 3 已确认结果。

## 13.1 User

| 字段 | 归属 | 说明 |
|---|---|---|
| `accountId` | User | 统一登录账号标识；Student 中对应学号，Administrator 中对应管理员账号 |
| `name` | User | 用户姓名 |
| password-related data | User | 密码认证信息，最终不明文持久化 |
| `accountStatus` | User | 账号状态，建议枚举 Active / Disabled |

## 13.2 Student

| 字段 | 归属 | 说明 |
|---|---|---|
| 继承 `accountId` | User | 学号 |
| 继承 `name` | User | 学生姓名 |
| 继承 password-related data | User | 学生账号认证信息 |
| 继承 `accountStatus` | User | 正常 / 禁用 |
| `className` | Student | 班级 |
| `major` | Student | 专业 |
| `contact` | Student | 联系方式，可选 |
| `createdAt` | Student | 学生账号创建时间 |
| `lastLoginAt` | Student | 学生最后登录时间 |

### Student 动态派生，不持久化

- 月度积分；
- 学期积分；
- 总积分；
- 当前月度 / 学期 / 总榜排名；
- 当前排行榜称号；
- 当前专项徽章等级；
- 当前各类别有效服务时长；
- 当前有效服务总时长。

## 13.3 Administrator

| 字段 | 归属 | 说明 |
|---|---|---|
| 继承 `accountId` | User | 管理员账号 |
| 继承 `name` | User | 管理员姓名 |
| 继承 password-related data | User | 管理员账号认证信息 |
| 继承 `accountStatus` | User | 管理员账号状态 |

当前不冻结独有持久字段。

## 13.4 VolunteerRecord

| 字段 | 说明 |
|---|---|
| `recordId` | 志愿记录稳定唯一标识 |
| `ownerAccountId` | 所属 Student 的稳定账号 ID，不长期保存 `Student*` |
| `serviceDate` | 服务实际发生日期，是月度 / 学期统计的唯一时间归属依据 |
| `appliedCategoryId` | 学生申报类别的稳定 ID |
| `finalCategoryId` | 审核通过后最终确认类别的稳定 ID；非 Approved 状态不形成正式结算值 |
| `appliedDuration` | 学生申报服务时长 |
| `finalDuration` | 审核通过后最终确认时长；非 Approved 状态不形成正式结算值 |
| `place` | 志愿服务地点 |
| `verifier` | 志愿事实证明人 |
| `summary` | 原始业务事实简述 |
| `status` | `RecordStatus` 强类型枚举：Pending / Approved / Rejected / Withdrawn |
| `reviewerAccountId` | 当前最近一次仍对当前记录状态有效的审核管理员账号 ID |
| `reviewNote` | 当前最近一次仍对当前记录状态有效的审核意见 |
| `settledCoefficient` | 审核通过时冻结的实际结算系数 |
| `finalScore` | 审核通过时冻结的最终结算积分 |

### VolunteerRecord 状态原则

`status` 使用：

```cpp
enum class RecordStatus
{
    Pending,
    Approved,
    Rejected,
    Withdrawn
};
```

只保留四种业务状态：

```text
Pending
Approved
Rejected
Withdrawn
```

不设置：

```text
Deleted
Invalid
isDeleted
isValid
```

管理员删除志愿记录采用物理删除。

### 当前审核信息与历史审核的边界

`reviewerAccountId` 与 `reviewNote` 属于 `VolunteerRecord` 的**当前审核信息**，不承担完整审核历史职责。

- `Pending`：当前不存在有效审核结果，审核人和审核意见为空；
- `Rejected`：保存本轮最新驳回对应的审核管理员与审核意见；
- `Rejected → Pending` 重新提交时：清空 `reviewerAccountId` 与 `reviewNote`；
- 新一轮审核结束后：写入新的当前审核信息；
- 历次管理员审核 / 治理行为由 `OperationLog` 独立保留。

因此：

```text
VolunteerRecord
= 当前业务事实 + 当前状态 + 当前有效审核结果

OperationLog
= 关键管理员行为历史
```

### 状态与结算字段不变量

当前冻结：

```text
Pending
→ reviewerAccountId = 空
→ reviewNote = 空
→ 无正式 finalCategoryId / finalDuration
→ 无 settledCoefficient / finalScore

Rejected
→ reviewerAccountId ≠ 空
→ reviewNote ≠ 空
→ 无正式结算结果

Approved
→ reviewerAccountId ≠ 空
→ finalCategoryId / finalDuration 有效
→ settledCoefficient / finalScore 有效
→ reviewNote 可为空或有说明

Withdrawn
→ 无当前审核结果
→ 无正式结算结果
```

不允许出现“Pending 但已有最终积分”“Rejected 但已有正式结算系数”等相互矛盾的字段组合。

## 13.5 VolunteerCategory

| 字段 | 说明 |
|---|---|
| 类别名称 | 志愿服务类型 |
| 当前积分系数 | 只用于后续新结算记录 |
| 启用状态 | 控制是否允许继续选择 |

## 13.6 BadgeRule

| 字段 | 说明 |
|---|---|
| `badgeRuleId` | 专项徽章规则独立稳定主键 |
| `badgeName` | 专项徽章展示名称，不承担主键职责 |
| `categoryId` | 唯一绑定的一种 VolunteerCategory |
| `bronzeThreshold` | 铜级累计有效服务时长门槛 |
| `silverThreshold` | 银级累计有效服务时长门槛 |
| `goldThreshold` | 金级累计有效服务时长门槛 |
| `enabled` | 是否启用 |

约束：

```text
0 < Bronze < Silver < Gold
```

关系唯一性：

```text
一套 BadgeRule
→ 必须且只能绑定一个 VolunteerCategory

一个 VolunteerCategory
→ 最多只能对应一套 BadgeRule
```

因此 `categoryId` 同时承担稳定关联键和业务唯一性约束，但不取代 `badgeRuleId` 作为 BadgeRule 自身主键。

当前学生专项徽章等级不持久化，由当前有效记录 + 当前启用 BadgeRule 动态判定。

## 13.7 Semester

| 字段 | 说明 |
|---|---|
| `semesterId` | 学期配置独立稳定主键 |
| `semesterName` | 学期展示 / 业务名称，不承担主键职责 |
| `startDate` | 学期统计开始边界 |
| `endDate` | 学期统计结束边界 |

`Semester` 不持久化 `isCurrent`。

系统“当前学期是谁”作为一个全局唯一配置事实，通过：

```text
currentSemesterId
```

单独表达。

当前学期判断：

```text
semester.semesterId == currentSemesterId
```

`VolunteerRecord` 不保存 `semesterId`。某条志愿记录是否进入当前学期统计，始终通过：

```text
semester.startDate
<= record.serviceDate
<= semester.endDate
```

动态判断。

因此 Semester 不拥有、不包含、也不直接标记 VolunteerRecord，只提供统计时间范围。

## 13.8 DiaryPost

| 字段 | 说明 |
|---|---|
| 内容编号 | 日记墙帖子唯一标识 |
| 关联志愿记录 | 必须来源于真实、当前存在且审核通过的 VolunteerRecord |
| 发布学生 | 必须与原记录所属 Student 一致 |
| 展示标题 | Student 可独立编辑 |
| 展示文案 | Student 可独立编辑 |
| 展示状态 | 待展示审核 / 已展示 / 已下架 |
| 正式公开时间 | 管理员通过展示审核、进入公开信息流的时刻 |

以下展示事实从关联 VolunteerRecord 获取，不重复保存：

- 学生姓名；
- 志愿类别；
- 服务日期；
- 服务时长；
- 地点；
- 本条记录积分。

点赞数也不持久化。

## 13.9 LikeRelation

| 字段 | 说明 |
|---|---|
| 点赞学生 | Student |
| 对应日记墙内容 | DiaryPost |
| 点赞时间 | 关系建立时间 |

唯一性：

```text
(StudentId, DiaryPostId)
```

## 13.10 OperationLog

| 字段 | 说明 |
|---|---|
| `logId` | 日志稳定唯一标识 |
| `operationTime` | 管理行为发生时间 |
| `operatorAccountId` | 执行该操作的 Administrator 稳定账号 ID |
| `operationType` | `enum class OperationType` 强类型操作类型 |
| `targetType` | `enum class OperationTargetType`，表示被操作业务对象类型 |
| `targetId` | 被操作实体的稳定 ID |
| `description` | 关键变更、原因和必要上下文摘要 |

OperationLog：

- 可以查看；
- 可以查询；
- 可以作为数据内容被导出；
- 不允许管理员手动修改；
- 不允许管理员手动删除；
- 不依赖被操作对象继续存在；
- 不额外保存管理员姓名快照，展示姓名需要时通过 `operatorAccountId` 解析。

### 日志记录范围修订

OperationLog 重点记录具有责任追溯价值的关键审核、治理、账号管理与规则配置行为。

当前明确不记录：

- 普通页面浏览；
- 普通查询；
- 查看排行榜 / 统计；
- **只读数据导出**。

数据导出仍属于 Administrator 的 `ExportData` 角色能力，但导出过程只读取并复制当前数据，不修改系统业务状态，因此不再生成 OperationLog。

---

# 14. 主要领域关系继续保持 Gate 2 冻结结论

```text
Student 1 -------- N VolunteerRecord

Administrator
---- reviews / governs ----
VolunteerRecord

VolunteerRecord
├── appliedCategory → VolunteerCategory
└── finalCategory   → VolunteerCategory

VolunteerCategory 1 ---- N VolunteerRecord
VolunteerCategory 1 ---- 0..1 BadgeRule

Student 1 -------- N DiaryPost
VolunteerRecord 1 ---- 0..1 DiaryPost

Student 1 ---- N LikeRelation N ---- 1 DiaryPost

Semester
---- provides date range for ----
VolunteerRecord statistics

Administrator 1 ---- N OperationLog
```

最终哪些采用组合、聚合、普通关联、ID、引用、指针、智能指针、容器，继续由后续 Gate 3 冻结。

---

# 15. 删除一致性链

```text
VolunteerRecord 物理删除
↓
关联 DiaryPost 同步删除
↓
关联 LikeRelation 同步清理
↓
月度 / 学期 / 总积分重新汇总
↓
排行榜重新排序
↓
排行榜称号重新判定
↓
专项徽章重新判定
↓
OperationLog 保留删除行为审计事实
```

删除不是 VolunteerRecord 的业务状态。

---

# 16. 排行榜、积分与荣誉的架构边界

以下概念继续不作为独立持久业务实体：

- 月度积分；
- 学期积分；
- 总积分；
- 月度排行榜；
- 学期排行榜；
- 总积分排行榜；
- 当前排行称号；
- 当前专项徽章等级；
- 点赞数量。

它们属于：

> **由当前有效业务数据与规则计算得到的派生结果。**

## 16.1 排行榜称号与专项徽章继续严格分离

```text
专项徽章
→ BadgeRule
→ 按某一志愿类别累计有效服务时长
→ 铜 / 银 / 金成长等级

排行榜称号
→ 排行榜动态派生
→ 按月度 / 学期 / 总积分绝对排名
→ 金 / 银 / 铜对应第 1 / 2 / 3 名
```

排行榜称号不由 BadgeRule 管理，也不持久化历史获得记录。

## 16.2 排行榜人数不足时的授予规则

排行榜荣誉只看**绝对名次**，不设置最低参评人数，不按参与人数比例缩放。

```text
榜上只有 1 人
→ 第 1 名获得金
→ 银、铜空缺

榜上只有 2 人
→ 第 1 名获得金
→ 第 2 名获得银
→ 铜空缺

榜上达到 3 人及以上
→ 第 1 / 2 / 3 名分别获得金 / 银 / 铜
```

不存在“因为参榜人数少就取消第一名金奖”或“自动把第二名提升为金奖”的规则。

这条规则同时适用于：

- 月度雷锋之星·金 / 银 / 铜；
- 学期志愿先锋·金 / 银 / 铜；
- 雷锋标兵·金 / 银 / 铜。

---

# 17. Qt 架构原则

最终 V1.0 使用：

> **C++ + Qt 窗口程序**

但领域对象原则上保持纯 C++。

不应让 `User`、`Student`、`Administrator`、`VolunteerRecord`、`DiaryPost` 等为了 GUI 直接承担：

- `QWidget`
- `QMessageBox`
- `QTableWidget`
- 页面跳转
- 控件读写
- 菜单显示

当前最终目标方向：

```text
Qt Presentation
        ↓
Application / Service
        ↓
Domain
        ↓
Repository / Persistence
```

这一分层是 V1.0 目标，V0.1 不要求一开始就完全达到。

---

# 18. 版本演进与 Gate 3 的关系

```text
V0.1
基础 OOP 正确
核心业务闭环
允许较多 cin / cout
允许一定耦合和职责偏重

↓

V0.2
核心业务重构与完善
职责细化
分层清晰
补齐统计、徽章、学期、日志等

↓

V0.9
Qt 集成验证
登录 / 提交 / 审核 / 查询

↓

V1.0
完整 Qt 最终系统
优秀档 / 满分导向
```

Gate 3 负责：

> **先确定最终正确方向，再允许版本真实逐步演进。**

---
