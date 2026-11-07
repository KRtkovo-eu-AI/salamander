// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "splitcbn.h"
#include "splitcbn.rh"
#include "splitcbn.rh2"
#include "lang\lang.rh"
#include "split.h"
#include "dialogs.h"

// *****************************************************************************
//
//  SplitFile
//

#define MAX_CMDLINE 127 // maximum command line length
#define MAX_FILENAME 34 // maximum file name length so that even the minimal copy command still fits on the line
#define MAX_BAT 100000

#define BUFSIZE1 (128 * 1024) // buffer size for a removable drive (kept small to check ESC presses)
#define BUFSIZE2 (256 * 1024) // size for the others

static BOOL EnsureDiskInsertedEtc(const char* targetDir, CQuadWord& qwPartSize, CQuadWord* freeSpace,
                                  UINT driveType, CQuadWord& bytesRemaining, CQuadWord* thisPartSize, HWND parent)
{
    CALL_STACK_MESSAGE1("EnsureDiskInserted()");
    *freeSpace = CQuadWord(1, 0);
    if (driveType == DRIVE_REMOVABLE)
    {
        while (1)
        {
            const DWORD attributes = GetFileAttributesW(SplitCBNPaths::ApiPath(targetDir).c_str());
            const DWORD err = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_SUCCESS;
            if (err == ERROR_SUCCESS && qwPartSize == SIZE_AUTODETECT)
            {
                SplitCBNPaths::DiskSpace(freeSpace, targetDir);
                if (*freeSpace == CQuadWord(-1, -1))
                {
                    SalamanderGeneral->ShowMessageBox(LoadStr(IDS_OUTOFSPACE), LoadStr(IDS_SPLIT), MSGBOX_ERROR);
                    return FALSE;
                }
            }

            if (err != ERROR_SUCCESS || *freeSpace == CQuadWord(0, 0))
            {
                if (SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_INSERTDISK),
                                                     LoadStr(IDS_SPLIT), MB_OKCANCEL | MB_ICONINFORMATION) == IDCANCEL)
                    return FALSE;
            }
            else
                break;
        }
    }

    if (driveType != DRIVE_REMOVABLE || qwPartSize != SIZE_AUTODETECT)
        SplitCBNPaths::DiskSpace(freeSpace, targetDir);

    if (qwPartSize == SIZE_AUTODETECT)
    {
        *thisPartSize = (*freeSpace < bytesRemaining) ? *freeSpace : bytesRemaining;
    }
    else
    {
        *thisPartSize = (qwPartSize < bytesRemaining) ? qwPartSize : bytesRemaining;
        while (*freeSpace < *thisPartSize)
        {
            if (driveType != DRIVE_REMOVABLE)
            {
                SalamanderGeneral->ShowMessageBox(LoadStr(IDS_OUTOFSPACE), LoadStr(IDS_SPLIT), MSGBOX_ERROR);
                return FALSE;
            }
            else
            {
                if (SalamanderGeneral->ShowMessageBox(LoadStr(IDS_INSERTDISK), LoadStr(IDS_SPLIT),
                                                      MSGBOX_EX_ERROR) == IDCANCEL)
                    return FALSE;
                SplitCBNPaths::DiskSpace(freeSpace, targetDir);
            }
        }
    }
    return TRUE;
}

