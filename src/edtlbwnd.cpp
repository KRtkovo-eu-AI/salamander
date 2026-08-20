// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "edtlbwnd.h"
#include "gui.h"

#if USE_DARKMODELIB
#include "third_party/darkmodelib/include/Darkmodelib.h"
#endif

//****************************************************************************
//
// CEditLBEdit
//

CEditLBEdit::CEditLBEdit(CEditListBox* editLB)
    : CWindow(ooAllocated)
{
    EditLB = editLB;
}

LRESULT
CEditLBEdit::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CEditLBEdit::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_GETDLGCODE:
    {
        return DLGC_WANTALLKEYS;
    }

    case WM_SYSKEYDOWN:
    {
        switch (wParam)
        {
        case VK_UP:
        case VK_DOWN:
        {
            if ((GetKeyState(VK_MENU) & 0x8000) != 0)
            {
                EditLB->OnPressButton();
                return 0;
            }
            break;
        }
        }
        break;
    }

    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_RETURN:
        {
            PostMessage(EditLB->HWindow, WM_COMMAND, MAKELPARAM(0, EN_KILLFOCUS), 0);
            EditLB->OnSaveEdit();
            EditLB->OnEndEdit();
            return 0;
        }

        case VK_ESCAPE:
        case VK_F2:
        {
            EditLB->OnEndEdit(); // discard changes
            return 0;
        }

        case VK_F4:
        {
            if (EditLB->Flags & ELB_RIGHTARROW)
            {
                EditLB->OnPressButton();
                return 0;
            }
            break;
        }

        case VK_TAB:
        {
            PostMessage(EditLB->HWindow, WM_KEYDOWN, wParam, lParam);
            EditLB->OnSaveEdit();
            EditLB->OnEndEdit();
            return 0;
        }
        }
        break;
    }

    case WM_KILLFOCUS:
    {
        EditLB->OnSaveEdit();
        break;
    }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}

//****************************************************************************
//
// CEditListBox
//

CEditListBox::CEditListBox(HWND hDlg, int ctrlID, DWORD flags, CObjectOrigin origin)
    : CWindow(hDlg, ctrlID, origin), FilterItems(20, 20)
{
    HDlg = hDlg;
    Header = NULL;
    HSearchEdit = NULL;
    SearchVisible = FALSE;
    FilterVisible = FALSE;
    SearchText[0] = 0;
    Flags = flags;
    DeleteAllItems();
    EditLine = NULL;
    DragAnchorIndex = -1;

    HNormalFont = NULL;
    HBoldFont = NULL;

    ButtonPressed = FALSE;
    ButtonDrag = FALSE;

    WaitForDrag = FALSE;
    Dragging = FALSE;
    HMarkWindow = NULL;

    UpdateFontMetrics((HFONT)SendMessage(HWindow, WM_GETFONT, 0, 0));

    // MakeDragList subclasses the list box and we stop receiving basic messages
    // such as WM_LBUTTONDOWN, WM_MOUSEMOVE, ect. That makes this function unusable,
    // so we implement drag&drop support ourselves.
    //  MakeDragList(HWindow);
    //  DragNotify = RegisterWindowMessage(DRAGLISTMSGSTRING);
}

void CEditListBox::UpdateFontMetrics(HFONT hFont)
{
    if (hFont == NULL)
        return;

    LOGFONT lf;
    if (GetObject(hFont, sizeof(lf), &lf) == 0)
        return;

    HFONT hNormalFont = HANDLES(CreateFontIndirect(&lf));
    lf.lfWeight = FW_BOLD;
    HFONT hBoldFont = HANDLES(CreateFontIndirect(&lf));
    if (hNormalFont == NULL || hBoldFont == NULL)
    {
        if (hNormalFont != NULL)
            HANDLES(DeleteObject(hNormalFont));
        if (hBoldFont != NULL)
            HANDLES(DeleteObject(hBoldFont));
        return;
    }

    if (HNormalFont != NULL)
        HANDLES(DeleteObject(HNormalFont));
    if (HBoldFont != NULL)
        HANDLES(DeleteObject(HBoldFont));
    HNormalFont = hNormalFont;
    HBoldFont = hBoldFont;

    TEXTMETRIC tm;
    HDC hdc = HANDLES(GetDC(HWindow));
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hOldFont);
    HANDLES(ReleaseDC(HWindow, hdc));
    int itemHeight = max(tm.tmHeight + 4, IconSizes[ICONSIZE_16]);
    SendMessage(HWindow, LB_SETITEMHEIGHT, 0, MAKELPARAM(itemHeight, 0));

    if (HSearchEdit != NULL)
        SendMessage(HSearchEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    if (EditLine != NULL)
        SendMessage(EditLine->HWindow, WM_SETFONT, (WPARAM)hFont, TRUE);
}

CEditListBox::~CEditListBox()
{
    if (HNormalFont != NULL)
        HANDLES(DeleteObject(HNormalFont));
    if (HBoldFont != NULL)
        HANDLES(DeleteObject(HBoldFont));
}

int CEditListBox::AddItem(INT_PTR itemID)
{
    // If you create the list box with an owner-drawn style but without the
    // LBS_HASSTRINGS style, the value of the lpsz parameter is stored as item
    // data instead of the string it would otherwise point to. You can send the
    // LB_GETITEMDATA and LB_SETITEMDATA messages to retrieve or modify the item data.

    // Under Windows XP with Common Controls 6 connected (via manifest.xml) MS has a bug:
    // for an owner-draw list box without LBS_HASSTRINGS flag, cannot receive lParam == 0
    // in LB_ADDSTRING and LB_INSERTSTRING. The message returns LB_ERR (-1) and the item is
    // not inserted. We work around it by passing a dummy lParam == 1 and then
    // assigning the real value calling LB_SETITEMDATA for the newly added item
    LRESULT res = SendMessage(HWindow, LB_INSERTSTRING, ItemsCount, 1); // 1 is a dummy value, we are working around a WinXP bug
    if (res != LB_ERR)
    {
        SendMessage(HWindow, LB_SETITEMDATA, res, ((Flags & ELB_ITEMINDEXES) ? ItemsCount : itemID));
        ItemsCount++;
    }
    return (int)res;
}

