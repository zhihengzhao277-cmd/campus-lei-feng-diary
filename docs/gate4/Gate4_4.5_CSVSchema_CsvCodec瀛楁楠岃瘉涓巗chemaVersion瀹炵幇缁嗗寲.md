# Gate 4.5 — CSV Schema、CsvCodec、字段验证与 schema_version 实现细化（候选冻结稿）

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 4 —— 实现审计  
> 前置依赖：
> - Gate 3.1：User / VolunteerRecord 等领域字段与职责
> - Gate 3.2：领域关系、stable ID、LikeRelation 逻辑唯一键
> - Gate 3.4：Repository / Persistence、多文件 CSV、CsvCodec、system_config.txt、全局 schema_version
> - Gate 4.1：PasswordData 与密码认证字段
> - Gate 4.3：PersistenceCoordinator、`.tmp/.bak`、Prepare / Commit
> - Gate 4.4：Stable ID、Date / DateTime、double 小时制、next sequence
>
> 文档性质：实现级候选冻结稿  
> 当前主题：CSV 语法规则、各文件 Header、字段序列化、可空字段、枚举 token、数值和时间格式、system_config、schema_version、启动加载顺序与完整性校验  
> 目标：让后续 AI、Codex、开发者或审计者在不依赖本次聊天上下文的情况下，可以准确实现本项目的 CSV 读写、配置加载与启动数据校验。
>
> 重要说明：
>
> 1. 本文件只定义“持久化数据如何表示、如何解析、如何验证”，不负责文件原子替换；文件安全提交由 Gate 4.3 负责。
> 2. `CsvCodec` 只负责 CSV 语法，不负责领域 enum、ID、Date、数值或跨文件引用是否合法。
> 3. 各 CSV 只保存对象自身权威事实与必要 stable ID，不保存可动态推导结果。
> 4. 当前不建立 schema migration framework；只接受当前支持的 `schema_version`。
> 5. 权威业务文件中的损坏数据不得静默跳过，否则会造成业务事实丢失。
> 6. 本文件的字段集合以当前 Gate 3 权威 Domain 模型为准，不擅自增加未冻结字段。

---

# 1. 本专题解决什么问题

当前 Gate 3 已经明确系统采用多文件持久化：

```text
students.csv
administrators.csv
volunteer_records.csv
volunteer_categories.csv
badge_rules.csv
semesters.csv
diary_posts.csv
likes.csv
operation_logs.csv
system_config.txt
```

并且运行期模型是：

```text
启动加载
↓
Repository 内存权威集合
↓
Service 查询与修改
↓
成功后及时持久化
```

但真正编码前仍需要统一：

```text
CSV 引号怎么处理
逗号怎么处理
换行怎么处理
Header 是否固定
Enum 怎么保存
Bool 怎么保存
空值怎么表示
Date / DateTime 怎么保存
double 怎么保存
每个文件有哪些字段
system_config 怎么解析
schema_version 怎么判断
启动时怎么验证损坏数据
跨文件引用怎么校验
```

如果这些不预先统一，多个 Repository 很容易分别实现出不兼容的格式。

---

# 2. CsvCodec 的职责边界

`CsvCodec` 只做：

```text
CSV textual syntax
```

也就是：

```text
vector<string>
↔
CSV record
```

概念接口可以类似：

```cpp
std::string encodeRecord(
    const std::vector<std::string>& fields
);

std::vector<std::string> parseRecord(
    /* CSV record input */
);
```

精确签名留真实代码时决定。

---

# 3. CsvCodec 不负责什么

`CsvCodec` 不负责：

```text
"Approved" 是否属于合法 RecordStatus
"REC000001" 是否符合 ID 格式
"2026-02-30" 是否是合法 Date
"abc" 是否能转成 double
ownerAccountId 是否真的存在
BadgeRule.categoryId 是否能解析
VolunteerRecord 状态字段组合是否合法
```

这些属于：

```text
Repository mapping
Domain value object construction
startup integrity validation
```

---

# 4. 禁止使用简单 split(',')

以下实现禁止：

```cpp
split(line, ',')
```

因为业务字段中可能出现：

```text
逗号
双引号
换行
```

例如：

```text
summary =
参加环保活动，清理操场

content =
他说"今天很有意义"
```

简单 split 会破坏字段边界。

---

# 5. 当前 CsvCodec 语法规则

采用轻量 RFC-4180 风格子集。

## 5.1 普通字段

```text
abc
```

