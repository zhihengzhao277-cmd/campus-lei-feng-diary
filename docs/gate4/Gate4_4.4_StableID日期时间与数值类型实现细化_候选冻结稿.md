# Gate 4.4 — Stable ID、日期时间与数值类型实现细化（候选冻结稿）

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 4 —— 实现审计  
> 前置依赖：
> - Gate 3.1：User / Student / Administrator 账号标识与对象职责
> - Gate 3.2：领域对象稳定 ID、跨对象关系、LikeRelation 逻辑唯一键
> - Gate 3.3：StatisticsService / RankingService / SemesterService
> - Gate 3.4：Repository / Persistence、stable ID 跨层定位、system_config.txt
> - Gate 3.5：Qt 只保存 stable ID，不长期持有 Domain 指针
> - Gate 4.2：Repository 查询、Statistics / Ranking 聚合与排序算法
> - Gate 4.3：多文件 Prepare / Commit 与 system_config 可参与统一提交
>
> 文档性质：实现级候选冻结稿  
> 当前主题：Stable ID 生成、ID 序号持久化、Date / DateTime 表示、CSV 日期时间格式、时长 / 系数 / 积分数值类型与规范化  
> 目标：让后续 AI、Codex、开发者或审计者在不依赖本次聊天上下文的情况下，能够准确理解本项目各类稳定标识、日期时间、时长、系数和积分的最终实现方向。
>
> 重要说明：
>
> 1. 本文件只细化 Gate 3 已冻结的数据语义，不重新修改领域关系。
> 2. 本文件冻结的是“ID / 时间 / 数值的语义与表示规则”，不提前死锁最终 C++ API 名称。
> 3. 当前选择“时长 = `double` 小时制”，这是用户已确认的正式候选方向。
> 4. CSV 具体完整 header、字段顺序与 schema_version 细节仍由 Gate 4.5 冻结。
> 5. Date / DateTime 继续保持纯 C++，Qt 只在 Presentation 层转换。

---

# 1. 本专题解决什么问题

Gate 3 已经明确：

```text
跨对象关系长期保存 stable ID
Qt 页面保存 stable ID
OperationLog 使用 targetId
VolunteerRecord 通过 ownerAccountId / categoryId 等稳定标识关联
LikeRelation 使用 (studentAccountId, postId) 逻辑唯一键
```

但此前没有完全冻结：

```text
recordId / postId / logId 等具体怎么生成
程序重启后怎样继续生成
对象删除后 ID 能不能复用
system_config 是否记录 next sequence
Date / DateTime 用什么纯 C++ 表示
CSV 里日期时间按什么格式
时长、系数、积分到底用 int / float / double
积分是否规范化
排行榜怎样避免浮点尾差
```

这些问题如果不提前统一，会直接影响：

```text
Repository
Persistence
CSV schema
OperationLog
Qt stable ID
Statistics
Ranking
Badge
测试数据
```

因此必须在正式大规模编码前冻结。

---

# 2. Stable ID 总体原则

当前 stable ID 必须满足：

```text
可长期持久化
可跨 Repository / Service / Presentation 传递
对象物理位置变化后仍有效
vector 下标变化不影响
程序重启后仍可识别
已成功提交过的历史 ID 不复用
```

当前不使用：

```text
vector index
pointer address
memory address
row number
Qt table row
```

作为长期业务身份。

---

# 3. accountId 是特殊类型的 Stable ID

`accountId` 不由系统自增 ID 生成器产生。

当前语义：

```text
Student.accountId
→ 学号

Administrator.accountId
→ 管理员账号
```

它属于：

> **业务外部提供且本身具有业务含义的稳定账号标识。**

因此不再额外创建：

```text
USR000001
```

这种内部账号 ID。

---

# 4. accountId 唯一性

Student 和 Administrator 共用同一账号登录入口。

因此：

> **accountId 在 Student + Administrator 两类用户之间全局唯一。**

