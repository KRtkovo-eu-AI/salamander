# S25 acceptance

## S25 — IMPLEMENTED / isolated cancellable scanner PASS (2026-10-23)

Pinned donor branch_view.h/.cpp scanner subset imported: Entry/Progress, owned
exact paths/UTF8 cache identity, explicit directory stack, bounded batches,
shared State independent of panel/window, Start/Cancel/Drain and snapshot
comparison. Actual State/Run/Start/Cancel/Drain block verified exact donor.
Directory name-surrogate junctions/symlinks are skipped, ordinary reparse/cloud
directories retained per donor policy; hidden/system directory filtering retained.
Cancellation does not join pending SMB I/O; worker's shared state survives owner
close and replacement gets independent storage. No panel/UI APIs or activation.

Native project EXACT; donor fixture ADAPTED with test-only retained future
scheduler/publication-sort sections explicitly uncompiled until dependencies in
S26/S30; no future assertions counted passed. Scanner/concurrent-write/snapshot/
replacement/cancel/owner-destruction assertions retained. Added missing-root
error assertion, actual check count and bounded temporary-root cleanup guards.
Host project includes donor NotUsing PCH setting (initial omission failed C1020,
then corrected); IDE filters registration scoped only. Existing runner discovers
native project automatically, no registration framework introduced.

Native Debug/Release2476 checks each PASS0.92/1.60s;605 files, duplicate parents,
Unicode long filename/parent paths, exact UTF8 identity per row, multiple batches,
missing-root error, changing tree, hidden filter, independent replacement,
cancellation and16 immediately closing owners. Actual directory symlink cycle
fixture RAN in both configurations, no policy-only substitute. Not a network
outage GUI test; pending-I/O independence verified by production ownership audit.
Host Debug/Release x64 /warnaserror PASS11.86/84.38s. All24 source/menu groups
PASS. Actual ordinary panel smoke D/R: long Unicode panel and2 files visible,
normal exit/no new reports. All4 source hashes unchanged; private configs restored.
Evidence ../evidence/S25 source hashes and bounded logs. Public feature still
unavailable. No SDK/layout changes, push/PR/CI. Next S26 gated panel integration.
