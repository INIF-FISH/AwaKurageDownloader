#include "awa/core/downloadmanager.h"

#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>

#include <algorithm>

namespace awa::core {

DownloadManager::DownloadManager(QObject* parent)
    : QObject(parent)
    , m_defaultSavePath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
{
    if (m_defaultSavePath.isEmpty()) {
        m_defaultSavePath = QDir::homePath();
    }
}

DownloadListModel* DownloadManager::downloads()
{
    return &m_downloads;
}

QString DownloadManager::defaultSavePath() const
{
    return m_defaultSavePath;
}

int DownloadManager::downloadLimitKiB() const
{
    return m_downloadLimitKiB;
}

int DownloadManager::uploadLimitKiB() const
{
    return m_uploadLimitKiB;
}

int DownloadManager::chokingAlgorithm() const
{
    return m_chokingAlgorithm;
}

int DownloadManager::seedChokingAlgorithm() const
{
    return m_seedChokingAlgorithm;
}

int DownloadManager::uploadSlots() const
{
    return m_uploadSlots;
}

int DownloadManager::optimisticSlots() const
{
    return m_optimisticSlots;
}

QString DownloadManager::trackerUrlsText() const
{
    return m_trackerUrlsText;
}

void DownloadManager::setTrackerUrlsText(const QString& text)
{
    const auto trackers = parseTrackerUrls(text);
    const QString normalized = trackers.join(QLatin1Char('\n'));
    if (normalized == m_trackerUrlsText) {
        return;
    }

    m_trackerUrlsText = normalized;
    emit trackersChanged();

    if (m_backend) {
        m_backend->setTrackers(trackers);
    }
}

void DownloadManager::setDefaultSavePath(const QString& path)
{
    if (path == m_defaultSavePath) {
        return;
    }
    m_defaultSavePath = path;
    emit defaultSavePathChanged();
}

void DownloadManager::setBackend(TorrentBackend* backend)
{
    if (m_backend == backend) {
        return;
    }

    if (m_backend) {
        disconnect(m_backend, nullptr, this, nullptr);
    }

    m_backend = backend;
    if (!m_backend) {
        return;
    }

    connect(m_backend, &TorrentBackend::itemUpdated, this, [this](const DownloadItem& item) {
        m_downloads.upsert(item);
    });
    connect(m_backend, &TorrentBackend::itemRemoved, &m_downloads, &DownloadListModel::removeById);
    connect(m_backend, &TorrentBackend::errorRaised, this, &DownloadManager::toastRequested);
    m_backend->setSpeedLimits(m_downloadLimitKiB, m_uploadLimitKiB);
    m_backend->setChokingStrategy(m_chokingAlgorithm, m_seedChokingAlgorithm, m_uploadSlots, m_optimisticSlots);
    m_backend->setTrackers(parseTrackerUrls(m_trackerUrlsText));
    m_backend->loadPersistedTasks();
}

void DownloadManager::addTorrentFile(const QString& path, const QVariantMap& options)
{
    if (!m_backend) {
        emit toastRequested(QStringLiteral("下载内核未就绪"));
        return;
    }
    m_backend->addTorrentFile(path, parseOptions(options));
}

void DownloadManager::addMagnet(const QString& uri, const QVariantMap& options)
{
    if (!m_backend) {
        emit toastRequested(QStringLiteral("下载内核未就绪"));
        return;
    }
    m_backend->addMagnet(uri, parseOptions(options));
}

void DownloadManager::pause(const QString& id)
{
    if (m_backend) {
        m_backend->pause(id);
    }
}

void DownloadManager::resume(const QString& id)
{
    if (m_backend) {
        m_backend->resume(id);
    }
}

void DownloadManager::remove(const QString& id, bool removeFiles)
{
    if (m_backend) {
        m_backend->remove(id, removeFiles);
    }
}

void DownloadManager::setSpeedLimits(int downloadKiB, int uploadKiB)
{
    m_downloadLimitKiB = std::max(0, downloadKiB);
    m_uploadLimitKiB = std::max(0, uploadKiB);
    emit speedLimitsChanged();

    if (m_backend) {
        m_backend->setSpeedLimits(m_downloadLimitKiB, m_uploadLimitKiB);
    }
    emit toastRequested(QStringLiteral("限速已应用"));
}

void DownloadManager::setChokingStrategy(int chokingAlgorithm, int seedChokingAlgorithm, int uploadSlots, int optimisticSlots)
{
    m_chokingAlgorithm = std::clamp(chokingAlgorithm, 0, 1);
    m_seedChokingAlgorithm = std::clamp(seedChokingAlgorithm, 0, 2);
    m_uploadSlots = std::clamp(uploadSlots, 1, 200);
    m_optimisticSlots = std::clamp(optimisticSlots, 0, 10);
    emit chokingStrategyChanged();

    if (m_backend) {
        m_backend->setChokingStrategy(m_chokingAlgorithm, m_seedChokingAlgorithm, m_uploadSlots, m_optimisticSlots);
    }
    emit toastRequested(QStringLiteral("稀有块优先与上传博弈策略已应用"));
}

DownloadOptions DownloadManager::parseOptions(const QVariantMap& options) const
{
    DownloadOptions parsed;
    parsed.savePath = options.value(QStringLiteral("savePath"), m_defaultSavePath).toString();
    parsed.startPaused = options.value(QStringLiteral("startPaused"), false).toBool();
    return parsed;
}

QStringList DownloadManager::parseTrackerUrls(const QString& text) const
{
    QStringList result;
    const auto parts = text.split(QRegularExpression(QStringLiteral("[\\r\\n,;\\s]+")), Qt::SkipEmptyParts);
    for (const auto& part : parts) {
        const auto url = part.trimmed();
        if ((url.startsWith(QStringLiteral("udp://"), Qt::CaseInsensitive)
                || url.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
                || url.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive))
            && !result.contains(url, Qt::CaseInsensitive)) {
            result.append(url);
        }
    }
    return result;
}

} // namespace awa::core
