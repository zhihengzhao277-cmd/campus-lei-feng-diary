# Gate 4.2 — Repository 数据结构、查询与统计算法实现细化（候选冻结稿）

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 4 —— 实现审计  
> 前置依赖：
> - Gate 3.3：StatisticsService / RankingService / 各业务 Service 职责边界
> - Gate 3.4：Repository / Persistence、`std::vector<T>` 权威内存集合、Repository 不维护双重索引
> - Gate 3.9：V0.1 起即按最终核心架构实现，V0.2 负责完整 Console 稳定化
> - Gate 4.1：认证、密码与账号实现细化
>
> 文档性质：实现级候选冻结稿  
> 当前主题：Repository 内存数据结构、查询算法、多条件筛选、统计聚合、排行榜生成与复杂度控制  
> 目标：让后续 AI、Codex、开发者或审计者在不依赖本次聊天上下文的情况下，能够准确理解本项目 Repository 与统计/排序算法的当前实现方向、适用场景、复杂度与设计原因。
>
> 重要说明：
>
> 1. 本文件不修改 Gate 3.4 已冻结的 Repository 架构，只细化其实现级数据结构与算法。
> 2. Repository 继续保存权威业务事实，不持久化积分、排行、徽章、点赞数等派生结果。
> 3. 本文件中的 `std::unordered_map` 只允许作为统计/排行榜计算过程中的临时辅助结构，不构成 Repository 长期索引，不形成第二份权威状态。
> 4. 当前项目规模较小，优先采用简单、稳定、可解释、易测试的数据结构；不为理论上的 O(1) 查询预先构建复杂索引系统。
> 5. 若未来真实压力测试证明线性查询成为可感知瓶颈，再重新审计主容器或索引策略；当前不得无依据提前复杂化。

---

# 1. 当前总体数据结构结论

Gate 3.4 已冻结 Repository 采用：

```text
启动加载
↓
运行期内存权威集合
↓
Service 查询 / 修改
↓
成功操作后及时持久化
```

当前主容器继续采用：

```text
std::vector<T>
```

典型结构：

```text
UserRepository
├── std::vector<Student>
└── std::vector<Administrator>

VolunteerRecordRepository
└── std::vector<VolunteerRecord>

RuleRepository
├── std::vector<VolunteerCategory>
└── std::vector<BadgeRule>

SemesterRepository
└── std::vector<Semester>

DiaryRepository
├── std::vector<DiaryPost>
└── std::vector<LikeRelation>

OperationLogRepository
└── std::vector<OperationLog>
```

当前不采用：

```text
vector + unordered_map 双重权威结构
vector + map 多索引体系
数据库索引
B 树
自定义哈希表
Generic Repository<T> 索引框架
```

---

# 2. 为什么继续使用 std::vector

当前项目的数据规模、访问方式和课程目标决定：

```text
vector
```

是综合成本最低的选择。

主要原因：

```text
数据规模较小
启动时全部载入内存
查询条件多样
统计场景需要大量顺序扫描
写操作数量不高
连续内存局部性好
遍历简单
排序方便
与 CSV 全集合安全重写兼容
容易测试
容易答辩
```

当前没有证据证明：

```text
线性扫描已经成为真实性能瓶颈
```

因此不为了理论上的平均 O(1) ID 查询引入长期索引维护成本。

---

# 3. Repository 查询算法总原则

当前统一：

```text
按唯一 ID 查单对象
→ std::find_if
→ O(n)

按条件查多对象
→ 单次遍历 / std::copy_if
→ O(n)

数量统计
→ std::count_if
→ O(n)

聚合
→ 单次 for 循环 / accumulate
→ O(n)

条件批量删除
→ std::remove_if + erase
→ O(n)

排序型派生结果
→ std::sort
→ O(n log n)
```

核心原则：

> **一次查询有多个条件时，应尽量在同一次扫描中同时判断，而不是先产生多个中间集合再反复扫描。**

---

# 4. UserRepository

## 4.1 当前主容器

```cpp
std::vector<Student> students_;
std::vector<Administrator> administrators_;
```

Student 与 Administrator 分别保存真实动态类型。

`accountId` 在当前系统中承担稳定账号标识。

---

## 4.2 典型使用场景与最优解

