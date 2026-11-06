// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "checksum.h"
#include "checksum.rh"
#include "checksum.rh2"
#include "lang\lang.rh"
#include "dialogs.h"
#include "misc.h"
#include "../../darkmode.h"

CWindowQueue ModelessQueue("CheckSum Modeless Windows");  // list of all modeless windows
CThreadQueue ThreadQueue("CheckSum Dialogs and Workers"); // list of all dialog and worker threads

#define BUFSIZE (4 * 65536) // buffer size for reading

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

#define CM_REMOVEITEM 100
#define CM_COPYALLVALUES 101

// even for small files we want to move the progress bar
// originally this constant was 1, but the progress behaved very non-linearly when processing thousands
// of small files mixed with 1MB+ files; experimentation led to this value
#define FILE_SIZE_FIX 10000

#define IDT_UPDATEUI 10
#define IDT_UPDATESUI_PERIOD 100 // [ms]

#define IDT_RESENDCLOSE 11


static void SetDispInfoText(NMLVDISPINFO* plvdi, const char* text)
{
    if (plvdi == NULL || (plvdi->item.mask & LVIF_TEXT) == 0 || text == NULL)
        return;
    if (plvdi->hdr.code == LVN_GETDISPINFOW)
    {
        int maxChars = plvdi->item.cchTextMax;
        if (maxChars <= 0 || plvdi->item.pszText == NULL)
            return;
        const std::wstring wide = ChecksumPaths::Wide(text);
        size_t count = (std::min)(wide.size(), static_cast<size_t>(maxChars - 1));
        if (count > 0 && count < wide.size() && wide[count - 1] >= 0xD800 && wide[count - 1] <= 0xDBFF) --count;
        memcpy(plvdi->item.pszText, wide.data(), count * sizeof(wchar_t));
        reinterpret_cast<wchar_t*>(plvdi->item.pszText)[count] = 0;
    }
    else
    {
        lstrcpynA(plvdi->item.pszText, text, plvdi->item.cchTextMax);
    }
}

static void ClearDispInfoText(NMLVDISPINFO* plvdi)
{
    if (plvdi == NULL || plvdi->item.pszText == NULL || plvdi->item.cchTextMax <= 0)
        return;
    if (plvdi->hdr.code == LVN_GETDISPINFOW)
        ((LPWSTR)plvdi->item.pszText)[0] = 0;
    else
        plvdi->item.pszText[0] = 0;
}

// ****************************************************************************************************
//
//  CSFVMD5Dialog
//

CSFVMD5Dialog::CSFVMD5Dialog(int id, HWND parent, BOOL alwaysOnTop) : CDialog(HLanguage, id, NULL), hParent(parent), FileList(1000, 1000)
{
    HANDLES(InitializeCriticalSection(&DataCS));
    hThread = NULL;
    iThreadID = 0;
    bThreadRunning = FALSE;
    bTerminateThread = FALSE;
    bAlwaysOnTop = alwaysOnTop;
    ScheduledScrollIndex = 0;
    ScrollIndex = 0;
    CurrentSize.Value = 0;
    ScheduledCurrentSize.Value = 0;
    DirtyRowMin = -1;
    DirtyRowMax = -1;
    ReadingDirectories = FALSE;
    StopReadingDirectories = FALSE;
}

CSFVMD5Dialog::~CSFVMD5Dialog()
{
    HANDLES(DeleteCriticalSection(&DataCS));
}

void CSFVMD5Dialog::InitList(int columns[], int widths[], int numcols, SHashInfo* pHashInfo)
{
    CALL_STACK_MESSAGE2("CSFVMD5Dialog::InitList( , , %d)", numcols);

    hImg = ImageList_Create(16, 16, SalamanderGeneral->GetImageListColorFlags() | ILC_MASK, 0, 1);
    static int id[] = {IDI_FILE1, IDI_FILE2, IDI_FILE3, IDI_FILE4};
    int i;
    for (i = 0; i < SizeOf(id); i++)
    {
        HICON hIcon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(id[i]), IMAGE_ICON,
                                       0, 0, SalamanderGeneral->GetIconLRFlags());
        ImageList_AddIcon(hImg, hIcon);
        DestroyIcon(hIcon);
    }

#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER 0x00010000
#endif
    hList = GetDlgItem(HWindow, IDC_LIST_FILES);
    ListView_SetExtendedListViewStyleEx(hList, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
    if (SalIsWindowsVersionOrGreater(6, 0, 0)) // WindowsVistaAndLater: Vista and later (CommonControls 6.0+)
        ListView_SetExtendedListViewStyleEx(hList, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);
    ListView_SetImageList(hList, hImg, LVSIL_SMALL); // the imagelist takes care of destruction

    int j;
    for (j = 0; j < numcols; j++)
    {
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        lvc.pszText = LoadStr(columns[j]);
        lvc.cchTextMax = (int)_tcslen(lvc.pszText);
        lvc.cx = widths[j];
        lvc.fmt = (j == 1) ? LVCFMT_RIGHT : 0;
        ListView_InsertColumn(hList, j, &lvc);
    }
    if (pHashInfo)
    {
        int k, l;
        for (k = l = 0; k < HT_COUNT; k++)
            if (pHashInfo[k].bCalculate)
            {
                LVCOLUMN lvc;
                lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
                lvc.pszText = LoadStr(pHashInfo[k].idColumnHeader);
                lvc.cchTextMax = (int)_tcslen(lvc.pszText);
                lvc.cx = widths[numcols + k];
                lvc.fmt = 0;
                ListView_InsertColumn(hList, numcols + l, &lvc);
                l++;
            }
    }
}

void CSFVMD5Dialog::InitLayout(int id[], int n, int m)
{
    CALL_STACK_MESSAGE3("CSFVMD5Dialog::InitLayout( , %d, %d)", n, m);

    RECT client, rc;
    GetClientRect(HWindow, &client);
    GetWindowRect(HWindow, &rc);
    minX = rc.right - rc.left;
    minY = rc.bottom - rc.top;
    GetWindowRect(hList, &rc);
    listX = client.right - (rc.right - rc.left);
    listY = client.bottom - (rc.bottom - rc.top);
    GetClientRect(HWindow, &client);
    int i;
    for (i = 0; i < n; i++)
    {
        GetWindowRect(GetDlgItem(HWindow, id[i]), &rc);
        ScreenToClient(HWindow, (LPPOINT)&rc);
        ctrlX[i] = (i < m) ? client.right - rc.left : rc.left;
        ctrlY[i] = client.bottom - rc.top;
    }
}

