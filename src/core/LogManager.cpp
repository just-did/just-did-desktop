#include "LogManager.h"
#include "common/Constants.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>

LogManager *LogManager::instance()
{
    static LogManager inst;
    return &inst;
}

bool LogManager::init(const QString &logDir)
{
    QMutexLocker lock(&mMutex);
    mLogDir = logDir;

    QDir dir(logDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    mFile.setFileName(dir.filePath("just-did.log"));
    if (mFile.open(QIODevice::Append | QIODevice::Text)) {
        mCurrentSize = mFile.size();
        return true;
    }
    return false;
}

void LogManager::debug(const QString &msg)
{
    write("DEBUG", msg);
}

void LogManager::info(const QString &msg)
{
    write("INFO", msg);
}

void LogManager::warn(const QString &msg)
{
    write("WARN", msg);
}

void LogManager::error(const QString &msg)
{
    write("ERROR", msg);
}

void LogManager::write(const QString &level, const QString &msg)
{
    QMutexLocker lock(&mMutex);

    QString line = QString("[%1] [%2] %3\n")
                       .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                       .arg(level)
                       .arg(msg);

    QByteArray data = line.toUtf8();

    if (mCurrentSize + data.size() > Constants::LOG_FILE_MAX_SIZE) {
        rotate();
    }

    mFile.write(data);
    mFile.flush();
    mCurrentSize += data.size();
}

void LogManager::rotate()
{
    mFile.close();

    QString basePath = mLogDir + "/just-did";

    // Remove oldest file
    QString f2 = basePath + ".2.log";
    if (QFile::exists(f2)) {
        QFile::remove(f2);
    }

    // Shift: .1.log → .2.log
    QString f1 = basePath + ".1.log";
    if (QFile::exists(f1)) {
        QFile::rename(f1, f2);
    }

    // Shift: .log → .1.log
    QFile::rename(basePath + ".log", f1);

    // Reopen new log file
    mFile.setFileName(basePath + ".log");
    if (mFile.open(QIODevice::Append | QIODevice::Text)) {
        mCurrentSize = 0;
    }

    // Check total size limit
    qint64 totalSize = 0;
    QStringList files = {basePath + ".log", f1, f2};
    QList<QPair<QString, qint64>> fileInfos;
    for (const auto &f : files) {
        QFileInfo fi(f);
        if (fi.exists()) {
            fileInfos.append({f, fi.size()});
            totalSize += fi.size();
        }
    }

    if (totalSize > Constants::LOG_TOTAL_MAX_SIZE && fileInfos.size() >= Constants::LOG_MAX_FILES) {
        // Delete oldest (largest index)
        QFile::remove(fileInfos.last().first);
    }
}
