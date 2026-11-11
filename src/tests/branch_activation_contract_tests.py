# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Keep active approved Branch scans on the short poll interval. | Accept the shared preparation timer without blocking activation. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Find Branch View translations by resource ID. | Section ordinals can shift when new string blocks are added. | #731 | Codex |
# | 2026-11-06 | Verify Branch copy-dialog controls in every language archive. | Missing checkbox rows prevented native Translator import despite complete string tables. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
"""Branch View activation must not create work or wait for background providers.

These integration guards complement scanner schedule tests and GUI activation
measurements. They deliberately preserve normal disk/archive activation behavior.
"""
from pathlib import Path
import importlib.util
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


def source(name):
    return (ROOT / name).read_text(encoding="utf-8-sig")


def block(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth = 1
    index = opening + 1
    while depth:
        depth += (text[index] == "{") - (text[index] == "}")
        index += 1
    return text[start:index]


class BranchActivationContracts(unittest.TestCase):
    def activation(self):
        return block(source("fileswn6.cpp"), "void CFilesWindow::Activate(")

    def branch_activation(self):
        return block(self.activation(), "if (IsBranchView())")

    def test_focus_does_not_probe_root_or_request_scan(self):
        branch = self.branch_activation()
        for forbidden in ("CheckPath(", "RefreshDiskFreeSpace(", "RefreshDirectory(",
                          "RefreshBranchView(", "WM_USER_REFRESH_DIR,", "WM_USER_REFRESH_DIR_EX,"):
            self.assertNotIn(forbidden, branch)
        self.assertTrue(branch.rstrip().endswith("return;\n    }"))
        activation = self.activation()
        self.assertLess(activation.index("if (IsBranchView())"), activation.index("CheckPath(FALSE)"))

    def test_actual_deferred_notification_keeps_automatic_message_and_timestamp(self):
        pending = block(self.branch_activation(), "if (InactiveRefreshTimerSet)")
        expected = ("KillTimer(HWindow, IDT_INACTIVEREFRESH)",
                    "InactiveRefreshTimerSet = FALSE",
                    "PostMessage(HWindow, WM_USER_INACTREFRESH_DIR, FALSE, InactRefreshLParam)")
        positions = [pending.index(value) for value in expected]
        self.assertEqual(positions, sorted(positions))
        dispatch = source("fileswnb.cpp")
        self.assertIn("const BOOL automatic = uMsg != WM_USER_REFRESH_DIR || wParam != 0", dispatch)
        self.assertIn("RefreshBranchView(automatic)", dispatch)

    def test_icon_resume_does_not_wait_on_provider(self):
        branch = self.branch_activation()
        resume = block(branch, "if (InactWinOptimizedReading)")
        self.assertIn("InactWinOptimizedReading = FALSE", resume)
        self.assertIn("WakeupIconCacheThread()", resume)
        for forbidden in ("SleepIconCacheThread(", "WaitFor", "EnterCriticalSection", "Invalidate"):
            self.assertNotIn(forbidden, branch)
        wake = block(source("fileswn1.cpp"), "void CFilesWindow::WakeupIconCacheThread()")
        self.assertIn("SetEvent(ICEventWork)", wake)
        self.assertNotIn("EnterCriticalSection", wake)

    def test_background_thumbnail_work_does_not_announce_branch_rescan(self):
        dispatch = source("fileswnb.cpp")
        begin = block(dispatch, "case WM_USER_ICONREADING_BEGIN:")
        self.assertIn("if (!IsBranchView() && UseThumbnails && DirectoryLine != NULL)", begin)
        self.assertIn("DirectoryLine->SetThrobber(TRUE, 150)", begin)
        self.assertIn("setWait = !IsBranchView() && ShouldShowWaitCursorForRefresh()", dispatch)
        end = dispatch[dispatch.index("case WM_USER_ICONREADING_END:"):]
        self.assertIn("DirectoryLine->SetThrobber(FALSE)", end)

    def test_preparing_keeps_stop_and_escape_available(self):
        scanning = block(source("branch_view.cpp"), "BOOL CFilesWindow::IsBranchViewScanning()")
        self.assertIn("BranchView->Status.Running", scanning)
        self.assertIn("BranchView->Preparing", scanning)
        menu = source("mainwnd3.cpp")
        self.assertIn("popup->EnableItem(CM_BRANCH_STOP, FALSE, branchPanel != NULL && branchPanel->IsBranchViewScanning())", menu)
        stop = menu[menu.index("case CM_BRANCH_STOP:"):menu.index("case CM_CHANGEFILTER:", menu.index("case CM_BRANCH_STOP:"))]
        self.assertIn("activePanel->CancelBranchViewScan()", stop)
        keys = source("fileswn0.cpp")
        self.assertIn("wParam == VK_ESCAPE && IsBranchViewScanning()", keys)

    def test_preparing_status_precedes_ready_and_cancel_drops_pending_work(self):
        status = block(source("branch_view.cpp"), "std::wstring CFilesWindow::GetBranchViewStatusW() const")
        self.assertIn("BranchView->Preparing", status)
        self.assertLess(status.index("IDS_BRANCH_PREPARING"), status.index("IDS_BRANCH_READY"))
        cancel = block(source("branch_view.cpp"), "void CFilesWindow::CancelBranchViewScan()")
        for required in ("BranchView->Preparing = false", "BranchView->Status.Cancelled = true",
                         "BranchView->IncomingEntries.clear()", "BranchView->Refresh.Cancel()"):
            self.assertIn(required, cancel)
        self.assertNotIn("BranchView->Entries.clear()", cancel)
        poll = block(source("branch_view.cpp"), "void CFilesWindow::PollBranchView()")
        self.assertLess(poll.index("if (BranchView->Status.Cancelled)"), poll.index("BranchView->Worker.Drain"))

    def test_scan_completion_waits_for_preparation_and_final_publication(self):
        poll = block(source("branch_view.cpp"), "void CFilesWindow::PollBranchView()")
        self.assertIn("if (scanFinished) BranchView->CompletionPending = true", poll)
        self.assertIn("const bool preparing = BranchView->CompletionPending && preparation->Queue.HasPending()", poll)
        self.assertIn("const bool finished = BranchView->CompletionPending && !preparation->Queue.HasPending()", poll)
        self.assertIn("BranchView->Dirty && !preparation->Queue.HasPending()", poll)
        self.assertLess(poll.index("RefreshDirectory();"), poll.index("BranchView->Refresh.Finished("))
        self.assertLess(poll.index("BranchView->CompletionPending = false"), poll.index("BranchView->Refresh.Finished("))
        self.assertLess(poll.index("BranchView->Refresh.Finished("), poll.index("BranchView->Refresh.Due("))
        self.assertIn("if (!preparation->Alive) return", poll)
        self.assertIn("preparation->Queue.HasPending() ||", poll)
        self.assertIn("(BranchView->UseProtectedListing && BranchView->Status.Running) ? 15 : 200", poll)
        cancel = block(source("branch_view.cpp"), "void CFilesWindow::CancelBranchViewScan()")
        self.assertIn("BranchView->CompletionPending = false", cancel)
        self.assertIn("BranchView->Preparation->Alive = false", cancel)

    def test_all_branch_status_translations_match_resources_and_format_arguments(self):
        parser_path = ROOT.parent / "tools/localization/verify_slt_roundtrip.py"
        spec = importlib.util.spec_from_file_location("branch_status_slt", parser_path)
        parser = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(parser)
        ids = {name: int(value) for name, value in re.findall(
            r"#define\s+(IDS_(?:MENU_)?BRANCH_\w+)\s+(\d+)", source("texts.rh2"))}
        english = {ids[name]: value for name, value in re.findall(
            r'\b(IDS_(?:MENU_)?BRANCH_\w+),\s*"([^"\r\n]*)"', source("lang/texts.rc2"))}
        self.assertEqual(set(english), set(range(14500, 14516)))
        self.assertEqual(english[14515].count("%I64u"), 2)
        paths = sorted((ROOT.parent / "translations").glob("*/salamand.slt"))
        self.assertEqual(len(paths), 11)
        for path in paths:
            with self.subTest(language=path.parent.name):
                raw = path.read_bytes()
                self.assertTrue(raw.startswith(b"\xef\xbb\xbf"))
                self.assertEqual(raw.count(b"\n"), raw.count(b"\r\n"))
                parsed = parser.parse(path)
                for sid, original in english.items():
                    entries = [value for key, value in parsed.items()
                               if re.fullmatch(rf"STRINGTABLE \d+:{sid},", key)]
                    self.assertEqual(len(entries), 1, f"expected one string resource {sid}")
                    line, translated = entries[0]
                    self.assertEqual(re.findall(r"%(?:I64)?[a-zA-Z]", original),
                                     re.findall(r"%(?:I64)?[a-zA-Z]", translated))
                    self.assertEqual(original.count("&"), translated.count("&"))
                    self.assertEqual(original.count(r"\t"), translated.count(r"\t"))
                    self.assertFalse(parser.validate_text(path, line, translated))
                headers = [int(value) for value in re.findall(r"^\[STRINGTABLE (\d+)\]$",
                    raw.decode("utf-8-sig").replace("\r\n", "\n"), re.MULTILINE)]
                self.assertEqual(headers, list(range(len(headers))))
                # New resource blocks can shift the ordinal of the final two tables.
                for table, sid in zip(headers[-2:], (14991, 14992)):
                    key = f"STRINGTABLE {table}:{sid},"
                    self.assertTrue(key in parsed, f"missing {key} in {path.name}")

    def test_copy_dialog_branch_placeholders_match_native_resources(self):
        resources = source("lang/lang.rc")
        dialogs = (("IDD_COPYMOVEMOREDIALOG", 205), ("IDD_COPYTOSELECTEDDIRSDIALOG", 8201))
        # The visible caption uses the already localized string table.
        self.assertIn("SetDlgItemText(HWindow, IDC_BRANCH_KEEP_PATHS, LoadStr(IDS_BRANCH_KEEP_PATHS))", source("dialogs3.cpp"))
        for symbol, dialog_id in dialogs:
            native = re.search(r"\b" + symbol + r" DIALOGEX [^\n]*\n(.*?)\nEND", resources, re.S).group(1)
            geometry = re.search(r'CONTROL\s+"",IDC_BRANCH_KEEP_PATHS,"Button",[^,\n]+,(\d+),(\d+),(\d+),(\d+)', native)
            self.assertIsNotNone(geometry)
            expected = tuple(map(int, geometry.groups()))
            for path in sorted((ROOT.parent / "translations").glob("*/salamand.slt")):
                with self.subTest(language=path.parent.name, dialog=dialog_id):
                    text = path.read_text(encoding="utf-8-sig")
                    dialog = text.split("[DIALOG " + str(dialog_id) + "]\n", 1)[1].split("\n[", 1)[0]
                    rows = re.findall(r'^14510,(\d+),(\d+),(\d+),(\d+),1,""$', dialog, re.M)
                    self.assertEqual(len(rows), 1)
                    self.assertEqual(tuple(map(int, rows[0])), expected)
                    separator = re.search(r'^221,\d+,(\d+),', dialog, re.M)
                    self.assertIsNotNone(separator)
                    self.assertGreaterEqual(int(separator[1]), expected[1] + expected[3])
                    dialog_height = int(dialog.strip().splitlines()[0].split(",")[1])
                    self.assertGreaterEqual(dialog_height, expected[1] + expected[3])

    def test_ordinary_activation_still_has_existing_refresh_policy(self):
        activation = self.activation()
        ordinary = activation[activation.index("BOOL needToRefreshIcons") :]
        for preserved in ("CheckPath(FALSE)", "RefreshDiskFreeSpace(FALSE, TRUE)",
                          "Configuration.DrvSpecRemoteDoNotRefreshOnAct", "WM_USER_REFRESH_DIR_EX",
                          "WM_USER_REFRESH_PLUGINFS", "SleepIconCacheThread()"):
            self.assertIn(preserved, ordinary)


if __name__ == "__main__":
    unittest.main()
