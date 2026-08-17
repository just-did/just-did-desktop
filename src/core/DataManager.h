#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QDate>
#include <QMap>

#include "common/Types.h"
#include "common/ErrorCode.h"

class FileManager;
class DatabaseManager;

class DataManager : public QObject
{
    Q_OBJECT
public:
    explicit DataManager(FileManager *fileMgr, DatabaseManager *dbMgr, QObject *parent = nullptr);

    // Local record
    ErrorCode addRecord(int year, int month, int day, const QString &time, const QString &content);

    // Sync operations
    ErrorCode mergeRecords(const QMap<QDate, QList<DailyRecord>> &recordsByDate, const QString &batchId);
    // 覆盖单元（提交路径与启动恢复共用）：逐日快照 rename 为正式文件并更新索引，
    // 快照缺失跳过（断点续跑），完成后置批次「已完成」
    ErrorCode coverBatch(const QString &batchId, const QList<QDate> &dates);
    QList<IndexEntry> getUpdatedIndexForBatch(const QString &batchId);

    // Queries
    QList<IndexEntry> getMonthIndex(int year, int month);
    // 按日期列表查索引条目（升序）；SQL 异常返回 InternalError
    ErrorCode getIndexForDates(const QList<QDate> &dates, QList<IndexEntry> &out);
    // 查询严格早于 threshold 的索引条目（升序）；SQL 异常返回 InternalError
    ErrorCode getIndexOlderThan(const QDate &threshold, QList<IndexEntry> &out);
    // 是否存在「覆盖中」批次（SQL 异常视为 false）
    bool hasPendingBatches();
    QList<DailyRecord> getDailyRecords(int year, int month, int day);

    // Management
    bool clearDate(int year, int month, int day);
    bool clearDateRange(const QDate &start, const QDate &end);
    QJsonObject getStorageStats();

    // 相对形态路径（备份 zip 条目名等对外约定使用，格式由 FileManager 一处生成）
    QString buildRelativePath(int year, int month, int day);

    struct FetchResult {
        bool notFound = true;
        QMap<QDate, QList<DailyRecord>> files;
    };
    FetchResult fetchFiles(const QList<QDate> &dates);

signals:
    void dataChanged(int year, int month, int day);

private:
    FileManager *mFileMgr;
    DatabaseManager *mDbMgr;
};
