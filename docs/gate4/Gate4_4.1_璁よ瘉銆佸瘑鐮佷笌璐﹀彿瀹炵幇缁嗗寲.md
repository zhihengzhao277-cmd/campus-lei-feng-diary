# Gate 4.1 — 认证、密码与账号实现细化（候选冻结稿）

> 项目：2025级《高级语言程序设计 C/C++》课程设计  
> 选题：任务 4.5 “校园雷锋日记”好人好事积分榜（★★★★★）  
> 所属阶段：Gate 4 —— 实现审计  
> 前置依赖：
> - Gate 3.1：`User / Student / Administrator`、密码职责边界（D5、D13）
> - Gate 3.3：`AuthenticationService / UserManagementService / PasswordHasher`
> - Gate 3.4：`UserRepository / Persistence`
> - Gate 3.9：V0.1 起即按最终核心架构实现，密码模块从 V0.1 起真实启用
>
> 文档性质：实现级候选冻结稿  
> 当前主题：密码认证数据结构、哈希算法、salt、迭代参数、持久化、创建 / 验证 / 修改 / 重置流程  
> 目标：让后续 AI、Codex、开发者或审计者在不依赖本次聊天上下文的情况下，能够准确理解本项目密码模块的当前实现方向与职责边界。
>
> 重要说明：
>
> 1. 本文件只细化已经在 Gate 3 冻结的密码实现，不重新修改 User 继承体系、权限体系、Repository 架构或 Qt 边界。
> 2. Gate 3.1 继续只负责“谁持有密码认证状态、谁负责算法”的架构边界；本文件才是具体密码实现参数的主要归档位置。
> 3. Gate 3.4 可在后续同步密码字段的 CSV 映射，但不应复制本文件中关于算法选择、salt、迭代次数等完整实现说明。
> 4. 本文件当前为候选冻结稿；如果后续 Gate 4 实际编码选择的密码库 API 与本文概念接口略有差异，应保持职责与算法语义不变，并通过显式修订说明，不无痕覆盖。

---

# 1. Gate 3 已冻结的密码职责边界

当前项目关于密码的上层架构已经明确。

## 1.1 User 的职责

`User` 是抽象基类，`Student` 与 `Administrator` 共享账号层面的密码认证状态。

概念：

```text
User <<abstract>>
├── accountId
├── name
├── password-related data
└── accountStatus
```

`User` 负责：

```text
保存当前账号自己的密码认证状态
通过受控方式更新密码认证状态
```

`User` 不负责：

```text
自己实现 PBKDF2
自己生成随机 salt
自己执行 SHA-256 / HMAC
自己访问 Repository
自己处理登录流程
自己处理 Qt 输入
```

也不允许把具体密码算法硬编码到 `User` 内。

---

## 1.2 PasswordHasher 的职责

`PasswordHasher` 是纯技术组件。

它负责：

```text
创建密码认证数据
验证输入密码
生成安全随机 salt
调用成熟密码库完成 PBKDF2-HMAC-SHA256
编码 / 比较必要的二进制结果
```

它不认识：

```text
Student
Administrator
Permission
AccountStatus
Repository
Qt
业务菜单
```

也就是说：

```text
PasswordHasher
= password cryptographic utility

不是：
User Service
Authentication Service
Repository
```

---

## 1.3 AuthenticationService 的职责

`AuthenticationService` 负责完整登录认证流程。

概念：

```text
输入 accountId + plaintext password
↓
UserRepository 查找实际 User
↓
PasswordHasher 验证密码
↓
检查 AccountStatus
↓
形成认证结果
```

它负责：

```text
账号查找
密码验证协调
账号状态检查
Student 成功登录后的 lastLoginAt 协调
登录结果构造
```

它不负责：

```text
直接实现 PBKDF2
自己读 CSV
自己解析密码哈希字段
```

---

## 1.4 UserManagementService 的职责

`UserManagementService` 负责账号管理相关密码操作，例如：

```text
创建 Student 时设置初始密码
Student 修改自己的密码
Administrator 重置 Student 密码
```

