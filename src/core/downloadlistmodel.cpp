#include "awa/core/downloadlistmodel.h"

#include <QVariantMap>

namespace awa::core {

QString stateToString(DownloadState state)
{
    switch (state) {
    case DownloadState::Queued: return QStringLiteral("排队中");
    case DownloadState::FetchingMetadata: return QStringLiteral("获取元数据");
    case DownloadState::Downloading: return QStringLiteral("下载中");
    case DownloadState::Paused: return QStringLiteral("已暂停");
    case DownloadState::Seeding: return QStringLiteral("做种中");
    case DownloadState::Finished: return QStringLiteral("已完成");
    case DownloadState::Error: return QStringLiteral("错误");
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
    case PieceCountRole: return item.pieceCount;
    case CompletedPiecesRole: return item.completedPieces;
    case BlockSizeRole: return item.blockSize;
    case PieceMapRole: return item.pieceMap;
    case RatioRole: return item.ratio;
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
        {PieceCountRole, "pieceCount"},
        {CompletedPiecesRole, "completedPieces"},
        {BlockSizeRole, "blockSize"},
        {PieceMapRole, "pieceMap"},
        {RatioRole, "ratio"},
        {StatusTextRole, "statusText"},
        {ErrorTextRole, "errorText"}
    };
}

QVariantMap DownloadListModel::get(int row) const
{
    QVariantMap result;
    if (row < 0 || row >= m_items.size()) {
        return result;
    }

    const auto roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        result.insert(QString::fromUtf8(it.value()), data(index(row), it.key()));
    }
    return result;
}

QVariantMap DownloadListModel::getById(const QString& id) const
{
    return get(indexOfId(id));
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

    m_items[row] = item;
    emit dataChanged(index(row), index(row), {
        IdRole,
        NameRole,
        SourceRole,
        SavePathRole,
        StateRole,
        StateTextRole,
        ProgressRole,
        TotalBytesRole,
        DownloadedBytesRole,
        DownloadRateRole,
        UploadRateRole,
        PieceCountRole,
        CompletedPiecesRole,
        BlockSizeRole,
        PieceMapRole,
        RatioRole,
        StatusTextRole,
        ErrorTextRole
    });
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