直接输出：

```text
abc
```

---

## 5.2 含逗号字段

原值：

```text
参加环保活动，清理操场
```

输出：

```csv
"参加环保活动，清理操场"
```

---

## 5.3 含双引号字段

原值：

```text
他说"很好"
```

CSV：

```csv
"他说""很好"""
```

规则：

```text
字段内部 "
→ ""
```

---

## 5.4 含换行字段

原值：

```text
第一行
第二行
```

CSV：

```csv
"第一行
第二行"
```

因此 CsvCodec 必须支持 quoted field 内换行。

---

## 5.5 空字段

例如：

```text
lastLoginAt = absent
```

CSV：

```csv
...,,
```

空字段本身是合法 CSV 表达。

但“这个业务字段在当前状态下能否为空”由领域映射层判断。

---

# 6. 什么时候必须加双引号

字段只要包含以下任一字符：

```text
,
"
\r
\n
```

就必须整体双引号包围。

例如：

```text
hello
→ hello

hello,world
→ "hello,world"
```

---

# 7. CsvCodec 解析算法

推荐使用逐字符状态机。

至少维护：

```text
insideQuotes
currentField
fields
```

核心状态：

```text
normal field
quoted field
quote escape
field separator
record end
```

遇到：

```text
""
```

在 quoted field 中解析为一个：

```text
"
```

如果：

```text
quoted field 没有正常闭合
```

则：

```text
CSV syntax error
```

不能擅自修复。

---

# 8. Header 规则

所有 CSV：

> **第一条逻辑记录必须是固定 Header。**

例如：

```csv
accountId,name,passwordAlgorithm,...
```

启动时：

```text
Header 数量
Header 名称
Header 顺序
```

必须与当前 schema 完全一致。

---

# 9. 为什么 Header 要严格匹配

当前没有：

```text
migration framework
字段别名机制
多版本兼容 reader
```

如果允许：

```text
少一列也继续
多一列也忽略
顺序不一致也猜
```

很容易把数据错读到其他字段。

因此：

```text
Header mismatch
→ startup schema failure
```

---

# 10. 空数据文件与 0-byte 文件

合法：

```csv
studentAccountId,postId,likedAt
```

只有 Header，没有 data row。

这表示：

```text
当前 LikeRelation = 0
```

非法：

```text
0-byte file
```

因为：

```text
缺失 Header
无法确认 schema
```

---

# 11. Enum 序列化

所有 `enum class` 使用：

> **稳定英文 token**

例如：

```text
RecordStatus
Pending
Approved
Rejected
Withdrawn
```

```text
AccountStatus
Active
Disabled
```

```text
Diary display status
PendingDisplayReview
Displayed
TakenDown
```

---

# 12. 为什么不用整数 enum 值

禁止：

```text
0
1
2
3
```

作为持久化语义。

因为如果以后 C++ enum 声明顺序调整：

```cpp
enum class RecordStatus {
    Pending,
    Withdrawn,
    Approved,
    Rejected
};
```

旧整数数据可能被误解。

稳定英文 token 不依赖 C++ 内部枚举 ordinal。

---

# 13. 为什么不用中文 UI 文案持久化

不保存：

```text
待审核
审核通过
已驳回
```

因为：

```text
UI 文案可能调整
中文措辞属于 Presentation
持久化格式应该稳定
```

因此统一英文 token。

---

# 14. 未知 enum token

例如：

```text
status = Approvd
```

或：

```text
status = Unknown
```

当前不允许：

```text
fallback Pending
```

而应该：

```text
load failure
```

---

# 15. Bool 序列化

统一：

```text
true
false
```

不使用：

```text
1 / 0
yes / no
启用 / 禁用
```

解析时只接受当前标准 token。

---

# 16. 可空字段

可选值统一：

> **空 CSV 字段表示 absent。**

例如：

```text
lastLoginAt
```

一个从未成功登录的 Student：

```csv
...,2026-09-03T18:00:00,
```

---

# 17. 空字段是否允许由状态决定

CsvCodec 只看到：

```text
""
```

但真正合法性由领域状态判断。

例如：

```text
VolunteerRecord.status = Approved
finalScore = empty
```

必须判定：

```text
invalid persisted domain state
```

而不是“合法空字段”。

---

# 18. Date / DateTime 序列化

沿用 Gate 4.4。

## Date

```text
YYYY-MM-DD
```

例如：

```text
2026-09-03
```

---

## DateTime

