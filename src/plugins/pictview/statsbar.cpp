// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "../../darkmode.h"

#include "lib\\pvw32dll.h"
#include "renderer.h"
#include "pictview.h"
#include "pictview.rh"
#include "pictview.rh2"
#include "lang\lang.rh"


namespace
{
HBITMAP CreateStatusBarIconDIB(int width, int height, BYTE** bits)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)bits, NULL, 0);
}

bool RenderIconForStatusBar(HICON hIcon, int width, int height, COLORREF background, BYTE** bits, HBITMAP* bitmap)
{
    *bits = NULL;
    *bitmap = CreateStatusBarIconDIB(width, height, bits);
    if (*bitmap == NULL || *bits == NULL)
        return false;

    HDC hDC = GetDC(NULL);
    HDC hMemDC = CreateCompatibleDC(hDC);
    if (hMemDC == NULL)
    {
        ReleaseDC(NULL, hDC);
        DeleteObject(*bitmap);
        *bitmap = NULL;
        *bits = NULL;
        return false;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, *bitmap);
    HBRUSH hBrush = CreateSolidBrush(background);
    RECT r = {0, 0, width, height};
    if (hBrush != NULL)
    {
        FillRect(hMemDC, &r, hBrush);
        DeleteObject(hBrush);
    }
    DrawIconEx(hMemDC, 0, 0, hIcon, width, height, 0, NULL, DI_NORMAL);
    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(NULL, hDC);
    return true;
}

HICON CreateInvertedStatusBarIcon(HICON hIcon)
{
    if (hIcon == NULL)
        return NULL;

    ICONINFO iconInfo;
    if (!GetIconInfo(hIcon, &iconInfo))
        return NULL;

    BITMAP bitmapInfo;
    ZeroMemory(&bitmapInfo, sizeof(bitmapInfo));
    HBITMAP hMeasureBitmap = iconInfo.hbmColor != NULL ? iconInfo.hbmColor : iconInfo.hbmMask;
    if (hMeasureBitmap != NULL)
        GetObject(hMeasureBitmap, sizeof(bitmapInfo), &bitmapInfo);

    int width = bitmapInfo.bmWidth > 0 ? bitmapInfo.bmWidth : 16;
    int height = bitmapInfo.bmHeight > 0 ? bitmapInfo.bmHeight : 16;
    if (iconInfo.hbmColor == NULL)
        height /= 2;

    if (iconInfo.hbmColor != NULL)
        DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask != NULL)
        DeleteObject(iconInfo.hbmMask);

    BYTE* blackBits = NULL;
    BYTE* whiteBits = NULL;
    BYTE* outputBits = NULL;
    HBITMAP hBlackBitmap = NULL;
    HBITMAP hWhiteBitmap = NULL;
    HBITMAP hOutputBitmap = NULL;
    HBITMAP hMaskBitmap = NULL;
    HICON hInvertedIcon = NULL;
    BYTE* maskBits = NULL;
    int maskStride = ((width + 15) / 16) * 2;
    ICONINFO outputIconInfo;
    ZeroMemory(&outputIconInfo, sizeof(outputIconInfo));

    if (!RenderIconForStatusBar(hIcon, width, height, RGB(0, 0, 0), &blackBits, &hBlackBitmap) ||
        !RenderIconForStatusBar(hIcon, width, height, RGB(255, 255, 255), &whiteBits, &hWhiteBitmap))
        goto cleanup;

    hOutputBitmap = CreateStatusBarIconDIB(width, height, &outputBits);
    if (hOutputBitmap == NULL || outputBits == NULL)
        goto cleanup;

    for (int i = 0; i < width * height; i++)
    {
        BYTE* black = blackBits + i * 4;
        BYTE* white = whiteBits + i * 4;
        BYTE* output = outputBits + i * 4;

        int maxDiff = max(abs((int)white[0] - (int)black[0]),
                          max(abs((int)white[1] - (int)black[1]),
                              abs((int)white[2] - (int)black[2])));
        int alpha = 255 - maxDiff;
        if (alpha <= 8)
        {
            output[0] = 0;
            output[1] = 0;
            output[2] = 0;
            output[3] = 0;
            continue;
        }

        int sourceBlue = min(255, ((int)black[0] * 255 + alpha / 2) / alpha);
        int sourceGreen = min(255, ((int)black[1] * 255 + alpha / 2) / alpha);
        int sourceRed = min(255, ((int)black[2] * 255 + alpha / 2) / alpha);

        output[0] = (BYTE)(255 - sourceBlue);
        output[1] = (BYTE)(255 - sourceGreen);
        output[2] = (BYTE)(255 - sourceRed);
        output[3] = (BYTE)alpha;
    }

    maskBits = (BYTE*)calloc(maskStride * height, 1);
    if (maskBits == NULL)
        goto cleanup;
    hMaskBitmap = CreateBitmap(width, height, 1, 1, maskBits);
    free(maskBits);
    if (hMaskBitmap == NULL)
        goto cleanup;

    outputIconInfo.fIcon = TRUE;
    outputIconInfo.hbmColor = hOutputBitmap;
    outputIconInfo.hbmMask = hMaskBitmap;
    hInvertedIcon = CreateIconIndirect(&outputIconInfo);

