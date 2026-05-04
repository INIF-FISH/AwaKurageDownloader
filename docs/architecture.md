# AwaKurageDownloader Architecture

AwaKurageDownloader is split into small CMake targets so the UI, local API, RSS automation, and BitTorrent session do not depend on each other's internals.

## Targets

- `awa_core`: task model, settings, facade, and interfaces.
- `awa_torrent`: libtorrent session ownership and torrent state mapping.
- `awa_rss`: feed fetching, matching, deduplication, and optional auto-add.
- `awa_api`: loopback HTTP/WebSocket control surface.
- `AwaKurageDownloader`: Qt Quick desktop application.

## Runtime Flow

The desktop, RSS service, and local API all call `DownloadManager`. `DownloadManager` forwards torrent actions to the `TorrentBackend` interface. `TorrentService` owns the single libtorrent session, pumps alerts, samples status, and emits normalized `DownloadItem` updates back to `DownloadListModel`.

The local API listens on `127.0.0.1` only. HTTP uses a bearer token, and WebSocket clients receive download list snapshots.

## Data Policy

User downloads and configuration are not deleted during uninstall. Task persistence is intentionally isolated behind `SettingsService` and future SQLite migrations so packaging changes do not affect user data.
