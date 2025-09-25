// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <commctrl.h>
#include <algorithm>
#include <string>

#include "tabwnd.h"
#include "mainwnd.h"
#include "fileswnd.h"

#ifndef TCM_SETINSERTMARK
#define TCM_SETINSERTMARK (TCM_FIRST + 44)
#endif

#ifndef TCINSERTMARK
typedef struct tagTCINSERTMARK
{
    UINT cbSize;
    DWORD dwFlags;
    int iItem;
} TCINSERTMARK, *PTCINSERTMARK;
#endif

#ifndef TCIMF_BEFORE
#define TCIMF_BEFORE 0x0000
#endif

#ifndef TCIMF_AFTER
#define TCIMF_AFTER 0x0001
#endif

#ifndef TCIS_BUTTONPRESSED
#define TCIS_BUTTONPRESSED 0x0001
#endif

#ifndef TCIS_HIGHLIGHTED
#define TCIS_HIGHLIGHTED 0x0002
#endif

//
// ****************************************************************************
// CTabWindow
//

namespace
{
    constexpr LPARAM kNewTabButtonParam = static_cast<LPARAM>(-1);
    const wchar_t kNewTabButtonText[] = L"+";

    COLORREF BlendColor(COLORREF from, COLORREF to, int weight)
    {
        if (weight < 0)
            weight = 0;
        if (weight > 256)
            weight = 256;
        int inv = 256 - weight;
        int r = (GetRValue(from) * inv + GetRValue(to) * weight + 128) >> 8;
        int g = (GetGValue(from) * inv + GetGValue(to) * weight + 128) >> 8;
        int b = (GetBValue(from) * inv + GetBValue(to) * weight + 128) >> 8;
        return RGB(r, g, b);
    }

    COLORREF LightenColor(COLORREF color, int weight)
    {
        return BlendColor(color, RGB(255, 255, 255), weight);
    }

    COLORREF DarkenColor(COLORREF color, int weight)
    {
        return BlendColor(color, RGB(0, 0, 0), weight);
    }

    bool IsColorDark(COLORREF color)
    {
        int luminance = 30 * GetRValue(color) + 59 * GetGValue(color) + 11 * GetBValue(color);
        return luminance < 128 * 100;
    }

    class CSelChangeGuard
    {
    public:
        explicit CSelChangeGuard(int& counter) : Counter(counter)
        {
            ++Counter;
        }

        ~CSelChangeGuard()
        {
            --Counter;
        }

    private:
        int& Counter;
    };

    int ComputeNewTabMinWidth(HWND hwnd)
    {
        if (hwnd == NULL)
            return 0;

        int minWidth = 0;
        HDC hdc = GetDC(hwnd);
        if (hdc != NULL)
        {
            HFONT oldFont = NULL;
            if (EnvFont != NULL)
                oldFont = (HFONT)SelectObject(hdc, EnvFont);
            SIZE size = {0, 0};
            if (GetTextExtentPoint32W(hdc, kNewTabButtonText, _countof(kNewTabButtonText) - 1, &size))
                minWidth = size.cx;
            if (oldFont != NULL)
                SelectObject(hdc, oldFont);
            ReleaseDC(hwnd, hdc);
        }

        if (minWidth <= 0)
            minWidth = EnvFontCharHeight;

        int padding = EnvFontCharHeight / 2;
        if (padding < 4)
            padding = 4;

        return minWidth + padding;
    }

}

CTabWindow::CTabWindow(CMainWindow* mainWindow, CPanelSide side)
#ifndef _UNICODE
    : CWindow(ooStatic, TRUE)
#else
    : CWindow(ooStatic)
#endif
{
    CALL_STACK_MESSAGE_NONE
    MainWindow = mainWindow;
    Side = side;
    ControlID = 0;
    SuppressSelectionNotifications = 0;
    DragTracking = false;
    Dragging = false;
    DragStartPoint.x = 0;
    DragStartPoint.y = 0;
    DragSourceIndex = -1;
    DragHasExternalTarget = false;
    DragCurrentTarget = -1;
    DragInsertMarkItem = -1;
    DragInsertMarkFlags = 0;
    SetRectEmpty(&DragIndicatorRect);
    DragIndicatorVisible = false;
}

CTabWindow::~CTabWindow()
{
    CALL_STACK_MESSAGE1("CTabWindow::~CTabWindow()");
    DestroyWindow();
}

BOOL CTabWindow::Create(HWND parent, int controlID)
{
    CALL_STACK_MESSAGE_NONE
    ControlID = controlID;
    DWORD style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_TABS | TCS_TOOLTIPS | TCS_FOCUSNEVER;
#ifndef _UNICODE
    HWND hwnd = CreateExW(0, WC_TABCONTROLW, L"", style, 0, 0, 0, 0, parent,
                          (HMENU)(INT_PTR)controlID, HInstance, this);
#else
    HWND hwnd = CreateEx(0, WC_TABCONTROL, "", style, 0, 0, 0, 0, parent,
                         (HMENU)(INT_PTR)controlID, HInstance, this);
#endif
    if (hwnd == NULL)
        return FALSE;
    SendMessage(HWindow, WM_SETFONT, (WPARAM)EnvFont, FALSE);
    EnsureNewTabButton();
    return TRUE;
}

void CTabWindow::DestroyWindow()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
    {
        HWND hwnd = HWindow;
        DetachWindow();
        ::DestroyWindow(hwnd);
    }
}

