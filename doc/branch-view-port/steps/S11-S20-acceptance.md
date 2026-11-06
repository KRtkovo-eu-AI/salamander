# S11–S20 acceptance

## S11–S20 — ACCEPTED after current-head fork CI (2026-10-23)

Accepted source commit adebcaa0bc48f7e937da92a0562b9e11ab3ca6fb.
Fork validation draft https://github.com/majklvi/salamander/pull/34 remains open;
no merge into the release-base mirror. All13 check entries SUCCESS, including
actual CodeQL initialization, Release-x64 build, analysis and SARIF upload in
PR run39869004250. CodeQL reports no new alerts on PR-changed lines.
Separate workflow-dispatch39869022507 full clean Debug solution rebuild passed
Win32/x64/ARM64 with project dependency preparation. Its own CodeQL job is skipped
by design; actual analysis ran in the PR workflow. SFTP remains excluded by
workflow policy. Evidence ../evidence/S21a/accepted-{pr34,full-build}.json.
