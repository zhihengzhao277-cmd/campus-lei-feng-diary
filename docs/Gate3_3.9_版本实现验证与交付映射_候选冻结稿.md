# Gate 3.9 — V0.1 → V0.2 → V0.3 → V1.0 版本实现、验证与交付映射

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 3 —— 架构审计  
> 前置依赖：Gate 3.1～3.7 当前冻结架构  
> 当前冻结决策范围：D259～D276  
> 文档性质：Gate 3.9 候选冻结稿 / 当前版本实现路线规范  
> 目标：明确最终架构如何按真实程序版本落地、验证、Qt 接驳并形成最终成果  
> 重要说明：本文件采用新的正式版本路线 `V0.1 → V0.2 → V0.3 → V1.0`。它覆盖此前《版本迭代规则与 Gate 3 架构原则 v1.1》中 `V0.1 → V0.2 → V0.9 → V1.0` 的旧程序版本路线；旧文件中的 Gate 3 架构原则仍可继续作为背景原则，但其中 V0.9 版本编号与“V0.1 允许较粗糙架构、V0.2 再重构”的版本演进设定不再作为当前实现路线。
>
> 本文件不改变 Gate 3.1～3.7 已冻结的最终架构事实。Gate 3.8 当前仍为 `REOPENED`，其图示修正任务独立存在；Gate 3.9 的版本实现映射不等于 Gate 3.8 已恢复 FINAL PASS。
>
> 本文件纳入项目源后，应同步更新：
>
> 1. `Gate3_00_总览_阅读规则与当前状态.md` 中 Gate 3.9 状态；
> 2. 《当前权威需求与架构检索索引》中 Gate 3.9 状态与版本路线；
> 3. 如继续保留《版本迭代规则与 Gate 3 架构原则 v1.1》，应增加显式覆盖说明，防止其他 AI 继续读取旧的 V0.9 路线。

---

> **Gate 4 实现细化导航**
>
> 本文冻结的是 Gate 3 架构层语义；对应实现级规则已在 Gate 4 进一步细化。
>
> 后续 AI / Codex / 开发者在实现或审计本文内容前，应先读取：
>
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> Gate 4 负责“具体如何实现”，不得反向修改本文已经冻结的 Gate 3 架构。若真实实现发现冲突，应按 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 中的 STOP 规则暂停并显式 reopen，不得自行覆盖。

# 1. Gate 3.9 的职责

Gate 3.1～3.7 已经回答：

```text
最终系统应该采用什么架构？
```

包括：

```text
Gate 3.1
→ User / Student / Administrator
→ 继承、权限、多态

Gate 3.2
→ Domain 关系
→ stable ID
→ 生命周期
→ 状态机

Gate 3.3
→ Application / Service

Gate 3.4
→ Repository / Persistence

Gate 3.5
→ Qt Presentation / Application

Gate 3.6
→ 设计模式

Gate 3.7
→ Result<T> 自定义类模板
```

Gate 3.9 不重新设计上述内容。

Gate 3.9 回答：

> **已经冻结的最终架构，真实开发时按照什么顺序实现、验证、接入 Qt、调试并最终冻结？**

因此 Gate 3.9 是：

```text
Architecture
        ↓
Implementation staging
        ↓
Verification staging
        ↓
GUI integration
        ↓
Final delivery
```

而不是重新进行一次架构设计。

---

# 2. 当前正式版本路线

当前程序版本路线正式冻结为：

```text
V0.1
最终业务架构的完整 Console 首实现
“全部正式业务先完整实现”

        ↓

V0.2
Console 全流程验证、联调、Debug、稳定化
“全部业务真正稳定跑通”

        ↓

V0.3
Qt Presentation 完整接入
“以成熟业务层接入完整 GUI”

        ↓

V1.0
Qt 全流程 Debug、稳定化、最终收口
“最终课程设计成果”
```

四个版本承担不同职责：

| 版本 | 核心定位 | 主要变化 |
|---|---|---|
| V0.1 | 完整控制台首实现 | 实现最终业务架构与全部正式业务，表现层使用 Console |
| V0.2 | 控制台稳定版 | 系统级联调、测试、异常路径、持久化验证、真实 Bug 修复 |
| V0.3 | Qt 完整接入版 | 用 Qt Presentation 替换正式 Console UI，完整接入既有 Service |
| V1.0 | 最终成果版 | Qt 全系统调试、稳定、UI 收口、最终测试、文档一致性与交付 |

---

# 3. D259 — 版本演进总原则

> **自 V0.1 起，项目即按照 Gate 3.1～Gate 3.7 已冻结的最终业务架构标准实现 Domain、Application / Service、Repository / Persistence、设计模式与自定义类模板，不再通过人为弱化早期架构来制造版本演进。V0.1 与最终 V1.0 的主要结构性差异在于表现层：V0.1 采用 Console Presentation；V0.3 起接入 Qt Presentation。V0.2 与 V1.0 分别承担控制台系统和 Qt 系统的完整业务验证、缺陷修复与稳定化。**

该决策意味着：

```text
不采用：
V0.1 故意写成巨大 main()
↓
V0.2 再“发现”需要 Service / Repository

不采用：
V0.1 故意大量 role + if/else
↓
V0.2 再“发现”需要 User 多态

不采用：
V0.1 业务逻辑直接写在 Console
↓
V0.3 再为了 Qt 被迫搬业务
```

当前采用：

```text
V0.1 起
Domain / Service / Repository / Persistence 已按最终方向建立

变化主要发生在：
实现成熟度
测试成熟度
Presentation 技术
最终稳定性
```

---

# 4. V0.1 —— 最终业务架构的完整控制台首实现版

## 4.1 D260 — V0.1 定位

> **V0.1 定位为“最终业务架构的完整控制台首实现版”。除 Qt Presentation 外，Gate 3.1～3.7 已冻结的核心 Domain、Application / Service、Repository / Persistence、设计模式、自定义模板及主要业务规则原则上均应在 V0.1 中实现。Console Presentation 只承担输入、菜单、结果显示和简单交互，不承载核心业务逻辑。**

V0.1 不是：

```text
Demo
最小原型
临时脚本
只有核心几个功能的版本
```

而是：

```text
完整业务系统
+
完整文件持久化
+
最终方向 OOP 架构
+
Console Presentation
```

---

> **V0.1 实现前置约束 → Gate 4**
>
> V0.1 正式编码前必须读取：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> `Gate4.1～4.6` 构成 V0.1 的实现级前置约束。编码过程中如触发 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 定义的 STOP 点，应暂停实现并进行对应审计，不得由执行层自行补造实现规则。

## 4.2 V0.1 总体架构

```text
┌───────────────────────────────────────┐
│         Console Presentation          │
│                                       │
│ 登录 / 菜单 / 输入 / 输出 / 选择 / 提示 │
└───────────────────┬───────────────────┘
                    │
                    ▼
┌───────────────────────────────────────┐
│        Application / Service          │
│                                       │
│ AuthenticationService                 │
│ UserManagementService                 │
│ StudentVolunteerService               │
│ VolunteerReviewService                │
│ StatisticsService                     │
│ RankingService                        │
│ BadgeService                          │
│ DiaryService                          │
│ RuleConfigurationService              │
│ SemesterService                       │
│ OperationLogService                   │
│ ExportService                         │
└───────────────┬──────────────┬────────┘
                │              │
                ▼              ▼
             Domain        Repository
                               │
                               ▼
                         Persistence
```

基本调用方式：

```text
Console
↓
Service
↓
Domain / Repository
↓
Persistence
↓
Result<T> / OperationResult
↓
Console 显示
```

Console 不允许绕过 Service 成为第二套业务实现。

---

# 5. V0.1 的架构构成

## 5.1 Domain 从 V0.1 即按最终方向建立

至少包含：

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
```

以及必要值对象、强类型枚举与领域类型，例如：

```text
UserCapabilities
Permission
AccountStatus
RecordStatus
DiaryDisplayStatus
OperationType
OperationTargetType
BadgeLevel
...
```

从 V0.1 开始即遵守：

```text
User::capabilities()
→ pure virtual

Student / Administrator
→ 运行时多态

Permission
→ enum class

RecordStatus
→ enum class + 受控状态转换

关键业务字段
→ 不提供机械 public setter
```

不因为 V0.1 是 Console 版本而降低 Domain 设计标准。

---

## 5.2 Service 从 V0.1 即按最终边界实现

V0.1 不允许将业务规则塞入菜单函数。

核心 Service 直接采用 Gate 3.3 已冻结方向：

```text
AuthenticationService
UserManagementService

