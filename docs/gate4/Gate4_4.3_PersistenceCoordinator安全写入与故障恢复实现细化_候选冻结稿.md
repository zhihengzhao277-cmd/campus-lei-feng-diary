# Gate 4.3 — PersistenceCoordinator、安全写入、恢复与严重故障处理实现细化（候选冻结稿）

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 4 —— 实现审计  
> 前置依赖：
> - Gate 3.3：Application / Service 业务协调边界
> - Gate 3.4：Repository / Persistence、运行期内存权威集合、Prepare / Commit、`.tmp / .bak`、PersistenceCoordinator
> - Gate 3.5：严重持久化故障必须在 Presentation 层被正确区分
> - Gate 3.9：V0.1 起即启用最终持久化架构，V0.2 负责完整持久化稳定性验证
> - Gate 4.2：Repository 以 `std::vector<T>` 作为权威运行时内存结构
>
> 文档性质：实现级候选冻结稿  
> 当前主题：单文件安全写入、多 Repository Prepare / Commit、操作前快照、`.tmp / .bak` 生命周期、Partial Commit、SeverePartialCommit、OperationLog 同步提交、启动 Recovery 边界  
> 目标：让后续 AI、Codex、开发者或审计者在不依赖本次聊天上下文的情况下，能够准确理解本项目运行期持久化提交的完整协议、失败边界和恢复策略。
>
> 重要说明：
>
> 1. 本文件不把 PersistenceCoordinator 扩展成数据库事务管理器、Unit of Work 或 WAL 系统。
> 2. 当前目标不是实现 ACID，而是在“本地 CSV + 多文件”约束下，以尽量简单的方式避免明显的数据破坏，并在无法保证一致性时 fail-safe 停止。
> 3. Gate 3 已经冻结 `.tmp / .bak`、Prepare / Commit、Partial Commit、OperationLog 同步提交等总体方向；本文件只把这些规则细化成可编码协议。
> 4. 本文件描述的是运行期修改提交。启动 Load、schema 检查、CSV 解析和跨文件完整性验证属于另一条启动路径，不由 PersistenceCoordinator 直接负责。
> 5. 当前为候选冻结稿。实际代码中的类名、方法名、返回结构可在 Gate 4 实现时轻微调整，但不得改变本文冻结的提交语义与故障边界。

---

# 1. 本专题解决什么问题

当前系统不是单文件程序。

一次完整业务操作可能同时影响多个 Repository 和多个物理文件。

例如管理员物理删除一条已审核通过的志愿记录，可能涉及：

```text
VolunteerRecordRepository
↓
volunteer_records.csv

DiaryRepository
↓
diary_posts.csv
likes.csv

OperationLogRepository
↓
operation_logs.csv
```

如果简单写成：

```text
save volunteer_records.csv
↓
save diary_posts.csv
↓
save likes.csv
↓
save operation_logs.csv
```

就存在：

```text
前几个文件已经成功
后一个文件失败
```

导致：

```text
部分文件是新状态
部分文件仍是旧状态
```

这就是：

> **Partial Commit（部分提交）**

因此 Gate 4.3 的目标不是追求数据库级事务，而是明确：

```text
什么时候可以安全回滚？
什么时候不能继续运行？
.tmp / .bak 如何使用？
Service 什么时候才可以返回 Success？
```

---

# 2. 当前持久化总体模型

系统运行模型继续保持：

```text
程序启动
↓
Repository 从文件加载
↓
形成运行期权威内存集合
↓
Service 执行业务
↓
Domain / Repository 内存状态发生合法变化
↓
PersistenceCoordinator 协调持久化
↓
持久化完整成功
↓
Service 才可以返回 Success
```

关键原则：

> **“内存修改成功”不等于“业务提交成功”。**

只有：

```text
所有要求落盘的数据都完成持久化
```

本次修改型业务才算真正成功。

---

# 3. 正式文件不能直接 truncate

当前禁止：

