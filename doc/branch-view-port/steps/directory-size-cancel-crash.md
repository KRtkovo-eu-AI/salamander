# Ordinary-panel directory-size cancellation crash (2026-10-28)

## Confirmed cause

The preserved `salamand.exe.18908.dmp` from the private d909daa4 x64
Release build reports `0xC0000409`, subcode 2
(`FAST_FAIL_STACK_COOKIE_CHECK_FAILURE`). Exact matching PDBs locate the
failed cookie check in `CFilesWindow::BuildScriptDir`. The frame contains a
620-byte `text` array followed by the directory-read diagnostic overwriting
the cookie, saved registers and return address. Reading that array from the
dump recovers **857 bytes before NUL**, including a 783-byte UTF-8 path
(333 UTF-16 units), three 75-character Japanese directory components and
`ERROR_PATH_NOT_FOUND`. This is a concrete buffer overwrite, not a stack
exhaustion or Branch View lifetime failure. Cancellation/return exposes
corruption already caused when the error dialog was prepared.

## Fix and scope

`BuildScriptDir` now keeps path-bearing diagnostic/confirmation storage in
an existing heap-backed `CPathBuffer`, sized for two `SAL_MAX_PATH` inserts.
All seven affected formatting sites use the existing
`Salamander::LocalizedText::Write` with the actual capacity. No new
localization framework or resource text is introduced. A malformed resource
or insufficient capacity stops the operation, restores both mutable paths,
and avoids publishing partial text or a malformed confirmation. Failure
inside the active enumeration closes its search through `BUILD_ERROR`.
Skip, Skip All, Cancel and their flags retain their original meaning.
Windows settings, /GS, SDK interfaces and file-operation admission are unchanged.

This fixes memory safety and cancellation, **not** the legacy ordinary-panel
ANSI enumeration limits. A directory the existing enumeration cannot read
may still produce an error and be skipped. Unavailable directory sizes must
not be interpreted as a complete measurement of the tree.

## Verification

- `python -B src/tests/directory_size_cancel_tests.py`: Debug and Release PASS.
  The native harness extracts the actual production storage declaration and
  read-error/Skip/Cancel branch, compiles with `/GS /W4 /WX`, records dialog
  payloads, checks complete Unicode/long/UNC text, literal format directives,
  restoration of source/target paths, Skip All and malformed-resource refusal.
  Its resource cases read all five changed message resources and their 11
  shipped translations. The UI facade is explicit; it is not a GUI test.
- The same command with `--source-ref d909daa4 --configuration Debug` and
  separately `Release` fails with **0xC0000409** in both configurations.
  Those are expected negative controls, not passing original-code tests.
- x64 host and project dependencies built with `/warnaserror` in Debug and
  Release, in `build/directory-size-cancel-20261028`, using v145. No source
  safety/optimization flags were weakened. Unchanged Salmon runtime was
  reused from the matching S32 configuration; its sources have an empty diff.
- Actual Computer Use GUI, ordinary disk panels, separate RegFile configs:
  an owned tree has the same 105-character ASCII root length and three
  75-character Japanese components as the dump. Debug and Release display
  the long `ERROR_PATH_NOT_FOUND` message; **Esc returns to a responsive
  panel without a crash**. Screenshots and accessibility state are saved.
  Both configurations also calculate the independent ordinary two-file,
  12-byte fixture correctly. The shorter-root Unicode tree completes with
  one 18-byte file and four directories in Debug. These focused tests do
  not claim a new end-to-end long-path enumeration implementation.
- Existing `disk_file_size_paths_tests.py` and `localized_text_tests.py`:
  Debug and Release PASS. `branch_view_file_action_contract_tests.py`: PASS.
- Original dump, original executable/PDB and private production ZIP are
  retained. Test fixtures, raw debugger output and snapshots stay outside Git.
- Full platform CI / CodeQL and a clean full plugin-suite build were not run
  for this local focused host correction; no push or remote PR was made.

## Evidence and reproduction commands

Local evidence: `../evidence/directory-size-cancel-20261028/` relative to
this worktree; debugger logs `dump-stack`, `dump-locals`, `dump-text`, bounded
build/test JSON and stdout logs, `fixture.json`, GUI snapshots and artifact
hashes. Individual commands are bounded to at most 95 seconds and all
process sessions are collected. `artifact-manifest.json` binds binaries,
symbols and source hashes to the tested patch; `final-state.json` records
the eventual local commit. Original crash evidence remains in
`C:/Samandarin/personal-builds/CrashCapture-d909daa4`.

The new test is registered in `tools/run_native_tests.ps1`; passing
`--configuration Debug` or `Release` allows the normal CI native runner to
exercise the matching configuration. The regression deliberately tests
failure unwinding without changing disk data or running terminal commands
through the application UI.
