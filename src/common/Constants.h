#pragma once

#include <QtGlobal>

namespace Constants {

// 网络
constexpr int DEFAULT_PORT = 18080;
constexpr int HEARTBEAT_INTERVAL_SEC = 15;   // 手机端心跳间隔

// 拉取限制
constexpr int MAX_FETCH_FILES = 32;
constexpr int MAX_FETCH_DAY_SPAN = 31;

// 提交批数据限制
constexpr qint64 MAX_BATCH_UNZIPPED_SIZE = 5 * 1024 * 1024;  // ZIP 解压总大小上限 5MB

// 批处理记录状态
inline constexpr const char *BATCH_STATUS_COVERING = "覆盖中";
inline constexpr const char *BATCH_STATUS_DONE = "已完成";

// 日志
constexpr qint64 LOG_FILE_MAX_SIZE = 4 * 1024 * 1024;    // 4 MB
constexpr qint64 LOG_TOTAL_MAX_SIZE = 12 * 1024 * 1024;  // 12 MB
constexpr int LOG_MAX_FILES = 3;

// 数据
constexpr int RECORD_MAX_CHARS = 2000;  // 单条记录最大字符数

} // namespace Constants
