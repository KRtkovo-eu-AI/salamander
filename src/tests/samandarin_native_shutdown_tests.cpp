// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cstdio>
#include "../plugins/samandarin/native_shutdown.h"

static int Checks = 0, Failures = 0, SentCalls = 0, PostedCalls = 0;
static void Check(bool success, const char* label)
{
    ++Checks;
    if (!success) { ++Failures; std::printf("FAIL: %s (Win32 %lu)\n", label, GetLastError()); }
}
static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_APP + 1) { ++SentCalls; return 42; }
    if (message == WM_APP + 2) { ++PostedCalls; return 0; }
    return DefWindowProcW(window, message, wParam, lParam);
}
struct SendJob { HWND window; LRESULT result; };
static DWORD WINAPI SendWorker(void* parameter)
{
    SendJob* job = static_cast<SendJob*>(parameter);
    job->result = SendMessageW(job->window, WM_APP + 1, 0, 0);
    return 0;
}
static DWORD WINAPI BlockWorker(void* parameter)
{
    return WaitForSingleObject(static_cast<HANDLE>(parameter), 10000) == WAIT_OBJECT_0 ? 0 : 1;
}
struct CallbackJob
{
    CSamandarinNativeCallGate* gate;
    HANDLE entered, release;
    bool allowed;
};
static DWORD WINAPI CallbackWorker(void* parameter)
{
    CallbackJob* job = static_cast<CallbackJob*>(parameter);
    job->allowed = job->gate->Enter();
    SetEvent(job->entered);
    WaitForSingleObject(job->release, 10000);
    job->gate->Leave();
    return 0;
}
int wmain(int argc, wchar_t** argv)
{
    Check(argc > 0, "process command line identifies its owned module");
    std::wstring expected(GetFullPathNameW(argv[0], 0, NULL, NULL), L'\0');
    const DWORD length = GetFullPathNameW(argv[0], static_cast<DWORD>(expected.size()), expected.data(), NULL);
    Check(length > 0 && length < expected.size(), "independent module path resolves without truncation");
    expected.resize(length);
    expected.resize(expected.find_last_of(L'\\') + 1);
    expected.append(L"Samandarin.Managed.dll");
    const std::wstring assemblyPath = GetSamandarinManagedAssemblyPath(NULL);
    Check(assemblyPath == expected, "assembly path preserves the actual Unicode/long module directory");
    Check(GetSamandarinManagedAssemblyPath(reinterpret_cast<HINSTANCE>(static_cast<INT_PTR>(-1))).empty(),
          "invalid module yields an explicit empty path");
    std::printf("Managed assembly path: %zu UTF16 units\n", assemblyPath.size());
    Check(WaitForSamandarinInitialization(NULL, 0) != FALSE, "no initialization needs no wait");
    Check(!WaitForSamandarinInitialization(INVALID_HANDLE_VALUE, 0), "invalid handle is rejected");
    Check(!WaitForSamandarinInitialization(NULL, INFINITE), "unbounded wait is rejected");
    HANDLE gate = CreateEventW(NULL, TRUE, FALSE, NULL);
    HANDLE blocked = gate ? CreateThread(NULL, 0, BlockWorker, gate, 0, NULL) : NULL;
    Check(gate != NULL && blocked != NULL, "own blocked initialization fixture started");
    if (blocked)
    {
        const ULONGLONG started = GetTickCount64();
        Check(!WaitForSamandarinInitialization(blocked, 20), "active initialization returns timeout");
        Check(GetTickCount64() - started < 1000, "deadline does not hang the caller");
        Check(WaitForSingleObject(blocked, 0) == WAIT_TIMEOUT, "timeout preserves live worker ownership");
        SetEvent(gate);
        Check(WaitForSamandarinInitialization(blocked, 2000) != FALSE, "the same worker can complete on retry");
        CloseHandle(blocked);
    }
    if (gate) CloseHandle(gate);
    CSamandarinNativeCallGate nativeGate;
    CallbackJob callback = {&nativeGate, CreateEventW(NULL, TRUE, FALSE, NULL),
                            CreateEventW(NULL, TRUE, FALSE, NULL), false};
    HANDLE activeCallback = callback.entered && callback.release ?
        CreateThread(NULL, 0, CallbackWorker, &callback, 0, NULL) : NULL;
    Check(activeCallback != NULL, "own admitted callback fixture started");
    if (activeCallback)
    {
        const DWORD entered = WaitForSingleObject(callback.entered, 2000);
        Check(entered == WAIT_OBJECT_0 && callback.allowed, "callback admitted before process exit");
        Check(!nativeGate.TryClose(), "live callback prevents host-resource teardown");
        Check(!nativeGate.IsClosing(), "failed close preserves callback admission for retry");
        SetEvent(callback.release);
        Check(WaitForSamandarinInitialization(activeCallback, 2000) != FALSE, "admitted callback drains normally");
        Check(nativeGate.TryClose() && nativeGate.IsClosing(), "drained callbacks allow native teardown");
        Check(!nativeGate.Enter(), "callbacks after process-exit transition are rejected");
        nativeGate.Leave();
        Check(nativeGate.TryClose(), "native teardown gate is idempotent");
        CloseHandle(activeCallback);
    }
    if (callback.entered) CloseHandle(callback.entered);
    if (callback.release) CloseHandle(callback.release);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"SamandarinNativeShutdownTest";
    const ATOM atom = RegisterClassW(&wc);
    HWND window = atom ? CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0,
                                       HWND_MESSAGE, NULL, wc.hInstance, NULL) : NULL;
    Check(window != NULL, "own message-only window created");
    if (window)
    {
        Check(PostMessageW(window, WM_APP + 2, 0, 0) != FALSE, "queued UI work posted before join");
        SendJob job = {window, 0};
        HANDLE sender = CreateThread(NULL, 0, SendWorker, &job, 0, NULL);
        Check(sender != NULL, "synchronous host callback worker started");
        if (sender)
        {
            const BOOL joined = WaitForSamandarinInitialization(sender, 2000);
            Check(joined && SentCalls == 1 && job.result == 42, "sent callback dispatched while joining");
            Check(PostedCalls == 0, "posted UI work is not dispatched during teardown");
            MSG message;
            Check(PeekMessageW(&message, window, WM_APP + 2, WM_APP + 2, PM_REMOVE) != FALSE,
                  "posted UI work remains queued for its owner");
            CloseHandle(sender);
        }
        DestroyWindow(window);
    }
    if (atom) UnregisterClassW(wc.lpszClassName, wc.hInstance);
    std::printf("Samandarin native shutdown: %d checks, %d failures\n", Checks, Failures);
    return Failures ? 1 : 0;
}