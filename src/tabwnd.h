// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//
// ****************************************************************************

class CMainWindow;
class CFilesWindow;
enum CPanelSide;

class CTabWindow : public CWindow
{
public:
    CFilesWindow* FilesWindow;
    CMainWindow* Owner;
    CPanelSide Side;
    UINT ControlId;
    HWND TabCtrl;

    //  protected:
    //    TDirectArray<CTabItem> TabItems;

public:
    CTabWindow(CFilesWindow* filesWindow);
    ~CTabWindow();

    BOOL Create(CMainWindow* owner, HWND parent, UINT controlId, CPanelSide side);
    int InsertTab(int index, const char* title);
    void RemoveTab(int index);
    void RenameTab(int index, const char* title);
    int GetCurSel() const;
    void SetCurSel(int index);
    int GetCount() const;

    void DestroyWindow();
    int GetNeededHeight();

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};