创建新账号时必须检查：

```text
students_
+
administrators_
```

两个集合。

不能出现：

```text
Student.accountId = 20250001
Administrator.accountId = 20250001
```

否则 AuthenticationService 无法唯一确定真实账号主体。

---

# 5. 哪些 ID 由系统生成

当前建议由系统生成：

```text
VolunteerRecord.recordId
VolunteerCategory.categoryId
BadgeRule.badgeRuleId
Semester.semesterId
DiaryPost.postId
OperationLog.logId
```

当前 LikeRelation：

```text
不新增 likeId
```

继续使用：

```text
(studentAccountId, postId)
```

作为逻辑唯一键。

---

# 6. 系统生成 ID 的格式

当前统一采用：

> **固定类型前缀 + 零填充单调递增序号**

例如：

```text
REC000001
REC000002

CAT000001
CAT000002

BDG000001

SEM000001

POST000001

LOG000001
```

建议前缀：

```text
REC  → VolunteerRecord
CAT  → VolunteerCategory
BDG  → BadgeRule
SEM  → Semester
POST → DiaryPost
LOG  → OperationLog
```

---

# 7. 为什么不用 UUID

UUID 例如：

```text
550e8400-e29b-41d4-a716-446655440000
```

虽然碰撞概率极低，但当前课程项目不需要这种规模的分布式唯一性。

缺点：

```text
CSV 可读性差
调试不方便
日志定位不直观
测试数据难阅读
答辩解释成本更高
```

因此不采用 UUID 作为默认 stable ID。

---

# 8. 为什么不用 timestamp 直接当 ID

例如：

```text
REC20260903180432715
```

不推荐。

原因：

```text
ID 与时间语义混淆
同一时间创建多个对象时仍要处理碰撞
测试数据不够稳定
重放测试不方便
```

时间字段和身份字段应该分离。

---

# 9. 为什么不能仅靠 max(existingId) + 1

假设曾经存在：

```text
REC000001
REC000002
REC000003
```

后来：

```text
REC000003
```

被物理删除。

如果程序重启后只扫描当前文件：

```text
max = 2
```

那么下一条可能又生成：

```text
REC000003
```

这会产生历史 ID 重用。

而 OperationLog 可能仍保留：

```text
targetId = REC000003
```

这样旧日志可能错误地看起来指向新的对象。

因此：

> **成功提交过的 stable ID 永不复用。**

---

# 10. next sequence 持久化

当前最优方案：

> **每一类系统生成 ID 都维护持久化 next sequence。**

建议存于：

```text
system_config.txt
```

概念：

```text
next_record_seq=124
next_category_seq=8
next_badge_rule_seq=5
next_semester_seq=7
next_post_seq=63
next_log_seq=418
```

含义：

```text
next_record_seq=124
→ 下一条 VolunteerRecord 使用 REC000124
```

成功分配后内存中：

```text
next_record_seq
→ 125
```

---

# 11. next sequence 与 Gate 4.3 的关系

创建一个新对象可能同时修改：

```text
目标 Repository 文件
+
system_config.txt
```

例如：

```text
创建 DiaryPost
↓
diary_posts.csv
+
system_config.txt
```

创建需要记录 OperationLog 的对象甚至可能影响：

```text
业务文件
+
operation_logs.csv
+
system_config.txt
```

这不构成新的架构问题。

Gate 4.3 已经冻结：

```text
多文件 Prepare All Before Commit
```

因此 `system_config.txt` 只是本次受影响持久化参与者之一。

---

# 12. ID 分配时机

不能在业务刚进入时立即永久消耗 ID。

推荐：

```text
业务前置校验通过
↓
权限检查通过
↓
确定 affected repositories
↓
建立操作前 snapshots
↓
读取当前 next sequence
↓
构造新 stable ID
↓
内存 next sequence + 1
↓
创建 Domain 对象
↓
Prepare
↓
Commit
```

---

