#include "SyncService.h"
#include "core/DataManager.h"
#include "core/DatabaseManager.h"
#include "core/FileManager.h"
#include "core/ConnectionStateMachine.h"
#include "common/Constants.h"
#include "common/ErrorCode.h"

#include "SimpleZip.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QTemporaryFile>
#include <QRegularExpression>
#include <QSysInfo>

// minizip-ng (ZIP 解压)
#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

SyncService::SyncService(DataManager *dataMgr, DatabaseManager *dbMgr,
                         ConnectionStateMachine *stateMachine)
    : mDataMgr(dataMgr), mDbMgr(dbMgr), mStateMachine(stateMachine)
{
}

// --- Submit ---

QJsonObject SyncService::buildUpdatedIndexResponse(const QString &batchId, const QString &message) const
{
    auto index = mDataMgr->getUpdatedIndexForBatch(batchId);
    QJsonArray idxArr;
    for (const auto &e : index) {
        QJsonObject obj;
        obj["year"] = e.year;
        obj["month"] = e.month;
        obj["day"] = e.day;
        obj["path"] = e.path;
        obj["file_size"] = e.fileSize;
        idxArr.append(obj);
    }

    QJsonObject resp;
    resp["code"] = 0;
    resp["message"] = message;
    resp["updated_index"] = idxArr;
    return resp;
}

QJsonObject SyncService::submit(const QByteArray &body, const QString &batchId)
{
    // 同步处理锁：同一时刻至多一个同步处理，占用时返回 -5
    if (!mSyncMutex.tryLock()) {
        QJsonObject resp;
        resp["code"] = -5;
        resp["message"] = "同步处理中，请稍后重试";
        return resp;
    }
    struct LockGuard {
        QMutex &m;
        ~LockGuard() { m.unlock(); }
    } guard{mSyncMutex};

    mStateMachine->onSyncStart();
    QJsonObject resp = submitLocked(body, batchId);
    mStateMachine->onSyncComplete();
    return resp;
}

QJsonObject SyncService::submitLocked(const QByteArray &body, const QString &batchId)
{
    // 批ID 白名单校验（批ID 会拼入快照文件名）
    static const QRegularExpression batchIdRe("^[A-Za-z0-9-]+$");
    if (!batchIdRe.match(batchId).hasMatch()) {
        QJsonObject resp;
        resp["code"] = -3;
        resp["message"] = "批ID不合法";
        return resp;
    }

    // 状态分支
    QString status = mDbMgr->getBatchStatus(batchId);

    // 已完成 → 幂等返回
    if (status == Constants::BATCH_STATUS_DONE) {
        return buildUpdatedIndexResponse(batchId, "同步成功");
    }

    // 覆盖中 → 忽略数据体；否则解析 ZIP
    QMap<QDate, QList<DailyRecord>> recordsByDate;
    if (status != Constants::BATCH_STATUS_COVERING) {
        StagingParse parsed = parseStagingZip(body, batchId);
        if (parsed.errorCode != 0) {
            QJsonObject resp;
            resp["code"] = parsed.errorCode;
            resp["message"] = parsed.errorCode == -2
                ? QString("批数据解压后超过 %1MB 上限").arg(Constants::MAX_BATCH_UNZIPPED_SIZE / 1024 / 1024)
                : "批数据解析失败";
            return resp;
        }
        recordsByDate = parsed.recordsByDate;
    }

    // 两阶段合并：暂存 → 覆盖
    ErrorCode ec = mDataMgr->mergeRecords(recordsByDate, batchId);

    if (ec == ErrorCode::StorageError) {
        QJsonObject resp;
        resp["code"] = 1;
        resp["message"] = "服务器存储错误";
        return resp;
    }

    if (ec == ErrorCode::InternalError) {
        QJsonObject resp;
        resp["code"] = -1;
        resp["message"] = "服务器内部错误";
        return resp;
    }

    return buildUpdatedIndexResponse(batchId, "同步成功");
}