cleanup:
    if (hBlackBitmap != NULL)
        DeleteObject(hBlackBitmap);
    if (hWhiteBitmap != NULL)
        DeleteObject(hWhiteBitmap);
    if (hOutputBitmap != NULL)
        DeleteObject(hOutputBitmap);
    if (hMaskBitmap != NULL)
        DeleteObject(hMaskBitmap);
    return hInvertedIcon;
}

HICON GetStatusBarIconForCurrentTheme(HICON normalIcon, HICON darkIcon)
{
    return DarkModeShouldUseDarkColors() && darkIcon != NULL ? darkIcon : normalIcon;
}
} // namespace

//****************************************************************************
//
// CStatusBar
//

CStatusBar::CStatusBar()
    : CWindow(ooAllocated)
{
    HCursor = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_SB_CURSOR), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags());
    HAnchor = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_SB_ANCHOR), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags());
    HSize = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_SB_SIZE), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags());
    HPipette = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_SB_PIPETTE), IMAGE_ICON, 16, 16, SalamanderGeneral->GetIconLRFlags());
    HCursorDark = CreateInvertedStatusBarIcon(HCursor);
    HAnchorDark = CreateInvertedStatusBarIcon(HAnchor);
    HSizeDark = CreateInvertedStatusBarIcon(HSize);
    HPipetteDark = CreateInvertedStatusBarIcon(HPipette);
    hProgBar = NULL;
}

CStatusBar::~CStatusBar()
{
    DestroyIcon(HCursor);
    DestroyIcon(HAnchor);
    DestroyIcon(HSize);
    DestroyIcon(HPipette);
    if (HCursorDark != NULL)
        DestroyIcon(HCursorDark);
    if (HAnchorDark != NULL)
        DestroyIcon(HAnchorDark);
    if (HSizeDark != NULL)
        DestroyIcon(HSizeDark);
    if (HPipetteDark != NULL)
        DestroyIcon(HPipetteDark);
    if (hProgBar)
    {
        DestroyWindow(hProgBar);
    }
}


HICON CStatusBar::GetCursorIcon() const
{
    return GetStatusBarIconForCurrentTheme(HCursor, HCursorDark);
}

HICON CStatusBar::GetAnchorIcon() const
{
    return GetStatusBarIconForCurrentTheme(HAnchor, HAnchorDark);
}

HICON CStatusBar::GetSizeIcon() const
{
    return GetStatusBarIconForCurrentTheme(HSize, HSizeDark);
}

HICON CStatusBar::GetPipetteIcon() const
{
    return GetStatusBarIconForCurrentTheme(HPipette, HPipetteDark);
}

#define ICON_MARGIN 32 // space for the icon and margins

