# Gate 3.5 — Qt 表现层与应用架构

> 状态：**FINAL PASS**。
>
> 下方正文逐行迁移自 v2.4 第 6848～12337 行；保留页面体系、导航、Model/View、刷新一致性、DiaryWall、文本全景图、流程图和终审。D177～D241 均保留在原上下文中。旧阶段状态已移入历史档案，避免与当前 FINAL PASS 混读。

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

# 52. Gate 3.5：Qt Presentation / Application 边界总体定位

Gate 3.1～Gate 3.4 已经分别冻结：

```text
User / Student / Administrator 继承与多态
↓
领域对象关系与 ID 技术映射
↓
Application / Service 职责边界
↓
Repository / Persistence 权威数据、对象生命周期与提交协议
```

因此 Gate 3.5 的任务不是重新设计业务，而是回答：

> **最终 V1.0 使用 Qt 后，界面层如何消费已经冻结的 Service / Domain / Repository 架构，同时避免 Qt 重新变成新的业务中心。**

本阶段重点审计：

1. Qt Presentation 可以负责什么、不能负责什么；
2. Qt 是否可以直接访问 Repository 或长期持有 Domain 对象；
3. Service 应向 Qt 返回什么形式的数据；
4. 当前登录用户上下文放在哪里；
5. LoginWindow / MainWindow / Page / Dialog 如何划分职责；
6. 页面之间如何导航；
7. Student / Administrator 的主功能页面如何收敛；
8. 公共页面如何复用；
9. Dialog 与 Page 如何选择；
10. VolunteerRecord 状态如何映射成学生 / 管理员的工作池视图。

本阶段继续遵守此前总原则：

```text
Qt Presentation
      ↓
Application / Service
      ↓
Domain
      ↓
Repository / Persistence
```

Qt 是最终 V1.0 的表现层技术，而不是第二套业务层。

---

# 53. Gate 3.5.1：Qt Presentation 与 Service 总边界

## 53.1 Qt Presentation 的职责

### 决策 D177

> **Qt Presentation 只负责输入、展示、页面状态、导航、Signal / Slot 与用户交互，不承担积分、排行榜、徽章、状态机、最终权限判断、OperationLog、Repository 或文件持久化等核心业务职责。**

Qt 层合理职责包括：

```text
输入控件
QLineEdit / QComboBox / QDateEdit 等

展示控件
QTableWidget / QTableView / QLabel / 卡片等

页面状态
当前 Tab / 当前选中行 / 输入框状态

交互
按钮点击 / 对话框确认 / Signal / Slot

导航
页面切换 / Dialog 打开关闭

反馈
QMessageBox / 状态栏 / 错误提示
```

Qt 层明确不负责：

```text
积分计算
排行榜排序
徽章等级判定
VolunteerRecord 状态机
最终权限判断
跨对象一致性规则
CSV 解析
Repository 查询实现
OperationLog 创建
Prepare / Commit
.tmp / .bak / 文件替换
```

核心表达：

> **Qt 负责“用户想做什么、结果怎么显示”；Service / Domain 负责“这件事能不能做、怎样保证业务正确”。**

---

## 53.2 Qt 不直接访问 Repository

### 决策 D178

> **Qt 页面不得直接访问 Repository / Persistence，也不得直接持有 Repository 内部领域实体的长期 `T* / const T*`。正常调用链保持 `Qt → Service → Repository / Domain`。**

禁止出现：

```text
ReviewPage
↓
VolunteerRecordRepository.findByStatus(...)
```

或：

```text
MyVolunteerPage
↓
VolunteerRecordRepository.findById(...)
↓
直接修改 VolunteerRecord
```

原因：

1. Qt 一旦知道 Repository 细节，就会与底层存储结构耦合；
2. Qt 可能绕过 Service 中的 Permission、AccountStatus、ownership、状态机、OperationLog 和 PersistenceCoordinator；
3. 页面更换、Qt Model/View 重构时会牵动业务层；
4. Repository 返回的领域对象指针本身已经被冻结为短生命周期 non-owning view，不适合跨页面持有。

因此最终边界保持：

```text
Qt Page / Dialog
      ↓
业务 Service
      ↓
Repository / Domain
```

而不是：

```text
Qt Page
  ├── Service
  └── Repository
```

---

## 53.3 Service 向 Qt 返回查询结果值

### 决策 D179

> **Service 可以向 Qt 返回普通 C++ 的只读查询结果值对象 / 结构，用于承载排行榜条目、统计结果、日记墙展示信息、记录摘要等跨多个领域对象组合出的展示数据；这些结果类型不属于新的持久领域实体，也不建立 DTO / Mapper 继承体系等重型架构。**

例如排行榜页面真正需要的是：

```text
rank
accountId
studentName
score
validDuration
validRecordCount
rankingTitle
```

这些不是一个独立持久业务实体，而是一次查询结果。

日记墙公开卡片可能需要：

```text
postId
studentName
categoryName
serviceDate
finalDuration
place
title
displayContent
finalScore
currentBadge
currentRankingTitle
likeCount
```

这些数据来自：

```text
DiaryPost
+ VolunteerRecord
+ Student
+ VolunteerCategory
+ BadgeService
+ RankingService
+ LikeRelation 派生统计
```

不应让 Qt 自己沿着 Repository / Domain 链路拼接。

因此允许出现概念上的普通值结果类型，例如：

```text
RankingEntry
StudentProfileView
VolunteerRecordSummary
DiaryPostView
StatisticsResult
```

但当前不引入：

```text
DTOBase
MapperBase
DtoFactory
ViewModel hierarchy
```

避免为课程项目制造不必要的映射框架。

---

## 53.4 Qt 长期保存 stable ID，而不是领域对象地址

### 决策 D180

> **Qt 对具体业务对象的长期定位优先保存 stable ID（如 `accountId / recordId / postId / categoryId`），调用 Service 时传入稳定 ID，而不是保存 Repository 对象地址。**

例如管理员表格选中一条记录后，页面保存：

```text
recordId = REC000123
```

审核时概念调用：

```text
VolunteerReviewService.approve(
    actorAccountId,
    recordId,
    finalCategoryId,
    finalDuration,
    reviewNote
)
```

而不是：

```text
VolunteerRecord* selectedRecord
```

长期留在 Qt 中。

这样可以保证：

- 页面不会因 `vector` reallocation / remove / reload 等导致悬空指针；
- 详情页重新进入时可以按 ID 查询最新业务事实；
- Qt 与 Repository 内部容器类型解耦。

---

## 53.5 当前登录上下文不进入业务全局状态

### 决策 D181

> **当前登录上下文属于 Application / Presentation 生命周期，不引入业务层全局 `SessionManager`、Singleton 或隐藏 `currentUser`。Qt 可长期保存 `currentAccountId` 及必要的显示 / 能力快照；每次真正业务执行仍由 Service 根据 accountId 重新解析真实 User 并执行 Permission、AccountStatus、归属、状态和业务规则检查。**

禁止：

```text
Service
↓
SessionManager::instance().currentUser()
```

或：

```text
global User* currentUser
```

推荐：

```text
Page
↓
currentAccountId
↓
Service(..., actorAccountId, ...)
↓
UserRepository
↓
解析真实 User
↓
重新执行授权
```

---

## 53.6 capabilities 只负责 UI 入口，不代替授权

### 决策 D182

> **`UserCapabilities` 在 Qt 中用于入口、菜单和按钮的展示 / 启用控制，但 UI 能力控制不构成最终授权。任何状态修改业务必须由 Service 再次进行真实授权检查。**

两层职责：

```text
ApplicationContext.capabilities
→ Presentation 体验层
→ 决定“是否显示 / 启用入口”

Service authorization
→ 业务安全边界
→ 决定“这一次操作实际上是否合法”
```

即使某个按钮因为 UI Bug 被错误显示，Service 仍必须拒绝无权限操作。

---

## 53.7 Qt Signal / Slot 不侵入纯业务核心

### 决策 D183

> **Qt Signal / Slot 主要限制在 Presentation / Application 交互范围，不让纯 Domain、Repository 为 GUI 继承 `QObject` 或建立 Qt 信号网络；当前没有真实需求引入 Observer / 事件总线。**

合理：

```text
button.clicked
↓
onSubmitClicked()
↓
StudentVolunteerService
```

不推荐：

```cpp
class VolunteerRecord : public QObject
{
    signals:
        void approved();
};
```

当前 Domain 保持纯 C++，Repository 也不为了 UI 发 Qt signals。

---

## 53.8 UI 预校验与业务校验分层

### 决策 D184

> **Qt 可以进行空值、格式、长度、输入范围等用户体验级预校验，但 Domain / Service 必须独立重复保证正式业务不变量；不得把“按钮不可点”“输入框限制”作为唯一业务规则保护。**

例如 Qt 可以提前判断：

```text
输入为空
日期格式错误
展示文案超过界面允许长度
服务时长文本无法解析
```

但以下必须由 Service / Domain 再验证：

```text
是否本人记录
记录是否 Pending
记录是否 Approved
是否允许申请 DiaryPost
BadgeRule 是否违反唯一性
最终时长是否合法
管理员是否有对应 Permission
```

---

## 53.9 Service → Qt 的错误结果必须分级

### 决策 D185

> **Service 返回给 Presentation 的执行结果必须具有足够的错误分类语义，至少能够区分普通业务 / 校验 / 权限 / Not Found 类失败、普通持久化失败与严重 Partial Commit 持久化故障；Qt 根据分类呈现不同反馈，严重持久化故障不得被当作普通失败后继续正常写业务。**

概念结果至少需要表达：

```text
Success
BusinessError / ValidationError
PermissionDenied
NotFound
PersistenceFailure
SeverePersistenceFailure
```

当前不冻结最终 `enum / class / template` 形式，但明确不允许所有 Service 只返回：

```cpp
bool success;
```

因为 Qt 必须能区分：

```text
“当前记录已不是 Pending”
```

与：

```text
“物理文件已经部分 Commit，当前持久化状态不可继续信任”
```

严重持久化故障的 Presentation 底线：

```text
明确提示严重错误
↓
禁止新的正常写操作
↓
引导安全退出 / 重启 / Recovery 路径
```

具体故障页面形式留待 Qt 实现阶段。

---

> **实现细化 → Gate 4**
>
> 本节冻结的是 Service 错误分类由 Qt 正确消费的架构语义；对应 `Result<T>`、`OperationResult`、`ServiceError` 与 Fatal Path 的实现级规则已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.6_Result_OperationResult_ServiceError与FatalPath实现细化_候选冻结稿(1).md`

# 54. Gate 3.5.2：登录上下文与页面导航架构专项审计

本节重点解决：

```text
LoginWindow 与 MainWindow 怎么分？
当前登录身份放在哪里？
Student / Administrator 是否需要两个主窗口？
页面怎么切换？
如何避免 MainWindow God Class？
logout / 切换账号如何清理会话？
```

---

## 54.1 LoginWindow 与 MainWindow 分离

### 决策 D186

> **V1.0 将认证前后的 Qt 生命周期分离为 `LoginWindow` 与认证后的 `MainWindow`。LoginWindow 只负责登录输入与认证交互；MainWindow 只作为认证后的应用壳层，不承担登录认证和核心业务规则。**

推荐结构：

```text
Application
│
├── LoginWindow
│     └── AuthenticationService
│
└── MainWindow
      └── 认证后 Application Shell
```

不采用：

```text
MainWindow
├── 登录
├── 学生业务
├── 管理员业务
├── Repository
└── 全部 Service 调用
```

登录成功推荐流程：

```text
LoginWindow
↓
用户输入 accountId + password
↓
AuthenticationService.login(...)
↓
认证 / 状态 / Student.lastLoginAt 提交流程完成
↓
返回值语义 LoginResult
↓
建立 ApplicationContext
↓
创建 MainWindow
↓
进入认证后 UI
```

---

## 54.2 ApplicationContext 的定位

### 决策 D187

> **正式引入轻量 `ApplicationContext` 作为 Presentation / Application 层当前登录上下文值容器，保存 `currentAccountId`、显示姓名及 `UserCapabilities` 等必要会话信息。它不是业务 Service、不是 Singleton、不会被 Domain / Repository 隐式访问，也不保存 Repository 内领域实体指针。**

当前推荐内容：

```text
ApplicationContext
├── currentAccountId
├── displayName
└── UserCapabilities
```

可以根据实现需要表达是否认证，但不加入业务对象集合。

明确禁止放入：

```text
User*
Student*
Administrator*
VolunteerRecord*
Repository*
PersistenceCoordinator*
任何 Service 引用
积分缓存
排行榜缓存
DiaryPost 列表缓存
```

ApplicationContext 的职责只有：

> **表达“当前认证后的 UI 会话是谁，以及当前 UI 应显示哪些角色能力入口”。**

---

## 54.3 登录结果使用值语义，不把 User* 交给 Qt

### 决策 D188

> **AuthenticationService 登录成功后向 Presentation 返回普通值语义登录结果，而不是把 Repository 中的长期 `User*` 交给 Qt。真正业务调用继续使用稳定 `accountId`，由 Service 每次重新解析真实 User。**

概念返回：

```text
AuthenticatedUserInfo / LoginResult
├── accountId
├── displayName
└── UserCapabilities
```

这是 Qt 会话快照，不是新的权威 User。

真正执行业务仍然：

```text
currentAccountId
↓
Service
↓
UserRepository.find...
↓
真实 User
↓
Permission + AccountStatus + ownership + state + rule
```

---

## 54.4 一个 MainWindow + capabilities 驱动导航

### 决策 D189

> **V1.0 优先采用一个统一 `MainWindow`，根据 `UserCapabilities` 动态构建 / 启用 Student 与 Administrator 对应导航入口，而不是分别维护高度重复的 StudentMainWindow / AdministratorMainWindow；公共页面可自然复用。**

未采用的方案：

```text
StudentMainWindow
AdministratorMainWindow
```

该方案虽然角色分离直观，但会重复：

```text
窗口壳层
顶部用户信息
logout
导航
公共 DiaryWallPage
公共 RankingPage
页面容器管理
```

推荐方案：

```text
MainWindow
↓
根据 ApplicationContext.capabilities 生成入口
```

例如 Student：

```text
首页
我的志愿
积分与荣誉
日记墙
排行榜
个人中心
```

Administrator：

```text
首页
志愿审核 / 治理
学生管理
规则配置
日记墙治理
全局统计
排行榜
操作日志
导出
```

不是通过大量 `if (role == "admin")` 来完成，而是由稳定业务能力决定入口。

---

## 54.5 MainWindow 定位为 Application Shell

### 决策 D190

> **MainWindow 定位为 Application Shell，只承担用户信息展示、导航、页面容器和 logout 等应用级职责。核心业务调用由具体 Page / Dialog 通过对应 Service 执行，不把全部 Service 调用和业务刷新集中进 MainWindow。**

MainWindow 可以负责：

```text
顶部当前用户信息
导航栏
PageId → 页面实例映射
QStackedWidget 页面容器
页面切换
页面懒加载入口
logout
严重持久化故障后的全局写操作禁用入口
```

MainWindow 不负责：

```text
审核算法
积分计算
志愿记录状态改变
CSV 保存
Badge 判定
排行榜排序
管理员治理细节
```

核心思想：

> **MainWindow 是“壳”，不是系统总控制器。**

---

## 54.6 页面容器与页面间导航

### 决策 D191

> **认证后主要页面优先组织在统一页面容器（当前推荐 `QStackedWidget`）中。页面之间只进行导航、必要稳定 ID / 简单参数传递和刷新通知，不互相调用对方的业务实现；具体页面是否懒加载留待实现级策略，但当前默认优先懒加载。**

推荐：

```text
MainWindow
│
├── Navigation
│
└── QStackedWidget
    ├── DashboardPage
    ├── MyVolunteerPage / ReviewPage
    ├── DiaryWallPage
    ├── RankingPage
    └── ...