| 使用场景 | 推荐算法 | 原因 |
|---|---|---|
| 登录按 `accountId` 查 User | `std::find_if` | 数据规模小，简单直接 |
| 查询某 Student | `std::find_if` | 单一 ID 查询 |
| 查询某 Administrator | `std::find_if` | 单一 ID 查询 |
| 创建账号前检查 ID 唯一 | 两个 vector 顺序查找 | 保证 Student / Administrator 全局唯一 |
| 按姓名搜索 Student | 单次遍历 / `copy_if` | 非唯一条件，索引价值低 |
| 列出全部 Student | 顺序遍历 | vector 天然适合 |
| UI 排序 | 查询结果值上排序 | Repository 不改变权威容器顺序 |

---

## 4.3 登录查询复杂度

最坏：

```text
O(S + A)
```

其中：

```text
S = Student 数量
A = Administrator 数量
```

对于本课程项目的几十到几百账号规模，这一成本可接受。

当前不使用：

```text
unordered_map<accountId, User*>
```

作为长期 Repository 索引。

原因：

```text
收益有限
会增加新增 / 删除 / 回滚一致性成本
会形成双结构维护
当前无真实性能瓶颈
```

---

# 5. VolunteerRecordRepository

## 5.1 当前主容器

```cpp
std::vector<VolunteerRecord> records_;
```

这是全系统最常被扫描的 Repository。

---

## 5.2 常见查询

| 场景 | 推荐算法 |
|---|---|
| `recordId` 查单条记录 | `std::find_if` |
| 某 Student 的全部记录 | 单次遍历 |
| Pending 记录 | 单次遍历 |
| Approved 记录 | 单次遍历 |
| Rejected 记录 | 单次遍历 |
| 日期范围 | 单次遍历 |
| 指定 Category | 单次遍历 |
| Student + Status | 一次遍历同时判断 |
| Student + Month | 一次遍历同时判断 |
| Student + Semester | 一次遍历同时判断 |
| Student + Status + Date | 一次遍历同时判断 |

---

## 5.3 多条件查询原则

不推荐：

```text
findByOwner()
↓
得到中间 vector

再 findByStatus()
↓
第二个中间 vector

再 findByDate()
↓
第三次扫描
```

推荐：

```cpp
for (const auto& record : records_) {
    if (record.ownerAccountId() != accountId)
        continue;

    if (record.status() != RecordStatus::Approved)
        continue;

    if (!matchesDateScope(record.serviceDate(), scope))
        continue;

    // 处理结果
}
```

即：

```text
一次扫描
+
多个条件同时判断
```

复杂度：

```text
O(R)
```

其中：

```text
R = VolunteerRecord 总数
```

---

## 5.4 为什么不建立多索引

如果为了所有查询建立：

```text
recordId index
ownerAccountId index
status index
categoryId index
date index
```

就会开始维护一个小型数据库索引系统。

每次：

```text
新增
删除
状态变化
强制更正
回滚
持久化失败
```

都要同步多份结构。

当前项目不需要承担这类复杂度。

---

# 6. RuleRepository

## 6.1 当前主容器

```cpp
std::vector<VolunteerCategory> categories_;
std::vector<BadgeRule> badgeRules_;
```

---

## 6.2 典型查询与最优解

```text
categoryId → VolunteerCategory
badgeRuleId → BadgeRule
categoryId → 对应 BadgeRule
列出全部 Category
列出全部 BadgeRule
```

统一：

```text
std::find_if
普通遍历
```

原因：

```text
规则数量极少
通常只有个位数或十几条
建立 hash / tree 索引没有实际价值
```

---

# 7. SemesterRepository

## 7.1 当前主容器

```cpp
std::vector<Semester> semesters_;
```

当前学期：

```text
currentSemesterId
```

存于系统配置。

---

## 7.2 查询

### 按 semesterId

```text
std::find_if
O(n)
```

### 按 serviceDate 判断学期

```cpp
for (const auto& semester : semesters_) {
    if (semester.startDate() <= date &&
        date <= semester.endDate()) {
        ...
    }
}
```

通常 Semester 数量很小，因此复杂度实际近似常数。

当前不建立：

```text
date tree
interval tree
```

等复杂结构。

---

# 8. DiaryRepository

## 8.1 当前主容器

