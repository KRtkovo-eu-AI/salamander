<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-26 | Document opening a path in the original tab. | Add a secondary action to highlighted rows with a live source tab. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document favorite badges on icons from other tabs. | Let the icon show both the tab origin and the favorite status. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Remove screen-reader support from the Alt+F12 plan. | Keep the current feature scope focused on history and visual indicators. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document independent shared and per-tab histories. | Preserve closed-tab entries and visit order when switching scope or restarting. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Allow history rows without an icon. | Keep the icon column focused on paths with an existing icon or a distinct status. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Drop the per-row lock icon from the current scope. | The active tab already indicates its lock state. Repeating it would obscure the destination icons. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Keep the history scope setting on the History page. | The existing control already selects the list opened by Alt+F12. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document optional icon colors in working directory history. | Distinguish live tab colors, closed-tab snapshots, and neutral icons. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document persistent source identities and closed-tab captions. | Describe restart, compatibility, and history-clearing behavior. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document live source labels and remaining persistence work. | Record the implemented session behavior and closed-tab cases. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document the working directory history implementation. | Define the initial popup change and the storage and navigation work that follows. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
-->

# Working directory history implementation plan

[Issue #867](https://github.com/KRtkovo-eu-AI/salamander/issues/867) covers the List of Working Directories opened by Alt+F12. The initial implementation adds a Hot Path star and a different-tab icon. The source is the `release/5.0-samandarin-0.17` branch. The project-tab integration depends on [issue #853](https://github.com/KRtkovo-eu-AI/salamander/issues/853).

## Existing behavior and integration points

| Area | Existing behavior and implications |
|---|---|
| Recording | `CMainWindow::DirHistoryAddPathUnique` records in both the shared and the originating panel's lists. Keep the existing conditions that decide when a location was worked in, including the disk, archive and plug-in filesystem callers. Refresh, fallback navigation and startup can take different paths through the recording code. |
| Deduplication | `CPathHistory::AddPathUnique` promotes an existing path to the most recent position. It can defer an insertion while `Execute` is navigating. Provenance must travel through both ordinary and deferred insertion, and a revisit must update the winning source even if the row is already first. Copies used for tab lifecycle and legacy recovery must carry any available provenance. |
| Popup routing | The keyboard shortcut, Commands menu, toolbar command and directory-line drop-down converge on `CFilesWindow::OpenDirHistory`. Change Drive also calls `FillHistoryPopupMenu` for its separate, capped recent-locations section. Back/Forward use `CPathHistory` as well. Keep their existing rendering and navigation behavior. |
| Tab identity | The runtime tab ID identifies live panels and asynchronous work. The history GUID identifies the same logical tab across a restart and is stored with each regular or detached tab. |
| Configuration | **History** already selects the scope for Alt+F12. **Share history between panels/tabs** uses one list across both panels and their tabs. **Keep history per panel/tab** uses the active tab's list. The scope and the option to save the working directory list are separate settings. |

## Phase 1 — Icons and live source tab names in the working directory popup

1. Record the originating tab alongside new working directory entries. Carry attribution through duplicate promotion, deferred inserts and copies. Legacy entries without source metadata display the path alone.
2. At popup construction, compare each location to the current Hot Paths. Reuse the existing lexical path comparison for the relevant path type and resolve supported Hot Path variables without displaying expansion errors. Do not open an archive, reconnect a plug-in, probe a network path or resolve a filesystem link merely to draw the menu. A bookmark edit should be reflected the next time the list opens. Hidden Hot Paths still qualify. Plug-in bookmarks match their stored filesystem name and path text, so a plug-in that uses different external and internal names can leave an alias unmarked.
3. Use a standalone Hot Path star in the current tab's list or when a shared entry identifies the current tab as its source. In shared history, a known source from another tab uses the other-tab icon. Add a small star in the upper-right corner of that icon when the path is also a Hot Path. Keep the existing path icon or empty slot for other entries, including shared entries with unknown origins.
4. Use the existing popup, icon column and keyboard handling. Reuse the shipped Favorites star and Duplicate Tab artwork through the panel's DPI-specific toolbar image lists. Draw the small star as an overlay within the same icon slot. Keep the row's command ID associated with the original history entry.

5. Show the undecorated live tab title as `[Tab name]` before a different tab's path, including rows with the small star. Escape literal ampersands and apply the prefix after assigning path accelerators. Use an ellipsis for unusually long names to limit menu growth.

## Phase 2 — Persistent attribution and closed-tab names

1. Store a history GUID for each regular and detached tab. Reordering, moving and detaching a tab carry the same GUID. Duplicating a tab starts a new identity and inherited visits keep their original sources. Undo Close Tab restores the identity of the reopened tab. Missing or invalid tab IDs receive fresh identities, and duplicate restored IDs are resolved so a visit has at most one live source.
2. Keep the existing numbered paths. Store the source GUID and caption as optional metadata associated with their individual working directory entries. Legacy paths and entries with unreadable metadata stay navigable. The existing **Save List of Working Directories** and **Clear History** controls also govern the source metadata.
3. Resolve a known source against the current tabs when opening the popup. Use the live, undecorated name if it is available. When the source closes, retain its last known name as a snapshot for its recorded visits. Save that snapshot with history so a closed tab can still be named after restart. A restored tab resumes supplying its live name.

## History icon colors

**Configuration → History → Use colored icons in the working directory list** is enabled by default. Both the standalone star and the small star are yellow. The other-tab icon uses the source tab's custom color when one is set. Disabling this option uses the neutral palette and preserves the icon shapes and the small star.

Resolve the current custom color of a live source, including a cleared color. When a tab closes, keep its last defined color alongside its caption as optional history metadata. Existing saved rows with no color metadata still resolve the color of their open source or use the neutral icon.

## Preserving history when switching scopes

Each recorded visit updates the shared list and the originating tab's list. Changing the scope selects which list Alt+F12 opens and refreshes the directory-line and command states. The shared list preserves visits from closed tabs and keeps one row per destination, using its latest visit for both its position and source. An empty per-tab list stays empty on a scope change.

When **Save List of Working Directories** is enabled, save both lists regardless of the selected scope. A legacy per-tab profile without a saved shared list can recover entries from its saved open and detached tabs. Those lists only record order within each tab, so the original cross-tab order is unavailable. An explicitly saved empty shared list skips this recovery.

## Open a path in its original tab

In the shared list, show **Open in original tab** at the right of the highlighted row when it has a different, open source tab. Click the label or press Ctrl+Enter to activate the source and open the recorded location there. Clicking the path or pressing Enter opens the location in the active tab. A closed source still supplies its saved name and color to the row.

Capture the selected destination before the popup tracks input and look up the source again by its stable ID just before opening. The action follows the source across panels, window detachment and restart. Its navigation uses the existing history rules and asks for confirmation before changing the path of a locked source tab.

## Follow-up — Project boundary icon

- Integrate the project boundary when #853 defines the navigation rule. Use that rule for both the icon and the action. Treat the root and its descendants according to the same path-component semantics as navigation. Cover similarly named siblings, different drives, UNC roots, and reparse points or virtual paths according to the project-tab rules.
- When available, the project boundary icon takes precedence over the history icon, including an other-tab icon with a small star. Opening a row continues to target the panel that opened the popup. Opening a history destination in a new tab remains a separate proposal.

## Verification

| Coverage | Expected result |
|---|---|
| Popup icons | A Hot Path from the current tab gets a standalone star. A Hot Path from another tab gets the tab icon with a small star and keeps its source prefix. A non-favorite visit from another tab gets the tab icon alone. Verify the small star with colors enabled and disabled and on an uncolored source tab. Editing a Hot Path is reflected on the next opening. |
| Recency and lifecycle | Visit the same destination in two tabs, including when its row is already newest. Verify attribution after using the popup to navigate, reordering or moving a tab, detaching it, closing it, and duplicating or reopening it. Compare the same-tab and other-tab views without changing which panel receives navigation. |
| Entry points and presentation | Compare Alt+F12, Commands, the toolbar and the directory-line drop-down. Check light and dark schemes, selected and unselected icons, DPI scaling, keyboard accelerators and the separate Change Drive and Back/Forward menus. |
| Open in original tab | Open the same recorded location with the right-side action and with Enter. Verify the target tab, source title and exact path. Exercise Ctrl+Enter, closed and unknown sources, moves between panels, detached windows and unlock cancellation. |
| Paths | Check literal and variable-expanded bookmarks, Unicode components, paths exceeding `MAX_PATH` in UTF-8 or UTF-16, case/trailing separators, an unavailable UNC destination, archive members and plug-in filesystem entries. Opening the popup should make no filesystem or plug-in call just to determine an icon. |
| Persistence | Save visits from regular and detached tabs, restart, and confirm their source identities resolve to the restored tabs. Rename a source after recording a visit, then close it and confirm the last known name survives both close and restart. Check Unicode, path-only legacy records, invalid metadata, saving disabled and Clear History. |
| Switching scopes | Interleave visits across tabs and visit the same destination in two tabs. Close a source tab, switch to per-tab history, and return to shared history. Confirm the closed tab's visit remains in place and the duplicate keeps its newest source. Repeat after a restart in per-tab mode with history saving enabled. |
| Project tabs | When project tabs are available, check project-root handling and verify that the name prefix remains visible with the project boundary icon. |

Use the existing Windows build and noninteractive tests first. Record manual UI checks separately from build and automated results.