```

禁止页面之间形成网状业务调用：

```text
ReviewPage
↓
直接调用 StudentManagementPage 的业务函数
```

页面之间只允许：

```text
导航请求
稳定 ID 参数
刷新请求 / 激活通知
```

---

## 54.7 跨页面业务目标继续使用 stable ID

### 决策 D192

> **Qt 页面和 Dialog 在跨页面、跨交互生命周期传递业务目标时使用稳定 ID，不传递或长期保存 Repository 中的领域对象裸指针。详情页面需要最新事实时通过 Service 按 ID 重新查询。**

例如：

```text
ReviewPage
↓
用户选中 REC000123
↓
打开 ReviewVolunteerDialog(recordId = REC000123)
↓
Dialog / Service 查询最新事实
```

而不是：

```text
ReviewPage
↓
VolunteerRecord*
↓
传到 Dialog
```

---

## 54.8 logout 与切换账号

### 决策 D193

> **logout 采用销毁认证后 UI 会话树并清除 ApplicationContext 的方式恢复到 LoginWindow，避免旧页面、旧 capabilities 和上一账号展示数据残留。账号切换不另建复杂机制，统一通过 logout → login 完成。**

推荐过程：

```text
用户点击 Logout
↓
停止当前认证后交互
↓
销毁 MainWindow
↓
连带销毁全部已创建 Page / Dialog
↓
销毁 ApplicationContext
↓
重新显示 / 创建 LoginWindow
```

账号切换：

```text
logout
↓
login
```

不额外设计复杂 `switchUser()`。

---

## 54.9 应用组合根与 Service 生命周期

### 决策 D194

> **Repository、PersistenceCoordinator 和业务 Service 由应用组合位置统一创建并管理其运行期生命周期；页面通过显式构造 / 依赖传递获得所需 Service 引用，不自行创建重复 Service，也不引入 DI Framework 或 Singleton。可以采用轻量 `Application / AppController` 负责启动、对象组装、LoginWindow / MainWindow 生命周期及 logout 重建流程，但其不得承担领域业务。**

推荐全局生命周期：

```text
main()
  ↓
Application / AppController
  │
  ├── Repository startup / bootstrap
  ├── PersistenceCoordinator
  ├── Services
  ├── LoginWindow
  └── 登录成功后：ApplicationContext + MainWindow
```

页面创建时只注入真实需要的 Service：

```text
ReviewPage
├── const ApplicationContext&
└── VolunteerReviewService&
```

而不是：

```text
EveryPage
└── AllServices / GlobalContainer
```

Application / AppController 负责的是：

> **应用生命周期与对象装配。**

它不负责：

```text
审核
点赞
积分
排行榜
徽章
规则修改
```

---

# 55. Gate 3.5.2 实现级注意事项

以下内容不新增新的 D 编号，但作为 D186～D194 的 V1.0 实现约束记录。

## 55.1 ApplicationContext 所有权与销毁顺序

`ApplicationContext` 由 `Application / AppController` 持有。

页面只拿非拥有只读引用或只读访问方式，不取得其生命周期所有权。

logout 必须遵守：

```text
销毁 MainWindow（包括所有已创建 Page / Dialog）
↓
销毁 ApplicationContext
↓
重新进入 LoginWindow
```

不能先销毁 Context 再保留引用它的页面，否则会制造悬空引用。

---

## 55.2 主 Page 默认优先懒加载

V1.0 当前默认实现策略：

```text
第一次导航到 PageId
↓
MainWindow 判断页面尚未创建
↓
创建 Page
↓
显式注入 ApplicationContext / 必要 Service
↓
加入 QStackedWidget
↓
以后复用该页面实例
```

这样可以避免 MainWindow 启动时一次性构造全部主 Page，并避免所有页面同时执行数据查询。

但这是默认策略，不是业务正确性的硬约束。极轻量、启动必需页面可例外提前创建。

---

## 55.3 导航由 MainWindow 统一处理

页面不直接操作 `QStackedWidget` 的物理 index。

不推荐：

```cpp
stackedWidget->setCurrentIndex(7);
```

因为页面会依赖另一个页面的物理位置。

推荐概念：

```text
enum class PageId
```

页面发出：

```text
navigateRequested(PageId)
```

或调用统一导航接口：

```text
navigateTo(PageId)
```

由 MainWindow / Application Shell 负责：

```text
权限入口检查
页面是否已创建
懒加载
页面激活
刷新
QStackedWidget 切换
```

`PageId` 的完整枚举内容等页面体系冻结后在实现阶段确定。

---

## 55.4 Student.lastLoginAt 持久化失败时不能进入 MainWindow

Student 登录完整语义不是“密码正确就成功”。

正确流程：

```text
账号存在
↓
密码验证成功
↓
AccountStatus == Active
↓
对 UserRepository 建立操作前快照
↓
Student 更新 lastLoginAt
↓
Prepare
↓
Commit
↓
成功后才向 Qt 返回正常登录成功
```

如果 Prepare 失败：

```text
恢复 UserRepository 快照
↓
AuthenticationService 返回可恢复 PersistenceFailure
↓
不进入 MainWindow
```

如果出现 Partial Commit Failure：

```text
SeverePersistenceFailure
↓
进入已冻结的严重持久化故障路径
↓
不得进入正常 MainWindow
```

认证失败（账号不存在、密码错误、Disabled）时不得更新 `lastLoginAt`。

Administrator 当前没有 `lastLoginAt` 字段，因此不存在该写回步骤。

---

## 55.5 公共页面采用 UI + Service 双层授权

例如 `DiaryWallPage` 为 Student / Administrator 共用。

UI 层：

```text
ApplicationContext.capabilities
↓
决定 Like / Moderate 等入口是否显示 / enable
```

Service 层：

```text
actorAccountId
↓
重新解析真实 User
↓
Permission
AccountStatus
ownership
DiaryPost status
业务规则
```

两者必须同时存在。

> **UI 隐藏不是安全边界。**

---

## 55.6 Page / Dialog 通过构造依赖获取 Service

`ApplicationContext` 只保存身份上下文，不保存业务 Service。

禁止把它写成：

```text
ApplicationContext
├── currentAccountId
├── capabilities
├── DiaryService*
├── RankingService*
└── UserRepository*
```

否则会退化为 Service Locator。

推荐：

```text
DiaryWallPage(
    const ApplicationContext& context,
    DiaryService& diaryService
)
```

每个页面只得到真实需要的依赖。

---

# 56. Gate 3.5.3：页面体系与功能模块映射审计

本节不冻结视觉主题、按钮颜色、具体控件尺寸，而是冻结：

> **哪些功能应该是主 Page，哪些应该是 Dialog，哪些应该合并，哪些页面需要 Student / Administrator 公共复用。**

页面组织遵循：

```text
A. Application Shell
→ LoginWindow / MainWindow

B. 主功能 Page
→ 长期存在于认证后主窗口中的稳定业务区域

C. 局部 Dialog
→ 针对单个对象的一次创建 / 编辑 / 审核 / 确认

D. 公共复用 Page
→ Student / Administrator 均可进入
```

核心原则：

```text
不采用“一按钮一个 Page”
不采用“全部业务一个超级 Page”
领域对象数量 ≠ Qt 页面数量
```

---

## 56.1 页面总体组织原则

### 决策 D195

> **V1.0 Qt 页面按“主功能 Page + 局部业务 Dialog + 公共复用 Page + Application Shell”组织，不采用“一按钮一个页面”或“全部功能集中一个页面”的极端结构；领域对象数量与 Qt 页面数量不存在机械一一对应关系。**

例如：

```text
VolunteerCategory
BadgeRule
Semester
```

是三个独立领域概念，但 UI 可以统一进入：

```text
ConfigurationPage
├── Category Tab
├── BadgeRule Tab
└── Semester Tab
```

同理 `LikeRelation` 是独立领域关系对象，但绝不需要 `LikeRelationPage`。

---

## 56.2 Student 主功能页面收敛

### 决策 D196

> **学生端主功能页面当前收敛为 `StudentDashboardPage`、`MyVolunteerPage`、`AchievementPage`、`ProfilePage`，并复用公共 `DiaryWallPage` 与 `RankingPage`。学生志愿记录的不同状态通过 MyVolunteerPage 内筛选和状态驱动操作表达，不为 Pending / Rejected / Approved 分别建立独立主页面。**

推荐学生导航：

```text
Student Navigation
├── 首页 / 概览
├── 我的志愿
├── 积分与荣誉
├── 日记墙
├── 排行榜
└── 个人中心
```

### StudentDashboardPage

轻量概览，不成为业务中心。

可展示：

```text
当前月积分
当前学期积分
总积分

当前月排名
当前学期排名
总榜排名

最近记录状态摘要
当前专项徽章摘要
```

数据来自已有 Service，不在 Dashboard 自己计算。

### MyVolunteerPage

统一承载学生志愿记录管理：

```text
查询自己的记录
状态筛选
新建
修改 Pending
撤回 Pending
查看 Rejected 原因
修改 Rejected 并重新提交
查看 Approved
Approved 申请 DiaryPost
查看 Withdrawn
```

状态驱动 UI，而不是不同状态各建立一个主页面。

### AchievementPage

统一表达“我的当前志愿成果”，可以采用 Tab / 子区域：

```text
积分概览
专项徽章
排行称号
```

与公共 `RankingPage` 分工：

```text
AchievementPage
→ 我的结果

RankingPage
→ 全局榜单
```

### ProfilePage

展示 Student：

```text
accountId
name
className
major
contact
accountStatus
createdAt
lastLoginAt
```

Student 自己只允许修改：

```text
contact
password
```

身份字段仍由管理员维护。

---

## 56.3 VolunteerRecord 创建 / 编辑 / 详情使用 Dialog

### 决策 D197

> **志愿记录新建与可修改场景优先复用一个 `VolunteerRecordDialog` 的不同模式；记录详情使用局部详情 Dialog。单次输入 / 编辑 / 确认型操作优先采用 Dialog，不占用 MainWindow 顶层导航。**

概念：

```text
VolunteerRecordDialog
├── Create mode
└── Edit mode
```

共有字段：

```text
category
serviceDate
duration
place
verifier
summary
```

Create 与 Edit 只是操作语义不同，不复制两个窗口类。

`VolunteerRecordDetailDialog` 可以复用展示：

```text
申报信息
当前状态
当前审核结果
finalCategory
finalDuration
finalScore
reviewNote
```

但具体可见操作仍由调用场景 + capabilities + Service 授权决定。

---

## 56.4 Administrator 主功能页面收敛

### 决策 D198

> **管理员主功能页面当前收敛为 `AdminDashboardPage`、`ReviewPage`、`StudentManagementPage`、`ConfigurationPage`、`DiaryModerationPage`、`StatisticsPage`、`OperationLogPage`，同时复用公共 `DiaryWallPage` 与 `RankingPage`。**

推荐管理员导航：

```text
Admin Navigation
├── 首页 / 全局概览
├── 志愿审核 / 治理
├── 学生管理
├── 规则配置
├── 日记墙治理
├── 全局统计
├── 排行榜
├── 操作日志
└── 数据导出
```

其中数据导出使用 Dialog，不作为永久主 Page。

### AdminDashboardPage

与 StudentDashboardPage 业务内容明显不同：

```text
待审核记录数
当前学生数
当前 Approved 记录数
当前总服务时长
类别分布摘要
当前月 / 学期榜概览
```

当前不强行建立 `BaseDashboardPage` 继承层次。

---

## 56.5 审核与已通过记录治理归一个业务模块

### 决策 D199

> **志愿审核、已通过记录强制更正和物理删除均归属于志愿审核 / 治理模块；ReviewPage 负责列表与入口，一次审核、更正、删除原因确认等采用相应 Dialog，不为每项治理动作建立独立主页面。**

ReviewPage 主要承担：

```text
Pending 工作队列
查询
选中记录
打开详情
发起审核
Approved 治理查询入口
```

一次审核：

```text
ReviewVolunteerDialog
├── appliedCategory
├── appliedDuration
├── finalCategory
├── finalDuration
├── reviewNote
├── Approve
├── Reject
└── Cancel
```

已通过记录强制更正：

```text
CorrectApprovedRecordDialog
├── before
├── after
└── reason
```

物理删除：

```text
确认 Dialog
+
删除原因
```

正常发现问题优先走：

```text
Pending → Rejected → Student 修改 → Pending
```

Approved 后异常才走强制治理。

---

## 56.6 ConfigurationPage 三 Tab，但 Service 边界不合并

### 决策 D200

> **管理员的 VolunteerCategory、BadgeRule 与 Semester 配置统一放入 `ConfigurationPage` 的不同 Tab / 子区域，由 `RuleConfigurationService` 与 `SemesterService` 分别承接业务；UI 合并不改变三个领域概念及其 Service 边界。**

结构：

```text
ConfigurationPage
├── VolunteerCategory Tab
├── BadgeRule Tab
└── Semester Tab
```

Category Tab：

```text
查看类别
新增类别
修改 coefficient
启用 / 停用
→ RuleConfigurationService
```

BadgeRule Tab：

```text
badgeName
category
bronze
silver
gold
enabled
新增 / 修改 / 停用
→ RuleConfigurationService
```

Semester Tab：

```text
学期列表
新增 / 修改时间范围
切换 currentSemesterId
→ SemesterService
```

三个 Tab 共处一个 Page 只是导航收敛，不意味着三个领域职责混合。

---

## 56.7 DiaryWallPage 与 DiaryModerationPage 分开

### 决策 D201

> **`DiaryWallPage` 作为 Student / Administrator 公共公开内容页面，负责浏览、筛选和公开互动；后台展示审核与治理采用独立 `DiaryModerationPage`，避免把公开信息流与后台治理流程混成一个页面。**

`DiaryWallPage`：

```text
卡片信息流
按类别筛选
按正式公开时间倒序
Like / Unlike（Student 能力）
公共浏览
```

`DiaryModerationPage`：

```text
PendingDisplayReview
查看 title / displayContent
关联 VolunteerRecord 事实
Approve / Reject 展示
Displayed 内容治理
TakeDown
```

学生对 Approved VolunteerRecord 申请展示时不建立永久“发布页面”，而是：

```text
MyVolunteerPage
↓
Approved record
↓
DiaryPostRequestDialog
↓
填写 title + displayContent
```

保持 DiaryPost 必须来源于真实 Approved VolunteerRecord。

---

## 56.8 RankingPage 与 StatisticsPage 分离

### 决策 D202

> **`RankingPage` 为公共复用页面，通过月度 / 学期 / 总榜三个 Tab 或等价子区域展示排行榜；管理员全局统计另由 `StatisticsPage` 承担，排行榜与聚合统计保持职责分离。**

RankingPage：

```text
月度榜
学期榜
总积分榜
```

典型列：

```text
rank
accountId
name
score
validDuration
validRecordCount
rankingTitle
```

StatisticsPage：

```text
全部学生积分概览
各类别记录数
各类别服务时长
全局服务总时长
必要统计图
```

两者不要互相复制算法。

---

## 56.9 Export 使用 Dialog

### 决策 D203

> **数据导出属于一次性工具型操作，当前优先采用 `ExportDialog` 而不是永久主 Page；OperationLog 查询仍由独立 `OperationLogPage` 承担，日志页面不自行实现底层文件导出。**

ExportDialog 概念内容：

```text
选择数据类型
选择时间 / 类别 / 学生等适用筛选条件
选择输出格式
选择目标位置
执行 ExportService
```

当前只存在稳定 `ExportData` Permission，不因为不同导出类型再拆一组权限。

OperationLogPage 只负责：

```text
查询
按时间
按管理员
按 OperationType
查看详情
```

OperationLog 不允许修改 / 删除。

---

## 56.10 每个 Page 只依赖真实需要的 Service

### 决策 D204

> **每个 Page / Dialog 只显式依赖自身真实需要的业务 Service，不统一注入全部 Service，也不通过 ApplicationContext 获取 Service。Dashboard 等纯展示页面可以组合调用多个现有只读 Service，不因为 UI 聚合展示立即创建新的 DashboardService。**

推荐映射：

```text
StudentDashboardPage
→ StatisticsService
→ RankingService
→ BadgeService