int CEditListBox::InsertItem(INT_PTR itemID, int index)
{
    if (Flags & ELB_ITEMINDEXES)
    {
        TRACE_E("CEditListBox::InsertItem is not ready for ELB_ITEMINDEXES Flag");
        return LB_ERR;
    }
    LRESULT res = SendMessage(HWindow, LB_INSERTSTRING, index, 1); // 1 is a dummy value, we are working around a WinXP bug
    if (res != LB_ERR)
    {
        SendMessage(HWindow, LB_SETITEMDATA, res, itemID);
        ItemsCount++;
    }
    return (int)res;
}

BOOL CEditListBox::SetItemData(INT_PTR itemID)
{
    int index = (int)SendMessage(HWindow, LB_ADDSTRING, 0, -1);
    if (Flags & ELB_ITEMINDEXES)
        itemID = ItemsCount;
    SendMessage(HWindow, LB_SETITEMDATA, ItemsCount, itemID);
    ItemsCount++;
    return TRUE;
}

BOOL CEditListBox::DeleteItem(int index)
{
    LRESULT ret = SendMessage(HWindow, LB_DELETESTRING, index, 0);
    if (ret != LB_ERR)
    {
        ItemsCount--;
        if (Flags & ELB_ITEMINDEXES)
        {
            int i;
            for (i = 0; i < ItemsCount; i++)
            {
                INT_PTR tmp = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, i, 0);
                if (tmp > index)
                    SendMessage(HWindow, LB_SETITEMDATA, i, tmp - 1);
            }
        }
    }
    return ret != LB_ERR;
}

void CEditListBox::DeleteAllItems()
{
    SendMessage(HWindow, LB_RESETCONTENT, 0, 0);
    int ret = (int)SendMessage(HWindow, LB_ADDSTRING, 0, -1);
    ItemsCount = 0;
}

BOOL CEditListBox::SetCurSel(int index)
{
    if (FilterVisible && index >= 0 && index < FilterItems.Count)
    {
        INT_PTR wanted = FilterItems[index];
        index = -1;
        for (int i = 0; i < ItemsCount; i++)
            if ((INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, i, 0) == wanted)
            {
                index = i;
                break;
            }
    }
    LRESULT ret = (SendMessage(HWindow, LB_SETCURSEL, index, 0) != LB_ERR || index == -1);
    CommandParent(LBN_SELCHANGE);
    return (BOOL)ret;
}

BOOL CEditListBox::GetCurSel(int& index)
{
    LRESULT res = SendMessage(HWindow, LB_GETCURSEL, 0, 0);
    if (res != LB_ERR)
        index = GetLogicalIndex((int)res);
    return res != LB_ERR;
}

BOOL CEditListBox::GetItemID(int index, INT_PTR& itemID)
{
    if (FilterVisible && index >= 0 && index < FilterItems.Count)
    {
        itemID = FilterItems[index];
        return TRUE;
    }
    itemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
    return TRUE;
}

BOOL CEditListBox::SetItemID(int index, INT_PTR itemID)
{
    SendMessage(HWindow, LB_SETITEMDATA, index, itemID);
    return TRUE;
}

void CEditListBox::Enable(BOOL enable)
{
    if (!enable)
        OnEndEdit();
    ::EnableWindow(HWindow, enable);
    if (Header != NULL)
    {
        if (!enable)
            Header->EnableToolbar(0);
        ::EnableWindow(Header->HWindow, enable);
        if (enable)
            OnSelChanged();
        RedrawWindow(Header->HWindow, NULL, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    if (HSearchEdit != NULL)
        ::EnableWindow(HSearchEdit, enable);
}

BOOL CEditListBox::GetCurSelItemID(INT_PTR& itemID)
{
    LRESULT res = SendMessage(HWindow, LB_GETCURSEL, 0, 0);
    if (res != LB_ERR)
    {
        itemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, res, 0);
        return TRUE;
    }
    return FALSE;
}

BOOL CEditListBox::MakeHeader(int ctrlID)
{
    Header = new CToolbarHeader(HDlg, ctrlID, HWindow,
                                TLBHDRMASK_MODIFY | TLBHDRMASK_NEW | TLBHDRMASK_DELETE |
                                    TLBHDRMASK_SEARCH | TLBHDRMASK_FILTER | TLBHDRMASK_TOP | TLBHDRMASK_UP | TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM);
    if (Header == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }
    Header->SetNotifyWindow(HWindow);
    HSearchEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                                 WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL,
                                 0, 0, 0, 0, Header->HWindow,
                                 (HMENU)0, HInstance, NULL);
    if (HSearchEdit != NULL)
    {
        SendMessage(HSearchEdit, WM_SETFONT, SendMessage(HWindow, WM_GETFONT, 0, 0), TRUE);
        ShowWindow(HSearchEdit, SW_HIDE);
    }
    LayoutSearchEdit();
    return TRUE;
}

