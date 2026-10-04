# Gate 3.4 — Repository 与 Persistence 架构

> 状态：**FINAL PASS**。
>
> 下方正文逐行迁移自 v2.4 第 4653～6847 行；保留持久化模型、文件格式、安全写入、恢复、架构图和终审内容。D92～D176 均保留在原上下文中。

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

# 36. Gate 3.4：Repository / Persistence 总体定位

Gate 3.3 已经冻结：

```text
Qt Presentation
        ↓
Application / Service
        ↓
Domain
        ↓
Repository / Persistence
```

Gate 3.4 进一步解决：

> **Service 已经知道“要完成什么业务”，Repository / Persistence 必须回答“权威业务对象从哪里恢复、怎样被统一访问、怎样安全持久化、文件损坏时怎样阻止错误状态继续运行”。**

Repository 不是“文件类”的同义词，也不是新的业务逻辑层。

当前边界冻结为：

```text
Service
→ 业务流程、权限、跨对象协调

Domain
→ 对象自身状态与不变量

Repository
→ 领域对象集合的统一数据访问边界

Persistence 技术组件
→ CSV 编解码、配置文件、临时文件、安全替换等底层机制
```

---

## 36.1 为什么 V1.0 需要 Repository

如果没有 Repository，各业务 Service 将直接知道：

- 文件路径；
- CSV 字段顺序；
- 分隔、转义和编码规则；
- 文件打开、关闭和重写方式；
- 数据从哪一个文件恢复。

最终会形成：

```text
AuthenticationService ──直接读 users 文件
StatisticsService     ──直接读 records 文件
RankingService        ──再次读 records 文件
DiaryService          ──直接读 posts / likes 文件
```

这会使 Service 与文件格式强耦合，并导致重复文件解析。

### 决策 D92

> **V1.0 正式采用 Repository 层作为 Application / Service 与文件 Persistence 之间的数据访问边界。业务 Service 不直接处理具体文件路径、字段分隔、序列化格式或底层文件读写细节。**

---

## 36.2 Repository 的划分粒度

不采用“每一个领域对象机械对应一个 Repository”，也不采用单一巨型 `DataRepository`。

当前按照稳定持久化业务集合划分：

```text
UserRepository
VolunteerRecordRepository
RuleRepository
SemesterRepository
DiaryRepository
OperationLogRepository
```

对应：

```text
UserRepository
├── Student
└── Administrator

VolunteerRecordRepository
└── VolunteerRecord

RuleRepository
├── VolunteerCategory
└── BadgeRule

SemesterRepository
└── Semester

DiaryRepository
├── DiaryPost
└── LikeRelation

OperationLogRepository
└── OperationLog
```

### 决策 D93

> **Repository 按稳定持久化业务集合划分，而非机械采用“一领域对象一个 Repository”。`UserRepository` 统一管理 Student / Administrator，`RuleRepository` 统一管理 VolunteerCategory / BadgeRule，`DiaryRepository` 统一管理 DiaryPost / LikeRelation。**

---

## 36.3 当前不建立 Repository 接口双层体系

当前真实持久化实现只有文件。

因此不提前建立：

```text
IUserRepository
      ↑
FileUserRepository
```

以及大量类似 `IXXXRepository + FileXXXRepository` 双层类。

### 决策 D94

> **当前 Repository 采用具体类，不为每个 Repository 额外创建接口 + 文件实现双层体系。只有未来真实出现第二种可替换持久化实现时，再重新评估 Repository 抽象接口。**

该决策用于避免为了形式上的“可扩展”提前企业化。

---

## 36.4 Repository 不承担业务逻辑

### 决策 D95

> **Repository 只负责数据访问与持久化，不承担业务状态转换、权限判断、积分算法、排行榜、徽章判定等业务逻辑。Service 协调业务流程，Domain 保护自身不变量，Repository 负责权威数据读写。**

架构边界：

```text
              Qt
               │
               ▼
        Application / Service
        ┌──────┼────────┐
        │      │        │
       权限   流程     跨对象协调
               │
               ▼
             Domain
        状态机 / 不变量
               │
               ▼
           Repository
        查询 / 保存 / 删除
               │
               ▼
           Persistence
      CSV / config / safe write
```

---

# 37. Gate 3.4.1：UserRepository 与多态对象恢复

`User` 已冻结为抽象基类：

```text
User <<abstract>>
├── Student
└── Administrator
```

因此持久化恢复时不能先恢复为一个普通 `User value` 再猜测角色。

---

## 37.1 一个 UserRepository，不要求一个物理文件

### 决策 D96

> **`UserRepository` 统一作为 Student 与 Administrator 的持久化访问入口，但两类用户不要求存入同一个物理文件。Repository 层统一，上层 Service 只依赖 UserRepository。**

### 决策 D97

> **Student 与 Administrator 分文件持久化，由 `UserRepository` 内部统一协调。这样既维持统一 User 查询入口，也避免两种派生类型在一个文件中出现大量无意义空字段。**

物理映射：

```text
UserRepository
├── students.csv
└── administrators.csv
```

---

## 37.2 动态类型恢复

### 决策 D98

> **持久化数据必须能够明确恢复用户实际派生类型，不得根据 accountId 长度或格式推断角色。类型由明确的数据来源或稳定类型语义确定，而不是字符串猜测。**

当前采用 Student / Administrator 分文件后，文件来源本身已经提供明确具体类型。

### 决策 D99

> **`UserRepository` 恢复用户对象时必须保留 Student / Administrator 的运行时动态类型，禁止按值转换为 User 造成对象切片。最终使用引用、指针还是智能指针留待 Gate 3.4.9 对象生命周期与接口审计。**

错误方向：

```text
Student object
   ↓ value copy
User object
   ↓
Student 专属状态丢失
```

正确架构语义：

```text
students.csv / administrators.csv
              ↓
         UserRepository
              ↓
  构造真实 Student / Administrator
              ↓
    以保留动态类型的方式交给上层
```

---

## 37.3 当前不引入 UserFactory

### 决策 D100

> **当前不额外引入 `UserFactory`。Student / Administrator 类型数量少、恢复逻辑清晰，由 UserRepository 根据持久化来源直接构造对应对象。只有未来对象构造逻辑明显复杂化时再评估 Factory。**

---

# 38. Gate 3.4.2：物理文件结构与 CsvCodec

## 38.1 当前推荐数据目录

### 决策 D101

> **V1.0 采用多文件持久化，按稳定业务集合拆分物理文件，避免把所有系统数据塞入单一大文件。**

当前数据目录：

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

逻辑 Repository 与物理文件映射：

```text
UserRepository
├── students.csv
└── administrators.csv

VolunteerRecordRepository
└── volunteer_records.csv

RuleRepository
├── volunteer_categories.csv
└── badge_rules.csv

SemesterRepository
├── semesters.csv
└── system_config.txt 中 currentSemesterId

DiaryRepository
├── diary_posts.csv
└── likes.csv

OperationLogRepository
└── operation_logs.csv
```

---

## 38.2 CSV 不能使用简单 split(',')

### 决策 D102（修订冻结）

> **固定字段、表格型领域数据优先采用 CSV 持久化。项目提供轻量 CSV 编解码能力，正确处理字段中的逗号、双引号、换行和空字段等基本 CSV 情况，不允许用简单 `split(',')` 方式解析复杂 CSV。CSV 编解码只负责格式语法，不承载领域业务含义。**

例如字段：

```text
参加环保活动，清理操场
```

不能被错误拆成两个字段。

---

## 38.3 CsvCodec 技术组件

### 决策 D107

> **引入轻量技术组件 `CsvCodec`，作为 Repository / Persistence 层公共 CSV 编解码组件。其职责限定为 CSV 字段转义、记录编码、记录解析及必要格式检查；不认识 Student、VolunteerRecord 等具体领域对象，不判断业务规则、不负责对象查询、不负责文件路径。各 Repository 负责“领域对象 ↔ 字段集合”的映射，并复用 CsvCodec 完成底层 CSV 编解码。**

架构图：

```text
              Service
                 │
                 ▼
             Repository
       ┌─────────┴─────────┐
       │                   │
领域对象 ↔ 字段集合映射   文件定位/集合访问
       │
       ▼
             CsvCodec
       CSV encode / parse
       escape / quote rules
       │
       ▼
              *.csv
```

