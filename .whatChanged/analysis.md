# Tabbed Panel Refactor Deep Analysis

## Scope
This review covers all commits from `db63b6d6be6be463e36a20de7ab54cafde275375` to the current `HEAD`, focusing on behavioural changes that persist even when tabbed panels are disabled. The intent is to document architectural shifts, compatibility safeguards, and any latent regression risks.

## Architectural additions
- `CMainWindow` now owns side-specific tab arrays (`LeftPanelTabs` / `RightPanelTabs`) alongside the traditional `LeftPanel` / `RightPanel` pointers, plus tab window handles and drag state used by the UI layer.【F:src/mainwnd.h†L386-L408】【F:src/mainwnd1.cpp†L285-L330】
- `CFilesWindow` instances track their logical side and optional tab customisations (colour & prefix). Constructors initialise these fields so existing panel logic receives a valid `PanelSide` even without tabs, and status/directory bars mirror the correct side when reassigned.【F:src/fileswnd.h†L764-L777】【F:src/fileswn1.cpp†L1312-L1367】【F:src/fileswn9.cpp†L57-L90】
- Helper accessors (`GetPanelTabs`, `GetPanelTabWindow`, `GetPanelTabIndex`) encapsulate the new arrays, keeping most call sites agnostic to the tab implementation.【F:src/mainwnd3.cpp†L91-L121】【F:src/mainwnd3.cpp†L364-L398】

## Panel lifecycle & activation
- `AddPanelTab` now wraps panel allocation, inserts it into the side array, mirrors the tab UI (when present), and immediately routes through `SwitchPanelTab`, so even the first “default” panel benefits from the same activation pipeline used by real tabs.【F:src/mainwnd3.cpp†L91-L121】
- `SwitchPanelTab` sets the side’s active pointer, synchronises the tab control selection, refreshes directory history, reattaches change notifications via `EnsureWatching`, optionally reloads disk paths that lost monitoring, and posts a deferred refresh tick. Hidden panels are flagged for refresh before being hidden, preserving stale-data safeguards when they come back.【F:src/mainwnd3.cpp†L123-L207】【F:src/mainwnd3.cpp†L435-L450】【F:src/snooper.cpp†L999-L1039】
- `UpdatePanelTabVisibility` shows only the active panel window and hides the tab strip unless both the feature is enabled and tabs exist, ensuring the legacy single-panel presentation remains intact when the feature is disabled.【F:src/mainwnd3.cpp†L435-L456】

## Configuration persistence & toggling
- Registry loading now understands per-side tab counts, but forcibly collapses to a single panel when `UsePanelTabs` is off, keeping legacy configurations compatible. Failed tab creations roll back array/tab-control state before deleting the panel to avoid leaks or dangling entries.【F:src/mainwnd2.cpp†L2667-L2768】
- Toggling the feature at runtime prunes extra tabs, falls back to the default tab per side, and refreshes layout/command states so menus and accelerators no longer expose tab actions when disabled.【F:src/mainwnd3.cpp†L1528-L1559】

## Command routing & backwards compatibility
- All tab management commands short-circuit when `UsePanelTabs` is false, so the classic command set behaves exactly as before. This includes creation, closure, cycling, cross-panel moves, and context menus invoked via command IDs such as `CM_LEFT_NEWTAB` or `CM_NEXTTAB`.【F:src/mainwnd3.cpp†L482-L1202】
- The duplicate-tab command now routes through the same helper for active, left, and right panels, letting the toolbar button and all menu entries duplicate in-place while respecting the focused side.【F:src/mainwnd3.cpp†L4820-L4865】【F:src/menu4.cpp†L229-L233】
- Layout calculations treat tab rows as having zero height unless both the feature and at least one tab are active on that side, preventing empty padding in non-tabbed mode.【F:src/mainwnd3.cpp†L7174-L7188】
- Panel swapping (`CM_SWAPPANELS`) now swaps entire tab collections and reassigns panel sides, ensuring that legacy swap behaviour persists while multiple tabs simply travel with their respective side. The active panel pointer is corrected afterwards to keep keyboard focus deterministic.【F:src/mainwnd3.cpp†L5882-L5950】

## Directory monitoring & refresh safety
- The change-notification subsystem now keeps a registry of watched paths keyed by normalised identifiers, allowing multiple tabs to share a watcher and reattach when a tab is reactivated. `EnsureWatching` reuses this mechanism, so temporarily hidden tabs still regain monitoring before a refresh, avoiding stale listings when switching back.【F:src/snooper.cpp†L960-L1039】
- Because `SwitchPanelTab` always funnels tab changes (including the implicit single tab), legacy single-panel users inherit the new watcher resilience with no behavioural toggles required.【F:src/mainwnd3.cpp†L123-L207】

## Plugin lifecycle protections
- Plug-in unload checks now iterate every tab on each side (skipping the active panel already checked) to confirm no hidden panel still depends on the plug-in before unloading. This prevents regressions where a background tab hosting a plug-in filesystem could lose its backing provider even if tabs are hidden from the UI.【F:src/mainwnd4.cpp†L361-L414】

## Identified risks & mitigation notes
- Tab arrays are always populated (even in non-tab mode), so any new code must continue to honour the `UsePanelTabs` guards when iterating them. Existing guard clauses cover command handlers, layout, and context menus; regression risk is therefore low but future changes should reuse the helper accessors and check `Configuration.UsePanelTabs` when exposing tab-only UI.【F:src/mainwnd3.cpp†L482-L1202】【F:src/mainwnd3.cpp†L7174-L7188】
- Configuration load paths create (and destroy on failure) hidden tabs even when disabled; early exits plus rollback logic keep state consistent, but installer/upgrader tests should still cover malformed registry data to ensure the loop that recreates tabs cannot starve or leak when repeated failures occur.【F:src/mainwnd2.cpp†L2696-L2768】
- Directory watcher sharing relies on accurate path normalisation. Any future change that bypasses `SwitchPanelTab` when swapping panels would risk leaving `WatchEntriesByPanel` out of sync; developers should continue to route panel activation through `SwitchPanelTab` to maintain consistency.【F:src/mainwnd3.cpp†L123-L207】【F:src/snooper.cpp†L999-L1039】

## Summary
The refactor introduces side-specific tab collections and richer panel metadata while wrapping legacy operations in guard rails that keep the single-tab experience intact. Activation, configuration, and plug-in management paths were updated to treat even the default panel as a tab entry, which centralises lifecycle logic and reduces divergence between tabbed and non-tabbed modes. The remaining regression risks centre on respecting the existing `UsePanelTabs` checks and keeping tab activation funnelled through the provided helpers.