```text
YYYY-MM-DDTHH:MM:SS
```

例如：

```text
2026-09-03T18:04:32
```

---

# 19. 非法 Date / DateTime

例如：

```text
2026-02-30
2026-13-01
2026-09-03T25:00:00
```

都必须：

```text
parse failure
```

不自动修正。

---

# 20. double 序列化

当前时长、系数、积分、Badge threshold 均使用 `double`。

建议权威 CSV 使用稳定十进制文本。

对于已冻结两位业务精度的正式值：

```text
1.50
2.00
2.63
```

优点：

```text
人类可读
测试稳定
Git diff 稳定
报告方便截图
```

---

# 21. 整数序列化

例如：

```text
passwordIterations
next_record_seq
```

使用标准十进制：

```text
600000
124
```

不添加：

```text
,
空格
科学计数法
```

---

# 22. students.csv

正式 Header：

```csv
accountId,name,passwordAlgorithm,passwordIterations,passwordSaltHex,passwordHashHex,accountStatus,className,major,contact,createdAt,lastLoginAt
```

字段：

```text
accountId
name
passwordAlgorithm
passwordIterations
passwordSaltHex
passwordHashHex
accountStatus
className
major
contact
createdAt
lastLoginAt
```

说明：

```text
Student 文件本身已经确定动态类型
不额外持久化 role=Student
```

---

# 23. administrators.csv

正式 Header：

```csv
accountId,name,passwordAlgorithm,passwordIterations,passwordSaltHex,passwordHashHex,accountStatus
```

字段：

```text
accountId
name
passwordAlgorithm
passwordIterations
passwordSaltHex
passwordHashHex
accountStatus
```

不为了和 Student 对齐而添加：

```text
className
major
contact
createdAt
lastLoginAt
```

---

# 24. volunteer_records.csv

当前权威 `VolunteerRecord` 完整持久字段共 15 个：

```text
recordId
ownerAccountId
serviceDate
appliedCategoryId
finalCategoryId
appliedDuration
finalDuration
place
verifier
summary
status
reviewerAccountId
reviewNote
settledCoefficient
finalScore
```

为了 CSV 可读性，推荐按：

```text
身份
→ 原始申报事实
→ 当前状态
→ 审核 / 正式结算结果
```

排列。

正式 Header：

```csv
recordId,ownerAccountId,serviceDate,appliedCategoryId,appliedDuration,place,verifier,summary,status,finalCategoryId,finalDuration,reviewerAccountId,reviewNote,settledCoefficient,finalScore
```

---

# 25. VolunteerRecord 不持久化的字段

当前禁止擅自增加：

```text
semesterId
createdAt
submittedAt
reviewedAt
submissionCount
totalScore
diaryPostId
```

因为当前权威 Domain 未冻结这些字段。

其中：

```text
semesterId
```

特别禁止，因为学期归属由：

```text
serviceDate
+
Semester.startDate/endDate
```

动态判断。

---

# 26. VolunteerRecord Pending 字段规则

当：

```text
status = Pending
```

应满足：

```text
finalCategoryId = empty
finalDuration = empty
settledCoefficient = empty
finalScore = empty

reviewerAccountId = empty
reviewNote = empty
```

因为 Pending 尚未形成正式审核结论。

---

# 27. VolunteerRecord Approved 字段规则

当：

```text
status = Approved
```

必须存在：

```text
finalCategoryId
finalDuration
settledCoefficient
finalScore
reviewerAccountId
```

`reviewNote`：

```text
允许为空
```

因为通过审核不一定需要备注。

---

# 28. VolunteerRecord Rejected 字段规则

当：

```text
status = Rejected
```

必须：

```text
reviewerAccountId present
reviewNote present
```

不得存在正式结算：

```text
finalCategoryId
finalDuration
settledCoefficient
finalScore
```

---

# 29. VolunteerRecord Withdrawn 字段规则

当：

```text
status = Withdrawn
```

不得存在正式结算：

```text
finalCategoryId
finalDuration
settledCoefficient
finalScore
```

关于 Withdrawn 是否保留某些当前审核字段，必须继续服从已有状态机语义；Persistence 不自行发明新规则。

---

# 30. volunteer_categories.csv

正式 Header：

```csv
categoryId,name,currentCoefficient,enabled
```

字段：

```text
categoryId
name
currentCoefficient
enabled
```

不保存：

```text
历史积分
历史 recordCount
```

---

# 31. badge_rules.csv

正式 Header：

