#pragma once

#include <QObject>
#include <QString>

class HttpServer;

class ConnectionViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString qrCodeUrl READ qrCodeUrl NOTIFY qrCodeUrlChanged)
    Q_PROPERTY(QString localIP READ localIP NOTIFY qrCodeUrlChanged)
    Q_PROPERTY(int port READ port NOTIFY portChanged)
    Q_PROPERTY(QString connectedDeviceName READ connectedDeviceName NOTIFY connectedDeviceNameChanged)

public:
    explicit ConnectionViewModel(HttpServer *server, QObject *parent = nullptr);

    int connectionState() const;
    QString qrCodeUrl() const;
    QString localIP() const;
    int port() const;
    QString connectedDeviceName() const;

    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void setPort(int port);

signals:
    void connectionStateChanged();
    void qrCodeUrlChanged();
    void localIPChanged();
    void portChanged();
    void connectedDeviceNameChanged();

private:
    void refreshState();

    HttpServer *mServer;
    int mState = 0;
    int mPort = 18080;
};