MyVolunteerPage
→ StudentVolunteerService

AchievementPage
→ StatisticsService
→ RankingService
→ BadgeService

ProfilePage
→ UserManagementService

DiaryWallPage
→ DiaryService

RankingPage
→ RankingService

AdminDashboardPage
→ StatisticsService

ReviewPage
→ VolunteerReviewService

StudentManagementPage
→ UserManagementService

ConfigurationPage
→ RuleConfigurationService
→ SemesterService

DiaryModerationPage
→ DiaryService

StatisticsPage
→ StatisticsService
→ RankingService

OperationLogPage
→ OperationLogService

ExportDialog
→ ExportService
```

当前不新建 `DashboardService`，除非未来真实发现 Dashboard 查询组合已经形成独立稳定的应用查询职责。

---

## 56.11 页面不是第二份业务真相

### 决策 D205

> **Page / Dialog 不直接实现跨领域核心业务算法；UI 页面主要负责输入、选择、调用 Service、消费结果和刷新。页面状态变化后优先重新通过 Service 获取当前权威查询结果，而不是长期维护与 Repository 并行的业务数据副本。**

例如审核成功后不应只：

```text
Qt 自己把当前表格某一行的 status 改成 Approved
```

然后假设全系统已经同步。

推荐：

```text
Service 操作成功
↓
Page 重新查询
↓
消费新的 query result
↓
重绘列表 / 卡片
```

Repository 仍然是运行期权威领域事实来源。

---

# 57. Gate 3.5.3 实现级注意事项与页面全景图

## 57.1 当前 V1.0 主 Page 清单

当前主 Page 收敛为约 13 个：

```text
Student / Common
1. StudentDashboardPage
2. MyVolunteerPage
3. AchievementPage
4. ProfilePage
5. DiaryWallPage            [Common]
6. RankingPage              [Common]

Administrator
7. AdminDashboardPage
8. ReviewPage
9. StudentManagementPage
10. ConfigurationPage
11. DiaryModerationPage
12. StatisticsPage
13. OperationLogPage
```

公共页面：

```text
DiaryWallPage
RankingPage
```

不因为 Student / Administrator 都能使用就复制两个页面版本。

---

## 57.2 当前主要 Dialog 清单

```text
VolunteerRecordDialog
VolunteerRecordDetailDialog
DiaryPostRequestDialog
ReviewVolunteerDialog
CorrectApprovedRecordDialog
StudentCreate / Edit Dialog
PasswordResetDialog
CategoryEditDialog
BadgeRuleEditDialog
SemesterEditDialog
ExportDialog
```

具体是否进一步合并某些 Student Dialog，留待实现时根据表单复用程度决定，但不为每一个按钮建立永久主页面。

---

## 57.3 页面默认懒加载与导航激活刷新

主 Page 当前默认策略：

```text
navigateTo(PageId)
↓
若页面不存在：lazy create
↓
注入 Context / Service
↓
加入 QStackedWidget
↓
调用 refresh() / onActivated()（具体名后定）
↓
显示页面
```

页面数据默认采用：

> **进入页面 / 导航激活时重新通过 Service 查询当前最新数据。**

不建议基于 Qt 控件焦点 `focusInEvent()` 作为业务刷新核心，因为“页面获得焦点”与“业务页面被导航激活”不是同一概念。

当前数据规模下优先采用简单刷新，不建立复杂：

```text
跨页面缓存
缓存版本号
Observer 总线
全局 invalidate 事件系统
```

Gate 3.5.4 将继续细化具体刷新机制与 QTableWidget / QTableView 选择。

---

## 57.4 公共页面受限操作的双层保障

`DiaryWallPage` 等公共页面内部：

```text
UI 层
ApplicationContext.capabilities
→ 是否展示 Like / Moderate 等入口

Service 层
actorAccountId
→ 重新鉴权
→ AccountStatus
→ target status
→ ownership / business rule
```

`RankingPage` 当前主要为只读，不因为其为公共页面而人为拆权限。

如果未来在公共页面增加管理员工具入口，仍采用同样双层保障。

---

## 57.5 StudentDashboardPage / AdminDashboardPage 不强造继承

当前两者业务内容明显不同。

因此：

```text
StudentDashboardPage
≠
AdminDashboardPage 的派生兄弟必须共用 BaseDashboard
```

当前不建立：

```text
BaseDashboardPage
├── StudentDashboardPage
└── AdminDashboardPage
```

只有后续真实代码出现稳定、足够大的共同可替换行为时再评估。

少量 UI 布局复用可以通过：

```text
辅助函数
组合 Widget
公共样式
```

解决，不需要为了“体现继承”制造 Qt 页面继承层次。

---

## 57.6 ConfigurationPage 内部 Tab 边界

推荐：

```text
ConfigurationPage
│
├── Category Tab
│   └── CategoryEditDialog
│       └── RuleConfigurationService
│
├── BadgeRule Tab
│   └── BadgeRuleEditDialog
│       └── RuleConfigurationService
│
└── Semester Tab
    └── SemesterEditDialog
        └── SemesterService
```

各 Tab 不因为共处同一 Page 就直接跨业务域操作其他 Service。

---

## 57.7 ExportDialog 权限边界

进入 Export 功能：

```text
ApplicationContext.capabilities.has(ExportData)
→ UI 是否提供导出入口
```

执行：

```text
ExportService
→ 重新解析 Administrator
→ 再检查 ExportData
→ 执行对应数据导出
```

当前不同导出数据类型不拆分 Permission。

如果未来业务明确出现不同管理员导出范围差异，再重新审计权限粒度。

---

## 57.8 Dialog 生命周期结果统一约定

Dialog 统一遵循 Qt 自身生命周期语义：

```text
Accepted
Rejected
```

推荐规则：

```text
Cancel / 关闭
→ Rejected
→ 不发生业务成功

用户提交
↓
Service 执行业务
↓
业务成功
→ Dialog Accepted
→ 调用 Page 根据 exec() 结果刷新

业务失败
→ Dialog 保持打开
→ 展示 ServiceResult 分类错误
→ 不伪装成 Accepted
```

如果某类 Dialog 需要额外返回值，可以使用专用值结果；不建立万能 `UniversalDialogResult<T>` 继承 / 模板体系。

---

## 57.9 登录 / 导航 / 页面 / Service 总体架构图

```text
main()
  │
  ▼
Application / AppController
  │
  ├────────────── Bootstrap / Repository / Persistence
  │
  ├────────────── construct Services
  │
  ▼
LoginWindow
  │
  │ AuthenticationService
  ▼
LoginResult（value）
  │
  ▼
ApplicationContext
(accountId / displayName / capabilities)
  │
  ▼
MainWindow  <<Application Shell>>
  │
  ├── Navigation
  │       │
  │       └── PageId / navigateTo(...)
  │
  └── QStackedWidget
          │
          ├── Student pages
          ├── Admin pages
          └── Common pages

Page / Dialog
  │
  ├── const ApplicationContext&
  ├── stable IDs
  └── only-required Service references
          │
          ▼
       Service
          │
          ▼
   Domain / Repository
```

---

## 57.10 Page / Dialog / Domain 不同层次关系图

```text
领域对象层：
Student
VolunteerRecord
VolunteerCategory
BadgeRule
Semester
DiaryPost
LikeRelation
OperationLog

        ≠ 一一对应

Qt 主功能层：
StudentDashboardPage
MyVolunteerPage
AchievementPage
ProfilePage
ReviewPage
StudentManagementPage
ConfigurationPage
DiaryWallPage
DiaryModerationPage
RankingPage
StatisticsPage
OperationLogPage

        +

局部交互 Dialog：
VolunteerRecordDialog
ReviewVolunteerDialog
CategoryEditDialog
...
```

说明：

> **领域对象按业务生命周期拆分，页面按用户任务和交互流程拆分，两者不存在机械一一对应。**

---

# 58. Gate 3.5.3 扩展：VolunteerRecord 状态工作池 / 逻辑视图

本节来自对 VolunteerRecord “待审核 / 被退回 / 已通过分别放不同库”的重新审计。

用户体验层面的“不同库”直觉是合理的：不同状态确实对应完全不同的工作流；但若在物理持久化层真的拆成多个 CSV / Repository，会把简单状态转换升级为跨文件迁移，并增加重复 ID、丢失记录、Partial Commit 等风险。

因此最终吸收为：

> **一个权威 VolunteerRecord 数据集 + 多个按状态过滤形成的逻辑业务池 / 视图。**

---

## 58.1 不按状态拆多个物理数据库

### 决策 D206

> **`VolunteerRecord` 继续只存在一个权威 Repository / 持久化集合，不按 Pending / Rejected / Approved / Withdrawn 拆分多个物理数据库或 CSV 文件；在业务查询与 Qt 展示层，根据 `RecordStatus` 形成四个逻辑工作池 / 视图：待审核池、被退回池、已通过池、已撤回池。状态池只是对同一权威数据集的筛选结果，不形成第二份业务数据，也不改变 D51～D56 已冻结的 VolunteerRecord 状态机。**

权威存储继续保持：

```text
VolunteerRecordRepository
└── volunteer_records.csv
```

业务视图：

```text
VolunteerRecordRepository
│
├── Pending Pool
├── Rejected Pool
├── Approved Pool
└── Withdrawn Pool
```

这些 Pool 本质是：

```text
same authoritative collection
+
RecordStatus filter
```

而不是独立 Repository / 独立 CSV。

---

## 58.2 为什么不物理拆库

若拆成：

```text
pending_records.csv
rejected_records.csv
approved_records.csv
withdrawn_records.csv
```

那么原本简单的：

```text
Pending → Approved
```

会变成：

```text
从 pending_records 删除
+
向 approved_records 新增
```

会引入：

```text
跨文件搬迁
recordId 跨文件全局唯一检查
新增 / 删除顺序问题
同一记录重复存在风险
记录两边都不存在风险
更大的 Partial Commit 故障面
Rejected → Pending 再次跨文件迁移
每新增状态就需要新增文件
```

当前单权威集合只需要：

```text
REC000123
status = Approved
```

对象身份始终不变。

因此：

> **状态是 VolunteerRecord 的业务属性，不是决定其物理存储位置的文件分类器。**

---

## 58.3 Student 侧四状态工作池

`MyVolunteerPage` 推荐直接把 D206 映射成四个状态视图 / Tab：

```text
┌────────────────────────────────────────┐
│ 我的志愿                               │
├────────┬────────┬────────┬─────────────┤
│ 待审核 │ 被退回 │ 已通过 │ 已撤回      │
└────────┴────────┴────────┴─────────────┘
```

### Pending Pool

允许：

```text
查看
修改
撤回
```

### Rejected Pool

允许：

```text
查看当前驳回原因
修改
重新提交
```

重新提交：

```text
Rejected
↓
修改 + resubmit
↓
Pending
```

不是从一个物理库搬到另一个物理库，而是同一权威对象发生受控状态转换。

### Approved Pool

允许 Student：

```text
查看
申请 DiaryPost（若尚不存在）
```

Student 不允许随意修改 Approved。

管理员对 Approved 的异常处理属于：

> **已通过记录治理**

而不是“调整志愿权限”。

### Withdrawn Pool

当前只允许：

```text
查看
```

因为 D52 没有冻结 Withdrawn → Pending 的重新提交路径，Qt 不得擅自提供“恢复 / 重提”按钮。

---

## 58.4 Administrator 侧审核工作池与治理区

管理员 `ReviewPage` 推荐重点呈现：

```text
ReviewPage
├── Pending 审核工作池
└── Approved 治理区
```

Pending：

```text
随时取出记录
查看详情
Approve
Reject
```

Approved：

```text
查询
必要时 CorrectApprovedRecord
必要时 DeleteVolunteerRecord
```

Rejected 一般不是管理员日常主工作队列，因为已经回到 Student 处理；如有审计 / 查询需要，可以作为筛选条件查看。

Withdrawn 同理主要作为查询事实，不进入日常审核队列。

---

## 58.5 VolunteerRecord 工作流全景图

```text
                         Student Submit
                               │
                               ▼
                      ┌────────────────┐
                      │  Pending Pool  │
                      │   待审核池     │
                      └───────┬────────┘
                              │
                    Administrator Review
                              │
                 ┌────────────┴────────────┐
                 │                         │
                 ▼                         ▼
        ┌────────────────┐        ┌────────────────┐
        │ Rejected Pool  │        │ Approved Pool  │
        │   被退回池     │        │   已通过池     │
        └───────┬────────┘        └───────┬────────┘
                │                         │
         Student 查看原因                 ├── Student 查看
         修改 / 重新提交                  ├── 申请 DiaryPost
                │                         ├── Statistics / Ranking / Badge
                │                         └── Admin Governance
                ▼
          Pending Pool

另外：

Pending
  │
  └── Student Withdraw
          │
          ▼
   ┌────────────────┐
   │ Withdrawn Pool │
   │   已撤回池     │
   └────────────────┘
```

这张图同时表达：

```text
业务状态机
+
Student 工作视图
+
Administrator 工作队列
```

但不表示多个物理数据库。

---

## 58.6 Repository 查询视角

概念上 Repository 可以支持数据访问条件：

```text
findByStatus(Pending)
findByStatus(Rejected)
findByStatus(Approved)
findByStatus(Withdrawn)
```

以及与 owner / date / category 等数据条件组合的查询。

这些仍然遵守 D135：

> Repository 只表达数据定位条件，不把“能不能审核”“能不能发布日记墙”等业务规则塞入 Repository。

例如：

```text
Repository
→ 找出 Approved 记录

StudentVolunteerService / DiaryService
→ 判断是不是本人、是否已经存在 DiaryPost、账号是否 Active 等
```

---

# 58A. Gate 3.5.4A：Qt 表格型页面的数据展示方案审计

本节承接 Gate 3.5.1～3.5.3 已冻结的 Qt Presentation 边界与页面体系，进一步解决表格型页面如何消费 Service 查询结果、如何绑定 stable ID、何时采用 QTableWidget、何时允许升级到 QTableView / QAbstractTableModel / QSortFilterProxyModel，以及“为了优秀档主动使用高级技术”与“避免无收益技术堆砌”之间的统一判断标准。

本节不把“简单”本身当作最高目标，也不把“高级”本身当作加分理由。最终 V1.0 的技术选择以**实际效果和综合收益**为最高判断标准。

---

## 58A.1 表格型页面范围

当前明显具有列表 / 表格展示需求的页面包括：

```text
MyVolunteerPage
ReviewPage
StudentManagementPage
ConfigurationPage
RankingPage
StatisticsPage（部分数据区）
OperationLogPage
DiaryModerationPage
```

它们的共同数据链路是：

```text
Service query
↓
ordinary C++ read-only result values
↓
Qt Page
↓
Table / View
↓
用户选择某条展示记录
↓
stable ID
↓
Service business operation / detail query
```

因此继续冻结：

> **Qt 表格是查询结果的展示载体，不是领域数据容器，也不是运行时第二份业务真相。**

---

## 58A.2 高级技术效果准入原则

此前项目一直坚持“不为了评分强行套技术”。该原则继续有效，但从 v2.0 开始进一步精化：

> **可以主动使用高级技术争取优秀档、展示效果和技术完整度；但高级方案必须在最终效果、用户体验、展示 / 评分价值、架构收益或扩展性上形成可解释优势。若高级方案没有明显效果优势，却显著增加代码冗余、运行负担、调试成本和失败风险，则应回退到更简单稳定的方案。**

统一技术比较模型：

```text
高级技术候选
      │
      ▼
