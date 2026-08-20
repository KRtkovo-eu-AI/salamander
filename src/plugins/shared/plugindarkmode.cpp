// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "plugindarkmode.h"

#include <commctrl.h>

#if USE_DARKMODELIB
#include "../../third_party/darkmodelib/include/Darkmodelib.h"
#endif

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
const wchar_t* PLUGIN_DARKMODE_MENU_PROP = L"Salamander.PluginDarkMode.Menu";

void ConfigurePluginDarkModelib(BOOL dark)
{
#if USE_DARKMODELIB
    static bool initialized = false;
    if (!initialized)
    {
        dmlib::initDarkMode();
        initialized = true;
    }
    dmlib::setDarkModeConfigEx(static_cast<UINT>(dark ? dmlib::DarkModeType::dark
                                                      : dmlib::DarkModeType::classic));
    dmlib::setDefaultColors(true);
#else
    UNREFERENCED_PARAMETER(dark);
#endif
}

void ResetPluginBrushes()
{
    if (gDialogBrush != NULL)
    {
        DeleteObject(gDialogBrush);
        gDialogBrush = NULL;
    }
    if (gInputBrush != NULL)
    {
        DeleteObject(gInputBrush);
        gInputBrush = NULL;
    }
}

const UINT_PTR PLUGIN_DARKMODE_HEADER_SUBCLASS_ID = 0x50444844; // "PDHD"

void FillPluginColorRect(HDC hdc, const RECT* rect, COLORREF color)
{
    if (hdc == NULL || rect == NULL)
        return;
    COLORREF oldColor = SetDCBrushColor(hdc, color);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(DC_BRUSH));
    PatBlt(hdc, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, PATCOPY);
    SelectObject(hdc, oldBrush);
    SetDCBrushColor(hdc, oldColor);
}

void PaintPluginDarkHeader(HWND hwnd, HDC hdc)
{
    if (hwnd == NULL || hdc == NULL)
        return;

    PluginDarkModeColors colors = PluginDarkMode_GetColors();
    RECT client;
    GetClientRect(hwnd, &client);
    FillPluginColorRect(hdc, &client, colors.background);

    const int count = Header_GetItemCount(hwnd);
    HFONT font = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    HFONT oldFont = font != NULL ? (HFONT)SelectObject(hdc, font) : NULL;
    COLORREF oldText = SetTextColor(hdc, colors.readableText);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x55, 0x55, 0x55));
    HPEN oldPen = pen != NULL ? (HPEN)SelectObject(hdc, pen) : NULL;

    for (int i = 0; i < count; ++i)
    {
        RECT item;
        if (!Header_GetItemRect(hwnd, i, &item))
            continue;
        FillPluginColorRect(hdc, &item, colors.background);
        TCHAR text[256] = {0};
        HDITEM hdi = {0};
        hdi.mask = HDI_TEXT | HDI_FORMAT;
        hdi.pszText = text;
        hdi.cchTextMax = _countof(text);
        SendMessage(hwnd, HDM_GETITEM, i, reinterpret_cast<LPARAM>(&hdi));

        RECT textRect = item;
        textRect.left += 6;
        textRect.right -= 6;
        UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
        if ((hdi.fmt & HDF_CENTER) != 0)
            format |= DT_CENTER;
        else if ((hdi.fmt & HDF_RIGHT) != 0)
            format |= DT_RIGHT;
        else
            format |= DT_LEFT;
        DrawText(hdc, text, -1, &textRect, format);

        if (pen != NULL)
        {
            MoveToEx(hdc, item.right - 1, item.top, NULL);
            LineTo(hdc, item.right - 1, item.bottom);
            MoveToEx(hdc, item.left, item.bottom - 1, NULL);
            LineTo(hdc, item.right, item.bottom - 1);
        }
    }

    if (oldPen != NULL)
        SelectObject(hdc, oldPen);
    if (pen != NULL)
        DeleteObject(pen);
    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldText);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
}