```csv
badgeRuleId,badgeName,categoryId,bronzeThreshold,silverThreshold,goldThreshold,enabled
```

字段：

```text
badgeRuleId
badgeName
categoryId
bronzeThreshold
silverThreshold
goldThreshold
enabled
```

---

# 32. BadgeRule 加载不变量

启动时至少检查：

```text
categoryId 能解析到 VolunteerCategory
bronzeThreshold > 0
bronzeThreshold < silverThreshold
silverThreshold < goldThreshold
```

并检查：

```text
一个 categoryId 最多绑定一套 BadgeRule
```

---

# 33. semesters.csv

正式 Header：

```csv
semesterId,semesterName,startDate,endDate
```

字段：

```text
semesterId
semesterName
startDate
endDate
```

不添加：

```text
isCurrent
```

当前学期由：

```text
system_config.current_semester_id
```

统一决定。

---

# 34. Semester 基本校验

至少：

```text
semesterId 唯一
startDate <= endDate
```

当前不在 Persistence 层擅自增加“学期时间范围绝不重叠”等尚未冻结业务规则。

---

# 35. diary_posts.csv

正式 Header：

```csv
postId,recordId,title,content,displayStatus,publishedAt
```

字段：

```text
postId
recordId
title
content
displayStatus
publishedAt
```

明确不保存：

```text
publisherAccountId
likeCount
rank
```

---

# 36. publisherAccountId 为什么不保存

发布者通过：

```text
DiaryPost.recordId
↓
VolunteerRecord.ownerAccountId
```

推导。

因此：

```text
publisherAccountId
```

如果再持久化，就会形成重复权威事实。

当前禁止。

---

# 37. DiaryPost publishedAt 状态语义

当前建议：

```text
PendingDisplayReview
→ publishedAt = empty

Displayed
→ publishedAt required

TakenDown
→ 保留原 publishedAt
```

原因：

```text
publishedAt
```

表达的是：

> **正式公开时间**

不是“提交日记申请时间”。

---

# 38. likes.csv

正式 Header：

```csv
studentAccountId,postId,likedAt
```

字段：

```text
studentAccountId
postId
likedAt
```

没有：

```text
likeId
studentName
postTitle
```

---

# 39. LikeRelation 唯一性

逻辑唯一键：

```text
(studentAccountId, postId)
```

启动时必须检查：

```text
不存在重复 composite key
```

否则意味着同一个 Student 对同一个 Post 存在重复点赞事实。

---

# 40. operation_logs.csv

正式 Header：

```csv
logId,operatorAccountId,operationType,targetType,targetId,description,operationTime
```

字段：

```text
logId
operatorAccountId
operationType
targetType
targetId
description
operationTime
```

---

# 41. OperationLog target 是历史审计软引用

OperationLog 的：

```text
targetType
targetId
```

不是运行期强引用。

例如：

```text
目标 VolunteerRecord 已被物理删除
```

旧日志仍必须保留：

```text
targetId = REC000123
```

因此：

```text
targetId 当前找不到对象
```

不等于：

```text
operation_logs.csv 损坏
```

---

# 42. OperationLog operator 也按历史审计信息处理

`operatorAccountId` 作为历史操作事实应持久保留。

即使未来目标账号发生：

```text
禁用
删除 / 不可解析
```

也不能因此删除旧日志。

具体 operator 是否要求在启动时当前存在，按历史审计软引用原则不作为强引用阻塞。

---

# 43. 当前正式 CSV Header 总表

| 文件 | Header |
|---|---|
| `students.csv` | `accountId,name,passwordAlgorithm,passwordIterations,passwordSaltHex,passwordHashHex,accountStatus,className,major,contact,createdAt,lastLoginAt` |
| `administrators.csv` | `accountId,name,passwordAlgorithm,passwordIterations,passwordSaltHex,passwordHashHex,accountStatus` |
| `volunteer_records.csv` | `recordId,ownerAccountId,serviceDate,appliedCategoryId,appliedDuration,place,verifier,summary,status,finalCategoryId,finalDuration,reviewerAccountId,reviewNote,settledCoefficient,finalScore` |
| `volunteer_categories.csv` | `categoryId,name,currentCoefficient,enabled` |
| `badge_rules.csv` | `badgeRuleId,badgeName,categoryId,bronzeThreshold,silverThreshold,goldThreshold,enabled` |
| `semesters.csv` | `semesterId,semesterName,startDate,endDate` |
| `diary_posts.csv` | `postId,recordId,title,content,displayStatus,publishedAt` |
| `likes.csv` | `studentAccountId,postId,likedAt` |
| `operation_logs.csv` | `logId,operatorAccountId,operationType,targetType,targetId,description,operationTime` |

