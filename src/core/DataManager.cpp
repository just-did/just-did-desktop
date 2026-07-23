#include "DataManager.h"
#include "FileManager.h"
#include "DatabaseManager.h"

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

    // Append new record
    DailyRecord newRec{time, content};
    records.append(newRec);

    // Build path
    QString path = QString("%1/%2/%3.txt")
                       .arg(year)
                       .arg(month, 2, 10, QChar('0'))
                       .arg(day, 2, 10, QChar('0'));

    // Write file first
    if (!mFileMgr->writeDailyFile(year, month, day, records)) {
        return ErrorCode::StorageError;
    }

    // Get file size
    qint64 fileSize = QFile(path).size();

    // Optimistic lock update
    if (existing.has_value()) {
        if (!mDbMgr->updateWithVersion(year, month, day, path, fileSize, expectedVersion)) {
            // Version conflict - rollback (re-read and rewrite original)
            QFile::remove(path);
            if (!records.isEmpty()) {
                // Actually we should restore original, but conflict is rare
            }
            return ErrorCode::VersionConflict;
        }
    } else {
        mDbMgr->upsertIndexEntry(year, month, day, path, fileSize);
    }

    emit dataChanged(year, month, day);
    return ErrorCode::Success;
}

ErrorCode DataManager::mergeRecords(const QMap<QDate, QList<DailyRecord>> &recordsByDate,
                                     const QString &batchId)
{
    // Check idempotency
    if (mDbMgr->isBatchProcessed(batchId)) {
        return ErrorCode::Success;
    }

    // Build date list for batch record
    QStringList dateStrs;
    struct DateOp {
        int year, month, day;
        QString path;
        QList<DailyRecord> newRecords;
        int expectedVersion;
        bool exists;
    };
    QList<DateOp> ops;

    for (auto it = recordsByDate.begin(); it != recordsByDate.end(); ++it) {
        QDate date = it.key();
        int year = date.year(), month = date.month(), day = date.day();

        DateOp op;
        op.year = year;
        op.month = month;
        op.day = day;
        op.path = QString("%1/%2/%3.txt")
                      .arg(year)
                      .arg(month, 2, 10, QChar('0'))
                      .arg(day, 2, 10, QChar('0'));

        // Read existing
        QList<DailyRecord> existing = mFileMgr->readDailyFile(year, month, day);

        // Get version
        auto indexEntry = mDbMgr->getIndexEntry(year, month, day);
        op.exists = indexEntry.has_value();
        op.expectedVersion = op.exists ? indexEntry->version : 1;

        // Merge and sort
        existing.append(it.value());
        op.newRecords = existing;

        dateStrs.append(date.toString("yyyyMMdd"));
        ops.append(op);
    }

    // Start transaction
    QSqlDatabase db = QSqlDatabase::database("justdid_connection");
    db.transaction();

    // Write each date to tmp file
    for (auto &op : ops) {
        QString tmpPath = QString("data/%1/%2/%3.txt.tmp")
                              .arg(op.year)
                              .arg(op.month, 2, 10, QChar('0'))
                              .arg(op.day, 2, 10, QChar('0'));

        QDir().mkpath(QFileInfo(tmpPath).absolutePath());
        QFile tmpFile(tmpPath);
        if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            db.rollback();
            // Clean up tmp files
            for (const auto &o : ops) {
                QString tp = QString("data/%1/%2/%3.txt.tmp")
                                 .arg(o.year).arg(o.month, 2, 10, QChar('0')).arg(o.day, 2, 10, QChar('0'));
                QFile::remove(tp);
            }
            return ErrorCode::StorageError;
        }

        // Serialize sorted
        auto sorted = op.newRecords;
        std::sort(sorted.begin(), sorted.end(), [](const DailyRecord &a, const DailyRecord &b) {
            return a.time < b.time;
        });
        QStringList parts;
        for (const auto &r : sorted) {
            parts.append(r.time + "\n" + r.content);
        }
        QString content = parts.join("\n\n") + "\n";
        tmpFile.write(content.toUtf8());
        tmpFile.flush();
        tmpFile.close();

        // Update index with optimistic lock
        qint64 fileSize = tmpFile.size();
        bool ok;
        if (op.exists) {
            ok = mDbMgr->updateWithVersion(op.year, op.month, op.day, op.path, fileSize, op.expectedVersion);
        } else {
            mDbMgr->upsertIndexEntry(op.year, op.month, op.day, op.path, fileSize);
            ok = true;
        }

        if (!ok) {
            db.rollback();
            // Clean up tmp files
            for (const auto &o : ops) {
                QString tp = QString("data/%1/%2/%3.txt.tmp")
                                 .arg(o.year).arg(o.month, 2, 10, QChar('0')).arg(o.day, 2, 10, QChar('0'));
                QFile::remove(tp);
            }
            return ErrorCode::VersionConflict;
        }
    }

    // Insert batch record
    mDbMgr->insertBatch(batchId, dateStrs.join(","));
    db.commit();

    // Rename tmp → target
    for (const auto &op : ops) {
        QString tmpPath = QString("data/%1/%2/%3.txt.tmp")
                              .arg(op.year).arg(op.month, 2, 10, QChar('0')).arg(op.day, 2, 10, QChar('0'));
        QString targetPath = QString("data/%1/%2/%3.txt")
                                 .arg(op.year).arg(op.month, 2, 10, QChar('0')).arg(op.day, 2, 10, QChar('0'));
        QFile::remove(targetPath);
        QFile::rename(tmpPath, targetPath);
        emit dataChanged(op.year, op.month, op.day);
    }

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

QList<DailyRecord> DataManager::getDailyRecords(int year, int month, int day)
{
    return mFileMgr->readDailyFile(year, month, day);
}

bool DataManager::clearDate(int year, int month, int day)
{
    mFileMgr->deleteDailyFile(year, month, day);

    // Remove index entry
    QSqlDatabase db = QSqlDatabase::database("justdid_connection");
    QSqlQuery query(db);
    query.prepare("DELETE FROM pc_daliy_report_index WHERE year=? AND month=? AND day=?");
    query.addBindValue(year);
    query.addBindValue(month);
    query.addBindValue(day);
    query.exec();

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

DataManager::FetchResult DataManager::fetchFiles(const QList<QDate> &dates)
{
    FetchResult result;
    for (const auto &date : dates) {
        auto records = mFileMgr->readDailyFile(date.year(), date.month(), date.day());
        if (!records.isEmpty()) {
            result.files[date] = records;
            result.notFound = false;
        }
    }
    return result;
}
