#pragma once

#include <QObject>
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

class SyncService : public QObject
{
    Q_OBJECT
public:
    explicit SyncService(DataManager *dataMgr, DatabaseManager *dbMgr,
                         QObject *parent = nullptr);

    // Submit: 同步锁 → 校验批ID → 状态分支 → ZIP解析 → 两阶段合并，返回响应
    QJsonObject submit(const QByteArray &body, const QString &batchId);

    // 启动恢复：补完所有「覆盖中」批次（与提交共用同步锁），返回失败批次ID列表
    QStringList recoverPendingBatches();

    // Fetch / FetchIndex: 解析请求 JSON 后返回响应（body + content-type）。
    // 200 时 body 为有效载荷（fetch 为 ZIP，fetchIndex 为 JSON）；
    // 非 200 时 errorJson 有效。
    struct FetchResponse {
        QByteArray body;
        QString contentType;
        int httpStatus = 404;   // 200, 400, 404, 500
        QJsonObject errorJson;  // set when httpStatus != 200
    };
    FetchResponse fetch(const QJsonObject &request);

    // Fetch index: 请求格式与 fetch 一致，返回 pc_daliy_report_index 条目列表（只读，不持锁）
    FetchResponse fetchIndex(const QJsonObject &request);

    // Health
    QJsonObject healthCheck();

    // 同步状态：提交处理或启动恢复执行中为 true（界面显示「同步中」）
    bool isSyncing() const;

signals:
    void syncingChanged();

private:
    // 批数据 ZIP 解析结果：errorCode==0 表示成功
    struct StagingParse {
        int errorCode = 0;
        QMap<QDate, QList<DailyRecord>> recordsByDate;
    };

    void setSyncing(bool syncing);
    QJsonObject submitLocked(const QByteArray &body, const QString &batchId);
    StagingParse parseStagingZip(const QByteArray &zipData, const QString &batchId);
    QJsonObject buildUpdatedIndexResponse(const QString &batchId, const QString &message) const;
    QJsonObject indexEntryToJson(const IndexEntry &e) const;
    QList<QDate> parseDates(const QJsonObject &request, QString &errorMsg) const;

    DataManager *mDataMgr;
    DatabaseManager *mDbMgr;

    // 全局同步处理锁：提交与启动恢复共用（同一时刻至多一个同步操作）。
    // 锁纪律（防死锁）：
    //  1. 提交侧必须 tryLock 非阻塞，拿不到即返回 -5；
    //  2. 恢复侧阻塞持锁，但持锁期间禁止任何跨线程阻塞等待。
    QMutex mSyncMutex;

    // 同步状态忙标志（全主线程访问，无需加锁）：
    // 提交拿到锁后点亮、处理完熄灭（-5 不点亮）；启动恢复存在覆盖中批次时点亮
    bool mSyncing = false;
};
