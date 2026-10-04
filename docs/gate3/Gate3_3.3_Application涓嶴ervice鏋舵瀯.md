# Gate 3.3 — Application 与 Service 架构

> 状态：**FINAL PASS**。
>
> 下方正文逐行迁移自 v2.4 第 3079～4652 行；保留 Service 职责划分、完整性终审、Service 结构总图与实现级风险。D63～D91 均保留在原上下文中。

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

# 26. Gate 3.3：Application / Service 层职责与边界

## 26.1 Gate 3.3 的目标

Gate 3.1 已经解决：

> `User / Student / Administrator` 如何形成有意义的继承与多态体系。

Gate 3.2 已经解决：

> 核心领域对象之间如何通过稳定 ID、关联、弱聚合和生命周期规则建立可靠关系。

Gate 3.3 继续回答：

> **当一个完整业务动作需要同时协调多个领域对象、权限、状态、日志与持久化时，应由谁组织整个流程？**

最终 V1.0 的目标分层继续保持：

```text
Qt Presentation
        ↓
Application / Service
        ↓
Domain
        ↓
Repository / Persistence
```

Service 层不是简单的“转发器”，也不是新的 God Class。

它的核心职责是：

> **接收一个完整业务用例，在不破坏领域对象封装的前提下，检查跨对象条件、组织调用顺序、协调相关领域对象与持久化边界，并返回业务结果。**

---

## 26.2 Service 与 Domain 的基本职责边界

### 决策 D63

> **Service 层承担跨领域对象、规则和持久化边界的业务流程协调，不把跨对象流程强塞进 `Student`、`Administrator`、`VolunteerRecord` 等领域实体。**

例如管理员删除一条已通过志愿记录时，完整业务动作可能涉及：

```text
Administrator
VolunteerRecord
DiaryPost
LikeRelation
OperationLog
Repository / Persistence
```

这一整套流程不应全部塞入 `Administrator`，也不应让 `VolunteerRecord` 自己去寻找和删除其他对象。

### 决策 D64

> **Service 是业务协调者，而不是领域状态的无约束修改者。单个领域对象自己的状态转换与不变量仍由领域对象保护；Service 负责检查跨对象条件、组织调用顺序并协调关联数据。**

因此统一采用：

```text
Service
→ 判断“这次操作是否允许、需要协调谁、按什么顺序执行”

Domain
→ 判断“我自身的状态转换是否合法，并维护自己的内部不变量”
```

禁止退化为：

```text
Service
→ 随意 setStatus / setScore / setAccountStatus
→ Domain 只剩 getter / setter
```

也禁止领域对象反向依赖 Service。

---

# 27. Gate 3.3.1：志愿记录主业务 Service

## 27.1 为什么不采用单一巨型 VolunteerService

志愿记录业务存在两条稳定主线：

```text
学生侧
→ 发起 / 管理本人志愿申请

管理员侧
→ 审核 / 治理志愿记录
```

虽然二者都围绕 `VolunteerRecord`，但业务关注点明显不同。

学生侧主要检查：

```text
是否本人记录
当前状态是否允许修改 / 撤回 / 重提
账号是否 Active
学生是否具有对应基础能力
```

管理员侧主要检查：

```text
管理员是否 Active
是否具有审核 / 治理能力
记录是否处于合法状态
最终类别 / 时长是否合法
结算信息如何形成
是否需要 OperationLog
是否存在 DiaryPost / LikeRelation 联动
```

如果全部进入一个 `VolunteerService`，随着功能增加极易演化为新的 God Class。

### 决策 D65

> **志愿记录主业务不采用单一巨型 `VolunteerService`，而是按稳定业务责任拆分为学生侧志愿流程服务与管理员侧审核 / 治理服务。**

当前概念类名采用：

```text
StudentVolunteerService
VolunteerReviewService
```

最终函数签名留到实现级审计。

### 决策 D66

> **`StudentVolunteerService` 负责学生对本人志愿记录的创建、提交、修改、撤回、被驳回后的修改与重新提交等流程；`VolunteerReviewService` 负责审核通过、审核驳回、最终类别 / 时长确认、已通过记录强制更正以及异常 / 违规记录物理删除等管理员治理流程。**

### 决策 D67

> **不为每一个按钮或单个动作建立独立 Service；Service 粒度以稳定业务域为单位，避免 `ApproveService`、`RejectService`、`WithdrawService` 等过度拆分。**

当前结构：

```text
                         VolunteerRecord
                         /             \
                        /               \
               student workflow      admin workflow
                      /                   \
                     v                     v
       StudentVolunteerService     VolunteerReviewService
```

---

# 28. Gate 3.3.2：Statistics / Ranking / Badge 三个 Service

积分、排行榜与专项徽章都依赖当前有效业务数据，但它们不是同一种职责。

当前继续坚持：

- 月度积分、学期积分、总积分是派生结果；
- 排行榜与排行称号是派生结果；
- 当前专项徽章等级是派生结果；
- 不将这些结果重复持久化到 `Student`。

## 28.1 StatisticsService

### 决策 D68

> **采用独立具体类 `StatisticsService` 负责基础派生统计，包括月度积分、学期积分、总积分、有效服务总时长、指定类别有效服务时长、有效记录数及必要的全局统计。**

