// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <commctrl.h>

// WinLib is also compiled by plugins which still support older Windows SDK
// headers. Keep the PMv2 helpers dynamically linked and avoid making the
// public WinLib ABI depend on newer SDK types.
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif

#ifndef WM_DPICHANGED_BEFOREPARENT
#define WM_DPICHANGED_BEFOREPARENT 0x02E2
#endif

#ifndef WM_DPICHANGED_AFTERPARENT
#define WM_DPICHANGED_AFTERPARENT 0x02E3
#endif

#ifndef WM_GETDPISCALEDSIZE
#define WM_GETDPISCALEDSIZE 0x02E4
#endif

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

typedef HANDLE(WINAPI* CWinLibSetThreadDpiAwarenessContext)(HANDLE dpiContext);
typedef UINT(WINAPI* CWinLibGetDpiForWindow)(HWND hwnd);
typedef int(WINAPI* CWinLibGetSystemMetricsForDpi)(int index, UINT dpi);
typedef BOOL(WINAPI* CWinLibSystemParametersInfoForDpi)(UINT action, UINT param, PVOID data, UINT flags, UINT dpi);

inline CWinLibSetThreadDpiAwarenessContext WinLibDPIGetSetThreadContext()
{
    static CWinLibSetThreadDpiAwarenessContext setThreadDpiAwarenessContext = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            setThreadDpiAwarenessContext = (CWinLibSetThreadDpiAwarenessContext)GetProcAddress(user32, "SetThreadDpiAwarenessContext");
        loaded = TRUE;
    }
    return setThreadDpiAwarenessContext;
}

// Windows captures a window's DPI awareness from the creating thread. Plugin
// callbacks and CLR-hosted code can temporarily change that thread context, so
// every WinLib top-level window is created under PMv2 explicitly and the
// caller's original context is restored immediately afterwards.
class CWinLibDPIContext
{
public:
    CWinLibDPIContext()
    {
        SetThreadContext = WinLibDPIGetSetThreadContext();
        OldContext = SetThreadContext != NULL
                         ? SetThreadContext((HANDLE)-4 /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */)
                         : NULL;
    }

    ~CWinLibDPIContext()
    {
        if (SetThreadContext != NULL && OldContext != NULL)
            SetThreadContext(OldContext);
    }

private:
    CWinLibDPIContext(const CWinLibDPIContext&);
    CWinLibDPIContext& operator=(const CWinLibDPIContext&);

    CWinLibSetThreadDpiAwarenessContext SetThreadContext;
    HANDLE OldContext;
};

inline UINT WinLibDPIGetWindowDPI(HWND hwnd)
{
    static CWinLibGetDpiForWindow getDpiForWindow = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            getDpiForWindow = (CWinLibGetDpiForWindow)GetProcAddress(user32, "GetDpiForWindow");
        loaded = TRUE;
    }

    if (getDpiForWindow != NULL && hwnd != NULL)
    {
        UINT dpi = getDpiForWindow(hwnd);
        if (dpi != 0)
            return dpi;
    }

    HDC dc = GetDC(hwnd);
    if (dc != NULL)
    {
        int dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(hwnd, dc);
        if (dpi > 0)
            return (UINT)dpi;
    }
    return USER_DEFAULT_SCREEN_DPI;
}

inline UINT WinLibDPIGetSystemDPI()
{
    HDC dc = GetDC(NULL);
    if (dc != NULL)
    {
        int dpi = GetDeviceCaps(dc, LOGPIXELSX);
        ReleaseDC(NULL, dc);
        if (dpi > 0)
            return (UINT)dpi;
    }
    return USER_DEFAULT_SCREEN_DPI;
}

inline int WinLibDPIToLogical(HWND hwnd, int pixels)
{
    return MulDiv(pixels, USER_DEFAULT_SCREEN_DPI, (int)WinLibDPIGetWindowDPI(hwnd));
}

