#include "awa/rss/rssservice.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <algorithm>

namespace awa::rss {

RssService::RssService(awa::core::DownloadManager* manager, QObject* parent)
    : QObject(parent)
    , m_manager(manager)
{
    addDefaultSubscriptions();
}

void RssService::addSubscription(const QString& title, const QUrl& url)
{
    if (!url.isValid() || url.scheme().isEmpty()) {
        m_lastStatus = QStringLiteral("RSS 地址无效");
        emit lastStatusChanged();
        emit errorRaised(m_lastStatus);
        return;
    }

    const QString normalizedUrl = url.adjusted(QUrl::NormalizePathSegments).toString();
    const auto exists = std::any_of(m_subscriptions.cbegin(), m_subscriptions.cend(), [&](const RssSubscription& item) {
        return item.url.adjusted(QUrl::NormalizePathSegments).toString() == normalizedUrl;
    });
    if (exists) {
        m_lastStatus = QStringLiteral("订阅已存在：%1").arg(title.isEmpty() ? normalizedUrl : title);
        emit lastStatusChanged();
        return;
    }

    RssSubscription subscription;
    subscription.id = QString::fromLatin1(QCryptographicHash::hash(normalizedUrl.toUtf8(), QCryptographicHash::Sha1).toHex());
    subscription.title = title;
    subscription.url = url;
    m_subscriptions.push_back(subscription);
    m_lastStatus = QStringLiteral("已添加订阅：%1").arg(title.isEmpty() ? url.toString() : title);
    emit subscriptionsChanged();
    emit lastStatusChanged();
}

void RssService::refreshAll()
{
    if (m_subscriptions.isEmpty()) {
        m_lastStatus = QStringLiteral("暂无 RSS 订阅");
        emit lastStatusChanged();
        return;
    }

    for (const auto& subscription : m_subscriptions) {
        auto* reply = m_network.get(QNetworkRequest(subscription.url));
        connect(reply, &QNetworkReply::finished, this, [this, reply, subscription]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                m_lastStatus = reply->errorString();
                emit lastStatusChanged();
                emit errorRaised(reply->errorString());
                return;
            }
            parseFeed(subscription, reply->readAll());
        });
    }
}

bool RssService::autoDownloadEnabled() const
{
    return m_autoDownloadEnabled;
}

void RssService::setAutoDownloadEnabled(bool enabled)
{
    m_autoDownloadEnabled = enabled;
    m_lastStatus = enabled ? QStringLiteral("RSS 自动匹配已启用") : QStringLiteral("RSS 自动匹配已关闭");
    emit lastStatusChanged();
}

int RssService::subscriptionCount() const
{
    return m_subscriptions.size();
}

QString RssService::lastStatus() const
{
    return m_lastStatus;
}

void RssService::addDefaultSubscriptions()
{
    addSubscription(QStringLiteral("FOSS Torrents"), QUrl(QStringLiteral("https://fosstorrents.com/feed/torrents.xml")));
    addSubscription(QStringLiteral("Academic Torrents"), QUrl(QStringLiteral("https://academictorrents.com/rss.xml")));
    m_lastStatus = QStringLiteral("已加载默认 RSS 源");
    emit lastStatusChanged();
}

void RssService::parseFeed(const RssSubscription& subscription, const QByteArray& data)
{
    QXmlStreamReader xml(data);
    QString currentTitle;
    QString currentLink;
    QString currentInfoHash;
    int matched = 0;

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QStringLiteral("item")) {
            currentTitle.clear();
            currentLink.clear();
            currentInfoHash.clear();
        } else if (xml.isStartElement() && xml.name() == QStringLiteral("title")) {
            currentTitle = xml.readElementText();
        } else if (xml.isStartElement() && xml.name() == QStringLiteral("link")) {
            currentLink = xml.readElementText();
        } else if (xml.isStartElement() && xml.name() == QStringLiteral("infohash")) {
            currentInfoHash = xml.readElementText().trimmed();
        } else if (xml.isEndElement() && xml.name() == QStringLiteral("item")) {
            if (!currentInfoHash.isEmpty()) {
                QUrlQuery query;
                query.addQueryItem(QStringLiteral("xt"), QStringLiteral("urn:btih:%1").arg(currentInfoHash));
                if (!currentTitle.isEmpty()) {
                    query.addQueryItem(QStringLiteral("dn"), currentTitle);
                }
                currentLink = QStringLiteral("magnet:?%1").arg(query.toString(QUrl::FullyEncoded));
            }

            if (currentLink.isEmpty() || m_seenLinks.contains(currentLink)) {
                continue;
            }

            const bool ruleMatched = std::any_of(subscription.rules.cbegin(), subscription.rules.cend(), [&](const RssRule& rule) {
                return rule.enabled && currentTitle.contains(rule.contains, Qt::CaseInsensitive);
            });

            if (ruleMatched || (m_autoDownloadEnabled && subscription.autoDownload)) {
                ++matched;
                m_seenLinks.insert(currentLink);
                addMatchedLink(currentLink, currentTitle);
            }
        }
    }

    if (xml.hasError()) {
        m_lastStatus = xml.errorString();
        emit lastStatusChanged();
        emit errorRaised(xml.errorString());
        return;
    }
    m_lastStatus = QStringLiteral("刷新完成，匹配 %1 个条目").arg(matched);
    emit lastStatusChanged();
    emit refreshed(subscription.id, matched);
}

void RssService::addMatchedLink(const QString& link, const QString& title)
{
    if (!m_manager) {
        return;
    }

    if (link.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
        m_manager->addMagnet(link);
        return;
    }

    const QUrl url(link);
    if (!url.isValid() || !url.path().endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive)) {
        return;
    }

    auto* reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, title]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_lastStatus = QStringLiteral("下载 RSS 种子失败：%1").arg(reply->errorString());
            emit lastStatusChanged();
            return;
        }

        const auto cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/rss-torrents");
        QDir().mkpath(cacheDir);
        const auto baseName = QString::fromLatin1(QCryptographicHash::hash(
            (title + QString::number(QDateTime::currentMSecsSinceEpoch())).toUtf8(), QCryptographicHash::Sha1).toHex());
        const auto path = cacheDir + QStringLiteral("/") + baseName + QStringLiteral(".torrent");
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            m_lastStatus = QStringLiteral("写入 RSS 种子缓存失败");
            emit lastStatusChanged();
            return;
        }
        file.write(reply->readAll());
        file.close();
        m_manager->addTorrentFile(path);
    });
}

} // namespace awa::rss