int CEditListBox::NotifyParent(void* nmhdr, UINT code, BOOL send)
{
    NMHDR* hdr = (NMHDR*)nmhdr;
    hdr->hwndFrom = HWindow;
    hdr->idFrom = (UINT_PTR)GetMenu(HWindow);
    hdr->code = code;

    HWND hParent = GetParent(HWindow);

    if (send)
    {
        SendMessage(hParent, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
        return (int)GetWindowLongPtr(hParent, DWLP_MSGRESULT);
    }

    SendMessage(GetParent(HWindow), WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
    return TRUE;
}

void CEditListBox::CommandParent(UINT code)
{
    SendMessage(GetParent(HWindow), WM_COMMAND,
                MAKELPARAM((WORD)(UINT_PTR)GetMenu(HWindow), code), (LPARAM)HWindow);
}

DWORD CEditListBox::GetEnabler()
{
    if (!IsWindowEnabled(HWindow))
        return 0;
    DWORD enabler = TLBHDRMASK_MODIFY | TLBHDRMASK_NEW | TLBHDRMASK_DELETE |
                   TLBHDRMASK_SEARCH | TLBHDRMASK_FILTER | TLBHDRMASK_TOP | TLBHDRMASK_UP | TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM;
    if (Flags & ELB_ENABLECOMMANDS)
    {
        int index = (int)SendMessage(HWindow, LB_GETCURSEL, 0, 0);
        DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
        DispInfo.Index = GetLogicalIndex(index);
        NotifyParent(&DispInfo, EDTLBN_ENABLECOMMANDS);
        enabler = DispInfo.Enable;
    }
    if (enabler & TLBHDRMASK_SEARCH)
        enabler |= TLBHDRMASK_FILTER;
    if (FilterVisible)
        enabler &= TLBHDRMASK_SEARCH | TLBHDRMASK_FILTER;
    return enabler;
}

void CEditListBox::OnSelChanged()
{
    BOOL disableAll = EditLine != NULL;
    int index = (int)SendMessage(HWindow, LB_GETCURSEL, 0, 0);

    DWORD enabler = GetEnabler();

    DWORD mask = 0;
    if (!disableAll)
        mask |= (enabler & (TLBHDRMASK_SEARCH | TLBHDRMASK_FILTER));
    if (!disableAll && !FilterVisible)
        mask |= (enabler & (TLBHDRMASK_MODIFY | TLBHDRMASK_NEW));
    if (!disableAll && !FilterVisible && ItemsCount > 0 && index != ItemsCount)
        mask |= (enabler & TLBHDRMASK_DELETE);
    if (!disableAll && !FilterVisible && index > 0 && index < ItemsCount)
        mask |= (enabler & (TLBHDRMASK_TOP | TLBHDRMASK_UP));
    if (!disableAll && !FilterVisible && index >= 0 && index < ItemsCount - 1)
        mask |= (enabler & (TLBHDRMASK_DOWN | TLBHDRMASK_BOTTOM));

    Header->EnableToolbar(mask);
    Header->CheckToolbar((SearchVisible ? TLBHDRMASK_SEARCH : 0) |
                         (FilterVisible ? TLBHDRMASK_FILTER : 0));
}



void CEditListBox::LayoutSearchEdit()
{
    if (Header == NULL || HSearchEdit == NULL)
        return;

    RECT headerRect;
    GetClientRect(Header->HWindow, &headerRect);

    const int buttonsWidth = 9 * 22 + 4; // Search, Filter, Modify, New, Delete, Top, Up, Down, Bottom.
    int width = headerRect.right - headerRect.left - buttonsWidth - 4;
    if (width > 180)
        width = 180;
    if (width < 40)
        width = 40;
    int left = headerRect.right - headerRect.left - buttonsWidth - width;
    if (left < 4)
        left = 4;
    SetWindowPos(HSearchEdit, HWND_TOP, left, 2,
                 width, headerRect.bottom - headerRect.top - 4,
                 (SearchVisible || FilterVisible) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
}

BOOL CEditListBox::SearchMatches(const char* text)
{
    if (!SearchVisible || SearchText[0] == 0)
        return FALSE;
    if (text == NULL)
        return FALSE;

    char needle[100];
    lstrcpyn(needle, SearchText, _countof(needle));
    CharLowerBuffA(needle, lstrlen(needle));

    char haystack[MAX_PATH];
    lstrcpyn(haystack, text, _countof(haystack));
    CharLowerBuffA(haystack, lstrlen(haystack));
    return strstr(haystack, needle) != NULL;
}

BOOL CEditListBox::TextMatches(const char* text)
{
    if (SearchText[0] == 0)
        return TRUE;
    if (text == NULL)
        return FALSE;
    char needle[100];
    lstrcpyn(needle, SearchText, _countof(needle));
    CharLowerBuffA(needle, lstrlen(needle));
    char haystack[MAX_PATH];
    lstrcpyn(haystack, text, _countof(haystack));
    CharLowerBuffA(haystack, lstrlen(haystack));
    return strstr(haystack, needle) != NULL;
}

int CEditListBox::GetLogicalIndex(int visibleIndex)
{
    if (!FilterVisible || visibleIndex < 0 || visibleIndex >= ItemsCount)
        return visibleIndex;
    INT_PTR itemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, visibleIndex, 0);
    for (int i = 0; i < FilterItems.Count; i++)
        if (FilterItems[i] == itemID)
            return i;
    return visibleIndex;
}

void CEditListBox::CaptureFilterItems()
{
    FilterItems.DestroyMembers();
    for (int i = 0; i < ItemsCount; i++)
        FilterItems.Add((INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, i, 0));
    if (!FilterItems.IsGood())
        FilterItems.ResetState();
}

void CEditListBox::RebuildFilteredItems(BOOL filter)
{
    INT_PTR selectedID = -1;
    int selected = (int)SendMessage(HWindow, LB_GETCURSEL, 0, 0);
    if (selected >= 0 && selected < ItemsCount)
        selectedID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, selected, 0);

    SendMessage(HWindow, LB_RESETCONTENT, 0, 0);
    SendMessage(HWindow, LB_ADDSTRING, 0, -1); // trailing editable placeholder
    ItemsCount = 0;
    int selectedVisible = -1;
    for (int i = 0; i < FilterItems.Count; i++)
    {
        INT_PTR itemID = FilterItems[i];
        BOOL include = TRUE;
        if (filter && SearchText[0] != 0)
        {
            DispInfo.ToDo = edtlbGetData;
            DispInfo.Buffer = Buffer;
            DispInfo.BufferLen = MAX_PATH;
            DispInfo.Index = i;
            DispInfo.ItemID = itemID;
            DispInfo.HIcon = NULL;
            DispInfo.Bold = FALSE;
            Buffer[0] = 0;
            NotifyParent(&DispInfo, EDTLBN_GETDISPINFO);
            include = TextMatches(Buffer);
        }
        if (include)
        {
            LRESULT inserted = SendMessage(HWindow, LB_INSERTSTRING, ItemsCount, 1);
            if (inserted != LB_ERR)
            {
                SendMessage(HWindow, LB_SETITEMDATA, inserted, itemID);
                if (itemID == selectedID)
                    selectedVisible = ItemsCount;
                ItemsCount++;
            }
        }
    }
    if (selectedVisible < 0 && ItemsCount > 0)
        selectedVisible = 0;
    SendMessage(HWindow, LB_SETCURSEL, selectedVisible, 0);
    CommandParent(LBN_SELCHANGE);
    InvalidateRect(HWindow, NULL, TRUE);
}

void CEditListBox::ApplyFilter()
{
    if (HSearchEdit != NULL)
        GetWindowText(HSearchEdit, SearchText, _countof(SearchText));
    RebuildFilteredItems(TRUE);
    OnSelChanged();
}

void CEditListBox::ApplySearch()
{
    if (HSearchEdit != NULL)
        GetWindowText(HSearchEdit, SearchText, _countof(SearchText));

    if (FilterVisible)
    {
        ApplyFilter();
        return;
    }
    if (SearchVisible && SearchText[0] != 0)
    {
        for (int i = 0; i < ItemsCount; i++)
        {
            DispInfo.ToDo = edtlbGetData;
            DispInfo.Buffer = Buffer;
            DispInfo.BufferLen = MAX_PATH;
            DispInfo.Index = i;
            DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, i, 0);
            DispInfo.HIcon = NULL;
            DispInfo.Bold = FALSE;
            Buffer[0] = 0;
            NotifyParent(&DispInfo, EDTLBN_GETDISPINFO);
            if (SearchMatches(Buffer))
            {
                SendMessage(HWindow, LB_SETCURSEL, i, 0);
                CommandParent(LBN_SELCHANGE);
                break;
            }
        }
    }
    InvalidateRect(HWindow, NULL, FALSE);
    OnSelChanged();
}