进一步冻结：

1. `StatisticsService` 当前是普通具体 Service 类；
2. 不作为 `RankingService` 或 `BadgeService` 的基类；
3. 核心统计方法当前采用普通非虚成员函数；
4. 不为了展示多态而人为加入 `virtual` / 纯虚函数；
5. 只有未来真实出现多种可替换统计实现时，才重新评估统计抽象接口。

因此明确否定：

```text
StatisticsService
├── RankingService
└── BadgeService
```

原因：

```text
RankingService is-a StatisticsService   ×
BadgeService is-a StatisticsService     ×
```

真实语义是：

```text
RankingService uses StatisticsService
BadgeService uses StatisticsService
```

## 28.2 RankingService

### 决策 D69

> **采用独立具体类 `RankingService` 负责月度、学期、总积分排行榜，以及由排行榜绝对名次直接派生的排行称号。**

`RankingService` 复用 `StatisticsService` 提供的统计结果，不重复实现积分统计规则。

统一排序规则继续为：

```text
积分降序
→ 有效服务总时长降序
→ 有效志愿记录数降序
→ 学号升序
```

排行称号直接由绝对名次派生：

```text
第 1 名 → 金
第 2 名 → 银
第 3 名 → 铜
```

如果榜上人数不足，对应不存在的名次保持空缺。

不额外建立 `TitleService`。

> **实现细化 → Gate 4**
>
> 本节冻结的是 StatisticsService / RankingService 的职责边界与派生结果语义；对应查询、一次扫描聚合与排序算法已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.2_Repository数据结构查询与统计算法实现细化_候选冻结稿(1).md`

## 28.3 BadgeService

### 决策 D70

> **采用独立具体类 `BadgeService` 负责专项徽章动态判定。**

其业务输入逻辑为：

```text
指定 Student
+
指定 VolunteerCategory 下当前有效服务时长
+
该 Category 唯一绑定的当前 BadgeRule
↓
判定当前最高有效等级
```

结果为概念上的：

```text
None / Bronze / Silver / Gold
```

`BadgeService` 使用 `StatisticsService` 提供类别有效服务时长，但不继承 `StatisticsService`。

`BadgeService` 与 `RankingService` 之间：

- 不建立继承；
- 不建立 `friend`；
- 不访问彼此内部状态；
- 不人为制造特殊耦合。

专项徽章与排行榜称号继续保持两套完全不同的荣誉体系。

## 28.4 派生结果不持久化

### 决策 D71

> **`StatisticsService`、`RankingService`、`BadgeService` 只负责计算和返回派生结果，不把月度 / 学期 / 总积分、排名、排行称号、当前专项徽章等级等写回 `Student` 或其他领域对象作为长期持久状态。**

底层有效数据发生变化后：

```text
源数据变化
↓
下一次查询
↓
按当前有效数据重新求值
```

因此当前不引入：

- 积分缓存；
- 排行榜缓存；
- 徽章缓存；
- 缓存版本号；
- 缓存失效事件；
- 复杂事件总线。

当前三者结构图：

```text
                    VolunteerRecord
                          |
                          v
                 StatisticsService
                   /             \
                  / uses       uses \
                 v                 v
         RankingService       BadgeService
              |                    |
              v                    v
      排名 + 排行称号        当前专项徽章等级
```

重要说明：

> **“删除记录后重新计算积分 / 排行榜 / 徽章”在当前动态派生架构下，主要意味着源数据变化后下一次查询基于新数据重新求值，而不是维护另一份持久化统计结果。**

---

# 29. Gate 3.3.3：日记墙与点赞 Service

## 29.1 为什么采用统一 DiaryService

`LikeRelation` 虽然是独立领域对象，但点赞业务高度依附 `DiaryPost`。

当前点赞只有轻量职责：

- 点赞；
- 取消点赞；
- `(studentAccountId, postId)` 唯一性；
- 点赞数量动态统计；
- 普通下架保留既有 LikeRelation；
- DiaryPost 物理删除时 LikeRelation 级联清理。

单独建立 `LikeService` 会产生职责过薄、且仍强依赖 `DiaryPost` 的服务类。

### 决策 D72

> **采用独立具体类 `DiaryService` 统一协调日记墙业务域，包括展示申请、展示审核、公开展示、下架、公开查询与筛选，以及轻量点赞 / 取消点赞流程；当前不额外拆分 `LikeService`。**

### 决策 D73

> **`LikeRelation` 保持独立领域对象，但其创建、删除、唯一性检查与点赞数量查询由 `DiaryService` 协调；“领域对象独立”不等于“必须一对象一 Service”。**

### 决策 D74

> **`DiaryPost` 自身状态转换仍由领域对象保护，`DiaryService` 不通过通用 setter 任意改变帖子状态；Service 负责 `Student`、`VolunteerRecord`、`DiaryPost`、`LikeRelation`、权限与日志之间的跨对象协调。**

例如展示申请：

```text
Student Active
+
RequestDiaryPost Permission
+
VolunteerRecord 属于当前 Student
+
VolunteerRecord.status == Approved
+
该 recordId 尚未生成 DiaryPost
+
展示标题 / 文案合法
↓
DiaryService 协调创建 DiaryPost
```

## 29.2 TakenDown 后的点赞边界

### 决策 D75

> **只有处于 `Displayed` 状态的 DiaryPost 允许普通学生产生新的点赞或取消点赞互动。`PendingDisplayReview` 与 `TakenDown` 不接受新的普通学生互动。**

进一步冻结：

```text
Displayed
→ 可点赞 / 取消点赞

