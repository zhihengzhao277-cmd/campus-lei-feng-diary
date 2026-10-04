# Gate 3.8 — 最终架构图与流程图终审

> 当前状态：**REOPENED — 图示问题已登记，修正延期**。
>
> 五张项目源原图全部保留；本轮不采用任何修正版图。G38-01～G38-06 的问题记录与后续修正要求见本文件末尾的“当前图示问题登记（v3.0 有效）”。

> **Gate 4 实现细化导航**
>
> 本文冻结的是 Gate 3 架构层语义；对应实现级规则已在 Gate 4 进一步细化。
>
> 后续 AI / Codex / 开发者在实现或审计本文内容前，应先读取：
>
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> Gate 4 负责“具体如何实现”，不得反向修改本文已经冻结的 Gate 3 架构。若真实实现发现冲突，应按 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 中的 STOP 规则暂停并显式 reopen，不得自行覆盖。

## v2.4 历史完成记录（不覆盖当前状态）

下方逐行迁移 v2.4 第 14950～15160 行，保留图表收口说明、各图职责和历史状态；其中“FINAL PASS”仅是 v2.4 的历史结论，已被本页顶部的 REOPENED 状态撤销。

## A.14 v2.4 修订说明

本次修订在 v2.3 基础上**只追加 Gate 3.8 最终架构图收口成果与 Gate 3 前期关键纠偏总结**，不改写、不删除、不重排 v2.3 既有正文与历史版本记录。

### 1. Gate 3.8 最终架构图体系完成

Gate 3.8 的目标不是重新打开 Gate 3.1～Gate 3.7 的全部架构决策，而是把前序已经冻结的架构整理为一套可直接用于实现、课程设计报告和答辩说明的最终图体系，并检查图与既有冻结结论之间不存在明显冲突。

本阶段最终形成五张主图：

```text
1. 系统总架构图
2. 核心 Domain UML
3. Service–Repository 模块依赖图
4. Qt 页面 / 导航图
5. 关键运行流程图
```

> **历史状态标记：以下为 v2.4 历史结论，不得作为当前任务依据；当前状态以本文件顶部的 `REOPENED` 为准。**

当前状态：

```text
Gate 3.8
→ FINAL PASS
```

### 2. 系统总架构图最终修正

系统总架构图最终明确代码依赖方向：

```text
Qt Presentation
        ↓
Application / Service
      ↙       ↘
   Domain    Repository
                ↓
        Persistence / Files
```

冻结说明：

- Presentation 只通过 Service 进入业务层；
- Service 可以依赖 Domain 与 Repository；
- Repository 负责 Domain 数据的存取与文件持久化接口；
- Domain 不依赖 Repository、Service 或 Qt；
- 不再使用容易误导为 `Domain → Repository` 的线性分层箭头；
- `AppController` 继续作为 Composition Root 负责对象创建与依赖注入；
- `PersistenceCoordinator` 继续只承担跨 Repository 持久化协调，不被描述为数据库事务管理器。

### 3. 核心 Domain UML 收口

核心 Domain UML 以当前已冻结 Domain 事实为准，集中表达：

```text
User <<abstract>>
├── Student
└── Administrator

VolunteerRecord
VolunteerCategory
BadgeRule
Semester
DiaryPost
LikeRelation
OperationLog

UserCapabilities
Permission

关键状态 / 类型枚举
```

本次收口特别澄清：

- `Administrator` 的稳定 Domain 关系包含 `VolunteerRecord.reviewerAccountId` 与 `OperationLog.operatorAccountId`；
- Administrator 对 Student、VolunteerCategory、BadgeRule、Semester、DiaryPost 等对象的治理职责通过 Service 表达，不伪造为持久拥有关系；
- `DiaryPost` 不重复保存 `publisherAccountId`，作者由 `recordId → VolunteerRecord.ownerAccountId` 推导；
- `VolunteerRecord` 对 VolunteerCategory 保留 `appliedCategoryId` 与 `finalCategoryId` 两种不同语义角色；
- `Student → LikeRelation` 与 `DiaryPost → LikeRelation` 的关系分别明确，不存在 `VolunteerRecord → LikeRelation` 直接关系；
- OperationLog 的 `targetType + targetId` 继续作为弱历史审计引用；
- Semester 与 VolunteerRecord 的学期归属继续按 `serviceDate` 与日期范围动态判定，不增加 `VolunteerRecord.semesterId`；
- `currentSemesterId` 继续保持外部配置事实，不为安放该字段临时新增无必要 Domain 类。