`CsvCodec` 即使最终实现为无数据成员技术类，也有明确架构理由：集中复用 CSV 规则，避免多个 Repository 各自复制解析逻辑；不得为了满足“类有数据成员”而人为塞入无意义状态。

---

> **实现细化 → Gate 4**
>
> 本节冻结的是 CsvCodec 的职责边界与多文件 CSV 架构；对应 CSV 语法、Header、字段验证与启动加载规则已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.5_CSVSchema_CsvCodec字段验证与schemaVersion实现细化_候选冻结稿(1).md`

## 38.4 system_config 与全局 schema

### 决策 D103

> **`system_config.txt` 用于保存少量全局键值配置，当前至少包括 `schema_version` 与 `currentSemesterId`。`currentSemesterId` 不重复写入 Semester 对象，也不通过多行 `isCurrent` 表达。**

示意：

```text
schema_version=1
current_semester_id=SEM2026_1
```

### 决策 D104

> **采用一个全局 `schema_version` 管理整个 data 目录的数据格式版本，不为每个 CSV 建立独立版本体系。当前只要求识别版本并为字段演进预留空间，不建立复杂 migration 框架。**

---

> **实现细化 → Gate 4**
>
> 本节冻结的是 `system_config`、全局 schema 与当前学期配置的架构语义；对应 stable ID sequence、配置字段和 schemaVersion 的实现级规则已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.4_StableID日期时间与数值类型实现细化_候选冻结稿(1).md`
> - `Gate4_4.5_CSVSchema_CsvCodec字段验证与schemaVersion实现细化_候选冻结稿(1).md`

## 38.5 权威事实与必要 ID

### 决策 D105

> **各持久文件只保存对应对象自身的权威事实和必要稳定关联 ID，不重复持久化可派生结果。**

例如 `diary_posts.csv` 不重复保存：

- `likeCount`；
- 学生姓名；
- 类别名称；
- 服务时长；
- 本条积分。

这些通过 `recordId` 等稳定关联动态获得。

### 决策 D106

> **Student / Administrator 文件只保存密码认证数据，不保存明文密码；LikeRelation 文件继续采用 `(studentAccountId, postId)` 逻辑唯一关系，不新增 `likeId`。**

---

# 39. Gate 3.4.3：Repository 的运行时权威状态模型

## 39.1 启动加载 + 内存集合 + 及时持久化

### 决策 D108（修订冻结）

> **V1.0 Repository 采用“启动加载 → 运行期内存集合 → 成功业务操作后及时持久化”的模式，不采用每次查询都重新打开文件的纯现读方案。一次会改变权威业务状态的操作，只有要求持久化的数据成功落盘后，才能向上层报告业务提交成功；若持久化失败，不得把仅存在于内存中的变更继续视为正常已提交状态。具体回滚/恢复机制由 D114～D121 继续冻结。**

核心语义：

```text
业务对象在内存中发生合法变化
          ↓
Persistence 提交成功？
       ├── 否 → 不算业务成功
       └── 是 → 才向 Qt / 调用方报告成功
```

---

## 39.2 Repository 内存集合不是派生缓存

### 决策 D109（修订冻结）

> **Repository 内存集合只保存领域对象的权威持久事实和稳定关联 ID，不得为了查询方便混入 `totalScore`、`currentRank`、`currentBadge`、`likeCount` 等动态派生值。派生结果仍由 StatisticsService、RankingService、BadgeService、DiaryService 等根据当前权威数据动态计算。**

因此：

```text
Repository 内存对象集合
≠
排行榜/积分/徽章缓存
```

---

## 39.3 当前物理文件写入分类

### 决策 D110（修订冻结）

当前持久化文件分为三类：

```text
A. 集合型 CSV
students.csv
administrators.csv
volunteer_records.csv
volunteer_categories.csv
badge_rules.csv
semesters.csv
diary_posts.csv
likes.csv
operation_logs.csv

B. 键值配置
system_config.txt

C. 临时安全写入文件
*.tmp / *.bak
```

> **普通运行时权威 CSV 当前以“更新内存集合 → 序列化完整集合 → 安全重写”作为基础策略。OperationLog 在业务语义上只新增、历史不可改，但物理文件写入不强制使用 `ios::app`；D120 已进一步统一其安全持久化语义。**

---

## 39.4 单一运行时 Repository 实例

### 决策 D111（修订冻结）

> **同一 Repository 类型在一次应用运行期间只维护一份权威实例，由应用级组合位置统一创建和管理，并供多个 Service 使用；不得让不同 Service 各自创建独立 Repository 副本，也不得通过全局变量或 Singleton 暴露。这里的“一份共享实例”只表达逻辑上的单一权威实例，不提前冻结 `shared_ptr` 等具体 C++ 所有权形式。当前 V1.0 业务与持久化访问默认在 Qt 主线程同步执行，不引入后台持久化线程、并发 Repository 写入或锁机制。**

架构图：

```text
                 Application Composition Root
                 /       |       |       \
                /        |       |        \
      UserRepository  RecordRepo DiaryRepo RuleRepo ...
            │             │        │        │
       ┌────┴───┐       ┌─┴────┐   │      ┌─┴──────┐
       │        │       │      │   │      │        │
Authentication UserMgmt Student  Review  Diary   RuleConfig
   Service       Service  Service Service Service   Service

同一 Repository 类型只有一份运行时权威实例；
多个 Service 使用同一实例，而不是各自加载一份数据。
```

---

## 39.5 加载失败不得部分运行

### 决策 D112（修订冻结）

> **对已经部署/初始化的数据环境，任一必需持久化文件解析失败、schema 不兼容或加载结果不完整时，不得把成功加载的部分 Repository 当作正常系统状态继续进入业务运行。当前 V1.0 的初始数据文件由项目部署阶段人工准备，不依赖运行中程序自动补造缺失核心文件。**

---

## 39.6 强业务引用与历史审计软引用

### 决策 D113（修订冻结）

> **启动引用完整性检查正式区分“当前业务强引用”和“历史审计软引用”。强引用悬空代表当前业务状态损坏；历史审计软引用即使目标当前无法解析，也不否定历史日志事实。**

### A. 当前业务强引用

至少检查：

```text
VolunteerRecord.ownerAccountId
→ Student

VolunteerRecord.appliedCategoryId
→ VolunteerCategory

Approved VolunteerRecord.finalCategoryId
→ VolunteerCategory

存在当前审核信息时 VolunteerRecord.reviewerAccountId
→ Administrator

BadgeRule.categoryId
→ VolunteerCategory

DiaryPost.recordId
→ VolunteerRecord

LikeRelation.studentAccountId
→ Student

LikeRelation.postId
→ DiaryPost

SystemConfig.currentSemesterId
→ Semester
```

强引用任一悬空：

> **不得进入正常业务。**

### B. 历史审计软引用

```text
OperationLog.operatorAccountId
OperationLog.targetType + targetId
```

`operatorAccountId` 业务语义仍指向历史执行 Administrator，但 OperationLog 是独立审计事实；历史目标对象被物理删除后 `targetId` 必须继续保留。

因此历史日志引用不要求目标当前仍存在。无法解析时：

```text
优先显示可解析对象信息
否则保留 / 显示稳定 ID
```

不得因为历史目标已不存在而删除或否定 OperationLog。

---

## 39.7 Gate 3.4 三条贯穿性约束

### P1——提交一致性

> 需要持久化的业务状态变更，持久化成功是业务提交成功的必要组成部分；文件写入失败时，内存中的未落盘变更不得继续被视为正常已提交结果。

### P2——单一运行时事实源

> 每种 Repository 在一次应用运行期间只有一份权威实例，由应用级对象统一管理并显式提供给 Service；这不是 Singleton，也不提前规定智能指针形式。

### P3——完整启动原则

> 已部署数据环境的加载、解析和强引用完整性检查必须整体通过后，系统才能进入正常业务；不得以部分 Repository 成功加载为由带着损坏数据继续运行。历史审计软引用不属于必须当前可解析的强引用 Gate。

---

# 40. Gate 3.4.4：安全写入与跨 Repository 提交

## 40.1 禁止直接 truncate 正式文件

### 决策 D114

> **普通持久化文件不得直接 truncate 后原地覆盖。保存集合时采用“完整写入临时文件 → 确认写入成功并关闭 → 再替换正式文件”的安全写入策略。正式文件在新的完整版本准备成功以前保持不变。**

基本流程：

