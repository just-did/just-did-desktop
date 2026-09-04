#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>

#include "common/Types.h"

class FileManager
{
public:
    explicit FileManager(const QString &dataRoot);

    QList<DailyRecord> readDailyFile(int year, int month, int day);
    bool existsDailyFile(int year, int month, int day);
    bool writeDailyFile(int year, int month, int day, const QList<DailyRecord> &records);
    bool deleteDailyFile(int year, int month, int day);
    QJsonObject getStats();

    // 相对形态（data/YYYY/MM/DD.txt）：索引存储、API 透传、备份 zip 条目名
    QString buildRelativePath(int year, int month, int day) const;
    // 绝对形态（{dataRoot}/data/YYYY/MM/DD.txt）：磁盘 IO
    QString buildAbsolutePath(int year, int month, int day) const;
    QString buildTmpPath(int year, int month, int day) const;
    // 按索引 path 解析物理路径（{dataRoot}/ + index.path）：索引驱动读取的唯一入口
    QString resolveIndexPath(const QString &indexPath) const;

    // 同步快照：data/{YYYY}/{MM}/{batchId}-{DD}.txt
    QString buildSnapshotPath(const QString &batchId, int year, int month, int day) const;
    void removeSnapshot(const QString &batchId, int year, int month, int day);

    static QString normalizeRecordContent(const QString &content);
    static QList<DailyRecord> parseContent(const QString &text);
    static QString serializeRecords(const QList<DailyRecord> &records);

private:
    QString buildDir(int year, int month) const;
    QString dataDir() const;
    void removeEmptyDirs(const QString &path);

    const QString mDataRoot;
};