# 13. 未提交 ID 是否可以恢复

如果：

```text
ID 已在内存中生成
但 Prepare 失败
```

Gate 4.3 会：

```text
恢复 Repository / config snapshot
```

那么：

```text
next sequence
```

也恢复到操作前值。

这种情况下该 ID 可以再次使用，因为：

> 它从未成功 Commit，也从未成为系统历史中的正式业务对象。

---

# 14. 成功提交后的 ID 永不复用

只要一次创建业务已经完整 Commit：

```text
REC000124
```

就永久视为已经消费。

即使未来：

```text
REC000124
```

被物理删除，也不能再次分配。

目的：

```text
历史日志不歧义
旧导出不歧义
调试记录不歧义
跨版本测试不歧义
```

---

# 15. 是否现在冻结 IdGenerator 的精确 API

不冻结。

可能存在合理实现：

```cpp
nextRecordId()
nextPostId()
nextLogId()
```

也可能采用：

```cpp
generate(IdType::VolunteerRecord)
```

或者：

```text
ConfigurationService / IdSequenceStore
```

统一管理。

当前只冻结：

```text
ID 格式
next sequence 语义
持久化
不可重用规则
```

最终函数签名在真实代码形成时再审计。

---

# 16. Date 与 DateTime 必须分开

当前业务中有两类时间语义。

## Date

只需要：

```text
年
月
日
```

适用于：

```text
VolunteerRecord.serviceDate
Semester.startDate
Semester.endDate
```

---

## DateTime

需要：

```text
年
月
日
时
分
秒
```

适用于：

```text
Student.createdAt
Student.lastLoginAt
DiaryPost 正式公开时间
LikeRelation 点赞时间
OperationLog.operationTime
```

不能为了方便把所有时间统一成 DateTime。

---

# 17. 为什么 serviceDate 必须是 Date

学期归属和月榜统计都是根据：

```text
哪一天完成志愿服务
```

判断。

不依赖：

```text
几点几分
```

因此：

```text
serviceDate
```

应使用纯 Date。

Semester 也只定义：

```text
startDate
endDate
```

时间范围。

---

# 18. Domain 不依赖 QDate / QDateTime

当前不允许：

```cpp
class VolunteerRecord {
    QDate serviceDate_;
};
```

也不允许：

```cpp
class OperationLog {
    QDateTime operationTime_;
};
```

原因：

```text
Domain 会依赖 Qt
Console V0.1 / V0.2 无法保持纯 C++
测试需要 Qt 环境
业务层与 Presentation 耦合
```

因此 Domain 使用纯 C++ 值对象。

---

# 19. Date 推荐结构

概念：

```cpp
struct Date {
    int year;
    int month;
    int day;
};
```

实际实现建议升级为：

```text
具有合法性约束的轻量 value object
```

至少拒绝：

```text
2026-02-30
2026-13-01
2026-00-10
```

---

# 20. DateTime 推荐结构

概念：

```cpp
struct DateTime {
    Date date;
    int hour;
    int minute;
    int second;
};
```

合法性至少包括：

```text
0 <= hour <= 23
0 <= minute <= 59
0 <= second <= 59
Date 本身合法
```

---

# 21. Date 比较规则

按：

```text
year
↓
month
↓
day
```

字典序比较。

应支持或等价实现：

```text
==
<
<=
>
>=
```

这样学期归属可以直接表达：

```cpp
semester.startDate() <= record.serviceDate()
&&
record.serviceDate() <= semester.endDate()
```

---

# 22. DateTime 比较规则

按：

```text
year
month
day
hour
minute
second
```

顺序比较。

用于：

```text
OperationLog 时间倒序
DiaryPost 时间排序
LikeRelation 时间排序
lastLoginAt 显示
```

---

# 23. CSV Date 格式

统一：

```text
YYYY-MM-DD
```

例如：

```text
2026-09-03
```

要求：

```text
month 两位
day 两位
year 四位
```

