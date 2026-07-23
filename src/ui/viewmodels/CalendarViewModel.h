#pragma once

#include <QObject>
#include <QVariantList>
#include <QDate>

class ReportService;

class CalendarViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int currentYear READ currentYear NOTIFY currentMonthChanged)
    Q_PROPERTY(int currentMonth READ currentMonth NOTIFY currentMonthChanged)
    Q_PROPERTY(QVariantList markedDays READ markedDays NOTIFY markedDaysChanged)

public:
    explicit CalendarViewModel(ReportService *reportService, QObject *parent = nullptr);

    int currentYear() const;
    int currentMonth() const;
    QVariantList markedDays() const;

    Q_INVOKABLE void loadMonth(int year, int month);
    Q_INVOKABLE void prevMonth();
    Q_INVOKABLE void nextMonth();

signals:
    void currentMonthChanged();
    void markedDaysChanged();

private:
    void refresh();

    ReportService *mReportService;
    int mYear;
    int mMonth;
    QVariantList mMarkedDays;
};
