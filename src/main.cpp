#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

#include "core/AppCore.h"
#include "service/ReportService.h"
#include "service/SyncService.h"
#include "service/HttpServer.h"
#include "ui/viewmodels/CalendarViewModel.h"
#include "ui/viewmodels/TimelineViewModel.h"
#include "ui/viewmodels/FloatingInputViewModel.h"
#include "ui/viewmodels/StorageViewModel.h"
#include "ui/viewmodels/ConnectionViewModel.h"
#include "ui/models/CalendarModel.h"
#include "ui/models/RecordListModel.h"

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
    SyncService syncService(appCore->dataManager(), appCore->databaseManager(),
                             appCore->connectionStateMachine());
    HttpServer httpServer(&syncService, appCore->connectionStateMachine());

    // Create UI models
    CalendarModel calendarModel;
    RecordListModel recordListModel;

    // Create ViewModels
    CalendarViewModel calendarVM(&reportService);
    TimelineViewModel timelineVM(&reportService, &recordListModel);
    FloatingInputViewModel floatingInputVM(&reportService);
    StorageViewModel storageVM(&reportService);
    ConnectionViewModel connectionVM(&httpServer, appCore->connectionStateMachine());

    // QML engine
    QQmlApplicationEngine engine;

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
        calendarModel.setMonthData(calendarVM.currentYear(), calendarVM.currentMonth(), days);
    });

    // Load initial data
    calendarVM.loadMonth(calendarVM.currentYear(), calendarVM.currentMonth());

    // Redirect Qt messages to file for debugging
    static QFile debugLog;
    debugLog.setFileName("qt_debug.log");
    debugLog.open(QIODevice::WriteOnly | QIODevice::Truncate);
    qInstallMessageHandler([](QtMsgType, const QMessageLogContext &, const QString &msg) {
        if (debugLog.isOpen()) {
            QTextStream s(&debugLog);
            s << msg << "\n";
            s.flush();
        }
    });

    // Load QML from filesystem
    QString mainQml = qmlDir + "/main.qml";
    engine.load(mainQml);

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    int ret = app.exec();
    appCore->shutdown();
    return ret;
}
