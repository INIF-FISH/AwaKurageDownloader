#pragma once

#include "awa/core/downloadmanager.h"

#include <QJsonArray>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocketServer>
#include <QPointer>
#include <QVector>

namespace awa::api {

class LocalApiServer final : public QObject {
    Q_OBJECT
    Q_PROPERTY(quint16 port READ port NOTIFY listeningChanged)
    Q_PROPERTY(bool listening READ isListening NOTIFY listeningChanged)
    Q_PROPERTY(QString token READ token CONSTANT)

public:
    explicit LocalApiServer(awa::core::DownloadManager* manager, QObject* parent = nullptr);

    Q_INVOKABLE bool start(quint16 port = 18777);
    Q_INVOKABLE void stop();
    quint16 port() const;
    bool isListening() const;
    QString token() const;

signals:
    void listeningChanged();
    void errorRaised(const QString& message);

private:
    void handleHttpConnection();
    void handleWebSocketConnection();
    void writeResponse(QTcpSocket* socket, int status, const QByteArray& contentType, const QByteArray& body);
    QJsonArray downloadsJson() const;
    bool authorized(const QByteArray& request) const;
    void broadcastDownloads();

    awa::core::DownloadManager* m_manager = nullptr;
    QTcpServer m_httpServer;
    QWebSocketServer m_wsServer;
    QVector<QPointer<QWebSocket>> m_wsClients;
    QString m_token;
};

} // namespace awa::api
