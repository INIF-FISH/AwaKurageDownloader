#include "awa/core/downloadlistmodel.h"

#include <QVariantMap>

namespace awa::core {
namespace {
QVariantMap itemToMap(const DownloadListModel& model, int row, bool includePieceMap)
{
    QVariantMap result;
    const auto roles = model.roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (!includePieceMap && it.key() == DownloadListModel::PieceMapRole) {
            continue;
        }
        result.insert(QString::fromUtf8(it.value()), model.data(model.index(row), it.key()));
    }
    return result;
}
} // namespace

QString stateToString(DownloadState state)
{
    switch (state) {
    case DownloadState::Queued: return QStringLiteral("排队中");
    case DownloadState::FetchingMetadata: return QStringLiteral("获取元数据");
    case DownloadState::Downloading: return QStringLiteral("下载中");
    case DownloadState::PausedDownloading: return QStringLiteral("暂停下载");
    case DownloadState::Seeding: return QStringLiteral("做种中");
    case DownloadState::Finished: return QStringLiteral("已完成");
    case DownloadState::Error: return QStringLiteral("错误");
    case DownloadState::PausedSeeding: return QStringLiteral("暂停做种");
    case DownloadState::Waiting: return QStringLiteral("等待下载");
    }
    return QStringLiteral("未知");
}

DownloadListModel::DownloadListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int DownloadListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant DownloadListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const auto& item = m_items.at(index.row());
    switch (role) {
    case IdRole: return item.id;
    case NameRole: return item.name;
    case SourceRole: return item.source;
    case SavePathRole: return item.savePath;
    case StateRole: return static_cast<int>(item.state);
    case StateTextRole: return stateToString(item.state);
    case ProgressRole: return item.progress;
    case TotalBytesRole: return item.totalBytes;
    case DownloadedBytesRole: return item.downloadedBytes;
    case DownloadRateRole: return item.downloadRate;
    case UploadRateRole: return item.uploadRate;
    case IsCompleteRole: return item.isComplete;
    case PieceCountRole: return item.pieceCount;
    case CompletedPiecesRole: return item.completedPieces;
    case BlockSizeRole: return item.blockSize;
    case PieceMapRole: return item.pieceMap;
    case RatioRole: return item.ratio;
    case PeerCountRole: return item.peerCount;
    case SeedCountRole: return item.seedCount;
    case ConnectionCountRole: return item.connectionCount;
    case TrackerCountRole: return item.trackerCount;
    case WorkingTrackerCountRole: return item.workingTrackerCount;
    case FailedTrackerCountRole: return item.failedTrackerCount;
    case DhtStatusTextRole: return item.dhtStatusText;
    case ConnectionHealthTextRole: return item.connectionHealthText;
    case StatusTextRole: return item.statusText;
    case ErrorTextRole: return item.errorText;
    default: return {};
    }
}

QHash<int, QByteArray> DownloadListModel::roleNames() const
{
    return {
        {IdRole, "downloadId"},
        {NameRole, "name"},
        {SourceRole, "source"},
        {SavePathRole, "savePath"},
        {StateRole, "state"},
        {StateTextRole, "stateText"},
        {ProgressRole, "progress"},
        {TotalBytesRole, "totalBytes"},
        {DownloadedBytesRole, "downloadedBytes"},
        {DownloadRateRole, "downloadRate"},
        {UploadRateRole, "uploadRate"},
        {IsCompleteRole, "isComplete"},
        {PieceCountRole, "pieceCount"},
        {CompletedPiecesRole, "completedPieces"},
        {BlockSizeRole, "blockSize"},
        {PieceMapRole, "pieceMap"},
        {RatioRole, "ratio"},
        {PeerCountRole, "peerCount"},
        {SeedCountRole, "seedCount"},
        {ConnectionCountRole, "connectionCount"},
        {TrackerCountRole, "trackerCount"},
        {WorkingTrackerCountRole, "workingTrackerCount"},
        {FailedTrackerCountRole, "failedTrackerCount"},
        {DhtStatusTextRole, "dhtStatusText"},
        {ConnectionHealthTextRole, "connectionHealthText"},
        {StatusTextRole, "statusText"},
        {ErrorTextRole, "errorText"}
    };
}

