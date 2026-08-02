// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include <tchar.h>
#include "darkmode_backend_darkmodelib.h"
#include "darkmode.h"
#include "salamand.rh"

#if USE_DARKMODELIB
#include "third_party/darkmodelib/include/Darkmodelib.h"
#endif

#include <algorithm>
#include <delayimp.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <vector>
#include <cwchar>
#include <string>

#ifndef HDM_SETBKCOLOR
#define HDM_SETBKCOLOR (HDM_FIRST + 29)
#endif

#ifndef HDM_SETTEXTCOLOR
#define HDM_SETTEXTCOLOR (HDM_FIRST + 30)
#endif

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

#ifndef DARKMODE_TRACE_CTLFLOW
#define DARKMODE_TRACE_CTLFLOW 0
#endif

#define DARKMODE_SUGGESTIONS_GRIP_HANDLE_TIMER 1 /* timer set to 1 ms, it was enough to keep the autocomplete suggestions list grip handle dark in darkmode */

namespace
{
#if DARKMODE_TRACE_CTLFLOW
bool IsDarkModeTraceControlId(int ctrlId)
{
    switch (ctrlId)
    {
#ifdef IDR_RECYCLE1
    case IDR_RECYCLE1:
#endif
#ifdef IDR_RECYCLE2
    case IDR_RECYCLE2:
#endif
#ifdef IDR_RECYCLE3
    case IDR_RECYCLE3:
#endif
#ifdef IDC_SAVEONCLOSE
    case IDC_SAVEONCLOSE:
#endif
#ifdef IDC_SETBYMAINWINDOW
    case IDC_SETBYMAINWINDOW:
#endif
#ifdef IDC_FILEMASK_HINT
    case IDC_FILEMASK_HINT:
#endif
        return true;
    default:
        return false;
    }
}

void DarkModeTraceCtlColor(UINT message, HWND ctrl, HDC hdc, COLORREF defaultTextColor, COLORREF defaultBackground,
                           HBRUSH brush, bool fallbackApplied)
{
    if (ctrl == NULL)
        return;
    const int ctrlId = GetDlgCtrlID(ctrl);
    if (!IsDarkModeTraceControlId(ctrlId))
        return;
    wchar_t className[32] = L"<null>";
    GetClassNameW(ctrl, className, _countof(className));
    const wchar_t* msgName = message == WM_CTLCOLORBTN ? L"WM_CTLCOLORBTN" : L"WM_CTLCOLORSTATIC";
    COLORREF finalText = defaultTextColor;
    COLORREF finalBk = defaultBackground;
    if (hdc != NULL)
    {
        finalText = GetTextColor(hdc);
        finalBk = GetBkColor(hdc);
    }
    TRACE_I("[DARKMODE_TRACE] msg=%S id=%d class=%S text=#%02X%02X%02X bg=#%02X%02X%02X brush=0x%p fallback=%d",
            msgName, ctrlId, className, GetRValue(finalText), GetGValue(finalText), GetBValue(finalText),
            GetRValue(finalBk), GetGValue(finalBk), GetBValue(finalBk), brush, fallbackApplied ? 1 : 0);
}
#endif

// Helpers borrowed from win32-darkmode project (MIT licensed).
template <typename T, typename T1, typename T2>
constexpr T RvaToVa(T1 base, T2 rva)
{
    return reinterpret_cast<T>(reinterpret_cast<ULONG_PTR>(base) + rva);
}

template <typename T>
constexpr T DataDirectoryFromModuleBase(void* moduleBase, size_t entryID)
{
    auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
    auto ntHdr = RvaToVa<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
    auto dataDir = ntHdr->OptionalHeader.DataDirectory;
    return RvaToVa<T>(moduleBase, dataDir[entryID].VirtualAddress);
}

PIMAGE_THUNK_DATA FindAddressByName(void* moduleBase,
                                   PIMAGE_THUNK_DATA impName,
                                   PIMAGE_THUNK_DATA impAddr,
                                   const char* funcName)
{
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal))
            continue;

        auto import = RvaToVa<PIMAGE_IMPORT_BY_NAME>(moduleBase, impName->u1.AddressOfData);
        if (strcmp(import->Name, funcName) != 0)
            continue;
        return impAddr;
    }
    return nullptr;
}

PIMAGE_THUNK_DATA FindAddressByOrdinal(void* moduleBase,
                                      PIMAGE_THUNK_DATA impName,
                                      PIMAGE_THUNK_DATA impAddr,
                                      uint16_t ordinal)
{
    for (; impName->u1.Ordinal; ++impName, ++impAddr)
    {
        if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
            return impAddr;
    }
    return nullptr;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void* moduleBase, const char* dllName, const char* funcName)
{
    auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (_stricmp(RvaToVa<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
            continue;

        auto impName = RvaToVa<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
        auto impAddr = RvaToVa<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
        return FindAddressByName(moduleBase, impName, impAddr, funcName);
    }
    return nullptr;
}

PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void* moduleBase, const char* dllName, uint16_t ordinal)
{
    auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
    for (; imports->DllNameRVA; ++imports)
    {
        if (_stricmp(RvaToVa<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
            continue;

        auto impName = RvaToVa<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
        auto impAddr = RvaToVa<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
        return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
    }
    return nullptr;
}

enum IMMERSIVE_HC_CACHE_MODE
{
    IHCM_USE_CACHED_VALUE,
    IHCM_REFRESH,
};

// 1903 18362
enum PreferredAppMode
{
    Default,
    AllowDark,
    ForceDark,
    ForceLight,
    Max,
};

enum WINDOWCOMPOSITIONATTRIB
{
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27,
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
};

using fnRtlGetNtVersionNumbers = void(WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
using fnDwmSetWindowAttribute = HRESULT(WINAPI*)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
using fnShouldAppsUseDarkMode = bool(WINAPI*)();
using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND hWnd, bool allow);
using fnAllowDarkModeForApp = bool(WINAPI*)(bool allow);
using fnFlushMenuThemes = void(WINAPI*)();
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();
using fnIsDarkModeAllowedForWindow = bool(WINAPI*)(HWND hWnd);
using fnGetIsImmersiveColorUsingHighContrast = bool(WINAPI*)(IMMERSIVE_HC_CACHE_MODE mode);
using fnOpenNcThemeData = HTHEME(WINAPI*)(HWND hWnd, LPCWSTR pszClassList);
using fnSetWindowTheme = HRESULT(WINAPI*)(HWND hWnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
using fnShouldSystemUseDarkMode = bool(WINAPI*)();
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode);
using fnIsDarkModeAllowedForApp = bool(WINAPI*)();

HMODULE gUxTheme = nullptr;
HMODULE gDwmApi = nullptr;
fnSetWindowCompositionAttribute gSetWindowCompositionAttribute = nullptr;
fnDwmSetWindowAttribute gDwmSetWindowAttribute = nullptr;
fnShouldAppsUseDarkMode gShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow gAllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp gAllowDarkModeForApp = nullptr;
fnFlushMenuThemes gFlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState gRefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow gIsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast gGetIsImmersiveColorUsingHighContrast = nullptr;
fnOpenNcThemeData gOpenNcThemeData = nullptr;
fnSetWindowTheme gSetWindowTheme = nullptr;
fnShouldSystemUseDarkMode gShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode gSetPreferredAppMode = nullptr;
fnIsDarkModeAllowedForApp gIsDarkModeAllowedForApp = nullptr;

DWORD gBuildNumber = 0;
bool gInitialized = false;
bool gSupported = false;
bool gEnabled = false;
bool gWindowsDarkSchemeSelected = false;
bool gScrollbarsHooked = false;
thread_local int gThemeChangeDepth = 0;
thread_local int gThemeBatchDepth = 0;
thread_local int gListViewColorUpdateDepth = 0;

static COLORREF gDialogTextColor = GetSysColor(COLOR_BTNTEXT);
static COLORREF gDialogBackgroundColor = GetSysColor(COLOR_BTNFACE);
static HBRUSH gDialogBrushHandle = NULL;
static HBRUSH gScrollbarTrackBrush = NULL;
static bool gDialogBrushOwned = false;
static DarkModeColors gColors = {GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_BTNTEXT), false};
static COLORREF gAutocompleteSelectedFg = RGB(255, 255, 255);
static COLORREF gAutocompleteSelectedBk = RGB(0, 120, 215);
static bool gPropagatingThemeChange = false;

const wchar_t* kDarkModeThemeProp = L"Salamander.DarkMode.Theme";
const wchar_t* kDarkModeClassicButtonProp = L"Salamander.DarkMode.ClassicButton";
const wchar_t* kDarkModeChoiceButtonProp = L"Salamander.DarkMode.ChoiceButton";

bool ShouldUseDarkColorsForSurfaces();

bool ControlHasCaptionButton(HWND hwnd)
{
    if (hwnd == NULL)
        return false;

    wchar_t className[16];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0)
        return false;

    if (lstrcmpiW(className, L"Button") != 0)
        return false;

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;

    switch (type)
    {
    case BS_GROUPBOX:
    case BS_AUTOCHECKBOX:
    case BS_CHECKBOX:
    case BS_AUTO3STATE:
    case BS_3STATE:
    case BS_AUTORADIOBUTTON:
    case BS_RADIOBUTTON:
        return true;
    }

    return false;
}

bool IsButtonTypeNeedingClassicFallback(HWND hwnd)
{
    if (hwnd == NULL)
        return false;
    wchar_t className[16];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0 || lstrcmpiW(className, L"Button") != 0)
        return false;
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;
    return type == BS_GROUPBOX;
}

bool IsRadioButtonControl(HWND hwnd)
{
    if (hwnd == NULL)
        return false;
    wchar_t className[16];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0 || lstrcmpiW(className, L"Button") != 0)
        return false;
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;
    return type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON;
}

bool IsCheckboxControl(HWND hwnd)
{
    if (hwnd == NULL)
        return false;
    wchar_t className[16];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0 || lstrcmpiW(className, L"Button") != 0)
        return false;
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;
    return type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
           type == BS_AUTO3STATE || type == BS_3STATE;
}

bool IsCheckboxOrRadioButtonControl(HWND hwnd)
{
    return IsRadioButtonControl(hwnd) || IsCheckboxControl(hwnd);
}

bool ShouldOwnerDrawChoiceButton(HWND hwnd)
{
    // With darkmodelib enabled, use the library's modern themed checkbox/radio
    // subclass instead of Salamander's legacy owner-draw fallback on all
    // supported Windows versions.
#if USE_DARKMODELIB
    (void)hwnd;
    return false;
#else
    // Without darkmodelib, radio captions need the fallback on all supported
    // builds. Checkbox text is already painted correctly by the themed control
    // on Windows 11, but Windows 10 keeps drawing it with the light-theme text
    // color in dark dialogs, so use the same owner-draw fallback there only.
    return IsRadioButtonControl(hwnd) || (IsCheckboxControl(hwnd) && gBuildNumber < 22000);
#endif
}

void EnsureClassicButtonTheme(HWND hwnd, bool forceClassic)
{
    if (hwnd == NULL || gSetWindowTheme == nullptr)
        return;

    HANDLE applied = GetPropW(hwnd, kDarkModeClassicButtonProp);

    if (forceClassic)
    {
        if (applied == nullptr)
        {
            gSetWindowTheme(hwnd, L"", L"");
            SetPropW(hwnd, kDarkModeClassicButtonProp, reinterpret_cast<HANDLE>(1));
            InvalidateRect(hwnd, nullptr, TRUE);
        }
    }
    else if (applied != nullptr)
    {
        RemovePropW(hwnd, kDarkModeClassicButtonProp);
        gSetWindowTheme(hwnd, nullptr, nullptr);
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}


constexpr UINT_PTR kDarkModeChoiceButtonSubclassId = 0x44524B52; // "DRKR"
constexpr UINT_PTR kDarkModeTabOverflowSubclassId = 0x4452544F; // "DRTO"
constexpr UINT_PTR kDarkModeRebarSeparatorSubclassId = 0x44525253; // "DRRS"
constexpr UINT_PTR kDarkModeRebarChildSeparatorSubclassId = 0x44524353; // "DRCS"
constexpr UINT_PTR kDarkModeRebarOverlaySeparatorSubclassId = 0x44524F53; // "DROS"
constexpr UINT_PTR kDarkModeAutoSuggestSubclassId = 0x44524153; // "DRAS"
constexpr UINT_PTR kDarkModeAutoSuggestTimerId = 0x44524154; // "DRAT"
const wchar_t* kDarkModeCustomTreeViewProp = L"Salamander.DarkModeLib.CustomTree";
const wchar_t* kDarkModeRebarOverlaySeparatorProp = L"Salamander.DarkMode.RebarOverlaySeparator";

void FillRectWithColor(HDC hdc, const RECT& rect, COLORREF color)
{
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
    COLORREF oldColor = SetDCBrushColor(hdc, color);
    FillRect(hdc, &rect, reinterpret_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
    SetDCBrushColor(hdc, oldColor);
    SelectObject(hdc, oldBrush);
}

void PaintDarkChoiceButton(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    const DarkModeColors& colors = DarkModeGetColors();
    FillRectWithColor(hdc, rc, colors.background);

    const BOOL enabled = IsWindowEnabled(hwnd);
    const LRESULT checkState = SendMessage(hwnd, BM_GETCHECK, 0, 0);
    const LRESULT buttonState = SendMessage(hwnd, BM_GETSTATE, 0, 0);
    const int menuCheckWidth = GetSystemMetrics(SM_CXMENUCHECK);
    const int glyphSize = menuCheckWidth > 13 ? menuCheckWidth : 13;
    const int glyphTopOffset = static_cast<int>((rc.bottom - rc.top - glyphSize) / 2);
    RECT glyph = rc;
    glyph.left += 1;
    glyph.right = glyph.left + glyphSize;
    glyph.top = rc.top + (glyphTopOffset > 0 ? glyphTopOffset : 0);
    glyph.bottom = glyph.top + glyphSize;

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;
    const bool isRadio = type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON;
    UINT state = isRadio ? DFCS_BUTTONRADIO
                         : ((type == BS_AUTO3STATE || type == BS_3STATE) ? DFCS_BUTTON3STATE : DFCS_BUTTONCHECK);
    if (checkState == BST_CHECKED || checkState == BST_INDETERMINATE)
        state |= DFCS_CHECKED;
    if (!enabled)
        state |= DFCS_INACTIVE;
    if ((buttonState & BST_PUSHED) != 0)
        state |= DFCS_PUSHED;
    DrawFrameControl(hdc, &glyph, DFC_BUTTON, state);

    wchar_t text[512] = {0};
    GetWindowTextW(hwnd, text, _countof(text));

    RECT textRect = rc;
    textRect.left = glyph.right + 4;
    COLORREF textColor = enabled ? colors.readableText : DarkModeGetDisabledTextColor();
    HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font != NULL ? SelectObject(hdc, font) : NULL;
    COLORREF oldText = SetTextColor(hdc, textColor);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldText);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
}

LRESULT CALLBACK DarkChoiceButtonSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkChoiceButtonSubclass, kDarkModeChoiceButtonSubclassId);
        break;

    case WM_ERASEBKGND:
        return TRUE;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (hdc != NULL)
            PaintDarkChoiceButton(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_PRINTCLIENT:
        PaintDarkChoiceButton(hwnd, reinterpret_cast<HDC>(wParam));
        return 0;

    case WM_ENABLE:
    case WM_SETTEXT:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case BM_SETCHECK:
    case BM_SETSTATE:
    {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, NULL, TRUE);
        return result;
    }
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnsureDarkChoiceButtonSubclass(HWND hwnd, bool enableDark)
{
    if (hwnd == NULL)
        return;

    const bool isDark = GetPropW(hwnd, kDarkModeChoiceButtonProp) != NULL;
    if (isDark == enableDark)
        return;

    if (enableDark)
    {
        EnsureClassicButtonTheme(hwnd, true);
        SetWindowSubclass(hwnd, DarkChoiceButtonSubclass, kDarkModeChoiceButtonSubclassId, 0);
        SetPropW(hwnd, kDarkModeChoiceButtonProp, reinterpret_cast<HANDLE>(1));
    }
    else
    {
        RemoveWindowSubclass(hwnd, DarkChoiceButtonSubclass, kDarkModeChoiceButtonSubclassId);
        RemovePropW(hwnd, kDarkModeChoiceButtonProp);
        EnsureClassicButtonTheme(hwnd, false);
    }
    InvalidateRect(hwnd, NULL, TRUE);
}

