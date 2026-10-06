# Gate 3.2 — 领域关系与生命周期架构

> 状态：**PASS**。
>
> 下方正文逐行迁移自 v2.4 第 1891～3078 行；其中包含关系映射、状态机、生命周期终审、表格和文本图。D22～D62 均保留在原上下文中。

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

# 22. Gate 3.2：组合 / 聚合 / 关联的 C++ 技术映射审计

Gate 3.2 的目标是把 Gate 2 中已经冻结的业务关系进一步映射为最终 V1.0 中清晰、稳定、可持久化、可解释的 C++ 关联方式。

当前统一原则：

1. 业务关系不等于必须在类内部直接保存对象容器。
2. 生命周期拥有关系与普通业务归属必须区分。
3. 独立实体之间的长期持久关联优先采用稳定 ID，而不是长期裸指针。
4. 不为了满足“组合 / 聚合”评分要求强行制造错误拥有关系。
5. 生命周期级联不等于必须使用嵌套 `vector`。
6. 可修改显示名称不承担主键职责。
7. 本节原始讨论发生在 Repository / Persistence 冻结之前；v1.9 Current Truth 下，其长期数据访问实现以 Gate 3.4 已冻结的 Repository / Persistence 架构为准。

## 22.1 Student 与 VolunteerRecord

### 决策 D22

> `Student` 与 `VolunteerRecord` 明确不是强组合关系。最终 V1.0 将其建模为具有弱整体—部分语义的 1:N 聚合 / 业务关联。Student 是 VolunteerRecord 的业务归属主体，但不负责 VolunteerRecord 对象的完整生命周期。VolunteerRecord 作为独立核心业务实体由系统统一管理，不直接内嵌于 Student 的 `vector<VolunteerRecord>` 中。

说明：

- Student 被禁用后既有 VolunteerRecord 仍然保留；
- 有业务数据的 Student 不能因账号处理而连带删除历史记录；
- Administrator 可以独立查询和治理 VolunteerRecord；
- 统计、排行榜、徽章、日记墙等多个模块都需要访问 VolunteerRecord。

因此这里的“聚合”表达的是业务归属，不等于 C++ 内存所有权。

### 决策 D23

> `VolunteerRecord` 对所属学生的长期持久关联采用稳定 `ownerAccountId`，而不是长期保存 `Student*` 裸指针。需要获取 Student 对象时，由后续数据访问 / 服务层根据 `accountId` 解析。当前不引入 `StudentRef`、缓存指针等额外抽象。

理由：

- ID 可直接持久化；
- 程序重启后仍稳定；
- 不依赖对象内存地址；
- 避免悬空指针；
- 更利于按学生查询记录。

> **实现细化 → Gate 4**
>
> 本节冻结的是跨对象长期关系使用稳定 ID、而非长期裸指针的架构语义；对应 Stable ID 的实现级规则已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.4_StableID日期时间与数值类型实现细化_候选冻结稿(1).md`

## 22.2 VolunteerRecord 与 VolunteerCategory

### 决策 D24

> `VolunteerRecord` 与 `VolunteerCategory` 采用稳定 ID 关联，不长期保存裸指针。VolunteerRecord 分别保存 `appliedCategoryId` 与 `finalCategoryId`。

概念结构：

```text
VolunteerRecord
├── appliedCategoryId
├── finalCategoryId
├── settledCoefficient
└── finalScore
```

### 决策 D25

> 历史结算信息独立冻结在 VolunteerRecord 中，至少保留 `settledCoefficient` 与 `finalScore`。`VolunteerCategory.currentCoefficient` 只影响未来新结算记录，不追溯修改历史已结算数据。

因此：

```text
categoryId
→ 表示记录关联的是哪个类别

