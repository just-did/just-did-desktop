## 1. 构建系统与项目骨架

- [x] 1.1 创建顶层 CMakeLists.txt（C++17, Qt6 模块, yaml-cpp FetchContent）
- [x] 1.2 创建 src/CMakeLists.txt（聚合四个子目录 + 最终可执行文件）
- [x] 1.3 创建 common/CMakeLists.txt（仅头文件库，依赖 Qt6::Core）
- [x] 1.4 创建 core/CMakeLists.txt（静态库，依赖 common + Qt6::Core + Qt6::Sql + yaml-cpp）
- [x] 1.5 创建 service/CMakeLists.txt（静态库，依赖 core + Qt6::Core + Qt6::Network + Qt6::HttpServer）
- [x] 1.6 创建 ui/CMakeLists.txt（静态库，依赖 service + Qt6::Core + Qt6::Quick + Qt6::Gui）
- [x] 1.7 验证空项目骨架可编译通过

## 2. common 层 — 共享数据结构

- [x] 2.1 实现 Types.h（DailyRecord, IndexEntry, BatchInfo, ConnectionState 枚举）
- [x] 2.2 实现 Constants.h（端口、心跳间隔、文件上限、拉取限制等）
- [x] 2.3 实现 ErrorCode.h（6 个错误码枚举）
- [x] 2.4 验证 common 层独立编译通过

## 3. core 层 — LogManager 日志管理器

- [x] 3.1 实现 LogManager 单例框架 + init() 初始化日志目录
- [x] 3.2 实现 debug/info/warn/error 四级日志方法（格式：[YYYY-MM-DD HH:MM:SS] [LEVEL] msg）
- [x] 3.3 实现文件滚动逻辑（4MB 触发 rotate：.log → .1.log → .2.log）
- [x] 3.4 实现总容量检查（超 12MB 删除最旧文件）
- [x] 3.5 添加 QMutex 线程安全保护
- [x] 3.6 验证 LogManager 编译通过 + 手动测试滚动

## 4. core 层 — ConfigManager 配置管理器

- [x] 4.1 实现 ConfigManager 框架 + load() 从 config.yml 读取
- [x] 4.2 实现默认值逻辑（文件缺失时使用 port=18080, heartbeat_timeout=45）
- [x] 4.3 实现 save() 持久化到 YAML 文件
- [x] 4.4 实现 port/heartbeatTimeout/floatingWindowPosition/mainWindowSize 的 getter/setter
- [x] 4.5 添加 QMutex 线程安全保护
- [x] 4.6 验证 ConfigManager 编译通过

## 5. core 层 — DatabaseManager 数据库管理器

- [x] 5.1 实现 open/close + PRAGMA journal_mode=WAL
- [x] 5.2 实现 migrate() 建表（pc_daliy_report_index + pc_processed_batches）
- [x] 5.3 实现 upsertIndexEntry / getIndexEntry / getIndexByMonth
- [x] 5.4 实现 updateWithVersion（乐观锁 UPDATE，返回 bool）
- [x] 5.5 实现 isBatchProcessed / insertBatch / getBatchDates
- [x] 5.6 验证 DatabaseManager 编译通过

## 6. core 层 — FileManager 文件管理器

- [x] 6.1 实现 buildPath/buildTmpPath（构建 data/YYYY/MM/DD.txt 路径）
- [ ] 6.2 实现 parseContent（解析 "HH:MM\ncontent\n\n" 格式 → QList<DailyRecord>）
- [ ] 6.3 实现 serializeContent（序列化 QList<DailyRecord> → 格式化文本，按时间排序）
- [ ] 6.4 实现 readDailyFile（读取现有日报文件）
- [ ] 6.5 实现 writeDailyFile（合并 + 排序 + 原子写入：.tmp → rename）
- [ ] 6.6 实现 deleteDailyFile（删除文件 + 清理空目录）
- [ ] 6.7 实现 recoverTmpFiles（启动时扫描 data/**/*.tmp → rename 恢复）
- [ ] 6.8 实现 getStats（遍历 data/ 目录统计总大小 + 按年月分类）
- [ ] 6.9 验证 FileManager 编译通过 + 手动测试读写

## 7. core 层 — DataManager 外观

- [x] 7.1 实现 DataManager 框架：持有 FileManager + DatabaseManager
- [x] 7.2 实现 addRecord（获取时间戳 + 读取文件 + 追加排序 + 乐观锁写入 + 更新索引）
- [x] 7.3 实现 mergeRecords（批量合并提交 → 乐观锁写入 + batch 记录 + 事务提交）
- [x] 7.4 实现 fetchFiles / getMonthIndex / getDailyRecords / clearDate / clearDateRange
- [x] 7.5 实现 dataChanged 信号（通知 UI 刷新）
- [x] 7.6 验证 DataManager 编译通过

## 8. core 层 — ConnectionStateMachine 连接状态机

- [x] 8.1 实现 5 种状态枚举 + 状态转移表（Unstarted/Disconnected/Connected/Syncing）
- [x] 8.2 实现 start/stop/onConnect/onSyncStart/onSyncComplete/onHeartbeatReceived
- [x] 8.3 实现心跳超时定时器（45s QTimer，收到心跳 reset）
- [x] 8.4 实现 stateChanged 信号
- [x] 8.5 验证 ConnectionStateMachine 编译通过

