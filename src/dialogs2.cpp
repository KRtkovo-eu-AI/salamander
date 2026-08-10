// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "cfgdlg.h"
#include "dialogs.h"
#include "configstorage.h"
#include "usermenu.h"
#include "execute.h"
#include "plugins.h"
#include "fileswnd.h"
#include "mainwnd.h"
#include "gui.h"
#include "shellib.h"
#include "reglib\src\regparse.h"

//****************************************************************************
//
// CViewerMasksItem
//

// this number keeps growing - is used as a source for unique IDs
DWORD ViewerHandlerID = 0;

CViewerMasksItem::CViewerMasksItem(const char* masks, const char* command, const char* arguments, const char* initDir,
                                   int viewerType, BOOL oldType)
{
    CALL_STACK_MESSAGE7("CViewerMasksItem(%s, %s, %s, %s, %d, %d)",
                        masks, command, arguments, initDir, viewerType, oldType);
    OldType = oldType;
    Masks = NULL;
    Command = Arguments = InitDir = ViewerLabel = NULL;
    ViewerType = viewerType;
    HandlerID = ViewerHandlerID++;
    Set(masks, command, arguments, initDir);
    SetViewerLabel("");
}

CViewerMasksItem::CViewerMasksItem()
{
    CALL_STACK_MESSAGE1("CViewerMasksItem()");
    Masks = NULL;
    Command = Arguments = InitDir = ViewerLabel = NULL;
    ViewerType = VIEWER_EXTERNAL;
    HandlerID = ViewerHandlerID++;
    OldType = FALSE;
    Set("", "", "\"$(Name)\"", "$(FullPath)");
    SetViewerLabel("");
}

CViewerMasksItem::CViewerMasksItem(CViewerMasksItem& item)
{
    CALL_STACK_MESSAGE1("CViewerMasksItem(&)");
    Masks = NULL;
    Command = Arguments = InitDir = ViewerLabel = NULL;
    ViewerType = item.ViewerType;
    OldType = item.OldType;
    HandlerID = item.HandlerID;
    Set(item.Masks->GetMasksString(), item.Command, item.Arguments, item.InitDir);
    SetViewerLabel(item.ViewerLabel);
}

CViewerMasksItem::~CViewerMasksItem()
{
    if (Masks != NULL)
        delete Masks;
    if (Command != NULL)
        free(Command);
    if (Arguments != NULL)
        free(Arguments);
    if (InitDir != NULL)
        free(InitDir);
    if (ViewerLabel != NULL)
        free(ViewerLabel);
}

BOOL CViewerMasksItem::IsGood()
{
    return Masks != NULL && Command != NULL && Arguments != NULL && InitDir != NULL && ViewerLabel != NULL;
}

BOOL CViewerMasksItem::SetViewerLabel(const char* viewerLabel)
{
    const char* value = viewerLabel != NULL ? viewerLabel : "";
    char* label = (char*)malloc(strlen(value) + 1);
    if (label == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }
    strcpy(label, value);
    if (ViewerLabel != NULL)
        free(ViewerLabel);
    ViewerLabel = label;
    return TRUE;
}

BOOL CViewerMasksItem::Set(const char* masks, const char* command, const char* arguments, const char* initDir)
{
    CALL_STACK_MESSAGE5("CViewerMasksItem::Set(%s, %s, %s, %s)", masks, command, arguments, initDir);

    char* commandName = (char*)malloc(strlen(command) + 1);
    char* argumentsName = (char*)malloc(strlen(arguments) + 1);
    char* initDirName = (char*)malloc(strlen(initDir) + 1);

    if (Masks == NULL)
        Masks = new CMaskGroup;
    if (Masks == NULL || commandName == NULL || argumentsName == NULL || initDirName == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }

    Masks->SetMasksString(masks);
    strcpy(commandName, command);
    strcpy(argumentsName, arguments);
    strcpy(initDirName, initDir);

    if (Command != NULL)
        free(Command);
    if (Arguments != NULL)
        free(Arguments);
    if (InitDir != NULL)
        free(InitDir);

    Command = commandName;
    Arguments = argumentsName;
    InitDir = initDirName;

    return TRUE;
}

BOOL CViewerMasks::Load(CViewerMasks& source)
{
    CALL_STACK_MESSAGE1("CViewerMasks::Load()");
    CViewerMasksItem* item;
    DestroyMembers();
    int i;
    for (i = 0; i < source.Count; i++)
    {
        item = new CViewerMasksItem(*source[i]);
        if (!item->IsGood())
        {
            delete item;
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
        Add(item);
        if (!IsGood())
        {
            delete item;
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    return TRUE;
}

//****************************************************************************
//
// CEditorMasksItem
//

// this number keeps growing - is used as a source for unique IDs
DWORD EditorHandlerID = 0;

CEditorMasksItem::CEditorMasksItem(char* masks, char* command, char* arguments, char* initDir)
{
    CALL_STACK_MESSAGE5("CEditorMasksItem(%s, %s, %s, %s)", masks, command, arguments, initDir);
    Masks = new CMaskGroup;
    Command = Arguments = InitDir = NULL;
    HandlerID = EditorHandlerID++;
    Set(masks, command, arguments, initDir);
}

CEditorMasksItem::CEditorMasksItem()
{
    CALL_STACK_MESSAGE1("CEditorMasksItem()");
    Masks = new CMaskGroup;
    Command = Arguments = InitDir = NULL;
    HandlerID = EditorHandlerID++;
    Set("", "", "\"$(Name)\"", "$(FullPath)");
}

CEditorMasksItem::CEditorMasksItem(CEditorMasksItem& item)
{
    CALL_STACK_MESSAGE1("CEditorMasksItem(&)");
    Masks = new CMaskGroup;
    Command = Arguments = InitDir = NULL;
    HandlerID = item.HandlerID;
    Set(item.Masks->GetMasksString(), item.Command, item.Arguments, item.InitDir);
}

CEditorMasksItem::~CEditorMasksItem()
{
    if (Masks != NULL)
        delete Masks;
    if (Command != NULL)
        free(Command);
    if (Arguments != NULL)
        free(Arguments);
    if (InitDir != NULL)
        free(InitDir);
}

BOOL CEditorMasksItem::Set(const char* masks, const char* command, const char* arguments, const char* initDir)
{
    CALL_STACK_MESSAGE5("CEditorMasksItem::Set(%s, %s, %s, %s)", masks, command, arguments, initDir);
    char* commandName = (char*)malloc(strlen(command) + 1);
    char* argumentsName = (char*)malloc(strlen(arguments) + 1);
    char* initDirName = (char*)malloc(strlen(initDir) + 1);
    if (commandName == NULL || argumentsName == NULL || initDirName == NULL)
    {
        TRACE_E(LOW_MEMORY);
        return FALSE;
    }
    if (Masks != NULL)
        Masks->SetMasksString(masks);
    strcpy(commandName, command);
    strcpy(argumentsName, arguments);
    strcpy(initDirName, initDir);

    if (Command != NULL)
        free(Command);
    if (Arguments != NULL)
        free(Arguments);
    if (InitDir != NULL)
        free(InitDir);

    Command = commandName;
    Arguments = argumentsName;
    InitDir = initDirName;
    return TRUE;
}

BOOL CEditorMasksItem::IsGood()
{
    return Masks != NULL && Command != NULL && Arguments != NULL && InitDir != NULL;
}

BOOL CEditorMasks::Load(CEditorMasks& source)
{
    CALL_STACK_MESSAGE1("CEditorMasks::Load()");
    CEditorMasksItem* item;
    DestroyMembers();
    int i;
    for (i = 0; i < source.Count; i++)
    {
        item = new CEditorMasksItem(*source[i]);
        if (!item->IsGood())
        {
            delete item;
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
        Add(item);
        if (!IsGood())
        {
            delete item;
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    return TRUE;
}

//
// ****************************************************************************
// CCommonDialog
//

void CCommonDialog::NotifDlgJustCreated()
{
    ArrangeHorizontalLines(HWindow);
}

INT_PTR
CCommonDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // prevent panels from refreshing on the background of modal dialogs or messageboxes
        if (Modal && MainWindow != NULL && Parent != NULL && Parent == MainWindow->HWindow)
        {
            BeginStopRefresh(FALSE, TRUE); // the sniffer takes a break
            CallEndStopRefresh = TRUE;
        }
        else
            CallEndStopRefresh = FALSE;

        // when opening the dialog set the plug-ins' msgbox parent to this dialog (main thread only)
        if (Modal && MainThreadID == GetCurrentThreadId())
        {
            HOldPluginMsgBoxParent = PluginMsgBoxParent;
            PluginMsgBoxParent = HWindow;
        }

        HWND hCenterBy;
        if (HCenterAgains != NULL)
            hCenterBy = HCenterAgains;
        else
            hCenterBy = Parent;

        if (MainWindow != NULL)
            hCenterBy = MainWindow->GetDetachedAwareDialogParent(hCenterBy);

        if (hCenterBy != NULL)
            MultiMonCenterWindow(HWindow, hCenterBy, TRUE);
        else
            MultiMonCenterWindow(HWindow, NULL, FALSE);

        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeRefreshTitleBar(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }

        break;
    }

        /* j.r.: the VK_ESCAPE variant seems better because clicking IDCANCEL does not set a variable
    case WM_COMMAND:
    {
      if (LOWORD(wParam) == IDCANCEL) // measure to avoid interrupting panel listing after each ESC
        WaitForESCReleaseBeforeTestingESC = TRUE;
      break;
    }
    */

    case WM_DESTROY:
    {
        if (GetKeyState(VK_ESCAPE) & 0x8000) // measure to avoid interrupting panel listing after each ESC
            WaitForESCReleaseBeforeTestingESC = TRUE;

        // the dialog is closing - the user might have changed the clipboard
        // (for example pasted text from an editline), so we'll verify it
        IdleRefreshStates = TRUE;  // force the state variables check during the next Idle
        IdleCheckClipboard = TRUE; // also let the clipboard be checked

        // when closing the dialog restore the msgbox parent for plug-ins
        if (HOldPluginMsgBoxParent != NULL)
            PluginMsgBoxParent = HOldPluginMsgBoxParent;

        if (CallEndStopRefresh)
        {
            EndStopRefresh(TRUE, FALSE, TRUE); // the sniffer will start again now
            CallEndStopRefresh = FALSE;
        }
        break;
    }

    case WM_THEMECHANGED:
    {
        if (WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeRefreshTitleBar(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        return TRUE;
    }

    case WM_SETTINGCHANGE:
    {
        if (DarkModeHandleSettingChange(uMsg, lParam) &&
            WinLib_DarkMode_ShouldApplyDialogTree(HWindow))
        {
            DarkModeApplyTree(HWindow);
            DarkModeRefreshTitleBar(HWindow);
            DarkModeApplyStaticTextColors(HWindow, NULL);
            WinLib_DarkMode_PostDeferredRedraw(HWindow);
        }
        return TRUE;
    }

    case WM_USER_COMMONDLG_DARKMODE_REDRAW:
    {
        RedrawWindow(HWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
        return TRUE;
    }

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORSCROLLBAR:
    {
        LRESULT brush = 0;
        if (DarkModeHandleCtlColor(uMsg, wParam, lParam, brush))
            return brush;
        break;
    }
    }

    return CDialog::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CCommonPropSheetPage
//

void CCommonPropSheetPage::NotifDlgJustCreated()
{
    ArrangeHorizontalLines(HWindow);
}

//
// ****************************************************************************
// CSizeResultsDlg
//

CSizeResultsDlg::CSizeResultsDlg(HWND parent, const CQuadWord& size, const CQuadWord& compressed,
                                 const CQuadWord& occupied, int files, int dirs, TDirectArray<CQuadWord>* sizes)
    : CCommonDialog(HLanguage, IDD_SIZERESULTS, IDD_SIZERESULTS, parent)
{
    Size = size;
    Compressed = compressed;
    Occupied = occupied;
    Files = files;
    Dirs = dirs;
    Sizes = sizes;
}

void CSizeResultsDlg::UpdateEstimate()
{
    char buf[100];
    SendDlgItemMessage(HWindow, IDC_EST_CLUSTER, WM_GETTEXT, 11, (LPARAM)buf);
    int bytesPerCluster = atoi(buf);

    if (Sizes != NULL && Sizes->IsGood() && bytesPerCluster > 0)
    {
        if (Sizes->Count != Files)
            TRACE_E("Sizes array is not consistent with number of files.");

        CQuadWord estimated(0, 0);
        CQuadWord s;
        int i;
        for (i = 0; i < Sizes->Count; i++)
        {
            s = Sizes->At(i);
            estimated += s - ((s - CQuadWord(1, 0)) % CQuadWord(bytesPerCluster, 0)) +
                         CQuadWord(bytesPerCluster - 1, 0);
        }

        SetWindowText(GetDlgItem(HWindow, IDC_EST_SIZE), PrintDiskSize(buf, estimated, 1));

        if (estimated == CQuadWord(0, 0))
            strcpy(buf, "0 %");
        else
        {
            sprintf(buf, "%-1.4lg %%", 100 * Size.GetDouble() / estimated.GetDouble());
            PointToLocalDecimalSeparator(buf, _countof(buf));
        }
        SetWindowText(GetDlgItem(HWindow, IDC_EST_UTIL), buf);

        EnableWindow(GetDlgItem(HWindow, IDC_EST_SIZE), TRUE);
        EnableWindow(GetDlgItem(HWindow, IDC_EST_UTIL), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(HWindow, IDC_EST_SIZE), FALSE);
        EnableWindow(GetDlgItem(HWindow, IDC_EST_UTIL), FALSE);
        SetWindowText(GetDlgItem(HWindow, IDC_EST_SIZE), UnknownText);
        SetWindowText(GetDlgItem(HWindow, IDC_EST_UTIL), UnknownText);
    }
}

INT_PTR
CSizeResultsDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CSizeResultsDlg::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        GetDlgItemText(HWindow, IDS_OCCUPIED, UnknownText, 100); // obtain the "unknown" string for later use

        char buf[100];

        SetWindowText(GetDlgItem(HWindow, IDS_FILESCOUNT), NumberToStr(buf, CQuadWord(Files, 0)));
        SetWindowText(GetDlgItem(HWindow, IDS_DIRSCOUNT), NumberToStr(buf, CQuadWord(Dirs, 0)));

        if (Occupied != CQuadWord(-1, -1))
        {
            SetWindowText(GetDlgItem(HWindow, IDS_OCCUPIED), PrintDiskSize(buf, Occupied, 1));
            if (Occupied == CQuadWord(0, 0))
                strcpy(buf, "0 %");
            else
            {
                double result = 100 * Size.GetDouble() / Occupied.GetDouble();
                // patch for a 2GB sparse file where 3.052e+006 % was shown instead of 3051757.83 %
                // for values above 1000, lg prints exponential form so we use lf
                // for smaller numbers lg is better because it prints 100 rather than 100.00
                if (result > 1000)
                    sprintf(buf, "%-1.2lf %%", result);
                else
                    sprintf(buf, "%-1.4lg %%", result);
                PointToLocalDecimalSeparator(buf, _countof(buf));
            }
            SetWindowText(GetDlgItem(HWindow, IDS_DISKUTILIZATION), buf);
        }
        else
        {
            EnableWindow(GetDlgItem(HWindow, IDS_OCCUPIED), FALSE);
            EnableWindow(GetDlgItem(HWindow, IDS_DISKUTILIZATION), FALSE);
        }

        SetWindowText(GetDlgItem(HWindow, IDS_SIZE), PrintDiskSize(buf, Size, 1));
        if (Compressed != CQuadWord(-1, -1))
        {
            SetWindowText(GetDlgItem(HWindow, IDS_COMPSIZE), PrintDiskSize(buf, Compressed, 1));
            if (Size == CQuadWord(0, 0))
            {
                strcpy(buf, "100 %");
            }
            else
            {
                sprintf(buf, "%-1.4lg %%", 100 * Compressed.GetDouble() / Size.GetDouble());
                PointToLocalDecimalSeparator(buf, _countof(buf));
            }
            SetWindowText(GetDlgItem(HWindow, IDS_COMPRATIO), buf);
        }
        else
        {
            EnableWindow(GetDlgItem(HWindow, IDS_COMPSIZE), FALSE);
            EnableWindow(GetDlgItem(HWindow, IDS_COMPRATIO), FALSE);
        }

        // fill the combobox

        DWORD clusterSize = 2048; // most likely used for CDs
        CFilesWindow* panel = MainWindow->GetNonActivePanel();
        if (panel->Is(ptDisk))
        {
            DWORD sectorsPerCluster, bytesPerSector, numberOfFreeClusters, totalNumberOfClusters;
            if (MyGetDiskFreeSpace(MainWindow->GetNonActivePanel()->GetPath(),
                                   &sectorsPerCluster, &bytesPerSector,
                                   &numberOfFreeClusters, &totalNumberOfClusters))
            {
                clusterSize = sectorsPerCluster * bytesPerSector;
            }
        }

        HWND hCombo = GetDlgItem(HWindow, IDC_EST_CLUSTER);
        SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
        SendMessage(hCombo, CB_LIMITTEXT, 11, 0);

        int selIndex = -1;
        DWORD arr[] = {512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, (DWORD)-1};
        int i;
        for (i = 0; arr[i] != -1; i++)
        {
            itoa(arr[i], buf, 10);
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)buf);
            if (clusterSize == arr[i])
                selIndex = i;
        }

        if (selIndex != -1)
            SendMessage(hCombo, CB_SETCURSEL, selIndex, 0);
        else
        {
            itoa(clusterSize, buf, 10);
            SendMessage(hCombo, WM_SETTEXT, 0, (LPARAM)buf);
        }

        if (Sizes == NULL || !Sizes->IsGood())
            EnableWindow(hCombo, FALSE);

        CheckDlgButton(HWindow, IDC_STAYONFILESYSTEM, Configuration.CountSizeStayOnFileSystem ? BST_CHECKED : BST_UNCHECKED);

        UpdateEstimate();

        break;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            PostMessage(HWindow, WM_COMMAND, MAKELPARAM(0, CBN_EDITCHANGE), 0);
        }
        if (HIWORD(wParam) == CBN_EDITCHANGE)
        {
            UpdateEstimate();
        }
        if (LOWORD(wParam) == IDOK)
        {
            Configuration.CountSizeStayOnFileSystem = IsDlgButtonChecked(HWindow, IDC_STAYONFILESYSTEM) == BST_CHECKED;
        }
        break;
    }
    }

    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//
// ****************************************************************************
// CSelectDialog
//

void CSelectDialog::Validate(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CSelectDialog::Validate()");
    HWND hWnd;
    if (ti.GetControl(hWnd, IDE_FILEMASK))
    {
        if (ti.Type == ttDataFromWindow)
        {
            char buf[MAX_PATH];
            strcpy(buf, Mask); // backup
            SendMessage(hWnd, WM_GETTEXT, MAX_PATH, (LPARAM)Mask);
            CMaskGroup mask(Mask);
            int errorPos;
            if (!mask.PrepareMasks(errorPos))
            {
                SalMessageBox(HWindow, LoadStr(IDS_INCORRECTSYNTAX), LoadStr(IDS_ERRORTITLE),
                              MB_OK | MB_ICONEXCLAMATION);
                SetFocus(hWnd);
                SendMessage(hWnd, CB_SETEDITSEL, 0, MAKELPARAM(errorPos, errorPos + 1));
                ti.ErrorOn(IDE_FILEMASK);
            }
            strcpy(Mask, buf); // restoration
        }
    }
}

void CSelectDialog::Transfer(CTransferInfo& ti)
{
    CALL_STACK_MESSAGE1("CSelectDialog::Transfer()");
    char** history = Configuration.SelectHistory;
    HWND hWnd;
    if (ti.GetControl(hWnd, IDE_FILEMASK))
    {
        if (ti.Type == ttDataToWindow)
        {
            LoadComboFromStdHistoryValues(hWnd, history, SELECT_HISTORY_SIZE);
            SendMessage(hWnd, CB_LIMITTEXT, MAX_PATH - 1, 0);
            SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)Mask);
        }
        else
        {
            SendMessage(hWnd, WM_GETTEXT, MAX_PATH, (LPARAM)Mask);
            AddValueToStdHistoryValues(history, SELECT_HISTORY_SIZE, Mask, FALSE);
        }
    }
}

