# S00 read-only donor boundary audit

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


## S17 size boundary complete; remaining copy regression belongs to S20

Only SalGetFileSize2 production change, exact donor file; S04 getters/services
unchanged. Ordinary donor getter assertions run, all future Branch metadata
assertions preserved behind S26 guard and explicitly not run. Disk-size native
79/config includes actual symlink and long paths, ordinary membership 196/config.
Actual D/R GUI copies short ASCII link to long Unicode target as identical
2107-byte regular files. A long Unicode SOURCE link copy fails later Error
Opening File; retained in evidence/S17, mandatory concrete retest in S20.
Do not present size helper support as complete file-operation migration.

## S16 Undelete legacy image boundary

Eight production files and C++ fixture exact donor; Python only configuration
selector. Wide restore is separate from unchanged ACP/fixed-capacity image
engine. Connect preserves full exact input and fails explicitly before backend
on unsupported paths. Actual GUI verified full edit value and capacity refusal,
no physical volume analysis. Native restore performs temporary disk I/O; EFS
is a stub, real encrypted Windows import/export not exercised. Conditional
ACP1250/1252 assertion remains unchanged; registry ACP is recorded separately.

## S15 PAK engine and hidden-command boundary

Five production files and C++ fixture exact donor. OP_EXISTING_ONLY changes
only an internal mode bit, signatures/layout/vtables unchanged. Engine is
compiled into pak.spl by current project; old paklib.dll loader is commented.
The donor/target Connect deliberately hides Optimize; ordinary menu case SKIP.
Actual D/R GUI command/engine exercised with a temporary AddMenuItem adapter,
source/binary hashes recorded separately, not equivalent to an unmodified
menu runtime. Adapter removed byte-exact before final D/R production rebuild
and commit. No public menu restoration in S15. Native production OpenPak
missing-file/noncreation evidence separate from mocked menu engine harness.

## S14 complete archive command boundary

Three exact 7-Zip donor production files and exact shared C++ menu harness.
All S13 deferred 7-Zip assertions restored; driver only adapts helper import.
Actual D/R GUI tests prove focused Unicode archive while another is selected.
Command harness engine doubles are distinct from this real archive testing;
host-managed packing remains S22. Staged engine DLL hashes are not presented
as observed loaded modules after the transient engine has already unloaded.
The S13 ZIP GetMenuItemState long-basename limitation remains unresolved.

## S13 ZIP command boundary and menu limitation

ZIP ExecuteMenuItem migration complete, three exact donor production files.
PackToArchive/engine remain S22. 7-Zip assertions preserved but deferred in
archive_menu_selection_tests ZIP scope until S14; no claim of their execution.
Real Debug selected archives and Release focused archive passed Test Archive
on long Unicode paths; isolated engine harness is separate evidence.
Unchanged donor/target GetMenuItemState uses CFileData.Name for archive masks:
100-CJK basename disables menu even with correct ZIP association. This GUI
case is SKIP; fix before claiming full menu coverage for such basenames.

## S12 completed boundary

Owned thumbnail CEnumFiles and paired EXIF v10 are integrated together.
Legacy EXIF export retained. Five exact donor files verified by Git blobs;
project includes only disk selection SDK, Unicode title/navigation remain S18.
S05 deferred CEnumFiles and both assertions now restored. Real EXIF native
51 checks/config and six actual GUI outputs pass against current binaries.
The separate baseline WIC repair is parent 270f2cb6; initial S12 evidence is
preserved and does not count as current-source full-run/CI validation.

## S12-preflight: WIC output boundary

Evidence confirms a target/donor baseline defect, separate from owned selection
and paired wide EXIF migration. SaveFrame is reused, not replaced by another
image engine. The adapter buffers the whole encoded image; cancellation does
not interrupt WIC encoding. Native adapter tests use a stub encoder dependency;
17 real Release DLL checks and Debug/Release GUI provide separate integration
evidence. S18 navigation and Unicode title remain deferred. S12 parked patch
must be restored after this commit and all its current-source gates rerun.

