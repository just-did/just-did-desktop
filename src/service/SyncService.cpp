#include "SyncService.h"
#include "core/DataManager.h"
#include "core/DatabaseManager.h"
#include "core/FileManager.h"
#include "core/LogManager.h"
#include "common/Constants.h"
#include "common/ErrorCode.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QTemporaryFile>
#include <QRegularExpression>
#include <QMutexLocker>
#include <ctime>

// minizip-ng (ZIP 解压 + 打包)
#include <mz.h>
#include <mz_os.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>

SyncService::SyncService(DataManager *dataMgr, DatabaseManager *dbMgr, QObject *parent)
    : QObject(parent), mDataMgr(dataMgr), mDbMgr(dbMgr)
{
}

// --- Submit ---

// 索引条目序列化：submit 的 updated_index 与 fetch-index 的 index 共用，保证两处条目格式一致
QJsonObject SyncService::indexEntryToJson(const IndexEntry &e) const
{
    QJsonObject obj;
    obj["year"] = e.year;
    obj["month"] = e.month;
    obj["day"] = e.day;
    obj["path"] = e.path;
    obj["file_size"] = e.fileSize;
    return obj;
}

QJsonObject SyncService::buildUpdatedIndexResponse(const QString &batchId, const QString &message) const
{
    auto index = mDataMgr->getUpdatedIndexForBatch(batchId);
    QJsonArray idxArr;
    for (const auto &e : index)
        idxArr.append(indexEntryToJson(e));

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

    // 拿到锁才算开始同步处理（-5 路径不点亮同步状态）
    setSyncing(true);
    QJsonObject resp = submitLocked(body, batchId);
    setSyncing(false);
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

// --- Startup recovery ---

QStringList SyncService::recoverPendingBatches()
{
    // 与提交共用同一把同步锁；持锁期间仅做 DB 查询与磁盘 rename，无任何跨线程等待
    QMutexLocker locker(&mSyncMutex);

    QStringList failed;
    const auto covering = mDbMgr->getCoveringBatches();
    if (covering.isEmpty())
        return failed;

    // 有覆盖中批次才算开始同步处理（空载启动不点亮同步状态）
    setSyncing(true);

    LogManager::instance()->info(
        QString("[SyncService] 启动同步恢复：发现 %1 个「覆盖中」批次").arg(covering.size()));

    int recovered = 0;
    for (const auto &entry : covering) {
        const QString &batchId = entry.first;

        // 解析受影响日期，非法日期跳过
        QList<QDate> dates;
        const QStringList dateStrs = entry.second.split(",", Qt::SkipEmptyParts);
        for (const auto &ds : dateStrs) {
            QDate d = QDate::fromString(ds, "yyyyMMdd");
            if (d.isValid()) dates.append(d);
        }

        ErrorCode ec = mDataMgr->coverBatch(batchId, dates);
        if (ec == ErrorCode::Success) {
            ++recovered;
        } else {
            // 失败：保持「覆盖中」状态，等待手机端重试同批ID续跑
            failed.append(batchId);
            LogManager::instance()->error(
                QString("[SyncService] 启动同步恢复：批次 %1 恢复失败，保持「覆盖中」状态").arg(batchId));
        }
    }

    LogManager::instance()->info(
        QString("[SyncService] 启动同步恢复完成：%1/%2 个批次已覆盖").arg(recovered).arg(covering.size()));
    setSyncing(false);
    return failed;
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
        resp.errorJson["code"] = 1;
        resp.errorJson["message"] = "无文件";
        return resp;
    }

    // 有文件存在：一律打包 ZIP（DEFLATE），条目路径相对 data 目录（YYYY/MM/DD.txt）
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

    void *writer = nullptr;
    mz_zip_writer_create(&writer);
    bool ok = writer != nullptr;
    if (ok)
        ok = mz_zip_writer_open_file(writer, tempPath.toUtf8().constData(), 0, 0) == MZ_OK;
    if (ok)
        mz_zip_writer_set_compress_method(writer, MZ_COMPRESS_METHOD_DEFLATE);

    for (auto it = fetchResult.files.begin(); ok && it != fetchResult.files.end(); ++it) {
        QDate date = it.key();
        const auto &records = it.value();

        QString entryPath = QString("%1/%2/%3.txt")
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
        // 空记录日期 → 空内容条目（与 FileManager::serializeContent 的空列表语义一致）
        QByteArray content = parts.isEmpty() ? QByteArray()
                                             : (parts.join("\n\n") + "\n").toUtf8();

        QByteArray nameUtf8 = entryPath.toUtf8();
        mz_zip_file fileInfo = {};
        fileInfo.version_madeby = MZ_VERSION_MADEBY;
        fileInfo.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        fileInfo.flag = MZ_ZIP_FLAG_UTF8;
        fileInfo.modified_date = time(nullptr);
        fileInfo.filename = nameUtf8.constData();
        fileInfo.filename_size = (uint16_t)nameUtf8.size();

        // 空内容条目：传非空指针 + 长度 0，产生合法空文件条目
        const char *data = content.isEmpty() ? "" : content.constData();
        if (mz_zip_writer_add_buffer(writer, (void *)data, (int32_t)content.size(), &fileInfo) != MZ_OK)
            ok = false;
    }

    if (ok)
        ok = mz_zip_writer_close(writer) == MZ_OK;
    mz_zip_writer_delete(&writer);

    if (!ok) {
        resp.httpStatus = 500;
        resp.errorJson["code"] = 1;
        resp.errorJson["message"] = "服务器内部错误";
        return resp;
    }

    // Read back the temp file into response
    tempFile.open();
    resp.body = tempFile.readAll();
    tempFile.close();
    resp.contentType = "application/zip";
    resp.httpStatus = 200;

    return resp;
}

// --- Fetch index ---

SyncService::FetchResponse SyncService::fetchIndex(const QJsonObject &request)
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

    // 按索引表查询：仅收录索引有条目的日期（升序），不做磁盘校验
    QList<IndexEntry> entries;
    if (mDataMgr->getIndexForDates(dates, entries) == ErrorCode::InternalError) {
        resp.httpStatus = 500;
        resp.errorJson["code"] = -1;
        resp.errorJson["message"] = "服务器内部错误";
        return resp;
    }

    if (entries.isEmpty()) {
        resp.httpStatus = 404;
        resp.errorJson["code"] = 1;
        resp.errorJson["message"] = "无文件";
        return resp;
    }

    QJsonArray idxArr;
    for (const auto &e : entries)
        idxArr.append(indexEntryToJson(e));

    QJsonObject success;
    success["code"] = 0;
    success["message"] = "成功";
    success["index"] = idxArr;

    resp.body = QJsonDocument(success).toJson(QJsonDocument::Compact);
    resp.contentType = "application/json";
    resp.httpStatus = 200;
    return resp;
}

// --- Sync status / Health ---

bool SyncService::isSyncing() const
{
    return mSyncing;
}

void SyncService::setSyncing(bool syncing)
{
    if (mSyncing == syncing) return;
    mSyncing = syncing;
    emit syncingChanged();
}

QJsonObject SyncService::healthCheck()
{
    // 纯存活探测：手机端靠它判断电脑端在线，无任何状态副作用
    QJsonObject resp;
    resp["status"] = "ok";
    resp["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return resp;
}
