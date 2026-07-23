#pragma once

#include <QObject>
#include <QVariantList>

class ReportService;
class RecordListModel;

class TimelineViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int selectedYear READ selectedYear NOTIFY selectedDateChanged)
    Q_PROPERTY(int selectedMonth READ selectedMonth NOTIFY selectedDateChanged)
    Q_PROPERTY(int selectedDay READ selectedDay NOTIFY selectedDateChanged)
    Q_PROPERTY(bool hasContent READ hasContent NOTIFY hasContentChanged)

public:
    explicit TimelineViewModel(ReportService *reportService, RecordListModel *recordModel,
                               QObject *parent = nullptr);

    int selectedYear() const;
    int selectedMonth() const;
    int selectedDay() const;
    bool hasContent() const;

    Q_INVOKABLE void selectDate(int year, int month, int day);
    Q_INVOKABLE void clearSelectedDate();
    Q_INVOKABLE void clearSelectedDates(QVariantList dates);

signals:
    void selectedDateChanged();
    void hasContentChanged();

private:
    ReportService *mReportService;
    RecordListModel *mRecordModel;
    int mYear = 0, mMonth = 0, mDay = 0;
};
