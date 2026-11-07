// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <string>

#include "dumpmem.h"
#include "array2.h"

#include "..\dll\pakiface.h"
#include "pak.rh"
#include "pak.rh2"
#include "lang\lang.rh"
#include "pak.h"

static HANDLE CreateFileLongPath(const char* fileName, DWORD desiredAccess, DWORD shareMode,
                                 DWORD creationDisposition, DWORD flagsAndAttributes)
{
    try
    {
        std::wstring fileNameW = SalamanderDiskSelection::WideFromPath(fileName);
        if (fileNameW.empty()) { SetLastError(ERROR_INVALID_NAME); return INVALID_HANDLE_VALUE; }
        if (fileNameW.length() >= MAX_PATH && wcsncmp(fileNameW.c_str(), L"\\\\?\\", 4) != 0)
            fileNameW = wcsncmp(fileNameW.c_str(), L"\\\\", 2) == 0 ? std::wstring(L"\\\\?\\UNC\\") + fileNameW.substr(2)
                                                                    : std::wstring(L"\\\\?\\") + fileNameW;
        return CreateFileW(fileNameW.c_str(), desiredAccess, shareMode, NULL,
                           creationDisposition, flagsAndAttributes, NULL);
    }
    catch (const std::bad_alloc&)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
}

DWORD ComputeDirDepth(const char* fileName)
{
    CALL_STACK_MESSAGE_NONE
    DWORD ret = 0;
    while (*fileName)
    {
        if (*fileName++ == '\\')
            ret++;
    }
    return ret;
}

BOOL CPluginInterfaceForArchiver::MakeFileList3(TIndirectArray2<CFileInfo>& files, BOOL* del, const char* archiveRoot, const char* sourcePath,
                                                SalEnumSelection2 next, void* nextParam)
{
    CALL_STACK_MESSAGE3("CPluginInterfaceForArchiver::MakeFileList3(, , %s, %s, , )",
                        archiveRoot, sourcePath);
    BOOL isDir;
    CQuadWord nextSize;
    const char* nextName;
    char pakName[PAK_MAXPATH];
    DWORD pakSize;
    char sourName[32768];
    BOOL skip;
    int errorOccured;

    ProgressTotal = CQuadWord(0, 0);
    *del = FALSE;

    while ((nextName = next(SalamanderGeneral->GetMsgBoxParent(), 3, NULL, &isDir, &nextSize, NULL,
                            NULL, nextParam, &errorOccured)) != NULL)
    {
        skip = FALSE;
        if (isDir)
            continue;
        lstrcpy(sourName, sourcePath);
        SalamanderGeneral->SalPathAppend(sourName, nextName, _countof(sourName));
        if (lstrlen(archiveRoot) + lstrlen(nextName) + 2 >= PAK_MAXPATH)
        {
            if (Silent & SF_LONGNAMES)
                continue;
            switch (SalamanderGeneral->DialogError(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_SKIPCANCEL, sourName, LoadStr(IDS_TOOLONGNAME2), NULL))
            {
            case DIALOG_SKIPALL:
                Silent |= SF_LONGNAMES;
            case DIALOG_SKIP:
                break;
            default:
                return FALSE;
            }
            continue;
        }
        lstrcpy(pakName, archiveRoot);
        SalamanderGeneral->SalPathAppend(pakName, nextName, PAK_MAXPATH);
        if (!PakIFace->FindFile(pakName, &pakSize))
            return FALSE;
        if (pakSize != -1 && (Silent & SF_SKIPALL))
            continue;
        if (pakSize != -1)
        {
            if (!(Silent & SF_OVEWRITEALL))
            {
                char name1[MAX_PATH + PAK_MAXPATH + 1];
                char data1[100];
                char data2[100];
                FILETIME ft;

                lstrcpy(name1, PakFileName);
                SalamanderGeneral->SalPathAppend(name1, pakName, MAX_PATH + PAK_MAXPATH + 1);
                PakIFace->GetPakTime(&ft);
                GetInfo(data1, &ft, pakSize);
                HANDLE file;
                file = INVALID_HANDLE_VALUE;
                while (file == INVALID_HANDLE_VALUE && !skip)
                {
                    file = CreateFileLongPath(sourName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
                    if (file != INVALID_HANDLE_VALUE)
                        break;
                    if (Silent & SF_IOERRORS)
                    {
                        skip = TRUE;
                        break;
                    }
                    char buf[1024];
                    lstrcpy(buf, LoadStr(IDS_ERROPEN));
                    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                                  GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf + lstrlen(buf), 1024 - lstrlen(buf), NULL);
                    switch (SalamanderGeneral->DialogError(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_RETRYSKIPCANCEL, sourName, buf, NULL))
                    {
                    case DIALOG_SKIPALL:
                        Silent |= SF_IOERRORS;
                    case DIALOG_SKIP:
                        skip = TRUE;
                        break;
                    case DIALOG_CANCEL:
                    case DIALOG_FAIL:
                        return FALSE;
                    }
                }
                if (skip)
                    continue;
                GetFileTime(file, NULL, NULL, &ft);
                CloseHandle(file);
                GetInfo(data2, &ft, (unsigned)nextSize.Value);
                // CONFIRM FILE OVERWRITE: filename1+filedata1+filename2+filedata2, buttons yes/all/skip/skip all/cancel
                switch (SalamanderGeneral->DialogOverwrite(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_YESALLSKIPCANCEL, name1, data1, sourName, data2))
                {
                case DIALOG_ALL:
                    Silent |= SF_OVEWRITEALL;
                case DIALOG_YES:
                    break;
                case DIALOG_SKIPALL:
                    Silent |= SF_SKIPALL;
                case DIALOG_SKIP:
                    skip = TRUE;
                    break;
                case DIALOG_CANCEL:
                case DIALOG_FAIL:
                    return FALSE;
                }
                if (skip)
                    continue;
            }
            if (!PakIFace->MarkForDelete())
                return FALSE;
            *del = TRUE;
        }
        if (nextSize > CQuadWord(0x80000000, 0))
        {
            if (Silent & SF_LARGE)
                continue;
            switch (SalamanderGeneral->DialogError(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_SKIPCANCEL, sourName, LoadStr(IDS_TOOLARGE), NULL))
            {
            case DIALOG_SKIPALL:
                Silent |= SF_LARGE;
            case DIALOG_SKIP:
                skip = TRUE;
                break;
            case DIALOG_CANCEL:
            case DIALOG_FAIL:
                return FALSE;
            }
        }
        if (skip)
            continue;
        CFileInfo* f;
        f = new CFileInfo(sourName, 0, ComputeDirDepth(sourName), (DWORD)nextSize.Value);
        if (!f || !f->Name)
        {
            if (f)
                delete f;
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_LOWMEM), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
            return FALSE;
        }
        if (!files.Add(f))
        {
            delete f;
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_LOWMEM), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
            return FALSE;
        }
        ProgressTotal += nextSize;
    }
    return errorOccured != SALENUM_CANCEL; // cancel aborts the operation
}

