## Context

"刚刚做了什么"电脑端是日报系统的唯一权威数据源。设计文档（`doc/` 目录下 5 份说明书）已完整定义业务流程、数据模型、接口规范和四层架构。本项目从零构建，无遗留代码。

## Goals / Non-Goals

**Goals:**
- 实现四层架构（common → core → service → ui），严格遵循自顶向下的依赖方向
- 通过 CMake `target_link_libraries` 在编译时强制边界约束
- 自底向上构建：每层可独立编译验证

**Non-Goals:**
- 不实现用户认证（设计明确仅局域网使用）
- 不实现单条记录编辑/删除（仅按天追加和清空）
- 不支持多设备同时连接
- 不实现搜索功能

## Decisions

### 构建顺序：自底向上

common → core → service → ui。每一层是上一层的编译依赖，必须按序实现。每层完成后即可编译验证，无需等上层完成。

### yaml-cpp 获取方式：CMake FetchContent

不用 vcpkg 或手动编译。在顶层 CMakeLists.txt 中 `FetchContent_Declare(yaml-cpp 0.8.0)`，首次 cmake 配置时自动从 GitHub 拉取并编译。零手动依赖。

### SQLite 连接：主线程独占 + WAL 模式

不引入独立数据库线程。开启 `PRAGMA journal_mode=WAL`，读不阻塞写，满足轻量场景。重 I/O（ZIP 压缩/解压）提交到 `QThreadPool`。

### HttpServer 职责边界

HttpServer 仅做 HTTP 适配（路由、Header 提取、Body 读取、StatusCode 设置），不包含业务逻辑。业务逻辑委托给 SyncService/ReportService。这使得 SyncService 可以不依赖 HTTP 层独立测试。

### 乐观锁实现位置

`DatabaseManager::updateWithVersion()` 提供带版本校验的 UPDATE。DataManager 在协调 FileManager 写入和 DatabaseManager 更新时使用此方法。冲突由调用方 (ReportService/SyncService) 决定重试策略。

### 日志系统自研，不引入第三方库

需求简单（格式化输出 + 文件滚动 + 容量限制），自研 LogManager 足够。使用 `QMutex` 保证线程安全。

## Risks / Trade-offs

- **Qt HttpServer 模块成熟度**：Qt 6.8 的 HttpServer 模块相对较新，API 可能不够完善。如遇问题，可降级为 `QTcpServer` + 手写 HTTP 解析。
- **单线程 SQLite**：主线程独占方案简单，但重写大量文件时可能阻塞 UI。WAL 模式可缓解读阻塞，写操作通过乐观锁保证正确性。
- **yaml-cpp FetchContent**：依赖 GitHub 网络连通性。首次构建需下载，之后缓存。国内网络可能较慢，可考虑手动指定镜像 URL。