```text
Repository 当前权威集合
          ↓
     serialize
          ↓
写 target.csv.tmp
          ↓
写入/关闭成功？
     ├── 否 → 正式文件不动
     └── 是
          ↓
进入正式替换 Commit
```

---

## 40.2 单份 .bak 保护

### 决策 D115

> **安全替换过程中允许使用单份 `.bak` 作为恢复候选，但 `.bak` 不属于业务版本历史，也不形成多代备份体系。程序开发版本历史仍由 Git 管理。**

---

## 40.3 Prepare / Commit 两阶段思维

### 决策 D116（修订冻结）

> **一次跨多个 Repository 的业务变更采用轻量“全部 Prepare 成功后才进入 Commit”的两阶段思维。Prepare 阶段先完成全部业务校验、内存变化以及所有受影响文件的 `.tmp` 准备；任一 Prepare 失败时，不修改正式文件，并恢复本次操作涉及的运行时内存状态。**

如果 Commit 已经开始，并出现：

```text
A 正式替换成功
B 正式替换成功
C 正式替换失败
```

则：

> **视为严重持久化错误。不得简单恢复内存快照后继续正常业务，也不得静默忽略。系统必须停止后续正常写业务，并进入明确的故障停止/恢复处理。**

当前不提前冻结“只读模式”“恢复页面”等具体 UI 形态。

---

## 40.4 运行时快照范围

### 决策 D117（修订冻结）

> **正常运行期间，在进入可能修改权威状态的业务流程前，只对本次操作实际会发生变更的 Repository 运行时集合建立临时回滚快照，不机械复制整个系统全部 Repository。当前数据规模较小，优先采用受影响 Repository 集合级快照；是否进一步缩小到对象级增量快照，留待实现阶段根据复杂度判断。**

示例：

```text
点赞
→ 主要涉及 DiaryRepository

审核通过
→ VolunteerRecordRepository
→ OperationLogRepository

物理删除 VolunteerRecord
→ VolunteerRecordRepository
→ DiaryRepository（若存在帖子/点赞）
→ OperationLogRepository
```

---

## 40.5 当前不承诺数据库级 ACID

### 决策 D118

> **V1.0 保证正常运行期间可检测的持久化失败不被当作成功业务继续使用，但不承诺多个独立 CSV 在进程突然崩溃、系统断电或 Commit 中断时具有数据库级跨文件 ACID 原子性。异常重启后依赖完整加载与强引用完整性 Gate 发现不一致，并拒绝带损坏状态进入正常业务。**

明确区分：

```text
无法提供数据库级原子事务
≠
允许损坏数据静默继续运行
```

---

## 40.6 文件机制属于 Persistence，不属于 Service

### 决策 D119

> **`.tmp`、`.bak`、安全文件替换和底层写入机制属于 Repository / Persistence 层职责。业务 Service 不直接操作这些文件机制。当前不冻结 TransactionManager、UnitOfWork 或 PersistenceCoordinator；只有后续接口审计证明存在真实必要时，才评估轻量跨 Repository 持久化协调组件。**

---

## 40.7 OperationLog：业务 append-only ≠ 文件必须 append

### 决策 D120

> **OperationLog 在业务语义上保持“只新增、历史日志不可修改/删除”，但不把这种语义机械等同于物理文件必须使用追加写模式。为了与其他业务文件使用统一安全写入策略，V1.0 可以将内存中的完整日志集合写入 `.tmp` 后安全替换正式 `operation_logs.csv`；旧 OperationLog 对象的业务内容不得被修改。**

因此：

```text
业务语义：append-only
物理保存：safe rewrite allowed
```

---

## 40.8 固定 Commit 顺序

### 决策 D121

> **跨多个持久化文件的 Commit 顺序由 Repository / Persistence 层采用统一、确定的规范顺序控制，业务 Service 不得自行决定文件替换顺序。当前原则上先提交业务权威数据文件，再提交 `operation_logs`；具体完整文件顺序在持久化接口落地时统一定义。固定顺序只提高故障行为的确定性、可诊断性和恢复分析能力，不意味着获得数据库级跨文件原子性。任何 Commit 阶段部分成功后失败仍按严重持久化错误处理。**

失败模型：

```text
A. 业务校验失败
   ↓
不进入 Persistence
   ↓
正常业务失败

B. Prepare 失败
   ↓
正式文件尚未替换
   ↓
恢复受影响 Repository 内存快照
   ↓
报告持久化失败

C. Commit 部分成功后失败
   ↓
磁盘可能“部分新 + 部分旧”
   ↓
禁止继续正常写业务
   ↓
故障安全停止 / 恢复处理
```

---

> **实现细化 → Gate 4**
>
> 本节冻结的是安全写入、快照、Prepare / Commit、OperationLog 同步提交与严重故障边界；对应运行期持久化协议已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.3_PersistenceCoordinator安全写入与故障恢复实现细化_候选冻结稿(1).md`

# 41. Gate 3.4.5：启动、部署数据与故障安全恢复

本项目当前已明确实际部署方式：

> **项目初始需要的持久化文件由开发者在项目数据准备阶段创建；初始 Administrator 也由开发者预先建立。程序运行时不承担自动生成一整套生产数据目录与首个管理员的 Bootstrap 职责。**

因此 V1.0 启动模型可以保持更严格、更简单。

---

## 41.1 初始化/部署标记

### 决策 D122（修订冻结）

> **V1.0 不由程序自动创建生产数据目录、基础 CSV 或初始管理员账号；这些由项目部署/初始数据准备阶段人工建立。`system_config.txt` 存在且其中 `schema_version` 可解析并受当前程序支持，作为“该目录已被初始化/部署”的识别标志，但不是系统 Ready 的充分条件。程序仍必须继续验证全部必需文件、基础配置和引用完整性。**

不能只根据：

```text
data/ 目录存在
```

判断系统已初始化，因为空目录可能并非合法数据环境。

---

## 41.2 初始 Administrator

### 决策 D123（修订冻结）

> **V1.0 的初始 Administrator 由项目初始数据准备阶段人工创建，不由运行中的程序自动生成。程序启动时必须验证至少存在一个结构合法、可用于认证的 Administrator；若不存在，则视为初始数据配置错误，不开放正常业务。当前仍不实现“管理员创建额外管理员”业务功能。**

因此管理员创建追踪项当前解释为：

```text
首个 Administrator
→ 人工预置

运行期新增 Administrator
→ 当前 V1.0 不提供

若未来支持
→ 重新审计 Permission / UserManagementService /
   OperationLog / UserRepository
```

---

## 41.3 正常启动必须先完成 Persistence Bootstrap

### 决策 D124

> **正常业务 Service 与 Qt 业务界面启用之前，必须先完成完整 Persistence Bootstrap：读取/验证 system_config → 校验 schema → 检查必需文件 → 加载所有 Repository → CsvCodec 解析 → 构造运行时对象集合 → 对象自身合法性检查 → 跨文件强引用完整性检查 → Ready 条件检查。全部通过后，才允许进入正常登录与业务运行。**

启动架构图：

```text
Application start
      │
      ▼
读取 system_config.txt
      │
schema_version 合法？────否────> FAIL-SAFE STARTUP STOP
      │是
      ▼
必需文件全部存在？───────否────> FAIL-SAFE STARTUP STOP
      │是
      ▼
加载所有 Repository
      │
CSV / config 解析成功？───否────> FAIL-SAFE STARTUP STOP
      │是
      ▼
对象自身状态/字段合法？───否────> FAIL-SAFE STARTUP STOP
      │是
      ▼
强引用完整性 PASS？───────否────> FAIL-SAFE STARTUP STOP
      │是
      ▼
Ready 条件 PASS？─────────否────> FAIL-SAFE STARTUP STOP
      │是
      ▼
创建 / 启用 Application Services
      │
      ▼