这些操作的共同流程是：

```text
收到新的 plaintext password
↓
调用 PasswordHasher
↓
生成新的 PasswordData
↓
更新 User 密码认证状态
↓
持久化
```

---

## 1.5 UserRepository 的职责

`UserRepository` 管理：

```text
Student
Administrator
```

以及它们的密码认证数据持久化。

当前物理文件方向：

```text
students.csv
administrators.csv
```

Repository 负责：

```text
PasswordData ↔ CSV 字段映射
读取
恢复
保存
```

Repository 不负责：

```text
判断密码是否正确
生成 salt
PBKDF2
登录业务
```

---

# 2. 当前密码安全底线

当前正式实现必须满足：

```text
不保存明文密码
不允许可逆“加密密码”替代密码哈希
不使用 std::hash<std::string> 作为密码哈希
不使用 rand() / srand(time()) 生成 salt
不自行设计密码学算法
```

密码模块采用：

> **不可逆密码派生 / 验证，而不是“加密后再解密”。**

因此不存在：

```text
decryptPassword()
```

登录时的正确逻辑是：

```text
输入密码
↓
使用保存的 salt + 参数重新派生
↓
比较派生结果
```

而不是把保存结果恢复成原密码。

---

# 3. 为什么不能直接使用 std::hash<std::string>

`std::hash` 主要用于：

```text
std::unordered_map
std::unordered_set
```

等哈希容器。

其设计目标主要是：

```text
快速散列
```

而密码认证需要的目标是：

```text
对暴力猜测具有显著计算成本
```

如果直接：

```cpp
std::hash<std::string>{}(password);
```

会产生几个问题：

1. 不是专门的密码哈希 / 密钥派生算法；
2. 计算速度过快，不适合抵抗离线暴力猜测；
3. 没有天然独立 salt 机制；
4. 同样密码容易产生相同摘要语义；
5. 跨平台 / 实现稳定性也不适合作为持久密码格式依据。

因此当前明确：

> **`std::hash` 不用于密码存储。**

---

# 4. 当前密码算法方案

当前候选冻结方案：

> **PBKDF2-HMAC-SHA256**

参数：

```text
Password KDF:
PBKDF2-HMAC-SHA256

Salt:
16 bytes = 128 bits

Iterations:
600000

Derived key length:
32 bytes = 256 bits

Persistence encoding:
hexadecimal text
```

结构：

```text
plaintext password
      +
random salt
      +
600000 iterations
      ↓
PBKDF2-HMAC-SHA256
      ↓
32-byte derived hash
```

---

# 5. 为什么选择 PBKDF2-HMAC-SHA256

当前项目并不是公网生产账号平台，而是：

```text
本地 C++ 课程设计
CSV 持久化
Windows / Qt 桌面程序
账号规模较小
需要可解释、可测试、可答辩
开发时间紧
```

现代密码存储还存在：

```text
Argon2id
scrypt
bcrypt
PBKDF2
```

等方案。

当前项目选择 PBKDF2-HMAC-SHA256，主要考虑：

```text
成熟
标准化
实现机制清晰
可由成熟密码库直接支持
参数容易解释
Windows + C++ 集成成本较低
足够满足本课程设计的密码认证要求
```

这里不宣称：

```text
PBKDF2 是所有现代系统的绝对最佳方案
```

而应准确解释为：

> 在本课程设计的体量、时间、可解释性和依赖成本下，选择成熟的 PBKDF2-HMAC-SHA256 作为密码派生方案。

---

# 6. Salt

## 6.1 Salt 是什么

Salt 是每次创建密码认证数据时随机生成的一段非秘密随机值。

例如：

```text
password = abc123

User A:
salt = random_A

User B:
salt = random_B
```

即使：

```text
password_A == password_B
```

因为：

```text
salt_A != salt_B
```

最终：

```text
hash_A != hash_B
```

---

## 6.2 Salt 是否需要保密

不需要。

Salt 可以和密码 hash 一起持久化。

