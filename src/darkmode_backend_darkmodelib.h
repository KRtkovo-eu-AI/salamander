// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

struct DarkModeColors;

namespace DarkModeBackendDarkModelib
{
// Returns true when backend is compiled-in and initialized.
bool IsAvailable();

// Backend mirrors of selected darkmode APIs used by the host implementation.
void ApplyTree(HWND hwnd);
void ApplyMenuBar(HWND hwnd, bool enableDark);
void CleanupWindow(HWND hwnd);
void ApplyCheckboxOrRadioButton(HWND hwnd, bool enableDark);
void ApplyStatusBar(HWND hwnd, bool enableDark);
void ApplyProgressBar(HWND hwnd, bool enableDark);
bool HandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result,
                    const DarkModeColors& colors, HBRUSH dialogBrush);
void ApplyStaticTextColors(HWND hwndParent, HWND specificCtrl, const DarkModeColors& colors);
void UpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors);
void UpdateTabControlOverflowButtons(HWND tabControl, bool enableDark);
void MarkCustomTabControl(HWND tabControl);
void RestoreCustomTabControls(HWND hwndParent);
} // namespace DarkModeBackendDarkModelib