QVariantMap DownloadListModel::get(int row) const
{
    if (row < 0 || row >= m_items.size()) {
        return {};
    }

    return itemToMap(*this, row, false);
}

QVariantMap DownloadListModel::getById(const QString& id) const
{
    const int row = indexOfId(id);
    if (row < 0) {
        return {};
    }
    return itemToMap(*this, row, false);
}

QString DownloadListModel::pieceMapById(const QString& id) const
{
    const int row = indexOfId(id);
    if (row < 0) {
        return {};
    }
    return m_items.at(row).pieceMap;
}

int DownloadListModel::pieceMapRole() const
{
    return PieceMapRole;
}

int DownloadListModel::indexOfId(const QString& id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

int DownloadListModel::count() const
{
    return m_items.size();
}

void DownloadListModel::upsert(const DownloadItem& item)
{
    const int row = indexOfId(item.id);
    if (row < 0) {
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.push_back(item);
        endInsertRows();
        return;
    }

    const auto previous = m_items.at(row);
    QVector<int> changedRoles;
    if (previous.id != item.id) changedRoles.append(IdRole);
    if (previous.name != item.name) changedRoles.append(NameRole);
    if (previous.source != item.source) changedRoles.append(SourceRole);
    if (previous.savePath != item.savePath) changedRoles.append(SavePathRole);
    if (previous.state != item.state) {
        changedRoles.append(StateRole);
        changedRoles.append(StateTextRole);
    }
    if (previous.progress != item.progress) changedRoles.append(ProgressRole);
    if (previous.totalBytes != item.totalBytes) changedRoles.append(TotalBytesRole);
    if (previous.downloadedBytes != item.downloadedBytes) changedRoles.append(DownloadedBytesRole);
    if (previous.downloadRate != item.downloadRate) changedRoles.append(DownloadRateRole);
    if (previous.uploadRate != item.uploadRate) changedRoles.append(UploadRateRole);
    if (previous.isComplete != item.isComplete) changedRoles.append(IsCompleteRole);
    if (previous.pieceCount != item.pieceCount) changedRoles.append(PieceCountRole);
    if (previous.completedPieces != item.completedPieces) changedRoles.append(CompletedPiecesRole);
    if (previous.blockSize != item.blockSize) changedRoles.append(BlockSizeRole);
    if (previous.pieceMap != item.pieceMap) changedRoles.append(PieceMapRole);
    if (previous.ratio != item.ratio) changedRoles.append(RatioRole);
    if (previous.peerCount != item.peerCount) changedRoles.append(PeerCountRole);
    if (previous.seedCount != item.seedCount) changedRoles.append(SeedCountRole);
    if (previous.connectionCount != item.connectionCount) changedRoles.append(ConnectionCountRole);
    if (previous.trackerCount != item.trackerCount) changedRoles.append(TrackerCountRole);
    if (previous.workingTrackerCount != item.workingTrackerCount) changedRoles.append(WorkingTrackerCountRole);
    if (previous.failedTrackerCount != item.failedTrackerCount) changedRoles.append(FailedTrackerCountRole);
    if (previous.dhtStatusText != item.dhtStatusText) changedRoles.append(DhtStatusTextRole);
    if (previous.connectionHealthText != item.connectionHealthText) changedRoles.append(ConnectionHealthTextRole);
    if (previous.statusText != item.statusText) changedRoles.append(StatusTextRole);
    if (previous.errorText != item.errorText) changedRoles.append(ErrorTextRole);

    m_items[row] = item;
    if (!changedRoles.isEmpty()) {
        emit dataChanged(index(row), index(row), changedRoles);
    }
}

void DownloadListModel::removeById(const QString& id)
{
    const int row = indexOfId(id);
    if (row < 0) {
        return;
    }

    beginRemoveRows({}, row, row);
    m_items.removeAt(row);
    endRemoveRows();
}

const QVector<DownloadItem>& DownloadListModel::items() const
{
    return m_items;
}

DownloadItem DownloadListModel::itemById(const QString& id) const
{
    const int row = indexOfId(id);
    return row >= 0 ? m_items.at(row) : DownloadItem {};
}

} // namespace awa::core
