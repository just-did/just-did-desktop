#include "ConnectionViewModel.h"
#include "service/HttpServer.h"
#include "service/SyncService.h"
#include "ui/QRCodeProvider.h"
#include "core/LogManager.h"

ConnectionViewModel::ConnectionViewModel(HttpServer *server, SyncService *syncService,
                                         QRCodeProvider *qrProvider, QObject *parent)
    : QObject(parent), mServer(server), mSyncService(syncService), mQRProvider(qrProvider)
{
    // 启动状态：由 HTTP 服务的 started/stopped 信号维护
    connect(mServer, &HttpServer::started, this, [this]() {
        mStarted = true;
        emit startedChanged();
        refreshQR();
    });
    connect(mServer, &HttpServer::stopped, this, [this]() {
        mStarted = false;
        emit startedChanged();
        refreshQR();
    });

    // 同步状态：转发 SyncService 的忙标志，并保证「同步中」至少可见 1 秒
    mSyncHoldTimer.setSingleShot(true);
    mSyncHoldTimer.setInterval(3000);
    connect(&mSyncHoldTimer, &QTimer::timeout, this, [this]() {
        if (!mSyncingDisplay) return;
        mSyncingDisplay = false;
        emit syncingChanged();
    });
    connect(mSyncService, &SyncService::syncingChanged, this, [this]() {
        if (mSyncService->isSyncing()) {
            mSyncHoldTimer.stop();
            if (!mSyncingDisplay) {
                mSyncingDisplay = true;
                emit syncingChanged();
            }
        } else {
            // 处理结束：保持「同步中」至少 3 秒，短促同步也可感知
            mSyncHoldTimer.start();
        }
    });
}

bool ConnectionViewModel::started() const { return mStarted; }
bool ConnectionViewModel::syncing() const { return mSyncingDisplay; }

QString ConnectionViewModel::qrCodeUrl() const
{
    return mServer->qrCodeUrl();
}

int ConnectionViewModel::qrVersion() const
{
    return mQRProvider ? mQRProvider->version() : 0;
}

QString ConnectionViewModel::localIP() const
{
    QString url = mServer->qrCodeUrl();
    int start = url.indexOf("://") + 3;
    int end = url.lastIndexOf(':');
    if (start > 2 && end > start)
        return url.mid(start, end - start);
    return {};
}

int ConnectionViewModel::port() const { return mPort; }

void ConnectionViewModel::startServer() { mServer->start(mPort); }
void ConnectionViewModel::stopServer() { mServer->stop(); }
void ConnectionViewModel::setPort(int port) { mPort = port; emit portChanged(); }

void ConnectionViewModel::refreshQR()
{
    QString url = mServer->qrCodeUrl();
    LogManager::instance()->info(QString("[ConnectionVM] refreshQR: started=%1, url=%2, qrProvider=%3")
                                     .arg(mStarted ? 1 : 0).arg(url)
                                     .arg(mQRProvider ? "有" : "NULL"));
    if (mQRProvider && mStarted) {
        mQRProvider->setUrl(url);
        LogManager::instance()->info(QString("[ConnectionVM] QR URL 已设置: %1").arg(url));
    } else {
        LogManager::instance()->warn(QString("[ConnectionVM] 跳过QR设置: provider=%1 started=%2")
                                        .arg(mQRProvider ? "有" : "NULL").arg(mStarted ? 1 : 0));
    }
    emit qrCodeUrlChanged();
}
