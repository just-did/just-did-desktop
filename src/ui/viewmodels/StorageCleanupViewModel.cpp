#include "StorageCleanupViewModel.h"
#include "service/ReportService.h"

StorageCleanupViewModel::StorageCleanupViewModel(ReportService *reportService, QObject *parent)
    : QObject(parent), mReportService(reportService)
{
    QDate today = QDate::currentDate();
    mYear = today.year();
    mMonth = today.month();
    refresh();
    recomputeThresholds();
}

int StorageCleanupViewModel::currentYear() const { return mYear; }
int StorageCleanupViewModel::currentMonth() const { return mMonth; }
QVariantList StorageCleanupViewModel::markedDays() const { return mMarkedDays; }
int StorageCleanupViewModel::pickedCount() const { return mPicked.size(); }
int StorageCleanupViewModel::weekCount() const { return mWeekCount; }
int StorageCleanupViewModel::monthCount() const { return mMonthCount; }
int StorageCleanupViewModel::yearCount() const { return mYearCount; }
CalendarModel *StorageCleanupViewModel::calendarModel() { return &mCalendarModel; }

bool StorageCleanupViewModel::canGoNext() const
{
    QDate today = QDate::currentDate();
    QDate currentFirst(mYear, mMonth, 1);
    QDate todayFirst(today.year(), today.month(), 1);
    return currentFirst < todayFirst;
}

void StorageCleanupViewModel::loadMonth(int year, int month)
{
    mYear = year;
    mMonth = month;
    refresh();
}

void StorageCleanupViewModel::prevMonth()
{
    if (--mMonth < 1) { mMonth = 12; --mYear; }
    refresh();
}

void StorageCleanupViewModel::nextMonth()
{
    if (++mMonth > 12) { mMonth = 1; ++mYear; }
    refresh();
}

void StorageCleanupViewModel::refresh()
{
    auto index = mReportService->getMonthIndex(mYear, mMonth);
    mMarkedDays.clear();
    for (const auto &e : index) {
        QVariantMap day;
        day["day"] = e.day;
        day["hasReport"] = true;
        mMarkedDays.append(day);
    }
    updateModel();
    emit currentMonthChanged();
    emit markedDaysChanged();
}

void StorageCleanupViewModel::updateModel()
{
    QList<int> days;
    for (const auto &v : mMarkedDays) {
        QVariantMap m = v.toMap();
        if (m["hasReport"].toBool())
            days.append(m["day"].toInt());
    }
    QDate today = QDate::currentDate();
    mCalendarModel.setMonthData(mYear, mMonth, days,
                                today.year(), today.month(), today.day(),
                                -1, -1, -1, mPicked);
}

void StorageCleanupViewModel::toggleDay(int day)
{
    if (day < 1)
        return;
    QDate date(mYear, mMonth, day);
    if (date > QDate::currentDate())
        return;   // 未来日期不可点选

    if (mPicked.contains(date))
        mPicked.remove(date);
    else
        mPicked.insert(date);

    updateModel();
    emit pickedChanged();
}

void StorageCleanupViewModel::toggleSelectAllMonth()
{
    QDate today = QDate::currentDate();
    QList<QDate> selectable;   // 当前显示月内非未来日期
    int daysInMonth = QDate(mYear, mMonth, 1).daysInMonth();
    for (int d = 1; d <= daysInMonth; ++d) {
        QDate date(mYear, mMonth, d);
        if (date <= today)
            selectable.append(date);
    }

    bool allPicked = true;
    for (const auto &date : selectable) {
        if (!mPicked.contains(date)) { allPicked = false; break; }
    }

    for (const auto &date : selectable) {
        if (allPicked)
            mPicked.remove(date);
        else
            mPicked.insert(date);
    }

    updateModel();
    emit pickedChanged();
}

QString StorageCleanupViewModel::requestOpen(bool serviceRunning)
{
    if (serviceRunning)
        return QStringLiteral("请先停止 HTTP 服务");
    if (mReportService->hasPendingBatches())
        return QStringLiteral("存在未完成同步批次，请重启应用等待恢复");

    // 放行：重置已选、跳回当前月、重算阈值计数
    mPicked.clear();
    QDate today = QDate::currentDate();
    mYear = today.year();
    mMonth = today.month();
    refresh();
    recomputeThresholds();
    emit pickedChanged();
    return {};
}

void StorageCleanupViewModel::recomputeThresholds()
{
    QDate today = QDate::currentDate();
    mWeekThreshold = today.addDays(-7);
    mMonthThreshold = today.addMonths(-1);
    mYearThreshold = today.addYears(-1);
    mWeekCount = mReportService->getIndexOlderThan(mWeekThreshold).size();
    mMonthCount = mReportService->getIndexOlderThan(mMonthThreshold).size();
    mYearCount = mReportService->getIndexOlderThan(mYearThreshold).size();
    emit thresholdCountsChanged();
}

int StorageCleanupViewModel::executeDates(const QList<QDate> &dates)
{
    int cleared = mReportService->clearDates(dates);

    // 已删除日期退出已选集合；阈值计数重算
    for (const auto &date : dates)
        mPicked.remove(date);
    updateModel();
    emit pickedChanged();
    recomputeThresholds();
    return cleared;
}

int StorageCleanupViewModel::executeSelected()
{
    return executeDates(mPicked.values());
}

int StorageCleanupViewModel::executeBeforeWeek()
{
    QList<QDate> dates;
    for (const auto &e : mReportService->getIndexOlderThan(mWeekThreshold))
        dates.append(QDate(e.year, e.month, e.day));
    return executeDates(dates);
}

int StorageCleanupViewModel::executeBeforeMonth()
{
    QList<QDate> dates;
    for (const auto &e : mReportService->getIndexOlderThan(mMonthThreshold))
        dates.append(QDate(e.year, e.month, e.day));
    return executeDates(dates);
}

int StorageCleanupViewModel::executeBeforeYear()
{
    QList<QDate> dates;
    for (const auto &e : mReportService->getIndexOlderThan(mYearThreshold))
        dates.append(QDate(e.year, e.month, e.day));
    return executeDates(dates);
}

int StorageCleanupViewModel::executePending(const QString &action)
{
    if (action == QLatin1String("selected"))
        return executeSelected();
    if (action == QLatin1String("week"))
        return executeBeforeWeek();
    if (action == QLatin1String("month"))
        return executeBeforeMonth();
    if (action == QLatin1String("year"))
        return executeBeforeYear();
    return 0;
}
