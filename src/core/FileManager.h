#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>

#include "common/Types.h"

class FileManager
{
public:
    FileManager() = default;

    QList<DailyRecord> readDailyFile(int year, int month, int day);
    bool writeDailyFile(int year, int month, int day, const QList<DailyRecord> &records);
    bool deleteDailyFile(int year, int month, int day);
    int recoverTmpFiles();
    QJsonObject getStats();

private:
    QString buildPath(int year, int month, int day) const;
    QString buildTmpPath(int year, int month, int day) const;
    QString buildDir(int year, int month) const;
    void removeEmptyDirs(const QString &path);
    QList<DailyRecord> parseContent(const QString &text) const;
    QString serializeContent(const QList<DailyRecord> &records) const;
};
