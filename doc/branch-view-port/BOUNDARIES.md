# S00 read-only donor boundary audit

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
