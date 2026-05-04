#include "awa/core/downloadmanager.h"

#include <QDir>
#include <QStandardPaths>

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
    if (!m_backend) {
        emit toastRequested(QStringLiteral("下载内核未就绪"));
        return;
    }

    m_backend->setSpeedLimits(downloadKiB, uploadKiB);
    emit toastRequested(QStringLiteral("限速已应用"));
}

DownloadOptions DownloadManager::parseOptions(const QVariantMap& options) const
{
    DownloadOptions parsed;
    parsed.savePath = options.value(QStringLiteral("savePath"), m_defaultSavePath).toString();
    parsed.startPaused = options.value(QStringLiteral("startPaused"), false).toBool();
    return parsed;
}

} // namespace awa::core