不采用：

```text
2026/9/3
09-03-2026
2026年9月3日
```

作为权威持久化格式。

---

# 24. CSV DateTime 格式

统一：

```text
YYYY-MM-DDTHH:MM:SS
```

例如：

```text
2026-09-03T18:04:32
```

使用：

```text
T
```

分隔 Date 与 Time。

不在权威 CSV 中使用：

```text
2026-09-03 18:04:32 PM
中文日期
区域化格式
```

---

# 25. 为什么选择 ISO 风格格式

优势：

```text
人类可读
解析简单
格式稳定
跨 locale
按固定格式文本也天然接近时间顺序
适合 CSV
适合测试
```

---

# 26. 时区策略

当前系统是：

```text
单机
校园本地应用
不跨时区同步
不运行分布式服务
```

因此不引入：

```text
UTC conversion
timezone database
IANA timezone
DST 复杂转换
offset 持久化
```

当前统一：

> **DateTime 使用运行机器当地 civil time，精确到秒。**

CSV 示例：

```text
2026-09-03T18:04:32
```

不追加：

```text
Z
+08:00
```

---

# 27. createdAt / lastLoginAt

Student：

```text
createdAt
→ 必有

lastLoginAt
→ 可空
```

因为一个刚创建但从未成功登录的 Student 没有真实 lastLoginAt。

不能使用：

```text
1970-01-01T00:00:00
```

等伪造哨兵值。

CSV 对可空时间可以使用空字段：

```text
...,2026-09-03T18:00:00,
```

精确 optional C++ 类型在 Gate 4.5 / API 细化时确定。

---

# 28. 当前主要数值字段

至少包括：

```text
VolunteerRecord.appliedDuration
VolunteerRecord.finalDuration

VolunteerCategory.currentCoefficient
VolunteerRecord.settledCoefficient

VolunteerRecord.finalScore

Statistics.score
Statistics.duration

BadgeRule.bronzeThreshold
BadgeRule.silverThreshold
BadgeRule.goldThreshold
```

这些字段不能在不同模块使用不同单位。

---

# 29. 时长正式选择：方案 A

用户已确认：

> **所有业务时长统一使用 `double`，单位为小时。**

示例：

```text
1.5
= 1 小时 30 分钟

2.25
= 2 小时 15 分钟

0.75
= 45 分钟
```

---

# 30. 为什么不采用 int minutes

`int minutes` 的优点：

```text
完全避免时长浮点表示问题
```

但当前业务和评分公式天然以：

```text
小时 × 系数
```

表达。

如果使用分钟，会在：

```text
UI
CSV
统计
BadgeRule
积分
报告
```

频繁做：

```text
minutes ↔ hours
```

转换。

对于当前课程项目：

> **`double` 小时更直观、更易解释，复杂度更低。**

---

# 31. 时长字段统一规则

以下全部使用：

```cpp
double
```

并且单位统一：

```text
hours
```

字段：

```text
appliedDuration
finalDuration
Statistics.duration
BadgeRule thresholds
```

不能出现：

```text
appliedDuration = hours
BadgeRule threshold = minutes
```

这种单位混用。

---

# 32. 系数类型

以下统一：

```cpp
double
```

```text
VolunteerCategory.currentCoefficient
VolunteerRecord.settledCoefficient
```

例如：

```text
1.0
1.25
1.5
2.0
```

不限定为 integer。

---

# 33. finalScore 类型

统一：

```cpp
double
```

计算：

```text
finalScore
=
finalDuration
×
settledCoefficient
```

并且是在 VolunteerRecord Approved 正式结算时形成冻结事实。

---

# 34. 历史积分不能重算

一旦 Approved：

```text
settledCoefficient
finalScore
```

成为该 VolunteerRecord 的历史结算快照。

未来管理员修改：

```text
VolunteerCategory.currentCoefficient
```

不能让历史：

```text
finalScore
```

