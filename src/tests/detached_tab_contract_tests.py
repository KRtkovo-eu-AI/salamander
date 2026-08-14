# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def require_before(text: str, first: str, second: str, description: str) -> None:
    first_pos = text.find(first)
    second_pos = text.find(second)
    if first_pos < 0 or second_pos < 0 or first_pos >= second_pos:
        raise AssertionError(f"Invalid ordering for {description}")


def main() -> None:
    mainwnd1 = (ROOT / "src/mainwnd1.cpp").read_text(encoding="utf-8")
    mainwnd2 = (ROOT / "src/mainwnd2.cpp").read_text(encoding="utf-8")
    mainwnd3 = (ROOT / "src/mainwnd3.cpp").read_text(encoding="utf-8")
    mainwnd4 = (ROOT / "src/mainwnd4.cpp").read_text(encoding="utf-8")
    mainwnd_h = (ROOT / "src/mainwnd.h").read_text(encoding="utf-8")
    zip_cpp = (ROOT / "src/zip.cpp").read_text(encoding="utf-8")
    plugins4 = (ROOT / "src/plugins4.cpp").read_text(encoding="utf-8")
    plugins2 = (ROOT / "src/plugins2.cpp").read_text(encoding="utf-8")
    salamatrix_packages = (ROOT / "src/plugins/salamatrix/salamatrix_packages.cpp").read_text(encoding="utf-8")
    configstorage = (ROOT / "src/configstorage.cpp").read_text(encoding="utf-8")
    drivelst = (ROOT / "src/drivelst.cpp").read_text(encoding="utf-8")
    stswnd = (ROOT / "src/stswnd.cpp").read_text(encoding="utf-8")
    tabwnd = (ROOT / "src/tabwnd.cpp").read_text(encoding="utf-8")
    toolbar1 = (ROOT / "src/toolbar1.cpp").read_text(encoding="utf-8")
    menubar = (ROOT / "src/menubar.cpp").read_text(encoding="utf-8")
    filesbx1 = (ROOT / "src/filesbx1.cpp").read_text(encoding="utf-8")
    filesbx2 = (ROOT / "src/filesbx2.cpp").read_text(encoding="utf-8")
    fileswnb = (ROOT / "src/fileswnb.cpp").read_text(encoding="utf-8")

    require(mainwnd1, "index <= 0 || panel->IsTabLocked()",
            "default/locked tab detach guard")
    require(mainwnd3, "appendMenuItem(CM_DETACHTAB, IDS_MENU_DETACH_TAB",
            "tab context-menu detach command")
    require(mainwnd3, "return DetachPanelTab(panel, &screenPt) != FALSE",
            "drag completion outside Salamander")
    require(tabwnd, "if (GetTabCount() <= 1)",
            "drag support when only default plus one detachable tab exist")

    require(mainwnd1, "ReattachDetachedTab(panel, cpsLeft)", "left reattach choice")
    require(mainwnd1, "ReattachDetachedTab(panel, cpsRight)", "right reattach choice")
    require(mainwnd1, "MSGBOXEX_YESNOOKCANCEL", "three actions plus safe cancel")
    require(mainwnd1, "DarkModeApplyWindow(hWnd)", "dark detached top-level window")
    require(mainwnd1, "if (uMsg == WM_ERASEBKGND)",
            "dark startup background before detached window user data is assigned")
    require(mainwnd1, "SetDCBrushColor(hDC, DarkModeGetColors().background)",
            "configured dark-scheme detached window startup background")
    require(filesbx1, "if (DarkModeIsWindowsDarkSchemeSelected())",
            "items-box dark startup background")
    require(filesbx1, "SetDCBrushColor(hDC, DarkModeGetColors().background)",
            "configured dark-scheme items-box startup background")
    require(mainwnd1, "DarkModeAllowDarkScrollbars(hWnd);",
            "detached top-level host dark scrollbar opt-in")
    if mainwnd1.count("DarkModeAllowDarkScrollbars(hWnd);") < 2:
        raise AssertionError("Both classic detached and detached-tab hosts need dark scrollbars")
    require(mainwnd1, "DarkModeApplyTree(hDetachedWindow);",
            "dark theme reapplication after panel reparenting")
    require(fileswnb, "if (DarkModeIsWindowsDarkSchemeSelected())",
            "panel gap dark startup background in every host")
    require(filesbx2, "FillRectWithColor((HDC)wParam, &r, DarkModeGetColors().background);",
            "header and bottom-bar dark startup backgrounds in every host")
    if filesbx2.count("FillRectWithColor((HDC)wParam, &r, DarkModeGetColors().background);") < 2:
        raise AssertionError("Both header and bottom bar need a dark initial erase")
    require(stswnd, "SetDCBrushColor(dc, DarkModeGetColors().background)",
            "information-line dark startup background in every host")
    require(mainwnd1, "DarkModeApplyTree(HRightDetachedWindow);",
            "classic detached child dark theme after reparenting")
    require(mainwnd3, "DarkModeAllowDarkScrollbars(HWindow);",
            "main-window dark scrollbar opt-in during creation")
    require(mainwnd3, "FillRect(dc, &clientRect, (HBRUSH)GetStockObject(DC_BRUSH));",
            "main-window dark initial erase")
    require(toolbar1, "if (DarkModeIsWindowsDarkSchemeSelected())",
            "toolbar dark erase before the Vista anti-flicker early return")
    require_before(
        toolbar1,
        "if (DarkModeIsWindowsDarkSchemeSelected())",
        "if (WindowsVistaAndLater) // pod vistou blika rebar",
        "dark toolbar background before suppressing erase",
    )
    require_before(filesbx1, "SetScrollInfo(HHScrollBar, SB_CTL, &si, TRUE);",
                   "ShowWindow(HHScrollBar, SW_SHOWNA);",
                   "horizontal scrollbar initialized before first show")
    require_before(filesbx1, "SetScrollInfo(HVScrollBar, SB_CTL, &si, TRUE);",
                   "ShowWindow(HVScrollBar, SW_SHOWNA);",
                   "vertical scrollbar initialized before first show")
    require(filesbx1, "DarkModeAllowDarkScrollbars(HHScrollBar);",
            "horizontal scrollbar direct dark-hook opt-in")
    require(filesbx1, "DarkModeAllowDarkScrollbars(HVScrollBar);",
            "vertical scrollbar direct dark-hook opt-in")
    require(filesbx1, "PaintDarkPanelScrollbar",
            "dark panel scrollbar paints its complete surface before native theme paint")
    require(filesbx1, "DrawDarkScrollArrow",
            "dark panel scrollbar remains visibly identifiable instead of a blank band")
    require(filesbx1, "SetWindowSubclass(HHScrollBar, DarkPanelScrollbarProc",
            "horizontal scrollbar custom dark first-frame rendering")
    require(filesbx1, "SetWindowSubclass(HVScrollBar, DarkPanelScrollbarProc",
            "vertical scrollbar custom dark first-frame rendering")
    if "DarkScrollBarPlaceholderProc" in filesbx1:
        raise AssertionError("Native scrollbars must remain visible instead of becoming blank placeholder bands")
    require(mainwnd1, "SaveDetachedTabConfigNow();",
            "explicit detached-tab close is persisted immediately")
    require(mainwnd2, "ConfigurationStorage.Flush(FALSE);",
            "explicit detached-tab close flushes portable configuration")
    require(tabwnd, "case WM_ERASEBKGND:\n        if (DarkModeIsWindowsDarkSchemeSelected())",
            "panel tab-bar dark initial erase")
    require_before(
        menubar,
        "if (DarkModeIsWindowsDarkSchemeSelected())",
        "if (WindowsVistaAndLater) // under Vista the rebar flickers",
        "dark menu-bar background before suppressing erase",
    )
    require(mainwnd1, "case WM_DPICHANGED:", "per-monitor DPI handling")
    require(mainwnd1, "RebuildDetachedTabToolbarImageLists",
            "per-window directory-line toolbar images")
    require(mainwnd1, "BuildWindowTitleForPanel(info.Panel, prefix, wideAppSuffix)",
            "configured full/composite/directory detached window title")
    require(stswnd, "!MainWindow->IsDetachedTabPanel(FilesWindow)",
            "hidden zoom/reattach button in detached-tab directory line")
    require(mainwnd3, "activePanel == DetachedTabPanel",
            "Change Drive routing to the detached tab")
    require(drivelst, "FilesWindow->IsLeftPanel() ? CM_LCHANGEDRIVE : CM_RCHANGEDRIVE",
            "detached Change Drive toolbar anchor")
    require(mainwnd1, "cmd == APPCOMMAND_BROWSER_BACKWARD ? CM_ACTIVEBACK : CM_ACTIVEFORWARD",
            "detached mouse back/forward history routing")
    require(mainwnd1, "CDetachedTabInfo& remaining = DetachedTabs.back();",
            "remaining detached tab context after closing the active window")
    require(mainwnd1, "UpdatePanelTabVisibility(targetSide);\n    }\n    SetWindowTitle();",
            "title refresh after reattach establishes the new active host context")
    require(mainwnd1, "case WM_COMMAND:\n            if (info != NULL && info->Panel != NULL)",
            "per-window detached command context routing")
    require(mainwnd1, "case WM_NOTIFY:\n            if (info != NULL && info->Panel != NULL)",
            "per-window detached notification context routing")
    require(mainwnd1, "case WM_CONTEXTMENU:\n            if (info != NULL && info->Panel != NULL)",
            "per-window detached context-menu routing")
    require(mainwnd_h, "CFilesWindow* MainWindowTitlePanel;",
            "last active main-window panel title ownership")
    require(mainwnd1, "wideText = BuildWindowTitleForPanel(mainTitlePanel, prefix, mainSuffix);",
            "main title isolated from detached tab navigation")
    if "wideText = BuildWindowTitleForPanel(DetachedPanels ? LeftPanel : GetActivePanel()" in mainwnd1:
        raise AssertionError("Detached tab navigation can still leak into the main window title")
    if "detachedText = BuildWindowTitleForPanel(DetachedPanels ? RightPanel : GetActivePanel()" in mainwnd1:
        raise AssertionError("Detached tab navigation can still leak into the detached-side title")

    require(zip_cpp, "MainWindow->IsDetachedTabPanel(sourcePanel)",
            "plugin source-side detection for the detached tab")
    require(mainwnd4, "GetActivePanel() == DetachedTabPanel && DetachedTabOriginalSide == cpsLeft",
            "explicit left plugin API routing to the detached tab")
    require(mainwnd4, "GetActivePanel() == DetachedTabPanel && DetachedTabOriginalSide == cpsRight",
            "explicit right plugin API routing to the detached tab")
    require(mainwnd_h, "activePanel == DetachedTabPanel",
            "detached target-panel routing by original side")
    require(zip_cpp, "PostMessage(detachedPanel->HWindow, WM_USER_REFRESH_DIR, 0, t1);",
            "async plugin FS refresh delivery to the detached tab")
    require(plugins4, "detachedPanel->GetPluginFS()->Contains(timer->TimerOwner)",
            "plugin FS timer delivery to the detached tab")
    require(zip_cpp, "detachedPanel->DirectoryLine->IsThrobberVisible(id)",
            "detached plugin FS throbber shutdown")

    require(mainwnd2, "SaveDetachedTabConfig(salamander)",
            "detached tab persistence save")
    require(mainwnd2, "LoadDetachedTabConfig(salamander)",
            "detached tab persistence load")
    require(mainwnd2, "info->Placement",
            "detached tab geometry persistence")
    require(mainwnd_h, "std::vector<CDetachedTabInfo> DetachedTabs",
            "independent state for multiple detached tab windows")
    require(mainwnd2, 'wsprintf(tabKeyName, "Tab%d", i + 1)',
            "one persisted registry subkey per detached tab")
    require(mainwnd1, "EnsurePanelRefreshAndRequest(panel, false, true)",
            "extension FS refresh restart after reparenting")
    require(mainwnd1, "if (!RestoringPanelPaths)\n        EnsurePanelRefreshAndRequest(panel, false, true);",
            "no duplicate extension refresh while restoring detached tabs")
    require(mainwnd2, "Open the saved path in its final window",
            "detached extension path restored only after final reparent")
    require(plugins2, "now - LastVisualUpdate >= 50",
            "startup extension progress repaint throttling")
    require(mainwnd3, "now - LastVisualUpdate >= 50",
            "shutdown extension progress repaint throttling")
    require(zip_cpp, "detachedIndex < MainWindow->GetDetachedTabCount()",
            "TabId lookup for asynchronous detached extension FS updates")
    require(zip_cpp, "FindDetachedTabPanelByPluginFS(modifiedFS)",
            "locked detached panel lookup from extension FS worker threads")
    require(stswnd, "FALSE, DriveIcon, TRUE, TRUE",
            "dynamic Change Drive icon reapplication after image-list rebinding")
    require(salamatrix_packages, "TryAcquireSRWLockExclusive(lock)",
            "non-blocking serialization of extension FS actions and listings")
    require(salamatrix_packages, "if (RefreshDeferred)",
            "load-on-start extension refresh batching")
    require(salamatrix_packages, "SALAMANDER_SERVICE_SHUTDOWN_PROGRESS",
            "extension catalog refresh suppression during shutdown")
    require(mainwnd2, "CONFIG_CNFRM_DETACHTABCLOSE",
            "close-confirmation setting persistence")
    require(mainwnd4, "panel->CapturePathForShutdown()",
            "plugin and extension path snapshot before shutdown unload")
    require(mainwnd2, "GetPathCapturedForShutdown()",
            "saved pre-unload plugin and extension path")
    require(configstorage, "ActiveRegistryKeys.Add(key);",
            "file-backed registry open-handle reference counting")
    if "if (ActiveRegistryKeys[i] == key)\n            return;" in configstorage:
        raise AssertionError("File-backed registry still deduplicates open handles")
    require(mainwnd2, "if (closeTabKey)\n            CloseKey(tabKey);",
            "detached config key close before reparenting and restoring plugin FS")
    require(mainwnd1, "EditWindow != NULL && EditWindow->KnowHWND(hwnd)",
            "late WM_KILLFOCUS shutdown crash guard")

    require(mainwnd_h, "BOOL PreserveDetachedPanelsOnShutdown;",
            "explicit detached-panel shutdown persistence state")
    require(mainwnd2, "DetachedPanels || PreserveDetachedPanelsOnShutdown ||",
            "detached-panel mode save across temporary shutdown reattach")
    require_before(
        mainwnd3,
        "if (uMsg == WM_USER_CLOSE_MAINWND && DetachedPanels && wParam == 0)",
        "Plugins.UnloadAll(closingProgress.HWindow",
        "detached close choice before plug-in unload",
    )
    require(mainwnd3, "if (wParam == 0 && !detachedPanelsCloseConfirmed)",
            "no duplicate detached close prompt after early confirmation")

    require(mainwnd_h, "HWND HPanelTabDetachPreview;",
            "detached-tab drag preview lifecycle state")
    require(mainwnd1, "SalamanderDetachedTabPreview",
            "detached-tab drag preview window")
    require(mainwnd1, "CombineRgn(outline, outline, interior, RGN_DIFF);",
            "detached-tab window outline region")
    require(mainwnd3, "ShowPanelTabDetachPreview(screenPt);",
            "detached-tab preview while dragging outside Salamander")
    require(mainwnd3, "HidePanelTabDetachPreview();",
            "detached-tab preview cleanup")
    require(mainwnd1, "panelRect.right - panelRect.left + nonClientWidth",
            "detached window client width preserved from the dragged panel")
    require(mainwnd1, "CreateDetachedTabWindow(this, panel, dropPoint)",
            "source panel geometry used by the real detached window")

    require(mainwnd3, "case CM_COPYFILES:", "copy operation guard")
    require(mainwnd3, "case CM_MOVEFILES:", "move operation guard")
    require(mainwnd3, "IDS_DETACHED_TAB_UNSUPPORTED",
            "localized unsupported-operation feedback")

    expected_ids = {str(value) for value in range(14337, 14347)}
    language_files = sorted((ROOT / "translations").glob("*/salamand.slt"))
    if not language_files:
        raise AssertionError("No salamand.slt translation files found")
    for language_file in language_files:
        ids = {
            line.split(",", 1)[0].strip()
            for line in language_file.read_text(encoding="utf-8-sig").splitlines()
            if "," in line
        }
        missing = expected_ids - ids
        if missing:
            raise AssertionError(
                f"{language_file.relative_to(ROOT)} misses detached-tab IDs: "
                + ", ".join(sorted(missing))
            )

    print("Detached tab source-contract tests passed.")


if __name__ == "__main__":
    main()
