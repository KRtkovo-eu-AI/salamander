# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
# | 2026-11-09 | Bind queued Find notifications to the originating search. | Ignore old refresh and completion messages after a restart. | | Codex |
# | 2026-11-09 | Check Find scheduling after a protected scope disconnects. | Continue other scopes without reauthorizing disconnected candidates. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check a single open for combined protected duplicate reads. | Defer filters until grouped candidates can share the digest handle. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check the protected Find revision across both passes. | Verify the candidate before reading any bytes for its digest. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check one protected Find open for text and tags. | Require a confirmed close before accepting the combined result. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Check duplicate verification at the comparison phase. | Require exact read lengths and keep stale empty candidates out of the result. | | GPT-6.1 Sol (Codex) |
# | 2026-11-09 | Verify protected Find refine scheduling. | Keep ordinary results responsive and retain rows on a declined subtraction check. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Check exact-file consent routing. | Distinguish refused local content reads from elevated scope. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Check multiple protected Find scopes. | Keep prompts serial and bind duplicate reads to the granting scope. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Cover the protected duplicate pipeline. | Verify collection order and scoped content validation. | | GPT-6.1 Sol (Codex) |
# | 2026-11-08 | Check protected Find consent handoff. | Publish ordinary hits before waiting and return consent to the launcher. | | GPT-6.1 Sol (Codex) |
# | 2026-11-07 | Add Find search scheduling contracts. | Prevent large Find searches from blocking on synchronous result-list refreshes. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | OpenAI GPT-5.6 Luna Pro |

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


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


def main() -> int:
    find_h = (ROOT / "src" / "find.h").read_text(encoding="utf-8-sig")
    find_cpp = (ROOT / "src" / "find.cpp").read_text(encoding="utf-8-sig")
    finddlg1 = (ROOT / "src" / "finddlg1.cpp").read_text(encoding="utf-8-sig")

    require("volatile LONG RefreshPosted;" in find_h,
            "Find data must track a queued result refresh atomically")
    start_search = function_body(finddlg1, "void CFindDialog::StartSearch(WORD command)")
    require("GrepData.RefreshPosted = FALSE;" in start_search,
            "StartSearch must clear the queued-refresh flag before creating the worker")

    request_refresh = function_body(find_cpp, "static void RequestFindResultsRefresh(CGrepData* data)")
    require("InterlockedExchange(&data->RefreshPosted, TRUE) == FALSE" in request_refresh,
            "Find refresh requests must be coalesced")
    require("PostMessage(data->HWindow, WM_USER_ADDFILE, data->SearchGeneration, 0)" in request_refresh,
            "Find refresh requests must not synchronously call the dialog")
    require("InterlockedExchange(&data->RefreshPosted, FALSE);" in request_refresh,
            "a failed refresh post must release the coalescing flag")
    require(find_cpp.count("RequestFindResultsRefresh(data);") == 5,
            "incremental, periodic, pre-consent, and final Find refreshes must use the helper")
    require("SendMessage(data->HWindow, WM_USER_ADDFILE" not in find_cpp,
            "the Find worker must not synchronously send result refresh messages")

    require(start_search.index("++GrepData.SearchGeneration;") < start_search.index("CreateThread("),
            "a new worker must start with its own notification generation")
    completion = function_body(finddlg1, "case WM_USER_FIND_COMPLETE:")
    require(completion.index("SearchInProgress && wParam == GrepData.SearchGeneration") <
            completion.index("SendMessage(HWindow, WM_COMMAND, IDC_FIND_STOP, 0);"),
            "a queued completion from an earlier search must never stop the new worker")
    add_file = function_body(finddlg1, "case WM_USER_ADDFILE:")
    require(add_file.index("wParam != GrepData.SearchGeneration") < add_file.index("InterlockedExchange("),
            "a stale refresh must not clear the current search's queued-refresh flag")
    require("InterlockedExchange(&GrepData.RefreshPosted, FALSE);" in add_file,
            "the dialog must release the queued-refresh flag before updating the list")
    require("UpdateListViewItems();" in add_file,
            "the dialog must update the virtual result list when the post is handled")

    grep_body = function_body(find_cpp, "unsigned GrepThreadFBody(void* ptr)")
    require(grep_body.rindex("RequestFindResultsRefresh(data);") < grep_body.index(
        "PostMessage(data->HWindow, WM_USER_FIND_COMPLETE, data->SearchGeneration, 0);"),
            "the final result refresh must be queued before search completion")
    require("SendMessage(data->HWindow, WM_USER_ADDLOG" in find_cpp,
            "synchronous log delivery must remain for stack-owned log payloads")

    require(grep_body.index("RequestFindResultsRefresh(data);") < grep_body.index("protectedQueue.Finish(duplicateCandidates);"),
            "ordinary hits must be published before waiting for administrator consent")
    require(grep_body.index("protectedQueue.Finish(duplicateCandidates);") <
            grep_body.index("duplicateCandidates->Examine(data, protectedSessions, protectedQueue.Remaining());"),
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
