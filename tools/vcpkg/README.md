# Third-party DLL build with vcpkg

This directory contains a small vcpkg manifest and helper script for building the
runtime DLLs that are not stored in the repository but are needed by the current
x64 distribution:

- `unrar.dll` for the UnRAR plugin
- `libeay32.dll` and `ssleay32.dll` for the FTP plugin's legacy OpenSSL loader

The script pins the vcpkg registry baseline and package versions so the produced
inputs are reproducible:

- `unrar` `7.2.6`
- `openssl` `1.0.2o-3`

OpenSSL 1.0.2 is intentionally used because the existing FTP plugin loads the
legacy OpenSSL 1.0.x DLL names and symbols from `src/plugins/ftp/ssl.cpp`.
Moving to the current OpenSSL 3.x vcpkg package would require porting that code.
OpenSSL 1.0.2 is end-of-life, so this is a compatibility build for the
existing FTP plugin rather than a long-term TLS upgrade.

## Build

Run from a Visual Studio Developer PowerShell on Windows:

```powershell
.\tools\vcpkg\build-third-party-libs.ps1
```

By default the script:

1. clones `https://github.com/microsoft/vcpkg.git` into `build\vcpkg` unless
   `VCPKG_ROOT` or `-VcpkgRoot` points at an existing checkout,
2. checks out the pinned vcpkg baseline,
3. bootstraps vcpkg,
4. installs the manifest in `tools\vcpkg\third-party-libs` for the
   `x64-windows` triplet,
5. copies the required runtime DLLs into `build\libs`.

Useful options:

```powershell
.\tools\vcpkg\build-third-party-libs.ps1 -Triplet x64-windows
.\tools\vcpkg\build-third-party-libs.ps1 -VcpkgRoot C:\src\vcpkg
.\tools\vcpkg\build-third-party-libs.ps1 -OutputDir C:\tmp\salamander-libs
```

The final output is:

```text
build\libs\unrar.dll
build\libs\libeay32.dll
build\libs\ssleay32.dll
```

These DLLs still need to be copied into the runtime layout expected by
Salamander:

```text
plugins\unrar\unrar.dll
utils\libeay32.dll
utils\ssleay32.dll
```
