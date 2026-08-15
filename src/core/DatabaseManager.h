#pragma once

#include <QString>
#include <QList>
#include <QPair>
#include <QDate>
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
    // 按日期列表查询索引条目（按日期升序）；SQL 执行失败返回 false
    bool getIndexByDates(const QList<QDate> &dates, QList<IndexEntry> &out);
    // 查询严格早于 threshold 的索引条目（按日期升序）；SQL 执行失败返回 false
    bool getIndexOlderThan(const QDate &threshold, QList<IndexEntry> &out);
    bool updateWithVersion(int year, int month, int day, const QString &path, qint64 fileSize, int expectedVersion);

    void removeIndexEntry(int year, int month, int day);

    // 批处理记录（batch_id 主键，status: 覆盖中/已完成）
    QString getBatchStatus(const QString &batchId);   // 记录不存在返回空字符串
    void insertBatchRecord(const QString &batchId, const QString &dates, const QString &status);
    void updateBatchStatus(const QString &batchId, const QString &status);
    QStringList getBatchDates(const QString &batchId);
    QList<QPair<QString, QString>> getCoveringBatches();   // 全部「覆盖中」批次 (batch_id, dates)
    // 是否存在「覆盖中」批次；SQL 执行失败返回 false
    bool hasPendingBatches(bool &out);

private:
    void migrate();
    void setPragmas();

    QSqlDatabase mDb;
};