void CEditListBox::ToggleSearch()
{
    if (FilterVisible)
    {
        FilterVisible = FALSE;
        RebuildFilteredItems(FALSE);
    }
    SearchVisible = !SearchVisible;
    if (!SearchVisible)
        SearchText[0] = 0;

    if (HSearchEdit != NULL)
    {
        SetWindowText(HSearchEdit, SearchText);
        ShowWindow(HSearchEdit, SearchVisible ? SW_SHOW : SW_HIDE);
        if (SearchVisible)
            SetFocus(HSearchEdit);
    }
    LayoutSearchEdit();
    ApplySearch();
}

void CEditListBox::ToggleFilter()
{
    if (SearchVisible)
        SearchVisible = FALSE;
    if (!FilterVisible)
    {
        CaptureFilterItems();
        FilterVisible = TRUE;
    }
    else
    {
        FilterVisible = FALSE;
        SearchText[0] = 0;
        RebuildFilteredItems(FALSE);
    }
    if (HSearchEdit != NULL)
    {
        SetWindowText(HSearchEdit, SearchText);
        ShowWindow(HSearchEdit, (SearchVisible || FilterVisible) ? SW_SHOW : SW_HIDE);
        if (FilterVisible)
            SetFocus(HSearchEdit);
    }
    LayoutSearchEdit();
    if (FilterVisible)
        ApplyFilter();
    else
        OnSelChanged();
}

void CEditListBox::OnMoveTop()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnMoveTop()");
    int index;
    if (!GetCurSel(index))
        return;
    if (index == ItemsCount || index < 1)
        return;
    if ((GetEnabler() & TLBHDRMASK_TOP) == 0)
        return;

    MoveItem(0);
}

void CEditListBox::OnMoveUp()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnMoveUp()");
    int index;
    if (!GetCurSel(index))
        return;
    if (index == ItemsCount || index < 1)
        return;
    if ((GetEnabler() & TLBHDRMASK_UP) == 0)
        return;

    MoveItem(index - 1);
}

void CEditListBox::OnMoveDown()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnMoveDown()");
    int index;
    if (!GetCurSel(index))
        return;
    if (index >= ItemsCount - 1)
        return;
    if ((GetEnabler() & TLBHDRMASK_DOWN) == 0)
        return;

    MoveItem(index + 2);
}

void CEditListBox::OnMoveBottom()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnMoveBottom()");
    int index;
    if (!GetCurSel(index))
        return;
    if (index >= ItemsCount - 1)
        return;
    if ((GetEnabler() & TLBHDRMASK_BOTTOM) == 0)
        return;

    MoveItem(ItemsCount);
}

void CEditListBox::MoveItem(int newIndex)
{
    CALL_STACK_MESSAGE2("CEditListBox::MoveItem(%d)", newIndex);
    if (FilterVisible)
        return;
    int index;
    if (!GetCurSel(index))
        return;
    if (index >= ItemsCount)
        return;
    if (newIndex < 0 || newIndex > ItemsCount)
        return;
    if (newIndex == index || newIndex == index + 1)
        return;
    if (newIndex > index)
        newIndex--; // shift downwards

    DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
    DispInfo.Index = index;
    DispInfo.NewIndex = newIndex;
    if (!NotifyParent(&DispInfo, EDTLBN_MOVEITEM2))
    {
        SendMessage(HWindow, LB_SETCURSEL, newIndex, 0);
        InvalidateRect(HWindow, NULL, FALSE);
        OnSelChanged();
    }
}

void CEditListBox::OnNew()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnNew()");
    if (FilterVisible)
        return;
    SendMessage(HWindow, LB_SETCURSEL, ItemsCount, 0);
    CommandParent(LBN_SELCHANGE);
    OnBeginEdit();
}