void PaintDarkTabOverflowButton(HWND hwnd, HDC hdc)
{
    RECT client;
    GetClientRect(hwnd, &client);

    const DarkModeColors& colors = DarkModeGetColors();
    const COLORREF face = colors.usingSchemeColors ? colors.background : RGB(0x20, 0x20, 0x20);
    const COLORREF hotFace = RGB((std::min)(255, GetRValue(face) + 0x12),
                                 (std::min)(255, GetGValue(face) + 0x12),
                                 (std::min)(255, GetBValue(face) + 0x12));
    const COLORREF pressedFace = RGB((std::max)(0, GetRValue(face) - 0x10),
                                     (std::max)(0, GetGValue(face) - 0x10),
                                     (std::max)(0, GetBValue(face) - 0x10));
    const COLORREF border = RGB(0x4A, 0x4A, 0x4A);
    const COLORREF arrow = IsWindowEnabled(hwnd) ? colors.readableText : RGB(0x88, 0x88, 0x88);

    POINT cursor;
    GetCursorPos(&cursor);
    ScreenToClient(hwnd, &cursor);
    const bool trackingMouse = PtInRect(&client, cursor) != FALSE;
    const bool mouseDown = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;

    const bool horizontal = (GetWindowLongPtr(hwnd, GWL_STYLE) & UDS_HORZ) != 0;
    RECT buttons[2] = {client, client};
    if (horizontal)
    {
        buttons[0].right = client.left + (client.right - client.left) / 2;
        buttons[1].left = buttons[0].right;
    }
    else
    {
        buttons[0].bottom = client.top + (client.bottom - client.top) / 2;
        buttons[1].top = buttons[0].bottom;
    }

    FillRectWithColor(hdc, client, face);

    for (int i = 0; i < 2; ++i)
    {
        RECT button = buttons[i];
        COLORREF buttonFace = face;
        if (trackingMouse && PtInRect(&button, cursor))
            buttonFace = mouseDown ? pressedFace : hotFace;
        FillRectWithColor(hdc, button, buttonFace);

        RECT edge = button;
        if (horizontal)
        {
            edge.left = (i == 0) ? button.right - 1 : button.left;
            edge.right = edge.left + 1;
        }
        else
        {
            edge.top = (i == 0) ? button.bottom - 1 : button.top;
            edge.bottom = edge.top + 1;
        }
        FillRectWithColor(hdc, edge, border);

        const int width = button.right - button.left;
        const int height = button.bottom - button.top;
        int arrowSize = (std::min)(width, height) / 3;
        if (arrowSize < 3)
            arrowSize = 3;
        if (arrowSize > 6)
            arrowSize = 6;

        POINT center = {button.left + width / 2, button.top + height / 2};
        POINT pts[3];
        if (horizontal)
        {
            if (i == 0)
            {
                pts[0].x = center.x - arrowSize / 2;
                pts[0].y = center.y;
                pts[1].x = center.x + arrowSize / 2;
                pts[1].y = center.y - arrowSize;
                pts[2].x = center.x + arrowSize / 2;
                pts[2].y = center.y + arrowSize;
            }
            else
            {
                pts[0].x = center.x + arrowSize / 2;
                pts[0].y = center.y;
                pts[1].x = center.x - arrowSize / 2;
                pts[1].y = center.y - arrowSize;
                pts[2].x = center.x - arrowSize / 2;
                pts[2].y = center.y + arrowSize;
            }
        }
        else
        {
            if (i == 0)
            {
                pts[0].x = center.x;
                pts[0].y = center.y - arrowSize / 2;
                pts[1].x = center.x - arrowSize;
                pts[1].y = center.y + arrowSize / 2;
                pts[2].x = center.x + arrowSize;
                pts[2].y = center.y + arrowSize / 2;
            }
            else
            {
                pts[0].x = center.x;
                pts[0].y = center.y + arrowSize / 2;
                pts[1].x = center.x - arrowSize;
                pts[1].y = center.y - arrowSize / 2;
                pts[2].x = center.x + arrowSize;
                pts[2].y = center.y - arrowSize / 2;
            }
        }

        HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
        HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));
        COLORREF oldBrushColor = SetDCBrushColor(hdc, arrow);
        COLORREF oldPenColor = SetDCPenColor(hdc, arrow);
        Polygon(hdc, pts, _countof(pts));
        SetDCPenColor(hdc, oldPenColor);
        SetDCBrushColor(hdc, oldBrushColor);
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
    }
}

void PaintDarkRebarSeparators(HWND hwnd, HDC hdc)
{
    if (hwnd == NULL || hdc == NULL || !DarkModeShouldUseDarkColors())
        return;

    RECT client;
    GetClientRect(hwnd, &client);
    if (client.right <= client.left || client.bottom <= client.top)
        return;

    std::vector<int> lines;
    lines.push_back(client.top);

    const int bandCount = static_cast<int>(SendMessage(hwnd, RB_GETBANDCOUNT, 0, 0));
    for (int i = 0; i < bandCount; ++i)
    {
        RECT bandRect = {0, 0, 0, 0};
        if (SendMessage(hwnd, RB_GETRECT, i, reinterpret_cast<LPARAM>(&bandRect)) != 0 &&
            bandRect.bottom > client.top && bandRect.bottom < client.bottom)
        {
            lines.push_back(bandRect.top);
            lines.push_back(bandRect.bottom - 1);
            lines.push_back(bandRect.bottom);
        }
    }

    lines.push_back(client.bottom - 1);
    std::sort(lines.begin(), lines.end());
    lines.erase(std::unique(lines.begin(), lines.end()), lines.end());

    // Row separators are drawn by small overlay child windows below.  Keep the
    // legacy parent/child repaint path from adding extra visible pixels; when
    // both paths draw, the separators become too thick and flicker at the
    // rebar's left edge during light->dark switches.
    lines.clear();
    const COLORREF separatorColor = DarkModeGetColors().background;
    HPEN pen = CreatePen(PS_SOLID, 1, separatorColor);
    if (pen == NULL)
        return;

    HGDIOBJ oldPen = SelectObject(hdc, pen);
    for (int y : lines)
    {
        MoveToEx(hdc, client.left, y, NULL);
        LineTo(hdc, client.right, y);
    }
    if (oldPen != NULL)
        SelectObject(hdc, oldPen);

    // Without native RBS_BANDBORDERS the toolbar child windows can cover the
    // row separator pixels. Paint the same separator over the affected child
    // windows so the dark-mode row lines remain visible without re-enabling the
    // native light-colored band borders.
    const int childCount = static_cast<int>(SendMessage(hwnd, RB_GETBANDCOUNT, 0, 0));
    for (int i = 0; i < childCount; ++i)
    {
        REBARBANDINFO rbi = {0};
        rbi.cbSize = sizeof(rbi);
        rbi.fMask = RBBIM_CHILD | RBBIM_STYLE | RBBIM_HEADERSIZE;
        RECT bandRect = {0, 0, 0, 0};
        if (SendMessage(hwnd, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&rbi)) == 0 ||
            SendMessage(hwnd, RB_GETRECT, i, reinterpret_cast<LPARAM>(&bandRect)) == 0)
        {
            continue;
        }

        if (rbi.hwndChild != NULL)
        {
            RECT childRect;
            GetWindowRect(rbi.hwndChild, &childRect);
            MapWindowPoints(NULL, hwnd, reinterpret_cast<LPPOINT>(&childRect), 2);

            HDC childDC = GetDC(rbi.hwndChild);
            if (childDC != NULL)
            {
                HGDIOBJ childOldPen = SelectObject(childDC, pen);
                for (int y : lines)
                {
                    if (y >= childRect.top && y <= childRect.bottom)
                    {
                        int childY = y - childRect.top;
                        const int childHeight = childRect.bottom - childRect.top;
                        if (childY >= childHeight)
                            childY = childHeight - 1;
                        MoveToEx(childDC, 0, childY, NULL);
                        LineTo(childDC, childRect.right - childRect.left, childY);
                    }
                }
                if (childOldPen != NULL)
                    SelectObject(childDC, childOldPen);
                ReleaseDC(rbi.hwndChild, childDC);
            }
        }

        if ((rbi.fStyle & RBBS_NOGRIPPER) == 0)
        {
            RECT gripRect = bandRect;
            gripRect.right = gripRect.left + 3;
            if (gripRect.right > bandRect.right)
                gripRect.right = bandRect.right;
            if (gripRect.right <= gripRect.left)
                continue;

            // Keep the original simple one-line gripper shape, but replace the
            // light native RGB(192,192,192) paint with the same dark separator
            // color used inside the toolbars.
            FillRectWithColor(hdc, gripRect, DarkModeGetColors().background);
            HPEN gripPen = CreatePen(PS_SOLID, 1, RGB(95, 95, 95));
            if (gripPen != NULL)
            {
                HGDIOBJ gripOldPen = SelectObject(hdc, gripPen);
                const int x = gripRect.left + 1;
                MoveToEx(hdc, x, gripRect.top + 3, NULL);
                LineTo(hdc, x, gripRect.bottom - 3);
                if (gripOldPen != NULL)
                    SelectObject(hdc, gripOldPen);
                DeleteObject(gripPen);
            }
        }
    }

    DeleteObject(pen);
}


void PaintDarkRebarChildSeparators(HWND hwnd, HDC hdc)
{
    (void)hwnd;
    (void)hdc;
}

void PaintDarkRebarOverlaySeparator(HWND hwnd, HDC hdc)
{
    if (hwnd == NULL || hdc == NULL)
        return;

    RECT client;
    GetClientRect(hwnd, &client);
    if (client.right <= client.left || client.bottom <= client.top)
        return;

    RECT topLine = client;
    topLine.bottom = topLine.top + 1;
    FillRectWithColor(hdc, topLine, RGB(56, 56, 56));

    RECT bottomLine = client;
    bottomLine.top = topLine.bottom;
    FillRectWithColor(hdc, bottomLine, RGB(14, 14, 14));
}

