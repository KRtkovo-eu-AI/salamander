# Validace portu a baseline S00

## S27f.03 Checksum Calculate/Verify locally validated (2026-10-24)

Only bundled checksum\checksum.spl Calculate ID1 and Verify ID2 pass the private
Branch dispatch policy. Auxiliary Focus ID99 stays blocked and is a remaining
S27 integration item; no blanket DLL permission. All58 tracked Checksum files
remain equal to pinned donor47661e92. Host/plugin D/R /warnaserror PASS;
command dispatch50/config, owned-path62/config and all28 contracts PASS.
Actual mixed-parent Branch GUI D/R:40 real digests independently match CRC32/MD5/
SHA1/SHA256/SHA512; two MD5 exports retain all8 full relative paths/digests;
16 positive Verify entries including >260 Unicode manifest parent. Debug negative
exactly1corrupted/3OK/0missing/0skipped; Release ordinary Verify4OK. All sources and
sentinels unchanged, private INIs restored, runtime matches launch, registry0.
No publication, remote CI/CodeQL/full clean matrix. See [S27f.03](steps/S27f03.md).
Goal ACTIVE; next S27f.04 Split & Combine; auxiliary Focus requires separate audit.

## S27f.02 Batch Rename locally validated (2026-10-24)

Bundled renamer\renamer.spl internal Rename ID1 now passes the existing private
Branch command policy. Other IDs remain blocked, including unpublished menu Undo
ID2; Undo runs inside the existing Rename dialog. No SDK or engine change.
All56 tracked Renamer files remain exactly equal to pinned donor47661e92.
D/R host and Renamer /warnaserror builds PASS; command dispatch47, panel67 and
filesystem89 native checks/config PASS; all28 source/menu contracts PASS.
Actual D/R Branch GUI: four mixed-parent files, duplicate Unicode basenames,
>260 UTF16 parent and >260 UTF8 basename; exact preview, Cancel, Rename and Undo
PASS with independent path/SHA256 checks. Outside-selection sentinels unchanged.
Ordinary two-file preview/Cancel PASS D/R. Private INIs restored, runtime hashes
match launch, own processes exited, LongPathsEnabled remains0.
No remote CI/CodeQL/full clean matrix or publication for this unit.
See [S27f.02](steps/S27f02.md). Goal ACTIVE; next S27f.03 Checksum separately.

## S27f.01 File Comparator locally validated (2026-10-24)

Only bundled filecomp\filecomp.spl internal Compare Files ID1 is accepted for
Branch plugin dispatch. Actual registered SUID resolves to that exact command;
hotkey dispatch checks policy before DLL init; direct/menu/toolbar dispatch checks
again before callback. Unknown DLLs/IDs/custom absolute identities stay blocked.
Private CPluginData nonvirtual method only; no SDK/vtable/data-layout change.
The accepted S06 adapter/engine blobs remain exactly equal to the pinned donor.

D/R host/dependency and Filecomp builds /warnaserror PASS. New actual private-policy
and dispatch native45/config; accepted owned-path/service native58/config; all28
source/menu groups PASS. Actual D/R Branch mixed-parent same.txt pair includes
>260 Unicode parent: exact dialog paths, one expected real content difference,
Recompare and Cancel PASS. Debug also exercises submenu/SUID dispatch. Ordinary
source-to-unique-target panel comparison PASS D/R. All6 input/control hashes
unchanged, private INIs restored, clean host/reporter exit/no crash reports.
Known S06 Unicode header rendering limitation remains visibly documented; do not
claim full Unicode output UI support. Last Command remains staged off in Branch.
Two Release attempts hit95s limits; subsequent full build PASS8.40s, failed/time-
limited logs retained. No remote CI/CodeQL/full matrix or publication for this unit.
See [S27f.01](steps/S27f01.md). Goal ACTIVE; next S27f.02 Batch Rename separately.

## S27e2 locally validated; S27e complete (2026-10-24)

Bundled ZIP/7-Zip Move packing is enabled through the same exact-relative callback.
Audit found legacy narrow source deletion: ZIP cleanup now decodes UTF8 first with
ACP fallback and uses extended wide DeleteFileW;7-Zip keeps owned full-path UTF8
and uses its existing long-wide conversion before deletion. Existing success,
Cancel/Skip/Retry and archive-self-lock semantics remain intact; no engine rewrite.

D/R host/dependencies and both archive engines /warnaserror PASS. Actual callback,
packer gate and source-deletion native tests421/config PASS, including real short
and >260 Unicode file removal, unselected sentinel and failure/Cancel. All28
registered source/menu groups PASS. User-confirmed GUI ZIP and7z Move each PASS
in D/R:5 exact relative names and independently decompressed SHA256,5 selected
sources removed, outside sentinel unchanged. Release checked-Move dialog Cancel
preserved all5 source hashes with no output. Private configs restored; policy0.
Debug repeated known heap notification (S22/S27e1; allocation origin not instrumented
this run); Release clean exit/no new crash reports. See [S27e2](steps/S27e2.md).
Unknown/external packers and Branch targets remain unavailable; no SDK/security
change. No push/remote CI/CodeQL/full clean matrix run. S27 goal remains ACTIVE:
next S27f.01 File Comparator, then separately verified plugin integrations.

## S27e1 locally validated — ZIP/7-Zip copy packing (2026-10-24)

CM_PACK now uses the accepted S22 exact-relative callback for disk Branch sources.
Only bundled ZIP/7-Zip copy packers and ordinary disk targets are accepted; Move,
external/unknown plugins and Branch targets stay blocked before archive effects.
Existing allocation-failure paths retain nothrow behavior. No SDK/engine changes.

Host/dependency and both engine builds PASS Debug/Release x64 /warnaserror.
Actual callback/gate native tests377 per configuration and28 source/menu groups PASS.
Real D/R GUI ZIP and7z archives each contain five exact relative file names with
independently decompressed SHA256 matches: duplicate basenames, Unicode parents,
UTF8-byte-long and UTF16-long source paths. Cancel preserves all sources; ordinary
panel7z control PASS both configurations. Private configs restored; policy stays0.
Debug shutdown showed the previously documented heap notification (S22); this run
only observed the notification, without proving identical allocations. Release
clean exit and no new crash report. See [S27e1](steps/S27e1.md).
No push/remote CI/CodeQL/clean full matrix run for this unit. Goal remains ACTIVE:
next S27e2 move-packer cleanup, then S27f.01-.11 separately validated integrations.

## S27d accepted; goal covers remaining S27 (2026-10-24)

User accepted S27d at df01f687f062672cb74a2cc21a9116508593bc36. PR40 current-head
checks, actual CodeQL analysis/upload and clean full supported Win32/x64/ARM64
Rebuild40079394417 all PASS. Accepted local ref refactor/pr-769-incremental advanced
by checked fast-forward from328cffd5; samandarin-0.16, donor and draft PR unchanged.

User explicitly requested a goal through completion of S27, overriding the usual
one-unit-per-run convention: retain one validated coherent commit per unit and
continue locally. No new push/upstream PR authorized. Candidate starts from S27d.

S27e split before activation:
- S27e1: copy-only host packing via bundled ZIP/7-Zip. Real Branch relative identity
  through accepted S22 callback. CM_PACK only, ordinary disk target only; block
  unknown/external packer and Move selection before any file side effect. Native
  callback/gates, D/R host and both engines, GUI real ZIP/7z entries and decompressed
  hashes, duplicate basenames, Unicode/long source, Cancel/untouched-source proof.
- S27e2: separately validate move-packer cleanup, file-link size cache/reset and
  cancellation/failure before enabling deletion. Keep unsupported routes gated.
- Then S27f.01-.11 separately: Comparator, Batch Rename, Checksum, Split/Combine,
  Automation, Salamatrix, PictView, ZIP commands, 7-Zip test, PAK optimize, Undelete.
  Audit pinned donor per unit, actual owned selection results and D/R builds/GUI.
  Unknown plugin ranges remain unavailable; no SDK-layout/security/settings change.

## S27d5b completed locally; S27d goal complete (2026-10-24)

Clipboard COPY/MOVE into Branch uses the actual collection root, independent of
focused file parent. Internal disk drag onto blank background uses that root;
all rows/updir, same-panel drag, external/virtual/archive sources and incoming
links remain rejected. Fake and non-HDROP clipboard objects remain gated. Callback
and queued worker check exact wide root; overcapacity/post/allocation failures
preserve data cleanup. Existing donor native ingestion/wide worker stays in use.

D/R host/dependencies /warnaserror final PASS12.21s/6.87s (initial Debug40.77s,
Release82.14s; final executables match actual GUI binary hashes;
first Release link hit95s deadline, preserved under evidence release-attempt1).
Actual root/queue/receive and all previous native harness361/config PASS; existing
real Shell fixture33/config and all28 source/menu groups PASS. Actual D/R GUI:
3 exact copies, cut/paste1, mouse move1 into >260 Unicode root, row rejection and
overwrite Cancel;7 final files, four original same-name sentinels unchanged.
Known S30 limitation: long target notification requires manual Ctrl+R; verified.
Private configs restored, clean exits, policy0, no security/system setting change.
See [S27d5b](steps/S27d5b.md). S27d1 through S27d5 locally complete within their
explicit staged boundaries. No push/PR/remote CI/CodeQL/full clean matrix run.
Next proposed integration unit S27e; publication remains a separate instruction.

## S27d5a completed locally (2026-10-24)

Selected left/right drag source now uses the existing complete wide IDataObject,
Windows OLE negotiation and scoped DropSourcePanel/COM cleanup. Incoming Branch
operations remain gated until S27d5b. Allocation preserves the existing low-memory
path with scoped nothrow/debug-new handling; no security policy changed.

Host/dependencies Debug/Release /warnaserror PASS13.42s/77.34s; actual-block native
harness340/config, real Shell fixture33/config, all28 source/menu groups PASS.
Actual mouse OLE MOVE D/R transfers4 mixed-parent ASCII/Unicode/long files into
ordinary disk panel, exact hashes, source removal, duplicate sources/sentinel
preserved and Branch auto-refresh to2. Default500ms rejects the instantaneous test
input; owned private Debug delay1ms, Release delay disabled for automation only.
Production defaults unchanged. Clean exits/private INIs restored, policy0.
COPY/right-button/Escape/error/allocation negotiation covered by explicit native
facades, not claimed as real GUI. See [S27d5a](steps/S27d5a.md).
No publication/remote CI. Goal continues with S27d5b destination semantics.

## S27d4 completed locally (2026-10-24)

Selected context menu enabled: one-parent selections use existing exact-wide Shell
menu; mixed parents retain the entire selection and use pinned donor host fallback.
Containing-folder command never reaches a Shell handler. Deferred fallback command
runs synchronously on source panel after Shell release and EndStopRefresh. Background/
collection-root and arbitrary plugin command ranges remain gated, activation off.

D/R host/dependencies /warnaserror PASS15.38s/74.01s; native actual-method/menu harness
327/config PASS; real Shell fixture33/config and all28 source/menu groups PASS.
Actual D/R mixed menu Properties reports6 files/Various Folders216/228 bytes. Single
long Unicode Shell menu, Escape and Open Containing Folder into deep ordinary panel
with correct long filename focus PASS.12 source hashes/sentinel unchanged, private
INIs restored, clean exits, LongPathsEnabled=0. No system/security settings changed.
See [S27d4](steps/S27d4.md), evidence ../evidence/S27d4. No publication/remote CI.
Goal continues with S27d5, split drag source/destination by actual dependencies.

## S27d3b completed locally (2026-10-24)

Paste from Branch clipboard into an explicit ordinary disk destination is verified.
Accepted S21 wide HDROP ingestion and S20 drop I/O are reused. Local dependency fix
replaces three legacy first-source buffers in DropCopyMove with one owned wide-first
UTF8 parent for source display, volume/scheduling metadata and notification input.
No implicit paste/drop into the collection root is enabled. Default-off gate intact.

D/R host/dependencies /warnaserror PASS (9.11s/77.11s), actual-method native harness
320/configuration PASS (8 new metadata cases), real Shell fixture33/configuration
PASS, all28 source/menu groups PASS. Actual D/R Ctrl+C/V copies4 mixed-parent files;
Ctrl+X/V moves the same4 with exact hashes and disappearance of just those sources.
Both duplicate same.txt inputs remain unchanged. Branch listings auto-refresh to2.
Actual D/R overwrite conflict Cancel preserves source copy and existing target.
Private INIs restored and hosts/reporters exit cleanly. LongPathsEnabled=0 unchanged.

See [S27d3b](steps/S27d3b.md); evidence ../evidence/S27d3b. No publication/remote
CI/CodeQL or clean full matrix. S30 notification transport still has its legacy
single-first-parent/short-path limit; other source panels may need manual Refresh.
Next goal units S27d4 context menu, then S27d5 drag/drop/destination semantics.

## S27d3a completed locally (2026-10-24)

Cut export now enabled through CM_CLIPCUT and selected saCutToClipboard, reusing
accepted S21 donor publication/ghost-state/cleanup and S27d1 exact capture guards.
Native actual-method harness312/configuration PASS; includes complete MOVE export,
failed effect blocking publication, exact cross-panel duplicate identities and
ClearCutToClipFlag. Existing real Shell tests33/configuration and all28 source/menu
groups PASS. D/R x64 host/dependencies /warnaserror PASS (6.92s/75.51s). Real D/R
Ctrl+X ghosted6 selected files and exported exact complete CF_HDROP/CIDA+MOVE;
Ctrl+C restored icons and COPY|LINK, independently inspected8 checks per payload.
Ordinary-panel smoke,12 unchanged source hashes/sentinel, clean exits and restored
private INIs PASS. LongPathsEnabled=0; no system/security change, no paste yet.

See [S27d3a](steps/S27d3a.md); external evidence ../evidence/S27d3a. No publication,
remote CI/CodeQL or clean full matrix for this unit. Goal remains active: next
S27d3b paste/shared-drop dependency, then S27d4 context menu and S27d5 drag/drop.

## S27d2 completed locally (2026-10-24)

Selected clipboard copy export is enabled after S27d1 d8c28521; candidate
integration/pr769-s27d2-candidate. Reuses accepted S21 complete wide Shell data
object/publication and S27d1 whole-selection identity guards. Only CM_CLIPCOPY /
selected saCopyToClipboard added; cut, paste, context menu, drag/drop remain gated.
Default-off experiment unchanged, no SDK/resources/system/security setting changes.

D/R x64 host/dependency builds /warnaserror PASS; actual-method native harness305
checks/configuration PASS; existing real Shell fixture33/configuration PASS;
all28 source/menu groups PASS. Actual D/R GUI Ctrl+C for six mixed-parent files,
focused long Unicode filename and ordinary ASCII folder passed independent native
OLE inspection (8 checks/case): exact CF_HDROP, complete CIDA identities and COPY|LINK,
never MOVE. Source hashes12/12, sentinel and empty destinations unchanged; private
INIs restored, hosts/reporters exited cleanly, LongPathsEnabled=0. No paste tested.

See [S27d2](steps/S27d2.md) for commands, provenance and limits. External evidence
../evidence/S27d2 binds tested parent/patch, binary hashes and committed inputs.
No push/new PR/remote CI/CodeQL or clean entire supported matrix for this unit.
Next smallest unit S27d3 cut/paste; split export/source-state and ingestion if needed.

## S27d1 completed locally (2026-10-24)

Properties is enabled for current Branch rows, including mixed-parent selections.
Scoped donor quiet-menu and SHMultiFileProperties fallback hunks reuse accepted
S21 full-path/PIDL/CIDA helpers; local null/index/current-cache/empty-path guards
reject the entire invalid selection. No basename/root reconstruction is used.
Only CM_PROPERTIES / selected saProperties is enabled. Clipboard, cut/paste,
context menus, drag/drop, dedicated security-tab and arbitrary plugin commands
remain gated; experiment activation remains off by default. No SDK/resources change.

Debug and Release x64 host/dependencies /warnaserror PASS. Existing actual-method
harness294/configuration PASS (269 retained +25 Properties/capture/gate checks,
explicit Shell facades); actual branch_operations_tests33/configuration PASS;
all28 source/menu groups PASS. Real D/R GUI single long Unicode file, mixed-parent
6-file selection and ordinary-folder Properties PASS, all closed with Escape/Cancel.
Twelve source hashes and sentinel unchanged, private INIs restored, hosts/reporters
exited cleanly. LongPathsEnabled=0. Native Windows displayed DOS-alias Location for
the long item; independent samefile checks confirmed the exact original parent.
No system/security/global setting was changed. No file attributes/permissions edited.

See [S27d1](steps/S27d1.md) for actual commands, donor mapping and limits.
Evidence ../evidence/S27d1 binds parent328cffd5, tested patch, final inputs and commit.
No push/new PR/remote CodeQL or clean whole-solution matrix for this new unit.
Next: S27d2 clipboard copy export, then S27d3 cut/paste, S27d4 context menu,
S27d5 drag/drop; split further by dependency/direction when needed.

## S27c1a–S27c4 accepted; S27d split before implementation (2026-10-24)

User accepted exact head328cffd557fe588b8eefe05873ac5acf8983bb0f after all PR39
checks and actual CodeQL succeeded, including clean supported Debug Win32/x64/ARM64
full-solution rebuild40029651148. Local accepted ref advances from e63167f0 to this
head. The internal draft remains open and is not merged into the reference base.

S27d is split by independently testable operation:
- S27d1: Properties only. Permit CM_PROPERTIES and selected saProperties; import
  donor quiet menu / complete IDataObject / SHMultiFileProperties fallback hunks.
  Reuse accepted S21 adapters and Shell helpers. Add current-identity/null/index/
  empty-path rejection at the panel capture boundary so no partial selection can
  reach Shell. Archive/plugin routes retain their accepted behavior.
- S27d2: clipboard copy export; exact whole selection and transfer effect.
- S27d3: cut export/paste ingestion and source cut-state cleanup, split further
  if the donor dependencies require it.
- S27d4: selection context menu/host fallback; arbitrary plugin actions stay gated.
- S27d5: drag/drop source and destination, split by direction if needed.
Recycle is already accepted in S27c4 and is not ported again.

This run implements only S27d1. Keep all other Shell actions, clipboard, drop,
security-tab command and plugin ranges blocked for Branch; activation stays off
by default. Native actual capture/fallback blocks plus existing Shell PIDL/CIDA
fixtures; real D/R GUI single long/Unicode file, mixed-parent duplicates, Cancel,
ordinary-panel Properties; owned source hashes unchanged; D/R host/dependency builds.
No publication authorized for the new candidate. Source pins and SDK unchanged.


## S27c1b–S27c4 completed locally (2026-10-23)

All four requested units are implemented sequentially on integration/pr769-s27c-candidate,
with no publication. S27c1b634d3403 adds optional relative-directory copy;
S27c2 0c657b31 adds move and explicit completion refresh; S27c3 af8bc2b1 adds
permanent deletion and unambiguous wide confirmation identities. This S27c4 unit
activates recycle modes and retains the stronger accepted S20c target policy:
FOF_ALLOWUNDO, FOFX_RECYCLEONDELETE, FOFX_ADDUNDORECORD and FOFX_EARLYFAILURE.
No application fallback from failed recycle to permanent deletion is introduced.

S27c4 checks owner/flags/abort HRESULTs, rejects null, invalid-index or stale Branch
sources, and performs the queue only after every exact source has been accepted.
Branch volume support is determined by Shell for each exact source rather than
turning a requested recycle into permanent deletion based on the synthetic root.
Ordinary disk volume policy stays unchanged. The temporary S27c3 recycle guard is
removed. Clipboard/drag/drop, archive/plugin destinations and later operations
remain outside these units; the process activation gate stays off by default.

Debug/Release x64 host/dependencies /warnaserror PASS. Actual getter/planning/queue
harness269/configuration PASS:248 prior checks plus21 actual queue-block checks with
explicit COM facades (mixed-volume path capture, duplicate/long identities, creation,
owner/flags/queue/parse/perform/abort failures, stale/null/index/normalization guards).
The queue facade does not prove actual Shell behavior. Separately, the actual
DoDeleteFile extraction now passes48/configuration with real filesystem deletion
and real force-recycle helper calls over five owned path variants; failed locked
recycle preserves the file, with no direct-delete fallback. UI/progress and selected
extension mask matching are facades. All31 prior permanent-worker checks remain.
The existing real operation_recycle_tests project built and ran D/R,15 checks each,
including real Shell recycling, long Unicode paths, ambiguous components and errors.
All28 registered source/menu groups PASS. The donor parser assertion was adapted
to the target's checked shellPath, adding an explicit OperationRecyclePath assertion.

D/R GUI Branch/ordinary panel smoke, Debug Files menu Delete availability and Release
Shift-F8 confirmation Cancel PASS. No GUI recycle/destructive confirmation was invoked;
actual destructive I/O is native-test-only. All12 GUI source hashes and sentinel remain
unchanged; hosts/reporters exited cleanly, private INIs restored. LongPathsEnabled=0
was read back; no system, security or global settings changed. Cross-volume/network
Shell behavior is not an actual runtime test on this single-volume fixture. The
legacy notification transport limit for very long panel roots remains S30 work:
manual Refresh may be needed there, while operation I/O retains exact wide paths.

Evidence ../evidence/S27c4 and earlier stage directories. No push/PR, remote CodeQL
or clean whole-solution matrix has run for these local commits. Accepted reference
remains S27b e63167f0. Publication/CI is the next review gate when authorized.


## S27c3 locally validated (2026-10-23)

Permanent deletion is enabled for Branch rows using their current exact parent/name.
The ordinary DOS-alias fallback is excluded for Branch sources. Donor extended
confirmation lists show relative paths; a local single-item adaptation uses the
owned wide relative identity instead of a shortened display mirror. Normal disk,
archive/plugin and later clipboard/drag policies remain unchanged. Recycle modes
are conservatively blocked only during this intermediate commit, never converted
to permanent deletion; S27c4 removes this transition guard after recycle validation.

Debug/Release x64 host/dependencies /warnaserror PASS. Actual getter/source/planning
harness248/configuration PASS. New registered native-generated test compiles the
actual DoDeleteFile and FileNameIsInvalid functions, includes the actual workerpath
helper and executes owned filesystem I/O:31 checks/configuration PASS. Cases include
ASCII, Unicode leaf/parents, UTF8-over-260 and Win32-long paths, duplicate basenames,
readonly removal, hidden Cancel/Skip, missing-source Cancel, invalid trailing-dot
alias and preservation of a sibling payload. UI/progress and recycle are explicit
facades; no GUI deletion is claimed. /W4 /WX exposed an existing potentially
uninitialized useRecycleBin in the worker. Safe initialization to TRUE preserves
all known modes and prevents unknown configuration from implying permanent deletion;
the new native test verifies that case preserves its source.

All28 source/menu groups PASS. Actual Debug GUI basic multi/single relative-identity
confirmation Cancel and Release extended relative list Cancel PASS. All12 GUI source
hashes and sentinel unchanged; own hosts/reporters cleanly exited, private INIs
restored. No actual destructive GUI action was performed; skill action-time approval
was therefore unnecessary. No registry/security changes. LongPathsEnabled remains0.
Evidence ../evidence/S27c3. No push/PR, CodeQL or clean whole-solution CI. Next S27c4.


## S27c2 locally validated (2026-10-23)

Branch F6 move now reuses the exact parent/name and optional relative-directory
planning. Donor per-source read/write storage access and cross-volume status flags
are restored. Current-row/cache gates remain; archive/plugin and Branch destinations,
clipboard, drag/drop and delete stay blocked. Explicit completion notifications now
use the scoped donor Branch AcceptChangeOnPathNotification block; this required
dependency is documented in PLAN and leaves S26 automatic-watcher policy unchanged.

Debug/Release x64 host/dependencies /warnaserror PASS; actual-block native harness
243 checks/configuration PASS (230 retained, six move and seven notification checks).
All28 source/menu groups PASS. Actual D/R F6 moved six files each with long Unicode
names, mixed parents and duplicate basenames into distinct relative destinations.
All12 destination hashes match; original source files are gone, sentinel unchanged;
Branch refreshed to zero rows without a manual key. Debug Cancel left all sources
and empty destination unchanged. Both own hosts/reporters exited cleanly and private
INIs were restored. GUI exercised same-volume moves; cross-volume status selection
is facade-tested, not a real second-volume transfer. Existing legacy MAX_PATH
notification transport remains a limitation for refreshing very long panel roots;
operation paths themselves are owned/wide. S30 notification work remains deferred.

Evidence: ../evidence/S27c2. No push/PR, remote CodeQL or clean whole-solution CI.
Next requested unit: S27c3 permanent deletion.


## S27c1b locally validated (2026-10-23)

Optional relative-directory copy is implemented from the pinned donor. The existing
localized string is reused in the expanded Branch copy dialog; ordinary panels do
not receive the option. Actual per-file planning creates missing Unicode ancestors
and reverse Skip labels, with CWD cleanup on planning failure. Flat copy remains the
default. Move/delete/recycle are still blocked at this commit.

Debug/Release x64 host and dependencies /warnaserror PASS. Actual extracted native
getter/planning harness: 230 checks/configuration, no failures; all 28 source/menu
groups PASS. Real D/R F5 copied six mixed-parent files per configuration, including
long Unicode paths and duplicate basenames, into exact relative destinations.
All 12 output hashes and all 12 source hashes verified; sentinel unchanged.
Debug Cancel/default-off/ordinary-dialog and default activation-off smoke PASS.
Both own hosts/reporters exited with no new reports; private INIs restored.
Known baseline Debug incremental lang linker failure was resolved by a bounded
lang rebuild with LinkIncremental=false, without source/workflow changes.

