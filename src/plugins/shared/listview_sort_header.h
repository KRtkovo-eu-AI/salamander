// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define LISTVIEW_SORT_BITMAP_W 8
#define LISTVIEW_SORT_BITMAP_H 8

HBITMAP CreateListViewSortHeaderBitmap(HINSTANCE instance, int bitmapId, BOOL darkMode, COLORREF darkArrow);

void PaintListViewHeaderSortArrow(HDC hdc, HBITMAP sortBitmap, const RECT* itemRect,
                                  const char* text, UINT dtAlign, BOOL reverse);

// Returns TRUE if the dialog should set DWLP_MSGRESULT to *result.
BOOL HandleListViewHeaderSortCustomDraw(LPNMCUSTOMDRAW cd, LRESULT* result,
                                        BOOL showSort, BOOL reverse, HBITMAP sortBitmap,
                                        BOOL darkMode, COLORREF darkBg, COLORREF darkText, COLORREF darkLine);

inline BOOL ListViewHeaderColumnShowsSort(int sortBy, int column, int timeColA, int timeColB)
{
    if (sortBy < 0)
        return FALSE;
    if (timeColA >= 0 && timeColB >= 0 && (sortBy == timeColA || sortBy == timeColB))
        return column == timeColA || column == timeColB;
    return sortBy == column;
}

// Overlay the hdrwnd.bmp arrow after dark-mode header painting. Light mode uses NM_CUSTOMDRAW.
void UpdateListViewSortHeaderOverlay(HWND listView, int sortBy, BOOL reverse, HBITMAP bitmap,
                                     int dateCol, int timeCol, BOOL paintOverlay);
