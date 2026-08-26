# Security policy

This document describes how Samandarin handles security reports and what the
current plugin trust model actually does. It is not a product guarantee, a
compliance statement, or a promise of formal security testing.

## Supported versions

Security fixes are applied to the **current Samandarin releases published on
GitHub**. There is no long-term support (LTS) commitment and no promise that
older installers will receive backported patches.

Use a current release from
[github.com/KRtkovo-eu-AI/salamander/releases](https://github.com/KRtkovo-eu-AI/salamander/releases).

## Reporting a vulnerability

Please report vulnerabilities **privately**. Do not open a public issue for an
unfixed security problem.

Preferred path:

1. Open a private report through
   [GitHub Security Advisories](https://github.com/KRtkovo-eu-AI/salamander/security/advisories/new)
   when private vulnerability reporting is enabled on the repository.
2. If that form is unavailable, contact the maintainer through the GitHub
   profile used for this repository and say that you have a private security
   report.

Please include the affected version, a clear description, and enough detail to
reproduce the issue. Do not attach exploit-ready payloads against third-party
systems.

We will try to acknowledge reports, but there is no SLA and no bug-bounty
program.

## Plugin trust model

Native `.spl` plugins and many helpers run **in-process** in `salamand.exe`.
There is no sandbox. A malicious or compromised plugin has the same access as
the file manager.

Official Plugin Updates auto-install is fail-closed:

1. The catalog JSON on `samandarin.net` must include a `packageSha256` digest
   of the official `.7z` payload.
2. After download, Samandarin hashes the archive and compares it with that
   catalog digest. A missing, empty, or mismatched hash cancels installation.
3. After extraction, first-party PE files (`.spl`, `.slg`, and other PE that
   are not known unsigned redistributables) are checked with `WinVerifyTrust`.
   The publisher must match the expected Certum Open Source signer. The verifier
   does not pin a leaf certificate thumbprint, so certificate renewal does not
   by itself break installs. Timestamped signatures remain acceptable after the
   leaf certificate expires.
4. Unofficial catalog entries are not auto-installed. They open a browser.
   Plugin Updates marks them as not cryptographically verified.

The catalog hash is **not** taken from GitHub release notes. GitHub hosts both
the notes and the `.7z` file, so a compromised release could rewrite both.

Install receipts and the last helper error are stored in `plugin-receipts.json`
next to `salamand.exe` so they travel with portable copies. They are not stored
in `%LOCALAPPDATA%` and they are not written into `configstorage.ini`.

Capability fields in the catalog (`networkAccess`, `externalProcesses`,
`scriptExecution`, `activeWebContent`, `elevation`) are **curated disclosure**.
They are not enforced at runtime.

## Signing model

Release binaries are Authenticode-signed with a Certum Open Source certificate
as described in `tools/codesign/codesign.md`. Signing is a manual release step.
Unsigned redistributables bundled inside plugins (for example 7-Zip, OpenSSL,
WebView2, and VC runtimes) are skipped by the codesign scripts and by the
package verifier.

Catalog JSON itself is currently served over HTTPS and is **not** minisign- or
GPG-signed. A follow-up could add an independent catalog signature.

## Untrusted files

Opening, viewing, unpacking, or hashing a file does not mean Samandarin
executed it. Treat archives, scripts, and downloaded plugins as untrusted until
you have a reason to install them. Official auto-install still extracts and
then runs native code in-process after the checks above succeed.

## What is tested, and what is not

This project runs automated checks such as CodeQL, source-contract tests, and
plugin-catalog updater tests. Those reduce some classes of regressions. They
are not a substitute for:

- a formal security audit
- reproducible builds
- continuous penetration testing between releases
- a sandbox for in-process plugins

If you need those properties, this project does not currently provide them.
