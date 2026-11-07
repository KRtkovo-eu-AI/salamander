# SPDX-FileCopyrightText: 2026 Samandarin Contributors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Integration boundaries that must continue to use Branch View item identity."""
from pathlib import Path
import unittest

SRC = Path(__file__).resolve().parents[1]
def source(name):
    return (SRC / name).read_text(encoding="utf-8-sig")
def between(text, start, end):
    return text.split(start, 1)[1].split(end, 1)[0]

class FileActionContractTests(unittest.TestCase):
    def test_per_item_user_menu_directory(self):
        callback = between(source("mainwnd4.cpp"), "BOOL GetNextFileFromPanel(", "// ExpandCommand2")
        self.assertEqual(callback.count("GetItemDirectoryW(*f)"), 2)
        self.assertNotIn("strcpy(name, f->Name)", callback)

    def test_selected_and_compared_names_use_exact_paths(self):
        command = between(source("mainwnd3.cpp"), "if (LOWORD(wParam) >= CM_USERMENU_MIN", "case CM_ACTIVESELECTALL:")
        self.assertIn("GetItemFullPathW", command)
        self.assertNotIn("SalPathAppend(fullName", command)
        self.assertNotIn("SalPathAppend(userMenuAdvancedData.CompareName", command)
        self.assertIn("CUserMenuAdvancedData::PathCapacity", command)

    def test_expansion_has_heap_path_capacity(self):
        expansion = between(source("mainwnd4.cpp"), "BOOL ExpandCommand2(", "void CMainWindow::UserMenu(")
        self.assertNotIn("[MAX_PATH]", expansion)
        self.assertIn("FileActionPaths::ShortPath(fileName)", expansion)
        self.assertIn("Normalize(item->Arguments)", expansion)
        self.assertIn("Normalize(item->InitDir)", expansion)
        scratch = between(source("execute.cpp"), "struct CExecuteExpData", "const char* WINAPI ExecuteExpDrive")
        self.assertIn("Storage(3 * SAL_MAX_PATH)", scratch)

    def test_external_editor_and_viewer_are_wide(self):
        text = source("fileswn5.cpp")
        self.assertEqual(text.count("FileActionPaths::ShortPath(name)"), 2)
        self.assertNotIn("GetShortPathName(name", text)
        self.assertIn("FileActionPaths::LaunchProcess(cmdLine, currentDir", text)
        for owner in ("editor", "viewer"):
            for field in ("Command", "Arguments", "InitDir"):
                self.assertIn(f"Normalize({owner}->{field})", text)

    def test_batch_does_not_execute_after_failed_directory_change(self):
        menu = between(source("mainwnd4.cpp"), "void CMainWindow::UserMenu(", "void CMainWindow::SetDefaultDirectories")
        self.assertIn("ShellExecuteExW(&sei)", menu)
        self.assertIn("@chcp 65001", menu)
        self.assertIn("|| exit /b 1", menu)
        self.assertNotIn("CharToOem", menu)
        self.assertIn("FileActionPaths::LaunchProcess", menu)

    def test_compare_dialog_keeps_unicode_long_paths(self):
        dialog = between(source("dialogs2.cpp"), "void CCompareArgsDlg::Validate", "CCompareArgsDlg::DialogProc")
        self.assertNotIn("MAX_PATH]", dialog)
        self.assertIn("ti.EditLineW", dialog)
        self.assertIn("std::vector<wchar_t> wide(SAL_MAX_PATH", dialog)
        self.assertIn("FileActionPaths::Utf8", dialog)

    def test_command_line_overflow_fails_before_launch(self):
        helper = source("fileactionpath.h")
        self.assertIn("commandW.size() >= 32767", helper)
        self.assertIn("ERROR_FILENAME_EXCED_RANGE", helper)
        self.assertIn("CreateProcessW", helper)
        self.assertIn("GetShortPathNameW", helper)

if __name__ == "__main__":
    unittest.main()
