## Why

"刚刚做了什么"日报系统的设计阶段已完成（5 份设计文档 v1.1/v1.2），需要将设计转化为可运行的 C++/Qt 桌面应用。电脑端是整个系统的唯一权威数据源，承担本地记录、HTTP 服务和全量存储三项核心职责。现在开始编码实现。

## What Changes

从零构建完整的电脑端应用，包含四层架构：

- **common 层**：纯数据结构（DailyRecord、IndexEntry、ErrorCode 等），无行为逻辑
- **core 层**：7 个基础设施模块（日志、配置、数据库、文件、外观数据管理、状态机、应用总管）
- **service 层**：3 个业务服务（本地日报操作、同步逻辑、HTTP 适配）
- **ui 层**：QML 界面 + C++ ViewModel/Model（悬浮窗、管理界面、系统托盘、日历、时间线、存储管理、连接状态）
- **构建系统**：CMake 项目配置（含 yaml-cpp FetchContent 自动获取）

## Capabilities

### New Capabilities

- `build-system`: CMake 项目骨架，含四层静态库、FetchContent 获取 yaml-cpp、Qt 6.8 模块依赖
- `common-layer`: 共享数据结构与常量（Types.h、Constants.h、ErrorCode.h）
- `core-layer`: 基础设施层（LogManager、ConfigManager、DatabaseManager、FileManager、DataManager、ConnectionStateMachine、AppCore）
- `service-layer`: 业务服务层（ReportService、SyncService、HttpServer）
- `ui-layer`: QML 用户界面与 C++ ViewModel/Model（悬浮窗、管理主窗口、系统托盘、日历、时间线、存储管理、连接状态）

### Modified Capabilities

<!-- 全新项目，无已有 specs 需修改 -->

## Impact

- **代码**：新建约 30 个 C++ 源文件 + 8 个 QML 文件 + 5 个 CMakeLists.txt
- **依赖**：Qt 6.8（Core/Quick/Network/Sql/HttpServer）、yaml-cpp 0.8.0（FetchContent）、MSVC 2022 编译器
- **数据**：首次运行自动创建 data/、logs/、just_do.db、config.yml
- **不影响**任何已有代码（全新项目）
