#include "HttpServer.h"
#include "SyncService.h"
#include "core/ConnectionStateMachine.h"
#include "core/LogManager.h"

#include <QHttpServer>
#include <QTcpServer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>

HttpServer::HttpServer(SyncService *syncService, ConnectionStateMachine *stateMachine,
                       QObject *parent)
    : QObject(parent), mSyncService(syncService), mStateMachine(stateMachine),
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
    mStateMachine->start();

    // Forward state changes to ViewModel (through service layer)
    connect(mStateMachine, &ConnectionStateMachine::stateChanged,
            this, &HttpServer::connectionStateChanged, Qt::UniqueConnection);

    // Log server info
    QString localIP = getLocalIP();
    QString qrUrl = QString("http://%1:%2").arg(localIP).arg(port);
    LogManager::instance()->info(QString("[HttpServer] HTTP 服务启动成功: %1").arg(qrUrl));

    emit started(port);
    return true;
}

int HttpServer::connectionState() const
{
    return static_cast<int>(mStateMachine->state());
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

    LogManager::instance()->info("[HttpServer] stop() - 停止状态机...");
    mStateMachine->stop();
    LogManager::instance()->info("[HttpServer] stop() - 状态机已停止");

    LogManager::instance()->info("[HttpServer] stop() - 发送 stopped 信号...");
    emit stopped();
    LogManager::instance()->info("[HttpServer] stop() 完成");
}

void HttpServer::setupRoutes()
{
    // POST /sync/connect
    mServer->route("/sync/connect", [this](const QHttpServerRequest &request) {
        if (request.method() != QHttpServerRequest::Method::Post) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::MethodNotAllowed);
        }

        QJsonDocument doc = QJsonDocument::fromJson(request.body());
        QJsonObject reqObj = doc.isObject() ? doc.object() : QJsonObject();

        QJsonObject resp = mSyncService->connectDevice(reqObj);
        QByteArray body = QJsonDocument(resp).toJson(QJsonDocument::Compact);

        return QHttpServerResponse("application/json", body);
    });

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

        mStateMachine->onSyncStart();
        QJsonObject resp = mSyncService->submit(request.body(), batchId);
        mStateMachine->onSyncComplete();

        using SC = QHttpServerResponse::StatusCode;
        SC httpCode = SC::Ok;
        int code = resp["code"].toInt();
        if (code == -4) httpCode = SC::Conflict;
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
            auto sc = fetchResp.httpStatus == 400
                ? QHttpServerResponse::StatusCode::BadRequest
                : QHttpServerResponse::StatusCode::NotFound;
            return QHttpServerResponse("application/json",
                                       QJsonDocument(fetchResp.errorJson).toJson(QJsonDocument::Compact), sc);
        }

        return QHttpServerResponse(fetchResp.contentType.toUtf8(), fetchResp.body);
    });
}

static bool isVirtualAdapter(const QNetworkInterface &iface)
{
    static const QStringList keywords = {
        "VMware", "VirtualBox", "Hyper-V", "vEthernet",
        "Docker", "WSL", "Virtual", "TAP", "Tunnel", "VPN",
        "Bluetooth", "Loopback"
    };
    const QString hrName = iface.humanReadableName();
    const QString name = iface.name();
    for (const auto &kw : keywords) {
        if (hrName.contains(kw, Qt::CaseInsensitive) ||
            name.contains(kw, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QString HttpServer::getLocalIP() const
{
    const auto interfaces = QNetworkInterface::allInterfaces();

    // First pass: prefer physical adapters that are up and running
    for (const auto &iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack))
            continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsUp) ||
            !iface.flags().testFlag(QNetworkInterface::IsRunning))
            continue;
        if (isVirtualAdapter(iface))
            continue;

        const auto entries = iface.addressEntries();
        for (const auto &entry : entries) {
            QHostAddress addr = entry.ip();
            if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                !addr.isLoopback()) {
                return addr.toString();
            }
        }
    }

    // Fallback: if all filtered out, return any available non-loopback IPv4
    for (const auto &iface : interfaces) {
        if (iface.flags().testFlag(QNetworkInterface::IsUp) &&
            !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            const auto entries = iface.addressEntries();
            for (const auto &entry : entries) {
                QHostAddress addr = entry.ip();
                if (addr.protocol() == QAbstractSocket::IPv4Protocol &&
                    !addr.isLoopback()) {
                    return addr.toString();
                }
            }
        }
    }
    return "127.0.0.1";
}

QString HttpServer::qrCodeUrl() const
{
    return QString("http://%1:%2").arg(getLocalIP()).arg(mPort);
}