```cpp
std::vector<DiaryPost> posts_;
std::vector<LikeRelation> likes_;
```

---

## 8.2 DiaryPost 查询

| 场景 | 推荐算法 |
|---|---|
| `postId` 查帖子 | `std::find_if` |
| `recordId` 查关联帖子 | `std::find_if` |
| Displayed 帖子 | 单次遍历 |
| PendingDisplayReview | 单次遍历 |
| TakenDown | 单次遍历 |

---

## 8.3 LikeRelation 查询

逻辑唯一键：

```text
(studentAccountId, postId)
```

### 判断是否已点赞

```text
std::find_if
```

### 统计某帖 likeCount

```text
std::count_if
```

### 取消点赞

```text
find_if + erase
```

### DiaryPost 物理删除时批量删除 LikeRelation

推荐 erase-remove idiom：

```cpp
likes_.erase(
    std::remove_if(
        likes_.begin(),
        likes_.end(),
        [&](const LikeRelation& like) {
            return like.postId() == postId;
        }
    ),
    likes_.end()
);
```

复杂度：

```text
O(L)
```

其中：

```text
L = LikeRelation 数量
```

---

# 9. OperationLogRepository

## 9.1 当前主容器

```cpp
std::vector<OperationLog> logs_;
```

---

## 9.2 常见查询

```text
按 operatorAccountId
按 OperationType
按 targetType
按 targetId
按时间范围
组合筛选
```

统一采用：

```text
单次遍历过滤
```

如果 Qt 需要时间倒序：

```text
查询结果上排序
```

或者：

```text
基于已追加顺序逆向展示
```

不改变 Repository 权威容器自身顺序。

当前不维护：

```text
multimap<time, log>
unordered_multimap<admin, log>
```

---

# 10. 单个 Student 的统计

## 10.1 错误实现方向

不推荐：

```text
calculateScore(student)
→ 扫 records 一次

calculateDuration(student)
→ 再扫一次

calculateRecordCount(student)
→ 再扫一次
```

这会产生三次重复扫描。

---

## 10.2 推荐聚合结构

概念：

```cpp
struct StudentStatistics {
    double score = 0.0;
    double duration = 0.0;
    int recordCount = 0;
};
```

一次扫描：

```cpp
for (const auto& record : records) {
    if (record.ownerAccountId() != studentId)
        continue;

    if (record.status() != RecordStatus::Approved)
        continue;

    if (!matchesScope(record.serviceDate(), scope))
        continue;

    result.score += record.finalScore();
    result.duration += record.finalDuration();
    ++result.recordCount;
}
```

---

## 10.3 复杂度

```text
O(R)
```

一次扫描同时得到：

```text
score
duration
recordCount
```

---

# 11. 月度 / 学期 / 总统计共用算法

三类统计不应复制三套遍历逻辑。

真正变化的是：

```text
记录是否属于当前统计范围
```

建议概念：

```text
StatisticsScope
├── Total
├── Month
└── Semester
```

或者使用等价的范围参数对象。

---

## 11.1 Total

条件：

```text
Approved
```

---

## 11.2 Month

条件：

```text
Approved
+
serviceDate 属于目标年月
```

---

## 11.3 Semester

条件：

```text
Approved
+
semester.startDate
<= serviceDate
<= semester.endDate
```

---

## 11.4 共用部分

完全共用：

```text
owner 判断
Approved 判断
scope 判断
score 累计
duration 累计
recordCount 累计
```

避免：

```text
calculateMonthlyXXX()
calculateSemesterXXX()
calculateTotalXXX()
```

三套复制逻辑。

---

# 12. 全体学生统计的核心问题

设：

```text
S = Student 数量
R = VolunteerRecord 数量
```

不推荐：

```text
for 每个 Student:
    再遍历全部 VolunteerRecord
```

复杂度：

```text
O(S × R)
```

虽然当前数据规模仍可运行，但算法结构存在明显重复扫描。

---

# 13. 全体学生统计的推荐方案

## 13.1 使用临时 unordered_map 聚合

允许 StatisticsService 在一次函数调用内部建立：

```cpp
struct Aggregate {
    double score = 0.0;
    double duration = 0.0;
    int recordCount = 0;
};

std::unordered_map<std::string, Aggregate> aggregates;
```

其中：

```text
key = ownerAccountId
```

