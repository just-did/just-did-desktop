#pragma once

#include <QObject>
#include <QTimer>

#include "common/Types.h"

class ConnectionStateMachine : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionStateMachine(QObject *parent = nullptr);

    void start();
    void stop();
    void onConnect();
    void onSyncStart();
    void onSyncComplete();
    void onHeartbeatReceived();

    ConnectionState state() const;

signals:
    void stateChanged(ConnectionState oldState, ConnectionState newState);

private:
    void setState(ConnectionState newState);
    void resetHeartbeatTimer();

    QTimer mHeartbeatTimer;
    ConnectionState mState = ConnectionState::Unstarted;
};
