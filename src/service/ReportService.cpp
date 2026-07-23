#include "ReportService.h"
#include "core/DataManager.h"
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

        QString entryPath = QString("data/%1/%2/%3.txt")
                                .arg(date.year())
                                .arg(date.month(), 2, 10, QChar('0'))
                                .arg(date.day(), 2, 10, QChar('0'));

        auto sorted = records;
        std::sort(sorted.begin(), sorted.end(), [](const DailyRecord &a, const DailyRecord &b) {
            return a.time < b.time;
        });
        QStringList parts;
        for (const auto &r : sorted)
            parts.append(r.time + "\n" + r.content);
        QString content = parts.join("\n\n") + "\n";

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