INT_PTR
CSelectDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CSelectDialog::DialogProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        InstallWordBreakProc(GetDlgItem(HWindow, IDE_FILEMASK)); // install WordBreakProc to the combobox

        CHyperLink* hl = new CHyperLink(HWindow, IDC_FILEMASK_HINT, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_MASKS_HINT));

        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//****************************************************************************
//
// CImportConfigDialog
//

CImportConfigDialog::CImportConfigDialog()
    : CCommonDialog(HLanguage, IDD_IMPORTCONFIG, NULL)
{
    StorageType = cstRegistry;
    RegFilePath[0] = 0;
}

CImportConfigDialog::~CImportConfigDialog()
{
}

static BOOL BrowseRegStorageFile(HWND hParent, char* path, int pathSize)
{
    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    char filter[] = "Registration Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0\0";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hParent;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = path;
    ofn.nMaxFile = pathSize;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "reg";
    return SafeGetSaveFileName(&ofn);
}

static BOOL CanWriteRegStorageFilePath(const char* path)
{
    if (path == NULL || path[0] == 0)
        return FALSE;

    DWORD attrs = GetFileAttributes(path);
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        if ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0)
            return FALSE;

        HANDLE file = HANDLES_Q(CreateFile(path, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL, NULL));
        if (file == INVALID_HANDLE_VALUE)
            return FALSE;

        HANDLES(CloseHandle(file));
        return TRUE;
    }

    char tmpPath[SAL_MAX_PATH];
    _snprintf_s(tmpPath, _TRUNCATE, "%s.%lu.test", path, GetCurrentProcessId());
    HANDLE file = HANDLES_Q(CreateFile(tmpPath, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_TEMPORARY, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    HANDLES(CloseHandle(file));
    DeleteFile(tmpPath);
    return TRUE;
}

static void EnableImportStoragePathControls(HWND hWindow)
{
    BOOL enabled = IsDlgButtonChecked(hWindow, IDC_IMPORT_SAVE_TO_FILE) == BST_CHECKED &&
                   IsWindowEnabled(GetDlgItem(hWindow, IDC_IMPORT_SAVE_TO_FILE));
    EnableWindow(GetDlgItem(hWindow, IDC_IMPORT_SAVE_TO_FILE_PATH), enabled);
    EnableWindow(GetDlgItem(hWindow, IDC_IMPORT_SAVE_TO_FILE_BROWSE), enabled);
}

extern const char* SalamanderConfigurationVersions[SALCFG_ROOTS_COUNT];

void CImportConfigDialog::Transfer(CTransferInfo& ti)
{
    if (ti.Type == ttDataToWindow)
    {
        char buff[5000];
        char buff2[5000];

        // CAPTION: Welcome to %s
        GetWindowText(HWindow, buff, 5000);
        _snprintf_s(buff2, _TRUNCATE, buff, SALAMANDER_TEXT_VERSION);
        SetWindowText(HWindow, buff2);

        // COMBOBOX Import Configuration
        SendDlgItemMessage(HWindow, IDC_IMPORTCONFIG, CB_ADDSTRING, 0, (LPARAM)LoadStr(IDS_IMPORTCFG_DEFCFG));
        int selIndex = 0; // use the default item if nothing better is found
        int i;
        for (i = 0; i < SALCFG_ROOTS_COUNT; i++)
        {
            if (ConfigurationExist[i])
            {
                // detect whether this is "Open Salamander", "Altap Salamander", or the old "Servant Salamander"
                BOOL openSalamander = StrIStr(SalamanderConfigurationRoots[i], "Open Salamander") != NULL;
                BOOL altapSalamander = StrIStr(SalamanderConfigurationRoots[i], "Altap Salamander") != NULL;
                const char* name = openSalamander    ? "Open Salamander %s"
                                   : altapSalamander ? "Altap Salamander %s"
                                                     : "Servant Salamander %s";
                sprintf(buff, name, SalamanderConfigurationVersions[i]);
                SendDlgItemMessage(HWindow, IDC_IMPORTCONFIG, CB_ADDSTRING, 0, (LPARAM)buff);
                if (selIndex == 0)
                    selIndex = 1; // the last configuration becomes default
            }
        }
        if (selIndex == 0) // nothing to choose from, disable the combobox
        {
            EnableWindow(GetDlgItem(HWindow, IDC_IMPORTCONFIG), FALSE);
        }
        SendDlgItemMessage(HWindow, IDC_IMPORTCONFIG, CB_SETCURSEL, selIndex, NULL);
        BOOL canSaveStorageTypeBootstrap = ConfigurationStorage.CanSaveStorageTypeBootstrap();
        if (RegFilePath[0] == 0)
        {
            CConfigurationStorageType bootstrapType = (CConfigurationStorageType)StorageType;
            ConfigurationStorage.LoadStorageTypeBootstrap(bootstrapType, RegFilePath, SizeOf(RegFilePath));
        }
        if (RegFilePath[0] == 0)
            ConfigurationStorage.GetPortableConfigFilePath(RegFilePath, SizeOf(RegFilePath));
        SetDlgItemText(HWindow, IDC_IMPORT_SAVE_TO_FILE_PATH, RegFilePath);
        if (!canSaveStorageTypeBootstrap)
            StorageType = cstRegistry;
        CheckRadioButton(HWindow, IDC_IMPORT_SAVE_TO_REGISTRY, IDC_IMPORT_SAVE_TO_FILE,
                         StorageType == cstRegFile ? IDC_IMPORT_SAVE_TO_FILE : IDC_IMPORT_SAVE_TO_REGISTRY);
        EnableWindow(GetDlgItem(HWindow, IDC_IMPORT_SAVE_TO_REGISTRY), canSaveStorageTypeBootstrap);
        EnableWindow(GetDlgItem(HWindow, IDC_IMPORT_SAVE_TO_FILE), canSaveStorageTypeBootstrap);
        EnableImportStoragePathControls(HWindow);

        // LISTVIEW Remove Configuration
        HWND hListView = GetDlgItem(HWindow, IDC_REMOVECONFIG);
        selIndex = -1;
        int index = 0;
        for (i = 0; i < SALCFG_ROOTS_COUNT; i++)
        {
            if (ConfigurationExist[i])
            {
                LVITEM lvi;
                lvi.mask = LVIF_TEXT | LVIF_STATE;
                lvi.iItem = index;
                lvi.iSubItem = 0;
                lvi.state = 0;

                // detect whether this is "Open Salamander", "Altap Salamander", or the old "Servant Salamander"
                BOOL openSalamander = StrIStr(SalamanderConfigurationRoots[i], "Open Salamander") != NULL;
                BOOL altapSalamander = StrIStr(SalamanderConfigurationRoots[i], "Altap Salamander") != NULL;
                const char* name = openSalamander    ? "Open Salamander %s"
                                   : altapSalamander ? "Altap Salamander %s"
                                                     : "Servant Salamander %s";
                sprintf(buff, name, SalamanderConfigurationVersions[i]);
                lvi.pszText = buff;
                ListView_InsertItem(hListView, &lvi);
                index++;
                if (selIndex == -1)
                {
                    DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
                    ListView_SetItemState(hListView, 0, state, state);
                    selIndex = 0;
                }
            }
        }
    }
    else
    {
        GetDlgItemText(HWindow, IDC_IMPORT_SAVE_TO_FILE_PATH, RegFilePath, SizeOf(RegFilePath));
        StorageType = ConfigurationStorage.CanSaveStorageTypeBootstrap() && IsDlgButtonChecked(HWindow, IDC_IMPORT_SAVE_TO_FILE) ?
                          cstRegFile :
                          cstRegistry;

        // COMBOBOX Import Configuration
        int sel = (int)SendDlgItemMessage(HWindow, IDC_IMPORTCONFIG, CB_GETCURSEL, 0, NULL);
        if (sel > 0)
        {
            sel--; // the first item is Don't import
            int index = 0;
            int i;
            for (i = 0; i < SALCFG_ROOTS_COUNT; i++)
            {
                if (ConfigurationExist[i])
                {
                    if (sel == index)
                    {
                        IndexOfConfigurationToLoad = i;
                        break;
                    }
                    index++;
                }
            }
        }

        // LISTVIEW Remove Configuration
        HWND hListView = GetDlgItem(HWindow, IDC_REMOVECONFIG);
        int itemsCount = ListView_GetItemCount(hListView);
        int index = 0;
        int i;
        for (i = 0; i < SALCFG_ROOTS_COUNT; i++)
        {
            if (ConfigurationExist[i])
            {
                DWORD state = ListView_GetItemState(hListView, index, LVIS_STATEIMAGEMASK);
                if (state == INDEXTOSTATEIMAGEMASK(2))
                    DeleteConfigurations[i] = TRUE;
                index++;
            }
        }
    }
}

void CImportConfigDialog::Validate(CTransferInfo& ti)
{
    if (ConfigurationStorage.CanSaveStorageTypeBootstrap() && IsDlgButtonChecked(HWindow, IDC_IMPORT_SAVE_TO_FILE))
    {
        char path[SAL_MAX_PATH];
        GetDlgItemText(HWindow, IDC_IMPORT_SAVE_TO_FILE_PATH, path, SizeOf(path));
        if (path[0] == 0)
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEPATHERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDC_IMPORT_SAVE_TO_FILE_PATH);
            return;
        }
        if (!CanWriteRegStorageFilePath(path))
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDC_IMPORT_SAVE_TO_FILE_PATH);
            return;
        }
    }
}

