# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-07 | Check timed Find batches and owned worker logs. | Protect final publication, count updates and dialog lifetime without requiring a GUI. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | GPT-6 (Codex) |
# | 2026-11-07 | Add Find search scheduling contracts. | Prevent large Find searches from blocking on synchronous result-list refreshes. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | OpenAI GPT-5.6 Luna Pro |

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def ordered(text: str, *fragments: str) -> None:
    cursor = 0
    for fragment in fragments:
        position = text.find(fragment, cursor)
        require(position >= 0, f"Missing or out of order: {fragment}")
        cursor = position + len(fragment)


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


def main() -> int:
    find_h = (ROOT / "src" / "find.h").read_text(encoding="utf-8-sig")
    find_cpp = (ROOT / "src" / "find.cpp").read_text(encoding="utf-8-sig")
    finddlg1 = (ROOT / "src" / "finddlg1.cpp").read_text(encoding="utf-8-sig")
    finddlg2 = (ROOT / "src" / "finddlg2.cpp").read_text(encoding="utf-8-sig")

    require("volatile LONG RefreshPending;" in find_h,
            "Find results and log updates must share an atomic pending flag")
    for obsolete_message in ("WM_USER_ADDFILE", "WM_USER_ADDLOG"):
        require(all(obsolete_message not in text for text in (find_h, find_cpp, finddlg1)),
                f"Find must not deliver per-update GUI messages through {obsolete_message}")
    require("FoundVisibleCount" not in find_cpp and "FoundVisibleTick" not in find_cpp,
            "the worker must not read the dialog's publication state")

    start = function_body(finddlg1, "void CFindDialog::StartSearch(WORD command)")
    ordered(start, "GrepData.FoundVisibleCount = 0;",
            "ListView_SetItemCount(FoundFilesListView->HWindow, 0);", "Log.Clean();", "UpdateListViewItems();",
            "GrepData.Log = &Log;", "GrepData.RefreshPending = FALSE;", "CreateThread(",
            "SetTimer(HWindow, IDT_REPAINT, 100, NULL);")
    require(start.count("GrepData.FoundVisibleCount = 0;") == 1,
            "Append must retain the count republished before the worker starts")

    request = function_body(find_cpp, "static void RequestFindResultsRefresh(CGrepData* data)")
    require("InterlockedExchange(&data->RefreshPending, TRUE);" in request,
            "successful worker additions must mark a pending GUI publication")
    require("SendMessage" not in request and "PostMessage" not in request,
            "marking a result batch must not enter or flood the dialog message queue")
    add_found = function_body(find_cpp, "BOOL AddFoundItem(")
    ordered(add_found, "data->FoundFilesListView->Add(foundData);", "RequestFindResultsRefresh(data);")
    grep = function_body(find_cpp, "unsigned GrepThreadFBody(void* ptr)")
    ordered(grep, "data->SearchStopped = data->StopSearch;", "RequestFindResultsRefresh(data);",
            "PostMessage(data->HWindow, WM_COMMAND, IDC_FIND_STOP, 0);")

    timer = function_body(finddlg1, "if (wParam == IDT_REPAINT)")
    require(re.search(r"if \(SearchInProgress && InterlockedExchange\(&GrepData.RefreshPending, FALSE\)\)"
                      r"\s+UpdateListViewItems\(\);", timer) is not None,
            "an active search must consume pending work before each timed publication")
    stop = function_body(finddlg1, "void CFindDialog::StopSearch()")
    ordered(stop, "GrepData.StopSearch = TRUE;", "WaitForSingleObject(GrepThread, 100)",
            "CloseHandle(GrepThread)", "GrepThread = NULL;",
            "InterlockedExchange(&GrepData.RefreshPending, FALSE);", "KillTimer(HWindow, IDT_REPAINT);",
            "UpdateListViewItems();", "OnShowLog();", "EnableControls();")
    require(re.search(r"^    UpdateListViewItems\(\);$", stop, re.MULTILINE) is not None,
            "completion and cancellation must publish the joined worker's final state even without a pending tick")

    update = function_body(finddlg1, "void CFindDialog::UpdateListViewItems()")
    ordered(update, "count != GrepData.FoundVisibleCount", "if (countChanged)")
    count_update = function_body(update, "if (countChanged)")
    ordered(count_update, "ListView_SetItemCountEx(", "LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL",
            "GrepData.FoundVisibleCount == 0 && count > 0", "LVIS_FOCUSED | LVIS_SELECTED",
            "GrepData.FoundVisibleCount = count;")
    require(update.count("ListView_SetItemCountEx(") == 1,
            "an unchanged count must not be republished to the list view")
    log_header = "TBHeader->SetErrorsInfosCount(Log.GetErrorCount(), Log.GetInfoCount());"
    require(log_header in update and log_header not in count_update,
            "a log-only batch must still update the error and information counts")

    add_log = function_body(find_cpp, "static void AddFindLog(")
    ordered(add_log, "data->Log->Add(flags, text, path);", "InterlockedExchange(&data->RefreshPending, TRUE);")
    require("SendMessage" not in add_log and "PostMessage" not in add_log,
            "the worker must copy its log payload without calling the GUI")
    require(find_cpp.count("AddFindLog(data, log.Flags, log.Text, log.Path);") >= 12,
            "all twelve existing Find worker log routes must retain owned log delivery")
    show_log = function_body(finddlg2, "void CFindDialog::OnShowLog()")
    ordered(show_log, "CFindLog snapshot;", "Log.CopySnapshotTo(snapshot)",
            "CFindLogDialog dlg(HWindow, &snapshot);", "dlg.Execute();")

    print("find_search_schedule_contract_tests: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