PendingDisplayReview
→ 不允许点赞 / 取消点赞

TakenDown
→ 不允许新的普通学生互动
→ 既有 LikeRelation 保留
```

来源 `VolunteerRecord` 被物理删除时：

```text
VolunteerReviewService
作为主流程协调者
        ↓
调用 DiaryService
        ↓
按 recordId 清理关联 DiaryPost
        ↓
清理该帖 LikeRelation
        ↓
VolunteerReviewService 继续完成记录删除主流程
```

即：

> **谁发起完整业务动作，谁负责主流程；被调用的其他 Service 只处理自己业务域中的子流程。**

这一原则用于避免 Service 之间出现“谁都可以成为总指挥”的循环协调。

---

# 30. Gate 3.3.4：Authentication / UserManagement / PasswordHasher

账号域需要严格区分：

```text
Authentication
→ 你是谁、能否登录？

Authorization
→ 你当前能否执行这个具体业务动作？

Account Management
→ 账号资料与账号状态如何维护？
```

当前不把三者混在一个巨大 `UserService` 中。

## 30.1 AuthenticationService

### 决策 D76

> **采用独立具体类 `AuthenticationService` 负责登录认证。**

其职责包括：

1. 根据 `accountId` 查找实际账号对象；
2. 调用密码认证技术组件验证凭据；
3. 检查账号是否允许登录；
4. 认证成功后，以 `User` 抽象类型统一向上层返回实际用户对象；
5. 运行时实际类型可能为 `Student` 或 `Administrator`；
6. 不依据账号长度、字符串格式或魔法规则推断角色。

此处只冻结“通过 User 抽象统一接收”的架构原则，不提前冻结最终 C++ 返回类型是引用、智能指针还是其他生命周期安全句柄，避免在 Repository / 生命周期审计前过早决定所有权模型。

### Student 成功登录时间

若实际对象为 `Student`：

```text
AuthenticationService
↓
完成成功认证
↓
调用 Student 的语义行为
recordSuccessfulLogin(time)（概念名称）
↓
Student 自身更新 lastLoginAt
```

不采用：

```text
AuthenticationService
→ student.setLastLoginAt(...)
```

最终函数名与签名后续实现级审计再定。

### 登录失败信息

认证内部可以区分：

- 账号不存在；
- 密码错误；
- 账号禁用；
- 其他认证失败。

这些内部结果可用于程序控制流程，但普通登录 UI 对外采用模糊失败反馈，不直接暴露可用于枚举账号状态的细分原因。

## 30.2 Authentication 与 Authorization 分离

### 决策 D77

> **`AuthenticationService` 只解决“当前用户是谁以及是否能够完成登录”，不承担具体业务授权。**

具体业务是否合法继续由对应业务 Service 综合：

```text
Permission
+
AccountStatus
+
对象归属
+
对象状态
+
具体业务规则
```

进行判断。

当前不引入独立 `AuthorizationService`。

角色基础能力继续由：

```text
UserCapabilities
```

表达；具体操作的最终授权由具体业务 Service 与领域规则共同完成。

## 30.3 PasswordHasher

### 决策 D78

> **密码哈希 / 验证算法由轻量技术组件 `PasswordHasher` 承担。**

`PasswordHasher` 只接收算法真正需要的数据，例如：

```text
明文密码
+
已存储认证数据
```

它不依赖：

- `User`；
- `Student`；
- `Administrator`；
- `VolunteerRecord`；
- `Permission`；
- Qt 页面；
- 其他业务对象。

它不负责：

- 查找账号；
- 判断角色；
- 登录流程；
- 账号状态管理；
- 业务授权；
- Qt 交互。

架构定位：

> **`PasswordHasher` 是技术服务 / 算法组件，不属于业务 Service，不与 `DiaryService`、`StatisticsService` 等业务域 Service 并列。**

可被：

```text
AuthenticationService
UserManagementService
```

复用。

## 30.4 UserManagementService

### 决策 D79

> **采用独立具体类 `UserManagementService` 作为账号域业务服务；它不是管理员专属 Service。**

它承接两类稳定账号用例。

### A. 管理员对 Student 的账号治理

包括：

```text
创建 Student 账号
禁用 Student
恢复 Student
重置 Student 密码
维护管理员允许修改的学生身份资料
```

授权逻辑：

```text
Administrator Active
+
ManageStudents Permission
+
目标 Student / 输入数据满足业务规则
```

涉及账号状态改变时：

```text
UserManagementService
→ 判断操作者权限与业务条件