Evidence: ../evidence/S27c1b. Native scheduling/attribute/error facades test actual
planning blocks, not the real filesystem worker; the GUI copies supply that I/O
coverage. No push, PR, remote CodeQL or clean whole-solution matrix in this stage.
Next: S27c2 move, as requested in the same explicit four-stage instruction.


## S27b accepted; S27c1a flat copy locally validated (2026-10-23)

Accepted S27b e63167f01b4b7a84f9710275a86bcce0cf191e88 after all
[PR38 checks including actual CodeQL](https://github.com/majklvi/salamander/actions/runs/40014097010)
and the [clean supported Debug Win32/x64/ARM64 matrix](https://github.com/majklvi/salamander/actions/runs/40014126302)
succeeded. The local accepted ref advances to that SHA; the validation draft
remains open and is not merged into the reference base.

S27c is divided into flat copy (S27c1a), optional relative-directory copy
(S27c1b), move (S27c2), permanent delete (S27c3) and recycle (S27c4).
Only S27c1a is implemented in this step. F5 resolves the actual parent/name
and per-source ADS/network/storage scheduling metadata of each Branch row.
The existing S19/S20b wide worker I/O is reused. Current metadata is required;
non-copy source actions, Branch destinations, archive/plugin routes,
copy-to-selected-folders, drag/drop and clipboard activation stay blocked.
The experimental process gate remains off by default. LongPathsEnabled is still0.

Local Debug/Release x64 host/dependency builds with /warnaserror PASS. Native
Branch actual-block/getter fixture215 checks/configuration (all previous200
retained), ordinary paths196/configuration + five released SDK-prefix checks,
BuildName18/configuration and fresh worker-path fixture11/configuration PASS.
All28 registered source/menu groups PASS after updating the old blanket-block
expectation to the copy-only transition. Real D/R GUI passed F5 Cancel with
no explicit selection and all6 selected, flat copy from mixed parents including
long Unicode/CJK100 names, duplicate collision Skip, separately copying the
second duplicate to second.txt, and ordinary-panel F5 Cancel. All12 output
hashes match their exact sources; all12 sources and sentinel unchanged.
Both own hosts/reporters exited cleanly; private config INIs restored.

Details, actual commands, test-facade limits and deferred cases:
[S27c1a](steps/S27c1a.md). Evidence: ../evidence/S27c1a. This stage is local only;
no new push/PR, remote CodeQL or clean whole-solution matrix has run. S27b CI
cannot validate this new patch. Next smallest unit: S27c1b relative-copy planning,
create/Skip labels and localized dialog option, followed by actual nested outputs.

## S27a2 accepted; S27b locally validated (2026-10-23)

S27a2 accepted at 1d29dcf97ea7cd01e5a920928c4ccefca86ce9f9 after all
[PR37 checks including actual CodeQL](https://github.com/majklvi/salamander/actions/runs/40002644218)
and [native/Python tests](https://github.com/majklvi/salamander/actions/runs/40002644221)
succeeded. Both original CodeQL findings are fixed. The separate
[clean supported Debug Win32/x64/ARM64 rebuild](https://github.com/majklvi/salamander/actions/runs/40002672861)
also succeeded, with the recorded SFTP/lang_sftp and unsupported zip2sfx
platform exclusions. The internal validation draft is not merged into the
reference base. The accepted local ref now points to 1d29dcf9.

S27b enables only F2 and inline Rename for current Branch rows. Donor metadata
remapping and all three success hooks are integrated with the existing S20a
wide I/O and owned temporary selection. Local adaptations release the metadata
mutex before cancellation/refresh and revalidate an owned cache identity after
the modal F2 dialog. Copy/move/delete, other unverified commands and plugin
activation remain deferred. The process-wide experimental gate remains off
by default; LongPathsEnabled stayed 0.

Host and dependencies Debug/Release x64 /warnaserror PASS. Actual-method native
Branch fixture: 200 checks/configuration (original186 retained); ordinary getter
fixture: 196/configuration plus 5 released SDK prefixes; existing temporary
selection fixture: 21/configuration; all28 registered source/menu groups PASS.
Real Debug dark-scheme and Release light-scheme GUI passed F2 Cancel, long
Unicode-parent rename and ordinary-panel F2 Cancel. Debug additionally passed
explicit-selection Cancel, 100-CJK-basename rename, inline rename and collision
Cancel. All12 file contents and the sentinel retain their hashes after exactly
five intended renames. Both own instances exited cleanly; both private config
INIs were restored byte-for-byte. See [S27b](steps/S27b.md) for commands and limits.

S27b is local only on integration/pr769-s27b-candidate. No new push, PR, clean
whole-solution matrix or remote CodeQL was requested/performed for this stage.
Prior S27a2 CI does not validate the new S27b code. Next: plan the smallest S27c
copy/move/delete operation unit before enabling it. Older entries below are
historical results for their stated revisions.

## S27a2 testovací cesty v CI (2026-10-23)

Head 5806dc95 vyřešil oba původní nálezy CodeQL (instances #3/#4 fixed)
a prošel čistým Rebuildem Win32/x64/ARM64. Selhala však nativní programová
fixture: platné krátké cesty se odmítaly ve fázi2 s 0x8007007B.

Stejné selhání bylo reprodukováno lokálně s vlastním dočasným adresářem,
jehož vstupní cesta používá skutečný 8.3 alias. GetFinalPathNameByHandleW
vrací dlouhý kanonický název; původní fixture proti němu porovnávala alias.
Produkční kontrola shody v tomto případě záměrně odmítá přepis identity.

Oprava je jen v branch_program_launch_tests.py: při dostupnosti se výslovně
použije 8.3 zápis vlastního testovacího kořene, poté resolve(strict=True)
a samefile ověří kanonickou cestu stejného vlastního adresáře. Až z ní vznikají
pozitivní scénáře. Nový výpis zaznamenává původní, 8.3 i kanonický kořen.
Není změněn žádný assertion, podporovaný případ, bezpečnostní kontrola,
produkční zdroj, workflow, limit ani Windows nastavení.

Před opravou vynucená 8.3 fixture selhávala; po opravě skutečný pomocník prošel
102 kontrolami v Debug i Release (včetně PE spuštění, fyzického CWD, falešné
identity/rodiče, traversal, MOTW integrity a odmítnutí dlouhé cesty). Všech28
registrovaných zdrojových/menu skupin PASS. Nový lokální build hostitele ani
GUI se pro změnu pouze testovacího Python kódu neopakovaly; fixture znovu
kompilovala skutečný salopen a prováděla reálná spuštění v obou konfiguracích.

Evidence ../evidence/S27a2-final/ci-alias-before* (očekávané selhání),
ci-alias-after-debug*, ci-alias-after-release*, Repro-ShortTemp.py.
Nové CI, CodeQL a čistá podporovaná sestava se spouštějí pro následující push;
jejich dokončení nelze vyvozovat z výsledků předchozího headu.

## S27a2 oprava hranice pomocníka po CodeQL (2026-10-23)

Dva nálezy cpp/path-injection v PR #37 (původní head 8f1dd145) vedly ke změně
soukromého protokolu na v3. Host otevře přesný vybraný soubor se zachovaným
read lockem a jeho adresář. Jen tyto dva handly zdědí pozastavený pomocník
přes explicitní PROC_THREAD_ATTRIBUTE_HANDLE_LIST. Host před ResumeThread
uvolní svoje kopie; pomocník drží zámek do stejné závěrečné hranice Windows Execute.

Pomocník zkopíruje požadavek do vlastní paměti a ověřuje stabilní kopii. Cestu
ze sdíleného paketu již nepředává do CreateFileW. Windows GetFinalPathNameByHandleW
vrací skutečné cesty připnutých objektů; ověřujeme přesný název, komponentovou
hranici rodiče a fyzický CWD pomocí file ID. Starý paket je odmítnut novým magic
a kontrolou velikosti. Reparse/aliasové podoby, které se neshodují s výslednou
kanonickou cestou, nejsou přepisovány: končí bezpečným odmítnutím.

PE typ, saved-file-aware zóna, CheckPolicy i konečné IAttachmentExecute::Execute
zůstávají povinné. Žádné potlačení CodeQL, přidání výjimky, filtrování pravidla,
změna registru ani náhradní spuštění. LongPathsEnabled stále 0.

Lokálně: host/salopen Debug i Release x64 s /warnaserror PASS; broker nyní
102 kontrol/config včetně podvrženého souboru, špatného rodiče, úniku z adresáře,
traversal/ADS/UNC a původních skutečných spouštění/MOTW testů. Branch getters
186/config, běžné panely 196/config + 5 SDK prefixů, všech 28 zdrojových/menu
skupin PASS. Nativní testy běžely odděleně D/R; timeout jednoho procesu je
75 s a celková brána 95 s. Původní 45s limit při prvním běhu Release vypršel;
tento běh není počítán jako PASS. Aktuální oddělené běhy prošly.

Nové CodeQL a čistý Rebuild celé podporované sestavy budou ověřeny pro nový
push; lokální testy nejsou důkazem uzavření vzdálených nálezů. Evidence
../evidence/S27a2-final/codeql-fix-*. Starší výsledky níže patří původní revizi.

## S27a2 dokončeno v dohodnutém rozsahu (2026-10-23)

Aktuální zadání výslovně přijímá dokončení s bezpečným odmítnutím
nepodporovaných případů. Tím nahrazuje starší podmínku, že dlouhé programové
cesty musejí být úspěšně spuštěny přes Shell. Historické záznamy PARTIAL níže
popisují tehdejší stav; nejsou aktuálním výsledkem etapy.

Oddělené větve nyní obsluhují ověřené PE programy .exe a dokumenty, skripty
či odkazy přes Shell. Program spouští Windows Attachment Services v existujícím
salopen pomocníkovi; neprovádíme raw CreateProcess cílového programu po pouhém
CheckPolicy. Windows zachovává závěrečné ověření a vlastní spuštění. Archivy
ponechávají ověřenou opravu širokého metadata I/O. Odmítnutí nemá alternativní
pokus, změnu bezpečnostního označení ani změnu registru.

Chyba dlouhé cesty obsahuje pouze čtený stav LongPathsEnabled a místní návod.
Výchozí tlačítko zavírá dialog; návod upozorňuje, že zapnutí není zárukou funkce
Shellu nebo aplikace a není řešením bezpečnostního odmítnutí. Skutečný systém
zůstal LongPathsEnabled=0. Podporované případy i omezení a přesné výsledky jsou
v [závěrečném protokolu S27a2](steps/S27a2-completion.md).

Lokální Debug/Release x64 build hostitele, salopen a jazykových resources prošel.
Programový broker: 93 kontrol v každé konfiguraci; Branch dispatch: 186/config;
běžné panely: 196/config a 5 SDK prefixů; 28 registrovaných zdrojových/menu skupin.
Skutečné GUI ověřilo krátký program se správným zdrojem/CWD a odmítnutí dlouhého
programu; tmavý i světlý dialog byly zkontrolovány. Debug shutdown opět ukázal
Detected memory leaks; příčina není určena a čisté ukončení Debug netvrdíme.

Bez publikace, čistého buildu celé sestavy a vzdáleného CodeQL v tomto kroku.
S27b a další etapy zůstávají odložené. Výchozí aktivace Branch View zůstává vypnutá.
Evidence: ../evidence/S27a2-final.

## S27a2 file-manager comparison (2026-10-23)

Pinned current FAR, Files, Double Commander and Explorer++ launch sources were
inspected, with official Directory Opus developer guidance. No imported Shell
route has demonstrated the required long-executable launch. FAR/Files direct
process launch is a separate semantic path, not equivalent Shell behavior.
New explicit .exe class Shell probe: ASCII child identity/CWD PASS; long source
via its existing alias FALSE/error4294967294, no child. Default registry unchanged.
All16 fixture inputs unchanged. No production changes or acceptance-ref advance.
Details and pinned sources: [comparison](steps/S27a2-shell-comparison.md).
S27a2 remains PARTIAL. Evidence ../evidence/S27a2-comparison.


## Desktop Shell probe: no long-path acceptance (2026-10-23)

At bff23a5c, an evidence-only probe reused the existing target PictView
ShellPreviewHost.cpp ShellExecuteUnelevated function, reaching desktop Explorer
through IShellDispatch2::ShellExecute. LongPathsEnabled stayed0. The corrected
explicit-record capture fixture proved ASCII child module/file identity, argv
and physical CWD. Its first version incorrectly required an inherited environment
variable; that result is not used as launch evidence.

The long executable in the same owned Unicode parent, submitted via its existing
DOS alias, produced no child record. The COM worker reached its15s deadline;
Explorer displayed Windows Security. The human user closed that dialog without
approving execution. No registry/zone setting or policy flag changed. This route
is not suitable for import; an out-of-process Shell call does not resolve the
observed limitation. No production code was changed or rebuilt in this probe.
All16 original input hashes remained unchanged; the two new owned diagnostic
executables were verified and removed. No probe process or security dialog remains.
Evidence ../evidence/S27a2-desktop/SUMMARY.json. S27a2 remains PARTIAL; accepted
ref remains at S27a1. This is a result for the tested host, not proof about every
Windows release or every Shell handler.


## S27a2 acceptance: default Windows configuration (2026-10-23)

User decision: long Shell launch must work with default Windows settings.
Do not require LongPathsEnabled=1 or any security/zone-policy changes for end
users. Modified settings may be used only as explicitly identified diagnostic
experiments; success there cannot satisfy the acceptance gate. The current
machine retains LongPathsEnabled=0; no registry change was made.

Follow-up normal-policy probes at candidate 9a870a02 failed with short aliases,
explicit Unicode/InvokeIDList flags, absent CWD, ACP and legacy path manifests.
Read-only zone diagnostics identified the original335-UTF16-character file and
its111-character DOS alias as the same volume/file ID. Both map to zone4
(Restricted), while the233-character ancestor and ordinary ASCII control map
to zone0 (Local Machine). Neither fixture executable has a Zone.Identifier
stream. This narrows the observed failure but does not prove all Windows
versions or Shell handlers behave identically. A minimal AttachmentExecute
CheckPolicy probe also rejects the working ASCII control, so that probe is not
independent proof that short Shell execution is denied.

Evidence ../evidence/S27a2-shell/SUMMARY.json and zone-matrix logs. No production
change, new GUI smoke, full build or remote CI in this diagnostic continuation.
S27a2 remains PARTIAL. Next gate: a demonstrated supported Shell dispatch under
default settings, preserving exact file identity, normal security checks and
physical working directory; then actual Debug/Release host verification.
An explicit long-path error is graceful failure, not completion of this gate.


## S27a2 - PARTIAL / archive correction validated locally (2026-10-23)

Parent 94c391a77282df03648aa47e20cb4410642764b0. Candidate enables gated
Enter using the exact current Branch item, owned wide source/directory and
UTF8 archive identity. Two archive metadata opens now use tracked CreateFileW
with the extended prefix instead of passing UTF8 to CreateFileA. This target
adaptation fixes the reproduced long Unicode ZIP failure, including refresh.
No archive engine, plugin SDK, workflow or default activation change.

Real Debug/Release host GUI passed long Unicode Branch ZIP Enter, listing,
refresh, F3 extraction/content and return to its exact parent. Debug additionally
passed a Unicode archive basename. Release ordinary ASCII duplicate ZIP showed
its distinct marker; a missing Branch archive preserved all eight rows and
opened correctly after the owned fixture was restored. Short ASCII Shell Enter
started the exact owned executable with its physical working directory in D/R.

Long executable Shell launch remains UNRESOLVED. Normal-policy plain/PIDL/alias
probes failed on this host; an ordinary ASCII control succeeded. Context-menu
probe timed out at 20s and its owned process tree was stopped. No security
settings/zone bypass/elevation or direct-process fallback. Candidate rejects
Shell source lengths >= MAX_PATH with the existing localized error, preserving
the Branch snapshot; this is a safe failure boundary, not long-launch support.
Accepted ref remains at S27a1; S27a2/S27a are not DONE and S27b onward is deferred.

Host x64 Debug/Release /warnaserror PASS (6.62/35.60s). Actual-method native
fixture 181 assertions/config PASS (original 99 retained; real archive metadata
I/O plus recorded dispatch/lifetime). Ordinary path/service 196/config and five
released SDK prefixes PASS; all 28 source/menu groups PASS. Existing unchanged
file-action D/R executables reran successfully (12 reported checks/config).
These are local incremental builds/tests, not full clean solution CI/CodeQL.

Debug shutdown displayed a heap-leak warning; acknowledged, then no owned host,
reporter or new report remained. Allocation profile was not captured, so prior
S22/S26b diagnostics do not prove the cause of this occurrence. Release exited
normally without a warning and left no host/reporter/new report. All 16 owned
input hashes verified unchanged; both private configstorage INIs restored byte
for byte. Installed user host and global settings untouched.

Evidence: ../evidence/S27a2; details [S27a2](steps/S27a2.md).
No publication or remote CI in this continuation.


## S27a1 — IMPLEMENTED / local validation complete (2026-10-23)

S21–S26 accepted at3dc52cb6 after current-head PR35 checks; acceptance commit
6f32c1d3. Completed the first S27 unit: core/configured external View/AltView,
Edit and exact Open Containing Folder. Process gate remains default off;
Enter, plugin viewers, View/Edit With and all remaining operations stay blocked.
Missing parent preflight preserves the Branch snapshot. Successful navigation
uses an owned wide basename to focus the real ordinary row, including a name
of100 CJK characters/304 UTF8 bytes. No generic navigator/SDK changes.

Final host x64 Debug/Release /warnaserror PASS12.58/76.84s. Actual-method native
fixture99 checks/config PASS, original71 retained; ordinary panel/service196
checks/config and5 released SDK class prefixes PASS; all28 source/menu groups
PASS. Existing file-action, viewer enumeration and Unicode native suites PASS.
Actual D/R GUI confirmed duplicate identities, viewer previous/next/refresh,
external argv+physical CWD, exact parent focus, missing-parent preservation,
blocked Enter/copy, normal-panel viewer and shutdown/no new reports. All16 owned
input hashes unchanged and both private configstorage INIs restored.

S27a is PARTIAL: an initial Enter trial failed for a long executable (Windows
Security) and a long ZIP (error3). No security barrier bypass/settings change.
That import was removed; fileswn2.cpp matches the accepted source. S27a2 must
resolve and validate Shell/archive boundaries before enabling Enter. Later
S27b–f/S28–S32 remain deferred. No new push, PR, CI or multi-phase goal.
Details [S27a1](steps/S27a1.md); evidence ../evidence/S27a. Existing S26b Debug
menu-heap limitation remains documented; this step does not claim to fix it.

## S26a — IMPLEMENTED / association preparation lifetime prerequisite PASS (2026-10-23)

Scoped donor AssociationPreparation/shared PreparationLifetime/PreparationSession
and Shell association epoch/cache/reentrancy guards imported ahead of first
panel publication. Preparation caches answers without mutating live icon records;
association release invalidates epoch and cached answers; reentrant insertion
rechecks the index before inserting. Header/helper/native fixture content exact
donor. Target GetFileIconForPanel HANDLES_ADD accounting kept; unrelated donor
removal excluded pending independent accounting audit. No Branch panel metadata,
activation, polling or user command added yet; S26 overall remains IN PROGRESS.

Native branch_preparation_tests/project EXACT:17 checks/config Debug/Release PASS
including deduplication, time/query limits, cancellation, argument ownership,
clear/replace/restart during callback and stale revision. These use deterministic
clock/resolver, not real third-party Shell reentrant callbacks. Actual Shell cache
integration is compiled and audited; no simulated callback claimed GUI proof.
Host x64 Debug/Release /warnaserror PASS46.92/81.10s final; initial Release95s link
timeout retained. All24 source/menu groups PASS. Initial panel-rendering contract
expected direct QueryShellAssociation call; imported exact donor update retaining
original query/normal cache assertions and additionally tracing prepared lookup.
Normal panel GUI D/R shows txt and unknown s26icons extension in long Unicode
parent, correct known/fallback icons; normal exit/no new reports. All4 own hashes
unchanged, private configstorage.ini restored; installed host untouched.

Evidence ../evidence/S26a manifests/bounded logs. No SDK interface/layout, public
gate or user preferences changed. No push/PR/CI. Next S26b must integrate full row
identity + icon/property lifetime before any experiment activation, default-off
runtime gate and read-only dispatch, paired resources and actual GUI acceptance.


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


## S24 — ALREADY COVERED / formatter audit PASS (2026-10-23)

No duplicate implementation: S20c already imported exact pinned donor typed
branch_view_text.h and native project, actual compressed-size caller in fileswn6.
Verified helper content parity ignoring checkout EOL only. Native test source
diff contains only retained future Branch-resource filter and truthful summary;
all status/general/compression formatter assertions match donor. Existing source
comment names S27c for deferred resources; current plan schedules resource IDs
with S26 and operation-specific callers with S27c. No absent fixture called PASS.

Rebuilt native targets Debug/Release3.25/4.10s; actual formatter61 checks/config
PASS, including12 existing localized compression patterns, nulls, uint64, unknown
specifiers literal, percent payloads and long Unicode text without truncation.
Remaining84 Branch resource patterns remain explicit S26 acceptance work. No
runtime source changed, so no redundant host rebuild: preceding S23 Debug/Release
and normal panel smoke bind this exact production source revision a858096b.
Evidence ../evidence/S24/audit.json and bounded test logs. Documentation-only
closure; no new SDK, localization framework, public activation, push/PR/CI.


## S23 — IMPLEMENTED / exact array reservation PASS (2026-10-23)

src/common/array.h and array_reserve_tests.cpp/vcxproj copied EXACT from pinned
donor47661e92. Optional Reserve retains Count/items/state/Base/Delta on failure,
rounds to the existing growth lattice, checks overflow; empty reserved storage
is released by DestroyMembers/DetachMembers without reviving failed storage.
No new container, scanner, ordering or panel publication changes. Existing test
runner discovers the new project automatically; no runner edits required.

Actual shipped template native20 checks/config Debug/Release PASS, with only
realloc failure injection: failure does not poison later insertion, rounding,
negative/overflow inputs, insert/delete, empty cleanup, object lifetime counts.
Common-header consumer audit: host+Salmon x64 and Translator+Trace Server Win32
(their supported platform), all8 Debug/Release /warnaserror builds PASS. Host
52.09/82.82s final; initial Release95s linker timeout retained separately. Native
builds2.69/2.99s. The plugin SDK does not include this common header; no unrelated
30-plugin SDK rebuild claimed. reglib source consumer is compiled in host;
historical standalone REGLIBA is outside current supported main solution.

All24 source/menu groups PASS. Actual normal panel smoke D/R: two files in >260
UTF16 Unicode directory, explicit selection50/54 bytes; normal exit/no reports.
All4 fixture hashes unchanged; private configs restored. Hashes/evidence under
../evidence/S23. No public gate, SDK layout, user configuration, push/PR/CI change.


## S22 — IMPLEMENTED / archive callback and ZIP smoke PASS (2026-10-23)

Pinned donor CPanelTmpEnumData owned relative UTF8 name/link-size cache and actual
PanelEnumDiskSelection file callback ported; existing root+relative SDK ABI and
ZIP/7-Zip engines unchanged. DiskDirBuildEnumeration uses full item identity.
ADAPTED donor IsBranchView guard to explicit ExactFileSelection state, false by
default and enabled by Pack only for file-only disk selections. Directories/mixed
selections retain original recursive tree. SourcePanel is initialized; no fake
Branch methods. S27e will connect real Branch selection. No Branch activation.
Fixture driver EXACT; cpp.in ADAPTED solely for route state binding instead of
future Branch method; all assertions retained. Registered as native-generated.

Actual production callback/state/converter fixtures362/362 per Debug/Release
PASS: duplicate basenames in separate parents, all enum modes, ordered reset,
64-bit metadata, pointer lifetime, invalid index/identity/conversion/allocation
failure, cached link sizes/ignored errors across move-packer source removal.
Dependencies are deterministic fixture panel/link replies, not actual packing.
Host x64 Debug/Release /warnaserror PASS36.42/93.93s; final restored Debug8.65s.
All24 source/menu groups PASS. GUI actual ZIP plugin Debug/Release: ordinary.txt
and Czech/CJK/emoji name inside Unicode parent, correct exact archive member
names and decompressed SHA256. All4 input hashes unchanged. Release normal exit.

Debug ZIP creation uncovered shutdown heap warning, not hidden or called clean:
641 bytes/21 blocks, ten system MENU48+16 pairs and one1-byte01 block. Temporary
heap report logging and explicitly disabled new callback route reproduced the
same exact allocation profile using legacy callback/ASCII inputs. This isolates
the warning from the S22 callback; underlying allocator origin remains unresolved.
Diagnostic patches/logs retained outside Git; heap.cpp byte restored, fileswn7
production manifest rechecked, final production Debug rebuilt. Warning closed
normally; no process/report remains. No suppression or unrelated engine fix.

Private configstorage.ini restored; installed user host untouched. Evidence in
../evidence/S22 includes hashes, legacy comparison and actual ZIP validations.
No real Branch packing/move-packer GUI claim (S27e). No push/PR/CI.


## S21b / S21 — IMPLEMENTED / ordinary Shell integration PASS (2026-10-23)

Pinned donor Shell adapters/call sites ported into ordinary disk panels. Exact
CreateIDataObjectForPaths/CreateIContextMenu2ForPaths wrappers and declarations;
scoped Properties/attribute wide identity hunks; disk name/full/UNC clipboard
route. ADAPTED donor Branch-only guard to Is(ptDisk), as with S20 ordinary
operations: disk identities are already available; no Branch placeholder API.
Archive/plugin virtual paths keep their original route. Selection menus bind the
real parent; Unicode verb directory supplied. Disk copy/cut publishes the full
exact data object and drop effect atomically. Cut matching uses full identity.
No Branch dispatch/public activation; mixed-parent host fallback stays S27d.
S20c exact-wide recycle-volume eligibility repair preserved.

Host x64 Debug/Release /warnaserror PASS11.79/44.08s. Existing S21a native helpers
unchanged (28+33 checks/config); not claimed rerun here. All24 registered source/
menu groups PASS; operation scope extended8 to10 with exact disk Shell/text
boundary regressions. Initial runner invocation lacked MSBuild PATH and failed;
final bounded invocation supplies baseline environment and passes.

Actual GUI Debug: Unicode file in >260 UTF16 path, correct real-parent context
menu; single Shell Properties29 bytes, exact full Unicode text; Copy/Cut exact
CF_HDROP and preferred effects5/2. Release: explicit2 files, Copy/Cut exact whole
selection/effects, multi Properties2 files56 bytes. Initial separate clipboard
reads observed Chromium text or a transient lock; final bounded reader runs
before UI action to observe publication without inter-tool overwrite. No claims
for failed reads or dedicated UNC/network GUI smoke. Native UNC cases in S21a.
All4 own file hashes intact. Both hosts normal exit, no new reports; private
configstorage.ini restored. User installed host untouched. Evidence ../evidence/
S21b ties runtime patch and final source/test hashes. No push/PR/CI.


## S21a — IMPLEMENTED / exact Shell helpers PASS (2026-10-23)

EXACT pinned-donor branchshell.h/branchclipboard.h and full native
branch_context_menu_tests + branch_operations_tests/projects. Host IDE header
registration scoped only; existing native runner discovers both targets without
new runner code. S20c operationrecycle.h wide-volume correction preserved exactly.
No host dispatch/call site changed; Branch View remains unavailable.

Host x64 Debug/Release /warnaserror PASS5.04/5.79s. Native context menus28 checks
per configuration PASS using actual Windows Shell canonical verbs versus independently
bound real-parent reference, UTF8-byte-long and Win32-long identities; mixed-parent
menu returns explicit unsupported rather than Desktop/first-parent menu.
Native operations33 checks/config D/R PASS: whole-selection CF_HDROP/CIDA, copy/move
drop effect, mixed-parent Properties data, Unicode/long paths, missing/unsafe
identity failure and actual recycle including locked-item failure. Fixtures own
unique temporary children and guarded cleanup. No real clipboard publication or
Properties dialog claimed: native IDataObject is the object under test.
All24 source/menu contract groups PASS. Source/patch hashes and bounded logs in
../evidence/S21a. GUI host runtime is unchanged by header-only registration;
actual ordinary-panel host integration smoke belongs to immediately following
S21b. No SDK/layout, user config, public gate, new push/PR or CI changes.


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


## S20c / S20 — IMPLEMENTED / local operation gates PASS (2026-10-23)

S20a rename/owned selection and S20b copy/move accepted separately; S20c completes
ordinary delete/recycle, attribute and compressed-size owned-wide path hunks.
Worker DoDeleteFile receives SourceNameW and uses wide delete/readonly/attribute
APIs; target NOHANDLES copy accounting repair is preserved. operationrecycle.h
ADAPTED from donor; branch_view_text.h EXACT generic typed formatter, no Branch
state/UI. Host ordinary-disk recycle queues the whole exact wide selection before
performing it, rejects ambiguous shell components and forces recycle semantics.

Real GUI exposed a pre-existing additional boundary defect: the configured all-
files recycle policy was bypassed by ANSI MyGetDriveType on long Unicode parent.
Initial Debug own3 files were deliberately deleted; no own matching recycle
records. Fresh Release confirmed policy1 in UI but normal Delete still requested
direct deletion; canceled. Local OperationRecycleDriveTypeW uses wide volume-root
resolution, including mounted volumes, before GetDriveTypeW. This adaptation is
required so an already-wide shell operation is reachable. Initial evidence kept;
new unique fixture used for final GUI. No unrelated MyGetDriveType callers changed.

Final host Debug/Release x64 /warnaserror PASS9.86/87.93s. Earlier Release95s
post-link timeout and initial builds retained, not counted as final success.
Native recycle-only adaptation of donor branch_operations_tests:15 checks/config
D/R PASS; five actual per-item recycle paths include duplicate basenames in
different parents, long Unicode, UTF8>260/UTF16<260, missing and locked failure;
new exact-wide volume gate check. These exercise production recycle helper,
not the full panel queue. Full shell/clipboard native fixture remains S21.
Native typed formatter61 checks/config D/R PASS, including all12 real compressed-
size localized patterns; future Branch resource fixtures retained but skipped.
Project EXACT; source ADAPTED only to defer unconsumed Branch resource patterns.
All24 registered source/menu groups PASS, scoped ordinary operation contracts8;
future Branch assertions retained in All and not run/reported PASS.

Final GUI: Shift+Delete Cancel removes temporary selection and preserves explicit
selection (Debug2 files/57 bytes; Release3/126). Initial Debug3/114 also passed.
Debug normal Delete recycles100-CJK file from long parent; Shift+Delete deletes
ASCII+Czech/CJK/emoji remaining2. Release Shift+Delete deletes100-CJK file; normal
Delete recycles remaining2 as one queue. All3 recycle metadata paths and content
hashes match exact source identities (transport prefix accounted for). All6 own
sources absent as intended. 182 protected records intact, including177 previous
paths/streams, both sentinels and initial Release3 files. Final D/R normal exit
PASS; private runtime configs restored, installed user host untouched.

S20 ordinary operation scope complete. Branch-only options/resources/translations,
recursive mixed-parent planning and metadata remap remain S26/S27c, no public
Branch activation or dependency stubs. Attributes/count-size changes compiled;
no claim of dedicated attribute/compressed-size-error GUI test. Legacy dialog
subject shortening remains; I/O identity is full. No new SDK virtual changes.

The user-authorized goal through S20 is implemented in coherent local commits;
S11-S20 publication/full clean CI/CodeQL are still pending a later authorized
push. S10 was accepted into local incremental integration after its successful
current-head fork checks; validation draft PR33 is not merged into reference base.
SOURCE/CONTRACT and release-base pins unchanged. Evidence ../evidence/S20c/
source-manifest.json, final-validation.json, recycle-evidence.json and bounded logs.


## S20b — IMPLEMENTED / ordinary copy and move PASS (2026-10-23)

Scoped donor fileswn6 BuildScriptMain/Main2/File heap buffers and Unicode Copy-of
name generation; fileswn8 split-path dialog boundary uses exact item parent/base.
ADAPTED ordinary copy/move captures complete NameW as owned UTF8 before BuildName;
legacy display mirror may be truncated and must not identify the operation.
No Branch relative-path option/scheduling, archive enumerator or new resources.
These remain S26/S27c. Delete/attributes/count-size wide hunks remain S20c.

Host x64 Debug/Release /warnaserror PASS10.10/9.23s final. Initial missing include
compile failure fixed with donor widepath include; Release95s post-link timeout
preserved, subsequent complete build PASS. Production patch/source hashes saved.
Real F5/F6 Cancel D/R: temporary long-CJK selection removed; explicit3-file
selection126/138 bytes preserved. Actual copy then move of ASCII, Czech+CJK+emoji
and100-CJK names from long Unicode parent succeeds D/R. All12 final copy/move
outputs hash-identical; original own sources intentionally moved and absent.
Retested original S17 failing long-source Unicode symlink in BOTH configurations:
new outputs regular2107-byte files with exact payload hash; old links unchanged.
162 prior path/stream records and sentinel intact. Four normal shutdowns PASS,
private runtime configs restored, installed user host untouched.

All24 source/menu groups PASS, including4 scoped operation assertions. Actual
BuildName18 checks/config D/R PASS. Initial unsupported Both CLI invocation saved;
separate supported config runs pass. No full CI/CodeQL or push in this local step.
Known legacy dialog subject truncates100-CJK display label; operation paths and
output names are complete and verified. Future Branch assertions retained in All,
not reported PASS. Evidence ../evidence/S20b/{source-manifest,final-validation}.json.


## S20a — IMPLEMENTED / rename + temporary selection PASS (2026-10-23)

EXACT donor panel_temporary_selection.h and native fixture/project; scoped
rename/caller/project registration hunks ADAPTED from donor47661e92.
Disk snapshots use already-available GetItemIdentityW even before Branch View;
virtual panels retain Name fallback. Explicit user selection creates no temporary
snapshot. Wide rename takes actual item parent/path and invalidates all three
visible arrays. Branch refresh/remap hooks remain S26, no dormant Branch state.

Host Debug/Release x64 /warnaserror PASS (9.88/67.19s final builds). First Release
95s LTCG timeout preserved. Native21 checks/config D/R PASS, simulated rows
exercise production header identity matcher; not a whole-host cleanup harness.
Real F2 Cancel D/R: no explicit selection leaves zero selected; three explicitly
selected files remain selected (174/186 bytes). Actual rename of 100-CJK basename
at UTF16 path488/490 to Czech+CJK+emoji succeeds with identical content hashes.
Renamed focused row is unselected after success, the other two stay selected;
selection preservation claim applies to Cancel. Normal shutdown D/R PASS.
155 previous path/stream records and own six files/sentinel intact, two deliberate
renames accounted for; private configs restored, installed user host untouched.

Scoped operations source contract2 PASS; all24 registered source/menu groups
and49 pytest PASS. Ordinary path/service196 and getters12 checks/config D/R PASS.
Regression harness now declares ViewerEnumeration constants/discovery facade
introduced S18b; initial compile FAIL and concurrent compiler-env timeout saved.
Actual ViewerEnumeration implementation is covered by S18b, this is discovery
facade only. Future Branch operation assertions retained in All scope, not run
or reported PASS here. Copy/move and delete remain S20b/S20c; no new UI resources,
SDK vtable changes, push, full-solution CI or CodeQL in this local substage.
Evidence ../evidence/S20a/source-manifest.json, final-validation.json and logs.


## S19 — IMPLEMENTED / lokální worker brána PASS (2026-10-22)

Převzat izolovaný EXACT workerpath.h a copy/retry/ADS/directory-create/attribute
I/O hunks worker.cpp, se souvisejícími call sites. Access/sharing/create flags,
identity checks, sync/async scheduling beze změny. Fileswn6 jen strict UTF8/ACP
dekódování a owned-wide copy planning error probe; capacity/Branch planning/UI
zatím S20. BuildName počítá UTF16 délku místo UTF8 bajtů, chrání empty parent;
jediná lokální úprava donorového body je explicitní l2=0 pro /W4 definite
assignment warning bez oslabení /WX. Nesouvisející icon tracking hunk vynechán.
DoDeleteFile/Recycling a jeho ThreadWorkerBody call jsou DEFERRED S20;
operationrecycle.h zde není importováno. Branch View stále vypnuté.

Kritická zachovaná target odchylka: první donor import přepsal NOHANDLES wide
target opens na HANDLES_Q před společným HANDLES_ADD_EX. Reálný Debug copy
proto při exit hlásil jeden monitored handle. Read-only SymFromName/PDB a
ReadProcessMemory pouze vlastního PID10576 identifikovaly přesný worker.cpp
target open; byla obnovena již existující target oprava, nikoli vypnuto měření.
Nový source contract hlídá dvojici NOHANDLES opens. Původní FAIL, source/binary
hashes, výstup i diagnostika zachovány; nový Debug skutečný retry/copy/ADS a
normal shutdown PASS. Také nový Release PASS. Původní Debug výstup zachován
nezměněný, finální testy zapisovaly do jiného vlastního cíle.

Host D/R x64 /warnaserror PASS (finální Debug9.02s, Release79.27s). Předchozí
Release95s LTCG/post-link timeouty zachovány, neoznačeny PASS. Native EXACT
worker_path_tests9/config a worker_copy_path_tests11/config D/R PASS, včetně
skutečného Unicode NTFS streamu a readonly helper recovery. CopyReopen je malá
reimplementace copy loop nad skutečnými production helpers, není celý worker.
Nový acceptance harness kompiluje skutečný BuildName + konverze:18 checks/config
D/R PASS (UTF8>32767 bytes při kratším UTF16, hranice32766/32767, DOS fallback,
empty parent, SkipAll/Focus). UI callbacks stub, nikoli skutečné dialogy.
Driver opravuje extrakci komentáře Translator a používá host CRT compatibility
define + C++17, /W4 /WX ponechány. Počáteční harness selhání zachována v evidence.
Šest worker source tests + celý23 source/PowerShell groups PASS, pytest49 PASS.

Skutečný GUI D/R long Unicode source copy: externí read-only shareMode0 handle
vlastního souboru vynutil error32. Po explicitním release a kliknutí Retry
worker dokončil kopii; hlavní data i pojmenovaný Unicode ADS byte/hash exact.
Zámky měly85s deadline, oba uvolněny vlastním flagem do27s, nikdy zdroj neměnily.
Normal shutdown obou finálních instancí bez handle diagnostiky/reportů. 144
starších inputs, dva source files/ADS, sentinel i počáteční output/ADS nezměněny;
konfigurace obnoveny, žádné neočekávané soubory. UNC pouze prefix assertion,
reálný EFS/net share neověřen; READONLY recovery je nativní helper test.

Parent756565a8; runtime patch caf0774032a9098ca53af19f9670ecd626cb152b4a0bffba4bd6b3969df3f005. Evidence evidence/S19 obsahuje manifest,
runtime.patch, host/worker/copy/build-name/contracts/python logy, GUI error32,
lock readiness/release, původní handle diagnostiku a final-validation.json.
Celý native runner/full solution CI/CodeQL neběžely. Lokální/nepushed; následuje
S20 rename+selection, copy/move a delete jako samostatné koherentní podetapy.


## S18 — IMPLEMENTED / obě podetapy lokálně ověřeny (2026-10-22)

S18a commit 1a8f5b77 zachovává úplné launch argumenty/cwd (samostatná evidence).
S18b atomicky přenáší host-owned Salamander.ViewerEnumeration přes QueryService,
owned std::wstring request/result, UI dispatcher v ordinary panelu i Find,
wide internal viewer open/reload/title/navigation a PictView navigation/title.
Legacy char API explicitně odmítá >MAX_PATH-byte result bez posunu cursoru;
wide služba zachytí aliasovaný vstup před vyprázdněním výstupu a krátkou kapacitu
odmítne bez posunu indexu. Žádné změny existujících virtual metod nebo CFileData.
consts/fileswnb mají jen viewer hunks, bez Branch timer/state. SDK zachovává
ordinary-panel komentáře S04/S05; zip ponechává provenance a přidává jen route.
finddlg1, salamdr6, viewer.cpp/2/3, pět PictView souborů a native fixture EXACT.

Host D/R x64 /warnaserror PASS; první Release95s timeout v LTCG zachován,
opakování86.60s PASS. Všech 30 podporovaných SDK consumerů x2 konfigurace PASS,
konkrétní project paths a 60 logů v consumer-matrix.json. Historický Portables
vcproj2015_manison a nepublikovaný SFTP zůstávají excluded. Podporovaný Portables
vcxproj byl samostatně ověřen. První souběžný Salamatrix Release selhal na
sdíleném MarkdigRenderer native mezivýstupu; Debug PASS a samostatný Release
repeat8.61s PASS, WebView2 oba buildy následně sériově PASS. Nejde o opravu SDK
ani o zamaskování původního FAIL. ABI golden pět released interfaces + osm
HEAD interfaces zachovány; release 5.0-samandarin-0.15.1 prefix PASS.

Native viewer_enumeration_tests EXACT: 11 checks/config D/R PASS (identity,
reorder/removal, case/canonical Unicode distinction, long decode/fullpath,
capacity/canary a skutečné ANSI-window wide title). Toto nejsou testy celého
UI dispatcher timeout stroje. Snapshot regression57 Debug PASS; všech22
registrovaných source-contract/Windows PowerShell menu groups PASS; pytest49
PASS; skutečné shipped conversion_table_tests D/R PASS po host staging/restartu.
Celý native runner ani full solution CI/CodeQL znovu neběžely.

Skutečný GUI internal viewer D/R: long Unicode open, obsah, úplný title včetně
CJK/emoji, previous/next; CtrlR načetl nový marker z vlastního změněného souboru,
poté byl source obnoven byte-exact. PictView D/R render64x48, next/prev přes
službu mezi ASCII a Unicode basenames na stejné >MAX_PATH cestě, úplné titles.
První Debug F3 otevřel podle původního private seed interní viewer; následně
byla pouze vlastní testovací konfigurace nastavena na PictView JPEG slot16.
Loaded pictview.spl hash match. Normal shutdown všech vlastních instancí bez
reportů; 135 starších inputs + osm nových files + sentinel nezměněny,
configstorage.ini obnoveny. Vše v evidence/S18b; žádný test uživatelských dat.

Enter donor hunk ve fileswn2 je pouze pro IsBranchView a vyžaduje branch_view.h;
zůstává DEFERRED S26, nikoli nový ordinary launch. Běžný Enter nemá S18 donor
změnu. Branch View stále vypnuté. Parent1a8f5b77; runtime patch ce3854a5b4bd39fcf1ef1b8301503d4e7a91b0f098379885818b2d2e74dc2703.
S18 lokální/nepushed; pokračovat S19 worker paths podle uživatelského goalu.


## S18a — IMPLEMENTED / lokální launch brána PASS (2026-10-22)

Převzaty EXACT donor fileactionpath.h, viewerpath.h, execute.cpp/.h,
dialogs2.cpp, mainwnd4.cpp a čtyři native launch fixture soubory plus source
contract. Fileswn5 obsahuje pouze launch/ViewFile/EditFile hunks; user-menu
mainwnd3 hunks doplňují přímý common/widepath.h include místo budoucího
branch_view.h. Native projekt je automaticky objeven runnerem; všech 59
starších groups zachováno a source contract přidán. SDK/ABI nezměněny.

Host D/R x64 /warnaserror PASS. První Debug build selhal na chybějícím include,
po opravě PASS. První Release build překročil 95s deadline po link/staging;
zachován jako TIMEOUT, opakovaný incremental doběhl 8.17s PASS. Native launch
D/R 12 checks/config PASS: skutečné CreateProcess/ShellExecute/UTF8 batch,
ASCII, Unicode name/parent, byte-long a UTF16-long paths, cleanup. Helper
test připouští explicitní neprovedení dlouhého cwd při chybějícím DOS aliasu;
zde wide DOS alias test prošel a samostatné GUI D/R prokazuje skutečný launch.

Skutečný GUI F4 a AltF3 D/R spustily fixture child z panelu s dlouhou cestou,
češtinou/CJK/emoji. Čtyři UTF16 záznamy přesně odpovídají celému full-name
argumentu a skutečnému cwd po rozvinutí DOS aliasu. 128 předchozích inputs,
dva nové sources a sentinel nezměněny; normální shutdown bez reportů,
konfigurace obnoveny. Shipped conversion_table_tests D/R PASS, sedm source
contracts PASS, pytest 49 PASS. Full runner/full solution CI/CodeQL NEBĚŽELY.

S18 není celá dokončená: následuje S18b enumeration service + PictView
navigation a internal viewer open/refresh/title/previous-next. Enter donor
hunk je pouze Branch-specific a patří S26. Branch View zůstává vypnuté.
Parent 9c6f0643; runtime patch SHA256 186939ddb76fb52a8bf44288a1932bae920f8dcd501b965ed7d777d8602cfce2.
Evidence evidence/S18a source-manifest.json, launch-*, host-*, conversion-*,
source-contract.*, python.*, GUI text/launch/hash manifests a final-validation.
S18a je lokální bez push/PR.


## S17 — IMPLEMENTED / lokální brána PASS s evidovanou S20 copy mezerou (2026-10-22)

Jediný production diff je SalGetFileSize2 v salamdr5.cpp; celý soubor je nyní
EXACT donor 47661e92. Funkce validuje UTF8 před ACP fallbackem, používá wide
open a heap absolutizaci dlouhých relativních cest, zachovává extended paths,
vrací přesný error i nulovou velikost při selhání. S04 ordinary getters,
membership služba, SDK/ABI a snapshot zůstávají beze změny. Žádný nový index
nebo Branch metadata stub. Branch View je stále vypnuté.

Oba disk_file_size_paths_tests soubory EXACT donor: skutečné source functions
a file I/O D/R 79 checks/config PASS, včetně Unicode/UTF8-byte-long/UTF16-long,
relativních a extended cest, symlink target size, ACP1250 fallback, null/missing
paths, error output a cleanup. Symlink subcase zde proběhl, nebyl SKIP.
Branch panel getters driver/template ADAPTED na tři skutečné ordinary gettery
ve fileswn1.cpp a existující wide helpers: 12 checks/config PASS. Všechny
donor Branch assertions zachovány pod S26 guardem, neprovedeny a nejsou PASS.
Existující panel_item_paths_tests znovu 196 checks/config D/R PASS: source,
target/focus, current membership/stale pointers, virtual rejection a frozen ABI.

Host D/R x64 /warnaserror PASS, pytest 49 PASS. Běžný GUI file-link copy D/R
PASS: krátká ASCII cesta link.bin ukazuje na vlastní >400-unit Unicode cíl.
Listing symlinku 0 bytes, skutečný výstup regular file 2107 bytes s přesným
SHA256 cíle. Host binary/source/runtime hash bound, normální shutdown bez
reportů, konfigurace obnoveny. 119 starších input files, šest vlastních
sources/links a sentinel nezměněny; dva nové výstupy ověřeny, žádné temp zbytky.

Samostatný Debug GUI pokus kopírovat symlink přímo z dlouhé Unicode source
cesty selhal Error Opening File / path not found a byl Cancel bez výstupu.
Toto je FAIL zachovaný v evidence/S17/Debug-long-copy-open-error.txt, nikoli
PASS nebo oprava celé file-operation vrstvy. Její migrace je S20; znovu
ověřit tento konkrétní vlastní fixture v S20. Native nová size funkce dlouhou
source cestu i link target podporuje; GUI ordinary smoke není tvrzení, že
všechny host copy I/O větve už jsou migrované.

Parent 60724bd9; runtime patch SHA256
0c91e4246eadec647768d22502face1000cd5e6336f6444ecba5f92d787f9623.
Evidence evidence/S17: source-manifest.json, focused-size.*, ordinary-getters.*,
membership-regression.*, host-*, final-validation.json a GUI logs. Runner
přidává dvě groups, všech 57 starších zachováno. Celý runner/full-solution CI/
CodeQL znovu neběžely; S17 nepushed. Pokračuje lokální S18 podle user goalu.

## S16 — IMPLEMENTED / lokální brána PASS (2026-10-22)

Osm Undelete production souborů a C++ fixture jsou EXACT donor 47661e92.
Owned disk snapshot se zachytí před destination dialogem, Restore používá
skutečné wide identity, heap buffers a RAII source/destination/raw cleanup.
Connect image předvyplní skutečný focused full path, zobrazí jej wide a
před legacy image backendem explicitně kontroluje ACP roundtrip a kapacitu.
Legacy image engine nebyl rozšířen; nepodporovaný vstup je chyba, ne truncation.
Library precomp má potřebné STL/SDK/path includes, celý Undelete consumer
se normálně kompiluje. Veřejný plugin SDK/ABI se nemění.

Python driver má pouze configuration selector (default Both). Native fixture
68 checks/config D/R PASS: skutečné entry/restore/SDK/host snapshot/path
functions a dočasné disk I/O; listing zánik, duplicate basenames, kolize,
focused source, UTF16-long target/source, UTF8-byte-long source, recursive
restore, own-descendant refusal, cancel, missing/create failure a cleanup.
EFS flow/callback/context error checks jsou STUB, real EFS calls=0; nejde
o Windows encrypted EFS import/export. Legacy path tests kontrolují ASCII,
oversize a terminator capacity; lossy ACP assertion je podmíněna 1250/1252.
Registry ACP zde 1250 je evidováno samostatně, není to tvrzení o ACP každého
konkrétního procesu. Donor conditional assertion nebyla oslabena ani odstraněna.

Host+Undelete včetně vlastních library sources D/R x64 /warnaserror PASS,
pytest 49 PASS. Skutečný GUI Connect image D/R PASS: vlastní focused
ž-東京.img v >400-unit cestě je v edit control přesně celý (UIA porovnáno
s manifestem). Po výslovné volbě Disk Image je vstup odmítnut hláškou
The filename or extension is too long před backendem; následně Cancel.
Fyzické disky nebyly analyzovány ani mountovány. Načtené undelete.spl hashově
odpovídají nové sestavě. Oba vlastní vstupy, 116 starších input files a sentinel
nezměněny, žádné temp zbytky, normální shutdown bez reportu, config obnoven.

Parent 8419831e; runtime patch SHA256
60407d0e492458a51c7b4683dfd5b124bf52766d111b759f45dc6cc4af5b4f4c.
Evidence evidence/S16/source-manifest.json, focused-undelete.*, host/undelete-*,
final-validation.json, *-connect-default/image-selected/image-limit-error.txt,
*-loaded-module.json, system-acp.json a shutdown logs. Runner přidává jednu
skupinu, všech 56 starších zachováno. Celý runner, full-solution CI/CodeQL
znovu neběžely; S16 nepushed. Pokračuje lokální S17 podle rozšířeného goalu.

## S15 — IMPLEMENTED / lokální brána PASS s testovacím menu adaptérem (2026-10-22)

Pět PAK production souborů dll/pak_dll.cpp, dll/pakiface.h, spl/pak2.cpp,
spl/precomp.h a vcxproj/pak.vcxproj a C++ fixture jsou EXACT donor 47661e92.
Owned paths a parents zachyceny před modal UI, per-parent notifikace a scope
cleanup. Optimize otevírá OP_WRITE_MODE | OP_EXISTING_ONLY; OpenPak tento
interní flag odstraní z desiredAccess a použije OPEN_EXISTING. Běžný pack
caller ponechává OPEN_ALWAYS. pakiface.h mění jen define, žádnou signaturu
nebo vtable/layout. Aktuální projekt kompiluje engine přímo do pak.spl;
historický dynamický paklib.dll loader je komentář, žádný starý paired DLL
nezůstává v runtime.

Python driver ADAPTED na configuration selector, aktuální shared snapshot
prefix (GetDiskSelectionService singleton, již existující worked-path stub,
vynechání nepoužívaného DemoPlug command shimu); donor C++ assertions EXACT.
15 checks/config D/R PASS: skutečný command body a SDK/host snapshot, listing
zánik, duplicates, per-parent notifications, cancel, UNC, archive-panel contract;
menu engine zde mock. Samostatně skutečné production OpenPak/ClosePak a wide
I/O helper: missing long Unicode input nevytvoří soubor, běžný caller jej
vytvoří, existing-only jej následně otevře; invalid path odmítnut.

Host+PAK D/R /warnaserror PASS, pytest 49 PASS. Optimize příkaz je v targetu
i donoru úmyslně odstraněn z Connect menu; samotný původní menu GUI je SKIP.
Pro skutečný command/engine GUI test použit explicitní dočasný AddMenuItem
adapter pouze v pak.cpp. Jeho combined runtime patch e2dc8bc50ff77a11bc694db
0028dc99843cbae0d0a63a071ebe51c84074de7d9 a testované binárky zachovány v
evidence/S15/gui-adapter; nemění ExecuteMenuItem ani engine. D/R GUI Optimize
na dlouhé Unicode cestě zmenšil vlastní PAK 4475 -> 379 bytes. Nezávislý
parser potvrzuje shodné jméno i bajty členu a odstranění 4096-byte mezery.
111 starších input files, dva nesouvisející PAKy a sentinel nezměněny, missing
inputs nevznikly, žádné temp zbytky, normální shutdown bez reportů, config
obnoven. Soukromý seed přidal PAK slot31; neúspěšné seed pokusy a jejich
příčina (povinný Copyright field) zachovány odděleně.

GUI adapter poté byte-exact odstraněn; pak.cpp nemá žádný diff a finální
production PAK D/R znovu /warnaserror PASS. Netvrdíme, že skrytý příkaz je
dostupný v běžném finálním menu. Původní světle vykreslený Optimize dialog
v Dark Mode je baseline, nebyl přidán nový dialog nebo změna resources.

Parent 9c738940; production runtime patch SHA256
639606ad29e584288b4518ff2360aac59ff84c060f4d13d527516480dff551a6.
Evidence evidence/S15: source-manifest.json, focused-pak-v3.*, host/pak-*,
final-validation.json, gui-adapter/* a private seed provenance. Initial harness
compile chyby jsou zachovány jako FAIL, v3 na finálním zdroji PASS.
Runner přidává PAK group, všech 55 starších zachováno. Celý runner, čistá celá
sestava a CodeQL znovu neběžely. S15 nepushed; navazuje lokální S16.

## S14 — IMPLEMENTED / lokální brána PASS (2026-10-22)

7zip.cpp, precomp.h a vcxproj/7zip.vcxproj jsou EXACT donor 47661e92.
TestArchive zachycuje FOCUSED_ONLY snapshot; skutečná úplná cesta a texty
jsou připravené před progress dialogem. Formátování používá heap string,
capture/invalid UTF16 selhání neodhadují root + basename. Engine, virtual
archive kontrakt, veřejné ABI a host-managed packing/S22 nejsou změněny.

Celý archive_menu_selection_tests.cpp.in je nyní EXACT donor; všechny
7-Zip assertions odložené v S13 obnoveny. Python driver má jedinou adaptaci:
import existujících disk-selection compiler/extraction helpers místo budoucího
renamer driveru. Runner už neomezuje scope na ZIP; počet groups zůstává 55.
Archive harness 34 checks/config D/R PASS: ZIP + 7-Zip Unicode/UNC/long paths,
capture failure, directories, invalid UTF16, corrupt/cancel a progress cleanup.
Engine a snapshot v harnessu jsou test doubles, žádný packing test.

Host+7-Zip a tranzitivní dependencies D/R x64 /warnaserror PASS, pytest 49 PASS.
Skutečný GUI 7-Zip Test Archive D/R PASS na dlouhé Unicode cestě: vybraný
ordinary.zip, focused ž-東京.zip; oba výsledky potvrzují právě ž-東京.zip is OK.
Načtený 7zip.spl odpovídá launch hashům. Engine DLL jsou transient a při
výsledkovém dialogu už unloaded; jejich staged hashes jsou zvlášť evidované,
netvrdíme snapshot jejich načtení. Šest vlastních archivů nezměněno, 12
nezávislých kontrol jejich členů, 104 předchozích input files a sentinel
nezměněny, žádné temp zbytky; normální shutdown bez reportů, config obnoven.
Runtime používal původní soukromý seed s 7-Zip asociací. GUI nekryl 100-CJK
basename; zděděné omezení ZIP menu ze S13 zůstává evidované a neopravené.

Parent baac372d; runtime patch SHA256
504afa34fd152d0b753a293cbba170034241e128f762593a53073142f7d9ded2.
Evidence evidence/S14: source-manifest.json, focused-menu.*, host/sevenzip-*,
final-validation.json, *-unicode-ok.txt, *-loaded-modules.json a *-shutdown.json.
Relevantní lokální brána PASS; celý runner ani full-solution CI/CodeQL znovu
neběžely. S14 nepushed. Další lokální etapa S15 podle rozšířeného user goalu.

## S13 — IMPLEMENTED / lokální brána PASS s baseline omezením menu (2026-10-22)

Tři produkční ZIP soubory main.cpp, precomp.h a vcxproj/zip.vcxproj jsou
EXACT donor 47661e92. ExecuteMenuItem používá jednou zachycený owned snapshot,
respektuje skutečné parents a explicitně odmítá SAL_MAX_PATH byte limit bez
truncation. Archive panel ponechává původní archivní kontrakt. ZIP engine ani
host-managed PackToArchive nebyly změněny; PackToArchive zůstává S22.

Archive menu test driver/template jsou ADAPTED: compiler helpers z již
existujícího disk_selection_snapshot_tests místo budoucího renamer driveru,
scope ZIP/All (default ZIP) výslovně odkládá pouze 7-Zip assertions do S14.
Všechny původní assertions zůstávají ve zdroji. ZIP scope 19 checks/config
D/R PASS: skutečný command body, úplné cesty při zániku listing, ASCII,
Unicode, UNC, UTF8 byte/UTF16 long paths, directories, capture failure,
notifications, formatting, invalid UTF16 a engine byte limit. Host snapshot
a engine jsou zde test doubles; toto není skutečný packing test.

Host+ZIP a tranzitivní dependencies D/R x64 /warnaserror PASS, pytest 49 PASS.
Skutečný ZIP engine přes GUI Test Archive: Debug dvě vybrané položky
ordinary.zip a ž-東京.zip, Release samostatný focused ž-東京.zip bez výběru,
vše v dlouhé Unicode cestě, každý výsledek is OK. Načtené zip.spl hashově
odpovídají právě sestavenému pluginu. Šest vlastních archivů nezměněno,
12 nezávislých ověření obsahu členů, 97 předchozích input files a sentinel
nezměněny, žádné temp zbytky, normální shutdown bez reportu, config obnoven.

Původní soukromý seed měl ZIP asociaci na 7-Zip. Pouze vlastní nový seed
pro S13 ji nastavuje na ZIP index -1, uživatelské nastavení nedotčeno.
Po správné asociaci se samostatně potvrdilo baseline omezení: menu při
výběru 100-CJK basename používá zkrácený CFileData.Name a zakáže příkaz.
Tento GUI případ je SKIP, nikoli PASS; ExecuteMenuItem a nové wide vstupy
se přes něj nespustí. GetMenuItemState je beze změny shodný s donorem.
Omezení musí být vyřešeno před tvrzením úplné GUI podpory takových názvů.

Source parent ac8641ac, runtime patch SHA256
a8c95db05b67ad0545a1edc5374183b341460ff97132c3622d8f12f6d6b1e1b4.
Evidence evidence/S13/source-manifest.json, final-validation.json,
focused-menu.*, host/zip-*.json, *-ok.txt, *-loaded-module.json a private seed
provenance. Runner zachovává starších 54 groups a přidává jednu; celý runner
znovu neběžel. Full-solution CI/CodeQL a push NEPROVEDENY. Pokračuje S14.

## S12 — IMPLEMENTED / lokální brána PASS (2026-10-22)

Navazuje na samostatný WIC baseline předkrok 270f2cb6. Owned snapshot
CEnumFiles a wide EXIF regenerace thumbnailu tvoří jeden paired krok.
thumbs.cpp a EXIF exif.c/.h/.def jsou EXACT donor 47661e92; nový C++ test
je také EXACT. Projekt adaptován pouze o spl_diskselection.h, Python driver
pouze o --configuration (default Both zachován). S18 navigation/title zůstává
DEFERRED. Testy S05 obnovují skutečný CEnumFiles a obě HostTests assertions,
zachovávají target DemoPlug a released SDK ABI kontroly.

Aktuální zdroj: parent 270f2cb6; runtime patch SHA256
254d9562832c4c8e17fb1bce6664a6a0a149acce910db9cea53585c9edfd1c52.
Host+PictView+paired EXIF D/R x64 /warnaserror PASS; skutečná staged EXIF
v10, wide i legacy export a bezpečné I/O: 51 checks/config PASS. Zahrnuje
Unicode/long path, temporary cleanup, původní obraz, read-only a locked
failure bez poškození originálu. Snapshot test 57 checks/config PASS,
WIC callback regresní test 39 checks/config PASS, decoder contract PASS,
pytest 49 PASS. Runner přidává thumbnail skupinu a zachovává všech 53 starších.
Celý runner znovu neběžel: jde o relevantní lokální bránu; starší initial
S12 timeouty a shard výsledky jsou zachované, nejsou aktuální full-run PASS.

Skutečné GUI D/R Update Thumbnail PASS: 3 vybrané JPEGy/config v dlouhé
Unicode cestě, ordinary.jpg, ž-東京.jpg a 100 CJK basename. Všech 6 výstupů
má platný JPEG thumbnail v EXIF IFD1; orientation=6 zachována a všechny
non-APP/COM JPEG segmenty včetně compressed scan bajtově shodné s originály.
Skutečně načtené PictView/EXIF DLL odpovídají launch hashes nové sestavy.
Předchozích 87 input files, 3 vstupy předkroku i sentinel nezměněny; žádné
temp zbytky, normální ukončení host/reporter bez reportu, konfigurace obnoveny.
WIC je skutečný dostupný engine; předkrok reuse encoderu není nový engine.

Evidence evidence/S12: source-manifest.json, resumed-final-validation.json,
resumed-*.json/log, *-launch.json, resumed-*-loaded-modules.json.
Starý runtime e22210d1 a původní neúspěšný GUI pokus zůstávají jako initial
evidence; po předkroku byla brána znovu provedena na aktuálním zdroji.
S12 nepushed; full-solution CI/CodeQL NEPROVEDENY. Pokračuje lokální S13.

## S12-preflight — IMPLEMENTED / lokální brána PASS (2026-10-22)

Samostatná oprava existujícího WIC backendu před obnovením S12. Původní
blob ef72188c53a6be5a713841475f654c96b102885d je shodný v targetu, release
base i donoru 47661e92. AddExtendedPrefixIfNeeded používal SAL_MAX_PATH
místo Win32 MAX_PATH; 415-unit JPEG tak skončil jako Shell bitmapa.
sPVSaveImage navíc odmítal NULL filename a neuměl PVSF_USERDEFINED_OUTPUT.
Oprava používá původní SaveFrame encoder přes seekable memory IStream,
pak kontrolované 64KiB callback writes. File output zachovává původní encoder.
Zrušení funguje před encodingem a při předávání výstupu; nepřerušuje vnitřní
WIC encoding. Celý encoded output je v paměti, limit legacy callbacku DWORD.

Host+PictView+EXIF Debug/Release x64 /warnaserror PASS, EXIF stále v9.
39 native checks/config skutečného prefix helperu a callback adaptéru PASS;
encoder dependency v tomto malém harnessu je stub. Doplňující skutečná
Release pictview.spl přes přesné PDB symboly: 17 checks PASS, dlouhá Unicode
cesta rozpoznána jako JPEG, platný 32x24 callback JPEG nezávisle dekódován,
short-write/cancel/invalid descriptor a běžné JPEG/PNG/TIFF file saves PASS.
Native decoder D/R build a runtime PASS, decoder source contract PASS,
pytest 49 PASS. Původních 52 runner groups zachováno, přidána jediná skupina;
celý runner v tomto předkroku znovu neběžel.

Skutečné GUI Update Thumbnail v obou konfiguracích nad vlastní krátkou
ASCII cestou PASS: 714 -> 1761 bytes, 1005-byte JPEG thumbnail v EXIF IFD1,
orientation 6 zachována, všechny non-APP/COM JPEG segmenty a compressed scan
bajtově shodné. Skutečně načtené plugin/EXIF DLL hashově evidovány, normální
ukončení bez reportu, konfigurace obnoveny, 94 předchozích input files a vlastní
sentinel nezměněny. GUI dlouhých cest patří obnovené S12, není zde tvrzeno.

Evidence evidence/S12-preflight; source-manifest.json, gui-final-validation.json,
actual-release-backend-v2.json, *-loaded-modules.json a bounded command logs.
Source parent 45d4a3e7; runtime patch SHA256
437e6119b04185901f98efcee245431e89c5715018a78dbdcf0c3d7bcbcf8d3c.
S12 donor candidate e22210d1 je odložen mimo Git; jeho starší výsledky nejsou
validační branou tohoto předkroku. S12 musí být obnovena a znovu ověřena.
Předkrok nepushed; full-solution CI/CodeQL pro něj NEPROVEDENY.

## S11 — IMPLEMENTED / lokální brána PASS (2026-10-22)

S10 přijato 60987d42 po zeleném PR #33 včetně skutečného CodeQL a celé
sestavy. Lokální S11 přebírá tři EXACT donor 47661e92 soubory Salamatrix:
salamatrix_packages.cpp, salamatrix_sides.h, vcxproj/salamatrix.vcxproj.
Shared context JSON vychází z owned wide snapshotů; native ItemInfo ABI,
JSON schema, 64-item limit a pět providerů se nemění. Native ItemInfo
překročení odmítá; JSON neprochází jeho fixed byte kapacitami.

Adaptace jsou pouze v testech/runneru: All scope aktivuje S10 odložené
TestSides/TestDynamicJsonContext a všech pět forwarding contracts. Nový
TestPayloadBoundaries kompiluje skutečné CopyResult a protocol header;
ověřuje exact/undersized/null buffer, reálný 65536-byte response limit,
1MiB whole-frame hranici, escaping a nezávislé JSON parsování celé struktury.
Žádné oslabení donor assertions; starší Automation-only scope stále 161
checks/config. All má 243 checks/config. Host+Salamatrix+Python provider
D/R x64 /warnaserror PASS, 52 native groups/config, Python 49 PASS.

Skutečný Python worker -> PackageManager::HostDispatch v D/R PASS: dvě
selected položky, focused 255 UTF16-unit / 763 UTF8-byte CJK basename,
plná 652-unit cesta. Oba hosty/reporter normálně zavřeny, žádný nový report;
3 nové a 84 předchozích input files hashově nezměněné. Soukromé konfigurace
obnoveny, vlastní testovací extensions archivovány mimo runtime. GUI není
běh všech pěti providerů; pět forwarding/capacity kontrol je source contract,
existující process-runtime testy mají skutečný Python/PowerShell a PHP SKIP.
PHP a LoadImageW subcase SKIPs zůstávají SKIP. Evidence evidence/S11,
source-manifest.json a final-validation.json; [steps/S11.md](steps/S11.md).
Runtime patch SHA256 970d582d38b9dbca897f7984a6ff15881117e1676d5c43236163d329fb766b28.

S11 nepushed; publikace/full-solution CI/CodeQL pro S11 NEPROVEDENY.
Podle rozšířeného user goalu po lokálním commitu pokračuje S12. Dřívější
baseline S10 Debug handle a file-focus setter vady nejsou touto etapou opraveny.

## S10 — publikováno a přijato (2026-10-22)

Automation 8ef4ab24 přijato po všech 12 SUCCESS checks aktuálního headu
[PR #33](https://github.com/majklvi/salamander/pull/33). Skutečný CodeQL job
119984548335 v runu 39824113333: Initialize, Release x64 MSBuild, Analysis
a SARIF SUCCESS; dokončen 17:53:32 UTC. Čistý full build 39824142309
SUCCESS Win32/x64/ARM64 včetně přípravy závislostí. Jeho CodeQL je záměrně
SKIP pro dispatch, skutečná analýza doložena v PR. Evidence evidence/S10:
accept-current-pr.stdout.log, check-full-build.stdout.log,
check-pr-build.stdout.log a publish-summary.json. Lokální baseline výhrady
(Debug handle tracking, starý dlouhý file-focus setter) i PHP/LoadImageW
subcase SKIPs zůstávají platné, zelené CI je neopravuje.

Přijímá se lokální checkpoint; interní draft se nemerguje do reference base.
Uživatel rozšířil zadání na postupné lokální etapy S11 až S20: zachovat
samostatné commity a každou lokální validační bránu; toto výslovné zadání
nahrazuje dřívější omezení na jedinou etapu v běhu. Další publikace/PR
nejsou tímto automaticky zadány. SOURCE/CONTRACT piny se nemění.

## S10 — IMPLEMENTED / lokálně ověřeno s baseline omezeními (2026-10-22)

S09 přijato na f13b3ed1 po ověření PR #32 a čisté sestavy. Lokální větev
integration/pr769-s10-candidate přebírá pouze Automation: vlastněné wide
COM položky, sdílený disk snapshot collections/enumerators, nezávislé cursory,
Reset bez nového capture a samostatné focused položky. SDK/COM IDL/host,
Salamatrix, CI a SOURCE/CONTRACT piny beze změny. Sedm produkčních souborů
EXACT donor 47661e92; itemcoll.cpp ADAPTED pouze inicializací iKey=-1 po
prokázaném MSVC C4701. Dva testy ADAPTED: explicitní Automation/All scope
odkládá pouze budoucí S11 a přidává skutečné COM collection/enumerator metody.
Všechny donorové Automation assertions zachovány, S11 assertions ponechány
v template/All režimu. Runner zachovává všech 51 starších skupin.

Host a Automation Debug/Release x64 /warnaserror PASS, 161 focused native
checks/config, 52 regresních groups/config a Python 49 PASS. Skutečný plugin
v D/R prošel 32 COM kontrolami/config nad devíti kategoriemi metadata/cest,
včetně spuštění skriptu na 344 UTF8-byte a 416 UTF16-unit cestě. Retained
items/collections/enumerators přežily deselection, refresh, změnu panel path
a další focused getter. Sort/destrukce řádků a poslední snapshot/COM release
ověřeny nativně nad skutečnými migrovanými metodami; GUI samo není důkazem
všech refcountů. Všech 13 finálních vlastních a 71 předchozích vstupů hashově
beze změny, konfigurace obnovena, oba finální hosty normálně ukončeny.

Baseline výhrady: původní scriptlist.cpp v Debug při raw CreateFileW a
tracked CloseHandle hlásí neevidovaný handle; před skriptem zaznamenáno a
potvrzeno, diagnostika nevypnuta. Původní host ChangeDir při přímém zaměření
souboru přes MAX_PATH vyvolal buffer assertion; první Debug host byl řízeně
ukončen, nejde o normální shutdown. Finální dlouhé cesty proto ověřeny přes
přechod do adresáře a skutečné COM Items. Tento starší setter limit zůstává
NEOPRAVEN; S10 nelze prezentovat jako úplnou podporu dlouhého file-focus
přechodu. Přesné místo a shoda s releasebase doloženy mimo Git.
PHP/LoadImageW subcase SKIPs zůstávají SKIP. Evidence evidence/S10 včetně
neúspěšných pokusů, source-manifest.json, runtime.patch, final-validation.json
a [steps/S10.md](steps/S10.md). Lokální runtime patch SHA256
cad448992989c7d69d9f6a90906765565eab8a6e1316f918979e447a0c13a7c7.
Publikační full build/platformní CI/CodeQL S10 NEPROVEDENY. S10 nepushed,
accepted reference zůstává f13b3ed1. S11 v tomto běhu nezahájena.

## S09 — publikováno a přijato (2026-10-22)

Split & Combine 8b97128d přijato po 12 SUCCESS checks přesného headu
v interním [draftu PR #32](https://github.com/majklvi/salamander/pull/32).
PR buildy Win32/x64/ARM64 /warnaserror, native/Python/source-contract,
VSIX, growth a squash guard prošly. Skutečný CodeQL job 119952659278
v PR runu 39814889856 dokončen 2026-10-22 16:48:06 UTC: Initialize,
Release x64 build, Perform Analysis a Upload SARIF SUCCESS. Samostatný
čistý full build 39814915560 SUCCESS na všech třech platformách včetně
přípravy závislostí. Jeho CodeQL záměrně SKIP pro workflow_dispatch;
skutečná analýza je doložena samostatně. Evidence mimo Git evidence/S09:
check-current-pr.stdout.log, check-full-build.stdout.log,
check-codeql-job.stdout.log a publish-summary.json. Lokální GUI/obsahové
důkazy a známé PHP/LoadImageW subcase SKIPs zůstávají evidované.

Přijímá se lokální checkpoint, draft se nemerguje do samandarin-0.16.
SOURCE/CONTRACT, donor/release piny, SDK a Branch gate beze změny.
Navazuje pouze S10 Automation; Salamatrix patří do S11.

## S09 — IMPLEMENTED / lokální brána PASS (2026-10-22)

S08 přijato na 68b66c33 po 12 SUCCESS checks PR #28, skutečné CodeQL analýze
včetně SARIF a čistém Win32/x64/ARM64 full buildu. Lokální větev
integration/pr769-s09-candidate přebírá jen Split & Combine: owned selection
před dialogem/I/O, skutečný parent pro default target/sibling discovery,
full paths explicitních dílů, wide API/dialogy a UTF8 BAT. Všech 10 produkčních
a 2 testové soubory jsou EXACT donor 47661e92. Jediný adaptační kód je
registrace selection_paths_tests.py s podporovaným --configuration ve
stávajícím runneru; všech 50 starších skupin zachováno. SDK/host/ostatní
pluginy/CI/Branch gate ani SOURCE/CONTRACT se nemění. Input volume/file IDs
se kontrolují před target creation; read handles bez write/delete sharing
chrání i před target-alias race. RAII zachovává handle/progress cleanup.

Host+plugin Debug/Release x64 /warnaserror PASS; native actual command/target/
plan/engine/BAT harness 210 checks/config PASS; celé native sady 51 groups/config
PASS; stávající CI pytest výběr 49 PASS. Host Debug prvně selhal až ve staging
skriptu na chybějícím Get-FileHash kvůli PSModulePath; opravena pouze invocation
environment, oba konečné buildy PASS. Neúspěšný protokol zachován. PHP runtime
a historický LoadImageW long-bitmap backend subcase zůstávají SKIP, ne PASS.

Skutečné GUI v produktovém Windows Dark Mode (experimental): Debug ASCII,
Debug Unicode UTF8-long a Release Unicode UTF16-long Split/Combine. Devět
vytvořených dílů porovnáno po bajtech a pořadí, tři výsledky přesně shodné
se zdroji; tři UTF8 BAT/metadata/CRC32 ověřeny nezávisle. Focus-only discovery
našlo správné tři sourozence a načetlo původní název (včetně % & ^ !).
Relativní nový Combine target se vztahoval ke skutečnému parentu. Skutečný
GUI pokus zapsat do input dílu i jeho hardlinku byl odmítnut bez změny dílů.
Aktivní Split i Combine nad 4GiB souborem byly zrušeny přes Cancel/Yes;
nedokončené outputy odstraněny, původní zdroj zachován. Všech 5 vlastních
+ 66 předchozích originálů má původní SHA256; přesný inventář 16 očekávaných
outputů navíc. Čtyři vlastní hosty/reportery normálně ukončeny, žádný nový
report, configstorage.ini obou konfigurací bajtově obnoveny.

Evidence mimo Git evidence/S09; testovaný parent 68b66c33 + runtime.patch
SHA256 18a3a0ac9ce186c114b295d266abeb5562dd6d0306fab394be4b92ff2ea267e2.
Source manifest ověřuje přesné donorové bloby/raw SHA i runner; launch logy
vazbu na skutečné host/plugin binárky. Finalize-S09.py znovu ověřil celý
řetězec po GUI. [steps/S09.md](steps/S09.md) uvádí příkazy a meze pokrytí.
Mixed-parent rows, alias race, resolver/read failure a odmítnutí adresáře
jsou native fixture; skutečný UNC share/BAT execution/pickery nebyly GUI
ověřeny. Publikační čistý full build/platformní CI/CodeQL S09 NEPROVEDENY,
větev zatím lokální. Další krok push/fork CI po zadání uživatele; po přijetí
S09 následuje S10 Automation. S10 v tomto běhu nezahájena.

## S08 — publikováno a přijato (2026-10-22)

Checksum c76b52b5 přijato po ověření všech 12 checks přesného headu
v interním [draftu PR #28](https://github.com/majklvi/salamander/pull/28).
Win32/x64/ARM64 Debug /warnaserror, native/Python/source-contract, VSIX,
growth a squash guard SUCCESS. Skutečný CodeQL job 116089251585 dokončen
2026-10-14 20:44:16 UTC: Release x64 build, Perform Analysis a Upload SARIF
SUCCESS. Čistý full build 38632382603 SUCCESS na všech třech platformách;
CodeQL v dispatch záměrně SKIP, skutečná analýza v PR runu 38632358631.
Evidence mimo Git evidence/S08/{resume-current-pr.stdout.log,
resume-full-build.stdout.log,resume-codeql-job.stdout.log,publish-summary.json}.
Lokální GUI/test důkazy a evidované PHP/LoadImageW subcase SKIPs platí dál.

Přijímá se lokální checkpoint; draft se nemerguje do samandarin-0.16.
SOURCE/CONTRACT, donor, release base, SDK i veřejný Branch gate se nemění.
Navazuje pouze S09 Split & Combine s exact input paths a input safety.

## S08 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S07 včetně obecné CI opravy přijato na dec9c828 po čerstvém ověření všech
10 checks PR #27, skutečné CodeQL analýzy/SARIF a čistého full buildu
Win32/x64/ARM64. Nová lokální větev integration/pr769-s08-candidate přebírá
Checksum jako jeden celek: owned capture před modeless UI/workerem, přesné
FullPath oddělené od display/export jména, wide rekurze/I/O/export a Verify
podle skutečného rodiče manifestu. Všech 9 produkčních a 2 testové soubory
odpovídají donorovým Git blobům 47661e92 beze změny. Jediná adaptace je
registrace testu do stávajícího runneru bez neexistujícího --configuration
přepínače (driver sám vždy testuje D/R). SDK, algoritmy, resources/formát
manifestu, disk-only guard, Branch gate a obecná CI oprava zůstávají zachovány.

Host+Checksum Debug/Release x64 /warnaserror PASS; native actual-consumer
harness 62 checks/config PASS, celé native sady 50 groups/config PASS,
CI pytest výběr 49 PASS. Native digest facade je stub; reálné algoritmy
ověřil skutečný plugin přes GUI: 80 nezávislých CRC/MD5/SHA1/SHA256/SHA512
porovnání nad osmi zdroji v D/R, 32 úplných exportních identit a digestů
(MD5 relativně, SHA256 v jiném Unicode adresáři), 32 pozitivních Verify
řádků. Negativní manifest přesně 1 Corrupted + 7 OK, 0 missing/skipped.
Zrušení aktivního výpočtu 512 MiB přešlo do Canceled bez exportu. Debug
Calculate/export přežil změnu source panelu a refresh po capture; native
queued worker přežil úplné zrušení panel rows včetně mixed-parent fixtures.
Všech 11 vlastních + 55 předchozích originálů má přesné původní cesty/obsahy;
jen 4 očekávané exporty navíc. Všechny 4 hosty/reportery normálně ukončeny
bez nového reportu, oba configstorage.ini bajtově obnoveny.

Vazba evidence: parent dec9c828 + runtime.patch SHA256
728f8592e28333d8d5b3e89ab8f348d0bc1202954afc36a5e20f8571a63f71b1,
donorové bloby/raw hashe a hashe skutečně spuštěného hostu/Checksum pluginu.
Podrobné příkazy a limity: [steps/S08.md](steps/S08.md), evidence mimo Git
v evidence/S08. Počáteční infrastruktura test runneru neměla msbuild v PATH;
po opravě pouze prostředí celá sada PASS, původní neúspěšný protokol zachován.
PHP a historický LoadImageW long-bitmap subcase SKIPs nejsou vydávány za
provedené testy. Nová publikační platformní CI/CodeQL a čistý full build S08
zatím NEPROVEDENY: větev je lokální. Accepted reference zůstává dec9c828.
Další checkpoint je push/fork CI po zadání uživatele, potom přijetí S08;
S09 Split & Combine nebylo zahájeno.

## S07 — publikováno a přijato (2026-10-14)

Batch Rename 3f9a0134 a obecná oprava full-build CI 6c1b034e přijaty po
čerstvém ověření všech 10 checks přesného headu 6c1b034e v interním
[draftu PR #27](https://github.com/majklvi/salamander/pull/27).
Win32/x64/ARM64 /warnaserror, native/Python, VSIX, growth a squash guard
SUCCESS. Skutečný CodeQL job 116018461833 dokončen 18:19:06 UTC: Initialize,
Release MSBuild, Perform Analysis i Upload SARIF SUCCESS. Samostatný čistý
full build 38611664410 SUCCESS ve všech třech architekturách včetně skutečné
instalace Lua a TagLib. Jeho CodeQL job je záměrně SKIP pro workflow_dispatch;
skutečná analýza proběhla v PR runu 38611675021. Původní job na 3f9a0134 byl
nahrazen novým headem a není vydáván za dokončený upload. Evidence mimo Git:
evidence/S07/{acceptance-current-pr.stdout.log,ci-fix-verified.json,
ci-fix-full-final.stdout.log,ci-fix-final-codeql-job.stdout.log,publish-summary.json}.

Přijímá se lokální integrační checkpoint, nikoli merge draftu do release base.
Draft zůstává otevřený; SOURCE/CONTRACT, donor a release base a84dbed6 se
nemění. PHP/LoadImageW subcase SKIPs zůstávají evidované. Navazuje pouze
S08 Checksum; smíšené rodiče zatím native fixture, veřejné Branch UI zůstává
vypnuté. S07 GUI/obsahové důkazy i známé Filecomp UI limity zůstávají platné.

## S07-CI — příprava závislostí čistého sestavení (2026-10-14)

Oprava společného .github/workflows/pr-msbuild.yml: workflow_dispatch
obchází Select changed projects, a proto nyní sám zapne instalaci Lua a
mmviewer TagLib přes stávající instalační krok. PR výběr, triplet mapping,
oddělený statický TagLib root a /warnaserror zachovány. Jde o obecnou opravu
manuálního full buildu, nikoli fork-specific aktivaci CI. Bez změny aplikace
či donorových 18 souborů S07. Předchozí run 38609591197 selhal na chybějících
Lua/TagLib ve všech třech architekturách. Lokální pytest tools/code_metrics/tests
+ tools/codeql/tests: 8 PASS; diff --check PASS. Nový čistý full build a
CodeQL přesného nového headu PR #27 jsou do ukončení CI PENDING; původní
S07 lokální runtime testy se nad identickým kódem neopakovaly. Výsledky a
konkrétní run/job ID budou uloženy v evidence/S07 mimo Git.

## S07 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S06 přijato na f43b9d86 po 12 úspěšných kontrolách PR #26 včetně skutečného
CodeQL. Nová lokální větev integration/pr769-s07-candidate obsahuje Batch
Rename jako jednu etapu: owned snapshot před workerem, přesné rodiče položek,
relative/full mode, preview, wide I/O a Undo. Všech 14 produkčních a 4 testové
soubory odpovídají blobům připnutého donoru 47661e92; vlastní adaptace je jen
registrace dvou testů do stávajícího runneru se zachováním všech starších testů.
SDK/CFileData, disk-only menu guard a Branch gate se nemění.

Host+Renamer Debug/Release x64 /warnaserror PASS. Native panel harness
67 checks/config a I/O harness 89 checks/config PASS; celé native sady
49 groups/config PASS; CI pytest výběr 49 PASS. Přijatý předchozí consumer
negativně: kompilace PASS, 31 očekávaných selhání ze 40 checks (Debug).
Pět vlastních GUI hostů, 21 skutečných přejmenování souborů a jejich Undo
ve čtyřech dávkách: celá devíti-souborová sada v D/R včetně ASCII, Unicode
jména, více Unicode rodičů, UTF8>260 i UTF16>260 cest. Samostatný Debug
duplicate test prokázal rozdílný obsah, třetí nevybranou položku a životnost
po refresh/sort panelu; relative mode zkontrolován v preview. Cancel dlouhého
jména beze změn. Všech 55 fixtures nakonec na původních cestách s původním
obsahem, oba configstorage.ini obnoveny; všech 5 hostů/reporterů normálně
ukončeno bez nového reportu. Uživatelská instalace nedotčena.

Evidenci váže parent f43b9d86 + runtime patch SHA256
c4ae16be26caf38ec5ea1f24520351ceb07a102b05de1236135998fabd06c653,
zdrojové/blokové a binární hashe; podrobně [steps/S07.md](steps/S07.md).
PHP není dostupné a dva historické Win32 LoadImageW long-bitmap subcases
jsou SKIP, nikoli provedené PASS. Nová S07 platformní CI/CodeQL ani čistý
build celé sestavy zatím neběžely: větev nebyla publikována. Accepted reference
zůstává f43b9d86. Nejbližší checkpoint je push a fork CI po zadání uživatele;
teprve po přijetí S07 navazuje S08 Checksum. S08 nebylo zahájeno.

## S06 — publikováno a přijato (2026-10-14)

Kód 9580b153434bde8942dd3b1604aa478fb5e276ea na
origin/integration/pr769-s06-candidate ověřen interním
[draftem PR #26](https://github.com/majklvi/salamander/pull/26).
Všech 12 checks přesného headu SUCCESS: Win32/x64/ARM64 /warnaserror,
native/Python/reporty, VSIX, growth a squash guard; CodeQL gate i skutečná
analýza. CodeQL job 115981207148 skončil 17:04:14 UTC; Initialize,
Release MSBuild, Perform Analysis a Upload SARIF SUCCESS. Žádný check
není pending/skipped. Lokální PHP/LoadImageW subcase SKIPs a známé nezměněné
Filecomp výstupní UI limity zůstávají pravdivě evidované níže.
Při zahájení S07 znovu ověřeno: evidence/S06-final/acceptance-current-pr.stdout.log,
ci-current-codeql-job.stdout.log a publish-summary.json.
Přijímá se integrační checkpoint S06; draft zůstává otevřený a nemerguje se
do samandarin-0.16. SOURCE/CONTRACT ani release base a84dbed6 beze změny.
Navazuje pouze S07 Batch Rename podle jejího úzkého donorového diffu.

## S06 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S06-preflightb přijat na 2d836336 po čerstvém ověření všech 12 kontrol
PR #25 včetně skutečné CodeQL analýzy a SARIF uploadu, job 115946770877.
Lokální accepted reference je 2d836336; draft se nemerguje do release base.

File Comparator nyní přebírá owned disk selection snapshots místo živých
řádků panelu. Přesné wide identity a názvy určují dvojici; nejednoznačný
target basename zůstane prázdný, chyba služby nevede k odhadu root/name.
Zachováno selected/focused chování, source/target a pořadí left/right.
UTF8 vstupy jsou heap-backed a kapacitně kontrolované před spuštěním threadu.
Produkční hunks a Python test driver odpovídají připnutému donoru;
native template navíc ověřuje 3 mezní případy aktivní snapshot větve.
SDK, CFileData, engine, dialog cleanup, ikonočtečka a Branch gate beze změny.

Host+Filecomp Debug/Release x64 /warnaserror PASS; skutečný command/service
native harness 58 checks/config PASS, celé sady 47 groups/config PASS,
pytest 49 PASS. Původní přijatý consumer negativně 23 failures/51 checks.
Ruční Compare: 11 úspěšných dvojic s přesnými vstupy/skutečnými obsahy,
Unicode jménem/adresáři a oběma dlouhými cestami v D/R, včetně správného
left/right pořadí při pravém zdroji. Všech 10 vlastních hostů i reporterů
normálně ukončeno, žádný nový report, 45 původních fixtures beze změny.
Známé nezměněné ANSI výstupní hlavičky a zkrácený Unicode caption suffix
nejsou označeny jako opravené ani jako plný Unicode UI PASS.

Podrobnosti, přesné příkazy a omezení: [steps/S06.md](steps/S06.md), evidence
mimo Git v evidence/S06-final. PHP a LoadImageW subcase SKIPs zachovány.
Větev integration/pr769-s06-candidate je lokální; nové S06 CI/CodeQL a čistá
publikační platformní matice NOT RUN. Před přijetím/pokračováním S07 nejprve
publikovat S06, Ensure-ForkCI a ověřit přesný head. Tento krok push neopravňuje.

## S06-preflightb — IMPLEMENTED / lokální brána PASS (2026-10-14)

Vlastní S06 Compare migrace narazila při shutdownu dlouhých panelů na původní
přepsání stacku v IconThreadThreadFBody. Podle CONTRACT se provádí jen tento
nejmenší bezpečnostní předkrok: heap-backed narrow/wide scratch, skutečná
kapacita konverze UTF8-first/ACP a wide offset v převedených znacích.
Legacy MAX_PATH guardy pro shell/overlay zůstávají; plná migrace S26/S28
se nepřebírá. SDK, CFileData, diagnostika a tmavé scrollbary beze změny.

Host i obnovený Bug Reporter Debug/Release x64 /warnaserror PASS; Filecomp
pro runtime znovu sestaven z přijatého parentu bez vlastní S06 migrace.
Skutečný preparační blok nativně 93 checks/config PASS, celé sady 46 groups/
config a pytest 49 PASS. PHP a LoadImageW subcase SKIPs zůstávají evidované.
Skutečné běžné panely se zdroji 390/391 UTF8 bajtů (220/221 UTF16 jednotek)
a 407/408 UTF16 jednotek: D/R listing, F3 s přesnou celou cestou/obsahem,
normální shutdown hostu i jeho reporteru, žádný nový report — čtyři PASS.
Všech 45 původních fixture souborů/sentinelů beze změny, bootstrapy obnovené.

Podrobnosti a omezení: [steps/S06-preflightb.md](steps/S06-preflightb.md).
Evidence mimo Git: evidence/S06-preflightb/{local-summary,source-manifest,
gui-observations}.json, runtime.patch, build reporty a JUnit. Připravená S06
migrace/testy jsou hashově uchovány v evidence/S06-owned/pending-S06-owned.patch;
v tomto commitu nejsou a S06 se neoznačuje za dokončené.

Větev integration/pr769-s06-preflightb-candidate je pouze lokální.
Nové fork CI, CodeQL a publikační platformní matice NOT RUN; accepted
reference zůstává bf78c81723abc983805a61acaeec6d92fd4b2316. Další krok:
publikovat tento předkrok, vytvořit/reuse interní draft přes Ensure-ForkCI,
ověřit přesný head a poté obnovit S06 patch včetně obou native registrací.

## S06-preflight — publikováno a přijato (2026-10-14)

Kód e0b69771bbaf683d7aa3debcf158d2efec65d93c na
origin/integration/pr769-s06-preflight-candidate je ověřen interním
[draftem #24](https://github.com/majklvi/salamander/pull/24). Všech 13 kontrol
přesného headu SUCCESS: Win32/x64/ARM64 /warnaserror, VSIX build/test,
native/Python/reporty, growth, guardy a skutečné CodeQL. Job 115856740584
dokončen 12:53:45 UTC; Initialize, Release MSBuild, analýza i SARIF upload
SUCCESS. Lokální PHP/LoadImageW subcase SKIPs zůstávají pravdivě evidované.
Draft zůstává otevřený a nemerguje se do reference base a84dbed6.
Evidence: evidence/S06/publish-final-ci.json a publish-final-codeql-job.json.
Předpoklad S06 splněn; pokračovat pouze vlastní owned-selection migrací.
Dokumentační acceptance nemění testovaný runtime ani SOURCE/CONTRACT piny.

## S06-preflight — lokální brána PASS (2026-10-14)

Opraven pouze lifecycle drop callbacku dvou File Comparator vstupů.
Podrobnosti, původ chyby, příkazy, omezení a další krok:
[steps/S06-preflight.md](steps/S06-preflight.md).
D/R host+plugin /warnaserror, 964 native lifecycle checks/config,
45 regresních groups/config, pytest 49 a skutečný Compare/Cancel/shutdown
PASS. Původní callback je v negativní kontrole prokazatelně vadný.
Předkrok je lokální; S06 owned-selection migrace PENDING. Před ní publikovat
předkrok a ověřit CI jeho přesného headu. Accepted reference ffa1519d (S05).

## S05 — publikováno a přijato (2026-10-14)

Kód `896989447ad410c4970d469d9f961c62e5b7a8dc` je pushnut na
`origin/integration/pr769-s05-candidate`. Interní draft
[majklvi/salamander#23](https://github.com/majklvi/salamander/pull/23) proti
nezměněné release base `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` zůstává otevřený;
neslouží k merge do samandarin-0.16. Všech 13 kontrol přesného headu SUCCESS:
čistá Win32/x64/ARM64 matice /warnaserror, VSIX build/test, native/Python,
reporty, growth a guardy, CodeQL gate i skutečná analýza. CodeQL job
115823621238 dokončen 2026-10-14 11:32:24 UTC, včetně Initialize, Release
MSBuild, analýzy a SARIF uploadu. Žádný z těchto checks není pending/skipped.
Detailní lokální výsledky a jejich PHP/LoadImageW subcase SKIPs níže zůstávají
v platnosti; zelený CI job je nepřejmenovává na PASS.

Evidence `evidence/S05/publish-final-ci.json`, `publish-final-codeql-job.json`
a `publish-summary.json`. Dokumentační acceptance nemění testovaný runtime.
Další nejmenší etapa S06: File Comparator; read-only baseline audit zatím
odhalil Debug heap hlášení už při otevření/zrušení původního Compare dialogu.
Před migrací izolovat příčinu, nevypínat diagnostiku a neskrývat ji změnou
heap ignore predikátu. SOURCE.json, CONTRACT.md a release piny beze změny.


## S05 — IMPLEMENTED / lokální brána PASS (2026-10-14)

S04 přijat na kódu 359a6b4f po opětovném ověření všech 12 current-head CI
checks včetně skutečného CodeQL. Dokumentační přijetí/accepted reference
`aef114a222f971efefb2029495784bb28b1488aa`; SOURCE a CONTRACT beze změny.
S05 vzniká na lokální `integration/pr769-s05-candidate` nad touto referencí.

Převzat celý spl_diskselection.h, CDiskSelectionService a jeho singleton,
nové samostatné SDK interfaces/constants a built-in QueryService route.
Žádná změna existujících virtual metod nebo CFileData. Owned snapshot kopíruje
cesty i metadata, zachovává pořadí a režimy výběru, vynechává '..', selhává
atomicky a po návratu nepoužívá živé řádky. Capture jen UI thread; immutable
čtení i na workeru; Release v alokujícím modulu. Fallback jen v podporovaném
QueryService-capable Samandarin ABI; starý Open Salamander není podporovaný.
DemoPlug disková DOP ukázka používá snapshot před otevřením progress dialogu;
původní archive example zůstává doslova zachovaný pod diskovým guardem.
Ostatní pluginové operace a Branch state/scanner se nepřebírají.

Donor ABI golden je byte-exact a nezměněný: 5.0-samandarin-0.15.1,
c0cddbe7442fe49abcb859624e271ce687aee0d8. GitHub release listing potvrzuje,
že je stále poslední vydaný tag; golden ověřen přímo proti jeho pěti interfaces
a celému spl_com.h. S04 navíc zachovává celé bodies vydané target base a84dbed6.
Testy SDK/storage/host/singleton a skutečného DemoPlug 55 checks/config PASS
/W4 /WX: 43 zachovaných donor assertions + 12 doplněných DemoPlug kontrol. Pouze PictView
CEnumFiles extrakce a přesně dvě consumer assertions jsou DEFERRED do S12;
žádná jiná donorová assertion nevypuštěna. S04 celý QueryService harness nyní
196/config (původních 193 + 3 nové discovery/version assertions); factory nové
služby je v tomto resolver testu discovery facade, skutečná service/singleton
jsou kompilované v owned harnessu a volané skutečným DemoPlug v GUI.

Host + všech 30 podporovaných SDK konzumentů Debug/Release x64 /warnaserror
PASS (60 consumer builds). Native runner 44 groups/config PASS, pytest 49 PASS.
Historical portables projekt a canonical BUILD_SFTP_PLUGIN=false excluded;
nejsou označené PASS. PHP chybí a LoadImageW long-bitmap backend Win32 2 je
explicitní subcase SKIPPED. Po finálním DemoPlug ABI guardu native runner znovu 44/config PASS
(58,02 / 61,45 s). Hosty po icon opravě přestavěny,
core handle test znovu PASS 132 Debug / 109 Release se stejným bitmap skipem.

GUI objevil baseline CPlugins::CreateIconsList chybu plugins2.cpp:1156:
raw CopyImage, pak HANDLES(DestroyIcon). Funkce v parentu i a84dbed6 shodná.
Nejmenší oprava HANDLES_Q(CopyImage) zachovává tracker i všechny větve.
První vlastní Debug proces 3768 ukončen v diagnostice, není PASS. Finální
Debug fresh 8776/restart 13300, Release fresh 2880/restart 7224: viditelné
panely, fresh F3 obsah/F5 kopie/hash, oba fresh DemoPlug 23 selected DOP files
včetně Unicode a dlouhého názvu, úspěšné deselect a všechny Alt+F4 shutdowny
bez heap/handle dialogu PASS. Kopie 33 bytes, 25 nových a 7 původních fixtures
beze změny. Vlastní bootstrapy obnovené, žádný vlastní GUI proces nezůstal.
Po finálním ABI guardu přestavěn jen DemoPlug D/R a zopakován skutečný
23-file GUI command/shutdown v procesech Debug 10760 / Release 256;
host a ostatní DLL se nezměnily. GUI fixture maximum 250 UTF16 units,
424 UTF8 bytes; skutečná delší UTF16 cesta je v compiled fixtures.
GUI Cancel nebyl doložen: Escape přišel po dokončení a výběr již byl zrušen;
nejde o Cancel PASS. Skutečné SMB I/O ani migrace long-path I/O konzumentů
nejsou S05; DemoPlug stále jen simuluje práci, snapshot není filesystem transakce.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S05`: přesné bounded
příkazy, source/donor/ABI manifests, consumer matrix, JUnit, GUI ledger,
runtime payload manifests a originální fixtures. Každý příkaz má parent SHA
a patch hash svého okamžiku; before-menu-fix.patch zachován. Menu fix přidává jen plugins2.cpp; pozdější ABI guard mění pouze DemoPlug
menu a jeho dva testové soubory, zbytek SDK/consumer zdrojových hashů shodný. První host post-build
FAIL kvůli chybějícímu PSModulePath/Get-FileHash vyřešen jen prostředím;
žádná produktová/workflow změna kvůli tomu. Evidence collector opraven pro
extended-length cesty; jeho přípravná selhání nejsou výsledky produktu.

S05 commit je lokální checkpoint; push/PR, nové CodeQL a publikační čistá
Win32/x64/ARM64 matice NOT RUN. Accepted reference zůstává na přijatém S04.
DemoPlug stále deklaruje kompatibilitu se SDK 103: nový diskový case proto
navíc vyžaduje SalamanderVersion >= 105 (QueryService zavedeno v spl_vers.h).
SDK 103/104 použije přesný původní postup bez jediného QueryService volání;
105/106 používá owned službu. Skutečný DOP case se kompiluje v testu proti
progress facade; staré verze, archive, destroy-on-open, Cancel a capture
failure dávají 12 nových assertions. Header zachovává svoji precondition:
volající musí před vstupem zaručit QueryService-capable ABI. Nemění se
verze SDK ani již existující nesouvisející PoC příkazy DemoPlug.

S06 nezačala. Další krok je samostatně autorizovaná publikace této stejné větve,
interní fork-CI draft přes Ensure-ForkCI.ps1 a ověření přesného head výsledku.


## Přijetí S04 — DONE (2026-10-14)

Na pokyn uživatele zahájit S05 přijat S04 checkpoint
`359a6b4f768c4a8769ba6dcb923f9ea663b91aad`. Přesný head interního
[PR #22](https://github.com/majklvi/salamander/pull/22) znovu ověřen:
12 COMPLETED/SUCCESS včetně Win32/x64/ARM64, native/Python, source growth,
guardů a skutečné MSBuild CodeQL analýzy/uploadu. Lokální host + 30 SDK
konzumentů Debug/Release, native 43/config, resolver 193/config, pytest 49
a GUI fresh/restart/F3/F5/shutdown PASS; historické PHP/LoadImageW skips
zachované. Chyba závislosti ABI testu na Git historii vyřešena v 359a6b4f.

Referenční refactor/pr-769-incremental posunut fast-forward přes tento
kód a dokumentační přijetí. Draft se nemerguje do samandarin-0.16;
SOURCE/CONTRACT/release/donor piny beze změny. Další etapa S05, pouze lokální
implementace a validace; nové zveřejnění není tímto zadáním autorizované.
Historické pending záznamy níže popisují dřívější stav.


## S04 CI předkrok — ABI test bez historického checkoutu (2026-10-14)

První current-head CI bfa74f8e: Win32/x64/ARM64 buildy, source growth a Python
PASS, native FAIL pouze `panel_item_paths_tests`. Přesný JUnit traceback:
Git show a84dbed6:src/plugins/shared/spl_gen.h chybí v omezeném CI checkoutu.
Je to chyba nové testové infrastruktury, nikoli potvrzená produktová regrese.
Workflow není měněné a žádná assertion nebyla vypuštěna.

`panel_item_paths_abi.json` nyní drží SHA256 všech pěti vydaných Abstract class
bodies ze spl_gen.h a celého spl_com.h, se source_commit a84dbed6. UTF8/BOM/EOL
normalizace deklarovaná. Normal test porovnává stejné celé definice/layout proti
zmrazené referenci bez Git historie. Explicitní `--verify-abi-source` zvlášť
ověřuje fixture against připnutý release; lokálně PASS včetně seznamu tříd.
Stejné production C++ assertions dál 193/config PASS. Samostatný adresář s
přesnými kopiemi zdrojů a bez .git prošel Debug i Release 193/config; cleanup
vlastního rootu ověřen. Původní parent-query negative control zůstává dostupný.

Produktové/SDK/runner/C++ template bytes z dd344546 se nemění. Buildy všech
konzumentů a GUI se bez runtime změny neopakují; nativní runner nového testu
opakovaný Debug/Release. Nová CI musí ověřit nové SHA, ne Bfa ani dd344546.
S04 nadále PENDING CI/acceptance, accepted reference ea1dd462, S05 nezačala.
Evidence S04: failed-head-test-results/native.xml, publish-ci-03,
ci-fix-focused, ci-fix-historyless/proof, ci-fix-native-*, ci-fix.patch.
Původní testovaný patch zachován jako local-stage-runtime.patch.

## S04 — IMPLEMENTED / lokální brána PASS (2026-10-14)

Kandidát na `integration/pr769-s04-candidate` nad přijatým `ea1dd462`.
Čtyři ordinary disk-row gettery z donorového `branch_view.cpp` přeneseny do
již kompilovaného `fileswn1.cpp`, deklarace a const GetPath z fileswnd.h.
Žádný Branch state/scanner. Donor Join nahrazen existujícím SalPathAppendW;
wide root/name zůstávají přesné, byte fallback volí validní UTF-8 před ACP.
Původní donorový ACP default reprodukoval čtyři neúspěšné assertions u UTF8
root/name bez wide dat; finální adaptace projde při zachování ACP testu.

CPanelItemPathsService/FindCurrentItem/GetItemFullPath a factory převzaty ze
zip.cpp, built-in route přes stávající QueryService. Nová optional SDK třída
CSalamanderPanelItemPathsAbstract, žádná změna vydaných vtables nebo CFileData.
Current array membership se ověří před dereference; explicitní UI-thread,
unsupported archive/FS, capacity/OOM/invalid errors. Výstup nikdy částečný.
Borrowed pointer nesmí přežít refresh/message pumping; kontrola adresy není
záruka proti ABA reuse. Synthetic '..' vrací navigation path root\.. stejně
jako ordinary donor gettery, nejde o rozšíření selection/file-operation API.

Host a všech 30 aktuálně podporovaných SDK konzumentů MSVC v145 Debug/Release
x64 /warnaserror PASS (60 consumer builds). Historical portables variant a
canonical BUILD_SFTP_PLUGIN=false zůstávají excluded, nejsou označeny PASS.
Native runner 43 groups/config PASS; nový whole-production resolver/service/
QueryService harness 193 checks/config /W4 /WX. Existujících 5 SDK Abstract
class bodies spl_gen.h a celý spl_com.h shodné s release a84dbed6.
Parent QueryService control s aktuálními resolvery expected FAIL 5 assertions;
není to build celého parent hostu. Pytest 49 PASS. PHP missing a long-bitmap
LoadImageW backend Win32 2 subcases zůstávají explicitně SKIPPED.

Actual GUI D/R fresh/restart panels, fresh F3 ASCII obsah, fresh F5 kopie a
všechny shutdowny PASS. Kopie SHA256/30 bytes shodné, vlastní nové empty configy,
sedm původních fixtures beze změny a bootstrapy obnovené. V SDK ještě není
migrovaný GUI consumer této služby; konkrétní service dispatch je testovaný
nativním whole-production harness s panel/UI/registry facades. GUI smoke je
běžná panelová regresní kontrola, není vydáván za GUI invocation nové služby.
Real SMB I/O, external/viewer long-path operation migration a Branch metadata
nejsou součástí S04. S05/consumer migration ani S26 scanner nejsou importovány.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S04`: bounded commands,
source/runtime/fixture manifests, 60-row consumer matrix, JUnit a GUI ledger.
Původní UTF8 BOM zip.cpp zachován; API source/extracted semantics se nemění.
Kód dd344546 na zadání uživatele publikován do forku na stejné
`integration/pr769-s04-candidate`, interní [draft PR #22](https://github.com/majklvi/salamander/pull/22)
proti samandarin-0.16/a84dbed6. Stav **PENDING current-head CI/acceptance**.
Publikační záznam je dokumentační commit, kód/test bytes beze změny. Jeho nové
CI se váže k aktuálnímu head SHA, nikoli pouze k lokálně ověřenému dd344546.
Poslední accepted reference zůstává ea1dd462; SOURCE/CONTRACT/release/donor
piny nezměněné. Další krok: ověření CI a přijetí S04 před S05.

### Přesné S04 příkazy a limity

- MSBuild salamand.vcxproj /m:1 /t:Build /p:Configuration=Debug|Release
  /p:Platform=x64 /p:OPENSAL_BUILD_DIR=<own pr769-baseline-v145>
  /p:PreferredToolArchitecture=x64 /warnaserror /nr:false: PASS, Debug 18,64 s
  po SDK změně + 5,33 s po encoding adaptaci; Release 41,81 s.
- Build-Consumer.ps1 -Name <supported consumer> -Configuration Debug|Release
  -Suffix -s04: PASS 60/60, přesné projects/environment/deadlines v JSON.
  Jde o přírůstkové rebuildy změněné hlavičky, není tvrzen nový čistý build.
- python -B src/tests/panel_item_paths_tests.py --configuration Both:
  193/config PASS. Actual SDK, conversion, SalPathAppendW, celé gettery,
  service/factory a QueryService; panel array/window/registry jsou adaptéry.
  Invalid/foreign/unaligned/one-past/old non-reused storage, actual worker
  thread, exact capacity/sentinel, real wstring allocation failure a registry
  fallback. Unicode parent/name, UTF8-byte-long/UTF16-long, ACP, UNC/extended
  a synthetic parent path. UNC je stringová hranice, nikoli síťové I/O.
- python -B src/tests/panel_item_paths_tests.py --configuration Debug
  --query-source-ref ea1dd462...: expected FAIL 5 discovery assertions.
  Před encoding adaptací tentýž harness expected FAIL 4 UTF8 assertions;
  předchozí source/patch zachovány v before-encoding-adaptation.*.
- tools/run_native_tests.ps1 -Configuration Debug|Release -Platform x64
  -ResultsPath <own S04 XML> -CommandTimeoutSeconds 60: 43/config PASS,
  40,40 / 42,46 s. JUnit group skipped=0 nepřeklasifikuje PHP/LoadImageW skips.
- python -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py
  tools/code_metrics/tests tools/codeql/tests tools/localization/tests
  --junitxml=<own S04 python.xml> -p no:cacheprovider: 49 PASS (3,28 s pytest).
- Computer Use vlastní runtime a configy: Debug Fresh PID 7116 / Restart 4208,
  Release Fresh 14124 / Restart 9792. Fresh panely/F3/F5, restart panely,
  všechny normal Alt+F4/shutdown bez warningu. Bez řízení osobního Salamanderu.
  Build helpers/compiler subprocesses ukončené; žádný report odeslán.

Publikační Win32/x64/ARM64 buildy a testy jsou v nové CI PR #22 rozběhnuté;
CodeQL čeká na úspěšnou build matici. CI dokončené pro S03 není důkaz S04.
API/SDK bytes po testech chrání source manifest. Nové assertions jsou vlastní
rozšíření ordinary donor fixture; Branch assertions jsou nadále DEFERRED.

## Přijetí S03 — DONE (2026-10-14)

Uživatel po ověření CI schválil pokračování a přijetí kódového checkpointu
`770f48e096260a596c4827341cf57490a0e2a37e`. Před přijetím znovu ověřeny přesné
head/base interního [PR #21](https://github.com/majklvi/salamander/pull/21):
12 COMPLETED/SUCCESS checks. Build Win32/x64/ARM64, native/Python, source growth
a squash guard prošly. CodeQL provedlo Release x64 build, vlastní analýzu,
filtr změněných řádků a upload SARIF; nebylo přeskočeno.

Lokální host/ZIP Debug/Release, SafeFile 268/config, ZIP lifetime/dispatch
151 Debug / 138 Release, native 42 groups/config, pytest 49 a skutečné cold
Unicode ZIP extraction/hash/restart/shutdown ověřují totožný kód. Historické
PHP/LoadImageW skips, negativní controls a diagnostické timeouty zůstávají
popsané níže; nejsou překlasifikované na PASS.

S03 a její baseline preflight DONE. Referenční `refactor/pr-769-incremental`
se posouvá fast-forward přes přijatý kód a tento dokumentační záznam. S02b
integration branch, SOURCE/CONTRACT/donor piny a release base se nemění.
Acceptance commit je pouze dokumentace; jeho automatické CI po pushi je
nový běh, oddělený od zelených výsledků přijatého kódu 770f48e0. Build/GUI se
bez změny kódu neopakují. Fork draft #21 zůstává otevřený pro validaci.
Další etapa [S04](steps/S04.md) je plánovaná, v tomto přijetí se neimplementuje.
Evidence: `C:/Samandarin/codex_pr769_prompts/evidence/S03-preflight/acceptance-*`
a `ci-current-codeql-job.stdout.log`, `ci-current-build-run.stdout.log`.

## Historický S03-preflight — lokální runtime brána PASS (2026-10-14)

Opraven potvrzený baseline únik `DupZipUtf8NameW`: hostův legacy SDK copy
nepřebírá `NameW`, takže wide kopii nyní vlastní interní `CZIPFileData` a jeho
stávající release callback. Reject/reset i metadata OOM mají jediný cleanup;
nízkopaměťová větev zachována přes úzké `std::nothrow`/debug-new obnovení.
Při cold archive smoke navíc prokázán AV při předání ZIP listing interface
nově nakonfigurovanému 7-Zip unpackeru. `PackUncompress` používá uloženého
vlastníka stejného panel/archive/interface; ostatní dispatch se nemění.
SDK layout/vtable, 7-Zip vendor kód a původní heap MENU predicate beze změny.

Host i ZIP plugin Debug/Release x64 v145 /warnaserror PASS. Finální native
runner 42 groups/config PASS, SafeFile 268 checks/config, nový skutečný ZIP
listing/dispatch harness 151 Debug / 138 Release checks; pytest 49 PASS.
Parent `919e804d` s totožným harness má expected FAIL (25 failed assertions,
včetně navazujících counter assertions), takže test odhaluje původní chyby.
PHP a long-bitmap LoadImageW subcases zůstávají explicitně SKIPPED.

Skutečné GUI cold ZIP rozbalení v Debug i Release ověřilo čtyři soubory
včetně Unicode názvu, Unicode adresáře a stejných leaf names v různých
adresářích; obsah/velikosti/SHA256 shodné. Restart panely a normální ukončení
PASS oběma konfiguracemi. Finální Debug po odstranění diagnostiky znovu
přestavěn a zavřen bez heap/handle dialogu. Doplňující restart capture ukazuje
pouze známých 16 normal / 512 bytes MENU, predicate known=1, exit 0.
Historický další restart warning nebyl v těchto finálních bězích zopakován;
je zachován jako pozorování, nikoli připsán nové neprokázané příčině.

Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S03-preflight`: přesné
bounded commands, source/runtime/fixture hash manifesty, negativní controls,
PDB crash stack a GUI ledger. Own crash report zachován, žádný report odeslán.
Dva diagnostické 90s deadline běhy nejsou PASS; startup interference po
vlastním crash reporteru je odlišená od produktového AV. Bootstrapy obnoveny,
vlastní procesy ukončeny, sedm původních fixtures a vlastní ZIP beze změny.

S03 kandidát zůstává na `integration/pr769-s03-candidate`, interní draft
[PR #21](https://github.com/majklvi/salamander/pull/21). Předchozí `919e804d`
měl všech 12 checks SUCCESS včetně skutečné CodeQL analýzy; tyto výsledky
nedokazují nové opravné SHA. Po pushi opravy je nutná nová current-head CI.
S03 **není přijatá**, accepted reference zůstává `17678e7c`. SOURCE/CONTRACT
piny se nemění. Žádný upstream PR, merge release base ani S04.

## Historický S03 — implementace a otevřená runtime brána (2026-10-13)

Parent `17678e7cac710f176683403b8cae382aee087dbf`, kód ověřen jako pracovní diff
na `refactor/pr-769-incremental`. Následně uživatel autorizoval kandidátní
commit/push na samostatné `integration/pr769-s03-candidate` a interní fork-CI
draft. Tím se nepřijímá S03 ani neodstraňuje runtime blokace. Identické code/test
bytes před commitem porovnány s validačním manifestem; dokumentace doplňuje
publikační stav. Code patch SHA256
`F6269765AFE375887DC14272EAD5523701BE62A152362C0805E3DE82BBF4BCBA`.
Evidence `C:/Samandarin/codex_pr769_prompts/evidence/S03`: bounded command JSON,
stdout/stderr, JUnit XML, source/runtime/fixture hash manifesty a GUI ledger.
Vše escalation, whole-process deadline 95/100 s, no node reuse/Python cache.

- `MSBuild src/vcxproj/salamand.vcxproj /m:1 /t:Build /p:Configuration=Debug|Release
  /p:Platform=x64 /p:OPENSAL_BUILD_DIR=<own pr769-baseline-v145> /warnaserror
  /nr:false`: PASS, 9,42 / 32,08 s. Po parent control finální Debug přestavěn
  (`host-debug-restored`, 5,88 s). SDK, shared headers ani pluginové zdroje se
  nemění; staging používá podporované konzumenty již ověřené v S02b a matching
  dependencies/resources. Nová 60-plugin matice zde není nutná ani tvrzená.
- Own venv `python -B src/tests/safe_file_paths_tests.py --configuration Both`:
  PASS, 268 checks Debug a 268 Release, /W4 /WX. Production SafeFile/SDK/wide
  helpery se extrahují celé, UI dialogy a jednorázové device/path allocation
  failure jsou testové adaptéry; Win32 file I/O a identity/content/cleanup jsou
  skutečné. Ověřen skutečný DOS alias i legacy ACP fallback, žádný jejich skip.
  Harness nenahrazuje Debug tracker ani produktový GUI test.
- Tentýž harness `--configuration Debug --source-ref 47661e92...`: expected
  runtime FAIL pouze relative long create (Win32 3, 254 checks/1 failure).
  Toto je negativní donor control konkrétní adaptační opravy, nikoli PASS.
- `tools/run_native_tests.ps1 -Configuration Debug|Release -Platform x64
  -ResultsPath <own XML> -CommandTimeoutSeconds 60`: 41 groups/config PASS
  (28,43 / 25,83 s). PHP missing a LoadImageW long-bitmap backend Win32 2
  subcases explicitně SKIPPED; group JUnit skipped=0 je nepřeklasifikuje.
- Own venv `python -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py
  tools/code_metrics/tests tools/codeql/tests tools/localization/tests
  --junitxml=<own python.xml> -p no:cacheprovider`: 49 PASS (3,15 s pytest).
- Computer Use, vlastní matching runtime a configy: první/restart panely D/R,
  F3 ASCII obsah a F5 kopie D/R PASS; obě kopie SHA256 odpovídají původnímu
  fixture. Debug rozbalení vlastního ZIPu přes `src/plugins/zip/extract.cpp`
  SafeFileCreate PASS, přesných 36 bytes výsledku ověřeno zvlášť. Nejprve byl
  cíl odmítnut jako nepodporovaná maska `*.*`; po zadání adresáře extrakce PASS.
  To není path resolver chyba. Release ZIP extraction nebyla provedena.

Debug Alt+F4 po extrakci vyvolal `Heap Message: Detected memory leaks!`.
Parent kontrola: jediný host runtime zdroj dočasně nahrazen přesnými bytes
`git show 17678e7c:src/safefile.cpp`, host Debug přestavěn a nové empty config
otevřelo stejné ZIP přímo v panelu. Bez extrakce reprodukován tentýž heap
dialog. JSON parent build/launch nese obecný digest S03 patch souboru;
control ledger a parent source hash výslovně odlišují skutečný parent runtime.
Finální source bytes obnoveny z vlastního snapshotu a ověřeny shodným SHA256,
host znovu přestavěn. Debug restart s configem z archive smoke hlásí heap znovu.
Release fresh/restart bez hlášení. Původ CRT bloků dosud neurčen; S03 celková
runtime brána **PENDING/BLOCKED**, navazuje [S03-preflight](steps/S03-preflight.md).
Autorizovaný kandidátní commit/push slouží pro fork CI; není accepted checkpoint.
Žádný upstream PR ani S04. Žádná heap/tracker kontrola nevypnuta. Žádný crash report odeslán. Own bootstrap obnoven; osobní instalace
a původní fixtures beze změny.

UNC root checks jsou pouze stringové hranice, nikoli skutečné SMB I/O.
Win32/ARM64, čistá publikační sestava a nové CodeQL nebyly spuštěny; předchozí
zelená CI S02b nedokazuje S03. SOURCE/CONTRACT/donor piny, SDK ABI, workflows
a budoucí etapy beze změny. Native/disk PASS není přijetí celé etapy.

## Přijetí S02b (2026-10-13)

Uživatel schválil přijetí kódu `23d5406d49eeee40cda9ef19288ff66b6618fd57`
po ověření dokončení CI. Fresh `gh pr view 20 --repo majklvi/salamander
--json headRefOid,baseRefOid,headRefName,baseRefName,isDraft,statusCheckRollup,url`
potvrdil přesné head SHA, base `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`
a všech 12 COMPLETED/SUCCESS checks. Evidence: externí
`S02b-preflightb/acceptance-current-head-ci.{json,stdout.log}`.

Build Win32/x64/ARM64 Debug (warnings as errors), native/Python, source growth,
squash guard a CodeQL včetně skutečného Analyze C++ jobu SUCCESS. CodeQL zde
nebylo skipped. Lokální runtime a native důkazy pro totožný kód níže; PHP a
LoadImageW backend subcase zůstávají explicitně SKIPPED. Optional širší plugin
operation smoke není povinná wrapper brána a zůstává neprovedený.

S02b a baseline preflighta/b DONE; spolu se S02a je S02 DONE. Referenční
`refactor/pr-769-incremental` se posouvá fast-forward, bez merge release base,
změny donor/source pinů nebo zahájení S03. Acceptance commit obsahuje pouze
dokumentaci, takže build/GUI nebyly zbytečně opakovány. Jeho automatické fork
CI po pushi je nový běh; výsledky 23d5406d jsou vázané na kódový checkpoint.
Dokumentační přijetí nemění historické commit trailery PENDING ani staré logy.

## S02b-preflightb — Samandarin native exit lifetime (2026-10-13)

Kandidát nad parent `20d6018c4815c90a3b359e2bb45ded452cfbd5f7` na
`integration/pr769-s02b-candidate`. Finální code patch SHA256
`CCC496FAEEEADE60A54FF1EBA5ED8D66EEEE33A7BCBE35F15F8CB933AE06A4E2`.
Externí evidence `C:/Samandarin/codex_pr769_prompts/evidence/S02b-preflightb`;
logs/binárky necommitované. SOURCE piny a accepted ef19d788 beze změny.

Skutečné příkazy jsou v jednotlivých bounded command JSON. Vše escalation,
MSVC v145, /warnaserror, /nr:false, own build root pr769-baseline-v145,
95/100 s procesní deadline. Použité příkazy:

- MSBuild src/vcxproj/salamand.vcxproj /m:1 /t:Build /p:Configuration=Debug
  (a Release) /p:Platform=x64 /p:OPENSAL_BUILD_DIR=<own root>;
  poslední `host-*-preflight-final` PASS (25,71 / 86,33 s).
- `Build-Consumer.ps1 -Name <supported consumer> -Configuration Debug|Release`:
  30 skutečných projects, 60 final PASS v consumer-final-matrix.json. Poslední
  Samandarin native export změna ověřena `samandarin-*-preflight-final`;
  shared tracker po celé 60 matici nezměněn. SFTP/legacy Portables EXCLUDED.
- `tools/run_native_tests.ps1 -Configuration Debug|Release -Platform x64
  -ResultsPath <own evidence XML> -CommandTimeoutSeconds 60`: 41 groups/config,
  real SDK tracker 426 Debug/89 Release, shutdown helpers 26/config. PHP není
  dostupné; nativní long-bitmap LoadImageW backend odmítl fixture (Win32 2).
  Tyto dvě subcases jsou SKIPPED, ne PASS navzdory group JUnit skipped=0.
- Own venv `python -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py
  tools/code_metrics/tests tools/codeql/tests tools/localization/tests
  --junitxml=<own evidence>/python.xml -p no:cacheprovider`: 49 PASS (6,19 s),
  bez Python cache. Pozdější host ordering fix tyto Python oblasti nemění.
- Actual parent tracker raw snapshot + then-current 422-check test: expected
  exit 1, unused-registry CRT allocation assertion failed. Parent negative
  evidence předchází přidání čtyř early-finalization checks; není 426 PASS.
- Actual private helper exe ve třech own Unicode/byte-long/UTF16-long paths,
  Debug/Release: 6 PASS. .NET launcher na UTF16-long cestě error 206; native
  CreateProcessW explicit application launcher PASS. Helper zdroj beze změny.

Full staged runtime obsahuje notifier, CLR, ostatní 30 supported plugins a
resources. Fresh configy nulové před startem, nové copy cíle. GUI první panely,
F3 ASCII obsah, F5 shodný SHA256 817596C4EC1AFF063B300263168678FF15FAAB50B37974F68B73422FE30EC606,
restart/shutdown ověřované v obou konfiguracích; 7 původních fixture hashů
včetně sentinelu kontrolovaných zvlášť. Pouze own runtime bootstrap obnoven.

Native debugger zachytil first/second chance C0000005 na dbg.cpp:285:
C__Handles destructor -> C__Trace::SendMessageToServer -> LdrShutdownProcess.
Host exit 0 s Bug Reporterem byl nesprávná prvotní indikace čistého ukončení;
není započten jako PASS. Časná idempotentní diagnostika opravila stale pointer,
`final-capture.stdout.log` exit 0 bez AV. Žádný report odeslán.

Zátěžový pomocník přes PDB identifikuje skutečný InitializeRuntimeThread,
pozdrží pouze vlastní worker a obnoví ho vlastním named eventem. GUI Alt+F4
probíhá přes Computer Use. Pozdní první hook umožnil jiným plugins zastavit
CLR před odmítnutím close; obnovení init pak -1. Proto readiness nově předchází
prvnímu Unload. Finální Debug `busy-debug-preflight`, Release `busy-release-stack`:
close timeout zachoval loaded plugins; resume -> initialization result 1;
retry exit 00000000, bez AV. Release stack dokládá main v GetMessage, nejde
o deadlock; některé starší debugger runs vyčerpaly vlastní 90 s deadline a
jsou NOT PASS. Jejich cíle byly ukončeny jen vlastním diagnostickým pomocníkem.

Historie přípravných selhání je zachovaná: PATH/MSBuild setup, původně prázdný
parent snapshot, managed shared-output collision, zamčené language modules
otevřeným vlastním GUI, wrong runner argument index a CreateEventW test compile
error (SDK má generic CreateEvent). Finální výsledky je nahrazují konkrétním
novým command záznamem; timeout se nikdy nepřejmenovává na PASS.

Nové current-head fork CI/CodeQL a povinná platformní matice PENDING. Parent
20d6018c měl 12 viditelných checks SUCCESS včetně CodeQL, nikoli nové SHA.
S02b formálně nepřijatá; S03 nezačala. Další krok: current-head CI a acceptance.

## S02b-preflighta — icon fix ověřena, úplná Debug brána BLOCKED

2026-10-13. Testovaný base `207a8a572e3908feadc3093055dbd34958f07772` + přesný
2-file icon patch v externí evidence/S02b-preflight/runtime.patch. Finální
kandidátní SHA/evidence vazba je v checkpoint.json. S02b SDK/test inputs jsou
bajtově shodné s předchozí validation-inputs.json; nic z portu není přepracováno.

| Kontrola | Výsledek |
| --- | --- |
| Service Explorer Debug/Release x64 v145 /warnaserror | PASS, 3.54 / 4.92 s |
| Host Debug/Release x64 /warnaserror | PASS, finální běhy 8.17 / 74.66 s |
| Debug první start, panely/F3/F5/hash | PASS, žádný icon tracker modal |
| Debug úplný shutdown | FAIL heap dialog; není clean gate |
| Release panely/F3/F5/hash/shutdown | PASS; finální relink dále ověřen restartem |
| Diagnostický host TRACE_TO_FILE | Build PASS 45.72 s; 4126 B / 21 normal blocks |
| Izolující control bez notifier SPL | Bez heap dialogu, Closing log file; NENÍ full runtime PASS |
| Native/Python SDK regression suite | Neopakována: SDK/test inputs beze změny; předchozí S02b 40 skupin/config a 49 pytest i CI 207a8a57 PASS |
| Fork CI / CodeQL 207a8a57 | 12 viditelných check runs SUCCESS; nový icon kandidát musí dostat vlastní current-head CI |
| Všech 7 původních GUI fixtures | Hash unchanged; Debug/Release ASCII kopie MATCH |

Všechny čtyři product buildy používají normální repo projekty a původní flags,
OPENSAL_BUILD_DIR work/build/pr769-baseline-v145. Dva přípravné host pokusy
selhaly: Debug staging kvůli PSModulePath (Get-FileHash v child Windows PS
nenalezen; napraveno scoped Windows PS module path), Release language DLL
byla zamčená spuštěným vlastním GUI (po jeho normálním ukončení build PASS).
Tyto FAIL nejsou zaměněny s finálními PASS.

Vlastní Trace Server nebyl pro CU targetable (custom toolbar caption/hidden
window); sky launch nenašel targetable window. Vlastní instance ukončeny.
Diagnostická alternativa: samostatný build root S02b-preflight-trace a
compiler env _CL_=/DTRACE_TO_FILE, stejné zdroje a matching plugin/resources.
TEMP/TMP omezené na vlastní trace-temp. Normální build flags ani zdroje se
nemění; diagnostický build není vydávanou binárkou. Žádné reporty odeslány.

Před heap sentinel došlo ke skipu Samandarin shutdown v plugins1.cpp:3245;
SPL a CLR stále načtené. Dump obsahuje SDK tracker allocation z mhandles.cpp:601,
3200 B; path/version-sized blocks 334/48 B a dva 16 B debug proxy bloky jsou
konzistentní s globálními std::wstring v managed_bridge.cpp. Dalších 16 bloků
(512 B) má MENU marker a odpovídá již existující baseline výjimce. V kontrolním
běhu stejný vlastní runtime a čerstvá konfigurace bez samandarin.spl došly až
k Closing log file, bez heap dump/dialogu. SPL následně obnoven. Zbytek sestavy
ani heap checker nebyly oslabeny. Control není náhradou úplné acceptance.

Host skip shutdown a celý Samandarin plugin beze změny proti TARGET_SHA;
SDK malloc/free pole trackeru beze změny proti S02a. Parent runtime není znovu
spuštěn. Oddělená baseline blokace identifikována; nejmenší další předkrok je
[S02b-preflightb](steps/S02b-preflightb.md). Piny a accepted checkpoint zachovány.
## Historický kandidát S02b 207a8a57 — PENDING/BLOCKED, 2026-10-13

**Build/native/Python PASS; čistá Debug runtime brána FAIL. Kandidátní commit není accepted checkpoint.**
Testovaný parent `ef19d788c1a2807e134a2ea9ba3aa9f1e1ac253d`; runtime patch `962125d93e6f7f4f27c715da91c962d513cf8101d8ea1069261e496874b506e4`;
code/test patch `772b2b635af915e68202cf6289defaf01faf24337d082f68023c4afe8a91f85d`. Raw source hashe, exact args/env/time/exit v
`evidence/S02b/validation-inputs.json` a per-run JSON; matrix, 67 finálních
úspěšných příkazů, GUI/files/runtime manifesty v consumer-matrix.json a
validation-summary.json. Prepared checkpoint neznamená accepted SHA.
Piny SOURCE/CONTRACT a čistý donor nezměněny; S03 nezačala. Kandidátní commit
na `integration/pr769-s02b-candidate` zveřejněn na výslovné zadání pro fork CI;
upstream PR nevytvořen. Kód/testy mají stejné raw source hashe jako tato validace.
Kandidátní SHA, fork draft a počáteční stav CI eviduje externí publication.json;
CI není PASS, dokud není doložen výsledek pro jeho přesné SHA.

| Kontrola | Skutečný výsledek |
| --- | --- |
| Host Debug/Release x64 v145 | PASS /warnaserror, 18,79 / 42,56 s |
| 30 podporovaných plugin consumers | PASS 60 Debug/Release x64 buildů s /warnaserror, matching dependencies/resources |
| Portables language peer Debug/Release | PASS explicitní lang_portables.vcxproj, 1,75 / 1,69 s |
| Native Debug/Release x64 | PASS 40 skupin/config, 0 failures, 54,89 / 60,11 s |
| Plugin live tracker native | PASS 214 Debug / 89 Release checks, file identity/content/name, faults, long/Unicode |
| S01/S02a regressions | PASS S01 99/config, core 132 Debug/109 Release; 1 raw LoadImageW-long backend subcase/config SKIPPED |
| CI pytest výběr | PASS 49, 5,80 s |
| Negative control parent plugin sources | EXPECTED FAIL compiler: explicit A/W members absent, 2,83 s |
| Debug GUI PID 7260 | Panely/F3/F5 matching copy PASS; startup 2 Service Explorer tracker messages; shutdown heap leak + Bug Reporter — FAIL clean runtime gate |
| Release GUI PID 7216 | PASS panels/F3/F5 matching copy, normal Alt+F4, no diagnostic |
| Fixture safety | PASS 2 copies exact SHA256, 7 originals unchanged, own bootstraps restored |
| SFTP + historical Portables 2015 project | EXCLUDED per current CI/non-shipping and main solution; not PASS |
| PHP process-runtime | SKIPPED php unavailable |
| S02b CodeQL, full clean publication platform matrix | NOT RUN |

Core wrapper source includes unchanged whole production SDK in native test;
registry/CheckCreate/CheckClose/dbg/widepath and Win32 files are actual. Only
malloc/free and WideCharToMultiByte diagnostic faults are test-injected;
GetLastError is deliberately disturbed to verify restoration. Release matches
real disabled-tracker behavior, so fewer checks. Legacy dbg.h unused parameter
warning suppression scoped to that header in test precomp; new tests W4/WX,
legacy production sources W3/WX. No product warning or tracking suppression.

Commands actually run in work/:

- MSBuild each of 30 project paths from consumer-inventory.json, Debug and
  Release x64, `/m:1 /t:Build /p:Configuration=<config> /p:Platform=x64`
  `/p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\pr769-baseline-v145\`
  `/p:PreferredToolArchitecture=x64 /warnaserror /nr:false /v:minimal`.
- Same flags for src/vcxproj/salamand.vcxproj and explicitly
  src/plugins/portables/vcxproj/lang_portables.vcxproj in both configurations.
- `pwsh -NoProfile -File tools/run_native_tests.ps1 -Configuration Debug` and
  Release, `-Platform x64 -ResultsPath ../evidence/S02b/native-<config>.xml`
  `-CommandTimeoutSeconds 60`.
- `python -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py`
  `tools/code_metrics/tests tools/codeql/tests tools/localization/tests`
  `--junitxml=../evidence/S02b/python.xml -p no:cacheprovider`.
- Negative native Debug build overrides PluginHandlesDir to exported parent
  mhandles pair in evidence/S02b/negative-plugin, output work/build/S02b-negative.

MSBuild VS18 Community/v145; isolated Python venv, child WindowsPowerShell
PSModulePath/PATH, PYTHONDONTWRITEBYTECODE=1/MSBUILDDISABLENODEREUSE=1. Native
whole-tree bound100 s, other build/checks95 s. None of 67 accepted final
commands timed out. /m:1 serializes project refs; independent owned project
roots sometimes built concurrently. Signing skipped by original hook.

Environment preparation (not product fixes): pinned vcpkg baseline
`a0b1c8d3a477c1cb4813d8e127a56961707ca42b`, TagLib2.3 + zlib/utfcpp from original
manifest/overlay triplet. Two95-s preparation timeouts (clone/setup then cache
submission) killed entire process trees; final vcpkg install succeeds, cache
submission disabled only for final local no-op confirmation. SDK10.0.401 and
SDK8.0.425 installed under own build roots using official dotnet-install;
net48 reference packages restored, SDK8 used for WebView2/net8 compatibility.
Scoped MSBuildSDKsPath/DOTNET_ROOT/CLI_HOME/PATH, RoslynCompilerType=Core and
MSBuildEnableWorkloadResolver=false (no optional .NET workloads here). Explicit
WebView2PackagesRoot points to restored own NuGet cache. npm from official
registry + bundled Node, existing npm ci lockfile installs Prism1.29.0;
Markdig AOT published by original targets. Details/hashes in external evidence.
Initial SDK resolver/ref-assembly/Roslyn/workload/native header/npm failures
are recorded separately, not counted PASS. No repo build rules changed.

First GUI PIDs15620/3500 waited inside missing Portables language dialog;
read-only own process stack showed LoadLanguageModule -> SalMessageBoxEx.
Own processes stopped; built missing peer resource DLL, then fresh GUI rerun.
Debug metadata startup then exposed two icon diagnostics at unchanged
serviceexplorer/fs1.cpp:48 and plugin.cpp:226. Raw LoadImage + HANDLES DestroyIcon
mismatch confirmed in parent source, including analogous plugin.cpp:190 path.
After ordinary operations Alt+F4 showed `Detected memory leaks!`; reporter then
visible. Heap cause not yet attributed to baseline or port. Bug Reporter CU
read had app approval timeout; no report sent. Only own helper processes with
matching parent PID/path stopped, user-installed helpers preserved.

Next minimal step [S02b-preflight](steps/S02b-preflight.md): resolve ownership
and attribute/fix heap, then repeat relevant runtime acceptance. Do not waive
heap checks, claim clean Debug shutdown, create accepted code commit or start
S03. No full plugin operation matrix, external viewer/editor Unicode launch
or all internal-viewer navigation claims. Prepared code/tests remain in work.

## Historická S02a — core A/W handle wrappery, 2026-10-13

**S02a DONE; lokální brána PASS. S02 celkem PARTIAL; S02b je další krok.**
Parent `2c1480e064d5fd8cca0174b02ae5503411701b7b`, runtime patch `6154561f61dc71387b5582b03ab0825752409a5b2bb8903637f5e5f77e661e61`,
code/test patch `e36fe9d1859e04a81faaa5a4615ba856c307ad7e170bcbdd6ef259679760827c`. Raw source hashe a přesné příkazy/args/env/časy/exit
v `evidence/S02a/validation-inputs.json`, per-run JSON a validation-summary.json.
Externí checkpoint váže finální commit na tento patch; logy ani binárky nejsou
v Gitu. SOURCE/CONTRACT beze změny; donor čistý a připnutý. Žádný push/PR.

| Kontrola | Skutečný výsledek |
| --- | --- |
| Host/resources Debug / Release x64 v145 | PASS exit 0, 19,53 / 43,98 s |
| Salmon Debug / Release x64 | PASS exit 0, 4,24 / 5,30 s |
| Translator Debug / Release Win32 | PASS exit 0, 7,41 / 8,18 s |
| Trace Server Debug / Release Win32 | PASS exit 0, 6,03 / 7,90 s |
| Native Debug / Release x64 | PASS 39 skupin/config, 0 failures, 39,87 / 46,14 s |
| Nový core handles native projekt | PASS Debug 132, Release 109 checks / 0 failures |
| Stávající S01 SafeFile harness | PASS 99 checks/config / 0 failures |
| CI pytest výběr | PASS 49, 4,26 s |
| Negative control proti parent core | EXPECTED FAIL exit 1, 3,10 s; chybějící explicitní A/W členy odmítne compiler/probe |
| GUI Debug PID 10296 / Release PID 15628 | PASS fresh panely, F3 ASCII obsah, F5 copy/hash, normální shutdown |
| PHP process-runtime interpreter | SKIPPED; PHP chybí |
| LoadImageW bitmap UTF-16-long cesta | 1 backend subcase/config SKIPPED; raw Win32 error 2; wrapper parity a nulový registry záznam při failure PASS |
| FindFirstChangeNotificationW UTF-16-long adresář | PASS skutečný signál správného vlastního adresáře a cleanup |
| S02a CodeQL / čistá publikační platformní-plugin matice | NOT RUN; lokální nepublikovaný checkpoint |

Skutečné MSBuild příkazy: každý z následujících projektů zvlášť pro Debug a
Release, `/m /t:Build /p:Configuration=<Debug|Release> /p:Platform=<platform>`
`/p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\pr769-baseline-v145\`
`/p:PreferredToolArchitecture=x64 /nr:false /v:minimal`:

- `src\vcxproj\salamand.vcxproj`, x64;
- `src\vcxproj\salmon\salmon.vcxproj`, x64;
- `src\vcxproj\translator\translator.vcxproj`, Win32;
- `src\vcxproj\tserver\tserver.vcxproj`, Win32.

Toto jsou všichni produkční konzumenti common/handles.cpp z auditované sady
vcxproj. Pluginy používají oddělené mhandles — S02b. Win32/ARM64 varianty
host/salmon a Release clean nejsou touto lokální bránou ani čistou publikační
maticí prohlášeny za ověřené. Translator/tserver mají pouze Win32 konfigurace.

```powershell
pwsh.exe -NoProfile -File tools\run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath ..\evidence\S02a\native-debug.xml -CommandTimeoutSeconds 60
pwsh.exe -NoProfile -File tools\run_native_tests.ps1 -Configuration Release -Platform x64 -ResultsPath ..\evidence\S02a\native-release.xml -CommandTimeoutSeconds 60
python.exe -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/code_metrics/tests tools/codeql/tests tools/localization/tests --junitxml=../evidence/S02a/python.xml -p no:cacheprovider
MSBuild.exe src\tests\handles_wide_tests.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64 /p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\S02a-negative\ /p:CoreHandlesDir=C:\Samandarin\codex_pr769_prompts\evidence\S02a\negative-core /p:PreferredToolArchitecture=x64 /nr:false /v:minimal
```

Poslední příkaz je úmyslná negativní kontrola se starým handles.h/.cpp z parentu
v odděleném outputu. Pozitivní nový projekt automaticky objeví stávající runner;
není nutná změna registrace. MSBuild VS18 Community v145, izolovaný baseline
venv a pwsh; child WindowsPowerShell PSModulePath/PATH, PYTHONDONTWRITEBYTECODE=1,
MSBUILDDISABLENODEREUSE=1. Whole-tree bound 95 s, native 100 s, subprocess60 s;
žádný timeout. Signing hook přeskočil podpis podle stávajícího nastavení.

Produktové dva soubory jsou EXACT vůči donorovým Git blobům po normalizaci
newline. Nový native test linkuje skutečné core handles/trace/messages/widepath
s aktivním Debug trackerem; Release přirozeně používá Win32 bez trackeru, proto
nižší počet kontrol. Nepoužívá fake registry. Ověřuje bitmap pixel bytes,
registry type/origin/count/cleanup, own resource icon/cursor, missing-file/
directory sentinel a legacy makra. Samostatná UNICODE translation unit
kompiluje W generic aliasy; hlavní ANSI TU ověřuje A generic aliasy.

Vlastní fixtures zahrnují ASCII, Unicode filename, Unicode parents včetně CJK/
emoji, UTF-8 byte délku nad 260 při kratší UTF-16 cestě a skutečnou UTF-16 long
cestu s extended prefixem. Raw LoadImageW na poslední bitmapě selže i mimo
wrapper; stejné selhání se neoznačuje jako úspěšné načtení. Není přidán nový
bitmapový loader ani workaround mimo donor. Sentinel a reversní owned cleanup
PASS. Je to přímá chybějící testovací mezera donoru, nikoli import širších
Branch View testů. Fixture ledger adaptovaný z donorového SafeFile testu;
minimální test precomp a vcxproj jsou integrační adaptéry. Nové testy W4/WX,
stávající produkční common sources W3/WX jako host, tracking/new nezměněny.

Přípravné neúspěšné kompilace test adaptéru (missing precomp/max/tracing,
legacy W4 warnings) a první fixture run (system-resource ownership a málo
hluboká cesta) jsou uchované mimo Git a nejsou PASS. Opraveny před finální
regresní bránou; produktové hunks se kvůli nim neměnily.

GUI přes Computer Use použilo pouze vlastní právě sestavená exe s novými
nulovými file-backed configy. Obě kopie shodné SHA256 se zdrojem; všech 7
originálů včetně dlouhého souboru beze změny. Vlastní bootstrapy obnoveny,
exe hashe odpovídají launchům a runtime manifestům, staged convert bytes
odpovídají targetu a .tab délky 256. Uživatelská instalace/nastavení nedotčeny.
External viewer/editor launch a úplná Unicode internal viewer navigace nejsou
součástí této změny ani GUI PASS; příslušné migrace až pozdější etapy.

REUSE: 408 řádků, 2 EXACT, 5 ADAPTED, 14 REPLACED, 387 DEFERRED. Dva core
hunks mají opravené původní nepřesné názvy; pluginové řádky S02b DEFERRED.
Všechna příští rozhraní a testy zůstávají v původních odložených etapách.

## Historická S01 — SafeFile Retry/Skip, 2026-10-12

**S01 DONE, lokální Debug/Release x64 brána PASS.** S02 nezačala. Piny SOURCE
beze změny. Parent `87b8afae7a8c2a934527468130bb7e7a2bac7e5c` plus runtime patch
`470A2BB28DC7E587D99D0CE9804F5AF978A0DA7A1233DCA5F20DE79E550BE815`; code/test/runner patch
`092530462D9C3CA59CCC27CE3361A3BDECA5CDE51FA06FE25C5C23D41529D150`.
Přesné raw source hashe v `evidence/S01/validation-inputs.json`, výsledné SHA
po commitu v `evidence/S01/checkpoint.json`. Souhrn, kopie, původní fixtures a
runtime manifesty v `evidence/S01/validation-summary.json` a sousedních JSON.

| Kontrola | Skutečný výsledek |
| --- | --- |
| Host/resources Debug x64 v145 | PASS exit 0, 8,46 s |
| Host/resources Release x64 v145 | PASS exit 0, 35,62 s |
| Native Debug x64 | PASS 38 skupin / 0 failures, 39,72 s |
| Native Release x64 | PASS 38 skupin / 0 failures, 40,66 s |
| SafeFile compiled-production harness | PASS 99 kontrol / 0 failures v každé konfiguraci |
| CI pytest výběr | PASS 49 testů, 4,82 s |
| Negative control proti parentu | Úmyslně FAIL 7/99; stejný harness, skutečné I/O; write prefix/read content/silent read/OOM regresní detekce |
| GUI Debug PID 7524 | PASS fresh panely, F3 ASCII, F5 copy/hash, normální shutdown |
| GUI Release PID 15768 | PASS stejný scénář a normální shutdown |
| PHP process-runtime subcase | SKIPPED, PHP interpreter nedostupný |
| S01 čistá plná platformní/plugin matice, CodeQL | NOT RUN; lokální nepublikovaný checkpoint |
| Unicode/long/recursive/root SafeFile scénáře | DEFERRED do S03, nikoli PASS |

Skutečné host příkazy (spuštěny v `work/`, obě konfigurace zvlášť):

```powershell
MSBuild.exe src\vcxproj\salamand.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64 /p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\pr769-baseline-v145\ /p:PreferredToolArchitecture=x64 /nr:false /v:minimal
MSBuild.exe src\vcxproj\salamand.vcxproj /m /t:Build /p:Configuration=Release /p:Platform=x64 /p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\pr769-baseline-v145\ /p:PreferredToolArchitecture=x64 /nr:false /v:minimal
pwsh.exe -NoProfile -File tools\run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath ..\evidence\S01\native-debug.xml -CommandTimeoutSeconds 60
pwsh.exe -NoProfile -File tools\run_native_tests.ps1 -Configuration Release -Platform x64 -ResultsPath ..\evidence\S01\native-release.xml -CommandTimeoutSeconds 60
python.exe -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/code_metrics/tests tools/codeql/tests tools/localization/tests --junitxml=../evidence/S01/python.xml -p no:cacheprovider
python.exe -B src\tests\safe_file_paths_tests.py --configuration Debug --source-ref 87b8afae7a8c2a934527468130bb7e7a2bac7e5c
```

MSBuild z VS18 Community, Python z izolovaného baseline-v145 venv. Zachován
child PATH/WindowsPowerShell PSModulePath, `PYTHONDONTWRITEBYTECODE=1` a
`MSBUILDDISABLENODEREUSE=1`. Každý celý process tree bounded na 95 s (native
100 s), žádný timeout. Přesná exe/args/env/čas/exit v per-run JSON; poslední
příkaz je záměrná negativní kontrola, exit 1 / 7 failures, ne chyba fixu.

Test je generovaný nativní C++ harness proti aktuálním SafeFile metodám,
existujícím SDK typům a target size/wide helperům. Dialogy a jednorázové chyby
ReadFile/WriteFile/DupStr jsou řízené testem; ostatní I/O je skutečné Win32.
A helper adaptéry a lokální warning 4701/4703 scope kolem staré file/directory
větve SafeFileCreate jsou testovací adaptace. Produkt nemá vypnutý tracker
ani warningy a žádná donorová assertion nebyla oslabena; S03 případy odloženy.
První dvě přípravné kompilace harnessu selhaly na escape v adaptéru a staré
Create control-flow diagnostice; opraveno před finální bránou, logy zachovány.

Obě GUI spouštění použila nový nulový file-backed config v našem build rootu,
obě vlastní kopie jsou hashově shodné s `ascii.txt`, všech 7 původních fixture
souborů včetně Unicode/long fixtures a sentinelu beze změny. Private bootstrapy
obnoveny; skutečné uživatelské konfigurace nedotčeny. Runtime manifesty mají
launch exe hash a stage patch; convert payload bajtově shodný s targetem.
Vynucené Retry/Skip nebyly prováděny v živém GUI — ověřeny v nativním harnessu.

Baseline opravy S00b/S00c byly mezitím publikované samostatně v upstream
[#774](https://github.com/KRtkovo-eu-AI/salamander/pull/774) a
[#775](https://github.com/KRtkovo-eu-AI/salamander/pull/775). Jejich vlastní fork
CI #18/#19 má pro stejné head SHA 14/14 SUCCESS včetně CodeQL. To jsou výsledky
baseline oprav, nikoli CodeQL ani platformní matice nového S01 checkpointu.

## Historická S00c — první panely, 2026-10-12

**S00c DONE; lokální x64 baseline PASS v deklarovaném rozsahu.** S01 nezačala.
Ověřený parent `216e79d1e4f6f56ba1da7a8297906b1da9796df0` plus runtime patch
SHA256 `18AC1A9A8078543763711991D4FF637DED17365EB00A900BED04C543D0FED47F`.
Dokumentační dokončení neovlivňuje tento runtime diff. Jediná kódová změna je
WM_CREATE v `src/mainwnd3.cpp`: dvě visibility aktualizace až po přiřazení
aktuálních panelů. Příčina je produktová, na přesném S00b skutečně reprodukována
s novým nulovým file-backed configem (PID 10412), tělo bez viditelných panelů.
LoadConfig při následném restartu dosud maskoval chybu aktivací uložených tabů.

Příkazy a child environment stejné jako S00b: skutečný MSBuild host Debug a
Release x64 s OPENSAL_BUILD_DIR `work/build/pr769-baseline-v145/`, `/m`, `/t:Build`,
PreferredToolArchitecture=x64 a `/nr:false`; venv/MSBuild PATH, WindowsPowerShell
child PSModulePath, PYTHONDONTWRITEBYTECODE=1. `tools/run_native_tests.ps1`
Debug/Release x64, CommandTimeoutSeconds=60, ResultsPath do `evidence/S00c`.
CI pytest výběr beze změny, Python `-B`, `-p no:cacheprovider`, vlastní JUnit.
Přesné executable/args/SHA/patch hash/čas/exit ve stejnojmenných JSON. Bounded
whole-tree limit 95 s, native 100 s; žádný timeout, všechny procesy pod 120 s.

| Kontrola | Výsledek |
| --- | --- |
| Host/resources Debug x64 | PASS exit 0, 15,46 s |
| Host/resources Release x64 | PASS exit 0, 60,84 s |
| Native Debug x64 | PASS 37 skupin, 0 failures, 59,80 s |
| Native Release x64 | PASS 37 skupin, 0 failures, 53,10 s |
| CI pytest výběr | PASS 49 testů, 7,55 s |
| PHP process-runtime subcase | SKIPPED, php.exe nenalezen |
| Native panel icon/conversion-table a tab/source-contract sady | PASS v obou runnerech |
| První Debug start | PASS oba panely, F3 ASCII, F5 copy, shutdown; PID 15376 |
| Debug restart | PASS oba panely a shutdown, PID 9352; viewer/copy neopakovány |
| První Release start | PASS oba panely, F3 ASCII, F5 copy, shutdown; PID 9296 |
| Release restart | PASS oba panely a shutdown, PID 16136; viewer/copy neopakovány |

Oba post-fix první starty měly vlastní nový config.reg přesně 0 bajtů a stejný
empty SHA256, vlastní absolutní RegFilePath v configstorage.ini. Oba panely byly
viditelné při prvním pozorování bez aktivace tabu, refresh, resize či restartu.
Po normálním uložení/restartech funguje dosavadní config restore. Není měněna
volba/theme palette ani Welcome/registry import; tyto scénáře nebyly testovány.
Computer Use vidělo přesné vlastní exe cesty, viewer skutečně zobrazil
`PR769 baseline ASCII content`, F5 správný nový vlastní cíl. Obě kopie mají
zdrojový hash; všech 7 původních fixture souborů beze změny. Debug oba běhy
normálně skončil bez handle/leak diagnostik, zachovává opravu S00b.

`Launch-Smoke.ps1` a `Record-Validation.ps1` jsou mimo Git v `evidence/S00c`.
Poslední obnovuje pouze vlastní předchozí runtime bootstrapy. Runtime manifesty
obsahují relativní cesty/hash/délku a označené mutable testovací .reg/INI; exe
hash odpovídá launch reportům. Convert payload bajtově odpovídá targetu, .tab
256 bajtů; 357 Inno staging souborů ověřeno, 0 vcpkg DLL payloadů. Nezměněné
helpers/resources jsou vlastní target/S00b runtime, žádné donor/installed DLL.
Code-growth proti tagu 0.15.1 net +58, samotné S00c +4/-2 net +2. Signing
původním hookem přeskočen. Plná publish matice/CodeQL, všechny plug-iny a donorové
scénáře NOT RUN; ruční long/Unicode viewer mimo scope této visibility opravy.

REUSE obsahuje nový samostatný baseline REPLACED řádek S00c, žádná donorová
implementace převzata; původních 393 DEFERRED řádků nezměněno. SOURCE piny a
CONTRACT beze změny, bez push/PR. Další nejmenší etapa je [S01](steps/S01.md).

## Historická S00b — evidence/cleanup, 2026-10-12

**S00b oprava PASS v provedeném rozsahu; baseline BLOCKED kvůli prvnímu startu
s prázdným private configem.** S01 nezačala. Testovaný parent
`0772e7d228a6fc5d587c8def3418f744a9ebb9a1` plus finální runtime patch SHA256
`EB4E10DC6D1448AB23D66252E2DF9C6F23C807B111D7246EA33BAD53F5A35017`;
patch, přesné příkazy, logy, JUnit a manifesty jsou v
`C:/Samandarin/codex_pr769_prompts/evidence/S00b`. Dokumentační dokončení nemění
testovaný runtime diff. Tento fix je nový doložený baseline adaptační kód,
nikoli převzetí donoru. Devět REPLACED řádků S00b v REUSE je odlišných od
původních 393 DEFERRED donorových řádků.

Finální příkazy jsou stejné MSBuild host a native/pytest příkazy jako S00a,
se stejným OPENSAL_BUILD_DIR a s výsledky v `evidence/S00b` (`*-final.xml`).
Použitý `Invoke-Bounded.ps1` navíc zaznamenává runtime patch hash. V PATH je
venv/MSBuild; pro postbuild Windows PowerShell je child `PSModulePath` omezen
na WindowsPowerShell Modules, aby se neimportovaly nekompatibilní PS7 moduly.
První Debug kompilace/link prošly, staging selhal na chybějícím Get-FileHash
kvůli module environment; záznam není PASS. Scoped child environment a nový
úspěšný staging problém odstranily, bez změny produkčních skriptů či globálního
prostředí. Žádný timeout; procesní limit 95/100 s, native subprocess limit 60 s.

| Finální kontrola | Výsledek |
| --- | --- |
| MSBuild Debug x64 host/resources | PASS exit 0, 10,45 s; `host-Debug-x64-accepted.*` |
| MSBuild Release x64 host/resources | PASS exit 0, 67,59 s; `host-Release-x64-accepted.*` |
| Native Debug x64 | PASS 37 skupin, 0 failures, 52,53 s |
| Native Release x64 | PASS 37 skupin, 0 failures, 60,78 s |
| CI pytest výběr | PASS 49 testů, 6,48 s |
| PHP process-runtime subcase | SKIPPED, php.exe nedostupné |
| Native conversion table a panel icon size | PASS v obou native sadách |
| Debug/Release GUI, initialized config | PASS panely, F3 ASCII, F5 copy, kontrola hashů a shutdown |
| Debug exit tracking | PASS finální proces ukončen normálně bez „not found“ či leak dialogu |
| První empty private config | Omezení trvá v opakování před finálními shutdown opravami; S00c znovu ověří finální revizi |

Runtime zůstává výhradně v tomto worktree a konfiguraci. Nezměněné
salmon/salopen/salspawn helpers jsou vlastní S00a buildy stejných target zdrojů,
včetně obnovených Debug utils po Release restagingu; žádná donor/installed DLL.
Host znovu ověřil 357 Inno payload souborů (0 změn), 0 vcpkg DLL payloadů.
Finální manifesty 469 Debug / 485 Release souborů obsahují hash a source+patch
vazbu; conversion data bajtově shodná s targetem. Code-growth hook zaznamenal
proti tagu 0.15.1 +92/-36 net +56, z toho samotný runtime fix +55/-26 net +29.
Signing přeskočen původním hookem; plná publikační matice/CodeQL NOT RUN.

Computer Use ověřilo úplné exe cesty a obsah vieweru. Finální Debug PID 7072,
Release PID 5880; nové prázdné copy cíle v původním vlastním GUID fixture rootu,
oba výsledky shodné s ASCII zdrojem. Všech 7 originálů včetně Unicode/long
fixtures a sentinelu beze změny. Ruční long/Unicode viewer scénáře v tomto
fixu NOT RUN (Unicode viewer na targetu byl ověřen S00a); path behavior se nemění.
`validation-summary.json`, `gui-*-accepted-launch.json`, runtime manifesty.

Meziverze Debug po první opravě odhalila další startup DestroyIcon diagnostiku
`icncache.cpp:1183`. Po initialized panel/viewer/copy meziverze hlásila 9
zbylých tracker položek. Read-only diagnostika vlastního procesu podle vlastního
PDB identifikovala dvě owned window ikony, šest štětců z gui.cpp a duplicitní
copy file handle (`debug-shutdown-handles.json`). Tyto konkrétní páry jsou
opraveny; tracker nebyl vypnut. Trace Server byl sestaven Win32 (x64 kombinace
nepodporována), neexponoval UI, vlastní pomocný proces ukončen; nebyl náhradou
diagnostiky. Finální shutdown bez dialogu ověřuje odstranění všech devíti.

Piny SOURCE a smlouva nezměněny. Donorové hunks stále DEFERRED, samostatné
baseline opravy nevydávat za S01 či S02. Bez push/PR. Další nejmenší krok je
[S00c](steps/S00c.md), diagnostika prvního private config startu.

## Historická S00a — obnova prostředí, 2026-10-12

**Baseline: BLOCKED kvůli Debug GUI startup diagnostikám.** v145 a testové
dependencies již nejsou blokací. S00a je pouze dokumentační checkpoint;
žádný runtime kód ani donorový test nebyl přenesen. Testovaný checkout
`75950e887e34ec4bdbe1eb71aa6c04b0c3bfaa8d` obsahuje S00 docs; runtime zdroje
jsou beze změny proti TARGET_SHA `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`.
Piny SOURCE a všech 393 DEFERRED řádků REUSE (231 donorových souborů) nezměněny.

### Prostředí a reprodukce

- Uživatelem autorizovaná instalace: VS 2026 Community 18.10.3,
  installationVersion 18.10.12224.181, MSBuild 18.10.1.42706,
  MSVC 14.51.36231 / v145, SDK 10.0.26100.0; bez požadavku na restart.
  Instalační/probe evidence je v `evidence/toolchain-install`.
- `git submodule update --init --recursive -- src/plugins/sftp 3rd-party/HardView`
  PASS; SFTP `a293b5beaecef97b56974d4d119a7adc26adb2d4`, HardView
  `b7e68cf032ad2a7dd556775da67ac8424896372f`. Žádný update jejich refs.
- Izolovaný `evidence/baseline-v145/venv`: pytest 9.1.1, lizard 1.23.0,
  přesné dependencies v `python-freeze.txt`. Globální Python/Git beze změny.
- CWD příkazů níže: `C:/Samandarin/codex_pr769_prompts/work`.
  PATH child procesů obsahovala venv Scripts a VS18 MSBuild před inherited PATH.
  `PYTHONDONTWRITEBYTECODE=1`, Python `-B`, MSBuild `/nr:false` a
  `MSBUILDDISABLENODEREUSE=1`. Žádná cache v pracovním stromu nalezena.
- Externí `evidence/baseline-v145/Invoke-Bounded.ps1` zaznamenává executable,
  argumenty, CWD, environment, SHA, exit, čas a logy. Celý procesní strom má
  limit 95 s (native runner 100 s), při timeoutu Kill(true); timeout nenastal.
  Vnitřní native subprocess limit 60 s. Všechny příkazy skončily pod 120 s.

```powershell
& 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe' src\vcxproj\salamand.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64 /p:OPENSAL_BUILD_DIR=C:\Samandarin\codex_pr769_prompts\work\build\pr769-baseline-v145\ /p:PreferredToolArchitecture=x64 /nr:false /v:minimal
# Stejný příkaz s Configuration=Release.
& ..\evidence\baseline-v145\venv\Scripts\python.exe -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/code_metrics/tests tools/codeql/tests tools/localization/tests --junitxml=../evidence/baseline-v145/python.xml -p no:cacheprovider
& .\tools\run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath ..\evidence\baseline-v145\native-debug.xml -CommandTimeoutSeconds 60
& .\tools\run_native_tests.ps1 -Configuration Release -Platform x64 -ResultsPath ..\evidence\baseline-v145\native-release.xml -CommandTimeoutSeconds 60
```

| Kontrola | Výsledek a skutečný čas celého procesu |
| --- | --- |
| Host + ProjectReference `src/vcxproj/lang.vcxproj`, Debug x64 | PASS exit 0, 26,60 s |
| Host + language resources, Release x64 | PASS exit 0, 55,63 s |
| CI pytest výběr | PASS 49 testů, exit 0, 6,63 s |
| Native Debug x64 | PASS 37 skupin, 0 failures, exit 0, 94,12 s |
| Native Release x64 | PASS 37 skupin, 0 failures, exit 0, 75,50 s |
| Native conversion-table | PASS obě konfigurace, skutečná shipped data |
| PHP process-runtime subcase | SKIPPED: php.exe nenalezen; skupinový JUnit skipped=0 toto nesděluje |
| Release GUI, inicializovaný private config | PASS běžné panely, ASCII/Unicode F3, ASCII F5 copy |
| Release GUI, první prázdný private config | FAIL pozorování vykreslení panelů; příčina neurčena |
| Debug GUI | FAIL při startupu, baseline nepřijata |

Native sada obsahuje 16 native a 21 source-contract/PowerShell skupin v každé
konfiguraci. Čas v JUnit (Debug 20,783 / Release 13,283 s) zahrnuje pouze testy,
nikoli úspěšné kompilace; nepoužívat jej jako dobu celého runneru.

### Runtime a GUI evidence

Host staging skutečně zkopíroval 357 Inno payload souborů (toolbars/doc/convert)
a našel 0 vcpkg DLL payloadů. Není to sestavení všech plug-inů. Release signing
zůstal přeskočen původním hookem. Source-growth hook úspěšně zaznamenal výchozí
+37/-10 (net +27) proti tagu 0.15.1; jde o původní target, nikoli portový růst.

Samostatně byly stejnými MSBuild flags sestaveny projekty
`src/vcxproj/salmon/salmon.vcxproj`, `salopen/salopen.vcxproj` a
`salspawn/salspawn.vcxproj`, Debug i Release x64, vše PASS. `MyEntryPoint`
potřebuje vlastní `utils/salmon.exe`. Release salopen/salspawn postbuild kopíruje
helpers i do Debug runtime; po Release buildech byly Debug salopen/salspawn
obnoveny `/t:Rebuild` (PASS). Nepoužívat souběžné helper buildy jako důkaz
konfigurační izolace. Žádná binárka z donoru ani instalované aplikace nepřimíchána.

Runtime kořen `work/build/pr769-baseline-v145/salamander/{Debug,Release}_x64`.
Manifesty `runtime-Debug-manifest.json` (469 souborů) a
`runtime-Release-manifest.json` (485) obsahují relativní cesty, SHA256 a délky,
vynechávají Intermediate a označují vlastní config jako mutable smoke stav.
Veškeré staged convert soubory se hashově rovnají target datům, všechny .tab
mají 256 bajtů. GUI běželo v novém procesu po stagingu.

Externí `smoke-fixture.json` popisuje vlastní GUID adresář pod izolovaným build
rootem: ASCII, české/CJK názvy a parent, duplicitní basename, UTF-8 376 bajtů
při UTF-16 206 jednotkách, UTF-16 393 jednotek a sentinel. Dlouhé cesty byly
vytvořeny, ručně však neověřeny; donorové scénáře zůstávají DEFERRED.

Každý runtime má vlastní `configstorage.ini` s `StorageType=RegFile` a vlastní
absolutní `RegFilePath`; skutečná uživatelská konfigurace se neimportovala.
Release byl nejprve spuštěn s platným prázdným config.reg: panely zůstaly bílé
i po refresh, změně view a resize. Po normálním zavření, uložení vlastní
konfigurace a novém startu byly panely viditelné. Příčina není prokázána.

Spuštění přes CLI přesné exe (nikoli instalovaného Salamanderu) použilo
zdrojově ověřené argumenty `-l "<own-fixture-root>" -r "<own-fixture-root>\copy"
-t "PR769 BASELINE Release" -language english`. `english.slg` byl chybný
přípravný pokus a není započten do PASS. `sky.launch_app` nezajistil požadovaný
staged proces; jeho případné instalované okno se také nezapočítává.
Skutečný Release PID 7208 a úplná exe cesta byly ověřeny v Computer Use.
F3 `ascii.txt` zobrazil `PR769 baseline ASCII content`; F5 kopie do prázdného
copy adresáře byla potvrzena se správným cílem. F3
`žluťoučký-東京/český-東京.txt` zobrazil `PR769 baseline CJK file content`.
Kontrola SHA256 potvrdila kopii a všech 7 nezměněných původních souborů včetně
sentinelu. Report `gui-smoke.json`, reprodukce `Prepare-Smoke.ps1` a
`Record-Smoke.ps1`; testovací okna Release byla normálně zavřena.

Debug PID 4496, vlastní Debug exe, dostal kopii pouze tohoto vlastního
materializovaného configu. Startup hlásil `DeleteObject: Handle ... not found`
z `src/iconlist.cpp:483`, poté po potvrzení z `:485`, oba
`src/common/handles.cpp:864`. Druhá diagnostika ukončila smoke; jen tento vlastní
proces byl terminován. Debug normální panel, viewer ani copy nejsou PASS.
Úzký diff potvrdil, že `iconlist.cpp` je shodný v targetu a donoru. Oprava není
součástí S00a; další předkrok [S00b](steps/S00b.md). SDK ani tracker se nevypínaly.

Plná publikační build matice, všechny plug-iny/dependencies a CodeQL NOT RUN;
toolchain probes Win32/ARM64 nejsou plná matice. Žádné vzdálené CI, push či PR.
Nová úplná evidence je mimo Git v
`C:/Samandarin/codex_pr769_prompts/evidence/baseline-v145`.

## Historický S00 před instalací — následující původní záznam

**Tehdejší baseline: BLOCKED.** Testovaná revize runtime zdrojů je
`a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449` (TARGET_SHA), 2026-10-12.
S00 přidává pouze dokumentaci. Žádný `salamand.exe` ani úplný staging z targetu
nevznikl; funkčnost aplikace ani GUI smoke se neprohlašují za ověřené.

## Prostředí a limity

- Windows `10.0.26200`, PowerShell Core `7.6.5`, Python `3.12.10`.
- `vswhere -all` našel pouze VS 2022 BuildTools `17.14.37614.0`, MSVC
  `14.44.35207` (v143). Target projekty i build.cmd vyžadují **v145 / VS 2026**.
- SDK include `10.0.26100.0` je dostupné. `msbuild`/`cl` nejsou v počáteční PATH.
  Jen pro child procesu native runneru byla PATH rozšířena o existující MSBuild.
- Chybí Python balíčky `pytest` a `lizard`. Nic nebylo instalováno ani retargetováno.
- Pinned SFTP submodule `a293b5beaecef97b56974d4d119a7adc26adb2d4` a HardView
  `b7e68cf032ad2a7dd556775da67ac8424896372f` nejsou inicializovány.
- Platí živé sandbox permissions; `.codex/config.toml` s danger-full-access
  je nepřepisuje. `.codex/environments` nemají setup script.
- AGENTS vyžaduje eskalaci každého příkazu a ukončení včetně child procesů
  před 120 s. Použitý externí `evidence/baseline/Invoke-Bounded.ps1` měl
  90s limit celého procesního stromu a `Kill(true)` při timeoutu. Žádný timeout
  nenastal. Native runner měl navíc 8s limit jednotlivého subprocessu.
- `PYTHONDONTWRITEBYTECODE=1`, Python `-B`, MSBuild node reuse vypnuto pro běhy.
  Žádná globální konfigurace se nezměnila. Při delším buildu rozdělit práci na
  povolené konkrétní projekty či použít zvlášť autorizované CI; limit neobcházet.

## Skutečně spuštěné příkazy

CWD všech níže uvedených build/test příkazů:
`C:/Samandarin/codex_pr769_prompts/work`. Jsou to obsahové příkazy uložené
v bounded child scriptech; každý skončil `exit $LASTEXITCODE`. Logy, JSON
s příkazem/SHA/časem a JUnit jsou mimo Git v
`C:/Samandarin/codex_pr769_prompts/evidence/baseline`.

```powershell
& .\build.ps1 -Configuration Debug -Platform x64 -BuildDir 'C:\Samandarin\codex_pr769_prompts\evidence\baseline\target-build'
& .\build.ps1 -Configuration Release -Platform x64 -BuildDir 'C:\Samandarin\codex_pr769_prompts\evidence\baseline\target-build'
```

Oba běhy: **BLOCKED**, hláška o chybějícím v145; nic se nekompilovalo.
Exit kód je v obou případech 0, což **není PASS**. Jde o zjištění ve výchozím
build wrapperu; S00 jej neopravuje. Evidence `build-debug-x64.*` a
`build-release-x64.*` (1,04 s / 1,02 s).

```powershell
$env:PATH = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin;' + $env:PATH
& .\tools\run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath '..\evidence\baseline\native-debug-x64.xml' -CommandTimeoutSeconds 8
```

Exit 1, 15,02 s. **37 skupin: 20 PASS, 17 FAIL**. Šestnáct nativních buildů
selhalo MSB8020 kvůli v145, jejich nativní programy nebyly spuštěny.
`ui_font_contract_tests` selhal FileNotFoundError na
`src/plugins/sftp/src/fs1.cpp` z neinicializovaného submodulu. Zbylých 20
source-contract/PowerShell smoke skupin prošlo; ty nejsou GUI smoke ani náhrada
nativních testů. Přesné názvy: `native-summary.json`, výsledky `native-debug-x64.xml`.

```powershell
python -B -m pytest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/code_metrics/tests tools/codeql/tests tools/localization/tests --junitxml=../evidence/baseline/python.xml -p no:cacheprovider
```

Exit 1, **BLOCKED**, `No module named pytest`; `python.xml` nevznikl.
Evidence `ci-python.*`. Chybí i `lizard`, potřebný pro část této sady/metriky.

```powershell
python -B -m unittest src/plugins/automation/tests/salamatrix_ai_local_tests.py tools/localization/tests/test_rebase_text_archive.py tools/localization/tests/test_serviceexplorer_resources.py tools/localization/tests/test_translate_slt_with_openai.py
```

Exit 0, **41 testů PASS**, 1,29 s včetně startu procesu. Evidence
`stdlib-python-subset.*`. Jde pouze o existující unittest kompatibilní subset;
pytest metrics/CodeQL testy tím nebyly provedeny.

## Matice pro další etapy

| Kontrola | Skutečný vstup/požadavek | Stav S00 |
| --- | --- | --- |
| Host + změněné části + transitivní dependencies | Debug i Release x64, v145, relevantní resources | BLOCKED v145 |
| Sdílené SDK/hlavičky | Všichni aktuálně podporovaní konzumenti + vydané ABI prefixy | Neprovedeno; žádná taková změna v S00 |
| Nativní sada | `tools/run_native_tests.ps1 -Configuration Debug -Platform x64 -ResultsPath ...` a Release podle smlouvy | Debug buildy blokované; Release neprovedeno |
| CI platformy | PR Build Debug Win32/x64/ARM64; `/t:Rebuild`, `/warnaserror`, `/m`, `/nr:false`, PreferredToolArchitecture=x64 | Neprovedeno |
| Čistá sestava pro publikaci | Celá podporovaná konfigurace; přesné SHA review větve | Neprovedeno, není publikační zadání |
| CodeQL | Po úspěšném PR Build, Release x64 host, `security-extended`, přesné head SHA | Neprovedeno |
| Source growth | Aktuální base a canonical release tagy, `lizard==1.23.0` | Neprovedeno; lizard chybí |
| Native conversion table | Skutečné shipped binární convert.cfg/.tab | Build blokovaný |
| Normal-panel a změněná operace | Skutečný staged runtime, GUI evidence | BLOCKED; žádný target runtime ani native UI ovládání |
| Donorové I/O fixtures | [FIXTURES.md](FIXTURES.md), současně s portovaným kódem | DEFERRED do příslušných etap |

`build.ps1` má pouze `Configuration=Debug/Release`, `Platform=Win32/x86/x64`,
`BuildDir`; nemá ARM64, target-list ani clean přepínač. ARM64 a výběr projektů
řeší skutečné MSBuild příkazy/filtr z `pr-msbuild.yml`, ne vymyšlené argumenty.
Native runner podporuje x64, Debug/Release, ResultsPath a CommandTimeoutSeconds
1–119 (default 110); jeho vnitřní timeout nenahrazuje limit celého příkazu.

PR Build nyní vybírá změněné projekty a filtruje nepodporované konfigurace.
SFTP je ve workflow výslovně vypnutý; to nevysvětluje chybějící submodule pro
source-contract testy. CodeQL gate sleduje `src/` C++ mimo `plugins/`; čistě
dokumentační/plug-in-only změna může analýzu přeskočit. Úspěšný workflow se
skipped buildem není provedený build. PR Tests používá Debug x64 a uvedený
pytest výběr; Studio workflow má vlastní paths filter a VSIX/native UI testy.
Při SDK změnách nespoléhat pouze na changed-project selection, pokud nepokrývá
všechny konzumenty požadované smlouvou.

## Staging a důkaz původu runtime

OPENSAL_BUILD_DIR musí mít vlastní absolutní kořen tohoto worktree, s trailing
backslash. Runtime je v `salamander/<Configuration>_<ShortPlatform>`, native
runner používá `work/build/ci-tests` a vlastní `ci-worker-root`. Donor se
nebuildí; žádný output z `C:/Samandarin/branch-view` ani jiného checkoutu se
nesmí přimíchat. Debug/Release mají oddělené podadresáře.

Host `salamand.vcxproj` po úspěšném buildu volá targetové staging skripty:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/stage_inno_payload.ps1 -RepositoryRoot <target-worktree> -DestinationRoot <configuration-runtime>
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/stage_vcpkg_payload_dlls.ps1 -RepositoryRoot <target-worktree> -DestinationRoot <configuration-runtime>
```

To jsou dokumentované vstupy s placeholdery, **v S00 nespouštěné**. Druhý
skript je x64 dependency staging a potřebuje odpovídající dependency payload.
`lang.vcxproj` je host ProjectReference pro `lang/english.slg`; přidat změněné
plug-iny, jejich DLL/resources a správné CRT. Host kopíruje config a Release
metadata/sample skripty. Inno staging kopíruje toolbars/doc/convert bajtově.
Starší `!populate_build_dir.cmd` je interaktivní a obsahuje pevné VS2022/SDK
cesty; nebyl spuštěn a sám není důkazem úplného současného stagingu.

Po úspěchu uložit mimo Git manifest relativních cest, SHA256 všech binárek,
DLL, resources a dat, testované source SHA, build logy i konfiguraci. Ověřit
spuštěnou exe cestu a návaznost na tento build, spustit nový proces po změně
convert (data se cachují). Conversion cfg zachovat v deklarovaných legacy
code pages, .tab přesně 256 bajtů. Teprve pak běžný panel a změněná operace
na vlastním dočasném korpusu. V tomto běhu tato vazba na runtime **neexistuje**.

## Evidence a pokračování

PASS znamená skutečně dokončenou kontrolu konkrétní revize; FAIL rozlišit na
chybu produktu a prostředí; BLOCKED znamená chybějící předpoklad. SKIPPED,
PENDING a NOT RUN nesmějí být přejmenovány na PASS. Evidence JSON uchovává
SHA, CWD, přesný příkaz, exit, timeout, čas a log. Při dirty kódové změně
navíc uložit hash diffu; samotné HEAD SHA nestačí. Plné logy nepatří do Gitu.

S00 staticky kontroluje manifest, 231/231 donorových souborů v REUSE,
32 stage dokumentů, lokální odkazy, staged scope a `git diff --check`.
Tyto dokumentační kontroly nenahrazují výše blokované runtime ověření.
Git identita pro jediný commit byla uživatelem potvrzena jako
`majklvi <7870647+majklvi@users.noreply.github.com>`; použije se jednorázově
bez změny globálního nastavení.

Před S01 nejprve zajistit autorizované v145 prostředí, úplné pinned submodules
a testové závislosti, zopakovat blokované kontroly a získat skutečný runtime
smoke. Dostupné API computer-use nyní umožňuje browser, native UI je vypnuté.
Bez potřebného ověření zůstává přechod na S01 blokován. Jakýkoli kandidátní
commit/CI kvůli ověření vyžaduje příslušné publikační zadání a přesné SHA.