```cpp
std::ofstream file(path, std::ios::trunc);
```

直接打开权威正式文件后逐行重写。

原因：

一旦在写入过程中出现：

```text
程序崩溃
磁盘写错误
进程被结束
异常退出
设备故障
```

正式文件可能已经被截断，只留下半份数据。

因此当前所有权威文件统一采用：

> **先写临时文件，再替换正式文件。**

---

# 4. 单文件安全写入协议

假设目标文件：

```text
volunteer_records.csv
```

当前推荐完整协议：

```text
Step 1
从 Repository 当前权威内存集合
序列化完整新文件内容

Step 2
写入同目录：
volunteer_records.csv.tmp

Step 3
完成写入
flush
close

Step 4
确认 tmp 写入成功
必要的最低格式验证通过

Step 5
如果旧正式文件存在：
volunteer_records.csv
→ volunteer_records.csv.bak

Step 6
volunteer_records.csv.tmp
→ volunteer_records.csv

Step 7
确认新的正式文件已经成功建立

Step 8
提交完全成功后
清理旧 .bak
```

核心不变量：

```text
新文件没有完整准备好之前
旧正式文件不动
```

---

# 5. `.tmp` 的定位

`.tmp` 表示：

> **已经序列化、但尚未成为权威正式文件的候选新版本。**

例如：

```text
volunteer_records.csv
→ 仍然是当前旧权威文件

volunteer_records.csv.tmp
→ 本次提交准备的新版本
```

`.tmp` 不属于业务数据。

也不属于：

```text
版本历史
永久备份
用户可直接操作的数据文件
```

它只服务于一次持久化提交。

---

# 6. `.bak` 的定位

`.bak` 只承担：

> **一次文件替换过程中的旧正式文件保护。**

例如：

```text
volunteer_records.csv
↓ rename
volunteer_records.csv.bak

volunteer_records.csv.tmp
↓ rename
volunteer_records.csv
```

如果第二次替换失败：

```text
volunteer_records.csv.bak
```

至少仍保留旧正式内容。

当前明确：

```text
.bak
≠ 自动历史版本
≠ 多代备份系统
≠ 数据库 rollback log
```

它只是：

```text
single recovery candidate
```

---

# 7. 为什么 `.tmp` 和 `.bak` 要放同目录

推荐：

```text
data/volunteer_records.csv
data/volunteer_records.csv.tmp
data/volunteer_records.csv.bak
```

而不是把 tmp 写到其他磁盘或其他目录。

主要原因：

```text
文件替换路径更简单
同一存储环境下行为更可控
减少跨盘 rename / move 差异
更容易检查和清理遗留文件
```

---

# 8. Prepare 与 Commit 的区别

当前多文件提交明确分成两个阶段。

## 8.1 Prepare

Prepare 只做：

```text
把当前新权威内存状态
完整序列化为对应 .tmp 文件
```

但：

```text
不替换任何正式文件
```

Prepare 成功意味着：

> “这个 Repository 的新文件候选已经准备好了。”

---

## 8.2 Commit

Commit 才真正：

```text
旧正式文件 → .bak
.tmp → 正式文件
```

因此：

```text
Prepare
= prepare candidate files

Commit
= change authoritative files
```

这个边界必须非常清楚。

---

# 9. 为什么必须 Prepare All Before Commit

假设一次业务影响：

```text
volunteer_records.csv
diary_posts.csv
likes.csv
operation_logs.csv
```

如果边 Prepare 边 Commit：

```text
prepare records
commit records
prepare posts
commit posts
prepare likes → 失败
```

就已经发生 Partial Commit。

因此统一要求：

```text
prepare records
prepare posts
prepare likes
prepare logs
↓
全部成功？
```

只有：

```text
全部成功
```

才允许开始：

```text
任何正式文件 Commit
```

这就是：

> **Prepare All Before Commit**

---

# 10. 修改前必须建立内存快照

