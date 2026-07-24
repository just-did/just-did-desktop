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
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY selectedDateChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth NOTIFY selectedDateChanged)
    Q_PROPERTY(int selectedDay READ selectedDay NOTIFY selectedDateChanged)
    Q_PROPERTY(bool canGoNext READ canGoNext NOTIFY currentMonthChanged)

public:
    explicit CalendarViewModel(ReportService *reportService, QObject *parent = nullptr);

    int currentYear() const;
    int currentMonth() const;
    QVariantList markedDays() const;
    int selectedYear() const;
    int selectedMonth() const;
    int selectedDay() const;
    bool canGoNext() const;

    Q_INVOKABLE void loadMonth(int year, int month);
    Q_INVOKABLE void prevMonth();
    Q_INVOKABLE void nextMonth();

public slots:
    void setSelectedDate(int year, int month, int day);

signals:
    void currentMonthChanged();
    void markedDaysChanged();
    void selectedDateChanged();

private:
    void refresh();

    ReportService *mReportService;
    int mYear;
    int mMonth;
    QVariantList mMarkedDays;
    int mSelectedYear;
    int mSelectedMonth;
    int mSelectedDay;
};
