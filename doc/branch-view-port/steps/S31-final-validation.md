# S31 — final local validation and remaining environment decisions

Implementation head: `1d6ef63846f1da654645778ecb431e26ed1daedc`.
This is the final finite-list record, not a new implementation substage.
G05 handoff explicitly approved by the user on 2026-10-28. Final local closure
is bound by the closing commit and external final record; no release approval.
No push, remote PR, CI, Windows setting change or S32 is authorized.

## Current functional results

| Requirement | Implementation | Verification / gate |
| --- | --- | --- |
| Complete extraction and exact source/volume deletion | Complete | ZIP/7Zip complete, partial/mask/Skip/error/Cancel D/R evidence; real ZIP three-volume set D/R and missing-volume preservation; incomplete/unowned 7Zip volume metadata never authorizes deletion. |
| Direct Branch Unpack | Complete | Ordinary/Branch disk roots, source/target/backend identity, post-init and per-delete checks; eight exact D/R GUI results plus existing no-delete/error cases. |
| Archive F5 source lifetime | Complete (`72d83019`) | Owned identity and listing epoch, live tab/owner callback checks, negative controls and actual ordinary/Branch D/R F5 payload proofs. |
| Pack target/lifetime | Complete (`1d6ef638`) | Qualified wide target and enumeration, sixteen Copy/Move/Cancel GUI readbacks D/R; native pipeline/scanner/OOM/stale tests; host/ZIP builds D/R. |
| Advanced ZIP long target | Explicit user-approved limit | Multi-volume/SFX refuse byte length >= MAX_PATH before preview/defaults/backend. Normal ZIP/7Zip retained. Short advanced routes preserved; not newly qualified. Native actual handlers/default loader D/R. |
| Core archive entry / shortcut / attributes / Properties | Complete scoped local validation | Seventeen relevant bodies unchanged from their qualified commits; exact original D/R proofs retained. Current operation contracts All 20, view contracts 19, plugin policy/dispatch native 179/config PASS. |
| EFS RAW restore cancellation | Complete (`59c39d5d`) | Actual Windows RAW and ADS six path classes D/R, pre/midstream cancellation; native failure/late-cancel tests and plugin D/R builds. Cleanup refusal is reported and may retain incomplete output. |

## G05: precise remaining limits, approved disposition

These rows are NOT_VERIFIED, not newly unsupported. They do not authorize
removing an existing function, relaxing admission or claiming a green release.
Historical OPEN entries elsewhere do not supersede the current evidence above.

| Boundary | What is verified | Missing proof and proposed action | Completion condition |
| --- | --- | --- | --- |
| Undelete Connect image | D/R exact wide prefill and lossless legacy ACP/byte-capacity refusal; seven current Connect bodies identical to qualified S27f11 | Full physical-disk and image recovery backend scenarios NOT_VERIFIED. Backend behavior is unchanged; keep available legacy routes and refuse the already guarded unrepresentable inputs. Defer backend qualification to a separate owned-image fixture task. | Real known-content filesystem image, exact recovered payload/read-only input proof and supported backend/path matrix. |
| SMB / live UNC / offline cancellation | Local NTFS operations and explicit UNC conversion/context tests | No dedicated remote SMB endpoint or controlled disconnect fixture supplied. Existing local admin shares alone do not qualify remote/offline behavior. Defer actual network/transport matrix; preserve functions and guard policy. | Owned remote share, exact source/output checks, controlled server-side disconnection and Cancel without affecting user connections. |
| ADS / ACL / hardlinks | Actual EFS default+ADS import/export above; existing local action proofs retain their own scopes | Do not infer all metadata/link preservation for all copy/move/recycle/plugin routes. Remaining scenario-specific claims NOT_VERIFIED; no new behavior restriction. | Per-operation filesystem metadata and link identity readbacks against explicit expected policy. |
| Localization runtime | Existing eleven Branch resource/ID checks; English host/plugin build and actual local GUI; S31 adds no resource strings | Complete eleven-language packaging/staging/GUI matrix NOT_RUN on final revision. Defer to publication qualification, not PASS. | All intended language packages staged and loaded with exact text/encoding/resource checks. |
| Supported platform / security scans | Relevant x64 Debug+Release host/plugin builds, native/controller tests and actual GUI proofs | Full clean Win32/x64/ARM64 CI and current-head CodeQL NOT_RUN under explicit no-push/no-remote-CI instruction. Old successful PR results cannot qualify this head. | Separately authorized publication; complete current-head required CI and CodeQL, failures fixed before release. |
| Long-source ChangeDir GUI | Previously diagnosed and explicitly deferred by user | Remains DEFERRED / NOT_VERIFIED. Do not reopen or discard original WIP. | Separate explicit resumption and successful Debug/Release ChangeDir+long-source GUI. |
| Debug shutdown heap report | Earlier S28 already recorded a shutdown caveat; Pack Debug operations and payload readbacks pass | Later warnings do not prove a new Pack regression or their allocation origin. Preserve report as separate investigation; do not claim clean shutdown or silently disable leak checks. | Reproducible allocation-origin evidence determines whether a port regression exists; then fix and verify if within scope. |

User decision on 2026-10-28: explicitly approved finishing local S31 with these
retained validation limits and handing the named environment/publication proofs
to follow-up work. The decision does not declare a release ready, remove core
functions, authorize a push or start S32. The unchanged long-source ChangeDir
work stays separately deferred. Each completion condition above remains binding
for the corresponding future qualification; none is relabeled PASS.

## Final evidence binding

`evidence/S31-final` outside Git binds current source files, original scoped
records and current operation-policy results. The original S30 donor audit is retained in Git and external
evidence. Current tables refresh file hashes/equality and mechanical hunk screens;
REUSE.tsv explains subsequent adaptations. Proven G01/G04 deferred hunks are now
resolved; only the two S32 release documents remain deferred. No stale binary or prior PR status substitutes for a
current proof. Final donor pin, untouched deferred patch, SDK prefix, source
coverage and clean worktree checks passed before local closure. Source-bound proof results are not a claim of
remote CI or of the unverified environment matrix above.
