// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

//
// ****************************************************************************

class CMainWindow;
enum CPanelSide;

class CTabWindow : public CWindow
{
public:
    CTabWindow(CMainWindow* mainWindow, CPanelSide side);
    ~CTabWindow();

    BOOL Create(HWND parent, int controlID);
    void DestroyWindow();
    int GetNeededHeight() const;

    int AddTab(int index, const char* text, LPARAM data);
    void RemoveTab(int index);
    void RemoveAllTabs();
    void SetTabText(int index, const char* text);
    void SetCurSel(int index);
    int GetCurSel() const;
    int GetTabCount() const;
    LPARAM GetItemData(int index) const;
    int HitTest(POINT pt) const;
    BOOL HandleNotify(LPNMHDR nmhdr, LRESULT& result);

    CPanelSide GetSide() const { return Side; }

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void EnsureSelection();
    void EnsureNewTabButton();
    int GetDisplayedTabCount() const;
    int GetNewTabButtonIndex() const;
    BOOL IsNewTabButtonIndex(int index) const;
    void UpdateNewTabButtonWidth();
    void HandleLButtonDown(const POINT& pt);
    void HandleMouseMove(WPARAM wParam, const POINT& pt);
    void HandleLButtonUp(const POINT& pt);
    void HandleCaptureChanged();
    bool CanDragTab(int index) const;
    int ComputeDropIndex(const POINT& pt, int draggedIndex) const;

    CMainWindow* MainWindow;
    CPanelSide Side;
    int ControlID;
    int SuppressSelectionNotifications;

    bool DragCandidate;
    bool Dragging;
    int DraggedTabIndex;
    POINT DragStartPoint;
};
