// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "common/winlibdpi.h"

#include <commctrl.h>
#include <algorithm>
#include <string>

#include "tabwnd.h"
#include "mainwnd.h"
#include "fileswnd.h"
#include "consts.h"
#include "darkmode.h"

#ifndef TCM_SETINSERTMARK
#define TCM_SETINSERTMARK (TCM_FIRST + 44)
#endif

#ifndef TCM_SETCURFOCUS
#define TCM_SETCURFOCUS (TCM_FIRST + 48)
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
    constexpr wchar_t kTabWidthPaddingChar = L'\x2007';
    const wchar_t kNewTabButtonText[] = L"+";
    const wchar_t kEllipsisText[] = L"...";

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
            HFONT font = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
            if (font != NULL)
                oldFont = (HFONT)SelectObject(hdc, font);
            SIZE size = {0, 0};
            if (GetTextExtentPoint32W(hdc, kNewTabButtonText, _countof(kNewTabButtonText) - 1, &size))
                minWidth = size.cx;
            TEXTMETRIC tm;
            int fontHeight = GetTextMetrics(hdc, &tm) ? tm.tmHeight : EnvFontCharHeight;
            if (oldFont != NULL)
                SelectObject(hdc, oldFont);
            ReleaseDC(hwnd, hdc);
            if (minWidth <= 0)
                minWidth = fontHeight;
            int padding = fontHeight / 2;
            if (padding < 4)
                padding = 4;
            return minWidth + padding;
        }
        return EnvFontCharHeight + max(4, EnvFontCharHeight / 2);
    }

    void TrimTabWidthPadding(std::wstring& text)
    {
        while (!text.empty() && text[text.length() - 1] == kTabWidthPaddingChar)
            text.erase(text.length() - 1);
    }

    std::wstring EllipsizeTextToWidth(const std::wstring& text, HDC hdc, int maxWidth)
    {
        if (maxWidth <= 0)
            return std::wstring(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);
        if (text.empty())
            return text;

        SIZE textSize = {0, 0};
        if (!GetTextExtentPoint32W(hdc, text.c_str(), (int)text.length(), &textSize))
            return text;
        if (textSize.cx <= maxWidth)
            return text;

        SIZE ellipsisSize = {0, 0};
        if (!GetTextExtentPoint32W(hdc, kEllipsisText, _countof(kEllipsisText) - 1, &ellipsisSize))
            ellipsisSize.cx = 0;
        if (ellipsisSize.cx > maxWidth)
            return std::wstring(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);

        int low = 0;
        int high = (int)text.length();
        std::wstring best(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);
        while (low <= high)
        {
            int rawMid = (low + high) / 2;
            int mid = rawMid;
            if (mid > 0)
            {
                wchar_t ch = text[mid - 1];
                if (ch >= 0xD800 && ch <= 0xDBFF)
                    mid--;
            }

            std::wstring candidate;
            if (mid <= 0)
                candidate.assign(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);
            else
            {
                candidate.assign(text, 0, mid);
                candidate.append(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);
            }

            SIZE candidateSize = {0, 0};
            if (!GetTextExtentPoint32W(hdc, candidate.c_str(), (int)candidate.length(), &candidateSize))
            {
                high = mid - 1;
                continue;
            }

            if (candidateSize.cx <= maxWidth)
            {
                best = candidate;
                low = rawMid + 1;
            }
            else
                high = rawMid - 1;
        }

        return best;
    }

}

static char g_TabTipText[MAX_PATH] = "";
static bool g_TabTipClassRegistered = false;
static const UINT_PTR kTabTipTimerId = 1001;
static const int kTabTipDelayMs = 500;
static const UINT WM_USER_ENSURE_SELECTED_TAB_VISIBLE = WM_APP + 321;


static LRESULT CALLBACK TabTipWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, (HBRUSH)(COLOR_INFOBK + 1));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_INFOTEXT));
        HFONT hOldFont = (HFONT)SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
        DrawText(hdc, g_TabTipText, -1, &rc, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);
        SelectObject(hdc, hOldFont);
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
    LastClickedIndex = -1;
    LastClickWasSelected = false;
    MouseWheelAccumulator = 0;
    InitialEnsureSelectedTabVisiblePending = true;
    HTabTipWnd = NULL;
    TabTipTabIndex = -1;
    TabTipHoverIndex = -1;
    TabTipTracking = false;
    CloseButtonHoverIndex = -1;
    HDPIFont = NULL;
    HDPIFontBold = NULL;
    DPIFontHeight = 0;
}

CTabWindow::~CTabWindow()
{
    CALL_STACK_MESSAGE1("CTabWindow::~CTabWindow()");
    DestroyWindow();
    if (HDPIFont != NULL)
        HANDLES(DeleteObject(HDPIFont));
    if (HDPIFontBold != NULL)
        HANDLES(DeleteObject(HDPIFontBold));
}

BOOL CTabWindow::Create(HWND parent, int controlID)
{
    CALL_STACK_MESSAGE_NONE
    ControlID = controlID;
    DWORD style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TCS_TABS | TCS_FOCUSNEVER;
#ifndef _UNICODE
    HWND hwnd = CreateExW(0, WC_TABCONTROLW, L"", style, 0, 0, 0, 0, parent,
                          (HMENU)(INT_PTR)controlID, HInstance, this);
#else
    HWND hwnd = CreateEx(0, WC_TABCONTROL, "", style, 0, 0, 0, 0, parent,
                         (HMENU)(INT_PTR)controlID, HInstance, this);
#endif
    if (hwnd == NULL)
        return FALSE;
    DarkModePreserveCustomTabControl(HWindow);
    RefreshDPIResources();

    EnsureNewTabButton();
    return TRUE;
}

void CTabWindow::DestroyWindow()
{
    CALL_STACK_MESSAGE_NONE
    KillTimer(HWindow, kTabTipTimerId);
    HideTabToolTip();
    if (HTabTipWnd != NULL)
    {
        ::DestroyWindow(HTabTipWnd);
        HTabTipWnd = NULL;
    }
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
    int fontHeight = DPIFontHeight > 0 ? DPIFontHeight : EnvFontCharHeight;
    int verticalPadding = fontHeight / 8;
    if (verticalPadding < 2)
        verticalPadding = 2;
    return fontHeight + 2 * verticalPadding;
}