必须保密的是：

```text
plaintext password
```

而不是：

```text
salt
```

---

## 6.3 Salt 长度

当前采用：

```text
16 bytes
= 128 bits
```

每次创建新密码认证状态时重新生成。

---

## 6.4 Salt 随机源

必须使用：

```text
cryptographically secure random bytes
```

不使用：

```cpp
std::rand()
srand(time(nullptr))
```

也不自行写伪随机算法。

优先调用最终密码库 / 系统密码学库提供的安全随机数能力。

---

# 7. Iterations / Work Factor

PBKDF2 不是只算一次 HMAC。

当前采用：

```text
iterations = 600000
```

作用是：

```text
增加每次密码猜测的计算成本
```

从而让攻击者离线暴力猜密码更昂贵。

当前参数应作为认证数据的一部分持久化，而不是只写死在代码里。

原因：

如果未来：

```text
600000
↓
800000
```

旧账号仍然需要知道自己原来采用的是：

```text
600000
```

这样旧账号仍可正常验证。

因此：

```text
PasswordData
→ 保存 iterations
```

---

# 8. Derived Hash 长度

PBKDF2-HMAC-SHA256 当前派生：

```text
32 bytes
= 256 bits
```

结果本质上是一段二进制 byte 数据。

这段结果是最终持久化的：

```text
password hash
```

不是原密码。

---

# 9. Hex 编码

Salt 和 derived hash 都是原始 bytes。

CSV 不适合直接保存任意二进制数据。

因此采用：

```text
hexadecimal
```

编码。

例如：

```text
16-byte salt
→ 32 个 hex 字符

32-byte hash
→ 64 个 hex 字符
```

优点：

```text
容易实现
容易查看
容易调试
容易答辩
不涉及复杂二进制 CSV 处理
```

虽然比 Base64 稍占空间，但项目账号数量很小，该开销可忽略。

---

# 10. PasswordData 值对象

当前建议将账号密码认证状态收敛为独立值对象：

```text
PasswordData
├── algorithm
├── iterations
├── saltHex
└── hashHex
```

概念 C++ 结构：

```cpp
enum class PasswordAlgorithm {
    Pbkdf2HmacSha256
};

class PasswordData {
private:
    PasswordAlgorithm algorithm_;
    int iterations_;
    std::string saltHex_;
    std::string hashHex_;

public:
    // 只提供必要的只读访问
};
```

最终精确构造函数、getter 名称、类型别名等留实际编码时冻结。

---

# 11. 为什么 PasswordData 要包含 algorithm

虽然当前只使用：

```text
PBKDF2-HMAC-SHA256
```

仍建议持久化：

```text
algorithm
```

原因：

> 持久化认证数据应该能够解释自己是如何生成的，而不是依赖“当前代码默认算法”去猜。

例如：

```text
algorithm = PBKDF2_HMAC_SHA256
iterations = 600000
salt = ...
hash = ...
```

这样即使未来代码升级，也能明确识别旧认证数据格式。

注意：

```text
PasswordAlgorithm
```

只是认证数据算法标识。

它不意味着当前系统采用：

```text
Password Strategy Pattern
```

当前仍只有一个真实算法方向，因此不引入第二套 Strategy / Factory / Registry。

---

# 12. User 与 PasswordData 的关系

最终概念结构：

```text
User <<abstract>>
├── accountId
├── name
├── PasswordData
└── AccountStatus
```

Student / Administrator 都继承同一套密码认证状态。

因此无需：

```text
StudentPasswordData
AdministratorPasswordData
```

两套结构。

---

# 13. PasswordHasher 概念接口

建议保持非常小：

```cpp
class PasswordHasher {
public:
    PasswordData hashPassword(
        const std::string& plainPassword
    ) const;

    bool verifyPassword(
        const std::string& plainPassword,
        const PasswordData& passwordData
    ) const;
};
```

如果实际密码库 API 要求辅助私有方法，可以内部增加：

```text
generateSalt()
deriveKey()
hexEncode()
hexDecode()
constantTimeCompare()
```

