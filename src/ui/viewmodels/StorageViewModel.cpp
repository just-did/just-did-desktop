#include "StorageViewModel.h"
#include "service/ReportService.h"

StorageViewModel::StorageViewModel(ReportService *reportService, QObject *parent)
    : QObject(parent), mReportService(reportService) {}

qint64 StorageViewModel::totalSize() const { return mTotalSize; }
QVariantList StorageViewModel::statsByYear() const { return mStatsByYear; }

QString StorageViewModel::totalSizeText() const
{
    if (mTotalSize < 1024) return QString::number(mTotalSize) + " B";
    if (mTotalSize < 1024 * 1024) return QString::number(mTotalSize / 1024) + " KB";
    return QString::number(mTotalSize / (1024 * 1024)) + " MB";
}

double StorageViewModel::usageRatio() const
{
    // Max threshold: 100 MB
    constexpr qint64 kMaxSize = 100 * 1024 * 1024;
    return qMin(static_cast<double>(mTotalSize) / kMaxSize, 1.0);
}

void StorageViewModel::refreshStats()
{
    auto stats = mReportService->getStorageStats();
    mTotalSize = stats["totalSize"].toInteger();

    mStatsByYear.clear();
    QJsonObject byYear = stats["byYear"].toObject();
    for (auto it = byYear.begin(); it != byYear.end(); ++it) {
        QVariantMap item;
        item["year"] = it.key().toInt();
        item["size"] = it.value().toInteger();
        mStatsByYear.append(item);
    }
    emit statsChanged();
}

void StorageViewModel::clearDateRange(int startYear, int startMonth, int startDay,
                                       int endYear, int endMonth, int endDay)
{
    QDate start(startYear, startMonth, startDay);
    QDate end(endYear, endMonth, endDay);
    mReportService->clearDateRange(start, end);
    refreshStats();
}

void StorageViewModel::backup(QVariantList dates)
{
    QList<QDate> dateList;
    for (const auto &v : dates) {
        QVariantMap m = v.toMap();
        dateList.append(QDate(m["year"].toInt(), m["month"].toInt(), m["day"].toInt()));
    }
    // TODO: Show save dialog and call backupToZip
}

void StorageViewModel::restore(QString zipPath)
{
    mReportService->restoreFromZip(zipPath);
    refreshStats();
}