Qt 登录界面
```

---

## 41.4 `.tmp` 残留

### 决策 D125（修订冻结）

> **残留 `.tmp` 仅表示未完成或未正式提交的候选写入，不自动视为比正式文件更新或正确。若整个正式数据集加载、解析和强引用检查均合法，则继续使用正式文件，并可在 Ready 后清理已识别的残留 `.tmp`；若正式文件非法，不自动使用 `.tmp` 覆盖，直接进入故障安全启动失败流程。**

---

## 41.5 `.bak` 恢复候选

### 决策 D126（修订冻结）

> **`.bak` 只表示上一份正式数据的人工恢复候选，不属于普通运行时权威状态。正式文件合法时优先使用正式文件；正式文件损坏而存在 `.bak` 时，V1.0 普通启动不自动回退，因为不同文件恢复到不同时间点可能造成跨文件版本错配。**

---

## 41.6 schema 兼容性

### 决策 D127

> **全局 `schema_version` 必须与当前程序明确支持的数据格式版本一致，或存在已经实现并验证过的升级路径。当前未建立 migration 机制，因此未知、更高或不受支持的旧 schema 均不得“尽量解析”后继续正常运行。**

---

## 41.7 缺文件与合法空集合

### 决策 D128

> **合法空集合与文件缺失严格区分。已部署系统中，只有表头而无数据的 CSV 可以表示合法空集合；必需持久化文件整体缺失不得静默解释成“0 条数据”。当前文件由部署阶段人工准备，缺失文件属于配置/持久化错误并导致启动失败。**

---

## 41.8 Ready 条件与 Semester

### 决策 D129（修订冻结）

> **V1.0 进入正常业务前，持久化数据中必须已经存在至少一个合法 Semester，且 `system_config.currentSemesterId` 必须引用其中一个合法 Semester。当前项目采用人工准备初始基础配置的方式，不设计独立首次启动学期配置向导或半初始化业务模式；若 Ready 条件不满足，程序按配置错误启动失败。系统正常运行后，管理员仍通过 `SemesterService` 维护学期与切换当前学期。**

当前 Ready 最低条件至少包括：

```text
1. system_config / schema 合法
2. 所有必需文件存在且可解析
3. 至少一个合法 Administrator
4. 至少一个合法 Semester
5. currentSemesterId → 合法 Semester
6. 所有当前业务强引用完整
```

不新增：

```text
SystemState
BootstrapService
InitializationService
```

等永久业务对象/服务。

---

## 41.9 Recovery 最低边界

### 决策 D130

> **V1.0 持久化恢复机制的最低边界正式冻结为“故障安全启动停止（fail-safe startup）”：一旦出现必需文件缺失、不可解析、schema 不兼容、Ready 条件不满足或强引用完整性失败，程序不得进入正常业务，也不得自动修补、自动回退或猜测正确数据；至少向用户提供明确的启动失败原因。错误最终通过 Qt 对话框、错误页面还是错误日志呈现，留待表现层/实现阶段确定。**

因此当前恢复语义不是一个空概念：

```text
检测异常
   ↓
停止正常启动
   ↓
明确报告故障原因
   ↓
不自动改变正式持久化数据
```

---

# 42. Gate 3.4 当前 Repository / Persistence 总架构图

## 42.1 分层总图

```text
┌──────────────────────────────────────────────────────────┐
│                    Qt Presentation                       │
└──────────────────────────┬───────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────┐
│                Application / Service                     │
│ Authentication / UserManagement / StudentVolunteer      │
│ VolunteerReview / Statistics / Ranking / Badge / Diary  │
│ RuleConfiguration / Semester / OperationLog / Export    │
└──────────────────────────┬───────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────┐
│                        Domain                            │
│ User / Student / Administrator / VolunteerRecord        │
│ VolunteerCategory / BadgeRule / Semester / DiaryPost    │
│ LikeRelation / OperationLog                             │
└──────────────────────────┬───────────────────────────────┘
                           │ stable IDs / object access
                           ▼
┌──────────────────────────────────────────────────────────┐
│                      Repository                          │
│ UserRepository        VolunteerRecordRepository         │
│ RuleRepository        SemesterRepository                │
│ DiaryRepository       OperationLogRepository            │
└──────────────────────────┬───────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────┐
│                Persistence Technical Layer               │
│ CsvCodec / config parser / temp file / safe replace     │
└──────────────────────────┬───────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────┐
│                       data/                              │
│ students.csv                administrators.csv           │
│ volunteer_records.csv       volunteer_categories.csv    │
│ badge_rules.csv              semesters.csv              │
│ diary_posts.csv              likes.csv                  │
│ operation_logs.csv           system_config.txt          │
└──────────────────────────────────────────────────────────┘
```

---

## 42.2 Repository / 文件映射图

```text
UserRepository
 ├── students.csv
 └── administrators.csv

VolunteerRecordRepository
 └── volunteer_records.csv

RuleRepository
 ├── volunteer_categories.csv
 └── badge_rules.csv

SemesterRepository
 └── semesters.csv
        │
        └── currentSemesterId ← system_config.txt

DiaryRepository
 ├── diary_posts.csv
 └── likes.csv

OperationLogRepository
 └── operation_logs.csv

所有 CSV Repository
 └── reuse → CsvCodec
```

---

## 42.3 跨 Repository 写入故障模型图

```text
Service 完成权限 / 业务校验
            │
            ▼
对受影响 Repository 建临时快照
            │
            ▼
修改内存权威状态
            │
            ▼
Prepare 所有受影响 *.tmp
       ┌────┴────┐
      FAIL       PASS
       │          │
       ▼          ▼
恢复内存快照    Commit 固定顺序替换
报告失败          │
               ┌──┴───────────────┐
             全成功            部分成功后失败
               │                  │
               ▼                  ▼
          业务正式成功       严重持久化错误
          才报告成功         停止正常写业务
                                  │
                                  ▼
                           下次启动完整性 Gate
```

---

## 42.4 启动完整性模型图

```text
                 已部署 data/
                      │
                      ▼
               system_config
                      │
              schema_version
                      │
                      ▼
          ┌─────加载所有 Repository─────┐
          │                             │
          ▼                             ▼
   对象字段/状态合法性             CsvCodec / config
          │                             │
          └──────────────┬──────────────┘
                         ▼
                  强引用完整性
                         │
        ┌────────────────┼─────────────────┐
        │                │                 │
Record.owner        DiaryPost.record   LikeRelation.post
→ Student           → Record           → DiaryPost
        │                │                 │
        └────────────────┼─────────────────┘
                         ▼
                    Ready 条件
                         │
             ┌───────────┴───────────┐
            PASS                    FAIL
             │                       │
             ▼                       ▼
        构建 Service            fail-safe stop
        显示 Qt 登录            明确错误原因
```

---

# 43. Gate 3.4.9：Repository 查询接口与对象生命周期

本节在 D92～D130 已冻结的 Repository / Persistence 基线上，进一步解决：

- `findById()` 的对象访问语义；
- Service 是否可以长期保存 Repository 内部对象引用；
- Repository 外部是否能够直接修改权威对象；
- 内部容器变化后引用 / 指针 / 迭代器如何失效；
- Repository 查询与业务规则查询的职责边界。

## 43.1 只读访问与受控修改必须区分

### 决策 D131

> **Repository 对外区分“只读查询语义”和“受控修改语义”。统计、排行、徽章、导出等只读流程不得获得可任意修改权威集合的访问能力；需要修改领域状态的 Service 仅在当前业务调用范围内定位权威对象，并通过 Domain 的语义行为完成合法状态转换。**

基本方向：

```text
只读业务
→ Repository 查询
→ const 对象访问
→ 只消费数据

