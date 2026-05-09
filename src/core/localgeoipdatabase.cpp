#include "awa/core/localgeoipdatabase.h"

#include <QCoreApplication>
#include <QAbstractSocket>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>

namespace awa::core {
namespace {
struct CountryCenter {
    const char* code;
    const char* name;
    double latitude;
    double longitude;
};

constexpr CountryCenter CountryCenters[] = {
    {"AD", "Andorra", 42.5, 1.6}, {"AE", "United Arab Emirates", 24.4, 54.4},
    {"AF", "Afghanistan", 34.5, 69.2}, {"AL", "Albania", 41.3, 19.8},
    {"AM", "Armenia", 40.2, 44.5}, {"AR", "Argentina", -34.6, -58.4},
    {"AT", "Austria", 48.2, 16.4}, {"AU", "Australia", -35.3, 149.1},
    {"AZ", "Azerbaijan", 40.4, 49.9}, {"BA", "Bosnia and Herzegovina", 43.9, 18.4},
    {"BD", "Bangladesh", 23.8, 90.4}, {"BE", "Belgium", 50.8, 4.4},
    {"BG", "Bulgaria", 42.7, 23.3}, {"BH", "Bahrain", 26.2, 50.6},
    {"BO", "Bolivia", -16.5, -68.1}, {"BR", "Brazil", -15.8, -47.9},
    {"BY", "Belarus", 53.9, 27.6}, {"CA", "Canada", 45.4, -75.7},
    {"CH", "Switzerland", 46.9, 7.4}, {"CL", "Chile", -33.4, -70.7},
    {"CN", "China", 39.9, 116.4}, {"CO", "Colombia", 4.7, -74.1},
    {"CR", "Costa Rica", 9.9, -84.1}, {"CZ", "Czechia", 50.1, 14.4},
    {"DE", "Germany", 52.5, 13.4}, {"DK", "Denmark", 55.7, 12.6},
    {"DO", "Dominican Republic", 18.5, -69.9}, {"DZ", "Algeria", 36.8, 3.1},
    {"EC", "Ecuador", -0.2, -78.5}, {"EE", "Estonia", 59.4, 24.7},
    {"EG", "Egypt", 30.0, 31.2}, {"ES", "Spain", 40.4, -3.7},
    {"FI", "Finland", 60.2, 24.9}, {"FR", "France", 48.9, 2.4},
    {"GB", "United Kingdom", 51.5, -0.1}, {"GE", "Georgia", 41.7, 44.8},
    {"GR", "Greece", 38.0, 23.7}, {"HK", "Hong Kong", 22.3, 114.2},
    {"HR", "Croatia", 45.8, 16.0}, {"HU", "Hungary", 47.5, 19.0},
    {"ID", "Indonesia", -6.2, 106.8}, {"IE", "Ireland", 53.3, -6.3},
    {"IL", "Israel", 31.8, 35.2}, {"IN", "India", 28.6, 77.2},
    {"IQ", "Iraq", 33.3, 44.4}, {"IR", "Iran", 35.7, 51.4},
    {"IS", "Iceland", 64.1, -21.9}, {"IT", "Italy", 41.9, 12.5},
    {"JP", "Japan", 35.7, 139.7}, {"KR", "South Korea", 37.6, 127.0},
    {"KZ", "Kazakhstan", 51.2, 71.4}, {"LT", "Lithuania", 54.7, 25.3},
    {"LU", "Luxembourg", 49.6, 6.1}, {"LV", "Latvia", 56.9, 24.1},
    {"MA", "Morocco", 34.0, -6.8}, {"MX", "Mexico", 19.4, -99.1},
    {"MY", "Malaysia", 3.1, 101.7}, {"NG", "Nigeria", 9.1, 7.5},
    {"NL", "Netherlands", 52.4, 4.9}, {"NO", "Norway", 59.9, 10.8},
    {"NZ", "New Zealand", -41.3, 174.8}, {"PE", "Peru", -12.0, -77.0},
    {"PH", "Philippines", 14.6, 121.0}, {"PK", "Pakistan", 33.7, 73.1},
    {"PL", "Poland", 52.2, 21.0}, {"PT", "Portugal", 38.7, -9.1},
    {"RO", "Romania", 44.4, 26.1}, {"RS", "Serbia", 44.8, 20.5},
    {"RU", "Russia", 55.8, 37.6}, {"SA", "Saudi Arabia", 24.7, 46.7},
    {"SE", "Sweden", 59.3, 18.1}, {"SG", "Singapore", 1.35, 103.8},
    {"SK", "Slovakia", 48.1, 17.1}, {"TH", "Thailand", 13.8, 100.5},
    {"TR", "Turkey", 39.9, 32.9}, {"TW", "Taiwan", 25.0, 121.5},
    {"UA", "Ukraine", 50.5, 30.5}, {"US", "United States", 39.8, -98.6},
    {"UY", "Uruguay", -34.9, -56.2}, {"VN", "Vietnam", 21.0, 105.8},
    {"ZA", "South Africa", -25.7, 28.2}
};

QStringList parseCsvLine(const QString& line)
{
    QStringList fields;
    QString field;
    bool quoted = false;

    for (int i = 0; i < line.size(); ++i) {
        const QChar ch = line.at(i);
        if (ch == QLatin1Char('"')) {
            if (quoted && i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
                field.append(ch);
                ++i;
            } else {
                quoted = !quoted;
            }
        } else if (ch == QLatin1Char(',') && !quoted) {
            fields.append(field.trimmed());
            field.clear();
        } else {
            field.append(ch);
        }
    }

    fields.append(field.trimmed());
    return fields;
}

bool ipv4ToInt(const QString& text, quint32* value)
{
    QHostAddress address(text);
    if (address.protocol() != QAbstractSocket::IPv4Protocol) {
        bool ok = false;
        const quint64 parsed = text.toULongLong(&ok);
        if (!ok || parsed > 0xffffffffULL) {
            return false;
        }
        *value = static_cast<quint32>(parsed);
        return true;
    }

    *value = address.toIPv4Address();
    return true;
}

bool numberAt(const QStringList& fields, int index, double* value)
{
    if (index < 0 || index >= fields.size()) {
        return false;
    }

    bool ok = false;
    const double parsed = fields.at(index).toDouble(&ok);
    if (!ok) {
        return false;
    }

    *value = parsed;
    return true;
}

QString joinedRegion(const QStringList& fields, int first, int last)
{
    QStringList parts;
    for (int i = first; i <= last && i < fields.size(); ++i) {
        const QString part = fields.at(i).trimmed();
        if (!part.isEmpty() && part != QStringLiteral("-")) {
            parts.append(part);
        }
    }
    return parts.join(QStringLiteral(", "));
}

bool readVarint(const QByteArray& data, qsizetype* offset, quint64* value, qsizetype limit)
{
    quint64 result = 0;
    int shift = 0;
    while (*offset < limit && shift <= 63) {
        const quint8 byte = static_cast<quint8>(data.at((*offset)++));
        result |= static_cast<quint64>(byte & 0x7f) << shift;
        if ((byte & 0x80) == 0) {
            *value = result;
            return true;
        }
        shift += 7;
    }
    return false;
}

bool skipField(const QByteArray& data, qsizetype* offset, quint64 tag, qsizetype limit)
{
    const int wireType = static_cast<int>(tag & 0x7);
    quint64 length = 0;
    switch (wireType) {
    case 0:
        return readVarint(data, offset, &length, limit);
    case 1:
        *offset += 8;
        return *offset <= limit;
    case 2:
        if (!readVarint(data, offset, &length, limit)) return false;
        *offset += static_cast<qsizetype>(length);
        return *offset <= limit;
    case 5:
        *offset += 4;
        return *offset <= limit;
    default:
        return false;
    }
}

bool centerForCountry(const QString& code, QString* name, double* latitude, double* longitude)
{
    for (const auto& center : CountryCenters) {
        if (code == QString::fromLatin1(center.code)) {
            *name = QString::fromLatin1(center.name);
            *latitude = center.latitude;
            *longitude = center.longitude;
            return true;
        }
    }
    return false;
}

bool parseV2rayCidr(const QByteArray& data, qsizetype first, qsizetype limit, quint32* network, int* prefix)
{
    qsizetype offset = first;
    QByteArray ip;
    int parsedPrefix = -1;

    while (offset < limit) {
        quint64 tag = 0;
        if (!readVarint(data, &offset, &tag, limit)) {
            return false;
        }
        const int field = static_cast<int>(tag >> 3);
        const int wireType = static_cast<int>(tag & 0x7);
        if (field == 1 && wireType == 2) {
            quint64 length = 0;
            if (!readVarint(data, &offset, &length, limit)) return false;
            if (length != 4 || offset + static_cast<qsizetype>(length) > limit) return false;
            ip = data.mid(offset, static_cast<qsizetype>(length));
            offset += static_cast<qsizetype>(length);
        } else if (field == 2 && wireType == 0) {
            quint64 value = 0;
            if (!readVarint(data, &offset, &value, limit)) return false;
            parsedPrefix = static_cast<int>(value);
        } else if (!skipField(data, &offset, tag, limit)) {
            return false;
        }
    }

    if (ip.size() != 4 || parsedPrefix < 0 || parsedPrefix > 32) {
        return false;
    }

    *network = (static_cast<quint32>(static_cast<quint8>(ip.at(0))) << 24)
        | (static_cast<quint32>(static_cast<quint8>(ip.at(1))) << 16)
        | (static_cast<quint32>(static_cast<quint8>(ip.at(2))) << 8)
        | static_cast<quint32>(static_cast<quint8>(ip.at(3)));
    *prefix = parsedPrefix;
    return true;
}
} // namespace

bool LocalGeoIpDatabase::ensureLoaded()
{
    if (m_loadAttempted) {
        return !m_ranges.isEmpty();
    }

    m_loadAttempted = true;
    const auto paths = candidatePaths();
    for (const auto& path : paths) {
    if (loadFromFile(path)) {
            m_loadedPath = path;
            return true;
        }
    }

    return false;
}

LocalGeoIpLocation LocalGeoIpDatabase::lookup(const QString& addressText)
{
    if (!ensureLoaded()) {
        return {};
    }

    quint32 address = 0;
    if (!ipv4ToInt(addressText, &address)) {
        return {};
    }

    int left = 0;
    int right = m_ranges.size() - 1;
    while (left <= right) {
        const int mid = left + (right - left) / 2;
        const auto& range = m_ranges.at(mid);
        if (address < range.first) {
            right = mid - 1;
        } else if (address > range.last) {
            left = mid + 1;
        } else {
            return {range.region, range.latitude, range.longitude, true};
        }
    }

    return {};
}

QString LocalGeoIpDatabase::loadedPath() const
{
    return m_loadedPath;
}

bool LocalGeoIpDatabase::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QByteArray content = file.readAll();
    QVector<Range> ranges;
    if (content.contains('\0') || (!content.isEmpty() && static_cast<quint8>(content.at(0)) < 9)) {
        qsizetype offset = 0;
        const qsizetype limit = content.size();
        while (offset < limit) {
            quint64 tag = 0;
            if (!readVarint(content, &offset, &tag, limit)) {
                break;
            }
            const int field = static_cast<int>(tag >> 3);
            const int wireType = static_cast<int>(tag & 0x7);
            if (field != 1 || wireType != 2) {
                if (!skipField(content, &offset, tag, limit)) {
                    break;
                }
                continue;
            }

            quint64 geoLength = 0;
            if (!readVarint(content, &offset, &geoLength, limit)) {
                break;
            }
            const qsizetype geoEnd = offset + static_cast<qsizetype>(geoLength);
            if (geoEnd > limit) {
                break;
            }

            QString countryCode;
            QVector<QPair<quint32, int>> cidrs;
            while (offset < geoEnd) {
                quint64 geoTag = 0;
                if (!readVarint(content, &offset, &geoTag, geoEnd)) {
                    break;
                }
                const int geoField = static_cast<int>(geoTag >> 3);
                const int geoWireType = static_cast<int>(geoTag & 0x7);
                if (geoField == 1 && geoWireType == 2) {
                    quint64 length = 0;
                    if (!readVarint(content, &offset, &length, geoEnd)) break;
                    if (offset + static_cast<qsizetype>(length) > geoEnd) break;
                    countryCode = QString::fromLatin1(content.mid(offset, static_cast<qsizetype>(length))).toUpper();
                    offset += static_cast<qsizetype>(length);
                } else if (geoField == 2 && geoWireType == 2) {
                    quint64 length = 0;
                    if (!readVarint(content, &offset, &length, geoEnd)) break;
                    const qsizetype cidrEnd = offset + static_cast<qsizetype>(length);
                    quint32 network = 0;
                    int prefix = -1;
                    if (cidrEnd <= geoEnd && parseV2rayCidr(content, offset, cidrEnd, &network, &prefix)) {
                        cidrs.append({network, prefix});
                    }
                    offset = cidrEnd;
                } else if (!skipField(content, &offset, geoTag, geoEnd)) {
                    break;
                }
            }
            offset = geoEnd;

            QString region;
            double latitude = 0.0;
            double longitude = 0.0;
            if (!centerForCountry(countryCode, &region, &latitude, &longitude)) {
                continue;
            }

            for (const auto& cidr : cidrs) {
                const int prefix = cidr.second;
                const quint32 mask = prefix == 0 ? 0u : (0xffffffffu << (32 - prefix));
                const quint32 first = cidr.first & mask;
                const quint32 last = first | ~mask;
                ranges.append({first, last, latitude, longitude, region});
            }
        }
    } else {
        QString text = QString::fromUtf8(content);
        QTextStream stream(&text);
        while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }

