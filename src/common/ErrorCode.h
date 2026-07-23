#pragma once

enum class ErrorCode {
    Success = 0,
    StorageError = 1,          // 文件写入失败
    InternalError = -1,        // 内部错误 / 解析失败
    TooManyFiles = -2,         // 请求文件数超限
    InvalidParameter = -3,     // 参数无效
    VersionConflict = -4       // 乐观锁版本冲突
};
