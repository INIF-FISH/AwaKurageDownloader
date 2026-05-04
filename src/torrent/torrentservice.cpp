#include "awa/torrent/torrentservice.h"

#include "awa/torrent/magnetutils.h"

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>

#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/error_code.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/settings_pack.hpp>
#include <libtorrent/torrent_info.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>

namespace awa::torrent {
namespace lt = libtorrent;

namespace {
QString hashId(const lt::info_hash_t& hashes)
{
    const auto best = hashes.get_best();
    return QString::fromLatin1(QByteArray(best.data(), best.size()).toHex());
}

QString sampledPieceMap(const lt::typed_bitfield<lt::piece_index_t>& pieces, int maxSamples = 96)
{
    const int total = pieces.size();
    if (total <= 0) {
        return {};
    }

    const int samples = std::min(total, maxSamples);
    QString map;
    map.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        const int from = i * total / samples;
        const int to = std::max(from + 1, (i + 1) * total / samples);
        bool complete = false;
        for (int piece = from; piece < to; ++piece) {
            if (pieces.get_bit(lt::piece_index_t(piece))) {
                complete = true;
                break;
            }
        }
        map.append(complete ? QLatin1Char('1') : QLatin1Char('0'));
    }
    return map;
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
    settings.set_bool(lt::settings_pack::piece_extent_affinity, true);
    settings.set_bool(lt::settings_pack::prioritize_partial_pieces, false);
    settings.set_str(lt::settings_pack::user_agent, "AwaKurageDownloader/0.1.0");
    settings.set_str(lt::settings_pack::listen_interfaces, "0.0.0.0:6881-6891,[::]:6881-6891");
    settings.set_str(lt::settings_pack::dht_bootstrap_nodes,
        "router.bittorrent.com:6881,dht.transmissionbt.com:6881,router.utorrent.com:6881");
    settings.set_int(lt::settings_pack::connections_limit, 500);
    settings.set_int(lt::settings_pack::active_downloads, 20);
    settings.set_int(lt::settings_pack::active_limit, 200);
    settings.set_int(lt::settings_pack::connection_speed, 50);
    settings.set_int(lt::settings_pack::request_queue_time, 6);
    settings.set_int(lt::settings_pack::max_out_request_queue, 768);
    settings.set_int(lt::settings_pack::whole_pieces_threshold, 20);
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

    connect(&m_alertTimer, &QTimer::timeout, this, &TorrentService::pollAlerts);
    connect(&m_statusTimer, &QTimer::timeout, this, &TorrentService::refreshStatuses);
    m_alertTimer.start(250);
    m_statusTimer.start(1000);
}

TorrentService::~TorrentService() = default;

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
    if (options.startPaused) {
        params.flags |= lt::torrent_flags::paused;
    }

    auto handle = m_session->add_torrent(std::move(params), ec);
    if (ec) {
        emit errorRaised(QStringLiteral("添加种子失败：%1").arg(QString::fromStdString(ec.message())));
        return;
    }
    rememberHandle(handle, awa::core::DownloadState::Queued);
    if (!options.startPaused) {
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();
    }
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
    if (params.trackers.empty()) {
        params.trackers.emplace_back("udp://tracker.opentrackr.org:1337/announce");
        params.trackers.emplace_back("udp://open.stealth.si:80/announce");
        params.trackers.emplace_back("udp://tracker.openbittorrent.com:6969/announce");
    }
    const auto infoHashes = params.info_hashes;
    const QString normalizedId = hashId(infoHashes);
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
    if (!options.startPaused) {
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();
        if (infoHashes.has_v1()) {
            m_session->dht_get_peers(infoHashes.v1);
        }
    }
}

void TorrentService::pause(const QString& id)
{
    if (m_handles.contains(id)) {
        auto handle = m_handles.value(id);
        handle.unset_flags(lt::torrent_flags::auto_managed);
        handle.set_flags(lt::torrent_flags::paused);
        handle.pause();

        auto item = m_items.value(id);
        if (item.id.isEmpty()) {
            item.id = id;
        }
        item.state = awa::core::DownloadState::Paused;
        item.downloadRate = 0;
        item.uploadRate = 0;
        item.statusText = QStringLiteral("已暂停");
        m_items.insert(id, item);
        emit itemUpdated(item);
    }
}

void TorrentService::resume(const QString& id)
{
    if (m_handles.contains(id)) {
        auto handle = m_handles.value(id);
        handle.unset_flags(lt::torrent_flags::auto_managed | lt::torrent_flags::paused | lt::torrent_flags::sequential_download);
        handle.resume();
        handle.force_reannounce();
        handle.force_dht_announce();
        handle.force_lsd_announce();

        auto item = itemFromHandle(handle);
        if (item.state == awa::core::DownloadState::Paused) {
            item.state = item.totalBytes > 0
                ? awa::core::DownloadState::Downloading
                : awa::core::DownloadState::FetchingMetadata;
        }
        item.statusText = QStringLiteral("正在恢复连接...");
        m_items.insert(id, item);
        emit itemUpdated(item);
    }
}

