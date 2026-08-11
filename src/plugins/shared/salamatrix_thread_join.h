// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Salamatrix
{
namespace Runtime
{

// A runtime pump can be blocked in a synchronous SendMessage while asking the
// Salamander UI thread to execute a host call. If that UI thread waits for the
// pump without dispatching sent messages, both threads deadlock and a timed
// release would leave executable callbacks in an unloaded plug-in. Dispatch
// only nonqueued sent messages here; posted UI work remains untouched.
inline BOOL WaitForThreadWithSentMessageDispatch(
    HANDLE thread, HWND mainWindow)
{
    if (thread == NULL || thread == INVALID_HANDLE_VALUE)
        return FALSE;

    const DWORD mainThreadId =
        mainWindow != NULL ? GetWindowThreadProcessId(mainWindow, NULL) : 0;
    if (mainThreadId == 0 || mainThreadId != GetCurrentThreadId())
        return WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0;

    for (;;)
    {
        const DWORD wait = MsgWaitForMultipleObjects(
            1, &thread, FALSE, INFINITE, QS_SENDMESSAGE);
        if (wait == WAIT_OBJECT_0)
            return TRUE;
        if (wait != WAIT_OBJECT_0 + 1)
            return FALSE;

        // PeekMessage dispatches pending cross-thread sent messages before it
        // examines the posted-message queue. PM_NOREMOVE and the WM_NULL-only
        // range ensure shutdown does not consume or dispatch posted UI work.
        MSG message;
        PeekMessage(&message, NULL, WM_NULL, WM_NULL, PM_NOREMOVE);
    }
}

} // namespace Runtime
} // namespace Salamatrix