static BOOL SplitFile(const char* fileName, const char* targetDir, CQuadWord& qwPartSize,
                      HWND parent, CSalamanderForOperationsAbstract* salamander)
{
    CALL_STACK_MESSAGE4("SplitFile(%s, %s, %I64u, , )", fileName, targetDir, qwPartSize.Value);

    // Allocate owned scratch before opening any source/output handles.
    SplitCBNPaths::Buffer nameStorage(SplitCBNPaths::Capacity), partStorage(SplitCBNPaths::Capacity);
    SplitCBNPaths::Buffer pathStorage(2 * SplitCBNPaths::Capacity), progressStorage(2 * SplitCBNPaths::Capacity);
    SplitCBNPaths::Buffer batchStorage(configCreateBatchFile ? MAX_BAT : 1);
    SplitCBNPaths::Buffer lineStorage(configCreateBatchFile ? 8 * SplitCBNPaths::Capacity : 1);
    char* name = nameStorage.data();
    char* name2 = partStorage.data();
    char* text = pathStorage.data();
    char* text2 = progressStorage.data();

    // create the target path
    DWORD silent = 0;
    BOOL bSkip;
    if (SalamanderSafeFile->SafeFileCreate(targetDir, 0, 0, 0, TRUE, parent, NULL, NULL, &silent,
                                           TRUE, &bSkip, NULL, 0, NULL, NULL) == INVALID_HANDLE_VALUE)
        return FALSE;

    // open the file
    SAFE_FILE file;
    CSplitCBNSafeFileScope fileScope(file);
    if (!SalamanderSafeFile->SafeFileOpen(&file, fileName, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
                                          FILE_FLAG_SEQUENTIAL_SCAN, parent, BUTTONS_RETRYCANCEL, NULL, NULL))
    {
        return FALSE;
    }
    // obtain the file size
    CQuadWord bytesRemaining;
    bytesRemaining.LoDWord = GetFileSize(file.HFile, &bytesRemaining.HiDWord);

    // obtain the file timestamp
    FILETIME ft;
    GetFileTime(file.HFile, NULL, NULL, &ft);

    // obtain the base name of the files
    strcpy(name, SalamanderGeneral->SalPathFindFileName(fileName));
    if (!configIncludeFileExt)
        StripExtension(name);

    // determine the type of the target media
    const std::wstring root = SplitCBNPaths::Root(SplitCBNPaths::Wide(targetDir));
    UINT driveType = GetDriveTypeW(root.c_str());

    BOOL abort = FALSE;

    // check whether the user selected Autodetect on a fixed drive
    if (driveType != DRIVE_REMOVABLE && qwPartSize == SIZE_AUTODETECT)
    {
        if (SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_FIXEDNOSENSE),
                                             LoadStr(IDS_SPLIT), MB_YESNO | MB_ICONQUESTION) == IDNO)
            abort = TRUE;
    }

    // check for more than 100 parts
    if (!qwPartSize.Value || (bytesRemaining - CQuadWord(1, 0)) / qwPartSize + CQuadWord(1, 0) > CQuadWord(100, 0))
    {
        if (SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_TOOMANYPARTS),
                                             LoadStr(IDS_SPLIT), MB_YESNO | MB_ICONQUESTION) == IDNO)
            abort = TRUE;
    }

    // if we split to a fixed disk, verify that there is free space (ignore the batch file)
    if (!abort && driveType != DRIVE_REMOVABLE &&
        !TestTargetSpace(parent, targetDir, bytesRemaining, IDS_SPLIT))
        abort = TRUE;

    // TODO! more accurate test
    // check whether the name is not too long (the batch file might not work)
    if (!abort && configCreateBatchFile && strlen(name) > MAX_FILENAME)
    {
        if (SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_TOOLONGNAME),
                                             LoadStr(IDS_SPLIT), MB_YESNO | MB_ICONQUESTION) == IDNO)
            abort = TRUE;
    }

    if (abort)
    {
        SalamanderSafeFile->SafeFileClose(&file);
        return TRUE;
    }

    // allocate the buffer
    DWORD dwBufSize = (driveType == DRIVE_REMOVABLE) ? BUFSIZE1 : BUFSIZE2;
#ifdef new
#define SPLITCBN_RESTORE_NEW
#undef new
#endif
    char* pBuffer = new (std::nothrow) char[dwBufSize];
