// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "darkmode_backend_darkmodelib.h"
#include "darkmode.h"

#include <delayimp.h>
#include <uxtheme.h>
#include <commctrl.h>

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
fnSetWindowCompositionAttribute gSetWindowCompositionAttribute = nullptr;
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

static COLORREF gDialogTextColor = GetSysColor(COLOR_BTNTEXT);
static COLORREF gDialogBackgroundColor = GetSysColor(COLOR_BTNFACE);
static HBRUSH gDialogBrushHandle = NULL;
static DarkModeColors gColors = {GetSysColor(COLOR_BTNTEXT), GetSysColor(COLOR_BTNFACE), GetSysColor(COLOR_BTNTEXT), false};
static bool gPropagatingThemeChange = false;

const wchar_t* kDarkModeThemeProp = L"Salamander.DarkMode.Theme";
const wchar_t* kDarkModeClassicButtonProp = L"Salamander.DarkMode.ClassicButton";

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
    // Radio captions need the fallback on all supported builds.  Checkbox text is
    // already painted correctly by the themed control on Windows 11, but Windows
    // 10 keeps drawing it with the light-theme text color in dark dialogs, so use
    // the same owner-draw fallback there only.
    return IsRadioButtonControl(hwnd) || (IsCheckboxControl(hwnd) && gBuildNumber < 22000);
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
    const int glyphSize = max(13, GetSystemMetrics(SM_CXMENUCHECK));
    RECT glyph = rc;
    glyph.left += 1;
    glyph.right = glyph.left + glyphSize;
    glyph.top = rc.top + max(0, (rc.bottom - rc.top - glyphSize) / 2);
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
    COLORREF textColor = enabled ? colors.readableText : RGB(0xA0, 0xA0, 0xA0);
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

    if (enableDark)
    {
        EnsureClassicButtonTheme(hwnd, true);
        SetWindowSubclass(hwnd, DarkChoiceButtonSubclass, kDarkModeChoiceButtonSubclassId, 0);
        InvalidateRect(hwnd, NULL, TRUE);
    }
    else
    {
        RemoveWindowSubclass(hwnd, DarkChoiceButtonSubclass, kDarkModeChoiceButtonSubclassId);
        EnsureClassicButtonTheme(hwnd, false);
        InvalidateRect(hwnd, NULL, TRUE);
    }
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

        char text[256];
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
            edge.left = edge.right - max(1, borders[2]);
            FillRectWithColor(hdc, edge, RGB(0x4A, 0x4A, 0x4A));
        }

        part.left += borders[2] + 2;
        part.right -= borders[0] + 2;

        const LRESULT textLen = SendMessage(hwnd, SB_GETTEXTLENGTH, i, 0);
        const DWORD flags = HIWORD(textLen);
        const int len = min(static_cast<int>(LOWORD(textLen)), 1023);
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

bool IsWindowsDarkSchemeSelected()
{
    return gWindowsDarkSchemeSelected;
}

bool ShouldUseDarkColorsForSurfaces()
{
    if (ShouldUseDarkColorsInternal())
        return true;
    if (!IsWindowsDarkSchemeSelected())
        return false;
    return ComputeLuminance(gDialogBackgroundColor) < 128;
}

bool ShouldApplyNativeDarkEnhancements()
{
    return IsWindowsDarkSchemeSelected() && !IsHighContrast();
}

BOOL CALLBACK ApplyTreeCallback(HWND hwnd, LPARAM)
{
    DarkModeApplyTree(hwnd);
    return TRUE;
}

