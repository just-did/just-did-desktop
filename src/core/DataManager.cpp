#include "DataManager.h"
#include "FileManager.h"
#include "DatabaseManager.h"
#include "common/Constants.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <algorithm>

DataManager::DataManager(FileManager *fileMgr, DatabaseManager *dbMgr, QObject *parent)
    : QObject(parent), mFileMgr(fileMgr), mDbMgr(dbMgr)
{
}

ErrorCode DataManager::addRecord(int year, int month, int day,
                                  const QString &time, const QString &content)
{
    // Read existing records
    QList<DailyRecord> records = mFileMgr->readDailyFile(year, month, day);

    // Get current version for optimistic lock
    int expectedVersion = 1;
    auto existing = mDbMgr->getIndexEntry(year, month, day);
    if (existing.has_value()) {
        expectedVersion = existing->version;
    }

    // Append new record and sort
    DailyRecord newRec{time, FileManager::normalizeRecordContent(content).trimmed()};
    records.append(newRec);
    std::sort(records.begin(), records.end(), [](const DailyRecord &a, const DailyRecord &b) {
        return a.time < b.time;
    });

    // Build paths（路径唯一入口：经 FileManager 构建）
    QString targetPath = mFileMgr->buildAbsolutePath(year, month, day);
    QString tmpPath = mFileMgr->buildTmpPath(year, month, day);

    // Write to tmp file only (don't rename yet)
    QDir().mkpath(QFileInfo(tmpPath).absolutePath());
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return ErrorCode::StorageError;
    }

    QString fileContent = FileManager::serializeRecords(records);
    tmpFile.write(fileContent.toUtf8());
    tmpFile.flush();
    tmpFile.close();

    qint64 fileSize = tmpFile.size();

    // Optimistic lock update
    if (existing.has_value()) {
        if (!mDbMgr->updateWithVersion(year, month, day, mFileMgr->buildRelativePath(year, month, day),
                                       fileSize, expectedVersion)) {
            // Version conflict - discard tmp, original file untouched
            QFile::remove(tmpPath);
            return ErrorCode::VersionConflict;
        }
    } else {
        mDbMgr->upsertIndexEntry(year, month, day, mFileMgr->buildRelativePath(year, month, day), fileSize);
    }

    // Now safe to atomically replace the target file
    QFile::remove(targetPath);
    QFile::rename(tmpPath, targetPath);

    emit dataChanged(year, month, day);
    return ErrorCode::Success;
}

ErrorCode DataManager::mergeRecords(const QMap<QDate, QList<DailyRecord>> &recordsByDate,
                                     const QString &batchId)
{
    // 已完成 → 幂等，不重复处理
    QString status = mDbMgr->getBatchStatus(batchId);
    if (status == Constants::BATCH_STATUS_DONE) {
        return ErrorCode::Success;
    }

    // 确定受影响日期：覆盖中续跑以记录为准，新批次以请求数据为准
    QList<QDate> dates;
    if (status == Constants::BATCH_STATUS_COVERING) {
        const QStringList dateStrs = mDbMgr->getBatchDates(batchId);
        for (const auto &ds : dateStrs) {
            QDate d = QDate::fromString(ds, "yyyyMMdd");
            if (d.isValid()) dates.append(d);
        }
    } else {
        dates = recordsByDate.keys();
    }

    // ---- 暂存：仅状态记录不存在时执行 ----
    if (status.isEmpty()) {
        for (const auto &date : dates) {
            // 清理该批旧快照（脏数据）
            mFileMgr->removeSnapshot(batchId, date.year(), date.month(), date.day());

            // 合并已有数据 + 批数据，按时间排序
            QList<DailyRecord> merged = mFileMgr->readDailyFile(date.year(), date.month(), date.day());
            merged.append(recordsByDate.value(date));
            std::sort(merged.begin(), merged.end(), [](const DailyRecord &a, const DailyRecord &b) {
                return a.time < b.time;
            });

            // 序列化并写快照
            QString content = FileManager::serializeRecords(merged);

            QString snapshotPath = mFileMgr->buildSnapshotPath(batchId, date.year(), date.month(), date.day());
            QDir().mkpath(QFileInfo(snapshotPath).absolutePath());
            QFile snapshotFile(snapshotPath);
            if (!snapshotFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                return ErrorCode::StorageError;
            }
            snapshotFile.write(content.toUtf8());
            snapshotFile.flush();
            snapshotFile.close();
        }

        // 插入批次记录，状态=覆盖中
        QStringList dateStrs;
        for (const auto &date : dates)
            dateStrs.append(date.toString("yyyyMMdd"));
        mDbMgr->insertBatchRecord(batchId, dateStrs.join(","), Constants::BATCH_STATUS_COVERING);
    }

    // ---- 覆盖：rename 快照 → 正式文件 ----
    return coverBatch(batchId, dates);
}