状态修改业务
→ Service 完成权限 / 归属 / 规则检查
→ Repository 定位权威对象
→ Domain 语义行为修改状态
→ 统一持久化提交
```

## 43.2 不采用“可变副本覆盖”作为统一更新模型

### 决策 D132

> **运行期权威实体不默认按值返回可变副本，避免 Repository 权威对象与 Service 可变副本形成两份状态。**

因此不采用以下思路作为主要业务修改方式：

```text
Repository
→ 返回 VolunteerRecord 副本
→ Service 修改副本
→ update(copy) 覆盖原对象
```

已有实体的正常修改应定位当前权威对象，再调用对象自身的受控领域行为。

## 43.3 Service 不长期持有对象引用

### 决策 D133

> **Service 不长期保存领域对象的裸指针、引用或迭代器。跨调用、跨页面或长期业务关联继续保存稳定 ID；一次 Service 调用中可以临时取得对象访问权，调用结束后不得依赖该对象访问句柄继续有效。**

长期关系仍使用：

```text
recordId
accountId
categoryId
postId
semesterId
...
```

而不是长期保存：

```text
VolunteerRecord*
Student*
DiaryPost&
vector iterator
```

## 43.4 Repository 内部容器不暴露给上层

### 决策 D134

> **Repository 内部 STL 容器属于实现细节，不向 Service / Domain 暴露可任意 `push_back / erase / clear` 的可写容器。**

因此不允许形成：

```text
repository.records().erase(...)
repository.students().push_back(...)
```

这保证后续即使主容器更换，上层接口也不被连带破坏。

## 43.5 Repository 查询只表达数据访问条件

### 决策 D135

Repository 可以承接：

```text
findById
findByOwnerAccountId
findByStatus
findByDateRange
```

但以下内容不下沉到 Repository：

```text
findGoldBadgeStudents
findCurrentTopThree
findRecordsThatCanBeApproved
```

因为后者已经包含徽章、排行榜、状态机等业务规则。

## 43.6 Not Found 与系统故障必须区分

### 决策 D136

> **“对象不存在”属于正常查询结果，不默认等同于系统异常。**

例如不存在的 `recordId` 可以正常返回 Not Found；CSV 损坏、Repository 未正确恢复、内部不变量破坏则属于系统级错误。

## 43.7 Repository 不依赖 Qt 类型

### 决策 D137

Repository / Domain / Service 保持纯 C++ 边界，不返回或依赖：

```text
QVector
QStringList
QVariantMap
```

Qt Presentation 通过 Service 获得普通 C++ 业务结果后自行完成界面适配。

## 43.8 物理删除由 Repository 执行存储移除

### 决策 D138

> **领域对象不自行销毁自身，也不跨 Repository 删除其他对象。**

例如删除 VolunteerRecord：

```text
VolunteerReviewService
→ 检查权限 / 状态 / 业务合法性
→ 协调 DiaryPost / LikeRelation 联动
→ VolunteerRecordRepository 移除 record
→ OperationLog
→ Persistence 提交
```

Repository 的 `removeById()` 只负责集合移除，不自行判断权限、审核规则或日志规则。

---

# 44. Gate 3.4.10：Repository 内部所有权与容器策略

## 44.1 普通实体采用值语义存储

### 决策 D139

> **普通非多态领域实体由对应 Repository 直接以值语义持有，不默认使用 `unique_ptr/shared_ptr` 等堆对象容器。**

当前主要实体均适合直接保存：

```text
VolunteerRecord
VolunteerCategory
BadgeRule
Semester
DiaryPost
LikeRelation
OperationLog
```

## 44.2 普通 Repository 当前优先 std::vector<T>

### 决策 D140

> **当前课程项目数据规模较小，普通 Repository 优先采用 `std::vector<T>` 保存运行期权威集合。**

理由：

- 顺序加载 / 保存自然；
- 全量统计遍历自然；
- 实现与答辩易解释；
- 当前无真实性能瓶颈；
- 不提前引入复杂索引一致性。

## 44.3 不维护双结构 ID 索引

### 决策 D141

> **当前不维护 `vector + map/unordered_map` 双结构索引。若未来真实性能测试证明 ID 线性查找成为瓶颈，优先评估直接替换 Repository 主容器为合适的关联容器；只有单一容器无法同时满足真实访问模式时，才考虑辅助索引。**

> **实现细化 → Gate 4**
>
> 本节冻结的是 Repository 使用 `std::vector<T>` 权威集合且不预建双结构索引的架构语义；对应查找、筛选、统计与排序算法已在 Gate 4 进一步冻结。
>
> 统一检索入口：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 相关专题：
> - `Gate4_4.2_Repository数据结构查询与统计算法实现细化_候选冻结稿(1).md`

## 44.4 RuleRepository / DiaryRepository 分集合保存

### 决策 D142

```text
RuleRepository
├── vector<VolunteerCategory>
└── vector<BadgeRule>

DiaryRepository
├── vector<DiaryPost>
└── vector<LikeRelation>
```

不为不具备 `is-a` 关系的对象强造统一基类或异构指针容器。

## 44.5 OperationLogRepository 使用普通顺序集合

### 决策 D143

OperationLog 的“只新增、不可业务修改 / 删除”由接口和业务规则保证，不要求使用特殊链式容器。

## 44.6 UserRepository 的多态存储方式

### 决策 D144

`UserRepository` 当前优先：

```text
UserRepository
├── vector<Student>
└── vector<Administrator>
```

而不是：

```text
vector<unique_ptr<User>>
```

统一用户访问通过 `User` 抽象视图实现。该抽象视图只在当前受控查询 / 业务调用内有效，不作为跨调用、跨页面、长期存储句柄。

## 44.7 accountId 全局唯一

### 决策 D145

> **UserRepository 在 Student 与 Administrator 两个集合之间保证 `accountId` 全局唯一。**

Repository 保证数据唯一性；谁有权创建账号仍由业务 Service 决定。

## 44.8 业务归属与技术存储所有权分离

### 决策 D146

```text
业务上：
VolunteerRecord 属于 Student

技术上：
VolunteerRecord 对象由 VolunteerRecordRepository 持有
```

Repository 是领域对象的运行时技术存储所有者；领域业务归属不等同于 C++ 内存所有权。

## 44.9 vector 引用失效约束

### 决策 D147

> **任何可能改变顺序容器结构的操作完成后，先前取得的该容器元素引用、指针和迭代器均不得假定继续有效。**

至少包括：

```text
add / push_back
remove / erase
clear
load / reload
整体替换集合
```

这一点同时约束 Repository 内部实现与 Service 使用方。

---

# 45. Gate 3.4.11：Repository 查询返回类型最终审计

## 45.1 单对象查询采用可空 non-owning pointer

### 决策 D148

> **Repository 单对象查找采用“可空、非拥有、临时对象访问”语义。当前实现方向优先使用指针；Not Found 使用 `nullptr` 表达。**

不采用：

```text
按值返回可变 T
用异常表达所有 Not Found
空对象模式
```

## 45.2 const / non-const 查询分层

### 决策 D149

```text
只读查询
→ const T*

受控状态修改
→ T*
```

Repository 始终拥有对象；调用者只获得临时访问权。

## 45.3 返回指针是短生命周期视图

### 决策 D150

> **`T* / const T*` 只在当前业务调用且相关 Repository 未发生结构性修改期间有效。不得跨 Service 调用、跨 Qt 页面生命周期、跨 Repository 插入 / 删除 / 重载长期保存。**

## 45.4 UserRepository 统一返回 User 抽象视图

### 决策 D151

`UserRepository` 可以通过：

```text
User* / const User*
```

访问 Student 或 Administrator 的真实动态对象，不按值返回 `User`，不发生对象切片。

## 45.5 多对象查询默认只读

### 决策 D152

常规多对象查询当前优先使用：

```text
std::vector<const T*>
```

而不是：

```text
复制所有 T
暴露内部可写 vector<T>&
```

结果集合仅用于当前 Service 调用中的临时消费。

## 45.6 Repository 对象访问不得泄漏到 Qt 长生命周期

### 决策 D153

Repository 返回“对象访问”，Service 对外返回“业务结果”。

例如：

```text
Repository
→ vector<const VolunteerRecord*>

RankingService
→ RankingEntry 等值结果