StudentVolunteerService
VolunteerReviewService

StatisticsService
RankingService
BadgeService

DiaryService

RuleConfigurationService
SemesterService

OperationLogService
ExportService
```

Service 继续承担：

```text
业务用例协调
跨对象条件
Permission
AccountStatus
ownership
对象状态
业务规则
日志协调
持久化协调
```

Domain 继续承担：

```text
自身状态转换
自身不变量
```

Repository 继续承担：

```text
权威对象集合访问
持久化映射
```

---

## 5.3 Repository / Persistence 从 V0.1 即真实启用

V0.1 不是纯内存程序。

需要直接采用：

```text
UserRepository
VolunteerRecordRepository
RuleRepository
SemesterRepository
DiaryRepository
OperationLogRepository
```

物理数据至少对应：

```text
data/
├── students.csv
├── administrators.csv
├── volunteer_records.csv
├── volunteer_categories.csv
├── badge_rules.csv
├── semesters.csv
├── diary_posts.csv
├── likes.csv
├── operation_logs.csv
└── system_config.txt
```

核心运行模型：

```text
启动
↓
加载权威文件
↓
Repository 运行期内存权威集合
↓
Service 操作
↓
成功修改内存状态
↓
Persistence 提交
↓
提交成功后才报告业务成功
```

不采用：

```text
每次查询
↓
重新打开 CSV
↓
全文件解析
```

作为正常运行模型。

---

## 5.4 PersistenceCoordinator 从 V0.1 即存在

跨 Repository / 多物理文件业务可能包括：

```text
删除 Approved VolunteerRecord
↓
VolunteerRecord 删除
↓
DiaryPost 删除
↓
LikeRelation 级联删除
↓
OperationLog 形成
```

因此 V0.1 即采用已冻结的轻量持久化协调机制。

PersistenceCoordinator：

```text
不是数据库事务管理器
不是完整 Unit of Work
不自动追踪所有 dirty entity
```

其职责是：

```text
协调已知受影响 Repository / 文件
prepare
deterministic commit
检测 partial commit failure
```

---

## 5.5 Result<T> / OperationResult / ServiceError 从 V0.1 即存在

V0.1 Service 边界直接采用：

```text
Result<T>
OperationResult
ServiceError
```

例如：

```text
login()
→ Result<AuthenticatedUserInfo>

queryRanking()
→ Result<RankingResult>

queryDiaryWall()
→ Result<DiaryPostPublicView collection / equivalent>

approve(...)
→ OperationResult
```

不采用：

```text
Console 阶段所有 Service 先返回 bool
↓
Qt 阶段再重构结果系统
```

错误语义至少能够区分：

```text
Validation
BusinessRuleViolation
PermissionDenied
NotFound
PersistenceFailure
SeverePartialCommit
```

具体枚举精确名称仍可在 Gate 4 实现阶段冻结。

---

## 5.6 设计模式从 V0.1 即按当前最终方案存在

V0.1 不等 Qt 才引入架构模式。

当前使用：

```text
Repository Pattern
Service Layer
Composition Root
Export Strategy
```

以及：

```text
Result<T>
→ 自定义类模板
```

Qt Model/View 属于 Qt Presentation，因此从 V0.3 才真正出现。

Qt Signal/Slot 同理从 V0.3 起出现。

---

## 5.7 Composition Root 从 V0.1 即存在

概念：

```text
main()
↓
Application / AppController / equivalent composition location
│
├── 创建 Repositories
├── 启动数据加载
├── 创建 PersistenceCoordinator
├── 创建 PasswordHasher
├── 创建 Services
└── 创建 Console Presentation
```

`main()` 可以承担：

```text
对象装配
应用启动
应用退出
```

不能承担：

```text
审核
积分
排行榜
日记
点赞
持久化业务流程
```

---

# 6. D261 — V0.1 完整业务范围

> **V0.1 必须实现当前已冻结正式业务范围的完整控制台版本，包括用户认证与账号管理、学生志愿记录流程、管理员审核与治理、志愿类别与积分规则、月度 / 学期 / 总积分统计、排行榜与排行荣誉、专项徽章、学期配置、日记墙申请与管理员治理、点赞互动、操作日志、全局统计以及数据导出。V0.1 不因缺少 Qt 而删减业务模块；Qt 相关页面、导航、Model/View、卡片布局及 Signal/Slot 等纯表现层技术留至 V0.3。**

下面给出完整模块映射。

---

# 7. V0.1 业务模块详表

## 7.1 用户、认证与账号

### Student / Administrator 公共账号体系

```text
User
├── accountId
├── name
├── password-related data
├── accountStatus
└── capabilities()
```

### Authentication

需要实现：

```text
登录
密码验证
账号状态验证
实际 Student / Administrator 动态类型恢复
Student 成功登录后的 lastLoginAt 更新
```

禁止：

```text
通过账号长度判断角色
明文密码持久化
```

### Student 本人维护

至少：

```text
查看本人资料
修改 contact
修改本人密码
```

### Administrator 管理 Student

至少：

```text
创建 Student
查询 Student
禁用 Student
恢复 Student
重置 Student 密码
维护管理员允许修改的学生资料
```

当前不因 V0.1 自行扩展“管理员创建管理员”完整业务体系。

初始管理员可采用部署阶段预置账号。

---

## 7.2 学生志愿记录完整流程

必须覆盖：

```text
创建
提交
查询本人记录

Pending：
├── 修改本人记录
└── 撤回本人记录

Rejected：
├── 修改
└── 重新提交
```

状态机：

```text
创建并提交
→ Pending

Pending
├── Approved
├── Rejected
└── Withdrawn

Rejected
└── Pending
```

禁止：

```text
Approved → Pending
Approved → Rejected
Withdrawn → Pending
```

等当前未冻结普通转换。

---

## 7.3 管理员审核与治理

V0.1 需要实现：

```text
查看待审核记录
审核通过
审核驳回
确认 / 修改最终类别
确认 / 修改最终服务时长
审核备注
```

同时需要实现已通过记录治理：

```text
强制更正
物理删除异常 / 违规记录
```

审核通过时形成：

```text
finalCategoryId
finalDuration
settledCoefficient
finalScore
reviewerAccountId
reviewNote
```

历史关键管理员行为进入 OperationLog。

---

## 7.4 历史结算快照

V0.1 必须从第一次实现就遵守：

```text
VolunteerCategory.currentCoefficient
→ 只影响未来新结算

VolunteerRecord.settledCoefficient
VolunteerRecord.finalScore
→ 历史结算事实
```

后续修改类别当前系数：

```text
不得追溯改变已 Approved 记录 finalScore
```

---

## 7.5 Statistics

V0.1 实现：

### 学生相关

```text
月度积分
学期积分
总积分
有效服务总时长
有效记录数
指定 Category 有效服务时长
```

### 全局统计

至少：

```text
学生积分概览
各类别有效记录数量
各类别有效服务时长
全局有效服务总时长
```

派生结果不写入 Student。

---

## 7.6 Ranking

V0.1 实现：

```text
月度排行榜
学期排行榜
总积分排行榜
```

统一排序：

```text
积分降序
↓
有效服务总时长降序
↓
有效志愿记录数降序
↓
accountId / 学号升序
```

排行榜荣誉：

```text
第 1 名 → 金
第 2 名 → 银
第 3 名 → 铜
```

人数不足时不存在的绝对名次保持空缺。

历史月榜 / 学期榜不作为独立持久实体保存。

---

## 7.7 Badge

V0.1 实现：

```text
BadgeRule
├── badgeRuleId
├── badgeName
├── categoryId
├── bronzeThreshold
├── silverThreshold
├── goldThreshold
└── enabled
```

动态计算：

```text
Student
+
指定 Category 当前有效服务时长
+
当前 BadgeRule
↓
None / Bronze / Silver / Gold
```

当前徽章等级不持久化到 Student。

BadgeRule 改动后按当前有效数据动态重判。

---

## 7.8 Semester

V0.1 实现：

```text
Semester
├── semesterId
├── semesterName
├── startDate
└── endDate
```

以及：

```text
currentSemesterId
```

管理员可：

```text
管理学期配置
切换 currentSemesterId
```

硬约束：

```text
VolunteerRecord 不保存 semesterId
Semester 不保存 isCurrent
```

学期归属：

```text
semester.startDate
<= record.serviceDate
<= semester.endDate
```

动态判断。

---

## 7.9 日记墙

V0.1 即实现完整业务，不等 Qt。

流程：

```text
Approved VolunteerRecord
↓
Student 申请公开展示
↓
DiaryPost
PendingDisplayReview
↓
Administrator 审核
↓
Displayed
```

管理员治理：

```text
Displayed / eligible post
↓
Administrator takedown
↓
TakenDown
```

硬覆盖：

> **学生没有“申请下架自己的日记”功能。TakenDown 只用于管理员治理下架。**

同一 VolunteerRecord 最多对应一个 DiaryPost。

DiaryPost 不重复持久化 publisherAccountId：

```text
recordId
↓
VolunteerRecord.ownerAccountId
↓
推导发布 Student
```

---

## 7.10 点赞

V0.1 实现：

```text
Like
Unlike
```

唯一键：

```text
(studentAccountId, postId)
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