inline int WinLibDPIFromLogical(HWND hwnd, int logical)
{
    return MulDiv(logical, (int)WinLibDPIGetWindowDPI(hwnd), USER_DEFAULT_SCREEN_DPI);
}

inline BOOL CALLBACK WinLibDPIApplyDialogFontChild(HWND child, LPARAM param)
{
    SendMessage(child, WM_SETFONT, (WPARAM)param, MAKELPARAM(FALSE, 0));
    return TRUE;
}

inline LRESULT CALLBACK WinLibDPIDialogFontSubclassProc(HWND hwnd, UINT message,
                                                        WPARAM wParam, LPARAM lParam,
                                                        UINT_PTR subclassID, DWORD_PTR data)
{
    if (message == WM_NCDESTROY)
    {
        RemovePropW(hwnd, L"Salamander.WinLib.DialogFontApplied");
        RemoveWindowSubclass(hwnd, WinLibDPIDialogFontSubclassProc, subclassID);
        DeleteObject((HFONT)data);
    }
    return DefSubclassProc(hwnd, message, wParam, lParam);
}

// Font-only fallback for windows not created through WinLib's template-aware paths.
// Resizing initialized controls here would double-scale dialogs whose runtime layout
// has already captured the resource geometry.
inline void WinLibDPIApplyDialogFont(HWND dialog, HFONT font)
{
    if (dialog == NULL || font == NULL)
        return;

    static const wchar_t* appliedProperty = L"Salamander.WinLib.DialogFontApplied";
    if (GetPropW(dialog, appliedProperty) != NULL)
        return;
    SetPropW(dialog, appliedProperty, (HANDLE)1);

    SendMessage(dialog, WM_SETFONT, (WPARAM)font, MAKELPARAM(FALSE, 0));
    EnumChildWindows(dialog, WinLibDPIApplyDialogFontChild, (LPARAM)font);
    RedrawWindow(dialog, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

inline const BYTE* WinLibDPISkipDialogTemplateField(const BYTE* ptr, const BYTE* end)
{
    if (ptr + sizeof(WORD) > end)
        return NULL;
    const WORD* value = (const WORD*)ptr;
    if (*value == 0)
        return ptr + sizeof(WORD);
    if (*value == 0xFFFF)
        return ptr + 2 * sizeof(WORD) <= end ? ptr + 2 * sizeof(WORD) : NULL;
    while ((const BYTE*)(value + 1) <= end)
    {
        if (*value++ == 0)
            return (const BYTE*)value;
    }
    return NULL;
}

inline BYTE* WinLibDPICloneDialogTemplateWithFont(const void* source, DWORD sourceSize,
                                                   const LOGFONT* logFont, UINT dpi,
                                                   DWORD* resultSize)
{
    if (source == NULL || logFont == NULL || sourceSize < sizeof(DLGTEMPLATE))
        return NULL;

    const BYTE* begin = (const BYTE*)source;
    const BYTE* end = begin + sourceSize;
    const WORD* words = (const WORD*)begin;
    BOOL extended = sourceSize >= 26 && words[0] == 1 && words[1] == 0xFFFF;
    const size_t styleOffset = extended ? 12 : offsetof(DLGTEMPLATE, style);
    const DWORD style = *(const DWORD*)(begin + styleOffset);
    const BYTE* ptr = begin + (extended ? 26 : sizeof(DLGTEMPLATE));
    ptr = WinLibDPISkipDialogTemplateField(ptr, end); // menu
    if (ptr != NULL)
        ptr = WinLibDPISkipDialogTemplateField(ptr, end); // class
    if (ptr != NULL)
        ptr = WinLibDPISkipDialogTemplateField(ptr, end); // title
    if (ptr == NULL)
        return NULL;

    const BYTE* oldItems;
    if ((style & DS_SETFONT) != 0)
    {
        const size_t fontHeader = extended ? 6 : 2;
        if (ptr + fontHeader > end)
            return NULL;
        const BYTE* afterFace = WinLibDPISkipDialogTemplateField(ptr + fontHeader, end);
        if (afterFace == NULL)
            return NULL;
        oldItems = (const BYTE*)(((ULONG_PTR)afterFace + 3) & ~(ULONG_PTR)3);
    }
    else
        oldItems = (const BYTE*)(((ULONG_PTR)ptr + 3) & ~(ULONG_PTR)3);
    if (oldItems > end)
        return NULL;

    WCHAR faceName[LF_FACESIZE];
#ifdef UNICODE
    lstrcpynW(faceName, logFont->lfFaceName, LF_FACESIZE);
#else
    if (MultiByteToWideChar(CP_ACP, 0, logFont->lfFaceName, -1,
                            faceName, LF_FACESIZE) == 0)
        return NULL;
    faceName[LF_FACESIZE - 1] = 0;
#endif
    const size_t faceChars = wcslen(faceName) + 1;
    const size_t fontBytes = (extended ? 6 : 2) + faceChars * sizeof(WCHAR);
    const size_t prefixBytes = ptr - begin;
    const size_t newItemsOffset = (prefixBytes + fontBytes + 3) & ~(size_t)3;
    const size_t itemsBytes = end - oldItems;
    if (newItemsOffset > MAXDWORD || itemsBytes > MAXDWORD - newItemsOffset)
        return NULL;

    const DWORD newSize = (DWORD)(newItemsOffset + itemsBytes);
    BYTE* result = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, newSize);
    if (result == NULL)
        return NULL;
    memcpy(result, begin, prefixBytes);
    *(DWORD*)(result + styleOffset) =
        (*(DWORD*)(result + styleOffset) | DS_SETFONT) & ~DS_FIXEDSYS;

    BYTE* font = result + prefixBytes;
    int pointSize = MulDiv(abs(logFont->lfHeight), 72,
                           dpi != 0 ? (int)dpi : USER_DEFAULT_SCREEN_DPI);
    if (pointSize < 1)
        pointSize = 1;
    *(WORD*)font = (WORD)pointSize;
    font += sizeof(WORD);
    if (extended)
    {
        *(WORD*)font = (WORD)logFont->lfWeight;
        font += sizeof(WORD);
        *font++ = logFont->lfItalic;
        *font++ = logFont->lfCharSet;
    }
    memcpy(font, faceName, faceChars * sizeof(WCHAR));
    memcpy(result + newItemsOffset, oldItems, itemsBytes);
    if (resultSize != NULL)
        *resultSize = newSize;
    return result;
}

inline BYTE* WinLibDPICloneResourceDialogWithFont(HINSTANCE module, int resID,
                                                  const LOGFONT* logFont, UINT dpi,
                                                  DWORD* resultSize)
{
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resID), MAKEINTRESOURCEW(5));
    if (resource == NULL)
        return NULL;
    HGLOBAL loaded = LoadResource(module, resource);
    DWORD size = SizeofResource(module, resource);
    const void* data = loaded != NULL ? LockResource(loaded) : NULL;
    return WinLibDPICloneDialogTemplateWithFont(data, size, logFont, dpi, resultSize);
}

