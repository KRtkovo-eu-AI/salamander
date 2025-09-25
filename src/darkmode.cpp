// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "darkmode.h"

#include <dwmapi.h>
#include <uxtheme.h>

namespace
{
    enum class PreferredAppMode
    {
        Default,
        AllowDark,
        ForceDark,
        ForceLight,
        Max,
    };

    using fnAllowDarkModeForWindow = BOOL(WINAPI*)(HWND, BOOL);
    using fnAllowDarkModeForApp = BOOL(WINAPI*)(BOOL);
    using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode);
    using fnShouldAppsUseDarkMode = BOOL(WINAPI*)();
    using fnIsDarkModeAllowedForWindow = BOOL(WINAPI*)(HWND);
    using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();
    using fnFlushMenuThemes = void(WINAPI*)();

    struct RTL_OSVERSIONINFOEXW_LOCAL
    {
        ULONG dwOSVersionInfoSize;
        ULONG dwMajorVersion;
        ULONG dwMinorVersion;
        ULONG dwBuildNumber;
        ULONG dwPlatformId;
        WCHAR szCSDVersion[128];
        USHORT wServicePackMajor;
        USHORT wServicePackMinor;
        USHORT wSuiteMask;
        UCHAR wProductType;
        UCHAR wReserved;
    };

    using fnRtlGetVersion = LONG(WINAPI*)(RTL_OSVERSIONINFOEXW_LOCAL*);

    HMODULE gUxTheme = NULL;
    fnAllowDarkModeForWindow gAllowDarkModeForWindow = nullptr;
    fnAllowDarkModeForApp gAllowDarkModeForApp = nullptr;
    fnSetPreferredAppMode gSetPreferredAppMode = nullptr;
    fnShouldAppsUseDarkMode gShouldAppsUseDarkMode = nullptr;
    fnIsDarkModeAllowedForWindow gIsDarkModeAllowedForWindow = nullptr;
    fnRefreshImmersiveColorPolicyState gRefreshImmersiveColorPolicyState = nullptr;
    fnFlushMenuThemes gFlushMenuThemes = nullptr;

    DWORD gBuildNumber = 0;
    bool gDarkModeSupported = false;
    bool gDarkModeEnabled = false;
    DarkModePreference gDarkModePreference = DarkModePreference_FollowSystem;

    DWORD GetBuildNumber()
    {
        if (gBuildNumber != 0)
            return gBuildNumber;

        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (ntdll == NULL)
            ntdll = LoadLibraryW(L"ntdll.dll");
        if (ntdll != NULL)
        {
            auto rtlGetVersion = reinterpret_cast<fnRtlGetVersion>(GetProcAddress(ntdll, "RtlGetVersion"));
            if (rtlGetVersion != nullptr)
            {
                RTL_OSVERSIONINFOEXW_LOCAL info = {};
                info.dwOSVersionInfoSize = sizeof(info);
                if (rtlGetVersion(&info) == 0)
                    gBuildNumber = info.dwBuildNumber;
            }
        }
        return gBuildNumber;
    }

    void UpdateDarkModeAppPreference(bool enable)
    {
        if (!gDarkModeSupported)
            return;

        if (gSetPreferredAppMode != nullptr)
        {
            gSetPreferredAppMode(enable ? PreferredAppMode::AllowDark : PreferredAppMode::Default);
        }
        else if (gAllowDarkModeForApp != nullptr)
        {
            gAllowDarkModeForApp(enable);
        }

        if (gFlushMenuThemes != nullptr)
            gFlushMenuThemes();

        if (gRefreshImmersiveColorPolicyState != nullptr)
            gRefreshImmersiveColorPolicyState();

        gDarkModeEnabled = enable;
    }

    void ApplyDarkModeToSingleWindow(HWND hwnd)
    {
        if (!gDarkModeSupported || hwnd == NULL)
            return;

        BOOL allow = gDarkModeEnabled ? TRUE : FALSE;
        if (gAllowDarkModeForWindow != nullptr)
            gAllowDarkModeForWindow(hwnd, allow);

        if (gIsDarkModeAllowedForWindow != nullptr && !gIsDarkModeAllowedForWindow(hwnd))
            return;

        HWND topLevel = GetAncestor(hwnd, GA_ROOT);
        if (topLevel == hwnd)
        {
            int attribute = GetDarkModeAttributeId();
            DwmSetWindowAttribute(hwnd, attribute, &allow, sizeof(allow));
        }

        if (gDarkModeEnabled)
            SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
        else
            SetWindowTheme(hwnd, L"Explorer", nullptr);

        SendMessage(hwnd, WM_THEMECHANGED, 0, 0);
        RedrawWindow(hwnd, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
    }

    BOOL CALLBACK ApplyDarkModeEnumProc(HWND hwnd, LPARAM)
    {
        ApplyDarkModeForWindow(hwnd);
        return TRUE;
    }

    bool ShouldUseDarkModeFromSystem()
    {
        return gShouldAppsUseDarkMode != nullptr && gShouldAppsUseDarkMode() != FALSE;
    }

    void ApplyDarkModeToAllWindows()
    {
        EnumThreadWindows(GetCurrentThreadId(), ApplyDarkModeEnumProc, 0);
    }

    bool CalculateShouldUseDarkMode()
    {
        switch (gDarkModePreference)
        {
        case DarkModePreference_ForceLight:
            return false;
        case DarkModePreference_ForceDark:
            return true;
        case DarkModePreference_FollowSystem:
        default:
            return ShouldUseDarkModeFromSystem();
        }
    }

    int GetDarkModeAttributeId()
    {
        return GetBuildNumber() >= 18362 ? 20 : 19;
    }
}

