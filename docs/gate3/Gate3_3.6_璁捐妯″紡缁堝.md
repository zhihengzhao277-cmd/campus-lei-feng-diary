# Gate 3.6 — 设计模式终审

> 状态：**FINAL PASS**。
>
> 下方正文逐行迁移自 v2.4 第 13222～13888 行；保留模式准入、Strategy 流程图、反模式排雷和最终审计。D242～D251 均保留在原上下文中。

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

# 60. Gate 3.6：设计模式终审

> 状态：**FINAL PASS**  
> 冻结范围：D242～D251  
> 前置依赖：Gate 3.3 Service Layer FINAL PASS、Gate 3.4 Repository / Persistence FINAL PASS、Gate 3.5 Qt Presentation FINAL PASS  
> 核心目标：准确识别当前架构中真正存在并有价值的设计模式/架构手法，同时明确拒绝为了“模式数量”而引入 State、Factory、Singleton、Command、Facade、Unit of Work 等不必要结构。

---

## 60.1 Gate 3.6 的模式审计标准

本项目不以“模式名字多”为目标。

一个模式只有在满足以下条件时才可正式认定：

```text
存在稳定、真实的设计问题
        ↓
该模式能直接解决问题
        ↓
职责分离或扩展性明显改善
        ↓
新增抽象成本可接受
        ↓
能准确解释为什么使用
        ↓
能准确解释为什么不用其他模式
```

因此：

> **“类看起来像某模式”不等于“系统采用了该模式”。**

典型误认必须避免：

```text
OperationLog
≠ Command Pattern

RecordStatus enum
≠ State Pattern

Qt Signal/Slot
≠ 自行实现 Observer Pattern

PersistenceCoordinator
≠ Unit of Work / ACID Transaction Manager
```

---

## 60.2 最终模式分类框架

本系统的模式说明正式分为四类：

```text
A. 经典 GoF 设计模式
   └── Strategy
       └── Export format serialization

B. 架构模式 / 组织手法
   ├── Repository
   ├── Service Layer
   ├── Composition Root
   └── Qt Model/View

C. Qt 框架提供、可以说明模式思想
   ├── QSortFilterProxyModel
   │   └── Proxy-style model mechanism
   └── Qt Signal/Slot
       └── Observer-style notification mechanism

D. 明确不采用
   ├── State Pattern
   ├── Factory Pattern
   ├── Singleton
   ├── Command Pattern
   ├── Facade Pattern
   ├── Adapter Pattern
   ├── Unit of Work
   ├── Generic Repository / Generic DAO
   ├── DI Framework
   └── Business EventBus / custom Observer infrastructure
```

最终报告必须保持这一分类，不把 Repository、Service Layer、Composition Root、Qt Model/View 冒充为 GoF 23 模式。

---

## 60.3 Repository Pattern

### 决策 D242

> **既有 Repository 层正式认定采用 Repository Pattern。Repository 作为 Application/Service 与文件持久化之间的数据访问边界，隐藏 CSV、物理文件、集合组织和稳定 ID 查找细节。当前只有一种文件存储后端，因此不为了模式形式引入 `IRepository<T>`、GenericRepository、Generic DAO 或多套存储实现接口。**

典型结构：

```text
VolunteerReviewService
        │
        ▼
VolunteerRecordRepository
        │
        ├── find/query by stable ID
        ├── authoritative in-memory collection
        └── persistence mapping
                 │
                 ▼
       volunteer_records.csv
```

Repository 解决的问题：

```text
Service 不需要知道：
- CSV 第几列是什么
- 文件如何编码/转义
- vector 如何组织
- tmp/bak 如何准备
```

Service 看到的是领域对象/查询接口，而不是物理存储细节。

### 为什么不增加 IRepository<T>

当前没有：

```text
CsvRepository
SqlRepository
CloudRepository
```

的真实替换需求。

因此：

```text
IRepository<T>
→ 只会增加一层接口和模板形式
→ 不产生真实替换价值
```

---

## 60.4 Export Strategy Pattern

### 决策 D243

