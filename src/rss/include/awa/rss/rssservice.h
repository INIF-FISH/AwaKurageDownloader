#pragma once

#include "awa/core/downloadmanager.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>
#include <QUrl>
#include <QVector>

namespace awa::rss {

struct RssRule {
    QString name;
    QString contains;
    QString savePath;
    bool enabled = true;
};

struct RssSubscription {
    QString id;
    QString title;
    QUrl url;
    QVector<RssRule> rules;
    bool autoDownload = false;
};

class RssService final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int subscriptionCount READ subscriptionCount NOTIFY subscriptionsChanged)
    Q_PROPERTY(QString lastStatus READ lastStatus NOTIFY lastStatusChanged)

public:
    explicit RssService(awa::core::DownloadManager* manager, QObject* parent = nullptr);

    Q_INVOKABLE void addSubscription(const QString& title, const QUrl& url);
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE bool autoDownloadEnabled() const;
    Q_INVOKABLE void setAutoDownloadEnabled(bool enabled);
    int subscriptionCount() const;
    QString lastStatus() const;

signals:
    void refreshed(const QString& subscriptionId, int matchedItems);
    void errorRaised(const QString& message);
    void subscriptionsChanged();
    void lastStatusChanged();

private:
    void addDefaultSubscriptions();
    void parseFeed(const RssSubscription& subscription, const QByteArray& data);
    void addMatchedLink(const QString& link, const QString& title);

    awa::core::DownloadManager* m_manager = nullptr;
    QNetworkAccessManager m_network;
    QVector<RssSubscription> m_subscriptions;
    QSet<QString> m_seenLinks;
    bool m_autoDownloadEnabled = false;
    QString m_lastStatus;
};

} // namespace awa::rss
