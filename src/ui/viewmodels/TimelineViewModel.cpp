#include "TimelineViewModel.h"
#include "service/ReportService.h"
#include "ui/models/RecordListModel.h"

TimelineViewModel::TimelineViewModel(ReportService *reportService, RecordListModel *recordModel,
                                     QObject *parent)
    : QObject(parent), mReportService(reportService), mRecordModel(recordModel) {}

int TimelineViewModel::selectedYear() const { return mYear; }
int TimelineViewModel::selectedMonth() const { return mMonth; }
int TimelineViewModel::selectedDay() const { return mDay; }
bool TimelineViewModel::hasContent() const { return mRecordModel->rowCount() > 0; }

void TimelineViewModel::selectDate(int year, int month, int day)
{
    mYear = year;
    mMonth = month;
    mDay = day;

    auto records = mReportService->getDailyRecords(year, month, day);
    mRecordModel->setRecords(records);

    emit selectedDateChanged();
    emit hasContentChanged();
}

void TimelineViewModel::clearSelectedDate()
{
    if (mYear && mMonth && mDay) {
        mReportService->clearDate(mYear, mMonth, mDay);
        mRecordModel->setRecords({});
        emit hasContentChanged();
    }
}

void TimelineViewModel::clearSelectedDates(QVariantList dates)
{
    for (const auto &v : dates) {
        QVariantMap m = v.toMap();
        int y = m["year"].toInt();
        int mo = m["month"].toInt();
        int d = m["day"].toInt();
        mReportService->clearDate(y, mo, d);
    }
    // If current selection was cleared, refresh the view
    if (mYear && mMonth && mDay) {
        auto remaining = mReportService->getDailyRecords(mYear, mMonth, mDay);
        mRecordModel->setRecords(remaining);
        emit hasContentChanged();
    }
}
