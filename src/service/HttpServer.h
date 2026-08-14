#pragma once

#include <QObject>
#include <QString>

class SyncService;
class QHttpServer;
class QTcpServer;

class HttpServer : public QObject
{
    Q_OBJECT
public:
    explicit HttpServer(SyncService *syncService, QObject *parent = nullptr);
    ~HttpServer();

    bool start(int port);
    void stop();
    QString qrCodeUrl() const;

signals:
    void started(int port);
    void stopped();

private:
    void setupRoutes();
    QString getLocalIP() const;

    SyncService *mSyncService;
    QHttpServer *mServer;
    QTcpServer *mTcpServer;
    int mPort = 0;
};