void CEditListBox::OnDelete()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnDelete()");
    int index;
    if (!GetCurSel(index))
        return;
    if (index >= ItemsCount)
        return;
    if ((GetEnabler() & TLBHDRMASK_DELETE) == 0)
        return;

    DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
    DispInfo.Index = index;
    if (!NotifyParent(&DispInfo, EDTLBN_DELETEITEM))
    {
        SendMessage(HWindow, LB_DELETESTRING, index, 0);
        ItemsCount--;
        if (Flags & ELB_ITEMINDEXES)
        {
            int i;
            for (i = 0; i < ItemsCount; i++)
            {
                INT_PTR tmp = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, i, 0);
                if (tmp > index)
                    SendMessage(HWindow, LB_SETITEMDATA, i, tmp - 1);
            }
        }
        SendMessage(HWindow, LB_SETCURSEL, index, 0);
        CommandParent(LBN_SELCHANGE);
        OnSelChanged();
    }
}

void CEditListBox::OnBeginEdit(int start, int end)
{
    CALL_STACK_MESSAGE3("CEditListBox::OnBeginEdit(%d, %d)", start, end);
    if (FilterVisible)
        return;
    SaveDisabled = FALSE;
    if (EditLine != NULL)
        return;
    int index;
    if (!GetCurSel(index))
        return;
    if ((GetEnabler() & TLBHDRMASK_MODIFY) == 0)
        return;

    RECT r;
    SendMessage(HWindow, LB_GETITEMRECT, index, (LPARAM)&r);

    EditLine = new CEditLBEdit(this);
    if (EditLine == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return;
    }

    int buttonWidth = 0;
    if (Flags & ELB_RIGHTARROW)
    {
        buttonWidth = r.bottom - r.top;
        ButtonRect = r;
        ButtonRect.left = ButtonRect.right - buttonWidth;
        ButtonPressed = FALSE;
        ButtonDrag = FALSE;
    }

    int iconWidth = 0;
    if (Flags & ELB_SHOWICON)
        iconWidth = IconSizes[ICONSIZE_16] + 2;

    EditLine->Create("edit",
                     "",
                     WS_BORDER | WS_CHILDWINDOW | ES_AUTOHSCROLL | ES_LEFT,
                     r.left + iconWidth,
                     r.top,
                     r.right - r.left - buttonWidth - iconWidth,
                     r.bottom - r.top,
                     HWindow,
                     (HMENU)0,
                     HInstance,
                     EditLine);

    SendMessage(EditLine->HWindow, WM_SETFONT, SendMessage(HWindow, WM_GETFONT, 0, 0), TRUE);
    SetFocus(EditLine->HWindow);
    if (index != ItemsCount)
    {
        DispInfo.ToDo = edtlbGetData;
        DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
        DispInfo.Buffer = Buffer;
        DispInfo.Index = index;
        DispInfo.BufferLen = MAX_PATH - 1;
        DispInfo.Buffer[0] = 0;
        NotifyParent(&DispInfo, EDTLBN_GETDISPINFO);
        DispInfo.Buffer[MAX_PATH - 1] = 0;
        SetWindowText(EditLine->HWindow, Buffer);
    }
    SendMessage(EditLine->HWindow, EM_SETLIMITTEXT, MAX_PATH, 0);
    SendMessage(EditLine->HWindow, EM_SETSEL, start, end);
    ShowWindow(EditLine->HWindow, SW_SHOW);
    if (Flags & ELB_RIGHTARROW)
        PaintButton();
    OnSelChanged();
}

void CEditListBox::OnEndEdit()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnEndEdit()");
    if (EditLine != NULL)
    {
        SaveDisabled = TRUE;
        DestroyWindow(EditLine->HWindow);
        EditLine = NULL;
    }
    OnSelChanged();
}

BOOL CEditListBox::OnSaveEdit()
{
    CALL_STACK_MESSAGE1("CEditListBox::OnSaveEdit()");
    int index;
    if (SaveDisabled || !GetCurSel(index))
        return FALSE;
    BOOL oldSD = SaveDisabled;
    SaveDisabled = TRUE;
    if (EditLine != NULL)
    {
        char buff[2];
        GetWindowText(EditLine->HWindow, buff, 2);
        if (buff[0] != 0)
        {
            DispInfo.ToDo = edtlbSetData;
            DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
            DispInfo.Index = index;
            DispInfo.Buffer = Buffer;
            DispInfo.BufferLen = MAX_PATH - 1;
            GetWindowText(EditLine->HWindow, DispInfo.Buffer, MAX_PATH);
            BOOL ret = !NotifyParent(&DispInfo, EDTLBN_GETDISPINFO);
            SaveDisabled = oldSD;
            return ret;
        }
    }
    SaveDisabled = oldSD;
    return FALSE;
}

void CEditListBox::PaintButton()
{
    HDC hDC = HANDLES(GetDC(HWindow));
    DWORD flags = DFCS_SCROLLRIGHT;
    if (ButtonPressed)
        flags |= DFCS_PUSHED;
    DrawFrameControl(hDC, &ButtonRect, DFC_SCROLL, flags);
    HANDLES(ReleaseDC(HWindow, hDC));
}

void CEditListBox::RedrawFocusedItem()
{
    CALL_STACK_MESSAGE1("CEditListBox::RedrawFocusedItem()");
    int index;
    if (!GetCurSel(index) || index == LB_ERR)
        return;
    RECT r;
    SendMessage(HWindow, LB_GETITEMRECT, index, (LPARAM)&r);
    InvalidateRect(HWindow, &r, FALSE);
}