settledCoefficient + finalScore
→ 表示该记录实际按照什么规则完成历史结算
```

### 决策 D26

> `VolunteerCategory` 必须拥有独立稳定的 `categoryId`，不使用 category name 作为主键或长期关联键。

概念结构：

```text
VolunteerCategory
├── categoryId
├── name
├── currentCoefficient
└── enabled
```

名称可以调整，但实体身份保持不变。

## 22.3 BadgeRule 与 VolunteerCategory

### 决策 D27

> `BadgeRule` 与 `VolunteerCategory` 通过稳定 `categoryId` 建立关联，不长期保存 `VolunteerCategory*` 裸指针。

### 决策 D28

> `BadgeRule` 拥有独立稳定的 `badgeRuleId`；`badgeName` 只是可展示、可修改的业务名称，不作为主键。

概念结构：

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

## 22.4 DiaryPost 与 VolunteerRecord / Student

### 决策 D29

> `DiaryPost` 通过稳定 `recordId` 关联来源 `VolunteerRecord`，不长期保存 `VolunteerRecord*` 裸指针。

### 决策 D30

> 同一 `VolunteerRecord` 最多对应一个 `DiaryPost`。创建帖子时必须检查 `recordId` 唯一关联约束。

### 决策 D31

> `DiaryPost` 不额外持久化 `publisherAccountId`；发布 Student 通过 `recordId → VolunteerRecord.ownerAccountId` 推导，避免重复存储和一致性风险。

因此业务上仍然存在：

```text
Student 1 ---- N DiaryPost
```

但技术上不重复保存第二份发布者 ID。

### 决策 D32

> `DiaryPost` 拥有独立稳定 `postId`，不使用 `recordId` 充当帖子主键。

概念结构：

```text
DiaryPost
├── postId
├── recordId
├── title
├── content
├── displayStatus
└── publishedAt
```

## 22.5 DiaryPost 与 LikeRelation

当前采用：

> 方案 B：LikeRelation 独立统一管理。

### 决策 D33

> `LikeRelation` 作为独立关系实体统一管理，不内嵌在 `DiaryPost` 的 `vector<LikeRelation>` 中。

原因：

LikeRelation 同时需要支持：

```text
按 postId 查询点赞关系
按 studentAccountId 查询点赞关系
```

### 决策 D34

> `LikeRelation` 通过稳定 `studentAccountId + postId` 关联 Student 与 DiaryPost，不长期保存 `Student*` 或 `DiaryPost*` 裸指针。

概念结构：

```text
LikeRelation
├── studentAccountId
├── postId
└── likedAt
```

### 决策 D35

> `LikeRelation` 的逻辑唯一键为 `(studentAccountId, postId)`。

用于保证同一学生对同一帖子最多点赞一次。

### 决策 D36

> `DiaryPost` 与 `LikeRelation` 存在强生命周期依赖：普通下架不删除 LikeRelation，但 DiaryPost 被物理删除时，相关 LikeRelation 必须级联删除。该约束由后续业务服务 / 数据管理层协调，不要求 DiaryPost 直接拥有点赞关系容器。

### 决策 D37

> `LikeRelation` 当前不单独设置 `likeId`。

因为：

```text
(studentAccountId, postId)
```

已经天然构成稳定复合唯一身份。

## 22.6 OperationLog 的异构 target

OperationLog 的目标可能包括：

```text
VolunteerRecord
Student
VolunteerCategory
BadgeRule
Semester
DiaryPost
...
```

这些对象不存在真实共同业务父类。

### 决策 D38

> `OperationLog` 不保存异构业务对象裸指针，也不为了日志让所有可审计对象继承统一 `LoggableTarget` 基类。

原因：

> “都能被日志记录”不是这些对象的真实 `is-a` 关系。

### 决策 D39

> `OperationLog` 使用 `OperationTargetType + targetId` 表示操作目标。`OperationTargetType` 采用强类型枚举，`targetId` 保存对应实体的稳定标识。

概念结构：

```text
OperationLog
├── logId
├── operatorAccountId
├── operationType
├── targetType
├── targetId
├── description
└── operationTime
```

示例：

```text
targetType = VolunteerRecord
targetId   = VR20260023
```

### 决策 D40

> OperationLog 保存的是历史审计事实，不依赖 target 对象继续存在。即使业务对象已被物理删除，日志中的 `targetType`、`targetId`、`description` 仍必须保留。

例如：

```text
VolunteerRecord 被物理删除
↓
VolunteerRecord 对象不存在
↓
OperationLog 仍保留：
targetType
targetId
operatorAccountId
operationTime
description
```

> **实现细化 → Gate 4**
>
> 本节冻结的是 OperationLog 的稳定 targetId、历史软引用与对象删除后的保留语义；对应 Stable ID 永不复用及日志字段加载规则已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.4_StableID日期时间与数值类型实现细化_候选冻结稿(1).md`
> - `Gate4_4.5_CSVSchema_CsvCodec字段验证与schemaVersion实现细化_候选冻结稿(1).md`

