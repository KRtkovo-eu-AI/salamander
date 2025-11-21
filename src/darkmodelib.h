// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

// Minimal adapter for the optional darkmodelib runtime. The library is loaded
// dynamically at startup and used only when present. All calls gracefully fall
// back to the built-in helpers when the DLL is missing or fails to initialize.
//
// The API mirrors the responsibilities of our dark-mode pipeline:
//  * opt-in / opt-out control (DarkModeLibSetEnabled)
//  * window and tree registration (DarkModeLibApplyWindow/ApplyTree)
//  * title bar refresh (DarkModeLibRefreshTitleBar)
//  * palette discovery including accent colors (DarkModeLibQueryPalette)
//  * optional automatic control retheming (DarkModeLibThemeWindowControls)
//  * query for whether dark colors should be used (DarkModeLibShouldUseDarkColors)
//
// None of these functions are required for a functional build—the adapter is
// designed to be robust against absent or partial implementations.

using fnDarkModeLibInitialize = bool(WINAPI*)();
using fnDarkModeLibIsSupported = bool(WINAPI*)();
using fnDarkModeLibSetEnabled = bool(WINAPI*)(bool enabled);
using fnDarkModeLibApplyWindow = void(WINAPI*)(HWND hwnd);
using fnDarkModeLibApplyTree = void(WINAPI*)(HWND hwnd);
using fnDarkModeLibRefreshTitleBar = void(WINAPI*)(HWND hwnd);
using fnDarkModeLibShouldUseDarkColors = bool(WINAPI*)();
using fnDarkModeLibQueryPalette = bool(WINAPI*)(COLORREF* text, COLORREF* background, COLORREF* accent);
using fnDarkModeLibThemeWindowControls = void(WINAPI*)(HWND hwnd);

struct DarkModeLibApi
{
    fnDarkModeLibInitialize Initialize = nullptr;
    fnDarkModeLibIsSupported IsSupported = nullptr;
    fnDarkModeLibSetEnabled SetEnabled = nullptr;
    fnDarkModeLibApplyWindow ApplyWindow = nullptr;
    fnDarkModeLibApplyTree ApplyTree = nullptr;
    fnDarkModeLibRefreshTitleBar RefreshTitleBar = nullptr;
    fnDarkModeLibShouldUseDarkColors ShouldUseDarkColors = nullptr;
    fnDarkModeLibQueryPalette QueryPalette = nullptr;
    fnDarkModeLibThemeWindowControls ThemeWindowControls = nullptr;
};

