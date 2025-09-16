// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//
// ****************************************************************************

class CFilesWindow;

class CTabWindow : public CWindow
{
public:
    CFilesWindow* FilesWindow;
    HWND TabHandle;

protected:
    HWND ToolTipHandle;
    char TooltipBuffer[2 * MAX_PATH];
#ifdef _UNICODE
    WCHAR TooltipBufferW[2 * MAX_PATH];
#endif

    //  protected:
    //    TDirectArray<CTabItem> TabItems;

public:
    CTabWindow(CFilesWindow* filesWindow);
    ~CTabWindow();

    void DestroyWindow();
    int GetNeededHeight();
    void DeleteAllTabs();
    void InsertTab(int index, const char* text);
    void RemoveTab(int index);
    void SetTabText(int index, const char* text);
    void SetActiveTab(int index);
    int GetSelectedTab() const;
    void SetFont(HFONT font);

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    void UpdateTooltipText(int tabIndex);
};
