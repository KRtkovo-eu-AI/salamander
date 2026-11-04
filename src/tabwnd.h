// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Always scroll the selected tab into view when it is clipped. | Startup cleared the one-shot pending flag while the first tab was still visible, so a later active overflow tab stayed hidden. | | Composer |
// | 2026-11-04 | Fix first-launch "+" selection and startup overflow scroll-to-active tab. | Clean default config left the new-tab button selected, and initial overflow positioning cleared its pending flag before the selected tab was actually visible. | | Composer |

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
    bool EnsureSelectedTabVisible();
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

    void RefreshLayout();
    void RefreshDPIResources();

    HWND GetTabToolTip() const { return HTabTipWnd; }

protected:
    virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void EnsureSelection();
    void EnsureInitialSelectedTabVisible();
    void RequestEnsureSelectedTabVisible();
    bool IsSelectedTabInView() const;
    bool GetSelectedTabVisibleRect(RECT& visibleRect) const;
    void EnsureNewTabButton();
    int GetDisplayedTabCount() const;
    int GetNewTabButtonIndex() const;
    BOOL IsNewTabButtonIndex(int index) const;
    void UpdateNewTabButtonWidth();
    void UpdateOverflowButtonColors();
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
    bool HandleMouseWheel(WPARAM wParam);
    void ScrollTabsByWheelSteps(int steps);
    void ShowTabToolTip(int tabIndex);
    void HideTabToolTip();
    void EnsureTabTipWnd();
    void RefreshTabToolTipFont();

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
                        bool selected, bool hot, bool hasFocus, bool hasCustomColor,
                        int tabIndex, int selectedIndex) const;

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
    int MouseWheelAccumulator;
    bool InitialEnsureSelectedTabVisiblePending;
    int EnsureVisibleRetryCount;

    std::vector<STabColor> TabColors;

    HWND HTabTipWnd;
    HFONT HTabTipFont;
    UINT TabTipDPI;
    int TabTipTabIndex;
    int TabTipHoverIndex;
    bool TabTipTracking;

    int CloseButtonHoverIndex;
    HFONT HDPIFont;
    HFONT HDPIFontBold;
    int DPIFontHeight;

    static RECT GetTabCloseButtonRect(const RECT& tabRect);
    bool ShouldShowCloseButton(int tabIndex, int selectedIndex) const;

};
