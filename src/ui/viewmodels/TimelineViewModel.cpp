#include "TimelineViewModel.h"
#include "service/ReportService.h"

TimelineViewModel::TimelineViewModel(ReportService *reportService, QObject *parent)
    : QObject(parent), mReportService(reportService) {}

int TimelineViewModel::selectedYear() const { return mYear; }
int TimelineViewModel::selectedMonth() const { return mMonth; }
int TimelineViewModel::selectedDay() const { return mDay; }
QVariantList TimelineViewModel::records() const { return mRecords; }
bool TimelineViewModel::hasContent() const { return !mRecords.isEmpty(); }

void TimelineViewModel::selectDate(int year, int month, int day)
{
    mYear = year;
    mMonth = month;
    mDay = day;

    auto records = mReportService->getDailyRecords(year, month, day);
    mRecords.clear();
    for (const auto &r : records) {
        QVariantMap item;
        item["time"] = r.time;
        item["content"] = r.content;
        mRecords.append(item);
    }

    emit selectedDateChanged();
    emit recordsChanged();
    emit hasContentChanged();
}

void TimelineViewModel::clearSelectedDate()
{
    if (mYear && mMonth && mDay) {
        mReportService->clearDate(mYear, mMonth, mDay);
        mRecords.clear();
        emit recordsChanged();
        emit hasContentChanged();
    }
}

void TimelineViewModel::clearSelectedDates(QVariantList dates)
{
    for (const auto &v : dates) {
        QVariantMap m = v.toMap();
        mReportService->clearDate(m["year"].toInt(), m["month"].toInt(), m["day"].toInt());
    }
}
