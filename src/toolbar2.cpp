﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "bitmap.h"
#include "toolbar.h"
#include "svg.h"
#include "darkmode.h"

//*****************************************************************************
//
// CToolBar
//

#define TB_SP_WIDTH 6 // sirka separatoru

#define TB_ICON_TB 3 // pocet bodu nad a pod ikonou, vcetne ramecku
#define TB_TEXT_TB 3

void CToolBar::SetFont()
{
    CALL_STACK_MESSAGE1("CToolBar::SetFont()");
    BOOL changed = FALSE;
    FontHeight = 0;

    if (Style & TLB_STYLE_TEXT)
    {
        HFont = EnvFont;

        HDC hDC = HANDLES(GetDC(NULL));
        TEXTMETRIC tm;
        HFONT hOldFont = (HFONT)SelectObject(hDC, HFont);
        GetTextMetrics(hDC, &tm);
        FontHeight = tm.tmHeight;
        SelectObject(hDC, hOldFont);
        HANDLES(ReleaseDC(NULL, hDC));

        changed = TRUE;
    }
    if (changed)
    {
        DirtyItems = TRUE;
        if (HWindow != NULL)
            InvalidateRect(HWindow, NULL, FALSE);
    }
}

int CToolBar::SetHotItem(int index)
{
    CALL_STACK_MESSAGE2("CToolBar::SetHotItem(%d)", index);
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return -1;
    }
    if (index == HotIndex)
        return HotIndex;
    int oldHotIndex = HotIndex;
    HotIndex = index;
    if (oldHotIndex != -1)
        DrawItem(oldHotIndex);
    if (HotIndex != -1)
        DrawItem(HotIndex);

    if (InserMarkIndex != -1)
    {
        HDC hDC = HANDLES(GetDC(HWindow));
        DrawInsertMark(hDC);
        HANDLES(ReleaseDC(HWindow, hDC));
    }
    return oldHotIndex;
}

int CToolBar::HitTest(int xPos, int yPos)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return -1;
    }
    BOOL vertical = (Style & TLB_STYLE_VERTICAL) != 0;
    if (xPos >= 0 && xPos <= Width && yPos >= 0 && yPos < Height)
    {
        int i;
        for (i = 0; i < Items.Count; i++)
        {
            CToolBarItem* item = Items[i];
            if (item->Style & TLBI_STYLE_SEPARATOR)
                continue;
            if (vertical)
            {
                int xOffset = (Width - item->Width) / 2;
                if (yPos >= item->Offset && yPos < item->Offset + item->Height &&
                    xPos >= xOffset && xPos < xOffset + item->Width)
                {
                    return i;
                }
            }
            else
            {
                int yOffset = (Height - item->Height) / 2;
                if (xPos >= item->Offset && xPos < item->Offset + item->Width &&
                    yPos >= yOffset && yPos < yOffset + item->Height)
                {
                    return i;
                }
            }
        }
    }
    return -1;
}

