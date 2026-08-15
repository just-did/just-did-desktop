#include "DatabaseManager.h"
#include "common/Constants.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QFileInfo>
#include <QDir>

bool DatabaseManager::open(const QString &dbPath)
{
    // Ensure directory exists
    QFileInfo fi(dbPath);
    QDir().mkpath(fi.absolutePath());

    mDb = QSqlDatabase::addDatabase("QSQLITE", "justdid_connection");
    mDb.setDatabaseName(dbPath);
    if (!mDb.open()) {
        return false;
    }

    setPragmas();
    migrate();
    return true;
}

void DatabaseManager::close()
{
    if (mDb.isOpen()) {
        mDb.close();
    }
}

void DatabaseManager::setPragmas()
{
    QSqlQuery query(mDb);
    query.exec("PRAGMA journal_mode=WAL");
}

void DatabaseManager::migrate()
{
    QSqlQuery query(mDb);
    query.exec(
        "CREATE TABLE IF NOT EXISTS pc_daliy_report_index ("
        "  year INTEGER NOT NULL,"
        "  month INTEGER NOT NULL,"
        "  day INTEGER NOT NULL,"
        "  path TEXT NOT NULL,"
        "  file_size INTEGER NOT NULL,"
        "  version INTEGER NOT NULL DEFAULT 1,"
        "  PRIMARY KEY (year, month, day)"
        ")");

    query.exec(
        "CREATE TABLE IF NOT EXISTS pc_batch_records ("
        "  batch_id TEXT PRIMARY KEY,"
        "  dates TEXT NOT NULL,"
        "  status TEXT NOT NULL"
        ")");

    // 旧表直接删除，数据不迁移
    query.exec("DROP TABLE IF EXISTS pc_processed_batches");
}

std::optional<IndexEntry> DatabaseManager::getIndexEntry(int year, int month, int day)
{
    QSqlQuery query(mDb);
    query.prepare("SELECT year, month, day, path, file_size, version "
                  "FROM pc_daliy_report_index WHERE year=? AND month=? AND day=?");
    query.addBindValue(year);
    query.addBindValue(month);
    query.addBindValue(day);

    if (query.exec() && query.next()) {
        IndexEntry e;
        e.year = query.value(0).toInt();
        e.month = query.value(1).toInt();
        e.day = query.value(2).toInt();
        e.path = query.value(3).toString();
        e.fileSize = query.value(4).toLongLong();
        e.version = query.value(5).toInt();
        return e;
    }
    return std::nullopt;
}

void DatabaseManager::upsertIndexEntry(int year, int month, int day, const QString &path, qint64 fileSize)
{
    QSqlQuery query(mDb);
    query.prepare("INSERT OR REPLACE INTO pc_daliy_report_index "
                  "(year, month, day, path, file_size, version) "
                  "VALUES (?, ?, ?, ?, ?, "
                  "  COALESCE((SELECT version FROM pc_daliy_report_index "
                  "            WHERE year=? AND month=? AND day=?), 1)"
                  ")");
    query.addBindValue(year);
    query.addBindValue(month);
    query.addBindValue(day);
    query.addBindValue(path);
    query.addBindValue(fileSize);
    query.addBindValue(year);
    query.addBindValue(month);
    query.addBindValue(day);
    query.exec();
}

QList<IndexEntry> DatabaseManager::getIndexByMonth(int year, int month)
{
    QList<IndexEntry> result;
    QSqlQuery query(mDb);
    query.prepare("SELECT year, month, day, path, file_size, version "
                  "FROM pc_daliy_report_index WHERE year=? AND month=? "
                  "ORDER BY day");
    query.addBindValue(year);
    query.addBindValue(month);

    if (query.exec()) {
        while (query.next()) {
            IndexEntry e;
            e.year = query.value(0).toInt();
            e.month = query.value(1).toInt();
            e.day = query.value(2).toInt();
            e.path = query.value(3).toString();
            e.fileSize = query.value(4).toLongLong();
            e.version = query.value(5).toInt();
            result.append(e);
        }
    }
    return result;
}

bool DatabaseManager::getIndexByDates(const QList<QDate> &dates, QList<IndexEntry> &out)
{
    out.clear();
    if (dates.isEmpty())
        return true;

    QStringList placeholders;
    for (const auto &date : dates)
        placeholders.append("(?,?,?)");

    QSqlQuery query(mDb);
    query.prepare("SELECT year, month, day, path, file_size, version "
                  "FROM pc_daliy_report_index "
                  "WHERE (year, month, day) IN (" + placeholders.join(",") + ") "
                  "ORDER BY year, month, day");
    for (const auto &date : dates) {
        query.addBindValue(date.year());
        query.addBindValue(date.month());
        query.addBindValue(date.day());
    }

    if (!query.exec())
        return false;

    while (query.next()) {
        IndexEntry e;
        e.year = query.value(0).toInt();
        e.month = query.value(1).toInt();
        e.day = query.value(2).toInt();
        e.path = query.value(3).toString();
        e.fileSize = query.value(4).toLongLong();
        e.version = query.value(5).toInt();
        out.append(e);
    }
    return true;
}

