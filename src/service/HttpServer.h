#pragma once

#include <QObject>
#include <QString>

class SyncService;
class ConnectionStateMachine;
class QHttpServer;
class QTcpServer;

class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(SyncService *syncService, ConnectionStateMachine *stateMachine,
                        QObject *parent = nullptr);
    ~HttpServer();

    bool start(int port);
    void stop();
    QString qrCodeUrl() const;
    int connectionState() const;

signals:
    void started(int port);
    void stopped();
    void connectionStateChanged();

private:
    void setupRoutes();
    QString getLocalIP() const;

    SyncService *mSyncService;
    ConnectionStateMachine *mStateMachine;
    QHttpServer *mServer;
    QTcpServer *mTcpServer;
    int mPort = 0;
};
