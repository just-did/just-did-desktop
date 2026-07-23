#pragma once

#include <QString>
#include <QJsonObject>
#include <QByteArray>
#include <QPair>
#include <QDate>
#include <QList>
#include <QMap>

#include "common/Types.h"

class DataManager;
class DatabaseManager;
class ConnectionStateMachine;

class SyncService
{
public:
    explicit SyncService(DataManager *dataMgr, DatabaseManager *dbMgr,
                         ConnectionStateMachine *stateMachine);

    // Submit: parse body text, merge records, return response
    QJsonObject submit(const QByteArray &body, const QString &batchId);

    // Fetch: parse request JSON, fetch files, return (body, content-type)
    // Returns empty body if 404
    struct FetchResponse {
        QByteArray body;
        QString contentType;
        int httpStatus = 404;   // 200, 400, or 404
        QJsonObject errorJson;  // set when httpStatus != 200
    };
    FetchResponse fetch(const QJsonObject &request);

    // Connect / Health
    QJsonObject connectDevice(const QJsonObject &request);
    QJsonObject healthCheck();

private:
    QMap<QDate, QList<DailyRecord>> parseSubmitBody(const QString &text) const;
    QList<QDate> parseDates(const QJsonObject &request, QString &errorMsg) const;

    DataManager *mDataMgr;
    DatabaseManager *mDbMgr;
    ConnectionStateMachine *mStateMachine;
};
