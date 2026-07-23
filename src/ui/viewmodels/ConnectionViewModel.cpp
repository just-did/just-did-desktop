#include "ConnectionViewModel.h"
#include "service/HttpServer.h"

ConnectionViewModel::ConnectionViewModel(HttpServer *server, QObject *parent)
    : QObject(parent), mServer(server), mPort(18080)
{
    connect(mServer, &HttpServer::started, this, [this]() {
        refreshState();
        emit qrCodeUrlChanged();
    });
    connect(mServer, &HttpServer::stopped, this, [this]() {
        refreshState();
        emit qrCodeUrlChanged();
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