> **Export 多格式输出正式采用 Strategy Pattern。ExportService 负责权限、业务查询、筛选、文件命名、导出用例协调，并将业务结果转换为中性的 `ExportDocument` / 等价纯 C++ 文档结构；具体格式策略只负责把中性文档结构序列化为 CSV、Markdown 等目标格式，不直接访问 Domain、Repository 或其他业务 Service。Strategy 接口保持单一、简单，不按“业务数据类型 × 格式”机械扩张，也不引入 Strategy Factory / Registry。**

### 60.4.1 为什么 Strategy 是真实需求

稳定部分：

```text
管理员权限
查询数据
应用筛选
组织导出标题/列/行
生成文件名
保存目标文件
```

变化部分：

```text
如何序列化成 CSV
如何序列化成 Markdown
```

如果全部写入 ExportService：

```text
ExportService
├── if CSV → CSV algorithm
├── if Markdown → Markdown algorithm
└── future format → more branches
```

会使 ExportService 随格式数量增长。

Strategy 将变化维度隔离：

```text
                         ExportService
                              │
                    build ExportDocument
                              │
                              ▼
                   ExportFormatStrategy
                      /              \
                     /                \
          CsvFormatStrategy     MarkdownFormatStrategy
```

### 60.4.2 中性 ExportDocument

`ExportDocument` 的架构价值是隔离：

```text
业务结果结构
vs
格式序列化算法
```

概念结构保持简单：

```text
ExportDocument
├── title / metadata
├── column headers
└── rows / cells
```

它不是富文本文档 AST，不提前引入：

```text
DocumentNode
Paragraph
Image
Style
HeadingTree
```

等无需求结构。

### 60.4.3 Strategy 完整流程图

```text
Qt ExportDialog
      │
      ▼
ExportService
      │
      ├── check ExportData permission
      ├── query Service/View Result
      ├── apply business query criteria
      ├── build ExportDocument
      └── select/use format strategy
                     │
                     ▼
             ExportFormatStrategy
              /              \
             ▼                ▼
        CSV formatter    Markdown formatter
             │                │
             └──────┬─────────┘
                    ▼
               serialized text
                    │
                    ▼
              target export file
```

### 60.4.4 Strategy 明确不负责

格式策略不负责：

```text
权限判断
业务筛选
Repository 查询
文件命名规则
OperationLog
Qt MessageBox
业务对象合法性
```

### 60.4.5 为什么不做“数据类型 × 格式”类矩阵

禁止：

```text
CsvVolunteerRecordExporter
CsvRankingExporter
CsvOperationLogExporter

MarkdownVolunteerRecordExporter
MarkdownRankingExporter
MarkdownOperationLogExporter
```

否则会造成类数量乘法增长。

策略粒度只围绕：

> **输出格式算法**

---

## 60.5 Qt Model/View Architecture

### 决策 D244

> **StudentManagementPage / OperationLogPage 正式认定采用 Qt Model/View architecture。报告中准确说明 Model、Proxy、View 的职责，不将其错误包装成“自行实现完整 MVC”。`QSortFilterProxyModel` 可说明为 Qt 提供的 Proxy-style 代理模型机制，但不单独声称系统自行实现 GoF Proxy Pattern。**

结构：

```text
Service View Result
        │
        ▼
QAbstractTableModel subclass
        │
        ▼
QSortFilterProxyModel
        │
        ▼
QTableView
```

Model：

```text
保存 Service 返回的 View Result
提供 row/column/data/header
提供 stable ID role
```

Proxy：

```text
Presentation sorting
Presentation lightweight filtering
```

View：

```text
render + selection + user interaction
```

业务规则仍在 Service。

---

## 60.6 Composition Root

### 决策 D245

> **Application/AppController 的统一对象创建与依赖装配正式认定为 Composition Root + 显式依赖传递的架构手法。Repository、Service、PersistenceCoordinator、窗口与页面依赖在应用根部集中组装；不引入 DI Framework、Service Locator 或 Singleton。**

结构：

```text
main()
  │
  ▼
Application / AppController
  │
  ├── create repositories
  ├── create persistence coordinator
  ├── create services
  ├── create LoginWindow
  └── create MainWindow / Pages when needed
```