修改型业务不能先改完内存再考虑失败怎么办。

当前要求：

> **第一次权威内存修改发生之前，就必须建立本次受影响 Repository 的操作前快照。**

例如物理删除 VolunteerRecord：

```text
受影响：
VolunteerRecordRepository
DiaryRepository
OperationLogRepository
```

则在任何 mutation 之前保存：

```text
oldRecords
oldPosts
oldLikes
oldLogs
```

这里的 snapshot 是：

```text
独立值语义副本
```

不是：

```text
指针列表
iterator 列表
裸地址
```

原因是：

```text
原 vector 之后可能 erase / reallocate
```

只有独立副本才能可靠恢复。

---

# 11. 只快照受影响 Repository

不需要每次把全系统所有 Repository 全部复制一遍。

例如只修改：

```text
Student.contact
```

那么只需要快照：

```text
UserRepository
```

不需要快照：

```text
RuleRepository
SemesterRepository
DiaryRepository
OperationLogRepository
```

除非本业务实际修改它们。

原则：

> **affected repositories only**

---

# 12. 标准修改型业务流程

所有修改型 Service 推荐遵循：

```text
1. validate
2. authorize
3. determine affected repositories
4. create snapshots before mutation
5. mutate Domain / Repository memory
6. append OperationLog if required
7. Prepare ALL affected storage
8. if Prepare failed:
       restore snapshots
       cleanup temporary artifacts
       return PersistenceFailure
9. Commit in deterministic order
10. if all Commit success:
       cleanup obsolete backup artifacts
       return Success
11. if Commit partially failed:
       enter severe persistence state
       return SeverePartialCommit
```

---

# 13. Prepare 失败为什么可以安全恢复

假设：

```text
records.tmp ✓
posts.tmp   ✓
likes.tmp   ✗
logs.tmp    尚未
```

此时：

```text
正式 records.csv
正式 posts.csv
正式 likes.csv
正式 logs.csv
```

全部仍然是操作前版本。

所以：

```text
磁盘权威状态
= 旧状态
```

此时只需要：

```text
恢复内存 snapshots
删除 / 清理本次 .tmp
```

即可让：

```text
内存
+
磁盘
```

重新一致。

因此：

```text
Prepare failure
```

属于：

> **普通 PersistenceFailure**

不是 SeverePartialCommit。

---

# 14. Prepare 失败后的处理

统一：

```text
Prepare failed
↓
禁止进入 Commit
↓
恢复受影响 Repository 的内存快照
↓
清理本次已生成 .tmp
↓
保留正式文件
↓
返回 PersistenceFailure
```

此后应用：

```text
可以继续运行
```

前提是：

```text
snapshot restore 完成
正式文件仍保持原状态
```

---

# 15. Commit 的确定顺序

多文件 Commit 不允许由每个 Service 临时决定顺序。

PersistenceCoordinator 应拥有：

> **统一 deterministic commit order**

本次只提交参与该业务的文件，但顺序按照全局次序的相对顺序执行。

例如本次参与：

```text
volunteer_records.csv
diary_posts.csv
likes.csv
operation_logs.csv
```

则顺序固定为：

```text
volunteer_records.csv
↓
diary_posts.csv
↓
likes.csv
↓
operation_logs.csv
```

当前最重要的规则：

> **OperationLog 原则上最后 Commit。**

---

# 16. 为什么 OperationLog 放最后

OperationLog 是：

```text
已发生管理员关键操作的审计事实
```

如果：

```text
operation_logs.csv
```

先提交成功，而主业务文件后失败，就可能形成：

```text
日志说操作已经发生
但业务事实实际上没有完整提交
```

因此业务权威数据应先提交。

只有主业务事实已经全部完成时，OperationLog 才最后提交。

---

# 17. OperationLog 仍属于同一提交单元

“日志最后提交”不等于：

```text
业务成功
↓
之后随便写日志
```

当前冻结：