BOOL CPluginInterfaceForArchiver::DelFilesToBeOverwritten(unsigned* deleted)
{
    CALL_STACK_MESSAGE1("CPluginInterfaceForArchiver::DelFilesToBeOverwritten()");
    Salamander->ProgressDialogAddText(LoadStr(IDS_DELFILES), TRUE);
    unsigned total;
    if (!PakIFace->GetDelProgressTotal(&total))
        return FALSE;
    ProgressTotal += CQuadWord(total, 0);
    BOOL opt;
    if (!PakIFace->DeleteFiles(&opt))
    {
        if (opt)
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_NEEDOPT), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
        return FALSE;
    }
    *deleted = total;
    return TRUE;
}

BOOL CPluginInterfaceForArchiver::AddFiles(TIndirectArray2<CFileInfo>& files, unsigned deleted, const char* sourPath, const char* archiveRoot)
{
    CALL_STACK_MESSAGE4("CPluginInterfaceForArchiver::AddFiles(, 0x%X, %s, %s)", deleted,
                        sourPath, archiveRoot);
    Abort = TRUE;
    CFileInfo* f;
    char message[MAX_PATH + 32];
    BOOL skip;
    CQuadWord currentProgress = CQuadWord(deleted, 0);
    int i;
    for (i = 0; i < files.Count; i++)
    {
        skip = FALSE;
        f = files[i];
        lstrcpy(message, LoadStr(IDS_PACKING));
        lstrcat(message, f->Name);
        Salamander->ProgressDialogAddText(message, TRUE);
        IOFile = INVALID_HANDLE_VALUE;
        while (IOFile == INVALID_HANDLE_VALUE)
        {
            IOFile = CreateFileLongPath(f->Name /* + sourPathLen*/, GENERIC_READ, FILE_SHARE_READ,
                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
            if (IOFile != INVALID_HANDLE_VALUE)
                break;
            if (Silent & SF_IOERRORS)
            {
                skip = TRUE;
                goto l_next;
            }
            char buf[1024];
            lstrcpy(buf, LoadStr(IDS_ERROPEN));
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                          GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf + lstrlen(buf), 1024 - lstrlen(buf), NULL);
            switch (SalamanderGeneral->DialogError(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_RETRYSKIPCANCEL, f->Name, buf, NULL))
            {
            case DIALOG_SKIPALL:
                Silent |= SF_IOERRORS;
            case DIALOG_SKIP:
                skip = TRUE;
                goto l_next;
            case DIALOG_CANCEL:
            case DIALOG_FAIL:
                return FALSE;
            }
        }
        IOFileName = f->Name;
        if (!Salamander->ProgressSetSize(CQuadWord(0, 0), CQuadWord(-1, -1), TRUE))
            return FALSE;
        Salamander->ProgressSetTotalSize(CQuadWord(f->Size, 0), ProgressTotal);
        BOOL ret;
        char pakName[PAK_MAXPATH];
        lstrcpy(pakName, archiveRoot);
        char* root;
        root = f->Name + lstrlen(sourPath);
        if (*root == '\\')
            root++;
        SalamanderGeneral->SalPathAppend(pakName, root, PAK_MAXPATH);
        ret = PakIFace->AddFile(pakName, f->Size);
        CloseHandle(IOFile);
        if (!ret && Abort)
            return FALSE;
        f->Status = STATUS_OK;

    l_next:
        currentProgress += CQuadWord(f->Size, 0);
        if (!Salamander->ProgressSetSize(CQuadWord(f->Size, 0), currentProgress, TRUE))
            return FALSE;
    }
    return TRUE;
}