void CSFVMD5Dialog::RecalcLayout(int cx, int cy, int id[], int n, int m)
{
    CALL_STACK_MESSAGE5("CSFVMD5Dialog::RecalcLayout(%d, %d, , %d, %d)", cx, cy, n, m);

    SetWindowPos(hList, NULL, 0, 0, cx - listX, cy - listY, SWP_NOZORDER | SWP_NOMOVE);
    int i;
    for (i = 0; i < n; i++)
        SetWindowPos(GetDlgItem(HWindow, id[i]), 0, (i < m) ? cx - ctrlX[i] : ctrlX[i], cy - ctrlY[i], 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

void CSFVMD5Dialog::SaveSizes(int sizes[], int numcols, SHashInfo* pHashInfo)
{
    CALL_STACK_MESSAGE2("CSFVMD5Dialog::SaveSizes( , %d)", numcols);

    RECT rc;
    GetWindowRect(HWindow, &rc);
    sizes[0] = rc.right - rc.left;
    sizes[1] = rc.bottom - rc.top;
    int i;
    for (i = 0; i < numcols; i++)
        sizes[i + 2] = ListView_GetColumnWidth(hList, i);

    if (pHashInfo)
    {
        int k = 0, l = numcols;
        for (; k < HT_COUNT; k++)
            if (pHashInfo[k].bCalculate)
                sizes[2 + numcols + k] = ListView_GetColumnWidth(hList, l++);
    }
}

void CSFVMD5Dialog::ConvertPath(char* str, char from, char to)
{
    while (NULL != (str = _tcschr(str, from)))
    {
        *str = to;
    }
}

void CSFVMD5Dialog::OnThreadEnd()
{
    CALL_STACK_MESSAGE1("CSFVMD5Dialog::OnThreadEnd()");
    SetDlgItemText(HWindow, IDC_BUTTON_CLOSE, LoadStr(IDS_CLOSE));
    ShowWindow(GetDlgItem(HWindow, IDC_LABEL), SW_HIDE);
    ShowWindow(GetDlgItem(HWindow, IDC_PROGRESS), SW_HIDE);
    bThreadRunning = FALSE;
    if (ScrollIndex < FileList.Count) // the worker already stopped counting (it may still be running), no sync needed
    {                                 // update the last "calculated" item so it does not remain calculating / verifying ...
        SetRowsDirty(ScrollIndex, ScrollIndex);
    }
}

void CSFVMD5Dialog::SetRowsDirty(int firstRow, int lastRow)
{
    if (lastRow < firstRow)
        TRACE_E("CSFVMD5Dialog::SetRowsDirty(): lastRow < firstRow");
    if (DirtyRowMin == -1 || firstRow < DirtyRowMin)
        DirtyRowMin = firstRow;
    if (lastRow > DirtyRowMax)
        DirtyRowMax = lastRow;
}

void CSFVMD5Dialog::SetItemTextAndIcon(int row, int col, const char* text, int icon)
{
    // CALL_STACK_MESSAGE4("CSFVMD5Dialog::SetItemTextAndIcon(%d, %d, , %d)", row, col, icon);
    FILELISTITEM* item = FileList[row]; // while the worker thread runs, the array is not modified (index is OK)

    // hashes and icon do not need synchronization; while the worker thread is running, the dialog thread
    // only reads items before ScrollIndex (which is at most ScheduledScrollIndex) and the worker writes
    // only to the ScheduledScrollIndex item, so there is no conflict
    if (text != NULL)
    {
        if (col < 2 || col - 2 >= HT_COUNT)
            TRACE_E("Wrong col: " << col);
        else
        {
            if (item->Hashes[col - 2] != NULL)
                free(item->Hashes[col - 2]);
            item->Hashes[col - 2] = _strdup(text);
        }
    }
    if (icon != -1)
        item->IconIndex = icon;
}

void CSFVMD5Dialog::GetItemText(int row, int col, char* text, int textMax)
{
    CALL_STACK_MESSAGE_NONE // frequently called function
        // CALL_STACK_MESSAGE4("CSFVMD5Dialog::GetItemText(%d, %d, , %d)", row, col, textMax);
        LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = row;
    lvi.iSubItem = col;
    lvi.pszText = text;
    lvi.cchTextMax = textMax;
    ListView_GetItem(hList, &lvi);
}

void CSFVMD5Dialog::IncreaseProgress(const CQuadWord& delta)
{
    CALL_STACK_MESSAGE_NONE // frequently called function
    // CALL_STACK_MESSAGE1("CSFVMD5Dialog::IncreaseProgress()");
    EnterDataCS();
    ScheduledCurrentSize += delta; // 64-bit value -> synchronization required in 32-bit code
    LeaveDataCS();
}

void CSFVMD5Dialog::DeleteItem(int index)
{
    CALL_STACK_MESSAGE2("CSFVMD5Dialog::DeleteItem(%d)", index);
    // while the worker thread runs, the array is not modified (this function must not be called either)
    if (bThreadRunning)
        TRACE_E("CSFVMD5Dialog::DeleteItem(): unexpected situation: worker thread should not be running!");
    if (FileList.Count > 0 && index < FileList.Count)
    {
        FileList.Delete(index);
        if (index >= FileList.Count)
            index = FileList.Count - 1;
        ListView_SetItemCountEx(hList, FileList.Count, 0);
        if (index >= 0)
            ListView_SetItemState(hList, index, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
    }
}

void CSFVMD5Dialog::ScrollToItem(int i)
{
    CALL_STACK_MESSAGE2("CSFVMD5Dialog::ScrollToItem(%d)", i);
    EnterDataCS();
    ScheduledScrollIndex = i;
    LeaveDataCS();
}

BOOL CSFVMD5Dialog::AddFileListItem(const char* name, CQuadWord size, BOOL fileExist, const char* fullPath)
try
{
    if (bThreadRunning)
        TRACE_E("CSFVMD5Dialog::AddFileListItem(): unexpected running worker");
#ifdef new
#undef new
#define CHECKSUM_RESTORE_ITEM_NEW
#endif
    std::unique_ptr<FILELISTITEM> item(new (std::nothrow) FILELISTITEM());
#ifdef CHECKSUM_RESTORE_ITEM_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_ITEM_NEW
#endif
    if (!item) return FALSE;
    item->Name = _strdup(name);
    if (item->Name == NULL) return FALSE;
    item->FullPath = fullPath != NULL ? fullPath : "";
    item->Size = size;
    item->FileExist = fileExist;
    FileList.Add(item.get());
    if (!FileList.IsGood()) { FileList.ResetState(); return FALSE; }
    item.release();
    return TRUE;
}
catch (const std::bad_alloc&) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE; }

INT_PTR CSFVMD5Dialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE_NONE // frequently called function
        //CALL_STACK_MESSAGE4("CSFVMD5Dialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        bScrollToItem = TRUE;
        bDisableNotification = FALSE;

        if (bAlwaysOnTop) // handle always-on-top at least "statically" (it is not in the system menu)
            SetWindowPos(HWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        CSFVMD5ListView* lv = new CSFVMD5ListView(this);
        if (lv != NULL)
        {
            lv->AttachToWindow(GetDlgItem(HWindow, IDC_LIST_FILES));
            if (lv->HWindow == NULL)
                delete lv; // not attached = automatic deallocation will not happen
        }

        ApplyChecksumDarkMode(HWindow);

        SetForegroundWindow(HWindow);

        SetTimer(HWindow, IDT_UPDATEUI, IDT_UPDATESUI_PERIOD, NULL);
        break;
    }

    case WM_THEMECHANGED:
    {
        ApplyChecksumDarkMode(HWindow);
        RedrawWindow(HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        return TRUE;
    }

    case WM_SETTINGCHANGE:
    {
        ConfigureChecksumDarkModeFromHost();
        if (DarkModeHandleSettingChange(uMsg, lParam))
        {
            ApplyChecksumDarkMode(HWindow);
            InvalidateRect(HWindow, NULL, TRUE);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSCROLLBAR:
    {
        INT_PTR result = 0;
        if (HandleChecksumDarkCtlColor(uMsg, wParam, lParam, &result))
            return result;
        break;
    }

    case WM_USER_ENDWORK:
        OnThreadEnd();
        return TRUE;

    case WM_TIMER:
    {
        if (wParam == IDT_RESENDCLOSE) // we should send ourselves WM_CLOSE; the blocking dialog should now be gone
        {
            KillTimer(HWindow, IDT_RESENDCLOSE);
            PostMessage(HWindow, WM_CLOSE, 0, 0);
            return TRUE;
        }
        if (wParam == IDT_UPDATEUI)
        {
            EnterDataCS();
            int progress = -1; // -1 = do not adjust the progress bar
            if (totalSize.Value != 0 && ScheduledCurrentSize != CurrentSize)
            {
                CurrentSize = ScheduledCurrentSize;
                progress = CurrentSize < totalSize ? (int)((CurrentSize * CQuadWord(1024, 0)) / totalSize).Value : 1024;
            }
            int i = -1; // -1 = do not call ensure visible
            if (ScheduledScrollIndex != ScrollIndex)
            {
                // at this moment items up to ScrollIndex are acknowledged as computed,
                // ScrollIndex itself is calculating / verifying and the items after it remain unprocessed
                // (even if they are already computed, they will surface in the next cycle)
                // show the newly acknowledged computed data + the new ScrollIndex must be repainted
                // (to print calculating / verifying)
                SetRowsDirty(ScrollIndex, ScheduledScrollIndex);
                ScrollIndex = ScheduledScrollIndex;
                if (bScrollToItem)
                    i = ScrollIndex;
            }
            int dirtyRowMin = DirtyRowMin;
            int dirtyRowMax = DirtyRowMax;
            if (DirtyRowMin != -1)
            {
                DirtyRowMin = -1;
                DirtyRowMax = -1;
            }
            LeaveDataCS();

            if (progress != -1)
            {
                HWND pr = GetDlgItem(HWindow, IDC_PROGRESS);
                SendMessage(pr, PBM_SETPOS, progress + 1, 0);
                // hack for progress bar lag in aero: http://stackoverflow.com/questions/22469876/progressbar-lag-when-setting-position-with-pbm-setpos
                SendMessage(pr, PBM_SETPOS, progress, 0);
            }
            if (dirtyRowMin != -1)
                ListView_RedrawItems(hList, dirtyRowMin, dirtyRowMax);
            if (i != -1)
                ListView_EnsureVisible(hList, i, FALSE);

            return TRUE;
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (bDisableNotification)
            break;

        if (wParam == IDC_LIST_FILES)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW nmhi = (LPNMLISTVIEW)nmh;
                if (!(nmhi->uOldState & LVIS_SELECTED) && nmhi->uNewState & LVIS_SELECTED)
                    bScrollToItem = FALSE; // user changed the selection -> disable autoscroll
            }
            }
        }
        break;
    }

    case WM_SIZING:
    {
        RECT* lprc = (RECT*)lParam;
        SIZE size = {lprc->right - lprc->left, lprc->bottom - lprc->top};
        if (size.cx < minX)
        {
            if (wParam == WMSZ_LEFT || wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT)
                lprc->left = lprc->right - minX;
            else
                lprc->right = lprc->left + minX;
        }
        if (size.cy < minY)
        {
            if (wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
                lprc->top = lprc->bottom - minY;
            else
                lprc->bottom = lprc->top + minY;
        }
        break;
    }

    case WM_CLOSE:
    {
        if (!IsWindowEnabled(HWindow))
        { // close all dialogs stacked above this one (send them WM_CLOSE and then send it here again)
            SalamanderGeneral->CloseAllOwnedEnabledDialogs(HWindow);
            if (iThreadID != 0) // if a thread is running, close its windows too and let it finish
            {                   // to avoid immediately opening another window with a new error
                bTerminateThread = TRUE;
                SalamanderGeneral->CloseAllOwnedEnabledDialogs(HWindow, iThreadID);
            }
            SetTimer(HWindow, IDT_RESENDCLOSE, 100, NULL);
            return TRUE;
        }

        EndDialog(HWindow, IDCANCEL);
        return TRUE;
    }

    case WM_DESTROY:
    {
        if (hThread != NULL) // if we started the thread, close it and wait for it
        {
            bTerminateThread = TRUE;
            ThreadQueue.WaitForExit(hThread, INFINITE);
            hThread = NULL;
            iThreadID = 0;
        }
        ModelessQueue.Remove(HWindow);
        KillTimer(HWindow, IDT_UPDATEUI);
        break;
    }
    }
    return CDialog::DialogProc(uMsg, wParam, lParam);
} /* CSFVMD5Dialog::DialogProc */

// ****************************************************************************************************
//
//  CCalculateDialog
//

CCalculateDialog::CCalculateDialog(HWND parent, BOOL alwaysOnTop, TSeedFileList* pFileList, const char* sourcePath)
    : CSFVMD5Dialog(IDD_CALCULATE, parent, alwaysOnTop)
{
    pSeedFileList = pFileList;
    SourcePath = sourcePath;
    memcpy(HashInfo, Config.HashInfo, sizeof(Config.HashInfo));
}

#define REFRESH_LIMIT 1000

void CCalculateDialog::RefreshUI()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // we want responsive GUI
    {
        if (!IsWindow(HWindow) || !IsDialogMessage(HWindow, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

BOOL CCalculateDialog::AddDir(const std::wstring& path, const std::string& relative, BOOL* ignoreAll)
{
    if (StopReadingDirectories) return FALSE;
    const std::wstring pattern = ChecksumPaths::IoPath(ChecksumPaths::Join(path, L"*"));
    WIN32_FIND_DATAW fd;
    HANDLE find;
    for (;;)
    {
        find = HANDLES_Q(FindFirstFileW(pattern.c_str(), &fd));
        if (find != INVALID_HANDLE_VALUE) break;
        const std::string display = ChecksumPaths::Utf8(path.c_str());
        if (SalamanderGeneral->DialogError(HWindow, BUTTONS_RETRYCANCEL, display.c_str(),
            LoadStr(IDS_ERRORREADINGDIR), NULL) != DIALOG_RETRY) return FALSE;
    }
    BOOL result = TRUE;
    try
    {
        do
        {
            if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
            const std::wstring child = ChecksumPaths::Join(path, fd.cFileName);
            const std::string full = ChecksumPaths::Utf8(child.c_str());
            const std::string name = relative + "\\" + ChecksumPaths::Utf8(fd.cFileName);
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                result = AddDir(child, name, ignoreAll);
            else
            {
                BOOL cancel = FALSE;
                CQuadWord size(fd.nFileSizeLow, fd.nFileSizeHigh), linkSize;
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                    SalamanderGeneral->GetLinkTgtFileSize(HWindow, full.c_str(), &linkSize, &cancel, ignoreAll))
                    size = linkSize;
                result = !cancel && AddFileListItem(name.c_str(), size, TRUE, full.c_str());
                if (result) totalSize += size + CQuadWord(FILE_SIZE_FIX, 0);
            }
            if (RefreshCounter++ > REFRESH_LIMIT)
            {
                ListView_SetItemCountEx(hList, FileList.Count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
                RefreshUI(); RefreshCounter = 0;
            }
        } while (!StopReadingDirectories && result && FindNextFileW(find, &fd));
    }
    catch (...) { HANDLES(FindClose(find)); throw; }
    HANDLES(FindClose(find));
    return result && !StopReadingDirectories;
}

BOOL CCalculateDialog::GetFileList()
try
{
    totalSize = CQuadWord(0, 0);
    RefreshCounter = 0;
    BOOL result = TRUE, ignoreAll = FALSE;
    for (int i = 0; result && i < pSeedFileList->Count; ++i)
    {
        SEEDFILEINFO* seed = (*pSeedFileList)[i];
        if (seed->bDir)
            result = AddDir(ChecksumPaths::Wide(seed->FullPath.c_str()), seed->Name, &ignoreAll);
        else
        {
            BOOL cancel = FALSE;
            CQuadWord linkSize;
            if ((seed->Attr & FILE_ATTRIBUTE_REPARSE_POINT) &&
                SalamanderGeneral->GetLinkTgtFileSize(HWindow, seed->FullPath.c_str(), &linkSize, &cancel, &ignoreAll))
                seed->Size = linkSize;
            result = !cancel && AddFileListItem(seed->Name.c_str(), seed->Size, TRUE, seed->FullPath.c_str());
            if (result) totalSize += seed->Size + CQuadWord(FILE_SIZE_FIX, 0);
            if (RefreshCounter++ > REFRESH_LIMIT) { RefreshUI(); RefreshCounter = 0; }
        }
    }
    if (result)
    {
        ListView_SetItemCountEx(hList, FileList.Count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
        if (FileList.Count > 0)
        {
            bDisableNotification = TRUE;
            ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
            bDisableNotification = FALSE;
        }
    }
    return result;
}
catch (const std::bad_alloc&) { Error(HWindow, 0, IDS_PLUGINNAME, IDS_OUTOFMEM); return FALSE; }

class CCalculateThread : public CCRCMD5Thread
{
public:
    CCalculateThread(CCalculateDialog* dlg, BOOL* terminate) : CCRCMD5Thread(terminate) { dialog = dlg; };
    virtual unsigned Body();

protected:
    CCalculateDialog* dialog;
};

unsigned CCalculateThread::Body()
{
    CALL_STACK_MESSAGE1("CCalculateThread::Body()");
    TRACE_I("Begin");

    BOOL skippedReadError = FALSE;
    BOOL skipAllReadErrors = FALSE;
    BOOL skip;
    CHashAlgo* pCalculators[HT_COUNT];
    int nCalculators = 0;

    int ii;
    for (ii = 0; ii < HT_COUNT; ii++)
    {
        if (dialog->HashInfo[ii].bCalculate)
        {
            pCalculators[nCalculators] = dialog->HashInfo[ii].Factory();
            if (!pCalculators[nCalculators])
            {
                while (nCalculators-- > 0)
                    delete pCalculators[nCalculators];
                TRACE_E("Could not initialize " << dialog->HashInfo[ii].sRegID);
                if (dialog->FileList.Count > 0)
                    dialog->SetItemTextAndIcon(0, 2, LoadStr(IDS_CANCELED));
                TRACE_I("End");
                PostMessage(dialog->HWindow, WM_USER_ENDWORK, 0, 0);
                return 0;
            }
            nCalculators++;
        }
    }

    // while the worker thread runs, the array is not modified (the number of items + indices do
    // not change = no need to synchronize access to them)
    int silent = 0;
    for (int i = 0; i < dialog->FileList.Count && !*Terminate; i++)
    {
        // scroll to the current item (only if the previous one is visible, i.e. the user did not jump to the top)
        dialog->ScrollToItem(i);

        // open the file
        HANDLE hFile;
        const char* path = dialog->FileList[i]->FullPath.c_str();
        if (!SafeOpenCreateFile(path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
                                &hFile, &skip, &silent, dialog->HWindow))
            break;
        if (skip)
        {
            dialog->SetItemTextAndIcon(i, 2, LoadStr(IDS_SKIPPED));
            // advance progress by the size of the skipped file
            WIN32_FIND_DATAW fd;
            memset(&fd, 0, sizeof(fd));
            HANDLE find = HANDLES_Q(FindFirstFileW(ChecksumPaths::IoPath(path).c_str(), &fd));
            if (find != INVALID_HANDLE_VALUE)
            {
                HANDLES(FindClose(find));
                CQuadWord size(fd.nFileSizeLow, fd.nFileSizeHigh);
                if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                    (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                {
                    if (!SalamanderGeneral->SalGetFileSize2(path, size, NULL))
                        size.Set(fd.nFileSizeLow, fd.nFileSizeHigh);
                }
                dialog->IncreaseProgress(size + CQuadWord(FILE_SIZE_FIX, 0));
            }
            continue;
        }

        // Now calculates the hashes
        int j;
        for (j = 0; j < nCalculators; j++)
            pCalculators[j]->Init();

        DWORD nr;
        CQuadWord done(0, 0);
        do
        {
            char buffer[BUFSIZE];
            if (!SafeReadFile(hFile, buffer, BUFSIZE, &nr, path, dialog->HWindow, &skippedReadError, &skipAllReadErrors))
            {
                nr = 0; // read error
                if (skippedReadError)
                {
                    dialog->SetItemTextAndIcon(i, 2, LoadStr(IDS_SKIPPED));
                    // advance progress by the size of the skipped file
                    WIN32_FIND_DATAW fd;
                    memset(&fd, 0, sizeof(fd));
                    HANDLE find = HANDLES_Q(FindFirstFileW(ChecksumPaths::IoPath(path).c_str(), &fd));
                    if (find != INVALID_HANDLE_VALUE)
                    {
                        HANDLES(FindClose(find));
                        CQuadWord size(fd.nFileSizeLow, fd.nFileSizeHigh);
                        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                            (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                        {
                            if (!SalamanderGeneral->SalGetFileSize2(path, size, NULL))
                                size.Set(fd.nFileSizeLow, fd.nFileSizeHigh);
                        }
                        if (size >= done)
                        {
                            size -= done;
                            dialog->IncreaseProgress(size);
                        }
                    }
                }
                else
                    *Terminate = TRUE;
            }
            if (nr > 0)
            {
                for (int j2 = 0; j2 < nCalculators; j2++)
                    pCalculators[j2]->Update(buffer, nr);
                dialog->IncreaseProgress(CQuadWord(nr, 0));
                done += CQuadWord(nr, 0);
            }
        } while (nr == BUFSIZE && !*Terminate && !skippedReadError);
        if (!*Terminate)
            dialog->IncreaseProgress(CQuadWord(FILE_SIZE_FIX, 0));
        CloseHandle(hFile);

        // store the results in the list
        if (!*Terminate && !skippedReadError)
        {
            for (int k = 0; k < nCalculators; k++)
                pCalculators[k]->Finalize();

            char digest[DIGEST_MAX_SIZE];
            char text[2 * DIGEST_MAX_SIZE + 1];

            int j2;
            for (j2 = 0; j2 < nCalculators; j2++)
            {
                int len = pCalculators[j2]->GetDigest(digest, SizeOf(digest));
                text[0] = 0;
                int k2;
                for (k2 = 0; k2 < len; k2++)
                    sprintf(text + k2 * 2, "%02X", digest[k2]);
                dialog->SetItemTextAndIcon(i, 2 + j2, text);
            }
        }
        else
        {
            if (*Terminate)
                dialog->SetItemTextAndIcon(i, 2, LoadStr(IDS_CANCELED));
        }
    }

    while (nCalculators > 0)
        delete pCalculators[--nCalculators];
    TRACE_I("End");
    PostMessage(dialog->HWindow, WM_USER_ENDWORK, 0, 0);
    return 0;
}

void CCalculateDialog::OnThreadEnd()
{
    CALL_STACK_MESSAGE1("CCalculateDialog::OnThreadEnd()");
    ShowWindow(GetDlgItem(HWindow, IDC_LABEL_HINT), SW_SHOW);
    if (ListView_GetItemCount(hList))
    {
        // Enable Save button only when calculating some checksums (their columns are visible)
        int i;
        for (i = 0; i < HT_COUNT; i++)
        {
            if (HashInfo[i].bCalculate)
            {
                EnableButtons(TRUE);
                break;
            }
        }
    }
    CSFVMD5Dialog::OnThreadEnd();
}

void CCalculateDialog::EnableButtons(BOOL bEnable)
{
    CALL_STACK_MESSAGE2("CCalculateDialog::EnableButtons(%d)", bEnable);
    EnableWindow(GetDlgItem(HWindow, IDC_BUTTON_SAVE), bEnable);
}

void CCalculateDialog::DeleteItem(int index)
{
    CALL_STACK_MESSAGE2("CCalculateDialog::DeleteItem(%d)", index);
    CSFVMD5Dialog::DeleteItem(index);
    if (!ListView_GetItemCount(hList))
        EnableButtons(FALSE);
}

BOOL CCalculateDialog::GetSaveFileName(std::wstring& result, LPCTSTR title)
{
    if (FileList.Count == 0) return FALSE;
    std::wstring first = ChecksumPaths::Base(ChecksumPaths::Wide(FileList[0]->Name));
    size_t dot = first.find_last_of(L'.');
    if (dot != std::wstring::npos) first.resize(dot);
    BOOL allSame = TRUE;
    for (int i = 1; i < FileList.Count; ++i)
    {
        std::wstring next = ChecksumPaths::Base(ChecksumPaths::Wide(FileList[i]->Name));
        dot = next.find_last_of(L'.');
        if (dot != std::wstring::npos) next.resize(dot);
        if (CompareStringOrdinal(first.c_str(), -1, next.c_str(), -1, TRUE) != CSTR_EQUAL) { allSame = FALSE; break; }
    }
    const std::wstring directory = ChecksumPaths::Wide(SourcePath);
    const std::wstring caption = ChecksumPaths::Wide(title);
    if (!allSame) first = ChecksumPaths::Base(directory);
    std::vector<wchar_t> buffer(SAL_MAX_PATH, L'\0');
    if (first.size() < buffer.size()) std::copy(first.begin(), first.end(), buffer.begin());
    std::wstring filter;
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = HWindow; ofn.hInstance = HLanguage;
    ofn.nFilterIndex = 1;
    int filterIndex = 0;
    for (int j = 0; j < HT_COUNT; ++j)
        if (HashInfo[j].bCalculate)
        {
            ++filterIndex;
            if (HashInfo[j].Type == Config.HashType) ofn.nFilterIndex = filterIndex;
            filter += ChecksumPaths::Wide(LoadStr(HashInfo[j].idSaveAsFilter));
        }
    for (size_t i = 0; i < filter.size(); ++i) if (filter[i] == L'|') filter[i] = L'\0';
    filter += L'\0';
    ofn.lpstrFilter = filter.c_str(); ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = static_cast<DWORD>(buffer.size()); ofn.lpstrInitialDir = directory.c_str();
    ofn.lpstrTitle = title != NULL ? caption.c_str() : NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    for (;;)
    {
        if (!::GetSaveFileNameW(&ofn)) return FALSE;
        int selected = 0;
        for (int j = 0; j < HT_COUNT; ++j)
            if (HashInfo[j].bCalculate && ++selected == static_cast<int>(ofn.nFilterIndex))
            { Config.HashType = static_cast<eHASH_TYPE>(j); break; }
        result = buffer.data();
        const std::wstring extension = ChecksumPaths::Wide(HashInfo[Config.HashType].sSaveAsExt);
        if (!result.empty() && result.back() == L'.') result.pop_back();
        else if (ofn.nFileExtension == 0 || result.substr(ofn.nFileExtension) != extension.substr(1))
            result += extension;
        if (GetFileAttributesW(ChecksumPaths::IoPath(result).c_str()) == INVALID_FILE_ATTRIBUTES) return TRUE;
        // Keep the localized overwrite question and present the complete wide path.
        const std::wstring pattern = ChecksumPaths::Wide(LoadStr(IDS_SAVE_OVERWRITE));
        const size_t token = pattern.find(L"%s");
        const std::wstring question = token == std::wstring::npos ? pattern :
            pattern.substr(0, token) + result + pattern.substr(token + 2);
        const std::string questionUtf8 = ChecksumPaths::Utf8(question.c_str());
        const int answer = SalamanderGeneral->SalMessageBox(HWindow, questionUtf8.c_str(),
            LoadStr(IDS_SAVE_TITLE), MB_YESNOCANCEL | MB_ICONQUESTION);
        if (answer == IDYES) return TRUE;
        if (answer == IDCANCEL) return FALSE;
    }
}

void CCalculateDialog::SaveHashes()
try
{
    CALL_STACK_MESSAGE1("CCalculateDialog::SaveHashes()");

    std::wstring filename;
    if (GetSaveFileName(filename, LoadStr(IDS_SAVE_TITLE)))
    {
        FILE* f;
        if ((f = _wfopen(ChecksumPaths::IoPath(filename).c_str(), L"w")) == NULL)
        {
            Error(HWindow, GetLastError(), IDS_SAVE_TITLE, IDS_ERRORCREATINGFILE);
            return;
        }

        std::unique_ptr<FILE, int (*)(FILE*)> output(f, fclose);
        /*if (sfv)*/ fprintf(f, "; Generated by Open Salamander, https://www.altap.cz\n;\n"); // why not promote ourselves...
        BOOL warn = FALSE;
        int colInd = 2;
        // Determine column index
        int j;
        for (j = 0; j < HT_COUNT; j++)
        {
            if (Config.HashType == HashInfo[j].Type)
                break;
            if (HashInfo[j].bCalculate)
                colInd++;
        }
        int i;
        for (i = 0; i < ListView_GetItemCount(hList); i++)
        {
            std::string entry = FileList[i]->Name;
            // Relative entries remain relative to the manifest itself. If the
            // user saves elsewhere, absolute entries preserve the same sources.
            if (CompareStringOrdinal(ChecksumPaths::Parent(filename).c_str(), -1,
                ChecksumPaths::Wide(SourcePath).c_str(), -1, TRUE) != CSTR_EQUAL)
                entry = FileList[i]->FullPath;
            std::vector<char> nameStorage(entry.begin(), entry.end());
            nameStorage.push_back('\0');
            char* name = nameStorage.data();
            char hash[HASH_MAX_SIZE];
            GetItemText(i, colInd, hash, SizeOf(hash));
            if (!hash[0] || !strcmp(hash, LoadStr(IDS_CANCELED)) || !strcmp(hash, LoadStr(IDS_SKIPPED)))
            { // Skip canceled / skipped files with empty hash/CRC
                warn = TRUE;
                continue;
            }
            _strlwr(hash);
            if (Config.HashType != HT_CRC)
                ConvertPath(name, '\\', '/');
            // CRC precedes filename, but hashes follow filename
            // Unix-based md5sum has this format:
            //  <128bit checksum><space><binary-flag><filename>
            //  where <binary-flag> is either <space> (text mode default) or * (binary mode -b option).
            //  -> We write 2 spaces
            fprintf(f, "%s  %s\n", (Config.HashType == HT_CRC) ? name : hash, (Config.HashType == HT_CRC) ? hash : name);
        }

        output.reset();

        // notify a change on the path (our file was added)
        const std::string changedDirectory = ChecksumPaths::Utf8(ChecksumPaths::Parent(filename).c_str());
        SalamanderGeneral->PostChangeOnPathNotification(changedDirectory.c_str(), FALSE);

        if (warn)
            SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_SKIPPEDFILES),
                                             LoadStr(IDS_SAVE_TITLE), MB_ICONINFORMATION);
    }
}

catch (const std::bad_alloc&) { Error(HWindow, 0, IDS_SAVE_TITLE, IDS_OUTOFMEM); }

void CCalculateDialog::OnContextMenu(int x, int y, eHASH_TYPE forceCopyHash)
{
    int focIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (focIndex == -1)
        return;

    CGUIMenuPopupAbstract* popup = SalamanderGUI->CreateMenuPopup();
    if (popup == NULL)
        return;

    char checksum[HASH_MAX_SIZE];
    LVITEM lvi;

    lvi.mask = LVIF_TEXT;
    lvi.iItem = focIndex;
    lvi.pszText = checksum;
    lvi.cchTextMax = SizeOf(checksum);

    // A better approach would loading a menu template from the language file
    MENU_ITEM_INFO mii;
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ID;
    mii.Type = MENU_TYPE_STRING;

    int forcedHashIndex = -1;

    /* used by the export_mnu.py script, which generates salmenu.mnu for Translator
   keep in sync with the InsertItem() calls below...
MENU_TEMPLATE_ITEM CalculateDialogMenu[] = 
{
  {MNTT_PB, 0
  {MNTT_IT, IDS_COPYTOCBOARD_CRC
  {MNTT_IT, IDS_COPYTOCBOARD_MD5
  {MNTT_IT, IDS_COPYTOCBOARD_SHA1
  {MNTT_IT, IDS_COPYTOCBOARD_SHA256
  {MNTT_IT, IDS_COPYTOCBOARD_SHA512
  {MNTT_IT, IDS_COPYALLTOCBOARD
  {MNTT_IT, IDS_REMOVEITEM
  {MNTT_PE, 0
};
*/

    int i, j;
    for (i = 0, j = 0; i < HT_COUNT; i++)
    {
        if (HashInfo[i].bCalculate)
        {
            lvi.iSubItem = 2 + j;
            ListView_GetItem(hList, &lvi);
            if (checksum[0] && strcmp(checksum, LoadStr(IDS_CANCELED)) && strcmp(checksum, LoadStr(IDS_SKIPPED)))
            { // Check if the values have been calculated
                mii.ID = ++j;
                mii.String = LoadStr(HashInfo[i].idContextMenu);
                popup->InsertItem(-1, TRUE, &mii);
                if (forceCopyHash != HT_COUNT && forceCopyHash == i)
                    forcedHashIndex = mii.ID;
            }
        }
    }
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ID;
    mii.Type = MENU_TYPE_STRING;
    mii.String = LoadStr(IDS_COPYALLTOCBOARD);
    mii.ID = CM_COPYALLVALUES;
    popup->InsertItem(-1, TRUE, &mii);

    mii.Type = MENU_TYPE_SEPARATOR;
    popup->InsertItem(-1, TRUE, &mii);

    mii.Mask = MENU_MASK_TYPE | MENU_MASK_STRING | MENU_MASK_ID | MENU_MASK_STATE;
    mii.Type = MENU_TYPE_STRING;
    mii.String = LoadStr(IDS_REMOVEITEM);
    mii.ID = CM_REMOVEITEM;
    mii.State = bThreadRunning ? MENU_STATE_GRAYED : 0;
    popup->InsertItem(-1, TRUE, &mii);

    int id = 0;
    if (forceCopyHash == HT_COUNT)
        id = popup->Track(MENU_TRACK_RETURNCMD | MENU_TRACK_RIGHTBUTTON, x, y, HWindow, NULL);
    else
        id = forcedHashIndex;
    if (id == CM_REMOVEITEM)
    {
        DeleteItem(focIndex);
    }
    else if (id == CM_COPYALLVALUES)
    {
        std::string rowText;
        if (focIndex >= 0 && focIndex < FileList.Count)
        {
            char num[64];
            SalamanderGeneral->NumberToStr(num, FileList[focIndex]->Size);

            rowText = LoadStr(IDS_COLUMN_FILE);
            rowText += ": ";
            rowText += FileList[focIndex]->Name != NULL ? FileList[focIndex]->Name : "";
            rowText += "\r\n";

            rowText += LoadStr(IDS_COLUMN_SIZE);
            rowText += ": ";
            rowText += num;

            for (int hashIndex = 0; hashIndex < HT_COUNT; hashIndex++)
            {
                if (HashInfo[hashIndex].bCalculate)
                {
                    rowText += "\r\n";
                    rowText += LoadStr(HashInfo[hashIndex].idColumnHeader);
                    rowText += ": ";
                    if (FileList[focIndex]->Hashes[hashIndex] != NULL)
                        rowText += FileList[focIndex]->Hashes[hashIndex];
                }
            }
        }
        SalamanderGeneral->CopyTextToClipboard(rowText.c_str(), -1, FALSE, NULL);
    }
    else if ((id >= 1) && (id <= HT_COUNT))
    {
        lvi.iSubItem = 2 + id - 1;
        ListView_GetItem(hList, &lvi);
        SalamanderGeneral->CopyTextToClipboard(checksum, -1, FALSE, NULL);
    }

    SalamanderGUI->DestroyMenuPopup(popup);
}

void GetListViewContextMenuPos(HWND hListView, POINT* p)
{
    if (ListView_GetItemCount(hListView) == 0)
    {
        p->x = 0;
        p->y = 0;
        ClientToScreen(hListView, p);
        return;
    }
    int focIndex = ListView_GetNextItem(hListView, -1, LVNI_FOCUSED);
    if (focIndex != -1)
    {
        if ((ListView_GetItemState(hListView, focIndex, LVNI_SELECTED) & LVNI_SELECTED) == 0)
            focIndex = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    }
    RECT cr;
    GetClientRect(hListView, &cr);
    RECT r;
    ListView_GetItemRect(hListView, 0, &r, LVIR_LABEL);
    p->x = r.left;
    if (p->x < 0)
        p->x = 0;
    if (focIndex != -1)
        ListView_GetItemRect(hListView, focIndex, &r, LVIR_BOUNDS);
    if (focIndex == -1 || r.bottom < 0 || r.bottom > cr.bottom)
        r.bottom = 0;
    p->y = r.bottom;
    ClientToScreen(hListView, p);
}

INT_PTR CCalculateDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE_NONE // frequently called function
        //CALL_STACK_MESSAGE4("CCalculateDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
        static int id[] = {IDC_BUTTON_CONFIGURE, IDC_BUTTON_SAVE, IDC_BUTTON_CLOSE, IDC_LABEL, IDC_PROGRESS, IDC_LABEL_HINT};
    static int columns[] = {IDS_COLUMN_FILE, IDS_COLUMN_SIZE};

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(HWindow, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_FILE4)));
        InitList(columns, Config.CalcDlgWidths + 2, SizeOf(columns), HashInfo);
        InitLayout(id, SizeOf(id), 3);
        SetWindowPos(HWindow, NULL, 0, 0, Config.CalcDlgWidths[0], Config.CalcDlgWidths[1], SWP_NOZORDER | SWP_NOMOVE);
        SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);

        EnableButtons(FALSE);

        ModelessQueue.Add(new CWindowQueueItem(HWindow));

        HWND hProgress = GetDlgItem(HWindow, IDC_PROGRESS);
        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 1024));
        if (SalIsWindowsVersionOrGreater(6, 0, 0)) // WindowsVistaAndLater: Vista and later (CommonControls 6.0+)
        {
            LONG_PTR style = GetWindowLongPtr(hProgress, GWL_STYLE);
#ifndef PBS_MARQUEE
#define PBS_MARQUEE 0x08
#endif
#ifndef PBM_SETMARQUEE
#define PBM_SETMARQUEE (WM_USER + 10)
#endif
            SetWindowLongPtr(hProgress, GWL_STYLE, style | PBS_MARQUEE);
            SendMessage(hProgress, PBM_SETMARQUEE, TRUE, 50);
        }

        CGUIHyperLinkAbstract* hl;
        hl = SalamanderGUI->AttachHyperLink(HWindow, IDC_LABEL_HINT, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_HINT));
        ShowWindow(GetDlgItem(HWindow, IDC_LABEL_HINT), SW_HIDE);

        PostMessage(HWindow, WM_USER_STARTWORK, 0, 0);
        break;
    }

    case WM_USER_STARTWORK:
    {
        ReadingDirectories = TRUE;
        if (!GetFileList())
        {
            EndDialog(HWindow, 0);
            ReadingDirectories = FALSE;
            break;
        }
        ReadingDirectories = FALSE;

        HWND hProgress = GetDlgItem(HWindow, IDC_PROGRESS);
        if (SalIsWindowsVersionOrGreater(6, 0, 0)) // WindowsVistaAndLater: Vista and later
        {
            SendMessage(hProgress, PBM_SETMARQUEE, FALSE, 0);
            LONG_PTR style = GetWindowLongPtr(hProgress, GWL_STYLE);
            SetWindowLongPtr(hProgress, GWL_STYLE, style & ~PBS_MARQUEE);
        }
        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 1024));
        if (ApplyChecksumDarkModeIfSelected(HWindow))
            RedrawWindow(hProgress, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);

        BOOL startThread = FALSE;
        for (int i = 0; i < HT_COUNT; i++)
        {
            if (HashInfo[i].bCalculate)
            {
                startThread = TRUE;
            }
        }

        if (startThread) // if there is nothing to compute, skip starting the thread
        {
            bTerminateThread = FALSE;
            hThread = NULL;
            iThreadID = 0;
            bThreadRunning = TRUE;
            CCalculateThread* pThread = new CCalculateThread(this, &bTerminateThread);
            if (pThread == NULL || (hThread = pThread->Create(ThreadQueue, 0, &iThreadID)) == NULL)
            {
                TRACE_E("CCalculateDialog::DialogProc(): Failed to create worker thread.");
                if (pThread != NULL)
                    delete pThread; // on failure the thread object needs to be deallocated
                bThreadRunning = FALSE;
            }
        }
        else
        {
            PostMessage(HWindow, WM_USER_ENDWORK, 0, 0);
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_CLOSE:
        case IDCANCEL:
        {
            if (bThreadRunning)
            {
                bTerminateThread = TRUE;
            }
            else
            {
                if (ReadingDirectories)
                    StopReadingDirectories = TRUE;
                else
                    EndDialog(HWindow, 0);
            }
            return TRUE; // we do not want the base dialog proc
        }

        case IDC_BUTTON_SAVE:
            SaveHashes();
            break;

        case IDC_BUTTON_CONFIGURE:
            if (IDOK == OnConfiguration(HWindow))
                SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_CONFIG_CHANGES_EFFECT),
                                                 LoadStr(IDS_PLUGINNAME), MB_ICONINFORMATION | MB_OK);
            break;
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (wParam == IDC_LIST_FILES)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case LVN_GETDISPINFO:
            case LVN_GETDISPINFOW:
            {
                NMLVDISPINFO* plvdi = (NMLVDISPINFO*)nmh;
                int index = plvdi->item.iItem;
                if (index < 0 || index >= FileList.Count) // while the worker thread runs, the array is not modified
                    break;                                // array size does not change = no synchronization
                if (plvdi->item.mask & LVIF_IMAGE)
                {
                    // ScrollIndex nor the icons before it are modified by the thread, no synchronization needed
                    if (bThreadRunning && index >= ScrollIndex)
                        plvdi->item.iImage = 0;
                    else
                        plvdi->item.iImage = FileList[index]->IconIndex;
                }
                if (plvdi->item.mask & LVIF_TEXT)
                {
                    int col = plvdi->item.iSubItem;
                    switch (col)
                    {
                    case 0:
                    { // Name: once added to the array it never changes = access is not synchronized
                        SetDispInfoText(plvdi, FileList[index]->Name);
                        break;
                    }

                    case 1:
                    { // Size: once added to the array it never changes = access is not synchronized
                        {
                            char num[64];
                            SalamanderGeneral->NumberToStr(num, FileList[index]->Size);
                            SetDispInfoText(plvdi, num);
                        }
                        break;
                    }

                    default:
                    {
                        // ScrollIndex nor hashes before it are modified by the thread, no synchronization needed
                        if (bThreadRunning && index >= ScrollIndex)
                        {
                            // while the worker thread runs: calculated data are acknowledged only up to ScrollIndex,
                            // ScrollIndex itself is calculating / verifying, items after it remain unprocessed (even if they might already be done)
                            if (index == ScrollIndex && col == 2)
                                SetDispInfoText(plvdi, LoadStr(IDS_CALCULATING));
                            else
                                ClearDispInfoText(plvdi);
                        }
                        else
                        {
                            if (col >= 2 && col - 2 < HT_COUNT && FileList[index]->Hashes[col - 2] != NULL)
                                SetDispInfoText(plvdi, FileList[index]->Hashes[col - 2]);
                        }
                        break;
                    }
                    }
                }
                break;
            }

            case NM_RCLICK:
            {
                DWORD pos = GetMessagePos();
                OnContextMenu(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
                break;
            }

            case LVN_KEYDOWN:
            {
                LPNMLVKEYDOWN nmhk = (LPNMLVKEYDOWN)nmh;
                BOOL controlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                BOOL shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                if (shiftPressed && nmhk->wVKey == VK_F10 || nmhk->wVKey == VK_APPS)
                {
                    POINT p;
                    GetListViewContextMenuPos(hList, &p);
                    OnContextMenu(p.x, p.y);
                }
                WORD wVKey = ((LPNMLVKEYDOWN)lParam)->wVKey;
                if (!bThreadRunning && wVKey == VK_DELETE)
                {
                    DeleteItem(ListView_GetSelectionMark(hList));
                }
                if (controlPressed)
                {
                    eHASH_TYPE hashType = HT_COUNT;
                    switch (wVKey)
                    {
                    case 'C':
                        hashType = HT_CRC;
                        break;
                    case 'M':
                        hashType = HT_MD5;
                        break;
                    case '1':
                        hashType = HT_SHA1;
                        break;
                    case '2':
                        hashType = HT_SHA256;
                        break;
                    case '5':
                        hashType = HT_SHA512;
                        break;
                    }
                    if (hashType != HT_COUNT)
                        OnContextMenu(0, 0, hashType);
                }
            }
            }
        }
        break;
    }

    case WM_SIZE:
    {
        RecalcLayout(LOWORD(lParam), HIWORD(lParam), id, SizeOf(id), 3);
        break;
    }

    case WM_DESTROY:
    {
        SaveSizes(Config.CalcDlgWidths, SizeOf(columns), HashInfo);
        break;
    }
    }
    return CSFVMD5Dialog::DialogProc(uMsg, wParam, lParam);
} /* CCalculateDialog::DialogProc */

