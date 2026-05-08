#pragma once

#include "awa/core/downloaditem.h"

namespace awa::torrent {

enum class PauseOwner {
    None,
    User,
    Scheduler
};

struct TaskStateInput {
    awa::core::DownloadState previousState = awa::core::DownloadState::Queued;
    PauseOwner pauseOwner = PauseOwner::None;
    bool priorityPausedSeed = false;
    bool seedOnCompletionEnabled = true;
    bool hasReliableLiveState = false;
    bool livePaused = false;
    bool hasMetadata = false;
    bool complete = false;
    bool seeding = false;
};

struct TaskStateResult {
    awa::core::DownloadState state = awa::core::DownloadState::Queued;
    bool isComplete = false;
};

class TaskStateMachine {
public:
    static TaskStateResult resolve(const TaskStateInput& input)
    {
        auto result = [&input](awa::core::DownloadState state) {
            return TaskStateResult {
                state,
                input.complete || isCompletedState(state)
            };
        };

        if (input.pauseOwner == PauseOwner::User) {
            return result(input.complete || input.seeding || shouldPauseAsSeeding(input.previousState)
                ? awa::core::DownloadState::PausedSeeding
                : awa::core::DownloadState::PausedDownloading);
        }

        if (input.priorityPausedSeed) {
            return result(input.seedOnCompletionEnabled
                ? awa::core::DownloadState::Seeding
                : awa::core::DownloadState::Finished);
        }

        if (input.complete && !input.seedOnCompletionEnabled) {
            return result(awa::core::DownloadState::Finished);
        }
        if (input.complete
            && isCompletedState(input.previousState)
            && !isPausedState(input.previousState)) {
            return result(input.previousState);
        }
        if (input.complete) {
            return result(input.seedOnCompletionEnabled
                ? awa::core::DownloadState::Seeding
                : awa::core::DownloadState::Finished);
        }

        if (input.pauseOwner == PauseOwner::Scheduler) {
            return result(awa::core::DownloadState::Waiting);
        }

        if (isCompletedState(input.previousState)) {
            return result(input.previousState);
        }

        if (!input.hasReliableLiveState && isCompletedState(input.previousState)) {
            return result(input.previousState);
        }

        if (input.livePaused) {
            if (!input.hasMetadata) {
                return result(awa::core::DownloadState::PausedDownloading);
            }
            return result(input.complete || input.seeding
                ? awa::core::DownloadState::PausedSeeding
                : awa::core::DownloadState::Waiting);
        }

        if (!input.hasMetadata) {
            return result(awa::core::DownloadState::FetchingMetadata);
        }

        if (input.seeding) {
            return result(awa::core::DownloadState::Seeding);
        }

        return result(awa::core::DownloadState::Downloading);
    }

private:
    static bool isPausedState(awa::core::DownloadState state)
    {
        return state == awa::core::DownloadState::PausedDownloading
            || state == awa::core::DownloadState::PausedSeeding;
    }

    static bool isCompletedState(awa::core::DownloadState state)
    {
        return state == awa::core::DownloadState::Finished
            || state == awa::core::DownloadState::Seeding
            || state == awa::core::DownloadState::PausedSeeding;
    }

    static bool shouldPauseAsSeeding(awa::core::DownloadState state)
    {
        return state == awa::core::DownloadState::Seeding
            || state == awa::core::DownloadState::Finished
            || state == awa::core::DownloadState::PausedSeeding;
    }
};

} // namespace awa::torrent