#ifdef SPLITCBN_RESTORE_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef SPLITCBN_RESTORE_NEW
#endif
    if (pBuffer == NULL)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_OUTOFMEM), LoadStr(IDS_SPLIT), MSGBOX_ERROR);
        SalamanderSafeFile->SafeFileClose(&file);
        return FALSE;
    }

    std::unique_ptr<char[]> bufferOwner(pBuffer);

    // init CRC
    UINT32 Crc = 0;

    // progress dialog
    CSplitCBNProgressScope progressScope(salamander);
    salamander->OpenProgressDialog(LoadStr(IDS_SPLIT), TRUE, NULL, FALSE);
    progressScope.Started();
    salamander->ProgressSetTotalSize(CQuadWord(-1, -1), bytesRemaining);
    BOOL delayed = (driveType != DRIVE_REMOVABLE); // splitting to floppies allegedly failed to repaint the dialog fast enough
    salamander->ProgressSetSize(CQuadWord(-1, -1), CQuadWord(0, 0), delayed);
    CQuadWord totalProgress = CQuadWord(0, 0), fileProgress;

    // from now on the progress dialog is the parent
    parent = SalamanderGeneral->GetMsgBoxParent();

    BOOL ret = TRUE;
    int partNum = 1;
    char buf[50];
    CQuadWord thisPartSize;
    CQuadWord freeSpace;

    while (bytesRemaining.Value)
    {
        if (!EnsureDiskInsertedEtc(targetDir, qwPartSize, &freeSpace, driveType, bytesRemaining, &thisPartSize, parent))
        {
            ret = FALSE;
            break;
        }

        // create the name of the target file
        sprintf(name2, "%s.%03d", name, partNum++);
        if (!SplitCBNPaths::Copy(pathStorage, SplitCBNPaths::Join(targetDir, name2)))
        {
            ret = FALSE;
            Error(IDS_SPLIT, IDS_TOOLONGNAME2);
            break;
        }

        // create the file
        GetInfo(buf, thisPartSize);
        SAFE_FILE outfile;
        CSplitCBNSafeFileScope outfileScope(outfile);
        if (SalamanderSafeFile->SafeFileCreate(text, GENERIC_WRITE, FILE_SHARE_READ, FILE_ATTRIBUTE_NORMAL,
                                               FALSE, parent, name2, buf, &silent, TRUE, &bSkip, NULL, 0, NULL, &outfile) == INVALID_HANDLE_VALUE &&
            !bSkip)
        {
            ret = FALSE;
            break;
        }

        // finally: copy thisPartSize bytes of the input file to the output file
        salamander->ProgressSetTotalSize(thisPartSize, CQuadWord(-1, -1));
        salamander->ProgressSetSize(CQuadWord(0, 0), CQuadWord(-1, -1), delayed);
        fileProgress = CQuadWord(0, 0);
        if (!bSkip)
        {
            sprintf(text2, "%s %s...", LoadStr(IDS_WRITING), name2);
            salamander->ProgressDialogAddText(text2, delayed);

            CQuadWord numBytes = thisPartSize;
            while (numBytes.Value)
            {
                DWORD toread = (numBytes > CQuadWord(dwBufSize, 0)) ? dwBufSize : numBytes.LoDWord;
                DWORD numread, numwr;
                if (!SalamanderSafeFile->SafeFileRead(&file, pBuffer, toread, &numread, parent, BUTTONS_RETRYCANCEL, NULL, NULL) ||
                    !SalamanderSafeFile->SafeFileWrite(&outfile, pBuffer, numread, &numwr, parent, BUTTONS_RETRYCANCEL, NULL, NULL))
                {
                    ret = FALSE;
                    break;
                }
                Crc = SalamanderGeneral->UpdateCrc32(pBuffer, numread, Crc);
                CQuadWord qwnr(numread, 0);
                numBytes -= qwnr;
                fileProgress += qwnr;
                if (!salamander->ProgressSetSize(fileProgress, totalProgress + fileProgress, delayed))
                {
                    ret = FALSE;
                    break;
                }
            }
            SalamanderSafeFile->SafeFileClose(&outfile);
            if (ret == FALSE)
            {
                DeleteFileW(SplitCBNPaths::ApiPath(text).c_str());
                break;
            }
        }
        else
        {
            CQuadWord distance = thisPartSize; // 'thisPartSize' must not change (use 'distance' instead)
            SalamanderSafeFile->SafeFileSeekMsg(&file, &distance, FILE_CURRENT, parent,
                                                BUTTONS_RETRYCANCEL, NULL, NULL, TRUE);
        }

        bytesRemaining -= thisPartSize;
        totalProgress += thisPartSize;

        // notify about inserting the next disk
        if (bytesRemaining.Value && driveType == DRIVE_REMOVABLE &&
            (qwPartSize == SIZE_AUTODETECT || freeSpace - qwPartSize < qwPartSize))
        {
            if (SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_INSERTNEXT),
                                                 LoadStr(IDS_SPLIT), MB_OKCANCEL | MB_ICONINFORMATION) == IDCANCEL)
            {
                ret = FALSE;
                break;
            }
        }
    }

    SalamanderSafeFile->SafeFileClose(&file);

    // create the batch file

    if (ret != FALSE && configCreateBatchFile)
    {
        char* batfile = batchStorage.data();
        char* line = lineStorage.data();
        int nparts = partNum - 1;
        const char* origName = SalamanderGeneral->SalPathFindFileName(fileName);

        SYSTEMTIME st;
        FileTimeToSystemTime(&ft, &st);
        const std::string metadataName = SplitCBNPaths::EscapeBatchName(origName, true);
        strcpy(batfile, metadataName.c_str());
        sprintf(line, LoadStr(IDS_BATFILE_DESCR), batfile);
        sprintf(batfile,
                "@echo off\r\n"
                "setlocal DisableDelayedExpansion\r\n"
                "chcp 65001 >nul\r\n"
                "rem encoding=utf8-cmd\r\n"
                "rem %s, https://www.altap.cz\r\n"
                "rem name=%s\r\n"
                "rem crc32=%X\r\n"
                "rem time=%d-%d-%d %d:%02d:%02d\r\n"
                "echo %s\r\n"
                "echo %s\r\n"
                "pause\r\n",
                LoadStr(IDS_BATFILE_GENBYSAL), metadataName.c_str(), Crc,
                (int)st.wYear, (int)st.wMonth, (int)st.wDay, (int)st.wHour, (int)st.wMinute, (int)st.wSecond,
                line, LoadStr(IDS_BATFILE_CTRL_C_TO_QUIT));

        const std::string commands = SplitCBNPaths::BatchCommands(name, origName, nparts);
        if (strlen(batfile) + commands.size() < MAX_BAT)
            strcat(batfile, commands.c_str());
        else
        {
            ret = FALSE;
            SalamanderGeneral->ShowMessageBox(LoadStr(IDS_BATTOOLONG), LoadStr(IDS_SPLIT), MSGBOX_ERROR);
        }

        if (ret)
        {
            CQuadWord batSize((DWORD)strlen(batfile), 0);
            bytesRemaining = batSize;
            thisPartSize = batSize;
            // Names are UTF-8; the batch selects that code page before using them.

            SplitCBNPaths::DiskSpace(&freeSpace, targetDir);
            if (driveType == DRIVE_REMOVABLE && freeSpace < batSize)
            { // "insert next disk"
                if (SalamanderGeneral->SalMessageBox(parent, LoadStr(IDS_INSERTNEXT),
                                                     LoadStr(IDS_SPLIT), MB_OKCANCEL | MB_ICONINFORMATION) == IDCANCEL)
                    ret = FALSE;
            }

            if (ret)
                if (EnsureDiskInsertedEtc(targetDir, batSize, &freeSpace, driveType, bytesRemaining,
                                          &thisPartSize, parent))
                {
                    strcpy(name2, name);
                    strcat(name2, ".bat");
                    if (!SplitCBNPaths::Copy(pathStorage, SplitCBNPaths::Join(targetDir, name2)))
                    {
                        progressScope.Close();
                        return Error(IDS_SPLIT, IDS_TOOLONGNAME2);
                    }

                    GetInfo(buf, batSize);
                    SAFE_FILE bf;
                    CSplitCBNSafeFileScope bfScope(bf);
                    if (SalamanderSafeFile->SafeFileCreate(text, GENERIC_WRITE, FILE_SHARE_READ, FILE_ATTRIBUTE_NORMAL,
                                                           FALSE, parent, name2, buf, &silent, TRUE, &bSkip, NULL, 0, NULL, &bf) != INVALID_HANDLE_VALUE &&
                        !bSkip)
                    {
                        sprintf(text, "%s %s", LoadStr(IDS_WRITING), name2);
                        salamander->ProgressDialogAddText(text, TRUE);
                        DWORD numw;
                        SalamanderSafeFile->SafeFileWrite(&bf, batfile, batSize.LoDWord, &numw, parent, BUTTONS_RETRYCANCEL, NULL, NULL);
                        SalamanderSafeFile->SafeFileClose(&bf);
                    }
                }
        }

    }

    progressScope.Close();
    SalamanderGeneral->PostChangeOnPathNotification(targetDir, FALSE);
    return ret;
}

