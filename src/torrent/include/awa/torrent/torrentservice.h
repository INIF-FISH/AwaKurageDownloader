#pragma once

#include "awa/core/downloadmanager.h"

#include <QHash>
#include <QTimer>

#include <libtorrent/session.hpp>
#include <libtorrent/torrent_handle.hpp>

#include <memory>

namespace awa::torrent {

class TorrentService final : public awa::core::TorrentBackend {
    Q_OBJECT

public:
    explicit TorrentService(QObject* parent = nullptr);
    ~TorrentService() override;

    void addTorrentFile(const QString& path, const awa::core::DownloadOptions& options) override;
    void addMagnet(const QString& uri, const awa::core::DownloadOptions& options) override;
    void pause(const QString& id) override;
    void resume(const QString& id) override;
    void remove(const QString& id, bool removeFiles) override;
    void setFilePriorities(const QString& id, const QVector<awa::core::FilePriority>& priorities) override;
    void setSpeedLimits(int downloadKiB, int uploadKiB) override;
    void setChokingStrategy(int chokingAlgorithm, int seedChokingAlgorithm, int uploadSlots, int optimisticSlots) override;

private slots:
    void pollAlerts();
    void refreshStatuses();

private:
    QString handleId(const libtorrent::torrent_handle& handle) const;
    awa::core::DownloadItem itemFromHandle(const libtorrent::torrent_handle& handle) const;
    void rememberHandle(const libtorrent::torrent_handle& handle, awa::core::DownloadState initialState);

    std::unique_ptr<libtorrent::session> m_session;
    QTimer m_alertTimer;
    QTimer m_statusTimer;
    QHash<QString, libtorrent::torrent_handle> m_handles;
    QHash<QString, awa::core::DownloadItem> m_items;
};

} // namespace awa::torrent
