#pragma once

#include <QObject>
#include <QString>
#include <QDate>

class ReportService;

class FloatingInputViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString inputText READ inputText WRITE setInputText NOTIFY inputTextChanged)
    Q_PROPERTY(bool isExpanded READ isExpanded NOTIFY isExpandedChanged)
    Q_PROPERTY(bool isVisible READ isVisible NOTIFY isVisibleChanged)

public:
    explicit FloatingInputViewModel(ReportService *reportService, QObject *parent = nullptr);

    QString inputText() const;
    void setInputText(const QString &text);
    bool isExpanded() const;
    bool isVisible() const;
    void setVisible(bool v);

    Q_INVOKABLE void submitRecord();
    Q_INVOKABLE void toggleExpand();
    Q_INVOKABLE void showMainWindow();

signals:
    void inputTextChanged();
    void isExpandedChanged();
    void isVisibleChanged();
    void recordSubmitted();
    void submitFailed(QString reason);
    void requestShowMainWindow();

private:
    ReportService *mReportService;
    QString mInputText;
    bool mExpanded = false;
    bool mVisible = true;
};