Pinned donor: `47661e92d4b3cbe5bb593ae3d75a3f1f698ef413`.
Diff base and target: `a84dbed6db632a9e63b5ed9f4bc6691e4cc5f449`.
Scope: changed-file list, selected critical function hunks and S01-S32 prompts. No implementation, donor build, fixture execution, mutation of donor or target, or assertion that a test passed. REUSE.tsv maps all 231 distinct donor paths; every row remains DEFERRED. Shared files have multiple stage rows; these are planning boundaries, not an assertion that all hunks already received independent parity review. S31 must resolve every actual hunk, including BOM-only differences and project registrations.

## S02 audit correction, 2026-10-13

The opening scope/status describes the historical S00 audit. Current progress
is in PROGRESS/REUSE/VALIDATION; not all rows are still DEFERRED. A narrow actual
core diff disproved the original generic CreateFile/CreateEvent hunk labels:
core A/W CreateFile and FindFirstFile are already in the target. S02a reuses
only the exact donor LoadImage and FindFirstChangeNotification A/W additions;
all four core projects and direct live-tracker/native/GUI coverage passed.
Plugin mhandles changes remain S02b DEFERRED. This adds no scanner dependency.

## Existing baseline facilities (reuse)

- `src/common/widepath.h/.cpp` already define `CPathBuffer`, `CWidePathBuffer`, `SalIsExtendedLengthPathW`, `SalPathAddExtendedPrefixW`, `IsValidPathUtf8Text`, `SalMultiByteToWidePath` and wide-to-byte conversion helpers. They are not changed donor files. S02 adds A/W handle wrapper coverage, not another conversion library.
- `src/salamdr3.cpp` already implements `SalCreateDirectoryExW`; `src/salamdr5.cpp` already supplies `SalGetFileSize`. S03's extracted SafeFile harness reads these existing implementations.
- `CSalamanderGeneralAbstract::QueryService` is already in baseline `src/plugins/shared/spl_gen.h:3658`, alongside existing Register/Unregister and appended panel/owned-service lifecycle methods. Concrete QueryService is `src/zip.cpp`. Do not add another virtual transport or alter existing slot order.
- Donor adds optional `Salamander.PanelItemPaths`, `Salamander.DiskSelection`, and `Salamander.ViewerEnumeration` classes/service IDs in spl_gen.h. CFileData remains untouched. `spl_diskselection.h` explicitly supports QueryService-capable Samandarin ABI; fallback must never call an absent historical virtual slot.
- Baseline native projects under src/tests: clipboard_contract_tests, conversion_table_tests, filenamecase_tests, native_decoder_tests, panel_icon_size_tests, storage_scheduling_tests, viewer_unicode_tests. Keep these; inspect other plugin test projects through runner discovery. They are not replaced by donor contracts.

## Concrete stage dependencies and smallest adaptations