LikeCount：

```text
动态派生
不持久化
```

---

## 7.11 OperationLog

V0.1 从第一次真实业务操作开始写日志。

典型管理员关键变更包括：

```text
审核通过
审核驳回
审核时修正类别 / 时长
强制更正 Approved 记录
删除记录
创建 Student
禁用 Student
恢复 Student
重置密码
修改 VolunteerCategory
修改 BadgeRule
切换 / 修改 Semester
日记审核
日记治理下架
```

OperationLog 至少表达：

```text
logId
operatorAccountId
operationType
targetType
targetId
description
operationTime
```

目标业务对象被物理删除后：

```text
OperationLog 仍必须保留
```

只读 Export：

```text
不生成 OperationLog
```

---

## 7.12 Export

V0.1 即实现导出。

架构：

```text
ExportService
↓
业务结果 / Query Result
↓
中性 ExportDocument
↓
ExportFormatStrategy
├── CsvFormatStrategy
└── MarkdownFormatStrategy
↓
外部文件
```

当前正式格式：

```text
CSV
Markdown
```

不是 Excel。

格式策略不负责：

```text
权限
业务查询
Repository
OperationLog
Qt
```

---

# 8. D262 — V0.1 与 V0.2 的功能边界

> **V0.2 不预设承担新的主要业务模块开发。V0.2 的目标是在 V0.1 完整业务范围基础上完成系统级测试、跨模块联调、异常路径补全、真实 Bug 修复、持久化一致性验证与控制台全业务闭环稳定化。若 V0.1 实现过程中确有遗漏或缺陷，可以在 V0.2 补齐，但不得为了制造版本增量而故意将已知正式业务功能留到 V0.2。**

这意味着版本日志应优先记录真实问题，例如：

```text
Rejected → Pending 后 reviewerAccountId 未清空
删除来源记录后 LikeRelation 未级联清理
currentSemesterId 切换后学期统计读取旧上下文
CSV 中换行字段恢复错误
排行榜同分 tie-break 顺序错误
```

而不是人为制造：

```text
V0.2 新增排行榜
V0.2 新增日记墙
```

等本应属于 V0.1 的主要业务增量。

---

> **V0.1 → V0.2 审计停点 → Gate 4**
>
> V0.1 全功能第一次闭环后，进入 V0.2 前应按 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 触发 `STOP-11`，完成 V0.1 审计后再继续稳定化工作。

# 9. V0.2 —— Console 全流程测试、Debug 与稳定化

## 9.1 D263 — V0.2 定位

> **V0.2 定位为“控制台完整业务稳定版”。其任务不是继续扩充主要业务模块，而是在 V0.1 已完成全部正式业务功能的基础上，进行系统级联调、异常路径验证、持久化一致性检查、真实 Bug 修复和全业务闭环验收，使控制台版本达到可稳定运行、可重复测试、可作为 Qt 接入基线的状态。**

V0.1 与 V0.2 的关系：

```text
V0.1
“功能已实现”

↓

V0.2
“功能已被系统性证明能够稳定协作”
```

---

# 10. V0.2 四层验证结构

## 10.1 D264 — V0.2 测试原则

> **V0.2 采用 Domain、Service、Repository/Persistence、Console End-to-End 四层验证方式，不以单独菜单或单独函数“能够运行”作为完成标准。测试必须覆盖正常流程、边界状态、异常路径、跨模块联动以及程序重启后的持久化恢复。**

四层：

```text
L1 Domain Test
↓
L2 Service Test
↓
L3 Repository / Persistence Test
↓
L4 Console End-to-End Test
```

### L1 Domain

重点：

```text
状态机
受控状态转换
对象不变量
边界值
强类型语义
```

### L2 Service

重点：

```text
Permission
AccountStatus
ownership
跨对象条件
业务流程协调
日志形成
ServiceError
```

### L3 Repository / Persistence

重点：

```text
加载
保存
恢复
CSV 编解码
引用完整性
级联
prepare / commit
失败处理
```

### L4 End-to-End

重点：

```text
从登录开始
↓
真实用户操作完整业务
↓
退出
↓
重启
↓
恢复
```

---

# 11. V0.2 必须跑通的主业务链

## 11.1 学生记录完整生命周期

测试：

```text
Student 登录
↓
创建记录
↓
提交
↓
Pending
```

分支 A：

```text
Pending
↓
修改
↓
仍为 Pending
↓
管理员读取最新数据
```

分支 B：

```text
Pending
↓
撤回
↓
Withdrawn
```

验证 Withdrawn：

```text
不进入积分
不进入排行榜
不具备日记展示资格
不能被管理员继续普通审核
```

分支 C：

```text
Pending
↓
Administrator
├── Approved
└── Rejected
```

---

## 11.2 Rejected → 修改 → 重提 → 再审核

重点验证当前审核信息清理。

第一次：

```text
Rejected
reviewerAccountId = admin01
reviewNote = reason
```

重新提交：

```text
Rejected
↓
Student 修改
↓
Pending

reviewerAccountId = empty
reviewNote = empty
```

但历史：

```text
admin01 曾驳回
```

仍存在于 OperationLog。

第二次审核成功后：

```text
VolunteerRecord
→ 只反映当前有效审核信息

OperationLog
→ 保留历次关键管理员行为
```

---

## 11.3 审核修正类别 / 时长与历史结算

测试：

```text
Student 申报 Category A / 5h
↓
Administrator 审核修正
↓
Category B / 4h
```

检查：

```text
appliedCategoryId = A
finalCategoryId = B
申报时长保留其原有事实字段
finalDuration = 4h
settledCoefficient = B 审核时当前系数
finalScore = finalDuration × settledCoefficient
```

然后修改 B 当前系数。

历史 Approved 记录：

```text
settledCoefficient
finalScore
```

不得变化。

---

## 11.4 积分 → 排行榜 → 荣誉联动

至少准备多个学生。

验证：

```text
月度积分
学期积分
总积分
```

以及：

```text
月榜
学期榜
总榜
```

专门制造 tie：

```text
积分相同
↓
服务时长

仍相同
↓
有效记录数

仍相同
↓
学号 / accountId 升序
```

确认排序不依赖 Repository 当前容器顺序。

荣誉：

```text
1 → Gold
2 → Silver
3 → Bronze
```

人数不足时保持绝对名次空缺。

---

## 11.5 Badge 边界

例如：

```text
Bronze 10h
Silver 30h
Gold 50h
```

测试：

```text
低于门槛
等于门槛
略高于门槛
多个门槛交界
```

同时测试删除一条有效记录后：

```text
有效服务时长下降
↓
BadgeService 下一次查询动态重判
```

当前徽章可下降，因为其不是永久历史实体。

---

## 11.6 Semester 切换

准备至少两个时间不重叠或规则合法的学期。

记录分别落在不同 serviceDate。

切换：

```text
currentSemesterId = Semester A
↓
查询学期统计 / 学期榜

currentSemesterId = Semester B
↓
重新查询
```

结果应变化。

VolunteerRecord 本身：

```text
不得因切换 currentSemesterId 被改写
```

---

## 11.7 DiaryPost 全生命周期

测试：

```text
Approved Record
↓
Student request
↓
PendingDisplayReview
↓
Admin approve
↓
Displayed
↓
浏览
↓
Like / Unlike
↓
Admin takedown
↓
TakenDown
```

验证：

```text
PendingDisplayReview
→ 不公开、不互动

Displayed
→ 可公开、可互动

TakenDown
→ 普通公开列表不可见
→ 不接受新的普通学生互动
→ 既有 LikeRelation 保留
```

同时确认：

