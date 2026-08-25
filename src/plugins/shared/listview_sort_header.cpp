// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>

#include "listview_sort_header.h"

struct ListViewSortHeaderOverlayInfo
{
    int sortBy;
    BOOL reverse;
    HBITMAP bitmap;
    int dateCol;
    int timeCol;
    BOOL paintOverlay;
};

static const char ListViewSortHeaderOverlayProp[] = "SalListViewSortHdr";
static const UINT_PTR ListViewSortHeaderOverlayId = 0x534C5648;

static void PaintOverlaySortArrows(HWND header, HDC hdc, const ListViewSortHeaderOverlayInfo* info)
{
    if (hdc == NULL || info == NULL || info->bitmap == NULL || info->sortBy < 0)
        return;

    int count = Header_GetItemCount(header);
    int col;
    for (col = 0; col < count; col++)
    {
        if (!ListViewHeaderColumnShowsSort(info->sortBy, col, info->dateCol, info->timeCol))
            continue;

        RECT rc;
        if (!Header_GetItemRect(header, col, &rc))
            continue;

        char text[256];
        text[0] = 0;
        HDITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = HDI_TEXT | HDI_FORMAT;
        item.pszText = text;
        item.cchTextMax = (int)(sizeof(text) / sizeof(text[0]));
        Header_GetItem(header, col, &item);

        UINT format = DT_LEFT;
        if ((item.fmt & HDF_RIGHT) != 0)
            format = DT_RIGHT;
        else if ((item.fmt & HDF_CENTER) != 0)
            format = DT_CENTER;
        PaintListViewHeaderSortArrow(hdc, info->bitmap, &rc, text, format, info->reverse);
    }
}

