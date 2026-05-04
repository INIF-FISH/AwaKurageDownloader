# Windows Packaging

The project uses CPack's WiX generator for the primary Windows MSI.

Planned installer behavior:

- Install into `Program Files/AwaKurageDownloader`.
- Create Start Menu entries.
- Register normal uninstall metadata.
- Preserve user configuration and downloaded data during uninstall.
- Add `.torrent` and `magnet:` associations in a follow-up WiX fragment once product branding and icon assets are finalized.
