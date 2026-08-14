#pragma once

#include <QString>
#include <QStringList>

// 单条日报记录
struct DailyRecord {
    QString time;      // "HH:MM"
    QString content;   // 记录正文
};

// SQLite 索引条目
struct IndexEntry {
    int year = 0;
    int month = 0;
    int day = 0;
    QString path;      // 相对路径，如 "2027/07/21.txt"
    qint64 fileSize = 0;
    int version = 1;   // 乐观锁版本号
};

// 批处理记录
struct BatchInfo {
    QString batchId;
    QStringList dates; // "20260723", "20260724", ...
};
