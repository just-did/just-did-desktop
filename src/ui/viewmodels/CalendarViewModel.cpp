#include "CalendarViewModel.h"
#include "service/ReportService.h"

CalendarViewModel::CalendarViewModel(ReportService *reportService, QObject *parent)
    : QObject(parent), mReportService(reportService)
{
    QDate today = QDate::currentDate();
    mYear = today.year();
    mMonth = today.month();
    mSelectedYear = today.year();
    mSelectedMonth = today.month();
    mSelectedDay = today.day();
}

int CalendarViewModel::currentYear() const { return mYear; }
int CalendarViewModel::currentMonth() const { return mMonth; }
QVariantList CalendarViewModel::markedDays() const { return mMarkedDays; }
int CalendarViewModel::selectedYear() const { return mSelectedYear; }
int CalendarViewModel::selectedMonth() const { return mSelectedMonth; }
int CalendarViewModel::selectedDay() const { return mSelectedDay; }

bool CalendarViewModel::canGoNext() const
{
    QDate today = QDate::currentDate();
    QDate currentFirst(mYear, mMonth, 1);
    QDate todayFirst(today.year(), today.month(), 1);
    return currentFirst < todayFirst;
}

void CalendarViewModel::loadMonth(int year, int month)
{
    mYear = year;
    mMonth = month;
    refresh();
}

void CalendarViewModel::prevMonth()
{
    if (--mMonth < 1) { mMonth = 12; --mYear; }
    refresh();
}

void CalendarViewModel::nextMonth()
{
    if (++mMonth > 12) { mMonth = 1; ++mYear; }
    refresh();
}

void CalendarViewModel::refresh()
{
    auto index = mReportService->getMonthIndex(mYear, mMonth);
    mMarkedDays.clear();
    for (const auto &e : index) {
        QVariantMap day;
        day["day"] = e.day;
        day["hasReport"] = true;
        mMarkedDays.append(day);
    }
    emit currentMonthChanged();
    emit markedDaysChanged();
}

void CalendarViewModel::setSelectedDate(int year, int month, int day)
{
    if (mSelectedYear == year && mSelectedMonth == month && mSelectedDay == day)
        return;

    mSelectedYear = year;
    mSelectedMonth = month;
    mSelectedDay = day;
    emit selectedDateChanged();
}