void CViewerWindow::SetupStatusBarItems()
{
    if (StatusBar == NULL)
        return;
    HWND hStatusBar = StatusBar->HWindow;

    HFONT hFont = (HFONT)SendMessage(hStatusBar, WM_GETFONT, 0, 0);

    // measure the widths of the items
    int whSize, rgbSize;
    SIZE sz;
    HDC hDC = GetDC(hStatusBar);
    HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
    TCHAR buff[255];
    _stprintf(buff, LoadStr(IDS_SB_WH), Renderer.pvii.Width, Renderer.pvii.Height);
    GetTextExtentPoint32(hDC, buff, (int)_tcslen(buff), &sz);
    whSize = sz.cx + ICON_MARGIN;
    _stprintf(buff, LoadStr(IDS_SB_RGB), 255, 255, 255);
    GetTextExtentPoint32(hDC, buff, (int)_tcslen(buff), &sz);
    rgbSize = max(whSize, sz.cx) + ICON_MARGIN + 10; // the bottom-right corner overwrites us
    SelectObject(hDC, hOldFont);
    ReleaseDC(hStatusBar, hDC);

    RECT r;
    GetClientRect(hStatusBar, &r);
    int width = r.right - r.left;

    // 0: text
    // 1: cursor
    // 2: size
    // 3: anchor/rgb
    INT parts[4];
    parts[3] = -1;
    parts[2] = width - rgbSize;
    parts[1] = parts[2] - whSize;
    parts[0] = parts[1] - whSize;
    SendMessage(hStatusBar, SB_SETPARTS, 4, (LPARAM)&parts);
}

void SafeSetStatusBarText(HWND hStatusBar, int part, LPCTSTR text)
{
    // if there is already text in the status bar, do not blink unnecessarily
    TCHAR buff[500];
    LRESULT ret = SendMessage(hStatusBar, SB_GETTEXT, part & 0xff, (LPARAM)buff);
    if (_tcscmp(buff, text) != NULL || HIWORD(ret) != (part & 0xffffff00))
    {
        lstrcpyn(buff, text, 500);
        SendMessage(hStatusBar, SB_SETTEXT, part, (LPARAM)buff);
    }
}

void SafeSetStatusBarIcon(HWND hStatusBar, int part, HICON hIcon)
{
    // if there is already an icon in the status bar, do not blink unnecessarily
    HICON hOldIcon = (HICON)SendMessage(hStatusBar, SB_GETICON, part, 0);
    if (hOldIcon != hIcon)
        SendMessage(hStatusBar, SB_SETICON, part, (LPARAM)hIcon);
}

