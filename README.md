# 校园雷锋日记 —— C++ 课程设计

## 1. 项目简介

“校园雷锋日记”是一个使用 **C++17 + Qt 6** 开发的校园志愿服务管理程序。

系统包含学生端和管理员端，主要功能包括：

- 学生 / 管理员登录；
- 学生提交、修改、删除志愿记录；
- 管理员审核志愿记录；
- 志愿积分统计；
- 月度 / 学期积分查询；
- 积分排行榜；
- 志愿徽章；
- 志愿日记发布与日记墙；
- 日记点赞及重复点赞限制；
- 学生、管理员账号创建；
- 密码修改；
- 文本文件持久化保存。

项目同时保留：

- Qt 图形界面程序：`leifeng_qt`
- 控制台程序：`leifeng_console`

---

## 2. 最简单的运行方法（Qt 与控制台）

### 2.1 直接运行 Qt 图形界面版

如果只需要运行最终图形界面程序，**不需要安装 Qt、MinGW 或 CMake**。

进入：

```text
release/
```

直接双击：

```text
校园雷锋日记.exe
```

即可启动 Qt 图形界面程序。

> 请保持 `release` 文件夹中的 DLL、插件目录和 `data` 目录完整，不要只单独复制 exe。

已经验证：将整个 `release` 目录复制到独立目录，并移除 Qt / MinGW 的 PATH 环境后，程序仍可以正常启动和运行。

### 2.2 运行控制台版

项目同时保留了控制台入口：

```text
console/main.cpp
```

对应的可执行目标名称为：

```text
leifeng_console
```

控制台程序读取的是项目根目录下的：

```text
data/
```

因此，**运行控制台程序时请保持 PowerShell 当前目录位于项目根目录**，不要进入 `build` 目录后再运行，也不建议直接双击 `build/leifeng_console.exe`，否则相对路径可能无法正确找到 `data/` 中的数据文件。

#### 方法一：使用 CMake 构建后运行（推荐）

先按照第 6 节配置 Qt、MinGW 和 CMake，然后在项目根目录执行：

```powershell
cmake --build build --target leifeng_console
```

构建完成后，仍在项目根目录运行：

```powershell
.\build\leifeng_console.exe
```

看到：

```text
校园雷锋日记系统启动成功。

1. 登录
0. 退出系统
```

即表示控制台版本启动成功。

#### 方法二：只使用 MinGW g++ 编译控制台版（无需 Qt）

如果只想检查控制台程序，并且 MinGW 的 `g++` 已经加入 `PATH`，可以在项目根目录直接执行：

```powershell
g++ -std=c++17 -Icore console/main.cpp core/user.cpp core/student.cpp core/administrator.cpp core/volunteer_record.cpp core/volunteer_category.cpp core/diary_post.cpp core/data_manager.cpp -o leifeng_console.exe
```

编译成功后运行：

```powershell
.\leifeng_console.exe
```

这种方式只编译控制台版本，**不需要安装 Qt，也不需要使用 CMake**；但需要本机已经安装可用的 MinGW g++。

控制台版与 Qt 版使用相同的演示账号，账号信息见下一节。

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
├── data/                       # 源码运行时使用的数据
│
└── release/                    # 可直接运行的发布版
    ├── 校园雷锋日记.exe
    ├── Qt6Core.dll
    ├── Qt6Gui.dll
    ├── Qt6Widgets.dll
    ├── ...
    ├── platforms/
    ├── imageformats/
    ├── styles/
    ├── translations/
    └── data/