Student / User
→ 通过 disable() / enable() 等语义行为维护自身状态
```

不提供任意 `setAccountStatus()`。

管理员关键账号治理行为按既有规则生成 `OperationLog`。

### B. 当前登录 Student 的本人账号维护

包括：

```text
修改本人 contact
修改本人密码
```

此类操作共享同一账号域 Service，但在接口与授权检查中明确区分：

- 当前操作者；
- 操作目标；
- 可修改字段；
- 所需认证条件。

学生本人普通联系方式 / 密码维护不因为使用 `UserManagementService` 就自动产生 `OperationLog`。

不额外建立 `ProfileService`。

登录认证仍属于 `AuthenticationService`，不并入 `UserManagementService`。

## 30.5 当前登录用户上下文

### 决策 D80

> **当前不引入 `SessionService`、`SessionManager` 等独立会话体系。**

必要的当前登录用户上下文只存在于：

```text
Qt Presentation
或
轻量 Application 协调层
```

不进入：

- `User`；
- `Student`；
- `Administrator`；
- `VolunteerRecord`；
- 其他领域对象；
- 业务 Service 的长期内部状态。

调用业务 Service 时，应显式提供当前操作主体或稳定身份信息，由对应 Service 执行授权检查。

禁止采用：

```text
global currentUser
静态单例 SessionManager
StatisticsService.currentUser
DiaryService.currentUser
```

等隐藏会话状态。

当前明确不引入：

```text
AuthorizationService
SessionService
SessionManager
ProfileService
```

---

# 31. Gate 3.3.5：配置类 Service 的边界

当前配置领域主要包含：

```text
VolunteerCategory
BadgeRule
Semester
```

虽然三者都存在管理员配置行为，但业务内聚性不同。当前明确不采用一个覆盖全部配置事项的巨型 `ConfigurationService`。

## 31.1 配置域的拆分原则

配置业务按稳定业务内聚性拆为两块：

```text
志愿规则配置域
→ VolunteerCategory + BadgeRule

学期时间配置域
→ Semester + currentSemesterId
```

### 决策 D81

> **不采用一个覆盖所有配置业务的巨型 `ConfigurationService`；配置层按照业务内聚性划分为志愿规则配置与学期时间配置两个稳定业务域。**

这样既避免：

```text
ConfigurationService
→ 什么配置都往里塞
```

也避免为每一个领域对象机械创建一个过薄 Service。

## 31.2 RuleConfigurationService

### 决策 D82

> **采用独立具体类 `RuleConfigurationService` 协调 `VolunteerCategory` 与 `BadgeRule` 的管理员配置流程。**

其主要跨对象协调职责包括：

```text
管理员是否 Active
是否具有 ManageVolunteerCategories / ManageBadgeRules 能力
目标 Category / BadgeRule 是否存在
BadgeRule 绑定的 categoryId 是否合法
一个 Category 是否已经存在一套 BadgeRule
是否满足 Category–BadgeRule 唯一绑定约束
是否需要生成 OperationLog
```

但单个领域对象自身的不变量仍由领域对象保护，例如：

```text
VolunteerCategory
→ 自身系数、启用状态等内部合法性

BadgeRule
→ 0 < Bronze < Silver < Gold
```

Service 不通过机械 setter 让领域对象退化为数据袋。

### 决策 D83

> **`RuleConfigurationService` 修改 `VolunteerCategory` 当前积分系数只影响未来新结算记录，不改变历史 Approved `VolunteerRecord` 已冻结的 `settledCoefficient` 与 `finalScore`；修改 `BadgeRule` 后也不向 Student 写入持久徽章状态，后续由 `BadgeService` 根据当前记录与当前规则动态重新判定。**

因此：

```text
Category coefficient 修改
→ 只改变未来结算规则
→ 历史 Approved 记录不追溯重算

BadgeRule threshold 修改
→ 不批量修改 Student
→ 下一次 BadgeService 查询时按新规则重新判定
```

## 31.3 SemesterService

`Semester` 与 Category / BadgeRule 不同，它的本质是：

> **统计时间上下文配置。**

### 决策 D84

> **采用独立具体类 `SemesterService` 负责 Semester 配置维护及当前学期切换；由其在应用层保证 `currentSemesterId` 始终引用合法存在的 Semester。Semester 自身保护自身时间边界合法性。**

职责边界：

```text
SemesterService
→ 管理学期配置
→ 校验目标 semesterId 是否存在
→ 协调 currentSemesterId 切换
→ 检查管理员权限
→ 必要时形成 OperationLog

Semester
→ 维护自身 semesterId / name / startDate / endDate
→ 保护自身时间区间合法性
```

### 决策 D85

> **`SemesterService` 管理 `currentSemesterId` 这一业务配置事实，但 `currentSemesterId` 的最终持久化位置、配置文件结构及 Repository 实现继续留待 Gate 3 Repository / Persistence 审计；`StatisticsService` 只消费 Semester 时间范围进行学期统计，不负责修改学期配置。**

因此当前只冻结：

```text
谁管理 currentSemesterId？
→ SemesterService
```

暂不冻结：

```text
currentSemesterId 具体保存在哪个文件 / Repository？
→ Gate 3.4 再决定
```

配置域结构：

```text
                 配置业务
                /       \
               /         \
              v           v
RuleConfigurationService  SemesterService
        |                      |
        v                      v
