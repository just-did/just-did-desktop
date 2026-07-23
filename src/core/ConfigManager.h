#pragma once

#include <QString>
#include <QPoint>
#include <QSize>
#include <QMutex>

namespace YAML { class Node; }

class ConfigManager
{
public:
    bool load(const QString &configPath);
    bool save();

    int port() const;
    void setPort(int port);
    int heartbeatTimeout() const;
    void setHeartbeatTimeout(int sec);

    QPoint floatingWindowPosition() const;
    void setFloatingWindowPosition(const QPoint &pos);
    QSize mainWindowSize() const;
    void setMainWindowSize(const QSize &size);

private:
    void applyDefaults();

    mutable QMutex mMutex;
    QString mConfigPath;

    int mPort = 18080;
    int mHeartbeatTimeout = 45;
    QPoint mFloatingWindowPos{100, 200};
    QSize mMainWindowSize{800, 600};
};