┌──────────────────────────────┐
│        综合效果准入审计      │
├──────────────────────────────┤
│ 1. 用户最终看到的效果更好吗？│
│ 2. 交互体验明显提升吗？      │
│ 3. 答辩 / 评分展示价值更高吗？│
│ 4. 架构或扩展性明显更好吗？  │
│ 5. 新增复杂度是否值得？      │
│ 6. 性能 / 调试风险可接受吗？ │
└──────────────┬───────────────┘
               │
      ┌────────┴────────┐
      ▼                 ▼
明显综合优势        无明显综合优势
      │                 │
      ▼                 ▼
采用高级方案      采用简单稳定方案
```

可以把判断理解为：

```text
技术价值
=
用户体验提升
+ 展示 / 评分价值
+ 架构收益
+ 扩展性收益
-
复杂度成本
-
性能成本
-
调试风险
```

因此，本项目今后的 Qt Model/View、Proxy、Delegate、动画、自定义 Widget、设计模式和模板等均采用同一准入原则。

### 关于“高级感”的特别说明

课程项目与大型生产系统的评价维度并不完全相同。只要最终实现质量良好、用户体验真实改善、代码结构清楚且能够在答辩中完整解释，那么：

```text
QTableView
+
自定义 QAbstractTableModel
+
QSortFilterProxyModel
```

即使不是“非它不可”，其技术展示价值也可以被计入收益。

但如果结果只是：

```text
新增大量 XXXTableModel
代码量显著膨胀
界面效果基本不变
Bug / 调试成本上升
程序更慢或更难维护
```

则高级方案不通过准入，应使用更直接的方案。

---

## 58A.3 QTableWidget 与 QTableView / Model/View 的候选关系

### 方案 A：QTableWidget

适合：

```text
数据量较小
列结构固定
页面主要做简单展示 / 选择 / CRUD 入口
刷新直接
无需多个 View 共享同一模型
```

典型优势：

```text
实现快
调试简单
课程时间成本低
与 Service Result 映射直接
```

如果最终高级 Model/View 没有形成明显优势，则：

> **QTableWidget + stable ID + 统一展示映射原则 + 良好刷新 UX**

就是高质量方案，而不是“低级方案”。

### 方案 B：QTableView + QAbstractTableModel / QSortFilterProxyModel

适合真实出现：

```text
多条件筛选
频繁排序
较大量行
动态列 / 复杂展示
多个 View 复用同一模型
模型级刷新明显更清楚
Proxy 可以显著改善交互
```

若例如 `StudentManagementPage`、`OperationLogPage`、`ReviewPage` 后续实际设计出现明显的搜索 / 多字段筛选 / 表头排序需求，允许针对这些页面采用更高级 Model/View，而不要求所有页面统一升级。

### 不追求所有页面技术形式统一

允许：

```text
ConfigurationPage
→ QTableWidget

RankingPage
→ QTableWidget 或轻量 QTableView

OperationLogPage
→ 若复杂筛选形成真实优势，可 QTableView + Proxy

StudentManagementPage
→ 根据实际交互复杂度决定
```

不允许为了“统一”而：

```text
所有页面强制 QTableWidget
```

也不允许：

```text
所有页面强制建立 XXXTableModel
```

---

## 58A.4 决策 D207——表格技术采用效果准入，而非预设唯一方案

> **D207：V1.0 的 Qt 表格展示不预先限定必须采用 `QTableWidget` 或 `QTableView + QAbstractTableModel`。高级 Qt Model/View 技术允许作为优秀档、用户体验和技术展示提升手段主动采用，但必须针对真实页面证明其在排序、筛选、数据更新、交互体验、代码组织、答辩展示或扩展性上的明显综合优势。若高级方案没有形成明显优势，反而显著增加代码冗余、调试成本、运行负担或失败风险，则该页面采用 `QTableWidget + stable ID + 统一展示映射原则` 作为更优实现。**

---

## 58A.5 决策 D208——表格只消费 Service 查询结果

> **D208：所有 Qt 表格消费 Service 返回的普通 C++ 只读查询结果值，而不是直接持有 Repository 内领域对象、领域对象指针或 Repository 容器。表格仅承担 Presentation 展示，不成为新的业务数据源。**

例如：

```text
StudentVolunteerService
↓
vector<VolunteerRecordSummary>
↓
MyVolunteerPage
↓
Table/View
```

不采用：

```text
MyVolunteerPage
↓
vector<VolunteerRecord*>
↓
长期绑定 Repository 对象地址
```

---

## 58A.6 决策 D209——stable ID 是行身份，row index 不是业务身份

> **D209：Qt 表格行必须通过 stable ID 与具体业务对象建立关联。`recordId / accountId / postId / categoryId / logId` 等可绑定到 `Qt::UserRole` 或等价隐藏数据中；row index、姓名、日期、类别名称等显示位置或文本不得承担业务身份。**

推荐：

```text
可见列：
日期 | 类别 | 时长 | 状态 | 积分

隐藏业务定位：
recordId → Qt::UserRole
```

禁止：

```text
第 3 行
=
Repository vector 第 3 个对象
```

因为：

```text
排序
筛选
刷新
删除
```

都会改变 row index。

---

## 58A.7 决策 D210——四状态池仍是同一权威数据的查询视图

> **D210：VolunteerRecord 的 Pending / Rejected / Approved / Withdrawn 四状态继续表现为同一权威数据集上的四个逻辑查询视图。`MyVolunteerPage` 可以采用四个 Tab / 表格分别展示状态池，但不得维护四份独立持久业务数据，也不得把四个 Qt Table 当作四份权威业务集合。**

推荐页面：

```text
MyVolunteerPage
├── Pending Tab
│   └── Edit / Withdraw
├── Rejected Tab
│   └── View reason / Edit + Resubmit
├── Approved Tab
│   └── View / Request Diary
└── Withdrawn Tab
    └── View
```

具体“每次刷新一个 Tab 还是四个 Tab”留到 Gate 3.5.4B 冻结。

---

## 58A.8 决策 D211——ReviewPage 操作始终使用 recordId

> **D211：`ReviewPage` 的待审核工作池与已通过治理区均通过 `recordId` 执行详情、审核、更正或删除操作；UI 排序、筛选、表格技术类型或行位置变化均不得改变业务对象定位方式。**

例如：

```text
Selected row/item
↓
recordId
↓
ReviewVolunteerDialog / Service
```

而不是：

```text
selected row index
↓
Repository[n]
```

---

## 58A.9 决策 D212——不同页面允许选择不同表格技术

> **D212：`RankingPage`、`OperationLogPage`、`ConfigurationPage`、`StudentManagementPage`、`ReviewPage` 等分别根据真实数据量、筛选 / 排序交互、刷新复杂度和展示效果选择 `QTableWidget` 或高级 Model/View；不为了形式统一强制所有页面采用同一种表格技术，也不机械制造大量 `XXXTableModel` 类。**

这意味着：

```text
“一个项目只准一种表格架构”
```

不是当前目标。

目标是：

> **每个页面选择最能形成最终综合优势的方案。**

---

## 58A.10 决策 D213——Service Result 保持纯 C++，Presentation 负责显示格式

> **D213：Service Result 保持纯 C++ 业务语义类型，不返回 Qt 控件类型、`QString` 或中文展示字符串。枚举到中文文本、数值单位、日期显示格式、状态标签等 Presentation 格式转换由 Qt Page / Dialog 完成。**

例如 Service 返回：

```text
RecordStatus::Approved
score = 7.5
duration = 2.5
```

Presentation 转换：

```text
Approved → “已通过”
7.5 → “7.5 分”
2.5 → “2.5 小时”
```

不让 Domain / Service 因 GUI 出现 `QString`。

---

## 58A.11 决策 D214——不预建全局 TableHelper，但允许真实重复后抽取

> **D214：当前不预先建立全局通用 `TableHelper / TableBinder / GenericTableAdapter` 等抽象。局部表格映射、选择 ID、格式化逻辑优先保留在对应 Page 的私有实现中；若实际编码后出现稳定、重复且可解释的公共 Presentation 逻辑，并能明确提升维护性、复用性或展示效果，则允许再抽取轻量辅助组件。**

因此不是：

```text
永远禁止 helper
```

而是：

```text
先证明真实重复 / 收益
再抽取
```

---

## 58A.12 QTableWidget 方案的统一展示映射原则

当某页面采用 `QTableWidget` 时，推荐统一遵守：

```text
Service Result
↓
Page-local mapping / formatter
↓
QTableWidgetItem
↓
visible presentation data
+
Qt::UserRole stable ID
```

表格填充逻辑应集中在页面的刷新 / 映射区域，不允许：

```text
UI click handler
随手拼一行
另一个 handler
再直接改一格
```

导致 Qt 表格逐渐成为“可变业务状态”。

业务变化成功后仍以重新消费 Service 查询结果为主。

---

## 58A.13 实现注意事项一：列定义与索引语义化

`QTableWidget` 填表不得大量散落：

```cpp
table->setItem(row, 0, ...);
table->setItem(row, 1, ...);
table->setItem(row, 5, ...);
```

每个 Page 应采用局部：

```text
列 enum
或
具名常量
```

集中管理列定义。

例如概念上：

```text
VolunteerColumn
├── Date
├── Category
├── Duration
├── Status
└── Score
```

目的：

```text
插入 / 调整列
↓
不需要全页面搜索硬编码数字
```

该规则属于实现规范，不额外生成新的架构决策编号。

---

## 58A.14 实现注意事项二：当前选中行 → stable ID 的局部封装

页面应通过选中 item 的 `Qt::UserRole` 或等价隐藏数据获取 stable ID。

允许在具体 Page 内建立语义明确的私有 helper，例如：

```text
MyVolunteerPage::selectedRecordId()
StudentManagementPage::selectedAccountId()
DiaryModerationPage::selectedPostId()
```

当前不为了这件事创建全局万能：

```text
getSelectedId(QTableWidget*)
TableIdManager
UniversalTableHelper
```

原因是不同 stable ID 的业务语义不同，局部 helper 更容易读懂与答辩解释。

---

## 58A.15 实现注意事项三：刷新后尽量保持选择与滚动位置

对于频繁刷新且列表可能较长的页面，推荐：

```text
refresh 前
↓
记录 selected stable ID
记录必要 scroll position
↓
重新查询 + 重绘
↓
若原 stable ID 仍存在
→ 恢复选中
→ 恢复合理视图位置
```

若对象已经被删除 / 不再属于当前筛选结果：

```text
原 ID 不存在
→ 不恢复选择
```

这是正常结果。

该机制不机械应用于所有小型配置表。对于只有几行数据、刷新极少的页面，简单刷新仍可接受。

这是“效果优先”原则的典型实现：只有用户可感知收益明显的页面才增加这层 UX 处理。

---

## 58A.16 实现注意事项四：四状态 Tab 按需刷新留到 3.5.4B

当前只冻结：

```text
四个 Tab
=
同一 VolunteerRecord 权威集合的四个逻辑状态视图
```

不在本节冻结：

```text
刷新当前 Tab
还是
一次刷新全部四个 Tab
```

初步倾向：

```text
当前可见 Tab
→ 按需刷新

其他 Tab
→ 切换到时再查询
```

但该方案需要与：

```text
Page activate
Dialog Accepted
dirty state
跨页面数据变化
```

一起审计，因此正式结论放在 Gate 3.5.4B。

---

## 58A.17 实现注意事项五：排序后不得缓存 row → ID

如果页面允许点击表头排序：

```text
QTableWidget item
+
Qt::UserRole stable ID
```

会随 item 一起移动。

因此操作时必须：

```text
current selected item
↓
item.data(Qt::UserRole)
↓
stable ID
```

禁止：

```text
ids[row]
```

或保存：

```text
row 5 → REC000123
```

这样的旁路映射。

排序、筛选、刷新后 row 位置都不具有业务稳定性。

---

## 58A.18 实现注意事项六：Presentation 格式化集中但不过度抽取

每个 Page / Dialog 内部可以有私有：

```text
formatStatus(...)
formatDuration(...)
formatScore(...)
formatDate(...)
```

避免同一页面内散落重复格式转换。

如果后续真实发现同一个稳定格式规则在多个 Presentation 组件大量重复，例如：

```text
RecordStatus → 中文文本
```

同时出现在：

```text
MyVolunteerPage
ReviewPage
VolunteerRecordDetailDialog
```

且实现完全相同，可以再抽出轻量：

```text
RecordDisplayFormatter
```

或等价 helper。

但不得为了共享格式化而让 Service / Domain 返回 `QString` 或中文文本。

---

## 58A.19 当前表格架构思维图

```text
                      Service
                        │
              pure C++ query result
                        │
                        ▼
                 Qt Presentation
                        │
         ┌──────────────┴──────────────┐
         │                             │
         ▼                             ▼
简单 / 中小规模页面              复杂交互页面候选
         │                             │
         ▼                             ▼
   QTableWidget                QTableView + Model
         │                       + optional Proxy
         │                             │
         └──────────────┬──────────────┘
                        │
             统一 stable ID 原则
                        │
                 Qt::UserRole
                        │
                        ▼
                    stable ID
                        │
                        ▼
                     Service
                        │
                        ▼
              authoritative Repository
```

关键：

```text
表格技术可以不同
业务身份原则必须相同
Service / Repository 边界必须相同
```

---

## 58A.20 Gate 3.5.4A 结论

当前正式冻结：

```text
D207 ～ D214
```

Gate 状态：

```text
Gate 3.5.4A
表格型页面的数据展示方案
→ FROZEN
```

本节没有预先宣布：

```text
所有页面最终都用 QTableWidget
```

也没有预先宣布：

```text
所有页面最终都用 QAbstractTableModel
```

最终原则是：

> **效果明显领先时允许并鼓励采用高级技术；效果相近时优先采用更清晰、更稳定、更低成本的方案。**

下一步：

> **Gate 3.5.4B——页面刷新与数据一致性审计。**

---


# 58B. Gate 3.5.4B：页面刷新与数据一致性审计

> 状态：**FROZEN**  
> 冻结范围：D215～D223  
> 前置依赖：D177～D214、Gate 3.3 Service FINAL PASS、Gate 3.4 Repository / Persistence FINAL PASS  
> 核心目标：在不建立第二份业务真相、不引入复杂缓存失效系统和事件总线的前提下，使 Qt 页面在业务修改、页面切换、Tab 切换、查询失败和 Dialog 操作后始终能够展示可解释、可信赖的当前数据。

---

## 58B.1 本阶段要解决的核心问题

Gate 3.5.4A 已经回答：

```text
Service Result
    ↓
Qt 表格 / 列表 / View
    ↓