int CTabWindow::GetNeededHeight() const
{
    CALL_STACK_MESSAGE_NONE
    return 2 + EnvFontCharHeight + 2;
}

int CTabWindow::AddTab(int index, const wchar_t* text, LPARAM data)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return -1;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT | TCIF_PARAM;
    item.pszText = const_cast<LPWSTR>(text != NULL ? text : L"");
    item.lParam = data;
    int count = GetTabCount();
    if (index < 0 || index > count)
        index = count;
    int insertIndex = index;
    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex >= 0 && insertIndex > newTabIndex)
        insertIndex = newTabIndex;
    int colorIndex = (insertIndex < count) ? insertIndex : count;
    int result;
    {
        CSelChangeGuard guard(SuppressSelectionNotifications);
        result = (int)SendMessageW(HWindow, TCM_INSERTITEMW, insertIndex, (LPARAM)&item);
    }
    if (result < 0)
        return result;
    InsertTabColorSlot(colorIndex, count);
    EnsureNewTabButton();
    return result;
}

void CTabWindow::RemoveTab(int index)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
    {
        if (index < 0 || index >= GetTabCount())
            return;
        BOOL removed = FALSE;
        {
            CSelChangeGuard guard(SuppressSelectionNotifications);
            removed = (BOOL)SendMessage(HWindow, TCM_DELETEITEM, index, 0);
        }
        if (removed)
            RemoveTabColorSlot(index);
        EnsureNewTabButton();
    }
}

void CTabWindow::RemoveAllTabs()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
    {
        {
            CSelChangeGuard guard(SuppressSelectionNotifications);
            SendMessage(HWindow, TCM_DELETEALLITEMS, 0, 0);
        }
        TabColors.clear();
        EnsureNewTabButton();
    }
}

void CTabWindow::SetTabText(int index, const wchar_t* text)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL || index < 0 || index >= GetTabCount())
        return;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<LPWSTR>(text != NULL ? text : L"");
    SendMessageW(HWindow, TCM_SETITEMW, index, (LPARAM)&item);
}

void CTabWindow::SetCurSel(int index)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
    {
        if (index >= 0 && index < GetTabCount())
        {
            CSelChangeGuard guard(SuppressSelectionNotifications);
            TabCtrl_SetCurSel(HWindow, index);
        }
    }
}

int CTabWindow::GetCurSel() const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return -1;
    int sel = TabCtrl_GetCurSel(HWindow);
    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex >= 0 && sel == newTabIndex)
        return -1;
    return sel;
}

int CTabWindow::GetTabCount() const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return 0;
    int count = GetDisplayedTabCount();
    if (GetNewTabButtonIndex() >= 0)
        count--;
    return count;
}

LPARAM CTabWindow::GetItemData(int index) const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL || index < 0 || index >= GetTabCount())
        return 0;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_PARAM;
    if (!SendMessageW(HWindow, TCM_GETITEMW, index, (LPARAM)&item))
        return 0;
    return item.lParam;
}

int CTabWindow::HitTest(POINT pt) const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return -1;
    TCHITTESTINFO info;
    ZeroMemory(&info, sizeof(info));
    info.pt = pt;
    return (int)SendMessage(HWindow, TCM_HITTEST, 0, (LPARAM)&info);
}

BOOL CTabWindow::HandleNotify(LPNMHDR nmhdr, LRESULT& result)
{
    CALL_STACK_MESSAGE_NONE
    if (nmhdr == NULL || nmhdr->hwndFrom != HWindow)
        return FALSE;

    switch (nmhdr->code)
    {
    case TCN_SELCHANGE:
    {
        if (SuppressSelectionNotifications > 0)
        {
            result = 0;
            return TRUE;
        }
        int sel = TabCtrl_GetCurSel(HWindow);
        if (IsNewTabButtonIndex(sel))
        {
            if (MainWindow != NULL)
                MainWindow->CommandNewTab(Side, TRUE);
            result = 0;
            return TRUE;
        }
        EnsureSelection();
        if (MainWindow != NULL)
        {
            int current = GetCurSel();
            MainWindow->OnPanelTabSelected(Side, current);
        }
        result = 0;
        return TRUE;
    }

    case NM_RCLICK:
    {
        POINT screen;
        GetCursorPos(&screen);
        POINT client = screen;
        ScreenToClient(HWindow, &client);
        int hit = HitTest(client);
        if (hit >= 0)
        {
            if (IsNewTabButtonIndex(hit))
            {
                if (MainWindow != NULL)
                    MainWindow->CommandNewTab(Side, TRUE);
            }
            else if (MainWindow != NULL)
            {
                MainWindow->OnPanelTabContextMenu(Side, hit, screen);
            }
        }
        result = 0;
        return TRUE;
    }

    }

    return FALSE;
}

void CTabWindow::EnsureSelection()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return;
    if (GetTabCount() <= 0)
        return;

    int sel = TabCtrl_GetCurSel(HWindow);
    if (sel < 0)
    {
        SetCurSel(0);
        return;
    }

    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex >= 0 && sel == newTabIndex)
    {
        if (newTabIndex > 0)
            SetCurSel(newTabIndex - 1);
        else
            SetCurSel(0);
    }
}

