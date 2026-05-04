#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>

namespace awa::core {

enum class DownloadState {
    Queued,
    FetchingMetadata,
    Downloading,
    Paused,
    Seeding,
    Finished,
    Error
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
    QString statusText;
    QString errorText;
    QDateTime createdAt;
    QStringList trackers;
};

QString stateToString(DownloadState state);

} // namespace awa::core

Q_DECLARE_METATYPE(awa::core::DownloadItem)
