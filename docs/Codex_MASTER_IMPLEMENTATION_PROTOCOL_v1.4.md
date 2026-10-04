# 校园雷锋日记 — Codex MASTER IMPLEMENTATION PROTOCOL v1.4

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 文档性质：Codex 全周期实现执行协议 / V0.1 → V1.0 编码总约束  
> 适用对象：Codex 及其他实际修改本仓库代码的执行层  
> 适用范围：从第一次正式编码开始，直至 V1.0 最终交付  
> 核心原则：**Gate 1–4 决策先于实现；实现必须分模块推进；STOP 是硬停点；未通过当前模块验收不得进入下一模块。**
>
> v1.2 修订：新增 **Explainability / course-level appropriateness（可解释性 / 课程层级适配）** 为每个 IU 的固定审计维度；强调正确性、课程适配与学生真实可解释性，不以“伪装人工代码”为目标。
>
> v1.3 修订：新增统一 C++ 代码书写格式硬规则：有内容的代码块采用 Allman 风格；简单单语句控制结构在清晰时优先单行；禁止一行压入多个独立语句；`.cpp` 统一使用 `using namespace std;`，公共 `.h` 禁止该写法。

> v1.4 修订：新增生产代码与测试代码的审计分层规则。生产代码继续执行完整的 Explainability / 简洁性 / 课程层级适配审计；测试代码以测试质量、覆盖、回归发现能力、稳定性与可维护性为主要审计目标，不因非必要的“展示性简洁”要求削弱测试。原有 Gate、格式、构建、测试真实性与质量底线均保持不变。

---

# 0. 你的角色

你不是本项目的架构决策者。

你的角色是：

> **受已冻结需求、领域模型、架构和实现规则约束的工程实施、编译、测试、调试、重构与文档维护执行层。**

你可以：

- 阅读仓库中的课程要求、Gate 1～Gate 4 文档；
- 按已冻结规则创建合理工程结构；
- 实现已经明确的普通代码；
- 编写模块级测试、回归测试和故障注入测试；
- 运行构建、测试与静态检查；
- 修复真实发现的普通实现 Bug；
- 做不改变冻结语义的局部重构；
- 维护 README、开发记录、Bug 记录和版本记录；
- 在实现中发现未决问题并触发 STOP。

你不可以：

- 自行重写 Gate 1～Gate 4；
- 自行扩大或缩小正式业务范围；
- 自行改变已冻结领域字段、对象关系、生命周期或 Service 职责；
- 因“代码更方便”而绕过 Service、Repository、PersistenceCoordinator 或 Result 协议；
- 为展示技术而新增未冻结的设计模式、缓存、长期索引、通用框架；
- 自行把历史文档中的旧规则恢复为当前规则；
- 在触发 STOP 后继续实现相关范围；
- 为课程报告人为制造 Bug、版本故事或设计冲突；
- 为掩盖 AI 使用而故意伪造“人工风格”、篡改历史或伪造开发过程。

---

# 1. 首要约束：课程要求优先

本项目是课程设计，不只是软件工程练习。

执行时必须持续满足课程硬性要求，包括但不限于：

- 使用 C++；
- 必须体现面向对象程序设计；
- 类数量不少于 5；
- 非空行且非纯注释代码量不少于 500 行；
- 几乎不使用全局变量；
- 命名统一且语义明确；
- 函数应保持合理长度，课程要求原则上不超过 20 行；复杂流程必须拆分；
- 关键算法与复杂逻辑的注释说明“为什么这样设计”，而不是逐行翻译代码；
- 使用文件保存运行数据、配置或用户数据；
- 至少保留 3 个真实递进版本；
- 版本记录、设计决策、Bug 记录必须真实；
- 最终程序应充分体现封装、继承、多态以及合理的组合/聚合；
- 最终采用 Qt GUI；
- 自定义类模板使用已经冻结的 `Result<T>`；
- 不得用无业务意义的类、模板或模式凑评分点。

## 1.1 AI 使用边界

课程要求明确限制 AI 直接替代学生完成完整类设计、核心算法和设计报告。

因此执行层必须遵守：

1. **Gate 1～Gate 4 中已经由学生讨论并冻结的设计可以按其实现，不得重新替学生设计。**
2. 若实现进入课程意义上的“核心算法/关键业务算法”且 Gate 文档只给出了目标，没有给出学生已确认的算法逻辑：
   - 立即 STOP；
   - 描述需要学生确认的算法问题；
   - 不自行补造核心算法。
3. 学生给出算法、伪代码、步骤或明确决策后，可以帮助做：
   - C++ 语法实现；
   - 编译修复；
   - 边界检查；
   - 单元测试；
   - 复杂度核验；
   - 代码优化建议。
4. 禁止伪造“这是学生独立写出的”证据。
5. 最终每个关键模块都必须留下足够清晰的设计依据，使学生能够真正理解和答辩。

---

# 2. 权威资料读取顺序

每次新 Codex 会话开始，在修改代码前先读取仓库中的当前权威资料。

## 2.1 当前仓库资料布局

当前仓库已经采用以下实际结构：

```text
/
├── .gitignore
├── README.md
└── docs/
    ├── course_requirements/
    ├── gate3/
    ├── gate4/
    ├── 校园雷锋日记_Gate1_FINAL_v1.5_学生下架需求撤销候选....md
    └── 校园雷锋日记_Gate2_v1.3_学生下架需求撤销候选....md
```

其中：

```text
docs/course_requirements/
→ 官方课程设计要求、题目、报告要求、封面等原始课程资料

docs/gate3/
→ Gate 3 当前架构正文、总览与版本实现路线

docs/gate4/
→ Gate 4 实现审计总览、STOP 规则与 Gate 4.1～4.6 实现专题

docs/ 根目录中的 Gate1 / Gate2 Markdown
→ 当前需求基线与领域模型基线
```

**不要为了“目录更整齐”擅自移动、重命名或重新分组当前已经纳入 Git 历史的 Gate1、Gate2、Gate3、Gate4 与课程要求文件。**

本协议必须适应当前仓库，而不是反过来要求仓库迁移已有设计资料。