VolunteerCategory            Semester
BadgeRule               currentSemesterId
```

---

# 32. Gate 3.3.6：OperationLog 与 Export Service

`OperationLog` 与 Export 都属于管理员后台能力，但职责完全不同：

```text
OperationLog
→ 审计已经发生的关键管理行为

Export
→ 读取当前数据并生成面向管理员的外部副本
```

只读导出继续遵循 D61：**不生成 OperationLog**。

## 32.1 OperationLogService

`OperationLog` 与 `OperationLogService` 必须区分：

```text
OperationLog
= 一条不可变的历史审计事实

OperationLogService
= 统一协调审计事实的创建与查询
```

### 决策 D86

> **采用独立具体类 `OperationLogService` 统一协调关键管理操作日志的创建与查询。`OperationLog` 仍是不可变审计领域对象，`OperationLogService` 不提供管理员手动修改或删除历史日志的能力。**

允许的方向：

```text
create / append audit fact
query audit facts
```

明确禁止：

```text
editLog(...)
deleteLog(...)
```

## 32.2 谁决定需要写日志

### 决策 D87

> **需要留痕的业务 Service 在关键管理操作成功后，通过 `OperationLogService` 形成统一的 `OperationLog`，而不在各 Service 中分别实现日志字段组织与持久化逻辑。`OperationLogService` 不决定业务动作本身是否合法，只负责把已发生的关键管理行为转换为统一审计事实。**

依赖方向：

```text
VolunteerReviewService       ─┐
UserManagementService        ─┤
DiaryService                 ─┤
RuleConfigurationService     ─┤──> OperationLogService
SemesterService              ─┘
```

是否写日志取决于具体业务动作，而不是“调用了哪个 Service”。例如：

```text
Student 修改本人 contact
→ 不记 OperationLog

Student 提交志愿记录
→ 不记 OperationLog

管理员查看统计 / 排行榜
→ 不记 OperationLog

管理员执行只读导出
→ 不记 OperationLog
```

关键审核、治理、账号管理与规则配置行为才进入日志。

## 32.3 业务变更与日志一致性

### 决策 D88

> **具有审计要求的“业务状态变更 + OperationLog 记录”属于同一个完整应用流程；当前只冻结这一一致性要求，不提前引入数据库事务、Unit of Work、TransactionManager 等复杂机制，具体文件持久化失败处理留待 Repository / Persistence 审计。**

因此当前承认并记录一个后续必须解决的一致性场景：

```text
业务数据保存成功
↓
OperationLog 写文件失败
↓
可能出现“状态已变但审计事实缺失”
```

这属于 Gate 3.4 Persistence consistency 的实现问题，不构成 Gate 3.3 Service 架构阻塞。

> **实现期审计停点 → Gate 4**
>
> 本节架构语义已冻结，但第一个多 Repository 修改业务的精确 affected set、snapshot、Prepare participant 与 Commit order 必须结合真实代码再决定。
>
> 后续实现时应先读取：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 触发对应 `STOP-05` 后应暂停实现并进行审计，不得由执行层自行补造实现规则。

## 32.4 ExportService

### 决策 D89

> **采用独立具体类 `ExportService` 负责管理员只读数据导出流程，包括权限检查、筛选条件协调、获取当前业务结果及输出文件生成；`ExportService` 不修改业务数据，也不生成 `OperationLog`。**

概念流程：

```text
Administrator
↓
ExportService
↓
检查 ExportData Permission
↓
读取当前数据 / 当前派生结果
↓
应用导出筛选条件
↓
格式化
↓
生成外部文件
```

## 32.5 ExportService 不重复实现业务算法

### 决策 D90

> **`ExportService` 不重复实现积分、排行榜、徽章或日志算法，而是使用 `StatisticsService`、`RankingService`、`BadgeService`、`OperationLogService` 等已有业务服务提供的当前结果，再完成导出组织与格式输出。**

例如：

```text
导出学生积分
→ StatisticsService
→ ExportService 格式化

导出排行榜
→ RankingService
→ ExportService 格式化

导出操作日志
→ OperationLogService
→ ExportService 格式化
```

## 32.6 Export 与 Persistence 明确分离

### 决策 D91

> **明确区分 `ExportService` 与 Repository / Persistence：前者生成面向用户的外部数据副本，后者负责系统自身运行数据的长期保存与恢复。CSV / TXT / Markdown 等具体多格式策略是否采用 Strategy 多态，留待后续设计模式审计，不在 Gate 3.3 提前冻结。**

例如：

```text
系统内部运行数据文件
→ Repository / Persistence