BOOL CToolBar::HitTest(int xPos, int yPos, int& index, BOOL& dropDown)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return FALSE;
    }
    BOOL vertical = (Style & TLB_STYLE_VERTICAL) != 0;
    Refresh();
    if (xPos >= 0 && xPos <= Width && yPos >= 0 && yPos < Height)
    {
        int i;
        for (i = 0; i < Items.Count; i++)
        {
            CToolBarItem* item = Items[i];
            if (vertical)
            {
                if (item->Style & TLBI_STYLE_SEPARATOR)
                    item->Width = Width - 2;
                int xOffset = (Width - item->Width) / 2;
                if (yPos >= item->Offset && yPos < item->Offset + item->Height &&
                    xPos >= xOffset && xPos < xOffset + item->Width)
                {
                    if ((item->Style & TLBI_STYLE_SEPARATEDROPDOWN) &&
                        xPos >= item->Offset + item->Width - SVGArrowDropDown.GetWidth() - 4)
                        dropDown = TRUE;
                    else
                        dropDown = FALSE;
                    index = i;
                    return TRUE;
                }
            }
            else
            {
                if (item->Style & TLBI_STYLE_SEPARATOR)
                    item->Height = Height - 2 * Padding.ToolBarVertical; // separator nema nastavenou vysku - udelam to ted
                int yOffset = (Height - item->Height) / 2;
                if (xPos >= item->Offset && xPos < item->Offset + item->Width &&
                    yPos >= yOffset && yPos < yOffset + item->Height)
                {
                    if ((item->Style & TLBI_STYLE_SEPARATEDROPDOWN) &&
                        xPos >= item->Offset + item->Width - SVGArrowDropDown.GetWidth() - 4)
                        dropDown = TRUE;
                    else
                        dropDown = FALSE;
                    index = i;
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

BOOL CToolBar::InsertMarkHitTest(int xPos, int yPos, int& index, BOOL& after)
{
    CALL_STACK_MESSAGE5("CToolBar::InsertMarkHitTest(%d, %d, %d, %d)", xPos,
                        yPos, index, after);
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return FALSE;
    }
    if (yPos >= 0 && yPos < Height)
    {
        CToolBarItem* item = NULL;
        int i;
        for (i = 0; i < Items.Count; i++)
        {
            item = Items[i];
            if (item->Style & TLBI_STYLE_SEPARATOR)
                item->Height = Height - 2 * Padding.ToolBarVertical; // separator nema nastavenou vysku - udelam to ted
            int yOffset = (Height - item->Height) / 2;
            if (xPos >= item->Offset && xPos < item->Offset + item->Width &&
                yPos >= yOffset && yPos < yOffset + item->Height)
            {
                int margin = 6;
                int iconSize = GetIconSizeForSystemDPI(ICONSIZE_16);
                if (item->Width < iconSize)
                    margin = 3;
                index = i;
                if (xPos <= item->Offset + margin)
                {
                    if (index > 0)
                    {
                        // prednostne vratime, ze jsem za minulou polozkou (eliminace blikani)
                        index--;
                        after = TRUE;
                    }
                    else
                        after = FALSE;
                    return TRUE;
                }
                if (xPos >= item->Offset + item->Width - margin)
                {
                    after = TRUE;
                    return TRUE;
                }
                // bod lezi nad tlacitkem, ale ne dost u jeho okraje
                return FALSE;
            }
        }
        if (item == NULL)
        {
            // zadna polozka
            index = -1;
            after = FALSE;
            return TRUE;
        }
        if (xPos >= item->Offset + item->Width)
        {
            // za posledni polozkou
            index = Items.Count - 1;
            after = TRUE;
            return TRUE;
        }
    }
    return FALSE;
}

void CToolBar::SetInsertMark(int index, BOOL after)
{
    CALL_STACK_MESSAGE3("CToolBar::SetInsertMark(%d, %d)", index, after);
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return;
    }
    if (InserMarkIndex != index || InserMarkAfter != after)
    {
        HDC hDC = HANDLES(GetDC(HWindow));
        InserMarkIndex = index;
        InserMarkAfter = after;
        DrawAllItems(hDC);
        HANDLES(ReleaseDC(HWindow, hDC));
    }
}

BOOL CToolBar::Refresh()
{
    CALL_STACK_MESSAGE1("CToolBar::Refresh()");
    if (!DirtyItems || HWindow == NULL)
        return FALSE;
    BOOL vertical = (Style & TLB_STYLE_VERTICAL) != 0;
    int offset = 0;
    int maxWidth = 0;
    int maxHeight = 0;
    HFONT hOldFont = NULL;
    if (Style & TLB_STYLE_TEXT)
        hOldFont = (HFONT)SelectObject(CacheBitmap->HMemDC, HFont);
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        CToolBarItem* item = Items[i];
        if (item->Style & TLBI_STYLE_SEPARATOR)
        {
            if (vertical)
                item->Height = TB_SP_WIDTH;
            else
                item->Width = TB_SP_WIDTH;
        }
        else
        {
            // musime urcit sirku podle obsahu

            int textWidth = 0;

            BOOL iconPresent = FALSE;
            BOOL textPresent = FALSE;
            BOOL innerDropPresent = FALSE;
            BOOL outterDropPresent = FALSE;

            if ((Style & TLB_STYLE_IMAGE) && (item->HIcon != NULL || item->ImageIndex != -1))
                iconPresent = TRUE;

            if (!vertical && (Style & TLB_STYLE_TEXT) && (item->Style & TLBI_STYLE_SHOWTEXT) && item->Text != NULL && *item->Text != 0)
            {
                // pokud polozka obsahuje text, omerime ho
                RECT r;
                r.left = 0;
                r.top = 0;
                r.right = 0;
                r.bottom = 0;
                DWORD noPrefix = item->Style & TLBI_STYLE_NOPREFIX ? DT_NOPREFIX : 0;
                DrawText(CacheBitmap->HMemDC, item->Text, item->TextLen,
                         &r, DT_NOCLIP | DT_LEFT | DT_SINGLELINE | noPrefix | DT_CALCRECT);
                textWidth = r.right;
                textPresent = TRUE;
            }

            if (!vertical && (item->Style & TLBI_STYLE_WHOLEDROPDOWN))
                innerDropPresent = TRUE;

            if (!vertical && (item->Style & TLBI_STYLE_SEPARATEDROPDOWN))
                outterDropPresent = TRUE;

            int width = 1; // levy okraj
            int height = 0;

            if (iconPresent)
            {
                width += Padding.IconLeft;
                item->IconX = width;

                int iconSize = GetIconSizeForSystemDPI(ICONSIZE_16);
                int imgW = item->HIcon != NULL ? iconSize : ImageWidth;
                int imgH = item->HIcon != NULL ? iconSize : ImageHeight;

                width += imgW + Padding.IconRight;
                height = max(height, TB_ICON_TB + imgH + TB_ICON_TB);
            }

            if (textPresent)
            {
                if (iconPresent)
                {
                    width -= Padding.IconRight;
                    width += Padding.ButtonIconText;
                }
                else
                    width += Padding.TextLeft;

                item->TextX = width;
                width += textWidth;
                if (!innerDropPresent)
                    width += Padding.TextRight;
                else
                    width += 3;
                height = max(height, TB_TEXT_TB + FontHeight + TB_TEXT_TB);
            }

            if (innerDropPresent)
            {
                item->InnerX = width;
                width += SVGArrowDropDown.GetWidth() + Padding.TextRight;
            }
            width++; // pravy okraj

            if (outterDropPresent)
            {
                if (!(item->Style & TLBI_STYLE_FIXEDWIDTH))
                {
                    item->OutterX = width + 2;
                    width += 2 + SVGArrowDropDown.GetWidth() + 2;
                }
                else
                    item->OutterX = width - (2 + SVGArrowDropDown.GetWidth() + 2); // ukousneme s sirky polozky
            }

            if (!(item->Style & TLBI_STYLE_FIXEDWIDTH))
                item->Width = width;
            item->Height = height;
        }
        item->Offset = offset;
        maxWidth = max(maxWidth, item->Width);
        maxHeight = max(maxHeight, item->Height);
        offset += vertical ? item->Height : item->Width;
    }
    if (hOldFont != NULL)
        SelectObject(CacheBitmap->HMemDC, hOldFont);
    CacheBitmap->Enlarge(maxWidth, maxHeight);
    DirtyItems = FALSE; // musim nastavit pred paintem, aby nedoslo k rekurzi

    if (HWindow != NULL)
    {
        InvalidateRect(HWindow, NULL, FALSE);
        UpdateWindow(HWindow);
        return TRUE;
    }
    else
        return FALSE;
}

void CToolBar::DrawDropDown(HDC hDC, int x, int y, BOOL grayed)
{
    CALL_STACK_MESSAGE_NONE
    SVGArrowDropDown.AlphaBlend(hDC,
                                x,
                                y,
                                -1, -1,
                                grayed ? SVGSTATE_DISABLED : SVGSTATE_ENABLED);
}

void CToolBar::DrawItem(int index)
{
    CALL_STACK_MESSAGE2("CToolBar::DrawItem(%d)", index);
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return;
    }
    if (Refresh())
        return; // pokud bylo prekresleno vse, nemusime uz nic delat

    HDC hDC = HANDLES(GetDC(HWindow));
    DrawItem(hDC, index);
    HANDLES(ReleaseDC(HWindow, hDC));
}