变化。

Statistics / Ranking 必须：

```text
sum(record.finalScore)
```

而不是：

```text
record.finalDuration
×
current Category coefficient
```

重新计算。

---

# 35. BadgeRule threshold 的单位

BadgeRule：

```text
bronzeThreshold
silverThreshold
goldThreshold
```

表示：

> **指定 Category 的累计有效服务时长门槛。**

因此它们统一：

```text
double
单位：小时
```

与：

```text
finalDuration
```

完全一致。

---

# 36. 为什么不用 float

统一使用：

```cpp
double
```

不混用：

```text
float
double
long double
```

原因：

```text
double 精度充足
标准库支持成熟
Qt 转换自然
代码统一
减少隐式转换
```

---

# 37. 浮点数问题

二进制 floating point 存在：

```text
0.1 + 0.2 != 精确十进制 0.3
```

但当前系统：

```text
不是金融结算系统
不是高精度科学计算
```

不需要引入：

```text
decimal library
fixed-point money type
BigDecimal
```

---

# 38. finalScore 两位小数规范化

当前冻结：

> **finalScore 在 Approved 正式结算时统一规范化到两位小数。**

例如：

```text
finalDuration = 1.75
settledCoefficient = 1.5

raw score = 2.625
↓
finalScore = 2.63
```

保存到 VolunteerRecord：

```text
2.63
```

而不是继续保存：

```text
2.625000000...
```

---

# 39. 为什么在结算时规范化

如果只在 UI 展示时：

```text
2.63
```

但内部保存：

```text
2.625
```

就可能出现：

```text
UI 看起来相同
排行榜内部比较不同
CSV 与展示不一致
```

因此：

> **正式业务值在结算点就统一规范化。**

---

# 40. 统计积分规范化

StatisticsService：

```text
sum(Approved finalScore)
```

得到总积分后，在形成正式统计结果时建议再次：

```text
normalize / round to 2 decimals
```

这样：

```text
Ranking
UI
Export
```

看到的是统一值。

---

# 41. 排行榜比较

冻结排序规则：

```text
score 降序
↓
validDuration 降序
↓
validRecordCount 降序
↓
accountId 升序
```

RankingService 使用：

```text
已规范化统计 score
```

作为第一排序键。

这样不需要在 comparator 里到处使用：

```cpp
fabs(a.score - b.score) < 1e-9
```

解决业务相等问题。

---

# 42. 时长与系数小数位

当前建议业务数据：

```text
duration
coefficient
```

正常输入 / CSV / UI 最多展示两位小数。

但本文件不提前把：

```text
“必须恰好两位”
```

变成所有 Domain 构造器的硬规则。

核心冻结的是：

```text
单位
类型
finalScore 规范化
统计规范化
```

精确输入限制可在实际 UI / Service validation 时决定。

---

# 43. 数值显示与持久化区别

应区分：

```text
内部值
CSV 序列化
UI 显示
```

但三者必须遵守统一业务精度。

不允许：

```text
内部 finalScore = 2.625
CSV = 2.625
UI = 2.63
Ranking = 2.625
```

当前 formal finalScore 应是：

```text
2.63
```

然后：

```text
CSV
Ranking
UI
```

都基于该正式值。

---

# 44. Date / DateTime 与 Qt 的转换

V0.3 / V1.0 Qt 层可以：

```text
Domain Date
↓
QDate

Domain DateTime
↓
QDateTime
```

用于显示 / Qt 控件。

反向输入：

```text
QDate
↓
Date

QDateTime
↓
DateTime
```

转换只能发生在：

```text
Presentation / adapter boundary
```

不得改变 Domain 类型。

---

# 45. Date / DateTime 与 Console

V0.1 / V0.2 Console 可以直接：

```text
parse YYYY-MM-DD
parse YYYY-MM-DDTHH:MM:SS
```

形成纯 C++：

```text
Date
DateTime
```

