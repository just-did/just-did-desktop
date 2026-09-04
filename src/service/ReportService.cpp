#include "ReportService.h"
#include "core/DataManager.h"
#include "core/FileManager.h"
#include "core/LogManager.h"
#include "core/FileManager.h"

#include "SimpleZip.h"

#include <QTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>

ReportService::ReportService(DataManager *dataMgr)
    : mDataMgr(dataMgr)
{
}

ErrorCode ReportService::addRecord(int year, int month, int day, const QString &content)
{
    QString time = QTime::currentTime().toString("HH:mm");
    return mDataMgr->addRecord(year, month, day, time, content);
}

QList<IndexEntry> ReportService::getMonthIndex(int year, int month)
{
    return mDataMgr->getMonthIndex(year, month);
}

QList<DailyRecord> ReportService::getDailyRecords(int year, int month, int day)
{
    return mDataMgr->getDailyRecords(year, month, day);
}

QList<IndexEntry> ReportService::getIndexOlderThan(const QDate &threshold)
{
    QList<IndexEntry> result;
    if (mDataMgr->getIndexOlderThan(threshold, result) == ErrorCode::InternalError) {
        LogManager::instance()->error(
            QString("[ReportService] getIndexOlderThan SQL 异常, threshold=%1").arg(threshold.toString(Qt::ISODate)));
    }
    return result;
}

bool ReportService::hasPendingBatches()
{
    return mDataMgr->hasPendingBatches();
}

ErrorCode ReportService::clearDate(int year, int month, int day)
{
    if (mDataMgr->clearDate(year, month, day))
        return ErrorCode::Success;
    return ErrorCode::StorageError;
}

ErrorCode ReportService::clearDateRange(const QDate &start, const QDate &end)
{
    if (mDataMgr->clearDateRange(start, end))
        return ErrorCode::Success;
    return ErrorCode::StorageError;
}

int ReportService::clearDates(const QList<QDate> &dates)
{
    for (const auto &date : dates) {
        mDataMgr->clearDate(date.year(), date.month(), date.day());
    }
    return dates.size();
}

QJsonObject ReportService::getStorageStats()
{
    return mDataMgr->getStorageStats();
}

bool ReportService::backupToZip(const QList<QDate> &dates, const QString &outputPath)
{
    SimpleZipWriter writer;
    if (!writer.open(outputPath))
        return false;

    for (const auto &date : dates) {
        auto records = mDataMgr->getDailyRecords(date.year(), date.month(), date.day());
        if (records.isEmpty()) continue;

        QString entryPath = mDataMgr->buildRelativePath(date.year(), date.month(), date.day());

        QString content = FileManager::serializeRecords(records);

        writer.addFile(entryPath, content.toUtf8());
    }
    writer.close();
    return true;
}

bool ReportService::restoreFromZip(const QString &zipPath)
{
    Q_UNUSED(zipPath)
    // TODO: Implement ZIP restore with SimpleZipReader
    return false;
}
