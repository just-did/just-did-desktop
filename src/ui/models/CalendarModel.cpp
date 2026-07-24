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
    case IsToday: return d.isToday;
    case IsSelected: return d.isSelected;
    case IsFuture: return d.isFuture;
    default: return {};
    }
}

QHash<int, QByteArray> CalendarModel::roleNames() const
{
    return {{DayNumber, "dayNumber"}, {HasReport, "hasReport"}, {IsCurrentMonth, "isCurrentMonth"},
            {IsToday, "isToday"}, {IsSelected, "isSelected"}, {IsFuture, "isFuture"}};
}

void CalendarModel::setMonthData(int year, int month, const QList<int> &markedDays,
                                  int todayYear, int todayMonth, int todayDay,
                                  int selectedYear, int selectedMonth, int selectedDay)
{
    beginResetModel();

    mDays.clear();

    QDate firstDay(year, month, 1);
    int daysInMonth = firstDay.daysInMonth();
    int startDayOfWeek = firstDay.dayOfWeek() % 7; // Qt: Mon=1, convert to Sun=0
    bool isTodayMonth = (year == todayYear && month == todayMonth);
    bool isSelectedMonth = (year == selectedYear && month == selectedMonth);

    // Determine if this entire month is in the future
    QDate today(todayYear, todayMonth, todayDay);
    bool monthIsFuture = firstDay > today;
    bool monthIsPast = firstDay.addMonths(1) <= today;

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
        d.isToday = isTodayMonth && day == todayDay;
        d.isSelected = isSelectedMonth && day == selectedDay;
        d.isFuture = QDate(year, month, day) > today;
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

void CalendarModel::setSelectedDate(int year, int month, int day)
{
    for (int i = 0; i < mDays.size(); ++i) {
        bool shouldBeSelected = mDays[i].isCurrentMonth && mDays[i].dayNumber == day;
        if (mDays[i].isSelected != shouldBeSelected) {
            mDays[i].isSelected = shouldBeSelected;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {IsSelected});
        }
    }
}
