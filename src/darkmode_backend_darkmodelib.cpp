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

#if USE_DARKMODELIB
static const wchar_t* DARKMODELIB_TREE_STATE_PROP = L"Salamander.DarkModeLib.TreeState";
static const wchar_t* DARKMODELIB_MENU_DARK_PROP = L"Salamander.DarkModeLib.MenuDark";
static const wchar_t* DARKMODELIB_CUSTOM_TAB_PROP = L"Salamander.DarkModeLib.CustomTab";

void RestoreCustomTabControls(HWND hwndParent);

static void RemoveControlSubclass(HWND hwnd)
{
    if (hwnd == NULL)
        return;

    wchar_t className[64] = {0};
    if (GetClassNameW(hwnd, className, _countof(className)) == 0)
        return;

    if (lstrcmpiW(className, L"Button") == 0)
    {
        dmlib::removeCheckboxOrRadioBtnCtrlSubclass(hwnd);
        dmlib::removeGroupboxCtrlSubclass(hwnd);
    }
    else if (lstrcmpiW(className, L"Static") == 0)
        dmlib::removeStaticTextCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"ComboBox") == 0)
        dmlib::removeComboBoxCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"ComboBoxEx32") == 0)
        dmlib::removeComboBoxExCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"Edit") == 0 || lstrcmpiW(className, L"ListBox") == 0)
        dmlib::removeCustomBorderForListBoxOrEditCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"SysListView32") == 0)
        dmlib::removeListViewCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"SysHeader32") == 0)
        dmlib::removeHeaderCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"msctls_updown32") == 0)
        dmlib::removeUpDownCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"SysTabControl32") == 0)
        dmlib::removeTabCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"msctls_statusbar32") == 0)
        dmlib::removeStatusBarCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"msctls_progress32") == 0)
        dmlib::removeProgressBarCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"SysIPAddress32") == 0)
        dmlib::removeIPAddressCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"msctls_hotkey32") == 0)
        dmlib::removeHotKeyCtrlSubclass(hwnd);
    else if (lstrcmpiW(className, L"SysDateTimePick32") == 0)
        dmlib::removeDTPCtrlSubclass(hwnd);
}

static BOOL CALLBACK RemoveControlSubclassProc(HWND hwnd, LPARAM)
{
    RemoveControlSubclass(hwnd);
    return TRUE;
}

static void RestoreCustomTabControl(HWND hwnd)
{
    if (hwnd == NULL || GetPropW(hwnd, DARKMODELIB_CUSTOM_TAB_PROP) == NULL)
        return;

    dmlib::removeTabCtrlSubclass(hwnd);
    dmlib::removeTabCtrlUpDownSubclass(hwnd);
}

static BOOL CALLBACK RestoreCustomTabControlProc(HWND hwnd, LPARAM)
{
    RestoreCustomTabControl(hwnd);
    return TRUE;
}

static void RemoveWindowAndChildSubclasses(HWND hwnd)
{
    dmlib::removeWindowCtlColorSubclass(hwnd);
    dmlib::removeWindowNotifyCustomDrawSubclass(hwnd);
    dmlib::removeWindowSettingChangeSubclass(hwnd);
    dmlib::removeWindowEraseBgSubclass(hwnd);
    dmlib::removeWindowMenuBarSubclass(hwnd);
    EnumChildWindows(hwnd, RemoveControlSubclassProc, 0);
}
#endif
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

static void ConfigureDarkModelib(bool dark)
{
#if USE_DARKMODELIB
    static bool dmlibInitialized = false;
    if (!dmlibInitialized)
    {
        dmlib::initDarkMode();
        dmlibInitialized = true;
    }
    dmlib::setDarkModeConfigEx(static_cast<UINT>(dark ? dmlib::DarkModeType::dark : dmlib::DarkModeType::classic));
    dmlib::setDefaultColors(true);
#else
    (void)dark;
#endif
}

void ApplyTree(HWND hwnd)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
    {
        const bool dark = DarkMode_ShouldUseDark() != FALSE;
        HANDLE appliedState = GetPropW(hwnd, DARKMODELIB_TREE_STATE_PROP);
        const bool wasApplied = appliedState != NULL;
        const bool wasDark = appliedState == reinterpret_cast<HANDLE>(2);
        if (wasApplied && wasDark == dark)
            return;

        ConfigureDarkModelib(dark);
        if (dark)
        {
            if (!wasDark)
            {
                dmlib::setDarkWndNotifySafe(hwnd);
                dmlib::setChildCtrlsSubclassAndTheme(hwnd);
                SetPropW(hwnd, DARKMODELIB_TREE_STATE_PROP, reinterpret_cast<HANDLE>(2));
            }
        }
        else
        {
            // Remove dark subclasses only on the dark-to-light transition. Repeatedly removing
            // subclasses while controls are painting loses checkbox/radio invalidations and flickers.
            if (wasDark)
            {
                RemoveWindowAndChildSubclasses(hwnd);
                dmlib::setChildCtrlsTheme(hwnd);
            }
            SetPropW(hwnd, DARKMODELIB_TREE_STATE_PROP, reinterpret_cast<HANDLE>(1));
        }
        ApplyMenuBar(hwnd, dark);
        dmlib::setDarkTitleBar(hwnd);
        if (wasApplied && wasDark != dark)
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
        RestoreCustomTabControls(hwnd);
    }
#else
    (void)hwnd;
#endif
}


