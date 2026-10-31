// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include <cstring>

// Measure the cursor below its hotspot so a modeless tooltip cannot cover the
// hover point and immediately dismiss itself. Keep bitmap and hotspot in the
// same pixel coordinates, including large/custom cursors.
inline int GetToolTipCursorHeight(HCURSOR cursor)
{
    int fallback = GetSystemMetrics(SM_CYCURSOR);
    if (fallback <= 0)
        fallback = 32;
    ICONINFO icon = {};
    if (cursor == NULL || !GetIconInfo(cursor, &icon))
        return fallback;
    BITMAP bitmap = {};
    bool measured = GetObject(icon.hbmColor != NULL ? icon.hbmColor : icon.hbmMask,
                              sizeof(bitmap), &bitmap) != 0;
    int width = bitmap.bmWidth;
    int height = icon.hbmColor != NULL ? bitmap.bmHeight : bitmap.bmHeight / 2;
    if (measured && height > 0 && icon.yHotspot < (DWORD)height)
        fallback = height - (int)icon.yHotspot;
    int result = fallback;
    if (measured && width > 0 && height > 0)
    {
        // BITMAPINFO only declares ONE color. A 1-bpp DIB needs TWO explicit
        // palette entries; reading the missing white entry from the stack made
        // the previous measurement nondeterministically return zero or 32.
        struct CursorBitmapInfo
        {
            BITMAPINFOHEADER Header;
            RGBQUAD Colors[2];
        } info = {};
        info.Header.biSize = sizeof(BITMAPINFOHEADER);
        info.Header.biWidth = width;
        info.Header.biHeight = height;
        info.Header.biPlanes = 1;
        info.Header.biBitCount = 1;
        info.Header.biCompression = BI_RGB;
        info.Header.biClrUsed = 2;
        info.Colors[1].rgbRed = info.Colors[1].rgbGreen = info.Colors[1].rgbBlue = 255;
        HDC dc = CreateCompatibleDC(NULL);
        void* bits = NULL;
        HBITMAP dib = dc != NULL ? CreateDIBSection(dc, (BITMAPINFO*)&info,
                                                   DIB_RGB_COLORS, &bits, NULL, 0) : NULL;
        if (dib != NULL && bits != NULL)
        {
            const size_t wordsPerRow = ((size_t)width + 31) / 32;
            std::memset(bits, 0xff, wordsPerRow * sizeof(DWORD) * height);
            HGDIOBJ previous = SelectObject(dc, dib);
            if (previous != NULL && previous != HGDI_ERROR)
            {
                BOOL drawn = DrawIconEx(dc, 0, 0, cursor, width, height, 0, NULL, DI_MASK);
                SelectObject(dc, previous);
                GdiFlush();
                if (drawn)
                {
                    int emptyRows = 0;
                    for (; emptyRows < height; ++emptyRows)
                    {
                        const DWORD* row = (const DWORD*)bits + emptyRows * wordsPerRow;
                        bool empty = true;
                        for (size_t column = 0; column < wordsPerRow; ++column)
                            if (row[column] != 0xffffffff)
                            {
                                empty = false;
                                break;
                            }
                        if (!empty)
                            break;
                    }
                    int bottom = height - emptyRows;
                    if (bottom > 0 && icon.yHotspot < (DWORD)bottom)
                        result = bottom - (int)icon.yHotspot;
                }
            }
        }
        if (dib != NULL)
            DeleteObject(dib);
        if (dc != NULL)
            DeleteDC(dc);
    }
    if (icon.hbmMask != NULL)
        DeleteObject(icon.hbmMask);
    if (icon.hbmColor != NULL)
        DeleteObject(icon.hbmColor);
    return result;
}
