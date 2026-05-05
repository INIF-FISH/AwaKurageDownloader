#pragma once

#include "awa/core/downloadmanager.h"

#include <QHash>
#include <QStringList>
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
    void setTrackers(const QStringList& trackers) override;
    void loadPersistedTasks() override;

private slots:
    void pollAlerts();
    void refreshStatuses();
    void requestResumeDataForAll();

private:
    QString handleId(const libtorrent::torrent_handle& handle) const;
    awa::core::DownloadItem itemFromHandle(const libtorrent::torrent_handle& handle) const;
    void rememberHandle(const libtorrent::torrent_handle& handle, awa::core::DownloadState initialState);
    QString persistenceDir() const;
    QString indexFilePath() const;
    QString resumeDataPath(const QString& id) const;
    void persistItems() const;
    void requestResumeData(const QString& id, bool force = false);
    void writeResumeData(const QString& id, const libtorrent::add_torrent_params& params) const;
    void applyTrackers(libtorrent::add_torrent_params& params) const;
    void applyTrackersToHandle(const libtorrent::torrent_handle& handle) const;

    std::unique_ptr<libtorrent::session> m_session;
    QTimer m_alertTimer;
    QTimer m_statusTimer;
    QTimer m_resumeSaveTimer;
    QHash<QString, libtorrent::torrent_handle> m_handles;
    QHash<QString, awa::core::DownloadItem> m_items;
    QStringList m_defaultTrackers;
    bool m_persistedTasksLoaded = false;
};

} // namespace awa::torrent