INT_PTR
CImportConfigDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // under W2K when launched via a shortcut set to MAXIMIZED
        // the dialog appeared maximized; SC_RESTORE fixes it
        INT_PTR ret = CCommonDialog::DialogProc(uMsg, wParam, lParam);
        SendMessage(HWindow, WM_SYSCOMMAND, SC_RESTORE, 0);

        // checkboxes for the listview
        HWND hListView = GetDlgItem(HWindow, IDC_REMOVECONFIG);
        DWORD exFlags = LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES;
        DWORD origFlags = ListView_GetExtendedListViewStyle(hListView);
        ListView_SetExtendedListViewStyle(hListView, origFlags | exFlags); // 4.71

        // add the Name column to the listview with columns
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_FMT;
        char buff[] = "aa";
        lvc.pszText = buff;
        lvc.fmt = LVCFMT_LEFT;
        lvc.iSubItem = 0;
        ListView_InsertColumn(hListView, 0, &lvc);
        ListView_SetColumnWidth(hListView, 0, LVSCW_AUTOSIZE);

        return ret;
    }

    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED &&
            (LOWORD(wParam) == IDC_IMPORT_SAVE_TO_FILE || LOWORD(wParam) == IDC_IMPORT_SAVE_TO_REGISTRY))
            EnableImportStoragePathControls(HWindow);
        else if (LOWORD(wParam) == IDC_IMPORT_SAVE_TO_FILE_BROWSE)
        {
            char path[SAL_MAX_PATH];
            GetDlgItemText(HWindow, IDC_IMPORT_SAVE_TO_FILE_PATH, path, SizeOf(path));
            if (BrowseRegStorageFile(HWindow, path, SizeOf(path)))
            {
                if (CanWriteRegStorageFilePath(path))
                    SetDlgItemText(HWindow, IDC_IMPORT_SAVE_TO_FILE_PATH, path);
                else
                    SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                                  MB_OK | MB_ICONEXCLAMATION);
            }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//****************************************************************************
//
// CLanguageSelectorDialog
//

CLanguageSelectorDialog::CLanguageSelectorDialog(HWND hParent, char* slgName, const char* pluginName)
    : CCommonDialog(NULL, pluginName == NULL ? IDD_SLGSELECTOR : IDD_SLGSELECTORPLUG, hParent), Items(5, 5)
{
    Web = NULL;
    SLGName = slgName;
    OpenedFromConfiguration = hParent != NULL && pluginName == NULL;
    OpenedForPlugin = pluginName != NULL;
    HListView = NULL;
    PluginName = pluginName;
    ExitButtonLabel[0] = 0;
}

CLanguageSelectorDialog::~CLanguageSelectorDialog()
{
    int i;
    for (i = 0; i < Items.Count; i++)
        Items[i].Free();
}

int CLanguageSelectorDialog::Execute()
{
    HINSTANCE hTmpLanguage = NULL;
    if (OpenedFromConfiguration || OpenedForPlugin)
    {
        // use the template from the currently running language version
        Modul = HLanguage;
    }
    else
    {
        // load the template from the best available SLG
        int index = GetPreferredLanguageIndex(SLGName);
        char path[SAL_MAX_PATH];
        GetModuleFileName(HInstance, path, SAL_MAX_PATH);
        sprintf(strrchr(path, '\\') + 1, "lang\\%s", Items[index].FileName);
        hTmpLanguage = HANDLES(LoadLibrary(path));
        if (hTmpLanguage != NULL)
            Modul = hTmpLanguage;
    }
    if (!LoadString(Modul, IDS_SELLANGEXITBUTTON, ExitButtonLabel, 100))
        strcpy(ExitButtonLabel, "Exit");
    int ret = (int)CCommonDialog::Execute();
    if (hTmpLanguage != NULL)
    {
        Modul = NULL;
        HANDLES(FreeLibrary(hTmpLanguage));
    }

    return ret;
}

BOOL CLanguageSelectorDialog::GetSLGName(char* path, int index)
{
    if (index >= Items.Count)
        return FALSE;
    lstrcpy(path, Items[index].FileName);
    return TRUE;
}

BOOL CLanguageSelectorDialog::SLGNameExists(const char* slgName)
{
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        if (StrICmp(Items[i].FileName, slgName) == 0)
            return TRUE;
    }
    return FALSE;
}

void CLanguageSelectorDialog::FillControls()
{
    int index = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
    if (index != -1)
    {
        SetDlgItemTextW(HWindow, IDC_SLG_AUTHOR, Items[index].AuthorW);
        SetDlgItemText(HWindow, IDC_SLG_WEB, Items[index].Web);
        SetDlgItemTextW(HWindow, IDC_SLG_COMMENT, Items[index].CommentW);
        if (PluginName == NULL)
            SetDlgItemText(HWindow, IDC_SLG_HELPDIR, Items[index].HelpDir);
        if (Web != NULL)
        {
            char buff[300];
            sprintf(buff, "http://%s", Items[index].Web);
            Web->SetActionOpen(buff);
        }
    }
}

void CLanguageSelectorDialog::LoadListView()
{
    char buff[500];
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        LVITEM lvi;
        lvi.mask = 0;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        ListView_InsertItem(HListView, &lvi);

        Items[i].GetLanguageName(buff, 200);
        ListView_SetItemText(HListView, i, 0, buff);
        sprintf(buff, "lang\\%s", Items[i].FileName);
        ListView_SetItemText(HListView, i, 1, buff);
    }

    int preferredIndex = GetPreferredLanguageIndex(SLGName);
    DWORD state = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(HListView, preferredIndex, state, state);
    ListView_EnsureVisible(HListView, preferredIndex, FALSE);

    FillControls();
}

void CLanguageSelectorDialog::Transfer(CTransferInfo& ti)
{
    if (PluginName != NULL) // show this checkbox only when selecting an alternative language for a plug-in
        ti.CheckBox(IDC_USESAMESLGINOTHERPLUGINS, Configuration.UseAsAltSLGInOtherPlugins);

    if (ti.Type == ttDataToWindow)
    {
        LoadListView();

        // we do not want a horizontal scrollbar, so first fill items and only then set the column widths
        RECT r;
        GetClientRect(HListView, &r);
        ListView_SetColumnWidth(HListView, 0, r.right / 1.6);
        ListView_SetColumnWidth(HListView, 1, LVSCW_AUTOSIZE_USEHEADER);
    }
    else
    {
        int index = ListView_GetNextItem(HListView, -1, LVIS_FOCUSED);
        if (index != -1)
        {
            // Check if the selected language's code page matches the Windows system code page
            DWORD expectedCP = GetExpectedCodePageForLanguageID(Items[index].LanguageID);
            if (expectedCP != 0)
            {
                DWORD systemCP = GetACP();
                // Skip warning if system code page is UTF-8 (65001) - Windows 11 Unicode beta support works with all languages
                if (systemCP != 65001 && systemCP != expectedCP)
                {
                    char msg[500];
                    _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_CODEPAGE_WARNING), systemCP, expectedCP);
                    SalMessageBox(HWindow, msg, LoadStr(IDS_CODEPAGE_WARNING_TITLE),
                                  MB_OK | MB_ICONWARNING);
                }
            }
            lstrcpy(SLGName, Items[index].FileName);
            if (PluginName != NULL) // store the alternative language name only when selecting an alternative language for a plug-in
            {
                if (Configuration.UseAsAltSLGInOtherPlugins)
                    lstrcpy(Configuration.AltPluginSLGName, SLGName);
                else
                    Configuration.AltPluginSLGName[0] = 0;
            }
        }
    }
}

BOOL CLanguageSelectorDialog::Initialize(const char* slgSearchPath, HINSTANCE pluginDLL)
{
    char path[SAL_MAX_PATH];
    if (slgSearchPath == NULL)
    {
        GetModuleFileName(NULL, path, SAL_MAX_PATH);
        lstrcpy(strrchr(path, '\\') + 1, "lang\\*.slg");
    }
    else
        lstrcpyn(path, slgSearchPath, MAX_PATH);

    WIN32_FIND_DATA file;
    HANDLE hFind = HANDLES_Q(FindFirstFile(path, &file));
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            char* point = strrchr(file.cFileName, '.');
            if (point != NULL && stricmp(point + 1, "slg") == 0) // it was returning *.slg*
            {
                CLanguage lang;
                if (lang.Init(file.cFileName, pluginDLL))
                {
                    Items.Add(lang);
                    if (!Items.IsGood())
                    {
                        Items.ResetState();
                        lang.Free();
                        return FALSE;
                    }
                }
            }
        } while (FindNextFile(hFind, &file));
        HANDLES(FindClose(hFind));
    }
    return TRUE;
}

