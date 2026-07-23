#include "ConnectionViewModel.h"
#include "service/HttpServer.h"
#include "core/ConnectionStateMachine.h"
#include "common/Constants.h"

ConnectionViewModel::ConnectionViewModel(HttpServer *server, ConnectionStateMachine *stateMachine,
                                         QObject *parent)
    : QObject(parent), mServer(server), mStateMachine(stateMachine), mPort(Constants::DEFAULT_PORT)
{
    connect(mStateMachine, &ConnectionStateMachine::stateChanged,
            this, &ConnectionViewModel::onStateChanged);
    connect(mServer, &HttpServer::started, this, [this]() { emit qrCodeUrlChanged(); });
    connect(mServer, &HttpServer::stopped, this, [this]() { emit qrCodeUrlChanged(); });
}

int ConnectionViewModel::connectionState() const
{
    return static_cast<int>(mStateMachine->state());
}

QString ConnectionViewModel::qrCodeUrl() const
{
    return mServer->qrCodeUrl();
}

QString ConnectionViewModel::localIP() const
{
    // Extract IP from QR code URL
    QString url = mServer->qrCodeUrl();
    // url is "http://IP:PORT", extract IP
    int start = url.indexOf("://") + 3;
    int end = url.lastIndexOf(':');
    if (start > 2 && end > start)
        return url.mid(start, end - start);
    return {};
}

int ConnectionViewModel::port() const { return mPort; }
QString ConnectionViewModel::connectedDeviceName() const { return {}; }

void ConnectionViewModel::startServer()
{
    mServer->start(mPort);
}

void ConnectionViewModel::stopServer()
{
    mServer->stop();
}

void ConnectionViewModel::setPort(int port)
{
    mPort = port;
    emit portChanged();
}

void ConnectionViewModel::onStateChanged()
{
    emit connectionStateChanged();
    emit connectedDeviceNameChanged();
}
