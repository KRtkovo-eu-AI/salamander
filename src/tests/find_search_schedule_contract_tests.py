# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-10 | Cover exact wide paths and iterative ordinary Find. | Verify wide directory reads, broker routing and UI actions. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Keep Find consent after the synchronous editor dispatch. | Detect modal consent on the wrong UI thread. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Cover protected editor dispatch and the original modal owner. | Guard exact wide paths, application policy and stale Branch View rows. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check Find F3 dispatch and identity lifetime. | Guard the Unicode result and the separate internal viewer consent. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Verify the common immediate broker foreground return. | Size prompts now keep their modal owner alive. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Cover the shared consent return and Shell recycle route. | Keep the disabled-owner snapshot and preserve the existing worker behavior. | #731 | GPT-6.1 Sol |
# | 2026-11-10 | Check activation routing for direct and worker UAC requests. | Keep focus with the invoking window unless the user switched away. | #731 | GPT-6 (Codex) |
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

    view = function_body(finddlg1, "void CFindDialog::OnViewFile(BOOL alternate)")
    ordered(view, "result->GetFullNameW()", "result->FoundView", "viewedIndex, HWindow",
            "WM_USER_VIEWFILE")
    require("MAX_PATH" not in view, "F3 must not truncate the selected Find path")
    publish = function_body(find_cpp, "void PublishCandidate(")
    ordered(publish, "maxViewerSnapshotBytes", "session.PrepareViewer(filePath",
            "AddFoundItem(", "std::move(foundView)")
    host = (ROOT / "src" / "fileswn5.cpp").read_text(encoding="utf-8-sig")
    internal = function_body(host, "BOOL ViewFileInt(")
    ordered(internal, "case VIEWER_INTERNAL:", "GetLastError() == ERROR_ACCESS_DENIED",
            "prepared = foundView", "!prepared && request.PrepareView", "ConfirmProtectedRead(",
            "prepared->ReadViewerSnapshot", "enumFileNamesSourceUID = enumFileNamesLastFileIndex = -1")

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
    require(find_cpp.count("AddFindLog(data,") >= 12,
            "Find worker log routes must retain owned log delivery")
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
    ordered(broker_launch, "ConsentForeground foreground(launch.hwnd)", "ShellExecuteExW(&launch)",
            "foreground.Restore()", "SetLastError(error)")
    restore = function_body(broker, "void ConsentForeground::Restore()")
    handler = function_body(broker, "void RestoreConsentForeground(")
    require("PostMessageW(owner, restoreConsentForegroundMessage" in restore and
            "thread == GetCurrentThreadId()" in restore and "IsOwnedWindow(foreground, owner)" in restore,
            "worker restoration must reach the owner UI thread and preserve an active owned dialog")
    ordered(handler, "GetForegroundWindow() != foreground", "input.dwTime != lastInput", "IsIconic(owner)",
            "GetLastActivePopup(owner)", "IsWindowEnabled(target)", "SetForegroundWindow(target)")
    for name in ("finddlg1.cpp", "dialogs.cpp"):
        ui = (ROOT / "src" / name).read_text(encoding="utf-8-sig")
        require("case UacF7::restoreConsentForegroundMessage:" in ui and
                "UacF7::RestoreConsentForeground(HWindow," in ui,
                f"{name} must handle the worker request on its UI thread")
    editor = function_body(broker, "DWORD ElevatedEditor::Launch(")
    ordered(editor, "ShellExecuteExW(&launch)", "return ERROR_SUCCESS", "ERROR_CANCELLED", "foreground.Restore()")
    find_edit = function_body(finddlg1, "void CFindDialog::OnEditFile()")
    ordered(find_edit, "result.IsDir", "result.GetFullNameW()", "SalWideToMultiBytePath(path.c_str(), CP_UTF8)",
            "COpenEditorData openData{longName.data(), result.Attr, editor}", "SendMessage(",
            "if (openData.NeedsConsent)", "ConfirmElevatedFileEdit(HWindow, document, editor)",
            "LaunchElevatedFileEdit(HWindow, document, editor)")
    require("MAX_PATH" not in find_edit, "Find F4 must preserve the full Unicode result path")
    dispatch = (ROOT / "src" / "fileswnb.cpp").read_text(encoding="utf-8-sig")
    ordered(dispatch.split("case WM_USER_EDITFILE:", 1)[1].split("case WM_USER_VIEWFILEWITH:", 1)[0],
            "reinterpret_cast<COpenEditorData*>", "EditFile(result.FileName, 0xFFFFFFFF, &result)")
    edit_host = (ROOT / "src" / "fileswn5.cpp").read_text(encoding="utf-8-sig")
    edit_action = function_body(edit_host, "void CFilesWindow::EditFile(")
    ordered(edit_action, "if (!(found->Attributes & excluded))", "CheckPath(FALSE, path.c_str()",
            "SystemPolicies.GetMyCanRun(expCommand.Data())", "NeedsAdministratorForEdit(protectedCandidate)",
            "elevated.Prepare(", "found->NeedsConsent = ready;", "found->UntrustedEditor = !ready;", "return;",
            "ConfirmElevatedFileEdit(HWindow", "branchGeneration != GetBranchViewGeneration()",
            "GetItemFullPathW(*file)", "LaunchElevatedFileEdit(HWindow, protectedCandidate, elevated)",
            "if (foundParentError == ERROR_ACCESS_DENIED)", "CreateProcessForFileAction(")
    confirm_edit = function_body(edit_host, "BOOL ConfirmElevatedFileEdit(")
    ordered(confirm_edit, "editor.GetExecutable()", "ConfirmProtectedRead(owner")
    launch_edit = function_body(edit_host, "void LaunchElevatedFileEdit(")
    ordered(launch_edit, "editor.Launch(owner, document)", "ERROR_CANCELLED")
    require("name == NULL && Is(ptDisk) && handlerID == 0xFFFFFFFF" in edit_action and
            "FILE_ATTRIBUTE_REPARSE_POINT" in edit_action and "FILE_ATTRIBUTE_READONLY" in edit_action,
            "Branch View may use default F4, but unsupported result types cannot request an elevated editor")
    recycle = (ROOT / "src" / "fileswn8.cpp").read_text(encoding="utf-8-sig")
    ordered(function_body(recycle, "BOOL CFilesWindow::DeleteThroughRecycleBin("),
            "const HWND owner = GetAncestor(HWindow, GA_ROOT)", "operation->SetOwnerWindow(owner)",
            "RetryProtectedRecycle(owner,")
    ordered(function_body(recycle, "static HRESULT RetryProtectedRecycle("),
            "ConsentForeground foreground(owner)", "RunSingleRecycleOperation(owner,", "foreground.Restore()")
    main_window = (ROOT / "src" / "mainwnd3.cpp").read_text(encoding="utf-8-sig")
    admin_shell = main_window.split('sei.lpVerb = "runas";', 1)[1].split("STARTUPINFO si;", 1)[0]
    ordered(admin_shell, "ShellExecuteEx(&sei)", "ERROR_CANCELLED", "foreground.Restore()",
            "else if (sei.hProcess != NULL)")
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
    require("openDenied && !protectedQueue.Add(data, fullPath" in traversal and
            "ignoreList, &file)" in traversal,
            "a denied ordinary text or tag read must queue the exact wide file")
    require("FindFirstFileW(pattern.c_str(), &file)" in traversal and
            "FindNextFileW(find.get(), &file)" in traversal and
            "!protectedQueue.Add(data, directory" in traversal and
            "directory.c_str(), leaf" in traversal and "MAX_PATH" not in traversal.replace("SAL_MAX_PATH", ""),
            "ordinary Find must retain wide directory paths in listing, broker queue and results")
    require("std::reverse(pending.begin() + firstChild, pending.end());" in traversal and
            "directory.resize(next.parentLength);" in traversal and traversal.count("SearchDirectory(") == 0,
            "the pending stack must preserve ordinary traversal order without recursion")
    focused = function_body(finddlg1, "std::string CFindDialog::GetFocusedFile(")
    require("file->GetFullNameW().c_str(), CP_UTF8" in focused and "MAX_PATH" not in focused,
            "Find handler actions must accept the full focused path")
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
