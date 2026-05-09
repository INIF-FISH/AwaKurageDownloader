# Packaging

Windows is the first-class packaging target.

## MSI

Configure and build with vcpkg:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-msvc
cmake --build build/windows-msvc --target package
```

The expected default English MSI output name is:

```text
AwaKurageDownloader-0.1.1-win64-en-us.msi
```

WiX Toolset must be installed and visible to CPack. The installer installs the application under `Program Files/AwaKurageDownloader` and exposes a normal Windows uninstall entry.

If WiX `candle.exe` and `light.exe` are not available, the same package target falls back to:

```text
AwaKurageDownloader-0.1.1-win64-portable.zip
```

## Portable Build

Portable zip output is reserved for a later packaging pass. The source tree keeps installer-specific files under `packaging/windows/` so adding portable output does not change application code.