void CTabWindow::EnsureNewTabButton()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return;

    int total = GetDisplayedTabCount();
    int newTabIndex = GetNewTabButtonIndex();
    {
        CSelChangeGuard guard(SuppressSelectionNotifications);
        if (newTabIndex >= 0)
        {
            TCITEMW item;
            ZeroMemory(&item, sizeof(item));
            item.mask = TCIF_TEXT;
            item.pszText = const_cast<LPWSTR>(kNewTabButtonText);
            SendMessageW(HWindow, TCM_SETITEMW, newTabIndex, (LPARAM)&item);
        }
        else
        {
            TCITEMW item;
            ZeroMemory(&item, sizeof(item));
            item.mask = TCIF_TEXT | TCIF_PARAM;
            item.pszText = const_cast<LPWSTR>(kNewTabButtonText);
            item.lParam = kNewTabButtonParam;
            SendMessageW(HWindow, TCM_INSERTITEMW, total, (LPARAM)&item);
        }
    }

    UpdateNewTabButtonWidth();
}

void CTabWindow::UpdateNewTabButtonWidth()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return;

    int minWidth = ComputeNewTabMinWidth(HWindow);
    if (minWidth > 0)
        TabCtrl_SetMinTabWidth(HWindow, minWidth);
}

int CTabWindow::GetDisplayedTabCount() const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return 0;
    return TabCtrl_GetItemCount(HWindow);
}

int CTabWindow::GetNewTabButtonIndex() const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return -1;
    int total = TabCtrl_GetItemCount(HWindow);
    if (total <= 0)
        return -1;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_PARAM;
    if (!SendMessageW(HWindow, TCM_GETITEMW, total - 1, (LPARAM)&item))
        return -1;
    if (item.lParam != kNewTabButtonParam)
        return -1;
    return total - 1;
}

BOOL CTabWindow::IsNewTabButtonIndex(int index) const
{
    CALL_STACK_MESSAGE_NONE
    if (index < 0)
        return FALSE;
    int newTabIndex = GetNewTabButtonIndex();
    return newTabIndex >= 0 && index == newTabIndex;
}

bool CTabWindow::IsReorderableIndex(int index) const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return false;
    if (index <= 0)
        return false;
    if (IsNewTabButtonIndex(index))
        return false;
    if (GetTabCount() <= 2)
        return false;
    return true;
}

void CTabWindow::StartDragTracking(int index, const POINT& pt)
{
    CALL_STACK_MESSAGE_NONE
    DragTracking = true;
    Dragging = false;
    DragSourceIndex = index;
    DragStartPoint = pt;
    DragHasExternalTarget = false;
    DragCurrentTarget = -1;
    ClearInsertMark();
}

void CTabWindow::UpdateDragTracking(const POINT& pt)
{
    CALL_STACK_MESSAGE_NONE
    if (!DragTracking)
        return;

    bool externalTarget = false;
    bool mainWindowUpdated = false;

    if (!Dragging)
    {
        int dx = pt.x - DragStartPoint.x;
        if (dx < 0)
            dx = -dx;
        int dy = pt.y - DragStartPoint.y;
        if (dy < 0)
            dy = -dy;

        int thresholdX = GetSystemMetrics(SM_CXDRAG);
        int thresholdY = GetSystemMetrics(SM_CYDRAG);
        if (thresholdX <= 0)
            thresholdX = 4;
        if (thresholdY <= 0)
            thresholdY = 4;

        if (dx >= thresholdX || dy >= thresholdY)
        {
            Dragging = true;
            if (HWindow != NULL && GetCapture() != HWindow)
                SetCapture(HWindow);
            if (MainWindow != NULL && DragSourceIndex >= 0)
            {
                POINT screenPt = pt;
                if (HWindow != NULL)
                    ClientToScreen(HWindow, &screenPt);
                MainWindow->OnPanelTabDragStarted(Side, DragSourceIndex);
                externalTarget = MainWindow->OnPanelTabDragUpdated(Side, DragSourceIndex, screenPt);
                mainWindowUpdated = true;
            }
        }
    }
    else if (MainWindow != NULL && DragSourceIndex >= 0)
    {
        POINT screenPt = pt;
        if (HWindow != NULL)
            ClientToScreen(HWindow, &screenPt);
        externalTarget = MainWindow->OnPanelTabDragUpdated(Side, DragSourceIndex, screenPt);
        mainWindowUpdated = true;
    }

    if (!Dragging)
    {
        DragHasExternalTarget = false;
        return;
    }

    bool newExternalTarget = mainWindowUpdated && externalTarget;
    DragHasExternalTarget = newExternalTarget;

    if (DragHasExternalTarget)
    {
        DragCurrentTarget = -1;
        ClearInsertMark();
        return;
    }

    UpdateDragIndicator(pt);
}

void CTabWindow::FinishDragTracking(const POINT& pt, bool canceled)
{
    CALL_STACK_MESSAGE_NONE
    if (!DragTracking)
        return;

    bool wasDragging = Dragging;
    int sourceIndex = DragSourceIndex;
    if (sourceIndex < 0)
        wasDragging = false;

    if (HWindow != NULL && GetCapture() == HWindow)
        ReleaseCapture();

    int lastTarget = DragCurrentTarget;
    ClearInsertMark();
    DragCurrentTarget = -1;

    DragTracking = false;
    Dragging = false;
    DragSourceIndex = -1;
    DragHasExternalTarget = false;

    bool movedToOtherSide = false;
    if (MainWindow != NULL)
    {
        if (!canceled && wasDragging && sourceIndex >= 0)
        {
            POINT screenPt = pt;
            if (HWindow != NULL)
                ClientToScreen(HWindow, &screenPt);
            movedToOtherSide = MainWindow->TryCompletePanelTabDrag(Side, sourceIndex, screenPt);
        }
        MainWindow->CancelPanelTabDrag();
    }

    if (movedToOtherSide || canceled || !wasDragging)
        return;

    int targetIndex = ComputeDragTargetIndex(pt, sourceIndex);
    if (targetIndex < 0)
        targetIndex = lastTarget;
    if (targetIndex < 0 || targetIndex == sourceIndex)
        return;

    MoveTabInternal(sourceIndex, targetIndex);
}