LRESULT CALLBACK DarkRebarOverlaySeparatorSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                   UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkRebarOverlaySeparatorSubclass, kDarkModeRebarOverlaySeparatorSubclassId);
        RemovePropW(hwnd, kDarkModeRebarOverlaySeparatorProp);
        break;

    case WM_ERASEBKGND:
        PaintDarkRebarOverlaySeparator(hwnd, reinterpret_cast<HDC>(wParam));
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintDarkRebarOverlaySeparator(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_PRINTCLIENT:
        PaintDarkRebarOverlaySeparator(hwnd, reinterpret_cast<HDC>(wParam));
        return 0;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

BOOL CALLBACK DestroyDarkRebarOverlaySeparatorProc(HWND hwnd, LPARAM)
{
    if (GetPropW(hwnd, kDarkModeRebarOverlaySeparatorProp) != NULL)
        DestroyWindow(hwnd);
    return TRUE;
}

std::vector<int> GetDarkRebarOverlaySeparatorLines(HWND rebar)
{
    std::vector<int> lines;

    RECT client;
    GetClientRect(rebar, &client);
    if (client.right <= client.left || client.bottom <= client.top)
        return lines;

    const int bandCount = static_cast<int>(SendMessage(rebar, RB_GETBANDCOUNT, 0, 0));
    for (int i = 0; i < bandCount; ++i)
    {
        RECT bandRect = {0, 0, 0, 0};
        if (SendMessage(rebar, RB_GETRECT, i, reinterpret_cast<LPARAM>(&bandRect)) == 0)
            continue;

        if (bandRect.bottom - 2 > client.top && bandRect.bottom < client.bottom)
            lines.push_back(bandRect.bottom - 2);
    }

    std::sort(lines.begin(), lines.end());
    lines.erase(std::unique(lines.begin(), lines.end()), lines.end());
    return lines;
}

void UpdateDarkRebarOverlaySeparators(HWND rebar, bool enable)
{
    if (rebar == NULL)
        return;

    EnumChildWindows(rebar, DestroyDarkRebarOverlaySeparatorProc, 0);
    if (!enable)
        return;

    RECT client;
    GetClientRect(rebar, &client);
    const int width = client.right - client.left;
    if (width <= 0)
        return;

    const std::vector<int> lines = GetDarkRebarOverlaySeparatorLines(rebar);
    for (int y : lines)
    {
        HWND line = CreateWindowExW(WS_EX_NOACTIVATE, L"STATIC", L"",
                                    WS_CHILD | WS_VISIBLE,
                                    client.left, y, width, 2,
                                    rebar, NULL, GetModuleHandle(NULL), NULL);
        if (line == NULL)
            continue;

        SetPropW(line, kDarkModeRebarOverlaySeparatorProp, reinterpret_cast<HANDLE>(1));
        SetWindowSubclass(line, DarkRebarOverlaySeparatorSubclass, kDarkModeRebarOverlaySeparatorSubclassId, 0);
        SetWindowPos(line, HWND_TOP, client.left, y, width, 2,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

LRESULT CALLBACK DarkRebarChildSeparatorSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                 UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkRebarChildSeparatorSubclass, kDarkModeRebarChildSeparatorSubclassId);
        break;

    case WM_PAINT:
    {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        HDC hdc = GetDC(hwnd);
        if (hdc != NULL)
        {
            PaintDarkRebarChildSeparators(hwnd, hdc);
            ReleaseDC(hwnd, hdc);
        }
        return result;
    }

    case WM_PRINTCLIENT:
    {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        PaintDarkRebarChildSeparators(hwnd, reinterpret_cast<HDC>(wParam));
        return result;
    }
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void UpdateDarkRebarChildSeparators(HWND rebar, bool enable)
{
    if (rebar == NULL)
        return;

    const int bandCount = static_cast<int>(SendMessage(rebar, RB_GETBANDCOUNT, 0, 0));
    for (int i = 0; i < bandCount; ++i)
    {
        REBARBANDINFO rbi = {0};
        rbi.cbSize = sizeof(rbi);
        rbi.fMask = RBBIM_CHILD;
        if (SendMessage(rebar, RB_GETBANDINFO, i, reinterpret_cast<LPARAM>(&rbi)) == 0 || rbi.hwndChild == NULL)
            continue;

        if (enable)
            SetWindowSubclass(rbi.hwndChild, DarkRebarChildSeparatorSubclass, kDarkModeRebarChildSeparatorSubclassId, 0);
        else
            RemoveWindowSubclass(rbi.hwndChild, DarkRebarChildSeparatorSubclass, kDarkModeRebarChildSeparatorSubclassId);
        InvalidateRect(rbi.hwndChild, NULL, TRUE);
    }
}

LRESULT CALLBACK DarkRebarSeparatorSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        UpdateDarkRebarOverlaySeparators(hwnd, false);
        RemoveWindowSubclass(hwnd, DarkRebarSeparatorSubclass, kDarkModeRebarSeparatorSubclassId);
        break;

    case WM_SIZE:
    case WM_WINDOWPOSCHANGED:
        UpdateDarkRebarOverlaySeparators(hwnd, DarkModeShouldUseDarkColors());
        break;

    case WM_PAINT:
    {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        HDC hdc = GetDC(hwnd);
        if (hdc != NULL)
        {
            PaintDarkRebarSeparators(hwnd, hdc);
            ReleaseDC(hwnd, hdc);
        }
        return result;
    }

    case WM_PRINTCLIENT:
    {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        PaintDarkRebarSeparators(hwnd, reinterpret_cast<HDC>(wParam));
        return result;
    }
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}


bool IsAutoSuggestDropdownClass(HWND hwnd)
{
    if (hwnd == NULL)
        return false;

    wchar_t className[128];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0)
        return false;

    return lstrcmpiW(className, L"Auto-Suggest Dropdown") == 0 ||
           wcsstr(className, L"Auto-Suggest") != NULL ||
           wcsstr(className, L"AutoSuggest") != NULL;
}

LRESULT CALLBACK DarkAutoSuggestSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR subclassId, DWORD_PTR refData);

HWND FindAutoSuggestDropdown(HWND hwnd)
{
    for (HWND candidate = hwnd; candidate != NULL; candidate = GetParent(candidate))
    {
        if (IsAutoSuggestDropdownClass(candidate))
            return candidate;
    }

    HWND rootOwner = GetAncestor(hwnd, GA_ROOTOWNER);
    if (IsAutoSuggestDropdownClass(rootOwner))
        return rootOwner;

    HWND root = GetAncestor(hwnd, GA_ROOT);
    if (IsAutoSuggestDropdownClass(root))
        return root;

    return NULL;
}

void ApplyAutoSuggestChildDarkMode(HWND hwnd)
{
    if (hwnd == NULL)
        return;

    DarkModeApplyWindow(hwnd);
    if (gSetWindowTheme != nullptr)
        gSetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);

    // The Shell auto-suggest list is wrapped in helper windows (for example
    // CtrlNotifySink) and its ListBox asks its immediate parent for
    // WM_CTLCOLORLISTBOX. Subclass every child window, not only the top-level
    // Auto-Suggest Dropdown, so the parent that actually receives CTLCOLOR can
    // return our dark brush.
    SetWindowSubclass(hwnd, DarkAutoSuggestSubclass, kDarkModeAutoSuggestSubclassId, 0);

    wchar_t className[64];
    if (GetClassNameW(hwnd, className, _countof(className)) != 0)
    {
        const DarkModeColors& colors = DarkModeGetColors();
        if (lstrcmpiW(className, WC_LISTVIEWW) == 0)
        {
            ListView_SetTextColor(hwnd, colors.readableText);
#if USE_DARKMODELIB
            ListView_SetTextBkColor(hwnd, dmlib::getCtrlBackgroundColor());
            ListView_SetBkColor(hwnd, dmlib::getCtrlBackgroundColor());
#else
            ListView_SetTextBkColor(hwnd, colors.background);
            ListView_SetBkColor(hwnd, colors.background);
#endif
#if USE_DARKMODELIB
            dmlib::setListViewCtrlSubclass(hwnd);
#endif
        }
        else if (lstrcmpiW(className, L"ListBox") == 0 || lstrcmpiW(className, L"Edit") == 0)
        {
#if USE_DARKMODELIB
            dmlib::setCustomBorderForListBoxOrEditCtrlSubclass(hwnd);
#endif
            if (lstrcmpiW(className, L"ListBox") == 0)
                SendMessage(hwnd, LB_SETITEMHEIGHT, 0, SendMessage(hwnd, LB_GETITEMHEIGHT, 0, 0));
        }
    }

    InvalidateRect(hwnd, NULL, TRUE);
}

BOOL CALLBACK ApplyAutoSuggestChildDarkModeProc(HWND hwnd, LPARAM)
{
    ApplyAutoSuggestChildDarkMode(hwnd);
    return TRUE;
}


bool IsListBoxWindow(HWND hwnd)
{
    if (hwnd == NULL)
        return false;

    wchar_t className[32];
    return GetClassNameW(hwnd, className, _countof(className)) != 0 &&
           lstrcmpiW(className, L"ListBox") == 0;
}

bool IsListViewWindow(HWND hwnd)
{
    if (hwnd == NULL)
        return false;

    wchar_t className[32];
    return GetClassNameW(hwnd, className, _countof(className)) != 0 &&
           lstrcmpiW(className, WC_LISTVIEWW) == 0;
}

bool IsAutoSuggestItemsWindow(HWND hwnd)
{
    return IsListBoxWindow(hwnd) || IsListViewWindow(hwnd);
}

void PaintAutoSuggestListBox(HWND hwnd, HDC hdc)
{
    if (hwnd == NULL || hdc == NULL || !DarkModeShouldUseDarkColors())
        return;

    RECT client;
    GetClientRect(hwnd, &client);
    const DarkModeColors& colors = DarkModeGetColors();
    FillRectWithColor(hdc, client, colors.background);

    const int count = static_cast<int>(SendMessage(hwnd, LB_GETCOUNT, 0, 0));
    if (count <= 0 || count == LB_ERR)
        return;

    const int topIndex = static_cast<int>(SendMessage(hwnd, LB_GETTOPINDEX, 0, 0));
    const int curSel = static_cast<int>(SendMessage(hwnd, LB_GETCURSEL, 0, 0));
    HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font != NULL ? SelectObject(hdc, font) : NULL;
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    for (int index = topIndex; index < count; ++index)
    {
        RECT itemRect;
        if (SendMessage(hwnd, LB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&itemRect)) == LB_ERR)
            break;
        if (itemRect.top >= client.bottom)
            break;

        const LRESULT selected = SendMessage(hwnd, LB_GETSEL, index, 0);
        const bool isSelected = selected > 0 || index == curSel;
        const COLORREF itemBk = isSelected ? gAutocompleteSelectedBk : colors.background;
        const COLORREF itemText = isSelected ? gAutocompleteSelectedFg : colors.readableText;
        FillRectWithColor(hdc, itemRect, itemBk);
        SetTextColor(hdc, itemText);

        const LRESULT textLength = SendMessageW(hwnd, LB_GETTEXTLEN, index, 0);
        if (textLength > 0)
        {
            std::wstring text;
            text.resize(static_cast<size_t>(textLength) + 1);
            SendMessageW(hwnd, LB_GETTEXT, index, reinterpret_cast<LPARAM>(&text[0]));
            text.resize(wcslen(text.c_str()));
            RECT textRect = itemRect;
            textRect.left += 4;
            textRect.right -= 4;
            DrawTextW(hdc, text.c_str(), static_cast<int>(text.length()), &textRect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        }
    }

    SetBkMode(hdc, oldBkMode);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
}


void PaintAutoSuggestListView(HWND hwnd, HDC hdc)
{
    if (hwnd == NULL || hdc == NULL || !DarkModeShouldUseDarkColors())
        return;

    RECT client;
    GetClientRect(hwnd, &client);
    const DarkModeColors& colors = DarkModeGetColors();
    FillRectWithColor(hdc, client, colors.background);

    const int count = ListView_GetItemCount(hwnd);
    if (count <= 0)
        return;

    int topIndex = ListView_GetTopIndex(hwnd);
    if (topIndex < 0)
        topIndex = 0;
    int perPage = ListView_GetCountPerPage(hwnd);
    if (perPage <= 0)
        perPage = count;
    const int requestedLastIndex = topIndex + perPage + 2;
    const int lastIndex = requestedLastIndex < count ? requestedLastIndex : count;

    HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font != NULL ? SelectObject(hdc, font) : NULL;
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    wchar_t text[4096];
    for (int index = topIndex; index < lastIndex; ++index)
    {
        RECT itemRect;
        if (!ListView_GetItemRect(hwnd, index, &itemRect, LVIR_BOUNDS))
            continue;
        if (itemRect.top >= client.bottom)
            break;

        const bool isSelected = ListView_GetItemState(hwnd, index, LVIS_SELECTED) != 0;
        const COLORREF itemBk = isSelected ? gAutocompleteSelectedBk : colors.background;
        const COLORREF itemText = isSelected ? gAutocompleteSelectedFg : colors.readableText;
        FillRectWithColor(hdc, itemRect, itemBk);
        SetTextColor(hdc, itemText);

        LVITEMW item = {};
        item.mask = LVIF_TEXT;
        item.iItem = index;
        item.iSubItem = 0;
        item.pszText = text;
        item.cchTextMax = _countof(text);
        text[0] = 0;
        if (SendMessageW(hwnd, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item)) != 0)
        {
            RECT textRect = itemRect;
            textRect.left += 4;
            textRect.right -= 4;
            DrawTextW(hdc, text, -1, &textRect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        }
    }

    SetBkMode(hdc, oldBkMode);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
}

void PaintAutoSuggestItemsWindow(HWND hwnd, HDC hdc)
{
    if (IsListBoxWindow(hwnd))
        PaintAutoSuggestListBox(hwnd, hdc);
    else if (IsListViewWindow(hwnd))
        PaintAutoSuggestListView(hwnd, hdc);
}