int CLanguageSelectorDialog::GetPreferredLanguageIndex(const char* selectSLGName, BOOL exactMatch)
{
    WORD langID = GetUserDefaultUILanguage();

    WORD primaryID = PRIMARYLANGID(langID);
    int localeIndex = -1;        // index corresponding to the user's locale
    int primarylocaleIndex = -1; // index corresponding to the user's primary language locale
    int englishIndex = -1;       // index of the file "english.slg"
    int i;
    for (i = 0; i < Items.Count; i++)
    {
        if (selectSLGName != NULL && stricmp(Items[i].FileName, selectSLGName) == 0)
            return i;
        if (localeIndex == -1 && Items[i].LanguageID == langID)
            localeIndex = i;
        if (primarylocaleIndex == -1 && PRIMARYLANGID(Items[i].LanguageID) == primaryID)
            primarylocaleIndex = i;
        if (stricmp(Items[i].FileName, "english.slg") == 0)
            englishIndex = i;
    }
    if (localeIndex == -1)
    {
        // if we didn't find a language exactly matching the user's settings
        if (primarylocaleIndex != -1)
        {
            // try to assign at least the primary language
            localeIndex = primarylocaleIndex;
        }
        else
        {
            if (!exactMatch)
            {
                if (englishIndex != -1)
                {
                    // if even that isn't found, prefer the English version
                    localeIndex = englishIndex;
                }
                else
                {
                    // otherwise take whichever one is available
                    localeIndex = 0;
                }
            }
        }
    }
    return localeIndex;
}

INT_PTR
CLanguageSelectorDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {

        // JRY: For AS 2.53 which ships with Czech, German and English we send other translations to the "Translations" section on the forum
        //     https://forum.altap.cz/viewforum.php?f=23 - in the hope that someone will be motivated to create a translation.

        // There is no download page for languages yet, so this button is disabled
        // EnableWindow(GetDlgItem(HWindow, IDB_GETMORELANGS), FALSE);

        if (!OpenedFromConfiguration && !OpenedForPlugin)
        {
            // put the program name in the title since this is the first window the user sees
            SetWindowText(HWindow, MAINWINDOW_NAME);
        }
        else
        {
            if (PluginName != NULL)
            {
                // put the plug-in name in the title so the user knows which plug-in the language is for
                char buf[200];
                _snprintf_s(buf, _TRUNCATE, "%s: ", PluginName);
                buf[99] = 0; // use only 100 characters for the plug-in name so some space remains for the original title dialog
                int len = (int)strlen(buf);
                if (GetWindowText(HWindow, buf + len, 200 - len))
                    SetWindowText(HWindow, buf);
            }
        }
        if (!OpenedFromConfiguration && PluginName == NULL) // turn the Cancel button into Exit
            SetDlgItemText(HWindow, IDCANCEL, ExitButtonLabel);
        if (PluginName != NULL) // disable closing
            EnableMenuItem(GetSystemMenu(HWindow, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

        Web = new CHyperLink(HWindow, IDC_SLG_WEB, STF_HYPERLINK_COLOR);

        HListView = GetDlgItem(HWindow, IDC_SLG_LIST);

        DWORD exFlags = LVS_EX_FULLROWSELECT;
        DWORD origFlags = ListView_GetExtendedListViewStyle(HListView);
        ListView_SetExtendedListViewStyle(HListView, origFlags | exFlags); // 4.71

        // add the Language and Path columns to the listview
        char buff[100];
        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_SUBITEM;
        lvc.pszText = buff;
        lvc.iSubItem = 0;
        GetDlgItemText(HWindow, IDC_SLG_DESCR, buff, 100);
        DestroyWindow(GetDlgItem(HWindow, IDC_SLG_DESCR));
        ListView_InsertColumn(HListView, 0, &lvc);

        lvc.iSubItem = 1;
        GetDlgItemText(HWindow, IDC_SLG_PATH, buff, 100);
        DestroyWindow(GetDlgItem(HWindow, IDC_SLG_PATH));
        ListView_InsertColumn(HListView, 1, &lvc);

        // under W2K when launched via a shortcut set to MAXIMIZED
        // the dialog appeared maximized; SC_RESTORE fixes it
        INT_PTR ret = CCommonDialog::DialogProc(uMsg, wParam, lParam);
        SendMessage(HWindow, WM_SYSCOMMAND, SC_RESTORE, 0);
        return ret;
    }

    case WM_COMMAND:
    {
        if (PluginName != NULL && LOWORD(wParam) == IDCANCEL)
            return 0;
        if (LOWORD(wParam) == IDB_GETMORELANGS)
            ShellExecute(HWindow, "open", "https://forum.altap.cz/viewforum.php?f=23", NULL, NULL, SW_SHOWNORMAL);
        if (LOWORD(wParam) == IDB_REFRESHLANGS)
        {
            ListView_DeleteAllItems(HListView);
            int i;
            for (i = 0; i < Items.Count; i++)
                Items[i].Free();
            Items.DestroyMembers();
            Initialize();
            if (GetLanguagesCount() == 0) // should not happen because this dialog is loaded from the .slg module (that .slg cannot be deleted)
            {
                MessageBox(HWindow, "Unable to find any language file (.SLG) in subdirectory LANG.\n"
                                    "Please reinstall Open Salamander.",
                           SALAMANDER_TEXT_VERSION, MB_OK | MB_ICONERROR);
                TRACE_E("CLanguageSelectorDialog: unexpected situation (no language file): calling ExitProcess(667).");
                //          ExitProcess(667);
                TerminateProcess(GetCurrentProcess(), 667); // harder exit (this call still performs some operations)
            }
            LoadListView();
        }
        break;
    }

    case WM_NOTIFY:
    {
        if (wParam == IDC_SLG_LIST)
        {
            LPNMHDR nmh = (LPNMHDR)lParam;
            switch (nmh->code)
            {
            case NM_DBLCLK:
            {
                LVHITTESTINFO ht;
                DWORD pos = GetMessagePos();
                ht.pt.x = GET_X_LPARAM(pos);
                ht.pt.y = GET_Y_LPARAM(pos);
                ScreenToClient(HListView, &ht.pt);
                ListView_HitTest(HListView, &ht);
                int index = ListView_GetNextItem(HListView, -1, LVNI_SELECTED);
                if (index != -1 && ht.iItem == index)
                {
                    PostMessage(HWindow, WM_COMMAND, MAKELPARAM(IDOK, BN_CLICKED),
                                (LPARAM)GetDlgItem(HWindow, IDOK));
                    return 0;
                }
                break;
            }

            case LVN_ITEMCHANGED:
            {
                FillControls();
                return 0;
            }
            }
        }
        break;
    }

    case WM_SYSCOLORCHANGE:
    {
        ListView_SetBkColor(HListView, GetSysColor(COLOR_WINDOW));
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//****************************************************************************
//
// CSkillLevelDialog
//

CSkillLevelDialog::CSkillLevelDialog(HWND hParent, int* level)
    : CCommonDialog(HLanguage, IDD_SKILLLEVEL, IDD_SKILLLEVEL, hParent)
{
    Level = level;
}

void CSkillLevelDialog::Transfer(CTransferInfo& ti)
{
    ti.RadioButton(IDC_SL_BEGINNER, SKILL_LEVEL_BEGINNER, *Level);
    ti.RadioButton(IDC_SL_INTERMEDIATE, SKILL_LEVEL_INTERMEDIATE, *Level);
    ti.RadioButton(IDC_SL_ADVANCED, SKILL_LEVEL_ADVANCED, *Level);
}

//****************************************************************************
//
// CCompareArgsDlg
//

CCompareArgsDlg::CCompareArgsDlg(HWND parent, BOOL comparingFiles, char* compareName1,
                                 char* compareName2, int* cnfrmShowNamesToCompare)
    : CCommonDialog(HLanguage, IDD_USERMENUCOMPAREARGS, comparingFiles ? IDH_USERMENUCOMPAREARGS_F : IDH_USERMENUCOMPAREARGS_D, parent)
{
    ComparingFiles = comparingFiles;
    CompareName1 = compareName1;
    CompareName2 = compareName2;
    CnfrmShowNamesToCompare = cnfrmShowNamesToCompare;
}

void CCompareArgsDlg::Validate(CTransferInfo& ti)
{
    char buf[MAX_PATH];
    ti.EditLine(IDE_UMC_NAME1, buf, MAX_PATH);
    if (buf[0] == 0)
    {
        SalMessageBox(HWindow, LoadStr(IDS_FF_EMPTYSTRING), LoadStr(IDS_ERRORTITLE),
                      MB_OK | MB_ICONEXCLAMATION);
        ti.ErrorOn(IDE_UMC_NAME1);
        return;
    }
    ti.EditLine(IDE_UMC_NAME2, buf, MAX_PATH);
    if (buf[0] == 0)
    {
        SalMessageBox(HWindow, LoadStr(IDS_FF_EMPTYSTRING), LoadStr(IDS_ERRORTITLE),
                      MB_OK | MB_ICONEXCLAMATION);
        ti.ErrorOn(IDE_UMC_NAME2);
        return;
    }
}

void CCompareArgsDlg::Transfer(CTransferInfo& ti)
{
    ti.EditLine(IDE_UMC_NAME1, CompareName1, MAX_PATH);
    ti.EditLine(IDE_UMC_NAME2, CompareName2, MAX_PATH);

    int c = !*CnfrmShowNamesToCompare;
    ti.CheckBox(IDC_UMC_SHOWTHISDLG, c);
    *CnfrmShowNamesToCompare = !c;
}

INT_PTR
CCompareArgsDlg::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        if (!ComparingFiles)
        {
            SetWindowText(HWindow, LoadStr(IDS_USERMENUCOMPAREARGSTITLE));
            SetDlgItemText(HWindow, IDT_UMC_NAME1, LoadStr(IDS_USERMENUCOMPAREARG1));
            SetDlgItemText(HWindow, IDT_UMC_NAME2, LoadStr(IDS_USERMENUCOMPAREARG2));
        }
        CHyperLink* hl = new CHyperLink(HWindow, IDT_UMC_HOWTOREVERT, STF_DOTUNDERLINE);
        if (hl != NULL)
            hl->SetActionShowHint(LoadStr(IDS_UMCCONFIRMHOWTOREV));
        break;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDB_UMC_BROWSENAME1:
        case IDB_UMC_BROWSENAME2:
        {
            int editID = LOWORD(wParam) == IDB_UMC_BROWSENAME1 ? IDE_UMC_NAME1 : IDE_UMC_NAME2;
            if (ComparingFiles)
                BrowseCommand(HWindow, editID, IDS_ALLFILTER);
            else
            {
                char path[SAL_MAX_PATH];
                GetDlgItemText(HWindow, editID, path, SizeOf(path));
                if (GetTargetDirectory(HWindow, HWindow, LoadStr(IDS_BROWSEUMCDIRTITLE),
                                       LoadStr(IDS_BROWSEUMCDIRTEXT), path, FALSE, path))
                {
                    SetDlgItemText(HWindow, editID, path);
                }
            }
            return TRUE;
        }
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}

//****************************************************************************
//
// CManageConfigsDialog
//

extern const char* SalamanderConfigurationRoots[];
extern const char* SalamanderConfigurationVersions[SALCFG_ROOTS_COUNT];
extern BOOL ExportConfiguration(HWND hParent, const char* fileName, BOOL clearKeyBeforeImport);
extern BOOL ImportConfiguration(HWND hParent, const char* fileName, BOOL ignoreIfNotExists,
                                BOOL autoImportConfig, BOOL* importCfgFromFileWasSkipped);
extern eRPE_ERROR CopyBranch(LPCTSTR branch, CSalamanderRegistryExAbstract* pInRegistry,
                             CSalamanderRegistryExAbstract* pOutRegistry);
extern void ShowFileError(HWND hParent, int errTextID, const char* fileName, DWORD err);

CManageConfigsDialog::CManageConfigsDialog(HWND parent)
    : CCommonDialog(HLanguage, IDD_MANAGECONFIGS, parent), LanguageItems(5, 5)
{
    ConfigsCount = 0;
    ZeroMemory(Configs, sizeof(Configs));
    ZeroMemory(ConfigChecked, sizeof(ConfigChecked));
    DeleteConfigurations = NULL;
    IndexOfConfigToLoad = -1;
    StorageType = cstRegistry;
    RegFilePath[0] = 0;
    CanSaveBootstrap = TRUE;
    SelectedSourceIndex = -1;
    SortColumn = -1;
    SortAscending = TRUE;
    ManageMode = FALSE;
    WelcomeProcessedLocation[0] = 0;
    HFontNormal = NULL;
    HFontBold = NULL;
    HFontItalic = NULL;
    HFontBoldItalic = NULL;
    CustomConfigName[0] = 0;
    CustomLanguage[0] = 0;
    DeleteSourceAfterMigration = FALSE;
    SyncNameAttentionActive = FALSE;
    HToolTip = NULL;
}

CManageConfigsDialog::~CManageConfigsDialog()
{
    for (int i = 0; i < LanguageItems.Count; i++)
        LanguageItems[i].Free();
    DestroyFonts();
}

static void EnableMCDStorageControls(HWND hWindow, BOOL canWrite, BOOL isFileStorage)
{
    EnableWindow(GetDlgItem(hWindow, IDC_MCD_FILE_PATH), canWrite && isFileStorage);
    EnableWindow(GetDlgItem(hWindow, IDC_MCD_FILE_BROWSE), canWrite && isFileStorage);
}

static void DrawRadioBtn(HDC hdc, int x, int y, int size, BOOL filled)
{
    HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    Ellipse(hdc, x, y, x + size, y + size);
    if (filled)
    {
        int inset = size / 4;
        HBRUSH hFillBrush = CreateSolidBrush(GetSysColor(COLOR_BTNTEXT));
        SelectObject(hdc, hFillBrush);
        Ellipse(hdc, x + inset, y + inset, x + size - inset, y + size - inset);
        DeleteObject(hFillBrush);
    }
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}


static int MCDGetConfigIndexFromListItem(HWND hList, int item)
{
    if (item < 0)
        return -1;

    LVITEM lvi;
    memset(&lvi, 0, sizeof(lvi));
    lvi.mask = LVIF_PARAM;
    lvi.iItem = item;
    if (!ListView_GetItem(hList, &lvi))
        return -1;
    return (int)lvi.lParam;
}

static int MCDFindListItemByConfigIndex(HWND hList, int configIndex)
{
    int count = ListView_GetItemCount(hList);
    for (int item = 0; item < count; item++)
    {
        if (MCDGetConfigIndexFromListItem(hList, item) == configIndex)
            return item;
    }
    return -1;
}

static void MCDSelectListItem(HWND hList, int item)
{
    if (item < 0 || item >= ListView_GetItemCount(hList))
        return;
    ListView_SetItemState(hList, item, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hList, item, FALSE);
}

static BOOL MCDIsCleanConfigItem(const CFoundConfig& cfg)
{
    return cfg.Exists && cfg.RootIndex == -1 && !cfg.IsPortable;
}

void CManageConfigsDialog::InitConfigsList()
{
    HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);

    DWORD exFlags = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_INFOTIP;
    DWORD origFlags = ListView_GetExtendedListViewStyle(hList);
    ListView_SetExtendedListViewStyle(hList, origFlags | exFlags);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;

    char col[256];

    lvc.cx = 75;
    strncpy_s(col, LoadStr(IDS_MCD_LASTUPDATE), _TRUNCATE);
    lvc.pszText = col;
    ListView_InsertColumn(hList, 0, &lvc);

    lvc.cx = 280;
    strncpy_s(col, LoadStr(IDS_MCD_CONFIGNAME), _TRUNCATE);
    lvc.pszText = col;
    ListView_InsertColumn(hList, 1, &lvc);

    lvc.cx = 120;
    strncpy_s(col, LoadStr(IDS_MCD_VERSION), _TRUNCATE);
    lvc.pszText = col;
    ListView_InsertColumn(hList, 2, &lvc);

    lvc.cx = 70;
    strncpy_s(col, LoadStr(IDS_MCD_LANG), _TRUNCATE);
    lvc.pszText = col;
    ListView_InsertColumn(hList, 3, &lvc);

    lvc.cx = 65;
    strncpy_s(col, LoadStr(IDS_MCD_STORAGE), _TRUNCATE);
    lvc.pszText = col;
    ListView_InsertColumn(hList, 4, &lvc);

    lvc.cx = 450;
    strncpy_s(col, LoadStr(IDS_MCD_LOCATION), _TRUNCATE);
    lvc.pszText = col;
    ListView_InsertColumn(hList, 5, &lvc);
}

void CManageConfigsDialog::PopulateConfigsList()
{
    HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
    ListView_DeleteAllItems(hList);
    for (int i = 0; i < ConfigsCount; i++)
    {
        if (!Configs[i].Exists)
            continue;

        // Last update jako prvni sloupec (0)
        char firstColText[50] = "";
        if (Configs[i].LastUpdate.dwLowDateTime != 0 || Configs[i].LastUpdate.dwHighDateTime != 0)
        {
            SYSTEMTIME st;
            if (FileTimeToSystemTime(&Configs[i].LastUpdate, &st))
            {
                if (!GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, firstColText, SizeOf(firstColText)))
                    _snprintf_s(firstColText, _TRUNCATE, "%02d/%02d/%04d", st.wMonth, st.wDay, st.wYear);
            }
            else
                strncpy_s(firstColText, LoadStr(IDS_MCD_LASTUPDATE_UNKNOWN), _TRUNCATE);
        }
        else
            strncpy_s(firstColText, LoadStr(IDS_MCD_LASTUPDATE_UNKNOWN), _TRUNCATE);

        LVITEM lvItem;
        memset(&lvItem, 0, sizeof(lvItem));
        lvItem.mask = LVIF_TEXT | LVIF_PARAM;
        lvItem.iItem = ListView_GetItemCount(hList);
        lvItem.iSubItem = 0;
        lvItem.pszText = firstColText;
        lvItem.lParam = i;
        int idx = ListView_InsertItem(hList, &lvItem);

        ListView_SetItemText(hList, idx, 1, Configs[i].DisplayName);
        ListView_SetItemText(hList, idx, 2, Configs[i].Version);
        ListView_SetItemText(hList, idx, 3, Configs[i].Language);
        ListView_SetItemText(hList, idx, 4, Configs[i].StorageTypeStr);
        ListView_SetItemText(hList, idx, 5, Configs[i].Location);
    }

    int itemToSelect = MCDFindListItemByConfigIndex(hList, SelectedSourceIndex);
    if (itemToSelect < 0 && ListView_GetItemCount(hList) > 0)
    {
        itemToSelect = 0;
        SelectedSourceIndex = MCDGetConfigIndexFromListItem(hList, itemToSelect);
    }
    MCDSelectListItem(hList, itemToSelect);
}

