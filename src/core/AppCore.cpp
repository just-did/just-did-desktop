#include "AppCore.h"
#include "ConfigManager.h"
#include "LogManager.h"
#include "DatabaseManager.h"
#include "FileManager.h"
#include "DataManager.h"

#include <QCoreApplication>
#include <QDir>

AppCore::AppCore(QObject *parent) : QObject(parent) {}

AppCore *AppCore::instance()
{
    static AppCore inst;
    return &inst;
}

bool AppCore::init()
{
    // 1. LogManager
    if (!LogManager::instance()->init("logs")) {
        emit startupFailed("日志系统初始化失败");
        return false;
    }
    LogManager::instance()->info("日志系统初始化完成");

    // 2. ConfigManager
    mConfigMgr.reset(new ConfigManager());
    if (!mConfigMgr->load("config.yml")) {
        LogManager::instance()->warn("配置文件加载失败，使用默认值");
    }
    LogManager::instance()->info("配置管理器初始化完成");

    // 3. 解析数据根：配置原始串缺失/空白 → 默认 just-did-data；相对路径按应用目录解析；绝对路径原样
    QString dataRoot = mConfigMgr->dataRoot().trimmed();
    if (dataRoot.isEmpty())
        dataRoot = "just-did-data";
    dataRoot = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(dataRoot);
    LogManager::instance()->info(QString("数据根目录：%1").arg(dataRoot));

    // 4. DatabaseManager
    mDbMgr.reset(new DatabaseManager());
    if (!mDbMgr->open(dataRoot + "/just_do.db")) {
        LogManager::instance()->error("数据库初始化失败");
        emit startupFailed("数据库初始化失败");
        return false;
    }
    LogManager::instance()->info("数据库初始化完成");

    // 5. FileManager
    mFileMgr.reset(new FileManager(dataRoot));
    LogManager::instance()->info("文件管理器初始化完成");

    // 6. DataManager
    mDataMgr.reset(new DataManager(mFileMgr.data(), mDbMgr.data()));
    LogManager::instance()->info("数据管理器初始化完成");

    // 7. HttpServer initialization is handled by service layer through startServer()

    mInitialized = true;
    LogManager::instance()->info("应用核心初始化完成");
    emit startupCompleted();
    return true;
}

void AppCore::shutdown()
{
    LogManager::instance()->info("应用正在关闭...");
    mInitialized = false;
    if (mDbMgr) {
        mDbMgr->close();
    }
}

ConfigManager *AppCore::configManager() const { return mConfigMgr.data(); }
LogManager *AppCore::logManager() const { return LogManager::instance(); }
DatabaseManager *AppCore::databaseManager() const { return mDbMgr.data(); }
FileManager *AppCore::fileManager() const { return mFileMgr.data(); }
DataManager *AppCore::dataManager() const { return mDataMgr.data(); }
