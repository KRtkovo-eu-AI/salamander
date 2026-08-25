// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

HIMAGELIST HSymbolsImageList = NULL;
char DirText[100];

namespace
{
std::wstring PreviewTextToWide(const char* text)
{
    if (text == NULL || *text == 0)
        return std::wstring();

    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    if (len == 0)
    {
        codePage = CP_ACP;
        flags = 0;
        len = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    }
    if (len == 0)
        return std::wstring();

    std::wstring wide(len - 1, L'\0');
    MultiByteToWideChar(codePage, flags, text, -1, &wide[0], len);
    return wide;
}

bool GetEditLineUtf8(HWND edit, int lineIndex, char* buffer, int bufferSize)
{
    if (buffer == NULL || bufferSize <= 0)
        return false;
    buffer[0] = 0;

    if (edit == NULL)
        return false;

    if (!IsWindowUnicode(edit))
    {
        int charIndex = (int)SendMessage(edit, EM_LINEINDEX, lineIndex, 0);
        if (charIndex < 0)
            return false;
        int lineLen = (int)SendMessage(edit, EM_LINELENGTH, charIndex, 0);
        if (lineLen >= bufferSize)
            return false;
        *LPWORD(buffer) = (WORD)bufferSize;
        int copied = (int)SendMessage(edit, EM_GETLINE, lineIndex, (LPARAM)buffer);
        buffer[copied] = 0;
        return true;
    }

    int textLen = GetWindowTextLengthW(edit);
    if (textLen <= 0)
        return lineIndex == 0;
    std::wstring text(textLen + 1, L'\0');
    GetWindowTextW(edit, &text[0], textLen + 1);
    text.resize(textLen);

    size_t start = 0;
    for (int line = 0; line < lineIndex; line++)
    {
        start = text.find(L'\n', start);
        if (start == std::wstring::npos)
            return false;
        start++;
    }
    size_t end = text.find(L'\n', start);
    if (end == std::wstring::npos)
        end = text.length();
    if (end > start && text[end - 1] == L'\r')
        end--;

    std::wstring line = text.substr(start, end - start);
    int required = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.length(), NULL, 0, NULL, NULL);
    if (required >= bufferSize)
        return false;
    int written = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.length(), buffer, bufferSize - 1, NULL, NULL);
    if (written < 0)
        written = 0;
    buffer[written] = 0;
    return true;
}
}

CPreviewWindow::CPreviewWindow(CRenamerDialog* renamerDialog)
    : RenamerOptions(renamerDialog->RenamerOptions),
      Renamer(renamerDialog->Root, renamerDialog->RootLen),
      Root(renamerDialog->Root),
      RootLen(renamerDialog->RootLen),
      SourceFiles(renamerDialog->SourceFiles),
      SourceFilesValid(renamerDialog->SourceFilesValid)
{
    CALL_STACK_MESSAGE1("CPreviewWindow::CPreviewWindow()");
    RenamerDialog = renamerDialog;
    TransferError = FALSE;
    Dirty = FALSE;
    Renamer.SetOptions(&RenamerOptions);
    CachedItem = -1;
    Static = 0;
    State = -1;
}

CPreviewWindow::~CPreviewWindow()
{
    CALL_STACK_MESSAGE1("CPreviewWindow::~CPreviewWindow()");
}