void CEditListBox::OnDrawItem(LPARAM lParam)
{
    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
    switch (lpdis->itemAction)
    {
    case ODA_FOCUS:
    case ODA_SELECT:
    case ODA_DRAWENTIRE:
    {
        if (EditLine != NULL && lpdis->itemState & ODS_SELECTED && (Flags & ELB_RIGHTARROW))
        {
            PaintButton();
        }
        else
        {
            COLORREF bkColor;
            const bool useDark = DarkModeShouldUseDarkColors();
            const DarkModeColors& darkColors = DarkModeGetColors();
#if USE_DARKMODELIB
            const COLORREF darkBg = dmlib::getCtrlBackgroundColor();
#else
            const COLORREF darkBg = darkColors.background;
#endif
            const BOOL enabled = IsWindowEnabled(HWindow);
            if (enabled && lpdis->itemState & ODS_SELECTED)
            {
                if (lpdis->itemState & ODS_FOCUS)
                    bkColor = useDark ? darkBg : COLOR_HIGHLIGHT;
                else
                    bkColor = useDark ? darkBg : COLOR_3DFACE;
            }
            else
                bkColor = useDark ? darkBg : COLOR_WINDOW;

            RECT itemRect = lpdis->rcItem;
            if (Flags & ELB_SHOWICON)
                itemRect.left += IconSizes[ICONSIZE_16];

            if (useDark)
            {
                HBRUSH hBrush = HANDLES(CreateSolidBrush(bkColor));
                FillRect(lpdis->hDC, &itemRect, hBrush);
                HANDLES(DeleteObject(hBrush));
            }
            else
                FillRect(lpdis->hDC, &itemRect, (HBRUSH)(UINT_PTR)(bkColor + 1));
            INT_PTR itemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, lpdis->itemID, 0);
            if (itemID == -1)
            {
                RECT r = itemRect;
                r.left += 4;
                r.top += 2;
                r.bottom -= 2;
                r.right = r.right / 3;
                DrawFocusRect(lpdis->hDC, &r);
            }
            else
            {
                DRAWTEXTPARAMS dtp;
                dtp.cbSize = sizeof(dtp);
                dtp.iLeftMargin = dtp.iRightMargin = 4;
                int oldBkMode = SetBkMode(lpdis->hDC, TRANSPARENT);
                COLORREF textColor;
                if (!enabled)
                    textColor = useDark ? DarkModeGetDisabledTextColor()
                                        : GetSysColor(COLOR_GRAYTEXT);
                else if (lpdis->itemState & ODS_SELECTED && lpdis->itemState & ODS_FOCUS)
                    textColor = useDark ? darkColors.readableText : GetSysColor(COLOR_HIGHLIGHTTEXT);
                else
                    textColor = useDark ? darkColors.readableText : GetSysColor(COLOR_WINDOWTEXT);
                int oldColor = SetTextColor(lpdis->hDC, textColor);
                DispInfo.ToDo = edtlbGetData;
                DispInfo.ItemID = itemID;
                DispInfo.Index = GetLogicalIndex(lpdis->itemID);
                DispInfo.Buffer = Buffer;
                DispInfo.BufferLen = MAX_PATH - 1;
                DispInfo.Buffer[0] = 0;
                DispInfo.Bold = FALSE;
                NotifyParent(&DispInfo, EDTLBN_GETDISPINFO);
                DispInfo.Buffer[MAX_PATH - 1] = 0;
                if (SearchMatches(Buffer))
                    DispInfo.Bold = TRUE;

                if (Flags & ELB_SHOWICON)
                {
                    if (DispInfo.HIcon != NULL)
                    {
                        // If a brush is passed to DrawIconEx as (HBRUSH)(COLOR_WINDOW + 1),
                        // under NT 4.0 US with 256 colors a black spot appears in the background;
                        // this patch fixes the problem.
                        HBRUSH hBrush = HANDLES(CreateSolidBrush(useDark ? darkBg : GetSysColor(COLOR_WINDOW)));
                        int iconSize = IconSizes[ICONSIZE_16];
                        DrawIconEx(lpdis->hDC, lpdis->rcItem.left + 1, lpdis->rcItem.top + 1,
                                   DispInfo.HIcon, iconSize, iconSize, 0, hBrush /*(HBRUSH)(COLOR_WINDOW + 1)*/, DI_NORMAL);
                        HANDLES(DeleteObject(hBrush));
                    }
                    else
                    {
                        // must clear the background
                        RECT r = lpdis->rcItem;
                        r.right = IconSizes[ICONSIZE_16] + 2;
                        if (useDark)
                        {
                            HBRUSH hBrush = HANDLES(CreateSolidBrush(darkBg));
                            FillRect(lpdis->hDC, &r, hBrush);
                            HANDLES(DeleteObject(hBrush));
                        }
                        else
                            FillRect(lpdis->hDC, &r, (HBRUSH)(COLOR_WINDOW + 1));
                    }
                }

                HFONT hOldFont;
                if (DispInfo.Bold)
                    hOldFont = (HFONT)SelectObject(lpdis->hDC, HBoldFont);
                else
                    hOldFont = (HFONT)SelectObject(lpdis->hDC, HNormalFont);
                DrawTextEx(lpdis->hDC, Buffer,
                           -1, &itemRect,
                           DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS,
                           &dtp);
                SelectObject(lpdis->hDC, hOldFont);
                SetTextColor(lpdis->hDC, oldColor);
                SetBkMode(lpdis->hDC, oldBkMode);
            }
            if (lpdis->itemState & ODS_FOCUS)
                DrawFocusRect(lpdis->hDC, &itemRect);
        }
        break;
    }
    }
}

void CEditListBox::OnPressButton()
{
    int index;
    if (!GetCurSel(index))
        return;
    DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
    DispInfo.Index = index;
    DispInfo.HEdit = EditLine->HWindow;
    DispInfo.Point.x = ButtonRect.right;
    DispInfo.Point.y = ButtonRect.top;
    ClientToScreen(HWindow, &DispInfo.Point);
    NotifyParent(&DispInfo, EDTLBN_CONTEXTMENU, FALSE);
}

