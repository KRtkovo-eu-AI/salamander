# Shell extension copy/paste regression checklist

This checklist covers the Salamander `CLIPFAKE` copy/paste path used when files are
copied from an archive panel and pasted into a filesystem destination.

## Current IPC flow

1. The source Salamander creates a temporary `CLIPFAKE` directory and places a fake
   shell data object on the clipboard.
2. The source records the fake directory, its main-window handle/PID/TID, the
   `PastedDataID`, and status fields in `CSalShExtSharedMem`.
3. For Explorer or other shell targets, the shell copy hook sees the `CLIPFAKE`
   source, cancels the shell copy, and asks the source Salamander to paste via
   `salShExtDoPasteEvent` (Vista and later) or `WM_USER_SALSHEXT_PASTE` on older
   systems.
4. For Salamander panel targets, `CFilesWindow::ClipboardPaste` now recognizes the
   fake data object and performs the same shared-memory/event handshake directly.
   This avoids relying on the shell Drop implementation calling the copy hook and
   works across UAC integrity-level boundaries.
5. The source Salamander performs the archive extraction or filesystem copy using
   the target path supplied through shared memory.

No broker/helper process is used. Consequently, there is no additional executable
manifest and no helper that runs elevated. The IPC boundary is the existing named
mutex, shared memory, and manual-reset paste event created with Salamander's shared
security attributes.

## Regression scenarios

Run each case for Copy and Cut where the source supports move semantics.

- [ ] Non-elevated source Salamander archive panel -> elevated Salamander regular
      filesystem panel.
- [ ] Elevated source Salamander archive panel -> non-elevated Salamander regular
      filesystem panel.
- [ ] Non-elevated source Salamander archive panel -> non-elevated Salamander
      regular filesystem panel (same user, baseline).
- [ ] Elevated source Salamander archive panel -> elevated Salamander regular
      filesystem panel (same user, baseline).
- [ ] Source and target Salamander instances under different users. Verify both a
      mutually writable destination and a destination where ACLs correctly deny the
      source process.
- [ ] Archive panel source -> Explorer destination, to verify the copy hook still
      cancels the `CLIPFAKE` shell copy and starts extraction.
- [ ] Archive panel source -> Salamander panel while the source Salamander is busy;
      verify the target reports failure and the clipboard data remain usable.
- [ ] Regular filesystem panel source -> elevated and non-elevated Salamander
      regular filesystem panels. This path should continue to use normal shell
      clipboard formats and must not regress.
- [ ] Repeat paste from the same archive clipboard data into two different
      Salamander instances to validate `PostMsgIndex` stale-message handling.
