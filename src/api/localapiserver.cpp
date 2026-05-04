#include "awa/api/localapiserver.h"

#include <QCryptographicHash>
#include <QAbstractItemModel>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QUuid>
#include <QWebSocket>

namespace awa::api {

LocalApiServer::LocalApiServer(awa::core::DownloadManager* manager, QObject* parent)
    : QObject(parent)
    , m_manager(manager)
    , m_wsServer(QStringLiteral("AwaKurageDownloader Events"), QWebSocketServer::NonSecureMode)
{
    m_token = QString::fromLatin1(QCryptographicHash::hash(QUuid::createUuid().toByteArray(), QCryptographicHash::Sha256).toHex());
    connect(&m_httpServer, &QTcpServer::newConnection, this, &LocalApiServer::handleHttpConnection);
    connect(&m_wsServer, &QWebSocketServer::newConnection, this, &LocalApiServer::handleWebSocketConnection);
    if (m_manager) {
        connect(m_manager->downloads(), &QAbstractItemModel::rowsInserted, this, &LocalApiServer::broadcastDownloads);
        connect(m_manager->downloads(), &QAbstractItemModel::rowsRemoved, this, &LocalApiServer::broadcastDownloads);
        connect(m_manager->downloads(), &QAbstractItemModel::dataChanged, this, &LocalApiServer::broadcastDownloads);
    }
}

bool LocalApiServer::start(quint16 port)
{
    if (m_httpServer.isListening()) {
        emit listeningChanged();
        return true;
    }

    const auto address = QHostAddress::LocalHost;
    if (!m_httpServer.listen(address, port)) {
        emit errorRaised(m_httpServer.errorString());
        return false;
    }
    if (!m_wsServer.listen(address, port + 1)) {
        emit errorRaised(m_wsServer.errorString());
        m_httpServer.close();
        return false;
    }
    emit listeningChanged();
    return true;
}

void LocalApiServer::stop()
{
    m_httpServer.close();
    m_wsServer.close();
    emit listeningChanged();
}

quint16 LocalApiServer::port() const
{
    return m_httpServer.serverPort();
}

bool LocalApiServer::isListening() const
{
    return m_httpServer.isListening();
}

QString LocalApiServer::token() const
{
    return m_token;
}

void LocalApiServer::handleHttpConnection()
{
    auto* socket = m_httpServer.nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        const QByteArray request = socket->readAll();
        if (!authorized(request)) {
            writeResponse(socket, 401, "application/json", R"({"error":"unauthorized"})");
            return;
        }

        const auto firstLine = request.left(request.indexOf('\n')).trimmed();
        if (firstLine.startsWith("GET /api/v1/downloads ")) {
            writeResponse(socket, 200, "application/json", QJsonDocument(downloadsJson()).toJson(QJsonDocument::Compact));
        } else if (firstLine.startsWith("POST /api/v1/downloads ")) {
            const int bodyAt = request.indexOf("\r\n\r\n");
            const auto body = bodyAt >= 0 ? request.mid(bodyAt + 4) : QByteArray {};
            const auto document = QJsonDocument::fromJson(body);
            const auto object = document.object();
            if (object.value(QStringLiteral("magnet")).isString()) {
                m_manager->addMagnet(object.value(QStringLiteral("magnet")).toString());
                writeResponse(socket, 202, "application/json", R"({"accepted":true})");
            } else if (object.value(QStringLiteral("torrentPath")).isString()) {
                m_manager->addTorrentFile(object.value(QStringLiteral("torrentPath")).toString());
                writeResponse(socket, 202, "application/json", R"({"accepted":true})");
            } else {
                writeResponse(socket, 400, "application/json", R"({"error":"missing magnet or torrentPath"})");
            }
        } else if (firstLine.startsWith("POST /api/v1/downloads/") && firstLine.contains("/pause ")) {
            const auto id = QString::fromUtf8(firstLine).section('/', 4, 4);
            m_manager->pause(id);
            writeResponse(socket, 202, "application/json", R"({"accepted":true})");
        } else if (firstLine.startsWith("POST /api/v1/downloads/") && firstLine.contains("/resume ")) {
            const auto id = QString::fromUtf8(firstLine).section('/', 4, 4);
            m_manager->resume(id);
            writeResponse(socket, 202, "application/json", R"({"accepted":true})");
        } else if (firstLine.startsWith("DELETE /api/v1/downloads/")) {
            const auto text = QString::fromUtf8(firstLine);
            const auto id = text.section('/', 4, 4).section('?', 0, 0).section(' ', 0, 0);
            const bool removeFiles = text.contains(QStringLiteral("removeFiles=true"));
            m_manager->remove(id, removeFiles);
            writeResponse(socket, 202, "application/json", R"({"accepted":true})");
        } else {
            writeResponse(socket, 404, "application/json", R"({"error":"not found"})");
        }
    });
}

void LocalApiServer::handleWebSocketConnection()
{
    auto* socket = m_wsServer.nextPendingConnection();
    m_wsClients.push_back(socket);
    connect(socket, &QWebSocket::disconnected, this, [this, socket]() {
        m_wsClients.removeAll(socket);
        socket->deleteLater();
    });
    socket->sendTextMessage(QString::fromUtf8(QJsonDocument(downloadsJson()).toJson(QJsonDocument::Compact)));
}

void LocalApiServer::writeResponse(QTcpSocket* socket, int status, const QByteArray& contentType, const QByteArray& body)
{
    const QByteArray statusText = status == 200 ? "OK" : status == 202 ? "Accepted" : status == 400 ? "Bad Request" : status == 401 ? "Unauthorized" : "Not Found";
    socket->write("HTTP/1.1 " + QByteArray::number(status) + " " + statusText + "\r\n");
    socket->write("Content-Type: " + contentType + "\r\n");
    socket->write("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    socket->write("Connection: close\r\n\r\n");
    socket->write(body);
    socket->disconnectFromHost();
}

QJsonArray LocalApiServer::downloadsJson() const
{
    QJsonArray array;
    if (!m_manager) {
        return array;
    }

    for (const auto& item : m_manager->downloads()->items()) {
        QJsonObject object;
        object["id"] = item.id;
        object["name"] = item.name;
        object["state"] = awa::core::stateToString(item.state);
        object["progress"] = item.progress;
        object["totalBytes"] = static_cast<double>(item.totalBytes);
        object["downloadedBytes"] = static_cast<double>(item.downloadedBytes);
        object["downloadRate"] = static_cast<double>(item.downloadRate);
        object["uploadRate"] = static_cast<double>(item.uploadRate);
        object["pieceCount"] = item.pieceCount;
        object["completedPieces"] = item.completedPieces;
        object["savePath"] = item.savePath;
        array.push_back(object);
    }
    return array;
}

bool LocalApiServer::authorized(const QByteArray& request) const
{
    return request.contains("Authorization: Bearer " + m_token.toUtf8());
}

void LocalApiServer::broadcastDownloads()
{
    const auto payload = QString::fromUtf8(QJsonDocument(downloadsJson()).toJson(QJsonDocument::Compact));
    for (const auto& client : std::as_const(m_wsClients)) {
        if (client) {
            client->sendTextMessage(payload);
        }
    }
}

} // namespace awa::api
