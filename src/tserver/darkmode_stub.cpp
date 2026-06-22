// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "../darkmode.h"

bool DarkModeInitialize() { return false; }
bool DarkModeIsSupported() { return false; }
void DarkModeSetEnabled(bool enabled) { UNREFERENCED_PARAMETER(enabled); }
bool DarkModeShouldUseDarkColors() { return false; }
BOOL DarkMode_ShouldUseDark() { return FALSE; }
void DarkModeApplyWindow(HWND hwnd) { UNREFERENCED_PARAMETER(hwnd); }
void DarkModeApplyTree(HWND hwnd) { UNREFERENCED_PARAMETER(hwnd); }
void DarkModeApplyDropdownListTheme(HWND hwnd) { UNREFERENCED_PARAMETER(hwnd); }
void DarkModeRefreshTree(HWND hwnd) { UNREFERENCED_PARAMETER(hwnd); }
void DarkModeApplyMenuBar(HWND hwnd) { UNREFERENCED_PARAMETER(hwnd); }
void DarkModeRefreshTitleBar(HWND hwnd) { UNREFERENCED_PARAMETER(hwnd); }
bool DarkModeHandleSettingChange(UINT message, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(lParam);
    return false;
}
void DarkModeFixScrollbars() {}
void DarkModeConfigureDialogColors(COLORREF textColor, COLORREF backgroundColor, HBRUSH dialogBrush)
{
    UNREFERENCED_PARAMETER(textColor);
    UNREFERENCED_PARAMETER(backgroundColor);
    UNREFERENCED_PARAMETER(dialogBrush);
}
void DarkModeSetConfiguredColors(COLORREF schemeTextColor, COLORREF schemeBackgroundColor,
                                 COLORREF fallbackTextColor, COLORREF fallbackBackgroundColor)
{
    UNREFERENCED_PARAMETER(schemeTextColor);
    UNREFERENCED_PARAMETER(schemeBackgroundColor);
    UNREFERENCED_PARAMETER(fallbackTextColor);
    UNREFERENCED_PARAMETER(fallbackBackgroundColor);
}
const DarkModeColors& DarkModeGetColors()
{
    static const DarkModeColors colors = {GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_WINDOW),
                                          GetSysColor(COLOR_WINDOWTEXT), false};
    return colors;
}
bool DarkModeHandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(result);
    return false;
}
HBRUSH DarkModeGetPanelFrameBrush() { return NULL; }
COLORREF DarkModeGetDialogTextColor() { return GetSysColor(COLOR_WINDOWTEXT); }
COLORREF DarkModeGetDialogBackgroundColor() { return GetSysColor(COLOR_WINDOW); }
COLORREF DarkModeEnsureReadableForeground(COLORREF foreground, COLORREF background)
{
    UNREFERENCED_PARAMETER(background);
    return foreground;
}
void DarkModeUpdateListViewColors(HWND listView) { UNREFERENCED_PARAMETER(listView); }
void DarkModeUpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors)
{
    UNREFERENCED_PARAMETER(listView);
    UNREFERENCED_PARAMETER(textColor);
    UNREFERENCED_PARAMETER(backgroundColor);
    UNREFERENCED_PARAMETER(applyHeaderColors);
}
void DarkModeApplyStaticTextColors(HWND hwndParent, HWND specificCtrl)
{
    UNREFERENCED_PARAMETER(hwndParent);
    UNREFERENCED_PARAMETER(specificCtrl);
}
void DarkModeUpdateTabControlOverflowButtons(HWND tabControl) { UNREFERENCED_PARAMETER(tabControl); }