> **需要记录日志的业务，OperationLog 必须与主业务一起 Prepare，并属于同一持久化提交单元。**

即：

```text
Prepare main data
Prepare operation log
↓
all prepare success
↓
Commit main data
↓
Commit operation log last
```

Service 不能先：

```text
return Success
```

再异步 / 延迟写 OperationLog。

---

# 18. OperationLogService 不独立 Commit

如果：

```text
VolunteerReviewService
```

在审核、删除、更正过程中调用：

```text
OperationLogService
```

OperationLogService 只负责：

```text
构造 / 添加日志对象到 OperationLogRepository 内存
```

不能自行：

```text
commit operation_logs.csv
```

否则会破坏主业务的统一持久化边界。

完整提交由：

```text
主流程 Service
+
PersistenceCoordinator
```

统一控制。

---

# 19. Partial Commit 是什么

假设 Commit 顺序：

```text
records
posts
likes
logs
```

执行：

```text
records ✓
posts   ✓
likes   ✗
logs    未执行
```

此时：

```text
records.csv = 新
posts.csv   = 新
likes.csv   = 旧
logs.csv    = 旧
```

磁盘已经混合。

这就是：

> **Partial Commit**

---

# 20. 为什么 Partial Commit 不能只恢复内存

如果此时简单：

```text
records_ = oldRecords
posts_ = oldPosts
likes_ = oldLikes
logs_ = oldLogs
```

那么：

```text
内存 = 旧状态
磁盘 = 部分新 + 部分旧
```

反而造成：

```text
运行期内存权威状态
和
磁盘持久化状态
完全不一致
```

因此：

> **一旦正式文件已经部分 Commit，普通内存 rollback 不再足够。**

---

# 21. 为什么不实现复杂自动跨文件 rollback

理论上可以尝试：

```text
records.bak → records
posts.bak → posts
```

但如果：

```text
恢复 records 成功
恢复 posts 失败
```

系统仍然是混合状态。

继续扩大就会进入：

```text
transaction journal
WAL
commit log
recovery log
atomic database transaction
```

这些已经超出本课程项目复杂度。

因此当前明确：

> **不伪装成数据库事务。**

---

# 22. SeverePartialCommit

一旦：

```text
至少一个正式文件已成功替换
+
后续某个 Commit 失败
```

统一认定：

```text
SeverePartialCommit
```

这是严重持久化故障。

它不同于：

```text
普通 PersistenceFailure
```

---

# 23. SeverePartialCommit 后的运行策略

当前推荐最保守策略：

```text
检测 SeverePartialCommit
↓
PersistenceCoordinator / Application 标记：
persistence state = fatal
↓
禁止后续普通写操作
↓
Presentation 显示严重持久化错误
↓
要求安全退出 / 重启
```

当前不继续允许：

```text
审核
提交
点赞
配置修改
密码修改
其他状态改变
```

---

# 24. 是否允许 SeverePartialCommit 后继续只读

技术上可能继续展示部分当前内存数据。

但当前课程项目不建议把它设计成复杂的：

```text
read-only degraded mode
```

原因：

```text
当前磁盘状态已无法完全确认
内存与磁盘也可能存在语义差异
继续提供普通业务容易让用户误认为系统仍可靠
```

因此当前更简单可靠：

> **进入 fatal persistence state 后停止正常业务流程，并引导退出 / 重启。**

---

# 25. SeverePartialCommit 与 ServiceError

Gate 3 已经冻结：

```text
ServiceError
```

需要至少能够区分：

```text
PersistenceFailure
SeverePartialCommit
```

语义：

```text
PersistenceFailure
→ 本次操作未提交
→ 已恢复操作前状态
→ 系统仍可继续

SeverePartialCommit
→ 持久化状态可能已经混合
→ 不得继续普通业务
```

这两个错误不能合并成同一个：

```text
SaveFailed
```

---

# 26. `.bak` 在 SeverePartialCommit 中的作用

`.bak` 可以作为：

