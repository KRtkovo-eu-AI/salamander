// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <commctrl.h>
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
    DragCurrentTarget = -1;
    DragInsertMarkItem = -1;
    DragInsertMarkFlags = 0;
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
    int result;
    {
        CSelChangeGuard guard(SuppressSelectionNotifications);
        result = (int)SendMessageW(HWindow, TCM_INSERTITEMW, insertIndex, (LPARAM)&item);
    }
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
        {
            CSelChangeGuard guard(SuppressSelectionNotifications);
            SendMessage(HWindow, TCM_DELETEITEM, index, 0);
        }
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

    case NM_CUSTOMDRAW:
    {
        LPNMCUSTOMDRAW draw = reinterpret_cast<LPNMCUSTOMDRAW>(nmhdr);
        if (draw == NULL)
            return FALSE;

        switch (draw->dwDrawStage)
        {
        case CDDS_PREPAINT:
            result = CDRF_NOTIFYITEMDRAW;
            return TRUE;

        case CDDS_ITEMPREPAINT:
        {
            int index = static_cast<int>(draw->dwItemSpec);
            if (index < 0)
            {
                result = CDRF_DODEFAULT;
                return TRUE;
            }
            if (IsNewTabButtonIndex(index))
            {
                result = CDRF_DODEFAULT;
                return TRUE;
            }
            TCITEMW paramItem;
            ZeroMemory(&paramItem, sizeof(paramItem));
            paramItem.mask = TCIF_PARAM;
            if (!SendMessageW(HWindow, TCM_GETITEMW, index, (LPARAM)&paramItem))
            {
                result = CDRF_DODEFAULT;
                return TRUE;
            }
            if (paramItem.lParam == kNewTabButtonParam)
            {
                result = CDRF_DODEFAULT;
                return TRUE;
            }

            CFilesWindow* panel = reinterpret_cast<CFilesWindow*>(paramItem.lParam);
            if (panel == NULL || !panel->HasCustomTabColor())
            {
                result = CDRF_DODEFAULT;
                return TRUE;
            }

            RECT rect = draw->rc;
            COLORREF baseColor = panel->GetCustomTabColor();
            bool selected = (draw->uItemState & CDIS_SELECTED) != 0;
            bool hot = (draw->uItemState & CDIS_HOT) != 0;

            COLORREF fillColor = baseColor;
            if (selected)
                fillColor = LightenColor(fillColor, 96);
            else if (hot)
                fillColor = LightenColor(fillColor, 48);

            HBRUSH brush = CreateSolidBrush(fillColor);
            if (brush != NULL)
            {
                FillRect(draw->hdc, &rect, brush);
                DeleteObject(brush);
            }

            COLORREF borderColor = DarkenColor(baseColor, 80);
            HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
            HPEN oldPen = NULL;
            HBRUSH oldBrush = NULL;
            if (pen != NULL)
            {
                oldPen = (HPEN)SelectObject(draw->hdc, pen);
                oldBrush = (HBRUSH)SelectObject(draw->hdc, GetStockObject(NULL_BRUSH));
                int radius = EnvFontCharHeight / 2;
                if (radius < 3)
                    radius = 3;
                RoundRect(draw->hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
                if (oldBrush != NULL)
                    SelectObject(draw->hdc, oldBrush);
                if (oldPen != NULL)
                    SelectObject(draw->hdc, oldPen);
                DeleteObject(pen);
            }

            RECT textRect = rect;
            InflateRect(&textRect, -4, -2);

            wchar_t textBuffer[512];
            textBuffer[0] = L'\0';
            TCITEMW item;
            ZeroMemory(&item, sizeof(item));
            item.mask = TCIF_TEXT;
            item.pszText = textBuffer;
            item.cchTextMax = _countof(textBuffer);
            if (!SendMessageW(HWindow, TCM_GETITEMW, index, (LPARAM)&item))
                textBuffer[0] = L'\0';

            HFONT oldFont = NULL;
            if (EnvFont != NULL)
                oldFont = (HFONT)SelectObject(draw->hdc, EnvFont);
            int oldBkMode = SetBkMode(draw->hdc, TRANSPARENT);
            COLORREF textColor = IsColorDark(fillColor) ? RGB(255, 255, 255) : RGB(0, 0, 0);
            COLORREF oldTextColor = SetTextColor(draw->hdc, textColor);
            DrawTextW(draw->hdc, textBuffer, -1, &textRect,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
            if (oldTextColor != CLR_INVALID)
                SetTextColor(draw->hdc, oldTextColor);
            SetBkMode(draw->hdc, oldBkMode);
            if (oldFont != NULL)
                SelectObject(draw->hdc, oldFont);

            result = CDRF_SKIPDEFAULT;
            return TRUE;
        }
        }
        break;
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
    DragCurrentTarget = -1;
    ClearInsertMark();
}

void CTabWindow::UpdateDragTracking(const POINT& pt)
{
    CALL_STACK_MESSAGE_NONE
    if (!DragTracking)
        return;

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
        }
    }

    if (Dragging)
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

    if (canceled || !wasDragging)
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
        return;

    DragInsertMarkItem = item;
    DragInsertMarkFlags = flags;

    if (HWindow == NULL)
        return;

    TCINSERTMARK mark;
    mark.cbSize = sizeof(mark);
    mark.dwFlags = flags;
    mark.iItem = item;
    SendMessage(HWindow, TCM_SETINSERTMARK, 0, (LPARAM)&mark);
}

void CTabWindow::ClearInsertMark()
{
    CALL_STACK_MESSAGE_NONE
    if (DragInsertMarkItem == -1 && DragInsertMarkFlags == 0)
        return;

    DragInsertMarkItem = -1;
    DragInsertMarkFlags = 0;

    if (HWindow == NULL)
        return;

    TCINSERTMARK mark;
    mark.cbSize = sizeof(mark);
    mark.dwFlags = 0;
    mark.iItem = -1;
    SendMessage(HWindow, TCM_SETINSERTMARK, 0, (LPARAM)&mark);
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

    if (MainWindow != NULL)
        MainWindow->OnPanelTabReordered(Side, from, insertIndex);
}

void CTabWindow::SetTabColor(int index, COLORREF color)
{
    CALL_STACK_MESSAGE_NONE
    (void)color;
    InvalidateTab(index);
}

void CTabWindow::ClearTabColor(int index)
{
    CALL_STACK_MESSAGE_NONE
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

LRESULT CTabWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTabWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
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