void CManageConfigsDialog::UpdateSourcePanel()
{
    if (SelectedSourceIndex >= 0 && SelectedSourceIndex < ConfigsCount && Configs[SelectedSourceIndex].Exists)
    {
        CFoundConfig& cfg = Configs[SelectedSourceIndex];
        // Pro prazdnou konfiguraci zobrazit nazev aplikace jako defaultni jmeno
        const char* cfgName = (cfg.RootIndex == -1 && !cfg.IsPortable) ? SALAMANDER_TEXT_VERSION : cfg.DisplayName;
        SetDlgItemText(HWindow, IDC_MCD_SRC_NAME, cfgName);
        SetDlgItemText(HWindow, IDC_MCD_SRC_VERSION, cfg.Version);
        SetDlgItemText(HWindow, IDC_MCD_SRC_STORAGE, cfg.StorageTypeStr);
        if (MCDIsCleanConfigItem(cfg))
        {
            const char* welcomeLanguage = Configuration.LoadedSLGName[0] != 0 ? Configuration.LoadedSLGName : Configuration.SLGName;
            SelectLanguageInCombo(welcomeLanguage != NULL && welcomeLanguage[0] != 0 ? welcomeLanguage : "english");
        }
        else
            SelectLanguageInCombo(cfg.Language);
        SetDlgItemText(HWindow, IDC_MCD_SRC_LOCATION, cfg.Location);
    }
    else
    {
        SetDlgItemText(HWindow, IDC_MCD_SRC_NAME, "");
        SetDlgItemText(HWindow, IDC_MCD_SRC_VERSION, "");
        SetDlgItemText(HWindow, IDC_MCD_SRC_STORAGE, "");
        SendMessage(GetDlgItem(HWindow, IDC_MCD_SRC_LANG), CB_SETCURSEL, (WPARAM)-1, 0);
        SetDlgItemText(HWindow, IDC_MCD_SRC_LOCATION, "");
    }
    UpdateSyncNameButton();
}

void CManageConfigsDialog::UpdateStorageControls()
{
    BOOL isFileStorage = IsDlgButtonChecked(HWindow, IDC_MCD_FILE_RADIO) == BST_CHECKED;
    BOOL canWrite = CanSaveBootstrap;

    // Radio buttony - disabled když nemáme práva, při no-write vždy registry
    EnableWindow(GetDlgItem(HWindow, IDC_MCD_REG_RADIO), canWrite);
    EnableWindow(GetDlgItem(HWindow, IDC_MCD_FILE_RADIO), canWrite);
    if (!canWrite)
    {
        CheckRadioButton(HWindow, IDC_MCD_REG_RADIO, IDC_MCD_FILE_RADIO, IDC_MCD_REG_RADIO);
        isFileStorage = FALSE;
    }
    EnableMCDStorageControls(HWindow, canWrite, isFileStorage);

    // Import button - disabled když nemáme práva pro zápis
    EnableWindow(GetDlgItem(HWindow, IDC_MCD_IMPORT), canWrite);
}

void CManageConfigsDialog::UpdateDeleteButtonState()
{
    BOOL hasSelection = (SelectedSourceIndex >= 0 && SelectedSourceIndex < ConfigsCount &&
                         Configs[SelectedSourceIndex].Exists);
    BOOL isEmptyConfig = (hasSelection && Configs[SelectedSourceIndex].RootIndex == -1 &&
                          !Configs[SelectedSourceIndex].IsPortable);

    BOOL canDelete = hasSelection && !isEmptyConfig;
    BOOL canExport = hasSelection && !isEmptyConfig;

    if (canDelete && hasSelection)
    {
        CFoundConfig& cfg = Configs[SelectedSourceIndex];
        // U file storage configu bez práv pro zápis je delete disabled
        if (cfg.IsPortable && !CanSaveBootstrap)
            canDelete = FALSE;
    }
    if (canExport && hasSelection)
    {
        CFoundConfig& cfg = Configs[SelectedSourceIndex];
        // Clean config nelze exportovat
        if (cfg.RootIndex == -1 && !cfg.IsPortable)
            canExport = FALSE;
    }

    EnableWindow(GetDlgItem(HWindow, IDC_MCD_DELETE_SEL), canDelete);
    EnableWindow(GetDlgItem(HWindow, IDC_MCD_EXPORT), canExport);
    BOOL canDeleteAfterMigration = canDelete && hasSelection && !Configs[SelectedSourceIndex].IsCurrentVersion;
    EnableWindow(GetDlgItem(HWindow, IDC_MCD_DELETE_SOURCE_AFTER), !ManageMode && canDeleteAfterMigration);
    if (ManageMode || !canDeleteAfterMigration)
        CheckDlgButton(HWindow, IDC_MCD_DELETE_SOURCE_AFTER, BST_UNCHECKED);
}



void CManageConfigsDialog::InitLanguageCombo()
{
    HWND hCombo = GetDlgItem(HWindow, IDC_MCD_SRC_LANG);
    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i < LanguageItems.Count; i++)
        LanguageItems[i].Free();
    LanguageItems.DestroyMembers();

    char path[SAL_MAX_PATH];
    GetModuleFileName(NULL, path, SAL_MAX_PATH);
    lstrcpy(strrchr(path, '\\') + 1, "lang\\*.slg");

    WIN32_FIND_DATA file;
    HANDLE hFind = HANDLES_Q(FindFirstFile(path, &file));
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            char* point = strrchr(file.cFileName, '.');
            if (point != NULL && stricmp(point + 1, "slg") == 0)
            {
                CLanguage lang;
                if (lang.Init(file.cFileName, NULL))
                {
                    int itemIndex = LanguageItems.Count;
                    LanguageItems.Add(lang);
                    if (!LanguageItems.IsGood())
                    {
                        LanguageItems.ResetState();
                        lang.Free();
                        break;
                    }
                    char name[200];
                    LanguageItems[itemIndex].GetLanguageName(name, SizeOf(name));
                    int comboIndex = (int)SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)name);
                    if (comboIndex != CB_ERR && comboIndex != CB_ERRSPACE)
                        SendMessage(hCombo, CB_SETITEMDATA, comboIndex, itemIndex);
                }
            }
        } while (FindNextFile(hFind, &file));
        HANDLES(FindClose(hFind));
    }
}

void CManageConfigsDialog::SelectLanguageInCombo(const char* language)
{
    HWND hCombo = GetDlgItem(HWindow, IDC_MCD_SRC_LANG);
    int select = -1;
    char slgName[100];
    slgName[0] = 0;
    if (language != NULL && language[0] != 0 && strcmp(language, "-") != 0)
    {
        strncpy_s(slgName, language, _TRUNCATE);
        if (strrchr(slgName, '.') == NULL)
            strncat_s(slgName, ".slg", _TRUNCATE);
    }
    for (int i = 0; i < (int)SendMessage(hCombo, CB_GETCOUNT, 0, 0); i++)
    {
        int langIndex = (int)SendMessage(hCombo, CB_GETITEMDATA, i, 0);
        if (langIndex >= 0 && langIndex < LanguageItems.Count && _stricmp(LanguageItems[langIndex].FileName, slgName) == 0)
        {
            select = i;
            break;
        }
    }
    SendMessage(hCombo, CB_SETCURSEL, select, 0);
}

