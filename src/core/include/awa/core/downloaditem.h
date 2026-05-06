#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace awa::core {

enum class DownloadState {
    Queued = 0,
    FetchingMetadata = 1,
    Downloading = 2,
    PausedDownloading = 3,
    Seeding = 4,
    Finished = 5,
    Error = 6,
    PausedSeeding = 7
};

struct DownloadOptions {
    QString savePath;
    bool startPaused = false;
};

struct FilePriority {
    int index = 0;
    int priority = 1;
};

struct DownloadItem {
    QString id;
    QString name;
    QString source;
    QString savePath;
    DownloadState state = DownloadState::Queued;
    double progress = 0.0;
    qint64 totalBytes = 0;
    qint64 downloadedBytes = 0;
    qint64 downloadRate = 0;
    qint64 uploadRate = 0;
    int pieceCount = 0;
    int completedPieces = 0;
    int blockSize = 0;
    QString pieceMap;
    double ratio = 0.0;
    int peerCount = 0;
    int seedCount = 0;
    int connectionCount = 0;
    int trackerCount = 0;
    int workingTrackerCount = 0;
    int failedTrackerCount = 0;
    QString dhtStatusText;
    QString connectionHealthText;
    QString statusText;
    QString errorText;
    QDateTime createdAt;
    QStringList trackers;
};

QString stateToString(DownloadState state);

} // namespace awa::core

Q_DECLARE_METATYPE(awa::core::DownloadItem)