void CViewerWindow::SetStatusBarTexts(int ID)
{
    if (StatusBar == NULL)
        return;
    HWND hStatusBar = StatusBar->HWindow;

    TCHAR buff[255];

    int imageWidth = Renderer.pvii.Width;
    int imageHeight = Renderer.pvii.Height;

    // tip
    buff[0] = 0;
    if (ID)
    {
        _tcscpy(buff, LoadStr(ID));
    }
    else
    {
        if (Renderer.CurrTool == RT_SELECT)
            _tcscpy(buff, LoadStr(IDS_SB_CAGE));
        if (Renderer.CurrTool == RT_ZOOM)
            _tcscpy(buff, LoadStr(IDS_SB_ZOOM));
        if (Renderer.CurrTool == RT_HAND)
            _tcscpy(buff, LoadStr(IDS_SB_HAND));
    }
    SafeSetStatusBarText(hStatusBar, 0 | SBT_NOBORDERS, buff);

    // cursor
    SafeSetStatusBarIcon(hStatusBar, 1, StatusBar->GetCursorIcon());
    DWORD newMousePos = GetMessagePos();
    POINT pt;
    pt.x = GET_X_LPARAM(newMousePos);
    pt.y = GET_Y_LPARAM(newMousePos);
    ScreenToClient(Renderer.HWindow, &pt);
    POINT clientPt = pt;
    BOOL validCursor = FALSE;

    if (Renderer.ImageLoaded)
    {
        Renderer.ClientToPicture(&pt); // convert pt into image coordinates
        if (pt.x >= 0 && pt.x < imageWidth && pt.y >= 0 && pt.y < imageHeight)
            validCursor = TRUE;
        if (validCursor)
            _stprintf(buff, LoadStr(IDS_SB_XY), pt.x, pt.y);
        else
            buff[0] = 0;
        SafeSetStatusBarText(hStatusBar, 1, buff);
    }

    // size
    SafeSetStatusBarIcon(hStatusBar, 2, StatusBar->GetSizeIcon());
    int sizeWidth, sizeHeight;
    if (IsCageValid(&Renderer.TmpCageRect))
    {
        // the dragged cage has priority
        sizeWidth = abs(Renderer.TmpCageRect.right - Renderer.TmpCageRect.left) + 1;
        sizeHeight = abs(Renderer.TmpCageRect.bottom - Renderer.TmpCageRect.top) + 1;
    }
    else if (IsCageValid(&Renderer.SelectRect))
    {
        // otherwise an existing selection
        sizeWidth = abs(Renderer.SelectRect.right - Renderer.SelectRect.left) + 1;
        sizeHeight = abs(Renderer.SelectRect.bottom - Renderer.SelectRect.top) + 1;
    }
    else
    {
        // otherwise the image size
        sizeWidth = imageWidth;
        sizeHeight = imageHeight;
    }
    _stprintf(buff, LoadStr(IDS_SB_WH), sizeWidth, sizeHeight);
    SafeSetStatusBarText(hStatusBar, 2, buff);

    // anchor
    if (IsCageValid(&Renderer.TmpCageRect))
    {
        SafeSetStatusBarIcon(hStatusBar, 3, StatusBar->GetAnchorIcon());
        _stprintf(buff, LoadStr(IDS_SB_XY), Renderer.TmpCageRect.left, Renderer.TmpCageRect.top);
        SafeSetStatusBarText(hStatusBar, 3, buff);
    }
    else
    {
        SafeSetStatusBarIcon(hStatusBar, 3, StatusBar->GetPipetteIcon());
        if (validCursor)
        {
            int ind;
            RGBQUAD rgb;

            Renderer.ClientToPicture(&clientPt);
            Renderer.GetRGBAtCursor(clientPt.x, clientPt.y, &rgb, &ind);
            _stprintf(buff, LoadStr(IDS_SB_RGB), rgb.rgbRed, rgb.rgbGreen, rgb.rgbBlue);
        }
        else
            buff[0] = 0;
    }
    SafeSetStatusBarText(hStatusBar, 3, buff);
}

void CViewerWindow::InitProgressBar()
{
    RECT r;

    if (StatusBar == NULL)
    {
        return;
    }
    SendMessage(StatusBar->HWindow, SB_GETRECT, 1, (LPARAM)&r);
    StatusBar->hProgBar = CreateWindowEx(0, PROGRESS_CLASS, NULL,
                                         PBS_SMOOTH | WS_CHILD | WS_VISIBLE, r.left, r.top,
                                         r.right - r.left, r.bottom - r.top,
                                         StatusBar->HWindow, NULL, DLLInstance, NULL);
    ConfigurePictViewDarkModeFromHost();
    DarkModeApplyTree(StatusBar->HWindow);

    SendMessage(StatusBar->hProgBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(StatusBar->hProgBar, PBM_SETPOS, 0, 0);
}

void CViewerWindow::KillProgressBar()
{
    if (StatusBar == NULL)
    {
        return;
    }
    if (StatusBar->hProgBar)
    {
        DestroyWindow(StatusBar->hProgBar);
        StatusBar->hProgBar = NULL;
    }
}

void CViewerWindow::SetProgress(int done)
{
    if (StatusBar && StatusBar->hProgBar)
    {
        SendMessage(StatusBar->hProgBar, PBM_SETPOS, done, 0);
    }
}