但这些属于 PasswordHasher 内部技术细节。

---

# 14. PasswordHasher 明确不负责什么

禁止让它负责：

```text
find user
check AccountStatus
check Permission
modify Student
write CSV
show QMessageBox
record OperationLog
```

它只回答：

```text
“根据密码和参数产生什么认证数据？”

以及：

“这个明文密码是否与这份 PasswordData 匹配？”
```

---

# 15. 创建 Student 的密码流程

管理员创建 Student：

```text
Administrator
↓
UserManagementService.createStudent(...)
↓
检查权限 / 输入合法性 / accountId 唯一性
↓
取得 plaintext initial password
↓
PasswordHasher.hashPassword()
↓
生成 random 16-byte salt
↓
PBKDF2-HMAC-SHA256
↓
iterations = 600000
↓
32-byte derived hash
↓
salt / hash 转 hex
↓
构造 PasswordData
↓
构造 Student
↓
UserRepository
↓
Persistence
↓
成功后返回 OperationResult
```

这里：

```text
plaintext password
```

只作为本次调用过程中的短期输入。

不能写入：

```text
Student
CSV
OperationLog
错误信息
```

---

# 16. 登录验证流程

完整概念：

```text
Presentation
↓
accountId + plaintext password
↓
AuthenticationService
↓
UserRepository.findByAccountId(accountId)
↓
获得真实 Student / Administrator
↓
PasswordHasher.verifyPassword(
    plaintext password,
    user.passwordData
)
↓
读取：
algorithm
iterations
saltHex
hashHex
↓
saltHex → bytes
↓
重新运行 PBKDF2-HMAC-SHA256
↓
candidateHash
↓
安全比较 candidateHash 与 storedHash
```

匹配：

```text
密码正确
↓
继续检查 AccountStatus
↓
完成后续认证流程
```

不匹配：

```text
认证失败
```

普通登录 UI 对外可以使用模糊失败提示，不必暴露：

```text
账号不存在
密码错误
```

之间的详细差异。

---

# 17. 为什么验证时不能“解密 hash”

Hash / KDF 的认证逻辑是单向的。

数据库中没有：

```text
可恢复原密码的密文
```

因此：

```text
verify
```

必须重新计算。

正确：

```text
plaintext input
+
stored salt
+
stored iterations
↓
derive again
↓
compare
```

错误：

```text
decrypt stored password
```

---

# 18. Hash 比较

功能上：

```cpp
candidateHash == storedHash
```

可以判断内容是否一致。

但密码模块建议使用成熟密码库提供的：

```text
constant-time comparison
```

而不是自行编写提前退出比较。

原因：

```text
普通比较可能在遇到首个不同 byte 时提前返回
```

密码库通常提供更适合认证数据的固定时间比较语义。

本课程项目不需要在报告中夸大 timing attack 风险，只需准确说明：

> 密码摘要比较优先采用密码库提供的恒定时间比较能力。

---

# 19. Student 修改自己的密码

流程：

```text
Student
↓
UserManagementService / dedicated account method
↓
验证当前用户身份与修改权限
↓
必要时验证旧密码
↓
收到 new plaintext password
↓
PasswordHasher.hashPassword(new password)
↓
生成全新的 salt
↓
生成全新的 PasswordData
↓
替换 User 当前 PasswordData
↓
UserRepository / Persistence
↓
成功后提交
```

关键：

> **设置新密码时必须生成新的 salt。**

不能：

```text
旧密码用 salt A
↓
新密码继续使用 salt A
```

当前统一采用：

```text
每次创建 / 修改 / 重置密码
→ 新 salt
```

---

# 20. Administrator 重置 Student 密码

管理员重置密码与本人改密在密码算法层完全相同：

```text
new plaintext password
↓
new salt
↓
new PBKDF2 result
↓
new PasswordData
```

差别只在：

```text
谁有权发起
是否需要旧密码
是否需要 OperationLog
```

这些都属于：

```text
UserManagementService
```