stable ID 定位业务对象
```

但仅解决“怎样显示”仍然不够。

Qt 页面具有自己的生命周期：

```text
创建
↓
首次显示
↓
离开
↓
其他页面发生业务变化
↓
重新回来
```

因此必须继续回答：

```text
谁是页面数据的事实源？
页面什么时候必须重新查询？
Dialog 成功后谁刷新？
不同 Page 之间是否需要互相通知？
多个 Tab 是否全部同时刷新？
查询失败时旧数据还能不能继续展示？
刷新过程中如何避免 Qt signal 重入？
```

本阶段最终采用：

> **Repository 单一权威事实源 + Service 查询 + 页面激活刷新 + 当前操作成功后局部立即刷新 + 不可见页面延迟到再次激活时刷新。**

页面展示始终只是某个时间点的 Presentation Snapshot，不形成第二份业务事实。

---

## 58B.2 页面数据事实源：Repository 仍然唯一权威

### 决策 D215

> **Qt Page / Dialog 的表格、卡片、标签、统计数字与查询结果只属于 Presentation 层展示快照，不构成新的权威业务数据源。需要最新业务事实时，页面必须通过对应 Service 重新查询；运行期权威领域事实继续由 Repository / Domain 维护，派生统计继续由相应 Service 计算。**

因此：

```text
Repository / Domain
        │
        │ authoritative facts
        ▼
      Service
        │
        │ query / derived result
        ▼
   Presentation
        │
        └── temporary display snapshot
```

明确禁止：

```text
QTableWidget / QListWidget / Card Widget
        ↓
被当作“当前业务数据库”
```

也禁止页面长期保存一份可独立修改的完整业务集合，并在后续操作中以该副本替代 Service / Repository 的当前事实。

---

## 58B.3 统一页面激活生命周期约定

单纯规定“页面进入时刷新”仍然存在一个架构风险：

```text
RankingPage.refreshRanking()
ReviewPage.refreshCurrentTab()
DashboardPage.refreshData()
AchievementPage.reload()
```

如果 MainWindow 逐个知道这些具体函数：

```text
MainWindow
├── if RankingPage → refreshRanking()
├── if ReviewPage → refreshCurrentTab()
└── if DashboardPage → refreshData()
```

则 MainWindow 会重新获得具体业务页面知识，破坏其 Application Shell 边界。

因此必须建立统一页面激活语义。

### 决策 D217（修订冻结）

> **所有可导航业务 Page 必须遵循统一的页面激活生命周期约定，并暴露语义一致的 `onActivated()`（或等价统一入口）。MainWindow / Application Shell 在完成 Page 解析、首次懒加载和导航后，只触发该统一生命周期入口，不根据具体 Page 类型硬编码不同刷新方法，也不理解页面内部需要查询什么数据。是否通过轻量共同 Page 基类虚函数、Qt Signal/Slot、callback 或其他等价机制实现，留待 Qt 实现阶段根据真实收益确定；当前不为了统一入口强制建立 `IRefreshablePage` 抽象层。**

统一生命周期语义：

```text
MainWindow::navigateTo(PageId)
          │
          ▼
   resolve / lazy-create
          │
          ▼
       show page
          │
          ▼
  unified onActivated()
          │
          ▼
 Page 自己决定刷新内容
```

MainWindow 只理解：

> “这个 Page 被激活了。”

MainWindow 不理解：

> “RankingPage 要重新算排行榜”  
> “MyVolunteerPage 要查 Pending”  
> “ReviewPage 要查哪个工作池”。

---

## 58B.4 页面导航与刷新架构图

```text
                         Application / AppController
                                   │
                                   ▼
                              MainWindow
                                   │
                         navigateTo(PageId)
                                   │
                    ┌──────────────┴──────────────┐
                    │                             │
              Page 已存在                    Page 未创建
                    │                             │
                    │                       lazy-create
                    │                             │
                    └──────────────┬──────────────┘
                                   ▼
                                show()
                                   │
                                   ▼
                        unified onActivated()
                                   │
                                   ▼
                              Page 自治
                                   │
                ┌──────────────────┼──────────────────┐
                ▼                  ▼                  ▼
          query Service      refresh current Tab   recompute view
                │                  │                  │
                └──────────────────┴──────────────────┘
                                   ▼
                          Update Presentation
```

关键边界：

```text
MainWindow
→ 负责激活生命周期

Page
→ 负责“激活后查什么、展示什么”

Service
→ 负责业务查询与规则

Repository
→ 负责权威事实
```

---

## 58B.5 当前页面发生业务修改后的即时刷新

如果当前 Page 自己完成了一次业务操作，例如：

```text
MyVolunteerPage
↓
VolunteerRecordDialog
↓
修改 Pending
```

若业务已经成功，而表格仍显示旧数据，则用户会看到：

> “修改成功” + “页面内容没变”。

这是不可接受的 Presentation 不一致。

### 决策 D216

> **任何会修改当前 Page 所展示业务事实的 Page/Dialog 操作，在对应 Service 返回完整成功、且必要持久化成功后，当前 Page 必须立即重新通过 Service 查询并刷新相关可见展示。`QDialog::Accepted` 只表示业务真正成功；取消、校验失败、业务失败或持久化失败不得伪装成成功并触发成功路径。**

典型流程：

```text
Page
 │
 ├── open Dialog
 │        │
 │        ▼
 │    user input
 │        │
 │        ▼
 │      Service
 │        │
 │      Success?
 │    ┌───┴────┐
 │    │        │
 │   YES       NO
 │    │        │
 │ accept()   keep open
 │    │        └── show error
 │    ▼
 └─ refresh current visible data
```

---

## 58B.6 Dialog 成功语义的实现级硬约束

业务型 Dialog 必须遵守：

```text
Service完整Success
        ↓
    才允许 accept()
```

任何以下情况均不得调用 `accept()`：

```text
ValidationError
PermissionDenied
BusinessError
NotFound
PersistenceFailure
SeverePersistenceFailure
```

推荐交互：

```text
业务失败
→ Dialog 保持打开
→ 用户输入仍然保留
→ 显示 Service 返回的分类错误
→ 用户可修改后重试 / Cancel
```

`SeverePersistenceFailure` 进入既有 D172 严重故障路径，不按普通表单错误继续工作。

---

## 58B.7 跨页面一致性：不建立页面通知网络

一项业务变化可能影响多个页面。

例如管理员审核通过一条 VolunteerRecord 后，理论上可能影响：

```text
ReviewPage
StudentDashboardPage
MyVolunteerPage
AchievementPage
RankingPage
StatisticsPage
Badge display
DiaryPost eligibility
```

若 ReviewPage 直接通知所有页面：

```text
ReviewPage
├── DashboardPage.refresh()
├── RankingPage.refresh()
├── AchievementPage.refresh()
└── StatisticsPage.refresh()
```

页面之间会形成网状依赖。

若 MainWindow 维护业务影响表：

```text
if recordApproved:
    refresh Dashboard
    refresh Ranking
    refresh Statistics
```

MainWindow 又会变成业务知识中心。

### 决策 D218

> **跨 Page 数据变化默认不建立直接 Page-to-Page 刷新通知网络。完成业务修改的当前 Page 立即刷新；其他 Page 在下一次被导航激活时通过 Service 重新查询最新数据。Page 之间不得因刷新需求形成互相调用、互相持有或业务影响依赖。**

因此：

```text
当前 Page
→ 业务成功
→ 立即 refresh

其他 Page
→ 当前无需处理
→ next onActivated()
→ query Service
→ 获得最新事实
```

---

## 58B.8 为什么当前不引入 dirty flag / EventBus / 缓存失效系统

理论上可以设计：

```text
RankingPage.dirty = true
AchievementPage.dirty = true
DashboardPage.dirty = true
```

也可以设计：

```text
BusinessEventBus
RecordApprovedEvent
RecordDeletedEvent
RuleChangedEvent
```

但这会迫使系统维护：

```text
什么业务变化
→ 影响哪些 Page
→ 哪些缓存失效
```

当前项目为：

```text
单机
单实例
单一当前登录用户
同步 Qt 主线程
Repository 内存数据规模较小
```

不存在多客户端实时同步需求。

### 决策 D219

> **当前不引入全局页面缓存失效系统、dirty flag 网络、业务 EventBus 或额外 Observer 架构。当前单机、单实例、同步 Qt 场景优先采用按需查询与页面激活刷新；只有后续真实性能或交互测试证明该方案形成明显瓶颈时，才重新审计缓存或事件机制。**

这不是禁止 Qt Signal/Slot。

Qt Signal/Slot 仍用于：

```text
按钮交互
Tab 切换
导航请求
Dialog 生命周期
Presentation 内局部交互
```

禁止的是为了页面业务一致性再额外建立一套全局业务事件总线。

---

## 58B.9 多 Tab 页面：不可见时可延迟，一旦激活必须重新查询

MyVolunteerPage：

```text
Pending
Rejected
Approved
Withdrawn
```

ReviewPage：

```text
Pending Review
Approved Governance
```

若每次刷新 Page 都同时查询所有 Tab：

```text
refresh Pending
refresh Rejected
refresh Approved
refresh Withdrawn
```

没有必要。

但是“不可见 Tab 延迟刷新”必须有一致性闭环：

> **一旦 Tab 变为可见，必须重新查询。**

### 决策 D220（修订冻结）

> **具有多个逻辑状态 Tab 的 Page 默认只维护当前可见 Tab 的最新展示。Page 首次激活时刷新当前 Tab；用户切换到任一其他 Tab 时，新激活 Tab 必须通过 Service 重新查询对应逻辑视图后再作为当前数据展示。不可见 Tab 可以不预先刷新，但不得把旧缓存直接视为最新数据。**

因此：

```text
Pending 当前可见
→ 只刷新 Pending

Approved 当前隐藏
→ 可以暂不刷新

用户切换 Approved
→ 必须 query Service
→ 再展示 Approved
```

操作成功后也遵循：

> **优先刷新当前可见业务区域；同一 Page 中其他不可见 Tab 不要求同步预刷新，其一致性由后续 Tab 激活强制查询保证。**

---

## 58B.10 四状态工作池刷新图

```text
                 MyVolunteerPage
                        │
                        ▼
                   onActivated()
                        │
                        ▼
               current visible Tab
                        │
       ┌────────────────┼────────────────┐
       ▼                ▼                ▼
    Pending          Rejected         Approved       Withdrawn
       │                │                │                │
       └──────── current Tab only ───────┴────────────────┘
                        │
                        ▼
               StudentVolunteerService
                        │
                        ▼
               queryOwnRecords(status)
                        │
                        ▼
                 rebuild current view
```

Tab 切换：

```text
QTabWidget.currentChanged
          ↓
new tab becomes visible
          ↓
query corresponding state view
          ↓
display fresh result
```

不存在四个独立数据库，也不存在四个长期缓存真相。

---

## 58B.11 派生结果页面必须重新查询 / 重新计算

以下数据本来就是动态派生：

```text
月度积分
学期积分
总积分
排行榜
排行称号
专项徽章
服务时长统计
全局统计
点赞数量
```

因此 Presentation 不应在登录时计算一次后长期缓存。

### 决策 D221

> **Dashboard、Achievement、Ranking、Statistics、Badge 等派生结果页面在被激活时，通过相应 Service 重新查询 / 计算当前结果；Presentation 不长期缓存这些结果并将其当作业务事实。**

例如：

```text
AchievementPage.onActivated()
        │
        ├── StatisticsService
        ├── RankingService
        └── BadgeService
                ↓
         current derived result
                ↓
            update UI
```

---

## 58B.12 普通查询失败的 UI 状态

此前严重持久化故障已经由 D172 处理，但普通页面查询失败也需要统一约定。

错误做法：

```text
上一次数据仍留在屏幕上
+
角落显示“刷新失败”
```

用户可能继续把旧数据理解为当前真实结果。

因此实现级硬约束：

> **普通查询 / refresh 失败时，Presentation 必须把当前展示标记为无效，不得继续把旧查询结果伪装成最新事实。默认可以清空或隐藏数据区域，显示明确 Error State 与重试入口。若未来采用“保留旧数据”的高级 UX，则必须显著标记“数据已过期 / 刷新失败”，不得让用户误认为其仍是当前结果。**

推荐模型：

```text
Page.onActivated()
      │
      ▼
 query Service
      │
   ┌──┴───┐
   │      │
Success  Failure
   │      │
   ▼      ▼
show    invalidate old view
data    show Error State
        + Retry
```

严重持久化故障：

```text
SeverePersistenceFailure
→ D172
→ 禁止继续正常写业务
```

---

## 58B.13 刷新后的选中对象与滚动位置恢复

全量刷新：

```text
clear
↓
rebuild
```

可能导致用户：

```text
正在看第 30 行
↓
刷新
↓
跳回顶部
```

对于高频或长列表页面，应优先恢复 UX 状态。

### 决策 D222

> **当前数据规模下，列表/表格刷新允许采用清空并重建的简单策略；对于高频或较长列表，优先在刷新前保存当前 selected stable ID 与必要滚动位置，并在刷新后对象仍存在时恢复选择和合理视图位置。只有真实性能 / 交互测试证明全量刷新明显卡顿时，才升级到增量 Model/View 更新。**

实现细化：

```text
before refresh
├── selectedStableId
└── scrollPosition

after refresh
├── ID still exists
│   ├── restore selection
│   └── restore/scroll to item
│
└── ID no longer exists
    ├── clear selection
    └── move to default reasonable position
        （默认顶部）
```

明确禁止使用旧 row index 恢复选择。

---

## 58B.14 刷新重入防护：处理 Qt 事件重入，而不是制造并发系统

当前系统没有异步多线程刷新，但 Qt Signal/Slot 仍可能产生同步事件重入：

```text
Tab changed
↓
refresh
↓
refresh modifies widget
↓
widget emits signal
↓
refresh again
```

因此实现时允许采用轻量防护：

```text
QSignalBlocker
局部 isRefreshing flag
操作期间暂时 disable 某些触发控件
```

但禁止因此引入：

```text
mutex
thread pool
async refresh framework
复杂并发状态机
```

当前问题是**事件重入控制**，不是多线程竞态。

---

## 58B.15 Service 成功后再更新 UI，不采用 optimistic UI

对于本地同步文件业务：

```text
click Like / Approve / Edit
↓
Service
↓
Persistence
↓
Success
↓
update UI
```

速度足够，不需要：

```text
先把 UI 改成成功
↓
后台再执行
↓
失败后回滚
```

### 决策 D223

> **当前状态修改交互默认采用“Service 完整成功后再更新 Presentation”的保守一致性策略，不预先引入 optimistic UI 与失败回滚机制。Severe Persistence Failure 继续按既有故障策略处理，页面不得在 Service 未确认成功前展示最终成功状态。**

该规则能够让：

```text
UI success
```

与：

```text
business + persistence success
```

保持同义。

---

## 58B.16 Gate 3.5.4B 完整刷新生命周期图

```text
                     User navigation
                           │
                           ▼
                MainWindow.navigateTo
                           │
                           ▼
                  lazy-create if needed
                           │
                           ▼
                    Page.onActivated
                           │
                           ▼
                      query Service
                           │
                 ┌─────────┴─────────┐
                 │                   │
              Success             Failure
                 │                   │
                 ▼                   ▼
          update current view    invalidate old view
                 │               show Error State
                 │               + Retry
                 │
                 ▼
         restore selected ID
         / scroll when valid
```

业务修改：

```text
Page / Dialog
      │
      ▼
   Service
      │
      ▼
business + persistence
      │
   ┌──┴────┐
   │       │
Success   Failure
   │       │
   ▼       ▼
accept()  Dialog remains
or mark   open / show error
success
   │
   ▼
refresh current visible area
   │
   ▼
other Pages do nothing now
   │
   ▼
next onActivated()
   │
   ▼
