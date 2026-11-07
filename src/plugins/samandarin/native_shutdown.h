// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>

// Metadata is operation-owned rather than retained by a CLR-pinned DLL.
inline std::wstring GetSamandarinManagedAssemblyPath(HINSTANCE module)
{
    std::wstring path(256, L'\0');
    for (;;)
    {
        const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0)
            return std::wstring();
        if (length < path.size())
        {
            path.resize(length);
            const size_t slash = path.find_last_of(L'\\');
            path.resize(slash == std::wstring::npos ? 0 : slash + 1);
            path.append(L"Samandarin.Managed.dll");
            return path;
        }
        path.resize(path.size() * 2);
    }
}

// A close may admit no new native callbacks and may reclaim host pointers only
// after existing callbacks return. Failure leaves the gate open for a retry.
class CSamandarinNativeCallGate
{
public:
    bool Enter()
    {
        AcquireSRWLockShared(&Lock);
        return Closing == 0;
    }
    void Leave() { ReleaseSRWLockShared(&Lock); }
    bool TryClose()
    {
        if (!TryAcquireSRWLockExclusive(&Lock))
            return false;
        InterlockedExchange(&Closing, 1);
        ReleaseSRWLockExclusive(&Lock);
        return true;
    }
    bool IsClosing() { return InterlockedCompareExchange(&Closing, 0, 0) != 0; }
private:
    SRWLOCK Lock = SRWLOCK_INIT;
    LONG Closing = 0;
};
// Adapted from shared/salamatrix_thread_join.h: retain its sent-message-only
// dispatch, but use a deadline for the optional Samandarin native exit hook.
// Posted UI work is not consumed or dispatched during plug-in teardown.
inline BOOL WaitForSamandarinInitialization(HANDLE thread, DWORD timeoutMs)
{
    if (timeoutMs == INFINITE)
        return FALSE;
    if (thread == NULL)
        return TRUE;
    if (thread == INVALID_HANDLE_VALUE)
        return FALSE;
    const ULONGLONG started = GetTickCount64();
    for (;;)
    {
        const ULONGLONG elapsed = GetTickCount64() - started;
        const DWORD remaining = elapsed < timeoutMs ? timeoutMs - static_cast<DWORD>(elapsed) : 0;
        const DWORD result = MsgWaitForMultipleObjects(1, &thread, FALSE, remaining, QS_SENDMESSAGE);
        if (result == WAIT_OBJECT_0)
            return TRUE;
        if (result != WAIT_OBJECT_0 + 1 || elapsed >= timeoutMs)
            return FALSE;
        MSG message;
        PeekMessage(&message, NULL, WM_NULL, WM_NULL, PM_NOREMOVE);
    }
}