BOOL CPreviewWindow::InitColumns()
{
    CALL_STACK_MESSAGE1("CPreviewWindow::InitColumns()");

    LV_COLUMN lvc;
    int header[] =
        {IDS_ORIGNAME_COLUMN, IDS_NEWNAME_COLUMN, IDS_SIZE_COLUMN, IDS_DATE_COLUMN,
         IDS_TIME_COLUMN, IDS_PATH_COLUMN, -1};

    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;
    int i;
    for (i = 0; header[i] != -1; i++) // create the columns
    {
        lvc.pszText = (LPSTR)LoadStr(header[i]);
        lvc.iSubItem = i;
        if (i == 2)
            lvc.fmt = LVCFMT_RIGHT;
        if (i == 5)
            lvc.fmt = LVCFMT_LEFT;
        if (ListView_InsertColumn(HWindow, i, &lvc) == -1)
            return FALSE;
    }

    RECT r;
    GetClientRect(HWindow, &r);
    DWORD cx = r.right - r.left + (1 ? -1 : 1);
    ListView_SetColumnWidth(HWindow, CI_TIME, ListView_GetStringWidth(HWindow, "00:00:00") + 20);
    ListView_SetColumnWidth(HWindow, CI_DATE, ListView_GetStringWidth(HWindow, "00.00.0000") + 20);
    ListView_SetColumnWidth(HWindow, CI_SIZE, ListView_GetStringWidth(HWindow, "000000") + 20);

    cx -= ListView_GetColumnWidth(HWindow, CI_TIME) + ListView_GetColumnWidth(HWindow, CI_DATE) +
          ListView_GetColumnWidth(HWindow, CI_SIZE) + GetSystemMetrics(SM_CXHSCROLL) - 1;

    ListView_SetColumnWidth(HWindow, CI_OLDNAME, cx / 3);
    ListView_SetColumnWidth(HWindow, CI_NEWNAME, cx / 3);

    cx -= ListView_GetColumnWidth(HWindow, CI_OLDNAME) +
          ListView_GetColumnWidth(HWindow, CI_NEWNAME);

    ListView_SetColumnWidth(HWindow, CI_PATH, cx);

    ListView_SetImageList(HWindow, HSymbolsImageList, LVSIL_SMALL);
    RecreateRenamerHeaderSortBitmap();
    UpdateListViewSortHeaderOverlay(HWindow, RenamerDialog->SortBy, RenamerDialog->ReverseSort,
                                    HRenamerHeaderSort, CI_DATE, CI_TIME,
                                    DarkModeShouldUseDarkColors());

    return TRUE;
}

void CPreviewWindow::Update(BOOL force)
{
    CALL_STACK_MESSAGE2("CPreviewWindow::Update(%d)", force);
    if (!Dirty && !force)
        return;

    if (RenamerDialog->TransferForPreview())
    {
        TransferError = FALSE;
        Renamer.SetOptions(&RenamerOptions);
    }
    else
        TransferError = TRUE;

    CachedItem = -1;

    RECT cl;
    GetClientRect(HWindow, &cl);

    RECT hr;
    GetWindowRect(ListView_GetHeader(HWindow), &hr);

    cl.top += hr.bottom - hr.top;
    if (!force)
    {
        cl.left = ListView_GetColumnWidth(HWindow, 0);
        cl.right = cl.left + ListView_GetColumnWidth(HWindow, 1);
        cl.left = 0; // so the icon is redrawn as well
    }

    InvalidateRect(HWindow, &cl, FALSE);

    // ListView_RedrawItems(HWindow, 0, SourceFiles.Count - 1);

    UpdateWindow(HWindow);
    Dirty = FALSE;
}

void CPreviewWindow::GetDispInfo(LV_DISPINFO* info)
{
    CALL_STACK_MESSAGE1("CPreviewWindow::GetDispInfo()");
    // TRACE_I("get-disp-info item=" << info->item.iItem <<
    //     " subitem=" << info->item.iSubItem <<
    //     (info->item.mask & LVIF_IMAGE ? " image" : "") <<
    //     (info->item.mask & LVIF_TEXT ? " text" : ""));

    if (info->item.iItem < SourceFiles.Count)
    {
        if (info->item.mask & LVIF_IMAGE)
        {
            if (CachedItem != info->item.iItem)
                GetItemText(info->item.iItem, CI_NEWNAME);
            info->item.iImage =
                NewNameValid ? (SourceFiles[info->item.iItem]->IsDir ? ILS_DIRECTORY : ILS_FILE) : ILS_WARNING;
        }
        if (info->item.mask & LVIF_TEXT)
            info->item.pszText = GetItemText(info->item.iItem, info->item.iSubItem);
    }
    else
    {
        static char emptyBuffer[] = "";
        if (info->item.mask & LVIF_IMAGE)
            info->item.iImage = ILS_FILE;
        if (info->item.mask & LVIF_TEXT)
            info->item.pszText = emptyBuffer;
    }
}

