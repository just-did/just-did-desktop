#pragma once

#include <QString>
#include <QMutex>
#include <QFile>

class LogManager
{
public:
    static LogManager *instance();

    bool init(const QString &logDir);

    void debug(const QString &msg);
    void info(const QString &msg);
    void warn(const QString &msg);
    void error(const QString &msg);

private:
    LogManager() = default;

    void write(const QString &level, const QString &msg);
    void rotate();

    QMutex mMutex;
    QString mLogDir;
    QFile mFile;
    qint64 mCurrentSize = 0;
};
