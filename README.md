# AwaKurageDownloader

AwaKurageDownloader is a C++20/CMake BitTorrent desktop downloader with a Qt Quick UI, libtorrent backend, RSS automation, local control API, and Windows MSI packaging.

## Features

- Add `.torrent` files and magnet links.
- Pause, resume, remove, and inspect download tasks.
- libtorrent-backed session with DHT, LSD, UPnP, NAT-PMP, uTP, and tracker alerts.
- RSS subscription service with rule matching and opt-in auto-download.
- Local loopback HTTP/WebSocket API.
- Modern Qt Quick interface with task animations and drag-and-drop.
- CPack/WiX packaging path for Windows MSI output.

## Build

Install Visual Studio 2022, CMake, vcpkg, Qt dependencies through vcpkg, and WiX Toolset for packaging.

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc --config RelWithDebInfo
ctest --preset default
cmake --build build/windows-msvc --target package --config RelWithDebInfo
```

The first configure can take a long time because Qt, Boost, and libtorrent are resolved by vcpkg.

## Layout

- `apps/desktop`: Qt Quick desktop shell and QML.
- `src/core`: download models, settings, and facade.
- `src/torrent`: libtorrent session adapter.
- `src/rss`: RSS feed refresh and matching.
- `src/api`: local HTTP/WebSocket API.
- `packaging/windows`: Windows packaging notes.
- `docs`: architecture and packaging notes.
- `tests`: Catch2 tests.
