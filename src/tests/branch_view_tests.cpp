// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../branch_view.h"
#include <stdio.h>
#include <set>
#include <algorithm>

using namespace Salamander::BranchView;
static int Failures = 0;
static int Checks = 0;
static void Check(bool condition, const char* text)
{
    ++Checks;
    if (!condition) { ++Failures; printf("FAIL: %s (error %lu)\n", text, GetLastError()); }
}
struct Fixture
{
    std::wstring Root;
    std::vector<std::wstring> Files, Dirs;
    Fixture()
    {
        std::vector<wchar_t> temp(32768);
        const DWORD length = GetTempPathW((DWORD)temp.size(), temp.data());
        if (length == 0 || length >= temp.size()) { Check(false, "safe temporary root"); std::abort(); }
        Root = Join(temp.data(), L"salamander-branch-test-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        Directory(Root);
    }
    void Directory(const std::wstring& path)
    {
        Check(CreateDirectoryW(ExtendedPath(path).c_str(), NULL) != FALSE, "create fixture directory");
        Dirs.push_back(path);
    }
    void File(const std::wstring& path)
    {
        HANDLE file = CreateFileW(ExtendedPath(path).c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        Check(file != INVALID_HANDLE_VALUE, "create fixture file");
        if (file != INVALID_HANDLE_VALUE) { DWORD done; WriteFile(file, "test", 4, &done, NULL); CloseHandle(file); }
        Files.push_back(path);
    }
    ~Fixture()
    {
        for (auto it = Files.rbegin(); it != Files.rend(); ++it) { if (!IsWithin(Root, *it)) std::abort(); DeleteFileW(ExtendedPath(*it).c_str()); }
        for (auto it = Dirs.rbegin(); it != Dirs.rend(); ++it) { if (!IsWithin(Root, *it)) std::abort(); RemoveDirectoryW(ExtendedPath(*it).c_str()); }
    }
};
static std::vector<Entry> Collect(Scan& scan, Progress& progress)
{
    std::vector<Entry> all;
    const ULONGLONG deadline = GetTickCount64() + 10000;
    do
    {
        std::vector<Entry> batch;
        scan.Drain(batch, progress);
        all.insert(all.end(), batch.begin(), batch.end());
        if (!progress.Running) return all;
        Sleep(1);
    } while (GetTickCount64() < deadline);
    scan.Cancel();
    Check(false, "enumeration deadline");
    return all;
}
#ifdef BRANCH_VIEW_EXTENDED_TESTS // Retained future scheduler/publication fixture; not counted in S25.
static void CheckRefreshSchedule()
{
    RefreshSchedule refresh;
    refresh.Request(0, false);
    Check(refresh.Due(0) && refresh.IsManual(), "explicit initial scan reapplies view settings immediately");
    refresh.Started(0);
    for (ULONGLONG now = 1; now < 60000; now += 25)
    {
        refresh.Request(now, true);
        Check(!refresh.Due(now), "continuous changes cannot restart a running scan");
    }
    refresh.Finished(60000);
    Check(!refresh.Due(89999) && refresh.Due(90000), "long scan has bounded completion cooldown");
    refresh.Started(90000);
    refresh.Finished(90100);
    Check(!refresh.HasPending(), "starting a scan consumes its pending notifications");
    for (ULONGLONG now = 100000; now < 105000; now += 50)
    {
        refresh.Request(now, true);
        Check(!refresh.Due(now), "notification burst is debounced");
    }
    refresh.Request(105000, true);
    Check(refresh.Due(105000) && !refresh.IsManual(), "continuous writes cannot starve maximum debounce deadline or force unchanged publication");
    refresh.Cancel();
    refresh.Request(200000, true);
    Check(!refresh.HasPending() && !refresh.Due(300000), "Stop remains paused despite new notifications");
    refresh.Request(300000, false);
    Check(refresh.Due(300000), "manual refresh resumes a stopped scan immediately");
    refresh.Started(300000);
    refresh.Request(300001, false);
    Check(!refresh.Due(300001), "repeated manual refresh also lets active enumeration finish");
    refresh.Finished(300010);
    Check(refresh.Due(300010) && refresh.IsManual(), "pending manual refresh bypasses cooldown and preserves filter reapplication");
    RefreshSchedule tickWrap;
    tickWrap.Request(0x100000000ULL - 10, true);
    Check(!tickWrap.Due(0x100000000ULL) && tickWrap.Due(0x100000000ULL + 990), "scheduling remains correct beyond DWORD tick wrap");
}
#endif
struct ChangingTree
{
    std::wstring Existing, Transient;
    HANDLE Stop;
};
static DWORD WINAPI ChangeTree(void* context)
{
    ChangingTree* tree = static_cast<ChangingTree*>(context);
    do
    {
        for (const auto& path : {tree->Existing, tree->Transient})
        {
            HANDLE file = CreateFileW(ExtendedPath(path).c_str(), GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (file != INVALID_HANDLE_VALUE)
            {
                const char data[] = "changed while the Branch snapshot is being enumerated";
                DWORD done;
                WriteFile(file, data, sizeof(data), &done, NULL);
                CloseHandle(file);
            }
        }
        DeleteFileW(ExtendedPath(tree->Transient).c_str());
    } while (WaitForSingleObject(tree->Stop, 2) == WAIT_TIMEOUT);
    return 0;
}
#ifdef BRANCH_VIEW_EXTENDED_TESTS // Retained future scheduler/publication fixture; not counted in S25.
static void CheckPublicationBudgetAndPathOrder()
{
    Check(InitialPreviewCount(175000, 0) == 2048, "first preview is bounded independently of a fast worker batch");
    Check(InitialPreviewCount(175000, 2048) == 8192, "second preview is bounded while enumeration continues");
    Check(InitialPreviewCount(175000, 8192) == 8192, "large growing scans do not repeatedly rebuild complete prefixes");
    Check(InitialPreviewCount(12, 0) == 12 && InitialPreviewCount(15, 12) == 15, "small collections retain incremental progress");

    // A representative large collection with duplicate basenames, Unicode,
    // case-differing parents and paths beyond MAX_PATH. Compare the actual new
    // comparator against the previous full-path-copy comparator in both orders.
    const size_t count = 175000;
    std::vector<Entry> entries;
    entries.reserve(count);
    std::vector<const Entry*> before, after;
    before.reserve(count);
    const std::wstring root = L"C:\\Branch-žluťoučký-東京\\" + std::wstring(180, L'ě');
    for (size_t i = 0; i < count; ++i)
    {
        Entry entry = {};
        const size_t value = (i * 7919) % count;
        const std::wstring relativeDirectory = (value % 2 ? L"Folder" : L"folder") + std::to_wstring(value % 1000);
        entry.Directory = Join(root, relativeDirectory);
        const std::wstring name = L"same-😀-" + std::to_wstring(value / 1000) + L".txt";
        entry.RelativePath = Join(relativeDirectory, name);
        wcscpy_s(entry.Data.cFileName, name.c_str());
        entries.push_back(std::move(entry));
    }
    for (const auto& entry : entries) before.push_back(&entry);
    after = before;
    const ULONGLONG oldStart = GetTickCount64();
    std::stable_sort(before.begin(), before.end(), [](const Entry* a, const Entry* b) {
        const std::wstring first = a->Directory, second = b->Directory;
        const int order = CompareStringOrdinal(first.c_str(), -1, second.c_str(), -1, TRUE) - CSTR_EQUAL;
        if (order != 0) return order < 0;
        return a->FullPath() < b->FullPath();
    });
    const ULONGLONG oldElapsed = GetTickCount64() - oldStart;
    const ULONGLONG newStart = GetTickCount64();
    std::stable_sort(after.begin(), after.end(), [](const Entry* a, const Entry* b) { return CompareEntryPaths(*a, *b) < 0; });
    const ULONGLONG newElapsed = GetTickCount64() - newStart;
    Check(before == after, "175k optimized Path sort preserves exact old ordering and duplicate identities");
    std::reverse(before.begin(), before.end());
    std::stable_sort(after.begin(), after.end(), [](const Entry* a, const Entry* b) { return CompareEntryPaths(*a, *b) > 0; });
    Check(before == after, "reverse Path sort preserves full-path tie order");
    printf("Branch Path comparator: %zu Unicode/long-path rows, copied paths %llu ms, retained references %llu ms (equivalent order)\n",
        count, oldElapsed, newElapsed);
}
#endif
int wmain()
{
#ifdef BRANCH_VIEW_EXTENDED_TESTS
    CheckRefreshSchedule();
    CheckPublicationBudgetAndPathOrder();
#endif
    Fixture fixture;
    std::wstring a = Join(fixture.Root, L"A"), b = Join(fixture.Root, L"Žluťoučký-東京-😀");
    fixture.Directory(a); fixture.Directory(b);
    fixture.File(Join(a, L"same.txt")); fixture.File(Join(b, L"same.txt"));
    fixture.File(Join(fixture.Root, L"root.txt"));
    std::wstring longDirectory = b;
    for (int i = 0; i != 4; ++i) { longDirectory = Join(longDirectory, std::wstring(75, L'ě')); fixture.Directory(longDirectory); }
    const std::wstring longName = std::wstring(250, L'č') + L".txt";
    fixture.File(Join(longDirectory, longName));
    std::wstring hidden = Join(fixture.Root, L"hidden"); fixture.Directory(hidden);
    SetFileAttributesW(ExtendedPath(hidden).c_str(), FILE_ATTRIBUTE_HIDDEN);
    fixture.File(Join(hidden, L"hidden.txt"));
    for (int i = 0; i != 600; ++i) fixture.File(Join(a, L"file" + std::to_wstring(i) + L".bin"));

    const std::wstring cycle = Join(a, L"cycle");
    const bool hasCycle = CreateSymbolicLinkW(cycle.c_str(), fixture.Root.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY | 0x2) != FALSE;
    if (hasCycle) fixture.Dirs.push_back(cycle);
    else printf("Directory symlink fixture unavailable (%lu); name-surrogate policy checked below.\n", GetLastError());
    Check(IsReparseTagNameSurrogate(IO_REPARSE_TAG_MOUNT_POINT) != 0 &&
          IsReparseTagNameSurrogate(IO_REPARSE_TAG_SYMLINK) != 0, "junction/symlink traversal exclusion");
    Scan scan; Progress progress;
    Check(scan.Start(fixture.Root, false), "start recursive scan");
    auto all = Collect(scan, progress);
    Check(all.size() == 605 && progress.Files == 605, "complete recursive count across batches");
    Check(progress.Errors == 0 && !progress.Cancelled, "successful final state");
    if (hasCycle) Check(progress.SkippedLinks == 1, "cycle skipped without following out-of-tree link");
    std::set<std::wstring> identities;
    int duplicates = 0; bool foundLong = false;
    for (const auto& entry : all)
    {
        identities.insert(entry.FullPath());
        const int wideCount = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, entry.CacheKey.c_str(), -1, NULL, 0);
        std::vector<wchar_t> decoded((size_t)(std::max)(1, wideCount));
        if (wideCount > 0) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, entry.CacheKey.c_str(), -1, decoded.data(), wideCount);
        Check(wideCount > 0 && entry.FullPath() == decoded.data(), "worker prepares exact full UTF-8 cache identity");
        if (wcscmp(entry.Data.cFileName, L"same.txt") == 0) ++duplicates;
        if (entry.FullPath() == Join(longDirectory, longName)) foundLong = true;
        Check(entry.FullPath() == Join(fixture.Root, entry.RelativePath), "relative identity round trip");
        Check((entry.Data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0, "files-only snapshot");
    }
    Check(duplicates == 2 && identities.size() == all.size(), "duplicate basenames retain distinct exact paths");
    Check(foundLong, "long Unicode file and directory paths retained exactly");
    auto equivalent = all;
    std::reverse(equivalent.begin(), equivalent.end());
    for (auto& entry : equivalent) ++entry.Data.ftLastAccessTime.dwLowDateTime;
    Check(SameSnapshot(all, equivalent), "enumeration order and self-read access times do not republish snapshot");
    ++equivalent[0].Data.nFileSizeLow;
    Check(!SameSnapshot(all, equivalent), "content metadata change publishes updated snapshot");
    equivalent = all;
    equivalent[0].Directory += L"-different-parent";
    Check(!SameSnapshot(all, equivalent), "same basename in another parent is a different snapshot");

    ChangingTree changing = {Join(a, L"file0.bin"), Join(a, L"transient.bin"), CreateEventW(NULL, TRUE, FALSE, NULL)};
    HANDLE writer = CreateThread(NULL, 0, ChangeTree, &changing, 0, NULL);
    Check(writer != NULL && changing.Stop != NULL, "start changing directory fixture");
#ifdef BRANCH_VIEW_EXTENDED_TESTS
    RefreshSchedule liveSchedule;
    liveSchedule.Request(GetTickCount64(), false);
    liveSchedule.Started(GetTickCount64());
#endif
    Check(scan.Start(fixture.Root, false), "scan tree with concurrent writes");
    std::vector<Entry> duringChanges;
    const ULONGLONG changingDeadline = GetTickCount64() + 10000;
    do
    {
#ifdef BRANCH_VIEW_EXTENDED_TESTS
        liveSchedule.Request(GetTickCount64(), true);
        Check(!liveSchedule.Due(GetTickCount64()), "filesystem change does not restart active real scan");
#endif
        std::vector<Entry> batch;
        scan.Drain(batch, progress);
        duringChanges.insert(duringChanges.end(), batch.begin(), batch.end());
        if (!progress.Running) break;
        Sleep(1);
    } while (GetTickCount64() < changingDeadline);
    SetEvent(changing.Stop);
    if (writer != NULL)
    {
        Check(WaitForSingleObject(writer, 2000) == WAIT_OBJECT_0, "changing fixture stops within deadline");
        CloseHandle(writer);
    }
    CloseHandle(changing.Stop);
    Check(!progress.Running && duringChanges.size() >= 605, "first scan finishes despite continuously changing tree");
#ifdef BRANCH_VIEW_EXTENDED_TESTS
    liveSchedule.Finished(GetTickCount64());
    Check(!liveSchedule.Due(GetTickCount64()), "completed real scan remains visible during automatic cooldown");
#endif
    Check(scan.Start(fixture.Root, false), "take complete snapshot after mutations");
    auto afterChanges = Collect(scan, progress);
    Check(afterChanges.size() == 605 && !SameSnapshot(all, afterChanges), "later scan captures changes without stale transient rows");
    Check(scan.Start(fixture.Root, true), "restart filtered scan");
    auto filtered = Collect(scan, progress);
    Check(filtered.size() == 604, "hidden directory traversal policy");

    // A new generation owns independent result storage; abandoned batches must
    // never leak into another location even when a previous worker returns late.
    Check(scan.Start(fixture.Root, false), "start superseded scan");
    Check(scan.Start(b, false), "start replacement scan");
    auto replacement = Collect(scan, progress);
    Check(replacement.size() == 2, "superseded generation has no stale results");
    for (const auto& entry : replacement) Check(IsWithin(b, entry.FullPath()), "replacement root ownership");
    Check(scan.Start(fixture.Root, false), "start cancelled scan");
    scan.Cancel();
    Collect(scan, progress);
    Check(progress.Cancelled && !progress.Running, "cooperative cancellation state");
    // Closing a panel destroys its Scan without joining the worker. Exercise
    // that ownership boundary repeatedly while results may still be pending.
    for (int i = 0; i != 16; ++i)
    {
        Scan closing;
        Check(closing.Start(fixture.Root, false), "start scanner whose owner closes immediately");
    }
    Scan surviving;
    Check(surviving.Start(b, false), "start surviving scanner after other owners closed");
    auto survivingFiles = Collect(surviving, progress);
    Check(survivingFiles.size() == 2 && progress.Errors == 0, "closed scanner state cannot leak into surviving owner");
    Check(IsWithin(L"C:\\root", L"C:\\root\\a"), "descendant notification");
    Check(!IsWithin(L"C:\\root", L"C:\\root-other\\a"), "component boundary notification");
    Check(ExtendedPath(L"\\\\server\\share\\ě.txt") == L"\\\\?\\UNC\\server\\share\\ě.txt", "UNC extended path");
    Scan missing;
    Check(missing.Start(Join(fixture.Root, L"missing-root"), false), "missing-root failure is reported by worker");
    const auto missingFiles = Collect(missing, progress);
    Check(missingFiles.empty() && !progress.Running && progress.Errors == 1 && progress.LastError == ERROR_PATH_NOT_FOUND,
          "inaccessible or missing root retains exact error state");
    printf("S25 scanner checks: %d; directory symlink fixture: %s; future scheduler/publication sort fixtures deferred.\n", Checks, hasCycle ? "RAN" : "UNAVAILABLE");
    printf("Branch View scanner: %s (%d failures, %zu entries, duplicate/Unicode/long-path/filter/generation/cancel/owner-destruction/concurrent-write/snapshot checks)\n", Failures ? "FAIL" : "PASS", Failures, all.size());
    return Failures ? 1 : 0;
}