// ****************************************************************************************************
//
//  CVerifyDialog
//

CVerifyDialog::CVerifyDialog(HWND parent, BOOL alwaysOnTop, const char* path, const char* file)
    : CSFVMD5Dialog(IDD_VERIFY, parent, alwaysOnTop), fileList(100, 100, dtDelete)
{
    CALL_STACK_MESSAGE1("CVerifyDialog::CVerifyDialog(, , )");
    sourcePath = path;
    sourceFile = file;
}

void CVerifyDialog::LTrimStr(char* str)
{
    CALL_STACK_MESSAGE_NONE // frequently called function
        // CALL_STACK_MESSAGE1("CVerifyDialog::LTrimStr()");
        int i = 0;
    while (str[i] && ((BYTE)str[i] <= ' '))
        i++;
    if (i)
        memmove(str, str + i, (int)strlen(str + i) + 1);
}

/*char* CVerifyDialog::GetLine(FILE* f, char* buffer, int max)
{
  CALL_STACK_MESSAGE2("CVerifyDialog::GetLine(, , %d)", max);
  char* ret = fgets(buffer, max-1, f);
  int len = strlen(buffer);
  if (buffer[len-1] == '\n') buffer[len-1] = 0;
  return ret;
}*/

char* CVerifyDialog::LoadFile(const char* name)
{
    CALL_STACK_MESSAGE2("CVerifyDialog::LoadFile(%s)", name);
    HANDLE file = CreateFileW(ChecksumPaths::IoPath(name).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        Error(HWindow, GetLastError(), IDS_VERIFYTITLE, IDS_ERROROPENING2, name);
        return NULL;
    }
    CQuadWord fsize;
    DWORD err;
    if (SalamanderGeneral->SalGetFileSize(file, fsize, err) && fsize.Value < 500 * 1024 * 1024) // 500 MB should really be enough
    {
#ifdef new
#undef new
#define CHECKSUM_RESTORE_LOAD_NEW
#endif
        char* text = new (std::nothrow) char[fsize.LoDWord + 1];
#ifdef CHECKSUM_RESTORE_LOAD_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_LOAD_NEW
#endif
        if (text == NULL)
        {
            CloseHandle(file);
            Error(HWindow, 0, IDS_VERIFYTITLE, IDS_OUTOFMEM);
            return NULL;
        }
        DWORD numr;
        if (!SafeReadFile(file, text, fsize.LoDWord, &numr, name, HWindow))
        {
            delete[] text;
            CloseHandle(file);
            return NULL;
        }
        CloseHandle(file);
        text[fsize.LoDWord] = 0;
        return text;
    }
    else
    {
        CloseHandle(file);
        Error(HWindow, err, IDS_VERIFYTITLE, IDS_ERROROPENING2, name);
        return NULL;
    }
}

