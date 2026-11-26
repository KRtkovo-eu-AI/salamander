<!--
| Date | Summary | Reason | Ticket | Model |
| 2026-11-26 | Document live source labels and remaining persistence work. | Record the implemented session behavior and closed-tab cases. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
| 2026-11-26 | Document the working directory history implementation. | Define the initial popup change and the storage and navigation work that follows. | https://github.com/KRtkovo-eu-AI/salamander/issues/867 | GPT-6 |
-->

# Working directory history implementation plan

[Issue #867](https://github.com/KRtkovo-eu-AI/salamander/issues/867) covers the List of Working Directories opened by Alt+F12. The initial implementation adds a Hot Path star and a different-tab icon. The source is the `release/5.0-samandarin-0.17` branch. The project-tab integration depends on [issue #853](https://github.com/KRtkovo-eu-AI/salamander/issues/853).

## Existing behavior and integration points

| Area | Existing behavior and implications |
|---|---|
| Recording | `CMainWindow::DirHistoryAddPathUnique` records in both the shared and the originating panel's lists. Keep the existing conditions that decide when a location was worked in, including the disk, archive and plug-in filesystem callers. Refresh, fallback navigation and startup can take different paths through the recording code. |
| Deduplication | `CPathHistory::AddPathUnique` promotes an existing path to the most recent position. It can defer an insertion while `Execute` is navigating. Provenance must travel through both ordinary and deferred insertion, and a revisit must update the winning source even if the row is already first. Copies used for tab lifecycle and scope changes must carry any available provenance. |
| Popup routing | The keyboard shortcut, Commands menu, toolbar command and directory-line drop-down converge on `CFilesWindow::OpenDirHistory`. Change Drive also calls `FillHistoryPopupMenu` for its separate, capped recent-locations section. Back/Forward use `CPathHistory` as well. Keep their existing rendering and navigation behavior. |
| Tab identity | `CFilesWindow::GetPanelTabId()` identifies a tab within the running process. Reorder and moves keep the panel object. A new panel gets a new ID, including when undoing a close. A persisted ID needs a separate lifetime because the runtime counter starts over on restart and existing callers use it to identify live panels and async work. |
| Configuration | **History** already owns the shared/per-tab preference. Switching to per-tab can seed empty lists with copies of shared history. Switching to shared rebuilds from open panels and detached tabs in enumeration order, which can discard older shared entries or misorder visits across tabs. The registry currently saves or clears the shared list according to the selected scope. Showing the same preference under **Tabs** does not on its own fix any of these storage behaviors. |

## Phase 1 — Icons and live source tab names in the working directory popup

1. Record the runtime tab ID alongside new working directory entries. Treat zero and entries loaded from legacy or existing configuration as unknown. Keep attribution through duplicate promotion, deferred inserts and copies. Preserve the existing path, icon and plug-in ownership rules in `CPathHistory`.
2. At popup construction, compare each location to the current Hot Paths. Reuse the existing lexical path comparison for the relevant path type and resolve supported Hot Path variables without displaying expansion errors. Do not open an archive, reconnect a plug-in, probe a network path or resolve a filesystem link merely to draw the menu. A bookmark edit should be reflected the next time the list opens. Hidden Hot Paths still qualify. Plug-in bookmarks match their stored filesystem name and path text, so a plug-in that uses different external and internal names can leave an alias unmarked.
3. Within this popup, use the Hot Path star first. Otherwise, in the shared view, use the tab icon if the recorded source is known and differs from the panel opening the popup. Keep the current path icon or empty icon slot for other rows. This keeps one icon at most and prevents copied entries in the per-tab view from looking like cross-tab search results.
4. Use the existing popup, icon column and keyboard handling. Reuse the shipped Favorites star and Duplicate Tab artwork through the panel's DPI-specific toolbar image lists. Both assets already ship in the light and dark toolbar sets. Keep the row's command ID associated with the original history entry.

5. Show the undecorated live tab title as `[Tab name]` before a different tab's path, including rows where the star wins. Escape literal ampersands and apply the prefix after assigning path accelerators. Use an ellipsis for unusually long names to limit menu growth.

Phase 1 uses session attribution. A known source can still differ from the active tab after its panel closes, so its icon remains available, while its name requires the tab to be open. Legacy entries with unknown origin show the path alone. Persisted attribution, names for closed or restored tabs, the **Tabs** settings control, lock/project indicators and new navigation actions are follow-up work.

## Phase 2 — Persistent attribution, history scope and tab names

1. Define a persistent tab identity for regular and detached tabs without replacing the runtime identifier. Restore it only for the same logical tab and handle missing, invalid and duplicate IDs. Decide whether Undo Close Tab reinstates the same identity. An intentional duplicate gets a new identity. Inherited history can retain its original provenance without turning the copied entries into new visits.
2. Store provenance as optional metadata so older path-only history is still readable. Apply **Save List of Working Directories**, **Clear History** and configuration import/export rules to the metadata and any stored tab names. Check the shared list, per-tab lists, detached-tab state and both newly created and older configurations. Retain closed-tab visits in the shared list within its existing history size limit.
3. Preserve chronological shared and per-tab histories when changing the preference. The shared list should have one row per path with the latest source, while each visiting tab should retain its own entry. Keep legacy visits of unknown origin in the shared list. Use the existing destination identity rules for disk paths, archive member paths and plug-in filesystem locations. Avoid implicitly merging lists in panel enumeration order.
4. Expose the same persisted preference in **Tabs**, keeping the saved value and the shared default. If the **History** control is retained, synchronize both controls when the user moves between pages and apply the scope change once. Keep the independent save control and document the two entry points in Help.
5. Decide whether a closed tab keeps a last-known name. For restored tabs, reuse the live title once the persistent identity resolves. Keep source labels independent of icon priority, and preserve the existing path accelerators and Unicode handling.

## Phase 3 — Navigation status

- Add the lock icon using the same decision as the active panel's unlock confirmation. Include disk, archive and plug-in navigation, canceled confirmation, and any same-path or refresh cases that bypass the prompt. Menu construction must not show a dialog or change the tab state.
- Integrate the project boundary when #853 defines the navigation rule. Use that rule for both the icon and the action. Treat the root and its descendants according to the same path-component semantics as navigation. Cover similarly named siblings, different drives, UNC roots, and reparse points or virtual paths according to the project-tab rules.
- Use the final priority order of project boundary, lock, Hot Path, different tab and ordinary path icon. Make the chosen status available to assistive technology without relying on color. Opening a row continues to target the panel that opened the popup. Opening a history destination in a new tab remains a separate proposal.

## Verification

| Coverage | Expected result |
|---|---|
| First-phase popup | A Hot Path from another tab gets the star. An unbookmarked visit from another tab gets the tab icon in shared history. Local and unknown entries retain their previous appearance unless they match a Hot Path. Editing a Hot Path is reflected on the next opening. |
| Recency and lifecycle | Visit the same destination in two tabs, including when its row is already newest. Verify attribution after using the popup to navigate, reordering or moving a tab, detaching it, closing it, and duplicating or reopening it. Compare the same-tab and other-tab views without changing which panel receives navigation. |
| Entry points and presentation | Compare Alt+F12, Commands, the toolbar and the directory-line drop-down. Check light and dark schemes, selected and unselected icons, DPI scaling, keyboard accelerators and the separate Change Drive and Back/Forward menus. |
| Paths | Check literal and variable-expanded bookmarks, Unicode components, paths exceeding `MAX_PATH` in UTF-8 or UTF-16, case/trailing separators, an unavailable UNC destination, archive members and plug-in filesystem entries. Opening the popup should make no filesystem or plug-in call just to determine an icon. |
| Persistence and later UI | Check upgrades, restart with saving on and off, Clear History, switching scopes in both directions, and restored/detached tabs. Verify a name prefix remains visible with a higher-priority icon, and check confirmation, cancellation and project-root handling when those phases land. |

Use the existing Windows build and noninteractive tests first. Record manual UI checks separately from build and automated results.