志愿记录_2026-09-01.csv
→ ExportService 生成的外部副本
```

因此：

> **Export ≠ Persistence。**

---

# 33. Gate 3.3 Service 层完整性终审

本节对 D63～D91 做总体验收，检查：

```text
业务覆盖
职责重叠
God Service 风险
过度拆分
Service–Domain 边界
继承 / friend / virtual 滥用
Service 间循环依赖
OperationLog 一致性边界
派生结果边界
当前登录用户上下文
Export / Persistence 边界
Gate 1 / Gate 2 / Gate 3.1 / Gate 3.2 一致性
```

## 33.1 业务覆盖完整性：PASS

当前主要业务域与 Service 映射如下：

| 业务域 | 承接类 / 组件 |
|---|---|
| 登录认证 | `AuthenticationService` |
| 账号资料与账号治理 | `UserManagementService` |
| 学生志愿记录流程 | `StudentVolunteerService` |
| 管理员审核 / 更正 / 删除 | `VolunteerReviewService` |
| 基础统计 | `StatisticsService` |
| 排行榜 / 排行称号 | `RankingService` |
| 专项徽章判定 | `BadgeService` |
| 日记墙 / 点赞 | `DiaryService` |
| Category + BadgeRule 配置 | `RuleConfigurationService` |
| Semester + currentSemesterId | `SemesterService` |
| 审计日志 | `OperationLogService` |
| 数据导出 | `ExportService` |
| 密码哈希 / 验证算法 | `PasswordHasher`（技术组件） |

Gate 1 已冻结的主要业务域均有明确承接，没有发现正式业务域完全无人负责。

结论：**PASS**。

## 33.2 职责重叠审计：PASS

以下边界已经清晰：

```text
AuthenticationService
→ 你是谁、能否登录

UserManagementService
→ 账号资料 / 状态如何维护

StatisticsService
→ 产生基础统计结果

RankingService
→ 使用统计结果排序并派生排行称号

BadgeService
→ 使用类别时长 + BadgeRule 判专项徽章

RuleConfigurationService
→ 修改规则

BadgeService
→ 消费规则判定当前徽章

SemesterService
→ 管理学期时间配置

StatisticsService
→ 消费时间范围进行统计

OperationLogService
→ 统一形成 / 查询审计事实
```

没有发现两个 Service 同时拥有同一核心业务决策权。

结论：**PASS**。

## 33.3 God Service 风险：PASS

当前未采用：

```text
巨型 VolunteerService
巨型 UserService
巨型 ConfigurationService
巨型 StatisticsService（吞并排行 / 徽章）
```

高风险业务均已按稳定业务责任拆分，同时没有为了类数量机械拆分到“一个按钮一个 Service”。

结论：**PASS**。

## 33.4 过度拆分审计：PASS

当前明确不引入：

```text
ApproveService
RejectService
WithdrawService
LikeService
UnlikeService
ProfileService
AuthorizationService
SessionService
TitleService
```

`LikeRelation` 等领域对象独立存在，不意味着必须机械对应一个独立 Service。

结论：**PASS**。

## 33.5 Service–Domain 边界：PASS

继续保持：

```text
Service
→ 跨对象权限、归属、顺序、关联、日志、持久化协调

Domain
→ 自身状态转换与内部不变量
```

禁止：

```text
Service 任意 setStatus / setScore / setAccountStatus
```

领域对象不因 Service 存在而退化为 getter / setter 数据袋。

结论：**PASS**。

## 33.6 Service 继承 / friend / virtual 滥用：PASS

当前：

```text
RankingService ──uses──> StatisticsService
BadgeService   ──uses──> StatisticsService
```

不是：

```text
StatisticsService
├── RankingService
└── BadgeService
```

三者不建立 `friend`；`StatisticsService` 也不为了“体现多态”人为把核心统计函数声明为 `virtual` / 纯虚。

结论：**PASS**。

## 33.7 Service 间循环依赖：PASS

当前主要 Service→Service 依赖方向：

```text
RankingService
      ↓
StatisticsService

BadgeService
      ↓
StatisticsService

VolunteerReviewService
      ↓
DiaryService

AuthenticationService ─┐
                       ├→ PasswordHasher
UserManagementService ─┘

关键管理员业务 Service
      ↓
OperationLogService

ExportService
 ├→ StatisticsService
 ├→ RankingService
 ├→ BadgeService
 └→ OperationLogService
```

当前未发现：

```text
A → B → A
```

循环依赖。

终审补充原则：

> **Service 间协作保持有向依赖；子流程 Service 不反向调用主流程 Service。**

例如：

```text
VolunteerReviewService → DiaryService
```

成立；不设计：

```text
DiaryService → VolunteerReviewService
```

结论：**PASS**。

## 33.8 OperationLog 一致性：PASS_WITH_DEFERRED_IMPLEMENTATION

架构已经明确：

> 需要审计的业务变更与 OperationLog 形成一个完整应用流程。

但文件持久化下如何处理：

```text
业务文件写成功
日志文件写失败
```

仍待 Gate 3.4 决定。

当前不因此引入企业级事务框架。

结论：**Service 架构 PASS；具体一致性机制递交 Gate 3.4。**

## 33.9 派生数据边界：PASS

当前不持久化：

```text
Student.totalScore
Student.currentRank
Student.currentBadge
DiaryPost.likeCount
```

而由：

```text
StatisticsService
RankingService
BadgeService
DiaryService
```

按当前有效事实动态求值。

结论：**PASS**。

## 33.10 当前登录用户上下文：PASS

当前登录用户上下文只保留在 Qt Presentation / 轻量 Application 协调层，并在调用业务 Service 时显式传递 actor 或稳定身份信息。

不采用：

```text
global currentUser
Service.currentUser
Singleton SessionManager
```

结论：**PASS**。

## 33.11 Export / Persistence 边界：PASS

```text
ExportService
→ 外部数据副本