BOOL CVerifyDialog::AnalyzeSourceFile()
{
    CALL_STACK_MESSAGE1("CVerifyDialog::AnalyzeSourceFile()");

    char* text = LoadFile(sourceFile);
    if (text == NULL)
        return FALSE;
    char* line = strtok(text, "\r\n");

    BOOL isSFV = TRUE, isMD5 = TRUE, isSHA1 = TRUE, isSHA256 = TRUE, isSHA512 = TRUE;
    while (line != NULL)
    {
        LTrimStr(line);
        // Patera 2006.08.17: # used by MD5summer (http://www.md5summer.org) for comment lines
        if (line[0] && (line[0] != ';') && (line[0] != '#'))
        {
            // WARNING: the following code must match CCRCAlgo::ParseDigest() !!!
            // checksum at the end (after ' ')

            int posLast, lenLast;
            GetLastWord(line, posLast, lenLast);
            BOOL lastIsHex = IsHex(line + posLast, lenLast);

            if (isSFV &&
                (lenLast != 8 || !lastIsHex))
            {                  // does not end with a checksum
                isSFV = FALSE; // not an SFV
            }

            // WARNING: the following code must match CGenericHashAlgo::ParseDigest() !!!
            // checksum at the beginning (before ' ') or checksum at the end (after ' ' or '=') and at the same time
            // the hash name at the beginning (before '(' or ' ')

            int posFirst, lenFirst;
            GetFirstWord(line, posFirst, lenFirst);
            BOOL firstIsHex = IsHex(line + posFirst, lenFirst);
            int posFirstHashName, lenFirstHashName;
            GetFirstWord(line, posFirstHashName, lenFirstHashName, '(');
            GetLastWord(line, posLast, lenLast, '='); // '=' is the delimiter for the rest of the hash
            lastIsHex = IsHex(line + posLast, lenLast);

            if (isMD5 &&
                (lenFirst != 32 || !firstIsHex) &&
                (lenLast != 32 || !lastIsHex || lenFirstHashName != 3 || memcmp(line + posFirstHashName, "MD5", 3) != 0))
            {                  // does not start with a checksum, nor is it: MD5 (apache_2.0.46-win32-x86-symbols.zip) = eb5ba72b4164d765a79a7e06cee4eead
                isMD5 = FALSE; // not an MD5
            }
            if (isSHA1 &&
                (lenFirst != 40 || !firstIsHex) &&
                (lenLast != 40 || !lastIsHex || lenFirstHashName != 4 || memcmp(line + posFirstHashName, "SHA1", 4) != 0))
            {                   // does not start with a checksum, nor is it: SHA1 (openldap-2.2.25.tgz) = a983e039486b8495819e69260a8fad1fb9c77520
                isSHA1 = FALSE; // not a SHA1
            }
            if (isSHA256 &&
                (lenFirst != 64 || !firstIsHex) &&
                (lenLast != 64 || !lastIsHex || lenFirstHashName != 6 || memcmp(line + posFirstHashName, "SHA256", 6) != 0))
            {                     // does not start with a checksum, nor is it: SHA256 (README) = baaa5da257f848a4eece4fcf7653a7a58930124ef244bda374a6e906207d8a73
                isSHA256 = FALSE; // not a SHA256
            }
            if (isSHA512 &&
                (lenFirst != 128 || !firstIsHex) &&
                (lenLast != 128 || !lastIsHex || lenFirstHashName != 6 || memcmp(line + posFirstHashName, "SHA512", 6) != 0))
            {                     // does not start with a checksum, nor is it: SHA512 (README) = baaa5da257f848a4eece4fcf7653a7a58930124ef244bda374a6e906207d8a73baaa5da257f848a4eece4fcf7653a7a58930124ef244bda374a6e906207d8a73
                isSHA512 = FALSE; // not a SHA512
            }
        }
        line = strtok(NULL, "\r\n");
    }

    delete[] text;
    if (!isMD5 && !isSFV && !isSHA1 && !isSHA256 && !isSHA512)
        return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_BADFILE);

    eHASH_TYPE HashType = isSFV ? HT_CRC : (isMD5 ? HT_MD5 : (isSHA1 ? HT_SHA1 : (isSHA256 ? HT_SHA256 : HT_SHA512)));
    for (int i = 0; i < HT_COUNT; i++)
        if (HashType == Config.HashInfo[i].Type)
        {
            pHashInfo = &Config.HashInfo[i];
            break;
        }

    return TRUE;
}