因此同一 Domain 可同时服务：

```text
Console
Qt
```

---

# 46. 当前禁止实现清单

后续 AI / Codex 不得擅自采用：

```text
vector 下标作为 stable ID
内存地址作为 stable ID
Qt row 作为 stable ID
成功提交过的 ID 重新使用
删除对象后通过 max(existing)+1 重新生成旧 ID
UUID 默认替代当前前缀序号方案
timestamp 直接作为对象 ID
LikeRelation 额外增加无需求的 likeId

Domain 使用 QDate / QDateTime
所有日期统一用 string 而没有值对象
用 1970-01-01 表示 lastLoginAt 未发生
普通业务引入复杂 timezone database

时长一部分用分钟、一部分用小时
float / double 混用
历史积分按 currentCoefficient 重算
只在 UI round，内部 formal score 不统一
使用 decimal / BigDecimal 等超出当前项目需要的数值体系
```

除非后续有显式修订。

---

# 47. 当前实现级候选决策

## G4-ID-01 — accountId

> **`accountId` 由账号业务提供，其中 Student 使用学号、Administrator 使用管理员账号；Student 与 Administrator 共用同一账号命名空间并保持全局唯一。`accountId` 不由系统内部自增 ID 生成器创建。**

---

## G4-ID-02 — 系统 Stable ID 格式

> **`recordId / categoryId / badgeRuleId / semesterId / postId / logId` 采用“固定类型前缀 + 零填充单调递增序号”的稳定文本 ID，例如 `REC000001 / CAT000001 / BDG000001 / SEM000001 / POST000001 / LOG000001`。LikeRelation 不设置独立 likeId，继续采用 `(studentAccountId, postId)` 逻辑唯一键。**

---

## G4-ID-03 — next sequence 与历史 ID 不复用

> **每类系统生成 ID 的 next sequence 持久化于 `system_config.txt`。任何已经成功 Commit 成为正式业务历史的 stable ID 永不复用，即使对应对象后来被物理删除。这样 OperationLog、导出或历史调试记录中的 targetId 不会被未来新对象重新占用。**

---

## G4-ID-04 — ID 分配与事务失败

> **ID 分配发生在业务前置校验和权限检查通过、操作前 snapshot 已建立之后。若业务在正式 Commit 前失败且完整恢复 snapshot，则尚未成功提交的 sequence 可随配置 snapshot 一并恢复；一旦创建操作完整 Commit，该 ID 永久视为已消费。**

---

## G4-TIME-01 — 纯 C++ Date / DateTime

> **Domain 使用纯 C++ `Date` 与 `DateTime` 值语义表示，不依赖 `QDate / QDateTime`。`Date` 保存 year/month/day；`DateTime` 在 Date 基础上保存 hour/minute/second，并负责基本合法性与比较。**

---

## G4-TIME-02 — 字段时间语义

> **`VolunteerRecord.serviceDate / Semester.startDate / Semester.endDate` 使用 Date；`Student.createdAt / Student.lastLoginAt / DiaryPost 正式公开时间 / LikeRelation 点赞时间 / OperationLog.operationTime` 使用 DateTime。`lastLoginAt` 允许为空，不使用虚构日期表示“从未登录”。**

---

## G4-TIME-03 — 持久化格式与时区

> **CSV 中 Date 使用 `YYYY-MM-DD`，DateTime 使用 `YYYY-MM-DDTHH:MM:SS`。系统作为单机校园应用统一使用运行机器当地 civil time，精确到秒，不引入 UTC / timezone database / offset 持久化体系。**

---

## G4-NUM-01 — 数值类型与单位

> **志愿时长、类别积分系数、结算系数、最终积分、统计积分以及 BadgeRule 时长门槛统一使用 `double`。所有时长统一以“小时”为业务单位，不在不同模块中混用分钟与小时；当前正式选择为方案 A：`double` 小时制。**

---

## G4-NUM-02 — finalScore 正式结算

