#include "awa/core/sharedpeerflowmodel.h"

#include <QVariantMap>

#include <algorithm>

namespace awa::core {

namespace {
qint64 flowRate(const SharedPeerFlow& flow)
{
    return flow.direction == QStringLiteral("upload") ? flow.uploadRate : flow.downloadRate;
}
} // namespace

SharedPeerFlowModel::SharedPeerFlowModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int SharedPeerFlowModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_flows.size();
}

QVariant SharedPeerFlowModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_flows.size()) {
        return {};
    }

    const auto& flow = m_flows.at(index.row());
    switch (role) {
    case DownloadIdRole: return flow.downloadId;
    case TaskNameRole: return flow.taskName;
    case AddressRole: return flow.address;
    case PortRole: return flow.port;
    case RegionRole: return flow.region;
    case LatitudeRole: return flow.latitude;
    case LongitudeRole: return flow.longitude;
    case DownloadRateRole: return flow.downloadRate;
    case UploadRateRole: return flow.uploadRate;
    case DirectionRole: return flow.direction;
    case RateRole: return flowRate(flow);
    default: return {};
    }
}

QHash<int, QByteArray> SharedPeerFlowModel::roleNames() const
{
    return {
        {DownloadIdRole, "downloadId"},
        {TaskNameRole, "taskName"},
        {AddressRole, "address"},
        {PortRole, "port"},
        {RegionRole, "region"},
        {LatitudeRole, "latitude"},
        {LongitudeRole, "longitude"},
        {DownloadRateRole, "downloadRate"},
        {UploadRateRole, "uploadRate"},
        {DirectionRole, "direction"},
        {RateRole, "rate"}
    };
}

QVariantMap SharedPeerFlowModel::get(int row) const
{
    if (row < 0 || row >= m_flows.size()) {
        return {};
    }

    QVariantMap result;
    const auto roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        result.insert(QString::fromUtf8(it.value()), data(index(row), it.key()));
    }
    return result;
}

int SharedPeerFlowModel::count() const
{
    return m_flows.size();
}

void SharedPeerFlowModel::replaceAll(const QVector<SharedPeerFlow>& flows)
{
    const int oldSize = m_flows.size();
    const int newSize = flows.size();
    const int commonSize = std::min(oldSize, newSize);

    if (newSize < oldSize) {
        beginRemoveRows({}, newSize, oldSize - 1);
        m_flows.resize(newSize);
        endRemoveRows();
    }

    if (commonSize > 0) {
        std::copy(flows.cbegin(), flows.cbegin() + commonSize, m_flows.begin());
        emit dataChanged(index(0), index(commonSize - 1));
    }

    if (newSize > oldSize) {
        beginInsertRows({}, oldSize, newSize - 1);
        m_flows.reserve(newSize);
        for (int i = oldSize; i < newSize; ++i) {
            m_flows.append(flows.at(i));
        }
        endInsertRows();
    }
}

} // namespace awa::core
