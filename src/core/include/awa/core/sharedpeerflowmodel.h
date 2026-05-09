#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace awa::core {

struct SharedPeerFlow {
    QString downloadId;
    QString taskName;
    QString address;
    int port = 0;
    QString region;
    double latitude = 0.0;
    double longitude = 0.0;
    qint64 downloadRate = 0;
    qint64 uploadRate = 0;
    QString direction;
};

class SharedPeerFlowModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        DownloadIdRole = Qt::UserRole + 1,
        TaskNameRole,
        AddressRole,
        PortRole,
        RegionRole,
        LatitudeRole,
        LongitudeRole,
        DownloadRateRole,
        UploadRateRole,
        DirectionRole,
        RateRole
    };

    explicit SharedPeerFlowModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;
    Q_INVOKABLE int count() const;

    void replaceAll(const QVector<SharedPeerFlow>& flows);

private:
    QVector<SharedPeerFlow> m_flows;
};

} // namespace awa::core

Q_DECLARE_METATYPE(awa::core::SharedPeerFlow)
Q_DECLARE_METATYPE(QVector<awa::core::SharedPeerFlow>)