void CEditListBox::EnableDrag(HWND hMarkWindow)
{
    HMarkWindow = hMarkWindow;
}

/*
BOOL
CEditListBox::IsDragNotifyMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, HWND HDropMarkWindow)
{
  if (DragNotify != 0 && DragNotify == uMsg)
  {
    DRAGLISTINFO *dli = (DRAGLISTINFO*)lParam;
    switch (dli->uNotification)
    {
      case DL_BEGINDRAG:
      {
        BOOL enableDrag = TRUE;
        if ((GetEnabler() & (TLBHDRMASK_UP | TLBHDRMASK_DOWN)) == 0) enableDrag = FALSE;
        int index = LBItemFromPt(HWindow, dli->ptCursor, TRUE);
        if (index >= ItemsCount) enableDrag = FALSE; // we must not allow dragging of an empty item
        Dragging = enableDrag;
        SetWindowLongPtr(GetParent(HWindow), DWLP_MSGRESULT, enableDrag);
        break;
      }

      case DL_DRAGGING:
      {
        int index = LBItemFromPt(HWindow, dli->ptCursor, TRUE);
        DrawInsert(HDropMarkWindow, HWindow, index);

        SetWindowLongPtr(GetParent(HWindow), DWLP_MSGRESULT, 0); // prevent cursor change
        break;
      }

      case DL_DROPPED:
      {
        if (Dragging)
        {
          DrawInsert(HDropMarkWindow, HWindow, -1);
          int index = LBItemFromPt(HWindow, dli->ptCursor, TRUE);
          MoveItem(index);
          Dragging = FALSE;
        }
        break;
      }

      case DL_CANCELDRAG:
      {
        DrawInsert(HDropMarkWindow, HWindow, -1);
        break;
      }
    }
    return TRUE;
  }
  return FALSE;
}
*/

LONG GetMessagePosClient(HWND hwnd, LPPOINT ppt)
{
    LPARAM lParam;
    POINT pt;
    if (!ppt)
        ppt = &pt;

    lParam = GetMessagePos();
    ppt->x = GET_X_LPARAM(lParam);
    ppt->y = GET_Y_LPARAM(lParam);
    ScreenToClient(hwnd, ppt);

    return MAKELONG(ppt->x, ppt->y);
}

#define IDT_EDITLB_TIMERLEN 50