Repository / Persistence
→ 系统自身运行数据
```

两者职责已明确区分。

结论：**PASS**。

## 33.12 已知但不阻塞：管理员账号创建问题

Gate 1 v1.4 §39.6 已冻结：

> 管理员账号“不开放普通注册；由系统预置或由已有管理员建立”。

当前 Gate 3.1 的 Administrator Permission 中只有：

```text
ManageStudents
```

当前 `UserManagementService` 也主要冻结了 Student 账号域治理，并没有正式冻结：

```text
ManageAdministrators
CreateAdministrator
```

因此存在一个需求追踪问题：

```text
如果最终实现选择“已有管理员可以创建管理员”
→ 当前 Permission / Service 边界不足
→ 需要单独审计并新增相应稳定能力

如果最终实现选择“管理员账号由系统预置”
→ 当前架构完全可满足 Gate 1 的 OR 条件
→ 无需新增管理员账号管理能力
```

当前终审决定：

> **不因此重开 D18 / D20 / D79，也不无依据新增管理员管理权限。V1.0 默认允许采用“系统预置管理员账号”完成 Gate 1 要求；若后续明确选择“已有管理员建立新管理员”，必须作为显式需求扩展 / 追踪修正重新审计。**

该问题记录为：

```text
KNOWN_TRACEABILITY_ITEM_ADMIN_ACCOUNT_CREATION
状态：KNOWN / NON-BLOCKING
递交：后续实现范围确认或 Gate 3.4 账号持久化审计时再次检查
```

结论：**不阻塞 Gate 3.3 PASS。**

## 33.13 Gate 3.3 最终判定

经过 D63～D91 与完整性终审：

```text
业务覆盖完整性             PASS
职责重叠                    PASS
God Service 风险            PASS
过度拆分                    PASS
Service–Domain 边界         PASS
继承 / friend / virtual 审计 PASS
Service 循环依赖            PASS
派生结果边界                PASS
当前登录上下文              PASS
Export / Persistence 边界   PASS
OperationLog 一致性          PASS_WITH_DEFERRED_IMPLEMENTATION
管理员账号创建追踪项         KNOWN / NON-BLOCKING
```

正式结论：

> ## **Gate 3.3——Application / Service 层职责与边界：PASS**

递交 Gate 3.4 的两个主要实现问题：

```text
1. Service 通过什么 Repository 获取 / 保存领域对象
2. 多文件持久化下“业务变更 + OperationLog”如何保证一致性
```

---

# 34. Gate 3.3 最终业务 Service 结构总图

## 34.1 分层总图

```text
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Qt Presentation                                │
│ 登录 / 学生端 / 管理员端 / 日记墙 / 排行榜 / 规则配置 / 导出等界面          │
└────────────────────────────────────┬────────────────────────────────────────┘
                                     │
                                     │ 调用业务用例，并显式传递 actor / 身份上下文
                                     v
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Application / Service Layer                          │
│                                                                             │
│ AuthenticationService      UserManagementService                            │
│                                                                             │
│ StudentVolunteerService    VolunteerReviewService                           │
│                                                                             │
│ StatisticsService          RankingService          BadgeService             │
│                                                                             │
│ DiaryService                                                                │
│                                                                             │
│ RuleConfigurationService   SemesterService                                  │
│                                                                             │
│ OperationLogService        ExportService                                    │
└────────────────────────────────────┬────────────────────────────────────────┘
                                     │
                                     │ 调用领域语义 / 读取或修改业务事实
                                     v
┌─────────────────────────────────────────────────────────────────────────────┐
│                                  Domain                                     │
│                                                                             │
│ User <<abstract>>                                                           │
│ ├── Student                                                                 │
│ └── Administrator                                                           │
│                                                                             │
│ VolunteerRecord    VolunteerCategory    BadgeRule                           │
│ Semester           DiaryPost            LikeRelation                        │
│ OperationLog                                                               │
└────────────────────────────────────┬────────────────────────────────────────┘
                                     │
                                     │ 系统运行数据长期保存 / 恢复
                                     v
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Repository / Persistence                            │
│                              Gate 3.4 待审计                                │
└─────────────────────────────────────────────────────────────────────────────┘

技术辅助组件：
AuthenticationService ──uses──> PasswordHasher
UserManagementService  ──uses──> PasswordHasher

PasswordHasher 不属于业务域 Service。
```

## 34.2 志愿记录主流程

```text
Student
   |
   | submit / modify / withdraw / resubmit
   v
StudentVolunteerService
   |
   v
VolunteerRecord

Administrator
   |
   | review / correct / delete
   v
VolunteerReviewService
   |           \
   |            \ 删除来源记录的关联公开内容
   v             v
VolunteerRecord  DiaryService
                   |
                   v
            DiaryPost / LikeRelation

需要审计的管理员关键动作
          |
          v
 OperationLogService
```

## 34.3 Statistics / Ranking / Badge

```text
Approved VolunteerRecord
          |
          v
  StatisticsService
      /         \
     /           \
  uses           uses
   v               v
RankingService   BadgeService
   |               |
   v               v
排行榜 / 排名      专项徽章等级
排行称号
```

三者：

```text
独立具体类
无继承
无 friend
不为凑多态增加 virtual
```

## 34.4 日记墙与点赞

```text
Student
   |
   | request / like / unlike
   v