inline void WinLibDPIFreeDialogTemplate(BYTE* dialogTemplate)
{
    if (dialogTemplate != NULL)
        HeapFree(GetProcessHeap(), 0, dialogTemplate);
}

inline void WinLibDPIApplyDialogLogFont(HWND dialog, const LOGFONT* logFont)
{
    if (dialog == NULL || logFont == NULL ||
        GetPropW(dialog, L"Salamander.WinLib.DialogFontApplied") != NULL)
    {
        return;
    }

    HFONT font = CreateFontIndirect(logFont);
    if (font == NULL)
        return;
    const UINT_PTR subclassID = 0xD1F0;
    if (!SetWindowSubclass(dialog, WinLibDPIDialogFontSubclassProc, subclassID, (DWORD_PTR)font))
    {
        DeleteObject(font);
        return;
    }
    WinLibDPIApplyDialogFont(dialog, font);
}

inline int WinLibDPIGetSystemMetric(HWND hwnd, int index)
{
    static CWinLibGetSystemMetricsForDpi getSystemMetricsForDpi = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            getSystemMetricsForDpi = (CWinLibGetSystemMetricsForDpi)GetProcAddress(user32, "GetSystemMetricsForDpi");
        loaded = TRUE;
    }
    if (getSystemMetricsForDpi != NULL)
        return getSystemMetricsForDpi(index, WinLibDPIGetWindowDPI(hwnd));
    return GetSystemMetrics(index);
}