```text
人工恢复候选
专门 Recovery 流程的依据
```

但当前普通运行期不自动决定：

```text
应该恢复哪些 .bak
哪些新正式文件应该保留
```

原因：

多文件可能处于不同提交阶段。

所以：

```text
.bak
```

不是：

```text
自动跨文件 rollback guarantee
```

---

# 27. 启动时残留 `.tmp`

程序崩溃后可能留下：

```text
records.csv.tmp
```

当前原则：

> **残留 `.tmp` 不自动提升为正式文件。**

因为系统不知道：

```text
它是否只是 Prepare 成功后程序崩溃
它是否已经完整
它是否对应一次已经部分 Commit 的操作
```

因此普通启动不能：

```text
if tmp exists:
    replace official
```

---

# 28. 启动时残留 `.bak`

同样：

> **普通启动不自动以 `.bak` 覆盖正式文件。**

因为正式文件可能已经是：

```text
完整且更新的新版本
```

如果无条件恢复 bak，就会人为回退已成功数据。

所以 `.bak` 只作为：

```text
Recovery candidate
```

而不是普通启动 fallback。

---

# 29. 启动 Load 与运行期 PersistenceCoordinator 分离

PersistenceCoordinator 只负责：

```text
正常运行期
修改后的 Prepare / Commit
```

不负责：

```text
应用启动
schema_version
CSV parse
duplicate ID
foreign reference
.tmp / .bak 自动裁决
```

启动路径：

```text
Files
↓
load / parse
↓
schema check
↓
construct Domain
↓
cross-file integrity validation
↓
Ready
```

如果失败：

```text
fail-safe startup
```

而不是让 PersistenceCoordinator 自动猜测。

---

# 30. 启动时正式数据检查

启动进入 Ready 前，至少要保证：

```text
schema 支持
CSV 可解析
必要字段合法
stable ID 不重复
强关联可解析
currentSemesterId 指向存在 Semester
关键状态不违反不变量
```

具体字段 schema 与 CSV 行规则将在 Gate 4.5 冻结。

---

# 31. `.tmp / .bak` 清理原则

正常成功提交后：

```text
.tmp
→ 应已转为正式文件

旧 .bak
→ 可清理
```

Prepare 失败：

```text
本次已生成 .tmp
→ 清理
```

SeverePartialCommit：

```text
不要激进清理 .bak
```

因为它们可能用于后续人工恢复分析。

---

# 32. 单 Repository 修改也必须统一协议

例如：

```text
Student 修改 contact
```

只影响：

```text
students.csv
```

也应：

```text
snapshot
↓
mutate memory
↓
prepare students.tmp
↓
commit
↓
Success
```

而不是 Service 直接：

```cpp
userRepository.saveAll();
```

旁路统一协议。

原因：

```text
单文件现在简单
未来可能增加 OperationLog
统一协议更容易测试和解释
```

---

# 33. Service 不直接处理文件路径

Service 只表达：

```text
本次哪些 Repository 被修改
```

不应该知道：

```text
data/volunteer_records.csv
.tmp
.bak
rename
ofstream
```

这些属于：

```text
Repository / Persistence technology
```

PersistenceCoordinator 负责：

```text
协调提交顺序
```

而不是业务文件内容。

---

# 34. Repository 在 Prepare 中负责什么

每个 Repository / 对应 Persistence 组件负责：

```text
读取自己的权威内存集合
↓
转换为字段集合
↓
通过 CsvCodec 序列化
↓
生成自己的 .tmp
```

它知道：

```text
自己的文件
自己的 schema
自己的对象映射
```

它不知道：

```text
整个业务为什么改
哪个管理员发起
Permission
排行榜
状态机
```

---

# 35. PersistenceCoordinator 负责什么

只负责：

```text
接收 affected persistence participants
↓
调用所有 Prepare
↓
判断是否全部成功
↓
按确定顺序 Commit
↓
识别：
Success
Prepare Failure
Partial Commit
```

