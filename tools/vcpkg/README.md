# Third-party DLL build with vcpkg

This directory contains a small vcpkg manifest and helper script for building the
runtime DLLs that are not stored in the repository but are needed by the current
x64 distribution:

- `unrar.dll` for the UnRAR plugin
- `libeay32.dll` and `ssleay32.dll` for the FTP plugin's legacy OpenSSL loader
- `dbghelp.dll` for the HTML Help / crash reporting (provided as prebuilt)

The script pins the vcpkg registry baseline and package versions so the produced
inputs are reproducible:

- `unrar` `7.2.6`
- `openssl` `1.0.2o-3`

OpenSSL 1.0.2 is intentionally used because the existing FTP plugin loads the
legacy OpenSSL 1.0.x DLL names and symbols from `src/plugins/ftp/ssl.cpp`.
Moving to the current OpenSSL 3.x vcpkg package would require porting that code.
OpenSSL 1.0.2 is end-of-life, so this is a compatibility build for the
existing FTP plugin rather than a long-term TLS upgrade.

### SFTP plugin dependencies

The SFTP plugin (`src/plugins/sftp`) uses a separate vcpkg manifest with
modern libraries:

- `libssh2` (latest)
- `openssl` 3.x

These are installed into `build/vcpkg_installed_sftp/` and are used for
compilation (headers + import libs) and at runtime (DLLs).

## Build

Run from a Visual Studio Developer PowerShell on Windows:

```powershell
.\tools\vcpkg\build-third-party-libs.ps1
.\tools\vcpkg\build-third-party-libs.ps1 -SftpPlugin    # incl. libssh2 + OpenSSL 3.x for SFTP plugin
.\tools\vcpkg\build-third-party-libs.ps1 -SftpPlugin -OnlySftpPlugin  # only SFTP deps
.\tools\vcpkg\build-third-party-libs.ps1 -PrebuiltDllsDir C:\path\to\dlls  # fallback for non-vcpkg DLLs
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
build\libs\dbghelp.dll
```

These DLLs still need to be copied into the runtime layout expected by
Salamander:

```text
plugins\unrar\unrar.dll
utils\libeay32.dll
utils\ssleay32.dll
utils\dbghelp.dll
```

### Salamatrix AI local model assets

The optional `Salamatrix AI Local LLaMA` companion plug-in uses a pinned
official CPU x64 llama.cpp release and a lightweight Qwen2.5-Coder GGUF model.
Stage these assets before building that companion plug-in:

```powershell
.\tools\vcpkg\build-salamatrixai-assets.ps1
msbuild .\src\plugins\salamatrixailocalllama\vcxproj\local_llama.vcxproj /p:Configuration=Debug /p:Platform=x64
```

The downloader verifies SHA-256 hashes, writes the assets to
`build\libs\salamatrixai`, and never starts Salamander. The plug-in build then
copies the executable, its llama.cpp DLLs, the model, and the accompanying
license/manifest files to `plugins\salamatrixailocalllama\runtime`. The model is not a
vcpkg library dependency: vcpkg can build llama.cpp, but it does not provide
the model weights or their redistribution terms.

With `-SftpPlugin`, the following are also installed into
`build\vcpkg_installed_sftp\`. Use `-OnlySftpPlugin` when you only need
these SFTP dependencies and want to skip the legacy third-party DLL manifest:

```text
build\vcpkg_installed_sftp\x64-windows\include\  (headers)
build\vcpkg_installed_sftp\x64-windows\lib\      (import libs)
build\vcpkg_installed_sftp\x64-windows\bin\      (DLLs)
```

These are used automatically by the SFTP plugin's MSBuild project.
