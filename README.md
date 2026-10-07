# 校园雷锋日记 —— C++ 课程设计

## 1. 项目简介

“校园雷锋日记”是一个使用 **C++17、Qt 6 和本地文本文件**实现的校园志愿服务课程设计。

Qt 是正式课程提交与演示目标。控制台程序 `leifeng_console` 仅作为兼容性历史 / 调试入口，保持可构建，不追求与 Qt 功能对等。

Qt 学生端和管理员端提供：

- 学生提交、查询、修改和删除志愿记录；管理员审核，并记录最终类别、时长、积分及审核说明；
- 总积分、月度积分、手动日期范围积分查询；累计总积分排行榜、Top 3 雷锋之星和类别徽章；
- 日记展示申请、管理员审核与下架、公开日记墙及点赞 / 取消点赞；
- 学生与管理员账号创建、个人资料和密码修改；
- 管理员 OperationLog 页面，查询志愿记录与日记管理事件。

“学期积分”是现有的手动日期范围查询，不代表系统实现了 Semester 子系统。

---

## 2. 最简单的运行方法（正式 Qt 产品）

### 2.1 运行 Qt 发布包

课程演示时推荐使用 `release/`。正确部署后无需在目标电脑安装 Qt、MinGW 或 CMake。

打开完整的 `release/` 目录并运行：

```text
校园雷锋日记.exe
```

不要只复制 exe；同目录 Qt / MinGW 运行库、插件目录和 `data/` 都需要保留。Qt 从 `<exe 所在目录>/data/` 读取运行数据，五个数据文件都必须存在且可读。

### 2.2 Console 兼容入口

`leifeng_console` 是兼容性历史 / 调试入口，正式维护的构建方式是 CMake。构建方法见第 6 节。Console 使用相对 `data/` 路径，运行时应从项目根目录启动；课程演示优先使用 Qt 发布包。

Console 和 Qt 使用同一份课程演示数据。账号信息见下一节。

---

## 3. 演示账号

### 学生账号

```text
账号：20250001
密码：20061015
```

### 管理员账号

```text
账号：admin001
密码：admin123
```

也可以使用管理员端创建新的学生或管理员账号。

---

## 4. 项目目录说明

推荐的最终目录结构如下：

```text
leifeng_low/
│
├── README.md
├── CMakeLists.txt
│
├── core/                       # 核心数据类与业务数据管理
├── qt/                         # Qt 图形界面
├── console/                    # 控制台入口
├── tests/                      # Core 行为测试
├── docs/                       # 需求、架构与开发路线文档
├── data/                       # 源码运行时使用的五个数据文件
│
└── release/                    # 可直接运行的发布版
    ├── 校园雷锋日记.exe
    ├── Qt / MinGW 运行库
    ├── platforms/
    ├── 其他 windeployqt 识别的插件目录
    └── data/                   # 发布包自己的五个数据文件
```

其中：

- `core/`：领域类、服务和本地数据管理；
- `qt/`：正式 Qt 界面；
- `console/`：兼容性历史 / 调试入口；
- `tests/`：不访问仓库源数据的 Core 行为测试；
- `data/`：本地演示数据；
- `release/`：本地生成的独立 Qt 运行包（按第 8 节验证后使用），Git 忽略该目录。

---

## 5. 运行数据文件

Qt 从可执行文件旁的 `data/` 读取本地运行数据。源码和发布目录都必须包含以下五个可读文件：

```text
data/students.txt
data/administrators.txt
data/records.txt
data/diaries.txt
data/operation_logs.csv
```

发布包中的位置必须是：

```text
<exe 所在目录>/data/
```

程序启动时会加载全部五个文件；缺失或无法读取时，会在打开登录界面前提示启动失败。`release/data/` 是发布包自己的数据副本，运行时不会回写仓库源 `data/`。

这些文件使用项目定义的分隔与字段格式，请勿手工改动结构。仓库中的数据用于课程演示；分发前应由维护者确认数据为虚构样例且不含真实账号或个人信息。

---

# 6. 从源码构建

## 6.1 已验证工具链

最近验证的 Windows 构建环境：

```text
C++：C++17
Qt：6.11.2
MinGW g++：13.1.0
Ninja：1.12.1
CMake：4.4.3（项目最低要求 3.16）
```

CMake 是维护的正式构建方式。Qt 是正式产品目标，Console 仅为兼容性历史 / 调试目标。

## 6.2 工具安装

安装 Qt 6.11.2（包含 Qt Widgets 和 MinGW 13.1.0）、CMake 3.16 或更高版本，以及 Ninja。Qt 和 MinGW 必须来自匹配的 Qt Kit。