### 4. Service–Repository 模块图收口

本次最终明确：

- 各业务 Service 作为并列应用服务存在；
- 不提前冻结 `StatisticsService → RankingService → BadgeService` 这类没有业务必要的链式依赖；
- Service–Service 协作只在真实流程需要时存在，整体必须保持无循环依赖；
- ExportService 位于读取 / 输出端，不允许其他业务 Service 反向依赖 ExportService；
- ExportService 继续使用已冻结的 Export Strategy，将查询结果转换为中性 `ExportDocument` 后交给 CSV / Markdown 等序列化策略；
- Repository 不反向依赖 Service；
- Domain 不依赖 Repository。

### 5. Qt 页面 / 导航图收口

Qt Presentation 最终保持：

```text
LoginWindow
   ↓
AppController
   ↓
MainWindow
   ↓
Student / Administrator / Common Pages
```

并冻结以下表现层边界：

- MainWindow 是中心导航壳；
- Page 之间不互相持有业务引用，不建立 Page→Page 刷新网络；
- 导航激活统一采用 `onActivated()` 或等价语义重新查询最新数据；
- Page / Dialog 只消费 Service 的纯 C++ View Result；
- Qt 页面不直接访问 Repository；
- stable ID 是跨页面与表格选择的业务身份；
- 不为 Gate 3.8 临时新增尚未冻结的新 Page / Dialog；
- 日记墙继续采用 `QScrollArea + FeedContainer QWidget + QVBoxLayout + DiaryPostCard QWidget`；
- StudentManagementPage 与 OperationLogPage 等筛选 / 排序收益明显的页面继续采用 Qt Model/View。

### 6. 关键运行流程图收口

修改型业务流程继续遵守：

```text
Qt 发起操作
↓
Service 权限 / 业务校验
↓
Domain / Repository 内存状态修改
↓
PersistenceCoordinator 完成持久化
↓
Service 返回成功
↓
当前 Page / Dialog 重新查询并刷新
```

冻结原则：

- 不采用 optimistic UI；
- 持久化失败时 UI 不假装业务成功；
- Dialog 只有在完整业务成功后才能 Accepted；
- 其他页面不通过事件网络立即互刷，而是在下次 activation 时重新查询；
- 登录流程中，Student 的 `lastLoginAt` 更新必须在持久化成功后，才完成认证态建立并进入 MainWindow。

### 7. Gate 3 前期关键纠偏总结

本节仅作为 Gate 3 历史设计演进总结，不改变前文已经冻结的 D1～D258 编号范围。

Gate 3 相比 Gate 1 / Gate 2 主要完成了以下纠偏与架构落实：

1. 将 Student / Administrator 的领域角色落实为 `User <<abstract>> → Student / Administrator` 的真实继承 / 多态体系；
2. 将“角色原则能力”和“一次具体操作是否合法”拆分为 Permission / UserCapabilities 与 Service + Domain 两层；
3. 将对象之间长期关系统一收敛到 stable ID，避免裸指针、循环持有和双向外键；
4. 删除 `DiaryPost.publisherAccountId` 重复真相，发布者由关联 VolunteerRecord 推导；
5. 将 Semester 修正为日期范围对象，删除 `isCurrent` 持久字段，以外部唯一 `currentSemesterId` 表达当前学期；
6. `VolunteerRecord` 不保存 `semesterId`，学期归属统一按 `serviceDate` 动态判定；
7. 将 VolunteerRecord 收紧为 Pending / Approved / Rejected / Withdrawn 四状态受控状态机，不使用 Deleted / Invalid 状态；
8. 当前审核结果由 VolunteerRecord 保存，历次关键管理员行为由 OperationLog 保存，不在 VolunteerRecord 内堆叠完整审核历史；
9. 将只读数据导出从 OperationLog 记录范围中移除，日志聚焦有责任追溯价值的管理员状态变更行为；
10. 正式建立 Service Layer，避免 Student / Administrator 演化为 God Class；
11. 正式建立 Repository / Persistence 边界、内存权威状态、Prepare / Commit 与 PersistenceCoordinator；
12. 正式建立 Qt Presentation / ApplicationContext / MainWindow / Page / Dialog 边界，Qt 不侵入 Domain；
13. 设计模式只保留真实成立的 Repository Pattern、Service Layer、Composition Root、Qt Model/View 与 Export Strategy，不为评分强行贴 Factory / State / Facade / Command 等标签；
14. 自定义模板最终收敛为一个具有真实应用价值的 `Result<T>`；
15. 原 Gate 1 中曾出现的“学生申请下架自己的日记墙内容”在后续架构中未形成完整权限 / Service / Qt 设计，本阶段正式放弃，不再向 V1.0 追加该功能；`TakenDown` 状态继续用于 Administrator 治理下架。