BOOL CVerifyDialog::LoadSourceFile()
try
{
    std::unique_ptr<char[]> text(LoadFile(sourceFile));
    if (!text) return FALSE;
    std::unique_ptr<CHashAlgo> calculator(pHashInfo->Factory());
    if (!calculator) return FALSE;
    std::vector<char> parsed(4 * SAL_MAX_PATH, '\0');
    char* line = strtok(text.get(), "\r\n");
    totalSize.Value = 0;
    BOOL result = TRUE, ignoreAll = FALSE;
    while (result && line != NULL)
    {
        LTrimStr(line);
        if (*line && *line != ';' && *line != '#')
        {
#ifdef new
#undef new
#define CHECKSUM_RESTORE_INFO_NEW
#endif
            std::unique_ptr<FILEINFO> info(new (std::nothrow) FILEINFO());
#ifdef CHECKSUM_RESTORE_INFO_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_INFO_NEW
#endif
            if (!info) return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_OUTOFMEM);
            info->size.Set(0, 0); info->bFileExist = FALSE;
            parsed[0] = '\0';
            if (!calculator->ParseDigest(line, parsed.data(), static_cast<int>(parsed.size()), info->digest))
                return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_TOOLONGNAME);
            ConvertPath(parsed.data(), '/', '\\');
            std::string relative = parsed.data();
            if (relative.empty() && line == text.get())
            {
                relative = ChecksumPaths::Utf8(ChecksumPaths::Base(ChecksumPaths::Wide(sourceFile)).c_str());
                const size_t dot = relative.find_last_of('.');
                if (dot != std::string::npos && !_stricmp(relative.c_str() + dot, pHashInfo->sSaveAsExt))
                    relative.resize(dot);
                else relative.clear();
            }
            if (relative.empty()) return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_BADFILE);
            info->fileName = ChecksumPaths::Resolve(sourcePath, relative.c_str());
            WIN32_FIND_DATAW fd;
            HANDLE find = HANDLES_Q(FindFirstFileW(ChecksumPaths::IoPath(info->fileName.c_str()).c_str(), &fd));
            info->bFileExist = find != INVALID_HANDLE_VALUE;
            if (info->bFileExist)
            {
                HANDLES(FindClose(find));
                info->size.Set(fd.nFileSizeLow, fd.nFileSizeHigh);
                BOOL cancel = FALSE;
                CQuadWord linkSize;
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                    (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
                    SalamanderGeneral->GetLinkTgtFileSize(HWindow, info->fileName.c_str(), &linkSize, &cancel, &ignoreAll))
                    info->size = linkSize;
                if (cancel) { result = FALSE; break; }
            }
            if (!AddFileListItem(relative.c_str(), info->size, info->bFileExist, info->fileName.c_str()))
                return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_OUTOFMEM);
            if (info->bFileExist) totalSize += info->size + CQuadWord(FILE_SIZE_FIX, 0);
            fileList.Add(info.get());
            if (!fileList.IsGood()) { fileList.ResetState(); return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_OUTOFMEM); }
            info.release();
        }
        line = strtok(NULL, "\r\n");
    }
    ListView_SetItemCountEx(hList, FileList.Count, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    if (result && FileList.Count > 0)
    {
        bDisableNotification = TRUE;
        ListView_SetItemState(hList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        bDisableNotification = FALSE;
    }
    return result;
}
catch (const std::bad_alloc&) { return Error(HWindow, 0, IDS_VERIFYTITLE, IDS_OUTOFMEM); }

class CVerifyThread : public CCRCMD5Thread
{
public:
    CVerifyThread(CVerifyDialog* dlg, BOOL* terminate) : CCRCMD5Thread(terminate) { dialog = dlg; }
    virtual unsigned Body();

protected:
    CVerifyDialog* dialog;
};

unsigned CVerifyThread::Body()
{
    CALL_STACK_MESSAGE1("CVerifyThread::Body()");
    TRACE_I("Begin");

    CHashAlgo* pCalculator = dialog->pHashInfo->Factory();

    if (NULL == pCalculator)
    {
        TRACE_E("CVerifyThread::Body(): Could not instantiate calculator");
        TRACE_I("End");
        if (dialog->fileList.Count > 0)
            dialog->SetItemTextAndIcon(0, 2, LoadStr(IDS_CANCELED));
        dialog->bCanceled = TRUE;
        PostMessage(dialog->HWindow, WM_USER_ENDWORK, 0, 0);
        return 0;
    }

    BOOL skip;

    // while the worker thread runs, the array is not modified (item count + indices
    // do not change = no need to synchronize access)
    for (int i = 0, silent = 0; i < dialog->fileList.Count && !*Terminate; i++)
    {
        FILEINFO* info = dialog->fileList[i];

        // scroll to the current item
        dialog->ScrollToItem(i);

        if (!info->bFileExist)
        {
            dialog->SetItemTextAndIcon(i, 0, NULL, 1);
            dialog->nMissing++; // used only from the thread + from the main thread when it is not running -> no sync
            continue;
        }

        // open the file
        HANDLE hFile;
        if (!SafeOpenCreateFile(info->fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,
                                &hFile, &skip, &silent, dialog->HWindow))
        {
            dialog->SetItemTextAndIcon(i, 2, LoadStr(IDS_CANCELED));
            dialog->bCanceled = TRUE;
            break;
        }
        if (skip)
        {
            dialog->SetItemTextAndIcon(i, 2, LoadStr(IDS_SKIPPED));
            // advance progress by the size of the skipped file
            dialog->IncreaseProgress(info->size + CQuadWord(FILE_SIZE_FIX, 0));
            dialog->nSkipped++; // used only from the thread + from the main thread when it is not running -> no sync
            continue;
        }

        // compute CRC or MD5
        pCalculator->Init();
        DWORD nr;
        do
        {
            char buffer[BUFSIZE];
            if (!SafeReadFile(hFile, buffer, BUFSIZE, &nr, info->fileName.c_str(), dialog->HWindow))
            {
                dialog->bCanceled = TRUE;
                *Terminate = TRUE;
            }
            if (!*Terminate)
            {
                dialog->IncreaseProgress(CQuadWord(nr, 0));
                pCalculator->Update(buffer, nr);
            }
        } while (nr == BUFSIZE && !*Terminate);
        if (!*Terminate)
        {
            pCalculator->Finalize();
            dialog->IncreaseProgress(CQuadWord(FILE_SIZE_FIX, 0));
        }
        CloseHandle(hFile);

        // store the results into the list
        if (!*Terminate)
        {
            char digest[DIGEST_MAX_SIZE];
            int len = pCalculator->GetDigest(digest, SizeOf(digest));
            BOOL ok = (len > 0) && !memcmp(info->digest, digest, len);

            dialog->SetItemTextAndIcon(i, 2, LoadStr(ok ? IDS_OK : IDS_CORRUPT), ok ? 3 : 2);
            if (!ok)
                dialog->nCorrupt++; // used only from the thread + from the main thread when it is not running -> no sync
        }
        else
            dialog->SetItemTextAndIcon(i, 2, LoadStr(IDS_CANCELED));
    }

    delete pCalculator;
    TRACE_I("End");
    PostMessage(dialog->HWindow, WM_USER_ENDWORK, 0, 0);
    return 0;
}

void CVerifyDialog::OnThreadEnd()
{
    CALL_STACK_MESSAGE1("CVerifyDialog::OnThreadEnd()");
    ShowWindow(GetDlgItem(HWindow, IDC_LABEL_RESULT), SW_SHOW);
    char text[200];
    if (bCanceled)
        strcpy(text, LoadStr(IDS_CANCELED));
    else
    {
        if (!nMissing && !nSkipped && !nCorrupt)
        {
            if (fileList.Count)                   // while the worker thread runs, the array is not modified
                strcpy(text, LoadStr(IDS_ALLOK)); // number of items does not change = no synchronization needed
            else
                strcpy(text, LoadStr(IDS_NOFILES));
        }
        else
        {
            sprintf(text, LoadStr(IDS_RESULT), nCorrupt, nMissing, nSkipped);
        }
    }
    SetDlgItemText(HWindow, IDC_LABEL_RESULT, text);
    CSFVMD5Dialog::OnThreadEnd();
}

INT_PTR CVerifyDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CVerifyDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    static int id[] = {IDC_BUTTON_FOCUS, IDC_BUTTON_CLOSE, IDC_LABEL, IDC_PROGRESS, IDC_LABEL_RESULT};
    static int columns[] = {IDS_COLUMN_FILE, IDS_COLUMN_SIZE, IDS_COLUMN_STATUS};

    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(HWindow, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_CHECKMARK)));
        InitList(columns, Config.VerDlgWidths + 2, SizeOf(columns));
        InitLayout(id, SizeOf(id), 2);
        SetWindowPos(HWindow, NULL, 0, 0, Config.VerDlgWidths[0], Config.VerDlgWidths[1], SWP_NOZORDER | SWP_NOMOVE);
        SalamanderGeneral->MultiMonCenterWindow(HWindow, hParent, TRUE);
        SendMessage(GetDlgItem(HWindow, IDC_PROGRESS), PBM_SETRANGE, 0, MAKELPARAM(0, 1024));
        ShowWindow(GetDlgItem(HWindow, IDC_LABEL_RESULT), SW_HIDE);
        nMissing = nCorrupt = nSkipped = 0;
        bCanceled = FALSE;
        ModelessQueue.Add(new CWindowQueueItem(HWindow));
        SetWindowText(HWindow, LoadStr(IDS_VERIFYTITLE)); // provisional title (avoid empty caption if an error pops up)

        PostMessage(HWindow, WM_USER_STARTWORK, 0, 0);
        break;
    }

    case WM_USER_STARTWORK:
    {
        if (!AnalyzeSourceFile() || !LoadSourceFile())
        {
            EndDialog(HWindow, 0);
        }
        else
        {
            SetWindowText(HWindow, LoadStr(pHashInfo->idVerifyTitle));
            bTerminateThread = FALSE;
            hThread = NULL;
            iThreadID = 0;
            bThreadRunning = TRUE;
            CVerifyThread* pThread = new CVerifyThread(this, &bTerminateThread);
            if (pThread == NULL || (hThread = pThread->Create(ThreadQueue, 0, &iThreadID)) == NULL)
            {
                TRACE_E("CVerifyDialog::DialogProc(): Failed to create worker thread.");
                if (pThread)
                    delete pThread; // on failure the thread object needs to be deallocated
                pThread = NULL;
                bThreadRunning = FALSE;
            }
        }
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDCANCEL:
        case IDC_BUTTON_CLOSE:
        {
            bCanceled = TRUE;
            if (bThreadRunning)
                bTerminateThread = TRUE;
            else
                EndDialog(HWindow, 0);
            return TRUE; // we do not want the base dialog proc
        }

        case IDC_BUTTON_FOCUS:
        {
            int i = -1;
            i = ListView_GetNextItem(hList, i, LVNI_SELECTED);
            // while the worker thread runs, the array is not modified (item count + indices
            // do not change = no need to synchronize access)
            if (i < 0 || i >= fileList.Count || !fileList[i]->bFileExist)
                break;

            if (SalamanderGeneral->SalamanderIsNotBusy(NULL))
            {
                Focus_Path = fileList[i]->fileName;
                SalamanderGeneral->PostMenuExtCommand(CMD_FOCUSFILE, TRUE);
                Sleep(500);        // switching to another window happens, so this Sleep should not hurt anything
                Focus_Path.clear(); // after 0.5 seconds we no longer want the focus (handles hitting the start of Salamander's BUSY mode)
            }
            else
                SalamanderGeneral->SalMessageBox(HWindow, LoadStr(IDS_BUSY), LoadStr(IDS_VERIFYTITLE), MB_ICONINFORMATION);

            break;
        }
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (wParam == IDC_LIST_FILES)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case LVN_GETDISPINFO:
            case LVN_GETDISPINFOW:
            {
                NMLVDISPINFO* plvdi = (NMLVDISPINFO*)nmh;
                int index = plvdi->item.iItem;
                // while the worker thread runs, the array is not modified (item count + indices
                // do not change = no need to synchronize access)
                if (index < 0 || index >= FileList.Count)
                    break;
                if (plvdi->item.mask & LVIF_IMAGE)
                {
                    // ScrollIndex nor the icons before it are modified by the thread, no synchronization needed
                    if (bThreadRunning && index >= ScrollIndex)
                        plvdi->item.iImage = 0;
                    else
                        plvdi->item.iImage = FileList[index]->IconIndex;
                }
                if (plvdi->item.mask & LVIF_TEXT)
                {
                    int col = plvdi->item.iSubItem;
                    switch (col)
                    {
                    case 0: // Name: once added to the array it never changes = no synchronization needed
                    {
                        SetDispInfoText(plvdi, FileList[index]->Name);
                        break;
                    }

                    case 1: // FileExist+Size: once added to the array it never changes = no synchronization needed
                    {
                        if (FileList[index]->FileExist)
                        {
                            char num[64];
                            SalamanderGeneral->NumberToStr(num, FileList[index]->Size);
                            SetDispInfoText(plvdi, num);
                        }
                        else
                            ClearDispInfoText(plvdi);
                        break;
                    }

                    case 2:
                    {
                        if (FileList[index]->FileExist)
                        {
                            // ScrollIndex nor hashes before it are modified by the thread, no synchronization needed
                            if (bThreadRunning && index >= ScrollIndex)
                            {
                                // while the worker thread runs: calculated data are acknowledged only up to ScrollIndex,
                                // ScrollIndex itself is calculating / verifying, items after it remain unprocessed (even if they might already be done)
                                if (index == ScrollIndex)
                                    SetDispInfoText(plvdi, LoadStr(IDS_VERIFYING));
                                else
                                    ClearDispInfoText(plvdi);
                            }
                            else
                            {
                                if (FileList[index]->Hashes[0] != NULL)
                                    SetDispInfoText(plvdi, FileList[index]->Hashes[0]);
                                else
                                    ClearDispInfoText(plvdi);
                            }
                        }
                        else
                        {
                            SetDispInfoText(plvdi, LoadStr(IDS_MISSING));
                        }
                        break;
                    }
                    }
                }
                break;
            }

            case LVN_ITEMCHANGED:
            {
                int i = -1;
                i = ListView_GetNextItem(hList, i, LVNI_SELECTED);
                // while the worker thread runs, the array is not modified (item count + indices
                // do not change = no need to synchronize access)
                if (i >= 0 && i < fileList.Count)
                {
                    EnableWindow(GetDlgItem(HWindow, IDC_BUTTON_FOCUS), fileList[i]->bFileExist);
                }
                break;
            }

            case NM_DBLCLK:
            {
                SendMessage(HWindow, WM_COMMAND, IDC_BUTTON_FOCUS, 0);
                break;
            }
            }
        }
        break;
    }

    case WM_SIZE:
    {
        RecalcLayout(LOWORD(lParam), HIWORD(lParam), id, SizeOf(id), 2);
        break;
    }

    case WM_DESTROY:
    {
        SaveSizes(Config.VerDlgWidths, SizeOf(columns));
        break;
    }
    }
    return CSFVMD5Dialog::DialogProc(uMsg, wParam, lParam);
} /* CVerifyDialog::DialogProc */