LRESULT CALLBACK DarkAutoSuggestSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        KillTimer(hwnd, kDarkModeAutoSuggestTimerId);
        RemoveWindowSubclass(hwnd, DarkAutoSuggestSubclass, kDarkModeAutoSuggestSubclassId);
        break;

    case WM_NCPAINT:
    {
        if (DarkModeShouldUseDarkColors())
        {
            DWORD style = static_cast<DWORD>(GetWindowLongPtr(hwnd, GWL_STYLE));

            if (IsAutoSuggestDropdownClass(hwnd) && (style & WS_THICKFRAME))
            {
                LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
                InvalidateRect(hwnd, NULL, FALSE);
                return result;
            }
        }
        break;
    }

    case WM_ERASEBKGND:
        if (DarkModeShouldUseDarkColors())
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRectWithColor(reinterpret_cast<HDC>(wParam), rc, DarkModeGetColors().background);
            return 1;
        }
        break;

    case WM_PAINT:
        if (IsAutoSuggestItemsWindow(hwnd) && DarkModeShouldUseDarkColors())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            PaintAutoSuggestItemsWindow(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        else if (IsAutoSuggestDropdownClass(hwnd) && DarkModeShouldUseDarkColors())
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);

            HDC hdcWnd = GetWindowDC(hwnd);
            if (hdcWnd != NULL)
            {
                RECT rcWnd;
                GetWindowRect(hwnd, &rcWnd);
                const int wndW = rcWnd.right - rcWnd.left;
                const int wndH = rcWnd.bottom - rcWnd.top;

                RECT rcCli;
                GetClientRect(hwnd, &rcCli);

                const int borderX = (wndW - rcCli.right) / 2;
                const int borderY = (wndH - rcCli.bottom) / 2;
                const int gripW = GetSystemMetrics(SM_CXVSCROLL);
                const int gripH = GetSystemMetrics(SM_CYHSCROLL);

                RECT gripRc = {wndW - borderX - gripW, wndH - borderY - gripH,
                               wndW - borderX, wndH - borderY};

                const DarkModeColors& colors = DarkModeGetColors();
                HBRUSH hBrush = CreateSolidBrush(colors.background);
                FillRect(hdcWnd, &gripRc, hBrush);
                DeleteObject(hBrush);

                HPEN hPen = CreatePen(PS_SOLID, 1, colors.readableText);
                HPEN hOldPen = static_cast<HPEN>(SelectObject(hdcWnd, hPen));
                for (int i = 0; i < gripW - 2; i += 4)
                {
                    MoveToEx(hdcWnd, gripRc.right - i - 2, gripRc.bottom - 2, NULL);
                    LineTo(hdcWnd, gripRc.right - 2, gripRc.bottom - i - 2);
                }
                SelectObject(hdcWnd, hOldPen);
                DeleteObject(hPen);

                ReleaseDC(hwnd, hdcWnd);
            }

            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_PRINTCLIENT:
        if (IsAutoSuggestItemsWindow(hwnd) && DarkModeShouldUseDarkColors())
        {
            PaintAutoSuggestItemsWindow(hwnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }
        break;

    case WM_TIMER:
        if (wParam == kDarkModeAutoSuggestTimerId && IsAutoSuggestDropdownClass(hwnd) &&
            DarkModeShouldUseDarkColors())
        {
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        break;

    case WM_KEYDOWN:
    case WM_CHAR:
    case WM_MOUSEMOVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_VSCROLL:
        if (IsAutoSuggestItemsWindow(hwnd) && DarkModeShouldUseDarkColors())
        {
            LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
            InvalidateRect(hwnd, NULL, TRUE);
            return result;
        }
        break;

    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORSCROLLBAR:
    {
        if (DarkModeShouldUseDarkColors())
        {
            if (IsAutoSuggestDropdownClass(hwnd))
                InvalidateRect(hwnd, NULL, FALSE);

            HDC hdc = reinterpret_cast<HDC>(wParam);
            if (hdc != NULL)
            {
                const DarkModeColors& colors = DarkModeGetColors();
                SetTextColor(hdc, colors.readableText);
                SetBkColor(hdc, colors.background);
                SetBkMode(hdc, OPAQUE);
            }
            return reinterpret_cast<LRESULT>(gDialogBrushHandle != NULL ? gDialogBrushHandle : GetSysColorBrush(COLOR_WINDOW));
        }
        break;
    }

    case WM_SHOWWINDOW:
        if (IsAutoSuggestDropdownClass(hwnd))
        {
            if (wParam != 0 && DarkModeShouldUseDarkColors())
            {
                SetWindowSubclass(hwnd, DarkAutoSuggestSubclass, kDarkModeAutoSuggestSubclassId, 0);
                if (gSetWindowTheme != nullptr)
                    gSetWindowTheme(hwnd, L"DarkMode_CFD", nullptr);
                DarkModeRefreshTitleBar(hwnd);
                if (gDwmSetWindowAttribute)
                {
                    BOOL useDark = TRUE;
                    gDwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDark, sizeof(useDark));
                }
                EnumChildWindows(hwnd, ApplyAutoSuggestChildDarkModeProc, 0);
                SetTimer(hwnd, kDarkModeAutoSuggestTimerId, DARKMODE_SUGGESTIONS_GRIP_HANDLE_TIMER, NULL);  /* timer set to 1 ms, it was enough to keep the autocomplete suggestions list grip handle dark in darkmode */
                RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_NOCHILDREN | RDW_UPDATENOW);
            }
        }
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ApplyAutoSuggestDropdownDarkMode(HWND dropdown)
{
    if (dropdown == NULL || !DarkModeShouldUseDarkColors())
        return;

    DarkModeApplyWindow(dropdown);
    if (gSetWindowTheme != nullptr)
        gSetWindowTheme(dropdown, L"DarkMode_CFD", nullptr);
#if USE_DARKMODELIB
    dmlib::setDarkWndNotifySafe(dropdown);
    dmlib::setChildCtrlsSubclassAndTheme(dropdown);
#endif
    DarkModeRefreshTitleBar(dropdown);
    if (gDwmSetWindowAttribute)
    {
        BOOL useDark = TRUE;
        gDwmSetWindowAttribute(dropdown, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDark, sizeof(useDark));
    }

    SetWindowSubclass(dropdown, DarkAutoSuggestSubclass, kDarkModeAutoSuggestSubclassId, 0);
    SetTimer(dropdown, kDarkModeAutoSuggestTimerId, DARKMODE_SUGGESTIONS_GRIP_HANDLE_TIMER, NULL);  /* timer set to 1 ms, it was enough to keep the autocomplete suggestions list grip handle dark in darkmode */
    EnumChildWindows(dropdown, ApplyAutoSuggestChildDarkModeProc, 0);
    RedrawWindow(dropdown, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void CALLBACK AutoSuggestWinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                                      DWORD eventThread, DWORD)
{
    if ((event != EVENT_OBJECT_CREATE && event != EVENT_OBJECT_SHOW) || hwnd == NULL || idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
        return;

    if (eventThread != GetCurrentThreadId())
        return;

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId != GetCurrentProcessId())
        return;

    ApplyAutoSuggestDropdownDarkMode(FindAutoSuggestDropdown(hwnd));
}

HWINEVENTHOOK gAutoSuggestWinEventHook = NULL;
DWORD gAutoSuggestWinEventThreadId = 0;

LRESULT CALLBACK DarkTabOverflowSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkTabOverflowSubclass, kDarkModeTabOverflowSubclassId);
        break;

    case WM_ERASEBKGND:
        return ShouldUseDarkColorsForSurfaces() ? TRUE : DefSubclassProc(hwnd, msg, wParam, lParam);

    case WM_PAINT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc != NULL)
                PaintDarkTabOverflowButton(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_PRINTCLIENT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PaintDarkTabOverflowButton(hwnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }
        break;

    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT track = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&track);
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    case WM_MOUSELEAVE:
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_ENABLE:
    case WM_SIZE:
    case WM_THEMECHANGED:
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnsureDarkTabOverflowSubclass(HWND hwnd, bool enableDark)
{
    if (hwnd == NULL)
        return;

    if (enableDark)
        SetWindowSubclass(hwnd, DarkTabOverflowSubclass, kDarkModeTabOverflowSubclassId, 0);
    else
        RemoveWindowSubclass(hwnd, DarkTabOverflowSubclass, kDarkModeTabOverflowSubclassId);
    InvalidateRect(hwnd, NULL, TRUE);
}

#if !USE_DARKMODELIB
constexpr UINT_PTR kDarkModeHeaderSubclassId = 0x44524844;    // "DRHD"
constexpr UINT_PTR kDarkModeStatusBarSubclassId = 0x44525342; // "DRSB"

void PaintDarkHeaderControl(HWND hwnd, HDC hdc)
{
    RECT client;
    GetClientRect(hwnd, &client);
    const DarkModeColors& colors = DarkModeGetColors();
    FillRectWithColor(hdc, client, RGB(0x20, 0x20, 0x20));

    HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font != NULL ? SelectObject(hdc, font) : NULL;
    COLORREF oldText = SetTextColor(hdc, colors.readableText);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    const int count = Header_GetItemCount(hwnd);
    for (int i = 0; i < count; ++i)
    {
        RECT itemRect;
        if (!Header_GetItemRect(hwnd, i, &itemRect))
            continue;

        TCHAR text[256];
        text[0] = 0;
        HDITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = HDI_TEXT | HDI_FORMAT;
        item.pszText = text;
        item.cchTextMax = _countof(text);
        Header_GetItem(hwnd, i, &item);

        RECT textRect = itemRect;
        textRect.left += 5;
        textRect.right -= 5;
        UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
        if ((item.fmt & HDF_RIGHT) != 0)
            format |= DT_RIGHT;
        else if ((item.fmt & HDF_CENTER) != 0)
            format |= DT_CENTER;
        else
            format |= DT_LEFT;
        DrawText(hdc, text, -1, &textRect, format);

        RECT line = itemRect;
        line.left = line.right - 1;
        FillRectWithColor(hdc, line, RGB(0x4A, 0x4A, 0x4A));
    }

    RECT bottom = client;
    bottom.top = bottom.bottom - 1;
    FillRectWithColor(hdc, bottom, RGB(0x4A, 0x4A, 0x4A));

    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldText);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
}

LRESULT CALLBACK DarkHeaderSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                    UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkHeaderSubclass, kDarkModeHeaderSubclassId);
        break;

    case WM_ERASEBKGND:
        return ShouldUseDarkColorsForSurfaces() ? TRUE : DefSubclassProc(hwnd, msg, wParam, lParam);

    case WM_PAINT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc != NULL)
                PaintDarkHeaderControl(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_PRINTCLIENT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PaintDarkHeaderControl(hwnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }
        break;

    case WM_THEMECHANGED:
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnsureDarkHeaderSubclass(HWND hwnd, bool enableDark)
{
    if (hwnd == NULL)
        return;

    if (enableDark)
        SetWindowSubclass(hwnd, DarkHeaderSubclass, kDarkModeHeaderSubclassId, 0);
    else
        RemoveWindowSubclass(hwnd, DarkHeaderSubclass, kDarkModeHeaderSubclassId);
    InvalidateRect(hwnd, NULL, TRUE);
}

void PaintDarkStatusBar(HWND hwnd, HDC hdc)
{
    RECT client;
    GetClientRect(hwnd, &client);
    const DarkModeColors& colors = DarkModeGetColors();
    FillRectWithColor(hdc, client, colors.background);

    HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = font != NULL ? SelectObject(hdc, font) : NULL;
    COLORREF oldText = SetTextColor(hdc, colors.readableText);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);

    int borders[3] = {0, 0, 0};
    SendMessage(hwnd, SB_GETBORDERS, 0, reinterpret_cast<LPARAM>(borders));
    const int partCount = static_cast<int>(SendMessage(hwnd, SB_GETPARTS, 0, 0));
    const int count = partCount > 0 ? partCount : 1;

    for (int i = 0; i < count; ++i)
    {
        RECT part = client;
        if (partCount > 0)
            SendMessage(hwnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&part));

        if (i + 1 < count)
        {
            RECT edge = part;
            edge.left = edge.right - (borders[2] > 1 ? borders[2] : 1);
            FillRectWithColor(hdc, edge, RGB(0x4A, 0x4A, 0x4A));
        }

        part.left += borders[2] + 2;
        part.right -= borders[0] + 2;

        const LRESULT textLen = SendMessage(hwnd, SB_GETTEXTLENGTH, i, 0);
        const DWORD flags = HIWORD(textLen);
        const int rawTextLen = static_cast<int>(LOWORD(textLen));
        const int len = rawTextLen < 1023 ? rawTextLen : 1023;
        TCHAR text[1024];
        text[0] = 0;
        const LRESULT itemData = SendMessage(hwnd, SB_GETTEXT, i, reinterpret_cast<LPARAM>(text));

        if ((flags & SBT_OWNERDRAW) != 0 && len == 0)
        {
            const UINT id = static_cast<UINT>(GetDlgCtrlID(hwnd));
            DRAWITEMSTRUCT dis = {0};
            dis.CtlType = ODT_STATIC;
            dis.CtlID = id;
            dis.itemID = static_cast<UINT>(i);
            dis.itemAction = ODA_DRAWENTIRE;
            dis.hwndItem = hwnd;
            dis.hDC = hdc;
            dis.rcItem = part;
            dis.itemData = static_cast<ULONG_PTR>(itemData);
            SendMessage(GetParent(hwnd), WM_DRAWITEM, id, reinterpret_cast<LPARAM>(&dis));
        }
        else
        {
            text[len] = 0;
            DrawText(hdc, text, -1, &part, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_PATH_ELLIPSIS);
        }
    }

    SetBkMode(hdc, oldBkMode);
    SetTextColor(hdc, oldText);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);

    // Draw the size-grip handle in the bottom-right corner when the
    // status bar has SBARS_SIZEGRIP.  The dark theme override
    // (PaintDarkStatusBar) replaces default painting, so the grip must
    // be drawn manually.
    if ((GetWindowLong(hwnd, GWL_STYLE) & SBARS_SIZEGRIP) != 0)
    {
        const int gripW = GetSystemMetrics(SM_CXVSCROLL);
        const int gripH = GetSystemMetrics(SM_CYHSCROLL);
        RECT gripRc = {client.right - gripW, client.bottom - gripH,
                       client.right, client.bottom};
        HPEN hPen = CreatePen(PS_SOLID, 1, colors.readableText);
        if (hPen != NULL)
        {
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
            for (int i = 0; i < gripW - 2; i += 4)
            {
                MoveToEx(hdc, gripRc.right - i - 2, gripRc.bottom - 2, NULL);
                LineTo(hdc, gripRc.right - 2, gripRc.bottom - i - 2);
            }
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        }
    }
}