void ApplyMenuBar(HWND hwnd, bool enableDark)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
    {
        const bool wasDark = GetPropW(hwnd, DARKMODELIB_MENU_DARK_PROP) != NULL;
        if (wasDark == enableDark)
            return;
        if (enableDark)
        {
            dmlib::setWindowMenuBarSubclass(hwnd);
            SetPropW(hwnd, DARKMODELIB_MENU_DARK_PROP, reinterpret_cast<HANDLE>(1));
        }
        else
        {
            dmlib::removeWindowMenuBarSubclass(hwnd);
            RemovePropW(hwnd, DARKMODELIB_MENU_DARK_PROP);
        }
        DrawMenuBar(hwnd);
    }
#else
    (void)hwnd;
    (void)enableDark;
#endif
}

void ApplyCheckboxOrRadioButton(HWND hwnd, bool enableDark)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
    {
        ConfigureDarkModelib(enableDark);
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

void ApplyStatusBar(HWND hwnd, bool enableDark)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
    {
        ConfigureDarkModelib(enableDark);
        if (enableDark)
            dmlib::setStatusBarCtrlSubclass(hwnd);
        else
            dmlib::removeStatusBarCtrlSubclass(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
    }
#else
    (void)hwnd;
    (void)enableDark;
#endif
}

void ApplyProgressBar(HWND hwnd, bool enableDark)
{
#if USE_DARKMODELIB
    if (hwnd != NULL)
    {
        ConfigureDarkModelib(enableDark);
        if (enableDark)
            dmlib::setProgressBarCtrlSubclass(hwnd);
        else
            dmlib::removeProgressBarCtrlSubclass(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
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
                        textColor = DarkMode_ShouldUseDark() ? RGB(0xF0, 0xF0, 0xF0)
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

void UpdateTabControlOverflowButtons(HWND tabControl, bool enableDark)
{
#if USE_DARKMODELIB
    if (tabControl != NULL)
    {
        if (enableDark)
            dmlib::setTabCtrlUpDownSubclass(tabControl);
        else
            dmlib::removeTabCtrlUpDownSubclass(tabControl);

        HWND upDown = NULL;
        while ((upDown = FindWindowEx(tabControl, upDown, UPDOWN_CLASS, NULL)) != NULL)
        {
            if (enableDark)
                dmlib::setUpDownCtrlSubclass(upDown);
            else
                dmlib::removeUpDownCtrlSubclass(upDown);

            InvalidateRect(upDown, NULL, TRUE);
        }
    }
#else
    (void)tabControl;
    (void)enableDark;
#endif
}

void UpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors)
{
#if USE_DARKMODELIB
    if (listView != NULL)
    {
        ListView_SetTextColor(listView, textColor);
        const COLORREF bg = DarkMode_ShouldUseDark() ? dmlib::getCtrlBackgroundColor() : backgroundColor;
        ListView_SetTextBkColor(listView, bg);
        ListView_SetBkColor(listView, bg);

        if (applyHeaderColors && DarkMode_ShouldUseDark())
        {
            const COLORREF headerBackground = RGB(0x20, 0x20, 0x20);
            const COLORREF headerEdge = RGB(0x38, 0x38, 0x38);
            dmlib::setViewBackgroundColor(DarkMode_ShouldUseDark() ? dmlib::getCtrlBackgroundColor() : backgroundColor);
            dmlib::setViewTextColor(textColor);
            dmlib::setHeaderBackgroundColor(headerBackground);
            dmlib::setHeaderHotBackgroundColor(RGB(0x2A, 0x2A, 0x2A));
            dmlib::setHeaderTextColor(textColor);
            dmlib::setHeaderEdgeColor(headerEdge);
            dmlib::updateViewBrushesAndPens();
            dmlib::replaceClientEdgeWithBorderSafe(listView);
            dmlib::setDarkListView(listView);
            dmlib::setDarkListViewCheckboxes(listView);
            dmlib::setListViewCtrlSubclass(listView);

            HWND header = ListView_GetHeader(listView);
            if (header != NULL)
            {
                dmlib::setHeaderCtrlSubclass(header);
                InvalidateRect(header, NULL, TRUE);
            }
        }
        else
        {
            dmlib::removeListViewCtrlSubclass(listView);
            dmlib::replaceClientEdgeWithBorderSafeEx(listView, false);
            HWND header = ListView_GetHeader(listView);
            if (header != NULL)
            {
                dmlib::removeHeaderCtrlSubclass(header);
                InvalidateRect(header, NULL, TRUE);
            }
        }

        InvalidateRect(listView, NULL, TRUE);
    }
#else
    (void)listView;
    (void)textColor;
    (void)backgroundColor;
    (void)applyHeaderColors;
#endif
}

void MarkCustomTabControl(HWND tabControl)
{
#if USE_DARKMODELIB
    if (tabControl != NULL)
    {
        SetPropW(tabControl, DARKMODELIB_CUSTOM_TAB_PROP, reinterpret_cast<HANDLE>(1));
        RestoreCustomTabControl(tabControl);
    }
#else
    (void)tabControl;
#endif
}

void RestoreCustomTabControls(HWND hwndParent)
{
#if USE_DARKMODELIB
    if (hwndParent != NULL)
    {
        RestoreCustomTabControl(hwndParent);
        EnumChildWindows(hwndParent, RestoreCustomTabControlProc, 0);
    }
#else
    (void)hwndParent;
#endif
}
} // namespace DarkModeBackendDarkModelib