SyncService::StagingParse SyncService::parseStagingZip(const QByteArray &zipData, const QString &batchId)
{
    StagingParse result;

    void *reader = nullptr;
    mz_zip_reader_create(&reader);

    int32_t err = mz_zip_reader_open_buffer(reader, (uint8_t *)zipData.constData(),
                                            (int32_t)zipData.size(), 0);
    if (err != MZ_OK) {
        result.errorCode = -3;  // ZIP 损坏或非 ZIP 格式
        mz_zip_reader_delete(&reader);
        return result;
    }

    const QString folderPrefix = batchId + "/";
    const QRegularExpression stagingRe("^staging-(\\d{8})\\.txt$");

    // 第一遍：检查解压总大小上限
    qint64 totalSize = 0;
    for (err = mz_zip_reader_goto_first_entry(reader); err == MZ_OK;
         err = mz_zip_reader_goto_next_entry(reader)) {
        mz_zip_file *info = nullptr;
        mz_zip_reader_entry_get_info(reader, &info);
        if (info) {
            totalSize += info->uncompressed_size;
        }
    }
    if (totalSize > Constants::MAX_BATCH_UNZIPPED_SIZE) {
        result.errorCode = -2;
        mz_zip_reader_close(reader);
        mz_zip_reader_delete(&reader);
        return result;
    }

    // 第二遍：提取 {batchId}/ 文件夹下的 staging 文件
    for (err = mz_zip_reader_goto_first_entry(reader); err == MZ_OK;
         err = mz_zip_reader_goto_next_entry(reader)) {
        mz_zip_file *info = nullptr;
        mz_zip_reader_entry_get_info(reader, &info);
        if (!info) continue;

        QString filename = QString::fromUtf8(info->filename);
        if (filename.endsWith('/')) continue;                  // 目录条目
        if (!filename.startsWith(folderPrefix)) continue;      // 批文件夹之外 → 忽略

        // 批文件夹内必须严格匹配 staging-YYYYMMdd.txt
        QString base = filename.mid(folderPrefix.length());
        auto match = stagingRe.match(base);
        if (!match.hasMatch()) {
            result.errorCode = -3;
            break;
        }
        QDate date = QDate::fromString(match.captured(1), "yyyyMMdd");
        if (!date.isValid()) {
            result.errorCode = -3;
            break;
        }

        // 读取条目内容
        mz_zip_reader_entry_open(reader);
        QByteArray content;
        char buf[4096];
        int32_t n;
        while ((n = mz_zip_reader_entry_read(reader, buf, sizeof(buf))) > 0) {
            content.append(buf, n);
        }
        mz_zip_reader_entry_close(reader);

        // 归一化换行后解析（与日报文件同构，复用 parseContent）
        QString text = QString::fromUtf8(content);
        text.replace("\r\n", "\n");
        auto records = FileManager::parseContent(text);
        if (!records.isEmpty()) {
            result.recordsByDate[date] = records;
        }
    }

    mz_zip_reader_close(reader);
    mz_zip_reader_delete(&reader);

    // 全部暂存文件均无有效记录
    if (result.errorCode == 0 && result.recordsByDate.isEmpty()) {
        result.errorCode = -3;
    }
    return result;
}

// --- Fetch ---

QList<QDate> SyncService::parseDates(const QJsonObject &request, QString &errorMsg) const
{
    // Prefer "dates" array over "start"/"end" range
    if (request.contains("dates") && request["dates"].isArray()) {
        QJsonArray arr = request["dates"].toArray();
        if (arr.size() > Constants::MAX_FETCH_FILES) {
            errorMsg = QString("请求文件数超过上限 %1").arg(Constants::MAX_FETCH_FILES);
            return {};
        }
        QList<QDate> dates;
        for (const auto &v : arr) {
            QDate d = QDate::fromString(v.toString(), "yyyyMMdd");
            if (d.isValid()) dates.append(d);
        }
        if (dates.isEmpty()) {
            errorMsg = "未提供有效日期";
        }
        return dates;
    }

    if (request.contains("start") && request.contains("end")) {
        QDate start = QDate::fromString(request["start"].toString(), "yyyyMMdd");
        QDate end = QDate::fromString(request["end"].toString(), "yyyyMMdd");

        if (!start.isValid() || !end.isValid()) {
            errorMsg = "日期格式无效";
            return {};
        }

        if (start.daysTo(end) + 1 > Constants::MAX_FETCH_DAY_SPAN) {
            errorMsg = QString("日期区间跨度超过 %1 天限制").arg(Constants::MAX_FETCH_DAY_SPAN);
            return {};
        }

        QList<QDate> dates;
        QDate d = start;
        while (d <= end) {
            dates.append(d);
            d = d.addDays(1);
        }
        return dates;
    }

    errorMsg = "未提供日期参数";
    return {};
}