## 22.7 OperationLog.operator 与 Administrator

业务上：

```text
Administrator 1 ---- N OperationLog
```

一条日志只对应一个实际执行管理员。

### 决策 D41

> `OperationLog` 与执行 Administrator 的长期关联使用稳定 `operatorAccountId`，不保存 `Administrator*` 裸指针。

理由：

- 文件中不能持久化内存地址；
- 程序重启后指针地址失效；
- Administrator 被重新加载或当前不驻留内存时，日志仍需可解释；
- 管理员账号后来 Disabled，不应影响历史操作事实。

概念结构：

```text
OperationLog
├── logId
├── operatorAccountId
├── operationType
├── targetType
├── targetId
├── description
└── operationTime
```

业务上的 Administrator 1:N OperationLog 关系仍然成立，只是技术映射使用 ID。

### 决策 D42

> OperationLog 当前不额外持久化 `operatorNameSnapshot`。

管理员姓名是展示信息，稳定责任主体由 `operatorAccountId` 确定。需要显示管理员姓名时，可根据 ID 解析当前 Administrator；如果无法解析，至少仍能展示稳定账号 ID。

当前需求没有“必须还原操作发生时管理员姓名”的历史快照要求，因此不额外制造重复真相。

### 决策 D43

> `operationType` 使用强类型 `enum class OperationType`。

与既有：

```text
Permission
OperationTargetType
RecordStatus
```

保持一致的强类型设计风格。

不使用：

```text
"approve"
"reject"
"disable"
```

等魔法字符串作为核心业务类型。

具体 OperationType 枚举项将在实现级接口审计时与最终日志范围保持一致，不在此处为了枚举数量提前冻结全部名字。

---

## 22.8 Semester 与 VolunteerRecord 的统计关系

Gate 2 已明确：

> Semester 只是学期统计时间边界，不拥有 VolunteerRecord。

### 决策 D44

> `VolunteerRecord` 不保存 `semesterId`。

志愿记录自身已经保存真实业务事实：

```text
serviceDate
```

某条记录是否进入某学期统计，由：

```text
semester.startDate
<= record.serviceDate
<= semester.endDate
```

动态判断。

这样避免出现：

```text
serviceDate 判断属于 Semester A
但 record.semesterId 却指向 Semester B
```

的双重真相冲突。

### 决策 D45

> Semester 是独立时间配置实体，只负责提供统计边界，不拥有、不包含、不直接标记 VolunteerRecord。

管理员切换 / 调整当前学期时：

- 不移动 VolunteerRecord；
- 不复制 VolunteerRecord；
- 不重写 VolunteerRecord 所属关系；
- 只改变学期统计、学期排行榜和学期称号的计算上下文。

### 决策 D46

> Semester 拥有独立稳定 `semesterId`；`semesterName` 只用于展示 / 业务命名，不承担主键职责。

概念结构：

```text
Semester
├── semesterId
├── semesterName
├── startDate
└── endDate
```

多个 Semester 配置可以长期共存，名称后续如果调整也不会改变实体身份。

---

