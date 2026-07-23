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

    // Management
    ErrorCode clearDate(int year, int month, int day);
    ErrorCode clearDateRange(const QDate &start, const QDate &end);
    QJsonObject getStorageStats();

    // Backup / Restore
    bool backupToZip(const QList<QDate> &dates, const QString &outputPath);
    bool restoreFromZip(const QString &zipPath);

private:
    DataManager *mDataMgr;
};
