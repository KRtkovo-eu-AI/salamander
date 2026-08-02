# Codex rules for Open Salamander path/Unicode work

These rules apply to all future agentic development in this repository.

## Hard command timeout
- Every command must run with escalation.
- Never allow any command or approval wait to exceed 120 seconds.
- If a command returns a session/cell, terminate it before 120 seconds.
- Never leave a command running while waiting for output.
- If escalation approval hangs, cancel the tool call; do not wait or retry.

## Windows paths and buffer sizes

- Do not introduce new `MAX_PATH`-sized buffers for file names, directory names, command lines, viewer/editor arguments, or paths.
- Use `SAL_MAX_PATH` for Open Salamander path buffers. If a buffer may be larger than a few KB or is used in nested call paths, prefer heap-backed storage such as `CPathBuffer`, `CWidePathBuffer`, `std::string`, `std::wstring`, or `std::vector` instead of large stack arrays.
- When passing a mutable path buffer to helpers such as `SalGetFullName`, pass the real buffer capacity (usually `SAL_MAX_PATH` or `buffer.Capacity()`), never rely on the default `MAX_PATH` argument.
- Any feature that accepts or constructs a file path must be checked for:
  - long file names,
  - long directory names,
  - full Win32 long paths,
  - Unicode characters in file names,
  - Unicode characters in any directory component,
  - paths whose UTF-8 byte length is greater than `MAX_PATH` even when the UTF-16 character count is not.

## Unicode and text conversion