> **实现细化 → Gate 4**
>
> 本节冻结的是 `serviceDate` 与 Semester 时间范围的架构语义；对应 Date、Stable ID 与时间格式的实现级规则已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.4_StableID日期时间与数值类型实现细化_候选冻结稿(1).md`

## 22.9 当前学期的唯一配置

Gate 1 业务语义要求：

> 同一时间只存在一个当前学期。

如果每个 Semester 都保存：

```text
bool isCurrent
```

就可能产生：

```text
Semester A = true
Semester B = true
```

或：

```text
全部 false
```

这种不一致状态。

### 决策 D47

> Semester 不持久化独立 `isCurrent` 布尔字段。系统“当前学期是谁”作为全局唯一配置，通过稳定 `currentSemesterId` 表达。

概念：

```text
currentSemesterId = SEM_2026_1
```

当前不冻结承载该值的一定是 `SystemConfig` 类；具体落在配置文件、配置对象还是学期管理模块，留待 Persistence / Service 审计。

### 决策 D48

> 判断某 Semester 是否为当前学期，通过 `semester.semesterId == currentSemesterId` 动态判断。

管理员切换当前学期：

```text
currentSemesterId
SEM_A
↓
SEM_B
```

只修改这一份权威配置，不修改多个 Semester 的布尔状态。

---

## 22.10 VolunteerRecord 当前审核管理员

### 决策 D49

> VolunteerRecord 的当前审核 Administrator 通过稳定 `reviewerAccountId` 关联，不长期保存 `Administrator*`。

这与：

```text
ownerAccountId
categoryId
operatorAccountId
```

等长期持久关联采用同一原则。

### 决策 D50

> VolunteerRecord 只保存当前最近一次仍对当前记录状态有效的审核结果，不保存完整历次审核历史。

例如：

```text
第一次：
Rejected
reviewer = admin01
note = 证明人信息不完整

学生修改后重新提交

第二次：
Approved
reviewer = admin02
note = 审核通过
```

最终 VolunteerRecord 当前审核字段只反映第二次。

历史上第一次驳回由 OperationLog 负责追溯。

因此不新增：

```text
ReviewRecord
AuditRecord
vector<ReviewHistory>
```

等审核历史实体 / 容器。

---

## 22.11 VolunteerRecord 状态枚举与受控状态机

### 决策 D51

> `VolunteerRecord.status` 使用强类型 `enum class RecordStatus`。

冻结状态只有：

```text
Pending
Approved
Rejected
Withdrawn
```

不使用字符串，不使用：

```text
isPending
isApproved
isRejected
isWithdrawn
```

多个 bool 拼接状态。

### 决策 D52

> VolunteerRecord 状态转换采用受控状态机，不提供任意 `setStatus()`。

当前合法主转换：

```text
创建并提交
→ Pending

Pending
├── Administrator 审核通过 → Approved
├── Administrator 审核驳回 → Rejected
└── Student 主动撤回       → Withdrawn

Rejected
└── Student 修改并重新提交 → Pending
```

当前没有业务依据支持：

```text
Approved → Pending
Approved → Rejected
Withdrawn → Pending
```

等普通直接转换。

已通过记录发现异常时走：

```text
管理员强制更正
或
管理员物理删除
```

而不是任意回退状态。

---

## 22.12 当前审核信息清理与历史日志

### 决策 D53

> `reviewerAccountId` 与 `reviewNote` 是 VolunteerRecord 的当前审核信息。

因此：

```text
Rejected
reviewerAccountId = admin01
reviewNote = 某驳回原因

↓ 学生修改并重新提交

Pending
reviewerAccountId = 空
reviewNote = 空
```

原因：

如果 Pending 仍显示旧审核人和旧驳回意见，就会形成“当前待审核但看起来已经被审核”的语义冲突。

### 决策 D54

> VolunteerRecord 不承担历次审核历史保存职责。每一次需要审计的管理员审核 / 治理关键操作都独立写入 OperationLog。

因此，即使：

- 当前 reviewer 被覆盖；
- Rejected → Pending 时 reviewNote 被清空；
- 后续另一管理员重新审核；

旧日志都不被修改、覆盖或删除。

形成稳定边界：

```text
VolunteerRecord
= 当前事实 + 当前状态 + 当前审核结果