SyncService::FetchResponse SyncService::fetch(const QJsonObject &request)
{
    FetchResponse resp;

    QString errorMsg;
    QList<QDate> dates = parseDates(request, errorMsg);
    if (dates.isEmpty()) {
        resp.httpStatus = 400;
        resp.errorJson["code"] = errorMsg.contains("超") ? -2 : -3;
        resp.errorJson["message"] = errorMsg.isEmpty() ? "参数无效" : errorMsg;
        return resp;
    }

    // Fetch files via DataManager
    auto fetchResult = mDataMgr->fetchFiles(dates);
    if (fetchResult.notFound) {
        resp.httpStatus = 404;
        resp.errorJson["code"] = -1;
        resp.errorJson["message"] = "文件不存在";
        return resp;
    }

    resp.httpStatus = 200;

    int count = fetchResult.files.size();

    if (count == 1) {
        // Single file: return as text/plain
        auto it = fetchResult.files.begin();
        const auto &records = it.value();

        QStringList parts;
        for (const auto &r : records)
            parts.append(r.time + "\n" + r.content);
        QString content = parts.join("\n\n") + "\n";

        resp.body = content.toUtf8();
        resp.contentType = "text/plain; charset=utf-8";
        resp.httpStatus = 200;
    } else {
        // Multiple files: return as ZIP using SimpleZipWriter (proper central directory)
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(true);
        if (!tempFile.open()) {
            resp.httpStatus = 500;
            resp.errorJson["code"] = 1;
            resp.errorJson["message"] = "服务器内部错误";
            return resp;
        }
        QString tempPath = tempFile.fileName();
        tempFile.close();

        SimpleZipWriter writer;
        if (!writer.open(tempPath)) {
            resp.httpStatus = 500;
            resp.errorJson["code"] = 1;
            resp.errorJson["message"] = "服务器内部错误";
            return resp;
        }

        for (auto it = fetchResult.files.begin(); it != fetchResult.files.end(); ++it) {
            QDate date = it.key();
            const auto &records = it.value();

            QString entryPath = QString("data/%1/%2/%3.txt")
                                    .arg(date.year())
                                    .arg(date.month(), 2, 10, QChar('0'))
                                    .arg(date.day(), 2, 10, QChar('0'));

            auto sorted = records;
            std::sort(sorted.begin(), sorted.end(), [](const DailyRecord &a, const DailyRecord &b) {
                return a.time < b.time;
            });
            QStringList parts;
            for (const auto &r : sorted)
                parts.append(r.time + "\n" + r.content);
            QString content = parts.join("\n\n") + "\n";

            writer.addFile(entryPath, content.toUtf8());
        }
        writer.close();

        // Read back the temp file into response
        tempFile.open();
        resp.body = tempFile.readAll();
        tempFile.close();
        resp.contentType = "application/zip";
        resp.httpStatus = 200;
    }

    return resp;
}

// --- Connect / Health ---

QJsonObject SyncService::connectDevice(const QJsonObject &request)
{
    QString deviceName = request["device_name"].toString("未知设备");
    mStateMachine->onConnect();

    QJsonObject resp;
    resp["code"] = 0;
    resp["message"] = "连接成功";

    QJsonObject serverInfo;
    serverInfo["version"] = "1.0.0";
    serverInfo["hostname"] = QSysInfo::machineHostName();
    resp["server_info"] = serverInfo;

    return resp;
}

QJsonObject SyncService::healthCheck()
{
    mStateMachine->onHeartbeatReceived();

    QJsonObject resp;
    resp["status"] = "ok";
    resp["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return resp;
}
