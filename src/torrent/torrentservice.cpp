#include "awa/torrent/torrentservice.h"

#include "awa/torrent/magnetutils.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInformation>
#include <QSaveFile>
#include <QStandardPaths>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/announce_entry.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/write_resume_data.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <map>
#include <vector>

namespace awa::torrent {
namespace lt = libtorrent;

namespace {
constexpr int DefaultRequestQueueTime = 6;
constexpr int DefaultMaxOutRequestQueue = 768;
constexpr int MinProbePieces = 4;
constexpr int MaxProbePieces = 64;
constexpr int ProbeBaselineSamples = 8;
constexpr int ProbeEvaluationSamples = 8;
constexpr int ProbeCooldownSamples = 5;
constexpr int ProbeMinGainPermille = 30;
constexpr int ProbeLossPermille = 30;

QString hashId(const lt::info_hash_t& hashes)
{
    const auto best = hashes.get_best();
    return QString::fromLatin1(QByteArray(best.data(), best.size()).toHex());
}

QString pieceMap(
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

bool hasMeaningfulTransferSnapshot(const lt::torrent_status& status)
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

qint64 jsonInt64(const QJsonObject& object, const QString& key)
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

double jsonDouble(const QJsonObject& object, const QString& key)
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

bool pieceMapHasProgress(const QString& map)
{
    return map.contains(QLatin1Char('1')) || map.contains(QLatin1Char('2'));
}

int completedPieceCount(const lt::typed_bitfield<lt::piece_index_t>& pieces)
{
    int count = 0;
    for (int piece = 0; piece < pieces.size(); ++piece) {
        if (pieces.get_bit(lt::piece_index_t(piece))) {
            ++count;
        }
    }
    return count;
}

qint64 completedBytes(const lt::torrent_info& info, const lt::typed_bitfield<lt::piece_index_t>& pieces)
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

QString resumePieceMap(
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

const lt::typed_bitfield<lt::piece_index_t>* bestCompletedResumePieces(const lt::add_torrent_params& params)
{
    if (!params.verified_pieces.empty() && completedPieceCount(params.verified_pieces) > 0) {
        return &params.verified_pieces;
    }
    if (!params.have_pieces.empty() && completedPieceCount(params.have_pieces) > 0) {
        return &params.have_pieces;
    }
    return nullptr;
}

bool hasTransferSnapshot(const awa::core::DownloadItem& item)
{
    return item.totalBytes > 0
        || item.progress > 0.0
        || item.downloadedBytes > 0
        || item.completedPieces > 0
        || item.pieceCount > 0
        || pieceMapHasProgress(item.pieceMap);
}

bool hasProgressSnapshot(const awa::core::DownloadItem& item)
{
    return item.progress > 0.0
        || item.downloadedBytes > 0
        || item.completedPieces > 0
        || pieceMapHasProgress(item.pieceMap);
}

void fillMissingTransferSnapshot(awa::core::DownloadItem& item, const awa::core::DownloadItem& fallback)
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
}

void applyResumeTransferSnapshot(awa::core::DownloadItem& item, const lt::add_torrent_params& params)
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
    resume.progress = resume.totalBytes > 0
        ? static_cast<double>(resume.downloadedBytes) / static_cast<double>(resume.totalBytes)
        : 0.0;
    resume.pieceMap = resumePieceMap(resume.pieceCount, pieces, params.unfinished_pieces);
    fillMissingTransferSnapshot(item, resume);
}

bool liveSnapshotWouldResetProgress(const awa::core::DownloadItem& stored, const lt::torrent_status& status)
{
    return hasProgressSnapshot(stored)
        && status.total_wanted_done <= 0
        && status.progress_ppm <= 0
        && status.num_pieces <= 0
        && (!status.pieces.empty() || status.total_wanted > 0 || status.block_size > 0);
}

bool liveSnapshotWouldResetPieceMap(const awa::core::DownloadItem& stored, const lt::torrent_status& status)
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

bool isPausedState(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::PausedDownloading
        || state == awa::core::DownloadState::PausedSeeding;
}

bool isCompletedState(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::Finished
        || state == awa::core::DownloadState::Seeding
        || state == awa::core::DownloadState::PausedSeeding;
}

bool shouldPauseAsSeeding(awa::core::DownloadState state)
{
    return state == awa::core::DownloadState::Seeding
        || state == awa::core::DownloadState::Finished
        || state == awa::core::DownloadState::PausedSeeding;
}

awa::core::DownloadState stateFromInt(int state)
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
    default: return awa::core::DownloadState::Queued;
    }
}

QJsonObject itemToJson(const awa::core::DownloadItem& item)
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

awa::core::DownloadItem itemFromJson(const QJsonObject& object)
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
    return item;
}
} // namespace

TorrentService::TorrentService(QObject* parent)
    : TorrentBackend(parent)
{
    lt::settings_pack settings;
    settings.set_bool(lt::settings_pack::enable_dht, true);
    settings.set_bool(lt::settings_pack::enable_lsd, true);
    settings.set_bool(lt::settings_pack::enable_upnp, true);
    settings.set_bool(lt::settings_pack::enable_natpmp, true);
    settings.set_bool(lt::settings_pack::announce_to_all_tiers, true);
    settings.set_bool(lt::settings_pack::announce_to_all_trackers, true);
    settings.set_bool(lt::settings_pack::allow_multiple_connections_per_ip, true);
    settings.set_bool(lt::settings_pack::enable_outgoing_utp, true);
    settings.set_bool(lt::settings_pack::enable_incoming_utp, true);
    settings.set_bool(lt::settings_pack::auto_sequential, false);
    settings.set_bool(lt::settings_pack::piece_extent_affinity, false);
    settings.set_bool(lt::settings_pack::prioritize_partial_pieces, false);
    settings.set_bool(lt::settings_pack::prefer_rc4, true);
    settings.set_str(lt::settings_pack::user_agent, "AwaKurageDownloader/0.1.1");
    settings.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:6881-6891,[::]:6881-6891");
    settings.set_str(lt::settings_pack::dht_bootstrap_nodes,
        "router.bittorrent.com:6881,dht.transmissionbt.com:6881,router.utorrent.com:6881,"
        "dht.aelitis.com:6881,dht.libtorrent.org:25401");
    settings.set_int(lt::settings_pack::out_enc_policy, lt::settings_pack::pe_enabled);
    settings.set_int(lt::settings_pack::in_enc_policy, lt::settings_pack::pe_enabled);
    settings.set_int(lt::settings_pack::allowed_enc_level, lt::settings_pack::pe_both);
    settings.set_int(lt::settings_pack::connections_limit, 500);
    settings.set_int(lt::settings_pack::active_downloads, 20);
    settings.set_int(lt::settings_pack::active_limit, 200);
    settings.set_int(lt::settings_pack::choking_algorithm, lt::settings_pack::fixed_slots_choker);
    settings.set_int(lt::settings_pack::seed_choking_algorithm, lt::settings_pack::anti_leech);
    settings.set_int(lt::settings_pack::unchoke_slots_limit, 8);
    settings.set_int(lt::settings_pack::num_optimistic_unchoke_slots, 1);
    settings.set_int(lt::settings_pack::connection_speed, 100);
    settings.set_int(lt::settings_pack::request_queue_time, DefaultRequestQueueTime);
    settings.set_int(lt::settings_pack::max_out_request_queue, DefaultMaxOutRequestQueue);
    settings.set_int(lt::settings_pack::whole_pieces_threshold, 0);
    settings.set_int(lt::settings_pack::initial_picker_threshold, 0);
    settings.set_int(lt::settings_pack::aio_threads, 8);
    settings.set_int(lt::settings_pack::checking_mem_usage, 128);
    settings.set_int(lt::settings_pack::alert_mask,
        lt::alert::error_notification |
        lt::alert::status_notification |
        lt::alert::storage_notification |
        lt::alert::tracker_notification |
        lt::alert::ip_block_notification |
        lt::alert::peer_notification);

    m_session = std::make_unique<lt::session>(settings);

    m_networkRestartTimer.setSingleShot(true);
    m_networkRestartTimer.setInterval(2000);

    connect(&m_alertTimer, &QTimer::timeout, this, &TorrentService::pollAlerts);
    connect(&m_statusTimer, &QTimer::timeout, this, &TorrentService::refreshStatuses);
    connect(&m_resumeSaveTimer, &QTimer::timeout, this, &TorrentService::requestResumeDataForAll);
    connect(&m_networkRestartTimer, &QTimer::timeout, this, &TorrentService::restartDownloadsAfterNetworkChange);
    m_alertTimer.start(250);
    m_statusTimer.start(1000);
    m_resumeSaveTimer.start(30000);

    if (QNetworkInformation::loadDefaultBackend()) {
        if (auto* networkInformation = QNetworkInformation::instance()) {
            connect(networkInformation, &QNetworkInformation::reachabilityChanged, this, [this] {
                m_networkRestartTimer.start();
            });
            connect(networkInformation, &QNetworkInformation::transportMediumChanged, this, [this] {
                m_networkRestartTimer.start();
            });
            connect(networkInformation, &QNetworkInformation::isBehindCaptivePortalChanged, this, [this] {
                m_networkRestartTimer.start();
            });
        }
    } else {
        spdlog::warn("No Qt network information backend is available; network-change restart is disabled");
    }
}