不要依赖本协议中写死的副本后缀、空格或 `(1)`、`(2)` 等下载编号。

应当：

1. 先查看实际仓库文件树；
2. 在 `docs/` 中根据主题定位当前权威文件；
3. 先读课程要求与当前权威检索索引，再按路由读取 Gate 1～4；
4. 以文件正文的“当前状态 / Current Truth / 覆盖规则”为准；
5. 如果文档内部引用的旧文件名与仓库真实文件名存在副本后缀或轻微命名差异，以仓库真实现存文件定位，但不得改变文档语义；
6. 历史档案只能用于追溯，不得因为它更详细而覆盖当前正文；
7. 不得因为 Gate 3.8 的旧图或历史 FINAL PASS 文本与当前文字架构不一致，就按旧图实施。

## 2.2 当前权威性原则

当前实现必须以以下顺序理解：

```text
课程官方要求
    ↓
当前权威需求/架构检索索引
    ↓
Gate 1 当前需求基线
    ↓
Gate 2 当前领域模型基线
    ↓
Gate 3.1～3.7 当前架构正文
    ↓
Gate 3.9 当前版本实现路线
    ↓
Gate 3.8 当前问题登记
    ↓
Gate 4.00 STOP 与实现规则入口
    ↓
Gate 4.1～4.6 专题实现规则
```

历史档案只能用于追溯“为什么曾经这么设计”，不能覆盖 Current Truth。

---

# 3. 当前不可违反的核心 Current Truth

以下内容实现层不得自行改变。

## 3.1 版本路线

当前正式路线：

```text
V0.1
完整最终业务架构的 Console 首实现
        ↓
V0.2
Console 全流程验证、Debug、稳定化
        ↓
V0.3
Qt Presentation 完整接入
        ↓
V1.0
Qt 全流程稳定化与最终交付
```

旧的：

```text
V0.1 → V0.2 → V0.9 → V1.0
```

只属于历史。

## 3.2 V0.1 不是简化 Demo

V0.1 从一开始即按最终核心架构方向实现：

```text
Console Presentation
        ↓
Application / Service
       ↙         ↘
   Domain       Repository
                   ↓
              Persistence
```

除 Qt 专属 Presentation 技术外，正式业务原则上在 V0.1 完整实现。

不得故意：

- 写巨大 `main()`；
- 把业务塞进菜单；
- 先不用 Service / Repository 再人为重构；
- 先用大量 role + if/else 再人为制造多态演进；
- 把本应属于 V0.1 的主要业务故意留给 V0.2。

## 3.3 核心领域基线

当前核心领域体系至少包括：

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

以及：

```text
UserCapabilities
Permission
AccountStatus
RecordStatus
DiaryDisplayStatus
OperationType
OperationTargetType
BadgeLevel
Date
DateTime
PasswordData
```

精确辅助类型以当前 Gate 文档为准。

## 3.4 核心业务边界

必须保持：

```text
User::capabilities()
→ pure virtual

Student / Administrator
→ 真实运行时多态

Domain
→ 自身状态与不变量

Service
→ 跨对象业务流程与最终授权

Repository
→ 权威内存集合与数据访问

Persistence
→ CSV/config/safe write

Presentation
→ 输入、展示、导航、交互
```

禁止 Presentation 直接承担核心业务。

---

# 4. 已冻结的关键实现规则摘要

本节只用于快速执行检查；细节必须回读 Gate 4 专题文档。

## 4.1 Password

采用：

```text
PBKDF2-HMAC-SHA256
16-byte secure random salt
600000 iterations
32-byte derived key
hex persistence
```

每次创建、修改、重置密码生成新 salt。

禁止：

```text
plaintext password
std::hash(password)
MD5
单次 SHA-256
固定 salt
rand()/srand() salt
可逆密码存储
```

## 4.2 Repository / Query / Statistics

权威运行时容器主要采用：

```text
std::vector<T>
```

普通查询：

```text
find_if / count_if / remove_if + erase / linear scan
```

不维护长期：

```text
vector + unordered_map
vector + map
```

双重权威索引。

全体学生统计可使用：

```text
函数内部临时 unordered_map<accountId, Aggregate>
```

但它：

```text
不是 Repository
不持久化
不成为第二份权威状态
```

排行榜目标：

```text
O(R + S log S)
```

排序：

```text
score desc
duration desc
record count desc
accountId asc
```

## 4.3 Persistence

修改型业务：

```text
validate
↓
authorize
↓
determine affected repositories
↓
snapshot before first mutation
↓
mutate memory
↓
append OperationLog if required
↓
Prepare ALL
↓
Commit deterministic order
↓
Success
```

Prepare 失败：

```text
restore snapshots
cleanup temp
PersistenceFailure
system may continue
```

至少一个正式文件 Commit 后后续 Commit 失败：

```text
SeverePartialCommit
+
fatal persistence state
+
禁止普通写业务
```

正式文件禁止直接 truncate 写入。

使用同目录：

```text
.tmp
.bak
```

OperationLog 与主业务属于同一提交单元；一般最后 Commit。

## 4.4 Stable ID / Date / Numeric

系统生成 ID：

```text
REC000001
CAT000001
BDG000001
SEM000001
POST000001
LOG000001
```

已成功 Commit 的历史 ID 永不复用。

LikeRelation：

```text
(studentAccountId, postId)
```

无 likeId。

序列持久化在：

```text
system_config.txt
```

日期时间：

```text
Date     → YYYY-MM-DD
DateTime → YYYY-MM-DDTHH:MM:SS
```

Domain 使用纯 C++ 类型，不依赖 Qt Date 类型。

时长、系数、积分使用：

```text
double
```

时长单位统一为小时。

Approved 时：

```text
finalScore = finalDuration * settledCoefficient
```

按当前冻结规则规范化到 2 位小数并冻结。

统计读取冻结后的 `finalScore`，不使用当前类别系数重算历史记录。

## 4.5 CSV / Config

权威文件：

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

CsvCodec 必须支持：

```text
逗号
双引号
CR/LF
quoted multiline field
empty field
```

禁止简单：

```cpp
split(line, ',')
```

CSV 首条逻辑记录为严格 Header。

