#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QDate>

class CalendarModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { DayNumber = Qt::UserRole + 1, HasReport, IsCurrentMonth };

    explicit CalendarModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMonthData(int year, int month, const QList<int> &markedDays);

private:
    struct DayInfo {
        int dayNumber = 0;
        bool hasReport = false;
        bool isCurrentMonth = true;
    };
    QList<DayInfo> mDays;
};