1. S01 has no new SDK/scanner/wide prerequisite. SafeFileRead and SafeFileWrite change silent `dlgRet |= DIALOG_SKIP` to assignment; both Retry reopen paths change only the disposition to OPEN_EXISTING while retaining the target's existing CreateFile boundary, desired access, sharing and flags. SafeFileCreate's failed owned filename duplication returns INVALID_HANDLE_VALUE and ERROR_NOT_ENOUGH_MEMORY after closing its handle. Do not import the wide-path helpers/catches merely because donor placed these changes nearby.
2. The full donor SafeFile harness cannot be copied unchanged for S01: cpp.in main unconditionally tests `SafeFileRootLength`, added only in S03, and runs Unicode/long-path scenarios whose implementation is deferred. Reuse `RoundTrip`, `InjectRead`, `InjectWrite`, `FailDuplicate`, `Fixtures` and the generator that compiles actual production SafeFile methods. Keep ASCII retry/offset/content/error tests in S01; explicitly stage the wide/root tests in S03. Add required Write Skip, Read/Write Skip All assertions without weakening existing donor coverage. Donor directly covers silent READ Skip, not all these promised cases.
3. S03 already has wide conversion/directory primitives in target; no Branch scanner dependency. Preserve S01 and the original UTF-8-first/ACP fallback byte-path contract. Core CreateFileA/W already exist in the baseline; S02a adds only LoadImageA/W and FindFirstChangeNotificationA/W. S02b supplies missing plugin CreateFileA/W and FindFirstFileA/W plus wide diagnostic text. S03 must reuse the existing core CreateFileW wrapper instead of duplicating it.
4. S04 ordinary getters are donor `CFilesWindow::GetItemDirectoryW`, `GetItemFullPathW`, `GetItemIdentityW`, `GetItemRelativePathW` in branch_view.cpp:269-309. `GetItemFullPathW` calls `Salamander::BranchView::Join`; the ordinary branch needs NameW/UseWideName plus existing widepath conversion. Extract only these ordinary parts into a compiled host translation unit and a single shared join implementation before scanner stage. The exact target path must be recorded when choosing that minimal extraction; REUSE.tsv retains donor destination as provisional while DEFERRED. Do not add CBranchViewState or stub state to satisfy these symbols. Getters' declarations belong to fileswnd.h.
5. `CPanelItemPathsService::FindCurrentItem/GetItemFullPath` is donor zip.cpp:720; address membership is proven before dereference, capacity and UI thread errors are explicit. `CDiskSelectionService::Capture` is zip.cpp:801 and uses all three getters. S05 therefore requires completed S04. CaptureFallback/OwnedSnapshot in spl_diskselection.h must preserve allocation-module Release and failure atomicity.
6. S17 partly overlaps S04: ordinary panel getters should already be done. Transfer remaining `SalGetFileSize2` wide size boundary in salamdr5.cpp, run getter/disk-size tests, defer real Branch metadata assertions to S26. There are no separate S17 getter modifications in zip.cpp in this pinned diff. Do not invent another host API.
7. S18 viewer enumeration must include `CFileNamesEnumData::LastFileName/FileName` std::wstring and W declarations in consts.h, concrete `CViewerEnumerationService` in salamdr6.cpp, QueryService route in zip.cpp, SDK service declaration, and PictView navigation consumers (`pictview.cpp/.h`, render1.cpp, unicodetitle.h). PictView thumbnail/EXIF is S12; previous/next/navigation is S18. fileactionpath.h includes viewerpath.h; production launch/argument expansion and actual child argv/current-directory tests belong together. S18 may be split launch then viewer service, one accepted commit at a time.
8. S19 workerpath.h is isolated and does not include branch_view.h. Its WorkerOperationPathW/WorkerOpenFile/etc need no scanner import. S20 should split rename+selection, copy/move, and delete; S21 should split native helper+tests and host call sites if size warrants it.
9. S23 Reserve must precede scanner/panel capacity consumers. S24 generic formatting is isolated; resource-dependent text assertions wait for matching S26 IDs. S25 donor branch_view.cpp already fences host code with BRANCH_VIEW_TEST for native scanner tests; production must compile normal code, never hide complete feature behind a compile gate.
10. S26 cannot defer exact cache keys, pointer lifetime, icon-reader shutdown order, generation checks or reentrant safety to S28/S30. Runtime gate defaults off and must cover mainwnd3 dispatch, keyboard activation, history/tab restore, and other real entry points. Experimental commands require explicit operation support. S27 splits a-f, with each plugin independently enabled only after actual end-to-end validation.
11. S29 donor salamand.h hunks concern CPathHistoryItem/CPathHistory BranchState only; viewer declarations are in consts.h. Keep every restoration behind the gate. S30 handles only refresh/publication work not already needed for correctness. S31 audits every original hunk, providers, paired runtime files, SDK and resources; S32 only activates after closure.
12. SDK ABI fixture `disk_selection_snapshot_abi.json` names release `5.0-samandarin-0.15.1` and commit `c0cddbe7442fe49abcb859624e271ce687aee0d8`. It is reusable evidence but not automatically sufficient for the newest release-prefix rule. S05 must also verify actual released target tag/API prefix; never regenerate a golden from the edited header to make tests pass.

## Existing fixture coverage and limits