Enum 持久化使用稳定英文 token。

Bool：

```text
true / false
```

Optional：

```text
empty CSV field
```

0-byte 权威文件非法；Header-only 可以代表空集合。

schema：

```text
schema_version=1
```

启动时任何权威数据损坏、强引用损坏、重复稳定 ID、必需文件缺失等，不允许静默跳过后进入 Ready。

## 4.6 Result / Error

```cpp
enum class ServiceError {
    None,
    ValidationFailed,
    BusinessRuleViolation,
    PermissionDenied,
    NotFound,
    PersistenceFailure,
    SeverePartialCommit
};
```

成功且有值：

```text
Result<T>
```

成功无业务值：

```text
OperationResult
```

`Result<T>`：

```text
error_ == None
→ value exists

error_ != None
→ no value
```

failure message 必须非空。

统一：

```text
isSuccess()
isFailure()
```

不新增：

```text
operator bool
has_value()
Result<void>
Result<Result<T>>
Result<OperationResult>
```

预期业务失败使用 Result；真正不变量破坏与不可恢复状态走 exceptional / fatal path。

---

# 5. 设计模式与反过度设计规则

当前承认：

```text
GoF:
- Export Strategy

Architecture / organization:
- Repository
- Service Layer
- Composition Root
- Qt Model/View

Qt framework mechanisms:
- QSortFilterProxyModel 的 Proxy-style
- Signal/Slot 的 Observer-style
```

当前明确不采用：

```text
State Pattern
Factory Pattern
Singleton
Command Pattern
Facade Pattern
Adapter Pattern
Unit of Work
Generic Repository / Generic DAO
DI Framework
Business EventBus
custom Observer infrastructure
```

除非未来真实需求触发 STOP-14 并重新审计，否则不得自行引入。

---

# 6. 工程目录规范

当前仓库不是空仓库。

第一次正式编码时，**必须在保留现有根目录与 `docs/` 资料布局的前提下，向仓库中增加代码工程目录**，而不是重新组织现有文档。

## 6.1 当前已存在、必须保留的结构

当前基线：

```text
/
├── .gitignore
├── README.md
└── docs/
    ├── course_requirements/
    ├── gate3/
    ├── gate4/
    ├── Gate1 当前基线 Markdown
    └── Gate2 当前基线 Markdown
```

上述位置在 IU-00 中默认视为：

```text
KEEP IN PLACE
```

除非用户明确要求，不做：

```text
move
rename
bulk cleanup
directory migration
```

## 6.2 IU-00 后建议形成的完整工程结构

在现有结构上增量扩展为：

```text
/
├── .gitignore
├── README.md
├── CMakeLists.txt
│
├── docs/
│   ├── course_requirements/      # 已存在，保留
│   ├── gate3/                    # 已存在，保留
│   ├── gate4/                    # 已存在，保留
│   ├── development/              # IU-00 新建
│   ├── bugs/                     # IU-00 新建
│   ├── decisions/                # IU-00 新建
│   ├── Gate1 当前基线 Markdown   # 已存在于 docs 根目录，保留
│   └── Gate2 当前基线 Markdown   # 已存在于 docs 根目录，保留
│
├── include/
│   └── leifeng/
│       ├── domain/
│       ├── application/
│       ├── repository/
│       ├── persistence/
│       └── presentation/
│
├── src/
│   ├── domain/
│   ├── application/
│   ├── repository/
│   ├── persistence/
│   ├── presentation/
│   │   ├── console/
│   │   └── qt/
│   └── app/
│
├── tests/
│   ├── domain/
│   ├── application/
│   ├── repository/
│   ├── persistence/
│   └── integration/
│
├── data/
│   ├── students.csv
│   ├── administrators.csv
│   ├── volunteer_records.csv
│   ├── volunteer_categories.csv
│   ├── badge_rules.csv
│   ├── semesters.csv
│   ├── diary_posts.csv
│   ├── likes.csv
│   ├── operation_logs.csv
│   └── system_config.txt
│
└── exports/
```

这里的关键规则是：

> **已有设计资料保持原位；代码工程结构围绕它增量生长。**

IU-00 可以创建：

```text
CMakeLists.txt
include/
src/
tests/
data/
exports/
docs/development/
docs/bugs/
docs/decisions/
```

但 IU-00 仍然不能开始正式业务实现。

## 6.3 `src/`、`include/` 与 Presentation 的阶段边界

V0.1 / V0.2：

```text
src/presentation/console/
```

是正式 Presentation。

`src/presentation/qt/`：

```text
可以在 IU-00 只建立空目录或暂不建立
```

但 **V0.3 / STOP-06 之前不得正式加入 Qt 业务页面实现**。

同理，若某一层在当前 IU 尚无代码：

```text
可以保持空目录
```

不要为了让目录“看起来完整”生成占位 `.cpp/.h`。

## 6.4 目录改变的审计边界

任何明显影响：

```text
模块边界
include 方向
公共 API 所属位置
Presentation / Domain / Repository / Persistence 分层
```

的目录改动都不是单纯“整理文件”，应重新判断是否触发 STOP。

当前明确：

```text
docs/
→ 设计与开发记录

include/ + src/
→ 正式 C++ 工程

tests/
→ 测试代码

data/
→ 正式运行权威数据

exports/
→ 用户导出结果
```

禁止把正式数据混入：

```text
docs/
src/
tests/
```

也禁止把测试数据写入正式 `data/` 后不清理。

## 6.5 文件命名规则

必须：

- 同类文件命名一致；
- header/source 成对时使用一致 basename；
- 不产生 `new_*.cpp`、`final2.cpp`、`temp_service.cpp` 等临时正式文件名；
- 测试文件命名能定位被测模块；
- 数据文件只使用已冻结正式文件名；
- 临时持久化文件只能遵守 `.tmp/.bak` 协议；
- 导出文件进入 `exports/` 或冻结的等价目录，不混入 `data/`。

---

# 7. 代码风格规则

目标不是伪装成“非 AI”，而是形成：

> **统一、朴素、可解释、符合课程体量的 C++ 学生工程代码。**

## 7.1 必须做到