ErrorCode DataManager::coverBatch(const QString &batchId, const QList<QDate> &dates)
{
    for (const auto &date : dates) {
        QString snapshotPath = mFileMgr->buildSnapshotPath(batchId, date.year(), date.month(), date.day());
        if (!QFile::exists(snapshotPath)) {
            // 快照缺失 = 该日已 rename 完成（断点续跑）
            continue;
        }

        qint64 fileSize = QFileInfo(snapshotPath).size();
        QString targetPath = mFileMgr->buildAbsolutePath(date.year(), date.month(), date.day());

        QFile::remove(targetPath);
        if (!QFile::rename(snapshotPath, targetPath)) {
            return ErrorCode::StorageError;
        }

        // rename 后更新索引，保证索引反映真实文件
        mDbMgr->upsertIndexEntry(date.year(), date.month(), date.day(),
                                 mFileMgr->buildRelativePath(date.year(), date.month(), date.day()), fileSize);
        emit dataChanged(date.year(), date.month(), date.day());
    }

    mDbMgr->updateBatchStatus(batchId, Constants::BATCH_STATUS_DONE);

    return ErrorCode::Success;
}

QList<IndexEntry> DataManager::getUpdatedIndexForBatch(const QString &batchId)
{
    QStringList dates = mDbMgr->getBatchDates(batchId);
    QList<IndexEntry> result;
    for (const auto &ds : dates) {
        QDate d = QDate::fromString(ds, "yyyyMMdd");
        if (!d.isValid()) continue;
        auto entry = mDbMgr->getIndexEntry(d.year(), d.month(), d.day());
        if (entry.has_value()) {
            result.append(entry.value());
        }
    }
    return result;
}

QList<IndexEntry> DataManager::getMonthIndex(int year, int month)
{
    return mDbMgr->getIndexByMonth(year, month);
}

ErrorCode DataManager::getIndexForDates(const QList<QDate> &dates, QList<IndexEntry> &out)
{
    return mDbMgr->getIndexByDates(dates, out) ? ErrorCode::Success : ErrorCode::InternalError;
}

ErrorCode DataManager::getIndexOlderThan(const QDate &threshold, QList<IndexEntry> &out)
{
    return mDbMgr->getIndexOlderThan(threshold, out) ? ErrorCode::Success : ErrorCode::InternalError;
}

bool DataManager::hasPendingBatches()
{
    bool has = false;
    return mDbMgr->hasPendingBatches(has) && has;
}

QList<DailyRecord> DataManager::getDailyRecords(int year, int month, int day)
{
    return mFileMgr->readDailyFile(year, month, day);
}

bool DataManager::clearDate(int year, int month, int day)
{
    mFileMgr->deleteDailyFile(year, month, day);
    mDbMgr->removeIndexEntry(year, month, day);
    emit dataChanged(year, month, day);
    return true;
}

bool DataManager::clearDateRange(const QDate &start, const QDate &end)
{
    QDate d = start;
    while (d <= end) {
        clearDate(d.year(), d.month(), d.day());
        d = d.addDays(1);
    }
    return true;
}

QJsonObject DataManager::getStorageStats()
{
    return mFileMgr->getStats();
}

QString DataManager::buildRelativePath(int year, int month, int day)
{
    return mFileMgr->buildRelativePath(year, month, day);
}

DataManager::FetchResult DataManager::fetchFiles(const QList<QDate> &dates)
{
    FetchResult result;
    for (const auto &date : dates) {
        // 按文件存在性收录：文件存在即收入（空文件也收），不存在跳过
        if (!mFileMgr->existsDailyFile(date.year(), date.month(), date.day()))
            continue;

        result.files[date] = mFileMgr->readDailyFile(date.year(), date.month(), date.day());
        result.notFound = false;
    }
    return result;
}