query fresh data
```

---

## 58B.17 本阶段明确不采用的方案

当前明确不采用：

```text
Page A 直接调用 Page B.refresh()
MainWindow 维护“业务动作 → 受影响页面”映射
全局 Session 数据缓存
全局 Page dirty flag 网络
Business EventBus
额外 Observer infrastructure
长期缓存排行榜 / 徽章 / 统计
row index 作为业务身份
Service 未成功就 optimistic 更新 UI
查询失败仍把旧数据作为当前数据展示
```

这些方案并非永远禁止。

若 V0.9 / V1.0 真实实现和性能测试出现明确需求，可以通过新的版本设计决策重新审计。

---

## 58B.18 D215～D223 冻结索引

| 编号 | 决策 |
|---|---|
| D215 | Page/Dialog 中的数据只是 Presentation Snapshot；权威事实继续来自 Repository / Service |
| D216 | 当前 Page 业务修改完整成功后立即重新查询并刷新；Dialog Accepted 只代表真正成功 |
| D217 | 所有可导航业务 Page 采用统一 onActivated / 等价生命周期入口；MainWindow 不硬编码具体页面刷新方法 |
| D218 | 跨 Page 默认不建立直接刷新通知；其他 Page 下次激活时重新查询 |
| D219 | 当前不引入 dirty flag 网络、业务 EventBus 或复杂页面缓存失效系统 |
| D220 | 多 Tab 页面只要求当前可见 Tab 最新；Tab 一旦激活必须通过 Service 强制重新查询 |
| D221 | Dashboard / Ranking / Badge / Statistics 等派生结果页面激活时重新查询 / 计算 |
| D222 | 当前允许 clear + rebuild；长列表优先恢复 selected stable ID / scroll；真实性能问题出现后再升级增量 Model/View |
| D223 | 状态修改默认 Service 完整成功后再更新 UI，不引入 optimistic UI / rollback 复杂机制 |
| D224 | DiaryWallPage 采用 QScrollArea + FeedContainer + QVBoxLayout + DiaryPostCard QWidget；真实性能问题后才重审 Model/View |
| D225 | DiaryPostCard 是 Presentation Component，只消费 DiaryPostPublicView，不持有 Domain / Repository 对象地址 |
| D226 | Card 只发 like/unlike 等用户意图，由 DiaryWallPage → DiaryService 执行业务 |
| D227 | Public View 由查询层一次组装公开字段和必要派生荣誉；后台字段从类型边界即不暴露 |
| D228 | Card 按 Header / Content / Facts / Honor Chips / Footer 组织；视觉装饰不进入 Domain |
| D229 | Like/Unlike 成功后重新获取完整 Public View，并复用 Card.updateView() 局部刷新 |
| D230 | Administrator 仅浏览 DiaryWall，不可点赞；治理只进入 DiaryModerationPage |
| D231 | V1.0 不增加 DiaryPost 图片 / 附件字段 |
| D232 | 区分业务查询筛选与 Presentation 临时筛选 |
| D233 | 多条件筛选可使用轻量 Query Criteria，不建立重型通用查询框架 |
| D234 | 业务排序由 Service 保证；Ranking rank/title 不由视觉排序重定义 |
| D235 | 普通管理表可启用 Presentation 排序/过滤，但 stable ID 不依赖 row |
| D236 | V1.0 采用混合 Qt 数据展示技术，不强制全部 QTableWidget 或全部 Model/View |
| D237 | StudentManagementPage 正式采用 QTableView + StudentTableModel + QSortFilterProxyModel |
| D238 | OperationLogPage 正式采用 QTableView + OperationLogTableModel；正式查询先 Service，Proxy 后视觉过滤 |
| D239 | 自定义 TableModel 只持有 Service View Result，不访问 Repository / Domain 业务地址 |
| D240 | 当前不建立全局传统分页；DiaryWall 性能问题优先分批 / Load More 后再重审 Model/View |
| D241 | UI 筛选 / 排序状态可保留；业务查询结果仍按页面激活机制重新查询 |
| D242 | Repository 层正式认定采用 Repository Pattern；不为单一文件后端引入 IRepository<T> / GenericRepository |
| D243 | Export 多格式输出采用 Strategy Pattern；ExportService 先组装中性 ExportDocument，格式策略只负责 CSV / Markdown 序列化，不引入 Strategy Factory / Registry |
| D244 | StudentManagement / OperationLog 正式采用 Qt Model/View；QSortFilterProxyModel 说明为 Qt 提供的 Proxy-style 机制，不冒充自行实现完整 MVC / Proxy |
| D245 | Application/AppController 作为 Composition Root + 显式依赖传递；不采用 DI Framework、Service Locator、Singleton |
| D246 | 当前 Service 层正式认定为 Service Layer；不同 Service 可有不同复杂度，不按按钮机械拆分 |
| D247 | PersistenceCoordinator 不认定为 Unit of Work / Transaction Manager；只做轻量多文件 prepare–commit 协调，不提供 ACID |
| D248 | Qt Signal/Slot 只认定为框架提供的 Observer-style 通知机制，不自行实现 Observer/EventBus 基础设施 |
| D249 | VolunteerRecord 使用 RecordStatus + 受控状态转换，不采用 State Pattern |
| D250 | User 创建/恢复当前不采用 Factory Pattern；Student / Administrator 类型少且来源明确 |
| D251 | AppController 不认定为 Facade；只做装配与生命周期，不聚合全部业务接口 |
| D252 | V1.0 引入轻量 Result<T> 类模板，统一 Application/Service 的强类型调用结果外壳 |
| D253 | Result<T> 只用于确有失败语义的 Application/Service 边界，不机械扩散到 Domain / Repository / Qt helper |
| D254 | 无成功业务值的修改操作优先 OperationResult / 等价非模板结果，不强制 Result<void> 特化 |
| D255 | Result<T> 复用统一 ServiceError，保持纯 C++，不携带 Qt Presentation 类型 |
| D256 | 不为额外模板加分引入 Generic Repository、自定义 STL 容器、通用 TableModel 或 Export 模板 |
| D257 | Result<T> 只包装独立完整的 View/Query Result，不吸收业务字段、不取代成功结果类型 |
| D258 | Result<T> / OperationResult 使用统一强类型错误枚举；message 只做补充说明，不承担程序错误分支判定 |

---

> **实现期审计停点 → Gate 4**
>
> 本节架构语义已冻结，但 Qt Model/View 的精确列结构、Role、Proxy、刷新实现和 Dialog 生命周期必须结合真实 Qt 代码再决定。
>
> 后续实现时应先读取：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 进入 V0.3 前触发 `STOP-06`；当 MainWindow、多个 Page 与 Dialog 已形成时触发 `STOP-07`。触发后应暂停实现并进行审计，不得由执行层自行补造实现规则。

## 58B.19 Gate 3.5.4B 实现级硬约束

以下内容不额外编号，但属于后续 Qt 实现与 Gate 4 审计必须检查的 Current Truth：

1. **统一页面激活入口**  
   MainWindow 不调用不同 Page 的不同业务刷新方法；统一触发 `onActivated()` 或等价生命周期入口。

2. **普通查询失败不得伪装旧数据为最新**  
   默认清空/隐藏旧展示并进入 Error State；若保留 stale data，则必须显著标识。

3. **Dialog 只有完整 Success 后才能 `accept()`**  
   Service / Persistence 失败时保持 Dialog 打开并显示错误，严重故障进入 D172。

4. **防止同步刷新重入**  
   优先使用 `QSignalBlocker`、局部 `isRefreshing` 等轻量机制，不引入线程锁或异步框架。

5. **selected stable ID 不存在时清除选择**  
   不使用旧 row index 恢复；默认回到顶部或页面定义的合理默认位置。

6. **Tab 激活强制刷新**  
   不可见 Tab 可以延迟，但一旦成为当前 Tab，必须重新查询后再作为最新数据展示。

7. **同一 Page 不可见子区域无需提前刷新**  
   操作成功后只保证当前可见业务区域立即更新；其他 Tab 的一致性由后续激活刷新保证。

---

## 58B.20 Gate 3.5.4B 结论

```text
Gate 3.5.4B
页面刷新与数据一致性
→ D215～D223
→ FROZEN
```

当前 Qt 数据一致性主线已经形成：

```text
Repository single truth
        ↓
Service query
        ↓
Page onActivated
        ↓
fresh Presentation Snapshot
```

业务修改主线：

```text
Service Success
        ↓
current Page immediate refresh
        ↓
other Pages refresh on next activation
```

该方案在当前课程项目规模下兼顾：

- 正确性；
- UI 一致性；
- 代码可解释性；
- Qt 生命周期清晰度；
- 最终答辩可讲性；
- 避免过度缓存 / 事件系统；
- 后续仍允许在真实性能证据下升级 Model/View。

下一步正式进入：

> **Gate 3.5.4C——DiaryWall 卡片信息流与视觉交互架构审计。**



# 58C. Gate 3.5.4C：DiaryWall 卡片信息流与视觉交互架构

> 状态：**FINAL FROZEN / PASS**  
> 冻结范围：D224～D231  
> 前置依赖：Gate 1 日记墙业务规则、D177～D223、DiaryService / RankingService / BadgeService 既有边界  
> 核心目标：将 Gate 1 已冻结的“QQ 空间 / 小红书式卡片信息流”产品定位映射为高质量 Qt Presentation，同时保持 Domain / Service / Repository 边界不被视觉层侵入。

---

## 58C.1 Feed 技术选型：效果优先而不是技术名词优先

日记墙不是后台业务表格，其产品语义是：

```text
公开内容流
→ 多条帖子
→ 独立标题 / 文案
→ 荣誉信息
→ 点赞互动
```

因此不采用普通 QTableWidget 作为主视图。

最终比较：

```text
A. QScrollArea + FeedContainer QWidget + QVBoxLayout + DiaryPostCard QWidget
B. QListWidget + setItemWidget()
C. QListView + QAbstractListModel + QStyledItemDelegate
```

终审结论：

> **当前数据规模预期几十条，极端数百条；主要目标是卡片视觉自由度、点赞按钮、hover、Chip、复杂布局和实现稳定性。A 方案在当前约束下综合效果最优。**

`QListView + Model/Delegate` 的优势主要体现在：

```text
大量数据
虚拟化
只绘制当前可见项
统一 Model
```

但其交互实现更复杂，尤其卡片内部按钮通常不是独立 QWidget，而需要在 Delegate 的 `editorEvent()` / 鼠标坐标区域中判断点击位置。

当前不存在足够数据规模去抵消这些复杂度。

### 决策 D224

> **V1.0 `DiaryWallPage` 正式采用卡片式滚动信息流，技术基线为 `QScrollArea + FeedContainer QWidget + QVBoxLayout + 自定义 DiaryPostCard QWidget`。只有真实压力测试证明 QWidget Feed 在页面创建、滚动或内存方面出现明显不可接受瓶颈时，才重新评估分批加载、分页或 `QListView + Model/Delegate`。不使用未经测试依据的固定卡片数量作为强制切换阈值。**

性能优化优先级：

```text
QScrollArea + Card
        │
        ▼
真实测试是否卡顿？
    ┌───┴────┐
    │        │
   否       是
    │        │
    ▼        ▼
  保持   优化 Card 结构/Effect
             │
             ▼
        分批加载 / Load More
             │
             ▼
          仍不满足
             │
             ▼
      再审 QListView + Model/Delegate
```

---

## 58C.2 DiaryPostCard 的架构定位

### 决策 D225

> **`DiaryPostCard` 是独立 Qt Presentation Component，不是领域对象。它只消费纯 C++ 的 `DiaryPostPublicView` / 等价公开查询结果，不长期持有 `DiaryPost*`、`VolunteerRecord*`、Repository 内对象地址。**

三层关系：

```text
DiaryPost
Domain Entity
    │
    ▼
DiaryPostPublicView
Application / Query Result
    │
    ▼
DiaryPostCard
Qt Presentation Component
```

禁止：

```text
DiaryPost : QObject
DiaryPostCard -> DiaryPost*
DiaryPostCard -> Repository
```

---

## 58C.3 Card 只表达用户意图，不执行业务

### 决策 D226

> **`DiaryPostCard` 不直接持有 DiaryService、Repository 或 ApplicationContext 作为业务执行入口。Card 只通过 Qt Signal / 等价 Presentation 事件表达用户意图，例如 `likeRequested(postId)`、`unlikeRequested(postId)`；由 `DiaryWallPage` 调用 `DiaryService` 完成最终业务。**

推荐结构：

```text
DiaryPostCard
   │
   ├── likeRequested(postId)
   └── unlikeRequested(postId)
             │
             ▼
      DiaryWallPage
             │
             ▼
       DiaryService
             │
             ▼
  Permission / Status / LikeRelation
```

Card 不判断：

```text
账号是否 Active
Student 是否有 LikeDiaryPost
帖子是否 Displayed
点赞关系是否已存在
持久化是否成功
```

这些都属于 Service / Domain 规则。

---

## 58C.4 Public View：公开字段从类型边界开始隔离

### 决策 D227（终审冻结）

> **公开日记墙使用显式 `DiaryPostPublicView` / 等价纯 C++ 查询结果作为 Presentation 数据边界。该 View 由 DiaryService 的公开 Feed 查询流程或等价 Application 查询组装一次性构造，必要时只读协作 BadgeService、RankingService 等既有派生结果服务；DiaryWallPage 与 DiaryPostCard 不自行跨多个 Domain / Service 拼接公开数据。后台字段从查询结果类型层面即不得暴露给公共 Card。**

推荐最小字段：

```text
DiaryPostPublicView
├── postId
├── authorName
├── title
├── categoryName
├── serviceDate
├── duration
├── place
├── displayContent
├── recordScore
├── specialtyBadgeTexts
├── rankingTitleTexts
├── likeCount
├── likedByCurrentStudent
└── publicationTime
```

明确不出现：

```text
verifier
reviewNote
reviewerAccountId
adminCorrectionReason
categoryCorrectionReason
durationCorrectionReason
OperationLog
其他后台治理字段
```

公开 Feed 查询架构：

```text
DiaryWallPage
      │
      ▼
DiaryService.queryPublicFeed(...)
      │
      ├── DiaryPost facts
      ├── VolunteerRecord public facts
      ├── Student display facts
      ├── Category display facts
      ├── BadgeService current result
      ├── RankingService current result
      └── LikeRelation current result
      │
      ▼
vector<DiaryPostPublicView>
      │
      ▼
DiaryPostCard widgets
```

关键原则：

> **Presentation 不自己拼业务真相，Query / Service 层先形成可公开、完整、最小化的数据视图。**

---

## 58C.5 Card 视觉区域与荣誉 Chip

### 决策 D228

> **DiaryPostCard 按 Header / Content / Service Facts / Honor Chips / Interaction Footer 等稳定展示区域组织；专项徽章和排行称号采用轻量 Chip / QLabel 等 Presentation 表达，任何颜色、圆角、阴影、排版、hover 等视觉装饰不得反向写入 Domain。**

推荐结构：

```text
DiaryPostCard
│
├── Header
│   ├── Author
│   ├── Category Chip
│   └── Publication Time
│
├── Content
│   ├── Title
│   └── Display Content
│
├── Service Facts
│   ├── Date
│   ├── Duration
│   ├── Place
│   └── Score
│
├── Honor Area
│   ├── Specialty Badge Chip
│   └── Ranking Title Chip
│
└── Footer
    └── Like Count / Student Like Button
```

UX 约束：

- `specialtyBadgeTexts` / `rankingTitleTexts` 可以包含多个结果；
- Card 不应机械把全部荣誉塞满；
- 优先展示与当前帖子类别直接相关的专项徽章，并展示少量当前有效排行称号；
- AchievementPage 承担完整荣誉展示职责。

---

## 58C.6 Like / Unlike 局部刷新

### 决策 D229

> **Like / Unlike 必须遵循 D223“Service 完整成功后再更新 UI”。点赞或取消点赞成功后，DiaryWallPage 默认重新获取该 `postId` 的完整 `DiaryPostPublicView`，并复用现有 Card 调用 `updateView()` 进行局部刷新；不得直接 `likeCount++ / --` 作为业务真相，也不默认重建整个 Feed。**

推荐流程：

```text
Student clicks Like
        │
        ▼
