// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <commctrl.h>

#include "tabwnd.h"
#include "mainwnd.h"
/*
#include "toolbar.h"
#include "cfgdlg.h"
#include "mainwnd.h"
#include "stswnd.h"
#include "plugins.h"
#include "fileswnd.h"
#include "shellib.h"
*/

//
// ****************************************************************************
// CTabWindow
//

CTabWindow::CTabWindow(CFilesWindow* filesWindow) : CWindow(ooStatic) /*, HotTrackItems(10, 5)*/
{
    CALL_STACK_MESSAGE_NONE
    FilesWindow = filesWindow;
    Owner = NULL;
    Side = (CPanelSide)0;
    ControlId = 0;
    TabCtrl = NULL;
}

CTabWindow::~CTabWindow()
{
    CALL_STACK_MESSAGE1("CTabWindow::~CTabWindow()");
}

BOOL CTabWindow::Create(CMainWindow* owner, HWND parent, UINT controlId, CPanelSide side)
{
    CALL_STACK_MESSAGE1("CTabWindow::Create()");
    Owner = owner;
    Side = side;
    ControlId = controlId;

    if (HWindow != NULL)
        return TRUE;

    HWND hwnd = CWindow::Create(CWINDOW_CLASSNAME2,
                                "",
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                0, 0, 0, 0,
                                parent,
                                (HMENU)(INT_PTR)controlId,
                                HInstance,
                                this);
    if (hwnd == NULL)
    {
        TRACE_E("CTabWindow::Create() unable to create window.");
        return FALSE;
    }

    return TRUE;
}

int CTabWindow::InsertTab(int index, const char* title)
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl == NULL)
        return -1;

    TCITEM item = {0};
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<char*>((title != NULL) ? title : "");

    return TabCtrl_InsertItem(TabCtrl, index, &item);
}

void CTabWindow::RemoveTab(int index)
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl == NULL)
        return;

    TabCtrl_DeleteItem(TabCtrl, index);
}

void CTabWindow::RenameTab(int index, const char* title)
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl == NULL)
        return;

    TCITEM item = {0};
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<char*>((title != NULL) ? title : "");
    TabCtrl_SetItem(TabCtrl, index, &item);
}

int CTabWindow::GetCurSel() const
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl == NULL)
        return -1;

    return TabCtrl_GetCurSel(TabCtrl);
}

void CTabWindow::SetCurSel(int index)
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl == NULL)
        return;

    TabCtrl_SetCurSel(TabCtrl, index);
}

int CTabWindow::GetCount() const
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl == NULL)
        return 0;

    return TabCtrl_GetItemCount(TabCtrl);
}

void CTabWindow::DestroyWindow()
{
    CALL_STACK_MESSAGE1("CTabWindow::DestroyWindow()");
    if (TabCtrl != NULL)
    {
        ::DestroyWindow(TabCtrl);
        TabCtrl = NULL;
    }
    if (HWindow != NULL)
    {
        ::DestroyWindow(HWindow);
        HWindow = NULL;
    }
}

int CTabWindow::GetNeededHeight()
{
    CALL_STACK_MESSAGE_NONE
    if (TabCtrl != NULL)
    {
        RECT r = {0, 0, 0, 0};
        TabCtrl_AdjustRect(TabCtrl, TRUE, &r);
        return r.bottom - r.top;
    }
    return 2 + EnvFontCharHeight + 2;
}

LRESULT
CTabWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTabWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CREATE:
    {
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_TAB_CLASSES;
        InitCommonControlsEx(&icex);

        TabCtrl = CreateWindowEx(0,
                                 WC_TABCONTROL,
                                 "",
                                 WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                 0,
                                 0,
                                 0,
                                 0,
                                 HWindow,
                                 (HMENU)(INT_PTR)ControlId,
                                 HInstance,
                                 NULL);
        if (TabCtrl == NULL)
        {
            TRACE_E("CTabWindow::WindowProc(): Unable to create tab control.");
            return -1;
        }
        SendMessage(TabCtrl, WM_SETFONT, (WPARAM)EnvFont, FALSE);
        return 0;
    }

    case WM_DESTROY:
    {
        TabCtrl = NULL;
        return 0;
    }

    case WM_SIZE:
    {
        if (TabCtrl != NULL)
        {
            RECT r;
            GetClientRect(HWindow, &r);
            MoveWindow(TabCtrl, 0, 0, r.right - r.left, r.bottom - r.top, TRUE);
        }
        return 0;
    }

    case WM_NOTIFY:
    {
        if (Owner != NULL && Owner->HWindow != NULL)
            return SendMessage(Owner->HWindow, WM_NOTIFY, wParam, lParam);
        break;
    }
    }

    return CWindow::WindowProc(uMsg, wParam, lParam);
}
