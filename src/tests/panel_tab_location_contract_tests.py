#!/usr/bin/env python3
"""Regression guards for panel-tab location ownership and restoration."""

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[2]
MAINWND2 = (ROOT / "src" / "mainwnd2.cpp").read_text(encoding="utf-8")
MAINWND3 = (ROOT / "src" / "mainwnd3.cpp").read_text(encoding="utf-8")
MAINWND1 = (ROOT / "src" / "mainwnd1.cpp").read_text(encoding="utf-8")


def body(source: str, signature: str) -> str:
    match = re.search(signature + r".*?^}\n", source, re.DOTALL | re.MULTILINE)
    if match is None:
        raise AssertionError(f"Missing implementation matching: {signature}")
    return match.group(0)


def require_order(source: str, *fragments: str) -> None:
    positions = [source.find(fragment) for fragment in fragments]
    if any(position < 0 for position in positions) or positions != sorted(positions):
        raise AssertionError("Required ordering was not preserved: " + " -> ".join(fragments))


def main() -> int:
    try:
        add_tab = body(MAINWND3, r"CFilesWindow\* CMainWindow::AddPanelTab\(CPanelSide side, int index\)")
        if "InsertPanelTabInstance(side, index, panel, false)" not in add_tab:
            raise AssertionError("AddPanelTab must register the new tab")
        if "SwitchPanelTab(panel);" in add_tab:
            raise AssertionError("AddPanelTab must not activate an uninitialized tab")

        default_dir = body(MAINWND1, r"void CMainWindow::UpdateDefaultDir\(BOOL activePrefered\)")
        if (
            "auto updateFromPanel = [](CFilesWindow* panel)" not in default_dir
            or "path == NULL || path[0] == 0 || path[1] != ':'" not in default_dir
            or "LowerCase[path[0]] < 'a' || LowerCase[path[0]] > 'z'" not in default_dir
            or "DefaultDir[driveIndex]" not in default_dir
        ):
            raise AssertionError("DefaultDir must reject uninitialized and non-drive tab paths")

        api_new_tab = body(MAINWND3, r"BOOL CMainWindow::CreatePanelTab\(CPanelSide side, const char\* path, int insertIndex,")
        require_order(api_new_tab,
                      "CFilesWindow* previous =",
                      "char targetPath[2 * MAX_PATH];",
                      "CFilesWindow* panel = AddPanelTab",
                      "panel->ChangeDir(targetPath)",
                      "SwitchPanelTab(panel);")

        new_tab = body(MAINWND3, r"void CMainWindow::CommandNewTab\(CPanelSide side, bool addAtEnd\)")
        require_order(new_tab,
                      "CFilesWindow* previous =",
                      "char targetPath[2 * MAX_PATH];",
                      "CFilesWindow* panel = AddPanelTab",
                      "panel->ChangeDir(targetPath);",
                      "SwitchPanelTab(panel);")

        duplicate_tab = body(MAINWND3, r"CFilesWindow\* CMainWindow::CreateDuplicatePanelTab\(")
        require_order(duplicate_tab,
                      "CFilesWindow* previousTarget =",
                      "char sourcePath[2 * MAX_PATH];",
                      "CFilesWindow* newPanel = AddPanelTab",
                      "newPanel->ChangeDir(sourcePath);",
                      "SwitchPanelTab(newPanel);")
        if duplicate_tab.find("sourcePanel->GetGeneralPath") > duplicate_tab.find("CFilesWindow* newPanel = AddPanelTab"):
            raise AssertionError("Duplicate must capture the source location before creating the target tab")

        close_tab = body(MAINWND3, r"void CMainWindow::ClosePanelTab\(CFilesWindow\* panel, bool storeForReopen\)")
        require_order(close_tab,
                      "tabs.Detach(index);",
                      "CFilesWindow* newPanel = tabs[index];",
                      "SwitchPanelTab(newPanel);")
        if "newPanel->ChangeDir(" in close_tab or "newPanel->ChangePathToDisk(" in close_tab:
            raise AssertionError("Closing a tab must never change the replacement tab location")

        automatic_refresh = body(MAINWND3, r"void CMainWindow::EnsurePanelAutomaticRefresh\(CFilesWindow\* panel\)")
        if re.search(r"\bpanel->(?:ChangePathToDisk|ChangeDir)\(", automatic_refresh):
            raise AssertionError(
                "Tab activation refresh must not navigate, shorten, or rescue the tab path"
            )
        if "EnsureWatching(panel, registerDevNotification);" not in automatic_refresh:
            raise AssertionError("Tab activation must keep directory monitoring for its own panel")

        refresh_and_request = body(MAINWND3, r"void CMainWindow::EnsurePanelRefreshAndRequest\(CFilesWindow\* panel, bool rebuildDriveBars,")
        require_order(refresh_and_request,
                      "EnsurePanelAutomaticRefresh(panel);",
                      "RequestPanelRefresh(panel, rebuildDriveBars, postRefreshMessage);")

        save_config = body(MAINWND2, r"void CMainWindow::SavePanelConfig\(CPanelSide side, HKEY hSalamander, const char\* reg\)")
        if "SavePanelSettingsToKey(tabs[i], tabKey, TRUE);" not in save_config:
            raise AssertionError("Shutdown must persist each tab's own location")

        load_config = body(MAINWND2, r"void CMainWindow::LoadPanelConfig\(char\* panelPath, CPanelSide side, HKEY hSalamander, const char\* reg\)")
        require_order(load_config,
                      "for (int i = 0; i < localTabs.Count; i++)",
                      "LoadPanelSettingsFromKey(panel, tabKey, path, _countof(path));",
                      "RestorePanelPathFromConfig(this, panel, path);",
                      "SwitchPanelTab(activePanel);")
    except AssertionError as error:
        print(f"Panel-tab location contract test failed: {error}")
        return 1

    print("Panel-tab location contract tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
