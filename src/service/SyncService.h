#pragma once

#include <QString>
#include <QJsonObject>
#include <QByteArray>
#include <QPair>
#include <QDate>
#include <QList>
#include <QMap>
#include <QMutex>

#include "common/Types.h"

class DataManager;
class DatabaseManager;
class ConnectionStateMachine;

class SyncService
{
public:
    explicit SyncService(DataManager *dataMgr, DatabaseManager *dbMgr,
                         ConnectionStateMachine *stateMachine);

    // Submit: 同步锁 → 校验批ID → 状态分支 → ZIP解析 → 两阶段合并，返回响应
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
    // 批数据 ZIP 解析结果：errorCode==0 表示成功
    struct StagingParse {
        int errorCode = 0;
        QMap<QDate, QList<DailyRecord>> recordsByDate;
    };

    QJsonObject submitLocked(const QByteArray &body, const QString &batchId);
    StagingParse parseStagingZip(const QByteArray &zipData, const QString &batchId);
    QJsonObject buildUpdatedIndexResponse(const QString &batchId, const QString &message) const;
    QList<QDate> parseDates(const QJsonObject &request, QString &errorMsg) const;

    DataManager *mDataMgr;
    DatabaseManager *mDbMgr;
    ConnectionStateMachine *mStateMachine;
    QMutex mSyncMutex;  // 全局同步处理锁
};