void CPreviewWindow::GetDispInfoW(NMLVDISPINFOW* info)
{
    CALL_STACK_MESSAGE1("CPreviewWindow::GetDispInfoW()");
    if (info->item.iItem < SourceFiles.Count)
    {
        if (info->item.mask & LVIF_IMAGE)
        {
            if (CachedItem != info->item.iItem)
                GetItemText(info->item.iItem, CI_NEWNAME);
            info->item.iImage =
                NewNameValid ? (SourceFiles[info->item.iItem]->IsDir ? ILS_DIRECTORY : ILS_FILE) : ILS_WARNING;
        }
        if (info->item.mask & LVIF_TEXT)
        {
            TextBufferW = PreviewTextToWide(GetItemText(info->item.iItem, info->item.iSubItem));
            info->item.pszText = const_cast<LPWSTR>(TextBufferW.c_str());
        }
    }
    else
    {
        if (info->item.mask & LVIF_IMAGE)
            info->item.iImage = ILS_FILE;
        if (info->item.mask & LVIF_TEXT)
            info->item.pszText = const_cast<LPWSTR>(L"");
    }
}

char* CPreviewWindow::GetItemText(int index, int subItem)
{
    CALL_STACK_MESSAGE3("CPreviewWindow::GetItemText(%d, %d)", index, subItem);
    CSourceFile* item = SourceFiles[index];
    static char emptyBuffer[] = "";
    char* ret = emptyBuffer;
    switch (subItem)
    {
    case CI_OLDNAME:
        switch (RenamerOptions.Spec)
        {
        case rsFileName:
            ret = item->Name;
            break;
        case rsRelativePath:
            ret = StripRoot(item->FullName, RootLen);
            break;
        case rsFullPath:
            ret = item->FullName;
            break;
        }
        break;

    case CI_NEWNAME:
        if (CachedItem != index)
        {
            NewNameValid = FALSE;
            if (RenamerDialog->ManualMode)
            {
                // optimization
                // int pos = SendDlgItemMessage(RenamerDialog->HWindow, IDE_MANUAL, EM_LINEINDEX, index, 0);
                // if (pos < 0)
                // {
                //   SalPrintf(NewNameCache, 3 * MAX_PATH, LoadStr(IDS_GENERICERR), LoadStr(IDS_MISLINES));
                // }
                // else
                // {
                //   int l = SendDlgItemMessage(RenamerDialog->HWindow, IDE_MANUAL, EM_LINELENGTH, pos, 0);

                //   if (l >= MAX_PATH)
                //   {
                //     SalPrintf(NewNameCache, 3 * MAX_PATH, LoadStr(IDS_GENERICERR), LoadStr(IDS_EXP_SMALLBUFFER));
                //   }
                //   else
                //   {
                if (!GetEditLineUtf8(RenamerDialog->ManualEdit->HWindow, index, NewNameCache, 3 * MAX_PATH))
                    SalPrintf(NewNameCache, 3 * MAX_PATH, LoadStr(IDS_GENERICERR), LoadStr(IDS_EXP_SMALLBUFFER));
                else
                    NewNameValid = ValidateFileName(NewNameCache, (int)strlen(NewNameCache), RenamerOptions.Spec, NULL, NULL);
                //  }
                // }
            }
            else
            {
                if (TransferError)
                    strcpy(NewNameCache, LoadStr(IDS_TRANSFERERROR));
                else
                {
                    if (Renamer.IsGood())
                    {
                        int l = Renamer.Rename(item, index, NewNameCache, FALSE);
                        if (l < 0)
                        {
                            SalPrintf(NewNameCache, 3 * MAX_PATH, LoadStr(IDS_GENERICERR), LoadStr(IDS_EXP_SMALLBUFFER));
                        }
                        else
                        {
                            NewNameValid = ValidateFileName(NewNameCache, l, RenamerOptions.Spec, NULL, NULL);
                        }
                    }
                    else
                    {
                        int error, errorPos1, errorPos2;
                        CRenamerErrorType errorType;
                        Renamer.GetError(error, errorPos1, errorPos2, errorType);
                        int et;
                        switch (errorType)
                        {
                        case retNewName:
                            et = IDS_NEWNAMEERR;
                            break;
                        case retBMSearch:
                            et = IDS_BMERR;
                            break;
                        case retRegExp:
                            et = IDS_REGEXPERR;
                            break;
                        case retReplacePattern:
                            et = IDS_REPLACEERR;
                            break;
                        default:
                            et = IDS_GENERICERR;
                            break;
                        }
                        SalPrintf(NewNameCache, 3 * MAX_PATH, LoadStr(et), LoadStr(error));
                    }
                }
            }
            CachedItem = index;
        }
        ret = NewNameCache;
        break;

    case CI_PATH:
        switch (RenamerOptions.Spec)
        {
        case rsFileName:
        {
            lstrcpyn(TextBuffer, item->FullName, _countof(TextBuffer));
            if (item->NameLen < _countof(TextBuffer) ||
                strchr(item->FullName + _countof(TextBuffer) - 1, '\\') == NULL)
            {
                SG->CutDirectory(TextBuffer);
            }
            ret = TextBuffer;
            break;
        }
        case rsRelativePath:
            ret = Root;
            break;
        case rsFullPath:
            break;
        }
        break;

    case CI_SIZE:
    {
        if (item->IsDir)
        {
            ret = DirText;
        }
        else
        {
            SG->NumberToStr(TextBuffer, item->Size);
            ret = TextBuffer;
        }
        break;
    }

    case CI_DATE:
    {
        // TODO: what time do we get from Salamander? what time do we get from FindXXFile?
        SYSTEMTIME st;
        FileTimeToSystemTime(&item->LastWrite, &st);
        if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, TextBuffer, 100))
            SalPrintf(TextBuffer, 100, "%u.%u.%u", st.wDay, st.wMonth, st.wYear);
        ret = TextBuffer;
        break;
    }

    case CI_TIME:
    {
        SYSTEMTIME st;
        FileTimeToSystemTime(&item->LastWrite, &st);
        if (!GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, TextBuffer, 100))
            SalPrintf(TextBuffer, 100, "%d:%d:%d", st.wHour, st.wMinute, st.wSecond);
        ret = TextBuffer;
        break;
    }
    }

    return ret;
}