void CTabWindow::CancelDragTracking()
{
    CALL_STACK_MESSAGE_NONE
    if (!DragTracking)
        return;

    if (HWindow != NULL && GetCapture() == HWindow)
        ReleaseCapture();

    ClearInsertMark();
    DragCurrentTarget = -1;

    DragTracking = false;
    Dragging = false;
    DragSourceIndex = -1;
    DragHasExternalTarget = false;

    if (MainWindow != NULL)
        MainWindow->CancelPanelTabDrag();
}

void CTabWindow::UpdateDragIndicator(const POINT& pt)
{
    CALL_STACK_MESSAGE_NONE
    if (!Dragging || HWindow == NULL)
    {
        ClearInsertMark();
        DragCurrentTarget = -1;
        return;
    }

    if (DragHasExternalTarget)
    {
        ClearInsertMark();
        DragCurrentTarget = -1;
        return;
    }

    int targetIndex = -1;
    int markItem = -1;
    DWORD markFlags = 0;
    if (ComputeDragTargetInfo(pt, DragSourceIndex, targetIndex, markItem, markFlags))
    {
        DragCurrentTarget = targetIndex;
        SetInsertMark(markItem, markFlags);
    }
    else
    {
        DragCurrentTarget = -1;
        ClearInsertMark();
    }
}

void CTabWindow::SetInsertMark(int item, DWORD flags)
{
    CALL_STACK_MESSAGE_NONE
    if (DragInsertMarkItem == item && DragInsertMarkFlags == flags)
    {
        UpdateInsertMarkRect();
        return;
    }

    DragInsertMarkItem = item;
    DragInsertMarkFlags = flags;

    if (HWindow != NULL)
    {
        TCINSERTMARK mark;
        mark.cbSize = sizeof(mark);
        mark.dwFlags = flags;
        mark.iItem = item;
        SendMessage(HWindow, TCM_SETINSERTMARK, 0, (LPARAM)&mark);
    }

    UpdateInsertMarkRect();
}

void CTabWindow::ClearInsertMark()
{
    CALL_STACK_MESSAGE_NONE
    if (DragInsertMarkItem == -1 && DragInsertMarkFlags == 0)
    {
        UpdateInsertMarkRect();
        return;
    }

    DragInsertMarkItem = -1;
    DragInsertMarkFlags = 0;

    if (HWindow != NULL)
    {
        TCINSERTMARK mark;
        mark.cbSize = sizeof(mark);
        mark.dwFlags = 0;
        mark.iItem = -1;
        SendMessage(HWindow, TCM_SETINSERTMARK, 0, (LPARAM)&mark);
    }

    UpdateInsertMarkRect();
}

void CTabWindow::UpdateInsertMarkRect()
{
    CALL_STACK_MESSAGE_NONE
    RECT oldRect = DragIndicatorRect;
    bool oldVisible = DragIndicatorVisible;

    DragIndicatorVisible = false;
    SetRectEmpty(&DragIndicatorRect);

    if (HWindow != NULL && DragInsertMarkItem >= 0)
    {
        RECT itemRect;
        if (TabCtrl_GetItemRect(HWindow, DragInsertMarkItem, &itemRect))
        {
            RECT indicatorRect = itemRect;

            int verticalMargin = EnvFontCharHeight / 6;
            if (verticalMargin < 2)
                verticalMargin = 2;
            if (verticalMargin * 2 >= (indicatorRect.bottom - indicatorRect.top))
                verticalMargin = 0;

            indicatorRect.top += verticalMargin;
            indicatorRect.bottom -= verticalMargin;

            int indicatorWidth = EnvFontCharHeight / 4;
            if (indicatorWidth < 4)
                indicatorWidth = 4;
            if (indicatorWidth > 12)
                indicatorWidth = 12;

            int center = (DragInsertMarkFlags == TCIMF_AFTER) ? itemRect.right : itemRect.left;
            indicatorRect.left = center - indicatorWidth / 2;
            indicatorRect.right = indicatorRect.left + indicatorWidth;

            int expandLimit = indicatorWidth;
            if (indicatorRect.left < itemRect.left - expandLimit)
                indicatorRect.left = itemRect.left - expandLimit;
            if (indicatorRect.right > itemRect.right + expandLimit)
                indicatorRect.right = itemRect.right + expandLimit;

            if (indicatorRect.left < 0)
                indicatorRect.left = 0;
            if (indicatorRect.right <= indicatorRect.left)
                indicatorRect.right = indicatorRect.left + indicatorWidth;

            DragIndicatorRect = indicatorRect;
            DragIndicatorVisible = true;
        }
    }

    if (HWindow != NULL)
    {
        bool sameRect = oldVisible && DragIndicatorVisible &&
                        oldRect.left == DragIndicatorRect.left &&
                        oldRect.top == DragIndicatorRect.top &&
                        oldRect.right == DragIndicatorRect.right &&
                        oldRect.bottom == DragIndicatorRect.bottom;

        if (oldVisible && !sameRect)
            InvalidateRect(HWindow, &oldRect, FALSE);
        if (DragIndicatorVisible && (!oldVisible || !sameRect))
            InvalidateRect(HWindow, &DragIndicatorRect, FALSE);
    }
}

