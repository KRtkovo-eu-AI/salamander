# S31 — qualified Pack target and selection lifetime

Qualified Pack captures source/target disk identity, listing epoch, arrays and
backend binding across modal dialogs, plugin initialization and enumeration.
The Branch target uses its real wide root. Busy/refresh scopes retain the source
and restore prior state. Unknown/custom backends do not gain admission; SDK
virtual layouts stay unchanged. Paths and source selection scratch are owned.

The qualified Pack recursive scanner uses wide Win32 enumeration and exact UTF8
names, with extended prefixes. Legacy enumerators explicitly keep their original
ACP bytes. Path/handle scopes restore state on error, Cancel and OOM. Capacity
failure aborts instead of silently omitting a subtree. ZIP preview scratch is
heap backed; directory metadata and exact empty-source-directory cleanup use wide
APIs. Nonempty directories are preserved. No recursive deletion helper is added.

Debug/Release host and ZIP builds passed /warnaserror. Actual native Pack pipeline
3847 checks/config, scanner 60/config, source identity 311/config, Branch callback
and cleanup 439/config and transition contracts passed. Existing assertions and
security flags remain. Negative controls without post-init validation fail.

GUI ZIP ordinary and 7Zip Branch source into a long Unicode Branch disk root:
Copy/Move plus Cancel D/R, 16 readbacks PASS. Five payloads include duplicate base
names in different directories, Unicode and byte/UTF16 long paths. Archive entry
names and bytes were independently decoded with the qualified bundled engine;
source removal/preservation, backups and sentinels checked exactly. Both Branch
panels refresh after Move. Private runtime profiles restored to original state.
Initial ZIP preview overflow, ANSI directory metadata failures, approved cancels,
harness failures and bounded build timeouts remain recorded separately.

The user explicitly approved a precise remaining limit: long-target ZIP
multi-volume and SFX packing is refused; short-path advanced modes retain prior
behavior. The byte limit also catches UTF8-long names. Actual initializer, click
handlers and defaults loader reject before legacy preview; backend entries guard
hidden-dialog calls too. Existing localized ZIP path error is reused. Boundary
and defaults tests D/R (77 checks/config) passed; final ZIP builds D/R passed.
The legacy volume-name terminator comparison now refuses length == MAX_PATH
before copying into a MAX_PATH caller. No new MAX_PATH path buffer is introduced.
Full long-path advanced ZIP qualification is follow-up work, not silently claimed.

Evidence outside Git: `evidence/S31-pack-target/final-audit.json`, exact GUI
manifest/readbacks, native/build records and preserved failures. GUI payload
proofs precede the final rejection-only ZIP gate; actual initializer/handler/
default-loader native tests qualify that changed behavior. Final artifacts have
separate hashes. A fixed-name Debug ZIP build receipt was superseded during a
rebuild; original GUI artifact identities remain in the manifest/readbacks.
Release signing and remote CI/CodeQL were not run. Whole S31 remains ACTIVE.

Adapted from Open Salamander Authors / Samandarin Contributors, PR #769, donor
47661e92d4b3cbe5bb593ae3d75a3f1f698ef413; notices and licenses preserved.