static LRESULT CALLBACK ListViewSortHeaderOverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                      UINT_PTR subclassId, DWORD_PTR refData)
{
    ListViewSortHeaderOverlayInfo* info = (ListViewSortHeaderOverlayInfo*)refData;
    if (msg == WM_NCDESTROY)
    {
        RemoveWindowSubclass(hwnd, ListViewSortHeaderOverlayProc, subclassId);
        RemovePropA(hwnd, ListViewSortHeaderOverlayProp);
        free(info);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    if ((msg == WM_PAINT || msg == WM_PRINTCLIENT) && info != NULL && info->paintOverlay)
    {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        HDC hdc = (msg == WM_PAINT) ? GetDC(hwnd) : (HDC)wParam;
        PaintOverlaySortArrows(hwnd, hdc, info);
        if (msg == WM_PAINT && hdc != NULL)
            ReleaseDC(hwnd, hdc);
        return result;
    }

    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void UpdateListViewSortHeaderOverlay(HWND listView, int sortBy, BOOL reverse, HBITMAP bitmap,
                                     int dateCol, int timeCol, BOOL paintOverlay)
{
    if (listView == NULL)
        return;

    HWND header = ListView_GetHeader(listView);
    if (header == NULL)
        return;

    ListViewSortHeaderOverlayInfo* info =
        (ListViewSortHeaderOverlayInfo*)GetPropA(header, ListViewSortHeaderOverlayProp);
    if (info == NULL)
    {
        info = (ListViewSortHeaderOverlayInfo*)malloc(sizeof(*info));
        if (info == NULL)
            return;
        memset(info, 0, sizeof(*info));
        if (!SetWindowSubclass(header, ListViewSortHeaderOverlayProc, ListViewSortHeaderOverlayId,
                               (DWORD_PTR)info))
        {
            free(info);
            return;
        }
        SetPropA(header, ListViewSortHeaderOverlayProp, info);
    }
    else
    {
        RemoveWindowSubclass(header, ListViewSortHeaderOverlayProc, ListViewSortHeaderOverlayId);
        SetWindowSubclass(header, ListViewSortHeaderOverlayProc, ListViewSortHeaderOverlayId,
                          (DWORD_PTR)info);
    }

    info->sortBy = sortBy;
    info->reverse = reverse;
    info->bitmap = bitmap;
    info->dateCol = dateCol;
    info->timeCol = timeCol;
    info->paintOverlay = paintOverlay;
    InvalidateRect(header, NULL, TRUE);
}

HBITMAP CreateListViewSortHeaderBitmap(HINSTANCE instance, int bitmapId, BOOL darkMode, COLORREF darkArrow)
{
    COLORMAP clrMap[3];
    clrMap[0].from = RGB(255, 0, 255);
    clrMap[0].to = GetSysColor(COLOR_BTNFACE);
    clrMap[1].from = RGB(255, 255, 255);
    clrMap[1].to = darkMode ? darkArrow : GetSysColor(COLOR_BTNHIGHLIGHT);
    clrMap[2].from = RGB(128, 128, 128);
    clrMap[2].to = darkMode ? darkArrow : GetSysColor(COLOR_BTNSHADOW);
    return CreateMappedBitmap(instance, bitmapId, 0, clrMap, 3);
}

void PaintListViewHeaderSortArrow(HDC hdc, HBITMAP sortBitmap, const RECT* itemRect,
                                  const char* text, UINT dtAlign, BOOL reverse)
{
    if (hdc == NULL || sortBitmap == NULL || itemRect == NULL)
        return;

    SIZE sz;
    sz.cx = 0;
    sz.cy = 0;
    int textLen = text != NULL ? (int)strlen(text) : 0;
    if (textLen > 0)
        GetTextExtentPoint32(hdc, text, textLen, &sz);

    int x;
    if ((dtAlign & DT_RIGHT) != 0)
    {
        x = itemRect->right - 5 - sz.cx - LISTVIEW_SORT_BITMAP_W - 2;
        if (x < itemRect->left + 5)
            x = itemRect->left + 5;
    }
    else
        x = itemRect->left + 5 + sz.cx + LISTVIEW_SORT_BITMAP_W;

    int y = itemRect->top + ((itemRect->bottom - itemRect->top) - LISTVIEW_SORT_BITMAP_H) / 2;
    HDC hMemDC = CreateCompatibleDC(hdc);
    if (hMemDC == NULL)
        return;
    HBITMAP oldBmp = (HBITMAP)SelectObject(hMemDC, sortBitmap);
    BitBlt(hdc, x, y, LISTVIEW_SORT_BITMAP_W, LISTVIEW_SORT_BITMAP_H,
           hMemDC, reverse ? LISTVIEW_SORT_BITMAP_W : 0, 0, SRCCOPY);
    SelectObject(hMemDC, oldBmp);
    DeleteDC(hMemDC);
}

BOOL HandleListViewHeaderSortCustomDraw(LPNMCUSTOMDRAW cd, LRESULT* result,
                                        BOOL showSort, BOOL reverse, HBITMAP sortBitmap,
                                        BOOL darkMode, COLORREF darkBg, COLORREF darkText, COLORREF darkLine)
{
    if (cd == NULL || result == NULL)
        return FALSE;

    if (!darkMode)
    {
        switch (cd->dwDrawStage)
        {
        case CDDS_PREPAINT:
            *result = CDRF_NOTIFYITEMDRAW;
            return TRUE;
        case CDDS_ITEMPREPAINT:
            *result = CDRF_NOTIFYPOSTPAINT;
            return TRUE;
        case CDDS_ITEMPOSTPAINT:
        {
            if (showSort)
            {
                char text[256];
                text[0] = 0;
                HDITEM item;
                memset(&item, 0, sizeof(item));
                item.mask = HDI_TEXT | HDI_FORMAT;
                item.pszText = text;
                item.cchTextMax = (int)(sizeof(text) / sizeof(text[0]));
                Header_GetItem(cd->hdr.hwndFrom, (int)cd->dwItemSpec, &item);
                UINT format = DT_LEFT;
                if ((item.fmt & HDF_RIGHT) != 0)
                    format = DT_RIGHT;
                else if ((item.fmt & HDF_CENTER) != 0)
                    format = DT_CENTER;
                PaintListViewHeaderSortArrow(cd->hdc, sortBitmap, &cd->rc, text, format, reverse);
            }
            *result = CDRF_DODEFAULT;
            return TRUE;
        }
        }
        return FALSE;
    }

    switch (cd->dwDrawStage)
    {
    case CDDS_PREPAINT:
        *result = CDRF_NOTIFYITEMDRAW;
        return TRUE;
    case CDDS_ITEMPREPAINT:
    {
        RECT rc = cd->rc;
        HBRUSH brush = CreateSolidBrush(darkBg);
        if (brush != NULL)
        {
            FillRect(cd->hdc, &rc, brush);
            DeleteObject(brush);
        }

        char text[256];
        text[0] = 0;
        HDITEM item;
        memset(&item, 0, sizeof(item));
        item.mask = HDI_TEXT | HDI_FORMAT;
        item.pszText = text;
        item.cchTextMax = (int)(sizeof(text) / sizeof(text[0]));
        Header_GetItem(cd->hdr.hwndFrom, (int)cd->dwItemSpec, &item);

        rc.left += 5;
        rc.right -= 5;
        if (showSort)
        {
            if ((item.fmt & HDF_RIGHT) != 0)
                rc.left += LISTVIEW_SORT_BITMAP_W + 2;
            else
                rc.right -= LISTVIEW_SORT_BITMAP_W * 3;
        }
        UINT format = DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS;
        if ((item.fmt & HDF_RIGHT) != 0)
            format |= DT_RIGHT;
        else if ((item.fmt & HDF_CENTER) != 0)
            format |= DT_CENTER;
        else
            format |= DT_LEFT;

        int oldBkMode = SetBkMode(cd->hdc, TRANSPARENT);
        COLORREF oldText = SetTextColor(cd->hdc, darkText);
        DrawText(cd->hdc, text, -1, &rc, format);
        SetTextColor(cd->hdc, oldText);
        SetBkMode(cd->hdc, oldBkMode);

        if (showSort)
            PaintListViewHeaderSortArrow(cd->hdc, sortBitmap, &cd->rc, text, format, reverse);

        RECT line = cd->rc;
        line.left = line.right - 1;
        HBRUSH lineBrush = CreateSolidBrush(darkLine);
        if (lineBrush != NULL)
        {
            FillRect(cd->hdc, &line, lineBrush);
            line = cd->rc;
            line.top = line.bottom - 1;
            FillRect(cd->hdc, &line, lineBrush);
            DeleteObject(lineBrush);
        }

        *result = CDRF_SKIPDEFAULT;
        return TRUE;
    }
    }
    return FALSE;
}