下面的目录只是示例；按本机安装位置设置变量：

```powershell
$QT = 'C:\Qt\6.11.2\mingw_64'
$MINGW = 'C:\Qt\Tools\mingw1310_64'
$NINJA = 'C:\Tools\Ninja\ninja.exe'
```

## 6.3 全新 Release 配置、构建与测试

在 PowerShell 中从项目根目录运行。每次干净配置都使用一个新的空构建目录：

```powershell
$PROJECT_ROOT = (Resolve-Path .).Path
$BUILD_DIR = Join-Path $PROJECT_ROOT 'build\release-ninja'
$QT = 'C:\Qt\6.11.2\mingw_64'
$MINGW = 'C:\Qt\Tools\mingw1310_64'
$NINJA = 'C:\Tools\Ninja\ninja.exe'

cmake -S $PROJECT_ROOT -B $BUILD_DIR -G Ninja `
  -DCMAKE_MAKE_PROGRAM="$NINJA" `
  -DCMAKE_CXX_COMPILER="$MINGW\bin\g++.exe" `
  -DCMAKE_PREFIX_PATH="$QT" `
  -DCMAKE_BUILD_TYPE=Release

cmake --build $BUILD_DIR --target leifeng_qt leifeng_console core_behavior_tests --parallel 8
ctest --test-dir $BUILD_DIR --output-on-failure
```

如果该构建目录已经存在，请为下一次 fresh configure 选择新的目录名。CMake 配置时会把缺失的初始数据文件复制到构建目录，不覆盖已经存在的运行数据。

### Windows 非 ASCII 路径

如果项目路径包含中文或其他非 ASCII 字符，且 Qt AutoMoc 报 `Invalid argument` 路径错误，优先从纯 ASCII 路径构建。也可在本机使用同一目录的 Windows 8.3 短路径；`LEIFEN~2` 只是本机示例，不是可移植路径。

## 6.4 运行源码构建版本

Qt 使用 exe 同级的构建运行数据目录；可以从任意当前工作目录启动：

```powershell
$oldPath = $env:PATH
try {
    $env:PATH = "$QT\bin;$MINGW\bin;" + $oldPath
    & "$BUILD_DIR\leifeng_qt.exe"
}
finally {
    $env:PATH = $oldPath
}
```

Console 是兼容入口，使用相对 `data/` 路径，运行时应从项目根目录启动：

```powershell
Set-Location $PROJECT_ROOT
& "$BUILD_DIR\leifeng_console.exe"
```

不再维护单独的手工 `g++` 源文件清单；CMake 是 Qt 与 Console 的正式构建入口。

---

# 7. 重新生成独立 Qt 发布包

发布目录 `/release/` 被 Git 忽略，不进入 Git 历史。用新的临时目录先构建和验证包，再将通过验证的目录放置为仓库根目录下的 `release/`。

在 PowerShell 中设置当前工具链路径，并从 Release 构建复制 Qt 程序：

```powershell
$PROJECT_ROOT = (Resolve-Path .).Path
$BUILD_DIR = Join-Path $PROJECT_ROOT 'build\release-ninja'
$PACKAGE_DIR = Join-Path $PROJECT_ROOT 'build\release-package'
$QT = 'C:\Qt\6.11.2\mingw_64'
$MINGW = 'C:\Qt\Tools\mingw1310_64'
$PACKAGE_EXE = Join-Path $PACKAGE_DIR '校园雷锋日记.exe'

New-Item -ItemType Directory -Path $PACKAGE_DIR
Copy-Item (Join-Path $BUILD_DIR 'leifeng_qt.exe') $PACKAGE_EXE
$oldPath = $env:PATH
try {
    $env:PATH = "$QT\bin;$MINGW\bin;" + $oldPath
    & "$QT\bin\windeployqt.exe" --release --compiler-runtime --dir $PACKAGE_DIR $PACKAGE_EXE
}
finally {
    $env:PATH = $oldPath
}

New-Item -ItemType Directory -Path (Join-Path $PACKAGE_DIR 'data')
$requiredData = @('students.txt', 'administrators.txt', 'records.txt', 'diaries.txt', 'operation_logs.csv')
foreach ($name in $requiredData) {
    Copy-Item (Join-Path $PROJECT_ROOT "data\$name") (Join-Path $PACKAGE_DIR "data\$name")
}
```

`windeployqt` 会依据 Release 程序部署 Qt 运行库和插件，并通过 `--compiler-runtime` 部署 MinGW 运行库。至少确认 `platforms/qwindows.dll` 存在。不要只分发 exe；整个目录都属于运行包。

