// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>

namespace PictViewUnicodeTitle {
// WinLibLite continues handling ANSI messages. Only the native caption boundary
// needs a Unicode procedure; CallWindowProcW performs the appropriate thunk for
// all other messages before invoking the original ANSI procedure.
static const wchar_t Property[] = L"Samandarin.PictView.UnicodeTitle.OriginalProc";
inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    WNDPROC original = reinterpret_cast<WNDPROC>(GetPropW(window, Property));
    if (message == WM_SETTEXT || message == WM_GETTEXT || message == WM_GETTEXTLENGTH)
        return DefWindowProcW(window, message, wParam, lParam);
    if (message == WM_NCDESTROY)
        RemovePropW(window, Property);
    return original != NULL ? CallWindowProcW(original, window, message, wParam, lParam)
                            : DefWindowProcW(window, message, wParam, lParam);
}
inline BOOL Install(HWND window)
{
    WNDPROC original = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
    if (original == NULL || !SetPropW(window, Property, reinterpret_cast<HANDLE>(original)))
        return FALSE;
    SetLastError(0);
    if (SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) == 0 && GetLastError() != 0)
    {
        RemovePropW(window, Property);
        return FALSE;
    }
    return TRUE;
}
}