- SafeFile: `src/tests/safe_file_paths_tests.cpp.in`, `Fixtures::{Owned,Directory,File}`, `RoundTrip`, `InjectRead`, `InjectWrite`, `main`. Own process/tick unique temp root; Own checks before deletion; reverse cleanup. Cases include ASCII, Czech/CJK/emoji name, multiple Unicode parents, 90 CJK-character basename, deep UTF-16 path. RoundTrip writes `first-` then forced-failure/retry `second`, checks 12-byte content and seek offset, silent read skip, overwrite cancel and unchanged complete file; main injects DupStr allocation failure and verifies exclusive reopen. UNC root construction is a string assertion, not an SMB test. Read-only/DOS-alias/Write Skip/SkipAll scenarios are not established by this fixture and remain required acceptance gaps.
- Renamer: `src/tests/renamer_io_tests.cpp.in`, `Fixture`, `FileCase`, `ConversionTests`, `RunFilesystem`. `RunFilesystem` explicitly checks `Utf8(utf8Long).size()>260 && utf8Long.size()<260` for an 85-Han-character parent; separately `deep.size()>260` for true Win32-long UTF-16. ASCII and Czech/CJK parents, duplicate same.png, Unicode names, actual Move/Undo and collision/normalization content preservation are available. The duplicate FileCase payload is basename-derived, so it alone does not establish differing same-name content hashes.
- Checksum: `src/tests/checksum_selection_paths_tests.cpp.in`, `Fixtures::{Owned,Directory,File}`, `Populate`, `RunPending`, `main`. same.bin under ASCII, Czech/CJK and long parents has sizes 1/2/3 (content x/xx/xxx), plus 90-CJK basename of size 4. Actual worker reads and different duplicate hashes are asserted after panel storage is cleared; Verify uses manifest's actual parent. CHashAlgo::GetDigest in this fixture is a one-byte sum STUB (lines 122-128), not CRC/SHA; this proves source bytes/identity but not real checksum algorithms. Add independent real hashes and actual plugin Calculate/Verify at S08/S27f. This is production command/worker extraction, not launched plugin smoke.
- Split/Combine: `src/plugins/splitcbn/tests/selection_paths_tests.cpp.in` has actual temp files for ASCII, Czech name/parents, Czech+CJK parents, UTF8-byte-long and deep UTF16 paths; verifies sibling discovery and input/output identity including hardlinks where supported. Its python wrapper drives a compiled harness, not native application UI.
- File launch: `src/tests/file_action_path_tests.cpp` uses `NativePathTestParent` (new shared test support) and real child processes to verify arguments/current directory for ASCII/CJK/emoji/Czech and long paths. Long current-directory behavior depends on DOS alias availability and must be reported, not assumed.
- All requested categories exist across donor harnesses, but no single pre-existing manifest covers every category with distinct hashes and untouched sentinels. Reuse generators per operation. For UI tests, define a temporary owned root, record full UTF-16 and UTF-8 lengths and different payload/hash per identity, plus an unrelated sentinel; create only during the test, verify expected mutations and all untouched inputs, clean only verified root children. S00 creates no fixture tree.
- Path-string tests, generated production-code harnesses, native component binaries, actual paired DLL tests, and full GUI end-to-end tests are distinct evidence types. PictView/EXIF requires actual paired DLL validation; Undelete EFS stub is not real EFS import/export; archive command harness engines may be mocked. Network, filesystem unsupported flags/ADS/hardlinks must be explicit skipped/blocking evidence.

## Reproducible stage recipe (not executed during this audit)

Run only after the corresponding code/test stage is introduced in work/, never in donor/. In an initialized x64 MSVC shell, runner source's native compile form is:

`msbuild.exe src\tests\<name>_tests.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64 /p:OPENSAL_BUILD_DIR=<absolute-worktree-local-build-root>\ /p:PreferredToolArchitecture=x64 /p:RestoreFallbackFolders= /nr:false /v:minimal`

Run `<build-root>\tests\Debug_x64\<name>_tests.exe`; repeat Release. Use a new configuration-specific root. Apply outer command deadline less than 120 seconds; terminate child process tree before timeout. The complete runner has per-child timeouts, but its full many-test loop can exceed 120 seconds, so split into bounded invocations/tests rather than leave it running unbounded.

Native donor projects by scope: array_reserve (S23); branch_context_menu/branch_operations (S21, relevant operation subsets S20); branch_preparation/branch_properties/explorer_property_work (S28); branch_view (S25, performance extension S30); branch_view_text (S24 then resource S26); file_action_path/viewer_enumeration (S18); panel_temporary_selection (S20); worker_path/worker_copy_path (S19). Every source has matching *_tests.vcxproj in REUSE.tsv.