DiaryPostCard emits likeRequested(postId)
        │
        ▼
DiaryWallPage
        │
        ▼
DiaryService.like(accountId, postId)
        │
        ▼
 business + persistence success
        │
        ▼
DiaryService.getPublicView(postId, viewerAccountId)
        │
        ▼
latest DiaryPostPublicView
        │
        ▼
existing Card.updateView(view)
```

优点：

- 不销毁全部 Card；
- UI 与最新派生荣誉 / 点赞数同步；
- Card 不维护业务计数；
- 与 D215 / D223 一致。

---

## 58C.7 Administrator 不能点赞；公共墙与治理后台分离

### 决策 D230

> **Administrator 对公共 DiaryWall 仅具有浏览能力，不具有点赞 / 取消点赞能力。DiaryPostCard 仅当当前登录上下文具有 `LikeDiaryPost` 且 Service 最终授权成立时才提供 Like / Unlike 交互；Administrator 只显示 likeCount，不显示可操作点赞状态。管理员审核、拒绝、下架等治理行为只进入 DiaryModerationPage，不叠加到公共 DiaryWall Card。**

角色行为：

```text
Student
→ browse
→ like / unlike
→ Service final auth

Administrator
→ browse
→ see likeCount
→ NO like interaction
→ moderation only in DiaryModerationPage
```

这保持：

```text
DiaryWallPage
= public feed

DiaryModerationPage
= backend governance
```

---

## 58C.8 V1.0 不加图片 / 附件

### 决策 D231

> **V1.0 明确不增加 DiaryPost 图片或附件业务字段。日记墙通过高质量文本排版、卡片、Chip、间距、轻量阴影与点赞微交互提升视觉完成度。若未来新增图片功能，必须重新进入 Domain / Persistence / Governance 生命周期审计，不能只在 Qt 层临时增加路径字段。**

因此当前不引入：

```text
imagePath
attachmentPath
thumbnail cache
image storage
missing image recovery
图片删除联动
图片审核生命周期
```

---

## 58C.9 Card Signal 与 Qt 生命周期实现约束

以下不额外编号，但属于 Gate 4 必查项：

1. Card 在创建时由 Page 统一 `connect()`。
2. Card 不持有 Page / Service 指针作为业务入口。
3. `updateView()` 只更新可见内容，不重复 `connect()`。
4. Feed 完整重建时必须真正销毁旧 Card，不能只 `removeWidget()` 后遗留隐藏对象。
5. 使用正确 Qt parent-child ownership，让 QObject 销毁时自动断开相关连接。
6. Like 局部更新优先复用现有 Card 实例。

典型错误：

```text
updateView()
→ connect again
→ 更新 5 次后一次 click 触发 5 次 Service
```

必须禁止。

---

## 58C.10 Gate 3.5.4C 架构总图

```text
                         DiaryWallPage
                              │
                    queryPublicFeed(criteria)
                              │
                              ▼
                         DiaryService
                              │
          ┌───────────────────┼────────────────────┐
          │                   │                    │
          ▼                   ▼                    ▼
   Diary/Record facts    Badge/Ranking       LikeRelation
          │                derived                │
          └───────────────────┼────────────────────┘
                              ▼
                   DiaryPostPublicView[]
                              │
                              ▼
                         FeedContainer
                              │
                       QVBoxLayout
                  ┌───────────┼───────────┐
                  ▼           ▼           ▼
              Card #1      Card #2      Card #N
                  │
          likeRequested(postId)
                  │
                  ▼
            DiaryWallPage
                  │
                  ▼
             DiaryService
                  │
           complete Success
                  │
                  ▼
        getPublicView(postId)
                  │
                  ▼
            Card.updateView()
```

---

## 58C.11 Gate 3.5.4C 结论

```text
Gate 3.5.4C
DiaryWall 卡片信息流与视觉交互架构
→ D224～D231
→ FINAL FROZEN / PASS
```

---

# 58D. Gate 3.5.4D：筛选、排序、分页与最终 Model/View 技术终审

> 状态：**FROZEN / PASS**  
> 冻结范围：D232～D241  
> 核心目标：明确业务查询筛选与 Presentation 临时筛选的边界，冻结 Model/View 高级技术真实使用点，保证高级 Qt 技术带来效果和维护性收益而不是机械堆砌。

---

## 58D.1 两类筛选必须严格区分

### 决策 D232

> **系统明确区分“业务查询筛选”和“Presentation 临时显示筛选”。会改变正式查询数据集的条件由 Qt 传给对应 Service / Repository 查询；只作用于已加载 View Result 的搜索、显示隐藏等可以由 Presentation 完成。Qt 不默认把整个权威数据集全部读取后承担正式业务查询职责。**

定义：

```text
业务查询筛选
→ 定义 Service 应返回哪些业务数据

Presentation Filter
→ 在 Service 已返回的小结果集上改变当前显示
```

例如：

```text
OperationLog:
时间范围 / 操作管理员 / OperationType
→ Service Query

StudentManagement:
姓名模糊搜索 / 学号前缀 / 本地状态过滤
→ 当前小规模结果上可由 Proxy 完成
```

---

## 58D.2 轻量 Query Criteria，而不是通用查询框架

### 决策 D233

> **具有多个可选查询条件的页面可以使用轻量纯 C++ Query Criteria 值对象承载筛选条件，但不引入 Specification、Generic Query Builder、表达式树等重型通用查询框架。**

可能：

```text
OperationLogQuery
├── startDate?
├── endDate?
├── operatorAccountId?
└── operationType?
```

仍然是普通值对象，不形成企业级查询 DSL。

---

## 58D.3 业务排序与视觉排序严格分离

### 决策 D234

> **业务规则规定的排序必须由业务 Service 保证。尤其 RankingPage 的“积分降序 → 有效时长降序 → 有效记录数降序 → 学号升序”及 rank/title 语义不得交由 Qt 表头排序重新定义；Presentation 的视觉排序不能改变业务排名事实。**

RankingPage 默认建议：

```text
sorting disabled
```

若未来允许视觉排序：

```text
业务 rank
奖牌
称号
Top 3 高亮
```

必须绑定：

```text
RankingEntry.rank
```

禁止绑定：

```text
current visual row index
```

---

## 58D.4 普通管理表的 Presentation 排序

### 决策 D235

> **普通管理型表格可以根据交互价值启用 Presentation 层列排序 / 文本过滤，但 stable ID 始终通过 item / model role 保存，任何排序或过滤后均不得使用 row index 作为业务身份。**

这继续继承 D209 / D211：

```text
stable ID
= business identity

row
= current visual position only
```

---

## 58D.5 V1.0 采用混合 Qt 数据展示架构

### 决策 D236

> **V1.0 不强制所有表格使用同一种 Qt 技术。小规模、操作型表格优先 QTableWidget；筛选 / 排序价值明显且数据相对较多的页面允许使用 QTableView + QAbstractTableModel + QSortFilterProxyModel；DiaryWall 使用自定义 QWidget Card Feed；Statistics 的高级可视化另按效果收益审计。**

最终技术分布：

```text
Simple business tables
→ QTableWidget

StudentManagement
→ QTableView
→ StudentTableModel
→ QSortFilterProxyModel

OperationLog
→ QTableView
→ OperationLogTableModel
→ QSortFilterProxyModel + Service Query

DiaryWall
→ QScrollArea
→ custom DiaryPostCard QWidget

Statistics
→ table + chart candidate
```

这体现：

> **按真实需求选技术，而不是为了项目“统一”机械统一。**

---

## 58D.6 StudentManagementPage：Model/View 正式落点

### 决策 D237

> **StudentManagementPage 正式作为高级 Model/View 使用点，采用 `QTableView + StudentTableModel + QSortFilterProxyModel` 或等价组合，以支持学生搜索、排序、状态 / 字段过滤并体现真实 Qt Model/View 技术价值。管理员有权查看的学生集合先由 UserManagementService 提供；当前学生规模可控，姓名、学号、班级、专业、账号状态等作用于已加载结果的交互式搜索 / 排序可以主要由 Proxy 完成。Proxy 不负责业务权限和账号管理合法性。**

结构：

```text
UserManagementService
        │
        ▼
vector<StudentSummary>
        │
        ▼
StudentTableModel
        │
        ▼
QSortFilterProxyModel
        │
        ▼
QTableView
```

Proxy 可处理：

```text
姓名模糊搜索
学号前缀
班级 / 专业显示过滤
账号状态显示过滤
列排序
```

Service 仍负责：

```text
谁有 ManageStudents
账号是否可禁用
账号是否允许删除
密码是否可重置
```

---

## 58D.7 OperationLogPage：正式查询先 Service，Proxy 后 Presentation

### 决策 D238（补强冻结）

> **OperationLogPage 正式采用 `QTableView + OperationLogTableModel`，并可结合 `QSortFilterProxyModel` 与 Service 查询条件。时间范围、执行管理员、OperationType 等正式业务查询条件必须先交给 OperationLogService；Proxy 只作用于 Service 已返回结果，用于视觉排序和当前结果集上的轻量文本过滤。不得在 Service 和 Proxy 中无意识重复实现同一个正式筛选规则。**

正确链路：

```text
Qt Query Controls
        │
        ▼
OperationLogQuery
        │
        ▼
OperationLogService.query(...)
        │
        ▼
OperationLogView[]
        │
        ▼
OperationLogTableModel
        │
        ▼
QSortFilterProxyModel
        │
        ▼
QTableView
```

禁止：

```text
read ALL operation logs
↓
Proxy 实现正式时间范围查询
```

---

## 58D.8 自定义 TableModel 的数据边界

### 决策 D239

> **自定义 Qt TableModel 只保存 Service 返回的只读 View Result 集合，不直接访问 Repository、Domain 实体地址或执行业务授权；Proxy Model 同样只负责 Presentation 排序 / 过滤，不承担业务规则。**

例如：

```text
StudentTableModel
→ vector<StudentSummary>

OperationLogTableModel
→ vector<OperationLogView>
```

禁止：

```text
StudentTableModel -> UserRepository
OperationLogTableModel -> OperationLogRepository
Proxy -> Permission logic
```

---

## 58D.9 分页：当前不建立全局机制

### 决策 D240

> **V1.0 当前不建立全局传统分页机制。当前几十至数百条业务数据优先直接查询与展示；DiaryWall 如真实压力测试出现 Card 创建 / 滚动性能问题，优先考虑分批加载 / “加载更多”，再根据实测决定是否升级 Model/View。**

不引入：

```text
GenericPaginator<T>
PageRequest
PageResult<T>
全局分页协议
```

OperationLog 即使达到数千条，QTableView + Service 查询仍可先满足当前课程规模。

---

## 58D.10 页面级 UI State 可以保留，但业务结果必须重新查询

### 决策 D241

> **筛选条件、当前 Tab、搜索文字、Presentation 排序方式等可以作为页面级 UI 状态保留；业务查询结果本身仍按 D215～D221 的页面激活策略重新查询。刷新后可以重新应用当前筛选 / 排序状态，但不得把旧结果当作最新事实。**

因此：

```text
filter UI state
→ may persist in Page

business result
→ refresh on activation
```

例如 StudentManagement：

```text
搜索框“人工智能”
↓
切到其他页面
↓
回来
↓
重新查询学生数据
↓
重新应用“人工智能”过滤
```

---

## 58D.11 Model 数据更新的 Qt 协议

实现级硬约束：

> **StudentTableModel / OperationLogTableModel 在整体替换 Service 查询结果时，必须使用 Qt Model/View 规定的模型重置通知，例如 `beginResetModel()` / `endResetModel()` 包围内部数据集合替换。不得静默修改内部 vector 后期望 QTableView 自动发现变化。**

默认：

```text
beginResetModel()
↓
replace internal View Result collection
↓
endResetModel()
```

只有未来真实采用增量变更时，才使用：

```text
beginInsertRows()
beginRemoveRows()
dataChanged()
```

避免：

- View 显示过期；
- QModelIndex 错乱；
- Selection 异常；
- 行数不同步。

---

## 58D.12 Ranking 高亮与业务 rank 绑定

实现级硬约束：

```text
🥇 / Gold highlight
🥈 / Silver highlight
🥉 / Bronze highlight
ranking title
```

必须来自：

```text
RankingEntry.rank
```

不得来自：

```text
visual row 0 / 1 / 2
```

当前默认推荐：

```text
RankingPage sorting disabled
```

如果未来开放视觉排序，高亮仍必须跟随业务 rank。

---

## 58D.13 Loaded / Empty / Error / Refreshing 统一状态

所有主要 Table / Feed 页面应明确区分：

```text
Loaded
Empty
Error
Refreshing
```

定义：

```text
Service Success + result > 0
→ Loaded

Service Success + result == 0
→ Empty

Service Failure
→ Error

Service call in progress
→ Refreshing
```

Empty 不是 Error。

示例：

```text
MyVolunteerPage
→ 暂无待审核志愿记录

StudentManagementPage
→ 未找到符合当前筛选条件的学生

OperationLogPage
→ 当前筛选条件下暂无操作日志

DiaryWallPage
→ 暂无公开志愿日记
```

Refreshing 时：

- 可临时禁用刷新 / 查询按钮；
- 可使用 Page-local `isRefreshing`；
- 当前同步 Qt 场景不因此引入线程、任务队列或复杂异步 Loading 系统。

---

## 58D.14 Gate 3.5.4D 决策索引

| 编号 | 决策 |
|---|---|
| D232 | 区分业务查询筛选与 Presentation 临时筛选 |
| D233 | 多条件筛选可使用轻量 Query Criteria，不建立通用查询框架 |
| D234 | 业务排序由 Service 保证；Ranking rank/title 不由视觉排序重定义 |
| D235 | 普通管理表可启用 Presentation 排序/过滤，但 stable ID 不依赖 row |
| D236 | V1.0 采用混合 Qt 数据展示技术，不强制全项目统一表格方案 |
| D237 | StudentManagementPage 正式采用 QTableView + StudentTableModel + Proxy |
| D238 | OperationLogPage 正式采用 QTableView + OperationLogTableModel；正式查询先 Service、Proxy 后视觉过滤 |
| D239 | 自定义 TableModel 只持有 Service View Result，不访问 Repository / Domain 业务地址 |
| D240 | 当前不建立全局传统分页；DiaryWall 性能问题先分批/Load More 再重审 Model/View |
| D241 | UI 筛选/排序状态可保留；业务查询结果仍按页面激活机制重新查询 |

---

## 58D.15 Gate 3.5.4D 结论

```text
Gate 3.5.4D
筛选、排序、分页与最终 Model/View 技术终审
→ D232～D241
→ FROZEN / PASS
```

---

# 58E. Gate 3.5：Qt Presentation / Application Architecture FINAL AUDIT

> 状态：**FINAL PASS**  
> 审计范围：D177～D241  
> 目的：反向验证 Qt Presentation、ApplicationContext、MainWindow、Page/Dialog、刷新、Model/View、DiaryWall、权限与 Service / Repository 边界之间是否存在职责冲突、循环依赖、重复业务事实或过度设计。

---

## 58E.1 Qt / Service / Domain / Repository 总边界

最终：

```text
Qt Presentation
       ↓
Application / Service
       ↓
Domain
       ↓