它不负责：

```text
Domain 状态修改
Permission
OperationLog 内容
CSV 字段语义
启动数据恢复
排行榜
```

---

# 36. PersistenceCoordinator 不是 Unit of Work

当前不具备：

```text
自动 dirty tracking
Identity Map
实体生命周期自动追踪
通用 change set
数据库 ACID
```

Service 明确知道：

```text
哪些 Repository 被改了
```

然后将这些参与者交给 Coordinator。

因此报告应准确称为：

> **轻量多文件持久化提交协调机制**

而不是：

```text
数据库事务
Unit of Work
Transaction Manager
```

---

# 37. 快照恢复失败怎么办

正常情况下，snapshot 是内存值副本，赋回 Repository 应属于可控操作。

如果连：

```text
snapshot restore
```

都因为程序内部异常而失败，说明系统已经不再满足正常运行假设。

这种情况不应映射成普通：

```text
PersistenceFailure
```

应进入：

```text
fatal / invariant broken
```

具体异常实现留 Gate 4.6。

---

# 38. Commit 顺序是否冻结完整文件表

当前不建议在本文件过早死锁：

```text
全部 10 个文件的绝对顺序
```

因为不同 Repository 可能内部映射多个文件，Gate 4.5 还要细化 schema / 文件写入参与者。

当前冻结的是：

```text
存在统一 deterministic order
Service 不自行决定
业务权威数据优先
OperationLog 原则上最后
```

最终完整排序表可在 Gate 4.5 文件映射固定后补充。

---

# 39. 典型流程一：学生修改个人信息

```text
UserManagementService
↓
validate / authorize
↓
affected:
UserRepository
↓
snapshot UserRepository
↓
修改 Student
↓
Prepare students.csv.tmp
↓
Prepare success
↓
Commit students.csv
↓
Success
```

如果 Prepare 失败：

```text
restore UserRepository snapshot
↓
PersistenceFailure
```

---

# 40. 典型流程二：管理员审核志愿记录

可能影响：

```text
VolunteerRecordRepository
OperationLogRepository
```

流程：

```text
VolunteerReviewService
↓
validate / authorize
↓
snapshot records + logs
↓
VolunteerRecord approve/reject
↓
append OperationLog
↓
Prepare volunteer_records.tmp
Prepare operation_logs.tmp
↓
ALL success
↓
Commit volunteer_records
↓
Commit operation_logs
↓
Success
```

---

# 41. 典型流程三：物理删除 Approved VolunteerRecord

可能影响：

```text
VolunteerRecordRepository
DiaryRepository
OperationLogRepository
```

物理文件：

```text
volunteer_records.csv
diary_posts.csv
likes.csv
operation_logs.csv
```

流程：

```text
VolunteerReviewService
↓
validate / authorize
↓
snapshot:
records
posts
likes
logs
↓
remove VolunteerRecord
↓
remove linked DiaryPost
↓
remove linked LikeRelation
↓
append OperationLog
↓
Prepare ALL:
records.tmp
posts.tmp
likes.tmp
logs.tmp
↓
全部成功
↓
Commit:
records
posts
likes
logs
↓
全部成功
↓
Success
```

---

# 42. 典型流程四：Prepare 失败

```text
records.tmp ✓
posts.tmp ✓
likes.tmp ✗
```

处理：

```text
NO COMMIT
↓
restore all affected snapshots
↓
cleanup prepared tmp files
↓
return PersistenceFailure
```

系统：

```text
可继续运行
```

---

# 43. 典型流程五：Partial Commit

```text
records commit ✓
posts commit ✓
likes commit ✗
```

处理：

```text
DO NOT pretend rollback succeeded
↓
mark fatal persistence state
↓
return SeverePartialCommit
↓
disable normal write operations
↓
show severe error
↓
safe exit / restart
```

---

# 44. Presentation 的职责

Console / Qt 只消费：