LRESULT CALLBACK DarkStatusBarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                       UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkStatusBarSubclass, kDarkModeStatusBarSubclassId);
        break;

    case WM_ERASEBKGND:
        return ShouldUseDarkColorsForSurfaces() ? TRUE : DefSubclassProc(hwnd, msg, wParam, lParam);

    case WM_PAINT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc != NULL)
                PaintDarkStatusBar(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_PRINTCLIENT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PaintDarkStatusBar(hwnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }
        break;

    case SB_SETTEXT:
    case SB_SETPARTS:
    case WM_SETTEXT:
    case WM_SIZE:
    case WM_THEMECHANGED:
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnsureDarkStatusBarSubclass(HWND hwnd, bool enableDark)
{
    if (hwnd == NULL)
        return;

    if (enableDark)
        SetWindowSubclass(hwnd, DarkStatusBarSubclass, kDarkModeStatusBarSubclassId, 0);
    else
        RemoveWindowSubclass(hwnd, DarkStatusBarSubclass, kDarkModeStatusBarSubclassId);
    InvalidateRect(hwnd, NULL, TRUE);
}
#endif

constexpr UINT_PTR kDarkModeStaticFrameSubclassId = 0x44525346; // "DRSF"

bool IsStaticFrameStyle(HWND hwnd)
{
    if (hwnd == NULL)
        return false;

    wchar_t className[16] = {0};
    if (GetClassNameW(hwnd, className, _countof(className)) == 0 || wcscmp(className, L"Static") != 0)
        return false;

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const LONG_PTR type = style & SS_TYPEMASK;
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    return type == SS_ETCHEDHORZ || type == SS_ETCHEDVERT || type == SS_ETCHEDFRAME ||
           type == SS_GRAYFRAME || type == SS_BLACKFRAME ||
           (exStyle & WS_EX_STATICEDGE) == WS_EX_STATICEDGE;
}

void PaintDarkStaticFrame(HWND hwnd, HDC hdc)
{
    RECT rc;
    GetClientRect(hwnd, &rc);

    const COLORREF lineColor = RGB(0x38, 0x38, 0x38);
    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const LONG_PTR type = style & SS_TYPEMASK;

    if (type == SS_ETCHEDVERT)
    {
        RECT line = rc;
        line.left += (line.right - line.left) / 2;
        line.right = line.left + 1;
        FillRectWithColor(hdc, line, lineColor);
    }
    else if (type == SS_ETCHEDFRAME || type == SS_GRAYFRAME || type == SS_BLACKFRAME ||
             (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_STATICEDGE) == WS_EX_STATICEDGE)
    {
        HBRUSH brush = CreateSolidBrush(lineColor);
        if (brush != NULL)
        {
            FrameRect(hdc, &rc, brush);
            DeleteObject(brush);
        }
    }
    else
    {
        RECT line = rc;
        line.top += (line.bottom - line.top) / 2;
        line.bottom = line.top + 1;
        FillRectWithColor(hdc, line, lineColor);
    }
}

LRESULT CALLBACK DarkStaticFrameSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                         UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    switch (msg)
    {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, DarkStaticFrameSubclass, kDarkModeStaticFrameSubclassId);
        break;

    case WM_ERASEBKGND:
        return ShouldUseDarkColorsForSurfaces() ? TRUE : DefSubclassProc(hwnd, msg, wParam, lParam);

    case WM_PAINT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc != NULL)
                PaintDarkStaticFrame(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        break;

    case WM_PRINTCLIENT:
        if (ShouldUseDarkColorsForSurfaces())
        {
            PaintDarkStaticFrame(hwnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }
        break;

    case WM_THEMECHANGED:
    case WM_ENABLE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnsureDarkStaticFrameSubclass(HWND hwnd, bool enableDark)
{
    if (hwnd == NULL || !IsStaticFrameStyle(hwnd))
        return;

    if (enableDark)
        SetWindowSubclass(hwnd, DarkStaticFrameSubclass, kDarkModeStaticFrameSubclassId, 0);
    else
        RemoveWindowSubclass(hwnd, DarkStaticFrameSubclass, kDarkModeStaticFrameSubclassId);
    InvalidateRect(hwnd, NULL, TRUE);
}

constexpr UINT_PTR kDarkModeListViewSurfaceSubclassId = 0x44524C56; // "DRLV"

void PaintDarkListViewBorder(HWND hwnd)
{
    HDC hdc = GetWindowDC(hwnd);
    if (hdc == NULL)
        return;

    RECT rc;
    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);
    HBRUSH brush = CreateSolidBrush(RGB(0x38, 0x38, 0x38));
    if (brush != NULL)
    {
        FrameRect(hdc, &rc, brush);
        DeleteObject(brush);
    }
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK DarkListViewSurfaceSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                             UINT_PTR subclassId, DWORD_PTR refData)
{
    (void)subclassId;
    (void)refData;

    if (msg == WM_NCDESTROY)
        RemoveWindowSubclass(hwnd, DarkListViewSurfaceSubclass, kDarkModeListViewSurfaceSubclassId);

    LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
    if ((msg == WM_NCPAINT || msg == WM_NCACTIVATE || msg == WM_SIZE) && ShouldUseDarkColorsForSurfaces())
        PaintDarkListViewBorder(hwnd);

    if (msg == WM_PAINT && ShouldUseDarkColorsForSurfaces())
    {
        HDC hdc = GetDC(hwnd);
        if (hdc == NULL)
            return result;

#if USE_DARKMODELIB
        const COLORREF background = dmlib::getCtrlBackgroundColor();
#else
        const COLORREF background = DarkModeGetColors().background;
#endif
        RECT unused;
        GetClientRect(hwnd, &unused);
        int count = ListView_GetItemCount(hwnd);
        if (count > 0)
        {
            HWND header = ListView_GetHeader(hwnd);
            int columnCount = header != NULL ? Header_GetItemCount(header) : 0;
            if (columnCount > 0)
            {
                RECT lastColumn;
                if (Header_GetItemRect(header, columnCount - 1, &lastColumn))
                {
                    MapWindowPoints(header, hwnd, reinterpret_cast<POINT*>(&lastColumn), 2);
                    int firstVisible = ListView_GetTopIndex(hwnd);
                    int lastVisible = (std::min)(count - 1, firstVisible + ListView_GetCountPerPage(hwnd));
                    for (int i = firstVisible; i <= lastVisible; ++i)
                    {
                        RECT row;
                        if (ListView_GetItemRect(hwnd, i, &row, LVIR_BOUNDS))
                        {
                            row.left = (std::max)(row.left, lastColumn.right);
                            row.right = unused.right;
                            if (row.left < row.right)
                                FillRectWithColor(hdc, row, background);
                        }
                    }
                }
            }

            RECT lastItem;
            if (ListView_GetItemRect(hwnd, count - 1, &lastItem, LVIR_BOUNDS))
                unused.top = (std::min)(unused.bottom, lastItem.bottom);
        }
        if (unused.top < unused.bottom)
            FillRectWithColor(hdc, unused, background);
        ReleaseDC(hwnd, hdc);
        PaintDarkListViewBorder(hwnd);
    }
    return result;
}

void EnsureDarkListViewSurfaceSubclass(HWND hwnd, bool enableDark)
{
    if (hwnd == NULL)
        return;

    if (enableDark)
        SetWindowSubclass(hwnd, DarkListViewSurfaceSubclass, kDarkModeListViewSurfaceSubclassId, 0);
    else
        RemoveWindowSubclass(hwnd, DarkListViewSurfaceSubclass, kDarkModeListViewSurfaceSubclassId);
}

int ComputeLuminance(COLORREF color)
{
    return (GetRValue(color) * 30 + GetGValue(color) * 59 + GetBValue(color) * 11) / 100;
}

COLORREF ResolveReadableForeground(COLORREF foreground, COLORREF background)
{
    const int backgroundLum = ComputeLuminance(background);
    const int foregroundLum = ComputeLuminance(foreground);

    if (backgroundLum < 128)
    {
        if (foregroundLum < backgroundLum + 40)
            return RGB(0xF0, 0xF0, 0xF0);
    }
    else
    {
        if (foregroundLum > backgroundLum - 40)
            return RGB(0x20, 0x20, 0x20);
    }

    return foreground;
}

bool IsHighContrast()
{
    HIGHCONTRASTW highContrast = {sizeof(highContrast)};
    if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(highContrast), &highContrast, FALSE))
        return (highContrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
    return false;
}

void RefreshColorPolicy()
{
    if (gRefreshImmersiveColorPolicyState)
        gRefreshImmersiveColorPolicyState();
    if (gGetIsImmersiveColorUsingHighContrast)
        gGetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
}

bool ShouldUseDarkColorsInternal()
{
    if (!gEnabled || !gSupported)
        return false;
    if (!gShouldAppsUseDarkMode)
        return false;
    return gShouldAppsUseDarkMode() && !IsHighContrast();
}

static bool DarkModeIsWindowsDarkSchemeSelectedInternal()
{
    return gWindowsDarkSchemeSelected;
}

bool ShouldUseDarkColorsForSurfaces()
{
    if (ShouldUseDarkColorsInternal())
        return true;
    if (!DarkModeIsWindowsDarkSchemeSelectedInternal())
        return false;
    return ComputeLuminance(gDialogBackgroundColor) < 128;
}

bool ShouldApplyNativeDarkEnhancements()
{
    return DarkModeIsWindowsDarkSchemeSelectedInternal() && !IsHighContrast();
}

BOOL CALLBACK ApplyTreeCallback(HWND hwnd, LPARAM)
{
    DarkModeApplyTree(hwnd);
    return TRUE;
}

void HookDarkScrollbars()
{
#if USE_DARKMODELIB
    dmlib::initDarkMode();
    dmlib::setDarkModeConfigEx(static_cast<UINT>(DarkModeShouldUseDarkColors() ? dmlib::DarkModeType::dark : dmlib::DarkModeType::classic));
    dmlib::setDefaultColors(true);
#endif
    gScrollbarsHooked = true;
}

bool MatchesAnyClass(const wchar_t* className, const wchar_t* const* list, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (wcscmp(className, list[i]) == 0)
            return true;
    }
    return false;
}

void ApplyControlTheme(HWND hwnd)
{
    if (hwnd == NULL)
        return;

    wchar_t className[64];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0)
        return;

    static const wchar_t* const explorerClasses[] = {
        L"ReBarWindow32",
        L"ToolbarWindow32",
        L"msctls_progress32",
        L"msctls_statusbar32",
        L"msctls_trackbar32",
        L"msctls_scrollbar32",
        L"ScrollBar",
    };

    static const wchar_t* const darkExplorerClasses[] = {
        L"SysListView32",
        L"SysTreeView32",
        L"SysHeader32",
        L"SysTabControl32",
        L"ComboBoxEx32",
        L"ReBarWindow32",
        L"ToolbarWindow32",
        L"ScrollBar",
    };

    static const wchar_t* const cfdClasses[] = {
        L"Edit",
        L"ComboBox",
        L"RichEdit20W",
        L"RICHEDIT50W",
    };

    const bool wantDark = ShouldUseDarkColorsInternal();
    const wchar_t* const appliedTheme = reinterpret_cast<const wchar_t*>(GetPropW(hwnd, kDarkModeThemeProp));
    const wchar_t* theme = nullptr;

    if (wantDark)
    {
        if (wcscmp(className, L"SysListView32") == 0 || wcscmp(className, L"SysHeader32") == 0)
            theme = L"ItemsView";
        else if (MatchesAnyClass(className, darkExplorerClasses, _countof(darkExplorerClasses)))
            theme = L"DarkMode_Explorer";
        else if (wcscmp(className, L"Button") == 0)
        {
            if (GetPropW(hwnd, kDarkModeClassicButtonProp) == NULL)
                theme = L"Explorer";
        }
        else if (MatchesAnyClass(className, explorerClasses, _countof(explorerClasses)))
            theme = L"Explorer";
        else if (MatchesAnyClass(className, cfdClasses, _countof(cfdClasses)))
            theme = L"CFD";
    }

    auto notifyThemeChanged = [](HWND target) {
        if (!gPropagatingThemeChange && gThemeChangeDepth == 0)
        {
            ++gThemeChangeDepth;
            gPropagatingThemeChange = true;
            SendMessageW(target, WM_THEMECHANGED, 0, 0);
            gPropagatingThemeChange = false;
            --gThemeChangeDepth;
        }
    };

    if (theme != nullptr)
    {
        if (appliedTheme != theme)
        {
            SetPropW(hwnd, kDarkModeThemeProp, reinterpret_cast<HANDLE>(const_cast<wchar_t*>(theme)));
            if (gSetWindowTheme)
                gSetWindowTheme(hwnd, theme, nullptr);
            notifyThemeChanged(hwnd);
        }
    }
    else if (appliedTheme != nullptr)
    {
        RemovePropW(hwnd, kDarkModeThemeProp);
        if (gSetWindowTheme)
            gSetWindowTheme(hwnd, nullptr, nullptr);
        notifyThemeChanged(hwnd);
    }
}

