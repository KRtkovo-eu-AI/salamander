// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// WinLib is also compiled by plugins which still support older Windows SDK
// headers. Keep the PMv2 helpers dynamically linked and avoid making the
// public WinLib ABI depend on newer SDK types.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef WM_DPICHANGED_BEFOREPARENT
#define WM_DPICHANGED_BEFOREPARENT 0x02E2
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

typedef HANDLE(WINAPI* CWinLibSetThreadDpiAwarenessContext)(HANDLE dpiContext);
typedef UINT(WINAPI* CWinLibGetDpiForWindow)(HWND hwnd);

inline CWinLibSetThreadDpiAwarenessContext WinLibDPIGetSetThreadContext()
{
    static CWinLibSetThreadDpiAwarenessContext setThreadDpiAwarenessContext = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            setThreadDpiAwarenessContext = (CWinLibSetThreadDpiAwarenessContext)GetProcAddress(user32, "SetThreadDpiAwarenessContext");
        loaded = TRUE;
    }
    return setThreadDpiAwarenessContext;
}

// Windows captures a window's DPI awareness from the creating thread. Plugin
// callbacks and CLR-hosted code can temporarily change that thread context, so
// every WinLib top-level window is created under PMv2 explicitly and the
// caller's original context is restored immediately afterwards.
class CWinLibDPIContext
{
public:
    CWinLibDPIContext()
    {
        SetThreadContext = WinLibDPIGetSetThreadContext();
        OldContext = SetThreadContext != NULL
                         ? SetThreadContext((HANDLE)-4 /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */)
                         : NULL;
    }

    ~CWinLibDPIContext()
    {
        if (SetThreadContext != NULL && OldContext != NULL)
            SetThreadContext(OldContext);
    }

private:
    CWinLibDPIContext(const CWinLibDPIContext&);
    CWinLibDPIContext& operator=(const CWinLibDPIContext&);

    CWinLibSetThreadDpiAwarenessContext SetThreadContext;
    HANDLE OldContext;
};

inline UINT WinLibDPIGetWindowDPI(HWND hwnd)
{
    static CWinLibGetDpiForWindow getDpiForWindow = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            getDpiForWindow = (CWinLibGetDpiForWindow)GetProcAddress(user32, "GetDpiForWindow");
        loaded = TRUE;
    }

    if (getDpiForWindow != NULL && hwnd != NULL)
    {
        UINT dpi = getDpiForWindow(hwnd);
        if (dpi != 0)
            return dpi;
    }

    HDC dc = GetDC(hwnd);
    if (dc != NULL)
    {
        int dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(hwnd, dc);
        if (dpi > 0)
            return (UINT)dpi;
    }
    return USER_DEFAULT_SCREEN_DPI;
}

inline int WinLibDPIToLogical(HWND hwnd, int pixels)
{
    return MulDiv(pixels, USER_DEFAULT_SCREEN_DPI, (int)WinLibDPIGetWindowDPI(hwnd));
}

inline int WinLibDPIFromLogical(HWND hwnd, int logical)
{
    return MulDiv(logical, (int)WinLibDPIGetWindowDPI(hwnd), USER_DEFAULT_SCREEN_DPI);
}

// Unlike dialogs, ordinary top-level windows are not automatically moved to
// the rectangle supplied with WM_DPICHANGED. Their derived WindowProc still
// receives the message first; this is the safe common fallback for windows
// which only need their normal WM_SIZE layout pass.
inline BOOL WinLibDPIApplySuggestedRect(HWND hwnd, UINT message, LPARAM lParam)
{
    if (message != WM_DPICHANGED || hwnd == NULL || lParam == 0 ||
        (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD) != 0)
    {
        return FALSE;
    }

    const RECT* suggested = (const RECT*)lParam;
    return SetWindowPos(hwnd, NULL, suggested->left, suggested->top,
                        suggested->right - suggested->left,
                        suggested->bottom - suggested->top,
                        SWP_NOACTIVATE | SWP_NOZORDER);
}