而不是 PasswordHasher。

---

# 21. 密码重置与 OperationLog

管理员执行密码重置属于关键账号管理行为。

OperationLog 可以记录：

```text
operatorAccountId
operationType = ResetStudentPassword / equivalent
targetType = Student
targetId = accountId
operationTime
description
```

但绝对不能记录：

```text
明文新密码
旧密码
passwordHash
salt
```

日志只记录“发生了密码重置”这一审计事实。

---

# 22. CSV 持久化字段建议

## 22.1 students.csv

概念字段：

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

实际列顺序留 Gate 4 Persistence 映射时统一冻结。

---

## 22.2 administrators.csv

概念字段：

```text
accountId
name
passwordAlgorithm
passwordIterations
passwordSaltHex
passwordHashHex
accountStatus
```

Administrator 当前没有为了“字段对称”而强行加入 Student 专属字段。

---

# 23. PasswordData 的 CSV 恢复

Repository 加载：

```text
CSV text fields
↓
parse algorithm
↓
parse iterations
↓
validate saltHex
↓
validate hashHex
↓
construct PasswordData
↓
construct Student / Administrator
```

加载时至少检查：

```text
algorithm 是否支持
iterations 是否有效
saltHex 是否为合法 hex
salt 长度是否符合当前格式
hashHex 是否为合法 hex
hash 长度是否符合当前格式
```

如果核心认证字段损坏：

```text
不得默默生成默认密码
不得把账号当成无密码账号
不得跳过密码校验
```

应视为持久化数据损坏 / 加载失败。

---

# 24. 不持久化 plaintext password

这一条是硬约束。

禁止 CSV：

```text
accountId,password
20250001,123456
```

禁止：

```text
plaintextPassword
initialPassword
currentPassword
```

作为长期字段。

创建账号时如果需要展示初始密码：

```text
只在创建流程当次向有权限调用者展示 / 输入
```

不形成权威持久字段。

---

# 25. 密码库原则

PBKDF2、HMAC、SHA-256、安全随机数、constant-time compare：

> **全部使用成熟密码学库。**

不自行从密码学公式实现：

```text
SHA-256
HMAC
PBKDF2
CSPRNG
```

原因：

1. 自行实现容易产生安全错误；
2. 这些不是本课程项目需要自行创造的业务算法；
3. 项目真正需要解释的是密码认证流程、数据结构和职责边界；
4. 成熟密码库更符合工程实践。

最终具体采用：

```text
OpenSSL
或其他在 Windows + CMake 下稳定可用的成熟密码库
```

由 Gate 4 实际环境配置时确定。

在密码库未最终选择前，不提前冻结某个库的函数名。

---

# 26. 为什么不自行实现“多轮 SHA-256”

不采用：

```text
hash = SHA256(password + salt)

for i in range(...)
    hash = SHA256(hash)
```

作为自己发明的密码方案。

虽然看起来具有：

```text
salt
多轮
SHA-256
```

但它不是本项目应自行设计的正式密码 KDF。

当前统一采用标准：

```text
PBKDF2-HMAC-SHA256
```

由成熟库执行。

---

# 27. 为什么不使用可逆加密保存密码

不采用：

```text
AES(password)
↓
database
↓
decrypt
```

作为密码认证。

密码认证只需要：

```text
确认输入密码是否正确
```

不需要系统知道用户原密码。

因此使用：

```text
one-way password derivation
```

更符合需求。

---

# 28. 默认参数与持久化参数的关系

代码可以存在：

```text
DEFAULT_PBKDF2_ITERATIONS = 600000
DEFAULT_SALT_BYTES = 16
DEFAULT_HASH_BYTES = 32
```

它们用于创建新密码认证数据。

但验证旧账号时：

```text
不要强制使用当前默认 iterations
```

而应该读取：

```text
PasswordData.iterations
```

即：

```text
Create new credential
→ use current defaults

Verify existing credential
→ use persisted parameters
```

---

# 29. 可选的未来参数升级

当前 V1.0 不要求复杂的自动密码升级机制。

