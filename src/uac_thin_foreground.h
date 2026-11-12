// | Date | Summary | Reason | Ticket | Model |
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
#endif
