# Code signing

This project signs Windows release binaries with Authenticode. The Visual Studio release projects call `tools\codesign\sign_with_retry.cmd` from post-build events. That file is intentionally only a small launcher; the Certum/SimplySign implementation lives next to it in `tools\codesign\codesign_certum.cmd`.

## Certificate

The expected certificate is **Certum Open Source Code Signing in the Cloud** on SimplySign. Install and activate:

1. SimplySign mobile application.
2. SimplySign Desktop on the Windows release machine.
3. The current Windows SDK, including `signtool.exe`.

Before building a signed release, sign in to SimplySign Desktop and confirm that `signtool.exe` can see the certificate in the current user's certificate store.

## Required environment

Signing is disabled by default so local developer release builds do not fail when no signing certificate is available. Enable signing only on the release machine:

```cmd
set CODESIGN_ENABLED=1
```

Optional variables:

| Variable | Default | Description |
| --- | --- | --- |
| `CODESIGN_SIGNTOOL` | `signtool.exe` | Full path to `signtool.exe` if it is not on `PATH`. |
| `CODESIGN_CERT_SUBJECT` | empty | Certificate subject passed to `signtool /n`. Useful when multiple certificates are available. |
| `CODESIGN_CERT_SHA1` | empty | Certificate SHA-1 thumbprint passed to `signtool /sha1`. Takes precedence over `CODESIGN_CERT_SUBJECT`. |
| `CODESIGN_TIMESTAMP_URL` | `http://timestamp.digicert.com` | RFC 3161 timestamp server. |
| `CODESIGN_DIGEST_ALGORITHM` | `SHA256` | File digest algorithm. |
| `CODESIGN_TIMESTAMP_DIGEST_ALGORITHM` | `SHA256` | Timestamp digest algorithm. |
| `CODESIGN_RETRIES` | `3` | Number of signing attempts. Useful because timestamp servers can fail temporarily. |
| `CODESIGN_RETRY_DELAY_SECONDS` | `10` | Delay between retries. |
| `CODESIGN_DESCRIPTION` | empty | Optional `/d` file description shown by Windows. |
| `CODESIGN_DESCRIPTION_URL` | empty | Optional `/du` URL shown by Windows. |

If neither `CODESIGN_CERT_SUBJECT` nor `CODESIGN_CERT_SHA1` is set, the script uses `signtool /a` and lets SignTool choose the best available signing certificate.

## Example setup

```cmd
set CODESIGN_ENABLED=1
set CODESIGN_SIGNTOOL=C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe
set CODESIGN_CERT_SUBJECT=Open Source Developer
set CODESIGN_DESCRIPTION=Open Salamander Samandarin
set CODESIGN_DESCRIPTION_URL=https://github.com/KRtkovo-eu-AI/salamander
```

Then build the release configuration normally. Each project that imports a release property sheet with the signing post-build event will call:

```cmd
tools\codesign\sign_with_retry.cmd "path\to\binary.exe"
```

## Manual test

After SimplySign is running and the environment variables are set, test one binary manually:

```cmd
tools\codesign\sign_with_retry.cmd build\x64\Release\salamand.exe
```

Verify a signed binary:

```cmd
signtool verify /pa /v build\x64\Release\salamand.exe
```

## Release order

1. Build release binaries.
2. Sign every produced PE binary (`.exe`, `.dll`, `.spl`, and helper tools).
3. Package the installer or release archive.
4. Sign the final installer if it is an executable installer.
5. Verify signatures on the final release artifacts.

Do not modify a signed file after signing it. Any post-signing modification invalidates the Authenticode signature.