void ApplyListTreeThemeRecursive(HWND hwnd, bool wantDark)
{
    if (hwnd == NULL)
        return;

    wchar_t className[64];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0)
        return;

    auto invalidateParentChain = [](HWND ctrl) {
        for (HWND p = GetParent(ctrl); p != NULL; p = GetParent(p))
            InvalidateRect(p, NULL, TRUE);
    };

    auto applyChromeTheme = [&](HWND ctrl, const wchar_t* className) {
        if (gSetWindowTheme == nullptr)
            return;
        if (wcscmp(className, L"ReBarWindow32") == 0 || wcscmp(className, L"ToolbarWindow32") == 0 ||
            wcscmp(className, L"MenuBar") == 0)
        {
            gSetWindowTheme(ctrl, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);
            InvalidateRect(ctrl, NULL, TRUE);
            invalidateParentChain(ctrl);
        }
    };

    if (gSetWindowTheme != nullptr)
    {
        if (wcscmp(className, L"SysListView32") == 0 || wcscmp(className, L"SysTreeView32") == 0)
        {
            gSetWindowTheme(hwnd, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);
            // Common controls do not consistently treat CLR_DEFAULT as a
            // reset value here (TreeView can interpret it as black), so restore
            // explicit system colors for all non-dark Salamander schemes.
            const COLORREF bg = wantDark ? DarkModeGetColors().background : GetSysColor(COLOR_WINDOW);
            const COLORREF fg = wantDark ? DarkModeGetColors().readableText : GetSysColor(COLOR_WINDOWTEXT);
            if (wcscmp(className, L"SysListView32") == 0)
            {
                ListView_SetTextColor(hwnd, fg);
#if USE_DARKMODELIB
                const COLORREF lvBg = DarkMode_ShouldUseDark() ? dmlib::getCtrlBackgroundColor() : bg;
                ListView_SetTextBkColor(hwnd, lvBg);
                ListView_SetBkColor(hwnd, lvBg);
#else
                ListView_SetTextBkColor(hwnd, bg);
                ListView_SetBkColor(hwnd, bg);
#endif
#if USE_DARKMODELIB
                DarkModeBackendDarkModelib::UpdateListViewColors(hwnd, fg, bg, wantDark && ShouldUseDarkColorsForSurfaces());
#else
                HWND header = ListView_GetHeader(hwnd);
                if (header != NULL)
                    EnsureDarkHeaderSubclass(header, wantDark && ShouldUseDarkColorsForSurfaces());
#endif
            }
            else
            {
                TreeView_SetTextColor(hwnd, fg);
                TreeView_SetBkColor(hwnd, bg);
            }
            RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }
        else if (wcscmp(className, L"SysHeader32") == 0)
        {
            gSetWindowTheme(hwnd, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);
            const COLORREF bg = DarkModeGetColors().background;
            const COLORREF fg = DarkModeGetColors().readableText;
            SendMessage(hwnd, HDM_SETTEXTCOLOR, 0, static_cast<LPARAM>(wantDark ? fg : CLR_DEFAULT));
            SendMessage(hwnd, HDM_SETBKCOLOR, 0, static_cast<LPARAM>(wantDark ? bg : CLR_DEFAULT));
#if !USE_DARKMODELIB
            EnsureDarkHeaderSubclass(hwnd, wantDark && ShouldUseDarkColorsForSurfaces());
#endif
            InvalidateRect(hwnd, NULL, TRUE);
            invalidateParentChain(hwnd);
            SendMessage(hwnd, WM_THEMECHANGED, 0, 0);
        }
        else if (wcscmp(className, L"tooltips_class32") == 0 || wcscmp(className, L"ScrollBar") == 0)
        {
            gSetWindowTheme(hwnd, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (wcscmp(className, L"Button") == 0)
        {
            const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            const LONG_PTR type = style & BS_TYPEMASK;
            if (type == BS_GROUPBOX)
            {
#if USE_DARKMODELIB
                // darkmodelib's setGroupboxCtrlSubclass handles groupbox rendering
                // natively. EnsureClassicButtonTheme strips the theme and produces
                // a classic look with white thick borders, which conflicts with
                // darkmodelib's dark groupbox styling.
#else
                EnsureClassicButtonTheme(hwnd, wantDark);
#endif
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else if (type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON ||
                     type == BS_AUTOCHECKBOX || type == BS_CHECKBOX || type == BS_AUTO3STATE ||
                     type == BS_3STATE)
            {
#if USE_DARKMODELIB
                DarkModeBackendDarkModelib::ApplyCheckboxOrRadioButton(hwnd, wantDark);
#endif
                if (ShouldOwnerDrawChoiceButton(hwnd))
                    EnsureDarkChoiceButtonSubclass(hwnd, wantDark);
                else
                {
                    EnsureDarkChoiceButtonSubclass(hwnd, false);
                    if (gSetWindowTheme != nullptr)
                        gSetWindowTheme(hwnd, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        else if (wcscmp(className, L"msctls_statusbar32") == 0)
        {
#if USE_DARKMODELIB
            DarkModeBackendDarkModelib::ApplyStatusBar(hwnd, wantDark && ShouldUseDarkColorsForSurfaces());
#else
            EnsureDarkStatusBarSubclass(hwnd, ShouldUseDarkColorsForSurfaces());
#endif
        }
        else if (wcscmp(className, L"msctls_progress32") == 0)
        {
#if USE_DARKMODELIB
            DarkModeBackendDarkModelib::ApplyProgressBar(hwnd, wantDark && ShouldUseDarkColorsForSurfaces());
#endif
        }
#if !USE_DARKMODELIB
        else if (wcscmp(className, L"SysHeader32") == 0)
        {
            EnsureDarkHeaderSubclass(hwnd, ShouldUseDarkColorsForSurfaces());
        }
#endif
        else if (wcscmp(className, L"Static") == 0)
        {
            EnsureDarkStaticFrameSubclass(hwnd, wantDark && ShouldUseDarkColorsForSurfaces());
        }
    }
    applyChromeTheme(hwnd, className);

    for (HWND child = GetWindow(hwnd, GW_CHILD); child != NULL; child = GetWindow(child, GW_HWNDNEXT))
        ApplyListTreeThemeRecursive(child, wantDark);
}

void EnsureInitialized()
{
    if (gInitialized)
        return;

    gInitialized = true;

    HMODULE hNt = GetModuleHandleW(L"ntdll.dll");
    if (hNt)
    {
        auto rtlGetVersion = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(hNt, "RtlGetNtVersionNumbers"));
        if (rtlGetVersion)
        {
            DWORD major = 0, minor = 0, build = 0;
            rtlGetVersion(&major, &minor, &build);
            build &= 0xFFFF;
            gBuildNumber = build;
        }
    }

    gUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (gUxTheme)
        gSetWindowTheme = reinterpret_cast<fnSetWindowTheme>(GetProcAddress(gUxTheme, "SetWindowTheme"));

    if (gBuildNumber < 17763)
    {
        gSupported = false;
        return;
    }

    if (!gUxTheme)
    {
        gSupported = false;
        return;
    }

    gAllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(133)));
    gShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(132)));
    gFlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(136)));
    gRefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(104)));
    gIsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(137)));
    gGetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(106)));
    gShouldSystemUseDarkMode = reinterpret_cast<fnShouldSystemUseDarkMode>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(138)));
    gIsDarkModeAllowedForApp = reinterpret_cast<fnIsDarkModeAllowedForApp>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(139)));

    if (gBuildNumber >= 18362)
        gSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(135)));
    else
        gAllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(135)));

    auto hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32)
        gSetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(hUser32, "SetWindowCompositionAttribute"));

    gDwmApi = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (gDwmApi)
        gDwmSetWindowAttribute = reinterpret_cast<fnDwmSetWindowAttribute>(GetProcAddress(gDwmApi, "DwmSetWindowAttribute"));

    gSupported = gAllowDarkModeForWindow != nullptr &&
                 (gAllowDarkModeForApp != nullptr || gSetPreferredAppMode != nullptr) &&
                 gShouldAppsUseDarkMode != nullptr;

    if (!gSupported)
        return;
}

} // namespace

bool DarkModeIsWindowsDarkSchemeSelected()
{
    return gWindowsDarkSchemeSelected;
}

bool DarkModeInitialize()
{
    EnsureInitialized();
    return gSupported;
}

bool DarkModeIsSupported()
{
    EnsureInitialized();
    return gSupported;
}

void DarkModeDetectAndEnableSystemDarkMode()
{
    EnsureInitialized();
    if (gSupported && gShouldAppsUseDarkMode && gShouldAppsUseDarkMode() && !IsHighContrast())
    {
        DarkModeSetEnabled(true);

        // Set up default dark dialog colors and brush so that WM_CTLCOLOR* handlers
        // return dark brushes for message boxes shown before ColorsChanged() runs.
        const COLORREF darkBg = RGB(0x20, 0x20, 0x20);
        const COLORREF darkText = RGB(0xDC, 0xDC, 0xDC);
        DarkModeSetConfiguredColors(darkText, darkBg,
                                    GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE));
        const DarkModeColors& palette = DarkModeGetColors();
        HBRUSH darkBrush = CreateSolidBrush(palette.background);
        DarkModeConfigureDialogColors(palette.readableText, palette.background, darkBrush);
        gDialogBrushOwned = true;
    }
}

void DarkModeSetEnabled(bool enabled)
{
    EnsureInitialized();
    if (!gSupported)
        return;

    gWindowsDarkSchemeSelected = enabled;
    bool newEnabled = enabled && ShouldApplyNativeDarkEnhancements();
    if (gEnabled == newEnabled)
        return;

    gEnabled = newEnabled;

    if (gAllowDarkModeForApp)
        gAllowDarkModeForApp(gEnabled);
    else if (gSetPreferredAppMode)
        gSetPreferredAppMode(gEnabled ? AllowDark : Default);

    // The scrollbar fix patches a process-wide comctl32 import with a callback
    // located in the calling module. Do not install it implicitly here because
    // plugins also compile this helper and can be unloaded while comctl32 is
    // still using their callback. The non-unloadable host installs the hook
    // explicitly through DarkModeFixScrollbars().
    RefreshColorPolicy();

    if (gFlushMenuThemes)
        gFlushMenuThemes();
}

bool DarkModeShouldUseDarkColors()
{
    EnsureInitialized();

    if (!DarkModeIsWindowsDarkSchemeSelected())
        return false;

    if (ShouldUseDarkColorsInternal())
        return true;

    // Fall back to the configured dialog palette only for the explicit Windows
    // Dark Mode scheme when native dark mode isn't available (for example on
    // older Windows builds), or when Salamander is dark while the system is
    // light.  Light/custom schemes must stay native light UI and must not
    // re-enter dark CTLCOLOR/subclass paths just because the system is dark.
    return ComputeLuminance(gDialogBackgroundColor) < 128;
}

BOOL DarkMode_ShouldUseDark()
{
    return DarkModeShouldUseDarkColors() ? TRUE : FALSE;
}

void DarkModeApplyWindow(HWND hwnd)
{
    EnsureInitialized();
    if (!gSupported || hwnd == NULL)
        return;

    if (gAllowDarkModeForWindow)
        gAllowDarkModeForWindow(hwnd, gEnabled);

    ApplyControlTheme(hwnd);
}

void DarkModeSuspendForLightCreation()
{
    EnsureInitialized();
    if (gSupported && gAllowDarkModeForApp)
        gAllowDarkModeForApp(false);
    else if (gSupported && gSetPreferredAppMode)
        gSetPreferredAppMode(Default);
}

void DarkModeResumeAfterLightCreation()
{
    EnsureInitialized();
    if (gSupported && gAllowDarkModeForApp)
        gAllowDarkModeForApp(gEnabled);
    else if (gSupported && gSetPreferredAppMode)
        gSetPreferredAppMode(gEnabled ? AllowDark : Default);
}

void DarkModeRemoveTree(HWND hwnd)
{
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::RemoveTree(hwnd);
#endif
    if (hwnd != NULL && gSupported && gAllowDarkModeForWindow)
        gAllowDarkModeForWindow(hwnd, false);
}

void DarkModeApplyTree(HWND hwnd)
{
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::ApplyTree(hwnd);
#endif
    EnsureInitialized();
    if (!gSupported || hwnd == NULL)
        return;

    DarkModeApplyWindow(hwnd);
    ApplyListTreeThemeRecursive(hwnd, DarkModeIsWindowsDarkSchemeSelected());
    EnumChildWindows(hwnd, ApplyTreeCallback, 0);
}

void DarkModeApplyMenuBar(HWND hwnd)
{
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::ApplyMenuBar(hwnd, DarkModeShouldUseDarkColors());
#else
    (void)hwnd;
#endif
}

void DarkModeRefreshTitleBar(HWND hwnd)
{
    EnsureInitialized();
    if (!gSupported || hwnd == NULL)
        return;

    // Keep the per-window native opt-in synchronized here too, not only in
    // DarkModeApplyWindow().  Shutdown/close dialogs can be created while the
    // main window is already tearing down; setting the DWM attribute without
    // first reasserting the opt-in can leave the caption on the native light
    // path for one paint and causes a white title-bar flash.
    if (gAllowDarkModeForWindow)
        gAllowDarkModeForWindow(hwnd, gEnabled);

    BOOL useDark = ShouldUseDarkColorsInternal() ? TRUE : FALSE;

    SetPropW(hwnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(useDark)));

    if (gDwmSetWindowAttribute)
    {
        HRESULT hr = gDwmSetWindowAttribute(hwnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &useDark, sizeof(useDark));
        if (FAILED(hr))
            gDwmSetWindowAttribute(hwnd, 19 /* DWMWA_USE_IMMERSIVE_DARK_MODE before 20H1 */, &useDark, sizeof(useDark));
    }
    else if (gSetWindowCompositionAttribute)
    {
        WINDOWCOMPOSITIONATTRIBDATA data = {WCA_USEDARKMODECOLORS, &useDark, sizeof(useDark)};
        gSetWindowCompositionAttribute(hwnd, &data);
    }
}