---

# 44. system_config.txt

当前建议使用：

```text
key=value
```

格式。

示例：

```ini
schema_version=1
current_semester_id=SEM000003
next_record_seq=124
next_category_seq=8
next_badge_rule_seq=5
next_semester_seq=7
next_post_seq=63
next_log_seq=418
```

---

# 45. system_config 语法

规则：

```text
一行一个 key=value
key/value 两端 whitespace trim
允许空行
```

当前不支持：

```text
注释语法
section
nested config
```

保持简单。

---

# 46. 必须存在的配置项

至少：

```text
schema_version
current_semester_id
next_record_seq
next_category_seq
next_badge_rule_seq
next_semester_seq
next_post_seq
next_log_seq
```

---

# 47. unknown / duplicate config key

当前严格模式：

```text
未知 key
→ configuration error

重复 key
→ configuration error

缺失 required key
→ configuration error

value parse 失败
→ configuration error
```

不静默忽略。

---

# 48. 为什么 unknown key 不静默忽略

例如误写：

```text
next_record_sq=124
```

如果程序无条件忽略 unknown key，真正的：

```text
next_record_seq
```

可能缺失。

严格失败比悄悄使用默认值更安全。

---

# 49. schema_version

整个：

```text
data/
```

目录只使用一个全局：

```text
schema_version
```

当前：

```text
schema_version=1
```

不为每张 CSV 单独保存版本。

---

# 50. schema_version 加载规则

启动：

```text
read system_config
↓
schema_version == supported?
```

当前只支持：

```text
1
```

如果：

```text
0
2
abc
missing
```

则：

```text
UnsupportedSchemaVersion
```

或等价启动错误。

---

# 51. 当前不实现自动 migration

不做：

```text
v1 自动升级 v2
字段自动补齐
旧 header 自动转换
```

原因：

```text
课程项目体量小
当前不存在历史生产数据迁移需求
复杂 migration framework 没有收益
```

未来如果 schema 真正变化，再单独设计。

---

# 52. 首次初始化与正常启动要区分

## 首次初始化

如果：

```text
data/
```

整个不存在，或者用户明确运行初始化流程：

```text
bootstrap
```

可以创建：

```text
完整目录
完整 Header 文件
初始配置
初始管理员
必要规则数据
```

---

## 正常启动

如果数据目录已经存在，但其中：

```text
某一个权威文件突然缺失
```

不能自动解释为“0 条数据”。

例如：

```text
volunteer_records.csv missing
```

应：

```text
startup failure
```

---

# 53. 为什么不能把 missing file 当 empty dataset

因为用户可能误删：

```text
volunteer_records.csv
```

如果程序自动创建一个空文件：

```text
所有志愿记录看起来合法地消失了
```

这是严重数据丢失掩盖。

因此正常系统缺权威文件必须失败。

---

# 54. 推荐启动加载顺序

当前建议：

```text
1. system_config.txt

2. students.csv
3. administrators.csv

4. volunteer_categories.csv
5. badge_rules.csv

6. semesters.csv

7. volunteer_records.csv

8. diary_posts.csv
9. likes.csv

10. operation_logs.csv
```

总体原则：

> **尽量先加载被强引用对象，再加载引用对象。**

---

# 55. 为什么 OperationLog 最后加载

OperationLog 的：

```text
targetId
```

是历史审计软引用。

它不参与当前业务对象图的强引用建立。

因此最后加载最自然。

---

# 56. 启动验证三层模型

当前启动校验分三层：

```text
Layer 1
Syntax

Layer 2
Entity invariant

Layer 3
Cross-file integrity
```

---

# 57. Layer 1 — Syntax

检查：

```text
CSV 引号合法
Header 合法
列数正确
整数可 parse
double 可 parse
bool token 合法
enum token 合法
Date 合法
DateTime 合法
config key/value 合法
```

这一层不判断跨对象关系。

---

# 58. Layer 2 — Entity Invariant

例如：

```text
stable ID 非空
stable ID 格式正确
同文件 ID 唯一
duration 合法
coefficient 合法
Badge threshold 顺序合法
Semester startDate <= endDate
VolunteerRecord 状态字段组合合法
DiaryPost status/publishedAt 合法
```