// ****************************************************************************************************
//
//  CCalculateDialogThread
//

class CCalculateDialogThread : public CThread
{
public:
    CCalculateDialogThread(HWND parent, BOOL alwaysOnTop, TSeedFileList* pFileList, const char* sourcePath) : CThread("Calculate SFV/MD5 Dialog")
    {
        hParent = parent;
        pSeedFileList = pFileList;
        SourcePath = sourcePath;
        bAlwaysOnTop = alwaysOnTop;
    }

    ~CCalculateDialogThread() { delete pSeedFileList; }
    virtual unsigned Body();

private:
    HWND hParent;
    BOOL bAlwaysOnTop;
    TSeedFileList* pSeedFileList;
    std::string SourcePath;
};

unsigned CCalculateDialogThread::Body()
{

    CALL_STACK_MESSAGE1("CCalculateDialogThread::Body()");
    TRACE_I("Begin");

    CCalculateDialog dlg(hParent, bAlwaysOnTop, pSeedFileList, SourcePath.c_str());
    dlg.Execute();


    TRACE_I("End");
    return 0;
}

BOOL OpenCalculateDialog(HWND parent)
try
{
    CSalamanderDiskSelection selection;
    if (!selection.Capture(SalamanderGeneral, PANEL_SOURCE, SALDISKSELECTION_SELECTED_OR_FOCUSED) ||
        selection.GetCount() == 0) return FALSE;
#ifdef new
#undef new
#define CHECKSUM_RESTORE_CAPTURE_NEW
#endif
    std::unique_ptr<TSeedFileList> files(new (std::nothrow) TSeedFileList(100, 100, dtDelete));
#ifdef CHECKSUM_RESTORE_CAPTURE_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_CAPTURE_NEW
#endif
    if (!files) return FALSE;
    for (int i = 0; i < selection.GetCount(); ++i)
    {
        const CSalamanderDiskSelectionItem* item = selection.GetItem(i);
#ifdef new
#undef new
#define CHECKSUM_RESTORE_SEED_NEW
#endif
        std::unique_ptr<SEEDFILEINFO> seed(new (std::nothrow) SEEDFILEINFO());
#ifdef CHECKSUM_RESTORE_SEED_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_SEED_NEW
#endif
        if (!seed) return FALSE;
        seed->Name = ChecksumPaths::Utf8(item->RelativePathW);
        seed->FullPath = ChecksumPaths::Utf8(item->FullPathW);
        seed->bDir = item->IsDir != FALSE; seed->Attr = item->Attr; seed->Size = item->Size;
        files->Add(seed.get());
        if (!files->IsGood()) { files->ResetState(); return FALSE; }
        seed.release();
    }
    const std::string source = ChecksumPaths::Utf8(selection.GetRootPathW());
    BOOL alwaysOnTop = FALSE;
    SalamanderGeneral->GetConfigParameter(SALCFG_ALWAYSONTOP, &alwaysOnTop, sizeof(alwaysOnTop), NULL);
    RefreshChecksumDarkModeFromHost(); ConfigureChecksumDarkModeFromHost();
#ifdef new
#undef new
#define CHECKSUM_RESTORE_THREAD_NEW
#endif
    std::unique_ptr<CCalculateDialogThread> thread(new (std::nothrow) CCalculateDialogThread(parent, alwaysOnTop, files.get(), source.c_str()));
#ifdef CHECKSUM_RESTORE_THREAD_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_THREAD_NEW
#endif
    if (!thread) return FALSE;
    files.release();
    if (thread->Create(ThreadQueue) == NULL) return FALSE;
    thread.release(); return TRUE;
}
catch (const std::bad_alloc&) { return Error(parent, 0, IDS_PLUGINNAME, IDS_OUTOFMEM); }