//#define DIR_DEPTH(i) (((CFileInfo *)(*files)[i])->DirDepth)

void SortByDirDepth(int left, int right, TIndirectArray2<CFileInfo>& files)
{
    CALL_STACK_MESSAGE_NONE
    int i = left, j = right;
    DWORD pivotOffset = files[(i + j) / 2]->DirDepth;
    do
    {
        while (files[i]->DirDepth <= pivotOffset && i < right)
            i++;
        while (pivotOffset <= files[j]->DirDepth && j > left)
            j--;
        if (i <= j)
        {
            CFileInfo* tmp = files[i];
            files[i] = files[j];
            files[j] = tmp;
            i++;
            j--;
        }
    } while (i <= j); //do they have to match?
    if (left < j)
        SortByDirDepth(left, j, files);
    if (i < right)
        SortByDirDepth(i, right, files);
}

void CPluginInterfaceForArchiver::DeleteSourceFiles(TIndirectArray2<CFileInfo>& files, const char* sourcePath)
{
    CALL_STACK_MESSAGE2("CPluginInterfaceForArchiver::DeleteSourceFiles(, %s)", sourcePath);
    if (files.Count == 0)
        return;
    CFileInfo* f;
    char path[MAX_PATH];
    SortByDirDepth(0, files.Count - 1, files);
    DWORD sourceDepth = ComputeDirDepth(sourcePath);

    int i;
    for (i = files.Count - 1; i >= 0; i--)
    {
        f = files[i];
        if (f->Status == STATUS_OK)
        {
            SalamanderGeneral->ClearReadOnlyAttr(f->Name);
            DeleteFile(f->Name);
            lstrcpy(path, f->Name);
            while (ComputeDirDepth(path) > sourceDepth + 1)
            {
                SalamanderGeneral->CutDirectory(path);
                if (!RemoveDirectory(path))
                    break;
            }
        }
    }
}