bool DarkModeHandleSettingChange(UINT message, LPARAM lParam)
{
    EnsureInitialized();
    if (!gSupported)
        return false;

    if (message != WM_SETTINGCHANGE && message != WM_THEMECHANGED)
        return false;

    bool isColor = (message == WM_THEMECHANGED);
    bool shouldRefresh = (message == WM_THEMECHANGED);
    if (lParam != 0)
    {
        // WM_SETTINGCHANGE delivers lParam in the character format of the
        // receiving window. Salamander's main window is an ANSI window in the
        // regular build, so treating the pointer unconditionally as LPCWSTR can
        // read past the short ANSI buffer when Windows broadcasts accent-color
        // changes such as "ImmersiveColorSet".
        LPCTSTR settingName = reinterpret_cast<LPCTSTR>(lParam);
        if (_tcsicmp(settingName, _T("ImmersiveColorSet")) == 0)
        {
            isColor = true;
            shouldRefresh = true;
        }
        else if (_tcsicmp(settingName, _T("WindowsThemeElement")) == 0)
        {
            isColor = true;
            shouldRefresh = true;
        }
    }
    else
    {
        shouldRefresh = true;
    }

    struct ThemeBatchScope
    {
        ThemeBatchScope() { ++gThemeBatchDepth; }
        ~ThemeBatchScope() { --gThemeBatchDepth; }
        bool IsRoot() const { return gThemeBatchDepth == 1; }
    } scope;

    if (shouldRefresh)
    {
        const bool nativeEnhancements = ShouldApplyNativeDarkEnhancements();
        if (gEnabled != nativeEnhancements)
            DarkModeSetEnabled(nativeEnhancements);
        RefreshColorPolicy();
        if (scope.IsRoot() && message == WM_THEMECHANGED)
            gThemeChangeDepth = 0;
    }

    return isColor;
}

void DarkModeFixScrollbars()
{
    // HookDarkScrollbars installs a callback into comctl32. A callback owned by
    // an unloadable plugin would become a dangling function pointer when that
    // plugin is released, so only allow the process executable to install it.
    HMODULE callerModule = NULL;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&DarkModeFixScrollbars), &callerModule) ||
        callerModule != GetModuleHandleW(NULL))
    {
        return;
    }

    EnsureInitialized();
    if (!gSupported)
        return;

#if USE_DARKMODELIB
    if (!gScrollbarsHooked)
        HookDarkScrollbars();
#endif
}

static void AllowDarkScrollbarsInHost(HWND hwnd)
{
    if (hwnd == NULL)
        return;

#if USE_DARKMODELIB && defined(_DARKMODELIB_USE_SCROLLBAR_FIX) && (_DARKMODELIB_USE_SCROLLBAR_FIX > 1)
    dmlib::enableDarkScrollBarForWindowAndChildren(hwnd);
    // Plugin windows normally opt in from WM_CREATE, after their WS_*SCROLL
    // non-client scrollbars have already obtained a theme handle.  Reapply
    // the Explorer theme to make those existing scrollbars reopen their
    // theme through the now-enabled scoped hook.
    static const wchar_t kScrollBarThemeAppliedProp[] = L"Salamander.DarkScrollBarThemeApplied";
    if (GetPropW(hwnd, kScrollBarThemeAppliedProp) == NULL)
    {
        // SetWindowTheme can synchronously deliver WM_THEMECHANGED.  Mark the
        // window first so its handler's dark-mode refresh does not recursively
        // attempt to reapply the same theme until the stack overflows.
        SetPropW(hwnd, kScrollBarThemeAppliedProp, reinterpret_cast<HANDLE>(1));
        dmlib::setDarkScrollBar(hwnd);
    }
#else
    (void)hwnd;
#endif
}

extern "C" __declspec(dllexport) void WINAPI SalamanderAllowDarkScrollbars(HWND hwnd)
{
    AllowDarkScrollbarsInHost(hwnd);
}

void DarkModeAllowDarkScrollbars(HWND hwnd)
{
    if (hwnd == NULL)
        return;

    // Plugins link their own copy of darkmode.cpp, whereas the IAT callback
    // and its scoped-window set belong to salamand.exe.  Route plugin calls
    // to the exported host entry point instead of updating their inert local
    // DarkModeLib instance.
    HMODULE callerModule = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(&DarkModeAllowDarkScrollbars), &callerModule);
    if (callerModule != GetModuleHandleW(NULL))
    {
        typedef void(WINAPI* HostAllowDarkScrollbarsFn)(HWND);
        HostAllowDarkScrollbarsFn hostAllow = reinterpret_cast<HostAllowDarkScrollbarsFn>(
            GetProcAddress(GetModuleHandleW(NULL), "SalamanderAllowDarkScrollbars"));
        if (hostAllow != NULL)
            hostAllow(hwnd);
        return;
    }

    AllowDarkScrollbarsInHost(hwnd);
}

void DarkModeDisallowDarkScrollbars(HWND hwnd)
{
    if (hwnd == NULL)
        return;

#if USE_DARKMODELIB && defined(_DARKMODELIB_USE_SCROLLBAR_FIX) && (_DARKMODELIB_USE_SCROLLBAR_FIX > 1)
    dmlib::disableDarkScrollBarForWindowAndChildren(hwnd);
#else
    (void)hwnd;
#endif
}

void DarkModeConfigureDialogColors(COLORREF textColor, COLORREF backgroundColor, HBRUSH dialogBrush)
{
    if (gDialogBrushOwned && gDialogBrushHandle != NULL && gDialogBrushHandle != dialogBrush)
    {
        ::DeleteObject(gDialogBrushHandle);
    }
    gDialogTextColor = textColor;
    gDialogBackgroundColor = backgroundColor;
    gDialogBrushHandle = dialogBrush;
    gDialogBrushOwned = false;
    gColors.text = textColor;
    gColors.background = backgroundColor;
    gColors.readableText = ResolveReadableForeground(textColor, backgroundColor);
    gColors.usingSchemeColors = true;
}

void DarkModeSetConfiguredColors(COLORREF schemeTextColor, COLORREF schemeBackgroundColor,
                                 COLORREF fallbackTextColor, COLORREF fallbackBackgroundColor)
{
    const COLORREF text = (schemeTextColor == CLR_INVALID) ? fallbackTextColor : schemeTextColor;
    const COLORREF background = (schemeBackgroundColor == CLR_INVALID) ? fallbackBackgroundColor : schemeBackgroundColor;
    gColors.text = text;
    gColors.background = background;
    gColors.readableText = ResolveReadableForeground(text, background);
    gColors.usingSchemeColors = (schemeTextColor != CLR_INVALID) && (schemeBackgroundColor != CLR_INVALID);
    gDialogTextColor = gColors.text;
    gDialogBackgroundColor = gColors.background;
}

const DarkModeColors& DarkModeGetColors()
{
    EnsureInitialized();
    gColors.readableText = ResolveReadableForeground(gColors.text, gColors.background);
    return gColors;
}

void DarkModeSetAutocompleteSelectedColors(COLORREF fg, COLORREF bk)
{
    gAutocompleteSelectedFg = fg;
    gAutocompleteSelectedBk = bk;
}

COLORREF DarkModeGetDialogTextColor()
{
    EnsureInitialized();
    return DarkModeGetColors().readableText;
}

COLORREF DarkModeGetDialogBackgroundColor()
{
    EnsureInitialized();
    return DarkModeGetColors().background;
}

COLORREF DarkModeGetDisabledTextColor()
{
    // Keep custom-drawn controls consistent with darkmodelib's disabled labels.
    return RGB(0x80, 0x80, 0x80);
}

COLORREF DarkModeEnsureReadableForeground(COLORREF foreground, COLORREF background)
{
    return ResolveReadableForeground(foreground, background);
}

void DarkModeUpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors)
{
    EnsureInitialized();

    if (listView == NULL)
        return;

    // Updating the native or darkmodelib theme for a list view may synchronously
    // send WM_THEMECHANGED back to the same control.  The find dialog's result
    // list handles that message by refreshing its list-view colors, so without a
    // guard the nested WM_THEMECHANGED re-enters this function until the stack is
    // exhausted.  Let the outer refresh finish; it already applies the requested
    // colors and invalidates the control.
    if (gListViewColorUpdateDepth != 0)
        return;

    struct ListViewColorUpdateScope
    {
        ListViewColorUpdateScope() { ++gListViewColorUpdateDepth; }
        ~ListViewColorUpdateScope() { --gListViewColorUpdateDepth; }
    } scope;

    const COLORREF resolvedText = DarkModeEnsureReadableForeground(textColor, backgroundColor);
    const COLORREF resolvedBackground = backgroundColor;

#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::UpdateListViewColors(listView, resolvedText, resolvedBackground, applyHeaderColors);
#endif

    DarkModeApplyWindow(listView);

    ListView_SetTextColor(listView, resolvedText);
#if USE_DARKMODELIB
    const COLORREF lvBg = DarkMode_ShouldUseDark() ? dmlib::getCtrlBackgroundColor() : resolvedBackground;
    ListView_SetTextBkColor(listView, lvBg);
    ListView_SetBkColor(listView, lvBg);
#else
    ListView_SetTextBkColor(listView, resolvedBackground);
    ListView_SetBkColor(listView, resolvedBackground);
#endif
    EnsureDarkListViewSurfaceSubclass(listView, applyHeaderColors && ShouldUseDarkColorsForSurfaces());

    HWND header = ListView_GetHeader(listView);
    if (header != NULL)
    {
        DarkModeApplyWindow(header);
        if (applyHeaderColors)
        {
            SendMessage(header, HDM_SETTEXTCOLOR, 0, static_cast<LPARAM>(resolvedText));
            SendMessage(header, HDM_SETBKCOLOR, 0, static_cast<LPARAM>(resolvedBackground));
#if !USE_DARKMODELIB
            EnsureDarkHeaderSubclass(header, ShouldUseDarkColorsForSurfaces());
#endif
        }
        else
        {
            SendMessage(header, HDM_SETTEXTCOLOR, 0, static_cast<LPARAM>(CLR_DEFAULT));
            SendMessage(header, HDM_SETBKCOLOR, 0, static_cast<LPARAM>(CLR_DEFAULT));
#if !USE_DARKMODELIB
            EnsureDarkHeaderSubclass(header, false);
#endif
        }
        InvalidateRect(header, NULL, TRUE);
    }

    InvalidateRect(listView, NULL, TRUE);
}

void DarkModeUpdateTabControlOverflowButtons(HWND tabControl)
{
    EnsureInitialized();
    if (tabControl == NULL)
        return;

    const bool enableDark = DarkModeShouldUseDarkColors();

    HWND upDown = NULL;
    while ((upDown = FindWindowEx(tabControl, upDown, UPDOWN_CLASS, NULL)) != NULL)
    {
        if (gSupported)
            DarkModeApplyWindow(upDown);
        if (gSetWindowTheme != nullptr)
            gSetWindowTheme(upDown, enableDark ? L"DarkMode_Explorer" : nullptr, nullptr);
        // Windows 10 keeps the tab control's internal up-down overflow buttons
        // in a light theme even after the dark Explorer theme is applied.
        // Owner-paint that child as a fallback there; Windows 11 already uses
        // correct native dark colors, so leave its renderer untouched.
        EnsureDarkTabOverflowSubclass(upDown, enableDark && gBuildNumber < 22000);
        InvalidateRect(upDown, NULL, TRUE);
    }
}

void DarkModePreserveCustomTabControl(HWND tabControl)
{
    EnsureInitialized();
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::MarkCustomTabControl(tabControl);
#else
    (void)tabControl;
#endif
}

void DarkModePreserveCustomTreeView(HWND treeView)
{
    EnsureInitialized();
    if (treeView == NULL)
        return;

    SetPropW(treeView, kDarkModeCustomTreeViewProp, reinterpret_cast<HANDLE>(1));

    const bool wantDark = DarkModeIsWindowsDarkSchemeSelected();
    if (gSetWindowTheme != nullptr)
        gSetWindowTheme(treeView, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);

    const COLORREF bg = wantDark ? DarkModeGetColors().background : GetSysColor(COLOR_WINDOW);
    const COLORREF fg = wantDark ? DarkModeGetColors().readableText : GetSysColor(COLOR_WINDOWTEXT);
    TreeView_SetTextColor(treeView, fg);
    TreeView_SetBkColor(treeView, bg);
    RedrawWindow(treeView, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}


void DarkModeEnableAutoSuggestSupport(HWND edit)
{
    EnsureInitialized();
    if (edit == NULL || !DarkModeShouldUseDarkColors())
        return;

    const DWORD threadId = GetWindowThreadProcessId(edit, NULL);
    if (threadId == 0)
        return;

    if (gAutoSuggestWinEventHook == NULL || gAutoSuggestWinEventThreadId != threadId)
    {
        if (gAutoSuggestWinEventHook != NULL)
            UnhookWinEvent(gAutoSuggestWinEventHook);

        gAutoSuggestWinEventHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, NULL,
                                                   AutoSuggestWinEventProc, GetCurrentProcessId(), threadId,
                                                   WINEVENT_OUTOFCONTEXT);
        gAutoSuggestWinEventThreadId = gAutoSuggestWinEventHook != NULL ? threadId : 0;
    }

    HWND dropdown = FindAutoSuggestDropdown(edit);
    if (dropdown != NULL)
        ApplyAutoSuggestDropdownDarkMode(dropdown);
}

void DarkModeApplyRebarSeparators(HWND rebar)
{
    EnsureInitialized();
    if (rebar == NULL)
        return;

    const bool enableDark = DarkModeShouldUseDarkColors();
    UpdateDarkRebarChildSeparators(rebar, enableDark);
    UpdateDarkRebarOverlaySeparators(rebar, enableDark);
    DWORD_PTR refData = 0;
    const bool subclassed = GetWindowSubclass(rebar, DarkRebarSeparatorSubclass,
                                              kDarkModeRebarSeparatorSubclassId, &refData) != FALSE;
    if (enableDark)
    {
        if (!subclassed)
            SetWindowSubclass(rebar, DarkRebarSeparatorSubclass, kDarkModeRebarSeparatorSubclassId, 0);
    }
    else if (subclassed)
        RemoveWindowSubclass(rebar, DarkRebarSeparatorSubclass, kDarkModeRebarSeparatorSubclassId);

    InvalidateRect(rebar, NULL, TRUE);
}

