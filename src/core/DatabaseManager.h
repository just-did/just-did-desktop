#pragma once

#include <QString>
#include <QList>
#include <QSqlDatabase>
#include <optional>

#include "common/Types.h"

class DatabaseManager
{
public:
    bool open(const QString &dbPath);
    void close();

    std::optional<IndexEntry> getIndexEntry(int year, int month, int day);
    void upsertIndexEntry(int year, int month, int day, const QString &path, qint64 fileSize);
    QList<IndexEntry> getIndexByMonth(int year, int month);
    bool updateWithVersion(int year, int month, int day, const QString &path, qint64 fileSize, int expectedVersion);

    bool isBatchProcessed(const QString &batchId);
    void insertBatch(const QString &batchId, const QString &dates);
    QStringList getBatchDates(const QString &batchId);

private:
    void migrate();
    void setPragmas();

    QSqlDatabase mDb;
};
