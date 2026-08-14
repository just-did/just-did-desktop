#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>

#include "common/Types.h"

class FileManager
{
public:
    FileManager() = default;

    QList<DailyRecord> readDailyFile(int year, int month, int day);
    bool existsDailyFile(int year, int month, int day);
    bool writeDailyFile(int year, int month, int day, const QList<DailyRecord> &records);
    bool deleteDailyFile(int year, int month, int day);
    QJsonObject getStats();

    // 同步快照：data/{YYYY}/{MM}/{batchId}-{DD}.txt
    static QString buildSnapshotPath(const QString &batchId, int year, int month, int day);
    static void removeSnapshot(const QString &batchId, int year, int month, int day);

    // 解析日报/暂存文件文本（\n\n 分块，首行时间，块内多行内容）
    static QList<DailyRecord> parseContent(const QString &text);

private:
    QString buildPath(int year, int month, int day) const;
    QString buildTmpPath(int year, int month, int day) const;
    QString buildDir(int year, int month) const;
    void removeEmptyDirs(const QString &path);
    QString serializeContent(const QList<DailyRecord> &records) const;
};