---

# 59. Layer 3 — Cross-file Integrity

强引用必须能解析。

至少：

```text
VolunteerRecord.ownerAccountId
→ Student 存在

VolunteerRecord.appliedCategoryId
→ VolunteerCategory 存在

VolunteerRecord.finalCategoryId
→ 如 present，则 Category 存在

BadgeRule.categoryId
→ Category 存在

DiaryPost.recordId
→ VolunteerRecord 存在

LikeRelation.studentAccountId
→ Student 存在

LikeRelation.postId
→ DiaryPost 存在

current_semester_id
→ Semester 存在
```

---

# 60. 强引用失败

例如：

```text
DiaryPost.recordId = REC000999
```

但：

```text
VolunteerRecord 不存在
```

当前属于：

```text
startup integrity failure
```

因为 DiaryPost 无法解释其业务来源。

---

# 61. OperationLog target 不作为强引用

例如：

```text
operation_logs.csv
targetId = REC000003
```

当前 VolunteerRecord 已经被物理删除。

合法：

```text
保留日志
```

不因为：

```text
target object missing
```

导致整个 data directory 无法启动。

---

# 62. 重复 ID 检查

启动至少检查：

```text
Student + Administrator accountId 全局唯一
recordId 唯一
categoryId 唯一
badgeRuleId 唯一
semesterId 唯一
postId 唯一
logId 唯一
```

以及逻辑唯一：

```text
LikeRelation(studentAccountId, postId)
DiaryPost.recordId
BadgeRule.categoryId
```

---

# 63. 重复数据不能“最后一条覆盖前面”

禁止：

```text
读取 unordered_map
重复 key
→ 后一行自动覆盖前一行
```

因为这会掩盖持久化损坏。

重复 stable ID：

```text
→ startup integrity failure
```

---

# 64. 损坏 data row 不静默跳过

例如：

```text
volunteer_records.csv
第 25 条 logical record
finalScore = abc
```

禁止：

```text
warning
skip
继续启动
```

原因：

```text
你已经静默丢失一条权威业务事实
```

正确：

```text
load failure
```

并提供：

```text
file
logical row / record index
field
error reason
```

---

# 65. 多行 quoted field 的行号问题

因为一个 CSV record 可能：

```text
content
```

包含换行，所以：

```text
物理行号
≠ 逻辑记录号
```

实际错误诊断最好能够记录：

```text
physical line start
logical record index
```

精确实现可以后续决定，但不能假设“一行就是一条记录”。

---

# 66. 启动失败时不能进入 Ready

以下任何情况：

```text
unsupported schema
corrupt CSV
invalid enum
invalid Date
invalid domain invariant
duplicate stable ID
broken strong reference
missing required authority file
invalid required config
```

都应：

```text
fail-safe startup
```

不能：

```text
部分 Repository 加载成功
↓
照常进入 Main Menu / MainWindow
```

---

# 67. 与 Gate 4.3 的边界

Gate 4.5：

```text
数据怎样表示
数据怎样验证
```

Gate 4.3：

```text
新数据怎样安全替换旧文件
```

因此：

```text
CsvCodec
```

不要知道：

```text
.tmp
.bak
commit order
```

而：

```text
PersistenceCoordinator
```

也不需要理解：

```text
RecordStatus::Approved
Badge threshold
```

---

# 68. 与 Gate 4.4 的边界

Gate 4.4 冻结：

```text
Date
DateTime
double 小时
Stable ID
next sequence
```

Gate 4.5 只决定：

```text
这些值在文件中如何表示
```

例如：

```text
Date
→ YYYY-MM-DD

double
→ decimal text

next sequence
→ system_config key=value
```

---

# 69. 与 Gate 4.1 的边界

Gate 4.1 冻结密码：

```text
PBKDF2-HMAC-SHA256
salt
iterations
hash
```

Gate 4.5 只负责持久化：

```text
passwordAlgorithm
passwordIterations
passwordSaltHex
passwordHashHex
```

不重新实现密码算法。

---

# 70. 当前禁止实现清单

后续 AI / Codex 不得擅自采用：

