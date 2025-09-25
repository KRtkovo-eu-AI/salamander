#pragma once

void InitializeDarkModeSupport();
void ApplyDarkModeForWindow(HWND hwnd);
bool HandleDarkModeSettingChange(LPARAM lParam);
bool IsDarkModeSupported();
bool IsDarkModeEnabled();
void RefreshDarkModeForProcess();