static void MCDNormalizeGeneratedNameText(char* text)
{
    if (text == NULL)
        return;
    for (char* p = text; *p != 0; p++)
        if (*p == '-')
            *p = ' ';
    char* end = text + strlen(text);
    while (end > text && end[-1] == ' ')
        *--end = 0;
}

static BOOL MCDLooksLikeGeneratedConfigName(const CFoundConfig& cfg)
{
    if (cfg.DisplayName[0] == 0 || cfg.Version[0] == 0)
        return FALSE;

    char name[256];
    strncpy_s(name, cfg.DisplayName, _TRUNCATE);
    MCDNormalizeGeneratedNameText(name);

    char version[64];
    strncpy_s(version, cfg.Version, _TRUNCATE);
    MCDNormalizeGeneratedNameText(version);

    return (_strnicmp(name, "Open Salamander ", 16) == 0 ||
            _strnicmp(name, "Altap Salamander ", 16) == 0 ||
            _strnicmp(name, "Servant Salamander ", 18) == 0) &&
           StrIStr(name, version) != NULL;
}

void CManageConfigsDialog::UpdateSyncNameButton()
{
    BOOL show = FALSE;
    if (SelectedSourceIndex >= 0 && SelectedSourceIndex < ConfigsCount && Configs[SelectedSourceIndex].Exists)
    {
        const CFoundConfig& cfg = Configs[SelectedSourceIndex];
        show = !MCDIsCleanConfigItem(cfg) && !cfg.IsCurrentVersion &&
               (cfg.IsGeneratedName || MCDLooksLikeGeneratedConfigName(cfg));
    }
    HWND hButton = GetDlgItem(HWindow, IDC_MCD_SYNC_NAME);
    ShowWindow(hButton, show ? SW_SHOW : SW_HIDE);
    EnableWindow(hButton, show);
    if (show && !SyncNameAttentionActive)
    {
        SyncNameAttentionActive = TRUE;
        SetTimer(HWindow, 1, 350, NULL);
    }
    else if (!show && SyncNameAttentionActive)
    {
        KillTimer(HWindow, 1);
        SyncNameAttentionActive = FALSE;
    }
}

void CManageConfigsDialog::OnSyncName()
{
    char name[256];
    strncpy_s(name, SALAMANDER_TEXT_VERSION, _TRUNCATE);
    char* end = name + strlen(name);
    while (end > name && end[-1] == ' ')
        *--end = 0;
    SetDlgItemText(HWindow, IDC_MCD_SRC_NAME, name);
    KillTimer(HWindow, 1);
    SyncNameAttentionActive = FALSE;
    HWND hButton = GetDlgItem(HWindow, IDC_MCD_SYNC_NAME);
    SetDlgItemText(HWindow, IDC_MCD_SYNC_NAME, LoadStr(IDS_MCD_SYNC_NAME));
    ShowWindow(hButton, SW_HIDE);
    EnableWindow(hButton, FALSE);
    InvalidateRect(hButton, NULL, TRUE);
}

void CManageConfigsDialog::SortConfigs()
{
    if (SortColumn < 0 || SortColumn > 5)
        return;

    char selectedLocation[MAX_PATH];
    selectedLocation[0] = 0;
    int selectedRootIndex = -2;
    BOOL selectedIsPortable = FALSE;
    if (SelectedSourceIndex >= 0 && SelectedSourceIndex < ConfigsCount && Configs[SelectedSourceIndex].Exists)
    {
        strncpy_s(selectedLocation, Configs[SelectedSourceIndex].Location, _TRUNCATE);
        selectedRootIndex = Configs[SelectedSourceIndex].RootIndex;
        selectedIsPortable = Configs[SelectedSourceIndex].IsPortable;
    }

    // Jednoduchy bubble sort (pole je male, max 100)
    for (int i = 0; i < ConfigsCount - 1; i++)
    {
        for (int j = i + 1; j < ConfigsCount; j++)
        {
            if (!Configs[i].Exists || !Configs[j].Exists)
                continue;

            const CFoundConfig& ca = Configs[i];
            const CFoundConfig& cb = Configs[j];
            BOOL caIsClean = MCDIsCleanConfigItem(ca);
            BOOL cbIsClean = MCDIsCleanConfigItem(cb);
            int cmp = 0;
            if (caIsClean != cbIsClean)
            {
                // Keep "Clean configuration with default values" pinned as the first row,
                // independently of the active sort column or direction.
                cmp = caIsClean ? -1 : 1;
            }
            else
            {
                switch (SortColumn)
                {
                case 0: cmp = CompareFileTime(&ca.LastUpdate, &cb.LastUpdate); break;
                case 1: cmp = _stricmp(ca.DisplayName, cb.DisplayName); break;
                case 2: cmp = _stricmp(ca.Version, cb.Version); break;
                case 3: cmp = _stricmp(ca.Language, cb.Language); break;
                case 4: cmp = _stricmp(ca.StorageTypeStr, cb.StorageTypeStr); break;
                case 5: cmp = _stricmp(ca.Location, cb.Location); break;
                }
                if (!SortAscending) cmp = -cmp;
            }
            if (cmp > 0)
            {
                CFoundConfig tmp = Configs[i];
                Configs[i] = Configs[j];
                Configs[j] = tmp;
            }
        }
    }

    if (selectedLocation[0] != 0)
    {
        SelectedSourceIndex = -1;
        for (int i = 0; i < ConfigsCount; i++)
        {
            if (Configs[i].Exists && Configs[i].RootIndex == selectedRootIndex &&
                Configs[i].IsPortable == selectedIsPortable &&
                _stricmp(Configs[i].Location, selectedLocation) == 0)
            {
                SelectedSourceIndex = i;
                break;
            }
        }
    }

    // Nastavit header sort indicator
    HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
    HWND hHeader = ListView_GetHeader(hList);
    for (int i = 0; i <= 5; i++)
    {
        HDITEM hdi;
        memset(&hdi, 0, sizeof(hdi));
        hdi.mask = HDI_FORMAT;
        Header_GetItem(hHeader, i, &hdi);
        hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
        if (i == SortColumn)
            hdi.fmt |= SortAscending ? HDF_SORTUP : HDF_SORTDOWN;
        Header_SetItem(hHeader, i, &hdi);
    }
}

void CManageConfigsDialog::OnUseAsSource()
{
    HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    int configIndex = MCDGetConfigIndexFromListItem(hList, sel);
    if (configIndex >= 0 && configIndex < ConfigsCount)
    {
        SelectedSourceIndex = configIndex;
        UpdateSourcePanel();
        InvalidateRect(hList, NULL, TRUE);
    }
}

void CManageConfigsDialog::OnDeleteSelected()
{
    if (SelectedSourceIndex < 0 || SelectedSourceIndex >= ConfigsCount || !Configs[SelectedSourceIndex].Exists)
        return;

    CFoundConfig& cfg = Configs[SelectedSourceIndex];
    char msg[1000];
    _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_MCD_DELETECONFIRM2),
                cfg.DisplayName, cfg.Version, cfg.StorageTypeStr, cfg.Location);
    if (SalMessageBox(HWindow, msg, LoadStr(IDS_QUESTION), MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
    int selItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    if (!DeleteConfigByIndex(SelectedSourceIndex))
    {
        UpdateSourcePanel();
        UpdateDeleteButtonState();
        return;
    }

    SelectedSourceIndex = -1;
    PopulateConfigsList();
    int itemCount = ListView_GetItemCount(hList);
    if (itemCount > 0)
    {
        if (selItem >= itemCount)
            selItem = itemCount - 1;
        MCDSelectListItem(hList, selItem);
        SelectedSourceIndex = MCDGetConfigIndexFromListItem(hList, selItem);
    }
    UpdateSourcePanel();
    UpdateDeleteButtonState();
}

void CManageConfigsDialog::OnExport()
{
    if (SelectedSourceIndex < 0 || SelectedSourceIndex >= ConfigsCount || !Configs[SelectedSourceIndex].Exists)
        return;

    CFoundConfig& cfg = Configs[SelectedSourceIndex];

    // Nelze exportovat clean config
    if (cfg.RootIndex == -1 && !cfg.IsPortable)
        return;

    // Sanitizovat configuration name pro pouziti jako jmeno souboru
    char sanitizedName[MAX_PATH];
    strncpy_s(sanitizedName, cfg.DisplayName, _TRUNCATE);
    for (char* p = sanitizedName; *p; p++)
    {
        if (*p == '\\' || *p == '/' || *p == ':' || *p == '*' || *p == '?' ||
            *p == '"' || *p == '<' || *p == '>' || *p == '|')
            *p = '_';
    }

    char file[SAL_MAX_PATH];
    _snprintf_s(file, _TRUNCATE, "%s.reg", sanitizedName);
    char defDir[SAL_MAX_PATH];

    if (WindowsVistaAndLater)
    {
        if (!CreateOurPathInRoamingAPPDATA(defDir))
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEPATHERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }
    else
    {
        GetModuleFileName(HInstance, defDir, SAL_MAX_PATH);
        char* slash = strrchr(defDir, '\\');
        if (slash != NULL)
            *slash = 0;
    }

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    char filter[] = "Registration Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0\0";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = HWindow;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = SizeOf(file);
    ofn.lpstrInitialDir = defDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "reg";

    if (SafeGetSaveFileName(&ofn))
    {
        if (SalGetFullName(file))
        {
            DWORD attrs = GetFileAttributes(file);
            if (attrs != INVALID_FILE_ATTRIBUTES)
            {
                char msg[500];
                _snprintf_s(msg, _TRUNCATE, "%s", LoadStr(IDS_MCD_OVERWRITECONFIRM));
                if (SalMessageBox(HWindow, msg, LoadStr(IDS_QUESTION), MB_YESNO | MB_ICONQUESTION) != IDYES)
                    return;
            }

            BOOL exported = FALSE;

            if (cfg.IsPortable)
            {
                // Export file storage - kopie souboru
                if (CopyFile(cfg.Location, file, FALSE))
                {
                    exported = TRUE;
                }
                else
                {
                    ShowFileError(HWindow, IDS_EXPORTCFG_FILEERR, file, GetLastError());
                }
            }
            else if (cfg.RootIndex >= 0)
            {
                // Export registry config
                char keyName[MAX_PATH];
                _snprintf_s(keyName, _TRUNCATE, "HKEY_CURRENT_USER\\%s", SalamanderConfigurationRoots[cfg.RootIndex]);

                CSalamanderRegistryExAbstract* activeReg = ConfigurationStorage.GetRegistry();
                CSalamanderRegistryExAbstract* sysReg = activeReg != NULL ? activeReg : REG_SysRegistryFactory();
                CSalamanderRegistryExAbstract* memReg = REG_MemRegistryFactory();

                if (sysReg != NULL && memReg != NULL)
                {
                    LoadSaveToRegistryMutex.Enter();
                    eRPE_ERROR regerr = CopyBranch(keyName, sysReg, memReg);
                    LoadSaveToRegistryMutex.Leave();

                    if (RPE_OK == regerr)
                    {
                        memReg->RemoveHiddenKeysAndValues();
                        if (memReg->Dump(file, keyName))
                        {
                            exported = TRUE;
                        }
                        else
                        {
                            ShowFileError(HWindow, IDS_EXPORTCFG_FILEERR, file, 0);
                        }
                    }
                    else
                    {
                        ShowFileError(HWindow, IDS_EXPORTCFG_REGERR, file, 0);
                    }
                }

                if (sysReg != NULL && sysReg != activeReg)
                    sysReg->Release();
                if (memReg != NULL)
                    memReg->Release();
            }

            if (exported)
            {
                SalMessageBox(HWindow, LoadStr(IDS_CONFIGEXPORTED), LoadStr(IDS_INFOTITLE),
                              MB_OK | MB_ICONINFORMATION);
            }
            else if (!exported)
            {
                // Export se nepodaril, smazat prazdny soubor
                DeleteFile(file);
            }
        }
    }
}

void CManageConfigsDialog::OnBrowseFile()
{
    char path[SAL_MAX_PATH];
    GetDlgItemText(HWindow, IDC_MCD_FILE_PATH, path, SizeOf(path));
    if (BrowseRegStorageFile(HWindow, path, SizeOf(path)))
    {
        if (CanWriteRegStorageFilePath(path))
            SetDlgItemText(HWindow, IDC_MCD_FILE_PATH, path);
        else
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
    }
}

void CManageConfigsDialog::OnStorageRadioChanged()
{
    UpdateStorageControls();
}

void CManageConfigsDialog::OnImport()
{
    char file[SAL_MAX_PATH] = "";
    char defDir[SAL_MAX_PATH];

    if (WindowsVistaAndLater)
    {
        if (!CreateOurPathInRoamingAPPDATA(defDir))
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEPATHERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            return;
        }
    }
    else
    {
        GetModuleFileName(HInstance, defDir, SAL_MAX_PATH);
        char* slash = strrchr(defDir, '\\');
        if (slash != NULL)
            *slash = 0;
    }

    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    char filter[] = "Registration Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0\0";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = HWindow;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = file;
    ofn.nMaxFile = SizeOf(file);
    ofn.lpstrInitialDir = defDir;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "reg";

    if (SafeGetOpenFileName(&ofn))
    {
        if (SalGetFullName(file))
        {
            // Overit ze soubor je validni .reg konfigurace
            DWORD err = 0;
            HANDLE hFile = HANDLES_Q(CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL,
                                                OPEN_EXISTING, 0, 0));
            if (hFile == INVALID_HANDLE_VALUE)
            {
                err = GetLastError();
                ShowFileError(HWindow, IDS_IMPORTCFG_OPENERR, file, err);
                return;
            }

            BOOL valid = FALSE;
            LPTSTR buf = NULL;
            CQuadWord size;
            if (SalGetFileSize(hFile, size, err))
            {
                if (size <= CQuadWord(10000000, 0))
                {
                    buf = (LPTSTR)malloc((DWORD)size.Value + sizeof(WCHAR));
                    if (buf != NULL)
                    {
                        DWORD bytesRead;
                        if (!ReadFile(hFile, buf, (DWORD)size.Value, &bytesRead, NULL))
                        {
                            ShowFileError(HWindow, IDS_IMPORTCFG_OPENERR, file, GetLastError());
                            free(buf);
                            buf = NULL;
                        }
                        else
                        {
                            if ((DWORD)size.Value > bytesRead)
                                size.Set(bytesRead, 0);
                        }
                    }
                }
                else
                    ShowFileError(HWindow, IDS_IMPORTCFG_TOOBIG, file, 0);
            }
            else
                ShowFileError(HWindow, IDS_IMPORTCFG_OPENERR, file, GetLastError());

            HANDLES(CloseHandle(hFile));

            if (buf != NULL && (DWORD)size.Value > 0)
            {
                *(WCHAR*)((LPBYTE)buf + (DWORD)size.Value) = 0;
                if (ConvertIfNeeded(&buf, (DWORD)size.Value) == 0)
                {
                    free(buf);
                    buf = NULL;
                }
            }

            if (buf != NULL)
            {
                // Zkusit naparsit do pametoveho registru pro validaci
                CSalamanderRegistryExAbstract* memReg = REG_MemRegistryFactory();
                LPTSTR bufMem = _tcsdup(buf);
                eRPE_ERROR regerr = bufMem != NULL ? Parse(bufMem, memReg, TRUE) : RPE_OUT_OF_MEMORY;
                free(bufMem);

                if (RPE_OK == regerr)
                {
                    valid = TRUE;
                }
                else
                {
                    int errTextID = IDS_IMPORTCFG_REGERR;
                    switch (regerr)
                    {
                    case RPE_NOT_REG_FILE:
                        errTextID = IDS_IMPORTCFG_NOTREG;
                        break;
                    case RPE_ROOT_INVALID_KEY:
                    case RPE_INVALID_KEY:
                    case RPE_VALUE_MISSING_QUOTE:
                    case RPE_VALUE_MISSING_ASSIG:
                    case RPE_VALUE_INVALID_TYPE:
                    case RPE_VALUE_DWORD:
                    case RPE_VALUE_STRING:
                    case RPE_VALUE_HEX:
                    case RPE_INVALID_MBCS:
                    case RPE_INVALID_FORMAT:
                        errTextID = IDS_IMPORTCFG_INVALIDFORMAT;
                        break;
                    }
                    ShowFileError(HWindow, errTextID, file, 0);
                }

                memReg->Release();
                free(buf);
            }

            if (valid)
            {
                int existingIndex = -1;
                for (int i = 0; i < ConfigsCount; i++)
                {
                    if (Configs[i].Exists && Configs[i].IsPortable && IsTheSamePath(Configs[i].Location, file))
                    {
                        existingIndex = i;
                        break;
                    }
                }

                // Pridat cestu k souboru do seznamu known file storage paths
                ConfigurationStorage.AddKnownFileStoragePath(file);

                if (existingIndex >= 0)
                {
                    SelectedSourceIndex = existingIndex;
                    PopulateConfigsList();
                    MCDSelectListItem(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST),
                                      MCDFindListItemByConfigIndex(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST), existingIndex));
                }
                else if (ConfigsCount < MCD_MAX_CONFIGS)
                {
                    CFoundConfig& cfg = Configs[ConfigsCount];
                    MCDReadFileConfigurationInfo(file, cfg, FALSE);
                    int importedIndex = ConfigsCount++;

                    SelectedSourceIndex = importedIndex;
                    PopulateConfigsList();
                    MCDSelectListItem(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST),
                                      MCDFindListItemByConfigIndex(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST), importedIndex));
                }
                else
                    PopulateConfigsList();

                SalMessageBox(HWindow, LoadStr(IDS_MCD_IMPORTSUCCESS), LoadStr(IDS_INFOTITLE),
                              MB_OK | MB_ICONINFORMATION);

                UpdateSourcePanel();
                UpdateDeleteButtonState();
            }
        }
    }
}

