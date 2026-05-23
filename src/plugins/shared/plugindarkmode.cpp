// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "plugindarkmode.h"

#include <commctrl.h>

#ifndef HDM_SETBKCOLOR
#define HDM_SETBKCOLOR (HDM_FIRST + 29)
#endif

#ifndef HDM_SETTEXTCOLOR
#define HDM_SETTEXTCOLOR (HDM_FIRST + 30)
#endif

namespace
{
using fnSetWindowTheme = HRESULT(WINAPI*)(HWND, LPCWSTR, LPCWSTR);
using fnDwmSetWindowAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);

BOOL gHostPolicyAvailable = FALSE;
BOOL gHostUseWindowsDarkScheme = FALSE;
PluginDarkModeColors gHostColors = {CLR_INVALID, CLR_INVALID, CLR_INVALID};
HBRUSH gDialogBrush = NULL;
HBRUSH gInputBrush = NULL;
fnSetWindowTheme gSetWindowTheme = NULL;
fnDwmSetWindowAttribute gDwmSetWindowAttribute = NULL;
thread_local int gThemeBatchDepth = 0;

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
        if (gSetWindowTheme != NULL)
            gSetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        PluginDarkModeColors c = PluginDarkMode_GetColors();
        ListView_SetTextColor(hwnd, c.readableText);
        ListView_SetTextBkColor(hwnd, c.background);
        ListView_SetBkColor(hwnd, c.background);
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else if (wcscmp(cls, L"SysTreeView32") == 0)
    {
        if (gSetWindowTheme != NULL)
            gSetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        PluginDarkModeColors c = PluginDarkMode_GetColors();
        TreeView_SetTextColor(hwnd, c.readableText);
        TreeView_SetBkColor(hwnd, c.background);
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else if (wcscmp(cls, L"SysHeader32") == 0)
    {
        if (gSetWindowTheme != NULL)
            gSetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        PluginDarkModeColors c = PluginDarkMode_GetColors();
        SendMessage(hwnd, HDM_SETTEXTCOLOR, 0, static_cast<LPARAM>(dark ? c.readableText : CLR_DEFAULT));
        SendMessage(hwnd, HDM_SETBKCOLOR, 0, static_cast<LPARAM>(dark ? c.background : CLR_DEFAULT));
        InvalidateRect(hwnd, NULL, TRUE);
        HWND parent = GetParent(hwnd);
        while (parent != NULL)
        {
            InvalidateRect(parent, NULL, TRUE);
            parent = GetParent(parent);
        }
    }
    else if (wcscmp(cls, L"tooltips_class32") == 0 || wcscmp(cls, L"ScrollBar") == 0 ||
             wcscmp(cls, L"ReBarWindow32") == 0 || wcscmp(cls, L"ToolbarWindow32") == 0)
    {
        if (gSetWindowTheme != NULL)
            gSetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else if (wcscmp(cls, L"Button") == 0)
    {
        const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        const LONG_PTR type = style & BS_TYPEMASK;
        if (gSetWindowTheme != NULL)
        {
            if (type == BS_GROUPBOX)
                gSetWindowTheme(hwnd, dark ? L"" : nullptr, nullptr);
            else if (type == BS_AUTOCHECKBOX || type == BS_CHECKBOX || type == BS_AUTO3STATE ||
                     type == BS_3STATE || type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON)
                gSetWindowTheme(hwnd, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }
    else if (wcscmp(cls, L"Static") == 0)
    {
        const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
        const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
        if ((exStyle & WS_EX_STATICEDGE) == WS_EX_STATICEDGE || (style & SS_ETCHEDFRAME) == SS_ETCHEDFRAME)
        {
            InvalidateRect(hwnd, NULL, TRUE);
            HWND parent = GetParent(hwnd);
            while (parent != NULL)
            {
                InvalidateRect(parent, NULL, TRUE);
                parent = GetParent(parent);
            }
        }
    }

    for (HWND child = GetWindow(hwnd, GW_CHILD); child != NULL; child = GetWindow(child, GW_HWNDNEXT))
        ApplyRecursive(child, dark);
}

bool IsThemeChangeMessageRelevant(UINT message, LPARAM lParam)
{
    if (message == WM_THEMECHANGED)
        return true;
    if (message != WM_SETTINGCHANGE)
        return false;
    if (lParam == 0)
        return true;
    LPCWSTR key = reinterpret_cast<LPCWSTR>(lParam);
    return CompareStringOrdinal(key, -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL ||
           CompareStringOrdinal(key, -1, L"WindowsThemeElement", -1, TRUE) == CSTR_EQUAL;
}

struct ThemeBatchScope
{
    ThemeBatchScope() { ++gThemeBatchDepth; }
    ~ThemeBatchScope() { --gThemeBatchDepth; }
    bool Root() const { return gThemeBatchDepth == 1; }
};

void InvalidateKnownDarkArtifacts(HWND hwnd)
{
    if (hwnd == NULL)
        return;
    wchar_t cls[64] = {0};
    if (GetClassNameW(hwnd, cls, _countof(cls)) == 0)
        return;
    if (wcscmp(cls, L"SysHeader32") == 0 || wcscmp(cls, L"ReBarWindow32") == 0 ||
        wcscmp(cls, L"ToolbarWindow32") == 0 || wcscmp(cls, L"Static") == 0)
        InvalidateRect(hwnd, NULL, TRUE);
    for (HWND child = GetWindow(hwnd, GW_CHILD); child != NULL; child = GetWindow(child, GW_HWNDNEXT))
        InvalidateKnownDarkArtifacts(child);
}
} // namespace

static void PluginDarkMode_EnsureApis()
{
    static bool loaded = false;
    if (loaded)
        return;
    loaded = true;

    HMODULE ux = LoadLibraryW(L"uxtheme.dll");
    if (ux != NULL)
        gSetWindowTheme = reinterpret_cast<fnSetWindowTheme>(GetProcAddress(ux, "SetWindowTheme"));

    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (dwm != NULL)
        gDwmSetWindowAttribute = reinterpret_cast<fnDwmSetWindowAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
}

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

void PluginDarkMode_SetHostResolvedColors(COLORREF text, COLORREF background, COLORREF readableText)
{
    gHostColors.text = text;
    gHostColors.background = background;
    gHostColors.readableText = readableText;
}

BOOL PluginDarkMode_ShouldUseDark()
{
    PluginDarkMode_EnsureApis();
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
    PluginDarkMode_EnsureApis();
    if (hwnd == NULL)
        return;
    const BOOL dark = PluginDarkMode_ShouldUseDark();
    if (gDwmSetWindowAttribute != NULL)
        gDwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
}

void PluginDarkMode_ApplyListTreeThemeRecursive(HWND hwnd)
{
    PluginDarkMode_EnsureApis();
    ApplyRecursive(hwnd, PluginDarkMode_ShouldUseDark());
}

HBRUSH PluginDarkMode_GetDialogCtlColorBrush(HDC dc, UINT)
{
    if (!PluginDarkMode_ShouldUseDark())
        return NULL;
    PluginDarkModeColors c = PluginDarkMode_GetColors();
    if (gDialogBrush == NULL)
        gDialogBrush = CreateSolidBrush(c.background);
    if (gInputBrush == NULL)
    {
        COLORREF inputBg = RGB(0x2A, 0x2A, 0x2A);
        gInputBrush = CreateSolidBrush(inputBg);
    }
    if (dc != NULL)
    {
        SetTextColor(dc, c.readableText);
        SetBkColor(dc, c.background);
        SetBkMode(dc, TRANSPARENT);
    }
    return gDialogBrush;
}

BOOL PluginDarkMode_HandleThemeMessage(HWND hwnd, UINT message, LPARAM lParam)
{
    PluginDarkMode_EnsureApis();
    if (!IsThemeChangeMessageRelevant(message, lParam))
        return FALSE;
    ThemeBatchScope scope;
    if (!scope.Root())
        return TRUE;
    PluginDarkMode_ApplyTitleBar(hwnd);
    PluginDarkMode_ApplyListTreeThemeRecursive(hwnd);
    InvalidateKnownDarkArtifacts(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
    return TRUE;
}

BOOL PluginDarkMode_HandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    if (result == NULL)
        return FALSE;
    if (message != WM_CTLCOLORSTATIC && message != WM_CTLCOLORBTN && message != WM_CTLCOLOREDIT &&
        message != WM_CTLCOLORLISTBOX && message != WM_CTLCOLORDLG && message != WM_CTLCOLORMSGBOX)
        return FALSE;

    HDC dc = reinterpret_cast<HDC>(wParam);
    HWND ctrl = reinterpret_cast<HWND>(lParam);
    PluginDarkModeColors c = PluginDarkMode_GetColors();
    HBRUSH brush = PluginDarkMode_GetDialogCtlColorBrush(dc, message);
    if (brush == NULL)
        return FALSE;
    if (ctrl != NULL && message == WM_CTLCOLORBTN)
    {
        wchar_t cls[16] = {0};
        if (GetClassNameW(ctrl, cls, _countof(cls)) != 0 && lstrcmpiW(cls, L"Button") == 0)
        {
            LONG_PTR style = GetWindowLongPtrW(ctrl, GWL_STYLE);
            LONG_PTR type = style & BS_TYPEMASK;
            if (type == BS_GROUPBOX && gSetWindowTheme != NULL)
                gSetWindowTheme(ctrl, PluginDarkMode_ShouldUseDark() ? L"" : nullptr, nullptr);
        }
    }
    if (ctrl != NULL && message == WM_CTLCOLORSTATIC)
    {
        wchar_t cls[16] = {0};
        if (GetClassNameW(ctrl, cls, _countof(cls)) != 0 && lstrcmpiW(cls, L"Static") == 0)
        {
            LONG_PTR style = GetWindowLongPtrW(ctrl, GWL_STYLE);
            if ((style & (SS_ICON | SS_BITMAP | SS_BLACKRECT | SS_GRAYRECT | SS_WHITERECT)) != 0)
                return FALSE;
        }
    }
    if (dc != NULL && (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX))
    {
        SetBkMode(dc, OPAQUE);
        SetBkColor(dc, RGB(0x2A, 0x2A, 0x2A));
        brush = gInputBrush != NULL ? gInputBrush : brush;
    }
    else if (dc != NULL)
        SetBkMode(dc, TRANSPARENT);
    if (dc != NULL)
    {
        SetTextColor(dc, c.readableText);
        SetBkColor(dc, c.background);
    }
    *result = reinterpret_cast<LRESULT>(brush);
    return TRUE;
}
