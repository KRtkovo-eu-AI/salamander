// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <windows.h>
#include <windowsx.h>
#include <crtdbg.h>
#include <tchar.h>
#include <ostream>
#include <uxtheme.h>

#include "../darkmode.h"

#if defined(_DEBUG) && defined(_MSC_VER) // without passing file+line to 'new' operator, list of memory leaks shows only 'crtdbg.h(552)'
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#pragma warning(3 : 4706) // warning C4706: assignment within conditional expression

#include "trace.h"
#include "handles.h"
#include "array.h"
#include "winlib.h"
#include "winlibdpi.h"
#include "multimon.h"
#include "sheets.h"

extern CWinLibHelp* WinLibHelp;

namespace
{
const UINT_PTR TREE_PROP_TREE_REDRAW_SUBCLASS_ID = 1;

COLORREF LightenColorSimple(COLORREF color, int amount)
{
    int r = GetRValue(color) + amount;
    int g = GetGValue(color) + amount;
    int b = GetBValue(color) + amount;
    if (r > 255)
        r = 255;
    if (g > 255)
        g = 255;
    if (b > 255)
        b = 255;
    return RGB(r, g, b);
}

COLORREF DarkenColorSimple(COLORREF color, int amount)
{
    int r = GetRValue(color) - amount;
    int g = GetGValue(color) - amount;
    int b = GetBValue(color) - amount;
    if (r < 0)
        r = 0;
    if (g < 0)
        g = 0;
    if (b < 0)
        b = 0;
    return RGB(r, g, b);
}

void ApplyTreeViewColors(HWND treeView)
{
    if (treeView == NULL)
        return;

    const bool useDark = DarkModeShouldUseDarkColors();
    const COLORREF text = useDark ? DarkModeGetDialogTextColor() : GetSysColor(COLOR_WINDOWTEXT);
    const COLORREF background = useDark ? DarkModeGetDialogBackgroundColor() : GetSysColor(COLOR_WINDOW);

    TreeView_SetTextColor(treeView, text);
    TreeView_SetBkColor(treeView, background);
    TreeView_SetLineColor(treeView, useDark ? DarkenColorSimple(background, 40) : GetSysColor(COLOR_WINDOWTEXT));

    if (IsAppThemed())
    {
        if (useDark)
            SetWindowTheme(treeView, L"DarkMode_Explorer", NULL);
        else
            SetWindowTheme(treeView, L"explorer", NULL);
    }

    RedrawWindow(treeView, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
}

void RepaintWindowTree(HWND hwnd)
{
    if (hwnd != NULL)
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

LRESULT CALLBACK TreeViewRedrawSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR subclassId, DWORD_PTR referenceData)
{
    (void)referenceData;

    switch (message)
    {
    case WM_HSCROLL:
    case WM_VSCROLL:
    case WM_SIZE:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
        return result;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, TreeViewRedrawSubclassProc, subclassId);
        break;
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

bool IsChoiceButton(HWND hwnd)
{
    if (hwnd == NULL)
        return false;

    wchar_t className[16];
    if (GetClassNameW(hwnd, className, _countof(className)) == 0 || lstrcmpiW(className, L"Button") != 0)
        return false;

    const LONG_PTR type = GetWindowLongPtr(hwnd, GWL_STYLE) & BS_TYPEMASK;
    return type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
           type == BS_AUTO3STATE || type == BS_3STATE ||
           type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON;
}

void RedrawChoiceButtonAfterClick(HWND ctrl)
{
    if (!IsChoiceButton(ctrl))
        return;

    // Some themed/light Configuration pages can defer repainting checkbox/radio
    // glyph changes until the page is otherwise invalidated (for example by a
    // resize).  Repaint the clicked control immediately without erasing or
    // invalidating the whole parent page.
    RedrawWindow(ctrl, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_UPDATENOW);
}
} // namespace

//
// ****************************************************************************
// CElasticLayout
//

CElasticLayout::CElasticLayout(HWND hWindow)
    : ResizeCtrls(2, 2), ResizeRightCtrls(2, 2), MoveRightCtrls(2, 2), MoveCtrls(20, 20)
{
    HWindow = hWindow;
    SplitY = 0;
    MoveCtrlsInitialized = FALSE;
}

void CElasticLayout::AddResizeCtrl(int resID)
{
    HWND hChild = GetDlgItem(HWindow, resID);
    if (hChild != NULL)
    {
        // The controls resized by this helper (lists/list-views/tree-views) can
        // grow across the area where lower controls are being moved during a
        // property-page resize.  Force sibling clipping so the resized control
        // cannot repaint over those controls while the layout is settling.
        LONG_PTR style = GetWindowLongPtr(hChild, GWL_STYLE);
        SetWindowLongPtr(hChild, GWL_STYLE, style | WS_CLIPSIBLINGS);

        RECT r;
        GetWindowRect(hChild, &r);

        // pokud je spodni hrana prvku vetsi nez SplitY, posuneme SplitY hranici
        POINT p = {r.right, r.bottom};
        ScreenToClient(HWindow, &p);
        int logicalBottom = WinLibDPIToLogical(HWindow, p.y);
        if (logicalBottom > SplitY)
            SplitY = logicalBottom;

        RECT cR;
        GetClientRect(HWindow, &cR);

        CElasticLayoutCtrl ctrl;
        ctrl.HCtrl = hChild;
        ctrl.Pos.x = WinLibDPIToLogical(HWindow, cR.right - p.x); // logical distance from the original right edge of the control to the dialog right edge
        ctrl.Pos.y = 0;
        ResizeCtrls.Add(ctrl);
    }
    else
    {
        TRACE_E("CElasticLayout::AddResizeCtrl() Unknown control: resID=" << resID);
    }
}



void CElasticLayout::AddResizeRightCtrl(int resID)
{
    HWND hChild = GetDlgItem(HWindow, resID);
    if (hChild != NULL)
    {
        RECT r;
        GetWindowRect(hChild, &r);
        POINT p = {r.right, r.bottom};
        ScreenToClient(HWindow, &p);

        RECT cR;
        GetClientRect(HWindow, &cR);

        CElasticLayoutCtrl ctrl;
        ctrl.HCtrl = hChild;
        ctrl.Pos.x = WinLibDPIToLogical(HWindow, cR.right - p.x); // logical distance from the original right edge of the control to the dialog right edge
        ctrl.Pos.y = 0;
        ResizeRightCtrls.Add(ctrl);
    }
    else
    {
        TRACE_E("CElasticLayout::AddResizeRightCtrl() Unknown control: resID=" << resID);
    }
}

void CElasticLayout::AddMoveRightCtrl(int resID)
{
    HWND hChild = GetDlgItem(HWindow, resID);
    if (hChild != NULL)
    {
        RECT r;
        GetWindowRect(hChild, &r);
        POINT p = {r.left, r.top};
        ScreenToClient(HWindow, &p);

        RECT cR;
        GetClientRect(HWindow, &cR);

        CElasticLayoutCtrl ctrl;
        ctrl.HCtrl = hChild;
        ctrl.Pos.x = WinLibDPIToLogical(HWindow, cR.right - p.x); // keep the original logical distance from the dialog right edge
        ctrl.Pos.y = 0;
        MoveRightCtrls.Add(ctrl);
    }
    else
    {
        TRACE_E("CElasticLayout::AddMoveRightCtrl() Unknown control: resID=" << resID);
    }
}

void CElasticLayout::AddMoveCtrl(HWND hChild)
{
    // pokud prvek lezi pod SplitY, pridame ho do seznamu prvku, ktere budou posouvat
    RECT r;
    GetWindowRect(hChild, &r);
    POINT p = {r.left, r.top};
    ScreenToClient(HWindow, &p);
    POINT logical = {WinLibDPIToLogical(HWindow, p.x),
                     WinLibDPIToLogical(HWindow, p.y)};
    if (logical.y >= SplitY)
    {
        CElasticLayoutCtrl mc;
        mc.HCtrl = hChild;
        mc.Pos = logical;
        MoveCtrls.Add(mc);
    }
}

void CElasticLayout::FindMoveCtrls()
{
    if (MoveCtrlsInitialized)
        return;
    MoveCtrlsInitialized = TRUE;

    // Only top-level dialog controls belong to the page layout.  EnumChildWindows()
    // is recursive and also returns implementation windows owned by compound controls
    // (for example SysHeader32 or the label-edit child inside a SysListView32).  Moving
    // those nested windows independently breaks list/list-view based configuration
    // pages after the property sheet is resized.
    HWND hChild = GetWindow(HWindow, GW_CHILD);
    while (hChild != NULL)
    {
        AddMoveCtrl(hChild);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    // najdeme obalku vsech 'move' prvku
    RECT rEnvelope = {0};
    for (int i = 0; i < MoveCtrls.Count; i++)
    {
        HWND hCtrl = MoveCtrls[i].HCtrl;

        RECT r;
        GetWindowRect(hCtrl, &r);
        if (r.left < rEnvelope.left)
            rEnvelope.left = r.left;
        if (r.top < rEnvelope.top)
            rEnvelope.top = r.top;
        if (r.right > rEnvelope.right)
            rEnvelope.right = r.right;
        if (r.bottom > rEnvelope.bottom)
            rEnvelope.bottom = r.bottom;
    }
    POINT p = {rEnvelope.right, rEnvelope.bottom};
    ScreenToClient(HWindow, &p);
    int envelopeBottom = WinLibDPIToLogical(HWindow, p.y);
    // souradnice 'MoveCtrlsY' budou vztazene od spodni hrany obalky
    for (int i = 0; i < MoveCtrls.Count; i++)
        MoveCtrls[i].Pos.y = envelopeBottom - MoveCtrls[i].Pos.y;

    // pro prvky ResizeCtrls ulozime jejich vzdalenost spodni hrany od spodni hrany obalky
    for (int i = 0; i < ResizeCtrls.Count; i++)
    {
        if (ResizeCtrls[i].Pos.y == 0)
        {
            RECT r;
            GetWindowRect(ResizeCtrls[i].HCtrl, &r);
            ResizeCtrls[i].Pos.y = WinLibDPIToLogical(HWindow, max(0, rEnvelope.bottom - r.bottom));
        }
    }
}

void CElasticLayout::LayoutCtrls()
{
    if (ResizeCtrls.Count == 0)
    {
        TRACE_E("No controls to layout!");
        return;
    }
    RECT cR;
    GetClientRect(HWindow, &cR);

    FindMoveCtrls();

    HDWP hdwp = HANDLES(BeginDeferWindowPos(ResizeCtrls.Count + ResizeRightCtrls.Count + MoveRightCtrls.Count + MoveCtrls.Count));
    if (hdwp != NULL)
    {
        for (int i = 0; i < ResizeCtrls.Count; i++)
        {
            HWND hCtrl = ResizeCtrls[i].HCtrl;
            RECT r;
            GetWindowRect(hCtrl, &r);
            POINT p = {r.left, r.top};
            ScreenToClient(HWindow, &p);
            hdwp = HANDLES(DeferWindowPos(hdwp, hCtrl, NULL,
                                           0, 0,
                                           cR.right - p.x - WinLibDPIFromLogical(HWindow, ResizeCtrls[i].Pos.x),
                                           cR.bottom - p.y - WinLibDPIFromLogical(HWindow, ResizeCtrls[i].Pos.y),
                                           SWP_NOMOVE | SWP_NOZORDER));
        }
        for (int i = 0; i < ResizeRightCtrls.Count; i++)
        {
            HWND hCtrl = ResizeRightCtrls[i].HCtrl;
            RECT r;
            GetWindowRect(hCtrl, &r);
            POINT p = {r.left, r.top};
            ScreenToClient(HWindow, &p);
            hdwp = HANDLES(DeferWindowPos(hdwp, hCtrl, NULL,
                                           0, 0,
                                           cR.right - p.x - WinLibDPIFromLogical(HWindow, ResizeRightCtrls[i].Pos.x),
                                           r.bottom - r.top,
                                           SWP_NOMOVE | SWP_NOZORDER));
        }
        for (int i = 0; i < MoveCtrls.Count; i++)
        {
            HWND hCtrl = MoveCtrls[i].HCtrl;
            int x = WinLibDPIFromLogical(HWindow, MoveCtrls[i].Pos.x);
            for (int j = 0; j < MoveRightCtrls.Count; j++)
            {
                if (MoveRightCtrls[j].HCtrl == hCtrl)
                {
                    x = cR.right - WinLibDPIFromLogical(HWindow, MoveRightCtrls[j].Pos.x);
                    break;
                }
            }
            hdwp = HANDLES(DeferWindowPos(hdwp, hCtrl, NULL,
                                           x, cR.bottom - WinLibDPIFromLogical(HWindow, MoveCtrls[i].Pos.y),
                                           0, 0,
                                           SWP_NOSIZE | SWP_NOZORDER));
        }
        HANDLES(EndDeferWindowPos(hdwp));
    }
}

//
// ****************************************************************************
// CPropSheetPage
//

CPropSheetPage::CPropSheetPage(const TCHAR* title, HINSTANCE modul, int resID,
                               DWORD flags, HICON icon, CObjectOrigin origin)
    : CDialog(modul, resID, NULL, origin)
{
    Init(title, modul, resID, icon, flags, origin);
}

CPropSheetPage::CPropSheetPage(const TCHAR* title, HINSTANCE modul, int resID, UINT helpID,
                               DWORD flags, HICON icon, CObjectOrigin origin)
    : CDialog(modul, resID, helpID, NULL, origin)
{
    Init(title, modul, resID, icon, flags, origin);
}

void CPropSheetPage::Init(const TCHAR* title, HINSTANCE modul, int resID,
                          HICON icon, DWORD flags, CObjectOrigin origin)
{
    Title = NULL;
    if (title != NULL)
    {
        int len = (int)_tcslen(title);
        Title = new TCHAR[len + 1];
        if (Title != NULL)
            _tcscpy_s(Title, len + 1, title);
        else
            TRACE_ET(_T("Low memory!"));
    }
    Flags = flags;
    Icon = icon;

    ParentDialog = NULL; // nastavuje se z CPropertyDialog::Execute()
    ParentPage = NULL;
    HTreeItem = NULL;
    Expanded = NULL;
    ElasticLayout = NULL;
    HorizontalLayoutCtrls = NULL;
    DPIControlLayouts = NULL;
    HorizontalLayoutWidth = 0;
    DPIChangeInProgress = FALSE;
    DPILayoutPosted = FALSE;
}

CPropSheetPage::~CPropSheetPage()
{
    if (Title != NULL)
        delete[] Title;
    if (ElasticLayout != NULL)
        delete ElasticLayout;
    if (HorizontalLayoutCtrls != NULL)
        delete HorizontalLayoutCtrls;
    if (DPIControlLayouts != NULL)
        delete DPIControlLayouts;
}

BOOL CPropSheetPage::ValidateData()
{
    CTransferInfo ti(HWindow, ttDataFromWindow);
    Validate(ti);
    if (!ti.IsGood())
    {
        if (PropSheet_GetCurrentPageHwnd(Parent) != HWindow)
            PropSheet_SetCurSel(Parent, HWindow, 0);

        ti.EnsureControlIsFocused(ti.FailCtrlID);
        return FALSE;
    }
    else
        return TRUE;
}

BOOL CPropSheetPage::TransferData(CTransferType type)
{
    CTransferInfo ti(HWindow, type);
    Transfer(ti);
    if (!ti.IsGood())
    {
        if (ti.Type == ttDataFromWindow &&
            PropSheet_GetCurrentPageHwnd(Parent) != HWindow)
            PropSheet_SetCurSel(Parent, HWindow, 0);

        ti.EnsureControlIsFocused(ti.FailCtrlID);
        return FALSE;
    }
    else
        return TRUE;
}

HPROPSHEETPAGE
CPropSheetPage::CreatePropSheetPage()
{
    PROPSHEETPAGE psp;
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = Flags;
    psp.hInstance = Modul;
    psp.pszTemplate = MAKEINTRESOURCE(ResID);
    psp.hIcon = Icon;
    psp.pszTitle = Title;
    psp.pfnDlgProc = CPropSheetPage::CPropSheetPageProc;
    psp.lParam = (LPARAM)this;
    psp.pfnCallback = NULL;
    psp.pcRefParent = NULL;
    return CreatePropertySheetPage(&psp);
}

BOOL CPropSheetPage::ElasticVerticalLayout(int count, ...)
{
    if (ElasticLayout != NULL)
    {
        TRACE_E("ElasticLayout already set!");
        return FALSE;
    }
    ElasticLayout = new CElasticLayout(HWindow);
    va_list list;
    va_start(list, count);
    for (int arg = 0; arg < count; arg++)
        ElasticLayout->AddResizeCtrl(va_arg(list, int));
    va_end(list);
    return TRUE;
}

BOOL CPropSheetPage::ElasticLayoutControls(int resizeCount, int resizeRightCount, int moveRightCount, ...)
{
    if (ElasticLayout != NULL)
    {
        TRACE_E("ElasticLayout already set!");
        return FALSE;
    }
    ElasticLayout = new CElasticLayout(HWindow);
    va_list list;
    va_start(list, moveRightCount);
    for (int arg = 0; arg < resizeCount; arg++)
        ElasticLayout->AddResizeCtrl(va_arg(list, int));
    for (int arg = 0; arg < resizeRightCount; arg++)
        ElasticLayout->AddResizeRightCtrl(va_arg(list, int));
    for (int arg = 0; arg < moveRightCount; arg++)
        ElasticLayout->AddMoveRightCtrl(va_arg(list, int));
    va_end(list);
    return TRUE;
}


#define PHLM_RESIZE_RIGHT 1
#define PHLM_MOVE_RIGHT 2

static BOOL IsHorizontalLayoutStatic(HWND hCtrl)
{
    LONG_PTR style = GetWindowLongPtr(hCtrl, GWL_STYLE);
    LONG_PTR type = style & SS_TYPEMASK;
    return type == SS_LEFT || type == SS_LEFTNOWORDWRAP || type == SS_CENTER ||
           type == SS_RIGHT || type == SS_ETCHEDHORZ;
}

static BOOL IsHorizontalLayoutButton(HWND hCtrl, BOOL* resizeRight)
{
    LONG_PTR style = GetWindowLongPtr(hCtrl, GWL_STYLE);
    LONG_PTR type = style & BS_TYPEMASK;
    if (type == BS_GROUPBOX)
    {
        *resizeRight = TRUE;
        return TRUE;
    }
    if (type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
        type == BS_AUTO3STATE || type == BS_3STATE)
    {
        return FALSE;
    }
    if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON || type == BS_OWNERDRAW)
    {
        *resizeRight = FALSE;
        return TRUE;
    }
    return FALSE;
}


static BOOL HasOverlappingControlToRight(HWND hParent, HWND hCtrl, const RECT* ctrlRect)
{
    HWND hChild = GetWindow(hParent, GW_CHILD);
    while (hChild != NULL)
    {
        if (hChild != hCtrl && IsWindowVisible(hChild))
        {
            RECT wR;
            GetWindowRect(hChild, &wR);
            POINT p1 = {wR.left, wR.top};
            POINT p2 = {wR.right, wR.bottom};
            ScreenToClient(hParent, &p1);
            ScreenToClient(hParent, &p2);
            RECT r = {WinLibDPIToLogical(hParent, p1.x),
                      WinLibDPIToLogical(hParent, p1.y),
                      WinLibDPIToLogical(hParent, p2.x),
                      WinLibDPIToLogical(hParent, p2.y)};
            if (r.left > ctrlRect->left && r.top < ctrlRect->bottom && r.bottom > ctrlRect->top)
                return TRUE;
        }
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
    return FALSE;
}

static BOOL HorizontalLayoutContains(TDirectArray<CPageHorizontalLayoutCtrl>* ctrls, HWND hCtrl)
{
    for (int i = 0; i < ctrls->Count; i++)
    {
        if ((*ctrls)[i].HCtrl == hCtrl)
            return TRUE;
    }
    return FALSE;
}

static void AddHorizontalLayoutCtrl(TDirectArray<CPageHorizontalLayoutCtrl>* ctrls, HWND hCtrl, const RECT& r, int mode)
{
    if (HorizontalLayoutContains(ctrls, hCtrl))
        return;
    CPageHorizontalLayoutCtrl ctrl;
    ctrl.HCtrl = hCtrl;
    ctrl.Rect = r;
    ctrl.Mode = mode;
    ctrls->Add(ctrl);
}

void CPropSheetPage::InitHorizontalLayout()
{
    if (HorizontalLayoutCtrls != NULL)
        delete HorizontalLayoutCtrls;
    HorizontalLayoutCtrls = new TDirectArray<CPageHorizontalLayoutCtrl>(20, 20);
    if (HorizontalLayoutCtrls == NULL)
    {
        TRACE_E("Low memory!");
        HorizontalLayoutWidth = 0;
        return;
    }

    RECT cR;
    GetClientRect(HWindow, &cR);

    int maxChildRight = 0;
    HWND hChild = GetWindow(HWindow, GW_CHILD);
    while (hChild != NULL)
    {
        RECT wR;
        GetWindowRect(hChild, &wR);
        POINT p = {wR.right, wR.bottom};
        ScreenToClient(HWindow, &p);
        int logicalRight = WinLibDPIToLogical(HWindow, p.x);
        if (logicalRight > maxChildRight)
            maxChildRight = logicalRight;
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }

    // When a tree property page is created, its window can already have the
    // final (larger) holder size while child controls are still at resource
    // coordinates. Use the controls' right edge as the design width in that
    // case; otherwise small right-edge buttons are not recognized as docked.
    HorizontalLayoutWidth = WinLibDPIToLogical(HWindow, cR.right);
    if (maxChildRight > 0 && HorizontalLayoutWidth - maxChildRight >= 40)
        HorizontalLayoutWidth = maxChildRight;

    hChild = GetWindow(HWindow, GW_CHILD);
    while (hChild != NULL)
    {
        TCHAR className[64];
        className[0] = 0;
        GetClassName(hChild, className, _countof(className));

        RECT wR;
        GetWindowRect(hChild, &wR);
        POINT p1 = {wR.left, wR.top};
        POINT p2 = {wR.right, wR.bottom};
        ScreenToClient(HWindow, &p1);
        ScreenToClient(HWindow, &p2);
        RECT r = {WinLibDPIToLogical(HWindow, p1.x),
                  WinLibDPIToLogical(HWindow, p1.y),
                  WinLibDPIToLogical(HWindow, p2.x),
                  WinLibDPIToLogical(HWindow, p2.y)};

        int mode = 0;
        BOOL resizeRight = FALSE;
        if (_tcsicmp(className, _T("Edit")) == 0 ||
            _tcsicmp(className, WC_LISTVIEW) == 0 ||
            _tcsicmp(className, WC_TREEVIEW) == 0 ||
            _tcsicmp(className, TOOLBARCLASSNAME) == 0)
        {
            // Stretch regular data controls, except very small numeric edits.
            if (r.right - r.left > 80 || HorizontalLayoutWidth - r.right < 40)
                mode = PHLM_RESIZE_RIGHT;
        }
        else if (_tcsicmp(className, _T("Static")) == 0)
        {
            LONG_PTR staticType = GetWindowLongPtr(hChild, GWL_STYLE) & SS_TYPEMASK;
            // Only horizontal separator lines should stretch automatically.
            // Text labels can have opaque backgrounds in dark mode and may cover
            // adjacent inputs if they are widened with the page.
            if (IsHorizontalLayoutStatic(hChild) &&
                (staticType == SS_ETCHEDHORZ || GetProp(hChild, _T("SalamanderToolbarHeader")) != NULL))
                mode = PHLM_RESIZE_RIGHT;
        }
        else if (_tcsicmp(className, _T("Button")) == 0 && IsHorizontalLayoutButton(hChild, &resizeRight))
        {
            if (resizeRight)
            {
                LONG_PTR buttonType = GetWindowLongPtr(hChild, GWL_STYLE) & BS_TYPEMASK;
                if (buttonType == BS_GROUPBOX || !HasOverlappingControlToRight(HWindow, hChild, &r))
                    mode = PHLM_RESIZE_RIGHT;
            }
            else if (HorizontalLayoutWidth - r.right < 40)
                mode = PHLM_MOVE_RIGHT;
        }

        if (mode != 0)
        {
            AddHorizontalLayoutCtrl(HorizontalLayoutCtrls, hChild, r, mode);

            if (mode == PHLM_RESIZE_RIGHT && _tcsicmp(className, _T("Edit")) == 0)
            {
                HWND hSibling = GetWindow(HWindow, GW_CHILD);
                while (hSibling != NULL)
                {
                    if (hSibling != hChild)
                    {
                        RECT sWR;
                        GetWindowRect(hSibling, &sWR);
                        POINT s1 = {sWR.left, sWR.top};
                        POINT s2 = {sWR.right, sWR.bottom};
                        ScreenToClient(HWindow, &s1);
                        ScreenToClient(HWindow, &s2);
                        RECT sR = {WinLibDPIToLogical(HWindow, s1.x),
                                   WinLibDPIToLogical(HWindow, s1.y),
                                   WinLibDPIToLogical(HWindow, s2.x),
                                   WinLibDPIToLogical(HWindow, s2.y)};

                        if (sR.left >= r.right && sR.left - r.right <= 8 &&
                            sR.top < r.bottom && sR.bottom > r.top &&
                            sR.right - sR.left <= 40)
                        {
                            AddHorizontalLayoutCtrl(HorizontalLayoutCtrls, hSibling, sR, PHLM_MOVE_RIGHT);
                            break;
                        }
                    }
                    hSibling = GetWindow(hSibling, GW_HWNDNEXT);
                }
            }
        }

        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
}


static void DockOverlappingEditButtons(HWND hWindow)
{
    int maxButtonWidth = WinLibDPIFromLogical(hWindow, 40);
    int buttonGap = WinLibDPIFromLogical(hWindow, 4);
    HWND hEdit = GetWindow(hWindow, GW_CHILD);
    while (hEdit != NULL)
    {
        TCHAR editClass[64];
        editClass[0] = 0;
        GetClassName(hEdit, editClass, _countof(editClass));
        if (_tcsicmp(editClass, _T("Edit")) == 0)
        {
            RECT eWR;
            GetWindowRect(hEdit, &eWR);
            POINT e1 = {eWR.left, eWR.top};
            POINT e2 = {eWR.right, eWR.bottom};
            ScreenToClient(hWindow, &e1);
            ScreenToClient(hWindow, &e2);
            RECT eR = {e1.x, e1.y, e2.x, e2.y};

            HWND hChild = GetWindow(hWindow, GW_CHILD);
            while (hChild != NULL)
            {
                if (hChild != hEdit)
                {
                    RECT cWR;
                    GetWindowRect(hChild, &cWR);
                    POINT c1 = {cWR.left, cWR.top};
                    POINT c2 = {cWR.right, cWR.bottom};
                    ScreenToClient(hWindow, &c1);
                    ScreenToClient(hWindow, &c2);
                    RECT cR = {c1.x, c1.y, c2.x, c2.y};
                    int cW = cR.right - cR.left;

                    if (cW <= maxButtonWidth && cR.left > eR.left && cR.left < eR.right &&
                        cR.top < eR.bottom && cR.bottom > eR.top)
                    {
                        SetWindowPos(hChild, NULL, eR.right + buttonGap, cR.top, 0, 0,
                                     SWP_NOSIZE | SWP_NOZORDER);
                    }
                }
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
        }
        hEdit = GetWindow(hEdit, GW_HWNDNEXT);
    }
}

void CPropSheetPage::ApplyHorizontalLayout()
{
    if (HorizontalLayoutCtrls == NULL || HorizontalLayoutWidth == 0)
        return;

    RECT cR;
    GetClientRect(HWindow, &cR);
    int logicalWidth = WinLibDPIToLogical(HWindow, cR.right);
    int dx = logicalWidth - HorizontalLayoutWidth;
    if (dx < 0)
        dx = 0;

    HDWP hdwp = HANDLES(BeginDeferWindowPos(HorizontalLayoutCtrls->Count));
    if (hdwp != NULL)
    {
        for (int i = 0; i < HorizontalLayoutCtrls->Count; i++)
        {
            CPageHorizontalLayoutCtrl* ctrl = &(*HorizontalLayoutCtrls)[i];
            RECT r = ctrl->Rect;
            if (ctrl->Mode == PHLM_RESIZE_RIGHT)
                r.right += dx;
            else if (ctrl->Mode == PHLM_MOVE_RIGHT)
            {
                r.left += dx;
                r.right += dx;
            }
            HANDLES(DeferWindowPos(hdwp, ctrl->HCtrl, NULL,
                                   WinLibDPIFromLogical(HWindow, r.left),
                                   WinLibDPIFromLogical(HWindow, r.top),
                                   WinLibDPIFromLogical(HWindow, r.right - r.left),
                                   WinLibDPIFromLogical(HWindow, r.bottom - r.top),
                                   SWP_NOZORDER));
        }
        HANDLES(EndDeferWindowPos(hdwp));
    }
}

void CPropSheetPage::CaptureDPIControlLayouts()
{
    if (DPIControlLayouts != NULL)
        delete DPIControlLayouts;
    DPIControlLayouts = new TDirectArray<CPageDPIControlLayout>(20, 20);
    if (DPIControlLayouts == NULL)
    {
        TRACE_E("Low memory!");
        return;
    }

    HWND child = GetWindow(HWindow, GW_CHILD);
    while (child != NULL)
    {
        RECT windowRect;
        if (GetWindowRect(child, &windowRect))
        {
            POINT topLeft = {windowRect.left, windowRect.top};
            POINT bottomRight = {windowRect.right, windowRect.bottom};
            ScreenToClient(HWindow, &topLeft);
            ScreenToClient(HWindow, &bottomRight);

            CPageDPIControlLayout layout;
            layout.HCtrl = child;
            layout.LogicalRect.left = WinLibDPIToLogical(HWindow, topLeft.x);
            layout.LogicalRect.top = WinLibDPIToLogical(HWindow, topLeft.y);
            layout.LogicalRect.right = WinLibDPIToLogical(HWindow, bottomRight.x);
            layout.LogicalRect.bottom = WinLibDPIToLogical(HWindow, bottomRight.y);
            DPIControlLayouts->Add(layout);
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
}

void CPropSheetPage::RestoreDPIControlLayouts()
{
    if (DPIControlLayouts == NULL)
        return;

    HDWP hdwp = HANDLES(BeginDeferWindowPos(DPIControlLayouts->Count));
    if (hdwp == NULL)
        return;

    for (int i = 0; i < DPIControlLayouts->Count; i++)
    {
        CPageDPIControlLayout* layout = &(*DPIControlLayouts)[i];
        if (!IsWindow(layout->HCtrl) || ::GetParent(layout->HCtrl) != HWindow)
            continue;

        const RECT& r = layout->LogicalRect;
        HDWP next = HANDLES(DeferWindowPos(
            hdwp, layout->HCtrl, NULL,
            WinLibDPIFromLogical(HWindow, r.left),
            WinLibDPIFromLogical(HWindow, r.top),
            WinLibDPIFromLogical(HWindow, r.right - r.left),
            WinLibDPIFromLogical(HWindow, r.bottom - r.top),
            SWP_NOACTIVATE | SWP_NOZORDER));
        if (next == NULL)
        {
            hdwp = NULL;
            break;
        }
        hdwp = next;
    }
    if (hdwp != NULL)
        HANDLES(EndDeferWindowPos(hdwp));
}

INT_PTR
CPropSheetPage::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        ParentDialog->HWindow = Parent;
        TransferData(ttDataToWindow);
        InitHorizontalLayout();
        ApplyHorizontalLayout();
        if (ElasticLayout != NULL)
            ElasticLayout->LayoutCtrls();
        DockOverlappingEditButtons(HWindow);
        CaptureDPIControlLayouts();
        return TRUE; // chci focus od DefDlgProc
    }

    case WM_SIZE:
    {
        // PMv2 sends WM_SIZE while it is still scaling the page's direct
        // children. Running our anchor layout in the middle of that cascade
        // makes the currently visible page get scaled twice.
        if (!DPIChangeInProgress)
        {
            ApplyHorizontalLayout();
            if (ElasticLayout != NULL)
                ElasticLayout->LayoutCtrls();
            DockOverlappingEditButtons(HWindow);
        }
        break;
    }

    case WM_DPICHANGED_BEFOREPARENT:
    {
        DPIChangeInProgress = TRUE;
        break;
    }

    case WM_DPICHANGED_AFTERPARENT:
    {
        // Queue exactly one layout after the complete PMv2 child cascade. The
        // baselines are stored in 96-DPI logical units, so this also restores
        // exact positions after any intermediate dialog-manager rounding.
        if (!DPILayoutPosted)
        {
            DPILayoutPosted = TRUE;
            PostMessage(HWindow, WM_APP + 0x3A8, 0, 0);
        }
        break;
    }

    case WM_APP + 0x3A8:
    {
        DPIChangeInProgress = FALSE;
        DPILayoutPosted = FALSE;
        // Discard every intermediate rectangle produced by the PMv2 child
        // cascade. Restore one immutable 96-DPI baseline and apply Salamander's
        // resizing rules exactly once for the final page size.
        RestoreDPIControlLayouts();
        ApplyHorizontalLayout();
        if (ElasticLayout != NULL)
            ElasticLayout->LayoutCtrls();
        DockOverlappingEditButtons(HWindow);
        RedrawWindow(HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
            RedrawChoiceButtonAfterClick(reinterpret_cast<HWND>(lParam));
        break;
    }

    case WM_HELP:
    {
        if (WinLibHelp != NULL && HelpID != -1)
        {
            WinLibHelp->OnHelp(HWindow, HelpID, (HELPINFO*)lParam,
                               (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                               (GetKeyState(VK_SHIFT) & 0x8000) != 0);
            return TRUE;
        }
        break; // F1 nechame propadnout do parenta
    }

    case WM_CONTEXTMENU:
    {
        if (WinLibHelp != NULL)
            WinLibHelp->OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));
        return TRUE;
    }

    case WM_NOTIFY:
    {
        if (((NMHDR*)lParam)->code == PSN_KILLACTIVE) // deaktivace stranky
        {
            if (ValidateData())
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
            else // nepovolime deaktivaci stranky
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
            return TRUE;
        }

        if (((NMHDR*)lParam)->code == PSN_SETACTIVE) // aktivace stranky
        {
            if (ParentDialog != NULL && ParentDialog->LastPage != NULL)
            { // zapamatovani posledni stranky
                *ParentDialog->LastPage = ParentDialog->GetCurSel();
            }
            break;
        }

        if (((NMHDR*)lParam)->code == PSN_APPLY)
        { // stisknuto tlacitko ApplyNow nebo OK
            if (TransferData(ttDataFromWindow))
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, PSNRET_NOERROR);
            else
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            return TRUE;
        }

        if (((NMHDR*)lParam)->code == PSN_WIZFINISH)
        { // stisknuto tlacitko Finish
            // neprislo PSN_KILLACTIVE - provedu validaci
            if (!ValidateData())
            {
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }

            // obehnu vsechny stranky pro transfer
            for (int i = 0; i < ParentDialog->Count; i++)
            {
                if (ParentDialog->At(i)->HWindow != NULL)
                {
                    if (!ParentDialog->At(i)->TransferData(ttDataFromWindow))
                    {
                        SetWindowLongPtr(HWindow, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
            }
            SetWindowLongPtr(HWindow, DWLP_MSGRESULT, FALSE);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORMSGBOX:
    {
        LRESULT brush = 0;
        if (DarkModeHandleCtlColor(uMsg, wParam, lParam, brush))
            return brush;
        break;
    }

    case WM_THEMECHANGED:
    {
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_SETTINGCHANGE:
    {
        if (DarkModeHandleSettingChange(uMsg, lParam) &&
            WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        break;
    }

    case WM_USER_COMMONDLG_DARKMODE_REDRAW:
    {
        RedrawWindow(HWindow, NULL, NULL, RDW_INVALIDATE | RDW_NOERASE | RDW_ALLCHILDREN);
        return TRUE;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK
CPropSheetPage::CPropSheetPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    CPropSheetPage* dlg;
    switch (uMsg)
    {
    case WM_INITDIALOG: // prvni zprava - pripojeni objektu k dialogu
    {
        dlg = (CPropSheetPage*)((PROPSHEETPAGE*)lParam)->lParam;
        if (dlg == NULL)
        {
            TRACE_ET(_T("Unable to create dialog."));
            return TRUE;
        }
        else
        {
            dlg->HWindow = hwndDlg;
            dlg->Parent = ::GetParent(hwndDlg);
            //--- zarazeni okna podle hwndDlg do seznamu oken
            if (!WindowsManager.AddWindow(hwndDlg, dlg)) // chyba
            {
                TRACE_ET(_T("Unable to create dialog."));
                return TRUE;
            }
            dlg->NotifDlgJustCreated(); // zavedeno jako misto pro upravu layoutu dialogu
        }
        break;
    }

    case WM_DESTROY: // posledni zprava - odpojeni objektu od dialogu
    {
        dlg = (CPropSheetPage*)WindowsManager.GetWindowPtr(hwndDlg);
        INT_PTR ret = FALSE; // pro pripad, ze ji nezpracuje
        if (dlg != NULL && dlg->Is(otDialog))
        {
            // Petr: posunul jsem dolu pod wnd->WindowProc(), aby behem WM_DESTROY
            //       jeste dochazely zpravy (potreboval Lukas)
            // WindowsManager.DetachWindow(hwndDlg);

            ret = dlg->DialogProc(uMsg, wParam, lParam);

            WindowsManager.DetachWindow(hwndDlg);
            if (dlg->IsAllocated())
                delete dlg;
            else
                dlg->HWindow = NULL; // informace o odpojeni
        }
        return ret;
    }

    default:
    {
        dlg = (CPropSheetPage*)WindowsManager.GetWindowPtr(hwndDlg);
#ifdef __DEBUG_WINLIB
        if (dlg != NULL && !dlg->Is(otPropSheetPage))
        {
            TRACE_CT(_T("This should never happen."));
            dlg = NULL;
        }
#endif
    }
    }
    //--- zavolani metody DialogProc(...) prislusneho objektu dialogu
    if (dlg != NULL)
        return dlg->DialogProc(uMsg, wParam, lParam);
    else
        return FALSE; // chyba nebo message neprisla mezi WM_INITDIALOG a WM_DESTROY
}

//
// ****************************************************************************
// CPropertyDialog
//

INT_PTR
CPropertyDialog::Execute()
{
    if (Count > 0)
    {
        CWinLibDPIContext dpiContext;
        PROPSHEETHEADER psh;
        psh.dwSize = sizeof(PROPSHEETHEADER);
        psh.dwFlags = Flags;
        psh.hwndParent = Parent;
        psh.hInstance = Modul;
        psh.hIcon = Icon;
        psh.pszCaption = Caption;
        psh.nPages = Count;
        if (StartPage < 0 || StartPage >= Count)
            StartPage = 0;
        psh.nStartPage = StartPage;
        HPROPSHEETPAGE* pages = new HPROPSHEETPAGE[Count];
        if (pages == NULL)
        {
            TRACE_ET(_T("Low memory!"));
            return -1;
        }
        psh.phpage = pages;
        for (int i = 0; i < Count; i++)
        {
            psh.phpage[i] = At(i)->CreatePropSheetPage();
            At(i)->ParentDialog = this;
        }
        psh.pfnCallback = Callback;
        INT_PTR ret = PropertySheet(&psh);
        delete pages;
        return ret;
    }
    else
    {
        TRACE_ET(_T("Incorrect call to CPropertyDialog::Execute."));
        return -1;
    }
}

int CPropertyDialog::GetCurSel()
{
    HWND tabCtrl = PropSheet_GetTabControl(HWindow);
    return TabCtrl_GetCurSel(tabCtrl);
}

//
// ****************************************************************************
// CTreePropDialog
//

#define _TPD_IDC_TREE 1
#define _TPD_IDC_HELP 9
#define _TPD_IDC_GRIP 10
#define _TPD_IDC_SEP 11
#define _TPD_IDC_CAPTION 3
#define _TPD_IDC_RECT 4
#define _TPD_IDC_OK 5
#define _TPD_WM_POST_INIT_REDRAW (WM_APP + 0x3A7)
#define _TPD_WM_POST_DPI_LAYOUT (WM_APP + 0x3A8)
// dimensions in dialog units
#define _TPD_LEFTMARGIN 4  // TreeView and caption left margin
#define _TPD_TOPMARGIN 4   // TreeView and caption top margin
#define _TPD_TREE_W 100    // TreeView width
#define _TPD_TREE_W_MAX 145 // max TreeView width in DLU (~219px at 96 DPI; matches English panel width)
#define _TPD_CAPTION_H 16  // caption height
#define _TPD_BUTTON_W 50   // button width
#define _TPD_BUTTON_H 14   // button height
#define _TPD_BUTTON_MARG 4 // button spacing

CTPHCaptionWindow::CTPHCaptionWindow(HWND hDlg, int ctrlID)
    : CWindow(hDlg, ctrlID, ooAllocated)
{
    Allocated = 0;
    Text = NULL;
}

CTPHCaptionWindow::~CTPHCaptionWindow()
{
    if (Text != NULL)
        free(Text);
}

void CTPHCaptionWindow::SetText(const TCHAR* text)
{
    int l = (int)_tcslen(text);
    if (Allocated < l + 2)
    {
        if (Text != NULL)
            free(Text);
        Text = (TCHAR*)malloc((l + 2) * sizeof(TCHAR));
        if (Text == NULL)
        {
            TRACE_ET(_T("Low memory!"));
            Allocated = 0;
            return;
        }
        Allocated = l + 2;
    }
    _tcscpy_s(Text, Allocated, text);
    InvalidateRect(HWindow, NULL, FALSE);
}

LRESULT
CTPHCaptionWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(HWindow, &ps);

        RECT r;
        GetClientRect(HWindow, &r);

        const bool useDark = DarkModeShouldUseDarkColors();
        const COLORREF background = useDark ? DarkModeGetDialogBackgroundColor() : GetSysColor(COLOR_BTNFACE);
        const int devCaps = GetDeviceCaps(hdc, NUMCOLORS);

        if (useDark)
        {
            HBRUSH hBrush = CreateSolidBrush(background);
            FillRect(hdc, &r, hBrush);
            DeleteObject(hBrush);

            COLORREF light = LightenColorSimple(background, 32);
            COLORREF shadow = DarkenColorSimple(background, 48);
            HPEN lightPen = CreatePen(PS_SOLID, 1, light);
            HPEN shadowPen = CreatePen(PS_SOLID, 1, shadow);
            HPEN oldPen = (HPEN)SelectObject(hdc, lightPen);
            MoveToEx(hdc, r.left, r.bottom - 1, NULL);
            LineTo(hdc, r.left, r.top);
            LineTo(hdc, r.right - 1, r.top);
            SelectObject(hdc, shadowPen);
            LineTo(hdc, r.right - 1, r.bottom - 1);
            LineTo(hdc, r.left, r.bottom - 1);
            SelectObject(hdc, oldPen);
            DeleteObject(lightPen);
            DeleteObject(shadowPen);
        }
        else if (devCaps == -1) // gradient pouzijeme pouze pri vice nez 256 barvach
        {
            HBRUSH hOldBrush = (HBRUSH)GetCurrentObject(hdc, OBJ_BRUSH);
#define TPH_STEPS 100
            double stepW = (double)(r.right - r.left + 1) / TPH_STEPS;
            RECT r2 = r;
            r2.right = (long)(r2.left + stepW + 1);
            COLORREF base = GetSysColor(COLOR_BTNFACE);
            for (int i = 0; i <= TPH_STEPS; i++)
            {
                LOGBRUSH lb;
                lb.lbStyle = BS_SOLID;
                lb.lbColor = RGB(max(GetRValue(base) - TPH_STEPS / 2 + i / 2 + 1, 0),
                                 max(GetGValue(base) - TPH_STEPS / 2 + i / 2 + 1, 0),
                                 max(GetBValue(base) - TPH_STEPS / 2 + i / 2 + 1, 0));
                HBRUSH hColorBrush = CreateBrushIndirect(&lb);
                FillRect(hdc, &r2, hColorBrush);
                DeleteObject(hColorBrush);
                r2.left = (long)(i * stepW);
                r2.right = (long)(r2.left + stepW + 1);
            }
        }
        else
            FillRect(hdc, &r, (HBRUSH)(COLOR_GRAYTEXT + 1));

        if (Text != NULL)
        {
            RECT textRect = r;
            textRect.left += 8;

            int oldBkMode = SetBkMode(hdc, TRANSPARENT);

            HFONT hOldFont = NULL;
            HFONT hFont = (HFONT)SendMessage(HWindow, WM_GETFONT, 0, 0);
            if (hFont == NULL)
                hFont = (HFONT)HANDLES(GetStockObject(DEFAULT_GUI_FONT));
            hOldFont = (HFONT)SelectObject(hdc, hFont);

            COLORREF textColor;
            if (useDark)
                textColor = DarkModeGetDialogTextColor();
            else if (devCaps == -1)
                textColor = GetSysColor(COLOR_BTNTEXT);
            else
                textColor = GetSysColor(COLOR_CAPTIONTEXT);
            int oldColor = SetTextColor(hdc, textColor);

            DrawText(hdc, Text, (int)_tcslen(Text), &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
            SetTextColor(hdc, oldColor);
            SelectObject(hdc, hOldFont);
            SetBkMode(hdc, oldBkMode);

        }
        EndPaint(HWindow, &ps);
        break;
    }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}

LRESULT
CTPHGripWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SETCURSOR:
    {
        // resize grip can now resize the dialog in both directions
        SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
        return TRUE;
    }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}

CTreePropHolderDlg::CTreePropHolderDlg(HWND hParent, DWORD* windowHeight, DWORD* windowWidth, DWORD* windowTreeWidth, int defaultWidthExtra)
    : CDialog(NULL, 0, hParent, ooStatic)
{
    HTreeView = NULL;
    ChildDialog = NULL;
    CaptionWindow = NULL;
    GripWindow = NULL;
    CurrentPageIndex = -1;
    ExitButton = -1;
    MinWindowSize.cx = 0;
    MinWindowSize.cy = 0;
    WindowHeight = windowHeight;
    WindowWidth = windowWidth;
    WindowTreeWidth = windowTreeWidth;
    PendingWindowHeight = 0;
    PendingWindowWidth = 0;
    DefaultWidthExtra = defaultWidthExtra;
    MinTreeWidth = 0;
    MinChildWidth = 0;
    TreeSplitDragging = FALSE;
    TreeWidthChanged = FALSE;
    CurrentDPI = USER_DEFAULT_SCREEN_DPI;
    LogicalMinWindowSize.cx = 0;
    LogicalMinWindowSize.cy = 0;
    LogicalTreeWidth = 0;
    LogicalMinTreeWidth = 0;
    LogicalMinChildWidth = 0;
    LogicalCaptionHeight = 0;
    LogicalButtonSize.cx = 0;
    LogicalButtonSize.cy = 0;
    LogicalButtonMargin = 0;
    LogicalMarginSize.cx = 0;
    LogicalMarginSize.cy = 0;
    LogicalWindowSize.cx = 0;
    LogicalWindowSize.cy = 0;
    DPIChangeInProgress = FALSE;
    DPILayoutPosted = FALSE;
    UserSizing = FALSE;
    TreeFont = NULL;
}

CTreePropHolderDlg::~CTreePropHolderDlg()
{
    if (TreeFont != NULL)
        HANDLES(DeleteObject(TreeFont));
}

INT_PTR
CTreePropHolderDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // WM_INITDIALOG zavolame az ve chvili, kdy budeme znat rozmery okna
    if (TPD != NULL && uMsg != WM_INITDIALOG)
        TPD->DialogProc(uMsg, wParam, lParam); // forward zprav
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        CurrentDPI = (int)WinLibDPIGetWindowDPI(HWindow);
        HWND hwnd = GetDlgItem(HWindow, _TPD_IDC_RECT);
        GetWindowRect(hwnd, &ChildDialogRect);
        POINT p;
        p.x = ChildDialogRect.left;
        p.y = ChildDialogRect.top;
        ScreenToClient(HWindow, &p);
        int w = ChildDialogRect.right - ChildDialogRect.left;
        int h = ChildDialogRect.bottom - ChildDialogRect.top;
        ChildDialogRect.left = p.x;
        ChildDialogRect.top = p.y;
        ChildDialogRect.right = p.x + w;
        ChildDialogRect.bottom = p.y + h;
        DestroyWindow(hwnd);
        HTreeView = GetDlgItem(HWindow, _TPD_IDC_TREE);
        SetWindowSubclass(HTreeView, TreeViewRedrawSubclassProc, TREE_PROP_TREE_REDRAW_SUBCLASS_ID, 0);
        UpdateTreeFontAndMetrics();
        BOOL appIsThemed = IsAppThemed();
        ApplyTreeViewColors(HTreeView);
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }

        int treeIndent = 0;
        if (appIsThemed)
        {
            RECT rect = {0, 0, 4, 8};
            MapDialogRect(HWindow, &rect); // ziskame baseUnitX a baseUnitY pro prepocet dlg-units na pixels
            treeIndent = MulDiv(9 /* odsazeni v dlg-units */, rect.right /* baseUnitX */, 4);
            TreeView_SetIndent(HTreeView, treeIndent);
        }
        TreeView_SetExtendedStyle(HTreeView, TVS_EX_AUTOHSCROLL, TVS_EX_AUTOHSCROLL);

        // dlg units -> pixels conversions
        RECT r = {_TPD_BUTTON_W, _TPD_BUTTON_H, _TPD_LEFTMARGIN, _TPD_TOPMARGIN};
        MapDialogRect(HWindow, &r);
        ButtonSize.cx = r.left;
        ButtonSize.cy = r.top;
        MarginSize.cx = r.right;
        MarginSize.cy = r.bottom;
        r = {_TPD_CAPTION_H, _TPD_BUTTON_MARG, 0, 0};
        MapDialogRect(HWindow, &r);
        CaptionHeight = r.left;
        ButtonMargin = r.top;

        CaptionWindow = new CTPHCaptionWindow(HWindow, _TPD_IDC_CAPTION);
        if (CaptionWindow == NULL)
            TRACE_ET(_T("Low memory!"));
        MinTreeWidth = BuildAndMeasureTree() + 2 * treeIndent + treeIndent / 2 +
                       WinLibDPIGetSystemMetric(HWindow, SM_CXVSCROLL);
        TreeWidth = MinTreeWidth;
        // Cap TreeView width so the dialog doesn't grow with longer translations (e.g. French vs English).
        {
            RECT maxTreeR = {_TPD_TREE_W_MAX, 0};
            MapDialogRect(HWindow, &maxTreeR);
            if (TreeWidth > maxTreeR.left)
                TreeWidth = maxTreeR.left;
            MinTreeWidth = TreeWidth;
        }
        if (TPD->StartPage < 0 || TPD->StartPage >= TPD->Count)
            TPD->StartPage = 0;
        TreeView_SelectItem(HTreeView, TPD->At(TPD->StartPage)->HTreeItem);

        GripWindow = new CTPHGripWindow(HWindow, _TPD_IDC_GRIP);
        DarkModeApplyWindow(GripWindow->HWindow);

        // default rozmery jsou minimalni - ulozime si je, abychom je nasledne mohli hlidat
        GetWindowRect(HWindow, &r);
        RECT cR;
        GetClientRect(HWindow, &cR);
        int marginW = (r.right - r.left) - cR.right;
        int marginH = (r.bottom - r.top) - cR.bottom;
        MinChildWidth = ChildDialogRect.right - ChildDialogRect.left;
        if (WindowTreeWidth != NULL && (int)*WindowTreeWidth > TreeWidth)
            TreeWidth = (int)*WindowTreeWidth;
        MinWindowSize.cx = MinTreeWidth + MinChildWidth + 3 * MarginSize.cx + marginW;
        MinWindowSize.cy = MarginSize.cy + CaptionHeight + MarginSize.cy +
                           ChildDialogRect.bottom - ChildDialogRect.top +
                           MarginSize.cy + 1 + MarginSize.cy +
                           ButtonSize.cy + MarginSize.cy + marginH;
        CaptureLogicalDpiMetrics();

        // nastavime uzivatelsky rozmer okna a provedeme layout prvku
        BOOL useDefaultWidthExtra = WindowWidth != NULL && *WindowWidth == 0;
        int width = WindowWidth != NULL ? (int)*WindowWidth : r.right - r.left;
        if (width < MinWindowSize.cx + TreeWidth - MinTreeWidth)
            width = MinWindowSize.cx + TreeWidth - MinTreeWidth;
        if (useDefaultWidthExtra)
            width += DefaultWidthExtra;
        int height = (int)*WindowHeight;
        RECT clipR; // nechceme byt vetsi nez vyska obrazovky
        MultiMonGetClipRectByWindow(HWindow, &clipR, NULL);
        if (height > clipR.bottom - clipR.top)
            height = clipR.bottom - clipR.top;
        if (height < MinWindowSize.cy)
            height = MinWindowSize.cy;
        SetWindowPos(HWindow, NULL, 0, 0, width, height,
                     SWP_NOZORDER | SWP_NOMOVE);
        GetWindowRect(HWindow, &r);
        LogicalWindowSize.cx = MulDiv(r.right - r.left, USER_DEFAULT_SCREEN_DPI, CurrentDPI);
        LogicalWindowSize.cy = MulDiv(r.bottom - r.top, USER_DEFAULT_SCREEN_DPI, CurrentDPI);

        LayoutControls();
        TreeView_EnsureVisible(HTreeView, TPD->At(TPD->StartPage)->HTreeItem);

        TPD->DialogProc(uMsg, wParam, lParam); // forward zprav

        // The first selected page can finish creating before the holder has its
        // final position/size. Queue one repaint after WM_INITDIALOG returns so
        // light-scheme controls are painted immediately instead of appearing only
        // after the window is moved.
        PostMessage(HWindow, _TPD_WM_POST_INIT_REDRAW, 0, 0);

        break;
    }

    case _TPD_WM_POST_INIT_REDRAW:
    {
        UpdateTreeFontAndMetrics();
        ApplyTreeViewColors(HTreeView);
        LayoutControls();
        if (ChildDialog != NULL && ChildDialog->HWindow != NULL)
        {
            ShowWindow(ChildDialog->HWindow, SW_SHOW);
            RepaintWindowTree(ChildDialog->HWindow);
        }
        RepaintWindowTree(HWindow);
        return TRUE;
    }

    case WM_DPICHANGED:
    {
        int newDPI = LOWORD(wParam);
        if (newDPI > 0 && CurrentDPI > 0 && newDPI != CurrentDPI)
        {
            DPIChangeInProgress = TRUE;
            // Arm the visible page before DefDlgProc starts the PMv2 child
            // cascade. WM_SIZE can arrive before
            // WM_DPICHANGED_BEFOREPARENT, and its anchor layout would
            // otherwise move controls while Windows is still scaling them.
            if (ChildDialog != NULL)
                ChildDialog->DPIChangeInProgress = TRUE;
            ApplyLogicalDpiMetrics(newDPI);
            CurrentDPI = newDPI;
            if (!DPILayoutPosted)
            {
                DPILayoutPosted = TRUE;
                PostMessage(HWindow, _TPD_WM_POST_DPI_LAYOUT, 0, 0);
            }
        }
        // Return through CDialog so DefDlgProc can perform the PMv2 dialog and
        // child-control scaling exactly once.
        break;
    }

    case WM_GETDPISCALEDSIZE:
    {
        SIZE* scaledSize = reinterpret_cast<SIZE*>(lParam);
        int newDPI = static_cast<int>(wParam);
        if (scaledSize != NULL && newDPI > 0 &&
            LogicalWindowSize.cx > 0 && LogicalWindowSize.cy > 0)
        {
            // Supply an exact size derived from the persistent 96-DPI
            // baseline. This prevents Windows from feeding a rounded previous
            // monitor size back into the next 100% <-> 150% transition.
            scaledSize->cx = MulDiv(LogicalWindowSize.cx, newDPI, USER_DEFAULT_SCREEN_DPI);
            scaledSize->cy = MulDiv(LogicalWindowSize.cy, newDPI, USER_DEFAULT_SCREEN_DPI);
            DPIChangeInProgress = TRUE;
            if (ChildDialog != NULL)
                ChildDialog->DPIChangeInProgress = TRUE;
            return TRUE;
        }
        break;
    }

    case _TPD_WM_POST_DPI_LAYOUT:
    {
        DPILayoutPosted = FALSE;
        // WM_GETDPISCALEDSIZE already supplied the exact outer size from the
        // stable 96-DPI baseline. Do not fight the PMv2 dialog manager here;
        // a second SetWindowPos during the same monitor transition can feed a
        // rounded physical size back into later move cycles.
        UpdateTreeFontAndMetrics();
        ApplyTreeViewColors(HTreeView);
        LayoutControls();
        DPIChangeInProgress = FALSE;
        RECT windowRect;
        if (GetWindowRect(HWindow, &windowRect))
        {
            PendingWindowWidth = windowRect.right - windowRect.left;
            PendingWindowHeight = windowRect.bottom - windowRect.top;
        }
        if (ChildDialog != NULL && ChildDialog->HWindow != NULL)
            RepaintWindowTree(ChildDialog->HWindow);
        RepaintWindowTree(HWindow);
        return TRUE;
    }

    case WM_HELP:
    {
        if (WinLibHelp != NULL && ChildDialog != NULL && ChildDialog->HelpID != -1)
        {
            WinLibHelp->OnHelp(HWindow, ChildDialog->HelpID, (HELPINFO*)lParam,
                               (GetKeyState(VK_CONTROL) & 0x8000) != 0,
                               (GetKeyState(VK_SHIFT) & 0x8000) != 0);
        }
        return TRUE; // F1 nenechame propadnout do parenta ani pokud nevolame WinLibHelp->OnHelp()
    }

    case WM_CLOSE:
    {
        ExitButton = IDCANCEL;
        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case _TPD_IDC_HELP:
        {
            if (WinLibHelp != NULL && ChildDialog != NULL && ChildDialog->HelpID != -1)
            {
                HELPINFO hi;
                memset(&hi, 0, sizeof(hi));
                hi.cbSize = sizeof(hi);
                hi.iContextType = HELPINFO_WINDOW;
                hi.dwContextId = ChildDialog->HelpID;
                GetCursorPos(&hi.MousePos);
                WinLibHelp->OnHelp(HWindow, ChildDialog->HelpID, &hi, FALSE, FALSE);
            }
            else
                TRACE_ET(_T("CTreePropHolderDlg::DialogProc(): ignoring _TPD_IDC_HELP: SetupWinLibHelp() was not called or ChildDialog is NULL or ChildDialog->HelpID is -1!"));
            return TRUE;
        }

        case _TPD_IDC_OK:
        {
            // musim provest validaci aktualni stranky
            if (!ChildDialog->ValidateData())
                return TRUE;

            // obehnu vsechny stranky pro transfer
            for (int i = 0; i < TPD->Count; i++)
                if (TPD->At(i)->HWindow != NULL)
                    if (!TPD->At(i)->TransferData(ttDataFromWindow))
                        return TRUE;
            wParam = IDOK;
            SaveWindowPlacement();
        }
        case IDCANCEL:
        {
            ExitButton = LOWORD(wParam);
            return TRUE;
        }
        }

        // preposleme message, aby chodil enter na default tlacitka
        if (ChildDialog != NULL && HIWORD(wParam) == BN_CLICKED)
            ::SendMessage(ChildDialog->HWindow, uMsg, wParam, lParam);

        break;
    }

    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        if (pnmh->hwndFrom == HTreeView)
        {
            switch (pnmh->code)
            {
            case TVN_SELCHANGING:
            {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                CPropSheetPage* page = (CPropSheetPage*)pnmtv->itemOld.lParam;
                if (page != NULL && page->HWindow != NULL)
                {
                    NMHDR nmhdr;
                    nmhdr.hwndFrom = HWindow;
                    nmhdr.idFrom = _TPD_IDC_TREE;
                    nmhdr.code = PSN_KILLACTIVE;
                    SendMessage(page->HWindow, WM_NOTIFY, _TPD_IDC_TREE, (LPARAM)&nmhdr);
                    LONG_PTR res = GetWindowLongPtr(page->HWindow, DWLP_MSGRESULT);
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, res);
                    return TRUE;
                }
                break;
            }

            case TVN_SELCHANGED:
            {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                CPropSheetPage* page = (CPropSheetPage*)pnmtv->itemNew.lParam;
                if (page != NULL)
                {
                    for (int i = 0; i < TPD->Count; i++)
                        if (TPD->At(i) == page)
                        {
                            SelectPage(i);
                            NMHDR nmhdr;
                            nmhdr.hwndFrom = HWindow;
                            nmhdr.idFrom = _TPD_IDC_TREE;
                            nmhdr.code = PSN_SETACTIVE;
                            SendMessage(page->HWindow, WM_NOTIFY, _TPD_IDC_TREE, (LPARAM)&nmhdr);
                            break;
                        }
                }
                break;
            }

            case TVN_ITEMEXPANDED:
            {
                LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
                CPropSheetPage* page = (CPropSheetPage*)pnmtv->itemNew.lParam;
                if (page != NULL && page->Expanded != NULL)
                    *page->Expanded = (pnmtv->itemNew.state & TVIS_EXPANDED) != 0;
                break;
            }
            }
        }
        break;
    }

    case WM_NCHITTEST:
    {
        LRESULT ht = DefWindowProc(HWindow, uMsg, wParam, lParam);
        SetWindowLongPtr(HWindow, DWLP_MSGRESULT, ht);
        return TRUE;
    }

    case WM_GETMINMAXINFO:
    {
        LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
        lpmmi->ptMinTrackSize.x = MinWindowSize.cx + TreeWidth - MinTreeWidth;
        lpmmi->ptMinTrackSize.y = MinWindowSize.cy;

        // Adjust the height: https://blogs.msdn.microsoft.com/oldnewthing/20150504-00/?p=44944
        // FIXME
        //MONITORINFO mi = { sizeof(mi) };
        //GetMonitorInfo(MonitorFromWindow(HWindow,
        //               MONITOR_DEFAULTTOPRIMARY), &mi);
        //lpmmi->ptMaxSize.y = mi.rcWork.bottom - mi.rcWork.top -
        //                     lpmmi->ptMaxPosition.y + rc.bottom;
        break;
    }

    case WM_SIZE:
    {
        if (!DPIChangeInProgress)
        {
            RECT r;
            GetWindowRect(HWindow, &r);
            PendingWindowHeight = r.bottom - r.top;
            PendingWindowWidth = r.right - r.left;
            LayoutControls();
            RepaintWindowTree(HWindow);
        }
        break;
    }

    case WM_SYSCOMMAND:
    {
        UINT command = (UINT)(wParam & 0xFFF0);
        if (command == SC_SIZE)
            UserSizing = TRUE;
        else if (command == SC_MOVE)
            UserSizing = FALSE;
        break;
    }

    case WM_ENTERSIZEMOVE:
    {
        // WM_ENTERSIZEMOVE covers both moving and sizing. UserSizing was
        // selected by the preceding SC_MOVE/SC_SIZE command; a pure move
        // across monitors must never replace the persistent 96-DPI baseline.
        break;
    }

    case WM_EXITSIZEMOVE:
    {
        if (UserSizing)
        {
            UserSizing = FALSE;
            RECT r;
            if (GetWindowRect(HWindow, &r))
            {
                PendingWindowWidth = r.right - r.left;
                PendingWindowHeight = r.bottom - r.top;
                if (CurrentDPI > 0)
                {
                    LogicalWindowSize.cx = MulDiv(
                        PendingWindowWidth, USER_DEFAULT_SCREEN_DPI, CurrentDPI);
                    LogicalWindowSize.cy = MulDiv(
                        PendingWindowHeight, USER_DEFAULT_SCREEN_DPI, CurrentDPI);
                }
            }
        }
        UserSizing = FALSE;
        break;
    }


    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int splitX = MarginSize.cx + TreeWidth + MarginSize.cx / 2;
        if (abs(x - splitX) <= max(3, MarginSize.cx))
        {
            TreeSplitDragging = TRUE;
            SetCapture(HWindow);
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
        }
        break;
    }

    case WM_LBUTTONUP:
    {
        if (TreeSplitDragging)
        {
            TreeSplitDragging = FALSE;
            ReleaseCapture();
            return TRUE;
        }
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        if (TreeSplitDragging)
        {
            RECT cRect;
            GetClientRect(HWindow, &cRect);
            int newTreeWidth = x - MarginSize.cx;
            int maxTreeWidth = cRect.right - MinChildWidth - 3 * MarginSize.cx;
            if (maxTreeWidth < MinTreeWidth)
                maxTreeWidth = MinTreeWidth;
            if (newTreeWidth < MinTreeWidth)
                newTreeWidth = MinTreeWidth;
            if (newTreeWidth > maxTreeWidth)
                newTreeWidth = maxTreeWidth;
            if (newTreeWidth != TreeWidth)
            {
                TreeWidth = newTreeWidth;
                LogicalTreeWidth = WinLibDPIToLogical(HWindow, TreeWidth);
                TreeWidthChanged = TRUE;
                LayoutControls();
            }
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
            return TRUE;
        }
        int splitX = MarginSize.cx + TreeWidth + MarginSize.cx / 2;
        if (abs(x - splitX) <= max(3, MarginSize.cx))
            SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        break;
    }

    case WM_SYSCOLORCHANGE:
    {
        ApplyTreeViewColors(HTreeView);
        break;
    }

    case WM_THEMECHANGED:
    {
        ApplyTreeViewColors(HTreeView);
        break;
    }

    case WM_SETTINGCHANGE:
    {
        ApplyTreeViewColors(HTreeView);
        break;
    }
    }
    return CDialog::DialogProc(uMsg, wParam, lParam);
}

void CTreePropHolderDlg::CaptureLogicalDpiMetrics()
{
    LogicalMinWindowSize.cx = WinLibDPIToLogical(HWindow, MinWindowSize.cx);
    LogicalMinWindowSize.cy = WinLibDPIToLogical(HWindow, MinWindowSize.cy);
    LogicalTreeWidth = WinLibDPIToLogical(HWindow, TreeWidth);
    LogicalMinTreeWidth = WinLibDPIToLogical(HWindow, MinTreeWidth);
    LogicalMinChildWidth = WinLibDPIToLogical(HWindow, MinChildWidth);
    LogicalCaptionHeight = WinLibDPIToLogical(HWindow, CaptionHeight);
    LogicalButtonSize.cx = WinLibDPIToLogical(HWindow, ButtonSize.cx);
    LogicalButtonSize.cy = WinLibDPIToLogical(HWindow, ButtonSize.cy);
    LogicalButtonMargin = WinLibDPIToLogical(HWindow, ButtonMargin);
    LogicalMarginSize.cx = WinLibDPIToLogical(HWindow, MarginSize.cx);
    LogicalMarginSize.cy = WinLibDPIToLogical(HWindow, MarginSize.cy);
}

void CTreePropHolderDlg::ApplyLogicalDpiMetrics(int dpi)
{
#define TPD_FROM_LOGICAL(value) MulDiv(value, dpi, USER_DEFAULT_SCREEN_DPI)
    MinWindowSize.cx = TPD_FROM_LOGICAL(LogicalMinWindowSize.cx);
    MinWindowSize.cy = TPD_FROM_LOGICAL(LogicalMinWindowSize.cy);
    TreeWidth = TPD_FROM_LOGICAL(LogicalTreeWidth);
    MinTreeWidth = TPD_FROM_LOGICAL(LogicalMinTreeWidth);
    MinChildWidth = TPD_FROM_LOGICAL(LogicalMinChildWidth);
    CaptionHeight = TPD_FROM_LOGICAL(LogicalCaptionHeight);
    ButtonSize.cx = TPD_FROM_LOGICAL(LogicalButtonSize.cx);
    ButtonSize.cy = TPD_FROM_LOGICAL(LogicalButtonSize.cy);
    ButtonMargin = TPD_FROM_LOGICAL(LogicalButtonMargin);
    MarginSize.cx = TPD_FROM_LOGICAL(LogicalMarginSize.cx);
    MarginSize.cy = TPD_FROM_LOGICAL(LogicalMarginSize.cy);
#undef TPD_FROM_LOGICAL
}

void CTreePropHolderDlg::UpdateTreeFontAndMetrics()
{
    if (HTreeView == NULL)
        return;

    // The dialog manager does not reliably replace an already assigned HFONT
    // for the subclassed caption/tree during a PMv2 transition. Build the
    // template's 9-point Segoe UI font from the explicit notification DPI.
    LOGFONT lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfHeight = -MulDiv(9, CurrentDPI, 72);
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = ANSI_CHARSET;
    lf.lfQuality = CLEARTYPE_QUALITY;
    _tcscpy_s(lf.lfFaceName, _T("Segoe UI"));
    HFONT newFont = HANDLES(CreateFontIndirect(&lf));
    if (newFont != NULL)
    {
        HFONT oldFont = TreeFont;
        TreeFont = newFont;
        SendMessage(HTreeView, WM_SETFONT, (WPARAM)TreeFont, TRUE);
        if (CaptionWindow != NULL)
            SendMessage(CaptionWindow->GetHWND(), WM_SETFONT, (WPARAM)TreeFont, TRUE);
        if (oldFont != NULL)
            HANDLES(DeleteObject(oldFont));
    }

    if (TreeFont != NULL)
    {
        HDC dc = HANDLES(GetDC(HTreeView));
        if (dc != NULL)
        {
            TEXTMETRIC tm;
            HFONT oldFont = (HFONT)SelectObject(dc, TreeFont);
            if (GetTextMetrics(dc, &tm))
                TreeView_SetItemHeight(HTreeView, tm.tmHeight + MulDiv(4, CurrentDPI, USER_DEFAULT_SCREEN_DPI));
            SelectObject(dc, oldFont);
            HANDLES(ReleaseDC(HTreeView, dc));
        }
    }
}

void CTreePropHolderDlg::LayoutControls()
{
    if (ChildDialog->HWindow == NULL)
        return;

    RECT cRect;
    GetClientRect(HWindow, &cRect);

    int sepY = cRect.bottom - MarginSize.cy - ButtonSize.cy - MarginSize.cy - 1;

    GripSize.cx = WinLibDPIGetSystemMetric(HWindow, SM_CXVSCROLL);
    GripSize.cy = WinLibDPIGetSystemMetric(HWindow, SM_CYHSCROLL);

    HDWP hdwp = HANDLES(BeginDeferWindowPos(8));
    if (hdwp != NULL)
    {
        // treeview
        hdwp = HANDLES(DeferWindowPos(hdwp, HTreeView, NULL,
                                      MarginSize.cx,
                                      MarginSize.cy,
                                      TreeWidth,
                                      sepY - 2 * MarginSize.cy,
                                      SWP_NOZORDER));
        // caption
        int captionX = MarginSize.cx + TreeWidth + MarginSize.cx;
        HWND hCaption = GetDlgItem(HWindow, _TPD_IDC_CAPTION);
        hdwp = HANDLES(DeferWindowPos(hdwp, hCaption, NULL,
                                      captionX,
                                      MarginSize.cy,
                                      cRect.right - MarginSize.cx - captionX,
                                      CaptionHeight,
                                      SWP_NOZORDER));
        // child dialog
        int dlgX = MarginSize.cx + TreeWidth + MarginSize.cx;
        int dlgY = MarginSize.cy + CaptionHeight + MarginSize.cy;
        ChildDialogRect.left = dlgX;
        ChildDialogRect.top = dlgY;
        ChildDialogRect.right = cRect.right - MarginSize.cx;
        ChildDialogRect.bottom = sepY - MarginSize.cy;
        hdwp = HANDLES(DeferWindowPos(hdwp, ChildDialog->HWindow, NULL,
                                      ChildDialogRect.left,
                                      ChildDialogRect.top,
                                      ChildDialogRect.right - ChildDialogRect.left,
                                      ChildDialogRect.bottom - ChildDialogRect.top,
                                      SWP_NOZORDER));
        // separator
        HWND hSeparator = GetDlgItem(HWindow, _TPD_IDC_SEP);
        hdwp = HANDLES(DeferWindowPos(hdwp, hSeparator, NULL,
                                      MarginSize.cx,
                                      sepY,
                                      cRect.right - 2 * MarginSize.cx,
                                      1,
                                      SWP_NOZORDER));
        // OK button
        int buttonsX = cRect.right - (3 * ButtonSize.cx + 2 * ButtonMargin) - GripSize.cx / 2;
        int buttonsY = sepY + 1 + MarginSize.cy;
        HWND hOK = GetDlgItem(HWindow, _TPD_IDC_OK);
        hdwp = HANDLES(DeferWindowPos(hdwp, hOK, NULL,
                                      buttonsX,
                                      buttonsY,
                                      ButtonSize.cx,
                                      ButtonSize.cy,
                                      SWP_NOZORDER));
        // Cancel button
        HWND hCancel = GetDlgItem(HWindow, IDCANCEL);
        hdwp = HANDLES(DeferWindowPos(hdwp, hCancel, NULL,
                                      buttonsX + ButtonSize.cx + ButtonMargin,
                                      buttonsY,
                                      ButtonSize.cx,
                                      ButtonSize.cy,
                                      SWP_NOZORDER));
        // Help button
        HWND hHelp = GetDlgItem(HWindow, _TPD_IDC_HELP);
        hdwp = HANDLES(DeferWindowPos(hdwp, hHelp, NULL,
                                      buttonsX + 2 * ButtonSize.cx + 2 * ButtonMargin,
                                      buttonsY,
                                      ButtonSize.cx,
                                      ButtonSize.cy,
                                      SWP_NOZORDER));
        // Grip (resize)
        HWND hGrip = GetDlgItem(HWindow, _TPD_IDC_GRIP);
        hdwp = HANDLES(DeferWindowPos(hdwp, hGrip, NULL,
                                      cRect.right - GripSize.cx,
                                      cRect.bottom - GripSize.cy,
                                      GripSize.cx,
                                      GripSize.cy,
                                      SWP_NOZORDER));

        HANDLES(EndDeferWindowPos(hdwp));
        // hack: TreeView/common controls apparently have a bug: if a scrollbar appears because of the content,
        // the selected item is not redrawn, so it gets clipped on the right; this may be related to full-row
        // selection and the Aero look; in any case, repainting under W7 does not flicker, so we can probably afford it
        RepaintWindowTree(HTreeView);
        RepaintWindowTree(HWindow);
    }
}

int CTreePropHolderDlg::BuildAndMeasureTree()
{
    int width = 0;
    for (int i = 0; i < TPD->Count; i++)
    {
        TVINSERTSTRUCT tvis;
        tvis.hParent = NULL;
        if (TPD->At(i)->ParentPage != NULL)
            tvis.hParent = TPD->At(i)->ParentPage->HTreeItem;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
        tvis.item.pszText = TPD->At(i)->Title;
        tvis.item.cchTextMax = (int)_tcslen(TPD->At(i)->Title);
        tvis.item.state = 0;
        // POZOR: expandovatelne polozky zde musime expandovat, jinak nasledne TreeView_GetItemRect() vrati FALSE
        // a nahodna data v obdelniku RECT r
        if (TPD->At(i)->Expanded != NULL)
            tvis.item.state |= TVIS_EXPANDED;
        tvis.item.stateMask = tvis.item.state;
        tvis.item.lParam = (LPARAM)TPD->At(i);
        TPD->At(i)->HTreeItem = TreeView_InsertItem(HTreeView, &tvis);
        RECT r;
        // Radeji navratovou hodnotu TreeView_GetItemRect() vezmeme v potaz
        if (TreeView_GetItemRect(HTreeView, TPD->At(i)->HTreeItem, &r, TRUE) && r.right - r.left > width)
            width = r.right - r.left;
    }
    // Nyni jiz muzeme pozavirat neexpandovane polozky
    for (int i = 0; i < TPD->Count; i++)
    {
        if (TPD->At(i)->Expanded != NULL && *TPD->At(i)->Expanded == FALSE)
            TreeView_Expand(HTreeView, TPD->At(i)->HTreeItem, TVE_COLLAPSE);
    }
    return width;
}

void CTreePropHolderDlg::EnableButtons()
{
}

BOOL CTreePropHolderDlg::SelectPage(int pageIndex)
{
    if (pageIndex < 0 || pageIndex >= TPD->Count)
        return FALSE;
    HWND HHideWindow = NULL;
    if (ChildDialog != NULL)
    {
        HHideWindow = ChildDialog->HWindow;
        ChildDialog = NULL;
    }

    if (pageIndex != CurrentPageIndex)
    {
        ChildDialog = TPD->At(pageIndex);
        if (ChildDialog->HWindow == NULL)
        {
            ChildDialog->SetParent(HWindow);
            ChildDialog->Create();
            if (WinLib_DarkMode_ShouldApplyDialogTree(ChildDialog->HWindow))
            {
                DarkModeApplyTree(ChildDialog->HWindow);
                WinLib_DarkMode_PostDeferredRedraw(ChildDialog->HWindow);
            }
        }

        NMHDR nmhdr;
        nmhdr.hwndFrom = HWindow;
        nmhdr.idFrom = 0;
        nmhdr.code = PSN_SETACTIVE;
        SendMessage(ChildDialog->HWindow, WM_NOTIFY, 0, (LPARAM)&nmhdr);

        if (HHideWindow != NULL)
            SetWindowPos(HHideWindow, 0, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_NOREDRAW | SWP_HIDEWINDOW);

        CaptionWindow->SetText(ChildDialog->Title);

        SetWindowPos(ChildDialog->HWindow, HTreeView,
                     ChildDialogRect.left, ChildDialogRect.top,
                     ChildDialogRect.right - ChildDialogRect.left,
                     ChildDialogRect.bottom - ChildDialogRect.top,
                     SWP_SHOWWINDOW);
        RepaintWindowTree(ChildDialog->HWindow);
        RepaintWindowTree(HWindow);
        CurrentPageIndex = pageIndex;
        EnableButtons();
    }
    return TRUE;
}

void CTreePropHolderDlg::SaveWindowPlacement()
{
    if (WindowHeight != NULL)
        *WindowHeight = PendingWindowHeight;
    if (WindowWidth != NULL)
        *WindowWidth = PendingWindowWidth;
    if (WindowTreeWidth != NULL && TreeWidthChanged)
        *WindowTreeWidth = TreeWidth;
}

void CTreePropHolderDlg::OnCtrlTab(BOOL shift)
{
    int pageIndex = CurrentPageIndex;
    pageIndex += shift ? -1 : 1;
    if (pageIndex < 0)
        pageIndex = TPD->Count - 1;
    if (pageIndex >= TPD->Count)
        pageIndex = 0;
    if (pageIndex != CurrentPageIndex)
    {
        if (TreeView_SelectItem(HTreeView, TPD->At(pageIndex)->HTreeItem))
        {
            HWND hFocus = GetFocus();
            if (hFocus != HTreeView)
            {
                HWND hFirst = GetNextDlgTabItem(ChildDialog->HWindow, NULL, FALSE);
                if (hFirst != NULL)
                    SetFocus(hFirst);
                else
                    SetFocus(HTreeView);
            }
        }
    }
}

int CTreePropHolderDlg::ExecuteIndirect(LPCDLGTEMPLATE hDialogTemplate)
{
    HWND hOldFocus = GetFocus();
    EnableWindow(Parent, FALSE);
    CWinLibDPIContext dpiContext;
    CreateDialogIndirectParam(Modul, hDialogTemplate, Parent,
                              (DLGPROC)CDialog::CDialogProc, (LPARAM)this);
    MSG msg;
    while (ExitButton == -1 && GetMessage(&msg, NULL, 0, 0))
    {
        CWindowsObject* wnd = WindowsManager.GetWindowPtr(GetActiveWindow());
        if ((msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) &&
            msg.wParam == VK_TAB && (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0)
        {
            if (msg.message == WM_KEYDOWN)
                OnCtrlTab((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
        }
        else if (wnd == NULL || !wnd->Is(otDialog) || !IsDialogMessage(wnd->HWindow, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    EnableWindow(Parent, TRUE);
    DestroyWindow(HWindow);
    SetFocus(hOldFocus);
    return ExitButton;
}

WORD* CTreePropDialog::lpdwAlign(WORD* lpIn)
{
    DWORD_PTR ul;
    ul = (DWORD_PTR)lpIn;
    ul += 3;
    ul >>= 2;
    ul <<= 2;
    return (WORD*)ul;
}

int WinLibCopyText(WCHAR* buf, const TCHAR* text, int bufLen)
{
#ifdef UNICODE
    lstrcpyn(buf, text, bufLen);
    return (int)wcslen(buf) + 1;
#else  // UNICODE
    return MultiByteToWideChar(CP_ACP, 0, text, -1, buf, bufLen);
#endif // UNICODE
}

int CTreePropDialog::AddItemEx(LPWORD& lpw, const TCHAR* className, WORD id, int x, int y, int cx, int cy,
                               UINT style, UINT exStyle, const TCHAR* text)
{
    lpw = lpdwAlign(lpw); // align DLGITEMTEMPLATEEX on DWORD boundary
    *(DWORD*)lpw = 0;     // helpID
    lpw += 2;
    *(DWORD*)lpw = exStyle; // exStyle
    lpw += 2;
    *(DWORD*)lpw = style; // style
    lpw += 2;
    *lpw++ = x;
    *lpw++ = y;
    *lpw++ = cx;
    *lpw++ = cy;
    *(DWORD*)lpw = id; // id
    lpw += 2;
    LPWSTR lpwsz = (LPWSTR)lpw;
    lpw += WinLibCopyText(lpwsz, className, 50);
    if (text == NULL)
        *lpw++ = 0; // no name
    else
    {
        lpwsz = (LPWSTR)lpw;
        lpw += WinLibCopyText(lpwsz, text, 50);
    }
    *lpw++ = 0; // no creation data
    return TRUE;
}

int CTreePropDialog::Execute(const TCHAR* buttonOK,
                             const TCHAR* buttonCancel,
                             const TCHAR* buttonHelp)
{
    if (Count > 0)
    {
        RECT maxPageRect;
        SetRectEmpty(&maxPageRect);

        // zjistim maximalni rozmery
        for (int i = 0; i < Count; i++)
        {
            At(i)->ParentDialog = this;
            HRSRC hrsrc = FindResource(Modul, MAKEINTRESOURCE(At(i)->ResID), RT_DIALOG);
            if (hrsrc == NULL)
            {
                TRACE_ET(_T("Unable to find resource for page number: ") << i);
                return 0;
            }
            HGLOBAL hglb = LoadResource(Modul, hrsrc);
            WORD* pageTemplate = (WORD*)LockResource(hglb);
            if (pageTemplate == NULL)
            {
                TRACE_ET(_T("Unable to find resource for page number: ") << i);
                return 0;
            }
            BOOL dlgEx = *pageTemplate /*dlgVer*/ == 1 && *(pageTemplate + 1) /*signature*/ == 0xffff; // DLGEX
            if (!dlgEx)
                TRACE_CT(_T("CTreePropDialog::Execute(): DLG is no longer supported! PageResID=") << At(i)->ResID);
            DWORD dlgStyle = 0;
            short dlgCX = 0;
            short dlgCY = 0;
            WCHAR* dlgTitle = NULL;

            dlgStyle = *(DWORD*)(pageTemplate + 6); // style
            dlgCX = *(short*)(pageTemplate + 11);   // cx
            dlgCY = *(short*)(pageTemplate + 12);   // cy
            WORD* t = pageTemplate + 13;            // menu, preskocime na tridu dialogu, a pak na jeho titulek
            if (*t == 0)
                t++; // zadne menu
            else
            {
                if (*t == 0xffff)
                    t += 2; // menu ID
                else
                    t += wcslen((wchar_t*)t) + 1; // menu string
            }
            if (*t == 0)
                t++; // zadna trida dialogu
            else
            {
                if (*t == 0xffff)
                    t += 2; // ID tridy dialogu
                else
                    t += wcslen((wchar_t*)t) + 1; // string tridy dialogu
            }
            dlgTitle = (WCHAR*)t;

            if (At(i)->Title == NULL)
            {
                int len = 1;
                if (dlgStyle & WS_CAPTION)
                {
#ifdef UNICODE
                    len += (int)wcslen(dlgTitle) + 1;
#else  // UNICODE
                    len += WideCharToMultiByte(CP_ACP, 0, dlgTitle, -1, NULL, 0, NULL, NULL);
#endif // UNICODE
                }
                At(i)->Title = new TCHAR[len];
                if (len > 1)
                {
#ifdef UNICODE
                    lstrcpyn(At(i)->Title, dlgTitle, len);
#else  // UNICODE
                    WideCharToMultiByte(CP_ACP, 0, dlgTitle, -1, At(i)->Title, len, NULL, NULL);
#endif // UNICODE
                }
                At(i)->Title[len - 1] = 0;
            }

            if (dlgCX > maxPageRect.right)
                maxPageRect.right = dlgCX;
            if (dlgCY > maxPageRect.bottom)
                maxPageRect.bottom = dlgCY;
        }

        // vyska od spodniho okraje dialogu ke spodku TreeView a ChildDialogu
        int lowMargin = 2 * _TPD_TOPMARGIN + _TPD_BUTTON_H + _TPD_TOPMARGIN + _TPD_TOPMARGIN / 2;
        SIZE dialogSize;
        dialogSize.cx = _TPD_LEFTMARGIN + _TPD_TREE_W + _TPD_LEFTMARGIN + maxPageRect.right +
                        _TPD_LEFTMARGIN;
        dialogSize.cy = _TPD_TOPMARGIN + _TPD_CAPTION_H + _TPD_TOPMARGIN + maxPageRect.bottom +
                        lowMargin;

        // postavim template dialogu: DLG nebo DLGEX, podle formatu stranek, musi byt shodny,
        // jinak dochazi k orezu controlu a lisi se pisma stranek a zbytku tree property dialogu

        HGLOBAL hgbl;

        LPWORD lpw;
        LPWSTR lpwsz;
        hgbl = GlobalAlloc(GMEM_ZEROINIT, 1024);
        if (!hgbl)
            return -1;
        lpw = (LPWORD)GlobalLock(hgbl); // Define a dialog box.
        *lpw++ = 1;
        *lpw++ = 0xffff;  // DLGEX
        *(DWORD*)lpw = 0; // helpID
        lpw += 2;
        *(DWORD*)lpw = 0; // exStyle
        lpw += 2;
        // style
        *(DWORD*)lpw = WS_VISIBLE | WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION |
                       DS_SETFONT | DS_MODALFRAME | DS_CENTER | WS_SIZEBOX;
        lpw += 2;
        *lpw++ = 8; // cDlgItems (number of controls)
        *lpw++ = 0; // x
        *lpw++ = 0; // y
        *lpw++ = 0; // cx
        *lpw++ = 0; // cy
        *lpw++ = 0; // no menu
        *lpw++ = 0; // predefined dialog box class (by default)
        lpwsz = (LPWSTR)lpw;
        lpw += WinLibCopyText(lpwsz, Caption, 100); // title
        *lpw++ = 9;                                 // font size, kept in sync with resource property pages
        *lpw++ = FW_NORMAL;                         // font weight
        *(BYTE*)lpw = FALSE;                        // is font italic?
        *((BYTE*)lpw + 1) = ANSI_CHARSET;           // font charset
        lpw++;
        lpwsz = (LPWSTR)lpw; // font typeface
        lpw += WinLibCopyText(lpwsz, _T("Segoe UI"), 50);

        BOOL appIsThemed = IsAppThemed();

        // TreeView
        AddItemEx(lpw, WC_TREEVIEW, _TPD_IDC_TREE,
                  0, 0, 0, 0,
                  WS_BORDER | WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                      TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_SHOWSELALWAYS |
                      (appIsThemed ? TVS_FULLROWSELECT : TVS_HASLINES),
                  0, NULL);
        // Caption
        AddItemEx(lpw, _T("static"), _TPD_IDC_CAPTION,
                  0, 0, 0, 0,
                  WS_CHILD | WS_VISIBLE, 0, NULL);
        // Static, ktery je behem initu nahrazen child dialogem
        AddItemEx(lpw, _T("static"), _TPD_IDC_RECT,
                  0, 0, maxPageRect.right, maxPageRect.bottom,
                  WS_CHILD, 0, NULL);
        // Separator
        AddItemEx(lpw, _T("static"), _TPD_IDC_SEP,
                  0, 0, 0, 0,
                  WS_GROUP | WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, 0, NULL);
        // Spodni rada tlacitek
        AddItemEx(lpw, _T("button"), _TPD_IDC_OK,
                  0, 0, 0, 0,
                  WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 0, buttonOK);
        AddItemEx(lpw, _T("button"), IDCANCEL,
                  0, 0, 0, 0,
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 0, buttonCancel);
        AddItemEx(lpw, _T("button"), _TPD_IDC_HELP,
                  0, 0, 0, 0,
                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP, 0, buttonHelp);
        // Grip (resize)
        AddItemEx(lpw, _T("scrollbar"), _TPD_IDC_GRIP,
                  0, 0, 0, 0,
                  WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | SBS_SIZEBOX | SBS_SIZEBOXBOTTOMRIGHTALIGN, 0, _T(""));

        GlobalUnlock(hgbl);

        return Dialog.ExecuteIndirect((LPDLGTEMPLATE)hgbl);
    }
    else
    {
        TRACE_ET(_T("Incorrect call to CPropertyDialog::Execute."));
        return -1;
    }
}

int CTreePropDialog::GetCurSel()
{
    TVITEM item;
    item.hItem = TreeView_GetSelection(Dialog.HTreeView);
    item.mask = TVIF_PARAM;
    TreeView_GetItem(Dialog.HTreeView, &item);
    CPropSheetPage* page = (CPropSheetPage*)item.lParam;
    for (int i = 0; i < Count; i++)
        if (At(i) == page)
            return i;
    return -1;
}

HWND CTreePropDialog::GetTreeViewHandle() const
{
    return Dialog.HTreeView;
}

HTREEITEM CTreePropDialog::GetPageTreeItem(const CPropSheetPage* page) const
{
    return (page != NULL) ? page->HTreeItem : NULL;
}

void CTreePropDialog::SetPageTreeItem(CPropSheetPage* page, HTREEITEM item)
{
    if (page != NULL)
        page->HTreeItem = item;
}

const TCHAR* CTreePropDialog::GetPageTitle(const CPropSheetPage* page) const
{
    return (page != NULL) ? page->Title : NULL;
}

int CTreePropDialog::Add(CPropSheetPage* page, CPropSheetPage* parent, BOOL* expanded)
{
    int ret = CPropertyDialog::Add(page);
    if (IsGood())
    {
        page->ParentPage = parent;
        if (expanded != NULL)
            page->Expanded = expanded;
    }
    return ret;
}