```text
Student 无自助下架功能
```

---

## 11.8 LikeRelation 唯一性

测试：

```text
Student A → Like Post X
→ success

Student A → Like Post X again
→ reject
```

然后：

```text
Unlike
↓
关系删除
↓
再次 Like
→ 可以创建
```

并验证不同 Student 对同一 Post 可分别建立关系。

---

## 11.9 删除 Approved VolunteerRecord 的级联

这是 V0.2 的核心高风险集成测试。

初始：

```text
Approved VolunteerRecord R
↓
Displayed DiaryPost P
↓
多个 LikeRelation
```

管理员删除 R：

```text
VolunteerReviewService
↓
删除 R
↓
调用 DiaryService / 对应子流程
↓
删除 P
↓
删除 P 的 LikeRelation
↓
形成 OperationLog
↓
Persistence 提交
```

之后重新查询：

```text
积分
排行榜
排行称号
Badge
```

应基于剩余权威事实重新计算。

不得维护第二份持久派生状态进行“手工减值”。

---

## 11.10 AccountStatus

测试：

```text
Student Active
→ 可登录、可执行 Student 业务

管理员 disable
↓
Disabled
```

此后：

```text
登录应拒绝
直接调用修改型 Student Service 也应拒绝
```

即使 Presentation 错误地给出入口，Service 仍必须守住授权边界。

恢复：

```text
enable
↓
业务能力恢复
```

历史记录和历史 LikeRelation 不因账号禁用删除。

---

## 11.11 OperationLog 完整性

对关键管理员写操作逐类检查：

```text
operatorAccountId
operationType
targetType
targetId
description
operationTime
```

重点：

```text
业务 target 被物理删除后
↓
OperationLog 仍可读取
```

---

## 11.12 Export 与 CsvCodec

导出：

```text
CSV
Markdown
```

测试 CSV 字段包含：

```text
逗号
双引号
换行
空字段
```

不允许简单 split(',')。

同时测试导出不产生 OperationLog。

---

## 11.13 退出 → 重启 → 恢复

完成多项业务操作后：

```text
退出进程
↓
重新启动
↓
加载
↓
重新登录
```

验证权威事实：

```text
Student
Administrator
VolunteerRecord
VolunteerCategory
BadgeRule
Semester
DiaryPost
LikeRelation
OperationLog
currentSemesterId
```

正确恢复。

再验证派生结果：

```text
积分
排行
Badge
LikeCount
```

由恢复后的权威事实重新计算正确。

---

## 11.14 损坏文件 / schema 错误

人为制造：

```text
CSV 格式错误
必需文件损坏
schema_version 不兼容
强业务引用损坏
```

系统应：

```text
检测启动失败
↓
阻止进入正常业务运行
```

禁止“部分 Repository 成功加载就继续运行”。

---

## 11.15 持久化失败

至少模拟一次无法正常落盘。

验证：

```text
内存动作发生
≠
业务已提交成功
```

只有 Persistence 成功后：

```text
Service → Success
```

否则返回对应持久化失败。

如果发生严重 Partial Commit：

```text
进入严重错误语义
不得继续把系统视为正常可写状态
```

---

# 12. D265 — V0.2 完成门槛

> **只有在全部正式业务模块能够通过 Console Presentation 完成完整业务闭环，核心状态机、权限、跨对象级联和派生结果通过验证，权威数据能够正确保存并在程序重启后恢复，且当前已发现的阻塞性 Bug 已修复后，V0.2 才可冻结。**

完成标准不写成：

```text
“完全不存在任何 Bug”
```

而采用：

```text
Blocking bug = 0
Critical business bug = 0
Critical persistence bug = 0

Known non-blocking defect
→ 必须登记
→ 明确是否进入 V0.3 / V1.0 修复
```

V0.2 冻结后：

> **它成为 Qt 接入的稳定业务基线。**

---

> **V0.2 → V0.3 审计停点 → Gate 4**
>
> V0.2 Console 系统冻结后，进入 Qt 实现前应按 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 触发 `STOP-12`，完成 Qt 前审计后再进入 V0.3。

# 13. V0.3 —— Qt Presentation 完整接入版

## 13.1 D266 — V0.3 总体定位

> **V0.3 定位为“Qt Presentation 完整接入版”。V0.2 已验证的 Domain、Application / Service、Repository / Persistence、设计模式、Result<T> 与核心业务规则原则上保持不变；V0.3 主要新增 Qt 窗口、页面、导航、Dialog、Model/View、Signal/Slot 与表现层数据映射，使全部正式业务能够通过 GUI 调用既有 Service 完成。**

核心变化：

```text
V0.2
Console Presentation
       ↓
Service

V0.3
Qt Presentation
       ↓
同一套 Service
```

---

# 14. V0.3 原则上保持不动的部分

## 14.1 Domain 不因 Qt 改造

禁止：

```cpp
class VolunteerRecord : public QObject
```

禁止 Domain 因 GUI 改成长期使用：

```text
QString
QDate
QIcon
QColor
```

等 Presentation 技术类型。

Domain 保持纯 C++。

---

## 14.2 Service 原则上不重写

Qt 页面继续调用：

```text
AuthenticationService
UserManagementService
StudentVolunteerService
VolunteerReviewService
StatisticsService
RankingService
BadgeService
DiaryService
RuleConfigurationService
SemesterService
OperationLogService
ExportService
```

Qt 不复制这些 Service 的业务规则。

---

## 14.3 Repository / Persistence 不向 Qt 暴露

Qt 页面不知道：

```text
文件路径
CSV 字段
Repository 容器
tmp / bak
commit 顺序
```

调用链始终：

```text
Qt
↓
Service
↓
Domain / Repository / Persistence
```

---

# 15. V0.3 Qt Application 结构

## 15.1 ApplicationContext

采用：

```text
ApplicationContext
├── currentAccountId
├── displayName
└── UserCapabilities
```

其职责：

```text
表示当前认证后的 UI 会话
控制当前 UI 应出现哪些角色能力入口
```

不保存：

```text
User*
Student*
Administrator*
Repository*
Service*
VolunteerRecord*
排行榜缓存
积分缓存
```

业务执行时仍显式传 actorAccountId，由 Service 重新解析权威 User。

---

## 15.2 LoginWindow

流程：

```text
Application bootstrap
↓
Repository startup
↓
LoginWindow
↓
输入 accountId + password
↓
AuthenticationService.login(...)
↓
Result<AuthenticatedUserInfo>
```

失败：

```text
停留 LoginWindow
显示适当失败反馈
```

成功：

```text
Student lastLoginAt 等必要写回完成
↓
Persistence 成功
↓
AuthenticatedUserInfo
↓
ApplicationContext
↓
MainWindow
```

如果 Student.lastLoginAt 等登录成功后的权威写回失败：

```text
不得先进入 MainWindow 再假装成功
```

---

## 15.3 统一 MainWindow

采用：

```text
一个 MainWindow
+
capabilities 驱动导航
```

不维护两套重复：

```text
StudentMainWindow
AdministratorMainWindow
```

MainWindow 负责：

```text
应用壳
当前用户显示
导航
QStackedWidget / 页面容器
logout
页面创建 / 激活
```

不负责：

```text
审核
积分
点赞
排行榜
Repository
```

---

# 16. Qt 页面体系

当前页面体系应直接服从 Gate 3.5 当前冻结结果。

## 16.1 Student

主 Page：

```text
StudentDashboardPage
MyVolunteerPage
AchievementPage
ProfilePage
```

公共：

```text
DiaryWallPage
RankingPage
```

注意：

```text
学生必须有公共 RankingPage 入口
```

这是 Gate 3.8 当前登记图示修正项之一。

---

## 16.2 Administrator

主 Page：

```text
AdminDashboardPage
ReviewPage
StudentManagementPage
ConfigurationPage
DiaryModerationPage
StatisticsPage
OperationLogPage
```

公共：

```text
DiaryWallPage
RankingPage
```

边界：

```text
ReviewPage
→ 只负责 VolunteerRecord 审核 / 治理

DiaryModerationPage
→ 只负责 DiaryPost 审核 / 治理
```

不得把日记审核重新塞回 ReviewPage。

---

## 16.3 Dialog

当前主要 Dialog 可包括：

```text
VolunteerRecordDialog
VolunteerRecordDetailDialog
DiaryPostRequestDialog
ReviewVolunteerDialog
CorrectApprovedRecordDialog
StudentCreate/Edit Dialog
PasswordResetDialog
CategoryEditDialog
BadgeRuleEditDialog
SemesterEditDialog
ExportDialog
```

