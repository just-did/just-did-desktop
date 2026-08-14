#include "ToastViewModel.h"

#include <QTimer>

ToastViewModel::ToastViewModel(QObject *parent)
    : QObject(parent)
{
    mTimer = new QTimer(this);
    mTimer->setSingleShot(true);
    mTimer->setInterval(1000);  // 提示 1 秒后自动消失
    connect(mTimer, &QTimer::timeout, this, [this]() {
        if (mMessage.isEmpty()) return;
        mMessage.clear();
        emit messageChanged();
    });
}

QString ToastViewModel::message() const
{
    return mMessage;
}

void ToastViewModel::show(const QString &text)
{
    mMessage = text;
    emit messageChanged();
    mTimer->start();  // 每次 show 重启 1 秒倒计时
}