然后：

```text
Page
→ constructor receives only needed Services
```

禁止：

```text
Page -> ServiceLocator::get()
Service -> Repository::instance()
global singleton context
```

---

## 60.7 Service Layer

### 决策 D246

> **当前 Service 层正式认定为 Service Layer。Service 用于封装应用用例、跨领域对象协调、业务授权、派生结果查询与持久化提交协调。简单查询 Service 与复杂工作流 Service 可以有不同复杂度，不要求所有 Service 形态相同，也不按按钮机械拆 Service。**

不同 Service 可以承担不同性质：

```text
VolunteerReviewService
→ workflow / governance coordination-heavy

RankingService
→ derived query / algorithm-heavy

BadgeService
→ derived rule calculation

DiaryService
→ public content workflow + like interaction

ExportService
→ application use-case coordination
```

共同点：

> Presentation 不直接操作 Repository / Domain 组合流程。

---

## 60.8 PersistenceCoordinator 不是 Unit of Work

### 决策 D247

> **PersistenceCoordinator 不认定为 Unit of Work Pattern 或数据库 Transaction Manager。它只是轻量的多 Repository / 多物理文件 prepare–commit 协调器，不自动跟踪实体 new/dirty/removed、不维护 Identity Map、不构建通用 change set，也不提供 ACID 保证。当前不为了模式命名把它扩展成完整 Unit of Work。**

实际职责：

```text
Service 已知哪些 Repository 被修改
        │
        ▼
PersistenceCoordinator
        │
        ├── Prepare affected storage
        ├── deterministic commit order
        └── detect partial commit failure
```

不是：

```text
自动发现 dirty entities
自动追踪 entity lifecycle
Identity Map
database ACID transaction
```

因此报告应表述为：

> **轻量多文件一致性提交协调机制**

而非“事务管理器”。

---

## 60.9 Qt Signal/Slot 与 Observer 思想边界

### 决策 D248

> **Qt Presentation 使用 Signal/Slot 作为框架提供的事件解耦机制，可以说明其具有 Observer 式通知思想，但不认定为系统自行实现的 Observer Pattern，也不额外建立 Subject/Observer/EventBus 基础设施。**

例：

```text
DiaryPostCard
    │
    └── likeRequested(postId)
              │
              ▼
        DiaryWallPage
```

或：

```text
QTabWidget.currentChanged
        ↓
refresh active tab
```

观察/分发机制由 Qt 提供。

当前不再写：

```text
IObserver
ISubject
notifyObservers()
BusinessEventBus
```

---

## 60.10 RecordStatus 不采用 State Pattern

### 决策 D249

> **VolunteerRecord 生命周期继续使用 `enum class RecordStatus + 受控状态转换`，不采用 State Pattern。当前状态数量少、转换规则稳定、状态专属行为有限，引入独立 State 类体系会造成类膨胀和额外间接层。只有未来状态行为显著复杂化时才重新审计 State Pattern。**

当前：

```text
Pending
├── Approved
├── Rejected
└── Withdrawn

Rejected
└── Pending
```

没有必要演化成：

```text
RecordState
├── PendingState
├── ApprovedState
├── RejectedState
└── WithdrawnState
```

这是“状态机存在，但不强套 State Pattern”的典型合理取舍。

---

## 60.11 User 创建/恢复不采用 Factory Pattern

### 决策 D250

> **当前不采用 Factory Pattern 创建/恢复 User。Student / Administrator 类型少、来源文件和恢复路径明确，由 UserRepository 直接恢复具体类型。只有未来用户类型显著增加或统一动态反序列化需求出现时才重新审计 Factory。**

当前：

```text
students.csv
→ Student

administrators.csv
→ Administrator
```

不存在复杂动态类型选择。

因此 `UserFactory` 只会增加一层 switch 包装。

---

## 60.12 AppController 不采用 Facade Pattern

### 决策 D251

> **AppController / Composition Root 不认定为 Facade Pattern。它只负责对象装配和应用生命周期，不向 Presentation 聚合全部业务操作。Page 继续显式依赖所需 Service，避免形成统一业务 Facade / God Object。**