```text
split(',') 解析 CSV
CSV 没 Header
Header 名称/顺序随意
enum 持久化为 C++ ordinal
enum 持久化为中文 UI 文案
未知 enum fallback 默认值
非法数值 fallback 0
非法 Date 自动修正
0-byte 文件视为合法空数据
正常运行缺文件时自动创建空文件
损坏 row 静默跳过
重复 ID 后一条覆盖前一条
强引用断裂仍进入 Ready

VolunteerRecord 增加未冻结字段
VolunteerRecord 保存 semesterId
DiaryPost 保存 publisherAccountId
DiaryPost 保存 likeCount
Student 保存 totalScore / rank
LikeRelation 增加 likeId

普通启动自动用 .tmp / .bak 修复
CsvCodec 处理领域业务规则
PersistenceCoordinator 处理 CSV enum / schema 语义
每个 CSV 单独维护 schema version
自动 migration framework
```

除非后续显式修订。

---

# 71. 当前实现级候选决策

## G4-CSV-01 — CsvCodec

> **所有领域 CSV 使用统一 `CsvCodec` 处理 CSV 语法。字段包含逗号、双引号、CR 或 LF 时整体使用双引号包围，字段内部 `"` 编码为 `""`；解析采用逐字符 quoted-field 状态机，禁止简单 `split(',')`。CsvCodec 只负责字符串记录的编码与解析，不负责 Domain 语义校验。**

---

## G4-CSV-02 — Header 与空数据文件

> **所有 CSV 第一条逻辑记录必须是固定 Header，当前 schema 下 Header 名称、数量和顺序严格匹配。只有 Header 而无 data row 的文件表示合法空集合；0-byte 文件视为 schema 不完整并导致加载失败。**

---

## G4-CSV-03 — 基础值序列化

> **`enum class` 持久化为稳定英文 token，bool 持久化为 `true/false`，Date 使用 `YYYY-MM-DD`，DateTime 使用 `YYYY-MM-DDTHH:MM:SS`，整数使用标准十进制文本，double 使用稳定十进制业务格式。未知 enum、非法 bool、非法数值、非法 Date / DateTime 均导致加载失败，不使用默认值兜底。**

---

## G4-CSV-04 — Optional 字段

> **可选字段统一使用空 CSV 字段表示 absent。字段在特定 Domain 状态下是否允许为空，由 Repository mapping / Domain invariant 判断，而不是由 CsvCodec 判断。**

---

## G4-CSV-05 — 只保存权威事实

> **各 CSV 只持久化对象自身权威事实和必要 stable ID，不持久化可动态派生结果。明确禁止持久化 `Student.totalScore / rank / currentBadge`、`DiaryPost.publisherAccountId / likeCount`、`VolunteerRecord.semesterId` 等已由当前架构明确为派生或排除的字段。**

---

## G4-CSV-06 — VolunteerRecord Schema

> **`volunteer_records.csv` 固定 Header 为：`recordId,ownerAccountId,serviceDate,appliedCategoryId,appliedDuration,place,verifier,summary,status,finalCategoryId,finalDuration,reviewerAccountId,reviewNote,settledCoefficient,finalScore`。Persistence 不得擅自增加 `semesterId / createdAt / submittedAt / reviewedAt / submissionCount / totalScore / diaryPostId` 等当前未冻结字段。**

---

## G4-CONFIG-01 — system_config.txt

> **`system_config.txt` 使用严格 `key=value` 格式，每行一个键值对，可有空行但不引入复杂 section / comment 语法。至少保存 `schema_version / current_semester_id / next_record_seq / next_category_seq / next_badge_rule_seq / next_semester_seq / next_post_seq / next_log_seq`。未知 key、重复 key、缺失 required key 或非法 value 均视为配置错误。**

---

## G4-SCHEMA-01 — 全局 schema_version

> **整个 data 目录共用一个 `schema_version`，当前初始版本为 `1`。启动只接受程序明确支持的版本，不为每张 CSV 单独维护 schema version，也不在当前版本实现自动 schema migration framework。**

---

## G4-LOAD-01 — 三层启动校验

> **启动数据校验按“语法与基础解析 → 单对象 invariant → 跨文件强引用完整性”三层执行。任一权威业务记录损坏不得静默跳过，任何关键层失败时应用不得进入 Ready。**

---

## G4-LOAD-02 — 强引用与历史审计软引用

> **业务强引用必须解析成功，例如 VolunteerRecord→Student/Category、DiaryPost→VolunteerRecord、LikeRelation→Student/DiaryPost、BadgeRule→Category、currentSemesterId→Semester。OperationLog 的 operator/target 属于历史审计软引用，目标对象已删除时仍保留稳定 ID，不因此删除日志或判定整个数据目录损坏。**

---

