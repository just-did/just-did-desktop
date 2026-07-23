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
    QList<IndexEntry> getUpdatedIndexForBatch(const QString &batchId);

    // Queries
    QList<IndexEntry> getMonthIndex(int year, int month);
    QList<DailyRecord> getDailyRecords(int year, int month, int day);

    // Management
    bool clearDate(int year, int month, int day);
    bool clearDateRange(const QDate &start, const QDate &end);
    QJsonObject getStorageStats();

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
