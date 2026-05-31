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
static COLORREF EnsureReadableForBackground(COLORREF text, COLORREF background)
{
    const int bgLum = (GetRValue(background) * 30 + GetGValue(background) * 59 + GetBValue(background) * 11) / 100;
    const int fgLum = (GetRValue(text) * 30 + GetGValue(text) * 59 + GetBValue(text) * 11) / 100;
    if (bgLum < 128 && fgLum < bgLum + 40)
        return RGB(0xF0, 0xF0, 0xF0);
    if (bgLum >= 128 && fgLum > bgLum - 40)
        return RGB(0x20, 0x20, 0x20);
    return text;
}

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
    {
        static bool dmlibInitialized = false;
        if (!dmlibInitialized)
        {
            dmlib::initDarkMode();
            dmlibInitialized = true;
        }
        const bool dark = DarkMode_ShouldUseDark() != FALSE;
        dmlib::setDarkModeConfigEx(static_cast<UINT>(dark ? dmlib::DarkModeType::dark : dmlib::DarkModeType::light));
        dmlib::setDefaultColors(true);
        dmlib::setDarkWndNotifySafe(hwnd);
        dmlib::setChildCtrlsSubclassAndTheme(hwnd);
        dmlib::setDarkTitleBar(hwnd, dark);
    }
#else
    (void)hwnd;
#endif
}

void ApplyCheckboxOrRadioButton(HWND hwnd, bool enableDark)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
    {
        if (enableDark)
            dmlib::setCheckboxOrRadioBtnCtrlSubclass(hwnd);
        else
            dmlib::removeCheckboxOrRadioBtnCtrlSubclass(hwnd);
    }
#else
    (void)hwnd;
    (void)enableDark;
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
            COLORREF textColor = colors.readableText;
            if (message == WM_CTLCOLORBTN && lParam != 0)
            {
                HWND ctrl = reinterpret_cast<HWND>(lParam);
                wchar_t className[16] = {0};
                if (GetClassNameW(ctrl, className, _countof(className)) != 0 && lstrcmpiW(className, L"Button") == 0)
                {
                    LONG_PTR style = GetWindowLongPtr(ctrl, GWL_STYLE);
                    LONG_PTR type = style & BS_TYPEMASK;
                    if (type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON ||
                        type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
                        type == BS_AUTO3STATE || type == BS_3STATE)
                    {
                        // Follow darkmodelib demo behavior expectation: button labels in dark mode
                        // must stay high-contrast (light) regardless of host palette drift.
                        textColor = dmlib::isDarkTheme() ? RGB(0xF0, 0xF0, 0xF0)
                                                         : EnsureReadableForBackground(colors.readableText, colors.background);
                    }
                }
            }
            SetTextColor(hdc, textColor);
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