// *****************************************************************************
//
//  SplitCommand
//

static BOOL SplitCommandImpl(HWND parent, CSalamanderForOperationsAbstract* salamander)
{
    CSalamanderDiskSelection selection;
    if (!selection.Capture(SalamanderGeneral, PANEL_SOURCE, SALDISKSELECTION_FOCUSED_ONLY))
        return Error(IDS_SPLIT, IDS_OPENERROR);
    const CSalamanderDiskSelectionItem* item = selection.GetItem(0);
    if (item == NULL || item->IsDir)
        return FALSE;
    const std::string path = SplitCBNPaths::Utf8(item->FullPathW);
    const std::string name = SplitCBNPaths::Utf8(item->NameW);
    const std::string sourceDirectory = SplitCBNPaths::Utf8(item->DirectoryW);
    std::string target = GetTargetDir(sourceDirectory.c_str(), name.c_str(), TRUE);

    // Open the actual item (also follows reparse points) to obtain the current size.
    HANDLE file = CreateFileW(SplitCBNPaths::ApiPath(path.c_str()).c_str(), FILE_READ_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return Error(IDS_SPLIT, IDS_OPENERROR);
    LARGE_INTEGER size;
    const BOOL haveSize = GetFileSizeEx(file, &size);
    const DWORD error = GetLastError();
    CloseHandle(file);
    if (!haveSize)
    {
        SetLastError(error);
        return Error(IDS_SPLIT, IDS_OPENERROR);
    }
    CQuadWord fileSize;
    fileSize.Value = size.QuadPart;
    if (fileSize.Value < 2)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_ZEROSIZE), LoadStr(IDS_SPLIT), MSGBOX_WARNING);
        return TRUE;
    }
    CQuadWord partSize;
    if (!SplitDialog(name.c_str(), fileSize, target, sourceDirectory.c_str(), &partSize, parent))
        return FALSE;
    const std::string targetRoot = GetTargetDir(sourceDirectory.c_str(), NULL, TRUE);
    if (!MakePathAbsolute(target, TRUE, targetRoot.c_str(), !configSplitToOther, IDS_SPLIT))
        return FALSE;
    return SplitFile(path.c_str(), target.c_str(), partSize, parent, salamander);
}

BOOL SplitCommand(HWND parent, CSalamanderForOperationsAbstract* salamander)
{
    CALL_STACK_MESSAGE1("SplitCommand( , )");
    try
    {
        return SplitCommandImpl(parent, salamander);
    }
    catch (const std::bad_alloc&)
    {
        SalamanderGeneral->ShowMessageBox(LoadStr(IDS_OUTOFMEM), LoadStr(IDS_SPLIT), MSGBOX_ERROR);
        return FALSE;
    }
}