void InitializeDarkModeSupport()
{
    if (gDarkModeSupported)
        return;

    DWORD build = GetBuildNumber();
    if (build < 17763)
        return;

    gUxTheme = LoadLibraryW(L"uxtheme.dll");
    if (gUxTheme == NULL)
        return;

    gRefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(
        GetProcAddress(gUxTheme, MAKEINTRESOURCEA(104)));
    gShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(
        GetProcAddress(gUxTheme, MAKEINTRESOURCEA(132)));
    gAllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
        GetProcAddress(gUxTheme, MAKEINTRESOURCEA(133)));

    if (build >= 18362)
    {
        gSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
            GetProcAddress(gUxTheme, MAKEINTRESOURCEA(135)));
    }
    else
    {
        gAllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(
            GetProcAddress(gUxTheme, MAKEINTRESOURCEA(135)));
    }

    gFlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(gUxTheme, MAKEINTRESOURCEA(136)));
    gIsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(
        GetProcAddress(gUxTheme, MAKEINTRESOURCEA(137)));

    if (gAllowDarkModeForWindow == nullptr || gShouldAppsUseDarkMode == nullptr ||
        (gSetPreferredAppMode == nullptr && gAllowDarkModeForApp == nullptr))
    {
        return;
    }

    gDarkModeSupported = true;
    bool enable = CalculateShouldUseDarkMode();
    UpdateDarkModeAppPreference(enable);
    ApplyDarkModeToAllWindows();
}

void ApplyDarkModeForWindow(HWND hwnd)
{
    ApplyDarkModeToSingleWindow(hwnd);
    EnumChildWindows(hwnd, ApplyDarkModeEnumProc, 0);
}

bool HandleDarkModeSettingChange(LPARAM lParam)
{
    if (!gDarkModeSupported || lParam == 0)
        return false;

#ifdef _UNICODE
    const wchar_t* setting = reinterpret_cast<const wchar_t*>(lParam);
    if (_wcsicmp(setting, L"ImmersiveColorSet") != 0 && _wcsicmp(setting, L"WindowsThemeElement") != 0)
        return false;
#else
    const char* setting = reinterpret_cast<const char*>(lParam);
    if (_stricmp(setting, "ImmersiveColorSet") != 0 && _stricmp(setting, "WindowsThemeElement") != 0)
        return false;
#endif

    if (gDarkModePreference != DarkModePreference_FollowSystem)
        return false;

    bool enable = ShouldUseDarkModeFromSystem();
    if (enable == gDarkModeEnabled)
        return false;

    UpdateDarkModeAppPreference(enable);
    RefreshDarkModeForProcess();
    return true;
}

bool IsDarkModeSupported()
{
    return gDarkModeSupported;
}

bool IsDarkModeEnabled()
{
    return gDarkModeSupported && gDarkModeEnabled;
}

void RefreshDarkModeForProcess()
{
    if (!gDarkModeSupported)
        return;

    ApplyDarkModeToAllWindows();
}

void SetDarkModePreference(DarkModePreference preference)
{
    gDarkModePreference = preference;

    if (!gDarkModeSupported)
        return;

    bool enable = CalculateShouldUseDarkMode();
    UpdateDarkModeAppPreference(enable);
    ApplyDarkModeToAllWindows();
}

DarkModePreference GetDarkModePreference()
{
    return gDarkModePreference;
}