### 8. Gate 3.9 状态说明

Gate 3.9 **尚未正式开始**。

其下一阶段任务仍为：

> **V0.1 → V0.2 → V0.9 → V1.0 的架构演进映射。**

Gate 3.9 需要在现有最终 V1.0 架构基础上，进一步明确：

- 各核心类 / Service / Repository / Qt 架构分别从哪个程序版本开始出现；
- V0.1 保留哪些基本正确但尚未成熟的结构；
- V0.2 基于真实问题完成哪些重构；
- V0.9 如何验证 Qt 与既有业务层的接口适配；
- V1.0 最终补齐哪些完整架构能力。

当前不得将本次“Gate 3 关键纠偏总结”视作 Gate 3.9 的完成结果。

### 9. v2.4 当前阶段状态

```text
Gate 3.1 → PASS
Gate 3.2 → PASS
Gate 3.3 → FINAL PASS
Gate 3.4 → FINAL PASS
Gate 3.5 → FINAL PASS
Gate 3.6 → FINAL PASS
Gate 3.7 → FINAL PASS
> **历史状态标记：以下为 v2.4 历史阶段状态，不得作为当前任务依据；当前状态以本文件顶部的 `REOPENED` 为准。**

Gate 3.8 → FINAL PASS
Gate 3.9 → NOT STARTED

Frozen decisions:
D1 ～ D258

Gate 3 overall:
NOT FINAL PASS
```

下一步：

> **Gate 3.9——V0.1 → V0.2 → V0.9 → V1.0 架构演进映射。**


---

## 当前图示问题登记（v3.0 有效）

## 1. 图示资产

| 原图 | 主题 | 当前处理 |
|---|---|---|
| `08-core_domain_uml_gate3_8_frozen-1-.png` | 核心 Domain UML | 保留；本轮未发现已确认的图—文冲突。 |
| `09-ChatGPT-Image-2026-9-2-23_46_05-3-.png` | Qt 页面与导航 | 保留；G38-03～G38-06 待后续集中修正。 |
| `10-ChatGPT-Image-2026-9-2-23_46_05-2-.png` | Service–Repository 依赖 | 保留；G38-02 待后续集中修正。 |
| `11-ChatGPT-Image-2026-9-3-00_07_05-2-.png` | 关键运行流程 | 保留；本轮未发现已确认的图—文冲突。 |
| `12-ChatGPT-Image-2026-9-3-00_07_04-1-.png` | 系统总架构 | 保留；G38-01 待后续集中修正。 |

## 2. 已登记问题

| ID | 图示问题 | 当前冻结依据 | 后续修正要求 |
|---|---|---|---|
| G38-01 | 系统总架构图缺少 `StudentVolunteerService`。 | D65–D66 | 补入学生侧志愿流程服务，保持 Qt → Service → Domain / Repository 方向。 |
| G38-02 | Service–Repository 图缺少 `StudentVolunteerService`。 | D65–D66 | 补齐真实依赖，不制造 Repository → Service 反向依赖或循环。 |
| G38-03 | Qt 导航图缺少 `DiaryModerationPage`。 | D198、D201 | 管理员侧补入该页，日记审核/治理只归此页。 |
| G38-04 | Qt 导航图没有学生侧公共 `RankingPage` 入口。 | D202 | 显示学生与管理员共同使用该页面。 |
| G38-05 | `ReviewPage` 职责混入日记审核。 | D199、D201 | 限定为志愿审核与记录治理。 |
| G38-06 | `ExportDialog` 写为 CSV / Excel。 | D203、D243 | 当前冻结输出为 CSV / Markdown。 |

## 3. 本轮结论

图示问题不改变 D1～D258，也不阻塞本轮 Gate 3 文档重构。Gate 3.8 不能恢复 FINAL PASS；当后续集中修图并完成图—文复核后，才可单独重审。

> **实现级阅读说明**
>
> 本文图示表达 Gate 3 架构关系，不承担 Gate 4 实现协议细节。
>
> Stable ID、Persistence、CSV、Result/Error 等实现规则统一由：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
> 继续路由。
