# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Exercise protected consent with asynchronous Find publication and cancellation | Integrate UAC with current upstream behavior. | #731 | Codex |
# | 2026-11-07 | Check asynchronous Stop and deferred Find close. | Keep cancellation and worker completion distinct. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | GPT-6 (Codex) |
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
            "GrepData.Log = &Log;", "GrepData.RefreshPending = FALSE;",
            "SetTimer(HWindow, IDT_REPAINT, 100, NULL)", "CreateThread(")
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
    ordered(grep, "data->SearchStopped = data->StopSearch;", "RequestFindResultsRefresh(data);")
    require("IDC_FIND_STOP" not in grep,
            "the worker must not send the same command as the user's Stop button")

    timer = function_body(finddlg1, "if (wParam == IDT_REPAINT)")
    ordered(timer, "WaitForSingleObject(GrepThread, 0) == WAIT_OBJECT_0", "FinishSearch();")
    require(re.search(r"if \(SearchInProgress && InterlockedExchange\(&GrepData.RefreshPending, FALSE\)\)"
                      r"\s+UpdateListViewItems\(\);", timer) is not None,
            "an active search must consume pending work before each timed publication")
    stop = function_body(finddlg1, "void CFindDialog::StopSearch()")
    ordered(stop, "SearchStopRequested = true;", "GrepData.StopSearch = true;",
            "SetFindActivity(CFindActivity::Stopping);")
    require(all(code not in stop for code in ("WaitForSingleObject", "PeekMessage", "FinishSearch")),
            "Stop must return without waiting or pumping a nested message loop")
    finish = function_body(finddlg1, "void CFindDialog::FinishSearch()")
    ordered(finish, "CloseHandle(GrepThread)", "GrepThread = NULL;",
            "InterlockedExchange(&GrepData.RefreshPending, FALSE);", "KillTimer(HWindow, IDT_REPAINT);",
            "UpdateListViewItems();", "OnShowLog();", "EnableControls();", "OnFindWorkIdle();")
    require(re.search(r"^    UpdateListViewItems\(\);$", finish, re.MULTILINE) is not None,
            "completion and cancellation must publish the joined worker's final state even without a pending tick")
    close = function_body(finddlg1, "case WM_USER_CLOSEFIND:")
    ordered(close, "GrepThread != NULL || FinishingSearch || IsSortingFindResults()",
            "CloseWhenFindIdle = true;", "return 0;", "DestroyWindow(HWindow);")

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

    require(start.index("GrepData.StopEvent = HANDLES(CreateEvent(") < start.index("CreateThread("),
            "Find must prepare the broker cancellation event before starting the worker")
    stop_search = function_body(finddlg1, "void CFindDialog::StopSearch()")
    require("SetEvent(GrepData.StopEvent);" in stop_search and "CloseHandle(GrepData.StopEvent)" not in stop_search,
            "Stop must signal cancellation and leave the event owned until the worker exits")
    require(finish.index("CloseHandle(GrepThread)") < finish.index("CloseHandle(GrepData.StopEvent)"),
            "completion must retain the cancellation event until the worker has joined")
    require("request.session.Start(request.data->ConsentWindow, request.data->StopEvent)" in find_cpp,
            "the protected Find launcher must pass the search cancellation event to the broker handshake")

    broker = (ROOT / "src" / "uacbroker" / "uacbroker.cpp").read_text(encoding="utf-8-sig")
    broker_launch = function_body(broker, "BOOL LaunchBrokerWithOwner(")
    require(broker_launch.index("GetForegroundWindow()") < broker_launch.index("ShellExecuteExW(&launch)") <
            broker_launch.index("SetForegroundWindow(owner)"),
            "foreground restoration must be scoped to the caller that was active before consent")
    require("foregroundProcess == GetCurrentProcessId()" in broker_launch and "IsWindowEnabled(owner)" in broker_launch and
            "!IsIconic(owner)" in broker_launch and "SetLastError(error)" in broker_launch,
            "restoration must target a usable owner and preserve the Windows launch error")
    listing_transfer = function_body(broker, "DWORD ListingSession::Transfer(")
    require("TransferRequest(pipe, buffer, length, send, deadline, nullptr, nullptr, nullptr, stopEvent)" in listing_transfer,
            "active listing requests must use local cancellation without a transfer cancellation byte")
    for name in ("SendPath", "OpenContent", "ReadContent", "CloseContent", "List"):
        method = function_body(broker, f"DWORD ListingSession::{name}(")
        require("Transfer(" in method and "TransferRequest(" not in method and "Close();" in method,
                f"cancelled {name} requests must release the read session")
    require("if (error == ERROR_CANCELLED && data->StopSearch)" in find_cpp,
            "Stop must not report a protected scope as skipped by an elevation decision")

    require(grep.index("RequestFindResultsRefresh(data);") < grep.index("protectedQueue.Finish(duplicateCandidates);"),
            "ordinary hits must be published before waiting for administrator consent")
    require(grep.index("protectedQueue.Finish(duplicateCandidates);") <
            grep.index("duplicateCandidates->Examine(data, protectedSessions, protectedQueue.Remaining());"),
            "protected candidates must join ordinary candidates before grouping and hashing")
    queue_add = function_body(find_cpp, "bool AddPath(CGrepData* search, std::wstring path")
    require("count == 0 && !request->Start()" in queue_add and "count == requests.size()" in queue_add,
            "only the first consent may begin during traversal and the queue must be bounded")
    request_finish = function_body(find_cpp, "UacF7::ListingSession* Finish(CDuplicateCandidates* candidates)")
    require("data->StopSearch || !Start()" in request_finish and
            request_finish.index("WaitForSingleObject(launchThread, INFINITE);") < request_finish.index("Scan(path, 0,"),
            "a stopped search must suppress queued prompts and traversal must wait for the approved session")
    match_file = function_body(find_cpp, "bool MatchFile(")
    require(match_file.index("session.OpenContent(") < match_file.index("predicates.Match(") <
            match_file.index("session.CloseContent()") and "return match && !error && !closeError" in match_file,
            "protected text and tags must use the same open and require a confirmed close")
    predicate = function_body(find_cpp, "bool Match(CGrepData* data, UacF7::ListingSession& session")
    require(predicate.index("MatchTags(") < predicate.index("MatchContent("),
            "combined protected predicates must evaluate both filters on the supplied session")
    for signature in ("bool MatchTags(", "bool MatchContent("):
        matcher = function_body(find_cpp, signature)
        require("session.OpenContent(" not in matcher and "session.CloseContent(" not in matcher,
                "a predicate must not reopen the file within a combined protected match")
    examine = function_body(find_cpp, "void CDuplicateCandidates::Examine(")
    require("file->ProtectedScope" in examine and "protectedSessions[scope - 1]" in examine,
            "protected duplicate reads must use the exact session that supplied the candidate")
    require("if (scope && (!session || !session->IsOpen()))" in examine and
            examine.index("if (scope && (!session || !session->IsOpen()))") < examine.index("GetMD5Digest("),
            "candidates from a disconnected scope must not be read or reauthorized")
    scan = function_body(find_cpp, "void Scan(const std::wstring& directory")
    require(scan.count("data->StopSearch || !session.IsOpen()") == 2 and
            "return session.IsOpen() && !data->Refine" in request_finish,
            "a lost session must end its traversal without discarding other scopes")
    digest = function_body(find_cpp, "BOOL CDuplicateCandidates::GetMD5Digest(")
    candidate = function_body(find_cpp, "bool MatchesCandidate(")
    require(candidate.index("FindDuplicateContent(*data)") < candidate.index("return MatchFile(") and
            "if ((candidates && FindDuplicateContent(*data))" in candidate,
            "protected content duplicate candidates must defer filters until after metadata grouping")
    require(digest.count("protectedSession->OpenContent(") == 1 and
            digest.index("protectedSession->OpenContent(") < digest.index("predicates.Match(") <
            digest.index("protectedSession->ReadContent(offset, buffer, read)") <
            digest.index("protectedSession->CloseContent()") and "while (match &&" in digest and
            digest.index("protectedSession->CloseContent()") < digest.index("if (!match)"),
            "protected duplicate filtering and hashing must use one open and close even on a nonmatch")
    require("protectedSession->ReadContent(offset, buffer, read)" in digest and
            "protectedSession->CloseContent()" in digest and
            digest.index("protectedSession->CloseContent()") < digest.index("context.finalize()"),
            "protected duplicate digests must use and validate the authorized content stream")
    require("AuthorizeFindFile(data, fullPath, singleFile)" in digest and
            "remainingFileConsents" in digest,
            "a denied ordinary duplicate read must have a bounded exact-file fallback")
    require("std::string(GetErrorText(error))" in digest and "IDS_FINDLOG_SKIP" not in digest,
            "a declined duplicate file must log cancellation, not a skipped directory")
    protected_log = function_body(find_cpp, "void Log(const std::wstring& entryPath, DWORD error, bool isFile = false)")
    require("error == ERROR_CANCELLED && !isFile ? LoadStr(IDS_FINDLOG_SKIP)" in protected_log and
            "Log(path, consent->result, singleFile);" in request_finish,
            "a declined protected request must distinguish an exact file from a directory")
    require("offset < size" in digest and "error = ERROR_HANDLE_EOF;" in digest and
            "current.ChangeTime.QuadPart != initial.ChangeTime.QuadPart" in digest,
            "a local digest must cover the listed length and reject a changed file")
    compare = examine[examine.index("for (i = Count - 1; i >= 0; i--)"):]
    before_digest = compare[:compare.index("GetMD5Digest(")]
    require("file->Size" not in before_digest and "if (file->Group)" in digest,
            "empty candidates must be verified without dereferencing a nonexistent digest")
    traversal = function_body(find_cpp, "void SearchDirectory(")
    require("openDenied && !protectedQueue.Add(" in traversal and
            "ignoreList, &file)" in traversal,
            "a denied ordinary text or tag read must queue a single file")
    refine = function_body(find_cpp, "void RefineData(")
    require("refineData->GetFullNameW()" in refine and
            "protectedQueue.AddRefined(data, fullPath, masksGroup, *refineData)" in refine and
            "apiPath.c_str()" in refine,
            "protected refinement must keep the original wide file path")
    require(refine.index("RequestFindResultsRefresh(data);") < refine.index("protectedQueue.Finish(nullptr);"),
            "ordinary refined rows must be published before waiting for the protected checks")
    require("if (singleFile && data->Refine == 2)" in request_finish and
            "PublishSingleFile(nullptr, false);" in request_finish,
            "declining a protected subtraction check must retain the existing row")
    refine_mode = function_body(find_cpp, "bool IncludeRefinedFindResult(")
    require("(mode == 1 && match) || (mode == 2 && !match)" in refine_mode,
            "refine modes must distinguish a proved match from a non-match")
    consent = function_body(finddlg1, "case WM_USER_FIND_PROTECTED_CONSENT:")
    require("IDS_UAC_FIND_FILE_CONFIRM" in consent, "an exact-file grant needs its own prompt")
    require("SetWindowLongPtr(HWindow, DWLP_MSGRESULT, approved);" in consent and "return TRUE;" in consent,
            "Find is a dialog and must return approval through DWLP_MSGRESULT")

    print("find_search_schedule_contract_tests: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
