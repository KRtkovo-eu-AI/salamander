# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    geticon = (ROOT / "geticon.cpp").read_text(encoding="utf-8")
    filesbox = (ROOT / "filesbx1.cpp").read_text(encoding="utf-8")
    fileswnd0 = (ROOT / "fileswn0.cpp").read_text(encoding="utf-8")
    fileswnd = (ROOT / "fileswn1.cpp").read_text(encoding="utf-8")
    fileswnd2 = (ROOT / "fileswn2.cpp").read_text(encoding="utf-8")
    fileswndb = (ROOT / "fileswnb.cpp").read_text(encoding="utf-8")
    mainwnd1 = (ROOT / "mainwnd1.cpp").read_text(encoding="utf-8")
    mainwnd2 = (ROOT / "mainwnd2.cpp").read_text(encoding="utf-8")
    mainwnd3 = (ROOT / "mainwnd3.cpp").read_text(encoding="utf-8")
    mainwnd4 = (ROOT / "mainwnd4.cpp").read_text(encoding="utf-8")
    logo = (ROOT / "logo.cpp").read_text(encoding="utf-8")
    plugins2 = (ROOT / "plugins2.cpp").read_text(encoding="utf-8")
    salamdr1 = (ROOT / "salamdr1.cpp").read_text(encoding="utf-8")
    samandarin = (ROOT / "plugins/samandarin/samandarin.cpp").read_text(encoding="utf-8")
    managed_bridge = (ROOT / "plugins/samandarin/managed_bridge.cpp").read_text(encoding="utf-8")

    if "IsSolidBlackIcon" not in geticon or "IsInvalidShellIcon" in geticon:
        print("shell icon validation must reject only the proven solid-black corruption")
        return 1

    resize = re.search(
        r"case WM_SIZE:\s*\{(.*?)break;\s*\}", filesbox, re.DOTALL
    )
    if resize is None or "LayoutChilds();" not in resize.group(1):
        print("files box no longer lays out children during WM_SIZE")
        return 1
    if "InvalidateRect(HWindow, NULL, FALSE);" not in resize.group(1):
        print("files box resize must invalidate stale persistent-DC content")
        return 1
    buffered_paint = re.search(r"case WM_PAINT:.*?case WM_HELP:", filesbox, re.DOTALL)
    if (
        buffered_paint is None
        or "CreateCompatibleBitmap" not in buffered_paint.group(0)
        or "HPrivateDC = memoryDC;" not in buffered_paint.group(0)
        or "BitBlt(paintDC" not in buffered_paint.group(0)
    ):
        print("full files-box WM_PAINT must publish one buffered frame")
        return 1

    production = geticon + fileswnd + fileswndb + mainwnd2 + plugins2 + salamdr1
    if (
        "OpenSalamander-icon-state.log" in production
        or "OpenSalamander-startup-timing.log" in production
        or "OpenSalamander-startup-timing2.log" in production
        or "OpenSalamander-startup-refresh.log" in production
        or "StartupPerfMark" in production
        or "StartupPerfFlush" in production
        or "StartupProbeMark" in production
        or "StartupProbeFlush" in production
    ):
        print("temporary runtime diagnostics remain in production sources")
        return 1
    if "CStartupTimingTrace" in mainwnd2 or "panel WM_CREATE total=" in fileswndb:
        print("startup timing instrumentation must not ship in production")
        return 1
    connect = re.search(
        r"void WINAPI CPluginInterface::Connect\(.*?\n\}", samandarin, re.DOTALL
    )
    if (
        connect is None
        or "ManagedBridge_BeginInitialize(parent)" not in connect.group(0)
        or "ManagedBridge_EnsureInitialized(parent)" not in connect.group(0)
        or "CreateThread(nullptr, 0, InitializeRuntimeThread" not in managed_bridge
        or managed_bridge.count("WaitForBackgroundInitialization();") < 2
        or "ResetRuntimeLocked();" not in managed_bridge
    ):
        print("Samandarin CLR prewarm must not block startup and must join before shutdown")
        return 1

    if "DiscardSolidBlackIcon(&hIconSmall" not in geticon:
        print("corrupt image-list icons must be discarded before direct shell fallback")
        return 1
    direct_fallback = re.search(
        r"DiscardSolidBlackIcon\(&hIconSmall.*?SHGFI_ICON\s*\|\s*SHGFI_SMALLICON",
        geticon,
        re.DOTALL,
    )
    if direct_fallback is None:
        print("solid-black image-list result must fall through to direct SHGFI_ICON")
        return 1
    if "AssocQueryStringW(ASSOCF_NONE, ASSOCSTR_DEFAULTICON" not in geticon:
        print("failed shell image-list extraction must use the registered DefaultIcon")
        return 1
    if "SHDefExtractIconW(iconPath.c_str(), iconIndex" not in geticon:
        print("registered DefaultIcon must be extracted at the panel pixel size")
        return 1
    if "useExplorerFileTypeIcon" in fileswnd or "GetExplorerFileTypeIcon" in fileswnd:
        print("ordinary disk icon loading must not bypass GetFileIcon")
        return 1
    association_refresh = re.search(
        r"case WM_USER_REFRESHINDEX:.*?case WM_USER_DROPCOPYMOVE:",
        fileswndb,
        re.DOTALL,
    )
    if (
        association_refresh is None
        or "RepaintIconsForExtension(buf);" not in association_refresh.group(0)
        or "RepaintIconOnly(-1)" in association_refresh.group(0)
    ):
        print("association icon delivery must not repaint every icon in both panels")
        return 1

    if "!TreeViewAutoHide || TreeViewAutoHideExpanded" not in fileswnd2:
        print("collapsed auto-hide Tree View must not be populated during startup")
        return 1
    if "RefreshTreeViewDPI();\n    RefreshTreeView();" not in fileswnd2:
        print("deferred auto-hide Tree View must be initialized when expanded")
        return 1
    if "!MainWindow->RestoringPanelPaths" not in fileswndb or "if (RestoringPanelPaths)" not in mainwnd4:
        print("panel restoration must suppress transient Tree View rebuilds")
        return 1
    if "batchToolbarLayout" not in mainwnd2:
        print("configured rebar bands must use one batched startup layout")
        return 1
    startup_lock_guard = (
        "const BOOL lockWindowUpdate = IsWindowVisible(HWindow) && "
        "!StartupWindowCloaked;"
    )
    if mainwnd1.count(startup_lock_guard) != 7:
        print("hidden startup must skip global LockWindowUpdate in all toolbar toggles")
        return 1
    config_done = mainwnd2.find("IfExistSetSplashScreenText(LoadStr(IDS_STARTUP_DATA));")
    panel_restore = mainwnd2.find("RestoringPanelPaths = TRUE;")
    if config_done < 0 or panel_restore < 0 or config_done > panel_restore:
        print("Reading configuration status must end before panel initialization")
        return 1
    panel_final = mainwnd2.find("MainWindow->UpdateDefaultDir(TRUE);")
    deferred_reveal = mainwnd2.find("if (deferMainWindowReveal)", panel_final)
    if (
        "deferMainWindowReveal = TRUE;" not in mainwnd2
        or panel_final < 0
        or deferred_reveal < panel_final
    ):
        print("main window must remain hidden until both panels are fully initialized")
        return 1
    if "RDW_ALLCHILDREN | RDW_UPDATENOW" not in mainwnd2[deferred_reveal:]:
        print("the first visible main-window frame must synchronously draw all panels")
        return 1
    if (
        "DarkModeSetWindowCloaked(HWindow, true)" not in mainwnd2
        or "void CMainWindow::RevealStartupWindow()" not in mainwnd2
        or "DarkModeSetWindowCloaked(HWindow, false)" not in mainwnd2
        or "MainWindow->RevealStartupWindow();" not in salamdr1
    ):
        print("the main window must stay cloaked through post-config plug-in startup")
        return 1
    plugin_startup = salamdr1.find("Plugins.HandleLoadOnStartFlag(MainWindow->HWindow);")
    final_reveal = salamdr1.find("MainWindow->RevealStartupWindow();")
    message_loop = salamdr1.find('CALL_STACK_MESSAGE1("WinMainBody::message_loop")')
    if plugin_startup < 0 or final_reveal < plugin_startup or message_loop < final_reveal:
        print("the first main-window frame must be revealed after plug-ins and before the message loop")
        return 1
    reveal_body = re.search(
        r"void CMainWindow::RevealStartupWindow\(\).*?\n\}",
        mainwnd2,
        re.DOTALL,
    )
    if (
        reveal_body is None
        or "WM_USER_END_SUSPMODE" not in reveal_body.group(0)
        or "SkipOneActivateRefresh = TRUE;" not in reveal_body.group(0)
        or "WM_USER_SKIPONEREFRESH" not in reveal_body.group(0)
    ):
        print("splash teardown must suppress its redundant first activation refresh")
        return 1
    startup_size = reveal_body.group(0).find(
        "PeekMessage(&msg, HWindow, WM_SIZE, WM_SIZE, PM_REMOVE)"
    )
    reveal_barrier = reveal_body.group(0).find(
        "PostMessage(HWindow, WM_TIMER, IDT_FINISHSTARTUPREVEAL, 0)"
    )
    if startup_size < 0 or reveal_barrier < startup_size:
        print("startup DPI layout must enter the message loop before final reveal")
        return 1
    if "SetTimer(HWindow, IDT_FINISHSTARTUPREVEAL" in reveal_body.group(0):
        print("startup reveal must not wait for a low-priority timer")
        return 1
    finish_body = re.search(
        r"void CMainWindow::FinishStartupWindowReveal\(\).*?\n\}",
        mainwnd2,
        re.DOTALL,
    )
    if (
        "DarkModeSetWindowCloaked(HWindow, false)" in reveal_body.group(0)
        or "CompletePendingStartupRefreshes();" in reveal_body.group(0)
        or finish_body is None
        or finish_body.group(0).count("CompletePendingStartupRefreshes();") != 2
    ):
        print("startup window must remain cloaked through the first message-loop turn")
        return 1
    final_refresh = finish_body.group(0).find("LeftPanel->CompletePendingStartupRefreshes();")
    final_redraw = finish_body.group(0).find("RedrawWindow(HWindow")
    final_uncloak = finish_body.group(0).find("DarkModeSetWindowCloaked(HWindow, false)")
    splash_close = finish_body.group(0).find("SplashScreenCloseIfExist();")
    if (
        final_refresh < 0
        or final_redraw < final_refresh
        or final_uncloak < final_redraw
        or splash_close < final_uncloak
        or "case IDT_FINISHSTARTUPREVEAL:" not in mainwnd3
    ):
        print("final refresh and redraw must complete before uncloak and splash close")
        return 1
    cancel_refresh = re.search(
        r"void CFilesWindow::CompletePendingStartupRefreshes\(\).*?\n\}",
        fileswnd0,
        re.DOTALL,
    )
    if (
        cancel_refresh is None
        or "KillTimer(HWindow, IDT_REFRESH_DIR_EX);" not in cancel_refresh.group(0)
        or "RefreshDirExTimerSet = FALSE;" not in cancel_refresh.group(0)
        or "WM_USER_REFRESH_DIR_EX_DELAYED" not in cancel_refresh.group(0)
        or "WM_USER_REFRESH_DIR," not in cancel_refresh.group(0)
        or "SendMessage(HWindow, msg.message, msg.wParam, msg.lParam);"
        not in cancel_refresh.group(0)
    ):
        print("startup reveal must finish required refreshes while still cloaked")
        return 1
    window_pos_changed = re.search(
        r"case WM_WINDOWPOSCHANGED:.*?LRESULT result = CWindow::WindowProc\(uMsg, wParam, lParam\);"
        r".*?clientWidth != WindowWidth.*?PostMessage\(HWindow, WM_SIZE",
        mainwnd3,
        re.DOTALL,
    )
    if window_pos_changed is None:
        print("WM_WINDOWPOSCHANGED must run the real WM_SIZE before considering its fallback")
        return 1

    reattach_tab = re.search(
        r"BOOL CMainWindow::ReattachDetachedTab\(CFilesWindow\* panel.*?\n\}",
        mainwnd1,
        re.DOTALL,
    )
    if (
        reattach_tab is None
        or "WM_SETREDRAW, FALSE" not in reattach_tab.group(0)
        or reattach_tab.group(0).count("WM_SETREDRAW, TRUE") < 3
        or "RDW_ALLCHILDREN | RDW_UPDATENOW" not in reattach_tab.group(0)
        or reattach_tab.group(0).rfind("DestroyWindow(detachedWindow)")
        < reattach_tab.group(0).rfind("RedrawWindow(targetHost")
        or reattach_tab.group(0).rfind("DarkModeSetWindowCloaked(detachedWindow, true)")
        < reattach_tab.group(0).rfind("RedrawWindow(targetHost")
    ):
        print("detached-tab reattach must publish one final target-host frame")
        return 1
    reattach_panels = re.search(
        r"BOOL CMainWindow::SetPanelsDetached\(BOOL detached\).*?"
        r"BOOL CMainWindow::TogglePanelsDetached",
        mainwnd1,
        re.DOTALL,
    )
    panels_body = reattach_panels.group(0) if reattach_panels is not None else ""
    detach_start = panels_body.find("if (detached)")
    reattach_start = panels_body.find("\n    else", detach_start)
    detach_panels = panels_body[detach_start:reattach_start]
    reattach_only = panels_body[reattach_start:]
    if (
        reattach_panels is None
        or detach_start < 0
        or reattach_start < 0
        or reattach_only.count("WM_SETREDRAW, FALSE") != 2
        or reattach_only.count("WM_SETREDRAW, TRUE") != 2
        or reattach_only.rfind("RedrawWindow(HWindow") < reattach_only.rfind("CM_SWAPPANELS")
        or reattach_only.rfind("ShowWindow(HRightDetachedWindow, SW_HIDE)")
        < reattach_only.rfind("RedrawWindow(HWindow")
        or "DarkModeSetWindowCloaked(HRightDetachedWindow, true)" not in reattach_only
    ):
        print("detached-panel reattach must freeze both hosts and publish one final frame")
        return 1
    if (
        detach_panels.count("WM_SETREDRAW, FALSE") != 2
        or detach_panels.count("WM_SETREDRAW, TRUE") != 4
        or "DarkModeSetWindowCloaked(HRightDetachedWindow, true)" not in detach_panels
        or detach_panels.rfind("RedrawWindow(HRightDetachedWindow")
        < detach_panels.rfind("Plugins.Event(PLUGINEVENT_TABCHANGED")
        or detach_panels.rfind("DarkModeSetWindowCloaked(HRightDetachedWindow, false)")
        < detach_panels.rfind("RedrawWindow(HWindow")
    ):
        print("panel detach must build both complete frames before compositor reveal")
        return 1
    splash_status = re.search(
        r"void CSplashScreen::SetText\(const char\* text\).*?\n\}", logo, re.DOTALL
    )
    if (
        splash_status is None
        or "dirtyR.left = max(0, dirtyR.left - 2);" not in splash_status.group(0)
        or splash_status.group(0).count("dirtyR.right - dirtyR.left") != 2
    ):
        print("splash status repaint must clear and publish glyph overhang pixels")
        return 1
    if re.search(r"while\s*\(PeekMessage\(&msg, NULL, 0, 0, PM_REMOVE\)\)", mainwnd2):
        print("configuration loading must not synchronously drain the UI message queue")
        return 1

    print("panel repaint and Explorer association icon contracts hold")
    return 0


if __name__ == "__main__":
    sys.exit(main())