OperationLog
= 关键管理员行为历史
```

---

## 22.13 VolunteerRecord 状态—审核—结算字段不变量

### 决策 D55

> VolunteerRecord 的审核字段和结算字段必须与 RecordStatus 保持状态一致性。

禁止：

```text
Pending + finalScore
Rejected + settledCoefficient
Withdrawn + reviewerAccountId
```

这类业务自相矛盾的数据。

### 决策 D56

> 只有 Approved 状态允许形成正式结算结果：

```text
finalCategoryId
finalDuration
settledCoefficient
finalScore
```

各状态约束如下。

#### Pending

```text
reviewerAccountId = 空
reviewNote = 空
无正式 finalCategoryId / finalDuration
无 settledCoefficient / finalScore
```

#### Rejected

```text
reviewerAccountId ≠ 空
reviewNote ≠ 空
无正式结算结果
```

管理员如果认为申报类别 / 时长有问题，可以通过驳回意见告知学生修改；Rejected 不提前形成正式结算事实。

#### Approved

```text
reviewerAccountId ≠ 空
finalCategoryId 有效
finalDuration 有效
settledCoefficient 有效
finalScore 有效
reviewNote 可为空或有说明
```

#### Withdrawn

```text
无当前审核结果
无正式结算结果
```

这些不变量后续应由领域对象与业务服务共同保护，而不是依赖 Qt 页面“不要填错”。

---

## 22.14 Administrator 与 Student 的管理关联

Gate 2 的：

```text
Administrator ---- manages ---- Student
```

表示管理员角色能够执行学生账号管理，并不表示某个学生固定“归属”某个管理员。

### 决策 D57

> Student 不保存 `managerAccountId`，也不长期保存 `Administrator* manager`。

因为系统允许：

```text
admin01 创建学生
admin02 重置密码
admin03 禁用
admin01 再恢复
```

不存在“当前固定负责管理员”这一业务事实。

因此不制造：

```text
Student.managerAccountId
```

这种错误领域语义。

### 决策 D58

> 对 Student 的关键账号管理事件通过 OperationLog 追溯执行管理员，不在 Student 中维护管理员操作历史。

当前包括已确认具有追溯价值的：

- 创建学生账号；
- 禁用；
- 恢复；
- 密码重置；
- 其他后续明确纳入日志范围的关键账号治理动作。

---

## 22.15 BadgeRule 与 VolunteerCategory 的唯一性补全

原 D27 只解决：

```text
BadgeRule.categoryId
```

能够指向 Category。

关系完整性检查进一步发现，还必须落实 Gate 2 的：

```text
VolunteerCategory 1 ---- 0..1 BadgeRule
```

### 决策 D59

> 一套 BadgeRule 必须且只能绑定一个 VolunteerCategory；一个 VolunteerCategory 最多只能对应一套 BadgeRule。

技术语义：

```text
badgeRuleId
→ BadgeRule 自身实体身份

categoryId
→ 与 Category 的稳定关联
→ 同时施加“一个 categoryId 最多被一个 BadgeRule 使用”的业务唯一性约束
```

允许：

```text
环保实践
└── 环保卫士（铜 / 银 / 金一整套）
```

不允许：

```text
环保实践
├── 环保卫士规则 A
└── 环保卫士规则 B
```

同一 BadgeRule 也不能同时绑定多个 Category。

---

## 22.16 排行榜称号的人数不足边界

排行榜称号与专项 BadgeRule 是两套独立体系：

```text
BadgeRule
→ 专项类别长期投入

Ranking Title
→ 月度 / 学期 / 总积分绝对排名
```

### 决策 D60

> 排行榜称号只依据绝对排名授予，不设置最低参评人数。

规则：

```text
只有 1 名有效参榜学生
→ 第 1 名 = 金
→ 银 / 铜空缺

只有 2 名
→ 第 1 名 = 金
→ 第 2 名 = 银
→ 铜空缺

