#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDate>
#include <QQuickWindow>
#include <QTimer>

#include "core/AppCore.h"
#include "core/DataManager.h"
#include "service/ReportService.h"
#include "service/SyncService.h"
#include "service/HttpServer.h"
#include "ui/viewmodels/CalendarViewModel.h"
#include "ui/viewmodels/TimelineViewModel.h"
#include "ui/viewmodels/FloatingInputViewModel.h"
#include "ui/viewmodels/StorageViewModel.h"
#include "ui/viewmodels/StorageCleanupViewModel.h"
#include "ui/viewmodels/ConnectionViewModel.h"
#include "ui/viewmodels/ToastViewModel.h"
#include "ui/models/CalendarModel.h"
#include "ui/models/RecordListModel.h"
#include "ui/QRCodeProvider.h"

// Global pointer for Qt message handler (must be accessible without capture)
static QFile *g_debugLog = nullptr;

static void debugMessageHandler(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    if (g_debugLog && g_debugLog->isOpen()) {
        QTextStream s(g_debugLog);
        s << msg << "\n";
        s.flush();
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    // Initialize core
    auto *appCore = AppCore::instance();
    if (!appCore->init()) {
        return -1;
    }

    // Create service layer objects
    ReportService reportService(appCore->dataManager());
    SyncService syncService(appCore->dataManager(), appCore->databaseManager());
    HttpServer httpServer(&syncService);

    // Create UI models
    CalendarModel calendarModel;
    RecordListModel recordListModel;

    // Create ViewModels
    CalendarViewModel calendarVM(&reportService);
    TimelineViewModel timelineVM(&reportService, &recordListModel);
    FloatingInputViewModel floatingInputVM(&reportService);
    StorageViewModel storageVM(&reportService);
    StorageCleanupViewModel storageCleanupVM(&reportService);
    ToastViewModel toastVM;
    // QR code image provider
    QRCodeProvider *qrProvider = new QRCodeProvider();

    ConnectionViewModel connectionVM(&httpServer, &syncService, qrProvider);

    // QML engine
    QQmlApplicationEngine engine;
    engine.addImageProvider("qrcode", qrProvider);

    // Find QML directory relative to executable
    QString qmlDir = QCoreApplication::applicationDirPath() + "/qml";
    if (!QDir(qmlDir).exists()) {
        // Fallback: running from project root during development
        qmlDir = QCoreApplication::applicationDirPath() + "/../../src/ui/qml";
    }
    engine.addImportPath(qmlDir);

    engine.rootContext()->setContextProperty("calendarVM", &calendarVM);
    engine.rootContext()->setContextProperty("timelineVM", &timelineVM);
    engine.rootContext()->setContextProperty("floatingInputVM", &floatingInputVM);
    engine.rootContext()->setContextProperty("storageVM", &storageVM);
    engine.rootContext()->setContextProperty("storageCleanupVM", &storageCleanupVM);
    engine.rootContext()->setContextProperty("toastVM", &toastVM);
    engine.rootContext()->setContextProperty("connectionVM", &connectionVM);
    engine.rootContext()->setContextProperty("calendarModel", &calendarModel);
    engine.rootContext()->setContextProperty("recordListModel", &recordListModel);

    // Sync calendar data when month changes
    QObject::connect(&calendarVM, &CalendarViewModel::markedDaysChanged, [&]() {
        QList<int> days;
        for (const auto &v : calendarVM.markedDays()) {
            QVariantMap m = v.toMap();
            if (m["hasReport"].toBool())
                days.append(m["day"].toInt());
        }
        QDate today = QDate::currentDate();
        calendarModel.setMonthData(calendarVM.currentYear(), calendarVM.currentMonth(), days,
                                   today.year(), today.month(), today.day(),
                                   calendarVM.selectedYear(), calendarVM.selectedMonth(), calendarVM.selectedDay());
    });

    // Sync calendar selection highlight when selected date changes
    QObject::connect(&calendarVM, &CalendarViewModel::selectedDateChanged, [&]() {
        calendarModel.setSelectedDate(calendarVM.selectedYear(), calendarVM.selectedMonth(), calendarVM.selectedDay());
    });

    // Sync calendar selection when timeline date changes
    QObject::connect(&timelineVM, &TimelineViewModel::selectedDateChanged, [&]() {
        calendarVM.setSelectedDate(timelineVM.selectedYear(), timelineVM.selectedMonth(), timelineVM.selectedDay());
    });

    // After submitting a record, navigate to today
    QObject::connect(&floatingInputVM, &FloatingInputViewModel::recordSubmitted, [&]() {
        QDate today = QDate::currentDate();
        calendarVM.loadMonth(today.year(), today.month());
        timelineVM.selectDate(today.year(), today.month(), today.day());
    });

    // 数据变更后自动刷新 UI（同步覆盖、本地记录、清空等统一走 dataChanged）
    QObject::connect(appCore->dataManager(), &DataManager::dataChanged,
                     &app, [&](int year, int month, int day) {
        calendarVM.loadMonth(calendarVM.currentYear(), calendarVM.currentMonth());
        if (year == timelineVM.selectedYear() && month == timelineVM.selectedMonth()
            && day == timelineVM.selectedDay()) {
            timelineVM.selectDate(year, month, day);
        }
        storageVM.refreshStats();
    });

    // Load initial data
    calendarVM.loadMonth(calendarVM.currentYear(), calendarVM.currentMonth());
    // Default to showing today's report
    QDate today = QDate::currentDate();
    timelineVM.selectDate(today.year(), today.month(), today.day());
    storageVM.refreshStats();

    // 启动同步恢复：事件循环第一拍执行（注册时机先于窗口 show 定时器，用户看到窗口前已完成）。
    // 与提交接口共用同步锁；失败批次 toast 提示 1 秒，无需用户操作。
    // UI 刷新由 dataChanged 接线统一处理（恢复产生的变更会自动触发）。
    QTimer::singleShot(0, &app, [&]() {
        const QStringList failed = syncService.recoverPendingBatches();
        if (!failed.isEmpty()) {
            toastVM.show(QString("%1 个批次恢复失败，无需操作").arg(failed.size()));
        }
    });

    // Redirect Qt messages to file for debugging
    QString logPath = QCoreApplication::applicationDirPath() + "/qt_debug.log";
    g_debugLog = new QFile(logPath);
    g_debugLog->open(QIODevice::WriteOnly | QIODevice::Truncate);
    qInstallMessageHandler(debugMessageHandler);

    // Load QML (window starts with visible=false)
    QString mainQml = qmlDir + "/main.qml";
    engine.load(mainQml);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    // Set window background before showing to avoid black flash
    auto windows = engine.rootObjects();
    for (auto *obj : windows) {
        auto *win = qobject_cast<QQuickWindow *>(obj);
        if (win) {
            win->setColor(QColor("#ffffff"));
            // Show after one event loop cycle to ensure QML is fully rendered
            QTimer::singleShot(0, win, &QQuickWindow::show);
        }
    }

    int ret = app.exec();
    appCore->shutdown();
    return ret;
}
