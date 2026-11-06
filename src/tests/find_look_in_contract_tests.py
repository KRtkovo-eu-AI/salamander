# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-05 | Add Find Look in HistoryComboBox contracts. | Guard against Unicode combo history overwriting the current panel path. | https://github.com/KRtkovo-eu-AI/salamander/issues/743 | Composer |
"""Source contracts for Find Look in following the current panel path (#743)."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]


def section(text: str, start: str, end: str) -> str:
    begin = text.index(start)
    return text[begin : text.index(end, begin + len(start))]


class FindLookInContracts(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.viewer = (ROOT / "src" / "viewer.cpp").read_text(encoding="utf-8-sig")
        cls.finddlg1 = (ROOT / "src" / "finddlg1.cpp").read_text(encoding="utf-8-sig")
        cls.fileswn5 = (ROOT / "src" / "fileswn5.cpp").read_text(encoding="utf-8-sig")

    def test_find_file_passes_current_panel_path(self):
        find_file = section(
            self.fileswn5,
            "void CFilesWindow::FindFile()",
            "void CFilesWindow::ViewFile(",
        )
        self.assertIn(
            'OpenFindDialog(MainWindow->HWindow, Is(ptDisk) ? GetPath() : "");',
            find_file,
        )

    def test_find_dialog_seeds_look_in_from_init_path(self):
        ctor = section(
            self.finddlg1,
            "CFindDialog::CFindDialog(HWND hCenterAgainst, const char* initPath)",
            "CFindDialog::~CFindDialog()",
        )
        self.assertIn("if (Data.LookInText[0] == 0)", ctor)
        self.assertIn("const char* s = initPath;", ctor)

    def test_history_combo_sets_text_on_combo_hwnd(self):
        setter = section(
            self.viewer,
            "static void SetHistoryControlTextUtf8(HWND ctrl, const char* text)",
            "static void AddHistoryComboString(HWND hwnd, const char* text)",
        )
        self.assertIn("SendMessageW(ctrl, WM_SETTEXT, 0, (LPARAM)wide.c_str());", setter)
        self.assertNotIn("SetWindowTextW(target,", setter)
        self.assertNotIn("SetWindowTextW(edit,", setter)

    def test_history_combo_reapplies_text_after_addstring(self):
        history = section(
            self.viewer,
            "void HistoryComboBox(HWND hWindow, CTransferInfo& ti, int ctrlID, char* Text,",
            "void DoHexValidation(HWND edit, const int textLen)",
        )
        add_pos = history.find("AddHistoryComboString(hwnd, history[i]);")
        reapply_pos = history.find("SetHistoryControlTextUtf8(hwnd, Text);", add_pos)
        self.assertGreater(add_pos, 0, "History list must use Unicode-aware CB_ADDSTRING")
        self.assertGreater(
            reapply_pos,
            add_pos,
            "Transferred text must be re-applied after CB_ADDSTRING (#743)",
        )


if __name__ == "__main__":
    unittest.main()