void DarkModeUpdateListViewColors(HWND listView)
{
    EnsureInitialized();

    if (listView == NULL)
        return;

    const COLORREF paletteText = DarkModeGetDialogTextColor();
    const COLORREF paletteBackground = DarkModeGetDialogBackgroundColor();
    const bool useCustomColors = paletteText != GetSysColor(COLOR_WINDOWTEXT) ||
                                 paletteBackground != GetSysColor(COLOR_WINDOW);

    DarkModeUpdateListViewColors(listView, paletteText, paletteBackground, useCustomColors);
}


bool DarkModeHandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    EnsureInitialized();

    const COLORREF background = DarkModeGetDialogBackgroundColor();
    const COLORREF textColor = DarkModeGetDialogTextColor();
    const COLORREF sysTextColor = GetSysColor(COLOR_BTNTEXT);
    const COLORREF sysBackground = GetSysColor(COLOR_BTNFACE);
    const bool usingNativeDark = gSupported && ShouldUseDarkColorsInternal();
    const bool hasCustomPalette = DarkModeIsWindowsDarkSchemeSelected() &&
                                  (textColor != sysTextColor || background != sysBackground);
#if USE_DARKMODELIB
    const bool forceClassicButtons = false;
#else
    const bool forceClassicButtons = hasCustomPalette;
#endif

    if (!usingNativeDark && !hasCustomPalette)
    {
        if ((message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN) && lParam != 0)
        {
            HWND ctrl = reinterpret_cast<HWND>(lParam);
            if (IsCheckboxOrRadioButtonControl(ctrl))
                EnsureDarkChoiceButtonSubclass(ctrl, false);
            else if (IsButtonTypeNeedingClassicFallback(ctrl))
                EnsureClassicButtonTheme(ctrl, false);
        }
        return false;
    }

    HBRUSH brush = gDialogBrushHandle != NULL ? gDialogBrushHandle : GetSysColorBrush(COLOR_BTNFACE);
    HDC hdc = reinterpret_cast<HDC>(wParam);
    if (hdc == NULL)
        return false;

#if USE_DARKMODELIB
    if ((message == WM_CTLCOLORBTN || message == WM_CTLCOLORSTATIC) && lParam != 0)
    {
        HWND ctrl = reinterpret_cast<HWND>(lParam);
        if (IsCheckboxOrRadioButtonControl(ctrl))
        {
            DarkModeBackendDarkModelib::ApplyCheckboxOrRadioButton(ctrl, usingNativeDark || hasCustomPalette);
            if (ShouldOwnerDrawChoiceButton(ctrl))
            {
                EnsureDarkChoiceButtonSubclass(ctrl, usingNativeDark || hasCustomPalette);
                SetTextColor(hdc, DarkModeGetColors().readableText);
                SetBkColor(hdc, background);
                SetBkMode(hdc, TRANSPARENT);
                result = reinterpret_cast<LRESULT>(brush);
                return true;
            }
            EnsureDarkChoiceButtonSubclass(ctrl, false);
        }
    }

    if (DarkModeBackendDarkModelib::HandleCtlColor(message, wParam, lParam, result, DarkModeGetColors(), brush))
        return true;
#endif

    auto setCommonColors = [&](bool transparent) {
        SetTextColor(hdc, textColor);
        SetBkColor(hdc, background);
        if (transparent)
            SetBkMode(hdc, TRANSPARENT);
        else
            SetBkMode(hdc, OPAQUE);
    };

    switch (message)
    {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
        SetBkColor(hdc, background);
        result = reinterpret_cast<LRESULT>(brush);
        return true;

    case WM_CTLCOLORSTATIC:
    {
        HWND ctrl = reinterpret_cast<HWND>(lParam);
        if (ctrl != NULL)
        {
            if (IsButtonTypeNeedingClassicFallback(ctrl))
            {
                EnsureClassicButtonTheme(ctrl, forceClassicButtons);
                SetTextColor(hdc, textColor);
                SetBkColor(hdc, background);
                SetBkMode(hdc, TRANSPARENT);
                result = reinterpret_cast<LRESULT>(brush);
                return true;
            }
            else
            {
                EnsureClassicButtonTheme(ctrl, false);
                wchar_t className[16];
                if (GetClassNameW(ctrl, className, _countof(className)) != 0 && lstrcmpiW(className, L"Static") == 0)
                {
                    LONG_PTR style = GetWindowLongPtr(ctrl, GWL_STYLE);
                    const bool linkLikeStatic = (style & SS_NOTIFY) != 0;
                    if (linkLikeStatic)
                    {
                        SetTextColor(hdc, RGB(130, 180, 255));
                    }
                    else if ((style & (SS_ICON | SS_BITMAP | SS_BLACKRECT | SS_GRAYRECT | SS_WHITERECT)) == 0)
                        SetTextColor(hdc, DarkModeGetColors().readableText);
                }
                else if (GetClassNameW(ctrl, className, _countof(className)) != 0 && lstrcmpiW(className, L"SysLink") == 0)
                {
                    SetTextColor(hdc, RGB(130, 180, 255));
                }
                else
                    SetTextColor(hdc, textColor);
            }
        }
        else
        {
            SetTextColor(hdc, textColor);
        }
        SetBkColor(hdc, background);
        SetBkMode(hdc, TRANSPARENT);
        result = reinterpret_cast<LRESULT>(brush);
#if DARKMODE_TRACE_CTLFLOW
        DarkModeTraceCtlColor(message, reinterpret_cast<HWND>(lParam), hdc, textColor, background, brush, true);
#endif
        return true;
    }

    case WM_CTLCOLORBTN:
    {
        HWND ctrl = reinterpret_cast<HWND>(lParam);
        if (IsCheckboxOrRadioButtonControl(ctrl))
        {
            if (ShouldOwnerDrawChoiceButton(ctrl))
                EnsureDarkChoiceButtonSubclass(ctrl, usingNativeDark || hasCustomPalette);
            else
                EnsureDarkChoiceButtonSubclass(ctrl, false);
        }
        else if (IsButtonTypeNeedingClassicFallback(ctrl))
            EnsureClassicButtonTheme(ctrl, forceClassicButtons);
        else
            EnsureClassicButtonTheme(ctrl, false);

        setCommonColors(true);
        if (ctrl != NULL)
        {
            wchar_t className[16];
            if (GetClassNameW(ctrl, className, _countof(className)) != 0 && lstrcmpiW(className, L"Button") == 0)
            {
                const LONG_PTR style = GetWindowLongPtr(ctrl, GWL_STYLE);
                const LONG_PTR type = style & BS_TYPEMASK;
                if (type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON ||
                    type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
                    type == BS_AUTO3STATE || type == BS_3STATE)
                {
                    SetTextColor(hdc, DarkModeGetColors().readableText);
                }
            }
        }
        result = reinterpret_cast<LRESULT>(brush);
#if DARKMODE_TRACE_CTLFLOW
        DarkModeTraceCtlColor(message, ctrl, hdc, textColor, background, brush, true);
#endif
        return true;
    }

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
        setCommonColors(false);
        result = reinterpret_cast<LRESULT>(brush);
        return true;

    case WM_CTLCOLORSCROLLBAR:
    {
        const COLORREF scrollbarTrack = RGB(23, 23, 23);
        SetBkColor(hdc, scrollbarTrack);
        if (gScrollbarTrackBrush == NULL)
            gScrollbarTrackBrush = CreateSolidBrush(scrollbarTrack);
        result = reinterpret_cast<LRESULT>(gScrollbarTrackBrush != NULL ? gScrollbarTrackBrush : brush);
        return true;
    }
    }

    return false;
}

int DarkModeMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType)
{
#if USE_DARKMODELIB
    if (DarkModeShouldUseDarkColors())
    {
        static bool dmlibInitialized = false;
        if (!dmlibInitialized)
        {
            dmlib::initDarkMode();
            dmlibInitialized = true;
        }
        dmlib::setDarkModeConfigEx(static_cast<UINT>(dmlib::DarkModeType::dark));
        dmlib::setDefaultColors(true);

#ifdef _UNICODE
        return static_cast<int>(dmlib::darkMessageBoxW(hWnd, lpText, lpCaption, uType));
#else
        wchar_t textW[10000];
        wchar_t captionW[512];
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpText != NULL ? lpText : "", -1, textW, _countof(textW));
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, lpCaption != NULL ? lpCaption : "", -1, captionW, _countof(captionW));
        textW[_countof(textW) - 1] = 0;
        captionW[_countof(captionW) - 1] = 0;
        return static_cast<int>(dmlib::darkMessageBoxW(hWnd, textW, captionW, uType));
#endif
    }
#endif
    return MessageBox(hWnd, lpText, lpCaption, uType);
}

int DarkModeMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
#if USE_DARKMODELIB
    if (DarkModeShouldUseDarkColors())
    {
        static bool dmlibInitialized = false;
        if (!dmlibInitialized)
        {
            dmlib::initDarkMode();
            dmlibInitialized = true;
        }
        dmlib::setDarkModeConfigEx(static_cast<UINT>(dmlib::DarkModeType::dark));
        dmlib::setDefaultColors(true);
        return static_cast<int>(dmlib::darkMessageBoxW(
            hWnd, lpText != NULL ? lpText : L"", lpCaption != NULL ? lpCaption : L"", uType));
    }
#endif
    return MessageBoxW(hWnd, lpText, lpCaption, uType);
}

void DarkModeApplyStaticTextColors(HWND hwndParent, HWND specificCtrl)
{
    EnsureInitialized();
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::ApplyStaticTextColors(hwndParent, specificCtrl, DarkModeGetColors());
#endif
    if (hwndParent == NULL)
        return;
    const COLORREF text = DarkModeGetColors().readableText;
    auto applyOne = [&](HWND ctrl) {
        if (ctrl == NULL)
            return;
        wchar_t className[16];
        if (GetClassNameW(ctrl, className, _countof(className)) == 0 || lstrcmpiW(className, L"Static") != 0)
            return;
        LONG_PTR style = GetWindowLongPtr(ctrl, GWL_STYLE);
        if ((style & (SS_ICON | SS_BITMAP | SS_BLACKRECT | SS_GRAYRECT | SS_WHITERECT)) != 0)
            return;
        if (gSetWindowTheme != nullptr)
            gSetWindowTheme(ctrl, DarkModeIsWindowsDarkSchemeSelected() ? L"DarkMode_Explorer" : nullptr, nullptr);
        InvalidateRect(ctrl, NULL, TRUE);
    };

    if (specificCtrl != NULL)
        applyOne(specificCtrl);
    else
    {
        for (HWND child = GetWindow(hwndParent, GW_CHILD); child != NULL; child = GetWindow(child, GW_HWNDNEXT))
            applyOne(child);
    }
}

void DarkModePrepareChooseColor(CHOOSECOLOR* chooseColor, bool forceDark)
{
    if (chooseColor == NULL || (!forceDark && !DarkModeIsWindowsDarkSchemeSelected()))
        return;

#if USE_DARKMODELIB
    static bool dmlibInitialized = false;
    if (!dmlibInitialized)
    {
        dmlib::initDarkMode();
        dmlibInitialized = true;
    }
    dmlib::setDarkModeConfigEx(static_cast<UINT>(dmlib::DarkModeType::dark));
    dmlib::setDefaultColors(true);
    if ((chooseColor->Flags & CC_ENABLEHOOK) == 0)
    {
        chooseColor->Flags |= CC_ENABLEHOOK;
        chooseColor->lpfnHook = reinterpret_cast<LPCCHOOKPROC>(dmlib::HookDlgProc);
    }
#endif
}

void DarkModePrepareChooseFont(CHOOSEFONT* chooseFont, bool forceDark)
{
    if (chooseFont == NULL || (!forceDark && !DarkModeIsWindowsDarkSchemeSelected()))
        return;

#if USE_DARKMODELIB
    static bool dmlibInitialized = false;
    if (!dmlibInitialized)
    {
        dmlib::initDarkMode();
        dmlibInitialized = true;
    }
    dmlib::setDarkModeConfigEx(static_cast<UINT>(dmlib::DarkModeType::dark));
    dmlib::setDefaultColors(true);
    if ((chooseFont->Flags & CF_ENABLEHOOK) == 0)
    {
        chooseFont->Flags |= CF_ENABLEHOOK;
        chooseFont->lpfnHook = reinterpret_cast<LPCFHOOKPROC>(dmlib::HookDlgProc);
    }
    if ((chooseFont->Flags & CF_ENABLETEMPLATE) == 0)
    {
        chooseFont->Flags |= CF_ENABLETEMPLATE;
        chooseFont->hInstance = GetModuleHandle(NULL);
        chooseFont->lpTemplateName = MAKEINTRESOURCE(IDD_DARK_FONT_DIALOG);
    }
#endif
}

HBRUSH DarkModeGetPanelFrameBrush()
{
    static HBRUSH brush = NULL;
    if (brush == NULL)
        brush = CreateSolidBrush(RGB(0x38, 0x38, 0x38));
    return brush;
}

void DarkModeShutdown()
{
    if (gDialogBrushOwned && gDialogBrushHandle != NULL)
    {
        ::DeleteObject(gDialogBrushHandle);
        gDialogBrushHandle = NULL;
        gDialogBrushOwned = false;
    }

    if (gAutoSuggestWinEventHook != NULL)
    {
        UnhookWinEvent(gAutoSuggestWinEventHook);
        gAutoSuggestWinEventHook = NULL;
    }

    if (gDwmApi != nullptr)
    {
        FreeLibrary(gDwmApi);
        gDwmApi = nullptr;
    }
    if (gUxTheme != nullptr)
    {
        FreeLibrary(gUxTheme);
        gUxTheme = nullptr;
    }
}

void DarkModeApplyUpDownSubclass(HWND hWnd)
{
#if USE_DARKMODELIB
    if (hWnd != NULL && DarkModeShouldUseDarkColors())
    {
        SetWindowTheme(hWnd, L"DarkMode_Explorer", nullptr);
        dmlib::setUpDownCtrlSubclass(hWnd);
    }
#else
    (void)hWnd;
#endif
}
