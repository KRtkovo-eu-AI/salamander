# S27a2 - File-manager launch comparison

Pozdější výsledek: oddělení programů a dokumentů bylo implementováno s plným
Windows ověřením a explicitně přijatými omezeními. Viz
[závěrečný protokol](S27a2-completion.md); tento dokument zachovává původní výzkum.

Date: 2026-10-23. Starting candidate: a2c81e00.
Research complete; a successful general long-executable Shell route on the tested
host has not been demonstrated. S27a2 remains PARTIAL. No production code or
Windows configuration changed.

## Evidence and scope

Actual launch source files from four open-source managers were inspected at the
pinned revisions below. Directory Opus developer statements supplement this
analysis; its current proprietary implementation was not inspected. No other
manager was installed or executed. Source inspection is not a runtime pass of
our fixture in those applications.

| Manager | Observed implementation | Applicability to S27a2 |
| --- | --- | --- |
| [FAR Manager](https://github.com/FarGroup/FarManager/blob/60567ba02403e90ac38b5a6953d746f198463f0b/far/execute.cpp#L592-L665) | Separates CreateProcess and ShellExecuteEx. Known programs can use direct creation before Shell. Shell can receive an explicit extension through SEE_MASK_CLASSNAME. | Direct launch is not a Shell fix. Explicit extension selection was a new supported candidate and was tested below; it did not fix the long path. |
| [Files](https://github.com/files-community/Files/blob/ee399b0d3e09084aa24d7261c9342d92bc89a5da/src/Files.App/Utils/Shell/LaunchHelper.cs#L65-L164) | LaunchHelper initially uses Process.Start with UseShellExecute=false; runas and other branches use Shell. Further fallbacks also exist. | A successful executable launch here need not be a successful Shell launch. Its PowerShell fallback and any transition after a security refusal are not imported. |
| [Double Commander](https://github.com/doublecmd/doublecmd/blob/d1d1b0d54dd4b9b0229aa06ea6b38265d376d43b/src/platform/uOSUtils.pas#L381-L439) | Its Windows ShellExecute path converts to UTF16 and calls ShellExecuteW. There are application-specific and missing-association branches. | This inspected call chain contains no general long-path fix for our executable; its wide Shell call corresponds to an already failing route. |
| [Explorer++](https://github.com/derceg/explorerplusplus/blob/b6dc6d5b193fd283b84ecfda1cf6935a49da6e8b/Explorer++/Helper/ShellHelper.cpp#L206-L234) | ExecuteFileAction uses ShellExecuteEx, with SEE_MASK_INVOKEIDLIST when supplied a PIDL. | Exact identity is useful but does not alone remove the limitation. Our previous PIDL/context-menu trials did not demonstrate successful long execution. |
| [Directory Opus, developer statement from April2024](https://resource.dopus.com/t/dopus-doesnt-handle-paths-longer-than-255-properly/48087/33) | Uses DOS/8.3 short paths for Shell where available; describes remaining limits. | We already tested this: an available alias identifies the same file, but is also rejected on this host. |

A [July2026 Directory Opus developer explanation](https://resource.dopus.com/t/dopus-exe-add-longpathaware-to-the-manifest/59885)
describes extended prefixes for its own file operations. That does not establish
that the prefix also works for Shell launching. A [January2014 statement by the
Total Commander author](https://www.ghisler.ch/board/viewtopic.php?p=278256)
explains its need for ShellExecuteEx for elevation. This is historical context,
not proof of its current implementation or compatibility.

FAR's [short-name retry logic](https://github.com/FarGroup/FarManager/blob/60567ba02403e90ac38b5a6953d746f198463f0b/far/execute.cpp#L742-L834) reacts to filename-length/directory
errors, rather than arbitrary security failures. Its known-executable direct
path is selected before Shell. This does not justify adding CreateProcess as a
retry after a Windows security refusal in Salamander.

## New actual runtime probe

The independent diagnostic added only SEE_MASK_CLASSNAME/lpClass=.exe to the
previous ordinary ShellExecuteExW probe. It used the existing DOS alias and
intended working directory. No NOZONECHECKS, registry changes or alternate launch
after refusal were used. This was an adaptation of our own probe inspired by the
FAR approach; no third-party production source was imported.

- MSVC x64 /W4 /WX diagnostic build: PASS.
- ASCII control: real child module/file identity and physical CWD verified.
- Original335-character UTF16 path with Unicode ancestors via its111-character
  alias: ShellExecuteExW=FALSE, GetLastError=4294967294, hInstApp=5; no child record.
- LongPathsEnabled remained0. Windows11 Pro build26200;
  shell32.dll10.0.26100.9549; urlmon/iertutil11.00.26100.8117.
- All16 original input SHA256 values unchanged. No host rebuild, Debug/Release
  GUI run, full clean solution build or CodeQL was performed for this research.

Evidence: ../evidence/S27a2-comparison, especially sources.json, files.json and
class-results.json. These observations do not prove identical behavior on every
Windows release or for every handler.

## Recommendation

1. Keep file I/O, archive access, document associations and executable launch
   distinct. Working long ZIP operations and explicitly configured viewer/editor
   dispatch must not be disabled because one Shell route rejects a path.
2. Preserve exact identity and normal Windows checks at the Shell boundary.
   A verified DOS alias is useful only where the receiving Shell accepts it.
   Do not automatically switch to another launch route after a security refusal.
3. Selecting a separate native-executable path, as FAR/Files do, is a possible
   direction for an independently reviewed design. It is not a ready substitute
   for this stage: security semantics, associations, elevation and CWD need an
   explicit contract. CreateProcessW alone does not demonstrate preservation of
   Shell attachment checks.
4. There is no basis to mark S27a2 complete under its current requirement for a
   successful long Shell launch with default Windows settings. Changing scope
   or accepting a documented limitation requires the user's decision; neither
   happened in this research step.

WinRT is not a general replacement for this executable scenario either:
[LaunchFileAsync documentation](https://learn.microsoft.com/en-us/uwp/api/windows.system.launcher.launchfileasync)
explicitly restricts code-bearing file types including.exe. Document activation
through that API does not prove arbitrary executable or long-CWD support.

## Pinned revisions and attribution

- FarGroup/FarManager: `60567ba02403e90ac38b5a6953d746f198463f0b`.
- doublecmd/doublecmd: `d1d1b0d54dd4b9b0229aa06ea6b38265d376d43b`.
- derceg/explorerplusplus: `b6dc6d5b193fd283b84ecfda1cf6935a49da6e8b`.
- files-community/Files: `ee399b0d3e09084aa24d7261c9342d92bc89a5da`.

Reference authors: Eugene Roshal/Far Group (BSD-3-Clause); Alexander Koblov and
Double Commander contributors (GPL-2.0-or-later); Explorer++ Project
(GPL-3.0-only); Files Community (MIT). No source from these projects was imported
into production. Downloaded source and original notices remain only in local
evidence outside Git. Retrieved-file hashes are in files.json.
