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
    bool IsReorderableIndex(int index) const;
    void StartDragTracking(int index, const POINT& pt);
    void UpdateDragTracking(const POINT& pt);
    void FinishDragTracking(const POINT& pt, bool canceled);
    void CancelDragTracking();
    int ComputeDragTargetIndex(POINT pt, int fromIndex) const;
    void MoveTabInternal(int from, int to);

    CMainWindow* MainWindow;
    CPanelSide Side;
    int ControlID;
    int SuppressSelectionNotifications;
    bool DragTracking;
    bool Dragging;
    POINT DragStartPoint;
    int DragSourceIndex;
};