BOOL CPluginInterfaceForArchiver::PackToArchive(CSalamanderForOperationsAbstract* salamander, const char* fileName,
                                                const char* archiveRoot, BOOL move, const char* sourcePath,
                                                SalEnumSelection2 next, void* nextParam)
{
    CALL_STACK_MESSAGE5("CPluginInterfaceForArchiver::PackToArchive(, %s, %s, %d, %s, ,)", fileName,
                        archiveRoot, move, sourcePath);

    PakIFace = PAKGetIFace();
    if (!PakIFace)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_LOWMEM), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        return FALSE;
    }

    BOOL ret = TRUE;
    Salamander = salamander;
    PakFileName = fileName;
    Silent = 0;

    CPakCallbacks pakCalls(this);
    PakIFace->Init(&pakCalls);
    char title[1024];
    sprintf(title, LoadStr(IDS_ADDPROGTITLE), SalamanderGeneral->SalPathFindFileName(PakFileName));
    Salamander->OpenProgressDialog(title, TRUE, NULL, FALSE);
    Salamander->ProgressDialogAddText(LoadStr(IDS_PREPAREDATA), FALSE);

    if (!PakIFace->OpenPak(fileName, OP_WRITE_MODE))
        ret = FALSE;
    else
    {
        TIndirectArray2<CFileInfo> files(256);
        BOOL del;
        const char* arcRoot = archiveRoot ? archiveRoot : "";
        if (*arcRoot == '\\')
            arcRoot++;
        if (!MakeFileList3(files, &del, arcRoot, sourcePath, next, nextParam))
            ret = FALSE;
        else
        {
            unsigned deleted;
            if (del && !DelFilesToBeOverwritten(&deleted))
                ret = FALSE;
            else
            {
                if (!PakIFace->StartAdding(files.Count))
                    ret = FALSE;
                else
                {
                    Salamander->ProgressDialogAddText(LoadStr(IDS_ADDFILES), TRUE);
                    if (!AddFiles(files, deleted, sourcePath, arcRoot))
                        ret = FALSE;
                    ret = PakIFace->FinalizeAdding() && ret;
                    if (ret && move)
                        DeleteSourceFiles(files, sourcePath);
                }
            }
        }
        PakIFace->ClosePak();
    }

    Salamander->CloseProgressDialog();

    PAKReleaseIFace(PakIFace);
    return ret;
}

BOOL CPluginInterfaceForArchiver::DeleteFiles(const char* archiveRoot, SalEnumSelection next, void* nextParam)
{
    CALL_STACK_MESSAGE2("CPluginInterfaceForArchiver::DeleteFiles(%s, , )", archiveRoot);
    const char* nextName;
    char nextFull[PAK_MAXPATH];
    int len;
    BOOL isDir;
    char file[PAK_MAXPATH];
    DWORD size;
    unsigned total;

    const char* arcRoot = archiveRoot ? archiveRoot : "";
    if (*arcRoot == '\\')
        arcRoot++;

    while ((nextName = next(NULL, 0, &isDir, NULL, NULL, nextParam, NULL)) != NULL)
    {
        lstrcpy(nextFull, arcRoot);
        SalamanderGeneral->SalPathAppend(nextFull, nextName, PAK_MAXPATH);
        len = lstrlen(nextFull);
        if (!PakIFace->GetFirstFile(file, &size))
            return FALSE;
        while (*file)
        {
            if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, nextFull, len, file, len) == CSTR_EQUAL &&
                (file[len] == 0 || file[len] == '\\'))
            {
                if (!PakIFace->MarkForDelete())
                    return FALSE;
                if (!isDir)
                    break;
            }
            if (!PakIFace->GetNextFile(file, &size))
                return FALSE;
        }
    }
    Salamander->ProgressDialogAddText(LoadStr(IDS_DELFILES), TRUE);
    if (!PakIFace->GetDelProgressTotal(&total))
        return FALSE;
    ProgressTotal = CQuadWord(total, 0);
    BOOL opt;
    if (!PakIFace->DeleteFiles(&opt))
    {
        if (opt)
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_NEEDOPT), LoadStr(IDS_PLUGINNAME), MSGBOX_INFO);
        return FALSE;
    }
    return TRUE;
}

