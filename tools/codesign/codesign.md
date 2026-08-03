# Code signing

This project signs Windows release binaries with Authenticode and Certum Open Source Code Signing in the Cloud on SimplySign.

Signing is intentionally a **manual release step**. Certum/SimplySign may ask for a PIN for every signature, so Visual Studio post-build signing is disabled by default. The post-build entry point `tools\codesign\sign_with_retry.cmd` now exits without signing unless `CODESIGN_ALLOW_POSTBUILD=1` is explicitly set.

Use `tools\codesign\codesign_certum.cmd` manually when you are ready to sign release artifacts.

## Certificate and tools

Install and activate on the Windows release machine:

1. SimplySign mobile application.
2. SimplySign Desktop.
3. The current Windows SDK, including `signtool.exe`.

Before signing, sign in to SimplySign Desktop and confirm that the certificate is visible in the current user's certificate store. Open the certificate details and copy the certificate thumbprint. Remove spaces from the thumbprint before assigning it to `CODESIGN_CERT_SHA1`.

## Required environment

```cmd
set CODESIGN_ENABLED=1
set CODESIGN_CERT_SHA1=YOUR_CERTUM_CERTIFICATE_THUMBPRINT_WITHOUT_SPACES
```

Optional variables:

| Variable | Default | Description |
| --- | --- | --- |
| `CODESIGN_SIGNTOOL` | `signtool.exe` | Full path to `signtool.exe` if it is not on `PATH`. |
| `CODESIGN_TIMESTAMP_URL` | `http://time.certum.pl` | RFC 3161 timestamp server. |
| `CODESIGN_DIGEST_ALGORITHM` | `sha256` | File digest algorithm. |
| `CODESIGN_TIMESTAMP_DIGEST_ALGORITHM` | `sha256` | Timestamp digest algorithm. |
| `CODESIGN_RETRIES` | `3` | Number of signing attempts. Useful because timestamp servers can fail temporarily. |
| `CODESIGN_RETRY_DELAY_SECONDS` | `10` | Delay between retries. |
| `CODESIGN_BATCH_SIZE` | `25` | Maximum number of files passed to one `signtool sign` invocation during bulk signing. Lower this if Windows reports that the filename or extension is too long. |
| `CODESIGN_DESCRIPTION` | empty | Optional `/d` file description shown by Windows. |
| `CODESIGN_DESCRIPTION_URL` | empty | Optional `/du` URL shown by Windows. |
| `CODESIGN_ALLOW_POSTBUILD` | empty | Set to `1` only if you intentionally want Visual Studio post-build signing. |

## Manual single-file test

Use this first to verify that SimplySign, the certificate, SignTool and the script are working:

```cmd
tools\codesign\codesign_certum.cmd --file "H:\_projects\salamander\output\salamander\Release_x64\salamand.exe"
```

The script signs only `.exe`, `.dll`, `.spl` and `.slg` files. Before signing, it checks whether the target already has a valid Authenticode signature and skips it if it is already signed. It verifies newly signed files with:

```cmd
signtool verify /pa /all /v "path\to\file.exe"
```

## Manual Inno x64 payload signing

Use this after the x64 payload directory has been populated and before building or publishing the installer:

```cmd
tools\codesign\codesign_certum.cmd --inno-x64 --payload-dir "H:\_projects\salamander\output\salamander\Release_x64"
```

If `--payload-dir` is omitted, the script uses `%OPENSAL_BUILD_DIR%\salamander\Release_x64`.

The script reads `doc\runbook-setup\inno_setup_salamander_x64.iss` and signs only files that are explicitly listed in that installer script and have one of these extensions:

- `.exe`
- `.dll`
- `.spl`
- `.slg`

Files that already have a valid Authenticode signature are skipped before signing. The remaining files are signed in batches of `CODESIGN_BATCH_SIZE` files, defaulting to 25 files per `signtool sign` invocation. This avoids Windows command-line length failures when the payload contains many `.slg` files. Verification still runs for each newly signed file.

If you want to sign only `.slg` files grouped by language, use:

```cmd
tools\codesign\codesign_certum.cmd --slg-by-lang --payload-dir "H:\_projects\salamander\output\salamander\Release_x64"
```

This mode reads the same Inno Setup script, groups existing `.slg` files by language file name, and then applies the same already-signed skip and batch signing logic to each language group.

External DLLs are skipped. The exclusion list includes:

- `7za.dll`, `7zwrapper.dll`, `unrar.dll`, `chmlib.dll`, `sqlite.dll`, `libeay32.dll`, `ssleay32.dll`
- `Newtonsoft.Json.dll`, `Markdig.dll`, `PrismSharp.dll`
- `WebView2*.dll`, `System.*.dll`, `Microsoft.Web.WebView2.*.dll`
- VC/UCRT/API-set runtime DLLs such as `api-ms-win-*.dll`, `ucrtbase.dll`, `vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll`, `concrt140.dll`
- `dbghelp.dll`

## Release order

1. Build and populate the release payload directory.
2. Sign a single file with `--file` if you want a smoke test.
3. Sign the Inno x64 payload with `--inno-x64`. If you need to handle `.slg` files separately, use `--slg-by-lang` with the same payload directory.
4. Build the Inno installer.
5. Sign the final installer separately with `--file`.
6. Verify the final installer with `signtool verify /pa /all /v`.

Do not modify a signed file after signing it. Any post-signing modification invalidates the Authenticode signature.
