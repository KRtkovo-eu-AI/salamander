// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Hand completion to the main window thread. | Avoid losing cross-thread activation when progress closes. | #731 | GPT-6 |
// | 2026-11-12 | Guard thin F5 foreground return decisions. | Preserve user focus and the native operation queue. | #731 | GPT-6 |
#pragma once

#ifdef SAM_UAC_THIN_COPY_TEST
// These helpers must be called on the progress dialog's owning thread.
// A last-input/foreground snapshot is not a security boundary and Windows
// can still refuse activation. An uncertain state never requests focus.
inline bool ThinProgressSnapshotMatches(HWND expectedForeground, DWORD expectedInput)
{
    LASTINPUTINFO input{sizeof(input)};
    return GetForegroundWindow() == expectedForeground && GetLastInputInfo(&input) &&
           input.dwTime == expectedInput;
}

inline bool ThinProgressIsActive(HWND progress, DWORD expectedInput)
{
    DWORD process = 0;
    const DWORD thread = GetWindowThreadProcessId(progress, &process);
    return thread == GetCurrentThreadId() && process == GetCurrentProcessId() &&
           IsWindowVisible(progress) && IsWindowEnabled(progress) && !IsIconic(progress) &&
           ThinProgressSnapshotMatches(progress, expectedInput);
}

inline HWND ThinProgressReturnTarget(HWND progress, HWND mainWindow, bool returned)
{
    if (!returned || !progress || !mainWindow || GetForegroundWindow() != progress)
        return nullptr;
    DWORD process = 0;
    if (!GetWindowThreadProcessId(mainWindow, &process) || process != GetCurrentProcessId() ||
        GetAncestor(mainWindow, GA_ROOT) != mainWindow || !IsWindowVisible(mainWindow) ||
        !IsWindowEnabled(mainWindow) || IsIconic(mainWindow))
        return nullptr;
    return mainWindow;
}

// A unique per-handoff value and snapshot are attached to the actual progress
// HWND. Delivery after removal is rejected, including when Windows recycles
// the numerical HWND. An already-running receiver can still be preempted
// between its final validation and SetForegroundWindow.
constexpr UINT thinMainForegroundHandoffMessage = WM_APP + 430;
constexpr wchar_t thinHandoffCookieProperty[] = L"Samandarin.ThinFocus.Cookie";
constexpr wchar_t thinHandoffInputProperty[] = L"Samandarin.ThinFocus.Input";

inline bool ThinMainForegroundHandoff(HWND mainWindow, HWND progress, ULONG_PTR cookie)
{
    if (!cookie || reinterpret_cast<ULONG_PTR>(GetPropW(progress, thinHandoffCookieProperty)) != cookie)
        return false;
    const ULONG_PTR inputValue = reinterpret_cast<ULONG_PTR>(GetPropW(progress, thinHandoffInputProperty));
    if (!inputValue || inputValue > 0xFFFFFFFFULL)
        return false;
    DWORD mainProcess = 0, progressProcess = 0;
    const DWORD mainThread = GetWindowThreadProcessId(mainWindow, &mainProcess);
    const DWORD progressThread = GetWindowThreadProcessId(progress, &progressProcess);
    if (!mainThread || mainThread != GetCurrentThreadId() || mainProcess != GetCurrentProcessId() ||
        !progressThread || progressProcess != mainProcess ||
        GetAncestor(mainWindow, GA_ROOT) != mainWindow || GetAncestor(progress, GA_ROOT) != progress ||
        !IsWindowVisible(mainWindow) || !IsWindowEnabled(mainWindow) || IsIconic(mainWindow) ||
        !IsWindowVisible(progress) || !IsWindowEnabled(progress) || IsIconic(progress) ||
        !ThinProgressSnapshotMatches(progress, static_cast<DWORD>(inputValue)) ||
        reinterpret_cast<ULONG_PTR>(GetPropW(progress, thinHandoffCookieProperty)) != cookie)
        return false;
    return SetForegroundWindow(mainWindow) != FALSE;
}

inline bool ThinSendMainForegroundHandoff(HWND progress, HWND mainWindow, DWORD expectedInput, bool queuedNext)
{
    if (queuedNext || !expectedInput || !ThinProgressIsActive(progress, expectedInput) ||
        ThinProgressReturnTarget(progress, mainWindow, true) != mainWindow)
        return false;
    // Never reuse a value in this process, including on 32-bit hosts.
    alignas(8) static volatile LONG64 generation = 0;
    const LONG64 next = InterlockedIncrement64(&generation);
    if (next <= 0 || static_cast<ULONGLONG>(next) > 0xFFFFFFFFULL)
        return false;
    const ULONG_PTR cookie = static_cast<ULONG_PTR>(next);
    if (!SetPropW(progress, thinHandoffInputProperty, reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(expectedInput))))
        return false;
    if (!SetPropW(progress, thinHandoffCookieProperty, reinterpret_cast<HANDLE>(cookie)))
    {
        RemovePropW(progress, thinHandoffInputProperty);
        return false;
    }
    DWORD_PTR response = 0;
    // No SMTO_BLOCK: activation may synchronously message this UI thread.
    // Arguments are values; no pointer to sender-owned stack storage crosses.
    const LRESULT sent = SendMessageTimeoutW(mainWindow, thinMainForegroundHandoffMessage,
                                              reinterpret_cast<WPARAM>(progress), static_cast<LPARAM>(cookie),
                                              SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 750, &response);
    if (reinterpret_cast<ULONG_PTR>(GetPropW(progress, thinHandoffCookieProperty)) == cookie)
    {
        RemovePropW(progress, thinHandoffCookieProperty);
        RemovePropW(progress, thinHandoffInputProperty);
    }
    return sent != 0 && response != 0;
}
#endif
