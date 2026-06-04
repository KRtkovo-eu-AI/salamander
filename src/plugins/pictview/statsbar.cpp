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

    int maskStride = ((width + 15) / 16) * 2;
    DWORD maskSize = maskStride * height;
    BYTE* andBits = (BYTE*)malloc(maskSize);
    BYTE* xorBits = (BYTE*)malloc(maskSize);
    HICON hInvertedIcon = NULL;
    HDC hDC = NULL;
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));

    if (andBits == NULL || xorBits == NULL || iconInfo.hbmMask == NULL)
        goto cleanup;

    ZeroMemory(andBits, maskSize);
    ZeroMemory(xorBits, maskSize);

    hDC = GetDC(NULL);
    if (hDC == NULL)
        goto cleanup;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 1;
    bmi.bmiHeader.biCompression = BI_RGB;

    if (GetDIBits(hDC, iconInfo.hbmMask, 0, height, andBits, &bmi, DIB_RGB_COLORS) != (int)height)
        goto cleanup;

    for (DWORD i = 0; i < maskSize; i++)
        xorBits[i] = ~andBits[i];

    hInvertedIcon = CreateIcon(DLLInstance, width, height, 1, 1, andBits, xorBits);

cleanup:
    if (hDC != NULL)
        ReleaseDC(NULL, hDC);
    if (iconInfo.hbmColor != NULL)
        DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask != NULL)
        DeleteObject(iconInfo.hbmMask);
    if (andBits != NULL)
        free(andBits);
    if (xorBits != NULL)
        free(xorBits);
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


LRESULT CStatusBar::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (DarkModeShouldUseDarkColors())
    {
        switch (uMsg)
        {
        case WM_ERASEBKGND:
            return TRUE;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(HWindow, &ps);

            RECT client;
            GetClientRect(HWindow, &client);
            HBRUSH backgroundBrush = CreateSolidBrush(DarkModeGetDialogBackgroundColor());
            if (backgroundBrush != NULL)
            {
                FillRect(hDC, &client, backgroundBrush);
                DeleteObject(backgroundBrush);
            }

            int borders[3] = {0, 0, 0};
            SendMessage(HWindow, SB_GETBORDERS, 0, (LPARAM)borders);
            int partCount = (int)SendMessage(HWindow, SB_GETPARTS, 0, 0);
            if (partCount <= 0)
                partCount = 1;

            HFONT hFont = (HFONT)SendMessage(HWindow, WM_GETFONT, 0, 0);
            HFONT hOldFont = hFont != NULL ? (HFONT)SelectObject(hDC, hFont) : NULL;
            COLORREF oldTextColor = SetTextColor(hDC, DarkModeGetDialogTextColor());
            int oldBkMode = SetBkMode(hDC, TRANSPARENT);

            for (int part = 0; part < partCount; part++)
            {
                RECT partRect = client;
                SendMessage(HWindow, SB_GETRECT, part, (LPARAM)&partRect);

                if (part + 1 < partCount)
                {
                    RECT divider = partRect;
                    divider.left = divider.right - max(1, borders[2]);
                    HBRUSH dividerBrush = CreateSolidBrush(RGB(0x4A, 0x4A, 0x4A));
                    if (dividerBrush != NULL)
                    {
                        FillRect(hDC, &divider, dividerBrush);
                        DeleteObject(dividerBrush);
                    }
                }

                RECT contentRect = partRect;
                contentRect.left += borders[2] + 2;
                contentRect.right -= borders[0] + 2;

                HICON hIcon = (HICON)SendMessage(HWindow, SB_GETICON, part, 0);
                if (hIcon != NULL)
                {
                    const int iconSize = 16;
                    int y = contentRect.top + (contentRect.bottom - contentRect.top - iconSize) / 2;
                    DrawIconEx(hDC, contentRect.left, y, hIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                    contentRect.left += iconSize + 4;
                }

                TCHAR text[512];
                text[0] = 0;
                LRESULT textLen = SendMessage(HWindow, SB_GETTEXTLENGTH, part, 0);
                int len = min((int)LOWORD(textLen), (int)_countof(text) - 1);
                SendMessage(HWindow, SB_GETTEXT, part, (LPARAM)text);
                text[len] = 0;
                DrawText(hDC, text, -1, &contentRect,
                         DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_PATH_ELLIPSIS);
            }

            SetBkMode(hDC, oldBkMode);
            SetTextColor(hDC, oldTextColor);
            if (hOldFont != NULL)
                SelectObject(hDC, hOldFont);
            EndPaint(HWindow, &ps);
            return 0;
        }
        }
    }

    return CWindow::WindowProc(uMsg, wParam, lParam);
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
