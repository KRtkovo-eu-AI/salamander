// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise partial listing recovery on worker seams. | Retain siblings and reject identity changes without duplicate rows. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <atomic>
#include <cstdio>
#include <memory>
using namespace Salamander::BranchView;
static int failures = 0;
static std::atomic<unsigned> localCloses{0};
static void Check(bool ok, const char* name) { std::printf("%s: %s\n", ok ? "PASS" : "FAIL", name); if (!ok) ++failures; }
struct Context { bool broker = false, changeParent = false, changeFile = false, disconnect = false, large = false; HANDLE stop = nullptr; };
struct Cursor { Context* owner; std::wstring path; unsigned page = 0; };
static Context local;
static const std::wstring root = L"C:\\synthetic-root";
static IdentifiedPageRow Row(const wchar_t* name, BYTE id, bool directory = false)
{
    IdentifiedPageRow row{}; wcscpy_s(row.Data.cFileName, name); row.FileId[0] = id;
    row.Data.dwFileAttributes = directory ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
    row.Data.nFileSizeLow = directory ? 0 : 1;
    return row;
}
static BYTE DirectoryId(const std::wstring& path)
{
    if (path == root) return 1;
    if (path == root + L"\\partial") return 2;
    if (path == root + L"\\partial\\visited") return 3;
    if (path == root + L"\\partial\\newdir") return 4;
    return 0;
}
static DWORD Open(void* raw, const std::wstring& path, const DeniedDirectory*, DeniedDirectory& actual, void*& out)
{
    Context* ctx = raw ? static_cast<Context*>(raw) : &local;
    const BYTE id = DirectoryId(path);
    if (!id) return ERROR_PATH_NOT_FOUND;
    actual.Volume = 7; actual.FileId[0] = (ctx->broker && ctx->changeParent && id == 2) ? 9 : id;
    actual.Verified = true;
    out = new Cursor{ctx, path}; return ERROR_SUCCESS;
}
static DWORD Next(void*, void* raw, std::vector<IdentifiedPageRow>& rows, bool& done)
{
    auto* cursor = static_cast<Cursor*>(raw); auto& ctx = *cursor->owner;
    const auto page = cursor->page++;
    if (!ctx.broker && cursor->path == root)
    {
        if (ctx.large)
        {
            for (unsigned i = page * 64; i < (page + 1) * 64 && i < 4097; ++i)
            {
                wchar_t name[32]{}; swprintf_s(name, L"file-%u.txt", i);
                rows.push_back(Row(name, static_cast<BYTE>(i)));
            }
            done = (page + 1) * 64 >= 4097;
            return ERROR_SUCCESS;
        }
        rows = {Row(L"ordinary.txt", 11), Row(L"partial", 2, true)}; done = true; return ERROR_SUCCESS;
    }
    if (!ctx.broker && cursor->path == root + L"\\partial")
    {
        if (page) return ERROR_ACCESS_DENIED;
        rows = {Row(L"a.txt", 12), Row(L"visited", 3, true)}; done = false; return ERROR_SUCCESS;
    }
    if (!ctx.broker && cursor->path == root + L"\\partial\\visited")
    {
        rows = {Row(L"prior-child.txt", 13)}; done = true; return ERROR_SUCCESS;
    }
    if (ctx.broker && cursor->path == root + L"\\partial")
    {
        if (ctx.disconnect && page) return ERROR_BROKEN_PIPE;
        rows = {Row(L"a.txt", ctx.changeFile ? 99 : 12), Row(L"visited", 3, true), Row(L"b.txt", 14)};
        if (ctx.disconnect) rows.push_back(Row(L"newdir", 4, true));
        done = !ctx.disconnect; return ERROR_SUCCESS;
    }
    return ERROR_INVALID_DATA;
}
static void Close(void*, void* raw) { auto* cursor = static_cast<Cursor*>(raw); if (!cursor->owner->broker) ++localCloses; delete cursor; }
static bool Capture(const std::wstring&, DeniedDirectory&) { return false; }
static void RunCase(bool changedParent, bool changedFile, bool disconnect)
{
    DirectoryPager pager{Open,Next,Close}; Scan scan; scan.SetLocalPager(pager); scan.SetCaptureDenied(Capture);
    Check(scan.Start(root, false) && scan.WaitForTest(5000), "ordinary scan with a partial page completes");
    std::vector<Entry> ordinary; Progress progress; scan.Drain(ordinary, progress);
    std::vector<DeniedDirectory> denied; scan.DrainDenied(denied);
    Check(ordinary.size() == 3 && denied.size() == 1 && denied[0].Verified && denied[0].Partial && denied[0].Anchor,
          "readable sibling, partial row and previously queued child survive while the original parent handle is retained");
    if (denied.empty()) return;
    auto ctx = std::make_shared<Context>(); ctx->broker = true; ctx->changeParent = changedParent;
    ctx->changeFile = changedFile; ctx->disconnect = disconnect;
    ctx->stop = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    Check(scan.StartProtectedPaged(denied, progress, pager, ctx, ctx->stop) && scan.WaitForTest(5000),
          "protected retry finishes on a separate worker");
    std::vector<Entry> retried; scan.Drain(retried, progress);
    if (changedParent)
        Check(retried.empty() && progress.Errors && progress.LastError == ERROR_FILE_INVALID,
              "replacement parent is rejected before publishing retry rows");
    else if (changedFile)
        Check(retried.size() == 1 && retried[0].FullPath() == root + L"\\partial\\b.txt" &&
              progress.Errors && progress.LastError == ERROR_FILE_INVALID,
              "same-name replacement is not published and unrelated retry row survives");
    else if (disconnect)
        Check(retried.size() == 1 && retried[0].FullPath() == root + L"\\partial\\b.txt" &&
              progress.Errors >= 2 && progress.LastError == ERROR_BROKEN_PIPE,
              "disconnect keeps completed page and counts the queued unprocessed descendant");
    else
        Check(retried.size() == 1 && retried[0].FullPath() == root + L"\\partial\\b.txt" && !progress.Errors,
              "the retried page emits only the new row and does not revisit the old child");
    scan.Cancel(); CloseHandle(ctx->stop);
}
int wmain()
{
    RunCase(false,false,false); RunCase(true,false,false); RunCase(false,true,false); RunCase(false,false,true);
    {
        DirectoryPager pendingPager{Open,Next,Close}; Scan pending; pending.SetLocalPager(pendingPager); pending.SetCaptureDenied(Capture);
        const auto before = localCloses.load();
        Check(pending.Start(root, false) && pending.WaitForTest(5000) && localCloses.load() == before + 2,
              "the denied partial directory remains pinned after ordinary scan completion");
        pending.Cancel();
        Check(localCloses.load() == before + 3, "cancelling the scan releases the retained parent handle");
    }
    local.large = true;
    DirectoryPager pager{Open,Next,Close}; Scan scan; scan.SetLocalPager(pager); scan.SetCaptureDenied(Capture);
    const ULONGLONG started = GetTickCount64();
    Check(scan.Start(root, false) && scan.WaitForTest(10000), "4097-row paged worker completes");
    std::vector<Entry> entries; Progress progress; scan.Drain(entries, progress);
    Check(entries.size() == 4097 && progress.Files == 4097 && progress.Errors == 0,
          "4097 rows are retained without the old one-page listing limit");
    std::printf("Synthetic 4097-row worker elapsed: %llu ms\n", GetTickCount64() - started);
    return failures ? 1 : 0;
}
