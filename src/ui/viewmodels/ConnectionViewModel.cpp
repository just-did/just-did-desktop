#include "ConnectionViewModel.h"
#include "service/HttpServer.h"
#include "ui/QRCodeProvider.h"
#include "core/LogManager.h"

ConnectionViewModel::ConnectionViewModel(HttpServer *server, QRCodeProvider *qrProvider, QObject *parent)
    : QObject(parent), mServer(server), mQRProvider(qrProvider), mPort(18080)
{
    auto refreshQR = [this]() {
        int state = mServer->connectionState();
        QString url = mServer->qrCodeUrl();
        LogManager::instance()->info(QString("[ConnectionVM] refreshQR: state=%1, url=%2, qrProvider=%3")
                                         .arg(state).arg(url)
                                         .arg(mQRProvider ? "有" : "NULL"));
        if (mQRProvider && state > 0) {
            mQRProvider->setUrl(url);
            LogManager::instance()->info(QString("[ConnectionVM] QR URL 已设置: %1").arg(url));
        } else {
            LogManager::instance()->warn(QString("[ConnectionVM] 跳过QR设置: provider=%1 state=%2")
                                            .arg(mQRProvider ? "有" : "NULL").arg(state));
        }
        emit qrCodeUrlChanged();
    };

    connect(mServer, &HttpServer::started, this, [this, refreshQR]() {
        refreshState();
        refreshQR();
    });
    connect(mServer, &HttpServer::stopped, this, [this, refreshQR]() {
        refreshState();
        refreshQR();
    });
    connect(mServer, &HttpServer::connectionStateChanged, this, [this]() {
        refreshState();
    });
}

int ConnectionViewModel::connectionState() const { return mState; }

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
QString ConnectionViewModel::connectedDeviceName() const { return {}; }

void ConnectionViewModel::startServer() { mServer->start(mPort); }
void ConnectionViewModel::stopServer() { mServer->stop(); }
void ConnectionViewModel::setPort(int port) { mPort = port; emit portChanged(); }

void ConnectionViewModel::refreshState()
{
    int newState = mServer->connectionState();
    if (newState != mState) {
        mState = newState;
        emit connectionStateChanged();
    }
}
