// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "tabwnd.h"
#include "fileswnd.h"

//
// ****************************************************************************
// CTabWindow
//

CTabWindow::CTabWindow(CFilesWindow* filesWindow)
    : CWindow(ooStatic), FilesWindow(filesWindow), TabHandle(NULL), ToolTipHandle(NULL)
{
    CALL_STACK_MESSAGE1("CTabWindow::CTabWindow()");
#ifdef _UNICODE
    TooltipBufferW[0] = 0;
#endif
    TooltipBuffer[0] = 0;
}

CTabWindow::~CTabWindow()
{
    CALL_STACK_MESSAGE1("CTabWindow::~CTabWindow()");
}

void CTabWindow::DestroyWindow()
{
    CALL_STACK_MESSAGE1("CTabWindow::DestroyWindow()");
    if (TabHandle != NULL)
    {
        DestroyWindow(TabHandle);
        TabHandle = NULL;
    }
    ToolTipHandle = NULL;
    CWindow::DestroyWindow();
}

int CTabWindow::GetNeededHeight()
{
    CALL_STACK_MESSAGE1("CTabWindow::GetNeededHeight()");
    if (TabHandle != NULL)
    {
        RECT r = {0, 0, 200, EnvFontCharHeight + 10};
        TabCtrl_AdjustRect(TabHandle, FALSE, &r);
        int height = r.bottom - r.top;
        if (height < EnvFontCharHeight + 8)
            height = EnvFontCharHeight + 8;
        return height;
    }
    return EnvFontCharHeight + 8;
}

void CTabWindow::DeleteAllTabs()
{
    CALL_STACK_MESSAGE1("CTabWindow::DeleteAllTabs()");
    if (TabHandle != NULL)
        SendMessage(TabHandle, TCM_DELETEALLITEMS, 0, 0);
}

void CTabWindow::InsertTab(int index, const char* text)
{
    CALL_STACK_MESSAGE2("CTabWindow::InsertTab(%d)", index);
    if (TabHandle == NULL)
        return;
    TCITEM item;
#ifdef _UNICODE
    WCHAR wText[2 * MAX_PATH];
    if (text != NULL)
        MultiByteToWideChar(CP_ACP, 0, text, -1, wText, _countof(wText));
    else
        wText[0] = 0;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = wText;
    SendMessage(TabHandle, TCM_INSERTITEM, index, (LPARAM)&item);
#else
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<char*>((text != NULL) ? text : "");
    SendMessage(TabHandle, TCM_INSERTITEM, index, (LPARAM)&item);
#endif
}

void CTabWindow::RemoveTab(int index)
{
    CALL_STACK_MESSAGE2("CTabWindow::RemoveTab(%d)", index);
    if (TabHandle != NULL)
        SendMessage(TabHandle, TCM_DELETEITEM, index, 0);
}

void CTabWindow::SetTabText(int index, const char* text)
{
    CALL_STACK_MESSAGE2("CTabWindow::SetTabText(%d)", index);
    if (TabHandle == NULL)
        return;
    TCITEM item;
#ifdef _UNICODE
    WCHAR wText[2 * MAX_PATH];
    if (text != NULL)
        MultiByteToWideChar(CP_ACP, 0, text, -1, wText, _countof(wText));
    else
        wText[0] = 0;
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = wText;
    SendMessage(TabHandle, TCM_SETITEM, index, (LPARAM)&item);
#else
    ZeroMemory(&item, sizeof(item));
    item.mask = TCIF_TEXT;
    item.pszText = const_cast<char*>((text != NULL) ? text : "");
    SendMessage(TabHandle, TCM_SETITEM, index, (LPARAM)&item);
#endif
}

void CTabWindow::SetActiveTab(int index)
{
    CALL_STACK_MESSAGE2("CTabWindow::SetActiveTab(%d)", index);
    if (TabHandle != NULL)
        SendMessage(TabHandle, TCM_SETCURSEL, index, 0);
}

int CTabWindow::GetSelectedTab() const
{
    return (TabHandle != NULL) ? (int)SendMessage(TabHandle, TCM_GETCURSEL, 0, 0) : -1;
}

void CTabWindow::SetFont(HFONT font)
{
    CALL_STACK_MESSAGE1("CTabWindow::SetFont()");
    if (TabHandle != NULL)
        SendMessage(TabHandle, WM_SETFONT, (WPARAM)font, FALSE);
}

void CTabWindow::UpdateTooltipText(int tabIndex)
{
    CALL_STACK_MESSAGE2("CTabWindow::UpdateTooltipText(%d)", tabIndex);
    if (FilesWindow != NULL)
    {
        FilesWindow->GetTabTooltipText(tabIndex, TooltipBuffer, sizeof(TooltipBuffer));
#ifdef _UNICODE
        MultiByteToWideChar(CP_ACP, 0, TooltipBuffer, -1, TooltipBufferW, _countof(TooltipBufferW));
#endif
    }
    else
        TooltipBuffer[0] = 0;
#ifdef _UNICODE
    if (TooltipBufferW[0] == 0)
        TooltipBufferW[0] = 0;
#endif
}

LRESULT CTabWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CTabWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CREATE:
    {
        TabHandle = CreateWindowEx(0, WC_TABCONTROL, TEXT(""),
                                   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                                       TCS_TOOLTIPS | TCS_HOTTRACK | TCS_FOCUSNEVER,
                                   0, 0, 0, 0,
                                   HWindow,
                                   (HMENU)IDC_TABCONTROL,
                                   HInstance,
                                   NULL);
        if (TabHandle == NULL)
        {
            TRACE_E("CreateWindowEx on tab control failed");
            return -1;
        }
        SendMessage(TabHandle, WM_SETFONT, (WPARAM)EnvFont, FALSE);
        ToolTipHandle = (HWND)SendMessage(TabHandle, TCM_GETTOOLTIPS, 0, 0);
        if (ToolTipHandle != NULL)
        {
            SendMessage(ToolTipHandle, TTM_SETMAXTIPWIDTH, 0, 400);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        if (TabHandle != NULL)
        {
            DestroyWindow(TabHandle);
            TabHandle = NULL;
        }
        ToolTipHandle = NULL;
        return 0;
    }

    case WM_SIZE:
    {
        if (TabHandle != NULL)
        {
            MoveWindow(TabHandle, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        }
        return 0;
    }

    case WM_NOTIFY:
    {
        NMHDR* hdr = (NMHDR*)lParam;
        if (hdr->hwndFrom == TabHandle)
        {
            switch (hdr->code)
            {
            case TCN_SELCHANGE:
            {
                if (FilesWindow != NULL)
                {
                    int index = GetSelectedTab();
                    FilesWindow->OnTabSelectionChanged(index, TRUE);
                }
                return 0;
            }
            }
        }
        else if (hdr->hwndFrom == ToolTipHandle)
        {
            switch (hdr->code)
            {
            case TTN_GETDISPINFOA:
            {
                NMTTDISPINFOA* info = (NMTTDISPINFOA*)hdr;
                UpdateTooltipText((int)info->hdr.idFrom);
                info->lpszText = TooltipBuffer;
                return TRUE;
            }
#ifdef _UNICODE
            case TTN_GETDISPINFOW:
            {
                NMTTDISPINFOW* info = (NMTTDISPINFOW*)hdr;
                UpdateTooltipText((int)info->hdr.idFrom);
                info->lpszText = TooltipBufferW;
                return TRUE;
            }
#endif
            }
        }
        break;
    }
    }

    return CWindow::WindowProc(uMsg, wParam, lParam);
}