BOOL CPreviewWindow::CustomDraw(LPNMLVCUSTOMDRAW cd, LRESULT& result)
{
    CALL_STACK_MESSAGE_NONE
    int item = (int)cd->nmcd.dwItemSpec; // x64 - dwItemSpec: The item number
    int subItem = cd->iSubItem;

    switch (cd->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
    {
        result = CDRF_NOTIFYITEMDRAW;
        return TRUE;
    }

    case CDDS_ITEMPREPAINT:
    {
        // request sending the CDDS_ITEMPREPAINT | CDDS_SUBITEM notification
        result = CDRF_NOTIFYSUBITEMDRAW;
        return TRUE;
    }

    case (CDDS_ITEMPREPAINT | CDDS_SUBITEM):
    {
        if (item >= SourceFiles.Count)
        {
            // we encountered two crashes when the number of elements in the SourceFiles array equaled 1 and item 1 was about to be drawn
            // the comment from one of the crashes is:
            // 5ADF745CD736D5EC-AS30B1PB87X64-120909-215934-A8CF45D9.7Z / mkolka@gmail.com
            // Mass renaming of files with detected duplicate files.
            // unfortunately I could not track down or reproduce the problem, so I simply handle this situation to avoid crashes
            TRACE_E("CPreviewWindow::CustomDraw() item=" << item << " Count=" << SourceFiles.Count);
            break;
        }

        if (DarkModeShouldUseDarkColors())
        {
            const DarkModeColors& colors = DarkModeGetColors();
            cd->clrTextBk = colors.background;
            if (subItem == CI_NEWNAME &&
                strcmp(GetItemText(item, CI_OLDNAME), GetItemText(item, CI_NEWNAME)) == 0)
                cd->clrText = RGB(0x90, 0x90, 0x90);
            else
                cd->clrText = colors.readableText;
        }
        else if (subItem == CI_NEWNAME &&
                 strcmp(GetItemText(item, CI_OLDNAME), GetItemText(item, CI_NEWNAME)) == 0)
            cd->clrText = GetSysColor(COLOR_GRAYTEXT);
        else
            cd->clrText = GetSysColor(COLOR_WINDOWTEXT);
        result = CDRF_NEWFONT;
        return TRUE;
    }
    }

    return FALSE;
}

