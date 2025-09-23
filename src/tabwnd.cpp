// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <commctrl.h>
#include <string>

#include "tabwnd.h"
#include "mainwnd.h"
#include "fileswnd.h"

//
// ****************************************************************************
// CTabWindow
//

namespace
{
    constexpr LPARAM kNewTabButtonParam = static_cast<LPARAM>(-1);
    const wchar_t kNewTabButtonText[] = L"+";

    std::wstring ToWide(const char* text)
    {
        if (text == NULL)
            return std::wstring();
        int length = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
        if (length <= 0)
            return std::wstring();
        std::wstring wide(length - 1, L'\0');
        if (length > 1)
            MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], length);
        return wide;
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

int CTabWindow::AddTab(int index, const char* text, LPARAM data)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return -1;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT | TCIF_PARAM;
    std::wstring textW = ToWide(text);
    item.pszText = textW.empty() ? const_cast<LPWSTR>(L"") : const_cast<LPWSTR>(textW.c_str());
    item.lParam = data;
    int count = GetTabCount();
    if (index < 0 || index > count)
        index = count;
    int insertIndex = index;
    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex >= 0 && insertIndex > newTabIndex)
        insertIndex = newTabIndex;
    int result = (int)SendMessageW(HWindow, TCM_INSERTITEMW, insertIndex, (LPARAM)&item);
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
        SendMessage(HWindow, TCM_DELETEITEM, index, 0);
        EnsureNewTabButton();
    }
}

void CTabWindow::RemoveAllTabs()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
    {
        SendMessage(HWindow, TCM_DELETEALLITEMS, 0, 0);
        EnsureNewTabButton();
    }
}

void CTabWindow::SetTabText(int index, const char* text)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL || index < 0 || index >= GetTabCount())
        return;
    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT;
    std::wstring textW = ToWide(text);
    item.pszText = textW.empty() ? const_cast<LPWSTR>(L"") : const_cast<LPWSTR>(textW.c_str());
    SendMessageW(HWindow, TCM_SETITEMW, index, (LPARAM)&item);
}

void CTabWindow::SetCurSel(int index)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
    {
        if (index >= 0 && index < GetTabCount())
            TabCtrl_SetCurSel(HWindow, index);
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
    if (GetTabCount() > 0 && TabCtrl_GetCurSel(HWindow) < 0)
        TabCtrl_SetCurSel(HWindow, 0);
}

void CTabWindow::EnsureNewTabButton()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return;

    int total = GetDisplayedTabCount();
    int newTabIndex = GetNewTabButtonIndex();
    if (newTabIndex >= 0)
    {
        TCITEMW item;
        ZeroMemory(&item, sizeof(item));
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<LPWSTR>(kNewTabButtonText);
        SendMessageW(HWindow, TCM_SETITEMW, newTabIndex, (LPARAM)&item);
        return;
    }

    TCITEMW item;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT | TCIF_PARAM;
    item.pszText = const_cast<LPWSTR>(kNewTabButtonText);
    item.lParam = kNewTabButtonParam;
    SendMessageW(HWindow, TCM_INSERTITEMW, total, (LPARAM)&item);
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

LRESULT CTabWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTabWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    return CWindow::WindowProc(uMsg, wParam, lParam);
}
