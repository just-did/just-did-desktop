# 刚刚做了什么 · Just Did（电脑端）

「刚刚做了什么」日报系统的**电脑端**，是整个系统的**唯一权威数据源**。它在本机记录日常事务、通过局域网 HTTP 服务接收手机端提交的数据，并全量存储所有日报内容。配合手机端 App 使用：电脑端启动服务后，手机扫码即可接入提交。

## 功能特性

- **悬浮输入窗**：屏幕右侧始终置顶的快捷记录窗口，未激活时缩起、单击展开输入、双击唤起主窗口
- **完整管理界面**：日历视图（有记录的日期带绿点）+ 时间线查看 + 存储统计与管理（按周/月/年阈值批量清理，带确认门槛）
- **扫码连接**：主界面显示局域网二维码，手机端扫码即可获取服务地址
- **HTTP 同步服务**：4 个 REST 接口（存活探测 / 提交 / 拉取 / 索引拉取），支持手机端数据互通
- **两阶段同步**：提交先「暂存」再「覆盖」，任一步失败可断点续跑；按批 ID 幂等，重复提交直接返回成功
- **启动自动恢复**：中断的同步批次在下次启动时自动完成覆盖并刷新界面
- **本地隐私**：所有日报数据仅存本机文件与本地 SQLite 索引，绝不出网
- **一键便携打包**：PowerShell 脚本自动完成编译 → Qt 运行库部署 → 压缩，产出解压即用的便携 zip

## 架构

```
┌──────────────────────────────────────────┐
│              电脑端（主端）               │
│                                          │
│  ┌──────────┐  ┌──────────────────┐     │
│  │ 悬浮输入窗 │  │   完整管理界面     │     │
│  │ (仅记录)  │  │  (日报查看+管理)   │     │
│  └────┬─────┘  └────────┬─────────┘     │
│       │                 │                │
│       └────────┬────────┘                │
│                ▼                         │
│  ┌──────────────────────────┐           │
│  │       本地存储             │           │
│  │  data/YYYY/MM/DD.txt     │           │
│  │  just_do.db (索引)        │           │
│  └────────────┬─────────────┘           │
│               │                          │
│  ┌────────────┴─────────────┐           │
│  │       HTTP 服务端         │           │
│  │  GET  /health            │◄── 手机端  │
│  │  POST /sync/submit       │◄── 手机端  │
│  │  POST /sync/fetch        │──► 手机端  │
│  │  POST /sync/fetch-index  │──► 手机端  │
│  └──────────────────────────┘           │
└──────────────────────────────────────────┘
```

代码按四层静态库组织，依赖方向为 `justdid_ui → justdid_service → justdid_core → justdid_common`：

| 层 | 目录 | 职责 |
|------|------|------|
| UI | `src/ui/` | QML 界面、ViewModels/Models、二维码生成 |
| 服务 | `src/service/` | HTTP 服务、同步状态机、日报查询/清理封装 |
| 核心 | `src/core/` | 配置、日志、SQLite、文件读写、业务逻辑 |
| 公共 | `src/common/` | 纯头文件：数据结构、错误码、常量 |

## 技术栈

| 组件 | 用途 |
|------|------|
| C++17 | 主语言 |
| Qt 6.8（Core / Quick / Network / Sql / HttpServer） | UI + 网络 + SQLite + HTTP 服务 |
| CMake 3.20+ | 构建 |
| SQLite | 日历索引与同步批次状态 |
| yaml-cpp | 配置文件解析 |
| minizip-ng | ZIP 解压（同步提交解析） |
| PowerShell | 一键打包脚本 |

## 构建

### 前置条件

- Windows 10/11
- Visual Studio 2022（含「使用 C++ 的桌面开发」工作负载）
- Qt 6.8 `msvc2022_64` 套件（需含 Qt HTTP Server 模块，Qt 6.4+ 已随 Qt 基础模块提供）
- CMake 3.20+

yaml-cpp 与 minizip-ng 由 CMake FetchContent 自动拉取，无需手动安装，但**首次配置需要联网**。

### 配置与编译

```bash
cmake -B build -S . -DCMAKE_PREFIX_PATH="C:/Qt/6.8.0/msvc2022_64"
cmake --build build --config Release
```

`-DCMAKE_PREFIX_PATH` 请指向**你自己的** Qt msvc2022_64 安装目录（上面是示例值；路径含空格时务必加引号）。产物为 `build/src/Release/JustDid.exe`。