int CPreviewWindow::CompareFunc(CSourceFile* f1, CSourceFile* f2, int sortBy)
{
    CALL_STACK_MESSAGE2("CPreviewWindow::CompareFunc(, , %d)", sortBy);
    int res;
    int next = 0, sortByIndex;
    const BOOL mixed = SortFilesAndDirsTogether;
    static int sortKeys[] = {CI_OLDNAME, CI_PATH, CI_SIZE, CI_TIME};
    switch (sortBy)
    {
    case CI_OLDNAME:
        next = 0;
        break;
    case CI_NEWNAME:
        next = 0;
        break;
    case CI_SIZE:
        next = 2;
        break;
    case CI_DATE:
        next = 3;
        break;
    case CI_TIME:
        next = 3;
        break;
    case CI_PATH:
        next = 1;
        break;
    }
    sortByIndex = next;
    do
    {
        switch (sortKeys[next])
        {
        case CI_OLDNAME:
        {
            if (f1->IsDir == f2->IsDir || SortFilesAndDirsTogether)
            {
                switch (RenamerOptions.Spec)
                {
                case rsFileName:
                    res = SG->RegSetStrICmp(f1->Name, f2->Name);
                    if (!res)
                        res = SG->RegSetStrCmp(f1->Name, f2->Name);
                    break;

                case rsFullPath:
                case rsRelativePath:
                    res = SG->RegSetStrICmp(f1->FullName, f2->FullName);
                    if (!res)
                        res = SG->RegSetStrCmp(f1->FullName, f2->FullName);
                    break;
                }
            }
            else
                res = f1->IsDir ? -1 : 1;
            break;
        }

        case CI_PATH:
        {
            if (f1->IsDir == f2->IsDir || mixed)
            {
                switch (RenamerOptions.Spec)
                {
                case rsFileName:
                {
                    res = SG->RegSetStrICmpEx(f1->FullName, (int)(f1->FullName - f1->Name),
                                              f2->FullName, (int)(f2->FullName - f2->Name), NULL);
                    if (!res)
                        res = SG->RegSetStrCmpEx(f1->FullName, (int)(f1->FullName - f1->Name),
                                                 f2->FullName, (int)(f2->FullName - f2->Name), NULL);
                    break;
                }
                case rsRelativePath:
                case rsFullPath:
                    res = 0;
                    break;
                }
            }
            else
                res = f1->IsDir ? -1 : 1;
            break;
        }

        case CI_SIZE:
        {
            if (f1->IsDir == f2->IsDir || SortFilesAndDirsTogether)
            {
                if (!SortFilesAndDirsTogether && f1->IsDir)
                    res = 0;
                else if (f1->Size < f2->Size)
                    res = -1;
                else if (f1->Size == f2->Size)
                    res = 0;
                else
                    res = 1;
            }
            else
                res = f1->IsDir ? -1 : 1;

            break;
        }

        case CI_TIME:
        {
            if (f1->IsDir == f2->IsDir || SortFilesAndDirsTogether)
                res = CompareFileTime(&f1->LastWrite, &f2->LastWrite);
            else
                res = f1->IsDir ? -1 : 1;
            break;
        }
        }
        if (res != 0 && RenamerDialog->ReverseSort &&
            (mixed || f1->IsDir == f2->IsDir))
            res = -res;
        if (next == sortByIndex)
        {
            if (sortByIndex != 0)
                next = 0;
            else
                next = 1;
        }
        else if (next + 1 != sortByIndex)
            next++;
        else
            next += 2;
    } while (res == 0 && next < 4);

    return res;
}

