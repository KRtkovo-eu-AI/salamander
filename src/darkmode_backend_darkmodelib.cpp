// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "darkmode_backend_darkmodelib.h"
#include "darkmode.h"

#if USE_DARKMODELIB
#include "third_party/darkmodelib/include/Darkmodelib.h"
#endif

namespace DarkModeBackendDarkModelib
{
bool IsAvailable()
{
#if USE_DARKMODELIB
    return true;
#else
    return false;
#endif
}

void ApplyTree(HWND hwnd)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
        dmlib::setDarkTitleBar(hwnd, true);
#else
    (void)hwnd;
#endif
}

bool HandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result,
                    const DarkModeColors& colors, HBRUSH dialogBrush)
{
#if USE_DARKMODELIB
    if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN || message == WM_CTLCOLOREDIT ||
        message == WM_CTLCOLORLISTBOX || message == WM_CTLCOLORDLG || message == WM_CTLCOLORMSGBOX ||
        message == WM_CTLCOLORSCROLLBAR)
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        if (hdc != NULL)
        {
            SetTextColor(hdc, colors.readableText);
            SetBkColor(hdc, colors.background);
            SetBkMode(hdc, TRANSPARENT);
            result = reinterpret_cast<LRESULT>(dialogBrush != NULL ? dialogBrush : GetSysColorBrush(COLOR_BTNFACE));
            return true;
        }
    }
#else
    (void)message;
    (void)wParam;
    (void)lParam;
    (void)result;
    (void)colors;
    (void)dialogBrush;
#endif
    return false;
}

void ApplyStaticTextColors(HWND hwndParent, HWND specificCtrl, const DarkModeColors& colors)
{
#if USE_DARKMODELIB
    (void)colors;
    if (specificCtrl != NULL)
        InvalidateRect(specificCtrl, NULL, TRUE);
    else if (hwndParent != NULL)
        InvalidateRect(hwndParent, NULL, TRUE);
#else
    (void)hwndParent;
    (void)specificCtrl;
    (void)colors;
#endif
}

void UpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors)
{
#if USE_DARKMODELIB
    (void)applyHeaderColors;
    if (listView != NULL)
    {
        ListView_SetTextColor(listView, textColor);
        ListView_SetTextBkColor(listView, backgroundColor);
        ListView_SetBkColor(listView, backgroundColor);
        InvalidateRect(listView, NULL, TRUE);
    }
#else
    (void)listView;
    (void)textColor;
    (void)backgroundColor;
    (void)applyHeaderColors;
#endif
}
} // namespace DarkModeBackendDarkModelib