void CTabWindow::PaintDragIndicator(HDC hdc) const
{
    CALL_STACK_MESSAGE_NONE
    if (!DragIndicatorVisible)
        return;
    if (hdc == NULL)
        return;

    RECT rect = DragIndicatorRect;
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    COLORREF baseColor = GetSysColor(COLOR_HIGHLIGHT);
    COLORREF fillColor = LightenColor(baseColor, 96);
    COLORREF borderColor = DarkenColor(baseColor, 64);

    HBRUSH fillBrush = CreateSolidBrush(fillColor);
    if (fillBrush != NULL)
    {
        FillRect(hdc, &rect, fillBrush);
        DeleteObject(fillBrush);
    }

    HPEN borderPen = CreatePen(PS_SOLID, 1, borderColor);
    if (borderPen != NULL)
    {
        HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
        MoveToEx(hdc, rect.left, rect.top, NULL);
        LineTo(hdc, rect.left, rect.bottom - 1);
        LineTo(hdc, rect.right - 1, rect.bottom - 1);
        LineTo(hdc, rect.right - 1, rect.top);
        LineTo(hdc, rect.left, rect.top);
        if (oldPen != NULL)
            SelectObject(hdc, oldPen);
        DeleteObject(borderPen);
    }

    int centerX = (rect.left + rect.right) / 2;
    int capLength = (rect.right - rect.left) * 2;
    if (capLength < 6)
        capLength = 6;
    if (capLength > 18)
        capLength = 18;

    HPEN capPen = CreatePen(PS_SOLID, 1, borderColor);
    if (capPen != NULL)
    {
        HPEN oldPen = (HPEN)SelectObject(hdc, capPen);
        MoveToEx(hdc, centerX, rect.top, NULL);
        LineTo(hdc, centerX, rect.bottom - 1);
        MoveToEx(hdc, centerX - capLength / 2, rect.top, NULL);
        LineTo(hdc, centerX + capLength / 2, rect.top);
        MoveToEx(hdc, centerX - capLength / 2, rect.bottom - 1, NULL);
        LineTo(hdc, centerX + capLength / 2, rect.bottom - 1);
        if (oldPen != NULL)
            SelectObject(hdc, oldPen);
        DeleteObject(capPen);
    }
}

bool CTabWindow::ComputeDragTargetInfo(POINT pt, int fromIndex, int& targetIndex, int& markItem, DWORD& markFlags) const
{
    CALL_STACK_MESSAGE_NONE
    targetIndex = -1;
    markItem = -1;
    markFlags = 0;

    if (HWindow == NULL)
        return false;

    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex <= 1)
        return false;

    if (fromIndex <= 0 || fromIndex >= newTabIndex)
        return false;

    int hit = HitTest(pt);
    if (hit >= 0)
    {
        if (hit <= 0)
            hit = 1;
        if (hit >= newTabIndex)
            hit = newTabIndex - 1;
        if (IsNewTabButtonIndex(hit))
            hit = newTabIndex - 1;

        if (hit == fromIndex)
        {
            RECT fromRect;
            if (TabCtrl_GetItemRect(HWindow, fromIndex, &fromRect))
            {
                int center = (fromRect.left + fromRect.right) / 2;
                if (pt.x < center && fromIndex > 1)
                    hit = fromIndex - 1;
                else if (pt.x > center && fromIndex < newTabIndex - 1)
                    hit = fromIndex + 1;
            }
        }

        if (hit != fromIndex)
        {
            if (hit < fromIndex)
            {
                if (hit < 1)
                    hit = 1;
                if (hit >= newTabIndex)
                    hit = newTabIndex - 1;
                targetIndex = hit;
                if (targetIndex < 1)
                    targetIndex = 1;
                if (targetIndex == fromIndex)
                    return false;

                markItem = targetIndex;
                markFlags = TCIMF_BEFORE;
            }
            else
            {
                if (hit >= newTabIndex)
                    hit = newTabIndex - 1;
                if (hit <= fromIndex)
                    return false;

                targetIndex = hit;
                markItem = hit;
                markFlags = TCIMF_AFTER;
            }

            if (targetIndex >= 1 && targetIndex < newTabIndex && targetIndex != fromIndex)
                return true;
        }
    }

    RECT previousRect;
    if (!TabCtrl_GetItemRect(HWindow, 0, &previousRect))
        return false;

    int slotIndex = newTabIndex;
    for (int index = 1; index <= newTabIndex; ++index)
    {
        RECT currentRect;
        if (!TabCtrl_GetItemRect(HWindow, index, &currentRect))
            continue;

        int boundary = (previousRect.right + currentRect.left) / 2;
        if (pt.x < boundary)
        {
            slotIndex = index;
            break;
        }

        previousRect = currentRect;
    }

    if (slotIndex <= 0)
        slotIndex = 1;
    if (slotIndex > newTabIndex)
        slotIndex = newTabIndex;

    int finalIndex = slotIndex;
    if (finalIndex > fromIndex)
        finalIndex--;

    if (finalIndex <= 0 || finalIndex >= newTabIndex || finalIndex == fromIndex)
        return false;

    targetIndex = finalIndex;

    if (targetIndex < fromIndex)
    {
        markItem = targetIndex;
        markFlags = TCIMF_BEFORE;
    }
    else
    {
        markItem = targetIndex;
        markFlags = TCIMF_AFTER;
        if (markItem >= newTabIndex)
            markItem = newTabIndex - 1;
        if (markItem < 1)
            markItem = 1;
    }

    return true;
}

