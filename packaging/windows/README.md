# Windows Packaging

The project uses CPack's WiX generator for the primary Windows MSI.

Installer language is selected at configure time with `-DAWA_WIX_CULTURE=zh-cn`, `en-us`, or `ja-jp`.

Planned installer behavior:

- Install into `Program Files/AwaKurageDownloader`.
- Create Start Menu entries.
- Register normal uninstall metadata.
- Preserve user configuration and downloaded data during uninstall.
- Add `.torrent` and `magnet:` associations in a follow-up WiX fragment once product branding and icon assets are finalized.
