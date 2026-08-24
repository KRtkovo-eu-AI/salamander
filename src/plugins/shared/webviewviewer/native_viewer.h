// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

class CSalamanderGUIAbstract;
class CSalamanderGeneralAbstract;

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
    const wchar_t* colors;
    const wchar_t* visualStudio;
    const wchar_t* defaultPrism;
    const wchar_t* customPalette;
    const wchar_t* editCustom;
    const wchar_t* editCustomTitle;
    const wchar_t* save;
    const wchar_t* cancel;
    const wchar_t* light;
    const wchar_t* dark;
    const wchar_t* tokenComment;
    const wchar_t* tokenPunctuation;
    const wchar_t* tokenKeyword;
    const wchar_t* tokenControlKeyword;
    const wchar_t* tokenClassName;
    const wchar_t* tokenFunction;
    const wchar_t* tokenString;
    const wchar_t* tokenNumber;
    const wchar_t* tokenBoolean;
    const wchar_t* tokenVariable;
    const wchar_t* tokenNamespace;
    const wchar_t* tokenRegex;
    const wchar_t* saveFailed;
    const wchar_t* uiUnavailable;
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
    CSalamanderGeneralAbstract* general;
};

bool NativeViewer_EnsureInitialized();
bool NativeViewer_Show(const NativeViewerRequest& request);
bool NativeViewer_RequestShutdown(bool forceClose);
void NativeViewer_Shutdown();