但数据结构已经允许未来：

```text
600000
↓
更高 work factor
```

或算法迁移。

如果未来真的需要：

```text
用户成功登录
↓
发现旧 work factor
↓
重新 hash
```

可以再审计。

当前不提前实现。

---

# 30. 密码复杂度规则

本文件当前只冻结：

```text
哈希 / 存储 / 验证实现
```

具体密码长度、字符组成、初始密码规则属于：

```text
业务校验 / 输入规则
```

如果 Gate 1 / Gate 4 后续需要，可以单独冻结。

当前不能因为密码哈希模块存在，就擅自补造：

```text
必须大写
必须特殊字符
必须 12 位
定期强制换密码
```

等尚未确认业务规则。

---

# 31. 密码错误与 ServiceError

PasswordHasher 本身优先返回：

```text
bool verifyPassword(...)
```

或等价技术结果。

AuthenticationService 再把认证失败映射到 Application 语义。

例如：

```text
Result<AuthenticatedUserInfo>
```

普通登录失败不要求向 UI 泄露内部具体分类。

Persistence / 数据损坏则属于另一类错误：

```text
PersistenceFailure
Fatal / corrupted authoritative data
```

不能伪装成：

```text
WrongPassword
```

---

# 32. Qt / Console 边界

V0.1 / V0.2：

```text
Console
↓
AuthenticationService
↓
PasswordHasher
```

V0.3 / V1.0：

```text
LoginWindow
↓
AuthenticationService
↓
PasswordHasher
```

两套 Presentation 共用同一认证实现。

因此密码模块不得依赖：

```text
std::cin
std::cout
QLineEdit
QMessageBox
QString
```

核心认证数据保持纯 C++。

---

# 33. 当前完整密码架构图

```text
                    Presentation
              Console / Qt LoginWindow
                         │
                         ▼
                AuthenticationService
                         │
              ┌──────────┴──────────┐
              │                     │
              ▼                     ▼
       UserRepository          PasswordHasher
              │                     │
              ▼                     │
     Student / Administrator        │
              │                     │
              └──── PasswordData ───┘
                         │
                         ▼
                  UserRepository
                         │
                         ▼
             students.csv /
             administrators.csv
```

创建 / 重置：

```text
UserManagementService
        │
        ▼
  plaintext password
        │
        ▼
   PasswordHasher
        │
        ▼
    PasswordData
        │
        ▼
       User
        │
        ▼
 UserRepository
        │
        ▼
 Persistence
```

---

# 34. 当前禁止实现清单

后续 AI / Codex 不得擅自采用：

```text
明文密码持久化
std::hash(password)
MD5
单次 SHA-256(password)
无 salt
所有用户共用固定 salt
rand() / srand(time()) salt
可逆加密密码
PasswordHasher 访问 Repository
PasswordHasher 判断 Permission
User 内硬编码 PBKDF2
Qt 直接验证 hash
Console 直接验证 hash
OperationLog 保存密码
密码修改复用旧 salt
损坏 PasswordData 时自动使用默认密码
```

除非后续出现明确修订。

---

# 35. 当前实现级候选决策

## G4-AUTH-01 — 密码存储与验证方案

> **Student 与 Administrator 不持久化明文密码。账号密码认证数据统一由 `PasswordData` 值对象表示，至少保存密码算法标识、迭代次数、随机盐和派生哈希。正式密码派生采用 `PBKDF2-HMAC-SHA256`，每次创建、修改或重置密码时生成独立的 16-byte 密码学安全随机 salt，默认 work factor 采用 600000 iterations，派生结果长度为 32 bytes；salt 与 hash 使用 hexadecimal 文本编码后持久化。`PasswordHasher` 负责凭据生成与验证，PBKDF2 / HMAC / SHA-256、安全随机数及恒定时间摘要比较均优先使用成熟密码库能力，不自行实现密码学原语。**

---

## G4-AUTH-02 — 密码职责边界