LRESULT
CEditListBox::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CEditListBox::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_SETFONT:
    {
        LRESULT result = CWindow::WindowProc(uMsg, wParam, lParam);
        UpdateFontMetrics((HFONT)wParam);
        return result;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == EN_KILLFOCUS)
            OnEndEdit();
        if ((HWND)lParam == HSearchEdit && HIWORD(wParam) == EN_CHANGE)
            ApplySearch();

        if (LOWORD(wParam) == (WORD)(UINT_PTR)GetMenu(Header->HWindow))
        {
            if (GetFocus() != HWindow)
                SetFocus(HWindow);
            switch (HIWORD(wParam))
            {
            case TLBHDR_MODIFY:
                OnBeginEdit();
                break;
            case TLBHDR_NEW:
                OnNew();
                break;
            case TLBHDR_DELETE:
                OnDelete();
                break;
            case TLBHDR_SEARCH:
                ToggleSearch();
                break;
            case TLBHDR_FILTER:
                ToggleFilter();
                break;
            case TLBHDR_TOP:
                OnMoveTop();
                break;
            case TLBHDR_UP:
                OnMoveUp();
                break;
            case TLBHDR_DOWN:
                OnMoveDown();
                break;
            case TLBHDR_BOTTOM:
                OnMoveBottom();
                break;
            }
        }
        return 0;
    }

    case WM_SIZE:
    case WM_WINDOWPOSCHANGED:
        LayoutSearchEdit();
        break;

    case WM_CHAR:
    {
        if (Dragging) // We don't want the listbox processing this if we are dragging.
            return 0;
        if (wParam >= 32)
        {
            // should we deliver the SPACE message as a click on the icon?
            if (wParam == 32 && (Flags & ELB_SPACEASICONCLICK))
            {
                if (FilterVisible)
                    return 0;
                int index = (int)SendMessage(HWindow, LB_GETCURSEL, 0, 0);
                if (index != LB_ERR)
                {
                    DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
                    DispInfo.Index = GetLogicalIndex(index);
                    NotifyParent(&DispInfo, EDTLBN_ICONCLICKED);
                }
            }
            else
            {
                OnBeginEdit();
                if (EditLine != NULL)
                    PostMessage(EditLine->HWindow, WM_CHAR, wParam, lParam);
            }
        }
        return 0;
    }

    case WM_VSCROLL:
    case WM_HSCROLL:
    {
        if (EditLine != NULL)
            return 0;
    }

    case WM_KEYUP:
    {
        if (Dragging) // We don't want the listbox processing this if we are dragging.
            return 0;
        break;
    }

    case WM_KEYDOWN:
    {
        if (Dragging)
        {
            if (wParam == VK_ESCAPE)
                SendMessage(HWindow, WM_RBUTTONDOWN, 0, 0); // cancel d&d
            return 0;
        }
        switch (wParam)
        {
        case VK_F2:
            OnBeginEdit();
            return 0;
        case VK_INSERT:
            OnNew();
            return 0;
        case VK_DELETE:
            OnDelete();
            return 0;
        }
        break;
    }

    case WM_SYSKEYDOWN:
    {
        if (Dragging)
        {
            if (wParam == VK_ESCAPE)
                SendMessage(HWindow, WM_RBUTTONDOWN, 0, 0); // cancel d&d
            return 0;
        }
        if (GetKeyState(VK_MENU) & 0x8000)
        {
            switch (wParam)
            {
            case VK_UP:
                OnMoveUp();
                return 0;
            case VK_DOWN:
                OnMoveDown();
                return 0;
            }
        }
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    {
        int index = (int)SendMessage(HWindow, LB_GETCURSEL, 0, 0);
        if (index != LB_ERR)
        {
            if (EditLine != NULL && (Flags & ELB_RIGHTARROW))
            {
                POINT p;
                p.x = LOWORD(lParam);
                p.y = HIWORD(lParam);
                if (PtInRect(&ButtonRect, p))
                {
                    ButtonPressed = TRUE;
                    ButtonDrag = TRUE;
                    SetCapture(HWindow);
                    PaintButton();
                    return 0;
                }
            }

            if (GetFocus() != HWindow)
            {
                // probably unnecessary under W2K+: without closing the edit window Salamander crashed:
                // it was enough to start editing an item and click on another
                if (EditLine != NULL)
                {
                    OnSaveEdit();
                    OnEndEdit();
                }
                SendMessage(GetParent(HWindow), WM_NEXTDLGCTL, (WPARAM)HWindow, TRUE);
            }

            int item = LOWORD(SendMessage(HWindow, LB_ITEMFROMPOINT, 0,
                                          MAKELPARAM(LOWORD(lParam), HIWORD(lParam))));

            // ignore clicks outside the item
            RECT r;
            SendMessage(HWindow, LB_GETITEMRECT, item, (LPARAM)&r);
            POINT pt;
            pt.x = LOWORD(lParam);
            pt.y = HIWORD(lParam);
            if (!PtInRect(&r, pt))
                return 0;

            SelChanged = item != index;
            if (item != index)
            {
                index = item;
                SendMessage(HWindow, LB_SETCURSEL, index, 0);
                CommandParent(LBN_SELCHANGE);
            }

            if ((Flags & ELB_SHOWICON) && (pt.x < IconSizes[ICONSIZE_16] + 2))
            {
                if (FilterVisible)
                    return 0;
                DispInfo.ItemID = (INT_PTR)SendMessage(HWindow, LB_GETITEMDATA, index, 0);
                DispInfo.Index = GetLogicalIndex(index);
                NotifyParent(&DispInfo, EDTLBN_ICONCLICKED);
            }
            else if (!FilterVisible && HMarkWindow != NULL && item < ItemsCount) // do not drag a filtered projection
            {
                WaitForDrag = TRUE;
                DragAnchor = pt;
                DragAnchorIndex = index;
                SetCapture(HWindow);
            }
            return 0;
        }
        break;
    }

    case WM_TIMER:
    {
        if (wParam != IDT_EDITLB)
            break;
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        lParam = GetMessagePosClient(HWindow, &pt);
    }
    // fall through
    case WM_MOUSEMOVE:
    {
        POINT p;
        p.x = LOWORD(lParam);
        p.y = HIWORD(lParam);
        if (ButtonDrag)
        {
            BOOL down = PtInRect(&ButtonRect, p);
            if (down != ButtonPressed)
            {
                ButtonPressed = down;
                PaintButton();
            }
            return 0;
        }
        if (!Dragging && WaitForDrag)
        {
            int dxClickRect = GetSystemMetrics(SM_CXDRAG);
            int dyClickRect = GetSystemMetrics(SM_CYDRAG);
            if (dxClickRect < 1)
                dxClickRect = 1;
            if (dyClickRect < 1)
                dyClickRect = 1;
            RECT r;
            SetRect(&r, DragAnchor.x - dxClickRect, DragAnchor.y - dyClickRect,
                    DragAnchor.x + dxClickRect, DragAnchor.y + dyClickRect);
            if (!PtInRect(&r, p))
            {
                SetTimer(HWindow, IDT_EDITLB, IDT_EDITLB_TIMERLEN, NULL);
                Dragging = TRUE;
                WaitForDrag = FALSE;
            }
        }
        if (Dragging)
        {
            POINT pt;
            DWORD pos = GetMessagePos();
            pt.x = GET_X_LPARAM(pos);
            pt.y = GET_Y_LPARAM(pos);
            int index = LBItemFromPt(HWindow, pt, TRUE);
            DrawInsert(HMarkWindow, HWindow, index);
            return 0;
        }
        break;
    }

    case WM_RBUTTONDOWN:
    case WM_CANCELMODE:
    case WM_LBUTTONUP:
    {
        if (ButtonDrag)
        {
            BOOL pressed = ButtonPressed;
            ButtonPressed = FALSE;
            ButtonDrag = FALSE;
            KillTimer(HWindow, IDT_EDITLB);
            ReleaseCapture();
            PaintButton();
            if (pressed && EditLine != NULL)
                OnPressButton();
            return 0;
        }

        POINT pt;
        DWORD pos = GetMessagePos();
        pt.x = GET_X_LPARAM(pos);
        pt.y = GET_Y_LPARAM(pos);
        int index = LBItemFromPt(HWindow, pt, TRUE);

        if (Dragging)
        {
            DrawInsert(HMarkWindow, HWindow, -1);

            if (uMsg == WM_LBUTTONUP)
            {
                MoveItem(index);
            }
            ReleaseCapture();
            Dragging = FALSE;
            int anchor = DragAnchorIndex;
            return 0;
        }
        else
        {
            ReleaseCapture();
            if ((Flags & ELB_SHOWICON) && (LOWORD(lParam) < IconSizes[ICONSIZE_16] + 2))
            {
            }
            else if (!SelChanged && index != -1)
                OnBeginEdit();
        }
        WaitForDrag = FALSE;
        DragAnchorIndex = -1;
        break;
    }

    case WM_MOUSEWHEEL:
    case WM_USER_MOUSEWHEEL:
    case WM_USER_MOUSEHWHEEL:
    {
        if (EditLine != NULL)
            return 0;
        break;
    }

    case WM_GETDLGCODE:
    {
        if (Dragging)
        {
            LRESULT ret = CWindow::WindowProc(uMsg, wParam, lParam);
            return ret | DLGC_WANTMESSAGE; // we want Escape
        }
        break;
    }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}
