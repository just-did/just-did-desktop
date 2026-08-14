#pragma once

#include <QObject>
#include <QString>

class QTimer;

// 全局瞬态提示：show() 设置消息，1 秒后自动清空（仅提示，无任何用户操作）
class ToastViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    explicit ToastViewModel(QObject *parent = nullptr);

    QString message() const;

    Q_INVOKABLE void show(const QString &text);

signals:
    void messageChanged();

private:
    QString mMessage;
    QTimer *mTimer;
};