---

## 13.2 扫描流程

```text
遍历全部 VolunteerRecord 一次
↓
只处理 Approved
↓
检查统计 scope
↓
aggregates[ownerAccountId]
    .score += finalScore

aggregates[ownerAccountId]
    .duration += finalDuration

aggregates[ownerAccountId]
    .recordCount++
```

平均复杂度：

```text
O(R)
```

---

## 13.3 为什么这里允许 unordered_map

这里的 `unordered_map`：

```text
只存在于一次计算函数内部
计算结束自动销毁
不持久化
不被 Repository 保存
不形成第二份权威状态
```

因此：

```text
Repository 的 vector
= authoritative data

StatisticsService 的 temporary unordered_map
= temporary working structure
```

两者完全不同。

这不违反“不维护 Repository 双索引”的冻结原则。

---

# 14. RankingService 排行榜生成

RankingService 不直接重复扫描所有记录。

推荐：

```text
StatisticsService
↓
一次批量聚合
↓
accountId → Aggregate
↓
RankingService
↓
构造 RankingEntry
↓
std::sort
↓
赋 rank / rankingTitle
```

---

# 15. RankingEntry

概念：

```text
RankingEntry
├── accountId
├── studentName
├── score
├── validDuration
├── validRecordCount
├── rank
└── rankingTitle
```

它属于：

```text
一次查询结果
```

不是持久 Domain 实体。

---

# 16. 排行榜排序

排序规则继续采用当前冻结规则：

```text
score 降序
↓
validDuration 降序
↓
validRecordCount 降序
↓
accountId 升序
```

概念 comparator：

```cpp
if (a.score != b.score)
    return a.score > b.score;

if (a.validDuration != b.validDuration)
    return a.validDuration > b.validDuration;

if (a.validRecordCount != b.validRecordCount)
    return a.validRecordCount > b.validRecordCount;

return a.accountId < b.accountId;
```

实际实现中如果 score / duration 使用浮点数，应避免随意使用不合理的精度判断；具体数值类型和比较策略留 Gate 4 对应数值实现审计。

---

# 17. 排行榜复杂度

整个流程：

```text
扫描 VolunteerRecord
→ O(R)

遍历 Student 构造 RankingEntry
→ O(S)

排序
→ O(S log S)
```

总目标：

```text
O(R + S log S)
```

而不是：

```text
O(S × R)
```

---

# 18. 月榜 / 学期榜 / 总榜共用

不实现三套排序代码。

统一流程：

```text
RankingScope
↓
StatisticsService 批量聚合
↓
RankingEntry
↓
同一 comparator
↓
std::sort
↓
rank / title
```

变化：

```text
scope
```

不变：

```text
聚合结构
排序
tie-break
名次
荣誉派生
```

---

# 19. 当前不默认一次算三榜

如果 UI 只请求：

```text
月榜
```

就只计算月榜。

当前不建立：

```text
month ranking cache
semester ranking cache
total ranking cache
cache version
cache invalidation
```

因为现有动态计算成本很低。

如果未来某个页面确实要同时展示三榜，可以再审计“一次扫描同时维护三个临时 aggregate”的方案。

当前不提前实现。

---

# 20. StatisticsService 与 RankingService 的分工

当前保持 Gate 3.3 已冻结语义：

```text
StatisticsService
→ 基础统计与批量聚合

RankingService
→ 排名、排序、排行称号
```

不推荐：

```text
for 每个 Student:
    StatisticsService.totalScore(student)
    StatisticsService.totalDuration(student)
    StatisticsService.recordCount(student)
```

因为可能导致重复 O(S × R) 扫描。

推荐：

```text
RankingService
↓
StatisticsService.batchAggregate(scope)
↓
一次得到全体 Student 的统计
↓
排序
```

---

# 21. BadgeService 与统计复用

BadgeService 需要：

```text
指定 Student
+
指定 Category
+
当前有效服务时长
```

建议调用 StatisticsService 的对应统计能力。

对于单个 Student / 单个 Category：

```text
一次扫描 VolunteerRecord
→ O(R)
```

当前没有必要提前维护：

```text
accountId + categoryId → duration
```

长期索引或缓存。

如果未来 AchievementPage 一次需要展示多个 Category Badge，可以考虑一次扫描内同时聚合：

