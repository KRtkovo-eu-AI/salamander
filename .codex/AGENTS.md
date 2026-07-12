# Codex rules for Open Salamander path/Unicode work

These rules apply to all future agentic development in this repository.

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