Qt
→ 使用 RankingEntry 展示
```

Qt 不长期保存 Repository 内部对象地址。

## 45.7 add / remove 不返回长期对象地址

### 决策 D154

新增成功后优先通过稳定 ID 表达身份；需要对象时重新查询。删除操作只返回操作结果，不返回已经销毁对象的地址。

## 45.8 Not Found 不替系统故障背锅

### 决策 D155

正常查询不到对象可以返回 `nullptr`；文件损坏、schema 错误、Repository 未恢复成功等不能伪装为普通 Not Found。

## 45.9 const 成员函数边界

### 决策 D156

> **Repository 的只读查询方法必须采用 `const` 成员函数语义并返回 `const T*` / 只读结果；需要修改权威对象时才通过非 const Repository 接口取得 `T*`。**

当前不使用 `mutable` 绕过只读业务边界。只有未来出现与业务状态无关、且确有必要的内部技术缓存时，才重新审计 `mutable`。

## 45.10 AuthenticationService 的局部 dynamic_cast

### 决策 D157

AuthenticationService 通过 `User*` 完成统一账号认证。Student 独有的 `lastLoginAt` 更新允许在这一明确、局部的类型边界使用受控：

```text
dynamic_cast<Student*>
```

不把 `dynamic_cast` 推广成普通权限判断和业务分派机制；也不为了该字段强行在 User 中制造虚拟空行为。

## 45.11 必须文档化引用 / 指针失效条件

### 决策 D158

Repository 实现文档和关键接口注释必须明确：

- 返回指针 / 引用的有效期；
- 哪些操作会使其失效；
- 不允许长期保存的原因。

这一项进入 Gate 4 代码审计重点。

## 45.12 消费多对象查询期间不得结构修改

### 决策 D159

> **使用 `std::vector<const T*>` 等查询结果期间，不得对对应 Repository 执行会改变容器结构的新增、删除、清空、重新加载等操作。**

当前默认 Qt 主线程同步运行，不引入并发访问机制。

## 45.13 系统级错误与业务 Not Found 分层

### 决策 D160

Repository 的 `nullptr` 只表示正常 Not Found。未初始化、唯一性不变量破坏、逻辑上不可能的内部状态等程序错误必须进入系统级错误处理；开发期可使用断言，运行期具体异常 / 错误结果形式留待 Gate 4。

---

# 46. Gate 3.4.12：Repository 写接口与 PersistenceCoordinator

## 46.1 Repository 写接口只负责数据 / 存储级不变量

### 决策 D161

Repository 可以检查：

```text
stable ID 是否重复
LikeRelation 复合唯一键是否重复
BadgeRule.categoryId 唯一性
accountId 是否重复
```

但不负责：

```text
权限判断
业务状态机
排行 / 徽章
审核规则
谁有权删除
```

## 46.2 禁止通用 update(T) 覆盖权威实体

### 决策 D162

> **普通实体不提供通用 `update(T)` / `replace(T)` 作为主要修改路径。**

已有实体通过 Repository 临时定位，再由 Domain 语义行为修改合法状态。

## 46.3 写能力收敛为新增 / 移除 / 持久化协作

### 决策 D163

Repository 不向 Qt 或其他上层暴露内部容器 CRUD；其基础写能力围绕：

```text
add
removeById
find
prepare / commit 等 persistence capability
```

展开。

## 46.4 Service 不自行 saveAll / 操作 tmp / bak

### 决策 D164

> **业务 Service 不自行决定多个 Repository 的保存顺序，也不直接操作 `.tmp/.bak/rename`。单 Repository 与跨 Repository 写入均遵循统一 Persistence 协议。**

## 46.5 正式引入 PersistenceCoordinator

### 决策 D165

正式引入轻量技术组件：

```text
PersistenceCoordinator
```

职责：

```text
接收本次受影响 Repository
→ 组织 Prepare
→ 确认全部 Prepare 成功
→ 按固定顺序 Commit
→ 返回提交结果类别
```

它不是：

```text
业务 Service
TransactionManager
UnitOfWork
数据库事务框架
```

## 46.6 Repository 自己知道“怎样保存自己”

### 决策 D166

各 Repository 继续负责：

- 自身领域集合 ↔ 文件字段映射；
- 调用 CsvCodec；
- 准备自己的 `.tmp`；
- 执行自身文件的安全替换能力。

PersistenceCoordinator 只协调阶段和顺序，不集中实现全部领域序列化。

## 46.7 add / remove 只先改内存，不自动独立落盘

### 决策 D167

> **Repository 的 `add/remove` 等写操作只修改运行期权威内存集合；完整业务操作只有在统一 Persistence Commit 成功后，Service 才能向上层报告 Success。**

内存已修改只是业务流程中间态，不等于业务已经提交。

## 46.8 Prepare 失败的内存恢复

### 决策 D168

正常 Prepare 失败时，Service 按本次业务影响范围恢复受影响 Repository 的操作前内存快照。

当前数据规模下，快照采用**完整值语义复制**：

- 快照与修改后的 Repository 不共享可变领域状态；
- 当前普通领域对象由 STL 值类型、字符串、数值、枚举和稳定 ID 构成，可直接依赖正常复制语义；
- 若未来引入拥有型指针或共享可变资源，必须重新审计复制语义。

当前不建立数据库式通用 rollback / undo framework。

## 46.9 OperationLog 与主业务统一提交

### 决策 D169

需要日志的管理业务：

```text
完成内存业务变化
→ 创建 OperationLog 对象
→ 加入 OperationLogRepository 内存集合
→ 与其他受影响 Repository 一起 Prepare / Commit
```

Prepare 失败时，新日志也随快照恢复被撤销。

## 46.10 单 Repository 也不绕过统一提交协议

### 决策 D170

即使只影响一个 Repository，也继续遵循：

```text
内存业务变化
→ Prepare
→ Commit
→ Success
```

不另设“单文件就直接 save”的旁路。

## 46.11 提交错误结果必须分级

### 决策 D171

Service 执行结果必须区分：

```text
普通业务失败
Prepare / 可恢复持久化失败
Partial Commit / 严重持久化错误
```

不得把严重持久化故障统一降级成普通 `false`。

## 46.12 Partial Commit 后禁止继续正常写业务

### 决策 D172

一旦存在正式文件已经替换、后续 Commit 失败：

```text
当前持久化状态不可继续信任
→ 明确提示严重持久化错误
→ 禁止继续新的正常状态修改业务
```

具体表现可以在 Qt / 实现阶段选择安全退出、故障页面等，不提前冻结所谓完整恢复模式。

## 46.13 PersistenceCoordinator 不参与启动加载

### 决策 D173

> **PersistenceCoordinator 只用于系统正常运行期间业务修改后的持久化提交，不参与启动 Repository Load。**

启动仍独立采用：

```text
Files
→ Repository Load
→ schema / parse
→ 对象合法性
→ 强引用完整性
→ Ready
```

Bootstrap / Recovery 与运行期 Commit 路径分离。

---

# 47. Gate 3.4.13：Repository / Persistence 总体终审修正

Gate 3.4 终审未发现需要推翻 D92～D173 的根本架构冲突，但发现三个必须在 FINAL PASS 前补齐的一致性边界。

## 47.1 快照时序必须发生在第一次内存修改之前

### 决策 D174

> **受影响 Repository 的操作前值语义快照必须在第一次权威内存状态修改之前建立。**

正确顺序：

```text
1. 尽可能完成前置业务校验
2. 确定本次受影响 Repository
3. 建立操作前值语义快照
4. 执行 Domain / Repository 内存变化
5. 创建 OperationLog（如需要）
6. Prepare 全部受影响物理文件
7. Commit
```

不能采用：

```text
先修改
→ 再建立所谓“操作前快照”
```

否则 Prepare 失败时没有真正可恢复状态。

## 47.2 Partial Commit 判断落实到物理文件级

### 决策 D175

一个 Repository 可能管理多个文件：

```text
UserRepository
→ students.csv
→ administrators.csv

RuleRepository
→ volunteer_categories.csv
→ badge_rules.csv

SemesterRepository
→ semesters.csv
→ system_config.txt

DiaryRepository
→ diary_posts.csv
→ likes.csv
```

因此：

> **Persistence 一致性必须以实际受影响的物理文件集合为准，而不能假定一次 Repository commit 天然原子。**

只要任一正式文件已经替换，后续同 Repository 或其他 Repository 的正式文件替换失败，均属于 Partial Commit Failure，并触发 D172。

## 47.3 OperationLogService 不允许嵌套独立提交

### 决策 D176

`OperationLogService` 作为审核、治理、用户管理、配置等主业务 Service 的子流程时：

```text
只负责生成 / 加入合法 OperationLog 内存事实
+ 日志查询能力
```

不得：

```text
自己先调用 PersistenceCoordinator
提前独立 Commit OperationLogRepository
```

主业务状态与审计日志必须由发起完整业务动作的主 Service 一并纳入同一次持久化提交。

---

# 48. Gate 3.4 实现级注意事项

以下内容不新增架构决策编号，但属于 V1.0 实现时必须遵守的明确约束。

## 48.1 Repository 操作前快照

当前受影响 Repository 的快照采用**完整独立值语义快照**：

```text
操作前集合
→ 完整值复制
→ snapshot
```

要求：

- snapshot 与当前权威集合在业务意义上相互隔离；
- 修改当前对象不能同步污染 snapshot；
- 当前 `std::string`、数值、枚举、稳定 ID 等按正常 C++ 值语义复制即可；
- 未来若对象中出现拥有型指针、共享可变资源等，必须重新审计复制构造 / 赋值行为。

## 48.2 推荐物理文件 Commit 顺序

D121 + D175 已冻结“统一固定顺序 + 物理文件级判定”。当前 V1.0 推荐实现基线如下：

```text
1. students.csv
2. administrators.csv
3. semesters.csv
4. system_config.txt
5. volunteer_categories.csv
6. badge_rules.csv
7. volunteer_records.csv
8. diary_posts.csv
9. likes.csv
10. operation_logs.csv
```

说明：

- `semesters.csv` 在 `system_config.txt` 前，便于使 `currentSemesterId` 的被引用目标优先进入正式状态；
- `operation_logs.csv` 原则上最后提交，以保持“业务权威状态优先、审计事实后落盘”的确定性；
- 上述顺序用于实现确定性、测试性和故障诊断，不等于跨文件 ACID；
- 最终代码落地时可以在 Persistence 层统一定义明确文件顺序，但 Service 不得各自发挥。

## 48.3 Student.lastLoginAt 走统一运行期提交

`AuthenticationService` 对 Student 的成功登录时间更新属于正式持久状态变化：

```text
账号存在
+ 密码验证成功
+ accountStatus == Active
        ↓