        const auto fields = parseCsvLine(line);
        if (fields.size() < 5 || fields.first().contains(QStringLiteral("start"), Qt::CaseInsensitive)) {
            continue;
        }

        quint32 first = 0;
        quint32 last = 0;
        if (!ipv4ToInt(fields.at(0), &first) || !ipv4ToInt(fields.at(1), &last) || first > last) {
            continue;
        }

        double latitude = 0.0;
        double longitude = 0.0;
        QString region;

        if (fields.size() >= 9 && numberAt(fields, 7, &latitude) && numberAt(fields, 8, &longitude)) {
            region = joinedRegion(fields, 2, 5);
        } else if (fields.size() >= 5
            && numberAt(fields, fields.size() - 2, &latitude)
            && numberAt(fields, fields.size() - 1, &longitude)) {
            region = joinedRegion(fields, 2, fields.size() - 3);
        } else {
            continue;
        }

        if (region.isEmpty()) {
            region = QStringLiteral("Unknown");
        }
        if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0) {
            continue;
        }

        ranges.append({first, last, latitude, longitude, region});
        }
    }

    if (ranges.isEmpty()) {
        return false;
    }

    std::sort(ranges.begin(), ranges.end(), [](const auto& left, const auto& right) {
        return left.first < right.first;
    });
    m_ranges = ranges;
    return true;
}

QStringList LocalGeoIpDatabase::candidatePaths() const
{
    return {
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("resources/geoip.dat")),
        QDir::current().filePath(QStringLiteral("resources/geoip.dat"))
    };
}

} // namespace awa::core
