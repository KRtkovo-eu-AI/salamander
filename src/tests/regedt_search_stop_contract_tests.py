# SPDX-FileCopyrightText: 2026 Open Salamander Authors
# SPDX-License-Identifier: GPL-2.0-or-later

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
FINDDLG = SRC / "plugins" / "regedt" / "finddlg.cpp"
FINDDLG2 = SRC / "plugins" / "regedt" / "finddlg2.cpp"
FINDDLG_H = SRC / "plugins" / "regedt" / "finddlg.h"
SEARCH_HELP = SRC / "plugins" / "regedt" / "help" / "hh" / "regedt" / "dlgboxes_search.htm"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def function_body(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace : index + 1]
    raise AssertionError("unterminated function: " + signature)


def assert_no_paint_under_lock(func: str, name: str) -> None:
    pos = 0
    found_lock = False
    while True:
        enter = func.find("Section.Enter();", pos)
        if enter < 0:
            require(found_lock, name + " does not take Section")
            return
        found_lock = True
        leave = func.find("Section.Leave();", enter)
        require(leave > enter, name + " takes Section without leaving it")
        body = func[enter:leave]
        require("UpdateText(" not in body, name + " calls UpdateText while holding Section")
        require("SendMessage" not in body, name + " sends a window message while holding Section")
        pos = leave + 1


def main() -> int:
    finddlg = FINDDLG.read_text(encoding="utf-8")
    finddlg2 = FINDDLG2.read_text(encoding="utf-8")
    finddlg_h = FINDDLG_H.read_text(encoding="utf-8")
    help_text = SEARCH_HELP.read_text(encoding="utf-8")

    set_base = function_body(finddlg, "void CStatusBar::SetBase(LPCWSTR text, BOOL updateInIdle)")
    set_text = function_body(finddlg, "void CStatusBar::Set(LPCWSTR text, BOOL updateInIdle)")
    update_text = function_body(finddlg, "void CStatusBar::UpdateText()")
    on_idle = function_body(finddlg, "void CStatusBar::OnEnterIdle()")
    scan_aux = function_body(
        finddlg,
        "BOOL CFindThread::ScanKeyAux(int root, LPWSTR key, BOOL& skip, BOOL& skipAllErrors,",
    )
    scan_key = function_body(
        finddlg,
        "BOOL CFindThread::ScanKey(int root, LPWSTR key, BOOL& skip, BOOL& skipAllErrors,",
    )
    update_list = function_body(finddlg2, "void CFindDialog::UpdateListViewItems()")
    start_search = function_body(finddlg2, "void CFindDialog::StartSearch()")
    stop_search = function_body(finddlg2, "void CFindDialog::StopSearch()")
    dialog_proc = function_body(finddlg2, "CFindDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)")

    assert_no_paint_under_lock(set_base, "CStatusBar::SetBase")
    assert_no_paint_under_lock(set_text, "CStatusBar::Set")
    assert_no_paint_under_lock(update_text, "CStatusBar::UpdateText")
    assert_no_paint_under_lock(on_idle, "CStatusBar::OnEnterIdle")

    require("Section.Leave();" in update_text, "UpdateText copies the caption under Section")
    require(
        update_text.find("Section.Leave();") < update_text.find("SendMessageW"),
        "UpdateText must SendMessageW only after leaving Section",
    )
    require("if (paintNow)" in set_base and "UpdateText();" in set_base,
            "SetBase paints only after leaving Section")
    require("if (paintNow)" in set_text and "UpdateText();" in set_text,
            "Set paints only after leaving Section")

    require('StatusBar->Set(key, TRUE)' in scan_aux,
            "registry search must not SendMessage the status bar from the worker thread")
    require("StatusBar->Set(key);" not in scan_aux,
            "registry search still updates the status bar synchronously from the worker")
    require("GetTickCount() + 100" in scan_aux,
            "status-bar dirty flag is not throttled")
    require("WM_USER_ADDFILE" not in scan_aux,
            "search worker still floods the dialog with WM_USER_ADDFILE")
    require("SetBase(base, TRUE)" in scan_key,
            "ScanKey still SendMessages the status-bar base from the worker thread")

    require("if (!SearchInProgress && GetFocus() != List->HWindow)" in update_list,
            "first results must not steal focus from Stop while searching")
    require(
        "SetTimer(HWindow, IDT_REFRESH_LISTVIEW, 100, NULL);" in start_search,
        "search dialog timer must stay responsive during a long registry scan",
    )
    require("SetEvent(CancelEvent);" in stop_search, "StopSearch must signal CancelEvent")
    require("StatusBar->OnEnterIdle();" in dialog_proc,
            "the search timer must paint deferred status-bar text")
    require("case IDOK:" in dialog_proc and "StopSearch();" in dialog_proc,
            "Find Now/Stop must abort an in-progress search")
    require("case WM_CLOSE:" in dialog_proc,
            "closing the search dialog must be handled")
    close_body = dialog_proc[dialog_proc.find("case WM_CLOSE:") : dialog_proc.find("case WM_DESTROY:")]
    require("StopSearch();" in close_body, "WM_CLOSE must abort an in-progress search")
    require("CloseWhenSearchFinishes = TRUE;" in close_body,
            "WM_CLOSE during search must close after the worker stops")
    idok_body = dialog_proc[dialog_proc.find("case IDOK:") : dialog_proc.find("case IDCANCEL:")]
    require("if (SearchInProgress)" in idok_body and "StopSearch();" in idok_body,
            "the Stop button must call StopSearch")

    command_to_notify = dialog_proc[dialog_proc.find("case WM_COMMAND:") : dialog_proc.find("case WM_NOTIFY:")].rstrip()
    require(
        command_to_notify.endswith("break; // do not fall through: WM_COMMAND lParam is a control HWND, not NMHDR\n    }")
        or "break; // do not fall through" in command_to_notify[-200:],
        "WM_COMMAND must not fall through into WM_NOTIFY (Search for combo SETFOCUS would crash)",
    )

    require("callers on a non-UI thread must pass updateInIdle = TRUE" in finddlg_h,
            "CStatusBar documents that worker threads must not paint immediately")
    require("replaced by the Stop button" in help_text,
            "help still documents that search can be stopped")

    print("regedt_search_stop_contract_tests: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