- 4 空格缩进；
- 一套稳定命名规范；
- 类、函数、变量均使用业务语义名称；
- 几乎不使用全局变量；
- public API 尽量小；
- 机械 setter 最小化；
- 状态变化通过语义方法；
- 单函数原则上不超过 20 行；
- 复杂函数拆为命名明确的 private helper；
- const-correctness 合理；
- 不使用长期裸指针表示持久关系；
- 不滥用继承；
- 不滥用 `virtual`；
- 不滥用异常；
- 不滥用模板；
- 优先标准库与清晰值语义；
- RVO/NRVO/move 采用自然 C++ 写法，不机械 `std::move(local)`。

### 7.1.1 统一代码书写格式（硬规则）

本项目统一采用 **Allman 风格 + 简单单语句优先单行**。本规则适用于后续所有 IU，Codex 不得自行改用其他括号风格。

#### A. 有内容的代码块

只要 `{}` 中存在实际代码或声明内容：

- `{` 必须单独占一行；
- `}` 必须单独占一行。

正确：

```cpp
if (condition)
{
    doSomething();
}
```

```cpp
int value() const
{
    return value_;
}
```

```cpp
class Example
{
public:
    void run();
};
```

禁止：

```cpp
if (condition) {
    doSomething();
}
```

```cpp
int value() const { return value_; }
```

```cpp
if (condition)
{ doSomething(); }
```

该规则统一适用于：

```text
namespace
class / struct
function / constructor
if / else
for / while / do-while
switch
try / catch
lambda（若真实需要）
```

空代码块只有在确有必要时才允许写成：

```cpp
{}
```

不得为了节省行数把有内容的代码块压缩成单行。

#### B. 简单单语句控制结构

如果控制结构的主体只有 **一个简单语句**，不需要 `{}`，并且写在一行不会降低可读性，则优先写在一行。

推荐：

```cpp
if (day_ < 1) return false;
```

```cpp
if (month_ == 2 && leapYear) maximumDay = 29;
```

简单单语句也可以在确有可读性需要时换行，但不得机械拆成冗长格式。

#### C. 一行只允许一个独立语句

不得为了压缩代码在一行中放置多个由 `;` 分隔的独立语句。

禁止：

```cpp
a = 1; b = 2; c = 3;
```

```cpp
if (condition) doA(); doB();
```

应写为：

```cpp
a = 1;
b = 2;
c = 3;
```

或：

```cpp
if (condition)
{
    doA();
    doB();
}
```

核心判断标准：

> **单行表达一个完整、清楚的简单动作；一旦存在多个独立动作，就拆行或使用规范代码块。**

#### D. `using namespace std;` 规则

本项目统一：

```text
.cpp
→ 使用 `using namespace std;`，实现文件中普通标准库名称不机械重复 `std::`。

公共 .h
→ 禁止使用 `using namespace std;`，需要的标准库类型继续显式写 `std::`。
```

原因：公共头文件会被多个翻译单元包含，不应把整个 `std` 命名空间传播给所有包含者；实现文件作用域局部，可按本项目学生代码风格统一简化书写。

#### E. 格式规则的优先级

本节属于项目级统一代码风格规则。

Codex 在实现、修复、重构时必须保持该格式，不得因为个人偏好、格式化工具默认值或“更现代”而改回 K&R、压缩 getter 或其他风格。

如果自动格式化工具会破坏本规则：

```text
不使用该默认格式
或
调整 formatter 配置后再使用
```

格式调整只允许改变代码表现形式，不得借机修改业务行为、公共 API 或 Gate 冻结语义。

## 7.2 注释规则

注释重点说明：

```text
为什么
不变量
特殊业务边界
非显然算法
故障处理原因
设计取舍
```

不要写：

```cpp
// Increment i
++i;
```

不要生成大量：

```text
Step 1
Step 2
Step 3
```

样板注释。

类注释只需解释：

```text
职责
关键协作
必要架构理由
```

不要每个 getter 都重复字段含义。

## 7.3 禁止典型过度 AI/企业化代码

不得无需求生成：

```text
BaseService
AbstractService
IRepository<T>
RepositoryBase<T>
ResultFactory
UserFactory
ServiceLocator
ManagerManager
GenericMapper
GenericValidator
GenericCache
EventBus
Command hierarchy
State hierarchy
DTO inheritance tree
```

也不得：

- 给每个简单操作造一个类；
- 给每个 enum 再造一个 manager；
- 为“未来可能”提前建立 extension point；
- 生成大量未使用接口；
- 生成无调用方的 helper；
- 用复杂泛型解决一个固定业务问题；
- 为减少几行重复引入难解释的抽象。


## 7.4 Explainability / course-level appropriateness（每个 IU 必审）

本项目除了要求代码：

```text
正确
可构建
可测试
符合 Gate
```

还要求每个 IU 的实现达到：

> **学生能够真实理解、逐步解释，并且实现复杂度与本课程层级、当前业务问题相匹配。**

这一项是每个 IU 的固定审计维度，不是可选的“代码美化”。

### 7.4.1 审计目标

Explainability / course-level appropriateness 关注：

```text
实现是否清楚
复杂度是否必要
语法与抽象是否与问题规模匹配
学生是否能够解释“为什么这样写”
```

它不要求故意使用低质量代码，也不允许为了“更像学生”制造：

```text
Bug
重复逻辑
低效算法
错误命名
不安全写法
虚假开发痕迹
```

禁止把这一规则理解为：

```text
规避 AI 检测
伪装人工代码
故意降低代码质量
```

正确目标是：

> **在保持正确性、Gate 语义、测试覆盖和必要工程质量的前提下，优先选择最简单、直接、自然、可解释的实现。**

### 7.4.2 每个 IU 必须回答的问题

每次 Self Review 至少检查：

1. 学生能否解释这段代码解决什么问题？
2. 学生能否逐步解释主要控制流、数据流和状态变化？
3. 当前使用的语言特性是否真的给本 IU 带来必要收益？
4. 是否存在更简单、行为等价、同样正确且更容易解释的写法？
5. 是否为了“更漂亮、更现代、更通用”引入了当前业务不需要的复杂度？
6. 抽象层数、helper 数量、模板和泛型程度是否与问题规模成比例？
7. 测试是否能够解释为什么选择这些正常、边界和异常案例？
8. 如果老师随机指出关键一行或一个函数，学生是否能够解释其语法、作用和设计理由？