BOOL CManageConfigsDialog::DeleteConfigByIndex(int configIndex)
{
    if (configIndex < 0 || configIndex >= ConfigsCount || !Configs[configIndex].Exists)
        return FALSE;

    CFoundConfig& cfg = Configs[configIndex];
    BOOL deleted = FALSE;

    if (cfg.IsPortable)
    {
        if (DeleteFile(cfg.Location))
        {
            deleted = TRUE;
            // Odstranit z known paths az po uspesnem smazani souboru.
            ConfigurationStorage.RemoveKnownFileStoragePath(cfg.Location);
        }
        else
            ShowFileError(HWindow, IDS_MCD_DELETEFILEERR, cfg.Location, GetLastError());
    }
    else if (cfg.RootIndex >= 0)
    {
        HKEY key;
        if (OpenKey(HKEY_CURRENT_USER, SalamanderConfigurationRoots[cfg.RootIndex], key))
        {
            ClearKey(key);
            CloseKey(key);
            DeleteKey(HKEY_CURRENT_USER, SalamanderConfigurationRoots[cfg.RootIndex]);
            deleted = TRUE;
        }
        if (deleted && DeleteConfigurations != NULL)
            DeleteConfigurations[cfg.RootIndex] = TRUE;
    }

    if (deleted)
        cfg.Exists = FALSE;

    return deleted;
}

void CManageConfigsDialog::Transfer(CTransferInfo& ti)
{
    if (ti.Type == ttDataToWindow)
    {
        char buff[5000];
        char buff2[5000];
        GetWindowText(HWindow, buff, 5000);
        _snprintf_s(buff2, _TRUNCATE, buff, SALAMANDER_TEXT_VERSION);
        SetWindowText(HWindow, buff2);

        InitLanguageCombo();
        InitConfigsList();
        PopulateConfigsList();

        char regPath[MAX_PATH];
        _snprintf_s(regPath, _TRUNCATE, "reg:\\HKEY_CURRENT_USER\\%s", SalamanderConfigurationRoots[0]);
        SetDlgItemText(HWindow, IDC_MCD_REGISTRY_PATH, regPath);

        if (RegFilePath[0] == 0)
        {
            CConfigurationStorageType bootstrapType = (CConfigurationStorageType)StorageType;
            ConfigurationStorage.LoadStorageTypeBootstrap(bootstrapType, RegFilePath, SizeOf(RegFilePath));
        }
        if (RegFilePath[0] == 0)
            ConfigurationStorage.GetPortableConfigFilePath(RegFilePath, SizeOf(RegFilePath));
        SetDlgItemText(HWindow, IDC_MCD_FILE_PATH, RegFilePath);

        CheckRadioButton(HWindow, IDC_MCD_REG_RADIO, IDC_MCD_FILE_RADIO,
                         StorageType == cstRegFile ? IDC_MCD_FILE_RADIO : IDC_MCD_REG_RADIO);

        if (ConfigsCount > 0)
        {
            UpdateSourcePanel();
            UpdateDeleteButtonState();
        }

        SetDlgItemText(HWindow, IDC_MCD_SYNC_NAME, LoadStr(IDS_MCD_SYNC_NAME));
        SetDlgItemText(HWindow, IDC_MCD_DELETE_SOURCE_AFTER, LoadStr(IDS_MCD_DELETE_SOURCE_AFTER));
        CheckDlgButton(HWindow, IDC_MCD_DELETE_SOURCE_AFTER, DeleteSourceAfterMigration ? BST_CHECKED : BST_UNCHECKED);
        ShowWindow(GetDlgItem(HWindow, IDC_MCD_DELETE_SOURCE_AFTER), ManageMode ? SW_HIDE : SW_SHOW);
        UpdateStorageControls();
        UpdateSyncNameButton();
    }
    else
    {
        if (IsDlgButtonChecked(HWindow, IDC_MCD_FILE_RADIO) == BST_CHECKED && CanSaveBootstrap)
        {
            StorageType = cstRegFile;
            GetDlgItemText(HWindow, IDC_MCD_FILE_PATH, RegFilePath, SizeOf(RegFilePath));
        }
        else
        {
            StorageType = cstRegistry;
            RegFilePath[0] = 0;
        }

        IndexOfConfigToLoad = -1;
        if (SelectedSourceIndex >= 0 && SelectedSourceIndex < ConfigsCount &&
            Configs[SelectedSourceIndex].Exists && !Configs[SelectedSourceIndex].IsPortable)
        {
            IndexOfConfigToLoad = Configs[SelectedSourceIndex].RootIndex;
        }

        // Precist editovatelný Configuration name z IDC_MCD_SRC_NAME
        char cfgName[256];
        GetDlgItemText(HWindow, IDC_MCD_SRC_NAME, cfgName, SizeOf(cfgName));
        // Do not write the edited target name back to the selected source row.
        // The row is a read-only import source; the name is applied to the target
        // configuration by the caller after the target storage has been created.
        // Ulozit custom name pro pripadne ulozeni do registrů
        strncpy_s(CustomConfigName, cfgName, _TRUNCATE);

        CustomLanguage[0] = 0;
        int langSel = (int)SendMessage(GetDlgItem(HWindow, IDC_MCD_SRC_LANG), CB_GETCURSEL, 0, 0);
        if (langSel != CB_ERR)
        {
            int langIndex = (int)SendMessage(GetDlgItem(HWindow, IDC_MCD_SRC_LANG), CB_GETITEMDATA, langSel, 0);
            if (langIndex >= 0 && langIndex < LanguageItems.Count)
                strncpy_s(CustomLanguage, LanguageItems[langIndex].FileName, _TRUNCATE);
        }
        DeleteSourceAfterMigration = !ManageMode && IsDlgButtonChecked(HWindow, IDC_MCD_DELETE_SOURCE_AFTER) == BST_CHECKED;
    }
}

void CManageConfigsDialog::Validate(CTransferInfo& ti)
{
    if (IsDlgButtonChecked(HWindow, IDC_MCD_FILE_RADIO) == BST_CHECKED && CanSaveBootstrap)
    {
        char path[SAL_MAX_PATH];
        GetDlgItemText(HWindow, IDC_MCD_FILE_PATH, path, SizeOf(path));
        if (path[0] == 0)
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEPATHERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDC_MCD_FILE_PATH);
            return;
        }
        if (!CanWriteRegStorageFilePath(path))
        {
            SalMessageBox(HWindow, LoadStr(IDS_CFGSTORAGE_FILEWRITEERR), LoadStr(IDS_ERRORTITLE),
                          MB_OK | MB_ICONEXCLAMATION);
            ti.ErrorOn(IDC_MCD_FILE_PATH);
            return;
        }
        // Overwrite confirmation - check if file already exists
        DWORD attrs = GetFileAttributes(path);
        if (attrs != INVALID_FILE_ATTRIBUTES)
        {
            char msg[500];
            _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_MCD_OVERWRITECONFIRM), path);
            if (SalMessageBox(HWindow, msg, LoadStr(IDS_QUESTION), MB_YESNO | MB_ICONQUESTION) != IDYES)
            {
                ti.ErrorOn(IDC_MCD_FILE_PATH);
                return;
            }
        }
    }
    else // Registry storage
    {
        // Registry target is the current-version root shown in IDC_MCD_REGISTRY_PATH.
        // Source rows may point elsewhere, but overwrite validation is for this explicit target.
        HKEY hKey;
        if (OpenKey(HKEY_CURRENT_USER, SalamanderConfigurationRoots[0], hKey))
        {
            HKEY hCfgKey;
            if (OpenKey(hKey, SALAMANDER_CONFIG_REG, hCfgKey))
            {
                CloseKey(hCfgKey);
                CloseKey(hKey);

                char regPath[MAX_PATH];
                GetDlgItemText(HWindow, IDC_MCD_REGISTRY_PATH, regPath, SizeOf(regPath));
                char msg[500];
                _snprintf_s(msg, _TRUNCATE, LoadStr(IDS_MCD_OVERWRITECONFIRM), regPath);
                if (SalMessageBox(HWindow, msg, LoadStr(IDS_QUESTION), MB_YESNO | MB_ICONQUESTION) != IDYES)
                {
                    ti.ErrorOn(IDC_MCD_REG_RADIO);
                    return;
                }
            }
            else
                CloseKey(hKey);
        }
    }
}