void CPreviewWindow::QuickSort(int left, int right, int sortBy)
{
    CALL_STACK_MESSAGE4("CFoundFilesListView::QuickSort(%d, %d, %d)", left,
                        right, sortBy);

LABEL_QuickSort:

    int i = left, j = right;
    CSourceFile* pivot = SourceFiles[(i + j) / 2];

    do
    {
        while (CompareFunc(SourceFiles[i], pivot, sortBy) < 0 && i < right)
            i++;
        while (CompareFunc(pivot, SourceFiles[j], sortBy) < 0 && j > left)
            j--;

        if (i <= j)
        {
            CSourceFile* swap = SourceFiles[i];
            SourceFiles[i] = SourceFiles[j];
            SourceFiles[j] = swap;
            i++;
            j--;
        }
    } while (i <= j);

    // we replaced the following "nice" code with code that is much more stack friendly (maximum log(N) recursion depth)
    //  if (left < j) QuickSort(left, j, sortBy);
    //  if (i < right) QuickSort(i, right, sortBy);

    if (left < j)
    {
        if (i < right)
        {
            if (j - left < right - i) // we need to sort both "halves", so we recurse into the smaller one and handle the other via "goto"
            {
                QuickSort(left, j, sortBy);
                left = i;
                goto LABEL_QuickSort;
            }
            else
            {
                QuickSort(i, right, sortBy);
                right = j;
                goto LABEL_QuickSort;
            }
        }
        else
        {
            right = j;
            goto LABEL_QuickSort;
        }
    }
    else
    {
        if (i < right)
        {
            left = i;
            goto LABEL_QuickSort;
        }
    }
}

void CPreviewWindow::SortItems(int sortBy)
{
    CALL_STACK_MESSAGE2("CPreviewWindow::SortItems(%d)", sortBy);

    HCURSOR hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (SourceFiles.Count > 0)
    {
        // store the position of the selected item
        // int focusIndex = ListView_GetNextItem(HWindow, -1, LVNI_FOCUSED);

        // sort the array by the requested criterion
        QuickSort(0, SourceFiles.Count - 1, sortBy);

        Update(TRUE);
        RecreateRenamerHeaderSortBitmap();
        UpdateListViewSortHeaderOverlay(HWindow, RenamerDialog->SortBy, RenamerDialog->ReverseSort,
                                        HRenamerHeaderSort, CI_DATE, CI_TIME,
                                        DarkModeShouldUseDarkColors());
    }

    SetCursor(hCursor);
}

int CPreviewWindow::GetSortCommand(int column)
{
    CALL_STACK_MESSAGE_NONE
    int cmd = 0;
    switch (column)
    {
    case CI_OLDNAME:
        cmd = CMD_SORTOLD;
        break;
    //case CI_NEWNAME: cmd = CMD_SORTOLD; break;
    case CI_SIZE:
        cmd = CMD_SORTSIZE;
        break;
    case CI_DATE:
        cmd = CMD_SORTTIME;
        break;
    case CI_TIME:
        cmd = CMD_SORTTIME;
        break;
    case CI_PATH:
        cmd = CMD_SORTPATH;
        break;
    }
    return cmd;
}