若动态对象为 Student
        ↓
建立 UserRepository 操作前快照
        ↓
更新 lastLoginAt
        ↓
Prepare
        ↓
Commit
        ↓
完整登录流程成功
```

必须保证：

- 密码错误时不更新；
- 账号不存在时不更新；
- Disabled 时不更新；
- 不允许只修改内存而不持久化；
- 如果该状态提交失败，不能把完整登录流程报告为正常成功。

---

# 49. Gate 3.4 最终全景架构图

## 49.1 运行时分层与所有权图

```text
┌──────────────────────────────────────────────────────────────┐
│                    Qt Presentation                           │
│   页面 / 控件 / Model / Signal-Slot / DTO 展示              │
└──────────────────────────────┬───────────────────────────────┘
                               │ 普通 C++ 业务结果
                               ▼
┌──────────────────────────────────────────────────────────────┐
│                 Application / Service                        │
│ Auth / UserMgmt / Volunteer / Review / Stats / Ranking ...  │
│ - 权限、归属、跨对象流程、日志协调                           │
│ - 不长期保存 Repository 对象地址                             │
└──────────────────────┬───────────────────────┬───────────────┘
                       │                       │
                       ▼                       ▼
┌────────────────────────────┐      ┌───────────────────────────┐
│           Domain           │      │      Repository           │
│ User / Student / Record... │      │ 运行期唯一权威对象集合   │
│ 状态机 / 自身不变量        │      │ find/add/remove           │
└────────────────────────────┘      │ 值语义拥有对象            │
                                    └──────────────┬────────────┘
                                                   │
                                                   ▼
                                    ┌───────────────────────────┐
                                    │ Persistence / CsvCodec    │
                                    │ tmp / bak / safe replace  │
                                    │ schema / files            │
                                    └───────────────────────────┘
```

## 49.2 Repository 内部集合图

```text
UserRepository
├── vector<Student>
└── vector<Administrator>

VolunteerRecordRepository
└── vector<VolunteerRecord>

RuleRepository
├── vector<VolunteerCategory>
└── vector<BadgeRule>

SemesterRepository
└── vector<Semester>

DiaryRepository
├── vector<DiaryPost>
└── vector<LikeRelation>

OperationLogRepository
└── vector<OperationLog>
```

运行时对象访问：

```text
长期关系    → stable ID
临时查询    → T* / const T*
多态用户    → User* / const User*
跨 Service  → 不保存 Repository pointer view
Qt 展示     → Service 值结果 / DTO
```

## 49.3 启动恢复通道

```text
                 已部署 data/
                       │
                       ▼
              system_config.txt
                       │
               schema_version
                       │
                       ▼
               Repository Load
                       │
             CSV / Config Parse
                       │
                       ▼
               对象自身合法性
                       │
                       ▼
              强业务引用完整性
                       │
                       ▼
                  Ready 条件
                 /          \
              PASS          FAIL
               │             │
               ▼             ▼
          构建 Service   fail-safe stop
          显示 Qt 登录   明确错误原因
```

`PersistenceCoordinator` 不参与此路径。

## 49.4 运行期修改与提交通道

```text
                    业务请求
                       │
                       ▼
               前置业务校验
                       │
                       ▼
            确定受影响 Repository
                       │
                       ▼
             建立操作前值语义快照
                       │
                       ▼
             Domain / Repository
               内存业务状态变化
                       │
                       ▼
           OperationLog 内存事实
                （如需要）
                       │
                       ▼
             PersistenceCoordinator
                       │
                 Prepare ALL
                /           \
             FAIL           PASS
              │              │
              ▼              ▼
        恢复内存快照      固定顺序 Commit
              │          /              \
              ▼       全成功          部分失败
        可恢复持久化失败   │               │
                           ▼               ▼
                        Success      Severe Persistence
                                          │
                                          ▼
                                  禁止继续正常写业务
```

## 49.5 物理文件级 Commit 视角

```text
PersistenceCoordinator
        │
        ▼
受影响物理文件列表
        │
        ├── students.csv.tmp
        ├── semesters.csv.tmp
        ├── volunteer_records.csv.tmp
        ├── diary_posts.csv.tmp
        ├── likes.csv.tmp
        └── operation_logs.csv.tmp
        │
        ▼
全部 Prepare 成功
        │
        ▼
按统一文件顺序安全替换正式文件
        │
   ┌────┴───────────────┐
   │                    │
全成功              任一正式文件已替换后
   │                 后续文件失败
   ▼                    │
业务 Success             ▼
                 Partial Commit Failure
                          │
                          ▼
               持久化状态不可继续信任