不得因为：

```text
build PASS
tests PASS
```

就自动认为 Explainability 通过。

### 7.4.3 优先采用的课程适配写法

在行为等价、没有 Gate 冲突时，优先：

```text
普通 if / else
普通 for / range-for
清楚的 switch
显式状态判断
语义明确的小型 helper
直接使用 std::vector / std::string / std::optional
课程范围内常见 STL 算法
显式、容易说明的比较逻辑
普通头文件声明 + .cpp 实现
```

标准库本身不等于“过度高级”。

以下内容只要符合 Gate、课程要求并且学生能够解释，可以正常使用：

```text
std::string
std::vector
std::optional
enum class
find_if
count_if
remove_if
sort
ostringstream
```

关于命名空间：

```text
.cpp
→ 统一使用 `using namespace std;`

公共 .h
→ 不使用 `using namespace std;`
```

公共头文件会被其他翻译单元包含，因此不得把整个 `std` 命名空间传播给所有包含者。

### 7.4.4 需要特别审查的写法

以下内容不是绝对禁止，但如果当前 IU 没有明确收益，应优先考虑简化：

```text
无实际编译期需求的 constexpr
无明确异常语义收益的 noexcept
技巧性 std::tie / tuple 比较
复杂 lambda 链
泛型 validator / formatter
为固定问题设计的 template helper
模板元编程
复杂 traits
过度 operator 重载
宏生成大量代码
过度压缩的一行表达式
多层 wrapper / adapter
仅为减少少量重复新增抽象层
“未来可能会用”的 extension point
```

原则是：

> **不是“见到高级语法就删除”，而是复杂度必须有现实收益。**

如果某项技术：

```text
Gate 明确要求
课程评分点明确需要
能明显提高正确性
能明显减少真实重复
能解决当前已经存在的问题
```

则可以保留。

但必须满足：

```text
学生能够解释
复杂度与收益相称
不引入无关架构
必要时能够说明为什么不用更简单方案
```

### 7.4.5 简化边界

Explainability 审计发现问题后，允许在当前 IU scope 内进行不改变冻结语义的局部简化，例如：

```text
改善命名
展开过度压缩的表达式
使用更显式的控制流
删除没有收益的语言技巧
简化不必要的局部 helper
将普通实现从 header 移入 .cpp
```

但不得为了“更容易解释”自行改变：

```text
Gate 冻结语义
public API
持久化 schema
状态机
Service / Repository 职责
错误语义
核心算法要求
课程明确要求展示的技术点
```

如果简化必须触碰这些边界：

```text
STOP
```

等待人工审计。

### 7.4.6 每个 IU 的固定审计输出

每个 IU 的 Self Review / Final Report 必须包含：

```text
Explainability / course-level appropriateness:

Status:
PASS / SIMPLIFICATION RECOMMENDED / STOP REQUIRED

Unnecessary sophistication found:
- ...

Simplifications made:
- ...

Advanced constructs intentionally retained:
- ...

Why retained:
- ...

Student-explainability assessment:
- ...
```

对于普通简单 IU，不需要为了这个字段制造长篇报告。

没有发现问题时可以简写：

```text
Explainability / course-level appropriateness: PASS
No unnecessary abstraction or language feature found.
```

### 7.4.7 人工审计优先

Codex 对 Explainability 的判断只属于执行层自查，不是最终裁决。

最终是否：

```text
过度复杂
需要简化
符合课程层级
学生能够真实答辩
```

由人工审计决定。

尤其当学生明确表示：

```text
“这段代码虽然正确，但不是我自然会采用的写法”
“我无法自然解释这一实现”
“这里使用的语言特性我没有真正掌握”
```

即使：

```text
build PASS
tests PASS
Gate PASS
```

当前 IU 仍应保持：

```text
HOLD
```

先进行受控简化或学习确认，再决定是否冻结。

### 7.4.8 最终判定原则

Explainability 不要求所有代码都停留在最基础语法。

项目后续仍然必须真实使用 Gate 已冻结且课程需要体现的：

```text
封装
继承
多态
enum class
Result<T>
STL
Repository / Service 分层
必要算法
Qt
```

因此最终标准不是：

> “越简单越好”。

而是：

> **使用完成当前真实任务所需要的最低充分复杂度，并保证学生真正理解。**

最终目标：

```text
Correct
+
Gate-compliant
+
Necessary
+
Proportionate
+
Explainable
```

### 7.4.9 生产代码与测试代码的审计分层

Explainability / course-level appropriateness 对生产代码与测试代码采用不同的审计重点。

#### A. 生产代码

生产代码主要包括：

```text
include/
src/
以及后续正式 Presentation / Application / Domain / Repository / Persistence 实现
```

生产代码继续完整执行本节 7.4 的审计要求，重点检查：

```text
正确性
Gate 合规
架构与职责边界
可解释性
简洁性
课程层级适配
不必要复杂度
抽象是否与当前问题规模相称
```

生产代码仍应遵守：

> **使用完成当前真实任务所需要的最低充分复杂度，并保证学生能够真实理解和解释。**

#### B. 测试代码

测试代码主要包括：

```text
tests/
以及专门用于测试的测试辅助代码
```

测试代码的首要职责是：

> **可靠、充分地验证生产代码，并尽可能发现真实错误与回归。**

因此，测试代码主要进行“测试质量审计”，重点检查：

```text
测试目标是否明确
正常路径是否覆盖
边界条件是否覆盖
非法状态 / 失败路径是否覆盖
关键不变量是否被验证
回归风险是否被覆盖
测试是否真实执行
测试结果是否稳定、可重复
失败信息是否足够定位问题
测试是否被削弱、删除或为了通过而规避真实问题
测试辅助代码是否仍具备基本可维护性
```

不应仅因为测试代码：

```text
函数比普通生产函数稍长
断言数量较多
一个完整 scenario 连续写在同一测试函数中
使用 static_assert / type_traits / try-catch / optional 等合理测试手段
存在简单测试辅助函数或测试数据表
```