> **`User` 只持有当前 `PasswordData` 并通过受控语义更新密码认证状态；`PasswordHasher` 不依赖 User、Repository、Qt 或 Permission；`AuthenticationService` 负责根据 accountId 找到真实 User、调用 PasswordHasher 验证凭据并检查账号状态；`UserManagementService` 负责创建账号、本人改密或管理员重置密码时协调新 PasswordData 的生成与持久化。**

---

## G4-AUTH-03 — 密码持久化格式原则

> **`students.csv` 与 `administrators.csv` 只持久化密码认证数据，不持久化明文密码。PasswordData 至少映射为 `passwordAlgorithm / passwordIterations / passwordSaltHex / passwordHashHex` 四类字段；Repository 负责字段解析和 PasswordData 恢复。认证字段损坏或算法不受支持时不得生成默认凭据或跳过验证，应作为持久化数据错误处理。**

---

## G4-AUTH-04 — 新密码重新生成认证数据

> **创建账号、本人修改密码和管理员重置密码均视为一次新的密码凭据生成：必须产生新的安全随机 salt，并重新计算完整 PasswordData；不得复用旧 salt。**

---

## G4-AUTH-05 — 密码算法与 Presentation 解耦

> **Console 与 Qt 共用同一 AuthenticationService / UserManagementService / PasswordHasher 实现。任何 Presentation 均不得直接执行密码哈希、读取或比较 passwordHash，也不得把 Qt 类型引入密码认证核心。**

---

# 36. 后续 Gate 4 实现待确认事项

以下内容当前尚未最终冻结：

```text
最终密码库：
OpenSSL / 其他成熟库

CMake 链接方式

PasswordData 最终 C++ getter / constructor

PasswordAlgorithm 的序列化字符串精确格式

hex encode / decode 是否使用密码库能力或轻量自写格式函数

AuthenticationService 最终函数签名

修改密码是否必须再次验证旧密码

Student 初始密码如何产生 / 输入

最终密码长度与字符规则

管理员重置密码后的 UI 提示方式
```

这些事项不影响当前已经冻结的密码算法主结构。

---

# 37. 给后续 AI / Codex 的最短读取指令

如果上下文空间不足，只需读取本节。

```text
当前密码实现路线：

1. User 保存 PasswordData，不保存明文密码。
2. PasswordData 至少包含：
   - algorithm
   - iterations
   - saltHex
   - hashHex

3. 当前正式候选算法：
   PBKDF2-HMAC-SHA256

4. 默认参数：
   - salt = 16 bytes secure random
   - iterations = 600000
   - derived hash = 32 bytes
   - salt/hash 使用 hex 保存

5. 每次创建、修改、重置密码：
   - 生成新 salt
   - 重新 PBKDF2
   - 形成新 PasswordData
   - 不复用旧 salt

6. PasswordHasher：
   - 只负责 hash / verify
   - 不认识 User / Repository / Permission / Qt
   - 使用成熟密码库实现 PBKDF2、HMAC、SHA-256、安全随机数和恒定时间比较
   - 不自行造密码学原语

7. AuthenticationService：
   - 按 accountId 从 UserRepository 找真实 User
   - 调 PasswordHasher.verifyPassword
   - 再检查 AccountStatus
   - 形成登录结果

8. UserManagementService：
   - 创建账号 / 改密 / 重置密码时调用 PasswordHasher
   - 协调 User 状态更新和 Persistence

9. CSV 只保存：
   passwordAlgorithm
   passwordIterations
   passwordSaltHex
   passwordHashHex

10. 禁止：
   明文密码
   std::hash
   MD5
   单次 SHA-256
   固定 salt
   rand() salt
   可逆加密密码
   Qt / Console 直接验证 hash
   日志记录密码
```

---

# 38. 当前状态建议

```text
Gate 4.1
认证、密码与账号实现细化

G4-AUTH-01 ～ G4-AUTH-05
→ CANDIDATE FROZEN
```

在实际密码库选型、编译链接和单元测试完成后，再进行 Gate 4.1 FINAL AUDIT。