> 若 `cmake` 不在 PATH 中，用完整路径调用即可，例如
> `"C:/Program Files/CMake/bin/cmake.exe" -B build -S . -DCMAKE_PREFIX_PATH=...`。

### 运行

任选其一：

1. 用 Qt 自带的 `windeployqt` 把 Qt 运行库部署到 exe 所在目录（见下方打包脚本，或手动执行）；
2. 把 Qt 的 `bin` 目录加入 PATH 后直接运行 exe。

### 常见问题

- **修改 CMakeLists.txt 后重新配置静默失败**（只打印一行就退出）：删除 `build/CMakeCache.txt` 与 `build/CMakeFiles` 后重新配置即可（部分缓存会导致增量配置失败）。
- **不要整目录删除 `build/`**：其中包含 windeployqt 部署的 Qt DLL（不在 CMake 构建清单里），推平后需重新部署 DLL 才能运行。

## 打包发布

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package.ps1 [-Version 1.0.0]
```

四步流水线：编译 → windeployqt 部署 Qt 运行库 → 组装暂存目录（自动剔除 `config.yml`、`just_do.db`、`data/`、`logs/` 等运行时脏数据）→ 压缩。产物为 `dist/JustDid_v<版本>_portable.zip`，解压即用。

- 版本号需与 `src/app.rc` 的 VERSIONINFO 同步修改
- 打包工具路径自动探测（VS 内置 CMake 经 vswhere 定位、Qt 套件在常见目录扫描取最高版本），也可用环境变量显式指定：`JUSTDID_CMAKE`、`JUSTDID_WINDEPLOYQT`

## HTTP API

4 个 REST 接口均通过局域网访问（服务启动后主界面显示二维码，手机端扫码获取地址）：

| 接口 | 方法 | 说明 |
|------|------|------|
| `/health` | GET | 存活探测，手机端每 15s 调一次，无状态副作用 |
| `/sync/submit` | POST | 接收手机端 ZIP 批数据，两阶段合并入本地日报文件 |
| `/sync/fetch` | POST | 按日期列表/范围返回日报内容 ZIP |
| `/sync/fetch-index` | POST | 按日期列表/范围返回索引条目（仅路径与大小，不含内容） |

错误码：

| 码 | 含义 |
|----|------|
| `0` | 成功（含幂等重复提交） |
| `1` | 存储错误（500）或请求日期均无数据（404） |
| `-1` | 内部错误（500） |
| `-2` | 请求超限（400） |
| `-3` | 参数无效（400） |
| `-5` | 同步处理中（429） |

（`-4` 版本冲突已废弃。）

要点：

- **幂等与断点续跑**：提交携带 `X-Batch-ID` 请求头，服务端以批 ID 为主键记录状态；「已完成」的重复请求直接返回成功；中断的批次下次同 ID 请求断点续跑
- **拉取限制**：日期列表 ≤32 个，或区间跨度 ≤31 天
- **同步互斥**：同一时刻至多一个同步处理，锁占用时返回 `-5`

## 数据存储

```
{应用根目录}/
  data/
    {YYYY}/{MM}/{DD}.txt   # 日报内容，UTF-8
  logs/
    just-did.log           # 运行日志（4MB 滚动）
  just_do.db               # SQLite 索引
  config.yml               # 配置文件（端口、窗口位置等）
```

- 日报文件格式：`HH:MM` 开头的时间块，块内多条记录直接换行，块间空行分隔；记录按时间递增排序
- SQLite 仅存索引：日历索引表（日期、路径、大小、乐观锁版本）与同步批次表（批 ID、受影响日期、状态）；**日历视图只查索引，点击具体日期才读文件**，内容与索引分离
- **隐私声明：所有数据只保存在你自己的电脑上**，应用不包含任何数据上传逻辑

## 许可证

本项目以 [Apache License 2.0](LICENSE) 开源，版权归 zhouyp001 所有。应用图标为作者自制。

本项目使用以下第三方组件，其版权归各自所有者：

| 组件 | 许可证 |
|------|--------|
| Qt 6.8 | LGPL-3.0 / GPL-3.0（本项目动态链接，遵循 LGPL 条款） |
| yaml-cpp | MIT |
| minizip-ng（含 zlib） | zlib License |
| qrcodegen（Project Nayuki） | MIT |

## 致谢

- [Project Nayuki](https://www.nayuki.io/) 的 QR Code 生成库
- [yaml-cpp](https://github.com/jbeder/yaml-cpp)、[minizip-ng](https://github.com/zlib-ng/minizip-ng)、Qt 项目