static void DrawCheckbox(HDC hdc, int x, int y, int size, BOOL checked)
{
    // Bile pozadi pro checkbox
    RECT rcBox;
    rcBox.left = x;
    rcBox.top = y;
    rcBox.right = x + size;
    rcBox.bottom = y + size;
    HBRUSH hBgBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
    FillRect(hdc, &rcBox, hBgBrush);
    DeleteObject(hBgBrush);

    // Ramecek
    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, y, x + size, y + size);

    // Hack pokud checked
    if (checked)
    {
        HPEN hCheckPen = CreatePen(PS_SOLID, 2, GetSysColor(COLOR_BTNTEXT));
        SelectObject(hdc, hCheckPen);
        MoveToEx(hdc, x + 3, y + size / 2, NULL);
        LineTo(hdc, x + size / 2 - 1, y + size - 3);
        MoveToEx(hdc, x + size / 2 - 1, y + size - 3, NULL);
        LineTo(hdc, x + size - 3, y + 3);
        DeleteObject(hCheckPen);
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void CManageConfigsDialog::CreateFonts()
{
    HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
    HFONT hOrigFont = (HFONT)SendMessage(hList, WM_GETFONT, 0, 0);

    LOGFONT lf;
    GetObject(hOrigFont, sizeof(lf), &lf);

    HFontNormal = CreateFontIndirect(&lf);

    lf.lfWeight = FW_BOLD;
    HFontBold = CreateFontIndirect(&lf);

    lf.lfWeight = FW_NORMAL;
    lf.lfItalic = TRUE;
    HFontItalic = CreateFontIndirect(&lf);

    lf.lfWeight = FW_BOLD;
    HFontBoldItalic = CreateFontIndirect(&lf);
}

void CManageConfigsDialog::DestroyFonts()
{
    if (HFontNormal) { DeleteObject(HFontNormal); HFontNormal = NULL; }
    if (HFontBold) { DeleteObject(HFontBold); HFontBold = NULL; }
    if (HFontItalic) { DeleteObject(HFontItalic); HFontItalic = NULL; }
    if (HFontBoldItalic) { DeleteObject(HFontBoldItalic); HFontBoldItalic = NULL; }
}

BOOL CManageConfigsDialog::IsConfigActive(int configIndex)
{
    if (configIndex < 0 || configIndex >= ConfigsCount)
        return FALSE;

    CFoundConfig& cfg = Configs[configIndex];

    // V welcome dialogu neni zadna aktivni konfigurace
    if (!ManageMode)
        return FALSE;

    // Registry config - porovnat s SALAMANDER_ROOT_REG
    if (cfg.RootIndex >= 0 && SALAMANDER_ROOT_REG != NULL)
    {
        return _stricmp(SALAMANDER_ROOT_REG, SalamanderConfigurationRoots[cfg.RootIndex]) == 0;
    }

    // File storage config - porovnat s Configuration.StorageType a cestou
    if (cfg.IsPortable && Configuration.StorageType == cstRegFile)
    {
        char activePath[MAX_PATH];
        ConfigurationStorage.GetPortableConfigFilePath(activePath, SizeOf(activePath));

        // Pokud je file storage cesta v configstorage.ini, pouzijeme ji
        CConfigurationStorageType bootstrapType = (CConfigurationStorageType)Configuration.StorageType;
        char bootstrapPath[MAX_PATH];
        bootstrapPath[0] = 0;
        ConfigurationStorage.LoadStorageTypeBootstrap(bootstrapType, bootstrapPath, SizeOf(bootstrapPath));
        if (bootstrapPath[0] != 0)
            strncpy_s(activePath, bootstrapPath, _TRUNCATE);

        return _stricmp(cfg.Location, activePath) == 0;
    }

    return FALSE;
}

HFONT CManageConfigsDialog::GetConfigFont(int configIndex, UINT itemState)
{
    BOOL isSelected = (itemState & CDIS_SELECTED) != 0;
    BOOL isActive = IsConfigActive(configIndex);

    if (isSelected && isActive)
        return HFontBoldItalic;
    else if (isSelected)
        return HFontBold;
    else if (isActive)
        return HFontItalic;
    else
        return HFontNormal;
}

INT_PTR
CManageConfigsDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        INT_PTR ret = CCommonDialog::DialogProc(uMsg, wParam, lParam);
        SendMessage(HWindow, WM_SYSCOMMAND, SC_RESTORE, 0);

        // Nastavit ikonu dialogu
        HICON hIcon = (HICON)LoadImage(HInstance, MAKEINTRESOURCE(IDI_SALAMANDER), IMAGE_ICON,
                                        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
        if (hIcon)
            SendMessage(HWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        HICON hIconSm = (HICON)LoadImage(HInstance, MAKEINTRESOURCE(IDI_SALAMANDER), IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
        if (hIconSm)
            SendMessage(HWindow, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);

        // Nastavit text tlacitka Write podle modu
        SetDlgItemText(HWindow, IDOK, ManageMode ? LoadStr(IDS_MCD_SAVEANDRESTART) : LoadStr(IDS_MCD_SAVEANDSTART));

        // Vytvorit fonty pro custom draw
        CreateFonts();

        // Nastavit property pro darkmode library - chceme vlastni custom draw
        HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
        SetPropW(hList, L"Salamander.DarkModeLib.CustomListView", (HANDLE)1);

        // Vytvorit tooltip control pro aktivni konfiguraci
        HToolTip = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_NOPREFIX | TTS_ALWAYSTIP,
                                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                  HWindow, NULL, HInstance, NULL);
        if (HToolTip != NULL)
        {
            TOOLINFO ti;
            memset(&ti, 0, sizeof(ti));
            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
            ti.hwnd = hList;
            ti.uId = (UINT_PTR)hList;
            ti.hinst = HInstance;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            SendMessage(HToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti);
            SendMessage(HToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 400);
            SendMessage(HToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 10000);
        }

        return ret;
    }

    case WM_DESTROY:
        if (SyncNameAttentionActive)
        {
            KillTimer(HWindow, 1);
            SyncNameAttentionActive = FALSE;
        }
        break;

    case WM_NOTIFY:
    {
        NMHDR* nmhdr = (NMHDR*)lParam;

        // TTN_GETDISPINFO prihazi z tooltip controlu
        if (nmhdr->code == TTN_GETDISPINFO || nmhdr->code == TTN_GETDISPINFOA || nmhdr->code == TTN_GETDISPINFOW)
        {
            NMTTDISPINFO* pDispInfo = (NMTTDISPINFO*)lParam;
            HWND hList = GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST);
            DWORD msgPos = GetMessagePos();
            POINT pt;
            pt.x = GET_X_LPARAM(msgPos);
            pt.y = GET_Y_LPARAM(msgPos);
            ScreenToClient(hList, &pt);
            LVHITTESTINFO ht;
            ht.pt = pt;
            int hitItem = ListView_HitTest(hList, &ht);
            int hitConfigIndex = MCDGetConfigIndexFromListItem(hList, hitItem);
            if (hitConfigIndex >= 0 && IsConfigActive(hitConfigIndex))
            {
                strncpy_s(pDispInfo->szText, sizeof(pDispInfo->szText) / sizeof(pDispInfo->szText[0]),
                          LoadStr(IDS_MCD_ACTIVECONFIGTOOLTIP), _TRUNCATE);
            }
            else
            {
                pDispInfo->szText[0] = 0;
            }
            break;
        }
        // LVN_GETDISPINFO - z LVS_EX_INFOTIP (prvni sloupec)
        if (nmhdr->code == LVN_GETINFOTIP)
        {
            NMLVGETINFOTIP* pInfoTip = (NMLVGETINFOTIP*)lParam;
            int configIndex = MCDGetConfigIndexFromListItem(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST), pInfoTip->iItem);
            if (configIndex >= 0 && IsConfigActive(configIndex))
            {
                strncpy_s(pInfoTip->pszText, pInfoTip->cchTextMax,
                          LoadStr(IDS_MCD_ACTIVECONFIGTOOLTIP), _TRUNCATE);
            }
            break;
        }

        if (nmhdr->hwndFrom == ListView_GetHeader(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST)) &&
            nmhdr->code == HDN_ITEMCLICK)
        {
            NMHEADER* nmh = (NMHEADER*)lParam;
            if (nmh->iItem >= 0 && nmh->iItem <= 5)
            {
                if (SortColumn == nmh->iItem)
                    SortAscending = !SortAscending;
                else
                {
                    SortColumn = nmh->iItem;
                    SortAscending = TRUE;
                }
                SortConfigs();
                PopulateConfigsList();
            }
            return TRUE;
        }

        if (nmhdr->idFrom == IDC_MCD_CONFIGS_LIST)
        {
            if (nmhdr->code == NM_CUSTOMDRAW)
            {
                NMLVCUSTOMDRAW* nmlvcd = (NMLVCUSTOMDRAW*)lParam;
                if (nmlvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
                {
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                    return TRUE;
                }
                if (nmlvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT)
                {
                    int configIndex = MCDGetConfigIndexFromListItem(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST),
                                                                       (int)nmlvcd->nmcd.dwItemSpec);
                    HFONT hFont = GetConfigFont(configIndex, nmlvcd->nmcd.uItemState);
                    SelectObject(nmlvcd->nmcd.hdc, hFont);
                    nmlvcd->clrTextBk = CLR_DEFAULT;
                    SetWindowLongPtr(HWindow, DWLP_MSGRESULT, CDRF_NEWFONT);
                    return TRUE;
                }
                SetWindowLongPtr(HWindow, DWLP_MSGRESULT, CDRF_DODEFAULT);
                return TRUE;
            }
            else if (nmhdr->code == LVN_ITEMCHANGED)
            {
                NMLISTVIEW* nmlv = (NMLISTVIEW*)lParam;
                if ((nmlv->uNewState & LVIS_SELECTED) && !(nmlv->uOldState & LVIS_SELECTED))
                {
                    SelectedSourceIndex = MCDGetConfigIndexFromListItem(GetDlgItem(HWindow, IDC_MCD_CONFIGS_LIST), nmlv->iItem);
                    UpdateSourcePanel();
                    UpdateDeleteButtonState();
                }
                else if (!(nmlv->uNewState & LVIS_SELECTED) && (nmlv->uOldState & LVIS_SELECTED))
                {
                    SelectedSourceIndex = -1;
                    UpdateSourcePanel();
                    UpdateDeleteButtonState();
                }
            }
        }
        break;
    }

    case WM_TIMER:
        if (wParam == 1)
        {
            static BOOL highlighted = FALSE;
            highlighted = !highlighted;
            char syncText[100];
            _snprintf_s(syncText, _TRUNCATE, highlighted ? ">> %s <<" : "%s", LoadStr(IDS_MCD_SYNC_NAME));
            SetDlgItemText(HWindow, IDC_MCD_SYNC_NAME, syncText);
            InvalidateRect(GetDlgItem(HWindow, IDC_MCD_SYNC_NAME), NULL, TRUE);
            return TRUE;
        }
        break;

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDC_MCD_DELETE_SEL:
            OnDeleteSelected();
            return TRUE;

        case IDC_MCD_EXPORT:
            OnExport();
            return TRUE;

        case IDC_MCD_IMPORT:
            OnImport();
            return TRUE;

        case IDC_MCD_FILE_BROWSE:
            OnBrowseFile();
            return TRUE;

        case IDC_MCD_SYNC_NAME:
            if (HIWORD(wParam) == BN_CLICKED)
                OnSyncName();
            return TRUE;

        case IDC_MCD_DELETE_SOURCE_AFTER:
            return TRUE;

        case IDC_MCD_SRC_LANG:
            return TRUE;

        case IDC_MCD_REG_RADIO:
        case IDC_MCD_FILE_RADIO:
            if (HIWORD(wParam) == BN_CLICKED)
                OnStorageRadioChanged();
            return TRUE;
        }
        break;
    }
    }
    return CCommonDialog::DialogProc(uMsg, wParam, lParam);
}
