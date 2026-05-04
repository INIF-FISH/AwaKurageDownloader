#include "awa/torrent/magnetutils.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Base32 BTIH magnet links are normalized to hex")
{
    const auto magnet = QStringLiteral("magnet:?xt=urn:btih:YQS3PJ7OH7B6Z6S3UE6HMJH7AYBR53R5");
    const auto normalized = awa::torrent::normalizeMagnetUri(magnet);

    REQUIRE(normalized.startsWith(QStringLiteral("magnet:?xt=urn:btih:")));
    const auto hash = normalized.section(QStringLiteral("btih:"), 1).section(QLatin1Char('&'), 0, 0);
    REQUIRE(hash.size() == 40);
    REQUIRE(hash == hash.toLower());
    REQUIRE(hash != QStringLiteral("YQS3PJ7OH7B6Z6S3UE6HMJH7AYBR53R5"));
}
