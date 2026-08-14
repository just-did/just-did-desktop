#pragma once

#include <QString>
#include <QList>
#include <QPair>
#include <QSqlDatabase>
#include <optional>

#include "common/Types.h"

class DatabaseManager
{
public:
    bool open(const QString &dbPath);
    void close();

    std::optional<IndexEntry> getIndexEntry(int year, int month, int day);
    void upsertIndexEntry(int year, int month, int day, const QString &path, qint64 fileSize);
    QList<IndexEntry> getIndexByMonth(int year, int month);
    bool updateWithVersion(int year, int month, int day, const QString &path, qint64 fileSize, int expectedVersion);

    void removeIndexEntry(int year, int month, int day);

    // 批处理记录（batch_id 主键，status: 覆盖中/已完成）
    QString getBatchStatus(const QString &batchId);   // 记录不存在返回空字符串
    void insertBatchRecord(const QString &batchId, const QString &dates, const QString &status);
    void updateBatchStatus(const QString &batchId, const QString &status);
    QStringList getBatchDates(const QString &batchId);
    QList<QPair<QString, QString>> getCoveringBatches();   // 全部「覆盖中」批次 (batch_id, dates)

private:
    void migrate();
    void setPragmas();

    QSqlDatabase mDb;
};
