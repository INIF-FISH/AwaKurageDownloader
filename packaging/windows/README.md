# Windows Packaging

The project uses CPack's WiX generator for the primary Windows MSI.

The installer defaults to English. Language can be selected at configure time with `-DAWA_WIX_CULTURE=en-us`, `zh-cn`, or `ja-jp`.

Planned installer behavior:

- Install into `Program Files/AwaKurageDownloader`.
- Create one Start Menu shortcut named as the main app.
- Register normal uninstall metadata.
- Preserve user configuration and downloaded data during uninstall.
- Add `.torrent` and `magnet:` associations in a follow-up WiX fragment once product branding and icon assets are finalized.
