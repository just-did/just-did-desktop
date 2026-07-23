#include "ConnectionStateMachine.h"
#include "common/Constants.h"

ConnectionStateMachine::ConnectionStateMachine(QObject *parent)
    : QObject(parent)
{
    mHeartbeatTimer.setSingleShot(true);
    connect(&mHeartbeatTimer, &QTimer::timeout, this, [this]() {
        if (mState == ConnectionState::Connected) {
            setState(ConnectionState::Disconnected);
        }
    });
}

void ConnectionStateMachine::start()
{
    setState(ConnectionState::Disconnected);
}

void ConnectionStateMachine::stop()
{
    mHeartbeatTimer.stop();
    setState(ConnectionState::Unstarted);
}

void ConnectionStateMachine::onConnect()
{
    if (mState == ConnectionState::Disconnected || mState == ConnectionState::Connected) {
        setState(ConnectionState::Connected);
        resetHeartbeatTimer();
    }
}

void ConnectionStateMachine::onSyncStart()
{
    if (mState == ConnectionState::Connected) {
        setState(ConnectionState::Syncing);
        // Keep heartbeat timer running during sync too
    }
}

void ConnectionStateMachine::onSyncComplete()
{
    if (mState == ConnectionState::Syncing) {
        setState(ConnectionState::Connected);
        resetHeartbeatTimer();
    }
}

void ConnectionStateMachine::onHeartbeatReceived()
{
    if (mState == ConnectionState::Connected || mState == ConnectionState::Syncing) {
        resetHeartbeatTimer();
    }
}

ConnectionState ConnectionStateMachine::state() const
{
    return mState;
}

void ConnectionStateMachine::setState(ConnectionState newState)
{
    if (mState != newState) {
        auto old = mState;
        mState = newState;
        emit stateChanged(old, newState);
    }
}

void ConnectionStateMachine::resetHeartbeatTimer()
{
    mHeartbeatTimer.start(Constants::HEARTBEAT_TIMEOUT_SEC * 1000);
}