BOOL CPluginInterfaceForArchiver::DeleteFromArchive(CSalamanderForOperationsAbstract* salamander, const char* fileName,
                                                    CPluginDataInterfaceAbstract* pluginData, const char* archiveRoot,
                                                    SalEnumSelection next, void* nextParam)
{
    CALL_STACK_MESSAGE3("CPluginInterfaceForArchiver::DeleteFromArchive(, %s, , %s, ,)", fileName, archiveRoot);

    PakIFace = PAKGetIFace();
    if (!PakIFace)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_LOWMEM), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        return FALSE;
    }

    BOOL ret = TRUE;
    Salamander = salamander;
    PakFileName = fileName;

    CPakCallbacks pakCalls(this);
    PakIFace->Init(&pakCalls);
    char title[1024];
    sprintf(title, LoadStr(IDS_DELPROGTITLE), SalamanderGeneral->SalPathFindFileName(PakFileName));
    Salamander->OpenProgressDialog(title, TRUE, NULL, FALSE);
    Salamander->ProgressDialogAddText(LoadStr(IDS_PREPAREDATA), FALSE);

    if (!PakIFace->OpenPak(fileName, OP_WRITE_MODE))
        ret = FALSE;
    else
    {
        Salamander->ProgressDialogAddText(LoadStr(IDS_EXTRACTFILES), FALSE);
        if (!DeleteFiles(archiveRoot, next, nextParam))
            ret = FALSE;
        PakIFace->ClosePak();
    }

    Salamander->CloseProgressDialog();

    PAKReleaseIFace(PakIFace);
    return ret;
}

BOOL CPakCallbacks::DelNotify(const char* fileName, unsigned fileProgressTotal)
{
    CALL_STACK_MESSAGE3("CPakCallbacks::DelNotify(%s, 0x%X)", fileName,
                        fileProgressTotal);
    char message[PAK_MAXPATH + 32];
    lstrcpy(message, LoadStr(IDS_DELETING));
    lstrcat(message, fileName);
    Plugin->Salamander->ProgressDialogAddText(message, TRUE);
    if (!Plugin->Salamander->ProgressSetSize(CQuadWord(0, 0), CQuadWord(-1, -1), TRUE))
    {
        Plugin->Salamander->ProgressDialogAddText(LoadStr(IDS_CANCELING), FALSE);
        Plugin->Salamander->ProgressEnableCancel(FALSE);
        return FALSE;
    }
    Plugin->Salamander->ProgressSetTotalSize(CQuadWord(fileProgressTotal, 0), Plugin->ProgressTotal);
    return TRUE;
}

BOOL CPakCallbacks::Read(void* buffer, DWORD size)
{
    CALL_STACK_MESSAGE2("CPakCallbacks::Read(, 0x%X)", size);
    if (size == 0)
        return TRUE;
    char buf[1024];
    DWORD pos;
    while (1)
    {
        pos = SetFilePointer(Plugin->IOFile, 0, NULL, FILE_CURRENT);
        if (pos != 0xFFFFFFFF)
            break;
        lstrcpy(buf, LoadStr(IDS_UNABLEGETFIELPOS));
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf + lstrlen(buf), 1024 - lstrlen(buf), NULL);
        if (Plugin->Silent & SF_IOERRORS)
        {
            Plugin->Abort = FALSE;
            return FALSE;
        }
        switch (SalamanderGeneral->DialogError(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_RETRYSKIPCANCEL, Plugin->IOFileName, buf, NULL))
        {
        case DIALOG_SKIPALL:
            Plugin->Silent |= SF_IOERRORS;
        case DIALOG_SKIP:
            Plugin->Abort = FALSE;
            return FALSE;
        case DIALOG_CANCEL:
        case DIALOG_FAIL:
            return FALSE;
        }
    }
    DWORD read;
    while (1)
    {
        if (ReadFile(Plugin->IOFile, buffer, size, &read, NULL) && size == read)
            return TRUE;
        lstrcpy(buf, LoadStr(IDS_UNABLEREAD));
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf + lstrlen(buf), 1024 - lstrlen(buf), NULL);
        if (Plugin->Silent & SF_IOERRORS)
        {
            Plugin->Abort = FALSE;
            return FALSE;
        }
        switch (SalamanderGeneral->DialogError(SalamanderGeneral->GetMsgBoxParent(), BUTTONS_RETRYSKIPCANCEL, Plugin->IOFileName, buf, NULL))
        {
        case DIALOG_SKIPALL:
            Plugin->Silent |= SF_IOERRORS;
        case DIALOG_SKIP:
            Plugin->Abort = FALSE;
            return FALSE;
        case DIALOG_CANCEL:
        case DIALOG_FAIL:
            return FALSE;
        }
        if (!SafeSeek(pos))
            return FALSE;
    }
    return TRUE;
}