void CPreviewWindow::SetItemCount(int count, DWORD flags, int state)
{
    CALL_STACK_MESSAGE4("CPreviewWindow::SetItemCount(%d, 0x%X, %d)", count,
                        flags, state);

    CachedItem = -1;

    int oldCount = ListView_GetItemCount(HWindow);
    if (oldCount == count) // to avoid unnecessary flicker
    {
        InvalidateRect(HWindow, NULL, FALSE);
        UpdateWindow(HWindow);
    }
    else
        ListView_SetItemCountEx(HWindow, count, flags);

    if (count == 0)
    {
        int message;
        switch (state)
        {
        case 1:
            message = IDS_EMPTYMSG_NOMATCH;
            break;
        case 2:
            message = IDS_EMPTYMSG_BADMASK;
            break;
        case 3:
            message = IDS_EMPTYMSG_GENERATING;
            break;
        case 4:
            message = IDS_EMPTYMSG_ERROR;
            break;
        default:
            message = IDS_EMPTYMSG_DEF;
            break;
        }
        if (!Static)
        {
            RECT hr;
            GetWindowRect(ListView_GetHeader(HWindow), &hr);

            RECT cl;
            GetClientRect(HWindow, &cl);

            Static = ::CreateWindow(
                "Static",
                LoadStr(message),
                SS_LEFT | WS_VISIBLE | WS_CHILD, // style
                4, 4 + hr.bottom - hr.top, cl.right - 4, cl.bottom - (hr.bottom - hr.top) - 4,
                HWindow,
                (HMENU)666,
                DLLInstance,
                NULL);

            HFONT font = (HFONT)SendMessage(HWindow, WM_GETFONT, 0, 0);
            SendMessage(Static, WM_SETFONT, WPARAM(font), 0);
            if (DarkModeShouldUseDarkColors())
            {
                DarkModeApplyWindow(Static);
                DarkModeApplyStaticTextColors(HWindow, Static);
                RedrawWindow(Static, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);
            }
        }
        else
        {
            if (State != state)
                SetWindowText(Static, LoadStr(message));
        }
    }
    else
    {
        if (Static)
        {
            DestroyWindow(Static);
            Static = NULL;
        }
    }
    State = state;
}

LRESULT
CPreviewWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CPreviewWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg,
                        wParam, lParam);
    switch (uMsg)
    {
    case WM_NOTIFY:
    {
        LPNMHDR hdr = (LPNMHDR)lParam;
        HWND header = ListView_GetHeader(HWindow);
        if (hdr != NULL && header != NULL && hdr->hwndFrom == header && hdr->code == NM_CUSTOMDRAW)
        {
            RecreateRenamerHeaderSortBitmap();
            LPNMCUSTOMDRAW cd = (LPNMCUSTOMDRAW)lParam;
            BOOL showSort = ListViewHeaderColumnShowsSort(RenamerDialog->SortBy, (int)cd->dwItemSpec,
                                                          CI_DATE, CI_TIME);
            BOOL dark = DarkModeShouldUseDarkColors();
            COLORREF darkBg = RGB(0x20, 0x20, 0x20);
            COLORREF darkText = dark ? DarkModeGetColors().readableText : GetSysColor(COLOR_BTNTEXT);
            COLORREF darkLine = RGB(0x38, 0x38, 0x38);
            LRESULT customDrawResult = 0;
            if (HandleListViewHeaderSortCustomDraw(cd, &customDrawResult, showSort, RenamerDialog->ReverseSort,
                                                   HRenamerHeaderSort, dark, darkBg, darkText, darkLine))
                return customDrawResult;
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
        if (HWND(lParam) == Static)
        {
            INT_PTR result = 0;
            if (HandleRenamerDarkCtlColor(uMsg, wParam, lParam, &result))
                return result;
            SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        break;

        // case WM_ERASEBKGND:
        // {
        //   if (ListView_GetItemCount(HWindow) > 0)
        //   {
        //     int top = ListView_GetTopIndex(HWindow);

        //     RECT ir;
        //     ListView_GetItemRect(HWindow, top, &ir, LVIR_BOUNDS);

        //     RECT cl;
        //     GetClientRect(HWindow, &cl);
        //     int right = cl.right;
        //     cl.right = ir.left;
        //     FillRect((HDC)wParam, &cl, (HBRUSH) (COLOR_WINDOW+1));

        //     cl.left = ir.right;
        //     cl.right = right;
        //     FillRect((HDC)wParam, &cl, (HBRUSH) (COLOR_WINDOW+1));
        //     return 1;
        //   }
        //   break;
        // }
    }
    return CWindow::WindowProc(uMsg, wParam, lParam);
}