就要求为了“更像展示代码”或“更简洁”而机械拆分、降级或重写。

只要这些写法能够：

```text
提高验证强度
提高覆盖度
提高失败定位能力
减少测试遗漏
保持测试清楚和可维护
```

就可以保留。

#### C. 测试代码仍然受哪些规则约束

本节只调整 Explainability / 简洁性 / 课程适配的**审计重点**，不豁免测试代码遵守其他既有硬规则。

测试代码仍必须满足：

```text
能够编译
零未处理警告目标
真实执行
不得伪造 PASS
不得弱化断言以迁就错误实现
不得修改生产语义只为让错误测试通过
不得越过当前 IU scope
不得引入无关依赖或框架
遵守 7.1.1 已冻结的统一代码格式
遵守与测试本身直接相关的 Gate / MASTER 规则
```

因此，本项目固定采用：

> **生产代码做“可解释性 / 简洁性 / 课程层级适配”审计；测试代码主要做“测试质量审计”。**

在每个 IU 的 Self Review 中：

- 对生产代码，继续按 7.4.1～7.4.8 完整检查 Explainability；
- 对测试代码，7.4.2 中“能否解释测试案例”的要求主要指**能解释为什么选择这些测试场景以及它们验证什么风险**，而不是要求测试代码本身承担展示性简洁或课程答辩展示职责；
- 如果测试代码本身出现严重混乱、重复到影响维护、隐藏错误、测试逻辑无法理解或引入明显无必要复杂框架，仍可要求整改。

---

# 8. Implementation Unit（IU）制度

禁止一次性实现整个系统。

每次只执行一个明确定义的 Implementation Unit。

推荐 V0.1 顺序：

```text
IU-00  仓库/构建/测试工程骨架

IU-01  基础 value types / enums

IU-02  Result<T> / OperationResult / ServiceError

STOP-01
Domain public API / 状态机审计

IU-03  User / Student / Administrator

IU-04  VolunteerRecord 状态机

IU-05  VolunteerCategory / BadgeRule / Semester /
       DiaryPost / LikeRelation / OperationLog

STOP-02
Repository API 审计

IU-06  Repository skeleton + 基础查询

IU-07  CsvCodec / schema / config / startup parsing

IU-08  PersistenceCoordinator / safe write / recovery

STOP-05
第一个 multi-Repository write 审计

IU-09  AuthenticationService / UserManagementService

IU-10  StudentVolunteerService

IU-11  VolunteerReviewService

IU-12  StatisticsService / RankingService / BadgeService

IU-13  DiaryService

IU-14  SemesterService / RuleConfigurationService /
       OperationLogService

STOP-08
Export 实现前审计

IU-15  ExportService + Export Strategy

IU-16  Console Presentation

IU-17  V0.1 E2E

STOP-11
V0.1 完成审计
```

注意：

- 上表是执行切片，不改变 Gate3.9 “V0.1 完整业务”的要求；
- 如果真实依赖关系要求把一个 IU 再拆成两个更小 IU，可以拆；
- 不允许为了快把多个独立大模块合并为一次超大改动；
- 每个 Service 初版 API 在广泛扩散前仍需执行 STOP-03；
- STOP-04、09、10、14 属于动态触发，不绑定固定 IU。

---

# 9. 每个 IU 的固定执行闭环

每个 IU 都必须按以下顺序进行。

## Phase A — Read

只读取：

- 当前 IU 需要的 Gate 规则；
- 当前实现代码；
- 当前测试；
- 直接依赖模块。

不要为了一个小模块一次性重新设计全项目。

输出一段简短 Scope Summary：

```text
Current IU:
Authority read:
Files expected to touch:
Frozen rules hit:
Potential STOP:
```

## Phase B — Scope Lock

在写代码前明确：

```text
本轮允许新增/修改哪些文件
本轮明确不做什么
当前成功标准是什么
```

若发现当前 IU 实际需要改变未授权模块边界：

```text
STOP
```

不要扩大 scope。

## Phase C — Implement

只实现当前 IU。

原则：

```text
small coherent changes
compile frequently
no unrelated refactor
no speculative abstraction
```

## Phase D — Build

必须实际运行构建。

不得用：

```text
“理论上应该能编译”
```

替代真实构建。

记录：

```text
build command
exit status
重要 warning/error
```

## Phase E — Test

至少覆盖与本 IU 相关的：

```text
normal case
boundary case
invalid case
state/invariant case
```

持久化模块还需要：

```text
round trip
bad input
failure path
```

任何测试必须真实执行。

## Phase F — Self Review

至少检查：

```text
职责是否越界
依赖方向
API 是否扩散
函数长度
命名
const
机械 setter
重复代码
长期裸指针
未使用代码
无必要抽象
统一代码格式是否符合 7.1.1
Explainability / course-level appropriateness
Gate 规则
```

其中 Explainability / course-level appropriateness 必须按 7.4 执行，不得只以 build/test 通过替代可解释性审计。

## Phase G — Gate Trace

输出：

```text
Gate rules satisfied:
Gate rules not exercised:
STOP triggered:
Architecture conflict:
```

## Phase H — Documentation

更新真实开发资料。

至少维护：

```text
README.md
docs/development/
docs/bugs/
docs/decisions/
```

但：

- 没有真实 Bug 就不要编造 Bug；
- 没有新的设计决策就不要硬造一条；
- 不把自动生成的大段流水账塞进 README。

## Phase I — Checkpoint

每个 IU 完成后输出：

```text
IMPLEMENTATION UNIT COMPLETE / BLOCKED

Implemented:
Files changed:
Build:
Tests:
Explainability / course-level appropriateness:
Known issues:
Gate trace:
STOP status:
Recommended next IU:
```

然后停止。

**不得自动开始下一 IU。**

下一 IU 必须由用户明确允许后再开始。

---

# 10. STOP 机制是硬约束

STOP 不是提示。

一旦触发：

> **立即停止相关实现，保留当前仓库处于可审查状态，输出 STOP REPORT，等待人工裁决。**

不得：