// ****************************************************************************************************
//
//  CVerifyDialogThread
//

class CVerifyDialogThread : public CThread
{
public:
    CVerifyDialogThread(HWND parent, BOOL alwaysOnTop) : CThread("Verify SFV/MD5 Dialog")
    {
        hParent = parent;
        bAlwaysOnTop = alwaysOnTop;
    }

    virtual unsigned Body();

    std::string sourcePath;
    std::string sourceFile;

private:
    HWND hParent;
    BOOL bAlwaysOnTop;
};

unsigned CVerifyDialogThread::Body()
{
    CALL_STACK_MESSAGE1("CVerifyDialogThread::Body()");
    TRACE_I("Begin");

    CVerifyDialog dlg(hParent, bAlwaysOnTop, sourcePath.c_str(), sourceFile.c_str());
    dlg.Execute();

    TRACE_I("End");
    return 0;
}

BOOL OpenVerifyDialog(HWND parent)
try
{
    CSalamanderDiskSelection selection;
    if (!selection.Capture(SalamanderGeneral, PANEL_SOURCE, SALDISKSELECTION_FOCUSED_ONLY) ||
        selection.GetCount() != 1 || selection.GetItem(0)->IsDir) return FALSE;
    const CSalamanderDiskSelectionItem* item = selection.GetItem(0);
    bool known = false;
    for (int i = 0; i < HT_COUNT; ++i)
        if (CompareStringOrdinal(item->ExtensionW, -1,
            ChecksumPaths::Wide(Config.HashInfo[i].sSaveAsExt + 1).c_str(), -1, TRUE) == CSTR_EQUAL) known = true;
    if (!known && SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_BADEXT), LoadStr(IDS_VERIFYTITLE),
        MSGBOXEX_YESNO | MSGBOXEX_ICONQUESTION | MSGBOXEX_DEFBUTTON2 | MSGBOXEX_ESCAPEENABLED) == IDNO) return FALSE;
    BOOL alwaysOnTop = FALSE;
    SalamanderGeneral->GetConfigParameter(SALCFG_ALWAYSONTOP, &alwaysOnTop, sizeof(alwaysOnTop), NULL);
    RefreshChecksumDarkModeFromHost(); ConfigureChecksumDarkModeFromHost();