DiaryService
   |       |         |
   v       v         v
Record   DiaryPost  LikeRelation
             ^
             |
      Administrator
      review / takedown
```

状态规则：

```text
PendingDisplayReview
→ 不允许点赞 / 取消点赞

Displayed
→ 允许点赞 / 取消点赞

TakenDown
→ 不允许新的普通学生互动
→ 既有 LikeRelation 保留
```

## 34.5 账号域

```text
                    Qt / Application
                         |
                  current User context
                         |
            +------------+-------------+
            |                          |
            v                          v
AuthenticationService       UserManagementService
            |                          |
            +------------+-------------+
                         uses
                          |
                          v
                    PasswordHasher
                 技术算法辅助组件
```

## 34.6 配置域

```text
                  Administrator
                       |
             +---------+---------+
             |                   |
             v                   v
RuleConfigurationService    SemesterService
      |          |                |
      v          v                v
VolunteerCategory BadgeRule    Semester
                                +
                        currentSemesterId
```

## 34.7 日志与导出

```text
关键管理员业务 Service
         |
         v
OperationLogService
         |
         v
  OperationLog

StatisticsService ─┐
RankingService    ─┤
BadgeService      ─┼──> ExportService ──> 外部导出文件
OperationLogService┘
```

`ExportService` 只读，不产生 OperationLog。

---

# 35. Gate 3.3 决策索引

| 编号 | 冻结决策 |
|---|---|
| D63 | Service 负责跨对象、规则与持久化边界的业务流程协调，不把跨对象流程塞入领域实体 |
| D64 | Service 不无约束修改领域状态；领域对象继续保护自身状态转换与不变量 |
| D65 | 志愿记录主业务拆为学生侧流程服务与管理员侧审核 / 治理服务，不采用巨型 VolunteerService |
| D66 | StudentVolunteerService 与 VolunteerReviewService 的稳定职责边界正式冻结 |
| D67 | Service 粒度按稳定业务域划分，不按按钮 / 单动作过度拆分 |
| D68 | StatisticsService 为独立具体类；负责基础统计；不作为 Ranking / Badge 基类；当前统计函数不为凑多态设 virtual |
| D69 | RankingService 为独立具体类；复用 StatisticsService；负责三类排行榜与排行称号 |
| D70 | BadgeService 为独立具体类；复用 StatisticsService；负责专项徽章动态判定；不与 RankingService 建继承 / friend |
| D71 | 统计、排行、称号、徽章等派生结果不持久化；当前不引入缓存与缓存失效机制 |
| D72 | DiaryService 统一日记墙业务与轻量点赞流程，当前不拆 LikeService |
| D73 | LikeRelation 仍是独立领域对象，但由 DiaryService 协调创建 / 删除 / 唯一性 / 数量查询 |
| D74 | DiaryPost 自身状态由领域对象保护；DiaryService 负责跨对象协调 |
| D75 | 只有 Displayed 帖子允许普通学生点赞 / 取消点赞；TakenDown 保留既有 LikeRelation，不接受新互动 |
| D76 | AuthenticationService 负责登录认证；以 User 抽象统一返回运行时 Student / Administrator；Student 成功登录用语义行为更新 lastLoginAt；UI 使用模糊失败信息 |
| D77 | Authentication 与 Authorization 分离；AuthenticationService 不承担具体业务授权 |
| D78 | PasswordHasher 是不依赖业务对象的技术算法组件，不属于业务 Service |
| D79 | UserManagementService 是账号域服务，统一管理员账号治理与学生本人允许的账号资料维护，不另设 ProfileService |
| D80 | 不引入 SessionService / SessionManager；当前登录上下文仅在 Qt / Application 层维护，不成为领域或业务 Service 隐藏状态 |
| D81 | 不采用巨型 ConfigurationService；配置业务按志愿规则配置与学期时间配置两个稳定业务域拆分 |
| D82 | RuleConfigurationService 协调 VolunteerCategory + BadgeRule 管理，并负责跨对象关联与唯一性检查 |
| D83 | Category 系数修改不追溯历史结算；BadgeRule 修改不写回 Student 持久徽章状态，由 BadgeService 动态重判 |
| D84 | SemesterService 负责 Semester 配置与 currentSemesterId 切换，并保证 currentSemesterId 合法 |
| D85 | SemesterService 管理 currentSemesterId 的业务事实；其持久化位置留待 Gate 3.4；StatisticsService 只消费学期范围 |
| D86 | OperationLogService 统一协调关键管理日志创建与查询；不允许手动修改 / 删除历史日志 |
| D87 | 需要留痕的业务 Service 通过 OperationLogService 统一形成审计事实；OperationLogService 不决定业务是否合法 |
| D88 | 审计要求下“业务变更 + OperationLog”属于同一应用流程；具体文件一致性机制留待 Gate 3.4 |
| D89 | ExportService 负责只读数据导出；不修改业务数据，也不生成 OperationLog |
| D90 | ExportService 复用 Statistics / Ranking / Badge / OperationLog 等已有结果，不重复实现业务算法 |
| D91 | Export 与 Persistence 分离；多格式 Strategy 是否采用留待后续设计模式审计 |

---