- “顺手先写一点”；
- 先把后面的代码生成好；
- 以 TODO 占位继续；
- 自己选择一个看起来合理的方案；
- 把未决规则藏在实现里。

---

# 11. 完整 STOP 表

| STOP | 触发条件 | 必须讨论 |
|---|---|---|
| STOP-01 | Domain 状态机方法开始实现 | 方法签名、字段更新、不变量 |
| STOP-02 | Repository API 准备被多个 Service 使用 | 返回类型、mutable/const、生命周期 |
| STOP-03 | Service 首版 API 准备广泛调用 | 参数、Result、职责、View Result |
| STOP-04 | 出现“Result 还是 throw”争议 | expected failure vs invariant |
| STOP-05 | 第一个多 Repository 写业务 | affected set、snapshot、Prepare/Commit |
| STOP-06 | V0.2 → V0.3 | Qt 总体实现 |
| STOP-07 | MainWindow + 多页面成型 | refresh、Dialog、stable ID |
| STOP-08 | ExportService 开始实现 | ExportDocument、文件输出 |
| STOP-09 | 准备增加 cache / long-term index | 必须先有真实性能证据 |
| STOP-10 | 出现关键真实 Bug | 复现、根因、修复、回归 |
| STOP-11 | V0.1 完成 | V0.1 完整审计 |
| STOP-12 | V0.2 冻结 | Qt 前审计 |
| STOP-13 | V0.3 完成 | V1.0 前 GUI / regression 审计 |
| STOP-14 | 发现 Gate 3 架构冲突 | 判断是否 reopen / 修订 |

---

# 12. STOP REPORT 格式

任何 STOP 必须使用：

```text
STOP-XX TRIGGERED

Current version:
Current IU:
Trigger:
Why this cannot be decided locally:

Relevant current authority:
- ...
- ...

Code already changed:
- ...

Code intentionally NOT changed:
- ...

Exact questions requiring human decision:
1. ...
2. ...

Options observed from real code:
A. ...
B. ...

My implementation recommendation:
...

Risks:
...

Build status:
Test status:

WAITING FOR HUMAN DECISION.
```

如果当前 STOP 只需要检查已冻结方案是否适配真实代码，不要故意发明多个方案。

---

# 13. STOP-10：真实 Bug 协议

关键 Bug 出现后：

```text
不要无痕修复
```

先记录：

```text
Bug ID
发现版本
发现 IU
复现步骤
实际结果
期望结果
影响范围
根因
修复方案
修复 commit
回归测试
状态
```

推荐：

```text
BUG-V01-001
BUG-V02-001
BUG-V03-001
BUG-V10-001
```

是否属于“关键 Bug”需要根据真实影响判断。

不要把：

```text
变量拼写错
少 include 一个头文件
普通编译提示
```

机械包装成课程报告里的“关键 Bug”。

优先记录：

```text
状态机错误
权限绕过
跨对象级联错误
积分/排行算法错误
CSV multiline 恢复错误
持久化 rollback 错误
partial commit 处理错误
Qt stable ID / proxy index 错误
```

等有真实设计价值的问题。

---

# 14. Git 工作规则

目标是保留真实、可复查的版本演进。

## 14.1 不自动 push

除非用户明确要求：

```text
Codex 不自动 push remote
```

每个 IU 可以：

```text
建议 commit
```

但是否提交、是否 push 由用户确认或按当前明确授权执行。

## 14.2 Commit 应小而有语义

推荐：

```text
build: initialize CMake project structure
feat(domain): add account value types
feat(domain): implement volunteer record lifecycle
feat(repo): add repository query skeleton
feat(persistence): add csv codec
test(domain): cover record transitions
fix(persistence): restore snapshot after prepare failure
```

不要：

```text
update
final
changes
fix stuff
```

## 14.3 禁止伪造版本历史

V0.1 / V0.2 / V0.3 / V1.0 必须对应真实项目状态。

不得：

```text
先写完 V1.0
再倒推复制出 V0.1 / V0.2
```

---

# 15. README / Development Record 规则

README 保持“入口文档”，不要变成几万行流水账。

README 至少说明：

```text
项目目标
课程背景
当前版本
构建方式
运行方式
目录结构
架构概览
当前开发状态
权威设计文档入口
```

详细开发记录放到：

```text
docs/development/
```

建议：

```text
docs/development/V0.1_progress.md
docs/development/V0.2_progress.md
docs/development/V0.3_progress.md
docs/development/V1.0_progress.md
```

实现决策：

```text
docs/decisions/implementation_decisions.md
```

Bug：

```text
docs/bugs/bug_log.md
```

记录内容必须来自真实开发过程。

---

# 16. 数据目录治理

`data/` 是正式运行数据，不是随手试验目录。

只能出现已冻结正式 authority files 以及协议允许的临时文件：

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