void HookDarkScrollbars()
{
    if (gScrollbarsHooked || !gSupported)
        return;

    HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hComctl)
        return;

    auto thunk = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", 49); // OpenNcThemeData
    if (!thunk)
        return;

    DWORD oldProtect;
    if (!VirtualProtect(thunk, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
        return;

    auto original = reinterpret_cast<fnOpenNcThemeData>(thunk->u1.Function);
    if (!original)
    {
        VirtualProtect(thunk, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
        return;
    }

    gOpenNcThemeData = original;
    auto replacement = [](HWND hWnd, LPCWSTR classList) -> HTHEME {
        if (classList != nullptr && wcscmp(classList, L"ScrollBar") == 0)
        {
            hWnd = nullptr;
            classList = L"Explorer::ScrollBar";
        }
        return gOpenNcThemeData ? gOpenNcThemeData(hWnd, classList) : nullptr;
    };

    thunk->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(replacement));
    VirtualProtect(thunk, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
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
        L"ScrollBar",
        L"msctls_scrollbar32",
    };

    static const wchar_t* const darkExplorerClasses[] = {
        L"SysListView32",
        L"SysTreeView32",
        L"SysHeader32",
        L"SysTabControl32",
        L"ComboBoxEx32",
        L"ReBarWindow32",
        L"ToolbarWindow32",
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
                ListView_SetTextBkColor(hwnd, bg);
                ListView_SetBkColor(hwnd, bg);
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
                EnsureClassicButtonTheme(hwnd, wantDark);
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
                else if (gSetWindowTheme != nullptr)
                    gSetWindowTheme(hwnd, wantDark ? L"DarkMode_Explorer" : nullptr, nullptr);
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
#if !USE_DARKMODELIB
        else if (wcscmp(className, L"SysHeader32") == 0)
        {
            EnsureDarkHeaderSubclass(hwnd, ShouldUseDarkColorsForSurfaces());
        }
        else if (wcscmp(className, L"msctls_statusbar32") == 0)
        {
            EnsureDarkStatusBarSubclass(hwnd, ShouldUseDarkColorsForSurfaces());
        }
#endif
        else if (wcscmp(className, L"Static") == 0)
        {
            const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
            const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
            if ((exStyle & WS_EX_STATICEDGE) == WS_EX_STATICEDGE || (style & SS_ETCHEDFRAME) == SS_ETCHEDFRAME)
                InvalidateRect(hwnd, NULL, TRUE);
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

    gSupported = gAllowDarkModeForWindow != nullptr &&
                 (gAllowDarkModeForApp != nullptr || gSetPreferredAppMode != nullptr) &&
                 gShouldAppsUseDarkMode != nullptr;

    if (!gSupported)
        return;
}

} // namespace

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

    if (gEnabled)
        HookDarkScrollbars();

    RefreshColorPolicy();

    if (gFlushMenuThemes)
        gFlushMenuThemes();
}

bool DarkModeShouldUseDarkColors()
{
    EnsureInitialized();

    if (ShouldUseDarkColorsInternal())
        return true;

    if (!IsWindowsDarkSchemeSelected())
        return false;

    // Fall back to the configured dialog palette only for the explicit Windows
    // Dark Mode scheme when native dark mode isn't available (for example on
    // older Windows builds).  Light/custom schemes must stay native light UI and
    // must not re-enter dark CTLCOLOR/subclass paths just because their panel
    // palette happens to be dark.
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

void DarkModeApplyTree(HWND hwnd)
{
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::ApplyTree(hwnd);
#endif
    EnsureInitialized();
    if (!gSupported || hwnd == NULL)
        return;

    DarkModeApplyWindow(hwnd);
    ApplyListTreeThemeRecursive(hwnd, IsWindowsDarkSchemeSelected());
    EnumChildWindows(hwnd, ApplyTreeCallback, 0);
}

void DarkModeRefreshTitleBar(HWND hwnd)
{
    EnsureInitialized();
    if (!gSupported || hwnd == NULL)
        return;

    BOOL useDark = FALSE;
    if (gIsDarkModeAllowedForWindow && gIsDarkModeAllowedForWindow(hwnd) && ShouldUseDarkColorsInternal())
        useDark = TRUE;

    if (gBuildNumber < 18362)
    {
        SetPropW(hwnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(useDark)));
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
        if (CompareStringOrdinal(reinterpret_cast<LPCWSTR>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
        {
            isColor = true;
            shouldRefresh = true;
        }
        else if (CompareStringOrdinal(reinterpret_cast<LPCWSTR>(lParam), -1, L"WindowsThemeElement", -1, TRUE) == CSTR_EQUAL)
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
    EnsureInitialized();
    if (!gSupported)
        return;

    HookDarkScrollbars();
}

void DarkModeConfigureDialogColors(COLORREF textColor, COLORREF backgroundColor, HBRUSH dialogBrush)
{
    gDialogTextColor = textColor;
    gDialogBackgroundColor = backgroundColor;
    gDialogBrushHandle = dialogBrush;
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

COLORREF DarkModeEnsureReadableForeground(COLORREF foreground, COLORREF background)
{
    return ResolveReadableForeground(foreground, background);
}

void DarkModeUpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors)
{
#if USE_DARKMODELIB
    DarkModeBackendDarkModelib::UpdateListViewColors(listView, textColor, backgroundColor, applyHeaderColors);
#endif
    EnsureInitialized();

    if (listView == NULL)
        return;

    const COLORREF resolvedText = DarkModeEnsureReadableForeground(textColor, backgroundColor);
    const COLORREF resolvedBackground = backgroundColor;

    DarkModeApplyWindow(listView);

    ListView_SetTextColor(listView, resolvedText);
    ListView_SetTextBkColor(listView, resolvedBackground);
    ListView_SetBkColor(listView, resolvedBackground);

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
    const bool hasCustomPalette = IsWindowsDarkSchemeSelected() &&
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
        SetBkColor(hdc, background);
        result = reinterpret_cast<LRESULT>(brush);
        return true;
    }

    return false;
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
            gSetWindowTheme(ctrl, IsWindowsDarkSchemeSelected() ? L"DarkMode_Explorer" : nullptr, nullptr);
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

HBRUSH DarkModeGetPanelFrameBrush()
{
    static HBRUSH brush = NULL;
    if (brush == NULL)
        brush = HANDLES(CreateSolidBrush(RGB(0x38, 0x38, 0x38)));
    return brush;
}
