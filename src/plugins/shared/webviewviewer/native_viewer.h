// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

class CSalamanderGUIAbstract;

enum class NativeViewerKind
{
    PrismText,
    RenderDocument,
};

struct NativeViewerTheme
{
    bool dark;
    COLORREF foreground;
    COLORREF background;
    COLORREF accent;
    COLORREF selectedForeground;
    COLORREF selectedBackground;
};

struct NativeViewerStrings
{
    const wchar_t* pluginName;
    const wchar_t* fileMenu;
    const wchar_t* viewMenu;
    const wchar_t* close;
    const wchar_t* refresh;
    const wchar_t* zoomIn;
    const wchar_t* zoomOut;
    const wchar_t* zoomReset;
    const wchar_t* lineNumbers;
    const wchar_t* wrapLines;
    const wchar_t* showWhitespace;
    const wchar_t* loading;
    const wchar_t* ready;
    const wchar_t* initializationFailed;
    const wchar_t* openFailed;
    const wchar_t* syntaxHighlighter;
    const wchar_t* automatic;
};

struct NativeViewerRequest
{
    HINSTANCE module;
    HWND owner;
    const wchar_t* filePath;
    RECT placement;
    UINT showCommand;
    bool alwaysOnTop;
    HANDLE closeEvent;
    NativeViewerKind kind;
    NativeViewerTheme theme;
    NativeViewerStrings strings;
    LOGFONT menuFont;
    CSalamanderGUIAbstract* gui;
    LOGFONT viewerFont;
};

bool NativeViewer_EnsureInitialized();
bool NativeViewer_Show(const NativeViewerRequest& request);
bool NativeViewer_RequestShutdown(bool forceClose);
void NativeViewer_Shutdown();
