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

## License

MIT License

Copyright (c) 2026 AwaKurageDownloader Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
