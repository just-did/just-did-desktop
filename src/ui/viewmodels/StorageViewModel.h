#pragma once

#include <QObject>
#include <QVariantList>

class ReportService;

class StorageViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 totalSize READ totalSize NOTIFY statsChanged)
    Q_PROPERTY(QVariantList statsByYear READ statsByYear NOTIFY statsChanged)

public:
    explicit StorageViewModel(ReportService *reportService, QObject *parent = nullptr);

    qint64 totalSize() const;
    QVariantList statsByYear() const;

    Q_INVOKABLE void refreshStats();
    Q_INVOKABLE void clearDateRange(int startYear, int startMonth, int startDay,
                                    int endYear, int endMonth, int endDay);
    Q_INVOKABLE void backup(QVariantList dates);
    Q_INVOKABLE void restore(QString zipPath);

signals:
    void statsChanged();

private:
    ReportService *mReportService;
    qint64 mTotalSize = 0;
    QVariantList mStatsByYear;
};
