// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

struct PluginDarkModeColors
{
    COLORREF text;
    COLORREF background;
    COLORREF readableText;
};

void PluginDarkMode_SetHostPolicyAvailable(BOOL available, BOOL useWindowsDarkScheme);
void PluginDarkMode_SetHostColors(COLORREF text, COLORREF background);
void PluginDarkMode_SetHostResolvedColors(COLORREF text, COLORREF background, COLORREF readableText);
BOOL PluginDarkMode_ShouldUseDark();
PluginDarkModeColors PluginDarkMode_GetColors();
void PluginDarkMode_ApplyTitleBar(HWND hwnd);
void PluginDarkMode_ApplyMenuBar(HWND hwnd);
void PluginDarkMode_ApplyStatusBar(HWND hwnd);
void PluginDarkMode_ApplyListTreeThemeRecursive(HWND hwnd);
HBRUSH PluginDarkMode_GetDialogCtlColorBrush(HDC dc, UINT message);
BOOL PluginDarkMode_HandleThemeMessage(HWND hwnd, UINT message, LPARAM lParam);
BOOL PluginDarkMode_HandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result);
