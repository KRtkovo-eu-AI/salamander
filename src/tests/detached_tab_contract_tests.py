# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def require(text: str, needle: str, description: str) -> None:
    if needle not in text:
        raise AssertionError(f"Missing {description}: {needle}")


def main() -> None:
    mainwnd1 = (ROOT / "src/mainwnd1.cpp").read_text(encoding="utf-8")
    mainwnd2 = (ROOT / "src/mainwnd2.cpp").read_text(encoding="utf-8")
    mainwnd3 = (ROOT / "src/mainwnd3.cpp").read_text(encoding="utf-8")
    mainwnd4 = (ROOT / "src/mainwnd4.cpp").read_text(encoding="utf-8")
    mainwnd_h = (ROOT / "src/mainwnd.h").read_text(encoding="utf-8")
    zip_cpp = (ROOT / "src/zip.cpp").read_text(encoding="utf-8")
    plugins4 = (ROOT / "src/plugins4.cpp").read_text(encoding="utf-8")
    configstorage = (ROOT / "src/configstorage.cpp").read_text(encoding="utf-8")
    drivelst = (ROOT / "src/drivelst.cpp").read_text(encoding="utf-8")
    stswnd = (ROOT / "src/stswnd.cpp").read_text(encoding="utf-8")
    tabwnd = (ROOT / "src/tabwnd.cpp").read_text(encoding="utf-8")

    require(mainwnd1, "index <= 0 || panel->IsTabLocked()",
            "default/locked tab detach guard")
    require(mainwnd3, "appendMenuItem(CM_DETACHTAB, IDS_MENU_DETACH_TAB",
            "tab context-menu detach command")
    require(mainwnd3, "return DetachPanelTab(panel, &screenPt) != FALSE",
            "drag completion outside Salamander")
    require(tabwnd, "if (GetTabCount() <= 1)",
            "drag support when only default plus one detachable tab exist")

    require(mainwnd1, "ReattachDetachedTab(cpsLeft)", "left reattach choice")
    require(mainwnd1, "ReattachDetachedTab(cpsRight)", "right reattach choice")
    require(mainwnd1, "MSGBOXEX_YESNOOKCANCEL", "three actions plus safe cancel")
    require(mainwnd1, "DarkModeApplyWindow(hWnd)", "dark detached top-level window")
    require(mainwnd1, "case WM_DPICHANGED:", "per-monitor DPI handling")
    require(mainwnd1, "RebuildDetachedTabToolbarImageLists",
            "per-window directory-line toolbar images")
    require(mainwnd1, "BuildWindowTitleForPanel(DetachedTabPanel, prefix, wideAppSuffix)",
            "configured full/composite/directory detached window title")
    require(stswnd, "FilesWindow != MainWindow->GetDetachedTabPanel()",
            "hidden zoom/reattach button in detached-tab directory line")
    require(mainwnd3, "activePanel == DetachedTabPanel",
            "Change Drive routing to the detached tab")
    require(drivelst, "FilesWindow->IsLeftPanel() ? CM_LCHANGEDRIVE : CM_RCHANGEDRIVE",
            "detached Change Drive toolbar anchor")
    require(mainwnd1, "cmd == APPCOMMAND_BROWSER_BACKWARD ? CM_ACTIVEBACK : CM_ACTIVEFORWARD",
            "detached mouse back/forward history routing")

    require(zip_cpp, "sourcePanel == MainWindow->GetDetachedTabPanel()",
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
    require(mainwnd2, "DetachedTabWindowPlacement",
            "detached tab geometry persistence")
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
    require(mainwnd2,
            "CloseKey(key);\n    RestorePanelPathFromConfig(this, panel, path.data());",
            "detached config key close before restoring plugin FS")
    require(mainwnd1, "EditWindow != NULL && EditWindow->KnowHWND(hwnd)",
            "late WM_KILLFOCUS shutdown crash guard")

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
