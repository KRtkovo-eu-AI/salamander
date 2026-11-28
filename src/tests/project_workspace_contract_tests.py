# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-28 | Assert distinct project-workspace and generic-lock tab badges. | Preserve separate workspace and lock visual states. | https://github.com/KRtkovo-eu-AI/salamander/issues/853 | openai/gpt-6-luna-pro |
# | 2026-11-28 | Guard project-workspace path routing and tab lifecycle contracts. | Catch bypasses through history, archives, plugin filesystems, recovery, or persistence. | https://github.com/KRtkovo-eu-AI/salamander/issues/853 | openai/gpt-6-luna-pro |
"""Source contracts for project-workspace navigation and tab lifecycle."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[1]


def source(name: str) -> str:
    return (ROOT / name).read_text(encoding="utf-8-sig")


def body(name: str, signature: str) -> str:
    text = source(name)
    start = text.index(signature)
    end = re.search(r"^}\s*$", text[start:], re.MULTILINE)
    if end is None:
        raise AssertionError(f"Missing function end: {signature}")
    return text[start:start + end.end()]


class ProjectWorkspaceContracts(unittest.TestCase):
    def test_disk_archive_and_plugin_routes_preflight_destinations(self):
        disk = body("fileswn2.cpp", "BOOL CFilesWindow::ChangePathToDisk(HWND parent,")
        self.assertIn("ConfirmUnlockTabForPathChange(resolvedPathW.c_str(), TRUE)", disk)
        self.assertIn("IsProjectWorkspacePath(fallbackPathW.c_str())", disk)
        self.assertIn("restrictFallbackToWorkspace", disk)
        self.assertLess(disk.index("ConfirmUnlockTabForPathChange("), disk.index("RefreshPathHistoryData()"))

        archive = body("fileswn2.cpp", "BOOL CFilesWindow::ChangePathToArchive(")
        self.assertIn("ConfirmUnlockTabForPathChange(archivePathW.c_str(), TRUE)", archive)
        self.assertLess(archive.index("ConfirmUnlockTabForPathChange("), archive.index("RefreshPathHistoryData()"))

        plugin = body("fileswn2.cpp", "BOOL CFilesWindow::ChangePathToPluginFS(")
        self.assertIn("ConfirmUnlockTabForPathChange(L\"\", FALSE)", plugin)

        detached = body("fileswn2.cpp", "BOOL CFilesWindow::ChangePathToDetachedFS(")
        self.assertIn("ConfirmUnlockTabForPathChange(L\"\", FALSE)", detached)

    def test_history_preflights_its_destination_before_execution(self):
        history = body("salamdr3.cpp", "void CPathHistory::ExecuteInOriginalTab(")
        self.assertIn("destinationIsDiskOrArchive", history)
        self.assertIn("ConfirmUnlockTabForPathChange(destinationIsDiskOrArchive ? destinationPathW.c_str() : L\"\",", history)
        self.assertLess(history.index("ConfirmUnlockTabForPathChange("), history.index("ExecuteDirectoryHistoryItem(destination, panel)"))

    def test_workspace_navigation_and_recovery_keep_the_stored_root(self):
        confirm = body("fileswn1.cpp", "BOOL CFilesWindow::ConfirmUnlockTabForPathChange(")
        self.assertIn("IsProjectWorkspacePath(targetPath)", confirm)
        self.assertIn("SetProjectWorkspaceRestricted(false)", confirm)

        go_root = body("fileswn1.cpp", "void CFilesWindow::GotoRoot()")
        self.assertIn("ProjectWorkspaceRoot.c_str()", go_root)
        self.assertIn("ChangePathToDiskW(HWindow, ProjectWorkspaceRoot.c_str())", go_root)

        recovery = body("mainwnd2.cpp", "static BOOL RestoreDeniedSavedPanelPath(")
        self.assertIn("IsProjectWorkspacePathLexicallyInside(candidate.c_str())", recovery)
        self.assertIn("IsProjectWorkspaceRootPath(candidate.c_str())", recovery)
        self.assertIn("if (projectRestricted)", recovery)
        self.assertNotIn("ChangeToFixedDrive(panel->HWindow)", recovery[:recovery.index("if (projectRestricted)")])

        root_restore = body("mainwnd2.cpp", "static BOOL TryRestoreProjectWorkspaceRoot(")
        self.assertIn("NULL, TRUE, FALSE, TRUE, NULL", root_restore)
        self.assertIn("CloseCurrentPath(panel->HWindow, FALSE, FALSE, FALSE, TRUE, FALSE)", root_restore)
        self.assertIn("panel->SetPath(rootPath.c_str())", root_restore)

        restore_config = body("mainwnd2.cpp", "static BOOL RestorePanelPathFromConfig(")
        restricted_restore = restore_config[:restore_config.index("if (RestoreDeniedSavedPanelPath(panel, path)")]
        self.assertIn("if (panel->IsProjectWorkspaceRestricted())", restricted_restore)
        self.assertIn("TryRestoreProjectWorkspaceRoot(panel)", restricted_restore)
        self.assertNotIn("ChangeToRescuePathOrFixedDrive", restricted_restore)

        load_tabs = body("mainwnd2.cpp", "void CMainWindow::LoadPanelConfig(")
        self.assertIn("IsProjectWorkspacePath(savedPath.c_str())", load_tabs)
        self.assertIn("GetProjectWorkspaceRoot().c_str()", load_tabs)

    def test_workspace_state_survives_tab_lifecycle_and_configuration(self):
        save = body("mainwnd2.cpp", "static void SavePanelSettingsToKey(")
        self.assertIn("PANEL_PROJECTROOT_REG", save)
        self.assertIn("PANEL_PROJECTRESTRICTED_REG", save)
        self.assertIn("PANEL_TABPREFIXW_REG", save)

        load = body("mainwnd2.cpp", "static void LoadPanelSettingsFromKey(")
        self.assertIn("SetProjectWorkspace(root.data(), restrictedValue != 0)", load)
        self.assertIn("PANEL_TABPREFIXW_REG", load)

        duplicate = body("mainwnd3.cpp", "CFilesWindow* CMainWindow::CreateDuplicatePanelTab(")
        self.assertIn("newPanel->SetProjectWorkspace(sourcePanel->GetProjectWorkspaceRoot().c_str()", duplicate)

        remember = body("mainwnd3.cpp", "void CMainWindow::RememberClosedTab(")
        self.assertIn("info.ProjectWorkspaceRoot = panel->GetProjectWorkspaceRoot()", remember)

        reopen = body("mainwnd3.cpp", "bool CMainWindow::CommandReopenClosedTab(")
        self.assertIn("panel->SetProjectWorkspace(entry.ProjectWorkspaceRoot.c_str()", reopen)

    def test_project_workspace_badge_is_distinct_from_generic_tab_lock(self):
        display = body("mainwnd3.cpp", "static std::wstring BuildTabDisplayText(")
        self.assertIn('else if (panel->IsTabLocked())', display)
        self.assertIn('result = L"\\U0001F512";', display)
        self.assertIn('if (panel->IsProjectWorkspaceRestricted())', display)
        self.assertIn('result += L"\\U0001F4BC";', display)

    def test_workspace_strings_are_localized_in_every_language(self):
        translations = ROOT.parent / "translations"
        string_ids = (15120, 15121, 15122, 15123, 15124, 15125, 15126)
        files = sorted(translations.glob("*/salamand.slt"))
        self.assertGreater(len(files), 0)
        for translation in files:
            text = translation.read_text(encoding="utf-8-sig")
            for string_id in string_ids:
                self.assertRegex(text, rf"(?m)^{string_id},1,\"")


if __name__ == "__main__":
    unittest.main()
