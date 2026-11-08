# SPDX-FileCopyrightText: 2026 Open Samandarin Authors
# SPDX-License-Identifier: GPL-2.0-or-later
# | Date | Summary | Reason | Ticket | Model |
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
    require("PostMessage(data->HWindow, WM_USER_ADDFILE, 0, 0)" in request_refresh,
            "Find refresh requests must not synchronously call the dialog")
    require("InterlockedExchange(&data->RefreshPosted, FALSE);" in request_refresh,
            "a failed refresh post must release the coalescing flag")
    require(find_cpp.count("RequestFindResultsRefresh(data);") == 4,
            "incremental, periodic, pre-consent, and final Find refreshes must use the helper")
    require("SendMessage(data->HWindow, WM_USER_ADDFILE" not in find_cpp,
            "the Find worker must not synchronously send result refresh messages")

    add_file = function_body(finddlg1, "case WM_USER_ADDFILE:")
    require("InterlockedExchange(&GrepData.RefreshPosted, FALSE);" in add_file,
            "the dialog must release the queued-refresh flag before updating the list")
    require("UpdateListViewItems();" in add_file,
            "the dialog must update the virtual result list when the post is handled")

    grep_body = function_body(find_cpp, "unsigned GrepThreadFBody(void* ptr)")
    require(grep_body.find("RequestFindResultsRefresh(data);") < grep_body.find(
        "PostMessage(data->HWindow, WM_COMMAND, IDC_FIND_STOP, 0);"),
            "the final result refresh must be queued before search completion")
    require("SendMessage(data->HWindow, WM_USER_ADDLOG" in find_cpp,
            "synchronous log delivery must remain for stack-owned log payloads")

    require(grep_body.index("RequestFindResultsRefresh(data);") < grep_body.index("protectedRequest.Finish(duplicateCandidates);"),
            "ordinary hits must be published before waiting for administrator consent")
    require(grep_body.index("protectedRequest.Finish(duplicateCandidates);") <
            grep_body.index("duplicateCandidates->Examine(data, protectedSession);"),
            "protected candidates must join ordinary candidates before grouping and hashing")
    digest = function_body(find_cpp, "BOOL CDuplicateCandidates::GetMD5Digest(")
    require("protectedSession->ReadContent(offset, buffer, read)" in digest and
            "protectedSession->CloseContent()" in digest and
            digest.index("protectedSession->CloseContent()") < digest.index("context.finalize()"),
            "protected duplicate digests must use and validate the authorized content stream")
    consent = function_body(finddlg1, "case WM_USER_FIND_PROTECTED_CONSENT:")
    require("SetWindowLongPtr(HWindow, DWLP_MSGRESULT, approved);" in consent and "return TRUE;" in consent,
            "Find is a dialog and must return approval through DWLP_MSGRESULT")

    print("find_search_schedule_contract_tests: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
