#include "CalendarModel.h"

CalendarModel::CalendarModel(QObject *parent) : QAbstractListModel(parent) {}

int CalendarModel::rowCount(const QModelIndex &) const { return mDays.size(); }

QVariant CalendarModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mDays.size()) return {};
    const auto &d = mDays[index.row()];
    switch (role) {
    case DayNumber: return d.dayNumber;
    case HasReport: return d.hasReport;
    case IsCurrentMonth: return d.isCurrentMonth;
    default: return {};
    }
}

QHash<int, QByteArray> CalendarModel::roleNames() const
{
    return {{DayNumber, "dayNumber"}, {HasReport, "hasReport"}, {IsCurrentMonth, "isCurrentMonth"}};
}

void CalendarModel::setMonthData(int year, int month, const QList<int> &markedDays)
{
    beginResetModel();

    mDays.clear();

    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();
    int startDayOfWeek = firstDay.dayOfWeek() % 7; // Qt: Mon=1, convert to Sun=0

    // Previous month padding
    QDate prevMonth = firstDay.addMonths(-1);
    int daysInPrevMonth = prevMonth.daysInMonth();
    for (int i = startDayOfWeek - 1; i >= 0; --i) {
        DayInfo d;
        d.dayNumber = daysInPrevMonth - i;
        d.isCurrentMonth = false;
        mDays.append(d);
    }

    // Current month
    for (int day = 1; day <= daysInMonth; ++day) {
        DayInfo d;
        d.dayNumber = day;
        d.hasReport = markedDays.contains(day);
        d.isCurrentMonth = true;
        mDays.append(d);
    }

    // Next month padding (fill to 42 cells = 6 rows)
    int remaining = 42 - mDays.size();
    for (int i = 1; i <= remaining && i <= 14; ++i) {
        DayInfo d;
        d.dayNumber = i;
        d.isCurrentMonth = false;
        mDays.append(d);
    }

    endResetModel();
}