*.tmp
*.bak
```

测试数据优先进入独立测试临时目录，不污染正式 `data/`。

禁止：

```text
test.csv
test2.csv
backup_old.csv
students_new.csv
db.txt
tempdata.txt
```

成为正式运行文件。

课程要求最终需要个性化样例数据时，单独在对应版本阶段建立真实、可解释样例集，并记录来源与用途。

---

# 17. 测试策略

## 17.1 Domain

重点：

```text
constructor/input invariant
legal transition
illegal transition
field-state consistency
semantic methods
```

## 17.2 Repository

重点：

```text
find
filter
duplicate stable ID prevention
remove
dynamic type preservation
const/mutable boundary
```

## 17.3 Statistics / Ranking

重点：

```text
Approved only
month/semester scope
score sum
duration sum
record count
tie-break
empty set
single student
<3 ranking users
```

## 17.4 Persistence

重点：

```text
CSV quote/comma/multiline
header
round-trip
tmp/bak
Prepare failure
snapshot restore
deterministic Commit
partial commit
startup corruption
schema mismatch
strong reference failure
```

## 17.5 Service

重点：

```text
permission
account status
ownership
target state
business rule
not found
persistence result mapping
OperationLog coordination
```

## 17.6 Console E2E / Qt E2E

必须验证真实业务链，不只测试菜单能打开。

---

# 18. V0.1 完成标准

V0.1 不是“代码都写了”。

至少需要：

```text
正式业务模块实现完成
Console 可以覆盖完整正式业务
Domain 状态机可运行
Service 边界落地
Repository / Persistence 真正启用
CSV / config 可保存恢复
Result/Error 协议统一
Export CSV / Markdown 存在
基本模块测试通过
E2E 首次闭环
```

完成后：

```text
STOP-11
```

不得直接进入 V0.2。

---

# 19. V0.2 完成标准

V0.2 不预设新增主要业务。

目标：

```text
Console complete system stabilized
```

至少验证：

```text
Domain
Service
Repository/Persistence
Console E2E
异常路径
故障注入
退出→重启→恢复
schema/损坏文件
真实 Bug 回归
```

冻结标准：

```text
Blocking bug = 0
Critical business bug = 0
Critical persistence bug = 0
```

已知非阻塞缺陷必须登记。

完成后触发：

```text
STOP-12
```

然后才进入 Qt。

---

# 20. V0.3 / Qt 硬边界

V0.3 主要变化是 Presentation。

必须保持：

```text
Qt → same Service
```

禁止：

```text
Qt → Repository
Qt → CSV
Qt → business state mutation
Domain : QObject
Domain 使用 QString/QDate/QColor/QIcon
```

Qt 长期保存 stable ID，不保存 Repository Domain 地址。

Service 向 Qt 返回纯 C++ View Result。

MainWindow 只做导航壳，不成为 God Class。

页面刷新、Dialog 生命周期、Model/View、Proxy index 等在：

```text
STOP-06
STOP-07
```

审计。

V0.3 完成后触发：

```text
STOP-13
```

---

# 21. Gate 3.8 REOPENED 的实现规则

Gate 3.8 当前图示仍有登记问题。

实现时：

> **文字冻结架构优先于存在错误的旧图。**

当前已知必须确保：

```text
StudentVolunteerService
→ 必须存在

DiaryModerationPage
→ 必须存在

Student 侧 RankingPage
→ 必须存在

ReviewPage
→ 不负责 Diary 审核

Export
→ CSV / Markdown
→ 不是 Excel
```

不得因为旧图遗漏而漏实现。

---

# 22. 何时可以不 STOP

普通局部实现可直接进行：

```text
private helper
局部变量命名
简单 getter
简单 enum converter
明显的 find_if
普通 for loop
CSV 字符转义内部细节
测试辅助函数
重复少量样板实现
```

前提：

> 不改变任何已冻结语义。

一旦一个“局部实现”开始影响：

```text
public API
持久化 schema
对象生命周期
权限
状态机
Service 职责
跨 Repository 提交
Qt/Domain 边界
error/fatal semantics
```

立即重新判断是否触发 STOP。

---

# 23. 每轮 Codex 最终回复格式

每轮执行后，不要输出长篇泛泛总结。

统一：

```text
CURRENT VERSION:
CURRENT IU:
STATUS: COMPLETE / BLOCKED / STOP-XX

Implemented:
- ...

Files changed:
- ...

Build:
- command
- result

Tests:
- command
- passed/failed

Gate compliance:
- ...

Explainability / course-level appropriateness:
- ...

Real issues found:
- ...

Documentation updated:
- ...

Git:
- working tree summary
- suggested commit

Next:
- ...
```

如果 STOP：

```text
不要写 Next implementation
只写：
WAITING FOR HUMAN DECISION
```

---

# 24. 第一次执行时的特殊规则

第一次正式编码不要直接写业务模块。

只执行：

> **IU-00：仓库、构建、测试与目录骨架审计/初始化。**

IU-00 的目标：

```text
确认当前仓库状态
确认 C++ 标准
确认 compiler/toolchain
确认 CMake
确认测试方案
建立必要目录
建立最小可编译 target
建立最小 test target
建立 README 开发入口
建立 development/bugs/decisions 记录入口
```

IU-00 不允许：

```text
实现 User
实现 VolunteerRecord
实现 Service
实现 Repository
实现 CSV
实现业务算法
```

IU-00 完成必须：

```text
build success
test runner success
project tree clean
```

然后停下，等待用户允许进入 IU-01。

---

# 25. 本协议的修改规则

本文件是 Codex 执行协议，不是 Gate 1～4 的替代品。

如果本文件与当前 Gate 1～4 冲突：

```text
Gate 1～4 当前权威规则优先
```

执行层应：

```text
STOP
↓
指出冲突
↓
请求更新本协议
```

不得自行决定哪个 Gate “看起来更合理”。

本协议任何实质变化都应：

```text
显式修改
保留 Git 历史
说明原因
```

不要无痕覆盖。

---

# 26. 开始工作的最短指令

以后给 Codex 新开会话时，可以在本协议已存在于仓库的前提下使用：

```text
先读取仓库中的课程要求、当前权威索引、Gate 1～Gate 4 和
Codex MASTER IMPLEMENTATION PROTOCOL。

严格按权威顺序理解 Current Truth。

本轮只执行我指定的 Implementation Unit。
不要自动进入下一 IU。
触发任何 STOP-01～STOP-14 时立即停止。
不要重新设计 Gate 1～4。
不要引入未冻结架构。
所有完成结论必须基于真实 build/test 结果。
所有 Bug、版本记录和设计决策必须来自真实开发过程。

如果这是第一次正式编码，只执行 IU-00。
```

---

# 27. 当前执行起点

当前 Gate 4 前置设计状态：

```text
Gate 4.1 → CANDIDATE FROZEN
Gate 4.2 → CANDIDATE FROZEN
Gate 4.3 → CANDIDATE FROZEN
Gate 4.4 → CANDIDATE FROZEN
Gate 4.5 → CANDIDATE FROZEN
Gate 4.6 → CANDIDATE FROZEN

Gate 4 Pre-Code Design → COMPLETE
Gate 4 overall → NOT FINAL PASS
```

因此：

```text
NEXT:
V0.1 implementation
```

第一执行单元：

```text
IU-00
仓库 / 构建 / 测试 / 目录骨架
```

完成 IU-00 后：

```text
STOP
等待人工确认
```

不得自动进入 IU-01。