#ifdef new
#undef new
#define CHECKSUM_RESTORE_VERIFY_NEW
#endif
    std::unique_ptr<CVerifyDialogThread> thread(new (std::nothrow) CVerifyDialogThread(parent, alwaysOnTop));
#ifdef CHECKSUM_RESTORE_VERIFY_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef CHECKSUM_RESTORE_VERIFY_NEW
#endif
    if (!thread) return FALSE;
    thread->sourcePath = ChecksumPaths::Utf8(item->DirectoryW);
    thread->sourceFile = ChecksumPaths::Utf8(item->FullPathW);
    if (thread->Create(ThreadQueue) == NULL) return FALSE;
    thread.release(); return TRUE;
}
catch (const std::bad_alloc&) { return Error(parent, 0, IDS_VERIFYTITLE, IDS_OUTOFMEM); }

//****************************************************************************
//
// CCommonDialog - Centered dialog
//

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, HWND hParent, CObjectOrigin origin)
    : CDialog(hInstance, resID, hParent, origin)
{
}

CCommonDialog::CCommonDialog(HINSTANCE hInstance, int resID, int helpID, HWND hParent, CObjectOrigin origin)
    : CDialog(hInstance, resID, helpID, hParent, origin)
{
}

INT_PTR
CCommonDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        ApplyChecksumDarkMode(HWindow);
        // Center horizontally & vertically to parent
        if (Parent != NULL)
            SalamanderGeneral->MultiMonCenterWindow(HWindow, Parent, TRUE);
        break; // Need focus from DefDlgProc
    }

    case WM_THEMECHANGED:
    {
        ApplyChecksumDarkMode(HWindow);
        RedrawWindow(HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        return TRUE;
    }

    case WM_SETTINGCHANGE:
    {
        ConfigureChecksumDarkModeFromHost();
        if (DarkModeHandleSettingChange(uMsg, lParam))
        {
            ApplyChecksumDarkMode(HWindow);
            InvalidateRect(HWindow, NULL, TRUE);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSCROLLBAR:
    {
        INT_PTR result = 0;
        if (HandleChecksumDarkCtlColor(uMsg, wParam, lParam, &result))
            return result;
        break;
    }
    } // switch
    return CDialog::DialogProc(uMsg, wParam, lParam);
}

//****************************************************************************
//
// CConfigurationDialog
//

CConfigurationDialog::CConfigurationDialog(HWND hParent)
    : CCommonDialog(HLanguage, IDD_CONFIGURATION, IDD_CONFIGURATION, hParent)
{
}

void CConfigurationDialog::Transfer(CTransferInfo& ti)
{
    int i;
    for (i = 0; i < HT_COUNT; i++)
        ti.CheckBox(IDC_CFG_SUM_1 + i, Config.HashInfo[i].bCalculate);
}

/*BOOL
CConfigurationDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_COMMAND:
    {
      break;
    }

  } // switch
  return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}*/