bool DatabaseManager::getIndexOlderThan(const QDate &threshold, QList<IndexEntry> &out)
{
    out.clear();
    QSqlQuery query(mDb);
    query.prepare("SELECT year, month, day, path, file_size, version "
                  "FROM pc_daliy_report_index "
                  "WHERE (year < ?) OR (year = ? AND month < ?) OR (year = ? AND month = ? AND day < ?) "
                  "ORDER BY year, month, day");
    query.addBindValue(threshold.year());
    query.addBindValue(threshold.year());
    query.addBindValue(threshold.month());
    query.addBindValue(threshold.year());
    query.addBindValue(threshold.month());
    query.addBindValue(threshold.day());

    if (!query.exec())
        return false;

    while (query.next()) {
        IndexEntry e;
        e.year = query.value(0).toInt();
        e.month = query.value(1).toInt();
        e.day = query.value(2).toInt();
        e.path = query.value(3).toString();
        e.fileSize = query.value(4).toLongLong();
        e.version = query.value(5).toInt();
        out.append(e);
    }
    return true;
}

bool DatabaseManager::updateWithVersion(int year, int month, int day, const QString &path,
                                         qint64 fileSize, int expectedVersion)
{
    QSqlQuery query(mDb);
    query.prepare("UPDATE pc_daliy_report_index "
                  "SET path=?, file_size=?, version=version+1 "
                  "WHERE year=? AND month=? AND day=? AND version=?");
    query.addBindValue(path);
    query.addBindValue(fileSize);
    query.addBindValue(year);
    query.addBindValue(month);
    query.addBindValue(day);
    query.addBindValue(expectedVersion);

    return query.exec() && query.numRowsAffected() > 0;
}

void DatabaseManager::removeIndexEntry(int year, int month, int day)
{
    QSqlQuery query(mDb);
    query.prepare("DELETE FROM pc_daliy_report_index WHERE year=? AND month=? AND day=?");
    query.addBindValue(year);
    query.addBindValue(month);
    query.addBindValue(day);
    query.exec();
}

QString DatabaseManager::getBatchStatus(const QString &batchId)
{
    QSqlQuery query(mDb);
    query.prepare("SELECT status FROM pc_batch_records WHERE batch_id=?");
    query.addBindValue(batchId);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return {};
}

void DatabaseManager::insertBatchRecord(const QString &batchId, const QString &dates,
                                        const QString &status)
{
    QSqlQuery query(mDb);
    query.prepare("INSERT OR IGNORE INTO pc_batch_records (batch_id, dates, status) VALUES (?, ?, ?)");
    query.addBindValue(batchId);
    query.addBindValue(dates);
    query.addBindValue(status);
    query.exec();
}

void DatabaseManager::updateBatchStatus(const QString &batchId, const QString &status)
{
    QSqlQuery query(mDb);
    query.prepare("UPDATE pc_batch_records SET status=? WHERE batch_id=?");
    query.addBindValue(status);
    query.addBindValue(batchId);
    query.exec();
}

QStringList DatabaseManager::getBatchDates(const QString &batchId)
{
    QSqlQuery query(mDb);
    query.prepare("SELECT dates FROM pc_batch_records WHERE batch_id=?");
    query.addBindValue(batchId);

    if (query.exec() && query.next()) {
        return query.value(0).toString().split(",", Qt::SkipEmptyParts);
    }
    return {};
}

QList<QPair<QString, QString>> DatabaseManager::getCoveringBatches()
{
    QList<QPair<QString, QString>> result;
    QSqlQuery query(mDb);
    query.prepare("SELECT batch_id, dates FROM pc_batch_records WHERE status=?");
    query.addBindValue(Constants::BATCH_STATUS_COVERING);

    if (query.exec()) {
        while (query.next()) {
            result.append({query.value(0).toString(), query.value(1).toString()});
        }
    }
    return result;
}

bool DatabaseManager::hasPendingBatches(bool &out)
{
    QSqlQuery query(mDb);
    query.prepare("SELECT COUNT(*) FROM pc_batch_records WHERE status=?");
    query.addBindValue(Constants::BATCH_STATUS_COVERING);

    if (!query.exec() || !query.next())
        return false;
    out = query.value(0).toInt() > 0;
    return true;
}