Repository / Persistence
```

审计结果：**PASS**

Qt Page：

```text
不直接访问 Repository
不读写 CSV
不持有长期 Domain pointer
不负责积分 / 排行 / 徽章算法
不实现正式权限判定
```

Domain / Repository：

```text
不继承 QWidget
不依赖 QMessageBox
不依赖 QTableWidget
不发业务 Qt Signal
```

---

## 58E.2 MainWindow God Class 审计

最终职责：

```text
MainWindow
├── user header
├── navigation
├── Page container
├── lazy-create Page
├── navigateTo(PageId)
├── trigger unified onActivated()
└── logout
```

MainWindow 不负责：

```text
审核
积分
排行榜
徽章
日志
文件持久化
帖子点赞规则
```

审计：**PASS**

---

## 58E.3 ApplicationContext 审计

最终：

```text
ApplicationContext
├── currentAccountId
├── displayName
└── UserCapabilities
```

不保存：

```text
User*
Repository*
Service*
业务缓存
```

审计：**PASS**

---

## 58E.4 Login / Logout 生命周期审计

```text
LoginWindow
↓
AuthenticationService
↓
LoginResult
↓
ApplicationContext
↓
MainWindow
```

Logout：

```text
destroy MainWindow + authenticated Pages
↓
destroy ApplicationContext
↓
show LoginWindow
```

无 Repository pointer / User pointer 长期残留。

审计：**PASS**

---

## 58E.5 权限双重保障审计

```text
ApplicationContext.capabilities
→ 控制 UI 入口 / 按钮可见与启用

Service
→ 最终检查 Permission + Active + ownership + state + rule
```

不存在“按钮隐藏 = 最终权限”。

审计：**PASS**

---

## 58E.6 页面数量与职责审计

Student：

```text
StudentDashboardPage
MyVolunteerPage
AchievementPage
ProfilePage
DiaryWallPage
RankingPage
```

Administrator：

```text
AdminDashboardPage
ReviewPage
StudentManagementPage
ConfigurationPage
DiaryModerationPage
StatisticsPage
OperationLogPage
```

局部一次性编辑 / 确认：

```text
Dialog
```

不存在“一按钮一 Page”，也不存在万能管理 Page。

审计：**PASS**

---

## 58E.7 四状态工作池与单一事实源审计

```text
VolunteerRecordRepository
→ one authoritative record collection
```

Qt：

```text
Pending
Rejected
Approved
Withdrawn
```

只是查询 View，不是四个文件 / Repository。

审计：**PASS**

---

## 58E.8 Presentation Snapshot 审计

```text
QTableWidget
QTableView Model
DiaryPostCard
Dashboard labels
```

都只是 Presentation Snapshot。

页面：

```text
onActivated()
→ Service query
→ current result
```

审计：**PASS**

---

## 58E.9 页面刷新与跨页一致性审计

当前业务操作：

```text
Service Success
→ current Page immediate refresh
```

其他 Page：

```text
next onActivated()
→ query fresh result
```

不使用：

```text
Page-to-Page refresh calls
EventBus
dirty flag graph
```

审计：**PASS**

---

## 58E.10 Dialog 成功语义审计

```text
Service complete Success
→ accept()

Failure
→ keep Dialog open
→ show classified error
```

审计：**PASS**

---

## 58E.11 Query Failure / Empty 审计

```text
Success + data
→ Loaded

Success + no data
→ Empty

Failure
→ Error

Severe Persistence Failure
→ D172
```

旧数据不得伪装为当前结果。

审计：**PASS**

---

## 58E.12 stable ID 与排序/过滤审计

QTableWidget：

```text
Qt::UserRole -> stable ID
```

Model/View：

```text
Model Role -> stable ID
```

任何业务操作不得依赖视觉 row。

审计：**PASS**

---

## 58E.13 高级 Qt 技术使用审计

技术分布：

```text
simple tables
→ QTableWidget

StudentManagement
→ QTableView + StudentTableModel + Proxy

OperationLog
→ QTableView + OperationLogTableModel + Proxy / Service Query

DiaryWall
→ QScrollArea + custom Card QWidget

Statistics
→ Chart candidate
```

技术使用有真实效果 / 维护性收益，没有机械堆叠。

审计：**PASS**

---

## 58E.14 Ranking 业务排序审计

Ranking 顺序：

```text
score desc
duration desc
valid record count desc
student ID asc
```

由 RankingService 保证。

Top3 UI：

```text
gold / silver / bronze
```

绑定业务 rank，不绑定 row。

审计：**PASS**

---

## 58E.15 DiaryWall 业务与隐私审计

Card 只获取：

```text
DiaryPostPublicView
```

不暴露：

```text
verifier
reviewNote
correction reason
OperationLog
```

Administrator：

```text
browse only
NO like
```

审计：**PASS**

---

## 58E.16 DiaryWall Service Dependency DAG 审计

允许：

```text
StatisticsService
      ↑      ↑
 Ranking   Badge
      \      /
       \    /
      Diary public view assembly
```

前提：

```text
RankingService / BadgeService
不反向依赖 DiaryService
```

形成单向 DAG，无循环。

审计：**PASS**

---

## 58E.17 分页与性能审计

当前不做全局分页。

DiaryWall 性能问题：

```text
优化 Card
↓
分批 / Load More
↓
仍不满足
↓
再审 Model/View
```

审计：**PASS**

---

## 58E.18 Qt 过度设计审计

当前未建立：

```text
IRefreshablePage hierarchy forest
BaseDashboardPage
BusinessEventBus
GenericPaginator<T>
GenericTableBinder
Service Locator
Qt Session Singleton
```

审计：**PASS**

---

## 58E.19 Gate 4 实现风险清单（不阻塞 Gate 3.5）

以下转入 Gate 4：

```text
Qt buildUi / refresh 函数 ≤ 20 行约束
Model beginResetModel/endResetModel
Signal 重复 connect 防护
QSignalBlocker / isRefreshing
Dialog accept() 成功条件
Card parent-child ownership
selected stable ID / scroll restore
Empty / Error / Refreshing 具体 Widget
Page onActivated() 最终技术形式
Qt 日期/数字/中文格式化
```

这些属于实现质量，不再重开架构。

---

## 58E.20 Qt Presentation 全景架构图

```text
                         main()
                           │
                           ▼
               Application / AppController
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          ▼                ▼                ▼
    Repositories        Services      PersistenceCoordinator
          │                │
          └────────────────┼────────────────┘
                           │
                           ▼
                      LoginWindow
                           │
                  AuthenticationService
                           │
                           ▼
                      LoginResult
                           │
                           ▼
                  ApplicationContext
             accountId / name / capabilities
                           │
                           ▼
                       MainWindow
                  <<Application Shell>>
                           │
                  navigateTo(PageId)
                           │
                  lazy create / reuse
                           │
                           ▼
                    unified onActivated()
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          ▼                ▼                ▼
    QTableWidget       Model/View        Custom Feed
    simple pages       advanced pages    DiaryWall
          │                │                │
          │      ┌─────────┴─────────┐      ▼
          │      ▼                   ▼  DiaryPostCard
          │ StudentTableModel  OperationLogModel │
          │      │                   │            │ signal
          │      ▼                   ▼            ▼
          │    Proxy               Proxy     DiaryWallPage
          │                                      │
          └──────────────────┬───────────────────┘
                             ▼
                          Services
                             │
                             ▼
                    Domain / Repository
```

---

## 58E.21 页面刷新总流程图

```text
User navigation
      │
      ▼
MainWindow.navigateTo(PageId)
      │
      ▼
resolve / lazy-create Page
      │
      ▼
show Page
      │
      ▼
Page.onActivated()
      │
      ▼
query Service
      │
   ┌──┴───────────────┐
   │                  │
Success             Failure
   │                  │
   ▼                  ▼
Loaded / Empty      Error State
   │               + Retry
   ▼
apply Presentation state
filter / sort / selected ID
```

业务修改：

```text
Page / Dialog
      │
      ▼
Service
      │
      ▼
business + persistence
      │
   ┌──┴──────┐
   │         │
Success     Failure
   │         │
   ▼         ▼
accept() /  keep Dialog
mark success show error
   │
   ▼
refresh current visible area
   │
   ▼
other Pages wait
   │
   ▼
next onActivated()
   │
   ▼
fresh query
```

---

## 58E.22 Gate 3.5 FINAL AUDIT 表

| 审计维度 | 结论 |
|---|---|
| Qt / Service 边界 | PASS |
| Qt / Repository 隔离 | PASS |
| Domain Qt 污染 | PASS |
| ApplicationContext | PASS |
| Login / Logout 生命周期 | PASS |
| MainWindow God Class | PASS |
| Page / Dialog 粒度 | PASS |
| Service 显式依赖 | PASS |
| stable ID | PASS |
| 权限双重保障 | PASS |
| 页面刷新一致性 | PASS |
| Query Failure / Empty | PASS |
| Dialog 成功语义 | PASS |
| Tab 按需刷新 | PASS |
| EventBus / cache overdesign | PASS |
| QTableWidget 使用合理性 | PASS |
| Model/View 使用合理性 | PASS |
| Proxy / Service 筛选边界 | PASS |
| Ranking 业务排序 | PASS |
| DiaryWall Card 架构 | PASS |
| DiaryWall 隐私边界 | PASS |
| Administrator 点赞规则 | PASS |
| DiaryWall 图片范围 | PASS |
| 分页策略 | PASS |
| Empty / Error / Refreshing UX | PASS |
| Qt dependency cycle | PASS |
| Qt 过度设计 | PASS |
| 高级技术效果价值 | PASS |
| 答辩可解释性 | PASS |

---

## 58E.23 Gate 3.5 最终状态

```text
Gate 3.5.1
Qt Presentation 总边界
→ FROZEN

Gate 3.5.2
ApplicationContext / Login / MainWindow
→ FROZEN

Gate 3.5.3
Page / Dialog 页面体系
→ FROZEN

Gate 3.5.4A
表格型页面数据展示
→ FROZEN

Gate 3.5.4B
页面刷新与数据一致性
→ FROZEN

Gate 3.5.4C
DiaryWall 卡片信息流
→ FINAL FROZEN / PASS

Gate 3.5.4D
筛选、排序、分页与 Model/View
→ FROZEN / PASS

Gate 3.5
Qt Presentation / Application Architecture
→ FINAL PASS
```

冻结范围：

```text
D177 ～ D241
```

当前 Gate 3 总冻结范围：

```text
D1 ～ D258
```

下一步：

> **Gate 3.6——设计模式终审**


# 59. Gate 3.5 当前冻结决策索引

| 编号 | 决策 |
|---|---|
| D177 | Qt Presentation 负责输入、展示、导航、交互，不承担核心业务 / Repository / Persistence 职责 |
| D178 | Qt 不直接访问 Repository / Persistence，不长期持有 Repository 内领域对象地址 |
| D179 | Service 可向 Qt 返回普通 C++ 查询结果值，不建立重型 DTO / Mapper 体系 |
| D180 | Qt 跨交互长期定位业务目标使用 stable ID |
| D181 | 当前登录上下文属于 Presentation / Application，不使用全局 SessionManager / hidden currentUser |
| D182 | capabilities 用于 UI 入口控制，最终授权仍由 Service 执行 |
| D183 | Signal / Slot 限于 Qt Presentation / Application，不侵入纯 Domain / Repository |
| D184 | Qt 可做体验级预校验，但 Service / Domain 独立保护正式业务规则 |
| D185 | Service → Qt 执行结果必须具有错误分类，SeverePersistenceFailure 不得按普通失败处理 |
| D186 | LoginWindow 与认证后 MainWindow 生命周期分离 |
| D187 | 引入轻量 ApplicationContext，只保存当前 UI 会话身份 / capabilities |
| D188 | AuthenticationService 向 Qt 返回值语义 LoginResult，不返回长期 User* |
| D189 | V1.0 采用统一 MainWindow + capabilities 驱动角色导航 |
| D190 | MainWindow 是 Application Shell，不成为业务 God Class |
| D191 | 认证后主页面使用统一页面容器，推荐 QStackedWidget；页面不互相调用业务实现 |
| D192 | 跨页面 / Dialog 业务目标传 stable ID，详情按 ID 重新查询 |
| D193 | logout 销毁认证后 UI 树与 Context，账号切换统一 logout → login |
| D194 | Application / AppController 作为轻量组合根，统一管理 Repository / Service / 窗口生命周期 |
| D195 | Qt 按主 Page + Dialog + 公共 Page + Shell 组织，领域对象与页面不一一对应 |
| D196 | Student 主 Page 收敛为 Dashboard / MyVolunteer / Achievement / Profile + 两个公共页面 |
| D197 | VolunteerRecord 创建 / 编辑复用 Dialog；详情使用局部 Dialog |
| D198 | Administrator 主 Page 收敛为 Dashboard / Review / StudentManagement / Configuration / DiaryModeration / Statistics / OperationLog + 公共页面 |
| D199 | 审核、Approved 强制更正、物理删除归属于 Review / Record Governance 模块 |
| D200 | Category / BadgeRule / Semester 共处 ConfigurationPage 不同 Tab，但 Service 边界保持分离 |
| D201 | DiaryWallPage 负责公共浏览；DiaryModerationPage 负责后台展示审核 / 治理 |
| D202 | RankingPage 公共复用；管理员聚合统计单独 StatisticsPage |
| D203 | Export 为一次性 ExportDialog；OperationLogPage 不自己实现底层导出 |
| D204 | Page / Dialog 只显式依赖真实需要的 Service，Context 不作为 Service Locator |
| D205 | Page 不维护第二份业务真相；业务变化后重新通过 Service 获取权威查询结果 |
| D206 | VolunteerRecord 单一权威 Repository + Pending / Rejected / Approved / Withdrawn 四个逻辑状态池，不按状态拆物理数据库 |
| D207 | Qt 表格技术采用“效果准入”而非预设唯一方案；高级 Model/View 只有形成综合优势时采用 |
| D208 | Qt 表格只消费 Service 的纯 C++ 只读查询结果，不直接绑定 Repository / Domain 权威对象 |
| D209 | 表格行通过 stable ID / Qt::UserRole 定位业务对象，row index 与展示文本不承担业务身份 |
| D210 | VolunteerRecord 四状态在 Qt 中仍只是同一权威数据集的四个逻辑查询视图 |
| D211 | ReviewPage 的审核 / 更正 / 删除等操作始终以 recordId 定位目标，不依赖行位置 |
| D212 | 不同页面可根据真实交互复杂度分别选择 QTableWidget 或 QTableView / Model / Proxy，不强制技术形式统一 |
| D213 | Service Result 保持纯 C++ 业务语义；中文文本、单位、日期等显示格式由 Presentation 转换 |
| D214 | 不预建全局 TableHelper / TableBinder；真实出现稳定重复且有收益时才抽取轻量 Presentation helper |
| D215 | Page/Dialog 中的数据只是 Presentation Snapshot；最新事实通过 Service 从权威 Repository / 派生计算重新获得 |
| D216 | 当前 Page 的业务修改完整成功后立即重新查询刷新；Dialog Accepted 只代表真正成功 |
| D217 | 可导航业务 Page 使用统一 onActivated / 等价生命周期入口，MainWindow 不硬编码页面具体刷新方法 |
| D218 | 跨 Page 不建立直接刷新通知；其他 Page 在下一次激活时重新查询 |
| D219 | 当前不引入 dirty flag 网络、业务 EventBus 或复杂缓存失效系统 |
| D220 | 多 Tab Page 只要求当前可见 Tab 最新；Tab 一旦激活必须通过 Service 强制重新查询 |
| D221 | Dashboard / Ranking / Badge / Statistics 等派生页面激活时重新查询 / 计算 |
| D222 | 当前允许全量重建；长列表恢复 selected stable ID / scroll，真实性能问题后再升级增量 Model/View |
| D223 | 状态修改采用 Service 完整成功后再更新 UI，不引入 optimistic UI / rollback 复杂机制 |

---
