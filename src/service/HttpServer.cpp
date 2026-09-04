#include "HttpServer.h"
#include "SyncService.h"
#include "core/LogManager.h"
#include "NetworkAddressSelector.h"

#include <QHttpServer>
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>

HttpServer::HttpServer(SyncService *syncService, QObject *parent)
    : QObject(parent), mSyncService(syncService),
      mServer(nullptr), mTcpServer(nullptr)
{
}

HttpServer::~HttpServer()
{
    stop();
}

bool HttpServer::start(int port)
{
    LogManager::instance()->info(QString("[HttpServer] 启动 HTTP 服务, 端口=%1 ...").arg(port));

    // Clean up any previous listeners (use deleteLater to avoid crash during signal chains)
    if (mTcpServer) { mTcpServer->close(); }
    if (mServer) { mServer->deleteLater(); mServer = nullptr; }
    if (mTcpServer) { mTcpServer->deleteLater(); mTcpServer = nullptr; }

    mPort = port;
    mTcpServer = new QTcpServer(this);
    mServer = new QHttpServer(this);

    setupRoutes();

    if (!mTcpServer->listen(QHostAddress::AnyIPv4, port)) {
        QString err = mTcpServer->errorString();
        LogManager::instance()->error(QString("[HttpServer] 监听端口 %1 失败: %2").arg(port).arg(err));
        return false;
    }

    mServer->bind(mTcpServer);

    // Log server info
    QString localIP = getLocalIP();
    QString qrUrl = QString("http://%1:%2").arg(localIP).arg(port);
    LogManager::instance()->info(QString("[HttpServer] HTTP 服务启动成功: %1").arg(qrUrl));

    emit started(port);
    return true;
}

void HttpServer::stop()
{
    LogManager::instance()->info("[HttpServer] stop() 开始");

    // 先关闭 TCP 监听，阻止新连接到达
    if (mTcpServer) {
        LogManager::instance()->info("[HttpServer] stop() - 关闭 QTcpServer 监听...");
        mTcpServer->close();
        LogManager::instance()->info("[HttpServer] stop() - QTcpServer 监听已关闭");
    }

    // 使用 deleteLater 延迟销毁，避免在信号链中直接 delete 导致崩溃
    // QHttpServer 和 QTcpServer 内部有事件队列交互，需要事件循环处理完再释放
    if (mServer) {
        LogManager::instance()->info("[HttpServer] stop() - 延迟销毁 QHttpServer...");
        mServer->deleteLater();
        mServer = nullptr;
        LogManager::instance()->info("[HttpServer] stop() - QHttpServer 已标记删除");
    }

    if (mTcpServer) {
        LogManager::instance()->info("[HttpServer] stop() - 延迟销毁 QTcpServer...");
        mTcpServer->deleteLater();
        mTcpServer = nullptr;
        LogManager::instance()->info("[HttpServer] stop() - QTcpServer 已标记删除");
    }

    LogManager::instance()->info("[HttpServer] stop() - 发送 stopped 信号...");
    emit stopped();
    LogManager::instance()->info("[HttpServer] stop() 完成");
}

