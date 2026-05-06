#pragma once

#include "awa/core/downloaditem.h"

#include <QAbstractListModel>
#include <QVector>

namespace awa::core {

class DownloadListModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
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
        PeerCountRole,
        SeedCountRole,
        ConnectionCountRole,
        TrackerCountRole,
        WorkingTrackerCountRole,
        FailedTrackerCountRole,
        DhtStatusTextRole,
        ConnectionHealthTextRole,
        StatusTextRole,
        ErrorTextRole
    };

    explicit DownloadListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE QVariantMap getById(const QString& id) const;
    Q_INVOKABLE QString pieceMapById(const QString& id) const;
    Q_INVOKABLE int pieceMapRole() const;
    Q_INVOKABLE int indexOfId(const QString& id) const;
    Q_INVOKABLE int count() const;

    void upsert(const DownloadItem& item);
    void removeById(const QString& id);
    const QVector<DownloadItem>& items() const;
    DownloadItem itemById(const QString& id) const;

private:
    QVector<DownloadItem> m_items;
};

} // namespace awa::core