int CTabWindow::ComputeDragTargetIndex(POINT pt, int fromIndex) const
{
    CALL_STACK_MESSAGE_NONE
    int targetIndex = -1;
    int markItem = -1;
    DWORD markFlags = 0;
    if (!ComputeDragTargetInfo(pt, fromIndex, targetIndex, markItem, markFlags))
        return -1;

    return targetIndex;
}

bool CTabWindow::ComputeExternalDropTarget(POINT screenPt, int& targetIndex, int& markItem, DWORD& markFlags) const
{
    CALL_STACK_MESSAGE_NONE
    targetIndex = -1;
    markItem = -1;
    markFlags = 0;

    if (HWindow == NULL)
        return false;

    POINT pt = screenPt;
    ScreenToClient(HWindow, &pt);

    RECT clientRect;
    if (!GetClientRect(HWindow, &clientRect))
        return false;

    int verticalMargin = EnvFontCharHeight / 2;
    if (verticalMargin < 6)
        verticalMargin = 6;
    int horizontalMargin = EnvFontCharHeight;
    if (horizontalMargin < 12)
        horizontalMargin = 12;
    InflateRect(&clientRect, horizontalMargin, verticalMargin);

    if (pt.x < clientRect.left || pt.x > clientRect.right || pt.y < clientRect.top || pt.y > clientRect.bottom)
        return false;

    int tabCount = GetTabCount();
    if (tabCount <= 0)
        return false;

    RECT previousRect;
    if (!TabCtrl_GetItemRect(HWindow, 0, &previousRect))
        return false;

    int slotIndex = tabCount;
    for (int index = 1; index < tabCount; ++index)
    {
        RECT currentRect;
        if (!TabCtrl_GetItemRect(HWindow, index, &currentRect))
            continue;

        int boundary = (previousRect.right + currentRect.left) / 2;
        if (pt.x < boundary)
        {
            slotIndex = index;
            break;
        }

        previousRect = currentRect;
    }

    if (slotIndex < 1)
        slotIndex = 1;
    if (slotIndex > tabCount)
        slotIndex = tabCount;

    targetIndex = slotIndex;

    if (slotIndex >= tabCount)
    {
        markItem = tabCount - 1;
        if (markItem < 0)
            markItem = 0;
        markFlags = TCIMF_AFTER;
    }
    else
    {
        markItem = slotIndex;
        if (markItem < 1)
            markItem = 1;
        markFlags = TCIMF_BEFORE;
    }

    return true;
}

void CTabWindow::ShowExternalDropIndicator(int markItem, DWORD markFlags)
{
    CALL_STACK_MESSAGE_NONE
    SetInsertMark(markItem, markFlags);
}

void CTabWindow::HideExternalDropIndicator()
{
    CALL_STACK_MESSAGE_NONE
    ClearInsertMark();
}

void CTabWindow::MoveTabInternal(int from, int to)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return;

    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex <= 0)
        return;

    if (from <= 0 || from >= newTabIndex)
        return;
    if (to <= 0 || to >= newTabIndex)
        return;
    if (from == to)
        return;

    wchar_t textBuffer[512];
    textBuffer[0] = L'\0';

    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE | TCIF_STATE;
    item.pszText = textBuffer;
    item.cchTextMax = _countof(textBuffer);
    item.dwStateMask = 0xFFFFFFFF;

    if (!SendMessageW(HWindow, TCM_GETITEMW, from, (LPARAM)&item))
        return;

    std::wstring text(textBuffer);
    item.pszText = text.empty() ? const_cast<LPWSTR>(L"") : &text[0];
    item.cchTextMax = (int)text.length() + 1;
    item.dwStateMask = 0xFFFFFFFF;
    item.mask = TCIF_TEXT | TCIF_PARAM | TCIF_IMAGE | TCIF_STATE;

    int insertIndex = to;

    {
        CSelChangeGuard guard(SuppressSelectionNotifications);

        SendMessage(HWindow, TCM_DELETEITEM, from, 0);
        SendMessageW(HWindow, TCM_INSERTITEMW, insertIndex, (LPARAM)&item);
        TabCtrl_SetCurSel(HWindow, insertIndex);
    }

    MoveTabColor(from, insertIndex);

    if (MainWindow != NULL)
        MainWindow->OnPanelTabReordered(Side, from, insertIndex);
}

void CTabWindow::SetTabColor(int index, COLORREF color)
{
    CALL_STACK_MESSAGE_NONE
    STabColor* entry = GetTabColor(index);
    if (entry != NULL)
    {
        entry->Valid = true;
        entry->Color = color;
    }
    InvalidateTab(index);
}

void CTabWindow::ClearTabColor(int index)
{
    CALL_STACK_MESSAGE_NONE
    STabColor* entry = GetTabColor(index);
    if (entry != NULL)
        entry->Valid = false;
    InvalidateTab(index);
}

