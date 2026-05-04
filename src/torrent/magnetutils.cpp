#include "awa/torrent/magnetutils.h"

#include <QRegularExpression>

namespace awa::torrent {
namespace {

int base32Value(QChar ch)
{
    const auto c = ch.toLatin1();
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a';
    }
    if (c >= '2' && c <= '7') {
        return 26 + (c - '2');
    }
    return -1;
}

QString base32BtihToHex(const QString& value)
{
    quint32 buffer = 0;
    int bitsLeft = 0;
    QByteArray bytes;
    bytes.reserve(20);

    for (const auto ch : value) {
        if (ch == QLatin1Char('=')) {
            break;
        }

        const int decoded = base32Value(ch);
        if (decoded < 0) {
            return {};
        }

        buffer = (buffer << 5) | static_cast<quint32>(decoded);
        bitsLeft += 5;

        while (bitsLeft >= 8) {
            bitsLeft -= 8;
            bytes.append(static_cast<char>((buffer >> bitsLeft) & 0xff));
        }
    }

    if (bytes.size() != 20) {
        return {};
    }
    return QString::fromLatin1(bytes.toHex());
}

bool isHexBtih(const QString& value)
{
    static const QRegularExpression re(QStringLiteral("^[0-9a-fA-F]{40}$"));
    return re.match(value).hasMatch();
}

bool isBase32Btih(const QString& value)
{
    static const QRegularExpression re(QStringLiteral("^[A-Z2-7a-z]{32}$"));
    return re.match(value).hasMatch();
}

} // namespace

QString normalizeMagnetUri(const QString& uri)
{
    static const QRegularExpression btihRe(QStringLiteral("(xt=urn:btih:)([^&]+)"));
    const auto match = btihRe.match(uri);
    if (!match.hasMatch()) {
        return uri;
    }

    const QString hash = match.captured(2);
    if (isHexBtih(hash)) {
        return uri;
    }
    if (!isBase32Btih(hash)) {
        return uri;
    }

    const QString hex = base32BtihToHex(hash);
    if (hex.isEmpty()) {
        return uri;
    }

    QString normalized = uri;
    normalized.replace(match.capturedStart(2), match.capturedLength(2), hex);
    return normalized;
}

} // namespace awa::torrent