void TorrentService::remove(const QString& id, bool removeFiles)
{
    if (!m_handles.contains(id)) {
        return;
    }

    if (removeFiles) {
        m_session->remove_torrent(m_handles.take(id), lt::session::delete_files);
    } else {
        m_session->remove_torrent(m_handles.take(id));
    }
    m_items.remove(id);
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

void TorrentService::pollAlerts()
{
    std::vector<lt::alert*> alerts;
    m_session->pop_alerts(&alerts);

    for (const auto* alert : alerts) {
        if (const auto* added = lt::alert_cast<lt::add_torrent_alert>(alert)) {
            rememberHandle(added->handle, awa::core::DownloadState::Queued);
        } else if (const auto* metadata = lt::alert_cast<lt::metadata_received_alert>(alert)) {
            rememberHandle(metadata->handle, awa::core::DownloadState::Downloading);
        } else if (const auto* reply = lt::alert_cast<lt::tracker_reply_alert>(alert)) {
            const QString id = handleId(reply->handle);
            auto item = m_items.value(id);
            item.statusText = QStringLiteral("Tracker 已响应，发现 %1 个 peer").arg(reply->num_peers);
            m_items.insert(id, item);
            emit itemUpdated(item);
        } else if (const auto* warning = lt::alert_cast<lt::tracker_warning_alert>(alert)) {
            const QString id = handleId(warning->handle);
            auto item = m_items.value(id);
            item.statusText = QStringLiteral("Tracker 警告：%1").arg(QString::fromStdString(warning->message()));
            m_items.insert(id, item);
            emit itemUpdated(item);
        } else if (const auto* trackerError = lt::alert_cast<lt::tracker_error_alert>(alert)) {
            const QString id = handleId(trackerError->handle);
            auto item = m_items.value(id);
            item.statusText = QStringLiteral("Tracker 暂无响应，继续通过 DHT 查找 peer");
            m_items.insert(id, item);
            emit itemUpdated(item);
        } else if (const auto* removed = lt::alert_cast<lt::torrent_removed_alert>(alert)) {
            const QString id = hashId(removed->info_hashes);
            m_handles.remove(id);
            m_items.remove(id);
            emit itemRemoved(id);
        } else if (const auto* error = lt::alert_cast<lt::torrent_error_alert>(alert)) {
            const QString id = handleId(error->handle);
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
    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        if (!it.value().is_valid()) {
            continue;
        }
        const auto item = itemFromHandle(it.value());
        m_items.insert(item.id, item);
        emit itemUpdated(item);
    }
}

QString TorrentService::handleId(const lt::torrent_handle& handle) const
{
    if (!handle.is_valid()) {
        return {};
    }
    return hashId(handle.info_hashes());
}

awa::core::DownloadItem TorrentService::itemFromHandle(const lt::torrent_handle& handle) const
{
    const auto status = handle.status(
        lt::status_flags_t::all() | lt::torrent_handle::query_save_path | lt::torrent_handle::query_name);
    awa::core::DownloadItem item = m_items.value(handleId(handle));
    item.id = handleId(handle);
    item.name = QString::fromStdString(status.name);
    item.savePath = QString::fromStdString(status.save_path);
    item.progress = status.progress_ppm / 1000000.0;
    item.totalBytes = status.total_wanted;
    item.downloadedBytes = status.total_wanted_done;
    item.downloadRate = status.download_rate;
    item.uploadRate = status.upload_rate;
    item.pieceCount = status.pieces.size();
    item.completedPieces = status.num_pieces;
    item.blockSize = status.block_size;
    item.pieceMap = sampledPieceMap(status.pieces);
    item.ratio = status.all_time_download > 0
        ? static_cast<double>(status.all_time_upload) / static_cast<double>(status.all_time_download)
        : 0.0;

    if ((status.flags & lt::torrent_flags::paused) != lt::torrent_flags_t {}) {
        item.state = awa::core::DownloadState::Paused;
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
        item.statusText = QStringLiteral("获取元数据：DHT %1，Peers %2，连接 %3")
            .arg(status.announcing_to_dht ? QStringLiteral("查询中") : QStringLiteral("等待"))
            .arg(status.num_peers + status.list_peers)
            .arg(status.num_connections);
    } else if (item.state == awa::core::DownloadState::Downloading) {
        item.statusText = QStringLiteral("下载中：Peers %1，连接 %2")
            .arg(status.num_peers + status.list_peers)
            .arg(status.num_connections);
    } else {
        item.statusText = awa::core::stateToString(item.state);
    }
    if (item.createdAt.isNull()) {
        item.createdAt = QDateTime::currentDateTimeUtc();
    }
    return item;
}

void TorrentService::rememberHandle(const lt::torrent_handle& handle, awa::core::DownloadState initialState)
{
    if (!handle.is_valid()) {
        return;
    }

    const QString id = handleId(handle);
    m_handles.insert(id, handle);

    auto item = itemFromHandle(handle);
    item.id = id;
    if (item.state == awa::core::DownloadState::Queued) {
        item.state = initialState;
    }
    if (item.name.isEmpty()) {
        item.name = QStringLiteral("Magnet %1").arg(id.left(12));
    }
    m_items.insert(id, item);
    emit itemUpdated(item);
}

} // namespace awa::torrent
