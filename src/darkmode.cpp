// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "darkmode.h"

#include <delayimp.h>
#include <uxtheme.h>

#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0x00000800
#endif

namespace
{
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
bool gScrollbarsHooked = false;

static COLORREF gDialogTextColor = GetSysColor(COLOR_BTNTEXT);
static COLORREF gDialogBackgroundColor = GetSysColor(COLOR_BTNFACE);
static HBRUSH gDialogBrushHandle = NULL;

const wchar_t* kDarkModeThemeProp = L"Salamander.DarkMode.Theme";

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
        L"Button",
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
    };

    static const wchar_t* const cfdClasses[] = {
        L"Edit",
        L"ComboBox",
        L"RichEdit20W",
        L"RICHEDIT50W",
    };

    const bool wantDark = ShouldUseDarkColorsInternal();
    const bool hadTheme = GetPropW(hwnd, kDarkModeThemeProp) != NULL;
    const wchar_t* theme = nullptr;

    if (wantDark)
    {
        if (MatchesAnyClass(className, darkExplorerClasses, _countof(darkExplorerClasses)))
            theme = L"DarkMode_Explorer";
        else if (MatchesAnyClass(className, explorerClasses, _countof(explorerClasses)))
            theme = L"Explorer";
        else if (MatchesAnyClass(className, cfdClasses, _countof(cfdClasses)))
            theme = L"CFD";
    }

    if (theme != nullptr)
    {
        if (gSetWindowTheme)
            gSetWindowTheme(hwnd, theme, nullptr);
        SetPropW(hwnd, kDarkModeThemeProp, reinterpret_cast<HANDLE>(1));
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }
    else if (hadTheme)
    {
        RemovePropW(hwnd, kDarkModeThemeProp);
        if (gSetWindowTheme)
            gSetWindowTheme(hwnd, nullptr, nullptr);
        SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
    }
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

    if (gBuildNumber < 17763)
    {
        gSupported = false;
        return;
    }

    gUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
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
    gSetWindowTheme = reinterpret_cast<fnSetWindowTheme>(GetProcAddress(gUxTheme, "SetWindowTheme"));

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

    bool newEnabled = enabled && !IsHighContrast();
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
    return ShouldUseDarkColorsInternal();
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
    EnsureInitialized();
    if (!gSupported || hwnd == NULL)
        return;

    DarkModeApplyWindow(hwnd);
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

    if (message != WM_SETTINGCHANGE)
        return false;

    bool isColor = false;
    if (lParam != 0)
    {
        if (CompareStringOrdinal(reinterpret_cast<LPCWSTR>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
        {
            RefreshColorPolicy();
            isColor = true;
        }
        else if (CompareStringOrdinal(reinterpret_cast<LPCWSTR>(lParam), -1, L"WindowsThemeElement", -1, TRUE) == CSTR_EQUAL)
        {
            RefreshColorPolicy();
            isColor = true;
        }
    }
    else
    {
        RefreshColorPolicy();
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
}

bool DarkModeHandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    EnsureInitialized();
    if (!gSupported || !ShouldUseDarkColorsInternal())
        return false;

    HBRUSH brush = gDialogBrushHandle != NULL ? gDialogBrushHandle : GetSysColorBrush(COLOR_BTNFACE);
    HDC hdc = reinterpret_cast<HDC>(wParam);
    if (hdc == NULL)
        return false;

    const COLORREF textColor = gDialogTextColor;
    const COLORREF background = gDialogBackgroundColor;

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
            LONG_PTR style = GetWindowLongPtr(ctrl, GWL_STYLE);
            if ((style & (SS_ICON | SS_BITMAP | SS_BLACKRECT | SS_GRAYRECT | SS_WHITERECT)) == 0)
                SetTextColor(hdc, textColor);
        }
        else
        {
            SetTextColor(hdc, textColor);
        }
        SetBkColor(hdc, background);
        SetBkMode(hdc, TRANSPARENT);
        result = reinterpret_cast<LRESULT>(brush);
        return true;
    }

    case WM_CTLCOLORBTN:
        setCommonColors(true);
        result = reinterpret_cast<LRESULT>(brush);
        return true;

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

HBRUSH DarkModeGetPanelFrameBrush()
{
    static HBRUSH brush = NULL;
    if (brush == NULL)
        brush = HANDLES(CreateSolidBrush(RGB(0x38, 0x38, 0x38)));
    return brush;
}

