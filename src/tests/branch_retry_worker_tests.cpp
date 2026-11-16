// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <cstdio>
#include <memory>
using namespace Salamander::BranchView;
static int failures = 0;
static void Check(bool value, const char* name) { std::printf("%s: %s\n", value ? "PASS" : "FAIL", name); if (!value) ++failures; }
struct Context {
    HANDLE stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE entered = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    bool block = false, overflow = false;
    ~Context() { CloseHandle(done); CloseHandle(entered); CloseHandle(stop); }
};
static DWORD List(void* raw, const std::wstring&, std::vector<WIN32_FIND_DATAW>& out)
{
    auto* ctx = static_cast<Context*>(raw);
    SetEvent(ctx->entered);
    if (ctx->block) { WaitForSingleObject(ctx->stop, 5000); SetEvent(ctx->done); return ERROR_OPERATION_ABORTED; }
    if (ctx->overflow) return ERROR_BUFFER_OVERFLOW;
    WIN32_FIND_DATAW file{}; wcscpy_s(file.cFileName, L"protected.txt");
    out.push_back(file);
    WIN32_FIND_DATAW link{}; wcscpy_s(link.cFileName, L"junction");
    link.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT;
    link.dwReserved0 = IO_REPARSE_TAG_MOUNT_POINT;
    out.push_back(link);
    return ERROR_SUCCESS;
}
int wmain()
{
    const DWORD needed = GetTempPathW(0, nullptr);
    if (!needed) return 2;
    std::vector<wchar_t> temp(needed + 1);
    if (!GetTempPathW(static_cast<DWORD>(temp.size()), temp.data())) return 2;
    std::wstring root(temp.data());
    while (!root.empty() && root.back() == L'\\') root.pop_back();
    Scan scan;
    Check(scan.Start(root, false) && scan.WaitForTest(30000), "ordinary worker completes");
    Progress initial{}; std::vector<Entry> ignored; scan.Drain(ignored, initial);
    DeniedDirectory candidate; candidate.Path = root + L"\\denied"; candidate.Relative = L"denied";
    auto ctx = std::make_shared<Context>();
    Check(!scan.StartProtected({candidate}, initial, List, ctx, ctx->stop), "unverified identity rejected before worker start");
    candidate.Verified = true;
    DeniedDirectory outside = candidate; outside.Path = L"C:\\outside";
    Check(!scan.StartProtected({outside}, initial, List, ctx, ctx->stop), "outside root rejected before worker start");
    Check(scan.StartProtected({candidate}, initial, List, ctx, ctx->stop) && scan.WaitForTest(5000), "protected worker completes");
    std::vector<Entry> entries; Progress progress; scan.Drain(entries, progress);
    Check(entries.size() == 1 && entries[0].FullPath() == candidate.Path + L"\\protected.txt" && progress.SkippedLinks == 1 && !progress.Running, "worker publishes files and skips reparse directory");
    ctx = std::make_shared<Context>(); ctx->overflow = true;
    Check(scan.StartProtected({candidate}, initial, List, ctx, ctx->stop) && scan.WaitForTest(5000), "overflow worker completes");
    scan.Drain(entries, progress);
    Check(progress.LimitExceeded && progress.LastError == ERROR_BUFFER_OVERFLOW && progress.Errors == initial.Errors + 1, "overflow is retained as incomplete");
    ctx = std::make_shared<Context>(); ctx->block = true;
    Check(scan.StartProtected({candidate}, initial, List, ctx, ctx->stop) && WaitForSingleObject(ctx->entered, 5000) == WAIT_OBJECT_0, "protected worker is waiting");
    scan.Cancel();
    Check(WaitForSingleObject(ctx->stop, 1000) == WAIT_OBJECT_0 &&
          WaitForSingleObject(ctx->done, 1000) == WAIT_OBJECT_0, "cancel signals the broker stop event and unblocks the worker");
    return failures ? 1 : 0;
}
