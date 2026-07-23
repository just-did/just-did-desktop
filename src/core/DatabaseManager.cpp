#include "DatabaseManager.h"

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
        "CREATE TABLE IF NOT EXISTS pc_processed_batches ("
        "  batch_id TEXT PRIMARY KEY,"
        "  dates TEXT NOT NULL"
        ")");
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

bool DatabaseManager::isBatchProcessed(const QString &batchId)
{
    QSqlQuery query(mDb);
    query.prepare("SELECT 1 FROM pc_processed_batches WHERE batch_id=?");
    query.addBindValue(batchId);
    return query.exec() && query.next();
}

void DatabaseManager::insertBatch(const QString &batchId, const QString &dates)
{
    QSqlQuery query(mDb);
    query.prepare("INSERT OR IGNORE INTO pc_processed_batches (batch_id, dates) VALUES (?, ?)");
    query.addBindValue(batchId);
    query.addBindValue(dates);
    query.exec();
}

QStringList DatabaseManager::getBatchDates(const QString &batchId)
{
    QSqlQuery query(mDb);
    query.prepare("SELECT dates FROM pc_processed_batches WHERE batch_id=?");
    query.addBindValue(batchId);

    if (query.exec() && query.next()) {
        return query.value(0).toString().split(",", Qt::SkipEmptyParts);
    }
    return {};
}