原则：

```text
Page
→ 持续主功能区域

Dialog
→ 一次性创建 / 编辑 / 审核 / 确认
```

Dialog 只有在完整业务成功后：

```text
Accepted
```

业务失败：

```text
保持打开
显示错误
允许用户继续修正
```

---

# 17. D267 — Qt 接入不重写核心业务

> **V0.3 接入 Qt 时，V0.2 已通过验证的 Domain、Service、Repository/Persistence 与业务规则原则上不因 GUI 接入而重写。若 Qt 接入迫使核心业务层大规模修改，必须将其视为架构适配问题并记录原因，而不能把业务逻辑复制到 Qt 页面中规避。**

判断标准：

```text
如果 Qt 接入主要新增 Presentation
→ 正常

如果 Qt 接入迫使大量 Service / Domain 重做
→ 必须分析为什么

如果为了赶工把业务复制进 Page
→ 架构违规
```

---

# 18. D268 — Qt Presentation 边界

> **V0.3 的 Qt 层只负责输入、展示、导航、页面状态、Dialog、Model/View、Signal/Slot、用户体验级预校验以及 Service Result 的 UI 映射；Qt 页面不得直接访问 Repository、不得长期持有领域实体地址、不得重新实现核心业务规则。跨页面与表格中的业务对象定位统一优先使用 stable ID。**

例如：

```text
ReviewPage
↓
selected recordId
↓
ReviewVolunteerDialog(recordId)
↓
Service 按 ID 查询最新权威事实
```

禁止：

```text
VolunteerRecord* 长期跨页面保存
```

---

# 19. Service View Result

Qt 不自行组合跨多个领域的数据。

例如 Ranking：

```text
RankingPage
↓
RankingService
↓
RankingResult
↓
展示
```

DiaryWall：

```text
DiaryWallPage
↓
DiaryService
↓
DiaryPostPublicView / equivalent
↓
展示
```

View Result：

```text
是一次查询成功结果
不是新的持久 Domain
不建立 DTO God hierarchy
```

---

# 20. Qt Model/View

对筛选 / 排序收益明显的页面采用：

```text
QAbstractTableModel subclass
↓
QSortFilterProxyModel
↓
QTableView
```

当前典型：

```text
StudentManagementPage
OperationLogPage
```

Model 负责：

```text
View Result 数据
row / column / data / header
stable ID role
```

Proxy 负责：

```text
Presentation 排序
Presentation 轻量筛选
```

业务 Ranking 排序仍由 RankingService 决定。

Qt Proxy 不取代业务排序规则。

---

# 21. DiaryWall Qt 化

采用已冻结方向：

```text
QScrollArea
↓
FeedContainer QWidget
↓
QVBoxLayout
├── DiaryPostCard
├── DiaryPostCard
└── ...
```

Card：

```text
只消费 DiaryPostPublicView / equivalent
长期定位使用 postId
```

互动：

```text
DiaryPostCard
↓
emit likeRequested(postId)
↓
DiaryWallPage
↓
DiaryService
```

Qt Signal/Slot 不进入 Domain。

---

# 22. D269 — 页面刷新原则

> **V0.3 不建立复杂 Page→Page 业务刷新网络。业务操作成功后当前页面按需要重新查询，其他页面在下次激活时通过 Service 重新获取当前权威数据和派生结果。**

推荐：

```text
navigateTo(PageId)
↓
lazy create if needed
↓
onActivated() / refresh()
↓
Service query
↓
render
```

不以：

```text
focusInEvent()
```

作为业务刷新核心。

不建立：

```text
全局 EventBus
跨页面 Observer 网络
缓存版本号
所有 Page 互相 emit 刷新
```

---

# 23. UI 预校验与业务校验

Qt 可以做：

```text
空值
格式
长度
输入范围
控件限制
```

这是 UX。

Service / Domain 仍必须正式验证：

```text
Permission
AccountStatus
ownership
RecordStatus
唯一性
BadgeRule 约束
业务不变量
```

原则：

```text
Presentation validation
≠
Business validation
```

---

# 24. Result<T> → Qt

V0.3 将 V0.1 / V0.2 已有 Service Result 映射成 UI。

例如：

```text
ValidationFailed
→ 表单提示

BusinessRuleViolation
→ 业务警告

PermissionDenied
→ 权限提示

NotFound
→ 数据可能变化，重新查询

PersistenceFailure
→ 保存失败

SeverePartialCommit
→ 严重错误
→ 禁止继续普通写操作
→ 引导安全退出 / 重启
```

ServiceError 保持纯 C++。

禁止 Service 返回：

```text
QMessageBox 类型
QColor
QIcon
Widget
```

---

# 25. Export Qt 接入

核心 Export 逻辑在 V0.1 已完成。

V0.3 只新增：

```text
ExportDialog
```

让管理员选择：

```text
导出内容
查询范围
输出格式
目标路径
```

然后调用既有 ExportService。

正式格式继续：

```text
CSV
Markdown
```

---

# 26. D270 — Console 保留原则

> **V0.3 可以保留 V0.2 的 Console Presentation 作为内部测试、诊断和业务层回归验证入口，但 Qt Presentation 成为正式用户界面。Console 与 Qt 必须调用同一套 Service，不形成两套业务实现。**

意义：

```text
Qt 出现问题
↓
使用 Console 回归同一 Service
↓
判断：
Presentation bug
or
Business bug
```

正式 V1.0 默认入口为 Qt。

---

# 27. D271 — V0.3 完成门槛

> **只有当全部正式业务模块均已建立对应 Qt 入口，主要 Student / Administrator / Common 页面及必要 Dialog 能够通过既有 Service 完成业务操作，Qt 不直接越过 Application / Service 层访问 Repository，且主要 GUI 业务链能够端到端运行时，V0.3 才视为完成。V0.3 可以保留尚待修复的界面细节、交互缺陷和 Qt 集成 Bug，这些进入 V1.0 稳定化阶段。**

V0.3 至少应通过 GUI 覆盖：

```text
登录
Student 导航
Administrator 导航
志愿创建 / 修改 / 撤回 / 重提
审核 / 治理
统计
排行榜
Badge
Diary 申请
Diary 审核 / 下架
Like / Unlike
Student 管理
规则配置
Semester
OperationLog
Global Statistics
Export
```

---

> **V0.3 → V1.0 审计停点 → Gate 4**
>
> V0.3 Qt 全页面接入完成后，进入 V1.0 前应按 `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md` 触发 `STOP-13`，完成 GUI、回归与严重错误路径审计后再进入最终稳定化。

# 28. V1.0 —— Qt 全系统稳定化与最终成果

## 28.1 D272 — V1.0 总体定位

> **V1.0 定位为最终交付版本。它建立在 V0.3 已完成 Qt Presentation 全面接入的基础上，不再承担新的核心架构设计，而是完成 Qt 全业务链路调试、界面与交互完善、异常处理补全、持久化与回归验证、最终数据准备、报告与程序一致性收口，并形成正式课程设计成果。**

关系：

```text
V0.3
“GUI 已完整接入”

↓

V1.0
“GUI + 业务 + 持久化 + 测试 + 文档
全部达到最终交付状态”
```

---

# 29. V1.0 的核心工作

## 29.1 Qt 全业务链回归

V0.2 已通过 Console 验证的全部主链，再通过 Qt 跑一遍。

重点变成：

```text
Qt 传参是否正确
stable ID 是否正确
Dialog 生命周期是否正确
页面是否刷新
错误是否正确映射
capabilities 是否正确显示入口
Service 是否仍守住最终授权
```

---

## 29.2 页面状态与导航激活刷新

逐页检查：

```text
首次进入
再次进入
业务修改后重新进入
logout
重新登录
切换账号
多次页面切换
```

防止：

```text
旧查询结果残留
旧账号数据残留
旧 capabilities 残留
```

logout：

```text
停止认证后交互
↓
销毁 MainWindow / Pages / Dialog
↓
销毁 ApplicationContext
↓
回到 LoginWindow
```

---

## 29.3 Dialog 成败边界

修改型 Dialog：

```text
用户确认
↓
Service
↓
Persistence
↓
Success?
```

成功：

```text
accept()
↓
刷新
```

失败：

```text
不 accept
↓
显示错误
↓
允许修正
```

禁止：

```text
先关闭 Dialog
↓
再发现持久化失败
```

---