## 9. core 层 — AppCore 应用总管

- [x] 9.1 实现 AppCore 单例框架
- [x] 9.2 实现 init() 启动序列（LogManager → ConfigManager → DatabaseManager → FileManager 恢复 → DataManager → ConnectionStateMachine）
- [x] 9.3 实现 shutdown() 安全关闭
- [x] 9.4 实现 startupCompleted / startupFailed 信号
- [x] 9.5 验证 AppCore + 整个 core 层编译通过

## 10. service 层 — ReportService 本地日报服务

- [x] 10.1 实现 ReportService 框架：通过 AppCore 获取 DataManager 实例
- [x] 10.2 实现 addRecord（生成 HH:mm 时间戳 → 委派 DataManager::addRecord）
- [x] 10.3 实现 getMonthIndex / getDailyRecords / clearDate / clearDateRange
- [x] 10.4 实现 getStorageStats
- [x] 10.5 实现 backupToZip / restoreFromZip（提交到 QThreadPool 工作线程）
- [x] 10.6 验证 ReportService 编译通过

## 11. service 层 — SyncService 同步业务逻辑

- [x] 11.1 实现 parseSubmitBody（按 \n\n\n 分割日期块 → 按 \n\n 分割记录 → 按 \n 拆分时间/内容）
- [x] 11.2 实现 submit（batch_id 幂等检查 + parseSubmitBody + mergeRecords + 构造响应 JSON）
- [x] 11.3 实现 parseDates（dates 列表 / start-end 区间 → QList<QDate> + 上限校验）
- [x] 11.4 实现 fetch（查索引 + 读文件 + 单文件返回文本 / 多文件返回 ZIP）
- [x] 11.5 实现 connectDevice / healthCheck（构造响应 JSON）
- [x] 11.6 验证 SyncService 编译通过

## 12. service 层 — HttpServer HTTP 适配

- [x] 12.1 实现 HttpServer 框架：QHttpServer + QTcpServer 监听指定端口
- [x] 12.2 注册 POST /sync/connect 路由（解析 JSON body → SyncService::connect → 调 ConnectionStateMachine）
- [x] 12.3 注册 GET /health 路由（调 ConnectionStateMachine::onHeartbeatReceived → 返回 status/timestamp）
- [x] 12.4 注册 POST /sync/submit 路由（提取 X-Batch-ID Header + Body → SyncService::submit）
- [x] 12.5 注册 POST /sync/fetch 路由（解析 JSON → SyncService::fetch → 设置 Content-Type）
- [x] 12.6 实现局域网 IP 获取 + qrCodeUrl() 生成
- [x] 12.7 实现 start/stop/setPort
- [x] 12.8 验证 HttpServer 编译通过

## 13. ui 层 — ViewModel 层

- [x] 13.1 实现 CalendarViewModel（currentYear/Month + markedDays 属性 + loadMonth/prevMonth/nextMonth）
- [x] 13.2 实现 TimelineViewModel（selectedYear/Month/Day + records 属性 + selectDate/clearSelectedDate）
- [x] 13.3 实现 FloatingInputViewModel（inputText + isExpanded + isVisible 属性 + submitRecord/toggleExpand/showMainWindow）
- [x] 13.4 实现 StorageViewModel（totalSize + statsByYear 属性 + refreshStats/clearDateRange/backup/restore）
- [x] 13.5 实现 ConnectionViewModel（connectionState + qrCodeUrl + localIP + port 属性 + startServer/stopServer/setPort）
- [x] 13.6 验证所有 ViewModel 编译通过

## 14. ui 层 — Model 层

- [x] 14.1 实现 CalendarModel（QAbstractListModel：dayNumber/hasReport/isCurrentMonth roles）
- [x] 14.2 实现 RecordListModel（QAbstractListModel：time/content roles）
- [x] 14.3 验证 Model 层编译通过

## 15. ui 层 — QML 界面

- [x] 15.1 创建 main.qml（ApplicationWindow 入口 + 系统托盘注册）
- [x] 15.2 创建 MainWindow.qml（管理主窗口框架 + StackLayout）
- [x] 15.3 创建 CalendarView.qml（月份日历 GridView + 翻月按钮）
- [x] 15.4 创建 TimelineView.qml（单日记录 ListView + RecordItem delegate）
- [x] 15.5 创建 FloatingWindow.qml（悬浮窗：展开/收起状态 + 输入框 + 提交按钮）
- [x] 15.6 创建 StorageView.qml（概览统计 + 清理/备份/恢复操作）
- [x] 15.7 创建 StatusBar.qml（状态指示灯 + 状态文字 + QR 码 + 启动/停止按钮）
- [x] 15.8 创建 components/RecordItem.qml + components/ConfirmDialog.qml
- [x] 15.9 验证 QML 文件可被 QQmlApplicationEngine 加载

## 16. main.cpp + 端到端集成

- [x] 16.1 实现 main.cpp（QApplication 初始化 → AppCore::init() → QQmlApplicationEngine 加载 QML → ViewModel 注册）
- [x] 16.2 配置 ViewModel 的 contextProperty 注册（将 ViewModel 实例暴露给 QML）
- [x] 16.3 创建 .qrc 资源文件（已创建，当前使用文件系统路径加载）
- [x] 16.4 端到端验证：启动应用 → 显示悬浮窗 → 输入记录 → 生成 data/ 文件 → 管理界面查看