DWORD
CPluginInterfaceForMenuExt::GetMenuItemState(int id, DWORD eventMask)
{
    CALL_STACK_MESSAGE3("CPluginInterfaceForMenuExt::GetMenuItemState(%d, 0x%X)",
                        id, eventMask);
    if (id != OPTIMIZE_MENUID)
        return 0;
    /*if ((eventMask & (MENU_EVENT_ARCHIVE_FOCUSED | MENU_EVENT_DISK)) ==
      (MENU_EVENT_ARCHIVE_FOCUSED | MENU_EVENT_DISK) || eventMask & MENU_EVENT_THIS_PLUGIN)*/
    if (eventMask & MENU_EVENT_DISK &&
            eventMask & (MENU_EVENT_ARCHIVE_FOCUSED | MENU_EVENT_FILES_SELECTED) ||
        eventMask & (MENU_EVENT_THIS_PLUGIN_ARCH))
        return MENU_ITEM_STATE_ENABLED;
    return 0;
}

static std::string OptimizeProgressTitle(const char* pattern, const char* name)
{
    std::string title;
    for (const char* p = pattern; p && *p;)
    {
        if (p[0] == '%' && p[1] == '%') { title += '%'; p += 2; }
        else if (p[0] == '%' && p[1] == 's') { if (name) title += name; p += 2; }
        else title += *p++;
    }
    return title;
}

BOOL CPluginInterfaceForMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent,
                                                 int id, DWORD eventMask)
{
    CALL_STACK_MESSAGE3("CPluginInterfaceForMenuExt::ExecuteMenuItem(, , %d, 0x%X)", id,
                        eventMask);
    if (id != OPTIMIZE_MENUID) return FALSE;
    try
    {
        CSalamanderDiskSelection selection;
        std::vector<std::string> paths, directories;
        if (eventMask & MENU_EVENT_DISK)
        {
            if (!selection.Capture(SalamanderGeneral, PANEL_SOURCE))
            {
                char error[1024];
                SalamanderGeneral->GetErrorText(GetLastError(), error, _countof(error));
                SalamanderGeneral->ShowMessageBox(error, LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
                return FALSE;
            }
            for (int i = 0; i < selection.GetCount(); ++i)
            {
                const CSalamanderDiskSelectionItem* item = selection.GetItem(i);
                if (item->IsDir) continue;
                paths.push_back(SalamanderDiskSelection::Utf8FromWide(item->FullPathW));
                directories.push_back(SalamanderDiskSelection::Utf8FromWide(item->DirectoryW));
            }
        }
        else
        {
            // Archive panels keep their existing archive-path contract.
            std::vector<char> panelPath(4 * SAL_MAX_PATH);
            char* archive = NULL;
            if (!SalamanderGeneral->GetPanelPath(PANEL_SOURCE, panelPath.data(),
                                                static_cast<int>(panelPath.size()), NULL, &archive) || !archive)
                return FALSE;
            *archive = 0;
            paths.push_back(panelPath.data());
            directories.push_back(SalamanderDiskSelection::Utf8FromWide(
                SalamanderDiskSelection::Detail::ParentPath(SalamanderDiskSelection::WideFromPath(panelPath.data())).c_str()));
        }
        SalamanderGeneral->SetUserWorkedOnPanelPath(PANEL_SOURCE);
        InterfaceForArchiver.Salamander = salamander;
        InterfaceForArchiver.PakIFace = PAKGetIFace();
        if (!InterfaceForArchiver.PakIFace)
        {
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_LOWMEM), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
            return FALSE;
        }
        CPakCallbacks pakCalls(&InterfaceForArchiver);
        struct PakScope
        {
            CPluginInterfaceForArchiver* Owner;
            bool Open = false, Progress = false;
            ~PakScope()
            {
                if (Progress) Owner->Salamander->CloseProgressDialog();
                if (Open) Owner->PakIFace->ClosePak();
                PAKReleaseIFace(Owner->PakIFace);
                Owner->PakIFace = NULL;
                Owner->PakFileName = NULL;
            }
        } scope = {&InterfaceForArchiver};
        InterfaceForArchiver.PakIFace->Init(&pakCalls);
        for (size_t i = 0; i < paths.size(); ++i)
        {
            const char* pakFile = paths[i].c_str();
            InterfaceForArchiver.PakFileName = pakFile;
            if (!InterfaceForArchiver.PakIFace->OpenPak(pakFile, OP_WRITE_MODE | OP_EXISTING_ONLY)) continue;
            scope.Open = true;
            COptDlgData dlgData;
            InterfaceForArchiver.PakIFace->GetOptimizedState(&dlgData.PakSize, &dlgData.ValData);
            if (DialogBoxParam(HLanguage, MAKEINTRESOURCE(IDD_OPTIMIZE), parent,
                               OptimizeDlgProc, (LPARAM)&dlgData) == IDOK)
            {
                unsigned progress;
                if (InterfaceForArchiver.PakIFace->InitOptimalization(&progress))
                {
                    const std::string title = OptimizeProgressTitle(LoadStr(IDS_OPTPROGTITLE),
                        SalamanderGeneral->SalPathFindFileName(pakFile));
                    salamander->OpenProgressDialog(title.c_str(), FALSE, NULL, FALSE);
                    scope.Progress = true;
                    salamander->ProgressDialogAddText(LoadStr(IDS_OPTIMIZING), FALSE);
                    salamander->ProgressSetTotalSize(CQuadWord(progress, 0), CQuadWord(-1, -1));
                    InterfaceForArchiver.PakIFace->OptimizePak();
                    salamander->CloseProgressDialog();
                    scope.Progress = false;
                }
                // Each archive may belong to a different directory in Branch View.
                SalamanderGeneral->PostChangeOnPathNotification(directories[i].c_str(), FALSE);
            }
            InterfaceForArchiver.PakIFace->ClosePak();
            scope.Open = false;
        }
        return TRUE;
    }
    catch (const std::bad_alloc&)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_LOWMEM), LoadStr(IDS_PLUGINNAME), MSGBOX_ERROR);
        return FALSE;
    }
}

