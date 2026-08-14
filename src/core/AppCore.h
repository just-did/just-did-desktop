#pragma once

#include <QObject>
#include <QScopedPointer>

class ConfigManager;
class LogManager;
class DatabaseManager;
class FileManager;
class DataManager;

class AppCore : public QObject
{
    Q_OBJECT
public:
    static AppCore *instance();

    bool init();
    void shutdown();

    ConfigManager *configManager() const;
    LogManager *logManager() const;
    DatabaseManager *databaseManager() const;
    FileManager *fileManager() const;
    DataManager *dataManager() const;

signals:
    void startupCompleted();
    void startupFailed(QString reason);

private:
    explicit AppCore(QObject *parent = nullptr);

    QScopedPointer<ConfigManager> mConfigMgr;
    QScopedPointer<DatabaseManager> mDbMgr;
    QScopedPointer<FileManager> mFileMgr;
    QScopedPointer<DataManager> mDataMgr;
    bool mInitialized = false;
};