```

---

# 50. Gate 3.4 决策索引

| 编号 | 当前冻结决策 |
|---|---|
| D92 | V1.0 引入 Repository 层，Service 不直接处理底层文件格式与路径 |
| D93 | Repository 按稳定持久化业务集合划分：User / Record / Rule / Semester / Diary / OperationLog |
| D94 | 当前 Repository 采用具体类，不建立无第二实现的 IXXXRepository + FileXXXRepository 双层体系 |
| D95 | Repository 只负责数据访问与持久化，不承担权限、状态机、积分、排行、徽章等业务逻辑 |
| D96 | UserRepository 统一 Student / Administrator 数据访问入口，但不要求一个物理文件 |
| D97 | Student / Administrator 分文件，由 UserRepository 内部统一协调 |
| D98 | 用户具体类型通过明确持久化来源 / 类型语义恢复，不按 accountId 格式猜测 |
| D99 | 恢复用户必须保留动态类型，禁止 User value 对象切片 |
| D100 | 当前不引入 UserFactory |
| D101 | 采用多文件 data 目录持久化结构 |
| D102 | CSV 使用正确转义 / 解析规则，不采用简单 split(',') |
| D103 | system_config.txt 保存 schema_version 与 currentSemesterId 等全局键值配置 |
| D104 | 一个全局 schema_version 管理整个数据目录，不建立逐文件迁移版本体系 |
| D105 | 文件仅保存权威事实和必要稳定 ID，不重复持久化派生结果 |
| D106 | 不持久化明文密码；LikeRelation 不新增 likeId |
| D107 | 引入轻量 CsvCodec，统一 CSV 编解码语法，不承载领域业务语义 |
| D108 | 启动加载 + 运行期内存集合 + 操作后及时持久化；持久化成功才算提交成功 |
| D109 | Repository 内存集合不混入 totalScore/currentRank/currentBadge/likeCount 等派生值 |
| D110 | 权威 CSV 采用完整集合安全重写基础策略；OperationLog 物理写入由 D120 精化 |
| D111 | 每种 Repository 一次运行只有一个权威实例；非 Singleton；当前主线程同步访问 |
| D112 | 已部署数据环境解析 / 加载失败不得部分运行 |
| D113 | 区分当前业务强引用与历史审计软引用，并冻结强引用完整性 Gate |
| D114 | 正式文件不得直接 truncate 原地覆盖，使用完整临时文件后再安全替换 |
| D115 | 允许单份 .bak 作为恢复候选，不形成多代数据备份历史 |
| D116 | 跨 Repository 采用 Prepare / Commit 轻量两阶段思维；Commit 部分失败属严重持久化错误 |
| D117 | 只快照本次实际受影响 Repository 集合，用于运行时恢复 |
| D118 | 不承诺独立 CSV 的数据库级跨文件 ACID，但损坏状态不得静默继续运行 |
| D119 | .tmp/.bak/安全替换属于 Persistence；不引入企业级 TransactionManager / UnitOfWork |
| D120 | OperationLog 业务上只新增，但物理文件允许安全整集合重写 |
| D121 | Commit 顺序由 Persistence 统一固定；OperationLog 原则上后提交；顺序不等于原子事务 |
| D122 | 初始数据目录 / 文件由人工部署；system_config + 合法 schema 是已部署标志而非 Ready 充分条件 |
| D123 | 初始 Administrator 人工预置；当前不实现运行期管理员创建管理员 |
| D124 | Persistence Bootstrap 全部通过后才启用 Service 与 Qt 正常业务 |
| D125 | 残留 .tmp 不自动恢复；正式数据整体合法后才可清理残留 tmp |
| D126 | .bak 只作为人工恢复候选，普通启动不自动回退 |
| D127 | schema 必须明确受支持，否则拒绝正常加载 |
| D128 | 已部署系统缺失必需文件不能解释为空集合 |
| D129 | Ready 前必须存在合法 Semester 且 currentSemesterId 有效；不设计半初始化业务模式 |
| D130 | 恢复最低边界为 fail-safe startup stop：明确报错，不自动修补 / 回退 / 猜测数据 |
| D131 | Repository 对外区分只读查询语义与受控修改语义 |
| D132 | 不按值返回可变副本作为权威实体统一更新方式 |
| D133 | Service 不长期保存领域对象指针 / 引用 / 迭代器；长期关系保存 stable ID |
| D134 | Repository 内部容器不向上层暴露可写访问 |
| D135 | Repository 查询只表达数据访问条件，不承载排行榜 / 徽章 / 状态业务规则 |
| D136 | Not Found 为正常查询结果，与持久化系统故障分离 |
| D137 | Repository / Domain / Service 不依赖 Qt 表现层容器类型 |
| D138 | 实体物理删除由 Repository 执行存储移除；跨对象联动仍由 Service 协调 |
| D139 | 普通非多态实体由 Repository 直接值语义持有 |
| D140 | 普通 Repository 当前优先采用 std::vector<T> |
| D141 | 当前不维护 vector + map/unordered_map 双结构索引；未来优先评估替换主容器 |
| D142 | RuleRepository / DiaryRepository 分别维护两个无继承关系的独立集合 |
| D143 | OperationLogRepository 可直接使用普通顺序集合 |
| D144 | UserRepository 分别维护 vector<Student> / vector<Administrator>；统一访问使用短生命周期 User 抽象视图 |
| D145 | UserRepository 保证 Student / Administrator 之间 accountId 全局唯一 |
| D146 | 区分业务归属与 C++ 运行时技术存储所有权；Repository 保存唯一权威对象 |
| D147 | Repository 结构性修改后旧指针 / 引用 / 迭代器不得继续使用 |
| D148 | 单对象查询采用可空、非拥有、临时 pointer 语义；Not Found 优先 nullptr |
| D149 | 只读查询返回 const T*；受控修改才获取 T* |
| D150 | Repository 返回 pointer 只在当前调用且无结构变化期间有效 |
| D151 | UserRepository 统一查询返回 User* / const User*，保留真实动态类型 |
| D152 | 常规多对象查询默认返回临时只读对象访问集合 |
| D153 | Repository 对象访问不泄漏到 Qt 长生命周期；Service 对外返回业务值结果 |
| D154 | add/remove 不返回内部对象长期地址；稳定身份继续使用 ID |
| D155 | nullptr 仅表示正常 Not Found，不能掩盖系统级持久化错误 |
| D156 | const Repository 查询必须返回 const 访问；当前不使用 mutable 绕过业务只读边界 |
| D157 | AuthenticationService 可在 Student 专属 lastLoginAt 边界局部受控 dynamic_cast<Student*> |
| D158 | Repository 接口必须文档化返回指针 / 引用的有效期和失效操作 |
| D159 | 消费多对象 pointer 查询期间禁止对应 Repository 结构性修改 |
| D160 | Not Found 与编程 / 内部不变量错误分层；系统错误不返回普通 nullptr |
| D161 | Repository 写接口只负责数据操作和存储级不变量，不承担权限 / 业务状态机 |
| D162 | 普通实体不提供通用 update(T) / replace(T) 覆盖权威实体作为主要修改路径 |
| D163 | Repository 基础写能力收敛为新增、物理移除与持久化协作 |
| D164 | Service 不自行 saveAll，不直接操作 tmp / bak / rename，统一走 Persistence 协议 |
| D165 | 引入轻量 PersistenceCoordinator 协调运行期 Prepare / Commit |
| D166 | Repository 负责自身集合 ↔ 文件映射和 Prepare/Commit 能力；Coordinator 只协调阶段与顺序 |
| D167 | add/remove 先改运行期权威内存；只有统一 Commit 成功后 Service 才报告业务成功 |
| D168 | Prepare 失败使用受影响 Repository 的完整值语义操作前快照恢复 |
| D169 | OperationLog 与主业务状态一起进入同一次 Prepare / Commit |
| D170 | 单 Repository 状态修改也不绕过统一提交协议 |
| D171 | Service 结果区分普通业务失败、可恢复持久化失败、严重 Partial Commit 错误 |
| D172 | Partial Commit 后禁止继续正常写业务，必须进入严重持久化故障处理 |
| D173 | PersistenceCoordinator 只参与正常运行期提交，不参与启动 Repository Load |
| D174 | 操作前快照必须在第一次权威内存修改之前建立 |
| D175 | Partial Commit 以物理文件级判断；同一 Repository 多文件同样可能部分提交 |
| D176 | OperationLogService 作为子流程时不得自行独立 Commit；由主 Service 与业务状态统一提交 |

---

# 51. Gate 3.4 最终完整性终审结果

## 51.1 分层与职责

| 审计项 | 结果 |
|---|---|
| Repository 作为 Service 与 Persistence 边界 | PASS |
| Repository 粒度 | PASS |
| Domain / Service / Repository 职责分离 | PASS |
| Repository 不承载业务权限 / 排行 / 徽章 | PASS |
| Qt 不侵入纯业务 / 持久化层 | PASS |

## 51.2 对象生命周期与 C++ 所有权

| 审计项 | 结果 |
|---|---|
| 普通实体值语义存储 | PASS |
| User 多态动态类型保持 | PASS |
| stable ID 长期关系 | PASS |
| temporary pointer 短生命周期访问 | PASS |
| vector 引用失效约束 | PASS |
| const / mutable 访问边界 | PASS |

## 51.3 文件与恢复模型

| 审计项 | 结果 |
|---|---|
| 多文件 data 结构 | PASS |
| CsvCodec / CSV 正确语法边界 | PASS |
| schema_version | PASS |
| currentSemesterId 全局配置 | PASS |
| 强引用 / 审计软引用分离 | PASS |
| `.tmp/.bak` 定位 | PASS |
| fail-safe startup | PASS |

## 51.4 运行期提交模型

| 审计项 | 结果 |
|---|---|
| 操作前值语义快照 | PASS |
| 快照时序 | PASS（D174 修正） |
| Prepare / Commit | PASS |
| 物理文件级 Partial Commit | PASS（D175 修正） |
| PersistenceCoordinator 边界 | PASS |
| OperationLog 与主业务一致性 | PASS（D176 修正） |
| 单 Repository 统一提交语义 | PASS |
| 严重持久化错误分类 | PASS |

## 51.5 循环依赖与过度设计

当前没有形成：

```text
Repository → Service
Domain → Repository
PersistenceCoordinator → Business Service
```

的反向依赖。

当前也没有引入：

```text
UnitOfWork
TransactionManager
WAL
EventBus
DI Framework
Repository 接口继承森林
```

等无当前必要性的企业级机制。

### Gate 3.4 正式结论

> **Gate 3.4——Repository / Persistence：FINAL PASS。**

冻结范围：

```text
D92 ～ D176
```

核心闭环：

```text
Repository 分层
→ 文件模型
→ 运行期权威对象
→ stable ID / temporary pointer
→ const 查询边界
→ 写接口
→ 值语义快照
→ PersistenceCoordinator
→ Prepare
→ 固定物理文件 Commit
→ Partial Commit 严重故障
→ Bootstrap / Recovery
→ fail-safe startup
```


---