## 29.4 ServiceError UI 映射完整化

V1.0 应确保错误分类不会全部显示成：

```text
操作失败
```

Presentation 根据：

```text
Validation
BusinessRule
Permission
NotFound
PersistenceFailure
SeverePartialCommit
```

选择合理反馈。

严重持久化错误：

```text
不能继续普通写业务
```

---

## 29.5 权限双层验证

验证：

```text
ApplicationContext.capabilities
→ 控制入口

Service authorization
→ 控制真实业务合法性
```

即使 Presentation 错误显示按钮：

```text
Service 仍必须拒绝非法调用
```

---

## 29.6 Qt Model/View 高风险点

尤其检查 Proxy：

```text
Proxy index
↓
mapToSource
↓
读取 stable ID
↓
Service
```

防止排序 / 筛选后：

```text
点击第 N 行
→ 错操作另一个 Student / Log
```

检查：

```text
beginResetModel / endResetModel
selection
stable ID role
refresh 后 index
```

等实现细节。

---

## 29.7 DiaryWall 最终体验

检查：

```text
Card layout
滚动
空状态
Like 状态
LikeCount
Like / Unlike 后局部显示
TakenDown 不公开
再次进入页面数据正确
```

Card 不直接持有 Domain 对象。

---

## 29.8 Persistence 最终回归

Qt 完整业务操作后：

```text
退出 GUI
↓
完全结束进程
↓
重新启动
↓
重新登录
↓
查询
```

权威数据正确恢复。

派生结果重新计算正确。

---

## 29.9 严重持久化错误

启动加载失败：

```text
不得进入正常 MainWindow
```

运行期 SeverePartialCommit：

```text
明确严重故障
↓
阻止继续普通写业务
↓
引导安全退出 / 重启 / Recovery
```

---

## 29.10 最终个性化测试数据

V1.0 必须准备具有展示价值的个人化样例数据。

不使用最终报告中的：

```text
张三
李四
测试用户1
```

作为唯一正式展示数据。

最终数据应覆盖：

```text
多个 Student
至少一个 Administrator
多个 VolunteerCategory
多个 Semester
Pending
Approved
Rejected
Withdrawn
不同积分
排行榜前三
tie-break
Badge Bronze / Silver / Gold
PendingDisplayReview
Displayed
TakenDown
多个 LikeRelation
OperationLog
```

这样同一套数据可服务：

```text
程序演示
截图
课程报告
测试
答辩
```

---

## 29.11 最终 UI 收口

在业务正确后处理：

```text
窗口尺寸
页面布局
控件间距
表头
空状态
按钮语义
状态展示
日期格式
文本换行
确认框
错误提示
导航顺序
视觉一致性
```

V0.3 先保证完整接入。

V1.0 再做最终表现层 polishing。

---

## 29.12 架构回归审计

最终扫描禁止项。

至少检查：

```text
Qt 是否直接访问 Repository？
Qt 是否直接修改 Domain？
Domain 是否出现 Qt 类型？
是否明文持久化密码？
是否出现机械 public setter？
是否出现全局 currentUser？
是否出现 Singleton / Service Locator？
Student 是否持久化 totalScore？
是否持久化 currentRank？
是否持久化 currentBadge？
VolunteerRecord 是否保存 semesterId？
Semester 是否保存 isCurrent？
DiaryPost 是否保存 publisherAccountId？
学生是否出现 request takedown？
Export 是否被写成 Excel？
ReviewPage 是否混入 Diary 审核？
```

发现偏离：

```text
修复
+
记录
```

不能无痕让报告继续描述旧架构。

---

# 30. D273 — V1.0 稳定化范围

> **V1.0 重点完成 Qt 全业务链调试、页面刷新一致性、Dialog 成败边界、ServiceError 到 UI 的正确映射、权限双层验证、Model/View 与 stable ID 使用、日记墙交互、持久化回归、严重错误处理、最终 UI 完善以及架构一致性审计，不通过新增无必要业务功能制造最终版本增量。**

---

# 31. D274 — V1.0 最终数据与测试

> **V1.0 必须使用项目最终个性化样例数据完成正常、边界、异常、持久化和端到端测试，并形成可直接用于课程报告和答辩展示的稳定运行结果。测试数据应覆盖主要业务状态、排行榜、徽章、日记墙、点赞与操作日志等展示场景。**

---

# 32. D275 — V1.0 代码—文档一致性

> **V1.0 冻结前必须进行代码、Gate 设计文档、最终架构图、流程图、测试记录、版本日志和课程设计报告之间的一致性审计。报告不得声明代码中实际不存在的架构、设计模式、算法或版本过程，也不得保留已被当前权威规则覆盖的历史需求。**

尤其检查：

```text
“学生申请下架日记”
→ 已撤销
→ 不得重新出现在最终报告 / GUI / 权限 / Service 中
```

以及：

```text
Export
→ CSV / Markdown

不是：
CSV / Excel
```

---

# 33. D276 — V1.0 完成门槛

> **V1.0 只有在全部正式业务能够通过 Qt Presentation 完成端到端运行，阻塞性与关键业务 / 持久化缺陷清零，主要边界及异常场景通过回归测试，程序重启后数据恢复正确，最终 UI、个性化样例数据和交付材料准备完成，并通过架构—实现—报告一致性审计后，才可作为最终成果冻结。**

概念：

```text
V1.0 DONE
=
Business complete
+
Qt complete
+
Blocking/Critical defects cleared
+
Persistence reliable
+
Regression passed
+
Architecture consistent
+
Documentation consistent
+
Ready for demonstration
+
Ready for report
+
Ready for defense
```

---

# 34. 四版本架构映射总表

| 架构 / 能力 | V0.1 | V0.2 | V0.3 | V1.0 |
|---|---|---|---|---|
| User / Student / Administrator | 完整实现 | 回归 / 修 Bug | 保持 | 最终回归 |
| User 多态 / capabilities | 完整实现 | 验证 | Qt 用于导航入口 | 双层权限终验 |
| Domain 其他实体 | 完整实现 | 完整测试 | 原则不改 | 最终一致性审计 |
| VolunteerRecord 状态机 | 完整实现 | 重点边界测试 | GUI 接入 | GUI 回归 |
| stable ID 关系 | 完整实现 | 引用完整性验证 | Qt 跨页面使用 | 最终审计 |
| Application / Service | 完整实现 | 系统联调 | Qt 复用 | 最终回归 |
| AuthenticationService | 完整 | 登录边界验证 | LoginWindow 接入 | 登录 / logout 回归 |
| UserManagementService | 完整 | 权限 / 状态验证 | GUI 接入 | 最终回归 |
| StudentVolunteerService | 完整 | 主链重点测试 | Student GUI 接入 | 最终回归 |
| VolunteerReviewService | 完整 | 审核 / 删除重点测试 | Admin GUI 接入 | 最终回归 |
| StatisticsService | 完整 | 派生结果验证 | GUI 展示 | 最终回归 |
| RankingService | 完整 | tie-break 测试 | RankingPage | 最终回归 |
| BadgeService | 完整 | 门槛 / 动态下降测试 | Achievement 等展示 | 最终回归 |
| DiaryService | 完整 | 生命周期 / Like 测试 | DiaryWall / Moderation | 最终回归 |
| RuleConfigurationService | 完整 | 规则修改测试 | ConfigurationPage | 最终回归 |
| SemesterService | 完整 | currentSemesterId 测试 | ConfigurationPage | 最终回归 |
| OperationLogService | 完整 | 审计完整性测试 | OperationLogPage | 最终回归 |
| ExportService | 完整 | CSV / MD 验证 | ExportDialog | 最终回归 |
| Repository | 完整 | 重点测试 | 保持 | 最终回归 |
| Persistence | 完整 | 重点测试 | 保持 | 最终回归 |
| CsvCodec | 完整 | 特殊字段测试 | 保持 | 最终回归 |
| PersistenceCoordinator | 完整 | 多文件失败测试 | 保持 | 最终回归 |
| Result<T> | 完整 | Error 路径验证 | 映射 Qt | 最终反馈收口 |
| OperationResult | 完整 | 验证 | 映射 Qt | 收口 |
| ServiceError | 完整语义方向 | 验证 | Qt 映射 | 最终错误体验 |
| Repository Pattern | 使用 | 验证 | 保持 | 审计 |
| Service Layer | 使用 | 验证 | 保持 | 审计 |
| Export Strategy | 使用 | 验证 | GUI 入口 | 审计 |
| Composition Root | Console 装配 | 验证 | 改装 Qt 生命周期 | 最终装配 |
| Console Presentation | 正式 UI | 正式稳定版 | 可保留为内部诊断 | 非最终正式入口 |
| Qt Presentation | 无 | 无 | 完整接入 | 最终成果 |
| ApplicationContext | 无正式 Qt 会话需求 | 无正式 Qt 会话需求 | 正式加入 | 最终验证 |
| LoginWindow / MainWindow | 无 | 无 | 正式加入 | 最终稳定 |
| Qt Page / Dialog | 无 | 无 | 完整接入 | 最终稳定 |
| Qt Model/View | 无 | 无 | 正式加入需要页面 | 最终验证 |
| Signal / Slot | 无 | 无 | 正式加入 | 最终验证 |
| DiaryPostCard | 无 | 无 | 正式加入 | 最终体验 |
| Console E2E | 首实现验证 | 主要完成门槛 | 回归辅助 | 可作诊断 |
| Qt E2E | 无 | 无 | 主要业务可运行 | 最终全链通过 |
| 个性化最终数据 | 可准备基础 | 可用于测试 | 可继续补 | 正式冻结 |
| 课程报告最终一致性 | 非最终 | 积累 Bug / 测试材料 | 积累 Qt 过程 | 最终审计 |
| 最终交付 | 否 | 否 | 否 | 是 |