3 名及以上
→ 第 1 / 2 / 3 名分别 = 金 / 银 / 铜
```

不会因为人数少：

- 取消第 1 名的金；
- 把第 2 名“提升”为金；
- 制造不存在的第 3 名。

此规则适用于月度、学期、总积分三类排行榜。

---

## 22.17 OperationLog 记录范围与数据导出修订

原 Gate 1 / Gate 2 曾把：

```text
执行数据导出
```

列入关键操作日志范围。

后续架构审计发现，普通数据导出属于：

```text
读取当前数据
→ 生成 CSV / TXT / Markdown 等外部副本
→ 不修改原系统业务状态
```

如果强行给导出日志设计实体 `targetId`，反而会诱导创建不存在的：

```text
Leaderboard entity
Statistics entity
ExportResult entity
```

等假领域对象。

### 决策 D61

> 只读数据导出不再生成 OperationLog。

这是一项**显式后续需求修订**：

- Administrator 仍保留 `ExportData` Permission；
- 导出服务仍然存在；
- 导出结果仍可生成；
- 但导出本身不再作为 OperationLog 管理事件。

当前日志边界：

```text
重点记录：
→ 审核
→ 治理
→ 账号关键管理
→ 规则 / 配置修改
→ 公开内容治理
→ 物理删除 / 强制更正等责任性操作

不记录：
→ 普通浏览
→ 普通查询
→ 查看统计
→ 查看排行榜
→ 只读数据导出
```

因此后续同步 Gate 1 / Gate 2 时必须明确写“后续架构审计修订”，不能无痕覆盖旧版本。

---

## 22.18 currentSemesterId 引用完整性

### 决策 D62

> `currentSemesterId` 必须始终指向当前存在的合法 Semester。

当前版本不设计 Semester 物理删除流程。

这样可以避免：

```text
currentSemesterId = SEM_A
但 SEM_A 已不存在
```

产生悬空配置。

如果以后确实提出 Semester 删除功能，应另行设计：

- 当前学期保护；
- 引用检查；
- 切换后删除；
- 历史配置处理；

不能在当前阶段默认为可直接删除。

---

# 23. Gate 3.2 关系完整性检查

Gate 2 主要业务关系与当前技术映射逐项核对如下。

| 业务关系 / 约束 | Gate 3.2 技术映射 | 结果 |
|---|---|---|
| Student 1:N VolunteerRecord | `VolunteerRecord.ownerAccountId` | PASS |
| Administrator → VolunteerRecord 当前审核 | `reviewerAccountId` | PASS |
| Administrator → Student 管理 | 不持久化 manager 关系；关键行为进 OperationLog | PASS |
| VolunteerRecord → appliedCategory | `appliedCategoryId` | PASS |
| VolunteerRecord → finalCategory | `finalCategoryId` | PASS |
| Category 历史结算 | Record 冻结 `settledCoefficient + finalScore` | PASS |
| VolunteerCategory 1:0..1 BadgeRule | `BadgeRule.categoryId` + categoryId 唯一约束 | PASS |
| VolunteerRecord 1:0..1 DiaryPost | `DiaryPost.recordId` + recordId 唯一关联 | PASS |
| Student 1:N DiaryPost | `recordId → VolunteerRecord.ownerAccountId` 推导 | PASS |
| Student N:M DiaryPost 点赞 | `LikeRelation(studentAccountId, postId)` | PASS |
| 点赞唯一性 | `(studentAccountId, postId)` 复合唯一键 | PASS |
| DiaryPost 删除 → LikeRelation | 服务 / 数据层级联清理 | PASS |
| Semester → Record statistics | `serviceDate` 与时间范围动态筛选 | PASS |
| 当前学期 | 独立 `currentSemesterId` | PASS |
| Administrator 1:N OperationLog | `operatorAccountId` | PASS |
| OperationLog → target | `OperationTargetType + targetId` | PASS |
| 排行榜称号 | 派生结果 + 绝对名次边界 | PASS |

关系完整性检查结论：

> **PASS**

没有发现必须新增领域实体或必须推翻 D22～D62 的关系缺口。

---

# 24. Gate 3.2 生命周期一致性终审

本轮检查对象删除、停用、禁用、状态切换和配置修改后，关联对象是否会产生非法状态。

## 24.1 Student

```text
Active ↔ Disabled
```

Disabled 后：

- 不能继续登录并发起学生操作；
- 既有 VolunteerRecord 保留；
- 既有 DiaryPost 保留；
- 既有 LikeRelation 保留；
- 既有有效记录继续参与动态积分 / 排行 / 徽章计算。

有业务数据的 Student 不物理删除。

结论：

> PASS

## 24.2 Administrator

Administrator Disabled 后：

- 不再允许继续执行管理操作；
- 历史 `reviewerAccountId` 保留；
- 历史 `operatorAccountId` 保留。

当前需求未设计 Administrator 物理删除，因此 Gate 3.2 不新增删除语义。

结论：

> PASS

## 24.3 VolunteerRecord

状态机与审核 / 结算字段不变量已经由 D51～D56 闭合。

Approved 记录被管理员物理删除时：

```text
VolunteerRecord 删除
↓
关联 DiaryPost 删除
↓
LikeRelation 级联删除
↓
积分重新汇总
↓
排行榜重算
↓
排行称号重判
↓
专项徽章重判
↓
OperationLog 保留删除审计事实
```

结论：

> PASS

## 24.4 VolunteerCategory

已投入使用的 Category 不物理删除，只能停用。

因此历史：

```text
appliedCategoryId
finalCategoryId
BadgeRule.categoryId
```

不会因类别删除悬空。

`currentCoefficient` 修改不追溯历史结算，历史由 Record 快照保护。

结论：

> PASS

## 24.5 BadgeRule

已投入使用后不物理删除，采用停用。

停用后当前学生专项徽章动态判定自然失效；由于学生当前徽章不持久化，不存在“已删除规则但学生仍挂着永久徽章对象”的矛盾。

门槛修改后按当前有效时长动态重判。

结论：

> PASS

## 24.6 Semester

VolunteerRecord 不持有 semesterId，因此切换当前学期或调整时间范围不会改写历史 Record。

`currentSemesterId` 通过 D62 保证指向真实存在的 Semester。

当前不设计 Semester 物理删除。

结论：

> PASS

## 24.7 DiaryPost

普通下架：

```text
DiaryPost 仍存在
LikeRelation 保留
VolunteerRecord 不受影响
积分 / 排行 / 徽章不受影响
```

来源 VolunteerRecord 被物理删除：

```text
DiaryPost 同步物理删除
LikeRelation 同步清理
```

Approved Record 在正常业务中不直接回退到 Pending / Rejected / Withdrawn，因此不会出现“帖子仍存在但来源记录普通状态回退失去展示资格”的非法状态。

结论：

> PASS

## 24.8 LikeRelation

```text
点赞
→ 创建 LikeRelation