> **`finalScore = finalDuration × settledCoefficient` 在 VolunteerRecord 审核通过形成正式结算事实时统一规范化到两位小数并冻结。后续 Statistics / Ranking 只使用已冻结 `finalScore`，不得根据当前 VolunteerCategory coefficient 重算历史积分。**

---

## G4-NUM-03 — 统计与浮点规范化

> **StatisticsService 在聚合已冻结 finalScore 后形成正式统计结果时统一规范化到业务显示精度，RankingService 使用该规范化统计值排序，避免 UI 展示值与排序值因浮点尾差产生不一致。当前不引入 decimal / fixed-point 等超出课程项目需要的数值体系。**

---

# 48. 后续实现待确认事项

以下内容当前不提前死锁：

```text
IdGenerator 最终类名
next sequence 配置组件最终归属
system_config.txt 精确 key 名称
前缀序号宽度是否永远为 6 位
序号超过 999999 后的行为
Date / DateTime 最终 class / struct 形式
Date / DateTime parse API
Date / DateTime operator 实现方式
lastLoginAt 最终是否 std::optional<DateTime>
C++ 标准版本
roundTo2Decimals 的最终实现函数
CSV double 最终格式化方式
UI 输入是否限定 0.5h / 0.25h 步长
duration / coefficient 的最大最小范围
```

这些应结合 Gate 4.5、真实 Domain / Service API 与 UI 实现继续确定。

---

# 49. 给后续 AI / Codex 的最短读取指令

如果上下文空间不足，只需读取本节。

```text
当前 Stable ID / Time / Numeric 路线：

ID：
1. accountId 不自增：
   - Student = 学号
   - Administrator = 管理员账号
   - 两类全局唯一

2. 系统生成：
   REC000001
   CAT000001
   BDG000001
   SEM000001
   POST000001
   LOG000001

3. LikeRelation 不要 likeId：
   唯一键 = (studentAccountId, postId)

4. 每类 next sequence 持久化到 system_config.txt。
5. 已成功 Commit 的历史 ID 永不复用。
6. 删除对象不能让旧 ID 被未来新对象重新占用。
7. ID 在 validation/auth 通过且 snapshot 建立后分配。
8. Commit 前失败且 snapshot 完整恢复时，未提交 sequence 可恢复。

Time：
1. Domain 只用纯 C++ Date / DateTime，不用 QDate / QDateTime。
2. Date：
   serviceDate
   Semester.startDate
   Semester.endDate
3. DateTime：
   Student.createdAt
   Student.lastLoginAt
   DiaryPost 正式公开时间
   LikeRelation 点赞时间
   OperationLog.operationTime
4. lastLoginAt 可空。
5. CSV：
   Date = YYYY-MM-DD
   DateTime = YYYY-MM-DDTHH:MM:SS
6. 单机系统使用机器当地 civil time，精确到秒，不做复杂 timezone。

Numeric：
1. 当前正式选择方案 A：
   所有时长 = double 小时
2. coefficient = double
3. finalScore = double
4. Badge threshold = double 小时
5. finalScore = finalDuration * settledCoefficient
6. Approved 时 finalScore round/normalize 到两位小数后冻结。
7. Statistics 使用冻结 finalScore，不重算历史 coefficient。
8. 排行榜使用规范化统计值，避免浮点尾差影响排序。
9. 不混用分钟和小时，不混 float/double，不引入 BigDecimal。
```

---

# 50. 当前状态建议

```text
Gate 4.4
Stable ID、日期时间与数值类型实现细化

G4-ID-01 ～ G4-ID-04
G4-TIME-01 ～ G4-TIME-03
G4-NUM-01 ～ G4-NUM-03

→ CANDIDATE FROZEN
```

在实际 system_config、Date / DateTime、ID 生成器、CSV 序列化以及统计排序代码完成并通过单元测试后，再进行 Gate 4.4 FINAL AUDIT。
