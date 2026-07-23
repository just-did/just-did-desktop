#include "HttpServer.h"
#include "SyncService.h"
#include "core/ConnectionStateMachine.h"

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
    // Clean up any previous listeners (fix S2: memory leak on double-start)
    if (mServer) { delete mServer; mServer = nullptr; }
    if (mTcpServer) { mTcpServer->close(); delete mTcpServer; mTcpServer = nullptr; }

    mPort = port;
    mTcpServer = new QTcpServer(this);
    mServer = new QHttpServer(this);

    setupRoutes();

    if (!mTcpServer->listen(QHostAddress::Any, port)) {
        return false;
    }

    mServer->bind(mTcpServer);
    mStateMachine->start();

    // Forward state changes to ViewModel (through service layer)
    connect(mStateMachine, &ConnectionStateMachine::stateChanged,
            this, &HttpServer::connectionStateChanged, Qt::UniqueConnection);

    emit started(port);
    return true;
}

int HttpServer::connectionState() const
{
    return static_cast<int>(mStateMachine->state());
}

void HttpServer::stop()
{
    if (mServer) {
        delete mServer;
        mServer = nullptr;
    }
    if (mTcpServer) {
        mTcpServer->close();
        delete mTcpServer;
        mTcpServer = nullptr;
    }
    mStateMachine->stop();
    emit stopped();
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

QString HttpServer::getLocalIP() const
{
    const auto interfaces = QNetworkInterface::allInterfaces();
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
