#pragma once

#include <QObject>
#include <QVariantList>
#include <QDate>
#include <QSet>

#include "models/CalendarModel.h"

class ReportService;

// 存储管理弹窗 ViewModel：月导航 + 多选集合 + 阈值计数 + 前置门槛 + 清理执行。
// 弹窗每次打开由 requestOpen() 重置状态并重算阈值计数；持有独立 CalendarModel 供弹窗日历使用。
class StorageCleanupViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentYear READ currentYear NOTIFY currentMonthChanged)
    Q_PROPERTY(int currentMonth READ currentMonth NOTIFY currentMonthChanged)
    Q_PROPERTY(QVariantList markedDays READ markedDays NOTIFY markedDaysChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY currentMonthChanged)
    Q_PROPERTY(int pickedCount READ pickedCount NOTIFY pickedChanged)
    Q_PROPERTY(int weekCount READ weekCount NOTIFY thresholdCountsChanged)
    Q_PROPERTY(int monthCount READ monthCount NOTIFY thresholdCountsChanged)
    Q_PROPERTY(int yearCount READ yearCount NOTIFY thresholdCountsChanged)
    Q_PROPERTY(CalendarModel *calendarModel READ calendarModel CONSTANT)

public:
    explicit StorageCleanupViewModel(ReportService *reportService, QObject *parent = nullptr);

    int currentYear() const;
    int currentMonth() const;
    QVariantList markedDays() const;
    bool canGoNext() const;
    int pickedCount() const;
    int weekCount() const;
    int monthCount() const;
    int yearCount() const;
    CalendarModel *calendarModel();

    Q_INVOKABLE void loadMonth(int year, int month);
    Q_INVOKABLE void prevMonth();
    Q_INVOKABLE void nextMonth();

    // 点选切换：当前显示月内的某天（未来日期忽略）
    Q_INVOKABLE void toggleDay(int day);
    // 全选/取消全选当前显示月的全部非未来日期
    Q_INVOKABLE void toggleSelectAllMonth();

    // 前置门槛：服务已停止 + 无「覆盖中」批次才放行；返回拒绝原因（空串=放行）。
    // 放行时重置已选、跳回当前月并重算阈值计数。
    Q_INVOKABLE QString requestOpen(bool serviceRunning);

    // 四个清理动作：返回清理日期数
    Q_INVOKABLE int executeSelected();
    Q_INVOKABLE int executeBeforeWeek();
    Q_INVOKABLE int executeBeforeMonth();
    Q_INVOKABLE int executeBeforeYear();
    // 确认层统一入口：action = "selected"/"week"/"month"/"year"，返回清理日期数（未知 action 返回 0）
    Q_INVOKABLE int executePending(const QString &action);

signals:
    void currentMonthChanged();
    void markedDaysChanged();
    void pickedChanged();
    void thresholdCountsChanged();

private:
    void refresh();
    void updateModel();
    void recomputeThresholds();
    int executeDates(const QList<QDate> &dates);

    ReportService *mReportService;
    CalendarModel mCalendarModel;
    int mYear;
    int mMonth;
    QVariantList mMarkedDays;
    QSet<QDate> mPicked;
    QDate mWeekThreshold;
    QDate mMonthThreshold;
    QDate mYearThreshold;
    int mWeekCount = 0;
    int mMonthCount = 0;
    int mYearCount = 0;
};