void CTabWindow::RefreshDPIResources()
{
    LOGFONT lf;
    HFONT font = WinLibDPIGetIconTitleLogFont(HWindow, &lf)
                     ? HANDLES(CreateFontIndirect(&lf))
                     : NULL;
    if (font == NULL)
        return;

    lf.lfWeight = FW_BOLD;
    HFONT bold = HANDLES(CreateFontIndirect(&lf));
    if (bold == NULL)
    {
        HANDLES(DeleteObject(font));
        return;
    }

    HDC dc = HANDLES(GetDC(HWindow));
    TEXTMETRIC tm;
    if (dc != NULL)
    {
        HFONT oldFont = (HFONT)SelectObject(dc, font);
        GetTextMetrics(dc, &tm);
        SelectObject(dc, oldFont);
        HANDLES(ReleaseDC(HWindow, dc));
    }
    else
        tm.tmHeight = EnvFontCharHeight;

    SendMessage(HWindow, WM_SETFONT, (WPARAM)font, FALSE);
    if (HDPIFont != NULL)
        HANDLES(DeleteObject(HDPIFont));
    if (HDPIFontBold != NULL)
        HANDLES(DeleteObject(HDPIFontBold));
    HDPIFont = font;
    HDPIFontBold = bold;
    DPIFontHeight = tm.tmHeight;
    RefreshLayout();
    InvalidateRect(HWindow, NULL, TRUE);
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
    SetTabText(insertIndex, text);
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
        {
            RemoveTabColorSlot(index);
            if (CloseButtonHoverIndex == index)
                CloseButtonHoverIndex = -1;
            else if (CloseButtonHoverIndex > index)
                CloseButtonHoverIndex--;
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
        TabColors.clear();
        CloseButtonHoverIndex = -1;
        EnsureNewTabButton();
    }
}

void CTabWindow::SetTabText(int index, const wchar_t* text)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL || index < 0 || index >= GetTabCount())
        return;
    std::wstring desired = (text != NULL) ? text : L"";

    auto setItemText = [&](const std::wstring& value) {
        TCITEMW item;
        ZeroMemory(&item, sizeof(item));
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPWSTR>(value.c_str());
        SendMessageW(HWindow, TCM_SETITEMW, index, (LPARAM)&item);
    };

    int minWidthPx = DipToPixels(Configuration.TabButtonMinWidth);
    int maxWidthPx = DipToPixels(Configuration.TabButtonMaxWidth);

    std::wstring finalText = desired;
    setItemText(finalText);
    bool ensureSelectedVisible = (index == TabCtrl_GetCurSel(HWindow));

    int closeBtnExtraPx = 0;
    if (ShouldShowCloseButton(index, TabCtrl_GetCurSel(HWindow)))
    {
        RECT tabRect;
        if (TabCtrl_GetItemRect(HWindow, index, &tabRect))
        {
            RECT closeRect = GetTabCloseButtonRect(tabRect);
            closeBtnExtraPx = (closeRect.right - closeRect.left) + 6;
        }
    }

    HDC hdc = GetDC(HWindow);
    if (hdc == NULL)
    {
        if (ensureSelectedVisible)
            EnsureSelectedTabVisible();
        return;
    }
    HFONT oldFont = NULL;
    bool selected = (index == TabCtrl_GetCurSel(HWindow));
    HFONT fontToUse = (selected && HDPIFontBold != NULL) ? HDPIFontBold :
                      (HDPIFont != NULL ? HDPIFont : EnvFont);
    if (fontToUse != NULL)
        oldFont = (HFONT)SelectObject(hdc, fontToUse);

    int desiredWidth = 0;
    if (!desired.empty())
    {
        SIZE desiredSize = {0, 0};
        if (GetTextExtentPoint32W(hdc, desired.c_str(), (int)desired.length(), &desiredSize))
            desiredWidth = desiredSize.cx;
    }

    RECT rect;
    if (!TabCtrl_GetItemRect(HWindow, index, &rect))
    {
        if (oldFont != NULL)
            SelectObject(hdc, oldFont);
        ReleaseDC(HWindow, hdc);
        if (ensureSelectedVisible)
            EnsureSelectedTabVisible();
        return;
    }
    int currentWidth = rect.right - rect.left;

    if (maxWidthPx > 0 && currentWidth > maxWidthPx && !desired.empty())
    {
        int extraWidth = currentWidth - desiredWidth;
        int allowedTextWidth = maxWidthPx - extraWidth - closeBtnExtraPx;
        if (allowedTextWidth <= 0)
            finalText.assign(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);
        else if (desiredWidth > allowedTextWidth)
            finalText = EllipsizeTextToWidth(desired, hdc, allowedTextWidth);

        for (int attempt = 0; attempt < 3; ++attempt)
        {
            setItemText(finalText);
            if (!TabCtrl_GetItemRect(HWindow, index, &rect))
                break;
            currentWidth = rect.right - rect.left;
            if (currentWidth <= maxWidthPx)
                break;

            allowedTextWidth -= (currentWidth - maxWidthPx);
            if (allowedTextWidth <= 0)
                finalText.assign(kEllipsisText, kEllipsisText + _countof(kEllipsisText) - 1);
            else
                finalText = EllipsizeTextToWidth(desired, hdc, allowedTextWidth);
        }
    }

    if (minWidthPx > 0 || closeBtnExtraPx > 0)
    {
        int targetWidth = minWidthPx;
        if (closeBtnExtraPx > 0)
        {
            if (targetWidth <= 0)
                targetWidth = currentWidth + closeBtnExtraPx;
            else
                targetWidth += closeBtnExtraPx;
        }
        if (maxWidthPx > 0 && targetWidth > maxWidthPx)
            targetWidth = maxWidthPx;

        for (int attempt = 0; attempt < 4; ++attempt)
        {
            if (!TabCtrl_GetItemRect(HWindow, index, &rect))
                break;
            currentWidth = rect.right - rect.left;
            if (currentWidth >= targetWidth)
                break;

            SIZE paddingSize = {0, 0};
            if (!GetTextExtentPoint32W(hdc, &kTabWidthPaddingChar, 1, &paddingSize) || paddingSize.cx <= 0)
                break;

            int addCount = (targetWidth - currentWidth + paddingSize.cx - 1) / paddingSize.cx;
            if (addCount <= 0)
                addCount = 1;
            finalText.append(addCount, kTabWidthPaddingChar);
            setItemText(finalText);
        }
    }

    if (oldFont != NULL)
        SelectObject(hdc, oldFont);
    ReleaseDC(HWindow, hdc);
    if (ensureSelectedVisible)
        EnsureSelectedTabVisible();
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
        EnsureInitialSelectedTabVisible();
    }
}