TorrentService::~TorrentService()
{
    int outstanding = 0;
    for (auto it = m_handles.cbegin(); it != m_handles.cend(); ++it) {
        if (it.value().is_valid()) {
            requestResumeData(it.key(), true);
            ++outstanding;
        }
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (outstanding > 0 && std::chrono::steady_clock::now() < deadline) {
        if (m_session->wait_for_alert(std::chrono::milliseconds(500)) == nullptr) {
            continue;
        }
        std::vector<lt::alert*> alerts;
        m_session->pop_alerts(&alerts);
        for (const auto* alert : alerts) {
            if (const auto* resumeData = lt::alert_cast<lt::save_resume_data_alert>(alert)) {
                writeResumeData(handleId(resumeData->handle), resumeData->params);
                --outstanding;
            } else if (lt::alert_cast<lt::save_resume_data_failed_alert>(alert)) {
                --outstanding;
            }
        }
    }
    persistItems();
}

void TorrentService::addTorrentFile(const QString& path, const awa::core::DownloadOptions& options)
{
    QDir().mkpath(options.savePath);

    lt::error_code ec;
    auto info = std::make_shared<lt::torrent_info>(path.toStdString(), ec);
    if (ec) {
        emit errorRaised(QStringLiteral("无法读取种子：%1").arg(QString::fromStdString(ec.message())));
        return;
    }

    lt::add_torrent_params params;
    params.ti = std::move(info);
    params.save_path = options.savePath.toStdString();
    params.flags &= ~lt::torrent_flags::auto_managed;
    params.flags &= ~lt::torrent_flags::sequential_download;
    applyTrackers(params);
    if (options.startPaused) {
        params.flags |= lt::torrent_flags::paused;
    }

    auto handle = m_session->add_torrent(std::move(params), ec);
    if (ec) {
        emit errorRaised(QStringLiteral("添加种子失败：%1").arg(QString::fromStdString(ec.message())));
        return;
    }
    m_removedIds.remove(handleId(handle));
    rememberHandle(handle, awa::core::DownloadState::Queued);
    auto item = m_items.value(handleId(handle));
    if (options.startPaused) {
        m_userPausedIds.insert(item.id);
    } else {
        m_userPausedIds.remove(item.id);
    }
    item.source = path;
    item.savePath = options.savePath;
    if (options.startPaused) {
        item.state = shouldPauseAsSeeding(item.state)
            ? awa::core::DownloadState::PausedSeeding
            : awa::core::DownloadState::PausedDownloading;
        item.downloadRate = 0;
        item.uploadRate = 0;
        item.statusText = awa::core::stateToString(item.state);
    }
    m_items.insert(item.id, item);
    emit itemUpdated(item);
    persistItems();
    if (!options.startPaused) {
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();
    }
    updateDownloadQueue();
}

void TorrentService::addMagnet(const QString& uri, const awa::core::DownloadOptions& options)
{
    QDir().mkpath(options.savePath);

    const QString normalizedUri = normalizeMagnetUri(uri.trimmed());
    lt::error_code ec;
    auto params = lt::parse_magnet_uri(normalizedUri.toStdString(), ec);
    if (ec) {
        emit errorRaised(QStringLiteral("磁力链接无效：%1").arg(QString::fromStdString(ec.message())));
        return;
    }

    params.save_path = options.savePath.toStdString();
    params.flags &= ~lt::torrent_flags::duplicate_is_error;
    params.flags &= ~lt::torrent_flags::auto_managed;
    params.flags &= ~lt::torrent_flags::sequential_download;
    params.dht_nodes.emplace_back("router.bittorrent.com", 6881);
    params.dht_nodes.emplace_back("dht.transmissionbt.com", 6881);
    params.dht_nodes.emplace_back("router.utorrent.com", 6881);
    params.dht_nodes.emplace_back("dht.aelitis.com", 6881);
    params.dht_nodes.emplace_back("dht.libtorrent.org", 25401);
    applyTrackers(params);
    const auto infoHashes = params.info_hashes;
    const QString normalizedId = hashId(infoHashes);
    m_removedIds.remove(normalizedId);
    if (params.name.empty() && !normalizedId.isEmpty()) {
        params.name = QStringLiteral("Magnet %1").arg(normalizedId.left(12)).toStdString();
    }
    if (options.startPaused) {
        params.flags |= lt::torrent_flags::paused;
    }

    awa::core::DownloadItem pending;
    pending.id = normalizedId;
    pending.name = QStringLiteral("Magnet %1").arg(pending.id.left(12));
    pending.source = normalizedUri;
    pending.savePath = options.savePath;
    pending.state = awa::core::DownloadState::FetchingMetadata;
    pending.statusText = QStringLiteral("正在连接 DHT/Tracker 获取元数据");
    pending.createdAt = QDateTime::currentDateTimeUtc();
    m_items.insert(pending.id, pending);
    emit itemUpdated(pending);
    persistItems();

    auto handle = m_session->add_torrent(std::move(params), ec);
    if (ec) {
        pending.state = awa::core::DownloadState::Error;
        pending.errorText = QString::fromStdString(ec.message());
        pending.statusText = QStringLiteral("添加失败：%1").arg(pending.errorText);
        m_items.insert(pending.id, pending);
        emit itemUpdated(pending);
        emit errorRaised(QStringLiteral("添加磁力失败：%1").arg(QString::fromStdString(ec.message())));
        return;
    }

    rememberHandle(handle, awa::core::DownloadState::FetchingMetadata);
    auto item = m_items.value(handleId(handle));
    if (options.startPaused) {
        m_userPausedIds.insert(item.id);
    } else {
        m_userPausedIds.remove(item.id);
    }
    item.source = normalizedUri;
    item.savePath = options.savePath;
    if (options.startPaused) {
        item.state = shouldPauseAsSeeding(item.state)
            ? awa::core::DownloadState::PausedSeeding
            : awa::core::DownloadState::PausedDownloading;
        item.downloadRate = 0;
        item.uploadRate = 0;
        item.statusText = awa::core::stateToString(item.state);
    }
    m_items.insert(item.id, item);
    emit itemUpdated(item);
    persistItems();
    if (!options.startPaused) {
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();
        if (infoHashes.has_v1()) {
            m_session->dht_get_peers(infoHashes.v1);
        }
    }
    updateDownloadQueue();
}

void TorrentService::pause(const QString& id)
{
    if (m_handles.contains(id)) {
        auto handle = m_handles.value(id);
        const auto previousItem = itemFromHandle(handle);
        m_userPausedIds.insert(id);
        m_priorityPausedSeeds.remove(id);
        handle.unset_flags(lt::torrent_flags::auto_managed);
        handle.set_flags(lt::torrent_flags::paused);
        handle.pause();

        auto item = previousItem;
        if (item.id.isEmpty()) {
            item.id = id;
        }
        item.state = shouldPauseAsSeeding(previousItem.state)
            ? awa::core::DownloadState::PausedSeeding
            : awa::core::DownloadState::PausedDownloading;
        item.downloadRate = 0;
        item.uploadRate = 0;
        item.statusText = awa::core::stateToString(item.state);
        m_items.insert(id, item);
        m_schedulerPausedIds.remove(id);
        emit itemUpdated(item);
        persistItems();
        requestResumeData(id, true);
    }
}

void TorrentService::resume(const QString& id)
{
    if (m_handles.contains(id)) {
        auto handle = m_handles.value(id);
        m_userPausedIds.remove(id);
        m_priorityPausedSeeds.remove(id);
        handle.unset_flags(lt::torrent_flags::auto_managed | lt::torrent_flags::paused | lt::torrent_flags::sequential_download);
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();

        auto item = itemFromHandle(handle);
        const auto previousState = m_items.value(id).state;
        if (previousState == awa::core::DownloadState::PausedSeeding) {
            item.state = awa::core::DownloadState::Seeding;
        } else if (previousState == awa::core::DownloadState::PausedDownloading) {
            item.state = item.totalBytes > 0
                ? awa::core::DownloadState::Downloading
                : awa::core::DownloadState::FetchingMetadata;
        }
        item.statusText = QStringLiteral("正在恢复连接...");
        m_items.insert(id, item);
        m_schedulerPausedIds.remove(id);
        emit itemUpdated(item);
        persistItems();
        requestResumeData(id, true);
        updateDownloadQueue();
    }
}

void TorrentService::remove(const QString& id, bool removeFiles)
{
    if (id.isEmpty()) {
        return;
    }

    m_removedIds.insert(id);
    if (m_handles.contains(id)) {
        if (removeFiles) {
            m_session->remove_torrent(m_handles.take(id), lt::session::delete_files);
        } else {
            m_session->remove_torrent(m_handles.take(id));
        }
    }
    m_items.remove(id);
    m_lastMetadataRetry.remove(id);
    m_lastPeerDiscovery.remove(id);
    m_metadataRetryCounts.remove(id);
    m_userPausedIds.remove(id);
    m_schedulerPausedIds.remove(id);
    m_pieceProbes.remove(id);
    QFile::remove(resumeDataPath(id));
    persistItems();
    emit itemRemoved(id);
}

void TorrentService::setFilePriorities(const QString& id, const QVector<awa::core::FilePriority>& priorities)
{
    if (!m_handles.contains(id)) {
        return;
    }

    auto handle = m_handles[id];
    for (const auto& item : priorities) {
        handle.file_priority(lt::file_index_t(item.index), static_cast<lt::download_priority_t>(item.priority));
    }
}

void TorrentService::setSpeedLimits(int downloadKiB, int uploadKiB)
{
    lt::settings_pack settings;
    settings.set_int(lt::settings_pack::download_rate_limit, std::max(0, downloadKiB) * 1024);
    settings.set_int(lt::settings_pack::upload_rate_limit, std::max(0, uploadKiB) * 1024);
    m_session->apply_settings(settings);
}

void TorrentService::setChokingStrategy(int chokingAlgorithm, int seedChokingAlgorithm, int uploadSlots, int optimisticSlots)
{
    lt::settings_pack settings;
    const int choker = chokingAlgorithm == 0
        ? lt::settings_pack::fixed_slots_choker
        : lt::settings_pack::rate_based_choker;
    const int seedChoker = seedChokingAlgorithm == 0
        ? lt::settings_pack::round_robin
        : seedChokingAlgorithm == 1
            ? lt::settings_pack::fastest_upload
            : lt::settings_pack::anti_leech;

    settings.set_int(lt::settings_pack::choking_algorithm, choker);
    settings.set_int(lt::settings_pack::seed_choking_algorithm, seedChoker);
    settings.set_int(lt::settings_pack::unchoke_slots_limit, std::clamp(uploadSlots, 1, 200));
    settings.set_int(lt::settings_pack::num_optimistic_unchoke_slots, std::clamp(optimisticSlots, 0, 10));
    m_session->apply_settings(settings);
}

void TorrentService::setMaxActiveDownloads(int count)
{
    m_maxActiveDownloads = std::clamp(count, 1, 200);

    lt::settings_pack settings;
    settings.set_int(lt::settings_pack::active_downloads, m_maxActiveDownloads);
    settings.set_int(lt::settings_pack::active_limit, std::max(200, m_maxActiveDownloads));
    m_session->apply_settings(settings);

    updateDownloadQueue();
    persistItems();
}

void TorrentService::setDynamicBlockTuningEnabled(bool enabled)
{
    if (m_dynamicBlockTuningEnabled == enabled) {
        return;
    }

    m_dynamicBlockTuningEnabled = enabled;
    updateDynamicBlockTuning();
}

void TorrentService::setSeedOnCompletionEnabled(bool enabled)
{
    if (m_seedOnCompletionEnabled == enabled) {
        return;
    }

    m_seedOnCompletionEnabled = enabled;
    updateDownloadQueue();
    updateSeedingPriority();
    persistItems();
}

void TorrentService::setTrackers(const QStringList& trackers)
{
    m_defaultTrackers.clear();
    for (const auto& tracker : trackers) {
        const auto trimmed = tracker.trimmed();
        if (!trimmed.isEmpty() && !m_defaultTrackers.contains(trimmed, Qt::CaseInsensitive)) {
            m_defaultTrackers.append(trimmed);
        }
    }
    std::sort(m_defaultTrackers.begin(), m_defaultTrackers.end(), [this](const QString& left, const QString& right) {
        return trackerHealthScore(left) > trackerHealthScore(right);
    });

    for (const auto& handle : m_handles) {
        applyTrackersToHandle(handle);
    }
    if (m_persistedTasksLoaded && !m_handles.isEmpty()) {
        requestResumeDataForAll();
    }
}

void TorrentService::loadPersistedTasks()
{
    if (m_persistedTasksLoaded) {
        return;
    }
    m_persistedTasksLoaded = true;

    QJsonArray downloads;
    QFile file(indexFilePath());
    if (file.open(QIODevice::ReadOnly)) {
        const auto document = QJsonDocument::fromJson(file.readAll());
        downloads = document.object().value(QStringLiteral("downloads")).toArray();
    }
    if (downloads.isEmpty()) {
        const auto resumeFiles = QDir(persistenceDir()).entryInfoList(
            {QStringLiteral("*.fastresume")},
            QDir::Files | QDir::Readable,
            QDir::Name);
        for (const auto& resumeFile : resumeFiles) {
            QJsonObject recovered;
            recovered.insert(QStringLiteral("id"), resumeFile.completeBaseName());
            recovered.insert(QStringLiteral("state"), static_cast<int>(awa::core::DownloadState::Downloading));
            downloads.append(recovered);
        }
    }
    for (const auto& value : downloads) {
        const auto object = value.toObject();
        auto stored = itemFromJson(object);
        if (stored.id.isEmpty()) {
            continue;
        }
        lt::error_code ec;
        lt::add_torrent_params params;
        QFile resumeFile(resumeDataPath(stored.id));
        if (resumeFile.open(QIODevice::ReadOnly)) {
            const QByteArray data = resumeFile.readAll();
            params = lt::read_resume_data(lt::span<char const>(data.constData(), data.size()), ec);
        }
        const bool resumeDataPaused = !ec
            && (params.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {};
        const bool schedulerPaused = object.value(QStringLiteral("schedulerPaused")).toBool()
            || stored.state == awa::core::DownloadState::Waiting;
        const bool userPaused = object.value(QStringLiteral("userPaused")).toBool()
            || isPausedState(stored.state)
            || (resumeDataPaused && !schedulerPaused);
        applyResumeTransferSnapshot(stored, params);

        auto restoreSchedulerPause = [&]() {
            if (!schedulerPaused || userPaused) {
                return;
            }

            m_userPausedIds.remove(stored.id);
            m_schedulerPausedIds.insert(stored.id);
            if (m_handles.contains(stored.id)) {
                auto handle = m_handles.value(stored.id);
                handle.set_flags(lt::torrent_flags::paused);
                handle.pause();
            }

            auto item = m_items.value(stored.id);
            if (item.id.isEmpty()) {
                return;
            }
            item.state = awa::core::DownloadState::Waiting;
            item.statusText = QStringLiteral("等待可用下载槽");
            item.downloadRate = 0;
            item.uploadRate = 0;
            m_items.insert(stored.id, item);
            emit itemUpdated(item);
        };

        if (ec || (!params.ti && params.info_hashes == lt::info_hash_t {})) {
            if (stored.source.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
                awa::core::DownloadOptions options;
                options.savePath = stored.savePath;
                options.startPaused = userPaused || schedulerPaused;
                addMagnet(stored.source, options);
                restoreSchedulerPause();
            } else if (QFileInfo::exists(stored.source)) {
                awa::core::DownloadOptions options;
                options.savePath = stored.savePath;
                options.startPaused = userPaused || schedulerPaused;
                addTorrentFile(stored.source, options);
                restoreSchedulerPause();
            } else {
                stored.state = awa::core::DownloadState::Error;
                stored.errorText = QStringLiteral("任务来源不可用，无法恢复");
                stored.statusText = stored.errorText;
                m_items.insert(stored.id, stored);
                emit itemUpdated(stored);
            }
            continue;
        }

        if (!stored.savePath.isEmpty()) {
            params.save_path = stored.savePath.toStdString();
        }
        const bool hasCompletePieces = !params.have_pieces.empty() && params.have_pieces.all_set();
        const bool hasVerifiedPieces = !params.verified_pieces.empty() && params.verified_pieces.all_set();
        const bool pieceCountMatches = params.ti
            && !params.have_pieces.empty()
            && params.have_pieces.count() == params.ti->num_pieces();
        const bool resumeSuggestsCompleted = hasCompletePieces
            || hasVerifiedPieces
            || pieceCountMatches
            || (params.finished_time > 0 && params.total_downloaded > 0 && params.unfinished_pieces.empty());
        const bool restoreAsCompleted = resumeSuggestsCompleted
            && (stored.state == awa::core::DownloadState::PausedDownloading || isCompletedState(stored.state));
        applyTrackers(params);
        params.flags &= ~lt::torrent_flags::auto_managed;
        params.flags &= ~lt::torrent_flags::sequential_download;
        const bool shouldRestorePaused = userPaused
            || schedulerPaused
            || stored.state == awa::core::DownloadState::Finished
            || (restoreAsCompleted && !m_seedOnCompletionEnabled);
        if (shouldRestorePaused) {
            params.flags |= lt::torrent_flags::paused;
        } else {
            params.flags &= ~lt::torrent_flags::paused;
        }

        auto handle = m_session->add_torrent(std::move(params), ec);
        if (ec) {
            stored.state = awa::core::DownloadState::Error;
            stored.errorText = QString::fromStdString(ec.message());
            stored.statusText = QStringLiteral("恢复任务失败：%1").arg(stored.errorText);
            m_items.insert(stored.id, stored);
            emit itemUpdated(stored);
            continue;
        }

        if (userPaused) {
            m_userPausedIds.insert(stored.id);
        } else {
            m_userPausedIds.remove(stored.id);
        }
        if (schedulerPaused) {
            m_schedulerPausedIds.insert(stored.id);
        } else {
            m_schedulerPausedIds.remove(stored.id);
        }
        m_items.insert(stored.id, stored);

        rememberHandle(handle, stored.state, false);
        auto restored = m_items.value(handleId(handle));
        fillMissingTransferSnapshot(restored, stored);
        restored.source = stored.source;
        restored.createdAt = stored.createdAt.isNull() ? restored.createdAt : stored.createdAt;
        if (restored.name.isEmpty()) {
            restored.name = stored.name;
        }
        if (userPaused) {
            restored.state = restoreAsCompleted
                || shouldPauseAsSeeding(stored.state)
                ? awa::core::DownloadState::PausedSeeding
                : awa::core::DownloadState::PausedDownloading;
            restored.statusText = awa::core::stateToString(restored.state);
            restored.downloadRate = 0;
            restored.uploadRate = 0;
        } else if (schedulerPaused) {
            restored.state = awa::core::DownloadState::Waiting;
            restored.statusText = QStringLiteral("等待可用下载槽");
            restored.downloadRate = 0;
            restored.uploadRate = 0;
        } else if (restoreAsCompleted || stored.state == awa::core::DownloadState::Finished) {
            restored.state = m_seedOnCompletionEnabled
                ? awa::core::DownloadState::Seeding
                : awa::core::DownloadState::Finished;
            restored.statusText = restored.state == awa::core::DownloadState::Seeding
                ? QStringLiteral("继续做种中")
                : QStringLiteral("下载完成");
            if (restored.state == awa::core::DownloadState::Finished) {
                restored.downloadRate = 0;
                restored.uploadRate = 0;
            }
        } else if (stored.state == awa::core::DownloadState::Seeding) {
            restored.state = awa::core::DownloadState::Seeding;
            restored.statusText = QStringLiteral("继续做种中");
        } else {
            restored.statusText = QStringLiteral("已恢复任务");
        }
        m_items.insert(restored.id, restored);
        emit itemUpdated(restored);
        if (!shouldRestorePaused) {
            handle.resume();
            handle.force_reannounce();
            handle.force_dht_announce();
            handle.force_lsd_announce();
        }
    }
    updateDownloadQueue();
    updateSeedingPriority();
    persistItems();
}

void TorrentService::pollAlerts()
{
    std::vector<lt::alert*> alerts;
    m_session->pop_alerts(&alerts);

    for (const auto* alert : alerts) {
        if (const auto* added = lt::alert_cast<lt::add_torrent_alert>(alert)) {
            if (shouldIgnoreId(handleId(added->handle))) {
                continue;
            }
            rememberHandle(added->handle, awa::core::DownloadState::Queued);
            updateSeedingPriority();
            persistItems();
        } else if (const auto* metadata = lt::alert_cast<lt::metadata_received_alert>(alert)) {
            if (shouldIgnoreId(handleId(metadata->handle))) {
                continue;
            }
            m_lastMetadataRetry.remove(handleId(metadata->handle));
            m_lastPeerDiscovery.remove(handleId(metadata->handle));
            m_metadataRetryCounts.remove(handleId(metadata->handle));
            rememberHandle(metadata->handle, awa::core::DownloadState::Downloading);
            updateSeedingPriority();
            requestResumeData(handleId(metadata->handle), true);
            persistItems();
        } else if (const auto* finished = lt::alert_cast<lt::torrent_finished_alert>(alert)) {
            const QString id = handleId(finished->handle);
            if (shouldIgnoreId(id)) {
                continue;
            }
            updateSeedingPriority();
            auto item = itemFromHandle(finished->handle);
            m_items.insert(id, item);
            emit itemUpdated(item);
            requestResumeData(id, true);
            persistItems();
        } else if (const auto* resumeData = lt::alert_cast<lt::save_resume_data_alert>(alert)) {
            if (shouldIgnoreId(handleId(resumeData->handle))) {
                continue;
            }
            writeResumeData(handleId(resumeData->handle), resumeData->params);
        } else if (const auto* reply = lt::alert_cast<lt::tracker_reply_alert>(alert)) {
            const QString id = handleId(reply->handle);
            if (shouldIgnoreId(id)) {
                continue;
            }
            recordTrackerResult(QString::fromUtf8(reply->tracker_url()), true, reply->num_peers);
            auto item = itemFromHandle(reply->handle);
            item.statusText = QStringLiteral("Tracker 已响应，发现 %1 个 peer").arg(reply->num_peers);
            m_items.insert(id, item);
            emit itemUpdated(item);
        } else if (const auto* warning = lt::alert_cast<lt::tracker_warning_alert>(alert)) {
            const QString id = handleId(warning->handle);
            if (shouldIgnoreId(id)) {
                continue;
            }
            recordTrackerResult(QString::fromUtf8(warning->tracker_url()), true);
            auto item = itemFromHandle(warning->handle);
            item.statusText = QStringLiteral("Tracker 警告：%1").arg(QString::fromStdString(warning->message()));
            m_items.insert(id, item);
            emit itemUpdated(item);
        } else if (const auto* trackerError = lt::alert_cast<lt::tracker_error_alert>(alert)) {
            const QString id = handleId(trackerError->handle);
            if (shouldIgnoreId(id)) {
                continue;
            }
            recordTrackerResult(QString::fromUtf8(trackerError->tracker_url()), false);
            auto item = itemFromHandle(trackerError->handle);
            item.statusText = QStringLiteral("Tracker 暂无响应，继续通过 DHT 查找 peer");
            m_items.insert(id, item);
            emit itemUpdated(item);
        } else if (const auto* removed = lt::alert_cast<lt::torrent_removed_alert>(alert)) {
            const QString id = hashId(removed->info_hashes);
            if (id.isEmpty()) {
                continue;
            }
            m_handles.remove(id);
            m_items.remove(id);
            m_userPausedIds.remove(id);
            m_priorityPausedSeeds.remove(id);
            m_lastMetadataRetry.remove(id);
            m_lastPeerDiscovery.remove(id);
            m_metadataRetryCounts.remove(id);
            QFile::remove(resumeDataPath(id));
            updateSeedingPriority();
            persistItems();
            emit itemRemoved(id);
        } else if (const auto* error = lt::alert_cast<lt::torrent_error_alert>(alert)) {
            const QString id = handleId(error->handle);
            if (shouldIgnoreId(id)) {
                continue;
            }
            auto item = m_items.value(id);
            item.id = id;
            item.state = awa::core::DownloadState::Error;
            item.errorText = QString::fromStdString(error->error.message());
            m_items.insert(id, item);
            emit itemUpdated(item);
        }
    }
}

void TorrentService::refreshStatuses()
{
    updateDownloadQueue();
    updateSeedingPriority();
    updateDynamicBlockTuning();
    const auto now = QDateTime::currentDateTimeUtc();
    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        if (!it.value().is_valid()) {
            continue;
        }
        const QString id = handleId(it.value());
        if (shouldIgnoreId(id)) {
            continue;
        }
        const auto status = it.value().status(
            lt::torrent_handle::query_save_path
            | lt::torrent_handle::query_name
            | lt::torrent_handle::query_pieces);
        std::vector<lt::partial_piece_info> downloadingPieces;
        it.value().get_download_queue(downloadingPieces);
        updateProbePieceSelection(id, it.value(), status, downloadingPieces);
        auto item = itemFromHandle(it.value());
        if (item.state == awa::core::DownloadState::FetchingMetadata) {
            const auto lastRetry = m_lastMetadataRetry.value(id);
            if (!lastRetry.isValid() || lastRetry.msecsTo(now) >= 15000) {
                auto handle = it.value();
                handle.force_reannounce();
                handle.force_dht_announce();
                handle.force_lsd_announce();
                const auto hashes = handle.info_hashes();
                if (hashes.has_v1()) {
                    m_session->dht_get_peers(hashes.v1);
                }
                const int attempts = m_metadataRetryCounts.value(id, 0) + 1;
                m_metadataRetryCounts.insert(id, attempts);
                m_lastMetadataRetry.insert(id, now);
                item.statusText = QStringLiteral("正在重新获取元数据，第 %1 次重试").arg(attempts);
            }
        } else if (item.state == awa::core::DownloadState::Downloading
            && item.peerCount <= 0
            && item.connectionCount <= 0) {
            const auto lastDiscovery = m_lastPeerDiscovery.value(id);
            if (!lastDiscovery.isValid() || lastDiscovery.msecsTo(now) >= 30000) {
                auto handle = it.value();
                applyTrackersToHandle(handle);
                handle.force_reannounce();
                handle.force_dht_announce();
                handle.force_lsd_announce();
                const auto hashes = handle.info_hashes();
                if (hashes.has_v1()) {
                    m_session->dht_get_peers(hashes.v1);
                }
                m_lastPeerDiscovery.insert(id, now);
                item.statusText = QStringLiteral("暂无 peer，正在重新向 Tracker/DHT 查找连接");
                item.connectionHealthText = QStringLiteral("正在重新发现 peer");
            }
        }
        m_items.insert(item.id, item);
        emit itemUpdated(item);
    }
    persistItems();
}

QString TorrentService::handleId(const lt::torrent_handle& handle) const
{
    if (!handle.is_valid()) {
        return {};
    }
    return hashId(handle.info_hashes());
}

bool TorrentService::shouldIgnoreId(const QString& id) const
{
    return id.isEmpty() || m_removedIds.contains(id);
}

int TorrentService::trackerHealthScore(const QString& tracker) const
{
    const auto health = m_trackerHealth.value(tracker);
    int score = 50;
    score += health.successes * 8;
    score += std::min(health.peers, 50);
    score -= health.failures * 12;

    if (health.lastSuccess.isValid()) {
        const qint64 ageMinutes = health.lastSuccess.secsTo(QDateTime::currentDateTimeUtc()) / 60;
        score -= static_cast<int>(std::min<qint64>(ageMinutes / 30, 12));
    }
    if (health.lastFailure.isValid() && health.lastFailure.secsTo(QDateTime::currentDateTimeUtc()) < 300) {
        score -= 10;
    }
    return std::clamp(score, 0, 100);
}

void TorrentService::recordTrackerResult(const QString& tracker, bool success, int peers)
{
    if (tracker.isEmpty()) {
        return;
    }

    auto health = m_trackerHealth.value(tracker);
    if (success) {
        health.successes = std::min(health.successes + 1, 1000);
        health.failures = std::max(0, health.failures - 1);
        health.peers = std::max(health.peers, peers);
        health.lastSuccess = QDateTime::currentDateTimeUtc();
    } else {
        health.failures = std::min(health.failures + 1, 1000);
        health.lastFailure = QDateTime::currentDateTimeUtc();
    }
    m_trackerHealth.insert(tracker, health);
    reprioritizeTrackers();
}

void TorrentService::reprioritizeTrackers()
{
    std::sort(m_defaultTrackers.begin(), m_defaultTrackers.end(), [this](const QString& left, const QString& right) {
        return trackerHealthScore(left) > trackerHealthScore(right);
    });
}

bool TorrentService::hasActiveDownloads() const
{
    for (auto it = m_handles.cbegin(); it != m_handles.cend(); ++it) {
        if (!it.value().is_valid() || shouldIgnoreId(it.key())) {
            continue;
        }

        const auto status = it.value().status();
        if ((status.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {}
            && !m_priorityPausedSeeds.contains(it.key())) {
            continue;
        }
        if (!status.has_metadata || status.progress_ppm < 1000000 || !status.is_seeding) {
            return true;
        }
    }
    return false;
}

void TorrentService::updateDynamicBlockTuning()
{
    int nextRequestQueueTime = DefaultRequestQueueTime;
    int nextMaxOutRequestQueue = DefaultMaxOutRequestQueue;

    if (m_dynamicBlockTuningEnabled) {
        qint64 aggregateDownloadRate = 0;
        int activeDownloads = 0;
        int activeConnections = 0;

        for (auto it = m_handles.cbegin(); it != m_handles.cend(); ++it) {
            const QString id = it.key();
            const auto handle = it.value();
            if (!handle.is_valid()
                || shouldIgnoreId(id)
                || m_userPausedIds.contains(id)
                || m_schedulerPausedIds.contains(id)
                || m_priorityPausedSeeds.contains(id)) {
                continue;
            }

            const auto status = handle.status();
            const bool isComplete = status.has_metadata && status.progress_ppm >= 1000000;
            if (isComplete || status.is_seeding) {
                continue;
            }

            ++activeDownloads;
            activeConnections += std::max(0, status.num_connections);
            aggregateDownloadRate += std::max(0, status.download_rate);
        }

        const qint64 averageRate = activeDownloads > 0 ? aggregateDownloadRate / activeDownloads : 0;
        if (activeDownloads == 0) {
            nextRequestQueueTime = DefaultRequestQueueTime;
            nextMaxOutRequestQueue = DefaultMaxOutRequestQueue;
        } else if (activeDownloads >= 8 || averageRate >= 8 * 1024 * 1024 || activeConnections >= 120) {
            nextRequestQueueTime = 10;
            nextMaxOutRequestQueue = 1536;
        } else if (activeDownloads >= 4 || averageRate >= 3 * 1024 * 1024 || activeConnections >= 64) {
            nextRequestQueueTime = 8;
            nextMaxOutRequestQueue = 1024;
        } else if (averageRate >= 1024 * 1024 || activeConnections >= 24) {
            nextRequestQueueTime = 6;
            nextMaxOutRequestQueue = 768;
        } else if (averageRate >= 256 * 1024 || activeConnections >= 8) {
            nextRequestQueueTime = 5;
            nextMaxOutRequestQueue = 512;
        } else {
            nextRequestQueueTime = 4;
            nextMaxOutRequestQueue = 384;
        }
    }

    if (m_requestQueueTime == nextRequestQueueTime && m_maxOutRequestQueue == nextMaxOutRequestQueue) {
        return;
    }

    m_requestQueueTime = nextRequestQueueTime;
    m_maxOutRequestQueue = nextMaxOutRequestQueue;

    lt::settings_pack settings;
    settings.set_int(lt::settings_pack::request_queue_time, m_requestQueueTime);
    settings.set_int(lt::settings_pack::max_out_request_queue, m_maxOutRequestQueue);
    m_session->apply_settings(settings);
}

void TorrentService::updateProbePieceSelection(
    const QString& id,
    const lt::torrent_handle& handle,
    const lt::torrent_status& status,
    const std::vector<lt::partial_piece_info>& downloadingPieces)
{
    auto clearProbe = [&]() {
        auto probe = m_pieceProbes.take(id);
        if (!handle.is_valid()) {
            return;
        }
        for (const int piece : probe.deadlinePieces) {
            if (piece >= 0) {
                handle.reset_piece_deadline(lt::piece_index_t(piece));
            }
        }
    };

    if (!m_dynamicBlockTuningEnabled
        || !handle.is_valid()
        || !status.has_metadata
        || status.is_seeding
        || status.progress_ppm >= 1000000
        || m_userPausedIds.contains(id)
        || m_schedulerPausedIds.contains(id)
        || m_priorityPausedSeeds.contains(id)) {
        clearProbe();
        return;
    }

    const int pieceCount = status.pieces.size();
    if (pieceCount <= 0) {
        clearProbe();
        return;
    }

    auto& probe = m_pieceProbes[id];
    const qint64 downloadedDelta = std::max<qint64>(0, status.total_wanted_done - probe.lastDownloadedBytes);
    probe.lastDownloadedBytes = status.total_wanted_done;
    const qint64 sampleRate = std::max<qint64>(0, std::max<qint64>(status.download_rate, downloadedDelta));
    probe.smoothedDownloadRate = probe.smoothedDownloadRate <= 0
        ? sampleRate
        : (probe.smoothedDownloadRate * 3 + sampleRate) / 4;
    auto resetProbeWindow = [&probe] {
        probe.probeActive = false;
        probe.probeSamples = 0;
        probe.probeBaselineRate = 0;
        probe.probeRateSum = 0;
    };
    auto resetPreProbeWindow = [&probe] {
        probe.preProbeRates.clear();
        probe.preProbeRateSum = 0;
    };
    auto addPreProbeSample = [&probe](qint64 rate) {
        probe.preProbeRates.enqueue(rate);
        probe.preProbeRateSum += rate;
        while (probe.preProbeRates.size() > ProbeBaselineSamples) {
            probe.preProbeRateSum -= probe.preProbeRates.dequeue();
        }
    };

    int activePieces = 0;
    int healthyPieces = 0;
    int requestedBlocks = 0;
    int progressingBlocks = 0;
    QSet<int> nextUnhealthyPieces;
    QVector<int> healthyActivePieces;
    QVector<int> activePieceIndexes;

    for (const auto& pieceInfo : downloadingPieces) {
        const int piece = static_cast<int>(pieceInfo.piece_index);
        if (piece < 0 || piece >= pieceCount || status.pieces.get_bit(lt::piece_index_t(piece))) {
            continue;
        }

        ++activePieces;
        activePieceIndexes.append(piece);
        requestedBlocks += pieceInfo.requested;
        progressingBlocks += pieceInfo.writing + pieceInfo.finished;

        bool hasPeerProgress = pieceInfo.writing > 0 || pieceInfo.finished > 0;
        for (int block = 0; block < pieceInfo.blocks_in_piece && pieceInfo.blocks != nullptr; ++block) {
            const auto& info = pieceInfo.blocks[block];
            hasPeerProgress = hasPeerProgress
                || info.bytes_progress > 0
                || info.num_peers > 0
                || info.state == lt::block_info::writing
                || info.state == lt::block_info::finished;
        }

        if (hasPeerProgress) {
            ++healthyPieces;
            healthyActivePieces.append(piece);
        }
        if (pieceInfo.requested > 0 && !hasPeerProgress) {
            nextUnhealthyPieces.insert(piece);
        }
    }

    const bool hasProbeActivity = activePieces > 0 || requestedBlocks > 0 || downloadedDelta > 0;
    const bool lowThroughput = (hasProbeActivity && status.download_rate < 160 * 1024)
        || (downloadedDelta == 0 && activePieces > 0 && requestedBlocks > progressingBlocks);
    const bool healthyThroughput = status.download_rate >= 512 * 1024
        || (downloadedDelta >= 384 * 1024 && healthyPieces >= std::max(1, activePieces / 2));

    if (lowThroughput) {
        probe.lowThroughputSamples = std::min(probe.lowThroughputSamples + 1, 8);
        probe.healthySamples = 0;
    } else if (healthyThroughput) {
        probe.healthySamples = std::min(probe.healthySamples + 1, 8);
        probe.lowThroughputSamples = 0;
    } else {
        probe.lowThroughputSamples = std::max(0, probe.lowThroughputSamples - 1);
        probe.healthySamples = std::max(0, probe.healthySamples - 1);
    }

    if (probe.probeCooldownSamples > 0) {
        --probe.probeCooldownSamples;
    }

    if (!probe.probeActive) {
        if (lowThroughput) {
            resetPreProbeWindow();
        } else if (healthyThroughput && activePieces >= probe.targetActivePieces && probe.probeCooldownSamples == 0) {
            addPreProbeSample(probe.smoothedDownloadRate);
        }
    }

    if (probe.probeActive) {
        ++probe.probeSamples;
        probe.probeRateSum += probe.smoothedDownloadRate;

        const bool probeStalled = probe.lowThroughputSamples >= 2;
        const bool probeWindowComplete = probe.probeSamples >= ProbeEvaluationSamples;
        if (probeStalled || probeWindowComplete) {
            const bool hasBaseline = probe.probeBaselineRate > 0;
            const qint64 probeAverageRate = probe.probeSamples > 0
                ? probe.probeRateSum / probe.probeSamples
                : probe.smoothedDownloadRate;
            const bool meaningfulGain = !hasBaseline
                || probeAverageRate * 1000 >= probe.probeBaselineRate * (1000 + ProbeMinGainPermille);
            const bool meaningfulLoss = hasBaseline
                && probeAverageRate * 1000 < probe.probeBaselineRate * (1000 - ProbeLossPermille);

            if (probeStalled || meaningfulLoss || !meaningfulGain) {
                probe.targetActivePieces = std::max(MinProbePieces, probe.probeBaselinePieces);
                probe.probeCooldownSamples = ProbeCooldownSamples;
            }

            resetProbeWindow();
            probe.healthySamples = 0;
        }
    }

    if (probe.probeCooldownSamples == 0
        && probe.lowThroughputSamples >= 2
        && activePieces <= probe.targetActivePieces) {
        probe.targetActivePieces = std::min(MaxProbePieces, probe.targetActivePieces + 2);
        probe.lowThroughputSamples = 0;
        resetProbeWindow();
        resetPreProbeWindow();
    } else if (!probe.probeActive
        && probe.probeCooldownSamples == 0
        && probe.healthySamples >= 3
        && probe.preProbeRates.size() >= ProbeBaselineSamples
        && activePieces >= probe.targetActivePieces
        && probe.targetActivePieces < MaxProbePieces) {
        probe.probeBaselinePieces = probe.targetActivePieces;
        probe.probeBaselineRate = probe.preProbeRateSum / probe.preProbeRates.size();
        probe.probeRateSum = 0;
        probe.probeSamples = 0;
        probe.probeActive = true;
        probe.targetActivePieces = std::min(MaxProbePieces, probe.targetActivePieces + 1);
        probe.healthySamples = 0;
        resetPreProbeWindow();
    }

    QSet<int> nextDeadlinePieces;
    for (const int piece : probe.deadlinePieces) {
        if (piece >= 0 && piece < pieceCount && !status.pieces.get_bit(lt::piece_index_t(piece))) {
            nextDeadlinePieces.insert(piece);
        }
    }
    for (const int piece : healthyActivePieces) {
        if (nextDeadlinePieces.size() >= probe.targetActivePieces) {
            break;
        }
        nextDeadlinePieces.insert(piece);
    }
    for (const int piece : activePieceIndexes) {
        if (nextDeadlinePieces.size() >= probe.targetActivePieces) {
            break;
        }
        nextDeadlinePieces.insert(piece);
    }
    for (int piece = 0; piece < pieceCount && nextDeadlinePieces.size() < probe.targetActivePieces; ++piece) {
        if (!status.pieces.get_bit(lt::piece_index_t(piece))) {
            nextDeadlinePieces.insert(piece);
        }
    }
    for (const int piece : nextDeadlinePieces) {
        const bool alreadyActive = activePieceIndexes.contains(piece);
        const int deadlineMs = healthyActivePieces.contains(piece)
            ? 0
            : alreadyActive
                ? 250
                : 750;
        handle.set_piece_deadline(lt::piece_index_t(piece), deadlineMs);
    }

    probe.deadlinePieces = nextDeadlinePieces;
    probe.unhealthyPieces = nextUnhealthyPieces;
}

void TorrentService::updateDownloadQueue()
{
    struct Candidate {
        QString id;
        libtorrent::torrent_handle handle;
        QDateTime createdAt;
    };

    QVector<Candidate> candidates;
    bool queueChanged = false;
    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        const QString id = it.key();
        auto handle = it.value();
        if (!handle.is_valid() || shouldIgnoreId(id) || m_userPausedIds.contains(id)) {
            continue;
        }

        const auto status = handle.status();
        const bool isComplete = status.has_metadata && status.progress_ppm >= 1000000;
        if (isComplete || status.is_seeding || m_priorityPausedSeeds.contains(id)) {
            m_schedulerPausedIds.remove(id);
            continue;
        }

        candidates.append({id, handle, m_items.value(id).createdAt});
    }

    std::sort(candidates.begin(), candidates.end(), [](const Candidate& left, const Candidate& right) {
        if (left.createdAt.isValid() && right.createdAt.isValid()) {
            return left.createdAt < right.createdAt;
        }
        if (left.createdAt.isValid() != right.createdAt.isValid()) {
            return left.createdAt.isValid();
        }
        return left.id < right.id;
    });

    for (int i = 0; i < candidates.size(); ++i) {
        auto handle = candidates.at(i).handle;
        const QString& id = candidates.at(i).id;
        const bool shouldRun = i < m_maxActiveDownloads;
        const auto status = handle.status();
        const bool isPaused = (status.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {};

        if (shouldRun) {
            if (m_schedulerPausedIds.remove(id) || isPaused) {
                handle.unset_flags(lt::torrent_flags::paused);
                handle.resume();
                handle.force_reannounce();
                handle.force_dht_announce();
                handle.force_lsd_announce();
                queueChanged = true;
            }
            continue;
        }

        if (!isPaused) {
            handle.set_flags(lt::torrent_flags::paused);
            handle.pause();
            queueChanged = true;
        }
        const bool wasSchedulerPaused = m_schedulerPausedIds.contains(id);
        m_schedulerPausedIds.insert(id);

        auto item = m_items.value(id);
        const bool shouldEmitWaitingUpdate = !wasSchedulerPaused
            || item.state != awa::core::DownloadState::Waiting
            || item.downloadRate != 0
            || item.uploadRate != 0;
        if (item.id.isEmpty()) {
            item.id = id;
        }
        item.state = awa::core::DownloadState::Waiting;
        item.statusText = QStringLiteral("等待可用下载槽");
        item.downloadRate = 0;
        m_items.insert(id, item);
        if (shouldEmitWaitingUpdate) {
            queueChanged = true;
            emit itemUpdated(item);
        }
    }

    if (queueChanged) {
        persistItems();
    }
}

void TorrentService::updateSeedingPriority()
{
    const bool activeDownloads = hasActiveDownloads();

    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        const QString id = it.key();
        auto handle = it.value();
        if (!handle.is_valid() || shouldIgnoreId(id)) {
            continue;
        }

        const auto status = handle.status();
        const bool isComplete = status.has_metadata && status.progress_ppm >= 1000000;
        const bool isSeeding = status.is_seeding;
        const bool isPaused = (status.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {};
        const bool priorityPaused = m_priorityPausedSeeds.contains(id);
        const bool userPaused = isPaused && !priorityPaused;
        if (!isComplete || userPaused) {
            continue;
        }

        auto item = m_items.value(id);
        if (item.id.isEmpty()) {
            item.id = id;
        }

        if (!m_seedOnCompletionEnabled) {
            if (!isPaused) {
                handle.set_flags(lt::torrent_flags::paused);
                handle.pause();
            }
            m_priorityPausedSeeds.remove(id);
            item = itemFromHandle(handle);
            item.state = awa::core::DownloadState::Finished;
            item.statusText = QStringLiteral("下载完成");
            item.downloadRate = 0;
            item.uploadRate = 0;
            m_items.insert(id, item);
            emit itemUpdated(item);
            continue;
        }

        if (activeDownloads && isSeeding && !isPaused) {
            handle.set_flags(lt::torrent_flags::paused);
            handle.pause();
            m_priorityPausedSeeds.insert(id);
            item = itemFromHandle(handle);
            item.state = awa::core::DownloadState::Seeding;
            item.statusText = QStringLiteral("下载优先，等待下载任务完成后继续做种");
            item.downloadRate = 0;
            m_items.insert(id, item);
            emit itemUpdated(item);
            continue;
        }

        if (!activeDownloads && priorityPaused) {
            handle.unset_flags(lt::torrent_flags::paused);
            handle.resume();
            handle.force_reannounce();
            handle.force_dht_announce();
            handle.force_lsd_announce();
            m_priorityPausedSeeds.remove(id);
            item = itemFromHandle(handle);
            item.state = awa::core::DownloadState::Seeding;
            item.statusText = QStringLiteral("继续做种中");
            m_items.insert(id, item);
            emit itemUpdated(item);
            continue;
        }

        if (priorityPaused) {
            item = itemFromHandle(handle);
            item.state = awa::core::DownloadState::Seeding;
            item.statusText = QStringLiteral("下载优先，等待下载任务完成后继续做种");
            item.downloadRate = 0;
            item.uploadRate = 0;
            m_items.insert(id, item);
            emit itemUpdated(item);
        }
    }
}

awa::core::DownloadItem TorrentService::itemFromHandle(const lt::torrent_handle& handle) const
{
    const auto status = handle.status(
        lt::torrent_handle::query_save_path
        | lt::torrent_handle::query_name
        | lt::torrent_handle::query_pieces);
    awa::core::DownloadItem item = m_items.value(handleId(handle));
    const auto storedSnapshot = item;
    const auto previousState = item.state;
    item.id = handleId(handle);
    const QString liveName = QString::fromStdString(status.name);
    if (!liveName.isEmpty()) {
        item.name = liveName;
    }
    const QString liveSavePath = QString::fromStdString(status.save_path);
    if (!liveSavePath.isEmpty()) {
        item.savePath = liveSavePath;
    }

    const bool hasTransferSnapshot = hasMeaningfulTransferSnapshot(status);
    const bool shouldApplyTransferSnapshot = hasTransferSnapshot
        && !liveSnapshotWouldResetProgress(storedSnapshot, status)
        && !liveSnapshotWouldResetPieceMap(storedSnapshot, status)
        && !(storedSnapshot.totalBytes > 0 && status.total_wanted <= 0);
    if (shouldApplyTransferSnapshot) {
        item.progress = status.progress_ppm / 1000000.0;
        item.totalBytes = status.total_wanted;
        item.downloadedBytes = status.total_wanted_done;
        item.pieceCount = status.pieces.size();
        item.completedPieces = status.num_pieces;
        item.blockSize = status.block_size;

        std::vector<lt::partial_piece_info> downloadingPieces;
        handle.get_download_queue(downloadingPieces);
        item.pieceMap = pieceMap(status.pieces, downloadingPieces, m_pieceProbes.value(item.id).unhealthyPieces);
    }
    fillMissingTransferSnapshot(item, storedSnapshot);
    item.downloadRate = status.download_rate;
    item.uploadRate = status.upload_rate;
    item.peerCount = std::max(0, status.num_peers + status.list_peers);
    item.seedCount = std::max(0, status.num_seeds);
    item.connectionCount = std::max(0, status.num_connections);
    item.dhtStatusText = status.announcing_to_dht
        ? QStringLiteral("DHT 查询中")
        : QStringLiteral("DHT 等待下一次查询");
    if (status.all_time_download > 0) {
        item.ratio = static_cast<double>(status.all_time_upload) / static_cast<double>(status.all_time_download);
    }
    const auto liveTrackers = handle.trackers();
    item.trackerCount = static_cast<int>(liveTrackers.size());
    item.workingTrackerCount = 0;
    item.failedTrackerCount = 0;
    if (!liveTrackers.empty()) {
        item.trackers.clear();
        for (const auto& tracker : liveTrackers) {
            const QString url = QString::fromStdString(tracker.url);
            item.trackers.append(url);
            const auto health = m_trackerHealth.value(url);
            bool trackerIsUpdating = false;
            bool trackerHasRecentFailure = false;
            for (const auto& endpoint : tracker.endpoints) {
                for (const auto& infoHash : endpoint.info_hashes) {
                    trackerIsUpdating = trackerIsUpdating || infoHash.updating;
                    trackerHasRecentFailure = trackerHasRecentFailure
                        || infoHash.fails > 0
                        || static_cast<bool>(infoHash.last_error);
                }
            }
            const bool trackerIsWorking = tracker.verified || health.successes > health.failures;
            if (trackerIsWorking) {
                ++item.workingTrackerCount;
            } else if (trackerHasRecentFailure || (health.failures > 0 && health.failures > health.successes)) {
                ++item.failedTrackerCount;
            } else if (trackerIsUpdating) {
                item.connectionHealthText = QStringLiteral("正在等待 Tracker 响应");
            }
        }
    }
    if (item.connectionCount > 0) {
        item.connectionHealthText = QStringLiteral("已连接 %1 个 peer").arg(item.connectionCount);
    } else if (item.peerCount > 0) {
        item.connectionHealthText = QStringLiteral("发现 %1 个 peer，正在建立连接").arg(item.peerCount);
    } else if (item.trackerCount > 0 || status.announcing_to_dht) {
        item.connectionHealthText = QStringLiteral("正在通过 Tracker/DHT 查找 peer");
    } else {
        item.connectionHealthText = QStringLiteral("等待可用 peer");
    }

    const bool completedWithoutSeeding = item.state == awa::core::DownloadState::Finished
        && !m_seedOnCompletionEnabled
        && status.has_metadata
        && status.progress_ppm >= 1000000;
    const bool preserveStoredPausedState = !hasTransferSnapshot
        && isPausedState(previousState);
    const bool previousStateWasCompleted = isCompletedState(previousState);
    const bool preserveStoredCompletedState = previousStateWasCompleted
        && (!hasTransferSnapshot
            || liveSnapshotWouldResetProgress(storedSnapshot, status)
            || liveSnapshotWouldResetPieceMap(storedSnapshot, status)
            || status.progress_ppm < 1000000);

    if (m_priorityPausedSeeds.contains(item.id)) {
        item.state = m_seedOnCompletionEnabled
            ? awa::core::DownloadState::Seeding
            : awa::core::DownloadState::Finished;
    } else if (preserveStoredPausedState) {
        item.state = previousState;
    } else if (preserveStoredCompletedState) {
        item.state = previousState;
    } else if (m_userPausedIds.contains(item.id)) {
        item.state = status.has_metadata && (status.is_seeding || status.progress_ppm >= 1000000)
            ? awa::core::DownloadState::PausedSeeding
            : awa::core::DownloadState::PausedDownloading;
    } else if (m_schedulerPausedIds.contains(item.id)) {
        item.state = awa::core::DownloadState::Waiting;
    } else if (completedWithoutSeeding) {
        item.state = awa::core::DownloadState::Finished;
    } else if ((status.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {}) {
        item.state = status.has_metadata && status.progress_ppm < 1000000
            ? awa::core::DownloadState::Waiting
            : status.has_metadata && (status.is_seeding || status.progress_ppm >= 1000000)
                ? awa::core::DownloadState::PausedSeeding
                : awa::core::DownloadState::PausedDownloading;
    } else if (!status.has_metadata) {
        item.state = awa::core::DownloadState::FetchingMetadata;
    } else if (status.is_seeding) {
        item.state = awa::core::DownloadState::Seeding;
    } else if (status.progress_ppm >= 1000000) {
        item.state = awa::core::DownloadState::Finished;
    } else {
        item.state = awa::core::DownloadState::Downloading;
    }

    if (item.state == awa::core::DownloadState::FetchingMetadata) {
        item.statusText = QStringLiteral("获取元数据：%1，Peers %2，连接 %3，Tracker %4/%5")
            .arg(item.dhtStatusText)
            .arg(item.peerCount)
            .arg(item.connectionCount)
            .arg(item.workingTrackerCount)
            .arg(item.trackerCount);
    } else if (item.state == awa::core::DownloadState::Downloading) {
        item.statusText = QStringLiteral("下载中：Peers %1，Seeds %2，连接 %3，Tracker %4/%5")
            .arg(item.peerCount)
            .arg(item.seedCount)
            .arg(item.connectionCount)
            .arg(item.workingTrackerCount)
            .arg(item.trackerCount);
    } else if (item.state == awa::core::DownloadState::Waiting) {
        item.statusText = QStringLiteral("等待可用下载槽");
    } else if (m_priorityPausedSeeds.contains(item.id)) {
        item.statusText = m_seedOnCompletionEnabled
            ? QStringLiteral("下载优先，等待下载任务完成后继续做种")
            : QStringLiteral("下载完成");
    } else if (m_userPausedIds.contains(item.id)) {
        item.statusText = awa::core::stateToString(item.state);
    } else {
        item.statusText = awa::core::stateToString(item.state);
    }
    if (item.createdAt.isNull()) {
        item.createdAt = QDateTime::currentDateTimeUtc();
    }
    return item;
}

void TorrentService::rememberHandle(
    const lt::torrent_handle& handle,
    awa::core::DownloadState initialState,
    bool emitUpdate)
{
    if (!handle.is_valid()) {
        return;
    }

    const QString id = handleId(handle);
    if (shouldIgnoreId(id)) {
        return;
    }
    m_handles.insert(id, handle);

    auto item = itemFromHandle(handle);
    item.id = id;
    if (item.state == awa::core::DownloadState::Queued) {
        item.state = initialState;
    }
    const auto existing = m_items.value(id);
    if (isCompletedState(existing.state) && !isCompletedState(item.state)) {
        item.state = existing.state;
        item.statusText = existing.statusText;
        item.downloadRate = 0;
        item.uploadRate = 0;
    }
    if (item.name.isEmpty()) {
        item.name = QStringLiteral("Magnet %1").arg(id.left(12));
    }
    if (item.state == awa::core::DownloadState::FetchingMetadata
        || initialState == awa::core::DownloadState::FetchingMetadata) {
        item.statusText = QStringLiteral("采用稀有块优先策略获取元数据");
    } else {
        item.statusText = awa::core::stateToString(item.state);
    }
    m_items.insert(id, item);
    if (emitUpdate) {
        emit itemUpdated(item);
    }
}

QString TorrentService::persistenceDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/.AwaKurageDownloader");
    }
    return QDir(dir).filePath(QStringLiteral("torrent-state"));
}

QString TorrentService::indexFilePath() const
{
    return QDir(persistenceDir()).filePath(QStringLiteral("downloads.json"));
}

QString TorrentService::resumeDataPath(const QString& id) const
{
    return QDir(persistenceDir()).filePath(id + QStringLiteral(".fastresume"));
}

void TorrentService::persistItems() const
{
    QDir().mkpath(persistenceDir());

    QJsonArray downloads;
    for (const auto& item : m_items) {
        if (!item.id.isEmpty()) {
            QJsonObject object = itemToJson(item);
            object.insert(QStringLiteral("userPaused"), m_userPausedIds.contains(item.id));
            object.insert(QStringLiteral("schedulerPaused"), m_schedulerPausedIds.contains(item.id));
            downloads.append(object);
        }
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("downloads"), downloads);

    QSaveFile file(indexFilePath());
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

void TorrentService::requestResumeData(const QString& id, bool force)
{
    if (!m_handles.contains(id)) {
        return;
    }

    auto handle = m_handles.value(id);
    if (!handle.is_valid()) {
        return;
    }

    try {
        auto flags = lt::torrent_handle::save_info_dict;
        if (force) {
            flags |= lt::torrent_handle::flush_disk_cache;
        } else {
            flags |= lt::torrent_handle::only_if_modified;
        }
        handle.save_resume_data(flags);
    } catch (const lt::system_error& error) {
        spdlog::warn("Failed to request resume data for {}: {}", id.toStdString(), error.what());
    }
}

void TorrentService::requestResumeDataForAll()
{
    for (auto it = m_handles.cbegin(); it != m_handles.cend(); ++it) {
        requestResumeData(it.key());
    }
    persistItems();
}

void TorrentService::restartDownloadsAfterNetworkChange()
{
    const auto* networkInformation = QNetworkInformation::instance();
    if (networkInformation
        && networkInformation->reachability() == QNetworkInformation::Reachability::Disconnected) {
        for (auto it = m_handles.cbegin(); it != m_handles.cend(); ++it) {
            if (shouldIgnoreId(it.key()) || !it.value().is_valid()) {
                continue;
            }

            auto item = m_items.value(it.key());
            if (item.id.isEmpty()) {
                item.id = it.key();
            }
            if (item.state != awa::core::DownloadState::PausedDownloading
                && item.state != awa::core::DownloadState::PausedSeeding) {
                item.downloadRate = 0;
                item.uploadRate = 0;
                item.statusText = QStringLiteral("网络已断开，等待恢复后重连");
                m_items.insert(it.key(), item);
                emit itemUpdated(item);
            }
        }
        persistItems();
        return;
    }

    bool restartedAny = false;
    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        const QString id = it.key();
        auto handle = it.value();
        if (shouldIgnoreId(id) || !handle.is_valid()) {
            continue;
        }

        auto item = m_items.value(id);
        if (item.state == awa::core::DownloadState::PausedDownloading
            || item.state == awa::core::DownloadState::PausedSeeding) {
            continue;
        }

        handle.unset_flags(lt::torrent_flags::auto_managed | lt::torrent_flags::paused);
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();
        const auto hashes = handle.info_hashes();
        if (hashes.has_v1()) {
            m_session->dht_get_peers(hashes.v1);
        }

        if (item.id.isEmpty()) {
            item.id = id;
        }
        item.state = item.totalBytes > 0
            ? awa::core::DownloadState::Downloading
            : awa::core::DownloadState::FetchingMetadata;
        item.statusText = QStringLiteral("网络变化，正在重新连接");
        m_items.insert(id, item);
        emit itemUpdated(item);
        restartedAny = true;
    }

    if (restartedAny) {
        persistItems();
        requestResumeDataForAll();
        emit errorRaised(QStringLiteral("网络环境变化，已重启下载连接"));
    }
}

void TorrentService::writeResumeData(const QString& id, const lt::add_torrent_params& params) const
{
    if (id.isEmpty() || !m_items.contains(id)) {
        return;
    }

    QDir().mkpath(persistenceDir());
    const auto data = lt::write_resume_data_buf(params);
    QSaveFile file(resumeDataPath(id));
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(data.data(), static_cast<qint64>(data.size()));
    file.commit();
}

void TorrentService::applyTrackers(lt::add_torrent_params& params) const
{
    auto trackers = m_defaultTrackers;
    std::sort(trackers.begin(), trackers.end(), [this](const QString& left, const QString& right) {
        return trackerHealthScore(left) > trackerHealthScore(right);
    });

    for (const auto& tracker : trackers) {
        const auto url = tracker.toStdString();
        const auto exists = std::any_of(params.trackers.cbegin(), params.trackers.cend(), [&url](const std::string& existing) {
            return existing == url;
        });
        if (!exists) {
            params.trackers.push_back(url);
        }
    }

    params.tracker_tiers.resize(params.trackers.size(), 0);
    std::fill(params.tracker_tiers.begin(), params.tracker_tiers.end(), 0);
}

void TorrentService::applyTrackersToHandle(const lt::torrent_handle& handle) const
{
    if (!handle.is_valid()) {
        return;
    }

    auto trackers = handle.trackers();
    auto defaults = m_defaultTrackers;
    std::sort(defaults.begin(), defaults.end(), [this](const QString& left, const QString& right) {
        return trackerHealthScore(left) > trackerHealthScore(right);
    });

    for (const auto& tracker : defaults) {
        const auto url = tracker.toStdString();
        const auto exists = std::any_of(trackers.cbegin(), trackers.cend(), [&url](const lt::announce_entry& existing) {
            return existing.url == url;
        });
        if (!exists) {
            lt::announce_entry entry(url);
            entry.tier = 0;
            trackers.push_back(entry);
        }
    }
    for (auto& tracker : trackers) {
        tracker.tier = 0;
    }
    std::sort(trackers.begin(), trackers.end(), [this](const lt::announce_entry& left, const lt::announce_entry& right) {
        const int leftScore = trackerHealthScore(QString::fromStdString(left.url));
        const int rightScore = trackerHealthScore(QString::fromStdString(right.url));
        return leftScore > rightScore;
    });
    handle.replace_trackers(trackers);
    handle.force_reannounce();
}

} // namespace awa::torrent
