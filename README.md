# AwaKurageDownloader

AwaKurageDownloader 是一个基于 Qt 6 和 libtorrent 的桌面 BitTorrent 下载器。它希望把常用的下载、订阅、状态查看和本地自动化能力放在一个清爽的界面里：能手动添加任务，也能挂着跑；能看见速度和连接状态，也能通过本地 API 接到自己的小工具里。

项目名里的 “AwaKurage” 来自日语里的水母。水母看起来轻轻的，但触手很会抓住该抓住的东西。这个下载器也想做成这样：安静、稳定、不过度打扰你。

## 现在能做什么

- 添加 Magnet 链接和 `.torrent` 文件下载任务。
- 查看任务进度、下载/上传速度、状态和分片信息。
- 暂停、继续、移除任务，并设置默认保存目录。
- 配置下载/上传限速、上传槽位、做种行为和 Tracker 列表。
- 添加 RSS 订阅，并按服务逻辑自动匹配下载内容。
- 启动仅监听 `127.0.0.1` 的本地 HTTP / WebSocket API，方便和脚本、面板或其他本地工具集成。
- 提供 Windows MSI 打包流程；没有 WiX 时会回退到便携 zip 包。

## 项目状态

当前版本是 `0.1.1`，仍处在早期开发阶段。核心结构已经拆分完成，桌面端、BT 会话、RSS 服务、本地 API 和设置管理都在独立模块中维护，但功能细节和界面文本仍会继续打磨。

如果你只是想试用，请把它当作一个正在成长中的下载器；如果你想参与开发，欢迎从小问题、构建体验和文档开始。

## 技术栈

- C++20
- Qt 6.6+
- Qt Quick / QML
- libtorrent
- spdlog
- CMake + vcpkg

## 项目结构

```text
apps/desktop/       Qt Quick 桌面应用入口和 QML 界面
src/core/           下载任务模型、设置服务和 DownloadManager
src/torrent/        libtorrent 会话封装、Magnet 工具和任务状态映射
src/rss/            RSS 订阅、刷新和自动添加逻辑
src/api/            本地 HTTP / WebSocket API
docs/               架构和打包说明
packaging/windows/  Windows 安装包相关文件
resources/          图标、海报和 Qt 资源
```

更细的模块说明可以看 [docs/architecture.md](docs/architecture.md)。

## 构建准备

你需要先安装：

- Visual Studio 2022 或其他支持 C++20 的编译器
- CMake 3.25+
- vcpkg
- Qt 6.6+ 相关依赖会通过 vcpkg 解析

确保环境变量 `VCPKG_ROOT` 指向你的 vcpkg 目录，例如：

```powershell
$env:VCPKG_ROOT = "C:\dev\vcpkg"
```

## 构建

Windows / MSVC：

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc
```

通用 Ninja 预设：

```powershell
cmake --preset default
cmake --build --preset default
```

## 打包

Windows 是当前优先支持的打包平台：

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc
cmake --build build/windows-msvc --target package
```

安装包默认输出类似：

```text
AwaKurageDownloader-0.1.1-win64.msi
```

如果系统里没有可用的 WiX Toolset，打包流程会回退生成：

```text
AwaKurageDownloader-0.1.1-win64-portable.zip
```

更多细节见 [docs/packaging.md](docs/packaging.md)。

## 本地 API

应用内可以启动本地 API 服务。它默认只监听 `127.0.0.1`，HTTP 请求需要携带 Bearer Token，WebSocket 会推送下载列表快照和状态变化。

这个 API 面向本机自动化场景，例如：

- 写一个脚本批量添加下载任务。
- 做一个本地状态面板。
- 把下载器接入自己的工作流工具。

具体接口仍在迭代中，使用前建议先看应用内的 “远程 API” 页面。

## 数据与卸载

卸载应用不会主动删除用户下载内容和配置数据。任务持久化和设置由 `SettingsService` 管理，后续如果引入 SQLite 迁移，也会尽量保持数据策略清晰、可预期。

## 贡献

欢迎提交 Issue 或 Pull Request。比较适合入手的方向包括：

- 修复界面文案、交互和小的可用性问题。
- 补充构建、打包和运行文档。
- 改进 Tracker、DHT、连接调度和状态展示。
- 完善 RSS 和本地 API 的真实使用场景。


## License

本项目使用仓库中的 [LICENSE.txt](LICENSE.txt) 所声明的许可证。