void CTabWindow::EnsureInitialSelectedTabVisible()
{
    CALL_STACK_MESSAGE_NONE
    if (!InitialEnsureSelectedTabVisiblePending || HWindow == NULL)
        return;

    int sel = TabCtrl_GetCurSel(HWindow);
    if (sel < 0 || IsNewTabButtonIndex(sel) || !IsWindowVisible(HWindow))
        return;

    EnsureSelectedTabVisible();
    InitialEnsureSelectedTabVisiblePending = false;
}

void CTabWindow::EnsureSelectedTabVisible()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return;

    int sel = TabCtrl_GetCurSel(HWindow);
    if (sel < 0 || IsNewTabButtonIndex(sel))
        return;

    HWND upDown = FindWindowEx(HWindow, NULL, UPDOWN_CLASS, NULL);
    if (upDown == NULL || !IsWindowVisible(upDown))
        return;

    CSelChangeGuard guard(SuppressSelectionNotifications);

    // TCM_SETCURFOCUS can move the native tab strip close to the selected tab,
    // but during startup/restored layouts it is not reliable enough by itself.
    // Use it as a cheap first attempt, then fall back to clicking the native
    // overflow arrows until the selected tab rectangle is inside the visible
    // strip area.
    SendMessage(HWindow, TCM_SETCURFOCUS, sel, 0);
    if (TabCtrl_GetCurSel(HWindow) != sel)
        TabCtrl_SetCurSel(HWindow, sel);

    RECT clientRect;
    if (!GetClientRect(HWindow, &clientRect))
        return;

    RECT visibleRect = clientRect;
    RECT upDownRect;
    if (GetWindowRect(upDown, &upDownRect))
    {
        POINT pt = {upDownRect.left, upDownRect.top};
        ScreenToClient(HWindow, &pt);
        upDownRect.left = pt.x;
        upDownRect.top = pt.y;
        pt.x = upDownRect.right;
        pt.y = upDownRect.bottom;
        ScreenToClient(HWindow, &pt);
        upDownRect.right = pt.x;
        upDownRect.bottom = pt.y;

        if (upDownRect.left > visibleRect.left && upDownRect.left < visibleRect.right)
            visibleRect.right = upDownRect.left;
    }

    RECT upDownClientRect;
    if (!GetClientRect(upDown, &upDownClientRect))
        return;

    int width = upDownClientRect.right - upDownClientRect.left;
    int height = upDownClientRect.bottom - upDownClientRect.top;
    if (width <= 0 || height <= 0)
        return;

    int leftArrowX = width / 4;
    int rightArrowX = (3 * width) / 4;
    int arrowY = height / 2;
    int maxScrollAttempts = GetDisplayedTabCount() * 2;
    if (maxScrollAttempts < 1)
        maxScrollAttempts = 1;

    for (int i = 0; i < maxScrollAttempts; ++i)
    {
        RECT tabRect;
        if (!TabCtrl_GetItemRect(HWindow, sel, &tabRect))
            break;

        if (tabRect.left >= visibleRect.left && tabRect.right <= visibleRect.right)
            break;

        bool scrollLeft = tabRect.left < visibleRect.left;
        LPARAM clickPoint = MAKELPARAM(scrollLeft ? leftArrowX : rightArrowX, arrowY);
        SendMessage(upDown, WM_LBUTTONDOWN, MK_LBUTTON, clickPoint);
        SendMessage(upDown, WM_LBUTTONUP, 0, clickPoint);

        if (TabCtrl_GetCurSel(HWindow) != sel)
            TabCtrl_SetCurSel(HWindow, sel);
    }

    UpdateOverflowButtonColors();
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
    if (nmhdr == NULL)
        return FALSE;

    if (nmhdr->hwndFrom != HWindow)
        return FALSE;

    switch (nmhdr->code)
    {
    case TCN_SELCHANGE:
    {
        TabTipHoverIndex = -1;
        KillTimer(HWindow, kTabTipTimerId);
        HideTabToolTip();
        if (SuppressSelectionNotifications > 0)
        {
            result = 0;
            return TRUE;
        }

        // Activating another tab is a selection operation, not a reorder.  Some
        // plug-in panels can run their activation code before the matching
        // button-up is delivered back here, so cancel any pending drag state now
        // to avoid leaving the insert mark painted in the tab bar.
        if (DragTracking)
            CancelDragTracking();
        else
            ClearInsertMark();

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
        if (MainWindow != NULL && MainWindow->ShouldSuppressPanelTabMouseWheelContextMenu(screen))
        {
            result = 0;
            return TRUE;
        }
        POINT client = screen;
        ScreenToClient(HWindow, &client);
        int hit = HitTest(client);
        if (MainWindow != NULL)
        {
            if (hit >= 0 && !IsNewTabButtonIndex(hit))
            {
                MainWindow->OnPanelTabContextMenu(Side, hit, screen);
            }
            else
            {
                MainWindow->OnPanelTabNewTabAreaContextMenu(Side, screen);
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
    UpdateOverflowButtonColors();
}

void CTabWindow::RefreshLayout()
{
    CALL_STACK_MESSAGE_NONE
    UpdateNewTabButtonWidth();
    UpdateOverflowButtonColors();
    EnsureInitialSelectedTabVisible();
}

void CTabWindow::EnsureTabTipWnd()
{
    CALL_STACK_MESSAGE_NONE
    if (HTabTipWnd != NULL || HWindow == NULL)
        return;

    if (!g_TabTipClassRegistered)
    {
        WNDCLASSEX wc = { sizeof(wc) };
        wc.lpfnWndProc = TabTipWndProc;
        wc.hInstance = HInstance;
        wc.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = "SalamanderTabTip";
        RegisterClassEx(&wc);
        g_TabTipClassRegistered = true;
    }

    HTabTipWnd = CreateWindowEx(0, "SalamanderTabTip", "",
                                WS_POPUP | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                                0, 0, 0, 0,
                                HWindow, NULL, HInstance, NULL);
}

void CTabWindow::ShowTabToolTip(int tabIndex)
{
    CALL_STACK_MESSAGE_NONE
    if (tabIndex < 0 || IsNewTabButtonIndex(tabIndex))
    {
        HideTabToolTip();
        return;
    }
    CFilesWindow* panel = reinterpret_cast<CFilesWindow*>(GetItemData(tabIndex));
    if (panel == NULL)
    {
        HideTabToolTip();
        return;
    }
    char path[MAX_PATH];
    if (!panel->GetGeneralPath(path, MAX_PATH) || path[0] == 0)
    {
        HideTabToolTip();
        return;
    }

    // Store text for the tooltip's WM_PAINT
    lstrcpy(g_TabTipText, path);

    EnsureTabTipWnd();
    if (HTabTipWnd == NULL)
        return;

    HDC hdc = GetDC(HTabTipWnd);
    SelectObject(hdc, (HFONT)GetStockObject(DEFAULT_GUI_FONT));
    SIZE sz;
    GetTextExtentPoint32(hdc, path, (int)strlen(path), &sz);
    ReleaseDC(HTabTipWnd, hdc);
    int w = sz.cx + 6;
    int h = sz.cy + 4;

    RECT tabRect;
    SendMessage(HWindow, TCM_GETITEMRECT, tabIndex, (LPARAM)&tabRect);
    POINT pt = { tabRect.left, tabRect.bottom };
    ClientToScreen(HWindow, &pt);

    HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    GetMonitorInfo(hMon, &mi);
    if (pt.x + w > mi.rcWork.right)
        pt.x = mi.rcWork.right - w;
    if (pt.y + h > mi.rcWork.bottom)
    {
        pt.y = tabRect.top;
        ClientToScreen(HWindow, &pt);
        pt.y -= h;
    }

    if (IsWindowVisible(HTabTipWnd))
    {
        SetWindowPos(HTabTipWnd, HWND_TOPMOST, pt.x, pt.y, w, h,
                     SWP_NOACTIVATE);
        InvalidateRect(HTabTipWnd, NULL, TRUE);
    }
    else
    {
        SetWindowPos(HTabTipWnd, HWND_TOPMOST, pt.x, pt.y, w, h,
                     SWP_SHOWWINDOW | SWP_NOACTIVATE);
    }

    TabTipTabIndex = tabIndex;
}

void CTabWindow::HideTabToolTip()
{
    CALL_STACK_MESSAGE_NONE
    if (HTabTipWnd != NULL && IsWindowVisible(HTabTipWnd))
        ShowWindow(HTabTipWnd, SW_HIDE);
    TabTipTabIndex = -1;
}

void CTabWindow::UpdateOverflowButtonColors()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
        DarkModeUpdateTabControlOverflowButtons(HWindow);
}

bool CTabWindow::HandleMouseWheel(WPARAM wParam)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return false;

    short zDelta = (short)HIWORD(wParam);
    if (zDelta == 0)
        return true;

    if ((zDelta < 0 && MouseWheelAccumulator > 0) ||
        (zDelta > 0 && MouseWheelAccumulator < 0))
    {
        MouseWheelAccumulator = 0;
    }

    MouseWheelAccumulator += zDelta;
    int steps = MouseWheelAccumulator / WHEEL_DELTA;
    if (steps != 0)
    {
        MouseWheelAccumulator -= steps * WHEEL_DELTA;
        ScrollTabsByWheelSteps(steps);
    }

    return true;
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
    if (GetTabCount() <= 1)
        return false;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_PARAM;
    if (!SendMessageW(HWindow, TCM_GETITEMW, index, (LPARAM)&item))
        return false;
    CFilesWindow* panel = reinterpret_cast<CFilesWindow*>(item.lParam);
    if (panel == NULL)
        return false;
    if (panel->IsTabLocked())
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

            int verticalOversize = (DPIFontHeight > 0 ? DPIFontHeight : EnvFontCharHeight) / 3;
            if (verticalOversize < 4)
                verticalOversize = 4;
            indicatorRect.top -= verticalOversize;
            indicatorRect.bottom += verticalOversize;

            int indicatorWidth = (DPIFontHeight > 0 ? DPIFontHeight : EnvFontCharHeight) / 4;
            if (indicatorWidth < 4)
                indicatorWidth = 4;
            if (indicatorWidth > 12)
                indicatorWidth = 12;
            if (DarkModeShouldUseDarkColors())
                indicatorWidth = indicatorWidth * 3 / 2;

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

    const bool useDark = DarkModeShouldUseDarkColors();
    COLORREF baseColor = GetSysColor(COLOR_HIGHLIGHT);
    COLORREF fillColor = LightenColor(baseColor, useDark ? 80 : 96);
    COLORREF borderColor = DarkenColor(baseColor, useDark ? 64 : 64);

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

    int fontHeight = DPIFontHeight > 0 ? DPIFontHeight : EnvFontCharHeight;
    int verticalMargin = fontHeight / 2;
    if (verticalMargin < 6)
        verticalMargin = 6;
    int horizontalMargin = fontHeight;
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

void CTabWindow::MoveTab(int from, int to)
{
    CALL_STACK_MESSAGE_NONE
    if (from == to)
        return;
    MoveTabInternal(from, to);
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

    CFilesWindow* panel = reinterpret_cast<CFilesWindow*>(item.lParam);
    if (panel != NULL && panel->IsTabLocked())
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

void CTabWindow::ExpandSelectedTabRect(RECT& rect) const
{
    int expand = 2;
    if (HWindow != NULL)
    {
        RECT clientRect;
        if (GetClientRect(HWindow, &clientRect))
        {
            if (rect.left > clientRect.left)
            {
                rect.left -= expand;
                if (rect.left < clientRect.left)
                    rect.left = clientRect.left;
            }
            if (rect.right < clientRect.right)
            {
                rect.right += expand;
                if (rect.right > clientRect.right)
                    rect.right = clientRect.right;
            }
            if (rect.top > clientRect.top)
            {
                rect.top -= expand;
                if (rect.top < clientRect.top)
                    rect.top = clientRect.top;
            }
            return;
        }
    }

    rect.left -= expand;
    rect.right += expand;
    rect.top -= expand;
}

void CTabWindow::ScrollTabsByWheelSteps(int steps)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL || steps == 0)
        return;

    HWND upDown = FindWindowEx(HWindow, NULL, UPDOWN_CLASS, NULL);
    if (upDown == NULL || !IsWindowVisible(upDown) || !IsWindowEnabled(upDown))
        return;

    RECT upDownClientRect;
    if (!GetClientRect(upDown, &upDownClientRect))
        return;

    int width = upDownClientRect.right - upDownClientRect.left;
    int height = upDownClientRect.bottom - upDownClientRect.top;
    if (width <= 0 || height <= 0)
        return;

    // The tab control's overflow arrows are implemented as an internal up-down child window.
    // Simulate clicks on that child instead of changing the tab control focus/selection:
    // clicking those arrows scrolls the visible tab strip without activating another tab.
    bool scrollLeft = steps > 0;
    int x = scrollLeft ? width / 4 : (3 * width) / 4;
    int y = height / 2;
    LPARAM clickPoint = MAKELPARAM(x, y);
    int count = steps > 0 ? steps : -steps;

    int oldSel = TabCtrl_GetCurSel(HWindow);
    for (int i = 0; i < count; ++i)
    {
        SendMessage(upDown, WM_LBUTTONDOWN, MK_LBUTTON, clickPoint);
        SendMessage(upDown, WM_LBUTTONUP, 0, clickPoint);
    }

    if (oldSel >= 0 && TabCtrl_GetCurSel(HWindow) != oldSel)
    {
        CSelChangeGuard guard(SuppressSelectionNotifications);
        TabCtrl_SetCurSel(HWindow, oldSel);
    }

    UpdateOverflowButtonColors();
    InvalidateRect(HWindow, NULL, FALSE);
}

void CTabWindow::InvalidateTab(int index)
{
    if (HWindow == NULL)
        return;
    if (index < 0)
        return;
    RECT rect;
    if (TabCtrl_GetItemRect(HWindow, index, &rect))
    {
        if (TabCtrl_GetCurSel(HWindow) == index)
            ExpandSelectedTabRect(rect);
        InvalidateRect(HWindow, &rect, FALSE);
    }
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

RECT CTabWindow::GetTabCloseButtonRect(const RECT& tabRect)
{
    int tabHeight = tabRect.bottom - tabRect.top;
    int btnSize = tabHeight - 4;
    if (btnSize < 8)
        btnSize = 8;
    int margin = 3;
    RECT closeRect;
    closeRect.right = tabRect.right - margin + 2;
    closeRect.left = closeRect.right - btnSize;
    closeRect.top = tabRect.top + (tabHeight - btnSize) / 2 - 1;
    closeRect.bottom = closeRect.top + btnSize;
    return closeRect;
}

bool CTabWindow::ShouldShowCloseButton(int tabIndex, int selectedIndex) const
{
    if (IsNewTabButtonIndex(tabIndex))
        return false;
    if (Configuration.TabCloseButtonAll)
        return true;
    if (Configuration.TabCloseButtonActive && tabIndex == selectedIndex)
        return true;
    return false;
}

void CTabWindow::PaintWithBase(HDC hdc, const RECT* clipRect, bool paintTabs, bool paintIndicator)
{
    if (hdc == NULL)
        return;

    if (DarkModeShouldUseDarkColors())
    {
        RECT clientRect = {0, 0, 0, 0};
        if (HWindow != NULL)
            GetClientRect(HWindow, &clientRect);

        RECT fillRect = clientRect;
        if (clipRect != NULL)
        {
            if (!IntersectRect(&fillRect, &clientRect, clipRect))
                SetRectEmpty(&fillRect);
        }

        HBRUSH backgroundBrush = HDialogBrush != NULL ? HDialogBrush : GetSysColorBrush(COLOR_BTNFACE);
        if (!IsRectEmpty(&fillRect))
            FillRect(hdc, &fillRect, backgroundBrush);

        HPEN hSeparatorPen = CreatePen(PS_SOLID, 1, RGB(56, 56, 56));
        HGDIOBJ hOldPen = SelectObject(hdc, hSeparatorPen);
        MoveToEx(hdc, clientRect.left, clientRect.top, nullptr);
        LineTo(hdc, clientRect.right, clientRect.top);
        SelectObject(hdc, hOldPen);
        DeleteObject(hSeparatorPen);

        HBRUSH frameBrush = DarkModeGetPanelFrameBrush();
        if (frameBrush != NULL && clientRect.bottom > clientRect.top)
        {
            RECT borderRect = clientRect;
            borderRect.top = borderRect.bottom - 1;
            if (borderRect.top < borderRect.bottom)
                FillRect(hdc, &borderRect, frameBrush);
        }
    }
    else
    {
        LPARAM printFlags = PRF_CLIENT | PRF_ERASEBKGND;
        if (DefWndProc != NULL)
            CallWindowProc((WNDPROC)DefWndProc, HWindow, WM_PRINTCLIENT, (WPARAM)hdc, printFlags);
        else
            DefWindowProc(HWindow, WM_PRINTCLIENT, (WPARAM)hdc, printFlags);
    }

    if (paintTabs)
        PaintCustomTabs(hdc, clipRect);
    if (paintIndicator)
        PaintDragIndicator(hdc);
}

bool CTabWindow::PaintBuffered(HDC targetDC, const RECT& updateRect, bool paintTabs, bool paintIndicator)
{
    if (targetDC == NULL)
        return false;

    int width = updateRect.right - updateRect.left;
    int height = updateRect.bottom - updateRect.top;
    if (width <= 0 || height <= 0)
        return false;

    HDC bufferDC = CreateCompatibleDC(targetDC);
    if (bufferDC == NULL)
        return false;

    HBITMAP bufferBitmap = CreateCompatibleBitmap(targetDC, width, height);
    if (bufferBitmap == NULL)
    {
        DeleteDC(bufferDC);
        return false;
    }

    HBITMAP oldBitmap = (HBITMAP)SelectObject(bufferDC, bufferBitmap);
    POINT oldOrigin = {0, 0};
    BOOL originChanged = SetViewportOrgEx(bufferDC, -updateRect.left, -updateRect.top, &oldOrigin);

    PaintWithBase(bufferDC, &updateRect, paintTabs, paintIndicator);

    if (originChanged)
        SetViewportOrgEx(bufferDC, oldOrigin.x, oldOrigin.y, NULL);
    BitBlt(targetDC, updateRect.left, updateRect.top, width, height, bufferDC, 0, 0, SRCCOPY);

    SelectObject(bufferDC, oldBitmap);
    DeleteObject(bufferBitmap);
    DeleteDC(bufferDC);
    return true;
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
    const bool useDark = DarkModeShouldUseDarkColors();
    COLORREF defaultBaseColor;
    if (useDark)
    {
        if (CurrentColors != NULL)
            defaultBaseColor = GetCOLORREF(CurrentColors[ITEM_BK_NORMAL]);
        else
            defaultBaseColor = DarkModeGetDialogBackgroundColor();
    }
    else
        defaultBaseColor = GetSysColor(COLOR_BTNFACE);

    for (int i = 0; i < total; ++i)
    {
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

        bool isNewTab = IsNewTabButtonIndex(i);

        COLORREF baseColor = defaultBaseColor;
        bool hasCustomColor = false;
        if (!isNewTab && TryResolveTabColor(i, baseColor))
            hasCustomColor = true;

        bool isSelected = (i == selected);
        bool isHot = (item.dwState & TCIS_HIGHLIGHTED) != 0;
        bool hasFocus = (focus == i) && (focusWnd == HWindow);

        std::wstring drawTextBuffer;
        const wchar_t* drawText = kNewTabButtonText;
        if (!isNewTab)
        {
            drawTextBuffer = textBuffer;
            TrimTabWidthPadding(drawTextBuffer);
            drawText = drawTextBuffer.c_str();
        }
        DrawColoredTab(hdc, itemRect, drawText, baseColor, isSelected, isHot, hasFocus, hasCustomColor, i, selected);
    }

    // Draw separator lines between tabs (not after the last tab or the new-tab button).
    COLORREF separatorColor;
    if (useDark)
    {
        if (CurrentColors != NULL)
            separatorColor = GetCOLORREF(CurrentColors[ITEM_BK_FOCUSED]);
        else
            separatorColor = DarkModeGetDialogBackgroundColor();
    }
    else
        separatorColor = GetSysColor(COLOR_BTNSHADOW);

    HPEN hSepPen = CreatePen(PS_SOLID, 1, separatorColor);
    if (hSepPen != NULL)
    {
        HPEN hOldPen = (HPEN)SelectObject(hdc, hSepPen);
        for (int i = 0; i < total - 1; ++i)
        {
            RECT itemRect;
            if (!TabCtrl_GetItemRect(HWindow, i, &itemRect))
                continue;
            if (IsNewTabButtonIndex(i))
                continue;
            int x = itemRect.right - 1;
            MoveToEx(hdc, x, itemRect.top, NULL);
            LineTo(hdc, x, itemRect.bottom);
        }
        SelectObject(hdc, hOldPen);
        DeleteObject(hSepPen);
    }
}

void CTabWindow::DrawColoredTab(HDC hdc, const RECT& itemRect, const wchar_t* text, COLORREF baseColor,
                                bool selected, bool hot, bool hasFocus, bool hasCustomColor,
                                int tabIndex, int selectedIndex) const
{
    if (hdc == NULL)
        return;

    RECT rect = itemRect;
    RECT fillRect = rect;
    if (selected)
    {
        ExpandSelectedTabRect(fillRect);
    }
    else
    {
        InflateRect(&fillRect, -1, 0);
        if (fillRect.right <= fillRect.left || fillRect.bottom <= fillRect.top)
            fillRect = rect;
    }

    const bool useDark = DarkModeShouldUseDarkColors();
    COLORREF fillColor = baseColor;
    if (useDark && !hasCustomColor)
    {
        int paletteIndex = ITEM_BK_NORMAL;
        if (selected)
            paletteIndex = ITEM_BK_FOCUSED;
        else if (hot)
            paletteIndex = ITEM_BK_HIGHLIGHT;

        if (CurrentColors != NULL)
            fillColor = GetCOLORREF(CurrentColors[paletteIndex]);
        else
            fillColor = DarkModeGetDialogBackgroundColor();
    }
    else
    {
        if (selected)
            fillColor = LightenColor(fillColor, 96);
        else if (hot)
            fillColor = LightenColor(fillColor, 48);
    }

    HBRUSH brush = CreateSolidBrush(fillColor);
    if (fillRect.right <= fillRect.left)
        fillRect.right = fillRect.left + 1;
    if (fillRect.bottom <= fillRect.top)
        fillRect.bottom = fillRect.top + 1;

    if (brush != NULL)
    {
        FillRect(hdc, &fillRect, brush);
        DeleteObject(brush);
    }

    RECT textRect = fillRect;
    InflateRect(&textRect, -4, 0);
    if (textRect.right <= textRect.left)
    {
        textRect.left = fillRect.left;
        textRect.right = fillRect.right;
    }

    int fontHeight = DPIFontHeight > 0 ? DPIFontHeight : EnvFontCharHeight;
    int topPadding = fontHeight / 12;
    if (topPadding < 1)
        topPadding = 1;
    int verticalLift = fontHeight / 6;
    if (verticalLift < 2)
        verticalLift = 2;
    ++verticalLift;
    if (selected && Configuration.TabActiveBorder)
    {
        int activeBorderTextLift = fontHeight / 8;
        if (activeBorderTextLift < 2)
            activeBorderTextLift = 2;
        verticalLift += activeBorderTextLift;
    }
    int bottomPadding = topPadding + verticalLift;

    textRect.top += topPadding;
    textRect.bottom -= bottomPadding;
    if (textRect.bottom <= textRect.top)
    {
        textRect.top = fillRect.top;
        textRect.bottom = fillRect.bottom;
    }

    bool showCloseBtn = ShouldShowCloseButton(tabIndex, selectedIndex);
    RECT closeBtnRect = {0, 0, 0, 0};
    if (showCloseBtn)
    {
        closeBtnRect = GetTabCloseButtonRect(itemRect);
        int closeBtnWidth = closeBtnRect.right - closeBtnRect.left + 6;
        textRect.right -= closeBtnWidth;
        if (textRect.right < textRect.left)
            textRect.right = textRect.left;
    }

    HFONT oldFont = NULL;
    HFONT fontToUse = (selected && HDPIFontBold != NULL) ? HDPIFontBold :
                      (HDPIFont != NULL ? HDPIFont : EnvFont);
    if (fontToUse != NULL)
        oldFont = (HFONT)SelectObject(hdc, fontToUse);
    int oldBkMode = SetBkMode(hdc, TRANSPARENT);
    COLORREF textColor;
    if (useDark)
    {
        if (!hasCustomColor)
        {
            if (CurrentColors != NULL)
            {
                int textIndex = ITEM_FG_NORMAL;
                if (selected)
                    textIndex = ITEM_FG_FOCUSED;
                else if (hot)
                    textIndex = ITEM_FG_HIGHLIGHT;
                textColor = GetCOLORREF(CurrentColors[textIndex]);
            }
            else
                textColor = DarkModeGetDialogTextColor();
        }
        else
            textColor = IsColorDark(fillColor) ? RGB(255, 255, 255) : RGB(0, 0, 0);

        textColor = DarkModeEnsureReadableForeground(textColor, fillColor);
    }
    else
        textColor = IsColorDark(fillColor) ? RGB(255, 255, 255) : RGB(0, 0, 0);
    COLORREF oldTextColor = SetTextColor(hdc, textColor);

    const wchar_t* drawText = (text != NULL) ? text : L"";
    UINT format = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX;
    if (Configuration.TabCaptionAlignment == TAB_CAPTION_ALIGN_LEFT)
        format |= DT_LEFT;
    else
        format |= DT_CENTER;
    DrawTextW(hdc, drawText, -1, &textRect, format);

    if (hasFocus)
    {
        RECT focusRect = fillRect;
        InflateRect(&focusRect, -3, -3);
        DrawFocusRect(hdc, &focusRect);
    }

    if (showCloseBtn)
    {
        bool isCloseHover = (CloseButtonHoverIndex == tabIndex);
        if (isCloseHover)
        {
            HBRUSH hoverBrush = CreateSolidBrush(useDark ? RGB(0x40, 0x40, 0x40) : RGB(0xE0, 0xE0, 0xE0));
            if (hoverBrush != NULL)
            {
                FillRect(hdc, &closeBtnRect, hoverBrush);
                DeleteObject(hoverBrush);
            }
        }
        COLORREF closeColor;
        if (isCloseHover)
            closeColor = useDark ? RGB(0xFF, 0xFF, 0xFF) : RGB(0x00, 0x00, 0x00);
        else
            closeColor = useDark ? RGB(0x88, 0x88, 0x88) : RGB(0x60, 0x60, 0x60);
        HPEN closePen = CreatePen(PS_SOLID, 1, closeColor);
        if (closePen != NULL)
        {
            HPEN oldPen = (HPEN)SelectObject(hdc, closePen);
            int inset = (closeBtnRect.right - closeBtnRect.left) / 4;
            if (inset < 3) inset = 3;
            MoveToEx(hdc, closeBtnRect.left + inset, closeBtnRect.top + inset, NULL);
            LineTo(hdc, closeBtnRect.right - inset, closeBtnRect.bottom - inset);
            MoveToEx(hdc, closeBtnRect.right - inset - 1, closeBtnRect.top + inset, NULL);
            LineTo(hdc, closeBtnRect.left + inset - 1, closeBtnRect.bottom - inset);
            if (oldPen != NULL)
                SelectObject(hdc, oldPen);
            DeleteObject(closePen);
        }
    }

    if (selected && Configuration.TabActiveBorder)
    {
        COLORREF borderColor;
        if (Configuration.TabActiveBorderColor != CLR_INVALID)
        {
            borderColor = Configuration.TabActiveBorderColor;
        }
        else
        {
            if (CurrentColors != NULL)
                borderColor = GetCOLORREF(CurrentColors[ACTIVE_CAPTION_BK]);
            else if (useDark)
                borderColor = DarkModeGetDialogBackgroundColor();
            else
                borderColor = GetSysColor(COLOR_ACTIVECAPTION);
            borderColor = LightenColor(borderColor, 96);
        }

        int borderHeight = 2;
        RECT borderRect = fillRect;
        if (HWindow != NULL)
        {
            RECT clientRect;
            if (GetClientRect(HWindow, &clientRect) && borderRect.bottom > clientRect.bottom - 1)
                borderRect.bottom = clientRect.bottom - 1;
        }
        borderRect.top = borderRect.bottom - borderHeight;
        if (borderRect.top < fillRect.top)
            borderRect.top = fillRect.top;
        if (borderRect.right > borderRect.left && borderRect.bottom > borderRect.top)
        {
            HBRUSH borderBrush = CreateSolidBrush(borderColor);
            if (borderBrush != NULL)
            {
                FillRect(hdc, &borderRect, borderBrush);
                DeleteObject(borderBrush);
            }
        }
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
    case WM_DPICHANGED_AFTERPARENT:
        RefreshDPIResources();
        return 0;

    case WM_SIZE:
    case WM_THEMECHANGED:
        DarkModePreserveCustomTabControl(HWindow);
        UpdateOverflowButtonColors();
        PostMessage(HWindow, WM_USER_ENSURE_SELECTED_TAB_VISIBLE, 0, 0);
        break;

    case WM_USER_ENSURE_SELECTED_TAB_VISIBLE:
        EnsureInitialSelectedTabVisible();
        return 0;

    case WM_PARENTNOTIFY:
        if (LOWORD(wParam) == WM_CREATE)
            UpdateOverflowButtonColors();
        break;

    case WM_PAINT:
    {
        if (HWindow == NULL)
            break;

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(HWindow, &ps);
        if (hdc != NULL)
        {
            RECT updateRect = ps.rcPaint;
            bool shouldPaintTabs = true;
            bool shouldPaintIndicator = DragIndicatorVisible;

            bool painted = false;
            if (shouldPaintTabs || shouldPaintIndicator)
                painted = PaintBuffered(hdc, updateRect, shouldPaintTabs, shouldPaintIndicator);

            if (!painted)
            {
                const RECT* clipRect = (shouldPaintTabs || shouldPaintIndicator) ? &updateRect : NULL;
                PaintWithBase(hdc, clipRect, shouldPaintTabs, shouldPaintIndicator);
            }

            EndPaint(HWindow, &ps);
        }
        return 0;
    }

    case WM_PRINTCLIENT:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        if (hdc != NULL)
        {
            int saved = SaveDC(hdc);
            PaintWithBase(hdc, NULL, true, DragIndicatorVisible);
            RestoreDC(hdc, saved);
        }
        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        if (MainWindow != NULL)
            MainWindow->ResetPanelTabMouseWheelContextMenuSuppression();
        break;
    }

    case WM_LBUTTONDOWN:
    {
        TabTipHoverIndex = -1;
        KillTimer(HWindow, kTabTipTimerId);
        HideTabToolTip();
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt;
        pt.x = pts.x;
        pt.y = pts.y;
        int hit = HitTest(pt);
        if (hit >= 0 && !IsNewTabButtonIndex(hit))
        {
            RECT itemRect;
            if (TabCtrl_GetItemRect(HWindow, hit, &itemRect))
            {
                RECT closeRect = GetTabCloseButtonRect(itemRect);
                if (PtInRect(&closeRect, pt) && ShouldShowCloseButton(hit, TabCtrl_GetCurSel(HWindow)))
                {
                    LastClickedIndex = -1;
                    if (MainWindow != NULL)
                    {
                        CFilesWindow* panel = MainWindow->GetPanelTabAt(Side, hit);
                        if (panel != NULL && !panel->IsTabLocked())
                            MainWindow->ClosePanelTab(panel);
                    }
                    break;
                }
            }
        }
        LastClickedIndex = hit;
        LastClickWasSelected =
            (hit >= 0 && !IsNewTabButtonIndex(hit) && TabCtrl_GetCurSel(HWindow) == hit);
        if (IsReorderableIndex(hit))
            StartDragTracking(hit, pt);
        else
            CancelDragTracking();
        break;
    }

    case WM_MOUSEWHEEL:
        if (MouseWheelMSGThroughHook && MouseWheelMSGTime != 0 && (GetTickCount() - MouseWheelMSGTime < MOUSEWHEELMSG_VALID))
            return 0;
        MouseWheelMSGThroughHook = FALSE;
        MouseWheelMSGTime = GetTickCount();
        if (MainWindow != NULL)
        {
            POINT screenPt;
            screenPt.x = GET_X_LPARAM(lParam);
            screenPt.y = GET_Y_LPARAM(lParam);
            if (MainWindow->TrySwitchPanelTabByMouseWheel(screenPt, wParam))
                return 0;
        }
        HandleMouseWheel(wParam);
        return 0;

    case WM_USER_MOUSEWHEEL:
        if (MainWindow != NULL)
        {
            POINT screenPt;
            screenPt.x = GET_X_LPARAM(lParam);
            screenPt.y = GET_Y_LPARAM(lParam);
            if (MainWindow->TrySwitchPanelTabByMouseWheel(screenPt, wParam))
                return 0;
        }
        HandleMouseWheel(wParam);
        return 0;



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
            if (panel != NULL && !panel->IsTabLocked())
                MainWindow->ClosePanelTab(panel);
        }
        return 0;
    }

    case WM_LBUTTONDBLCLK:
    {
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt;
        pt.x = pts.x;
        pt.y = pts.y;
        int hit = HitTest(pt);
        if (hit >= 0 && !IsNewTabButtonIndex(hit) && MainWindow != NULL)
        {
            MainWindow->CommandDuplicateTab(Side, hit);
            return 0;
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        if (!TabTipTracking)
        {
            TRACKMOUSEEVENT tme = { sizeof(tme) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = HWindow;
            TrackMouseEvent(&tme);
            TabTipTracking = true;
        }
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt = { pts.x, pts.y };
        TCHITTESTINFO hti;
        hti.pt = pt;
        int hitIndex = (int)SendMessage(HWindow, TCM_HITTEST, 0, (LPARAM)&hti);
        if (hitIndex >= 0 && !IsNewTabButtonIndex(hitIndex))
        {
            if (TabTipHoverIndex != hitIndex)
            {
                // New tab hovered – start/restart delay timer
                TabTipHoverIndex = hitIndex;
                KillTimer(HWindow, kTabTipTimerId);
                SetTimer(HWindow, kTabTipTimerId, kTabTipDelayMs, NULL);
            }
        }
        else
        {
            if (TabTipHoverIndex != -1 || TabTipTabIndex != -1)
            {
                TabTipHoverIndex = -1;
                KillTimer(HWindow, kTabTipTimerId);
                HideTabToolTip();
            }
        }
        int newCloseHover = -1;
        if (hitIndex >= 0 && !IsNewTabButtonIndex(hitIndex) &&
            ShouldShowCloseButton(hitIndex, TabCtrl_GetCurSel(HWindow)))
        {
            RECT itemRect;
            if (TabCtrl_GetItemRect(HWindow, hitIndex, &itemRect))
            {
                RECT closeRect = GetTabCloseButtonRect(itemRect);
                if (PtInRect(&closeRect, pt))
                    newCloseHover = hitIndex;
            }
        }
        if (CloseButtonHoverIndex != newCloseHover)
        {
            int oldHover = CloseButtonHoverIndex;
            CloseButtonHoverIndex = newCloseHover;
            if (oldHover >= 0)
                InvalidateTab(oldHover);
            if (newCloseHover >= 0)
                InvalidateTab(newCloseHover);
        }
        if (DragTracking)
        {
            POINTS pts = MAKEPOINTS(lParam);
            POINT pt;
            pt.x = pts.x;
            pt.y = pts.y;
            UpdateDragTracking(pt);
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        POINTS pts = MAKEPOINTS(lParam);
        POINT pt;
        pt.x = pts.x;
        pt.y = pts.y;
        bool wasDragging = Dragging;
        int clickedIndex = LastClickedIndex;
        bool clickedSelected = LastClickWasSelected;
        FinishDragTracking(pt, false);
        LastClickedIndex = -1;
        LastClickWasSelected = false;
        if (!wasDragging && clickedIndex >= 0 && clickedSelected && MainWindow != NULL &&
            !IsNewTabButtonIndex(clickedIndex))
        {
            MainWindow->OnPanelTabSelected(Side, clickedIndex);
        }
        break;
    }

    case WM_CAPTURECHANGED:
        if ((HWND)lParam != HWindow)
            CancelDragTracking();
        break;

    case WM_CANCELMODE:
        CancelDragTracking();
        break;

    case WM_TIMER:
        if (wParam == kTabTipTimerId)
        {
            KillTimer(HWindow, kTabTipTimerId);
            if (TabTipHoverIndex >= 0 && TabTipTabIndex != TabTipHoverIndex)
                ShowTabToolTip(TabTipHoverIndex);
        }
        break;

    case WM_MOUSELEAVE:
        TabTipTracking = false;
        TabTipHoverIndex = -1;
        KillTimer(HWindow, kTabTipTimerId);
        HideTabToolTip();
        if (CloseButtonHoverIndex >= 0)
        {
            int oldHover = CloseButtonHoverIndex;
            CloseButtonHoverIndex = -1;
            InvalidateTab(oldHover);
        }
        break;
    }

    return CWindow::WindowProc(uMsg, wParam, lParam);
}