void HttpServer::setupRoutes()
{
    // GET /health
    mServer->route("/health", [this](const QHttpServerRequest &request) {
        Q_UNUSED(request)
        QJsonObject resp = mSyncService->healthCheck();
        QByteArray body = QJsonDocument(resp).toJson(QJsonDocument::Compact);
        return QHttpServerResponse("application/json", body);
    });

    // POST /sync/submit
    mServer->route("/sync/submit", [this](const QHttpServerRequest &request) {
        if (request.method() != QHttpServerRequest::Method::Post) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
        }

        QString batchId = request.value("X-Batch-ID");
        if (batchId.isEmpty()) {
            QJsonObject err;
            err["code"] = -3;
            err["message"] = "缺少 X-Batch-ID Header";
            return QHttpServerResponse("application/json",
                                       QJsonDocument(err).toJson(QJsonDocument::Compact),
                                       QHttpServerResponse::StatusCode::BadRequest);
        }

        QJsonObject resp = mSyncService->submit(request.body(), batchId);

        using SC = QHttpServerResponse::StatusCode;
        SC httpCode = SC::Ok;
        int code = resp["code"].toInt();
        if (code == -5) httpCode = SC::TooManyRequests;
        else if (code == -2 || code == -3) httpCode = SC::BadRequest;
        else if (code != 0) httpCode = SC::InternalServerError;

        return QHttpServerResponse("application/json",
                                   QJsonDocument(resp).toJson(QJsonDocument::Compact),
                                   httpCode);
    });

    // POST /sync/fetch
    mServer->route("/sync/fetch", [this](const QHttpServerRequest &request) {
        if (request.method() != QHttpServerRequest::Method::Post) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
        }

        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            QJsonObject err;
            err["code"] = -3;
            err["message"] = "请求体不是有效 JSON";
            return QHttpServerResponse("application/json",
                                       QJsonDocument(err).toJson(QJsonDocument::Compact),
                                       QHttpServerResponse::StatusCode::BadRequest);
        }

        auto fetchResp = mSyncService->fetch(doc.object());

        if (fetchResp.httpStatus != 200) {
            // 按实际语义映射：400 → BadRequest，404 → NotFound，500 → InternalServerError
            QHttpServerResponse::StatusCode sc = QHttpServerResponse::StatusCode::InternalServerError;
            if (fetchResp.httpStatus == 400)
                sc = QHttpServerResponse::StatusCode::BadRequest;
            else if (fetchResp.httpStatus == 404)
                sc = QHttpServerResponse::StatusCode::NotFound;
            return QHttpServerResponse("application/json",
                                       QJsonDocument(fetchResp.errorJson).toJson(QJsonDocument::Compact), sc);
        }

        return QHttpServerResponse(fetchResp.contentType.toUtf8(), fetchResp.body);
    });

    // POST /sync/fetch-index
    mServer->route("/sync/fetch-index", [this](const QHttpServerRequest &request) {
        if (request.method() != QHttpServerRequest::Method::Post) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
        }

        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        if (!doc.isObject()) {
            QJsonObject err;
            err["code"] = -3;
            err["message"] = "请求体不是有效 JSON";
            return QHttpServerResponse("application/json",
                                       QJsonDocument(err).toJson(QJsonDocument::Compact),
                                       QHttpServerResponse::StatusCode::BadRequest);
        }

        auto indexResp = mSyncService->fetchIndex(doc.object());

        if (indexResp.httpStatus != 200) {
            // 按实际语义映射：400 → BadRequest，404 → NotFound，500 → InternalServerError
            QHttpServerResponse::StatusCode sc = QHttpServerResponse::StatusCode::InternalServerError;
            if (indexResp.httpStatus == 400)
                sc = QHttpServerResponse::StatusCode::BadRequest;
            else if (indexResp.httpStatus == 404)
                sc = QHttpServerResponse::StatusCode::NotFound;
            return QHttpServerResponse("application/json",
                                       QJsonDocument(indexResp.errorJson).toJson(QJsonDocument::Compact), sc);
        }

        return QHttpServerResponse(indexResp.contentType.toUtf8(), indexResp.body);
    });
}

static bool isTunnelAdapter(const QNetworkInterface &iface)
{
    static const QStringList keywords = {
        "VMware", "VirtualBox", "Hyper-V", "vEthernet",
        "Docker", "WSL", "Virtual", "TAP", "Tunnel", "VPN",
        "TUN", "Wintun", "WireGuard", "Mihomo", "Clash", "sing-box",
        "ZeroTier", "Tailscale", "Bluetooth", "Loopback"
    };
    const QString hrName = iface.humanReadableName();
    const QString name = iface.name();
    for (const auto &kw : keywords) {
        if (hrName.contains(kw, Qt::CaseInsensitive) ||
            name.contains(kw, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return iface.type() == QNetworkInterface::Virtual
        || iface.type() == QNetworkInterface::Loopback;
}

static bool isPrivateIPv4(const QHostAddress &address)
{
    const quint32 ip = address.toIPv4Address();
    return (ip & 0xff000000U) == 0x0a000000U
        || (ip & 0xfff00000U) == 0xac100000U
        || (ip & 0xffff0000U) == 0xc0a80000U;
}

QString HttpServer::getLocalIP() const
{
    QList<NetworkAddressCandidate> candidates;
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const auto &iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) ||
            !iface.flags().testFlag(QNetworkInterface::IsRunning))
            continue;
        if (isTunnelAdapter(iface)) {
            LogManager::instance()->debug(QString("[HttpServer] 排除隧道/虚拟接口: %1 (%2)")
                                              .arg(iface.humanReadableName(), iface.name()));
            continue;
        }

        const auto entries = iface.addressEntries();
        for (const auto &entry : entries) {
            const QHostAddress addr = entry.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                !addr.isLoopback()) {
                const bool physical = iface.type() == QNetworkInterface::Ethernet
                    || iface.type() == QNetworkInterface::Wifi;
                const bool privateAddress = isPrivateIPv4(addr);
                const int score = (privateAddress ? 100 : 0) + (physical ? 50 : 0);
                candidates.append({addr.toString(), iface.name(), physical, privateAddress, false});
                LogManager::instance()->debug(
                    QString("[HttpServer] IPv4 候选: %1, 接口=%2, score=%3")
                        .arg(addr.toString(), iface.humanReadableName()).arg(score));
            }
        }
    }

    const QString selected = selectLanAddress(candidates);
    if (selected != QStringLiteral("127.0.0.1")) {
        LogManager::instance()->info(
            QString("[HttpServer] 选择局域网 IPv4 地址: %1").arg(selected));
        return selected;
    }
    LogManager::instance()->warn("[HttpServer] 未找到非隧道 IPv4 地址，二维码回退到 127.0.0.1");
    return "127.0.0.1";
}

QString HttpServer::qrCodeUrl() const
{
    return QString("http://%1:%2").arg(getLocalIP()).arg(mPort);
}
