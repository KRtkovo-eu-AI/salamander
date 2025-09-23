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
    int count = TabCtrl_GetItemCount(HWindow);
    if (index < 0 || index > count)
        index = count;
    return (int)SendMessageW(HWindow, TCM_INSERTITEMW, index, (LPARAM)&item);
}

void CTabWindow::RemoveTab(int index)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
        SendMessage(HWindow, TCM_DELETEITEM, index, 0);
}

void CTabWindow::RemoveAllTabs()
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow != NULL)
        SendMessage(HWindow, TCM_DELETEALLITEMS, 0, 0);
}

void CTabWindow::SetTabText(int index, const char* text)
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
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
        TabCtrl_SetCurSel(HWindow, index);
}

int CTabWindow::GetCurSel() const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return -1;
    return TabCtrl_GetCurSel(HWindow);
}

int CTabWindow::GetTabCount() const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
        return 0;
    return TabCtrl_GetItemCount(HWindow);
}

LPARAM CTabWindow::GetItemData(int index) const
{
    CALL_STACK_MESSAGE_NONE
    if (HWindow == NULL)
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
        EnsureSelection();
        if (MainWindow != NULL)
        {
            int sel = GetCurSel();
            MainWindow->OnPanelTabSelected(Side, sel);
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
            if (GetCurSel() != hit)
            {
                SetCurSel(hit);
                EnsureSelection();
                if (MainWindow != NULL)
                    MainWindow->OnPanelTabSelected(Side, hit);
            }
            if (MainWindow != NULL)
                MainWindow->OnPanelTabContextMenu(Side, hit, screen);
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
    if (TabCtrl_GetItemCount(HWindow) > 0 && TabCtrl_GetCurSel(HWindow) < 0)
        TabCtrl_SetCurSel(HWindow, 0);
}

LRESULT CTabWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTabWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    return CWindow::WindowProc(uMsg, wParam, lParam);
}
