#pragma once

enum class ErrorCode {
    Success = 0,
    StorageError = 1,          // 文件写入失败
    InternalError = -1,        // 内部错误 / 解析失败
    TooManyFiles = -2,         // 请求超限（fetch 文件数 / submit 批大小）
    InvalidParameter = -3,     // 参数无效
    VersionConflict = -4,      // 乐观锁版本冲突（仅本地记录使用，提交接口已废弃）
    SyncBusy = -5              // 同步处理中（锁占用）
};
