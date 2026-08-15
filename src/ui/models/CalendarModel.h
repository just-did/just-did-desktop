#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QDate>
#include <QSet>

class CalendarModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { DayNumber = Qt::UserRole + 1, HasReport, IsCurrentMonth, IsToday, IsSelected, IsFuture, IsPicked };

    explicit CalendarModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMonthData(int year, int month, const QList<int> &markedDays,
                      int todayYear, int todayMonth, int todayDay,
                      int selectedYear, int selectedMonth, int selectedDay,
                      const QSet<QDate> &pickedDates = {});
    void setSelectedDate(int year, int month, int day);

private:
    struct DayInfo {
        int dayNumber = 0;
        bool hasReport = false;
        bool isCurrentMonth = true;
        bool isToday = false;
        bool isSelected = false;
        bool isFuture = false;
        bool isPicked = false;
    };
    QList<DayInfo> mDays;
};
