#pragma once

#include "awa/core/downloadmanager.h"

#include <QHash>
#include <QSet>
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
    void setMaxActiveDownloads(int count) override;
    void setDynamicBlockTuningEnabled(bool enabled) override;
    void setSeedOnCompletionEnabled(bool enabled) override;
    void setTrackers(const QStringList& trackers) override;
    void loadPersistedTasks() override;

private slots:
    void pollAlerts();
    void refreshStatuses();
    void requestResumeDataForAll();
    void restartDownloadsAfterNetworkChange();

private:
    struct TrackerHealth {
        int successes = 0;
        int failures = 0;
        int peers = 0;
        QDateTime lastSuccess;
        QDateTime lastFailure;
    };

    struct PieceProbeState {
        qint64 lastDownloadedBytes = 0;
        int lowThroughputSamples = 0;
        int healthySamples = 0;
        int targetActivePieces = 4;
        QSet<int> deadlinePieces;
        QSet<int> unhealthyPieces;
    };

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
    bool shouldIgnoreId(const QString& id) const;
    int trackerHealthScore(const QString& tracker) const;
    void recordTrackerResult(const QString& tracker, bool success, int peers = 0);
    void reprioritizeTrackers();
    bool hasActiveDownloads() const;
    void updateDynamicBlockTuning();
    void updateProbePieceSelection(
        const QString& id,
        const libtorrent::torrent_handle& handle,
        const libtorrent::torrent_status& status,
        const std::vector<libtorrent::partial_piece_info>& downloadingPieces);
    void updateDownloadQueue();
    void updateSeedingPriority();

    std::unique_ptr<libtorrent::session> m_session;
    QTimer m_alertTimer;
    QTimer m_statusTimer;
    QTimer m_resumeSaveTimer;
    QTimer m_networkRestartTimer;
    QHash<QString, libtorrent::torrent_handle> m_handles;
    QHash<QString, awa::core::DownloadItem> m_items;
    QHash<QString, QDateTime> m_lastMetadataRetry;
    QHash<QString, QDateTime> m_lastPeerDiscovery;
    QHash<QString, int> m_metadataRetryCounts;
    QHash<QString, TrackerHealth> m_trackerHealth;
    QHash<QString, PieceProbeState> m_pieceProbes;
    QSet<QString> m_removedIds;
    QSet<QString> m_userPausedIds;
    QSet<QString> m_schedulerPausedIds;
    QSet<QString> m_priorityPausedSeeds;
    QStringList m_defaultTrackers;
    bool m_persistedTasksLoaded = false;
    bool m_seedOnCompletionEnabled = true;
    bool m_dynamicBlockTuningEnabled = true;
    int m_maxActiveDownloads = 20;
    int m_requestQueueTime = 6;
    int m_maxOutRequestQueue = 768;
};

} // namespace awa::torrent