---

# 35. 四版本业务状态总表

| 业务模块 | V0.1 | V0.2 | V0.3 | V1.0 |
|---|---|---|---|---|
| 登录 | Console 完整 | 稳定 | Qt 登录 | 最终 |
| Student 资料 | Console 完整 | 稳定 | Qt 页面 | 最终 |
| Student 管理 | Console 完整 | 稳定 | Qt 页面 | 最终 |
| VolunteerRecord | 全流程 | 全链测试 | Qt 完整接入 | 最终 |
| 审核 / 治理 | 完整 | 重点测试 | Qt 完整接入 | 最终 |
| Category | 完整 | 规则测试 | Qt 配置 | 最终 |
| Semester | 完整 | 切换测试 | Qt 配置 | 最终 |
| Statistics | 完整 | 联动验证 | Qt 展示 | 最终 |
| Ranking | 完整 | tie-break 验证 | Qt 展示 | 最终 |
| Badge | 完整 | 边界验证 | Qt 展示 | 最终 |
| DiaryPost | 完整 | 生命周期验证 | Qt Feed / Moderation | 最终 |
| Like | 完整 | 唯一性验证 | Qt 互动 | 最终 |
| OperationLog | 完整 | 审计验证 | Qt Model/View | 最终 |
| Export | CSV / Markdown | 编解码测试 | Qt Dialog | 最终 |

---

# 36. Presentation 替换关系

最重要的版本结构变化：

```text
V0.1 / V0.2

Console Presentation
         │
         ▼
Application / Service
         │
    ┌────┴────┐
    ▼         ▼
 Domain   Repository
              │
              ▼
         Persistence
```

进入 V0.3：

```text
                 ┌────────────────────┐
                 │ Qt Presentation    │
                 │ 正式用户界面       │
                 └─────────┬──────────┘
                           │
                           ▼
                  Application / Service
                           │
                    ┌──────┴──────┐
                    ▼             ▼
                 Domain       Repository
                                  │
                                  ▼
                             Persistence

Console Presentation
→ 可保留为内部测试 / 诊断入口
→ 调用同一 Service
```

因此：

> **版本演进不是两套业务系统，而是同一业务系统从 Console Presentation 验证到 Qt Presentation 最终交付。**

---

# 37. 不允许出现的“伪版本演进”

## 37.1 故意留架构错误

禁止：

```text
V0.1 故意不用 Service
V0.2 再加 Service
```

禁止：

```text
V0.1 故意不用 Repository
V0.2 再加 Repository
```

禁止：

```text
V0.1 用 role + if/else
V0.2 再改 User 多态
```

当前课程时间约束下，不采用这条路线。

---

## 37.2 故意留正式功能

禁止为了版本日志：

```text
V0.1 不做排行榜
V0.2 新增排行榜
```

如果排行榜已属于正式需求，则应进入 V0.1。

只有真实遗漏 / 缺陷才允许在 V0.2 补齐，并必须如实记录。

---

## 37.3 Qt 复制业务

禁止：

```text
ConsoleBusiness.cpp
QtBusiness.cpp
```

形成两套规则。

所有 Presentation 使用：

```text
同一套 Service
```

---

# 38. 真实版本日志应如何形成

## V0.1

记录：

```text
首次实现哪些完整模块
采用哪些最终架构
为何 Console 先行
已知哪些问题
哪些业务尚未完成系统联调
```

设计决策可写：

> 为了在进入 Qt 前先验证业务正确性，V0.1 采用最终业务架构 + Console Presentation。这样可以在不受 GUI 调试干扰的情况下先完成业务逻辑与持久化，同时保证后续 Qt 只替换表现层。

---

## V0.2

记录真实 Bug，例如：

```text
Bug ID
复现步骤
根因
影响
修复
回归测试
```

重点材料来源：

```text
状态机
审核历史
级联删除
CSV
Persistence
Ranking tie
Semester
Permission
```

设计决策可写：

> V0.2 不继续增加主要业务模块，而把开发重点转为端到端验证和持久化一致性，以保证 Qt 接入建立在稳定业务基线上。

---

## V0.3

记录：

```text
Qt 页面接入
Console → Qt Presentation
ApplicationContext
LoginWindow
MainWindow
Page / Dialog
stable ID
Model/View
DiaryPostCard
Result → UI
```

真实问题重点：

```text
Proxy index
页面刷新
Dialog accept
旧数据残留
logout 生命周期
```

设计决策可写：

> V0.3 不重写经过 V0.2 验证的业务层，而通过既有 Service 接入 Qt，使 GUI 只承担表现职责，从而降低引入第二套业务规则的风险。

---

## V1.0

记录：

```text
Qt 集成 Bug
UI 收口
持久化回归
异常路径
最终个性化数据
最终测试
架构一致性
报告一致性
```

设计决策可写：

> V1.0 不继续堆叠新技术，而将主要精力投入最终稳定性、完整性、可解释性和交付一致性。

---

# 39. Gate 3.9 与课程版本要求的对应

课程要求至少保留三个递进开发版本。

当前保留四个：

```text
V0.1
V0.2
V0.3
V1.0
```

每版均有不同的真实开发目标：

```text
V0.1
→ 完整实现

V0.2
→ 稳定与验证

V0.3
→ GUI 集成

V1.0
→ 最终稳定与交付
```

版本演进不仅表现为“新增按钮”，也表现为：

```text
功能实现成熟度
系统可靠性
表现层技术
测试覆盖
交付完整性
```

这属于真实的软件版本演进。

---

# 40. 当前版本路线对旧版本规范的覆盖关系

此前版本规范采用：

```text
V0.1
核心业务原型

V0.2
核心系统重构与完善

V0.9
Qt GUI 集成

V1.0
最终完整系统
```

Gate 3.9 当前重新冻结为：

```text
V0.1
最终业务架构完整 Console 首实现

V0.2
Console Debug / 全链稳定

V0.3
Qt 完整接入

V1.0
Qt Debug / 最终成果
```

覆盖点有两个：

### 覆盖 1：V0.1 / V0.2 的架构演进逻辑

旧：

```text
V0.1 可较粗糙
↓
V0.2 真实重构
```

新：

```text
V0.1 即按最终核心架构实现
↓
V0.2 主要 Debug / 联调 / 稳定
```

### 覆盖 2：Qt 里程碑编号

旧：

```text
V0.9
```

新：

```text
V0.3
```

因此后续任何 AI 检索版本路线时：

> **应以 Gate 3.9 本文件为当前程序版本路线依据。**

旧版本规则只保留历史背景价值，不得覆盖本文件。

---

# 41. Gate 3.8 与 Gate 3.9 的关系

Gate 3.8 当前仍：

```text
REOPENED
```

已登记图示问题：

```text
G38-01
系统总架构图缺 StudentVolunteerService

G38-02
Service–Repository 图缺 StudentVolunteerService

G38-03
Qt 导航图缺 DiaryModerationPage

G38-04
Qt 导航图缺 Student 公共 RankingPage

G38-05
ReviewPage 混入 Diary 审核

G38-06
ExportDialog 错写 CSV / Excel
应为 CSV / Markdown
```

