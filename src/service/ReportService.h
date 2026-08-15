#pragma once

#include <QString>
#include <QList>
#include <QDate>
#include <QJsonObject>

#include "common/Types.h"
#include "common/ErrorCode.h"

class DataManager;

class ReportService
{
public:
    explicit ReportService(DataManager *dataMgr);

    // Local record
    ErrorCode addRecord(int year, int month, int day, const QString &content);

    // View
    QList<IndexEntry> getMonthIndex(int year, int month);
    QList<DailyRecord> getDailyRecords(int year, int month, int day);
    // 严格早于 threshold 的索引条目（升序）；SQL 异常返回空列表
    QList<IndexEntry> getIndexOlderThan(const QDate &threshold);
    // 是否存在「覆盖中」批次（SQL 异常视为 false）
    bool hasPendingBatches();

    // Management
    ErrorCode clearDate(int year, int month, int day);
    ErrorCode clearDateRange(const QDate &start, const QDate &end);
    // 逐日清理（删文件+删索引+dataChanged），返回清理日期数
    int clearDates(const QList<QDate> &dates);
    QJsonObject getStorageStats();

    // Backup / Restore
    bool backupToZip(const QList<QDate> &dates, const QString &outputPath);
    bool restoreFromZip(const QString &zipPath);

private:
    DataManager *mDataMgr;
};
