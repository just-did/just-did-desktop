#include "FileManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDirIterator>
#include <QRegularExpression>
#include <QTime>
#include <algorithm>

FileManager::FileManager(const QString &dataRoot)
    : mDataRoot(dataRoot)
{
}

// --- Path utilities ---

QString FileManager::dataDir() const
{
    return mDataRoot + "/data";
}

QString FileManager::buildDir(int year, int month) const
{
    return QString("%1/%2/%3").arg(dataDir()).arg(year).arg(month, 2, 10, QChar('0'));
}

QString FileManager::buildRelativePath(int year, int month, int day) const
{
    return QString("data/%1/%2/%3.txt")
        .arg(year)
        .arg(month, 2, 10, QChar('0'))
        .arg(day, 2, 10, QChar('0'));
}

QString FileManager::buildAbsolutePath(int year, int month, int day) const
{
    return mDataRoot + "/" + buildRelativePath(year, month, day);
}

QString FileManager::buildTmpPath(int year, int month, int day) const
{
    return buildAbsolutePath(year, month, day) + ".tmp";
}

QString FileManager::resolveIndexPath(const QString &indexPath) const
{
    return mDataRoot + "/" + indexPath;
}

// --- Parsing ---

QList<DailyRecord> FileManager::parseContent(const QString &text)
{
    QList<DailyRecord> records;
    QString normalizedText = text;
    normalizedText.replace("\r\n", "\n");
    normalizedText.replace('\r', '\n');
    static const QRegularExpression timeHeader("^(\\d{2}):(\\d{2})$");
    const auto isValidTimeHeader = [](const QString &line) {
        const auto match = timeHeader.match(line.trimmed());
        return match.hasMatch()
            && QTime(match.captured(1).toInt(), match.captured(2).toInt()).isValid();
    };

    DailyRecord current;
    QStringList contentLines;
    int pendingEmptyLines = 0;
    const auto flushCurrent = [&]() {
        if (current.time.isEmpty()) return;
        current.content = normalizeRecordContent(contentLines.join('\n').trimmed());
        if (!current.content.isEmpty()) records.append(current);
        current = {};
        contentLines.clear();
        pendingEmptyLines = 0;
    };

    const QStringList lines = normalizedText.split('\n', Qt::KeepEmptyParts);
    for (const QString &line : lines) {
        if ((current.time.isEmpty() || pendingEmptyLines > 0) && isValidTimeHeader(line)) {
            flushCurrent();
            current.time = line.trimmed();
            continue;
        }
        if (current.time.isEmpty()) continue;
        if (line.isEmpty()) {
            ++pendingEmptyLines;
            continue;
        }
        while (pendingEmptyLines > 0) {
            contentLines.append(QString());
            --pendingEmptyLines;
        }
        contentLines.append(line);
    }
    flushCurrent();
    return records;
}

QString FileManager::normalizeRecordContent(const QString &content)
{
    QString normalized = content;
    normalized.replace("\r\n", "\n");
    normalized.replace('\r', '\n');
    normalized.replace(QRegularExpression("\\n{2,}"), "\n");
    return normalized;
}

// --- Snapshots ---

QString FileManager::buildSnapshotPath(const QString &batchId, int year, int month, int day) const
{
    // {batchId}-{DD}.txt（批ID在前，与规格一致）
    return QString("%1/%2/%3/%5-%4.txt")
        .arg(dataDir())
        .arg(year)
        .arg(month, 2, 10, QChar('0'))
        .arg(day, 2, 10, QChar('0'))
        .arg(batchId);
}

void FileManager::removeSnapshot(const QString &batchId, int year, int month, int day)
{
    QFile::remove(buildSnapshotPath(batchId, year, month, day));
}

QString FileManager::serializeRecords(const QList<DailyRecord> &records)
{
    if (records.isEmpty()) return {};

    // Sort by time
    auto sorted = records;
    std::sort(sorted.begin(), sorted.end(), [](const DailyRecord &a, const DailyRecord &b) {
        return a.time < b.time;
    });

    QStringList parts;
    for (const auto &r : sorted) {
        const QString content = normalizeRecordContent(r.content).trimmed();
        if (!r.time.trimmed().isEmpty() && !content.isEmpty())
            parts.append(r.time.trimmed() + "\n" + content);
    }
    return parts.isEmpty() ? QString() : parts.join("\n\n") + "\n";
}

// --- Read / Write / Delete ---

QList<DailyRecord> FileManager::readDailyFile(int year, int month, int day)
{
    QString path = buildAbsolutePath(year, month, day);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QString content = QString::fromUtf8(file.readAll());
    file.close();
    return parseContent(content);
}

bool FileManager::existsDailyFile(int year, int month, int day)
{
    return QFile::exists(buildAbsolutePath(year, month, day));
}

bool FileManager::writeDailyFile(int year, int month, int day, const QList<DailyRecord> &records)
{
    QString dir = buildDir(year, month);
    QDir().mkpath(dir);

    QString tmpPath = buildTmpPath(year, month, day);
    QString targetPath = buildAbsolutePath(year, month, day);

    // Write to temp file
    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QString content = serializeRecords(records);
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
    QString path = buildAbsolutePath(year, month, day);
    if (!QFile::exists(path)) return true;

    if (!QFile::remove(path)) return false;

    removeEmptyDirs(buildDir(year, month));
    return true;
}

void FileManager::removeEmptyDirs(const QString &path)
{
    // 只清理到 {dataRoot}/data 为止：先删空月目录，再删空年目录，不越过 data 目录层
    QDir dir(path);
    if (dir.exists() && dir.isEmpty())
        dir.rmdir(".");

    QDir parent = QFileInfo(path).absoluteDir();
    if (parent.path() != dataDir() && parent.exists() && parent.isEmpty())
        parent.rmdir(".");
}

// --- Stats ---

QJsonObject FileManager::getStats()
{
    qint64 totalSize = 0;
    QMap<int, qint64> byYear;

    QDirIterator it(dataDir(), QDir::Files, QDirIterator::Subdirectories);
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
