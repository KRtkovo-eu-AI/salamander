// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "../shared/webviewviewer/native_viewer.h"
#include "../../darkmode.h"

#include <string>

extern CSalamanderGUIAbstract* SalamanderGUI;

namespace
{
std::wstring PathToWide(const char* path)
{
    if (path == nullptr)
        return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (count <= 0)
    {
        codePage = CP_ACP;
        flags = 0;
        count = MultiByteToWideChar(codePage, flags, path, -1, nullptr, 0);
    }
    if (count <= 0)
        return {};
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(codePage, flags, path, -1, result.data(), count);
    result.resize(static_cast<size_t>(count - 1));
    return result;
}

NativeViewerTheme ReadTheme()
{
    NativeViewerTheme theme = {false, GetSysColor(COLOR_WINDOWTEXT), GetSysColor(COLOR_WINDOW),
                               GetSysColor(COLOR_HIGHLIGHT), GetSysColor(COLOR_HIGHLIGHTTEXT),
                               GetSysColor(COLOR_HIGHLIGHT)};
    BOOL dark = FALSE;
    int type = 0;
    if (SalamanderGeneral != nullptr &&
        SalamanderGeneral->GetConfigParameter(SALCFG_USEWINDOWSDARKMODE, &dark, sizeof(dark), &type))
    {
        theme.dark = dark != FALSE;
        theme.foreground = SalamanderGeneral->GetCurrentColor(SALCOL_VIEWER_FG_NORMAL);
        theme.background = SalamanderGeneral->GetCurrentColor(SALCOL_VIEWER_BK_NORMAL);
        theme.accent = SalamanderGeneral->GetCurrentColor(SALCOL_HOT_PANEL);
        theme.selectedForeground = SalamanderGeneral->GetCurrentColor(SALCOL_VIEWER_FG_SELECTED);
        theme.selectedBackground = SalamanderGeneral->GetCurrentColor(SALCOL_VIEWER_BK_SELECTED);
    }
    return theme;
}

LOGFONT ReadMenuFont()
{
    LOGFONT font = {};
    if (SalamanderGeneral == nullptr ||
        !SalamanderGeneral->GetConfigParameter(SALCFG_MENUFONT, &font, sizeof(font), nullptr))
    {
        GetObjectW(GetStockObject(DEFAULT_GUI_FONT), sizeof(font), &font);
    }
    return font;
}

NativeViewerStrings ReadStrings()
{
    return {SalamanderGeneral->LoadStrW(HLanguage, IDS_PLUGINNAME),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_FILE),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_VIEW),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_CLOSE),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_REFRESH),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_ZOOM_IN),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_ZOOM_OUT),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_ZOOM_RESET),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_LINE_NUMBERS),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_WRAP_LINES),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_SHOW_WHITESPACE),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_LOADING),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_READY),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_WEBVIEW_FAILED),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_OPEN_FAILED),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_SYNTAX_HIGHLIGHTER),
            SalamanderGeneral->LoadStrW(HLanguage, IDS_VIEWER_AUTOMATIC)};
}

LOGFONT ReadViewerFont()
{
    LOGFONT font = {};
    if (SalamanderGeneral == nullptr ||
        !SalamanderGeneral->GetConfigParameter(SALCFG_VIEWERFONT, &font, sizeof(font), nullptr))
    {
        font.lfHeight = -13;
        font.lfWeight = FW_NORMAL;
        strcpy_s(font.lfFaceName, "Consolas");
    }
    return font;
}
}

bool ManagedBridge_EnsureInitialized(HWND)
{
    return NativeViewer_EnsureInitialized();
}

void ManagedBridge_Shutdown()
{
    NativeViewer_Shutdown();
}

bool ManagedBridge_RequestShutdown(HWND, bool forceClose)
{
    return NativeViewer_RequestShutdown(forceClose);
}

bool ManagedBridge_ViewTextFile(HWND parent, const char* filePath, const RECT& placement,
                                UINT showCmd, BOOL alwaysOnTop, HANDLE fileLock, bool)
{
    std::wstring path = PathToWide(filePath);
    NativeViewerRequest request = {DLLInstance, parent, path.c_str(), placement, showCmd,
                                   alwaysOnTop != FALSE, fileLock, NativeViewerKind::PrismText,
                                   ReadTheme(), ReadStrings(), ReadMenuFont(), SalamanderGUI,
                                   ReadViewerFont()};
    return NativeViewer_Show(request);
}

extern "C" __declspec(dllexport) UINT32 __stdcall TextViewer_GetCurrentColor(int color)
{
    return SalamanderGeneral != nullptr ? SalamanderGeneral->GetCurrentColor(color) : 0;
}

extern "C" __declspec(dllexport) void __stdcall TextViewer_SetDarkModeState(BOOL enabled)
{
    DarkModeSetEnabled(enabled != FALSE);
}

extern "C" __declspec(dllexport) void __stdcall TextViewer_ApplyDarkModeTree(HWND window)
{
    DarkModeAllowDarkScrollbars(window);
    DarkModeApplyTree(window);
}