## G4-LOAD-03 — 正常启动缺文件与重复 ID

> **首次 bootstrap 与已有系统正常启动严格区分。已有 data 目录中缺失任一权威文件不得自动创建空文件。所有 stable ID 和业务逻辑唯一键必须在启动时检查重复；发现重复不得使用“后一条覆盖前一条”等静默策略。**

---

# 72. 后续实现待确认事项

以下内容不在本专题提前死锁：

```text
CsvCodec 最终类名 / namespace
parseRecord 是否支持 stream-based logical record
物理行号与逻辑记录号诊断 API
double 精确 format helper
Date / DateTime parse helper 名称
enum token converter 放在哪个模块
Repository row mapper 最终类名
StartupIntegrityChecker 是否独立成类
bootstrap 工具最终入口
错误信息是否携带 file/row/field
current_semester_id 为空时是否允许无当前学期
最终所有 OperationType / OperationTargetType token 集合
```

这些需结合：

```text
真实 C++ 代码
Gate 4.6 Result / Error
最终 Gate 1 业务规则
```

再确定。

---

# 73. 给后续 AI / Codex 的最短读取指令

如果上下文空间不足，只需读取本节。

```text
当前 CSV / Config / Startup Load 路线：

1. 所有 CSV 用统一 CsvCodec。
2. 禁止 split(',')。
3. CsvCodec 支持：
   - comma
   - quote
   - CR/LF
   - empty field
   - quoted multiline field
4. 特殊字段双引号包围，内部 " → ""。
5. CsvCodec 只负责 syntax，不负责 Domain 校验。

6. 每个 CSV 第一条逻辑记录必须是固定 Header。
7. Header 名称、数量、顺序严格匹配。
8. Header-only = 合法空集合。
9. 0-byte = 错误。

10. enum → stable English token。
11. bool → true/false。
12. Date → YYYY-MM-DD。
13. DateTime → YYYY-MM-DDTHH:MM:SS。
14. optional → empty field。
15. unknown enum / invalid number / invalid date → load failure。

16. 正式 Headers：
students.csv:
accountId,name,passwordAlgorithm,passwordIterations,passwordSaltHex,passwordHashHex,accountStatus,className,major,contact,createdAt,lastLoginAt

administrators.csv:
accountId,name,passwordAlgorithm,passwordIterations,passwordSaltHex,passwordHashHex,accountStatus

volunteer_records.csv:
recordId,ownerAccountId,serviceDate,appliedCategoryId,appliedDuration,place,verifier,summary,status,finalCategoryId,finalDuration,reviewerAccountId,reviewNote,settledCoefficient,finalScore

volunteer_categories.csv:
categoryId,name,currentCoefficient,enabled

badge_rules.csv:
badgeRuleId,badgeName,categoryId,bronzeThreshold,silverThreshold,goldThreshold,enabled

semesters.csv:
semesterId,semesterName,startDate,endDate

diary_posts.csv:
postId,recordId,title,content,displayStatus,publishedAt

likes.csv:
studentAccountId,postId,likedAt

operation_logs.csv:
logId,operatorAccountId,operationType,targetType,targetId,description,operationTime

17. 不保存派生字段：
   Student.totalScore/rank/badge
   DiaryPost.publisherAccountId/likeCount
   VolunteerRecord.semesterId

18. system_config.txt:
   key=value
   required:
   schema_version
   current_semester_id
   next_record_seq
   next_category_seq
   next_badge_rule_seq
   next_semester_seq
   next_post_seq
   next_log_seq

19. schema_version 当前 = 1。
20. 不实现自动 migration。

21. 启动验证三层：
   syntax
   → entity invariant
   → cross-file integrity

22. 权威数据行损坏不得 skip。
23. 强引用必须解析。
24. OperationLog target/operator 是历史审计软引用，可保留已删除对象 ID。
25. 已有系统缺权威文件 → startup failure，不自动创建空文件。
26. stable ID / composite unique key 重复 → startup failure。
```

---

# 74. 当前状态建议

```text
Gate 4.5
CSV Schema、CsvCodec、字段验证与 schema_version

G4-CSV-01 ～ G4-CSV-06
G4-CONFIG-01
G4-SCHEMA-01
G4-LOAD-01 ～ G4-LOAD-03

→ CANDIDATE FROZEN
```

在实际 CsvCodec、Repository row mapping、system_config parser、bootstrap 和启动完整性测试完成后，再进行 Gate 4.5 FINAL AUDIT。
