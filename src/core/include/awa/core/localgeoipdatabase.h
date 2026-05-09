#pragma once

#include <QVector>
#include <QString>

namespace awa::core {

struct LocalGeoIpLocation {
    QString region;
    double latitude = 0.0;
    double longitude = 0.0;
    bool valid = false;
};

class LocalGeoIpDatabase final {
public:
    bool ensureLoaded();
    LocalGeoIpLocation lookup(const QString& address);
    QString loadedPath() const;

private:
    struct Range {
        quint32 first = 0;
        quint32 last = 0;
        double latitude = 0.0;
        double longitude = 0.0;
        QString region;
    };

    bool loadFromFile(const QString& path);
    QStringList candidatePaths() const;

    QVector<Range> m_ranges;
    QString m_loadedPath;
    bool m_loadAttempted = false;
};

} // namespace awa::core
