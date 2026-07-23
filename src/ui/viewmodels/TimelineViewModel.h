#pragma once

#include <QObject>
#include <QVariantList>

class ReportService;

class TimelineViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY selectedDateChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth NOTIFY selectedDateChanged)
    Q_PROPERTY(int selectedDay READ selectedDay NOTIFY selectedDateChanged)
    Q_PROPERTY(QVariantList records READ records NOTIFY recordsChanged)
    Q_PROPERTY(bool hasContent READ hasContent NOTIFY hasContentChanged)

public:
    explicit TimelineViewModel(ReportService *reportService, QObject *parent = nullptr);

    int selectedYear() const;
    int selectedMonth() const;
    int selectedDay() const;
    QVariantList records() const;
    bool hasContent() const;

    Q_INVOKABLE void selectDate(int year, int month, int day);
    Q_INVOKABLE void clearSelectedDate();
    Q_INVOKABLE void clearSelectedDates(QVariantList dates);

signals:
    void selectedDateChanged();
    void recordsChanged();
    void hasContentChanged();

private:
    ReportService *mReportService;
    int mYear = 0, mMonth = 0, mDay = 0;
    QVariantList mRecords;
};