Generated real-C++ harness commands: `python -B src\tests\safe_file_paths_tests.py --configuration Debug` then Release (S01 minimal adapted subset, full S03); `python -B src\tests\renamer_io_tests.py` (S07); `python -B src\tests\checksum_selection_paths_tests.py` (S08); `python -B src\plugins\splitcbn\tests\selection_paths_tests.py` (S09). Only safe_file_paths shown here has its inspected --configuration flag. Other wrappers expose different interfaces; read their parser before adding arguments. Their generated source compiles production snippets into temporary executables, with mocks for selected UI/services; do not count them as just source assertions or as complete application smoke.

Run source contracts registered for the current stage with `python -B <script.py>` and retain all baseline contracts. Keep donor-only tests DEFERRED until dependencies land. Test evidence records exact target candidate SHA, source dirty/staged state if applicable, toolchain/configuration, command, exit result, output/runtime paths and hashes, fixture capability skips, actual GUI steps and expected/actual outputs. No donor's historical assertion count or performance percentage is a port result.
## Additional harness coupling found during final review

- disk_selection_snapshot_tests.py and cpp.in extract PictView `CEnumFiles` from thumbs.cpp and instantiate it in HostTests before the core snapshot cases. This is S12, so S05 needs a narrowly documented deferred PictView subset while retaining all owned SDK/ABI tests; restore and run it in S12. REUSE.tsv has separate S12 rows.
- branch_panel_getters_tests.py extracts `Entry`, `Entry::FullPath`, Join and whole getters from branch_view.cpp. For ordinary S04, adapt only the generator/source location and ordinary subset; keep Branch-state assertions DEFERRED until S26. Do not import scanner just to satisfy test extraction.
- S11 donor diff changes only salamatrix_packages.cpp, salamatrix_sides.h and its project. No new dedicated long-CJK JSON payload test is among the 231 donor files. Existing salamatrix_regression_tests.py is unchanged and source-contract coverage; ordinary SDK long-CJK extension tests are not proof of context serialization/payload limits. S11 must inspect reusable existing tests and add the minimal missing actual serialization/runtime acceptance coverage, with all five provider forwarding contracts validated.
- Host project registration includes `spl_diskselection.h` in S05. Its .filters diff only registers explorerpropertywork, panel_temporary_selection and branch_view_text; do not invent other donor filter hunks.
## S10 current integration boundaries (2026-10-22)

Automation production has no S11 dependency. Its donor selection_snapshot_tests
has a test-only coupling: SidesService::CopyDiskItemInfo and selected/focused
methods, wide SideItemJson and DiskSideContextJson, plus five provider forwarding
contracts. Keep these assertions in the template/explicit All scope and run in
S11; default Automation scope compiles actual item/collection/enumerator methods
and CUnknownImpl. TestAutomation is unchanged. Do not silently import Salamatrix
to satisfy extraction. S11 still needs its actual context serialization/payload
acceptance beyond these tests.

GUI discovered two unchanged releasebase defects outside the S10 diff:
scriptlist.cpp raw CreateFileW versus HANDLES(CloseHandle) registration in Debug;
and fileswn3.cpp CFilesWindow::ChangeDir file-focus branch char
shortenedPath[MAX_PATH] followed by strcpy(shortenedPath, copy). The latter
asserts for long full-file navigation through unchanged COM put_Path. Do not
claim this setter is repaired by owned COM properties. Final S10 GUI navigates
the parent directory then reads actual Items; native tests cover row destruction,
reordering and final ownership release. Initial asserted Debug process was
terminated and recorded separately, not relabeled normal shutdown. These remain
explicit follow-up defects; no host/diagnostic suppression included in S10.

## S11 closure of deferred consumer tests (2026-10-22)

S10 TestSides/TestDynamicJsonContext and all five forwarding contracts now run
in All scope. Native new coverage compiles actual PackageManager::CopyResult
and salamatrix_runtime_protocol.h, then independently parses emitted JSON.
All five providers retain response(65536); protocol whole-frame limit is 1MiB,
including framing overhead. Dynamic context exceeding either capacity is an
explicit failure, not a promise of unbounded transport. Native fixed ItemInfo
ABI remains unchanged and retains insufficient-buffer error for valid CJK names.
GUI is real Python only (D/R), never reported as five worker executions.