```

其中：

- `core/`：领域类、数据管理和持久化实现；
- `qt/`：登录窗口、学生端、管理员端及统一样式；
- `console/`：控制台版本入口；
- `data/`：学生、管理员、志愿记录和日记数据；
- `release/`：已经部署好 Qt 运行库的独立运行版本。

---

## 5. 数据文件

程序使用文本文件保存数据：

```text
data/students.txt
data/administrators.txt
data/records.txt
data/diaries.txt
```

分别保存：

- 学生账号；
- 管理员账号；
- 志愿服务记录；
- 志愿日记与点赞数据。

请不要随意删除或修改这些文件中的字段分隔结构。

发布版运行时，应保留：

```text
release/data/
```

与 `校园雷锋日记.exe` 一起使用。

---

# 6. 从源码重新构建

## 6.1 已验证开发环境

本项目已在以下环境完成构建和测试：

```text
操作系统：Windows 11
C++ 标准：C++17
Qt：6.11.2
编译器：MinGW g++ 13.1.0
CMake：4.4.3
```

构建系统使用：

```text
MinGW Makefiles
```

推荐使用相同版本复现。

---

## 6.2 安装环境

需要安装：

1. **Qt 6.11.2**
2. **MinGW 13.1.0**
3. **CMake 3.16 或更高版本**

推荐通过 Qt 官方安装器安装 Qt，并同时安装对应的 MinGW 工具链。

例如 Qt 安装目录可能为：

```text
C:\Qt\6.11.2\mingw_64
C:\Qt\Tools\mingw1310_64
```

实际路径以本机安装位置为准。

---

## 6.3 配置工程

打开 PowerShell，进入项目根目录，例如：

```powershell
cd C:\path\to\leifeng_low
```

先设置本机 Qt 和 MinGW 路径。

示例：

```powershell
$QT = "C:\Qt\6.11.2\mingw_64"
$MINGW = "C:\Qt\Tools\mingw1310_64"
```

如果 Qt 安装在其他位置，只需要修改以上两个变量。

然后执行：

```powershell
cmake -S . -B build `
-G "MinGW Makefiles" `
-DCMAKE_PREFIX_PATH="$QT" `
-DCMAKE_CXX_COMPILER="$MINGW\bin\g++.exe" `
-DCMAKE_MAKE_PROGRAM="$MINGW\bin\mingw32-make.exe"
```

配置成功后执行：

```powershell
cmake --build build
```

构建完成后会得到：

```text
build/leifeng_qt.exe
build/leifeng_console.exe
```

---

## 6.4 运行源码构建版本

### Qt 图形界面版

运行 Qt 版本之前，将 Qt 和 MinGW 的运行库加入当前 PowerShell 的 PATH：

```powershell
$env:PATH = "$QT\bin;$MINGW\bin;" + $env:PATH
```

然后在项目根目录运行：

```powershell
.\build\leifeng_qt.exe
```

### 控制台版

控制台版本不链接 Qt Widgets。构建完成后，请保持当前目录位于项目根目录，再运行：

```powershell
.\build\leifeng_console.exe
```

控制台程序使用相对路径读取 `data/students.txt`、`data/administrators.txt`、`data/records.txt` 和 `data/diaries.txt`，因此从项目根目录启动最稳妥。

如果只希望编译控制台版，也可以使用第 2.2 节给出的 MinGW g++ 命令，无需运行 Qt 图形界面程序。

---

# 7. 重新生成独立发布版

如果重新编译了程序，并希望重新生成一个无需安装 Qt 即可运行的发布目录，可执行以下步骤。

## 7.1 创建 release 目录

```powershell
New-Item -ItemType Directory -Force release
```

复制 Qt 程序：

```powershell
Copy-Item .\build\leifeng_qt.exe .\release\校园雷锋日记.exe
```

复制数据：

```powershell
Copy-Item -Recurse .\data .\release\data
```

---

## 7.2 部署 Qt 运行库

执行 Qt 自带的 `windeployqt`：

```powershell
& "$QT\bin\windeployqt.exe" .\release\校园雷锋日记.exe
```

该工具会自动复制程序需要的 Qt DLL 和插件，例如：

```text
Qt6Core.dll
Qt6Gui.dll
Qt6Widgets.dll
platforms/qwindows.dll
```

以及程序所需的其他运行库。

完成后整个：

```text
release/
```

目录即可作为独立运行版本分发。

---

# 8. 独立运行验证方法

为了确认发布版没有依赖开发电脑上的 Qt 环境，可以进行以下测试。

先备份当前 PATH：

```powershell
$oldPath = $env:PATH
```

临时只保留 Windows 基本路径：

```powershell
$env:PATH = "C:\Windows\System32;C:\Windows"
```

进入发布目录：

```powershell
cd release
```

运行：

```powershell
.\校园雷锋日记.exe
```

如果程序能够正常启动、登录并读取数据，即说明发布目录中的依赖完整。

测试完成后恢复 PATH：

```powershell
$env:PATH = $oldPath
```

本项目最终发布版已经通过上述方式验证。

---

# 9. 主要功能说明

## 学生端

学生登录后可以：

- 查看个人信息；
- 修改登录密码；
- 提交志愿服务记录；
- 查看自己的志愿记录；
- 修改待审核或已驳回记录；
- 删除待审核或已驳回记录；
- 查看总积分；
- 查询月度积分；
- 查询指定学期时间范围内的积分；
- 查看积分排行榜；
- 查看不同志愿类别的徽章；
- 使用审核通过的志愿记录发布日记；
- 查看日记墙；
- 点赞其他日记。

其中：

- `Pending`：待审核；
- `Approved`：审核通过；
- `Rejected`：审核驳回。

审核通过的志愿记录不能再修改或删除。

被驳回的记录修改后会重新进入待审核状态。

---

## 管理员端

管理员登录后可以：

- 查看系统基本统计信息；
- 查看全部志愿记录；
- 按审核状态筛选记录；
- 查看志愿记录详情；
- 通过或驳回待审核记录；
- 查看分类统计；
- 查看积分排行榜；
- 创建学生账号；
- 创建管理员账号；
- 查看个人信息；
- 修改自己的登录密码。

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