- Be explicit about encoding. Do not assume that `char*` path text is always ANSI/ACP.
- For paths or names that may come from panels, file listings, history, viewers, or plugins, prefer converting as UTF-8 first and fall back to ACP only when UTF-8 conversion fails.
- Preserve and use wide path data (`wchar_t`, `std::wstring`, `NameW`, `GetPathW()`) when it is already available. Do not round-trip through ACP if wide data exists.
- Use wide Win32 APIs (`CreateFileW`, `CreateProcessW`, `GetFileAttributesW`, etc.) when opening or launching paths that may contain Unicode or be longer than `MAX_PATH`.
- Add the Win32 extended-length prefix (`\\?\` or `\\?\UNC\...`) for wide paths that reach/exceed `MAX_PATH`, unless the path already has that prefix.
- Avoid truncating UTF-8 text by raw byte count. Truncation must not split a multi-byte sequence or surrogate pair, and must not introduce the replacement character `�`.
- When shortening text for display, logs, captions, menus, or error messages, trim at Unicode code point / grapheme-safe boundaries where possible. If that is not possible, prefer a well-tested helper over ad hoc `lstrcpyn`/`strncpy` on UTF-8 text.

## Viewer/editor/file-action specifics

- Internal viewer code must support long and Unicode paths end-to-end: panel selection, history, title/caption, file open, refresh/reload, previous/next file navigation, and error reporting.
- Editor and external viewer launch code must support long and Unicode paths through argument expansion, initial-directory expansion, command-line assembly, policy checks, and process creation.
- Be careful with scratch buffers inside expansion callbacks. A caller using `SAL_MAX_PATH` is not enough if a nested helper still copies into a `MAX_PATH` buffer.
- Do not place multiple `SAL_MAX_PATH`/`2 * SAL_MAX_PATH`/`4 * SAL_MAX_PATH` arrays on the stack in the same function. Use heap-backed buffers for large temporary data to avoid stack overflows.
- If a function keeps a current directory or file path only for UI convenience (for example an Open dialog initial directory), bound the copy by the destination capacity. If the path is too long, clear the optional UI field instead of overflowing or truncating unsafely.

## Testing expectations for path-related changes

For any change touching file paths, viewers, editors, file actions, command expansion, shell integration, or process/file Win32 APIs, test or reason through at least these cases:

1. A normal ASCII path under `MAX_PATH`.
2. A Unicode file name in an ASCII directory.
3. An ASCII file name in a Unicode directory.
4. Multiple Unicode directory components.
5. A path whose UTF-8 byte length exceeds `MAX_PATH`.
6. A Win32 long path whose UTF-16 length exceeds `MAX_PATH`.
7. External viewer/editor launch with the long/Unicode path in arguments and in the initial directory.
8. Internal viewer open, refresh, previous/next file, and title/caption rendering with long/Unicode paths.

When a Windows build/test environment is unavailable, document that limitation clearly and still run static checks that look for newly introduced `MAX_PATH` path buffers, ANSI-only Win32 APIs, unsafe copies, and nested `MAX_PATH` scratch buffers.

## Credits and attributions

- Add visible attribution to authors of the third party component, to the repository used as a reference point for implementation or fixing issues in our code or for taking an inspiration, in the doc/third_party.md file (keep the existing structure).
- Preserve all authors copyright/SPDX/provenance notices in files directly derived from other sources.
- Add comments or documentation where substantial code or tooling was copied or adapted from other sources.

## Darkmode

By Darkmode we always means the situation when user has set the Configuration - Colors - Scheme as Windows Dark Mode (experimental) scheme, not the operating system being in dark mode (if we means the system settings, we mention it specifically in the task prompt). All others color schemes are in light mode.

- Darkmode win32 winapi components and controls should use the win32-darkmodelib library included our project. USE_DARKMODELIB=1
- Darkmode windows and dialogs have to use dark window title bar, IMMERSIVEMODE and other undocumented Windows API calls.
- If creating any new window, any new dialog, showing new messagebox, creating new plugin etc., always don't forget to support the darkmode!

## Setup/Installer

By the Setup/Installer for Salamander we mean the Inno Setup script in doc/runbook-setup/inno_setup_salamander_x64.iss

## Localization and translate

All texts and strings has to be localized!
## SVG icons in CMenuPopup menus

When adding SVG icons to Salamander popup menus, use the same shared toolbar/menu image-list path that the Tab button bar context menu uses:

1. Add the SVG file to both `toolbars/` and `src/res/toolbars/`, and include it in the Inno Setup script (`doc/runbook-setup/inno_setup_salamander_x64.iss`) so installed builds receive it.
2. Do not hardcode newly added menu SVG artwork in `src/svg.cpp`; keep the SVG content in the files above and let `LoadToolbarSVG()` load it from the installed `toolbars` directory.
3. Reserve a stable `IDX_TB_*` image index in `src/consts.h`. Keep `IDX_TB_FD` after all statically reserved SVG indices so dynamically appended shell icons do not overlap them.
4. Add the SVG name and image index to `GetSVGIconsMainToolbar()` in `src/toolbar4.cpp`. Extra menu-only SVGs can be appended there without adding them as toolbar buttons.
5. For every `CMenuPopup` that owns icon-bearing items, including nested submenus, call `SetImageList(HGrayToolBarImageList)` and `SetHotImageList(HHotToolBarImageList)`.
6. Insert menu items with `MENU_MASK_IMAGEINDEX` and the reserved `IDX_TB_*` value. Do not create a private image list for these SVG menu icons unless there is a strong reason.
7. If the SVG has a non-16x16 viewBox, make sure the common SVG renderer scales and centers it into the requested icon bounds before rasterization.

## Plugin SDK binary compatibility

When changing any plug-in-facing SDK interface, preserve binary compatibility with plug-ins built against older headers. In particular, never insert, remove, reorder, or change the signature of existing `virtual` methods in `*Abstract` interfaces such as `CSalamanderGeneralAbstract`, `CSalamanderGUIAbstract`, `CSalamanderConnectAbstract`, `CPluginInterfaceAbstract`, or related shared headers under `src/plugins/shared/`. New virtual methods must be appended at the end of the relevant interface and mirrored in the concrete implementation class in the same appended order. Before committing SDK/interface changes, compare the virtual method order against the latest released Samandarin tag and verify that all pre-existing methods remain an unchanged prefix of the new interface layout. This is required because older binary plug-ins call methods by vtable slot, so even source-compatible insertions can crash existing plug-ins at runtime.

## Salamatrix runtime parity

- Treat the JavaScript, Python, PowerShell, PHP, and Lua runtime providers as one shared public capability surface. Their language syntax may be idiomatic, but supported operations, host wire methods, payload fields, defaults, return semantics, error handling, and lifecycle behavior must remain equivalent.
- Any runtime change that adds, removes, or modifies a function, option, payload, capability, or behavior must update all five runtime providers and worker facades in the same change: `src/plugins/javascriptruntime`, `src/plugins/pythonruntime`, `src/plugins/powershellruntime`, `src/plugins/phpruntime`, and `src/plugins/luaruntime`.
- Update the corresponding demos, manifests, documentation, source-contract checks, and isolated process-runtime tests together. Do not merge a runtime capability change while another runtime still exposes an older contract.

## Allocation failure handling and CodeQL

- Never silence CodeQL's "Incorrect allocation-error handling" warning by deleting an existing `NULL` check after a C++ `new`. In Open Salamander, that check usually documents intentional non-throwing, low-memory behavior; changing it to throwing allocation can terminate an operation or the application instead of following the established failure path.
- When an allocation is followed by a `NULL`/`LOW_MEMORY` branch, or the surrounding API is explicitly exception-free, use `new (std::nothrow)` and preserve the complete existing failure behavior. For required objects, return or otherwise stop before any later dereference; for optional UI helpers, retain the guarded degraded-mode path.
- Main Salamander and many plug-ins redefine `new` in `precomp.h` for CRT debug allocation tracking. A placement form such as `new (std::nothrow)` must use the established narrow workaround: conditionally `#undef new`, define a uniquely named local restore marker, perform only the nothrow allocation, then immediately restore `#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)` and undefine the marker. Do not disable the debug macro for a wider scope.
- Before applying an automated CodeQL/Copilot suggestion about allocation handling, inspect the constructor's ownership, all later dereferences, and the caller's low-memory contract. The correct fix must satisfy both CodeQL's throwing-`new` semantics and Salamander's existing runtime behavior; a clean alert alone is not sufficient.