验证包通过后再用它替换旧 `release/`。最终目录应包含 `校园雷锋日记.exe`、部署所需 DLL / 插件，以及上述五个 `data/` 文件。

---

# 8. 发布包独立启动与冒烟验证

先确认包目录中的五个数据文件齐全。为排除 Qt / MinGW 开发环境依赖，在 PowerShell 中临时将 PATH 限定为 Windows 系统目录，再启动发布程序。关闭程序后，`finally` 会恢复原 PATH：

```powershell
$oldPath = $env:PATH
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    & .\release\校园雷锋日记.exe
}
finally {
    $env:PATH = $oldPath
}
```

到达登录界面表示启动阶段已成功加载 exe 同级的五个数据文件。持久化回归必须在 `release/` 的临时副本中进行，不得用仓库源 `data/`。

**本地验证记录（2026-10-07）：** fresh Release 构建的 Qt、Console 和 Core 测试目标均构建成功，CTest 通过；发布目录在仅含 Windows 系统目录的 PATH 下启动并到达登录界面。隔离副本中的学生登录、学生主页、退出登录、管理员登录、创建学生，以及创建的测试学生在关闭并重启后再次登录均通过。仓库源 `data/` 未改变。此记录覆盖启动、登录路由和基本持久化，不代表对所有页面完成了逐页人工视觉审查。

完整目录复制到独立目录后也应重复启动验证。后续发布仍应按本节步骤复核实际使用的发布包。

---

# 9. 主要功能说明

## 学生端

学生登录后可以：

- 查看个人信息；
- 修改登录密码；
- 提交志愿服务记录；
- 查询、修改和删除自己的志愿记录；
- 查看总积分；
- 查询月度积分；
- 按手动设置的日期范围查询积分（页面现有名称为“学期积分”，不含 Semester 管理）；
- 查看累计总积分排行榜、Top 3 雷锋之星和类别徽章；
- 使用审核通过的志愿记录提交日记展示申请；
- 查看日记申请状态和公开日记墙；
- 点赞或取消点赞其他日记。

志愿记录审核结果分为：

- `Pending`：待审核；
- `Approved`：审核通过；
- `Rejected`：审核驳回。

管理员审核后，记录保留最终类别、最终时长、最终积分和审核说明。审核通过的志愿记录不能再修改或删除；被驳回的记录修改后会重新进入待审核状态。

日记展示申请由管理员审核；日记可能处于待审核、已展示、已驳回或已下架状态。只有获准展示的日记进入公开日记墙。

---

## 管理员端

管理员登录后可以：

- 查看现有统计信息和积分排行；
- 查询并审核志愿记录，查看最终类别、时长、积分、审核说明和审核人；
- 创建学生账号；
- 创建管理员账号；
- 管理日记展示申请，并对已展示日记执行治理下架；
- 在 OperationLog 页面查看志愿记录审核与日记管理事件；
- 查看个人信息并修改自己的登录密码。

学生端不提供 Student Management、停用或恢复学生等功能。

---

# 10. 积分规则

志愿积分按照：

```text
积分 = 志愿服务时长 × 类别系数
```

计算。

当前类别系数：

| 志愿类别 | 系数 |
|---|---:|
| 劳动服务 | 2.0 |
| 环保服务 | 1.5 |
| 互助服务 | 1.0 |

只有审核通过的志愿记录计入积分。

---

# 11. 徽章规则

徽章根据各类别**审核通过的累计志愿时长**计算。

| 累计时长 | 等级 |
|---|---|
| 10 小时及以上 | 铜级 |
| 30 小时及以上 | 银级 |
| 60 小时及以上 | 金级 |

三类专项徽章分别为：

```text
劳动服务：劳动先锋
环保服务：环保卫士
互助服务：互助之星
```

---

# 12. 持久化说明

系统采用文本文件持久化。

为避免字段分隔符破坏文件结构，用户输入中的：

```text
|
换行
```

会在相关输入位置受到限制。

程序生成的志愿记录编号和日记编号会基于现有最大编号继续递增，避免删除旧记录后出现编号重复。

---

# 13. 说明

本项目为 C++ 课程设计项目，重点展示：

- C++ 类与对象；
- 继承和虚函数；
- STL 容器；
- 自定义类模板；
- 枚举状态；
- 文件读写；
- 数据查询和排序；
- Qt Widgets 图形界面开发；
- 基础分层设计；
- 数据持久化。

如仅进行课程作业检查，推荐直接运行：

```text
release/校园雷锋日记.exe
```

无需配置开发环境。