取消点赞
→ 删除 LikeRelation

DiaryPost 普通下架
→ 保留 LikeRelation

DiaryPost 物理删除
→ 级联删除 LikeRelation
```

Disabled Student 不能登录继续操作，但其既有点赞关系可保留。

结论：

> PASS

## 24.9 OperationLog

OperationLog 是历史审计事实：

- 不允许手动修改；
- 不允许手动删除；
- target 对象之后物理删除不影响日志；
- operator 之后 Disabled 不影响日志；
- 只读数据导出不再创建日志。

结论：

> PASS

---

# 25. Gate 3.2 最终结论

经过：

```text
D22 ～ D62
↓
关系技术映射
↓
关系完整性检查
↓
生命周期一致性终审
```

当前正式结论：

> **Gate 3.2——组合 / 聚合 / 关联技术映射：PASS**

当前稳定原则包括：

```text
业务聚合
≠
C++ 内存所有权

独立实体长期关系
→ 稳定 ID 优先

历史事实
→ 不依赖当前对象内存地址

可修改名称
≠
实体主键

派生结果
→ 不重复持久化

当前状态
≠
历史事件

状态转换
→ 受控领域语义

关系唯一性
→ 显式业务约束

生命周期级联
→ Service / 数据层协调

只读操作
→ 不为了日志制造假领域实体
```


---