```text
OperationResult
Result<T>
ServiceError
```

普通 PersistenceFailure：

```text
提示保存失败
本次操作未提交
系统仍可继续
```

SeverePartialCommit：

```text
明确严重持久化故障
禁止继续普通写业务
提示安全退出 / 重启
```

Presentation 不自行：

```text
rename bak
restore tmp
修改 Repository
```

---

# 45. 测试要求

Gate 4.3 编码后至少测试：

```text
单文件 Prepare 成功 + Commit 成功
单文件 Prepare 失败
多文件全部 Prepare 成功
多文件某个 Prepare 失败
第一文件 Commit 失败
前几个 Commit 成功后后续失败
OperationLog Prepare 失败
OperationLog Commit 失败
.tmp 遗留
.bak 遗留
重启后正式文件合法
snapshot restore 后内存一致
SeverePartialCommit 后禁止继续写
```

---

# 46. 当前禁止实现清单

后续 AI / Codex 不得擅自采用：

```text
直接 truncate 正式文件
边 Prepare 边 Commit
任一 Prepare 失败后继续 Commit
Commit 部分失败后只恢复内存继续运行
自动把 .tmp 提升为正式文件
普通启动自动用 .bak 回退
OperationLog 独立提前 Commit
Service 直接操作 .tmp / .bak
Service 自己决定文件提交顺序
PersistenceCoordinator 承担业务规则
PersistenceCoordinator 承担启动 Load
把 PersistenceCoordinator 宣称为 Unit of Work
假装当前 CSV 多文件具有 ACID
```

除非后续显式修订。

---

# 47. 当前实现级候选决策

## G4-PERSIST-01 — 单文件安全写入

> **所有权威持久文件禁止直接 truncate 正式文件。Repository / Persistence 将当前完整权威集合序列化到同目录 `.tmp` 文件，确认写入、flush 和关闭成功后才进入 Commit；Commit 使用单份 `.bak` 保护旧正式文件，成功替换后清理 `.bak`。`.tmp / .bak` 仅为提交与恢复辅助文件，不属于业务历史版本。**

---

## G4-PERSIST-02 — 操作前快照

> **修改型业务在第一次权威内存变化之前，必须确定本次受影响 Repository，并为这些 Repository 建立完整独立值语义操作前快照。未受影响 Repository 不快照。**

---

## G4-PERSIST-03 — Prepare All Before Commit

> **所有受影响 Repository / 物理文件必须先全部 Prepare 成功，才允许开始任何正式文件 Commit。任一 Prepare 失败时正式文件保持旧状态，恢复内存快照、清理本次临时文件，并返回普通 `PersistenceFailure`。**

---

## G4-PERSIST-04 — 固定 Commit 顺序

> **PersistenceCoordinator 采用统一确定的全局提交次序；本次参与提交的文件按该次序的相对顺序 Commit。业务权威数据文件优先，`operation_logs.csv` 原则上最后提交。Service 不自行决定文件顺序。最终完整文件优先级表可在 Gate 4.5 文件 schema 与物理映射全部冻结后补充。**

---

## G4-PERSIST-05 — Partial Commit

> **一旦至少一个正式文件已经完成替换，而后续文件 Commit 失败，即认定为 `SeverePartialCommit`。不得仅恢复内存快照并继续正常业务，也不实现复杂自动跨文件事务回滚；应用进入严重持久化故障状态，禁止后续普通写操作，并引导安全退出 / 重启。**

---

## G4-PERSIST-06 — OperationLog 同步提交

> **需要生成 OperationLog 的业务，日志与主业务状态属于同一提交单元；OperationLog 必须参与同一轮 Prepare，并原则上最后 Commit。OperationLogService 作为子流程时不得独立提交物理文件。**

---

## G4-PERSIST-07 — 启动 Recovery 分离

