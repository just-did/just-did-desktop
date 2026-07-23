#include "FloatingInputViewModel.h"
#include "service/ReportService.h"

FloatingInputViewModel::FloatingInputViewModel(ReportService *reportService, QObject *parent)
    : QObject(parent), mReportService(reportService) {}

QString FloatingInputViewModel::inputText() const { return mInputText; }
void FloatingInputViewModel::setInputText(const QString &text) { mInputText = text; emit inputTextChanged(); }
bool FloatingInputViewModel::isExpanded() const { return mExpanded; }
bool FloatingInputViewModel::isVisible() const { return mVisible; }
void FloatingInputViewModel::setVisible(bool v) { mVisible = v; emit isVisibleChanged(); }

void FloatingInputViewModel::submitRecord()
{
    if (mInputText.trimmed().isEmpty()) return;

    QDate today = QDate::currentDate();
    auto code = mReportService->addRecord(today.year(), today.month(), today.day(), mInputText.trimmed());

    if (code == ErrorCode::Success) {
        mInputText.clear();
        emit inputTextChanged();
        emit recordSubmitted();
    } else if (code == ErrorCode::VersionConflict) {
        emit submitFailed("保存失败，请重试");
    } else {
        emit submitFailed("保存失败");
    }
}

void FloatingInputViewModel::toggleExpand()
{
    mExpanded = !mExpanded;
    emit isExpandedChanged();
}

void FloatingInputViewModel::showMainWindow()
{
    mVisible = false;
    emit isVisibleChanged();
    emit requestShowMainWindow();
}
