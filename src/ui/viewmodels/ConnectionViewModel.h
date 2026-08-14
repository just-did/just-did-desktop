#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class HttpServer;
class SyncService;
class QRCodeProvider;

class ConnectionViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool started READ started NOTIFY startedChanged)
    Q_PROPERTY(bool syncing READ syncing NOTIFY syncingChanged)
    Q_PROPERTY(QString qrCodeUrl READ qrCodeUrl NOTIFY qrCodeUrlChanged)
    Q_PROPERTY(int qrVersion READ qrVersion NOTIFY qrCodeUrlChanged)
    Q_PROPERTY(QString localIP READ localIP NOTIFY qrCodeUrlChanged)
    Q_PROPERTY(int port READ port NOTIFY portChanged)

public:
    explicit ConnectionViewModel(HttpServer *server, SyncService *syncService,
                                 QRCodeProvider *qrProvider = nullptr,
                                 QObject *parent = nullptr);

    bool started() const;
    bool syncing() const;
    QString qrCodeUrl() const;
    int qrVersion() const;
    QString localIP() const;
    int port() const;

    Q_INVOKABLE void startServer();
    Q_INVOKABLE void stopServer();
    Q_INVOKABLE void setPort(int port);

signals:
    void startedChanged();
    void syncingChanged();
    void qrCodeUrlChanged();
    void localIPChanged();
    void portChanged();

private:
    void refreshQR();

    HttpServer *mServer;
    SyncService *mSyncService;
    QRCodeProvider *mQRProvider = nullptr;
    bool mStarted = false;
    bool mSyncingDisplay = false;               // 展示用同步状态（含最小可见时长）
    QTimer mSyncHoldTimer;                      // 「同步中」最短展示时长
    int mPort = 18080;
};
