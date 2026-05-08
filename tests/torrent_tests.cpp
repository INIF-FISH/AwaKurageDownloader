#include "awa/torrent/magnetutils.h"
#include "awa/torrent/taskstate.h"

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

TEST_CASE("Task state keeps stopped completed tasks complete when live status is incomplete")
{
    awa::torrent::TaskStateInput input;
    input.previousState = awa::core::DownloadState::Finished;
    input.seedOnCompletionEnabled = false;
    input.hasReliableLiveState = true;
    input.hasMetadata = true;
    input.complete = true;
    input.seeding = false;

    const auto result = awa::torrent::TaskStateMachine::resolve(input);

    REQUIRE(result.state == awa::core::DownloadState::Finished);
    REQUIRE(result.isComplete);
}

TEST_CASE("Task state keeps user paused seeding complete and resumable")
{
    awa::torrent::TaskStateInput input;
    input.previousState = awa::core::DownloadState::PausedSeeding;
    input.pauseOwner = awa::torrent::PauseOwner::User;
    input.seedOnCompletionEnabled = true;
    input.hasReliableLiveState = true;
    input.livePaused = true;
    input.hasMetadata = true;
    input.complete = true;
    input.seeding = false;

    const auto result = awa::torrent::TaskStateMachine::resolve(input);

    REQUIRE(result.state == awa::core::DownloadState::PausedSeeding);
    REQUIRE(result.isComplete);
}

TEST_CASE("Task state lets user pause override automatic seed priority pause")
{
    awa::torrent::TaskStateInput input;
    input.previousState = awa::core::DownloadState::PausedSeeding;
    input.pauseOwner = awa::torrent::PauseOwner::User;
    input.priorityPausedSeed = true;
    input.seedOnCompletionEnabled = true;
    input.hasReliableLiveState = true;
    input.livePaused = true;
    input.hasMetadata = true;
    input.complete = true;
    input.seeding = false;

    const auto result = awa::torrent::TaskStateMachine::resolve(input);

    REQUIRE(result.state == awa::core::DownloadState::PausedSeeding);
    REQUIRE(result.isComplete);
}

TEST_CASE("Task state keeps user paused incomplete downloads paused")
{
    awa::torrent::TaskStateInput input;
    input.previousState = awa::core::DownloadState::PausedDownloading;
    input.pauseOwner = awa::torrent::PauseOwner::User;
    input.seedOnCompletionEnabled = true;
    input.hasReliableLiveState = true;
    input.livePaused = true;
    input.hasMetadata = true;
    input.complete = false;
    input.seeding = false;

    const auto result = awa::torrent::TaskStateMachine::resolve(input);

    REQUIRE(result.state == awa::core::DownloadState::PausedDownloading);
    REQUIRE_FALSE(result.isComplete);
}

TEST_CASE("Task state uses persisted completion even when live progress is absent")
{
    awa::torrent::TaskStateInput input;
    input.previousState = awa::core::DownloadState::Downloading;
    input.seedOnCompletionEnabled = true;
    input.hasReliableLiveState = true;
    input.hasMetadata = true;
    input.complete = true;
    input.seeding = false;

    const auto result = awa::torrent::TaskStateMachine::resolve(input);

    REQUIRE(result.state == awa::core::DownloadState::Seeding);
    REQUIRE(result.isComplete);
}

TEST_CASE("Task state does not let scheduler waiting override completion")
{
    awa::torrent::TaskStateInput input;
    input.previousState = awa::core::DownloadState::Waiting;
    input.pauseOwner = awa::torrent::PauseOwner::Scheduler;
    input.seedOnCompletionEnabled = true;
    input.hasReliableLiveState = true;
    input.hasMetadata = true;
    input.complete = true;
    input.seeding = false;

    const auto result = awa::torrent::TaskStateMachine::resolve(input);

    REQUIRE(result.state == awa::core::DownloadState::Seeding);
    REQUIRE(result.isComplete);
}
