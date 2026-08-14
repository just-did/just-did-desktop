#include "ConfigManager.h"
#include "common/Constants.h"

#include <yaml-cpp/yaml.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>

bool ConfigManager::load(const QString &configPath)
{
    QMutexLocker lock(&mMutex);
    mConfigPath = configPath;

    if (!QFile::exists(configPath)) {
        applyDefaults();
        return save();
    }

    try {
        YAML::Node config = YAML::LoadFile(configPath.toStdString());

        if (config["server"]) {
            auto server = config["server"];
            if (server["port"]) mPort = server["port"].as<int>();
        }

        if (config["ui"]) {
            auto ui = config["ui"];
            if (ui["floating_window"]) {
                auto fw = ui["floating_window"];
                if (fw["x"] && fw["y"])
                    mFloatingWindowPos = QPoint(fw["x"].as<int>(), fw["y"].as<int>());
            }
            if (ui["main_window"]) {
                auto mw = ui["main_window"];
                int w = mw["width"] ? mw["width"].as<int>() : 800;
                int h = mw["height"] ? mw["height"].as<int>() : 600;
                mMainWindowSize = QSize(w, h);
            }
        }
    } catch (const YAML::Exception &) {
        applyDefaults();
        return save();
    }

    return true;
}

bool ConfigManager::save()
{
    YAML::Emitter out;
    out << YAML::BeginMap;

    out << YAML::Key << "server" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "port" << YAML::Value << mPort;
    out << YAML::EndMap;

    out << YAML::Key << "ui" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "floating_window" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "x" << YAML::Value << mFloatingWindowPos.x();
    out << YAML::Key << "y" << YAML::Value << mFloatingWindowPos.y();
    out << YAML::EndMap;
    out << YAML::Key << "main_window" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "width" << YAML::Value << mMainWindowSize.width();
    out << YAML::Key << "height" << YAML::Value << mMainWindowSize.height();
    out << YAML::EndMap;
    out << YAML::EndMap;

    out << YAML::EndMap;

    // Ensure directory exists
    QFileInfo fi(mConfigPath);
    QDir().mkpath(fi.absolutePath());

    QFile file(mConfigPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    file.write(out.c_str());
    file.close();
    return true;
}

void ConfigManager::applyDefaults()
{
    mPort = Constants::DEFAULT_PORT;
    mFloatingWindowPos = QPoint(100, 200);
    mMainWindowSize = QSize(800, 600);
}

int ConfigManager::port() const { QMutexLocker lock(&mMutex); return mPort; }
void ConfigManager::setPort(int port) { QMutexLocker lock(&mMutex); mPort = port; save(); }
QPoint ConfigManager::floatingWindowPosition() const { QMutexLocker lock(&mMutex); return mFloatingWindowPos; }
void ConfigManager::setFloatingWindowPosition(const QPoint &pos) { QMutexLocker lock(&mMutex); mFloatingWindowPos = pos; save(); }
QSize ConfigManager::mainWindowSize() const { QMutexLocker lock(&mMutex); return mMainWindowSize; }
void ConfigManager::setMainWindowSize(const QSize &size) { QMutexLocker lock(&mMutex); mMainWindowSize = size; save(); }