> **PersistenceCoordinator 只处理正常运行期提交，不参与启动 Load。启动时不自动将残留 `.tmp` 提升为正式文件，也不自动以 `.bak` 回退；正式数据必须经过 schema、解析、对象合法性和强引用完整性检查后才能进入 Ready，异常时 fail-safe 停止并进入人工 / 专门恢复路径。**

---

## G4-PERSIST-08 — 单 Repository 不走旁路

> **即使一次业务只修改一个 Repository，也继续采用 snapshot → memory mutation → Prepare → Commit → Success 的统一协议，不允许 Service 直接调用绕过 PersistenceCoordinator 语义的即时 `saveAll()` 旁路。**

---

# 48. 后续 Gate 4 实现待确认事项

以下内容当前不在本专题死锁：

```text
PersistenceCoordinator 最终 C++ 类签名
PrepareParticipant / CommitParticipant 是否独立抽象
Repository prepare() / commit() 的最终函数名
snapshot 的最终 C++ 保存形式
.tmp / .bak rename 的平台 API
Windows 下文件替换具体调用
文件 fsync / FlushFileBuffers 是否需要
commit 失败后的 exact diagnostic 信息
fatal persistence state 存放位置
应用安全退出的具体 UI
启动 Recovery 工具是否单独提供
最终全文件 deterministic order 表
```

这些事项需要结合：

```text
实际 C++17 / C++20 标准
Windows 文件 API
最终密码库 / Qt / CMake 环境
Gate 4.5 schema
真实代码结构
```

再确定。

---

# 49. 给后续 AI / Codex 的最短读取指令

如果上下文空间不足，只需读取本节。

```text
当前运行期持久化协议：

1. Repository 内存集合是运行期权威状态。
2. 修改型业务只有持久化成功后才能返回 Success。
3. 禁止直接 truncate 正式文件。
4. 单文件：
   - serialize full state
   - write same-directory .tmp
   - flush/close
   - Commit 时 old official → .bak
   - .tmp → official
   - success 后清理 .bak

5. 多文件：
   - 先确定 affected repositories
   - 第一次内存 mutation 前建立完整值语义 snapshots
   - 修改内存
   - 如果需要则添加 OperationLog
   - Prepare ALL
   - 只有全部 Prepare 成功才允许 Commit

6. 任一 Prepare 失败：
   - 不 Commit
   - 恢复内存 snapshot
   - 清理本次 tmp
   - 返回 PersistenceFailure
   - 系统可继续

7. Commit：
   - 使用统一 deterministic order
   - 业务权威数据优先
   - OperationLog 原则上最后
   - Service 不自行决定顺序

8. 如果至少一个正式文件已 Commit，后续 Commit 失败：
   - SeverePartialCommit
   - 不允许仅恢复内存后继续
   - 不实现复杂自动跨文件 rollback
   - 进入 fatal persistence state
   - 禁止后续普通写操作
   - 引导安全退出 / 重启

9. OperationLog：
   - 与主业务同一提交单元
   - 一起 Prepare
   - 原则上最后 Commit
   - OperationLogService 不独立提交

10. .tmp / .bak：
   - 都不是业务历史版本
   - 启动时不自动提升 tmp
   - 启动时不自动 fallback bak
   - 只作为提交辅助 / recovery candidate

11. PersistenceCoordinator：
   - 只负责运行期多文件 Prepare / Commit 协调
   - 不负责业务规则
   - 不负责 CSV 字段语义
   - 不负责启动 Load
   - 不是 Unit of Work / ACID transaction manager

12. 单 Repository 修改也使用同一协议，不走 saveAll() 旁路。
```

---

# 50. 当前状态建议

```text
Gate 4.3
PersistenceCoordinator、安全写入、恢复与严重故障处理

G4-PERSIST-01 ～ G4-PERSIST-08
→ CANDIDATE FROZEN
```

在实际 PersistenceCoordinator、Repository prepare/commit、故障注入测试和 V0.2 持久化回归测试完成后，再进行 Gate 4.3 FINAL AUDIT。