```text
categoryId → duration
```

但仍然是函数内部临时结构，不进入 Repository。

---

# 22. erase-remove 的适用范围

推荐用于：

```text
删除一组满足条件的 LikeRelation
清除某 Post 的全部 LikeRelation
其他明确的 vector 条件批量删除
```

不应机械用于所有单对象删除。

单个 stable ID 删除通常：

```text
find_if
↓
erase
```

更清楚。

---

# 23. Repository 是否返回指针

Gate 3 已冻结：

```text
长期持久关系使用 stable ID
Qt 不长期持有 Domain pointer
```

实现时 Repository 可以在一次 Service 调用内部提供：

```text
短生命周期 non-owning pointer / reference
```

或其他安全查询形式。

但不能：

```text
Page 长期保存 vector 内对象地址
```

因为：

```text
vector reallocation
erase
reload
```

可能使地址 / iterator 失效。

本文件不重新冻结 Repository 最终返回类型，只继续遵守 stable ID 与生命周期边界。

---

# 24. CSV 文件查询不是运行期主查询路径

当前明确：

```text
程序启动
↓
CSV 读取到 vector
↓
运行期查询 vector
```

不是：

```text
每次 findById()
↓
重新打开 CSV
↓
逐行读硬盘
```

因此：

```text
O(n)
```

主要是：

```text
内存 vector 线性扫描
```

而不是磁盘全文件扫描。

这是当前线性算法可接受的重要前提。

---

# 25. 当前算法总表

| 操作 | 推荐算法 | 复杂度 |
|---|---|---:|
| ID 查找 | `std::find_if` | O(n) |
| 条件筛选 | 单次遍历 / `std::copy_if` | O(n) |
| 数量统计 | `std::count_if` | O(n) |
| 单 Student 统计 | 一次扫描同时聚合 | O(R) |
| 全体 Student 批量统计 | 临时 `unordered_map` 聚合 | 平均 O(R) |
| 单对象删除 | `find_if + erase` | O(n) |
| 条件批量删除 | `remove_if + erase` | O(n) |
| 排行榜排序 | `std::sort` | O(S log S) |
| 完整排行榜 | 聚合 + 排序 | O(R + S log S) |
| CSV 启动恢复 | 顺序读取 | O(n) |

---

# 26. 为什么这套方案是当前最优解

这里的“最优”不是指每个单独查询都有理论最小 Big-O。

而是：

> **在当前项目规模、功能复杂度、实现时间、可维护性、可解释性和课程目标下的综合最优。**

综合考虑：

```text
vector 作为长期权威结构
+
线性扫描解决多样查询
+
一次扫描合并多条件与多统计
+
临时 unordered_map 解决全体统计聚合
+
std::sort 解决排行榜
```

比：

```text
多个长期索引
复杂缓存
自定义树结构
数据库
```

更适合当前项目。

---

# 27. 当前明确不采用

后续 AI / Codex 不得擅自引入：

```text
Repository 长期 unordered_map 索引
vector + unordered_map 双权威状态
多字段索引系统
B 树
自定义数据库索引
排行榜缓存
积分缓存
Badge 缓存
LikeCount 持久化
Student.totalScore 持久化
Student.currentRank 持久化
Student.currentBadge 持久化
每个 Student 重新扫全 records 构造排行榜
每个指标独立重扫一次 records
```

除非后续有真实性能证据并形成显式修订。

---

# 28. 当前实现级候选决策

## G4-DATA-01 — Repository 主容器与基础查询

> **各 Repository 继续以 Gate 3.4 已冻结的 `std::vector<T>` 顺序集合为主要权威内存结构，不建立额外 map / unordered_map 双结构索引。单对象查询采用 `std::find_if` 等线性查找，多对象查询采用单次遍历过滤；统计采用单次扫描聚合，条件批量删除采用 erase-remove，排序型派生结果由对应 Service 使用 `std::sort` 完成。**

---

## G4-DATA-02 — 多条件查询一次扫描

> **对于同时具有多个查询条件的业务，应尽量在一次遍历中同时完成条件判断，而不是产生多个中间集合反复扫描。Repository 只承担数据访问条件；积分、排行榜、徽章等业务算法仍由相应 Service 完成。**

---

## G4-DATA-03 — 不预建长期索引