错误方向：

```text
AppFacade
├── submitRecord()
├── reviewRecord()
├── queryRanking()
├── manageStudent()
├── likePost()
└── exportData()
```

这会把 Service Layer 再次汇聚成 God Class。

---

## 60.13 其他模式反向排雷

以下当前不认定：

### OperationLog ≠ Command Pattern

OperationLog 是：

```text
操作完成后的不可变审计事实
```

不是：

```text
Command
├── execute()
└── undo()
```

### DiaryPostPublicView 组装 ≠ Adapter Pattern

它是 Application Query Result assembly，不存在两个不兼容接口需要适配。

### PasswordHasher ≠ 第二个 Strategy Pattern

当前只有一个真实密码算法实现方向，不存在多算法运行时替换需求。

### Ranking ≠ Strategy

月度 / 学期 / 总榜使用相同排序算法，只是统计时间范围不同。

### BadgeRule ≠ Strategy

门槛参数可配置属于 parameterized rule，不代表算法策略可替换。

### Repository ≠ Generic DAO

Repository 按领域职责组织，不退化成 `CRUD<T>`。

---

## 60.14 Gate 3.6 模式地图

```text
                         Gate 3.6
                            │
       ┌────────────────────┼─────────────────────┐
       │                    │                     │
       ▼                    ▼                     ▼
经典 GoF             架构模式/手法        Framework mechanism
       │                    │                     │
       ▼                    ├── Repository        ├── Proxy-style
   Strategy                 ├── Service Layer     │   QSortFilterProxyModel
   Export                    ├── Composition Root │
                             └── Qt Model/View     └── Observer-style
                                                   Qt Signal/Slot

                            │
                            ▼
                      Explicitly rejected
                            │
         ┌──────────────────┼────────────────────┐
         ▼                  ▼                    ▼
       State              Factory              Singleton
       Command            Facade               Unit of Work
       Adapter            Generic DAO          EventBus
```

---

## 60.15 Gate 3.6 FINAL AUDIT

| 审计项 | 结论 |
|---|---|
| Repository 使用真实 | PASS |
| Strategy 有真实算法替换需求 | PASS |
| ExportDocument 防止业务类型×格式爆炸 | PASS |
| Strategy 与业务查询职责分离 | PASS |
| Service Layer 定性准确 | PASS |
| Composition Root 定性准确 | PASS |
| Qt Model/View 定性准确 | PASS |
| Proxy 认定不过度 | PASS |
| Signal/Slot 认定不过度 | PASS |
| PersistenceCoordinator 未误认 UoW | PASS |
| OperationLog 未误认 Command | PASS |
| RecordStatus 未强套 State | PASS |
| User 创建未强套 Factory | PASS |
| AppController 未强套 Facade | PASS |
| 无 Singleton / Service Locator | PASS |
| 无 Generic Repository | PASS |
| 无 EventBus 过度设计 | PASS |
| 模式分类清晰 | PASS |
| 报告可准确解释 | PASS |
| 答辩可解释性 | PASS |

最终：

```text
Gate 3.6
Design Pattern Audit
→ D242～D251
→ FINAL PASS
```

推荐答辩总起句：

> **本系统正式采用 1 个经典 GoF 行为模式——Strategy，用于隔离不同导出格式的序列化算法；同时采用 Repository、Service Layer、Composition Root 和 Qt Model/View 等架构模式或组织手法。Qt 的 Signal/Slot 与 QSortFilterProxyModel 分别体现框架提供的观察通知和代理模型思想。对于 State、Factory、Command、Singleton 等模式，经过业务适用性审计后没有为了增加模式数量而强行引入。**

---

> **实现期模式约束 → Gate 4**
>
> 本文已经冻结当前允许声明的设计模式集合。实现期如认为需要新增 Pattern，不得由执行层自行增加。
>
> 应先读取：
> `Gate4_00_实现审计总览专题索引与讨论停点规则_候选冻结稿(1).md`
>
> 只有在真实实现产生明确架构冲突并完成显式 reopen 后，才允许调整。