BOOL WINAPI
CPluginInterfaceForMenuExt::HelpForMenuItem(HWND parent, int id)
{
    int helpID = 0;
    switch (id)
    {
    case OPTIMIZE_MENUID:
        helpID = IDH_OPTIMIZE;
        break;
    }
    if (helpID != 0)
        SalamanderGeneral->OpenHtmlHelp(parent, HHCDisplayContext, helpID, FALSE);
    return helpID != 0;
}

char* PrintDiskSize(char* buf, CQuadWord size)
{
    CALL_STACK_MESSAGE2("PrintDiskSize(, %g)", size.GetDouble());
    buf[0] = 0;

    char num[50];
    sprintf(buf, "%s bytes", SalamanderGeneral->NumberToStr(num, size));
    char* s = NULL;
    if (size.GetDouble() >= 1023.5)
    {
        char num2[100];
        sprintf(buf + lstrlen(buf), " (%s)", SalamanderGeneral->PrintDiskSize(num2, size, 0));
    }
    return buf;
}

INT_PTR WINAPI OptimizeDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("OptimizeDlgProc(, 0x%X, 0x%IX, 0x%IX)", uMsg, wParam,
                        lParam);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        // SalamanderGUI->ArrangeHorizontalLines(hDlg); // should be called, but we skip it here; there are no horizontal lines
        if (((COptDlgData*)lParam)->PakSize <= ((COptDlgData*)lParam)->ValData)
        {
            EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
            SendMessage(hDlg, DM_SETDEFID, IDCANCEL, 0);
        }
        char buf[100];
        SetDlgItemText(hDlg, IDC_VALIDSIZE, PrintDiskSize(buf, CQuadWord(((COptDlgData*)lParam)->ValData, 0)));
        SetDlgItemText(hDlg, IDC_TOTALSIZE, PrintDiskSize(buf, CQuadWord(((COptDlgData*)lParam)->PakSize, 0)));
        SetDlgItemText(hDlg, IDC_WASTEDSPACE, PrintDiskSize(buf, CQuadWord(((COptDlgData*)lParam)->PakSize - ((COptDlgData*)lParam)->ValData, 0)));

        HWND hParent = GetParent(hDlg);
        if (hParent != NULL)
            SalamanderGeneral->MultiMonCenterWindow(hDlg, hParent, TRUE);
        return FALSE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
            EndDialog(hDlg, IDOK);
            return FALSE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return FALSE;
        }
    }
    }
    return FALSE;
}
