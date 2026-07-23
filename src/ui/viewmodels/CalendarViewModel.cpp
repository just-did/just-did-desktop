#include "CalendarViewModel.h"
#include "service/ReportService.h"

CalendarViewModel::CalendarViewModel(ReportService *reportService, QObject *parent)
    : QObject(parent), mReportService(reportService)
{
    QDate today = QDate::currentDate();
    mYear = today.year();
    mMonth = today.month();
}

int CalendarViewModel::currentYear() const { return mYear; }
int CalendarViewModel::currentMonth() const { return mMonth; }
QVariantList CalendarViewModel::markedDays() const { return mMarkedDays; }

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
