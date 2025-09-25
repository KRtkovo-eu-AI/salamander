#pragma once

enum DarkModePreference
{
    DarkModePreference_FollowSystem = 0,
    DarkModePreference_ForceLight = 1,
    DarkModePreference_ForceDark = 2,
};

void InitializeDarkModeSupport();
void ApplyDarkModeForWindow(HWND hwnd);
bool HandleDarkModeSettingChange(LPARAM lParam);
bool IsDarkModeSupported();
bool IsDarkModeEnabled();
void RefreshDarkModeForProcess();
void SetDarkModePreference(DarkModePreference preference);
DarkModePreference GetDarkModePreference();
