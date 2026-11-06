// SPDX-License-Identifier: GPL-2.0-or-later
#include "../explorerpropertywork.h"
#include <cstdio>
#include <thread>
static int Failures = 0;
static void Check(bool ok, const char* text)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", text);
    if (!ok) ++Failures;
}
struct Task : CExplorerPropertyTaskLifetime
{
    volatile LONG* Destroyed;
    Task(HWND hwnd, volatile LONG* destroyed) : CExplorerPropertyTaskLifetime(hwnd), Destroyed(destroyed) {}
    ~Task() override { InterlockedIncrement(Destroyed); }
};
static HWND Window()
{
    return CreateWindowExW(0, L"STATIC", L"property-task-test", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandleW(NULL), NULL);
}
int main()
{
    int calls = 0;
    auto viewport = ExplorerPropertyDisplayRows(174935, 100000, 80, [&](int) { ++calls; return true; });
    Check(viewport.size() == 80 && calls == 80 && viewport.front() == 100000 && viewport.back() == 100079,
          "174935-row collection inspects exactly 80 visible rows");
    calls = 0;
    auto batch = ExplorerPropertyDisplayRows(174935, 0, 1000, [&](int) { ++calls; return true; });
    Check(batch.size() == 128 && calls == 128, "large viewport produces a bounded 128-row batch");
    auto next = ExplorerPropertyDisplayRows(174935, 0, 1000, [](int row) { return row >= 128; });
    Check(next.size() == 128 && next.front() == 128 && next.back() == 255, "cached empty or populated rows allow the next batch without starvation");
    auto end = ExplorerPropertyDisplayRows(174935, 174900, 1000, [](int) { return true; });
    Check(end.size() == 35 && end.back() == 174934, "last viewport never exceeds row count");
    Check(ExplorerPropertyDisplayRows(174935, 999999, 1000, [](int) { return true; }).empty() &&
          ExplorerPropertyDisplayRows(0, 0, 80, [](int) { return true; }).empty(), "empty and stale viewport bounds are safe");
    Check(ExplorerPropertyRequestNeedsReplacement(4, -1, 4, 21, false), "viewport job is superseded by an explicit property sort");
    Check(ExplorerPropertyRequestNeedsReplacement(4, 21, 4, 37, false), "switching between property columns replaces the old sort");
    Check(ExplorerPropertyRequestNeedsReplacement(4, 21, 4, -1, false), "returning to Path or Name requests display-only work");
    Check(!ExplorerPropertyRequestNeedsReplacement(4, 21, 4, 21, false), "unchanged property sort keeps its active worker");
    Check(ExplorerPropertyRequestNeedsReplacement(4, 21, 4, 21, true), "switch-away-and-back cannot reuse a cancelled partial sort");
    Check(ExplorerPropertyRequestNeedsReplacement(4, -1, 5, -1, false), "new listing generation replaces otherwise identical display request");
    const UINT message = WM_APP + 73;
    volatile LONG destroyed = 0;
    HWND hwnd = Window();
    Check(hwnd != NULL, "create owned message-only completion target");
    Task* queued = new Task(hwnd, &destroyed);
    queued->PostCompletion(message, (LPARAM)queued);
    queued->CancelCompletion(message);
    MSG msg;
    Check(!PeekMessageW(&msg, hwnd, message, message, PM_REMOVE), "cancel drains already-posted completion before destruction");
    queued->Release();
    Check(destroyed == 1, "panel-owned task is released once");
    DestroyWindow(hwnd);
    bool raceOk = true;
    ULONGLONG maximumCancel = 0;
    for (int i = 0; i < 100; ++i)
    {
        hwnd = Window();
        HANDLE release = CreateEventW(NULL, TRUE, FALSE, NULL);
        Task* task = new Task(hwnd, &destroyed);
        task->AddRef();
        std::thread worker([=] {
            WaitForSingleObject(release, 2000);
            task->PostCompletion(message, (LPARAM)task);
            task->Release();
        });
        const ULONGLONG before = GetTickCount64();
        task->CancelCompletion(message);
        task->Release();
        maximumCancel = (std::max)(maximumCancel, GetTickCount64() - before);
        DestroyWindow(hwnd);
        HWND replacement = Window(); // a reused HWND must not receive the old job
        SetEvent(release);
        worker.join();
        CloseHandle(release);
        if (PeekMessageW(&msg, NULL, message, message, PM_REMOVE)) raceOk = false;
        DestroyWindow(replacement);
    }
    Check(raceOk && destroyed == 101, "100 late-worker/closed-HWND races release once and never post after disconnect");
    Check(maximumCancel < 100, "cancellation never waits for blocked property work");
    hwnd = Window();
    Task* stale = new Task(hwnd, &destroyed);
    InterlockedExchange(&stale->Cancelled, 1);
    stale->PostCompletion(message, (LPARAM)stale);
    Check(PeekMessageW(&msg, hwnd, message, message, PM_REMOVE) != FALSE,
          "superseded generation still completes so the live panel can schedule replacement");
    stale->CancelCompletion(message);
    stale->Release();
    DestroyWindow(hwnd);
    printf("Performance counts: full collection=174935, visible snapshot=80, batch limit=128; max cancel=%llu ms\n", maximumCancel);
    return Failures ? 1 : 0;
}