void CToolBar::DrawItem(HDC hDC, int index)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return;
    }
    if (index < 0 || index >= Items.Count)
    {
        // meli jsme nekolik padacek v CToolBar::DrawItem
        TRACE_E("index=" << index << " Items.Count=" << Items.Count);
        return;
    }
    BOOL vertical = (Style & TLB_STYLE_VERTICAL) != 0;

    CToolBarItem* item = Items[index];
    int width = item->Width;
    int height = item->Height;
    int centerOffset;
    if (vertical)
        centerOffset = (Width - width) / 2;
    else
        centerOffset = (Height - height) / 2;

    //  TRACE_I("TB DrawItem index:"<<index<<" x:"<<item->Offset);

    // podmazu plochu podkladovou barvou
    RECT r1;
    r1.left = 0;
    r1.top = 0;
    if (vertical)
    {
        r1.right = Width;
        r1.bottom = height;
    }
    else
    {
        r1.right = width;
        r1.bottom = Height;
    }
    FillRect(CacheBitmap->HMemDC, &r1, HDialogBrush);

    if (item->Style & TLBI_STYLE_SEPARATOR)
    {
        if (vertical)
        {
            int y = height / 2 - 1;
            HPEN hOldPen = (HPEN)SelectObject(CacheBitmap->HMemDC, BtnShadowPen);
            MoveToEx(CacheBitmap->HMemDC, 1, y, NULL);
            LineTo(CacheBitmap->HMemDC, Width - 1, y);
            SelectObject(CacheBitmap->HMemDC, BtnHilightPen);
            MoveToEx(CacheBitmap->HMemDC, 1, y + 1, NULL);
            LineTo(CacheBitmap->HMemDC, Width - 1, y + 1);
            SelectObject(CacheBitmap->HMemDC, hOldPen);
        }
        else
        {
            int x = width / 2 - 1;
            HPEN hOldPen = (HPEN)SelectObject(CacheBitmap->HMemDC, BtnShadowPen);
            MoveToEx(CacheBitmap->HMemDC, x, 1, NULL);
            LineTo(CacheBitmap->HMemDC, x, Height - 1);
            SelectObject(CacheBitmap->HMemDC, BtnHilightPen);
            MoveToEx(CacheBitmap->HMemDC, x + 1, 1, NULL);
            LineTo(CacheBitmap->HMemDC, x + 1, Height - 1);
            SelectObject(CacheBitmap->HMemDC, hOldPen);
        }
    }
    else
    {
        int imgW = 0;
        int imgH = 0;
        BOOL iconPresent = FALSE;
        BOOL textPresent = FALSE;
        BOOL innerDropPresent = FALSE;
        BOOL outterDropPresent = FALSE;
        if ((Style & TLB_STYLE_IMAGE) && (item->HIcon != NULL || item->ImageIndex != -1))
        {
            iconPresent = TRUE;
            if (item->HIcon != NULL)
            {
                int iconSize = GetIconSizeForSystemDPI(ICONSIZE_16);
                imgW = iconSize;
                imgH = iconSize;
            }
            else
            {
                if (HImageList == NULL)
                    TRACE_E("CToolBar::DrawItem HImageList is not assigned.");

                imgW = ImageWidth;
                imgH = ImageHeight;
            }
        }
        if (!vertical && (Style & TLB_STYLE_TEXT) && (item->Style & TLBI_STYLE_SHOWTEXT) && item->Text != NULL && *item->Text != 0)
            textPresent = TRUE;
        if (!vertical && (item->Style & TLBI_STYLE_WHOLEDROPDOWN))
            innerDropPresent = TRUE;
        if (!vertical && (item->Style & TLBI_STYLE_SEPARATEDROPDOWN))
            outterDropPresent = TRUE;

        RECT r;
        r.left = 0;
        r.top = centerOffset;
        r.right = width;
        r.bottom = r.top + height;

        BOOL bodyDown = FALSE; // je telo zamackle ?
        BOOL dropDown = FALSE; // je drop down zamackly?
        BOOL checked = FALSE;
        BOOL grayed = !Customizing && (item->State & TLBI_STATE_GRAYED);
        if (HelpMode && HotIndex == index)
            grayed = FALSE; // v helpmode jsou i disabled polozky vysviceny

        // vykreslim ramecek
        if (!grayed && ((HotIndex == index || item->State & TLBI_STATE_CHECKED) || (item->State & TLBI_STATE_PRESSED)))
        {
            if (outterDropPresent)
                r.right -= 2 + SVGArrowDropDown.GetWidth() + 2;

            bodyDown = !Customizing && ((item->State & TLBI_STATE_PRESSED) || (item->State & TLBI_STATE_CHECKED));
            dropDown = !Customizing && (item->State & TLBI_STATE_DROPDOWNPRESSED);

            if (bodyDown && (item->State & TLBI_STATE_CHECKED))
            {
                if (DarkModeShouldUseDarkColors())
                {
                    RECT fill = {r.left + 1, r.top + 1, r.right - 1, r.bottom - 1};
                    if (DarkCheckedUseAccent)
                    {
                        if (HotIndex != index)
                            FillRect(CacheBitmap->HMemDC, &fill, HSelectedBkBrush);
                    }
                    else
                        FillRect(CacheBitmap->HMemDC, &fill, HNormalBkBrush);
                }
                else if (HotIndex != index)
                {
                    // ditherovane zamackle pozadi
                    SetBrushOrgEx(CacheBitmap->HMemDC, 0, r.top, NULL);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(CacheBitmap->HMemDC, HDitherBrush);
                    int oldTextColor = SetTextColor(CacheBitmap->HMemDC, GetSysColor(COLOR_BTNFACE));
                    int oldBkColor = SetBkColor(CacheBitmap->HMemDC, GetSysColor(COLOR_3DHILIGHT));
                    PatBlt(CacheBitmap->HMemDC, r.left + 1, r.top + 1,
                           r.right - r.left - 2, r.bottom - r.top - 2, PATCOPY);
                    SetTextColor(CacheBitmap->HMemDC, oldTextColor);
                    SetBkColor(CacheBitmap->HMemDC, oldBkColor);
                    SelectObject(CacheBitmap->HMemDC, hOldBrush);
                }
                checked = TRUE;
            }

            // ramecek kolem tela
            if (DarkModeShouldUseDarkColors())
            {
                HBRUSH frameBrush = DarkModeGetPanelFrameBrush();
                if (frameBrush != NULL)
                {
                    RECT frameRect = r;
                    FrameRect(CacheBitmap->HMemDC, &frameRect, frameBrush);
                }
            }
            else
            {
                DWORD mode = bodyDown ? BDR_SUNKENOUTER : BDR_RAISEDINNER;
                DrawEdge(CacheBitmap->HMemDC, &r, mode, BF_RECT);
            }

            if (HotIndex == index && outterDropPresent)
            {
                // ramecek kolem drop down
                r.left = r.right;
                r.right = width;
                if (DarkModeShouldUseDarkColors())
                {
                    HBRUSH frameBrush = DarkModeGetPanelFrameBrush();
                    if (frameBrush != NULL)
                        FrameRect(CacheBitmap->HMemDC, &r, frameBrush);
                }
                else
                {
                    DWORD mode = dropDown ? BDR_SUNKENOUTER : BDR_RAISEDINNER;
                    DrawEdge(CacheBitmap->HMemDC, &r, mode, BF_RECT);
                }
            }
        }

        if (iconPresent)
        {
            if (grayed)
            {
                // kreslime bud s pozadim (rychlejsi) nebo v priade checked transparentne
                int offset = bodyDown ? 1 : 0;
                int x = item->IconX + offset;
                int y = centerOffset + (item->Height - imgH) / 2 + offset;
                int iconSize = GetIconSizeForSystemDPI(ICONSIZE_16);
                if (item->HIcon != NULL)
                    DrawIconEx(CacheBitmap->HMemDC, x, y, item->HIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                else
                {
                    HIMAGELIST hImageList = HImageList;
                    ImageList_Draw(hImageList, item->ImageIndex, CacheBitmap->HMemDC,
                                   x, y, checked ? ILD_TRANSPARENT : ILD_NORMAL);
                }
                if (item->HOverlay != NULL)
                    DrawIconEx(CacheBitmap->HMemDC, x, y, item->HOverlay, iconSize, iconSize, 0, NULL, DI_NORMAL);
            }
            else
            {
                // kreslime bud s pozadim (rychlejsi) nebo v priade checked transparentne
                int offset = bodyDown ? 1 : 0;
                int x = item->IconX + offset;
                int y = centerOffset + (item->Height - imgH) / 2 + offset;
                int iconSize = GetIconSizeForSystemDPI(ICONSIZE_16);
                if (item->HIcon != NULL)
                    DrawIconEx(CacheBitmap->HMemDC, x, y, item->HIcon, iconSize, iconSize, 0, NULL, DI_NORMAL);
                else
                {
                    HIMAGELIST hImageList = HHotImageList ? HHotImageList : HImageList;
                    //if (HHotImageList != NULL && (bodyDown || HotIndex == index))
                    //  hImageList = HHotImageList;

                    ImageList_Draw(hImageList, item->ImageIndex, CacheBitmap->HMemDC,
                                   x, y, checked ? ILD_TRANSPARENT : ILD_NORMAL);
                    //          if (!checked)
                    //          {
                    //            COLORREF bkC = ImageList_GetBkColor(hImageList);
                    //          }
                }
                if (item->HOverlay != NULL)
                    DrawIconEx(CacheBitmap->HMemDC, x, y, item->HOverlay, iconSize, iconSize, 0, NULL, DI_NORMAL);
            }
        }

        if (textPresent)
        {
            int offset = bodyDown ? 1 : 0;
            r.left = item->TextX + offset;
            r.right = width + offset;
            r.top = centerOffset + offset;
            r.bottom = r.top + item->Height;
            DWORD noPrefix = item->Style & TLBI_STYLE_NOPREFIX ? DT_NOPREFIX : 0;
            HFONT hOldFont = (HFONT)SelectObject(CacheBitmap->HMemDC, HFont);
            COLORREF defaultText = DarkModeShouldUseDarkColors() ? GetCOLORREF(CurrentColors[ITEM_FG_NORMAL])
                                                                 : GetSysColor(COLOR_BTNTEXT);
            if (grayed)
            {
                RECT textR2 = r;
                textR2.left++;
                textR2.top++;
                textR2.right++;
                textR2.bottom++;
                COLORREF disabledHighlight =
                    DarkModeShouldUseDarkColors() ? RGB(200, 200, 200) : GetSysColor(COLOR_BTNHILIGHT);
                SetTextColor(CacheBitmap->HMemDC, disabledHighlight);
                DrawText(CacheBitmap->HMemDC, item->Text, item->TextLen,
                         &textR2, noPrefix | DT_NOCLIP | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
                COLORREF disabledText =
                    DarkModeShouldUseDarkColors() ? RGB(128, 128, 128) : GetSysColor(COLOR_BTNSHADOW);
                SetTextColor(CacheBitmap->HMemDC, disabledText);
            }
            else
                SetTextColor(CacheBitmap->HMemDC, defaultText);
            DrawText(CacheBitmap->HMemDC, item->Text, item->TextLen, &r,
                     noPrefix | DT_NOCLIP | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            if (hOldFont != NULL)
                SelectObject(CacheBitmap->HMemDC, hOldFont);
        }

        if (innerDropPresent || outterDropPresent)
        {
            int y = 1 + centerOffset + (height - SVGArrowDropDown.GetHeight()) / 2;
            if (innerDropPresent)
            {
                int offset = 0;
                if (!grayed && bodyDown)
                    offset = 1;
                DrawDropDown(CacheBitmap->HMemDC, item->InnerX + offset, y + offset, grayed);
            }
            if (outterDropPresent)
            {
                int offset = 0;
                if (!grayed && dropDown)
                    offset = 1;
                // zde budeme posouvat pouze dolu - mame malo mista
                DrawDropDown(CacheBitmap->HMemDC, item->OutterX, y + offset, grayed);
            }
        }
    }
    if (vertical)
        BitBlt(hDC, 0, item->Offset, Width, height, CacheBitmap->HMemDC, 0, 0, SRCCOPY);
    else
        BitBlt(hDC, item->Offset, 0, width, Height, CacheBitmap->HMemDC, 0, 0, SRCCOPY);
}

void CToolBar::DrawAllItems(HDC hDC)
{
    CALL_STACK_MESSAGE1("CToolBar::DrawAllItems()");
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return;
    }
    if (Refresh())
        return; // pokud bylo prekresleno vse, nemusime uz nic delat

    BOOL vertical = (Style & TLB_STYLE_VERTICAL) != 0;

    int start = 0;
    int end = Width;

    RECT clipRect;
    int clipRectType = GetClipBox(hDC, &clipRect);
    if (clipRectType == SIMPLEREGION || clipRectType == COMPLEXREGION)
    {
        if (vertical)
        {
            start = clipRect.top;
            end = clipRect.bottom;
        }
        else
        {
            start = clipRect.left;
            end = clipRect.right;
        }
    }
    int offset = 0;
    int length;

    int i;
    for (i = 0; i < Items.Count; i++)
    {
        CToolBarItem* item = Items[i];
        offset = item->Offset;
        if (vertical)
        {
            length = item->Height;
            if (offset < Height &&
                !((offset < start && offset + length <= start) || (offset >= end && offset + length > end)))
                DrawItem(hDC, i);
        }
        else
        {
            length = item->Width;
            if (offset < Width &&
                !((offset < start && offset + length <= start) || (offset >= end && offset + length > end)))
                DrawItem(hDC, i);
        }
        offset += length;
    }
    // domazu zbytek na konci
    if (vertical)
    {
        if (offset < Height)
        {
            length = Height;
            if (!((offset < start && offset + length <= start) || (offset >= end && offset + length > end)))
            {
                RECT r;
                r.left = 0;
                r.top = offset;
                r.right = Width;
                r.bottom = offset + length;
                FillRect(hDC, &r, HDialogBrush);
            }
        }
    }
    else
    {
        if (offset < Width)
        {
            length = Width;
            if (!((offset < start && offset + length <= start) || (offset >= end && offset + length > end)))
            {
                RECT r;
                r.left = offset;
                r.top = 0;
                r.right = offset + length;
                r.bottom = Height;
                FillRect(hDC, &r, HDialogBrush);
            }
        }
    }
    if (InserMarkIndex != -1)
        DrawInsertMark(hDC);
}

void CToolBar::DrawInsertMark(HDC hDC)
{
    CALL_STACK_MESSAGE1("CToolBar::DrawInsertMark()");
    if (HWindow == NULL)
    {
        TRACE_E("HWindow == NULL");
        return;
    }
    if (InserMarkIndex == -1)
        return;
    int x = 0;
    // urcime posizici
    if (InserMarkIndex >= 0 && InserMarkIndex < Items.Count)
    {
        CToolBarItem* item = Items[InserMarkIndex];
        x = item->Offset;
        if (InserMarkAfter)
            x += item->Width;
    }
    x -= 1;
    HPEN hPen = HANDLES(CreatePen(PS_SOLID, 0, RGB(0, 0, 0)));
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);
    // vrchni dve vodorovne cary
    MoveToEx(hDC, x - 2, 0, NULL);
    LineTo(hDC, x + 4, 0);
    MoveToEx(hDC, x - 1, 1, NULL);
    LineTo(hDC, x + 3, 1);
    // dve svisle cary
    MoveToEx(hDC, x, 2, NULL);
    LineTo(hDC, x, Height - 2);
    MoveToEx(hDC, x + 1, 2, NULL);
    LineTo(hDC, x + 1, Height - 2);
    // spodni dve vodorovne cary
    MoveToEx(hDC, x - 1, Height - 2, NULL);
    LineTo(hDC, x + 3, Height - 2);
    MoveToEx(hDC, x - 2, Height - 1, NULL);
    LineTo(hDC, x + 4, Height - 1);
    SelectObject(hDC, hOldPen);
    HANDLES(DeleteObject(hPen));
}
