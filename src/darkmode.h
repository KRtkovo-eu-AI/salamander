// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

// Initializes the dark mode helpers. Safe to call multiple times.
bool DarkModeInitialize();

// Returns true if native dark mode APIs are available on this system.
bool DarkModeIsSupported();

// Enables or disables native dark mode integration for the process.
void DarkModeSetEnabled(bool enabled);

// Returns true if dark colors should currently be used.
bool DarkModeShouldUseDarkColors();

// Applies dark mode opt-in for the specified window (and keeps the opt-in
// flag in sync when toggling the configuration).
void DarkModeApplyWindow(HWND hwnd);

// Applies dark mode opt-in to the specified window and all of its descendants.
void DarkModeApplyTree(HWND hwnd);

// Refreshes the non-client area/title bar to match the current dark mode
// preference and system state.
void DarkModeRefreshTitleBar(HWND hwnd);

// Handles WM_SETTINGCHANGE/WM_THEMECHANGED broadcasts. Returns true if the
// message represents a color scheme change (ImmersiveColorSet).
bool DarkModeHandleSettingChange(UINT message, LPARAM lParam);

// Installs the dark scrollbar hook (no-op on unsupported systems).
void DarkModeFixScrollbars();

// Supplies dialog foreground/background colors and brush for WM_CTLCOLOR helpers.
void DarkModeConfigureDialogColors(COLORREF textColor, COLORREF backgroundColor, HBRUSH dialogBrush);

// Handles WM_CTLCOLOR* messages for dark mode aware parents. Returns true when
// a dark brush was supplied and the caller should stop default processing.
bool DarkModeHandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result);

// Returns a shared brush used for drawing dark-mode panel frames and borders.
HBRUSH DarkModeGetPanelFrameBrush();
COLORREF DarkModeGetDialogTextColor();
COLORREF DarkModeGetDialogBackgroundColor();
COLORREF DarkModeEnsureReadableForeground(COLORREF foreground, COLORREF background);

