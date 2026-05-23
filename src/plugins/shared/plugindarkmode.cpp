// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "plugindarkmode.h"

#include <uxtheme.h>
#include <dwmapi.h>
#include <commctrl.h>

namespace
{
BOOL gHostPolicyAvailable = FALSE;
BOOL gHostUseWindowsDarkScheme = FALSE;
PluginDarkModeColors gHostColors = {CLR_INVALID, CLR_INVALID, CLR_INVALID};
HBRUSH gDialogBrush = NULL;

COLORREF EnsureReadable(COLORREF fg, COLORREF bg)
{
    const int bl = (GetRValue(bg) * 30 + GetGValue(bg) * 59 + GetBValue(bg) * 11) / 100;
    const int fl = (GetRValue(fg) * 30 + GetGValue(fg) * 59 + GetBValue(fg) * 11) / 100;
    if (bl < 128 && fl < bl + 40)
        return RGB(0xF0, 0xF0, 0xF0);
    if (bl >= 128 && fl > bl - 40)
        return RGB(0x20, 0x20, 0x20);
    return fg;
}

BOOL DetectSystemDarkFallback()
{
    HKEY key = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                      0, KEY_READ, &key) != ERROR_SUCCESS)
        return FALSE;
    DWORD value = 1;
    DWORD size = sizeof(value);
    RegQueryValueExW(key, L"AppsUseLightTheme", NULL, NULL, reinterpret_cast<LPBYTE>(&value), &size);
    RegCloseKey(key);
    return value == 0 ? TRUE : FALSE;
}

void ApplyRecursive(HWND hwnd, BOOL dark)
{
    if (hwnd == NULL)
        return;
    wchar_t cls[64] = {0};
    GetClassNameW(hwnd, cls, _countof(cls));
    if (wcscmp(cls, L"SysListView32") == 0)
    {
        SetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        PluginDarkModeColors c = PluginDarkMode_GetColors();
        ListView_SetTextColor(hwnd, c.readableText);
        ListView_SetTextBkColor(hwnd, c.background);
        ListView_SetBkColor(hwnd, c.background);
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else if (wcscmp(cls, L"SysTreeView32") == 0)
    {
        SetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        PluginDarkModeColors c = PluginDarkMode_GetColors();
        TreeView_SetTextColor(hwnd, c.readableText);
        TreeView_SetBkColor(hwnd, c.background);
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else if (wcscmp(cls, L"SysHeader32") == 0 || wcscmp(cls, L"tooltips_class32") == 0 || wcscmp(cls, L"ScrollBar") == 0)
    {
        SetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        InvalidateRect(hwnd, NULL, TRUE);
    }

    for (HWND child = GetWindow(hwnd, GW_CHILD); child != NULL; child = GetWindow(child, GW_HWNDNEXT))
        ApplyRecursive(child, dark);
}
} // namespace

void PluginDarkMode_SetHostPolicyAvailable(BOOL available, BOOL useWindowsDarkScheme)
{
    gHostPolicyAvailable = available;
    gHostUseWindowsDarkScheme = useWindowsDarkScheme;
}

void PluginDarkMode_SetHostColors(COLORREF text, COLORREF background)
{
    gHostColors.text = text;
    gHostColors.background = background;
    gHostColors.readableText = EnsureReadable(text, background);
}

BOOL PluginDarkMode_ShouldUseDark()
{
    if (gHostPolicyAvailable)
        return gHostUseWindowsDarkScheme;
    return DetectSystemDarkFallback();
}

PluginDarkModeColors PluginDarkMode_GetColors()
{
    PluginDarkModeColors out = {GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_BTNTEXT)};
    if (PluginDarkMode_ShouldUseDark())
    {
        out.background = RGB(32, 32, 32);
        out.text = RGB(220, 220, 220);
    }
    if (gHostColors.text != CLR_INVALID)
        out.text = gHostColors.text;
    if (gHostColors.background != CLR_INVALID)
        out.background = gHostColors.background;
    out.readableText = EnsureReadable(out.text, out.background);
    return out;
}

void PluginDarkMode_ApplyTitleBar(HWND hwnd)
{
    if (hwnd == NULL)
        return;
    const BOOL dark = PluginDarkMode_ShouldUseDark();
    DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
}

void PluginDarkMode_ApplyListTreeThemeRecursive(HWND hwnd)
{
    ApplyRecursive(hwnd, PluginDarkMode_ShouldUseDark());
}

HBRUSH PluginDarkMode_GetDialogCtlColorBrush(HDC dc, UINT)
{
    PluginDarkModeColors c = PluginDarkMode_GetColors();
    if (gDialogBrush != NULL)
        DeleteObject(gDialogBrush);
    gDialogBrush = CreateSolidBrush(c.background);
    if (dc != NULL)
    {
        SetTextColor(dc, c.readableText);
        SetBkColor(dc, c.background);
        SetBkMode(dc, TRANSPARENT);
    }
    return gDialogBrush;
}
