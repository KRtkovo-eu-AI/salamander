# Elevated file-operation model

Open Salamander remains an `asInvoker` process. File operations that fail with
`ERROR_ACCESS_DENIED`, `ERROR_PRIVILEGE_NOT_HELD`, or `ERROR_ELEVATION_REQUIRED`
are candidates for an elevated retry only when all of the following are true:

1. the current process is not already elevated;
2. the current account is a member of the local Administrators group;
3. the affected path canonicalizes under Windows, System32, Program Files,
   Program Files (x86), or ProgramW6432.

This deliberately excludes normal ACL/share failures in user data directories.
The central predicate is `CanOfferElevatedRetryForFileError(error, path)`. Once the user confirms the retry, `RunElevatedFileOperation()` starts `saladmin.exe` with `runas`, waits for completion, and returns the broker exit code to the caller.

## Operations in scope

The broker protocol is limited to file operations that Salamander already exposes:
copy file, move/rename file, delete file/directory, create directory, and set attributes.
It is intended for copy, move, delete, rename, attribute changes, and writes into
Program Files or system directories. Owner/group/DACL changes remain out of the
first executable broker version and must not be routed until a constrained
security-descriptor format is implemented.

## Broker

`src/saladmin` contains the elevated broker executable. Its first executable version implements copy-file, move-file, delete-file/delete-directory, create-dir, and set-attributes. Its manifest is
`requireAdministrator`; it should be launched only after the user chooses the UI
prompt "Retry as Administrator". The broker is not a shell and must never accept
an arbitrary command line to execute.

## IPC protocol

Version 1 requests contain:

- protocol version;
- fixed verb: `copy-file`, `move-file`, `delete-file`, `create-dir`,
  `set-attributes`;
- canonical source and/or target paths;
- operation flags and attributes specific to the verb;
- parent Salamander process ID for auditing/lifetime checks.

The broker must canonicalize every received path before execution, reject paths
that do not round-trip through Win32 canonicalization, and reject verbs outside
the allow-list. The main process owns all user interaction; the broker executes
only after UAC consent and the Salamander confirmation prompt.

## UI

The shared strings added for the prompt are `IDS_ELEVATEDRETRY_TITLE`,
`IDS_ELEVATEDRETRY_PROMPT`, and `IDS_ELEVATEDRETRY_BUTTON`.

## UAC prompt ownership and cancellation

All Salamander-owned `ShellExecuteEx` calls that can surface UAC UI should use
`SalShellExecuteEx`. The wrapper supplies a safe owner window when the caller did
not set `SHELLEXECUTEINFO::hwnd`, treats `ERROR_CANCELLED` as user cancellation,
and restores focus to the owner instead of reporting cancellation as a normal
operation failure. Plugin or setup code that cannot link to the main wrapper
should mirror the same behavior locally.

## Current integration points

The connected worker paths are:

- direct file deletion: `DeleteFile` failures can retry through `delete-file`;
- directory deletion: direct `RemoveDirectory` failures can retry through `delete-file`;
- directory creation: `CreateDirectory` failures can retry through `create-dir`;
- target creation during copy: target open/create failures can retry through `copy-file`;
- move/rename: normal move failures can retry through `move-file`;
- attribute changes: `SetFileAttributes` failures can retry through `set-attributes`.

Security descriptor changes still need dedicated integration so permission-copy semantics
and a constrained descriptor format are preserved.