Gate 3.9 不改变这些问题。

版本实现时必须按文字冻结架构：

```text
StudentVolunteerService
→ 存在

DiaryModerationPage
→ 存在

Student RankingPage
→ 存在

ReviewPage
→ 不负责 Diary 审核

Export
→ CSV / Markdown
```

不能因为当前原图仍未修正就按错误图实现。

---

# 42. Gate 3.9 当前决策索引

| 编号 | 当前冻结决策 |
|---|---|
| D259 | 自 V0.1 起即按 Gate 3.1～3.7 最终核心架构实现；版本差异主要来自实现成熟度、验证和 Presentation |
| D260 | V0.1 是最终业务架构的完整 Console 首实现版，Console 不承载核心业务逻辑 |
| D261 | V0.1 必须实现全部当前正式业务模块，Qt 专属 Presentation 技术除外 |
| D262 | V0.2 不预设新增主要业务；负责 V0.1 的系统联调、Bug 修复与稳定 |
| D263 | V0.2 定位为控制台完整业务稳定版 |
| D264 | V0.2 采用 Domain / Service / Repository-Persistence / Console E2E 四层验证 |
| D265 | V0.2 冻结要求 Blocking、Critical Business、Critical Persistence 缺陷清零并通过持久化恢复 |
| D266 | V0.3 定位为 Qt Presentation 完整接入版 |
| D267 | Qt 接入原则上不重写 V0.2 已验证核心业务；被迫大改必须登记架构适配原因 |
| D268 | Qt 只承担 Presentation；不直接访问 Repository、不长期持有 Domain 地址、使用 stable ID |
| D269 | 页面采用激活时重新通过 Service 查询，不建立复杂 Page→Page 刷新网络 |
| D270 | Console 可作为内部测试 / 诊断入口保留，但与 Qt 共用同一 Service |
| D271 | V0.3 要求全部正式业务建立 Qt 入口并完成主要 GUI 端到端链路 |
| D272 | V1.0 定位为 Qt 全系统稳定化与最终交付版本 |
| D273 | V1.0 聚焦 Qt 全链调试、刷新、错误映射、Model/View、持久化、UI 和架构一致性 |
| D274 | V1.0 使用最终个性化样例数据完成正常、边界、异常、持久化与 E2E 测试 |
| D275 | V1.0 冻结前进行代码—Gate—图示—测试—版本日志—报告一致性审计 |
| D276 | V1.0 完成需业务、Qt、关键缺陷、Persistence、回归、文档和交付全部满足最终门槛 |

---

# 43. Gate 3.9 FINAL AUDIT

| 审计项 | 结论 |
|---|---|
| 版本数量满足至少 3 个递进版本 | PASS |
| V0.1 不故意写差 | PASS |
| V0.1 与最终业务架构一致 | PASS |
| V0.1 具有完整业务范围 | PASS |
| Console 不侵入业务层 | PASS |
| V0.2 有真实版本价值 | PASS |
| V0.2 可产生真实 Bug 与测试材料 | PASS |
| V0.2 不依赖人为留功能制造版本故事 | PASS |
| Qt 接入前已有稳定业务基线 | PASS |
| V0.3 只替换 / 扩展 Presentation | PASS |
| Qt 不直接访问 Repository | PASS |
| Qt 不长期持有 Domain 指针 | PASS |
| stable ID 跨页面原则保留 | PASS |
| ApplicationContext 边界保留 | PASS |
| Page / Dialog 边界保留 | PASS |
| 页面激活刷新策略保留 | PASS |
| Model/View 边界保留 | PASS |
| DiaryWall Card 架构保留 | PASS |
| Result<T> 可从 Console 复用到 Qt | PASS |
| Console 可作为回归入口继续使用 | PASS |
| V1.0 不以无必要新增功能制造版本增量 | PASS |
| V1.0 有明确最终测试门槛 | PASS |
| V1.0 有架构一致性门槛 | PASS |
| V1.0 有报告一致性门槛 | PASS |
| 当前学生自助下架撤销规则保持一致 | PASS |
| Export CSV / Markdown 保持一致 | PASS |
| Semester / VolunteerRecord 当前规则保持一致 | PASS |
| DiaryPost publisher 推导规则保持一致 | PASS |
| Gate 3.8 REOPENED 状态未被错误覆盖 | PASS |
| 当前版本路线与旧 V0.9 路线冲突已显式声明覆盖 | PASS |
| 其他 AI 可通过本文独立理解版本路线 | PASS |

结论：

```text
Gate 3.9
V0.1 → V0.2 → V0.3 → V1.0
Implementation / Verification / GUI Integration / Delivery Mapping

D259 ～ D276

→ FINAL PASS CANDIDATE
```

说明：

> 本文件纳入当前项目源并同步 Gate3_00 / 权威检索索引后，Gate 3.9 可标记为 `FINAL PASS`。  
> 但 Gate 3 overall 暂不能因此标记 `FINAL PASS`，因为 Gate 3.8 当前仍为 `REOPENED`，需要完成 G38-01～G38-06 图示修正与图—文复核。

---

# 44. 后续实施顺序

Gate 3.9 完成以后，程序真实开发建议严格按照：

```text
Step 1
建立 V0.1
→ 完整 Console 业务系统

Step 2
跑 V0.2
→ 四层测试
→ 修复真实 Bug
→ 冻结稳定 Console 基线

Step 3
建立 V0.3
→ 完整 Qt 接入
→ 保留 Console 回归入口

Step 4
形成 V1.0
→ Qt Debug
→ Regression
→ UI 收口
→ 最终个性化数据
→ 架构一致性
→ 报告一致性
→ 交付
```

Gate 4 实现审计不得重新改变 Gate 3.9 的版本职责，除非真实实现中发现新的架构级阻塞；如果发生，必须通过显式修订记录说明原因，不无痕覆盖。

---

# 45. 给后续 AI / Codex / 开发对话的最短读取指令

如果上下文空间不足，可优先读取本节。

```text
当前程序版本路线已经冻结为：

V0.1
= 最终核心业务架构 + 全部正式业务 + Console Presentation。
Domain、Service、Repository/Persistence、Result<T>、Export Strategy 等均从 V0.1 开始使用。
V0.1 不是故意粗糙原型。

V0.2
= V0.1 的 Console 系统级联调、四层测试、持久化验证与真实 Bug 修复。
原则上不新增主要业务模块。
Blocking / Critical Business / Critical Persistence Bug 清零后冻结。

V0.3
= 在 V0.2 稳定业务基线上完整接入 Qt。
Qt → Service，不直接访问 Repository。
Qt 使用 stable ID，不长期持有 Domain 对象地址。
ApplicationContext / LoginWindow / MainWindow / Page / Dialog / Model-View / DiaryPostCard 均在此版本正式落地。
Console 可保留为内部回归入口，但与 Qt 共用同一套 Service。

V1.0
= V0.3 的 Qt 全系统 Debug、回归、持久化验证、UI 收口、最终样例数据、架构/报告一致性与最终交付。
V1.0 不通过新增无必要功能制造版本增量。

硬性业务覆盖：
- Student 无“申请下架自己日记”功能；
- TakenDown 只用于 Administrator 治理；
- VolunteerRecord 不保存 semesterId；
- Semester 不保存 isCurrent，使用 currentSemesterId；
- DiaryPost 不保存 publisherAccountId，通过 recordId → ownerAccountId 推导；
- 只读 Export 不写 OperationLog；
- Export 正式格式是 CSV / Markdown，不是 Excel。

Gate 3.8 当前仍 REOPENED，原图中的 G38-01～G38-06 不得作为实现依据。
Gate 3.9 当前决策范围 D259～D276。
```

---

# 46. 当前状态建议

当本文件正式纳入项目源并同步索引后，建议项目状态更新为：

```text
Gate 3.1 → PASS
Gate 3.2 → PASS
Gate 3.3 → FINAL PASS
Gate 3.4 → FINAL PASS
Gate 3.5 → FINAL PASS
Gate 3.6 → FINAL PASS
Gate 3.7 → FINAL PASS
Gate 3.8 → REOPENED
Gate 3.9 → FINAL PASS

Frozen decisions:
D1 ～ D276

Gate 3 overall:
NOT FINAL PASS
```

Gate 3 overall 下一阻塞项：

```text
Gate 3.8
G38-01 ～ G38-06
图示修正
↓
图—文复核
↓
Gate 3.8 FINAL PASS
↓
Gate 3 FINAL AUDIT
```
