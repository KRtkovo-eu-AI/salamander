// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <vector>

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

    int AddTab(int index, const wchar_t* text, LPARAM data);
    void RemoveTab(int index);
    void RemoveAllTabs();
    void SetTabText(int index, const wchar_t* text);
    void SetCurSel(int index);
    int GetCurSel() const;
    int GetTabCount() const;
    LPARAM GetItemData(int index) const;
    int HitTest(POINT pt) const;
    BOOL HandleNotify(LPNMHDR nmhdr, LRESULT& result);

    void SetTabColor(int index, COLORREF color);
    void ClearTabColor(int index);

    bool ComputeExternalDropTarget(POINT screenPt, int& targetIndex, int& markItem, DWORD& markFlags) const;
    void ShowExternalDropIndicator(int markItem, DWORD markFlags);
    void HideExternalDropIndicator();
    void MoveTab(int from, int to);

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
    void UpdateDragIndicator(const POINT& pt);
    void SetInsertMark(int item, DWORD flags);
    void ClearInsertMark();
    void UpdateInsertMarkRect();
    void PaintDragIndicator(HDC hdc) const;
    bool ComputeDragTargetInfo(POINT pt, int fromIndex, int& targetIndex, int& markItem, DWORD& markFlags) const;
    int ComputeDragTargetIndex(POINT pt, int fromIndex) const;
    void MoveTabInternal(int from, int to);
    void InvalidateTab(int index);
    void ExpandSelectedTabRect(RECT& rect) const;

    struct STabColor
    {
        bool Valid;
        COLORREF Color;
    };

    void EnsureTabColorCapacity();
    void InsertTabColorSlot(int index, int currentCount);
    void RemoveTabColorSlot(int index);
    void MoveTabColor(int from, int to);
    STabColor* GetTabColor(int index);
    const STabColor* GetTabColor(int index) const;
    bool TryResolveTabColor(int index, COLORREF& color) const;
    void PaintWithBase(HDC hdc, const RECT* clipRect, bool paintTabs, bool paintIndicator);
    bool PaintBuffered(HDC targetDC, const RECT& updateRect, bool paintTabs, bool paintIndicator);
    void PaintCustomTabs(HDC hdc, const RECT* clipRect) const;
    void DrawColoredTab(HDC hdc, const RECT& itemRect, const wchar_t* text, COLORREF baseColor,
                        bool selected, bool hot, bool hasFocus) const;

    CMainWindow* MainWindow;
    CPanelSide Side;
    int ControlID;
    int SuppressSelectionNotifications;
    bool DragTracking;
    bool Dragging;
    POINT DragStartPoint;
    int DragSourceIndex;
    bool DragHasExternalTarget;
    int DragCurrentTarget;
    int DragInsertMarkItem;
    DWORD DragInsertMarkFlags;
    RECT DragIndicatorRect;
    bool DragIndicatorVisible;

    int LastClickedIndex;
    bool LastClickWasSelected;

    std::vector<STabColor> TabColors;

};
