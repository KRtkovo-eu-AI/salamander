// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>

struct DarkModeColors
{
    COLORREF text;
    COLORREF background;
    COLORREF readableText;
    bool usingSchemeColors;
};

// Initializes the dark mode helpers. Safe to call multiple times.
bool DarkModeInitialize();

// Releases dark mode resources. Call during application shutdown.
void DarkModeShutdown();

// Returns true if native dark mode APIs are available on this system.
bool DarkModeIsSupported();

// Enables or disables native dark mode integration for the process.
void DarkModeSetEnabled(bool enabled);

// Returns true if dark colors should currently be used.
bool DarkModeShouldUseDarkColors();
// Compatibility helper used by legacy call sites.
BOOL DarkMode_ShouldUseDark();

// Applies dark mode opt-in for the specified window (and keeps the opt-in
// flag in sync when toggling the configuration).
void DarkModeApplyWindow(HWND hwnd);

// Applies dark mode opt-in to the specified window and all of its descendants.
void DarkModeApplyTree(HWND hwnd);

// Applies or removes dark custom drawing for a native window menu bar.
void DarkModeApplyMenuBar(HWND hwnd);

// Refreshes the non-client area/title bar to match the current dark mode
// preference and system state.
void DarkModeRefreshTitleBar(HWND hwnd);

// Handles WM_SETTINGCHANGE/WM_THEMECHANGED broadcasts. Returns true if the
// message represents a color scheme change (ImmersiveColorSet).
bool DarkModeHandleSettingChange(UINT message, LPARAM lParam);

// Installs the process-wide dark scrollbar hook. Only the main executable can
// install it; calls from unloadable plugins are ignored.
void DarkModeFixScrollbars();

// Allows the limited dark scrollbar hook to affect this window subtree.
void DarkModeAllowDarkScrollbars(HWND hwnd);

// Removes a window subtree from the limited dark scrollbar hook.
void DarkModeDisallowDarkScrollbars(HWND hwnd);

// Supplies dialog foreground/background colors and brush for WM_CTLCOLOR helpers.
void DarkModeConfigureDialogColors(COLORREF textColor, COLORREF backgroundColor, HBRUSH dialogBrush);
void DarkModeSetConfiguredColors(COLORREF schemeTextColor, COLORREF schemeBackgroundColor,
                                 COLORREF fallbackTextColor, COLORREF fallbackBackgroundColor);
const DarkModeColors& DarkModeGetColors();

// Handles WM_CTLCOLOR* messages for dark mode aware parents. Returns true when
// a dark brush was supplied and the caller should stop default processing.
bool DarkModeHandleCtlColor(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result);

// Shows a dark-mode aware replacement for MessageBox when dark colors are active.
// Uses the shared darkmodelib TaskDialog path where available and falls back to
// the native MessageBox for unsupported/light configurations.
int DarkModeMessageBox(HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType);

// Wide-character variant for ANSI plugin translation units. Keeps UTF-8
// service text Unicode-safe while using the same darkmodelib implementation.
int DarkModeMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);

#if USE_DARKMODELIB
#define DARKMODE_RETURN_IF_HANDLED(handled, brushResult) \
    do                                                    \
    {                                                     \
        if (handled)                                      \
            return (brushResult);                         \
    } while (0)
#else
#define DARKMODE_RETURN_IF_HANDLED(handled, brushResult) \
    do                                                    \
    {                                                     \
    } while (0)
#endif

// Returns a shared brush used for drawing dark-mode panel frames and borders.
HBRUSH DarkModeGetPanelFrameBrush();
COLORREF DarkModeGetDialogTextColor();
COLORREF DarkModeGetDialogBackgroundColor();
COLORREF DarkModeEnsureReadableForeground(COLORREF foreground, COLORREF background);
void DarkModeUpdateListViewColors(HWND listView);
void DarkModeUpdateListViewColors(HWND listView, COLORREF textColor, COLORREF backgroundColor, bool applyHeaderColors);

// Repairs native list-view checkbox painting that can remain light on Win10
// when the application uses dark colors.
void RemoveListViewWhiteClientEdge(HWND listView);
bool ShouldCustomDrawListViewCheckboxes();
void DrawDarkModeListViewCheckboxes(HWND listView, NMLVCUSTOMDRAW* customDraw, int columnCount);
void DarkModeApplyStaticTextColors(HWND hwndParent, HWND specificCtrl);
void DarkModeUpdateTabControlOverflowButtons(HWND tabControl);
void DarkModePreserveCustomTabControl(HWND tabControl);
void DarkModePreserveCustomTreeView(HWND treeView);
void DarkModeApplyRebarSeparators(HWND rebar);
void DarkModeEnableAutoSuggestSupport(HWND edit);
void DarkModeSetAutocompleteSelectedColors(COLORREF fg, COLORREF bk);
void DarkModeApplyUpDownSubclass(HWND hWnd);
void DarkModePrepareChooseColor(CHOOSECOLOR* chooseColor, bool forceDark = false);
void DarkModePrepareChooseFont(CHOOSEFONT* chooseFont, bool forceDark = false);

// Temporarily suspends/resumes process-wide dark mode so that a child
// dialog can be created in light mode (e.g. for live dialog previews).
void DarkModeSuspendForLightCreation();
void DarkModeResumeAfterLightCreation();

// Removes dark mode from a window and its children (undo of DarkModeApplyTree).
void DarkModeRemoveTree(HWND hwnd);

// Detects whether the OS is configured for dark mode and enables it if so.
// Call early during startup, before any dialogs are shown.
void DarkModeDetectAndEnableSystemDarkMode();