inline BOOL WinLibDPIGetNonClientMetricsForDPI(UINT dpi, NONCLIENTMETRICS* metrics)
{
    if (metrics == NULL)
        return FALSE;

    static CWinLibSystemParametersInfoForDpi systemParametersInfoForDpi = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            systemParametersInfoForDpi = (CWinLibSystemParametersInfoForDpi)GetProcAddress(user32, "SystemParametersInfoForDpi");
        loaded = TRUE;
    }

    memset(metrics, 0, sizeof(*metrics));
    metrics->cbSize = sizeof(*metrics);
    if (systemParametersInfoForDpi != NULL &&
        systemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, metrics->cbSize, metrics, 0,
                                   dpi != 0 ? dpi : USER_DEFAULT_SCREEN_DPI))
    {
        return TRUE;
    }
    return SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics->cbSize, metrics, 0);
}

inline BOOL WinLibDPIGetNonClientMetrics(HWND hwnd, NONCLIENTMETRICS* metrics)
{
    return WinLibDPIGetNonClientMetricsForDPI(WinLibDPIGetWindowDPI(hwnd), metrics);
}

inline BOOL WinLibDPIGetIconTitleLogFont(HWND hwnd, LOGFONT* font)
{
    if (font == NULL)
        return FALSE;

    static CWinLibSystemParametersInfoForDpi systemParametersInfoForDpi = NULL;
    static BOOL loaded = FALSE;
    if (!loaded)
    {
        HMODULE user32 = GetModuleHandleA("user32.dll");
        if (user32 != NULL)
            systemParametersInfoForDpi = (CWinLibSystemParametersInfoForDpi)GetProcAddress(user32, "SystemParametersInfoForDpi");
        loaded = TRUE;
    }

    UINT dpi = WinLibDPIGetWindowDPI(hwnd);
    BOOL result = systemParametersInfoForDpi != NULL &&
                  systemParametersInfoForDpi(SPI_GETICONTITLELOGFONT, sizeof(*font),
                                             font, 0, dpi);
    if (!result)
        result = SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(*font), font, 0);
    if (!result)
    {
        NONCLIENTMETRICS metrics;
        if (!WinLibDPIGetNonClientMetrics(hwnd, &metrics))
            return FALSE;
        *font = metrics.lfMessageFont;
    }

    // Match the normalization used by the main window for old themes whose
    // icon-title font can be returned at an unscaled legacy height.
    if (font->lfHeight != 0)
    {
        int height = abs(font->lfHeight);
        int expected = MulDiv(12, (int)dpi, USER_DEFAULT_SCREEN_DPI);
        if (height < expected - 1 || height > expected + 2)
            font->lfHeight = font->lfHeight < 0 ? -expected : expected;
    }
    return TRUE;
}

inline HFONT WinLibDPICreateMessageFont(HWND hwnd)
{
    NONCLIENTMETRICS metrics;
    if (!WinLibDPIGetNonClientMetrics(hwnd, &metrics))
        return NULL;
    return CreateFontIndirect(&metrics.lfMessageFont);
}