void CTabWindow::InvalidateTab(int index)
{
    if (HWindow == NULL)
        return;
    if (index < 0)
        return;
    RECT rect;
    if (TabCtrl_GetItemRect(HWindow, index, &rect))
        InvalidateRect(HWindow, &rect, FALSE);
    else
        InvalidateRect(HWindow, NULL, FALSE);
}

void CTabWindow::EnsureTabColorCapacity()
{
    int count = GetTabCount();
    if (count < 0)
        count = 0;
    if ((int)TabColors.size() < count)
    {
        STabColor empty = {false, RGB(0, 0, 0)};
        TabColors.resize(count, empty);
    }
    else if ((int)TabColors.size() > count)
    {
        TabColors.resize(count);
    }
}

void CTabWindow::InsertTabColorSlot(int index, int currentCount)
{
    if (currentCount < 0)
        currentCount = 0;
    if ((int)TabColors.size() < currentCount)
    {
        STabColor empty = {false, RGB(0, 0, 0)};
        TabColors.resize(currentCount, empty);
    }
    else if ((int)TabColors.size() > currentCount)
    {
        TabColors.resize(currentCount);
    }
    STabColor empty = {false, RGB(0, 0, 0)};
    if (index < 0)
        index = 0;
    if (index > (int)TabColors.size())
        index = (int)TabColors.size();
    TabColors.insert(TabColors.begin() + index, empty);
}

void CTabWindow::RemoveTabColorSlot(int index)
{
    if (index < 0 || index >= (int)TabColors.size())
        return;
    TabColors.erase(TabColors.begin() + index);
}

void CTabWindow::MoveTabColor(int from, int to)
{
    EnsureTabColorCapacity();
    if (from < 0 || from >= (int)TabColors.size())
        return;
    if (to < 0)
        to = 0;
    if (to > (int)TabColors.size())
        to = (int)TabColors.size();
    STabColor entry = TabColors[from];
    TabColors.erase(TabColors.begin() + from);
    if (to > (int)TabColors.size())
        to = (int)TabColors.size();
    TabColors.insert(TabColors.begin() + to, entry);
}

CTabWindow::STabColor* CTabWindow::GetTabColor(int index)
{
    if (index < 0)
        return NULL;
    EnsureTabColorCapacity();
    if (index < 0 || index >= (int)TabColors.size())
        return NULL;
    return &TabColors[index];
}

const CTabWindow::STabColor* CTabWindow::GetTabColor(int index) const
{
    if (index < 0)
        return NULL;
    const_cast<CTabWindow*>(this)->EnsureTabColorCapacity();
    if (index < 0 || index >= (int)TabColors.size())
        return NULL;
    return &TabColors[index];
}

bool CTabWindow::HasAnyCustomTabColors() const
{
    int total = GetDisplayedTabCount();
    for (int i = 0; i < total; ++i)
    {
        if (IsNewTabButtonIndex(i))
            continue;
        COLORREF color;
        if (TryResolveTabColor(i, color))
            return true;
    }
    return false;
}

bool CTabWindow::TryResolveTabColor(int index, COLORREF& color) const
{
    if (index < 0)
        return false;

    CFilesWindow* panel = reinterpret_cast<CFilesWindow*>(GetItemData(index));
    if (panel != NULL && panel->HasCustomTabColor())
    {
        color = panel->GetCustomTabColor();
        return true;
    }

    const STabColor* entry = GetTabColor(index);
    if (entry != NULL && entry->Valid)
    {
        color = entry->Color;
        return true;
    }

    if (MainWindow != NULL)
    {
        CFilesWindow* fallback = MainWindow->GetPanelTabAt(Side, index);
        if (fallback != NULL && fallback->HasCustomTabColor())
        {
            color = fallback->GetCustomTabColor();
            return true;
        }
    }

    return false;
}

void CTabWindow::PaintCustomTabs(HDC hdc, const RECT* clipRect) const
{
    if (hdc == NULL)
        return;
    if (HWindow == NULL)
        return;

    int total = GetDisplayedTabCount();
    if (total <= 0)
        return;

    int selected = TabCtrl_GetCurSel(HWindow);
    int focus = TabCtrl_GetCurFocus(HWindow);
    HWND focusWnd = GetFocus();

    for (int i = 0; i < total; ++i)
    {
        if (IsNewTabButtonIndex(i))
            continue;

        COLORREF baseColor;
        if (!TryResolveTabColor(i, baseColor))
            continue;

        RECT itemRect;
        if (!TabCtrl_GetItemRect(HWindow, i, &itemRect))
            continue;

        if (clipRect != NULL)
        {
            RECT intersection;
            if (!IntersectRect(&intersection, &itemRect, clipRect))
                continue;
        }

        wchar_t textBuffer[512];
        textBuffer[0] = L'\0';

        TCITEMW item;
        ZeroMemory(&item, sizeof(item));
        item.mask = TCIF_TEXT | TCIF_STATE;
        item.pszText = textBuffer;
        item.cchTextMax = _countof(textBuffer);
        item.dwStateMask = 0xFFFFFFFF;
        if (!SendMessageW(HWindow, TCM_GETITEMW, i, (LPARAM)&item))
            textBuffer[0] = L'\0';

        bool isSelected = (i == selected);
        bool isHot = (item.dwState & TCIS_HIGHLIGHTED) != 0;
        bool hasFocus = (focus == i) && (focusWnd == HWindow);

        DrawColoredTab(hdc, itemRect, textBuffer, baseColor, isSelected, isHot, hasFocus);
    }
}