LRESULT CALLBACK PluginDarkHeaderSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR subclassId, DWORD_PTR)
{
    if (subclassId != PLUGIN_DARKMODE_HEADER_SUBCLASS_ID)
        return DefSubclassProc(hwnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, PluginDarkHeaderSubclass, PLUGIN_DARKMODE_HEADER_SUBCLASS_ID);
        break;

    case WM_ERASEBKGND:
        if (PluginDarkMode_ShouldUseDark())
            return TRUE;
        break;

    case WM_PAINT:
        if (PluginDarkMode_ShouldUseDark())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintPluginDarkHeader(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_PRINTCLIENT:
        if (PluginDarkMode_ShouldUseDark())
        {
            PaintPluginDarkHeader(hwnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnsurePluginDarkHeaderSubclass(HWND hwnd, BOOL dark)
{
    if (hwnd == NULL)
        return;
    if (dark)
        SetWindowSubclass(hwnd, PluginDarkHeaderSubclass, PLUGIN_DARKMODE_HEADER_SUBCLASS_ID, 0);
    else
        RemoveWindowSubclass(hwnd, PluginDarkHeaderSubclass, PLUGIN_DARKMODE_HEADER_SUBCLASS_ID);
    InvalidateRect(hwnd, NULL, TRUE);
}

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
        HWND header = ListView_GetHeader(hwnd);
        if (header != NULL)
        {
            if (gSetWindowTheme != NULL)
                gSetWindowTheme(header, dark ? L"DarkMode_Explorer" : nullptr, nullptr);
            SendMessage(header, HDM_SETTEXTCOLOR, 0, static_cast<LPARAM>(dark ? c.readableText : CLR_DEFAULT));
            SendMessage(header, HDM_SETBKCOLOR, 0, static_cast<LPARAM>(dark ? c.background : CLR_DEFAULT));
            EnsurePluginDarkHeaderSubclass(header, dark);
        }
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
        EnsurePluginDarkHeaderSubclass(hwnd, dark);
        InvalidateRect(hwnd, NULL, TRUE);
        HWND parent = GetParent(hwnd);
        while (parent != NULL)
        {
            InvalidateRect(parent, NULL, TRUE);
            parent = GetParent(parent);
        }
    }
    else if (wcscmp(cls, L"Edit") == 0 || wcscmp(cls, L"ComboBox") == 0 || wcscmp(cls, L"ComboBoxEx32") == 0 ||
             wcscmp(cls, L"ListBox") == 0)
    {
        if (gSetWindowTheme != NULL)
            gSetWindowTheme(hwnd, dark ? L"CFD" : nullptr, nullptr);
        InvalidateRect(hwnd, NULL, TRUE);
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
            else
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
        wcscmp(cls, L"ToolbarWindow32") == 0 || wcscmp(cls, L"Static") == 0 ||
        wcscmp(cls, L"Edit") == 0 || wcscmp(cls, L"ComboBox") == 0 ||
        wcscmp(cls, L"ComboBoxEx32") == 0 || wcscmp(cls, L"ListBox") == 0)
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
    ResetPluginBrushes();
    gHostColors.text = text;
    gHostColors.background = background;
    gHostColors.readableText = EnsureReadable(text, background);
}

void PluginDarkMode_SetHostResolvedColors(COLORREF text, COLORREF background, COLORREF readableText)
{
    ResetPluginBrushes();
    gHostColors.text = text;
    gHostColors.background = background;
    gHostColors.readableText = readableText;
}

BOOL PluginDarkMode_ShouldUseDark()
{
    PluginDarkMode_EnsureApis();
    if (gHostPolicyAvailable)
        return gHostUseWindowsDarkScheme;
    // Plugin dark mode must follow host-selected "Windows Dark Mode (experimental)" policy only.
    // If host policy is not provided, stay in light mode to avoid accidental dark activation
    // based solely on OS/app defaults.
    return FALSE;
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
    if (gHostColors.readableText != CLR_INVALID)
        out.readableText = gHostColors.readableText;
    else
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

void PluginDarkMode_ApplyMenuBar(HWND hwnd)
{
    if (hwnd == NULL)
        return;
    const BOOL dark = PluginDarkMode_ShouldUseDark();
    ConfigurePluginDarkModelib(dark);
#if USE_DARKMODELIB
    const BOOL wasDark = GetPropW(hwnd, PLUGIN_DARKMODE_MENU_PROP) != NULL;
    // The menu-bar subclass also supplies the explicit WM_SETFONT font. Keep
    // it attached in light mode when a caller provided that font; otherwise a
    // native menu bar silently falls back to the system menu font.
    const BOOL useConfiguredFont = GetPropW(hwnd, L"OpenSalamander.UIFont") != NULL;
    if ((dark || useConfiguredFont) && !wasDark)
    {
        dmlib::setWindowMenuBarSubclass(hwnd);
        SetPropW(hwnd, PLUGIN_DARKMODE_MENU_PROP, reinterpret_cast<HANDLE>(1));
    }
    else if (!dark && !useConfiguredFont && wasDark)
    {
        dmlib::removeWindowMenuBarSubclass(hwnd);
        RemovePropW(hwnd, PLUGIN_DARKMODE_MENU_PROP);
    }
#endif
    DrawMenuBar(hwnd);
}

void PluginDarkMode_ApplyStatusBar(HWND hwnd)
{
    if (hwnd == NULL)
        return;
    const BOOL dark = PluginDarkMode_ShouldUseDark();
    ConfigurePluginDarkModelib(dark);
#if USE_DARKMODELIB
    if (dark)
        dmlib::setStatusBarCtrlSubclass(hwnd);
    else
        dmlib::removeStatusBarCtrlSubclass(hwnd);
#endif
    InvalidateRect(hwnd, NULL, TRUE);
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
    PluginDarkMode_ApplyMenuBar(hwnd);
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
            if (gSetWindowTheme != NULL)
            {
                const BOOL dark = PluginDarkMode_ShouldUseDark();
                if (type == BS_GROUPBOX)
                    gSetWindowTheme(ctrl, dark ? L"" : nullptr, nullptr);
                else if (type == BS_AUTOCHECKBOX || type == BS_CHECKBOX || type == BS_AUTO3STATE ||
                         type == BS_3STATE || type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON)
                    gSetWindowTheme(ctrl, dark ? L"" : nullptr, dark ? L"" : nullptr);
            }
        }
    }
    if (ctrl != NULL && message == WM_CTLCOLORSTATIC)
    {
        wchar_t cls[32] = {0};
        if (GetClassNameW(ctrl, cls, _countof(cls)) != 0 && lstrcmpiW(cls, L"Static") == 0)
        {
            LONG_PTR style = GetWindowLongPtrW(ctrl, GWL_STYLE);
            if ((style & (SS_ICON | SS_BITMAP | SS_BLACKRECT | SS_GRAYRECT | SS_WHITERECT)) != 0)
                return FALSE;
            if ((style & SS_NOTIFY) != 0)
                c.readableText = RGB(130, 180, 255);
        }
        else if (lstrcmpiW(cls, L"SysLink") == 0)
        {
            c.readableText = RGB(130, 180, 255);
        }
    }
    const bool isInput = (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX);
    if (dc != NULL && isInput)
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
        SetBkColor(dc, isInput ? RGB(0x2A, 0x2A, 0x2A) : c.background);
    }
    *result = reinterpret_cast<LRESULT>(brush);
    return TRUE;
}