inline HFONT WinLibDPICreateMessageFontForDPI(UINT dpi)
{
    NONCLIENTMETRICS metrics;
    if (!WinLibDPIGetNonClientMetricsForDPI(dpi, &metrics))
        return NULL;

    // Remote sessions can return the font for the DPI at which the process
    // started instead of the explicitly requested DPI. This happens in both
    // directions: 12px can be returned for 144 DPI, and 18px can be returned
    // for 96 DPI. Geometry is already correct in that case, so normalize the
    // obviously mismatched height symmetrically around the standard 9pt
    // message font.
    if (dpi != 0 && metrics.lfMessageFont.lfHeight != 0)
    {
        int height = abs(metrics.lfMessageFont.lfHeight);
        int expectedHeight = MulDiv(12, (int)dpi, USER_DEFAULT_SCREEN_DPI);
        if (height < expectedHeight - 1 || height > expectedHeight + 2)
        {
            metrics.lfMessageFont.lfHeight =
                metrics.lfMessageFont.lfHeight < 0 ? -expectedHeight : expectedHeight;
        }
    }
    return CreateFontIndirect(&metrics.lfMessageFont);
}

inline BOOL WinLibDPIGetStatusLogFontForDPI(UINT dpi, LOGFONT* font)
{
    if (font == NULL)
        return FALSE;

    NONCLIENTMETRICS metrics;
    if (!WinLibDPIGetNonClientMetricsForDPI(dpi, &metrics))
        return FALSE;

    if (dpi != 0 && metrics.lfStatusFont.lfHeight != 0)
    {
        int height = abs(metrics.lfStatusFont.lfHeight);
        int expectedHeight = MulDiv(12, (int)dpi, USER_DEFAULT_SCREEN_DPI);
        if (height < expectedHeight - 1 || height > expectedHeight + 2)
        {
            metrics.lfStatusFont.lfHeight =
                metrics.lfStatusFont.lfHeight < 0 ? -expectedHeight : expectedHeight;
        }
    }
    *font = metrics.lfStatusFont;
    return TRUE;
}

inline void WinLibDPIScaleLogFontBetweenDPI(LOGFONT* font, UINT sourceDpi,
                                            UINT destinationDpi)
{
    if (font == NULL)
        return;
    if (sourceDpi == 0)
        sourceDpi = USER_DEFAULT_SCREEN_DPI;
    if (destinationDpi == 0)
        destinationDpi = USER_DEFAULT_SCREEN_DPI;
    font->lfHeight = MulDiv(font->lfHeight, (int)destinationDpi, (int)sourceDpi);
    font->lfWidth = MulDiv(font->lfWidth, (int)destinationDpi, (int)sourceDpi);
}

inline void WinLibDPIScaleLogFont(HWND hwnd, LOGFONT* font)
{
    WinLibDPIScaleLogFontBetweenDPI(font, USER_DEFAULT_SCREEN_DPI,
                                    WinLibDPIGetWindowDPI(hwnd));
}

// Fonts inherited from the Salamander configuration are already expressed at
// the primary/system DPI. Scale them only by the relative monitor-DPI change;
// treating them as 96-DPI logical fonts makes them 225% at a 150% system DPI.
inline void WinLibDPIScaleSystemLogFont(HWND hwnd, LOGFONT* font)
{
    WinLibDPIScaleLogFontBetweenDPI(font, WinLibDPIGetSystemDPI(),
                                    WinLibDPIGetWindowDPI(hwnd));
}

// Unlike dialogs, ordinary top-level windows are not automatically moved to
// the rectangle supplied with WM_DPICHANGED. Their derived WindowProc still
// receives the message first; this is the safe common fallback for windows
// which only need their normal WM_SIZE layout pass.
inline BOOL WinLibDPIApplySuggestedRect(HWND hwnd, UINT message, LPARAM lParam)
{
    if (message != WM_DPICHANGED || hwnd == NULL || lParam == 0 ||
        (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD) != 0)
    {
        return FALSE;
    }

    const RECT* suggested = (const RECT*)lParam;
    return SetWindowPos(hwnd, NULL, suggested->left, suggested->top,
                        suggested->right - suggested->left,
                        suggested->bottom - suggested->top,
                        SWP_NOACTIVATE | SWP_NOZORDER);
}
