# Elevated file-operation model

Open Salamander remains an `asInvoker` process. File operations that fail with
`ERROR_ACCESS_DENIED`, `ERROR_PRIVILEGE_NOT_HELD`, or `ERROR_ELEVATION_REQUIRED`
are candidates for an elevated retry only when all of the following are true:

1. the current process is not already elevated;
2. the current account is a member of the local Administrators group;
3. the affected path canonicalizes under Windows, System32, Program Files,
   Program Files (x86), or ProgramW6432.

This deliberately excludes normal ACL/share failures in user data directories.
The central predicate is `CanOfferElevatedRetryForFileError(error, path)`.

## Operations in scope

The broker protocol is limited to file operations that Salamander already exposes:
copy file, move/rename file, delete file, create directory, set attributes, and
set owner/group/DACL. It is intended for copy, move, delete, rename, attribute
changes, owner/ACL changes, and writes into Program Files or system directories.

## Broker

`src/saladmin` contains the elevated broker executable. Its manifest is
`requireAdministrator`; it should be launched only after the user chooses the UI
prompt "Retry as Administrator". The broker is not a shell and must never accept
an arbitrary command line to execute.

## IPC protocol

Version 1 requests contain:

- protocol version;
- fixed verb: `copy-file`, `move-file`, `delete-file`, `create-dir`,
  `set-attributes`, or `set-security`;
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