> **当前不因理论上的 O(1) ID 查找优势引入 `unordered_map` 长期 Repository 索引。只有真实压力测试证明现有线性扫描成为可感知性能瓶颈时，才重新审计主容器或索引策略，而不是预先维护多份权威结构。**

---

## G4-STAT-01 — 单 Student 单次聚合

> **单个学生的积分、有效服务时长和有效记录数在同一次 VolunteerRecord 线性扫描中同时完成聚合，不为三个指标分别重复遍历。月度、学期和总统计共用同一聚合逻辑，仅通过统计范围条件区分。**

---

## G4-STAT-02 — 全体 Student 批量聚合

> **全体学生统计与排行榜生成不得采用“每个 Student 分别重新遍历全部 VolunteerRecord”的 O(S×R) 方案。StatisticsService 对符合统计范围的 Approved VolunteerRecord 进行一次扫描，并使用函数内部临时 `std::unordered_map<accountId, Aggregate>` 聚合各学生的 score、duration、recordCount；该 map 只是一次计算工作区，不属于 Repository 权威状态或持久索引。**

---

## G4-STAT-03 — RankingService 排序与目标复杂度

> **RankingService 基于 StatisticsService 的批量统计结果构造 RankingEntry，并使用 `std::sort` 按“积分降序 → 有效服务时长降序 → 有效记录数降序 → accountId 升序”完成排序。排行榜整体目标复杂度为 O(R + S log S)，月榜、学期榜和总榜共用同一排序与排名算法，只替换统计范围条件。**

---

# 29. 后续实现待确认事项

以下内容当前尚未最终冻结：

```text
各 Repository 最终方法签名
findById 返回 pointer / reference / optional-like handle 的精确形式
View Result 精确结构名
StatisticsScope / RankingScope 的精确 C++ 表达
日期范围匹配函数签名
StudentStatistics / Aggregate 是否使用同一结构
unordered_map 是否提前 reserve
score / duration 精确数值类型
浮点比较策略
Repository 查询是否返回 vector<T> / vector<summary> / ID 列表
Qt TableModel 的分页需求
是否需要真实性能基准测试
```

这些不改变当前数据结构和算法主方向。

---

# 30. 给后续 AI / Codex 的最短读取指令

如果上下文空间不足，只需读取本节。

```text
当前 Repository / Statistics / Ranking 实现路线：

1. Repository 权威内存结构继续使用 std::vector<T>。
2. 不维护 vector + map/unordered_map 双重长期索引。
3. 运行期查询的是内存 vector，不是每次重新扫描 CSV 文件。
4. ID 查询：
   std::find_if
   O(n)

5. 多条件查询：
   一次遍历中同时判断 owner/status/date/category 等条件。
   不生成多个中间 vector 反复扫描。

6. 统计：
   单 Student 的 score/duration/recordCount 在一次 records 扫描里同时累计。
   月度/学期/总统计共用同一核心，只替换 scope 条件。

7. 全体 Student 统计：
   不允许每个 Student 各扫一遍 records。
   StatisticsService 一次扫描 Approved records，
   用函数内部临时 unordered_map<accountId, Aggregate> 聚合。
   该 map 不是 Repository，不持久化，不是第二份权威状态。

8. RankingService：
   读取批量 Aggregate
   构造 RankingEntry
   std::sort：
   score desc
   → duration desc
   → recordCount desc
   → accountId asc

9. 排行榜目标复杂度：
   O(R + S log S)

10. DiaryRepository：
    LikeRelation 判断用 find_if
    likeCount 用 count_if
    物理删除 Post 关联 LikeRelation 用 remove_if + erase。

11. 不采用：
    Repository 长期 unordered_map 索引
    数据库索引
    排行榜/积分/徽章缓存
    Student.totalScore/currentRank/currentBadge 持久化
    O(S×R) 排行榜实现
```

---

# 31. 当前状态建议

```text
Gate 4.2
Repository 数据结构、查询与统计算法实现细化

G4-DATA-01 ～ G4-DATA-03
G4-STAT-01 ～ G4-STAT-03

→ CANDIDATE FROZEN
```

在实际 Repository / StatisticsService / RankingService 编码完成并通过单元测试、复杂度检查和端到端测试后，再进行 Gate 4.2 FINAL AUDIT。
