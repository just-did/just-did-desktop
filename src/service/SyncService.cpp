#include "SyncService.h"
#include "core/DataManager.h"
#include "core/DatabaseManager.h"
#include "core/ConnectionStateMachine.h"
#include "common/Constants.h"
#include "common/ErrorCode.h"

#include "SimpleZip.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QDateTime>
#include <QBuffer>

SyncService::SyncService(DataManager *dataMgr, DatabaseManager *dbMgr,
                         ConnectionStateMachine *stateMachine)
    : mDataMgr(dataMgr), mDbMgr(dbMgr), mStateMachine(stateMachine)
{
}

// --- Submit ---

QMap<QDate, QList<DailyRecord>> SyncService::parseSubmitBody(const QString &text) const
{
    QMap<QDate, QList<DailyRecord>> result;

    // Split by triple newlines to get date blocks
    QStringList dateBlocks = text.split("\n\n\n", Qt::SkipEmptyParts);

    for (const auto &block : dateBlocks) {
        QString trimmed = block.trimmed();
        if (trimmed.isEmpty()) continue;

        // First line is date (YYYYMMDD)
        int firstNewline = trimmed.indexOf('\n');
        if (firstNewline <= 0) continue;

        QString dateStr = trimmed.left(firstNewline).trimmed();
        QDate date = QDate::fromString(dateStr, "yyyyMMdd");
        if (!date.isValid()) continue;

        // Remaining is records
        QString recordsPart = trimmed.mid(firstNewline + 1);

        // Split by double newlines to get individual records
        QStringList recordBlocks = recordsPart.split("\n\n", Qt::SkipEmptyParts);
        QList<DailyRecord> records;

        for (const auto &recBlock : recordBlocks) {
            int nlPos = recBlock.indexOf('\n');
            if (nlPos <= 0) continue;

            DailyRecord r;
            r.time = recBlock.left(nlPos).trimmed();
            r.content = recBlock.mid(nlPos + 1).trimmed();

            if (!r.time.isEmpty() && !r.content.isEmpty()) {
                records.append(r);
            }
        }

        if (!records.isEmpty()) {
            result[date] = records;
        }
    }

    return result;
}

QJsonObject SyncService::submit(const QByteArray &body, const QString &batchId)
{
    QString text = QString::fromUtf8(body);

    // Check idempotency
    if (mDbMgr->isBatchProcessed(batchId)) {
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
        resp["message"] = "暂存提交成功";
        resp["updated_index"] = idxArr;
        return resp;
    }

    // Parse body
    auto recordsByDate = parseSubmitBody(text);
    if (recordsByDate.isEmpty()) {
        QJsonObject resp;
        resp["code"] = -3;
        resp["message"] = "请求体解析失败或无有效记录";
        return resp;
    }

    // Merge records
    ErrorCode ec = mDataMgr->mergeRecords(recordsByDate, batchId);

    if (ec == ErrorCode::VersionConflict) {
        QJsonObject resp;
        resp["code"] = -4;
        resp["message"] = "数据版本冲突，请重试";
        return resp;
    }

    if (ec == ErrorCode::StorageError) {
        QJsonObject resp;
        resp["code"] = 1;
        resp["message"] = "服务器存储错误";
        return resp;
    }

    // Success
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
    resp["message"] = "暂存提交成功";
    resp["updated_index"] = idxArr;
    return resp;
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
        return resp; // notFound = true
    }

    // Fetch files via DataManager
    auto fetchResult = mDataMgr->fetchFiles(dates);
    if (fetchResult.notFound) {
        return resp; // notFound = true
    }

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
        resp.notFound = false;
    } else {
        // Multiple files: return as ZIP
        QByteArray zipData;
        QBuffer buffer(&zipData);
        buffer.open(QIODevice::WriteOnly);

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

            QByteArray contentBytes = content.toUtf8();
            QByteArray entry;
            QDataStream es(&entry, QIODevice::WriteOnly);
            es.setByteOrder(QDataStream::LittleEndian);

            quint16 mtime = (quint16)((QDateTime::currentSecsSinceEpoch() / 2) & 0xFFFF);
            quint16 mdate = (quint16)(((QDate::currentDate().year() - 1980) << 9) |
                                       (QDate::currentDate().month() << 5) |
                                       QDate::currentDate().day());

            es << (quint32)0x04034b50;
            es << (quint16)20 << (quint16)0 << (quint16)0;
            es << mtime << mdate;
            es << qChecksum(contentBytes);
            es << (quint32)contentBytes.size();
            es << (quint32)contentBytes.size();
            es << (quint16)entryPath.toUtf8().size();
            es << (quint16)0;

            buffer.write(entry);
            buffer.write(entryPath.toUtf8());
            buffer.write(contentBytes);
        }

        // End of central directory (empty central dir - minimal valid ZIP)
        QByteArray eocd;
        QDataStream eocs(&eocd, QIODevice::WriteOnly);
        eocs.setByteOrder(QDataStream::LittleEndian);
        eocs << (quint32)0x06054b50;
        eocs << (quint16)0 << (quint16)0;
        eocs << (quint16)0 << (quint16)0;
        eocs << (quint32)0 << (quint32)0 << (quint16)0;
        buffer.write(eocd);
        buffer.close();

        resp.body = zipData;
        resp.contentType = "application/zip";
        resp.notFound = false;
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
    serverInfo["hostname"] = deviceName;
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
