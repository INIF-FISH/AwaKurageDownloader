#include "awa/core/downloadmanager.h"

#include <QDesktopServices>
#include <QDir>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

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

int DownloadManager::maxActiveDownloads() const
{
    return m_maxActiveDownloads;
}

bool DownloadManager::dynamicBlockTuningEnabled() const
{
    return m_dynamicBlockTuningEnabled;
}

bool DownloadManager::seedOnCompletionEnabled() const
{
    return m_seedOnCompletionEnabled;
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
    m_backend->setMaxActiveDownloads(m_maxActiveDownloads);
    m_backend->setDynamicBlockTuningEnabled(m_dynamicBlockTuningEnabled);
    m_backend->setSeedOnCompletionEnabled(m_seedOnCompletionEnabled);
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

void DownloadManager::openSavePath(const QString& id)
{
    const auto item = m_downloads.itemById(id);
    if (item.id.isEmpty() || item.savePath.isEmpty()) {
        emit toastRequested(QStringLiteral("保存目录不可用"));
        return;
    }

    const QDir directory(item.savePath);
    if (!directory.exists()) {
        emit toastRequested(QStringLiteral("保存目录不存在"));
        return;
    }

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(directory.absolutePath()))) {
        emit toastRequested(QStringLiteral("无法打开保存目录"));
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

void DownloadManager::setMaxActiveDownloads(int count)
{
    const int clamped = std::clamp(count, 1, 200);
    if (m_maxActiveDownloads == clamped) {
        return;
    }

    m_maxActiveDownloads = clamped;
    emit downloadConcurrencyChanged();

    if (m_backend) {
        m_backend->setMaxActiveDownloads(m_maxActiveDownloads);
    }
    emit toastRequested(QStringLiteral("并行下载任务数已应用"));
}

void DownloadManager::setDynamicBlockTuningEnabled(bool enabled)
{
    if (m_dynamicBlockTuningEnabled == enabled) {
        return;
    }

    m_dynamicBlockTuningEnabled = enabled;
    emit blockStrategyChanged();

    if (m_backend) {
        m_backend->setDynamicBlockTuningEnabled(m_dynamicBlockTuningEnabled);
    }
    emit toastRequested(m_dynamicBlockTuningEnabled
        ? QStringLiteral("动态并行块已启用")
        : QStringLiteral("动态并行块已关闭"));
}

void DownloadManager::setSeedOnCompletionEnabled(bool enabled)
{
    if (m_seedOnCompletionEnabled == enabled) {
        return;
    }

    m_seedOnCompletionEnabled = enabled;
    emit seedingBehaviorChanged();

    if (m_backend) {
        m_backend->setSeedOnCompletionEnabled(m_seedOnCompletionEnabled);
    }
    emit toastRequested(m_seedOnCompletionEnabled
        ? QStringLiteral("任务完成后将继续做种")
        : QStringLiteral("任务完成后将停止做种"));
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
