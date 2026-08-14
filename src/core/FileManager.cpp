#include "FileManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDirIterator>
#include <algorithm>

// --- Path utilities ---

QString FileManager::buildDir(int year, int month) const
{
    return QString("data/%1/%2").arg(year).arg(month, 2, 10, QChar('0'));
}

QString FileManager::buildPath(int year, int month, int day) const
{
    return QString("data/%1/%2/%3.txt")
        .arg(year)
        .arg(month, 2, 10, QChar('0'))
        .arg(day, 2, 10, QChar('0'));
}

QString FileManager::buildTmpPath(int year, int month, int day) const
{
    return buildPath(year, month, day) + ".tmp";
}

// --- Parsing ---

QList<DailyRecord> FileManager::parseContent(const QString &text)
{
    QList<DailyRecord> records;

    QStringList blocks = text.split("\n\n", Qt::SkipEmptyParts);
    for (const auto &block : blocks) {
        int newlinePos = block.indexOf('\n');
        if (newlinePos <= 0) continue;

        DailyRecord r;
        r.time = block.left(newlinePos).trimmed();
        r.content = block.mid(newlinePos + 1).trimmed();

        if (!r.time.isEmpty() && !r.content.isEmpty()) {
            records.append(r);
        }
    }
    return records;
}

// --- Snapshots ---

QString FileManager::buildSnapshotPath(const QString &batchId, int year, int month, int day)
{
    // {batchId}-{DD}.txt（批ID在前，与规格一致）
    return QString("data/%1/%2/%4-%3.txt")
        .arg(year)
        .arg(month, 2, 10, QChar('0'))
        .arg(day, 2, 10, QChar('0'))
        .arg(batchId);
}

void FileManager::removeSnapshot(const QString &batchId, int year, int month, int day)
{
    QFile::remove(buildSnapshotPath(batchId, year, month, day));
}

QString FileManager::serializeContent(const QList<DailyRecord> &records) const
{
    if (records.isEmpty()) return {};

    // Sort by time
    auto sorted = records;
    std::sort(sorted.begin(), sorted.end(), [](const DailyRecord &a, const DailyRecord &b) {
        return a.time < b.time;
    });

    QStringList parts;
    for (const auto &r : sorted) {
        parts.append(r.time + "\n" + r.content);
    }
    return parts.join("\n\n") + "\n";
}

// --- Read / Write / Delete ---

QList<DailyRecord> FileManager::readDailyFile(int year, int month, int day)
{
    QString path = buildPath(year, month, day);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return parseContent(content);
}

bool FileManager::writeDailyFile(int year, int month, int day, const QList<DailyRecord> &records)
{
    QString dir = buildDir(year, month);
    QDir().mkpath(dir);

    QString tmpPath = buildTmpPath(year, month, day);
    QString targetPath = buildPath(year, month, day);

    // Write to temp file
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QString content = serializeContent(records);
    tmpFile.write(content.toUtf8());
    tmpFile.flush();
    tmpFile.close();

    // Atomic rename
    if (QFile::exists(targetPath)) {
        QFile::remove(targetPath);
    }
    if (!QFile::rename(tmpPath, targetPath)) {
        QFile::remove(tmpPath);
        return false;
    }

    return true;
}

bool FileManager::deleteDailyFile(int year, int month, int day)
{
    QString path = buildPath(year, month, day);
    if (!QFile::exists(path)) return true;

    if (!QFile::remove(path)) return false;

    removeEmptyDirs(buildDir(year, month));
    return true;
}

void FileManager::removeEmptyDirs(const QString &path)
{
    QDir dir(path);
    if (dir.exists() && dir.isEmpty()) {
        dir.rmdir(".");
        // Also try to remove parent year directory if empty
        QDir parent = QFileInfo(path).absoluteDir();
        if (parent.exists() && parent.isEmpty()) {
            // Only remove if it's a year directory (4 digits)
            bool isYear = parent.dirName().length() == 4;
            if (isYear) {
                parent.rmdir(".");
            }
        }
    }
}

// --- Stats ---

QJsonObject FileManager::getStats()
{
    qint64 totalSize = 0;
    QMap<int, qint64> byYear;

    QDirIterator it("data", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        if (fi.suffix() == "tmp") continue;

        qint64 size = fi.size();
        totalSize += size;

        // Extract year from path like "data/2026/07/23.txt"
        QString dirName = QDir(fi.absolutePath()).dirName();
        int month = dirName.toInt();
        QDir parentDir = QFileInfo(fi.absolutePath()).absoluteDir();
        int year = parentDir.dirName().toInt();

        if (year > 0) {
            byYear[year] += size;
        }
    }

    QJsonObject result;
    result["totalSize"] = totalSize;

    QJsonObject yearsObj;
    for (auto it = byYear.begin(); it != byYear.end(); ++it) {
        yearsObj[QString::number(it.key())] = it.value();
    }
    result["byYear"] = yearsObj;

    return result;
}