void CTabWindow::DrawColoredTab(HDC hdc, const RECT& itemRect, const wchar_t* text, COLORREF baseColor,
                                bool selected, bool hot, bool hasFocus) const
{
    if (hdc == NULL)
        return;

    RECT rect = itemRect;
    RECT fillRect = rect;
    if (!selected)
    {
        InflateRect(&fillRect, -1, -1);
        if (fillRect.right <= fillRect.left || fillRect.bottom <= fillRect.top)
            fillRect = rect;
    }

    COLORREF fillColor = baseColor;
    if (selected)
        fillColor = LightenColor(fillColor, 96);
    else if (hot)
        fillColor = LightenColor(fillColor, 48);

    HBRUSH brush = CreateSolidBrush(fillColor);
    if (brush != NULL)
    {
        FillRect(hdc, &fillRect, brush);
        DeleteObject(brush);
    }

    COLORREF borderColor = DarkenColor(baseColor, 80);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    if (pen != NULL)
    {
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        int radius = EnvFontCharHeight / 2;
        if (radius < 3)
            radius = 3;
        RoundRect(hdc, fillRect.left, fillRect.top, fillRect.right, fillRect.bottom, radius, radius);
        if (oldBrush != NULL)
            SelectObject(hdc, oldBrush);
        if (oldPen != NULL)
            SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    RECT textRect = fillRect;
    InflateRect(&textRect, -4, -2);
    if (textRect.right <= textRect.left)
    {
        textRect.left = fillRect.left;
        textRect.right = fillRect.right;
    }
    if (textRect.bottom <= textRect.top)
    {
        textRect.top = fillRect.top;
        textRect.bottom = fillRect.bottom;
    }

    HFONT oldFont = NULL;
    if (EnvFont != NULL)
        oldFont = (HFONT)SelectObject(hdc, EnvFont);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF textColor = IsColorDark(fillColor) ? RGB(255, 255, 255) : RGB(0, 0, 0);
    COLORREF oldTextColor = SetTextColor(hdc, textColor);

    const wchar_t* drawText = (text != NULL) ? text : L"";
    DrawTextW(hdc, drawText, -1, &textRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    if (hasFocus)
    {
        RECT focusRect = fillRect;
        InflateRect(&focusRect, -3, -3);
        DrawFocusRect(hdc, &focusRect);
    }

    if (oldTextColor != CLR_INVALID)
        SetTextColor(hdc, oldTextColor);
    SetBkMode(hdc, oldBkMode);
    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
}

LRESULT CTabWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTabWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_PAINT:
    {
        RECT updateRect;
        BOOL hasUpdate = (HWindow != NULL) ? GetUpdateRect(HWindow, &updateRect, FALSE) : FALSE;
        LRESULT baseResult = CWindow::WindowProc(uMsg, wParam, lParam);
        if (HWindow != NULL)
        {
            bool hasCustomColors = HasAnyCustomTabColors();
            bool shouldPaintIndicator = DragIndicatorVisible;
            if (hasCustomColors || shouldPaintIndicator)
            {
                HDC hdc = GetDC(HWindow);
                if (hdc != NULL)
                {
                    int saved = SaveDC(hdc);
                    if (hasUpdate)
                        IntersectClipRect(hdc, updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);
                    if (hasCustomColors)
                        PaintCustomTabs(hdc, hasUpdate ? &updateRect : NULL);
                    if (shouldPaintIndicator)
                        PaintDragIndicator(hdc);
                    RestoreDC(hdc, saved);
                    ReleaseDC(HWindow, hdc);
                }
            }
        }
        return baseResult;
    }

    case WM_PRINTCLIENT:
    {
        LRESULT baseResult = CWindow::WindowProc(uMsg, wParam, lParam);
        if (HasAnyCustomTabColors())
        {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            if (hdc != NULL)
            {
                int saved = SaveDC(hdc);
                PaintCustomTabs(hdc, NULL);
                RestoreDC(hdc, saved);
            }
        }
        return baseResult;
    }

    case WM_LBUTTONDOWN:
    {
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt;
        pt.x = pts.x;
        pt.y = pts.y;
        int hit = HitTest(pt);
        if (IsReorderableIndex(hit))
            StartDragTracking(hit, pt);
        else
            CancelDragTracking();
        break;
    }

    case WM_MBUTTONDOWN:
    {
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt;
        pt.x = pts.x;
        pt.y = pts.y;
        int hit = HitTest(pt);
        if (hit > 0 && !IsNewTabButtonIndex(hit) && MainWindow != NULL)
        {
            CFilesWindow* panel = MainWindow->GetPanelTabAt(Side, hit);
            if (panel != NULL)
                MainWindow->ClosePanelTab(panel);
        }
        return 0;
    }

    case WM_MOUSEMOVE:
        if (DragTracking)
        {
            POINTS pts = MAKEPOINTS(lParam);
            POINT pt;
            pt.x = pts.x;
            pt.y = pts.y;
            UpdateDragTracking(pt);
        }
        break;

    case WM_LBUTTONUP:
    {
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt;
        pt.x = pts.x;
        pt.y = pts.y;
        FinishDragTracking(pt, false);
        break;
    }

    case WM_CAPTURECHANGED:
        if ((HWND)lParam != HWindow)
            CancelDragTracking();
        break;

    case WM_CANCELMODE:
        CancelDragTracking();
        break;
    }

    return CWindow::WindowProc(uMsg, wParam, lParam);
}
