<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-20 | Record Branch View viewer integration and verification. | Preserve evidence for the five bundled viewers. |  | gpt-6.2-sol |
-->

# Additional viewers in Branch View

The starting revision is `f11b99f63`. The host admits the bundled MMViewer, JSON Viewer, WebView2 Render Viewer, IEViewer and UnISO registrations at the existing resolved-handler gate. F3 and View With pass the current row's full path, source UID and index. The public viewer SDK is unchanged.

MMViewer uses the wide viewer enumeration service with its existing association and selection policy. Older hosts can still use the legacy service. The renderer owns the full path. Legacy media readers open wide paths, and the WAV parser reads an already open Windows file handle through MMIO. JSON Viewer reads from a wide Windows file handle through `StreamReader`, preserving BOM detection. IEViewer keeps the complete Markdown filename across the viewer thread and reload. UnISO reads the selected disc image and its CCD sidecar using wide file I/O.

## Observed GUI cases

The local fixture contains `same.html`, `same.iso`, `same.json`, `same.md` and `same.wav` in each of five different directories. Files carry a distinct heading, property, volume label or duration. The path classes include separate ASCII parents, a Unicode parent, a 161-unit UTF-16 path encoded as 317 UTF-8 bytes and a 314-unit UTF-16 path encoded as 494 UTF-8 bytes.

| Viewer | Debug x64 | Release x64 |
| --- | --- | --- |
| MMViewer | F3 opened the 5-second WAV on the long path. Previous and Next navigated the actual WAV durations in both directions. | View With opened the 2-second `b/same.wav`. Previous reached 1 second. Next followed 2, 4, 5 and 3 seconds in name order and stopped at the end. After reordering from Size to Name, Next from the 2-second file reached the 4-second file. Turning off the source Branch View left the 5-second file open in both directions. F3 in the ordinary right panel opened the 1-second file, and Next stayed in that panel. |
| JSON Viewer | Associated F3 displayed `BRANCH VIEW FIXTURE 4` on the long path. The original 0.16 plugin had opened the short fixture through the candidate host but rejected the long path with a .NET Framework path-length error. | Explicit View With displayed `BRANCH VIEW FIXTURE 4` from the long path. |
| WebView2 Render Viewer | Explicit View With on `b/same.html` displayed `BRANCH VIEW FIXTURE 1`. A diagnostic host with the official plugin returned `0x80070057` for HTML in the long parent folder. | Associated F3 displayed `BRANCH VIEW FIXTURE 3` for HTML with a Unicode path longer than 260 UTF-8 bytes. |
| IEViewer | Explicit View With on the long Markdown path displayed `BRANCH VIEW FIXTURE 4`. | The same explicit route displayed the long-path Markdown heading. |
| UnISO | Explicit View With on `b/same.iso` displayed `BRANCH_FIXTURE_1` in the image information report. | Associated F3 on the long ISO path displayed `BRANCH_FIXTURE_4` in the image information report. |

Only MMViewer among these five viewers has panel file navigation. WebView2's existing virtual-host folder mapping is limited by the runtime to a Windows `MAX_PATH` folder. HTML in the 314-unit fixture is a known unsupported case for that mapping. [Microsoft's Win32 WebView2 API documentation](https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2_3#SetVirtualHostNameToFolderMapping)

The GUI coverage is sampled by viewer and configuration as recorded above. The full matrix across both entry points, both Path display settings, Unicode basenames, IEViewer non-Markdown long paths, other media formats, CCD sidecars and UnISO archive browsing remains unverified. The earlier PictView GUI run is documented in [the PictView verification note](pictview-navigation-verification.md).

## Builds and automated checks

The five plugin projects passed Debug and Release x64. Tests ran with locally staged binaries under the existing test build directory. The clean host compile hit the pre-existing Shell menu error in `shellsup.cpp`. The host compiled and linked in both configurations with the temporary Shell correction used by the PictView verification. Installer staging then failed because `7za.dll` was absent. The temporary Shell correction is outside the commit.

The Branch command contracts passed all 30 tests. The compiled panel getter and dispatch tests passed 414 checks in each configuration. The viewer enumeration tests passed 11 cases in each configuration, including exact duplicate identities, reorder and bounds behavior. A standalone .NET Framework probe compiled the production JSON path reader and verified all five content identities in Debug and Release.
