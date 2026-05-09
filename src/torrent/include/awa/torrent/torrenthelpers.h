#pragma once

#include "awa/core/downloaditem.h"
#include "awa/torrent/taskstate.h"

#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <QString>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/address.hpp>
#include <libtorrent/bitfield.hpp>
#include <libtorrent/download_priority.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_info.hpp>

#include <algorithm>
#include <map>
#include <vector>

namespace awa::torrent {
namespace lt = libtorrent;

namespace detail {
constexpr int DefaultRequestQueueTime = 6;
constexpr int DefaultMaxOutRequestQueue = 768;
constexpr int MinProbePieces = 4;
constexpr int MaxProbePieces = 64;
constexpr int ProbeBaselineSamples = 8;
constexpr int ProbeEvaluationSamples = 8;
constexpr int ProbeCooldownSamples = 5;
constexpr int ProbeMinGainPermille = 30;
constexpr int ProbeLossPermille = 30;
constexpr int MaxSharedPeerFlows = 48;

inline QString hashId(const lt::info_hash_t& hashes)
{
    const auto best = hashes.get_best();
    return QString::fromLatin1(QByteArray(best.data(), best.size()).toHex());
}

inline QString pieceMap(
    const lt::typed_bitfield<lt::piece_index_t>& pieces,
    const std::vector<lt::partial_piece_info>& downloadingPieces = {},
    const QSet<int>& unhealthyPieces = {})
{
    const int total = pieces.size();
    if (total <= 0) {
        return {};
    }

    QString map;
    map.reserve(total);
    for (int piece = 0; piece < total; ++piece) {
        map.append(pieces.get_bit(lt::piece_index_t(piece)) ? QLatin1Char('1') : QLatin1Char('0'));
    }
    for (const auto& pieceInfo : downloadingPieces) {
        const int piece = static_cast<int>(pieceInfo.piece_index);
        if (piece >= 0 && piece < total && map.at(piece) != QLatin1Char('1')
            && (pieceInfo.requested > 0 || pieceInfo.writing > 0 || pieceInfo.finished > 0)) {
            map[piece] = QLatin1Char('2');
        }
    }
    for (const int piece : unhealthyPieces) {
        if (piece >= 0 && piece < total && map.at(piece) != QLatin1Char('1')) {
            map[piece] = QLatin1Char('3');
        }
    }
    return map;
}

inline bool hasMeaningfulTransferSnapshot(const lt::torrent_status& status)
{
    return status.has_metadata
        && (status.total_wanted > 0
            || status.total_wanted_done > 0
            || status.progress_ppm > 0
            || !status.pieces.empty()
            || status.num_pieces > 0
            || status.block_size > 0
            || status.is_seeding);
}

inline qint64 jsonInt64(const QJsonObject& object, const QString& key)
{
    const auto value = object.value(key);
    if (value.isString()) {
        return value.toString().toLongLong();
    }
    if (value.isDouble()) {
        return static_cast<qint64>(value.toDouble());
    }
    return 0;
}

inline double jsonDouble(const QJsonObject& object, const QString& key)
{
    const auto value = object.value(key);
    if (value.isString()) {
        return value.toString().toDouble();
    }
    if (value.isDouble()) {
        return value.toDouble();
    }
    return 0.0;
}

inline bool pieceMapHasProgress(const QString& map)
{
    return map.contains(QLatin1Char('1')) || map.contains(QLatin1Char('2'));
}

inline quint32 ipv4ToInt(const lt::address_v4& address)
{
    return address.to_uint();
}

inline bool isPublicAddress(const lt::address& address)
{
    if (address.is_loopback() || address.is_unspecified() || address.is_multicast()) {
        return false;
    }

    if (address.is_v4()) {
        const quint32 value = ipv4ToInt(address.to_v4());
        const quint8 a = static_cast<quint8>((value >> 24) & 0xff);
        const quint8 b = static_cast<quint8>((value >> 16) & 0xff);
        if (a == 10 || a == 127 || a == 0) return false;
        if (a == 100 && b >= 64 && b <= 127) return false;
        if (a == 169 && b == 254) return false;
        if (a == 172 && b >= 16 && b <= 31) return false;
        if (a == 192 && b == 168) return false;
        if (a >= 224) return false;
        return true;
    }

    if (address.is_v6()) {
        const auto bytes = address.to_v6().to_bytes();
        if ((bytes[0] & 0xfe) == 0xfc) return false;
        if (bytes[0] == 0xfe && (bytes[1] & 0xc0) == 0x80) return false;
        return true;
    }

    return false;
}

inline int completedPieceCount(const lt::typed_bitfield<lt::piece_index_t>& pieces)
{
    int count = 0;
    for (int piece = 0; piece < pieces.size(); ++piece) {
        if (pieces.get_bit(lt::piece_index_t(piece))) {
            ++count;
        }
    }
    return count;
}

inline qint64 completedBytes(const lt::torrent_info& info, const lt::typed_bitfield<lt::piece_index_t>& pieces)
{
    qint64 bytes = 0;
    const int pieceCount = std::min(pieces.size(), info.num_pieces());
    for (int piece = 0; piece < pieceCount; ++piece) {
        if (pieces.get_bit(lt::piece_index_t(piece))) {
            bytes += info.piece_size(lt::piece_index_t(piece));
        }
    }
    return bytes;
}

inline QString resumePieceMap(
    int pieceCount,
    const lt::typed_bitfield<lt::piece_index_t>* completedPieces,
    const std::map<lt::piece_index_t, lt::bitfield>& unfinishedPieces)
{
    QString map;
    map.reserve(pieceCount);
    for (int piece = 0; piece < pieceCount; ++piece) {
        const bool completed = completedPieces
            && piece < completedPieces->size()
            && completedPieces->get_bit(lt::piece_index_t(piece));
        map.append(completed ? QLatin1Char('1') : QLatin1Char('0'));
    }
    for (const auto& [pieceIndex, blocks] : unfinishedPieces) {
        const int piece = static_cast<int>(pieceIndex);
        if (piece >= 0 && piece < pieceCount && !blocks.empty() && map.at(piece) != QLatin1Char('1')) {
            map[piece] = QLatin1Char('2');
        }
    }
    return map;
}

inline const lt::typed_bitfield<lt::piece_index_t>* bestCompletedResumePieces(const lt::add_torrent_params& params)
{
    if (!params.verified_pieces.empty() && completedPieceCount(params.verified_pieces) > 0) {
        return &params.verified_pieces;
    }
    if (!params.have_pieces.empty() && completedPieceCount(params.have_pieces) > 0) {
        return &params.have_pieces;
    }
    return nullptr;
}

inline bool hasTransferSnapshot(const awa::core::DownloadItem& item)
{
    return item.totalBytes > 0
        || item.progress > 0.0
        || item.downloadedBytes > 0
        || item.completedPieces > 0
        || item.pieceCount > 0
        || pieceMapHasProgress(item.pieceMap);
}

inline bool hasProgressSnapshot(const awa::core::DownloadItem& item)
{
    return item.progress > 0.0
        || item.downloadedBytes > 0
        || item.completedPieces > 0
        || pieceMapHasProgress(item.pieceMap);
}

inline bool pieceMapIsComplete(const QString& map)
{
    if (map.isEmpty()) {
        return false;
    }

    for (const auto ch : map) {
        if (ch != QLatin1Char('1')) {
            return false;
        }
    }
    return true;
}

inline bool hasCompleteTransferSnapshot(const awa::core::DownloadItem& item)
{
    return (item.totalBytes > 0 && item.downloadedBytes >= item.totalBytes)
        || item.progress >= 0.999999
        || (item.pieceCount > 0 && item.completedPieces >= item.pieceCount)
        || pieceMapIsComplete(item.pieceMap);
}

inline void normalizeTransferSnapshot(awa::core::DownloadItem& item)
{
    if (item.totalBytes > 0) {
        item.downloadedBytes = std::clamp<qint64>(item.downloadedBytes, 0, item.totalBytes);
    } else if (item.downloadedBytes < 0) {
        item.downloadedBytes = 0;
    }

    if (item.pieceCount > 0) {
        item.completedPieces = std::clamp(item.completedPieces, 0, item.pieceCount);
    } else if (item.completedPieces < 0) {
        item.completedPieces = 0;
    }

    item.progress = std::clamp(item.progress, 0.0, 1.0);
    if (item.totalBytes > 0) {
        item.progress = std::clamp(
            static_cast<double>(item.downloadedBytes) / static_cast<double>(item.totalBytes),
            0.0,
            1.0);
    }
}

inline void fillMissingTransferSnapshot(awa::core::DownloadItem& item, const awa::core::DownloadItem& fallback)
{
    if (!hasTransferSnapshot(fallback)) {
        return;
    }

    if (!hasProgressSnapshot(item) && hasProgressSnapshot(fallback)) {
        item.progress = fallback.progress;
        item.downloadedBytes = fallback.downloadedBytes;
        item.completedPieces = fallback.completedPieces;
    }
    if (item.totalBytes <= 0 && fallback.totalBytes > 0) {
        item.totalBytes = fallback.totalBytes;
    }
    if (item.pieceCount <= 0 && fallback.pieceCount > 0) {
        item.pieceCount = fallback.pieceCount;
    }
    if (item.blockSize <= 0 && fallback.blockSize > 0) {
        item.blockSize = fallback.blockSize;
    }
    if (!pieceMapHasProgress(item.pieceMap) && pieceMapHasProgress(fallback.pieceMap)) {
        item.pieceMap = fallback.pieceMap;
    } else if (item.pieceMap.isEmpty() && !fallback.pieceMap.isEmpty()) {
        item.pieceMap = fallback.pieceMap;
    }
    normalizeTransferSnapshot(item);
}

inline void applyResumeTransferSnapshot(awa::core::DownloadItem& item, const lt::add_torrent_params& params)
{
    if (!params.ti) {
        return;
    }

    awa::core::DownloadItem resume;
    resume.totalBytes = params.ti->total_size();
    resume.pieceCount = params.ti->num_pieces();
    resume.blockSize = params.ti->piece_length();
    const auto* pieces = bestCompletedResumePieces(params);
    if (pieces) {
        resume.completedPieces = completedPieceCount(*pieces);
        resume.downloadedBytes = completedBytes(*params.ti, *pieces);
    }
    if (params.total_downloaded > resume.downloadedBytes) {
        resume.downloadedBytes = params.total_downloaded;
    }
    normalizeTransferSnapshot(resume);
    resume.progress = resume.totalBytes > 0
        ? static_cast<double>(resume.downloadedBytes) / static_cast<double>(resume.totalBytes)
        : 0.0;
    resume.pieceMap = resumePieceMap(resume.pieceCount, pieces, params.unfinished_pieces);
    fillMissingTransferSnapshot(item, resume);
}

inline bool liveSnapshotWouldResetProgress(const awa::core::DownloadItem& stored, const lt::torrent_status& status)
{
    return hasProgressSnapshot(stored)
        && status.total_wanted_done <= 0
        && status.progress_ppm <= 0
        && status.num_pieces <= 0
        && (!status.pieces.empty() || status.total_wanted > 0 || status.block_size > 0);
}

inline bool liveSnapshotWouldResetPieceMap(const awa::core::DownloadItem& stored, const lt::torrent_status& status)
{
    if (!pieceMapHasProgress(stored.pieceMap) || status.pieces.empty()) {
        return false;
    }

    for (int piece = 0; piece < status.pieces.size(); ++piece) {
        if (status.pieces.get_bit(lt::piece_index_t(piece))) {
            return false;
        }
    }
    return true;
}

inline bool isPausedState(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::PausedDownloading
        || state == awa::core::DownloadState::PausedSeeding;
}

inline bool isCompletedState(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::Finished
        || state == awa::core::DownloadState::Seeding
        || state == awa::core::DownloadState::PausedSeeding;
}

inline bool isDeletedState(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::Deleted;
}

inline bool hasCompletedIntent(const awa::core::DownloadItem& item)
{
    return item.isComplete || isCompletedState(item.state);
}

inline bool completedPayloadIsMissing(const lt::torrent_handle& handle, const lt::torrent_status& status, const awa::core::DownloadItem& stored)
{
    if (!hasCompletedIntent(stored) && !hasCompleteTransferSnapshot(stored) && !isDeletedState(stored.state)) {
        return false;
    }

    const auto info = handle.torrent_file();
    if (!info) {
        return false;
    }

    const QString statusSavePath = QString::fromStdString(status.save_path);
    const QString savePath = statusSavePath.isEmpty() ? stored.savePath : statusSavePath;
    if (savePath.isEmpty()) {
        return false;
    }

    const auto& files = info->files();
    bool checkedPayloadFile = false;
    const std::string nativeSavePath = savePath.toStdString();
    for (int i = 0; i < files.num_files(); ++i) {
        const auto fileIndex = lt::file_index_t(i);
        if (files.pad_file_at(fileIndex)) {
            continue;
        }

        checkedPayloadFile = true;
        const QFileInfo fileInfo(QString::fromStdString(files.file_path(fileIndex, nativeSavePath)));
        if (!fileInfo.exists() || !fileInfo.isFile()) {
            return true;
        }

        const auto expectedSize = files.file_size(fileIndex);
        if (expectedSize > 0 && fileInfo.size() < expectedSize) {
            return true;
        }
    }

    return !checkedPayloadFile;
}

inline void applyDeletedPayloadState(awa::core::DownloadItem& item)
{
    item.state = awa::core::DownloadState::Deleted;
    item.isComplete = false;
    item.downloadRate = 0;
    item.uploadRate = 0;
    item.statusText = QStringLiteral("本地文件已删除");
    item.connectionHealthText = QStringLiteral("本地文件已删除");
    if (item.createdAt.isNull()) {
        item.createdAt = QDateTime::currentDateTimeUtc();
    }
    normalizeTransferSnapshot(item);
}

inline bool shouldPauseAsSeeding(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::Seeding
        || state == awa::core::DownloadState::Finished
        || state == awa::core::DownloadState::PausedSeeding;
}

inline bool shouldUserPauseAsSeeding(const awa::core::DownloadItem& item)
{
    return hasCompletedIntent(item)
        || hasCompleteTransferSnapshot(item)
        || shouldPauseAsSeeding(item.state);
}

inline void applyUserPausedState(
    awa::core::DownloadItem& item,
    const awa::core::DownloadItem* fallback = nullptr)
{
    const bool pauseAsSeed = shouldUserPauseAsSeeding(item)
        || (fallback && shouldUserPauseAsSeeding(*fallback));
    item.state = pauseAsSeed
        ? awa::core::DownloadState::PausedSeeding
        : awa::core::DownloadState::PausedDownloading;
    item.isComplete = isCompletedState(item.state);
    item.statusText = awa::core::stateToString(item.state);
    item.downloadRate = 0;
    item.uploadRate = 0;
}

inline QString pauseOwnerToString(PauseOwner owner)
{
    switch (owner) {
    case PauseOwner::User: return QStringLiteral("user");
    case PauseOwner::Scheduler: return QStringLiteral("scheduler");
    case PauseOwner::None: return QStringLiteral("none");
    }
    return QStringLiteral("none");
}

inline PauseOwner pauseOwnerFromString(const QString& owner)
{
    if (owner == QStringLiteral("user")) {
        return PauseOwner::User;
    }
    if (owner == QStringLiteral("scheduler")) {
        return PauseOwner::Scheduler;
    }
    return PauseOwner::None;
}

inline TaskStateInput taskStateInputForItemAndStatus(
    const awa::core::DownloadItem& item,
    const awa::core::DownloadItem& storedSnapshot,
    const lt::torrent_status& status,
    PauseOwner pauseOwner,
    bool priorityPausedSeed,
    bool seedOnCompletionEnabled,
    bool hasReliableLiveState)
{
    const bool livePaused = (status.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {};
    const bool liveComplete = status.has_metadata && status.progress_ppm >= 1000000;
    const bool snapshotComplete = hasCompleteTransferSnapshot(item) || hasCompleteTransferSnapshot(storedSnapshot);
    const bool persistedComplete = hasCompletedIntent(item) || hasCompletedIntent(storedSnapshot);
    const bool effectiveComplete = liveComplete || snapshotComplete || persistedComplete;

    return {
        storedSnapshot.state,
        pauseOwner,
        priorityPausedSeed,
        seedOnCompletionEnabled,
        hasReliableLiveState,
        livePaused,
        status.has_metadata,
        effectiveComplete,
        status.is_seeding || (effectiveComplete && isCompletedState(storedSnapshot.state))
    };
}

inline TaskStateResult taskStateForItemAndStatus(
    const awa::core::DownloadItem& item,
    const lt::torrent_status& status,
    PauseOwner pauseOwner,
    bool priorityPausedSeed,
    bool seedOnCompletionEnabled,
    bool hasReliableLiveState = true)
{
    return TaskStateMachine::resolve(taskStateInputForItemAndStatus(
        item,
        item,
        status,
        pauseOwner,
        priorityPausedSeed,
        seedOnCompletionEnabled,
        hasReliableLiveState));
}

inline QString statusTextForState(
    awa::core::DownloadState state,
    const awa::core::DownloadItem& item,
    bool priorityPausedSeed,
    bool seedOnCompletionEnabled)
{
    if (state == awa::core::DownloadState::Deleted) {
        return QStringLiteral("本地文件已删除");
    }

    if (state == awa::core::DownloadState::FetchingMetadata) {
        return QStringLiteral("获取元数据：%1，Peers %2，连接 %3，Tracker %4/%5")
            .arg(item.dhtStatusText)
            .arg(item.peerCount)
            .arg(item.connectionCount)
            .arg(item.workingTrackerCount)
            .arg(item.trackerCount);
    }

    if (state == awa::core::DownloadState::Downloading) {
        return QStringLiteral("下载中：Peers %1，Seeds %2，连接 %3，Tracker %4/%5")
            .arg(item.peerCount)
            .arg(item.seedCount)
            .arg(item.connectionCount)
            .arg(item.workingTrackerCount)
            .arg(item.trackerCount);
    }

    if (state == awa::core::DownloadState::Waiting) {
        return awa::core::stateToString(state);
    }

    if (priorityPausedSeed && !isPausedState(state)) {
        return seedOnCompletionEnabled
            ? QStringLiteral("下载优先，等待下载任务完成后继续做种")
            : awa::core::stateToString(awa::core::DownloadState::Finished);
    }

    return awa::core::stateToString(state);
}

inline awa::core::DownloadState stateFromInt(int state)
{
    switch (state) {
    case static_cast<int>(awa::core::DownloadState::Queued): return awa::core::DownloadState::Queued;
    case static_cast<int>(awa::core::DownloadState::FetchingMetadata): return awa::core::DownloadState::FetchingMetadata;
    case static_cast<int>(awa::core::DownloadState::Downloading): return awa::core::DownloadState::Downloading;
    case static_cast<int>(awa::core::DownloadState::PausedDownloading): return awa::core::DownloadState::PausedDownloading;
    case static_cast<int>(awa::core::DownloadState::Seeding): return awa::core::DownloadState::Seeding;
    case static_cast<int>(awa::core::DownloadState::Finished): return awa::core::DownloadState::Finished;
    case static_cast<int>(awa::core::DownloadState::Error): return awa::core::DownloadState::Error;
    case static_cast<int>(awa::core::DownloadState::PausedSeeding): return awa::core::DownloadState::PausedSeeding;
    case static_cast<int>(awa::core::DownloadState::Waiting): return awa::core::DownloadState::Waiting;
    case static_cast<int>(awa::core::DownloadState::Deleted): return awa::core::DownloadState::Deleted;
    default: return awa::core::DownloadState::Queued;
    }
}

inline QJsonObject itemToJson(const awa::core::DownloadItem& item)
{
    QJsonArray trackers;
    for (const auto& tracker : item.trackers) {
        trackers.append(tracker);
    }

    return {
        {QStringLiteral("id"), item.id},
        {QStringLiteral("name"), item.name},
        {QStringLiteral("source"), item.source},
        {QStringLiteral("savePath"), item.savePath},
        {QStringLiteral("state"), static_cast<int>(item.state)},
        {QStringLiteral("progress"), item.progress},
        {QStringLiteral("totalBytes"), QString::number(item.totalBytes)},
        {QStringLiteral("downloadedBytes"), QString::number(item.downloadedBytes)},
        {QStringLiteral("isComplete"), item.isComplete},
        {QStringLiteral("pieceCount"), item.pieceCount},
        {QStringLiteral("completedPieces"), item.completedPieces},
        {QStringLiteral("blockSize"), item.blockSize},
        {QStringLiteral("pieceMap"), item.pieceMap},
        {QStringLiteral("ratio"), item.ratio},
        {QStringLiteral("peerCount"), item.peerCount},
        {QStringLiteral("seedCount"), item.seedCount},
        {QStringLiteral("connectionCount"), item.connectionCount},
        {QStringLiteral("trackerCount"), item.trackerCount},
        {QStringLiteral("workingTrackerCount"), item.workingTrackerCount},
        {QStringLiteral("failedTrackerCount"), item.failedTrackerCount},
        {QStringLiteral("dhtStatusText"), item.dhtStatusText},
        {QStringLiteral("connectionHealthText"), item.connectionHealthText},
        {QStringLiteral("statusText"), item.statusText},
        {QStringLiteral("errorText"), item.errorText},
        {QStringLiteral("createdAt"), item.createdAt.toUTC().toString(Qt::ISODateWithMs)},
        {QStringLiteral("trackers"), trackers}
    };
}

inline awa::core::DownloadItem itemFromJson(const QJsonObject& object)
{
    awa::core::DownloadItem item;
    item.id = object.value(QStringLiteral("id")).toString();
    item.name = object.value(QStringLiteral("name")).toString();
    item.source = object.value(QStringLiteral("source")).toString();
    item.savePath = object.value(QStringLiteral("savePath")).toString();
    item.state = stateFromInt(object.value(QStringLiteral("state")).toInt());
    item.progress = jsonDouble(object, QStringLiteral("progress"));
    item.totalBytes = jsonInt64(object, QStringLiteral("totalBytes"));
    item.downloadedBytes = jsonInt64(object, QStringLiteral("downloadedBytes"));
    item.isComplete = object.contains(QStringLiteral("isComplete"))
        ? object.value(QStringLiteral("isComplete")).toBool()
        : isCompletedState(item.state);
    item.pieceCount = object.value(QStringLiteral("pieceCount")).toInt();
    item.completedPieces = object.value(QStringLiteral("completedPieces")).toInt();
    item.blockSize = object.value(QStringLiteral("blockSize")).toInt();
    item.pieceMap = object.value(QStringLiteral("pieceMap")).toString();
    item.ratio = object.value(QStringLiteral("ratio")).toDouble();
    item.peerCount = object.value(QStringLiteral("peerCount")).toInt();
    item.seedCount = object.value(QStringLiteral("seedCount")).toInt();
    item.connectionCount = object.value(QStringLiteral("connectionCount")).toInt();
    item.trackerCount = object.value(QStringLiteral("trackerCount")).toInt();
    item.workingTrackerCount = object.value(QStringLiteral("workingTrackerCount")).toInt();
    item.failedTrackerCount = object.value(QStringLiteral("failedTrackerCount")).toInt();
    item.dhtStatusText = object.value(QStringLiteral("dhtStatusText")).toString();
    item.connectionHealthText = object.value(QStringLiteral("connectionHealthText")).toString();
    item.statusText = object.value(QStringLiteral("statusText")).toString();
    item.errorText = object.value(QStringLiteral("errorText")).toString();
    item.createdAt = QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODateWithMs);
    for (const auto& tracker : object.value(QStringLiteral("trackers")).toArray()) {
        item.trackers.append(tracker.toString());
    }
    item.isComplete = item.isComplete || hasCompletedIntent(item) || hasCompleteTransferSnapshot(item);
    normalizeTransferSnapshot(item);
    return item;
}
} // namespace detail


} // namespace awa::torrent

