// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | 2026-11-08 | Prevent copying and deleting from the protected archive pilot. | Keep the archive listing read-only. | | GPT-6.1 Sol (Codex) |
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-06 | Capture implicit archive destinations before callbacks and validation. | Preserve upstream fixes and Branch View path and lifetime contracts. | https://github.com/KRtkovo-eu-AI/salamander/pull/769 | GPT-6 (Codex) |
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "cfgdlg.h"
#include "mainwnd.h"
#include "usermenu.h"
#include "execute.h"
#include "plugins.h"
#include "fileswnd.h"
#include "dialogs.h"
#include "zip.h"
#include "pack.h"
#include "common/widepath.h"
#include "common/localizedtext.h"
#include "fileactiondisktarget.h"
#include "fileactionarchivesource.h"
#include "fileactionpackcontext.h"
#include "unpackvolumes.h"

//
// ****************************************************************************
// CFilesWindow
//

CPanelTmpEnumData::CPanelTmpEnumData()
{
    ExactFileSelection = FALSE;
    SourcePanel = NULL;
    ContextIsCurrent = NULL;
    Context = NULL;
    ErrGetFileSizeOfLnkTgtIgnAll = FALSE;
    Indexes = NULL;
    CurrentIndex = 0;
    IndexesCount = 0;
    ZIPPath = NULL;
    Dirs = NULL;
    Files = NULL;
    ArchiveDir = NULL;
    WorkPath[0] = 0;
    EnumLastDir = NULL;
    EnumLastIndex = 0;
    EnumLastPath[0] = 0;
    EnumTmpFileName[0] = 0;
    DiskDirectoryTree = NULL;
    EnumLastDosPath[0] = 0;
    EnumTmpDosFileName[0] = 0;
    FilesCountReturnedFromWP = 0;
}

CPanelTmpEnumData::~CPanelTmpEnumData()
{
    if (DiskDirectoryTree != NULL)
        delete DiskDirectoryTree;
}

void CPanelTmpEnumData::Reset()
{
    CurrentIndex = 0;
    EnumLastDir = NULL;
    EnumLastIndex = -1;
    EnumLastPath[0] = 0;
    EnumTmpFileName[0] = 0;
    EnumLastDosPath[0] = 0;
    EnumTmpDosFileName[0] = 0;
    FilesCountReturnedFromWP = 0;
}

const char* _PanelSalEnumSelection(int enumFiles, const char** dosName, BOOL* isDir, CQuadWord* size,
                                   const CFileData** fileData, void* param, HWND parent, int* errorOccured)
{
    SLOW_CALL_STACK_MESSAGE2("_PanelSalEnumSelection(%d, , , , , , ,)", enumFiles);
    CPanelTmpEnumData* data = (CPanelTmpEnumData*)param;
    if (dosName != NULL)
        *dosName = NULL;
    if (isDir != NULL)
        *isDir = FALSE;
    if (size != NULL)
        *size = CQuadWord(0, 0);
    if (fileData != NULL)
        *fileData = NULL;

    if (enumFiles == -1)
    {
        data->Reset();
        return NULL;
    }

    static char errText[1000];
    const char* curZIPPath;
    if (data->DiskDirectoryTree == NULL)
        curZIPPath = data->ZIPPath;
    else
        curZIPPath = "";

ENUM_NEXT:

    if (!data->HasCurrentContext())
    {
        SetLastError(ERROR_RETRY);
        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
        return NULL;
    }
    if (data->CurrentIndex >= data->IndexesCount)
        return NULL;
    if (enumFiles == 0)
    {
        int i = data->Indexes[data->CurrentIndex++];
        BOOL localIsDir = i < data->Dirs->Count;
        if (isDir != NULL)
            *isDir = localIsDir;
        CFileData* f = &(localIsDir ? data->Dirs->At(i) : data->Files->At(i - data->Dirs->Count));
        if (localIsDir)
        {
            if (size != NULL)
                *size = data->ArchiveDir->GetDirSize(data->ZIPPath, f->Name);
        }
        else
        {
            if (size != NULL)
                *size = f->Size;
            data->FilesCountReturnedFromWP++; // just in case someone changes 'enumFiles' between 0 and 3
        }
        if (fileData != NULL)
            *fileData = f;
        return f->Name;
    }
    else
    {
        if (data->EnumLastDir == NULL) // the directory needs to be "opened"
        {
            int i = data->Indexes[data->CurrentIndex];
            BOOL localIsDir = i < data->Dirs->Count;
            if (isDir != NULL)
                *isDir = localIsDir;
            CFileData* f = &(localIsDir ? data->Dirs->At(i) : data->Files->At(i - data->Dirs->Count));
            if (localIsDir)
            {
                int zipPathLen = (int)strlen(curZIPPath);
                if (zipPathLen + (zipPathLen > 0 ? 1 : 0) + f->NameLen >= SAL_MAX_PATH)
                { // path is too long
                    if (errorOccured != NULL)
                        *errorOccured = SALENUM_ERROR;
                    _snprintf_s(errText, _TRUNCATE, LoadStr(IDS_NAMEISTOOLONG), f->Name, curZIPPath);
                    if (parent != NULL &&
                        SalMessageBox(parent, errText, LoadStr(IDS_ERRORTITLE),
                                      MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                    {
                        if (errorOccured != NULL)
                            *errorOccured = SALENUM_CANCEL;
                        return NULL; // cancel, we are done
                    }
                    data->CurrentIndex++; // skip the directory with a long name and continue with another directory or file
                    goto ENUM_NEXT;
                }
                else
                {
                    memcpy(data->EnumLastPath, curZIPPath, zipPathLen);
                    if (zipPathLen > 0)
                        data->EnumLastPath[zipPathLen++] = '\\';
                    strcpy_s(data->EnumLastPath + zipPathLen, _countof(data->EnumLastPath) - zipPathLen, f->Name);
                    if (data->DiskDirectoryTree != NULL)
                    { // if EnumLastDosPath is used, zipPathLen will be 0
                        strcpy_s(data->EnumLastDosPath, _countof(data->EnumLastDosPath), (f->DosName == NULL) ? f->Name : f->DosName);
                    }
                    if (data->DiskDirectoryTree == NULL)
                        data->EnumLastDir = data->ArchiveDir;
                    else
                        data->EnumLastDir = data->DiskDirectoryTree;
                    data->EnumLastDir = data->EnumLastDir->GetSalamanderDir(data->EnumLastPath, TRUE);
                    if (data->EnumLastDir == NULL)
                        return NULL; // most likely the ".." directory, otherwise an unexpected error
                    data->EnumLastIndex = 0;
                    goto FIND_NEXT;
                }
            }
            else
            {
                if (size != NULL)
                {
                    *size = f->Size;
                    if (enumFiles == 3 && data->DiskDirectoryTree != NULL && (f->Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                    { // the determined size of the target file for the link must be taken from data->DiskDirectoryTree
                        CFileData const* f2 = data->DiskDirectoryTree->GetFile(data->FilesCountReturnedFromWP);
                        if (f2 != NULL && strcmp(f2->Name, f->Name) == 0) // always true
                            *size = f2->Size;
                        else
                        {
                            TRACE_E("_PanelSalEnumSelection(): unexpected situation: file \"" << f->Name << "\" not found or different in data->DiskDirectoryTree");
                        }
                    }
                }
                data->CurrentIndex++;
                if (data->DiskDirectoryTree != NULL)
                {
                    if (dosName != NULL)
                        *dosName = (f->DosName == NULL) ? f->Name : f->DosName;
                }
                if (fileData != NULL)
                    *fileData = f;
                data->FilesCountReturnedFromWP++;
                return f->Name;
            }
        }
        else
        {
            data->EnumLastIndex++; // move to the next item

        FIND_NEXT: // find the next file in the tree

            while (1)
            {
                if (!data->HasCurrentContext())
                {
                    SetLastError(ERROR_RETRY);
                    if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
                    return NULL;
                }
                if (data->EnumLastDir->IsDirectory(data->EnumLastIndex)) // directory -> descend
                {
                    CFileData* f = data->EnumLastDir->GetDirEx(data->EnumLastIndex);
                    BOOL tooLong1 = strlen(data->EnumLastPath) + 1 + f->NameLen >= SAL_MAX_PATH;
                    BOOL tooLong2 = data->DiskDirectoryTree != NULL && strlen(data->EnumLastDosPath) + 1 +
                                                                               (f->DosName == NULL ? f->NameLen : strlen(f->DosName)) >=
                                                                           SAL_MAX_PATH;
                    if (tooLong1 || tooLong2)
                    { // path is too long
                        if (errorOccured != NULL)
                            *errorOccured = SALENUM_ERROR;
                        _snprintf_s(errText, _TRUNCATE, LoadStr(IDS_NAMEISTOOLONG),
                                    (tooLong1 || f->DosName == NULL ? f->Name : f->DosName),
                                    (tooLong1 ? data->EnumLastPath : data->EnumLastDosPath));
                        if (parent != NULL &&
                            SalMessageBox(parent, errText, LoadStr(IDS_ERRORTITLE),
                                          MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                        {
                            if (errorOccured != NULL)
                                *errorOccured = SALENUM_CANCEL;
                            return NULL; // cancel, we are done
                        }
                        data->EnumLastIndex++; // skip the directory with a long name and continue with another directory or file (or exit the directory)
                    }
                    else
                    {
                        strcat_s(data->EnumLastPath, _countof(data->EnumLastPath), "\\");
                        strcat_s(data->EnumLastPath, _countof(data->EnumLastPath), f->Name);
                        if (data->DiskDirectoryTree != NULL)
                        {
                            strcat_s(data->EnumLastDosPath, _countof(data->EnumLastDosPath), "\\");
                            strcat_s(data->EnumLastDosPath, _countof(data->EnumLastDosPath), (f->DosName == NULL) ? f->Name : f->DosName);
                        }
                        data->EnumLastDir = data->EnumLastDir->GetSalamanderDir(data->EnumLastIndex);
                        data->EnumLastIndex = 0;
                    }
                }
                else
                {
                    if (data->EnumLastDir->IsFile(data->EnumLastIndex)) // file -> found
                    {
                        if (enumFiles == 2)
                            goto ENUM_NEXT; // no files from subdirectories, subdirectories are enough

                        CFileData* f = data->EnumLastDir->GetFileEx(data->EnumLastIndex);
                        int zipPathLen = (int)strlen(curZIPPath);
                        BOOL tooLong1 = strlen(data->EnumLastPath) - (zipPathLen + (zipPathLen > 0 ? 1 : 0)) +
                                            1 + f->NameLen >=
                                        SAL_MAX_PATH;
                        BOOL tooLong2 = data->DiskDirectoryTree != NULL && strlen(data->EnumLastDosPath) + 1 +
                                                                                   (f->DosName == NULL ? f->NameLen : strlen(f->DosName)) >=
                                                                               SAL_MAX_PATH;
                        if (tooLong1 || tooLong2)
                        { // path is too long
                            if (errorOccured != NULL)
                                *errorOccured = SALENUM_ERROR;
                            _snprintf_s(errText, _TRUNCATE, LoadStr(IDS_NAMEISTOOLONG),
                                        (tooLong1 || f->DosName == NULL ? f->Name : f->DosName),
                                        (tooLong1 ? data->EnumLastPath + zipPathLen + (zipPathLen > 0 ? 1 : 0) : data->EnumLastDosPath));
                            if (parent != NULL &&
                                SalMessageBox(parent, errText, LoadStr(IDS_ERRORTITLE),
                                              MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
                            {
                                if (errorOccured != NULL)
                                    *errorOccured = SALENUM_CANCEL;
                                return NULL; // cancel, we are done
                            }
                            data->EnumLastIndex++; // skip the file with a long name and continue with the next file (or exit the directory)
                        }
                        else
                        {
                            if (isDir != NULL)
                                *isDir = FALSE;
                            if (size != NULL)
                                *size = f->Size;
                            strcpy_s(data->EnumTmpFileName, _countof(data->EnumTmpFileName), data->EnumLastPath + zipPathLen + (zipPathLen > 0 ? 1 : 0));
                            strcat_s(data->EnumTmpFileName, _countof(data->EnumTmpFileName), "\\");
                            strcat_s(data->EnumTmpFileName, _countof(data->EnumTmpFileName), f->Name);
                            if (data->DiskDirectoryTree != NULL)
                            {
                                strcpy_s(data->EnumTmpDosFileName, _countof(data->EnumTmpDosFileName), data->EnumLastDosPath); // if used, zipPathLen == 0
                                strcat_s(data->EnumTmpDosFileName, _countof(data->EnumTmpDosFileName), "\\");
                                strcat_s(data->EnumTmpDosFileName, _countof(data->EnumTmpDosFileName), (f->DosName == NULL) ? f->Name : f->DosName);
                                if (dosName != NULL)
                                    *dosName = data->EnumTmpDosFileName;
                            }
                            if (fileData != NULL)
                                *fileData = f;
                            return data->EnumTmpFileName;
                        }
                    }
                    else // we are at the end of a directory -> exit
                    {
                        // split the path into directory and subdirectory
                        const char* dir;
                        char* subDir = strrchr(data->EnumLastPath, '\\');
                        char* subDirDos = NULL;
                        if (data->DiskDirectoryTree != NULL)
                            subDirDos = strrchr(data->EnumLastDosPath, '\\');
                        if (subDir != NULL)
                        {
                            dir = data->EnumLastPath;
                            *subDir++ = 0;
                            if (data->DiskDirectoryTree != NULL)
                                *subDirDos++ = 0;
                        }
                        else // we have definitely exited the tree
                        {
                            dir = "";
                            subDir = data->EnumLastPath;
                            if (data->DiskDirectoryTree != NULL)
                                subDirDos = data->EnumLastDosPath;
                        }
                        // check whether we have already exited the tree
                        if (strlen(dir) == strlen(curZIPPath))
                        {
                            if (fileData != NULL) // find CFileData for the subdirectory that we are exiting
                            {
                                int i = data->Indexes[data->CurrentIndex];
                                if (i < data->Dirs->Count)
                                    *fileData = &data->Dirs->At(i);
                                else
                                    TRACE_E("Unexpected situation in _PanelSalEnumSelection.");
                            }

                            data->CurrentIndex++; // we have already processed this directory
                            data->EnumLastDir = NULL;
                            data->EnumLastIndex = -1;

                            if (isDir != NULL)
                                *isDir = TRUE;
                            if (size != NULL)
                                *size = CQuadWord(0, 0);
                            if (data->DiskDirectoryTree != NULL)
                            {
                                if (dosName != NULL)
                                    *dosName = subDirDos;
                            }
                            return subDir; // return the directory when exiting
                        }
                        // finish exiting and move forward
                        if (data->DiskDirectoryTree == NULL)
                            data->EnumLastDir = data->ArchiveDir;
                        else
                            data->EnumLastDir = data->DiskDirectoryTree;
                        data->EnumLastDir = data->EnumLastDir->GetSalamanderDir(dir, TRUE);
                        data->EnumLastIndex = data->EnumLastDir->GetIndex(subDir);

                        if (fileData != NULL) // find CFileData for the subdirectory that we are exiting
                        {
                            *fileData = data->EnumLastDir->GetDirEx(data->EnumLastIndex);
                        }

                        if (isDir != NULL)
                            *isDir = TRUE;
                        if (size != NULL)
                            *size = CQuadWord(0, 0);
                        int zipPathLen = (int)strlen(curZIPPath);
                        strcpy_s(data->EnumTmpFileName, _countof(data->EnumTmpFileName), data->EnumLastPath + zipPathLen + (zipPathLen > 0 ? 1 : 0));
                        strcat_s(data->EnumTmpFileName, _countof(data->EnumTmpFileName), "\\");
                        strcat_s(data->EnumTmpFileName, _countof(data->EnumTmpFileName), subDir);
                        if (data->DiskDirectoryTree != NULL)
                        {
                            strcpy_s(data->EnumTmpDosFileName, _countof(data->EnumTmpDosFileName), data->EnumLastDosPath); // if used, zipPathLen == 0
                            strcat_s(data->EnumTmpDosFileName, _countof(data->EnumTmpDosFileName), "\\");
                            strcat_s(data->EnumTmpDosFileName, _countof(data->EnumTmpDosFileName), subDirDos);
                            if (dosName != NULL)
                                *dosName = data->EnumTmpDosFileName;
                        }
                        return data->EnumTmpFileName; // return the directory when exiting
                    }
                }
            }
        }
    }
    return NULL;
}

const char* WINAPI PanelSalEnumSelection(HWND parent, int enumFiles, BOOL* isDir, CQuadWord* size,
                                         const CFileData** fileData, void* param, int* errorOccured)
{
    CALL_STACK_MESSAGE_NONE
    if (errorOccured != NULL)
        *errorOccured = SALENUM_SUCCESS;
    if (enumFiles == 3)
    {
        TRACE_E("PanelSalEnumSelection(): invalid parameter: enumFiles==3, changing to 1");
        enumFiles = 1;
    }
    return _PanelSalEnumSelection(enumFiles, NULL, isDir, size, fileData, param, parent, errorOccured);
}

namespace
{
struct CDiskDirEnumEntry
{
    std::string Name;
    std::string DosName;
    BOOL IsDirectory;
    CQuadWord Size;
    DWORD Attributes;
    FILETIME LastWrite;
};

struct CDiskDirEnumData
{
    CFilesWindow* Panel;
    std::vector<int> Indexes;
    std::vector<CDiskDirEnumEntry> Entries;
    std::wstring OutputPath;
};

static std::wstring DiskDirFileNameW(const CFileData* file)
{
    if (file->UseWideName())
        return file->NameW;
    std::wstring name = SalMultiByteToWidePath(file->Name, CP_UTF8);
    if (name.empty())
        name = SalMultiByteToWidePath(file->Name, CP_ACP);
    return name;
}

static std::string DiskDirFileNameUtf8(const wchar_t* name)
{
    return SalWideToMultiBytePath(name, CP_UTF8);
}

static bool DiskDirSamePath(const std::wstring& first,
                            const std::wstring& second)
{
    return !first.empty() && !second.empty() &&
           CompareStringOrdinal(first.c_str(), -1, second.c_str(), -1,
                                TRUE) == CSTR_EQUAL;
}

static bool DiskDirAddEnumEntry(CDiskDirEnumData* data,
                                const std::wstring& relativeName,
                                const wchar_t* dosName, BOOL isDirectory,
                                const CQuadWord& size, DWORD attributes,
                                const FILETIME& lastWrite)
{
    CDiskDirEnumEntry entry;
    entry.Name = DiskDirFileNameUtf8(relativeName.c_str());
    if (entry.Name.empty())
        return false;
    if (dosName != NULL && dosName[0] != 0)
        entry.DosName = DiskDirFileNameUtf8(dosName);
    entry.IsDirectory = isDirectory;
    entry.Size = size;
    entry.Attributes = attributes;
    entry.LastWrite = lastWrite;
    data->Entries.push_back(entry);
    return true;
}

static bool DiskDirReportEnumerationError(HWND parent,
                                          const std::wstring& path,
                                          DWORD error)
{
    std::string pathUtf8 = DiskDirFileNameUtf8(path.c_str());
    const char* errorText = GetErrorText(error);
    std::string message = LoadStr(IDS_CANNOTREADDIR);
    size_t pathPos = message.find("%s");
    size_t errorPos = pathPos == std::string::npos
                          ? std::string::npos
                          : message.find("%s", pathPos + 2);
    if (errorPos != std::string::npos)
        message.replace(errorPos, 2, errorText);
    if (pathPos != std::string::npos)
        message.replace(pathPos, 2, pathUtf8);
    SalMessageBox(parent, message.c_str(), LoadStr(IDS_ERRORTITLE),
                  MB_OK | MB_ICONEXCLAMATION);
    return false;
}

static bool DiskDirEnumDirectory(HWND parent, const std::wstring& fullPath,
                                 const std::wstring& relativePath,
                                 CDiskDirEnumData* data)
{
    std::wstring searchPath = fullPath;
    if (!SalPathAppendW(searchPath, L"*"))
        return DiskDirReportEnumerationError(parent, fullPath,
                                             ERROR_FILENAME_EXCED_RANGE);
    searchPath = SalPathAddExtendedPrefixW(searchPath.c_str());

    WIN32_FIND_DATAW found = {};
    HANDLE find = FindFirstFileW(searchPath.c_str(), &found);
    if (find == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND || error == ERROR_NO_MORE_FILES)
            return true;
        return DiskDirReportEnumerationError(parent, fullPath, error);
    }

    bool result = true;
    do
    {
        if (found.cFileName[0] == L'.' &&
            (found.cFileName[1] == 0 ||
             (found.cFileName[1] == L'.' && found.cFileName[2] == 0)))
            continue;

        std::wstring childRelative = relativePath;
        std::wstring childFull = fullPath;
        if (!SalPathAppendW(childRelative, found.cFileName) ||
            !SalPathAppendW(childFull, found.cFileName))
        {
            result = DiskDirReportEnumerationError(
                parent, childFull, ERROR_FILENAME_EXCED_RANGE);
            break;
        }
        if (DiskDirSamePath(childFull, data->OutputPath))
            continue;

        BOOL isDirectory =
            (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        CQuadWord size(isDirectory ? 0 : found.nFileSizeLow,
                       isDirectory ? 0 : found.nFileSizeHigh);
        if (!DiskDirAddEnumEntry(data, childRelative,
                                 found.cAlternateFileName, isDirectory, size,
                                 found.dwFileAttributes, found.ftLastWriteTime))
        {
            result = false;
            break;
        }

        if (isDirectory &&
            (found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0 &&
            !DiskDirEnumDirectory(parent, childFull, childRelative, data))
        {
            result = false;
            break;
        }
    } while (FindNextFileW(find, &found));

    if (result)
    {
        DWORD error = GetLastError();
        if (error != ERROR_NO_MORE_FILES)
            result = DiskDirReportEnumerationError(parent, fullPath, error);
    }
    FindClose(find);
    return result;
}

static bool DiskDirBuildEnumeration(HWND parent, int enumFiles,
                                    CDiskDirEnumData* data)
{
    std::wstring panelPath = data->Panel->GetPathW();
    if (panelPath.empty())
    {
        panelPath = SalMultiByteToWidePath(data->Panel->GetPath(), CP_UTF8);
        if (panelPath.empty())
            panelPath = SalMultiByteToWidePath(data->Panel->GetPath(), CP_ACP);
    }
    if (panelPath.empty())
        return false;

    for (int index : data->Indexes)
    {
        BOOL isDirectory = index < data->Panel->Dirs->Count;
        CFileData* file =
            &(isDirectory
                  ? data->Panel->Dirs->At(index)
                  : data->Panel->Files->At(index - data->Panel->Dirs->Count));
        std::wstring name = DiskDirFileNameW(file);
        if (name.empty() || name == L"..")
            continue;

        std::wstring fullPath = data->Panel->GetItemFullPathW(*file);
        if (DiskDirSamePath(fullPath, data->OutputPath))
            continue;

        std::wstring dosName;
        if (file->DosName != NULL)
            dosName = SalMultiByteToWidePath(file->DosName, CP_ACP);
        if (!DiskDirAddEnumEntry(data, name,
                                 dosName.empty() ? NULL : dosName.c_str(),
                                 isDirectory, file->Size, file->Attr,
                                 file->LastWrite))
            return false;

        if (enumFiles > 0 && isDirectory &&
            (file->Attr & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
        {
            if (!DiskDirEnumDirectory(parent, fullPath, name, data))
                return false;
        }
    }
    return true;
}

static void DiskDirPrepareFileListEntry(const CDiskDirEnumEntry& entry,
                                        const std::string& panelPath,
                                        CFileData& file, std::string& name,
                                        std::string& path)
{
    size_t slash = entry.Name.find_last_of("\\/");
    name = slash == std::string::npos ? entry.Name : entry.Name.substr(slash + 1);
    path = panelPath;
    if (slash != std::string::npos)
    {
        if (!path.empty() && path.back() != '\\')
            path.push_back('\\');
        path.append(entry.Name, 0, slash);
    }

    file = {};
    file.Name = const_cast<char*>(name.c_str());
    file.NameLen = static_cast<int>(name.size());
    file.DosName = entry.DosName.empty()
                       ? NULL
                       : const_cast<char*>(entry.DosName.c_str());
    char* extension = strrchr(file.Name, '.');
    file.Ext = !entry.IsDirectory && extension != NULL
                   ? extension + 1
                   : file.Name + file.NameLen;
    file.Size = entry.Size;
    file.Attr = entry.Attributes;
    file.LastWrite = entry.LastWrite;
    file.IsLink = !entry.IsDirectory && IsFileLink(file.Ext);
    file.IconOverlayIndex = ICONOVERLAYINDEX_NOTUSED;
}
}

BOOL CFilesWindow::MakeFileListRecursive(HANDLE hFile,
                                         const char* outputFileName)
{
    CALL_STACK_MESSAGE2("CFilesWindow::MakeFileListRecursive(, %s)",
                        outputFileName);
    if (!Is(ptDisk) || outputFileName == NULL || FilesActionInProgress)
        return FALSE;

    CDiskDirEnumData data = {};
    data.Panel = this;
    data.OutputPath = SalMultiByteToWidePath(outputFileName, CP_UTF8);
    if (data.OutputPath.empty())
        data.OutputPath = SalMultiByteToWidePath(outputFileName, CP_ACP);
    data.OutputPath = SalPathRemoveExtendedPrefixW(data.OutputPath.c_str());
    int selectedCount = GetSelCount();
    if (selectedCount > 0)
    {
        data.Indexes.resize(selectedCount);
        GetSelItems(selectedCount, data.Indexes.data());
    }
    else
        data.Indexes.push_back(GetCaretIndex());

    BOOL upDir = Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0;
    if (data.Indexes[0] < 0 ||
        (upDir && data.Indexes.size() == 1 && data.Indexes[0] == 0))
        return FALSE;

    FilesActionInProgress = TRUE;
    BOOL result = FALSE;
    if (DiskDirBuildEnumeration(HWindow, 1, &data))
    {
        const DWORD validData =
            VALID_DATA_EXTENSION | VALID_DATA_SIZE | VALID_DATA_DATE |
            VALID_DATA_TIME | VALID_DATA_ATTRIBUTES | VALID_DATA_DOSNAME |
            VALID_DATA_ISLINK;
        std::string panelPath = SalWideToMultiBytePath(GetPathW(), CP_UTF8);
        if (panelPath.empty())
            panelPath = GetPath();

        int maxSizes[100];
        ZeroMemory(maxSizes, sizeof(maxSizes));
        std::vector<char> line(4 * SAL_MAX_PATH);
        result = TRUE;
        for (int pass = 0; pass < 2 && result; ++pass)
        {
            size_t entryIndex = 0;
            for (const CDiskDirEnumEntry& entry : data.Entries)
            {
                CFileData file;
                std::string name;
                std::string path;
                DiskDirPrepareFileListEntry(entry, panelPath, file, name, path);
                if (pass == 0)
                {
                    if (!ExpandMakeFileList(
                            HWindow, Configuration.FileListHistory[0],
                            &PluginData, &file, entry.IsDirectory, NULL, 0,
                            TRUE, maxSizes, _countof(maxSizes), validData,
                            path.c_str(), entryIndex != 0))
                    {
                        result = FALSE;
                        break;
                    }
                }
                else
                {
                    if (!ExpandMakeFileList(
                            HWindow, Configuration.FileListHistory[0],
                            &PluginData, &file, entry.IsDirectory, line.data(),
                            static_cast<int>(line.size()), FALSE, maxSizes,
                            _countof(maxSizes), validData, path.c_str(), TRUE))
                    {
                        result = FALSE;
                        break;
                    }
                    DWORD length = static_cast<DWORD>(strlen(line.data()));
                    DWORD written = 0;
                    if (length != 0 &&
                        (!WriteFile(hFile, line.data(), length, &written, NULL) ||
                         written != length))
                    {
                        DWORD error = GetLastError();
                        SalMessageBox(HWindow, GetErrorText(error),
                                      LoadStr(IDS_ERRORTITLE),
                                      MB_OK | MB_ICONEXCLAMATION);
                        result = FALSE;
                        break;
                    }
                }
                ++entryIndex;
            }
        }
    }
    FilesActionInProgress = FALSE;
    return result;
}

static BOOL UnpackArchiveToPluginFSViaTemp(CFilesWindow* source, CFilesWindow* target,
                                           CPanelTmpEnumData* data, int sourceFiles, int sourceDirs,
                                           char* targetPath, BOOL& invalidPathOrCancel);
static BOOL UnpackArchiveToArchiveViaTemp(CFilesWindow* source, CPanelTmpEnumData* data,
                                         char* targetPath, BOOL& invalidPathOrCancel);

// Validate ASCII restrictions as bytes; UTF-8 continuation bytes are not control characters.
static BOOL IsUnpackDirectoryComponentInvalid(const char* component, size_t length)
{
    if (length == 0) return FALSE;
    return (unsigned char)component[0] <= ' ' ||
           (unsigned char)component[length - 1] <= ' ' || component[length - 1] == '.';
}

// The dialog and drop paths share byte capacity, but never a shortened identity.
static BOOL CopyUnpackDestination(const char* source, char* destination, int capacity)
{
    if (source == NULL || destination == NULL || capacity <= 0) return FALSE;
    const size_t length = strlen(source);
    if (length >= (size_t)capacity) return FALSE;
    memmove(destination, source, length + 1);
    return TRUE;
}

static BOOL ReadUnpackDiskDestination(const wchar_t* source, char* destination, int capacity)
{
    if (source == NULL || *source == 0) return FALSE;
    const std::string encoded = SalWideToMultiBytePath(source, CP_UTF8);
    return !encoded.empty() && SalMultiByteToWidePath(encoded.c_str(), CP_UTF8) == source &&
           CopyUnpackDestination(encoded.c_str(), destination, capacity);
}

// Best-effort cleanup of the exact newly-created tree: never delete files or follow reparse directories.
static void RemoveEmptyUnpackDirectoriesW(const std::wstring& root)
{
    if (root.empty()) return;
    std::vector<std::wstring> directories(1, root);
    for (size_t index = 0; index < directories.size(); ++index)
    {
        const std::wstring directory = directories[index];
        const std::wstring ioPath = SalPathAddExtendedPrefixW(directory.c_str());
        const DWORD attributes = GetFileAttributesW(ioPath.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) continue;
        std::wstring mask = directory;
        SalPathAppendW(mask, L"*");
        WIN32_FIND_DATAW data;
        const std::wstring ioMask = SalPathAddExtendedPrefixW(mask.c_str());
        HANDLE find = HANDLES_Q(FindFirstFileW(ioMask.c_str(), &data));
        if (find == INVALID_HANDLE_VALUE) continue;
        do
        {
            if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
                wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
            std::wstring child = directory;
            SalPathAppendW(child, data.cFileName);
            directories.push_back(child);
        } while (FindNextFileW(find, &data));
        HANDLES(FindClose(find));
    }
    for (size_t index = directories.size(); index > 0; --index)
    {
        const std::wstring ioPath = SalPathAddExtendedPrefixW(directories[index - 1].c_str());
        const DWORD attributes = GetFileAttributesW(ioPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !(attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            RemoveDirectoryW(ioPath.c_str()); // succeeds only for an empty ordinary directory
    }
}

void CFilesWindow::UnpackZIPArchive(CFilesWindow* target, BOOL deleteOp, const char* tgtPath,
                                    BOOL* changeTargetRequested)
{
    if (IsProtectedZip())
        return;
    // Resolve the implicit destination before any Branch capability or lifetime snapshot checks.
    if (!deleteOp && target == NULL && tgtPath == NULL)
        target = MainWindow->GetOtherPanel(this);

    if (!deleteOp && target != NULL && CFilesActionDiskTarget::FindLive(MainWindow, target) == NULL)
    {
        SalMessageBox(HWindow, GetErrorText(ERROR_RETRY), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    const HWND operationParent = MainWindow != NULL ? MainWindow->HWindow : NULL;
    CFilesActionArchiveSource archiveSource;
    if (!deleteOp && !archiveSource.Capture(MainWindow, this))
    {
        SalMessageBox(operationParent, GetErrorText(ERROR_RETRY), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    // Keep the source tab alive through modal and plugin callbacks. Existing
    // callers may already own the busy flag; restore their exact previous state.
    struct CArchiveSourceBusyScope
    {
        CFilesActionArchiveSource* Source;
        BOOL Previous;
        CArchiveSourceBusyScope(CFilesActionArchiveSource* source) : Source(source), Previous(FALSE)
        {
            if (Source != NULL) { Previous = Source->Panel->FilesActionInProgress; Source->Panel->FilesActionInProgress = TRUE; }
        }
        ~CArchiveSourceBusyScope()
        {
            if (Source != NULL)
            {
                CFilesWindow* live = CFilesActionDiskTarget::FindLive(MainWindow, Source->Panel);
                if (live != NULL && live->GetPanelTabId() == Source->Tab) live->FilesActionInProgress = Previous;
            }
        }
    } sourceBusy(deleteOp ? NULL : &archiveSource);

    if (IsBranchView() || target != NULL && target->IsBranchView() &&
        (deleteOp || !target->Is(ptDisk) ||
         PackGetVerifiedBranchUnpacker(this, GetZIPArchive(), PluginData.GetInterface()) == NULL)) return; // Only a qualified archive listing can copy to a Branch root.

    if (changeTargetRequested != NULL)
        *changeTargetRequested = FALSE;
    CALL_STACK_MESSAGE3("CFilesWindow::UnpackZIPArchive(, %d, %s)", deleteOp, tgtPath);
    if (Files->Count + Dirs->Count == 0)
        return;

    CFilesActionDiskTarget diskTarget;
    const bool watchDiskTarget = !deleteOp && target != NULL && target->Is(ptDisk);
    if (watchDiskTarget && !diskTarget.Capture(MainWindow, target, TRUE))
    {
        SalMessageBox(HWindow, LoadStr(IDS_PATHISINVALID), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    const auto validateDiskTarget = [&]() {
        if (!deleteOp && !archiveSource.IsCurrent(MainWindow))
        {
            SalMessageBox(operationParent, GetErrorText(ERROR_RETRY), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
            return false;
        }
        if (!watchDiskTarget || (diskTarget.IsCurrent(MainWindow) &&
            (!diskTarget.Branch || PackGetVerifiedBranchUnpacker(this, GetZIPArchive(), PluginData.GetInterface()) != NULL))) return true;
        SalMessageBox(HWindow, GetErrorText(ERROR_RETRY), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
        return false;
    };

    // restore DefaultDir
    MainWindow->UpdateDefaultDir(MainWindow->GetActivePanel() == this);

    if (!validateDiskTarget()) return;

    BeginStopRefresh(); // the snooper takes a break

    //---  obtain the files and directories to work with
    CPathBuffer subjectBuffer(SAL_MAX_PATH + 100), pathBuffer;
    char* subject = subjectBuffer.Data(); // text for the Unpack dialog
    char* path = pathBuffer.Data();
    char expanded[200];
    CPanelTmpEnumData data;
    BOOL subDir;
    if (Dirs->Count > 0)
        subDir = (strcmp(Dirs->At(0).Name, "..") == 0);
    else
        subDir = FALSE;
    data.IndexesCount = GetSelCount();
    if (data.IndexesCount > 1) // valid selection
    {
        int files = 0; // number of selected files
        data.Indexes = new int[data.IndexesCount];
        if (data.Indexes == NULL)
        {
            TRACE_E(LOW_MEMORY);
            EndStopRefresh(); // the snooper resumes now
            return;
        }
        else
        {
            GetSelItems(data.IndexesCount, data.Indexes);
            int i = data.IndexesCount;
            while (i--)
            {
                BOOL isDir = data.Indexes[i] < Dirs->Count;
                CFileData* f = isDir ? &Dirs->At(data.Indexes[i]) : &Files->At(data.Indexes[i] - Dirs->Count);
                if (!isDir)
                    files++;
            }
        }
        // build the subject for the dialog
        ExpandPluralFilesDirs(expanded, 200, files, data.IndexesCount - files, epfdmNormal, FALSE);
    }
    else // take the selected file or directory
    {
        int index;
        if (data.IndexesCount == 0)
            index = GetCaretIndex();
        else
            GetSelItems(1, &index);

        if (subDir && index == 0)
        {
            EndStopRefresh(); // the snooper resumes now
            return;           // nothing to do
        }
        else
        {
            data.Indexes = new int[1];
            if (data.Indexes == NULL)
            {
                TRACE_E(LOW_MEMORY);
                EndStopRefresh(); // the snooper resumes now
                return;
            }
            else
            {
                data.Indexes[0] = index;
                data.IndexesCount = 1;
                // build the subject for the dialog
                BOOL isDir = index < Dirs->Count;
                CFileData* f = isDir ? &Dirs->At(index) : &Files->At(index - Dirs->Count);
                AlterFileName(path, f->Name, -1, Configuration.FileNameFormat, 0, index < Dirs->Count);
                lstrcpy(expanded, LoadStr(isDir ? IDS_QUESTION_DIRECTORY : IDS_QUESTION_FILE));
            }
        }
    }
    sprintf(subject, LoadStr(deleteOp ? IDS_CONFIRM_DELETEFROMARCHIVE : IDS_COPYFROMARCHIVETO), expanded);
    CTruncatedString str;
    str.Set(subject, data.IndexesCount > 1 ? NULL : path);

    data.CurrentIndex = 0;
    data.ZIPPath = deleteOp ? GetZIPPath() : archiveSource.ArchiveRoot.c_str();
    if (!deleteOp)
    {
        data.Context = &archiveSource;
        data.ContextIsCurrent = [](void* context) -> BOOL {
            return static_cast<CFilesActionArchiveSource*>(context)->IsCurrent(MainWindow);
        };
    }
    data.Dirs = Dirs;
    data.Files = Files;
    data.ArchiveDir = GetArchiveDir();
    data.EnumLastDir = NULL;
    data.EnumLastIndex = -1;

    CPathBuffer changesRootBuffer;
    char* changesRoot = changesRootBuffer.Data(); // full notification prefix
    changesRoot[0] = 0;

    if (!deleteOp) // copy
    {
        int sourceFiles = 0;
        int i;
        for (i = 0; i < data.IndexesCount; i++)
        {
            if (data.Indexes[i] >= Dirs->Count)
                sourceFiles++;
        }
        int sourceDirs = data.IndexesCount - sourceFiles;

        //---  obtain the target directory
        if (!validateDiskTarget())
        {
            delete[] data.Indexes;
            EndStopRefresh();
            return;
        }
        if (watchDiskTarget)
        {
            if (!ReadUnpackDiskDestination(diskTarget.Root.c_str(), path, pathBuffer.Capacity()))
            {
                SalMessageBox(HWindow, LoadStr(IDS_TOOLONGPATH), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                delete[] data.Indexes;
                EndStopRefresh();
                return;
            }

            target->UserWorkedOnThisPath = TRUE; // default action = operate with the path in the target panel
        }
        else
        {
            path[0] = 0;
            if (target != NULL && target->Is(ptZIPArchive))
            {
                target->GetGeneralPath(path, MAX_PATH);
                SalPathAddBackslash(path, MAX_PATH);

                int format = PackerFormatConfig.PackIsArchive(target->GetZIPArchive());
                if (format != 0 && PackerFormatConfig.GetUsePacker(format - 1))
                {
                    target->UserWorkedOnThisPath = TRUE; // default action = operate with the path in the target panel
                }
                else
                {
                    path[0] = 0;
                }
            }
            else
            {
                if (target != NULL && target->Is(ptPluginFS) && target->GetPluginFS()->NotEmpty() &&
                    target->GetPluginFS()->IsServiceSupported(FS_SERVICE_COPYFROMDISKTOFS))
                {
                    target->GetGeneralPath(path, MAX_PATH);
                    const std::string fallbackPath(path);
                    if (!target->GetPluginFS()->CopyOrMoveFromDiskToFS(TRUE, 1,
                                                                       target->GetPluginFS()->GetPluginFSName(),
                                                                       HWindow, NULL, NULL, NULL,
                                                                       sourceFiles, sourceDirs, path, NULL))
                    {
                        if (path[0] == 0)
                            lstrcpyn(path, fallbackPath.c_str(), MAX_PATH);
                    }
                    else
                    {
                        // convert the path to external format (before showing it in the dialog)
                        PluginFSConvertPathToExternal(path);

                        // Unpacking from an archive does not support operation masks.
                        // Strip the mask appended by the plugin's CopyOrMoveFromDiskToFS(Mode=1)
                        // and ensure the path ends with a backslash/slash.
                        int pathLen = (int)strlen(path);
                        if (pathLen >= 3 && strcmp(path + pathLen - 3, "*.*") == 0)
                        {
                            path[pathLen - 3] = 0;
                        }
                        else if (pathLen >= 1 && path[pathLen - 1] == '*')
                        {
                            path[pathLen - 1] = 0;
                        }

                        target->UserWorkedOnThisPath = TRUE; // default action = operate with the path in the target panel
                    }
                }
            }
        }

        CCopyMoveDialog dlg(HWindow, path, pathBuffer.Capacity(), LoadStr(IDS_UNPACKCOPY), &str, IDD_COPYDIALOG,
                            Configuration.CopyHistory, COPY_HISTORY_SIZE, TRUE,
                            changeTargetRequested != NULL);

    _DLG_AGAIN:

        if (!validateDiskTarget())
        {
            delete[] data.Indexes;
            EndStopRefresh();
            return;
        }
        int dialogResult = tgtPath != NULL ? IDOK : (int)dlg.Execute();
        if (dialogResult == ID_CHANGE_SELECTED_TARGET_TAB)
        {
            *changeTargetRequested = TRUE;
            dialogResult = IDCANCEL;
        }
        if (dialogResult == IDOK)
        {
            if (!validateDiskTarget())
            {
                delete[] data.Indexes;
                EndStopRefresh();
                return;
            }

            if (tgtPath != NULL && !CopyUnpackDestination(tgtPath, path, pathBuffer.Capacity()))
            {
                SalMessageBox(HWindow, LoadStr(IDS_TOOLONGPATH), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                delete[] data.Indexes;
                EndStopRefresh();
                return;
            }
            const bool useDefaultDiskTarget = watchDiskTarget && tgtPath == NULL &&
                                              diskTarget.HasDefaultText(path, pathBuffer.Capacity());
            UpdateWindow(MainWindow->HWindow);
            if (!validateDiskTarget())
            {
                delete[] data.Indexes;
                EndStopRefresh();
                return;
            }
            if (watchDiskTarget && diskTarget.Branch && !useDefaultDiskTarget)
            {
                SalMessageBox(HWindow, LoadStr(IDS_PATHISINVALID), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                delete[] data.Indexes;
                EndStopRefresh();
                return;
            }
            //---  for disk paths, convert '/' to '\\' and remove duplicate '\\'
            if (!IsPluginFSPath(path) &&
                (path[0] != 0 && path[1] == ':' ||                                             // paths like X:...
                 (path[0] == '/' || path[0] == '\\') && (path[1] == '/' || path[1] == '\\') || // UNC paths
                 Is(ptDisk) || Is(ptZIPArchive)))                                              // disk+archive relative paths
            {                                                                                  // this is a disk path (absolute or relative) - convert all '/' to '\\' and remove duplicate '\\'
                SlashesToBackslashesAndRemoveDups(path);
            }
            //---  adjust the entered path -> convert to absolute, without '.' and '..'

            int len = (int)strlen(path);
            BOOL backslashAtEnd = (len > 0 && path[len - 1] == '\\'); // path ends with a backslash -> must be a directory
            BOOL mustBePath = (len == 2 && LowerCase[path[0]] >= 'a' && LowerCase[path[0]] <= 'z' &&
                               path[1] == ':'); // a path like "c:" must remain a directory after expansion (not a file)

            int pathType;
            BOOL pathIsDir;
            char* secondPart;
            CPathBuffer textBuffer(2 * SAL_MAX_PATH + 200);
            char* textBuf = textBuffer.Data();
            const BOOL parsed = ParsePath(path, pathType, pathIsDir, secondPart, LoadStr(IDS_ERRORCOPY), NULL, NULL, pathBuffer.Capacity());
            if (!validateDiskTarget())
            {
                delete[] data.Indexes;
                EndStopRefresh();
                return;
            }
            if (parsed)
            {
                if (useDefaultDiskTarget &&
                    !diskTarget.MatchesParsedDirectoryDefault(path, pathBuffer.Capacity(), secondPart, pathType, pathIsDir))
                {
                    SalMessageBox(HWindow, GetErrorText(ERROR_RETRY), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                    delete[] data.Indexes;
                    EndStopRefresh();
                    return;
                }
                // instead of a 'switch', use 'if' so that 'break' and 'continue' work correctly
                if (pathType == PATH_TYPE_WINDOWS) // Windows path (disk + UNC)
                {
                    CPathBuffer newDirsBuffer;
                    char* newDirs = newDirsBuffer.Data(); // first newly created directory for cleanup
                    newDirs[0] = 0;

                    if (pathIsDir) // the existing part of the path is a directory
                    {
                        if (*secondPart != 0) // the path contains a segment that does not exist
                        {
                            if (!backslashAtEnd && !mustBePath) // the new path must end with a backslash; otherwise it is an operation mask (unsupported)
                            {
                                SalMessageBox(HWindow, LoadStr(IDS_UNPACK_OPMASKSNOTSUP), LoadStr(IDS_ERRORCOPY),
                                              MB_OK | MB_ICONEXCLAMATION);
                                if (tgtPath != NULL)
                                {
                                    UpdateWindow(MainWindow->HWindow);
                                    delete[] (data.Indexes);
                                    EndStopRefresh();
                                    return;
                                }
                                goto _DLG_AGAIN;
                            }

                            // create the new directories
                            strcpy(newDirs, path);

                            if (Configuration.CnfrmCreatePath) // ask whether the path should be created
                            {
                                BOOL dontShow = FALSE;
                                if (!Salamander::LocalizedText::Write(textBuf, static_cast<size_t>(textBuffer.Capacity()), LoadStr(IDS_MOVECOPY_CREATEPATH), newDirs))
                                {
                                    // Never present a malformed/truncated target confirmation.
                                    SalMessageBox(HWindow, GetErrorText(ERROR_INVALID_DATA), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                                    delete[] data.Indexes;
                                    EndStopRefresh();
                                    return;
                                }

                                MSGBOXEX_PARAMS params;
                                memset(&params, 0, sizeof(params));
                                params.HParent = HWindow;
                                params.Flags = MSGBOXEX_YESNO | MSGBOXEX_ICONQUESTION | MSGBOXEX_SILENT | MSGBOXEX_ESCAPEENABLED | MSGBOXEX_HINT;
                                params.Caption = LoadStr(IDS_UNPACKCOPY);
                                params.Text = textBuf;
                                params.CheckBoxText = LoadStr(IDS_MOVECOPY_CREATEPATH_CNFRM);
                                params.CheckBoxValue = &dontShow;
                                BOOL cont = (SalMessageBoxEx(&params) != IDYES);
                                Configuration.CnfrmCreatePath = !dontShow;
                                if (cont)
                                {
                                    SalPathAddBackslash(path, pathBuffer.Capacity());
                                    if (tgtPath != NULL)
                                    {
                                        UpdateWindow(MainWindow->HWindow);
                                        delete[] (data.Indexes);
                                        EndStopRefresh();
                                        return;
                                    }
                                    goto _DLG_AGAIN;
                                }
                            }

                            if (!validateDiskTarget())
                            {
                                delete[] data.Indexes;
                                EndStopRefresh();
                                return;
                            }
                            // Compute notification root before any new directory is created.
                            memcpy(changesRoot, path, secondPart - path);
                            changesRoot[secondPart - path] = 0;
                            BOOL ok = TRUE;
                            bool createdAny = false;
                            char* st = newDirs + (secondPart - path);
                            char* firstSlash = NULL;
                            while (1)
                            {
                                if (!validateDiskTarget())
                                {
                                    if (createdAny)
                                    {
                                        if (firstSlash != NULL) *firstSlash = 0;
                                        RemoveEmptyUnpackDirectoriesW(SalMultiByteToWidePath(newDirs, IsValidPathUtf8Text(newDirs) ? CP_UTF8 : CP_ACP));
                                        MainWindow->PostChangeOnPathNotification(changesRoot, TRUE);
                                    }
                                    delete[] data.Indexes;
                                    EndStopRefresh();
                                    return;
                                }
                                char* slash = strchr(st, '\\');
                                const size_t componentLength = slash != NULL ? (size_t)(slash - st) : strlen(st);
                                const BOOL invalidPath = IsUnpackDirectoryComponentInvalid(st, componentLength);
                                if (slash != NULL)
                                {
                                    *slash = 0;
                                    if (firstSlash == NULL)
                                        firstSlash = slash;
                                }
                                std::wstring newDirsW = SalMultiByteToWidePath(newDirs, IsValidPathUtf8Text(newDirs) ? CP_UTF8 : CP_ACP);
                                if (invalidPath || (newDirsW.empty() || !SalCreateDirectoryExW(newDirsW.c_str(), NULL)))
                                {
                                    const bool haveMessage = Salamander::LocalizedText::Write(textBuf, static_cast<size_t>(textBuffer.Capacity()), LoadStr(IDS_CREATEDIRFAILED), newDirs);
                                    SalMessageBox(HWindow, haveMessage ? textBuf : GetErrorText(ERROR_INVALID_DATA), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                                    ok = FALSE;
                                    break;
                                }
                                createdAny = true;
                                if (slash != NULL)
                                    *slash = '\\';
                                else
                                    break; // that was the last '\\'
                                st = slash + 1;
                            }

                            if (!ok)
                            {
                                //---  refresh directories that are not automatically refreshed
                                // if directory creation failed, report immediately changes (the user may
                                // choose a completely different path next time); the new path is kept (almost dead code)
                                MainWindow->PostChangeOnPathNotification(changesRoot, TRUE);

                                SalPathAddBackslash(path, pathBuffer.Capacity());
                                if (tgtPath != NULL)
                                {
                                    UpdateWindow(MainWindow->HWindow);
                                    delete[] (data.Indexes);
                                    EndStopRefresh();
                                    return;
                                }
                                goto _DLG_AGAIN;
                            }
                            if (firstSlash != NULL)
                                *firstSlash = 0; // put the name of the first created directory into newDirs
                        }
                    }
                    else // overwrite file - 'secondPart' points to the filename in 'path'
                    {
                        SalMessageBox(HWindow, LoadStr(IDS_UNPACK_OPMASKSNOTSUP), LoadStr(IDS_ERRORCOPY),
                                      MB_OK | MB_ICONEXCLAMATION);
                        if (backslashAtEnd || mustBePath)
                            SalPathAddBackslash(path, pathBuffer.Capacity());
                        if (tgtPath != NULL)
                        {
                            UpdateWindow(MainWindow->HWindow);
                            delete[] (data.Indexes);
                            EndStopRefresh();
                            return;
                        }
                        goto _DLG_AGAIN;
                    }

                    // if no new directories are created, changes start at the target path
                    if (changesRoot[0] == 0)
                        strcpy(changesRoot, path);

                    //---  actual unpacking
                    if (!validateDiskTarget())
                    {
                        if (newDirs[0] != 0)
                            RemoveEmptyUnpackDirectoriesW(SalMultiByteToWidePath(newDirs, IsValidPathUtf8Text(newDirs) ? CP_UTF8 : CP_ACP));
                        if (changesRoot[0] != 0) MainWindow->PostChangeOnPathNotification(changesRoot, TRUE);
                        delete[] data.Indexes;
                        EndStopRefresh();
                        return;
                    }
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
                    const BOOL unpacked = PackUncompress(MainWindow->HWindow, this, archiveSource.ArchiveFile.c_str(), archiveSource.PluginData,
                                       path, archiveSource.ArchiveRoot.c_str(), PanelSalEnumSelection, &data,
                                       watchDiskTarget ? &diskTarget : NULL, data.ContextIsCurrent, data.Context);
                    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                    const bool targetCurrent = validateDiskTarget();
                    if (unpacked && targetCurrent)
                    {                        // unpacking succeeded
                        if (tgtPath == NULL && archiveSource.IsCurrent(MainWindow)) // if it is not drag&drop (selection is not cleared there)
                        {
                            SetSel(FALSE, -1, TRUE);                        // explicit redraw
                            PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // sel-change notify
                        }
                    }
                    else if (!unpacked)
                    {
                        if (newDirs[0] != 0)
                            RemoveEmptyUnpackDirectoriesW(SalMultiByteToWidePath(newDirs, IsValidPathUtf8Text(newDirs) ? CP_UTF8 : CP_ACP));
                    }

                    if (archiveSource.IsCurrent(MainWindow) && GetForegroundWindow() == MainWindow->HWindow) // restore focus only to the original source
                        RestoreFocusInSourcePanel();
                }
                else
                {
                    if (strlen(path) >= MAX_PATH)
                    {
                        SalMessageBox(HWindow, LoadStr(IDS_TOOLONGPATH), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                        if (tgtPath != NULL)
                        {
                            delete[] data.Indexes;
                            EndStopRefresh();
                            return;
                        }
                        goto _DLG_AGAIN;
                    }
                    BOOL invalidPathOrCancel = FALSE;
                    if ((pathType == PATH_TYPE_FS && target != NULL && target->Is(ptPluginFS) &&
                         UnpackArchiveToPluginFSViaTemp(this, target, &data, sourceFiles, sourceDirs,
                                                        path, invalidPathOrCancel)) ||
                        (pathType == PATH_TYPE_ARCHIVE &&
                         UnpackArchiveToArchiveViaTemp(this, &data, path, invalidPathOrCancel)))
                    {
                        if (tgtPath == NULL && archiveSource.IsCurrent(MainWindow)) // if it is not drag&drop (selection is not cleared there)
                        {
                            SetSel(FALSE, -1, TRUE);                        // explicit redraw
                            PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // sel-change notify
                        }
                    }
                    else
                    {
                        if (!invalidPathOrCancel)
                        {
                            SalMessageBox(HWindow, LoadStr(IDS_UNPACK_ONLYDISK), LoadStr(IDS_ERRORCOPY),
                                          MB_OK | MB_ICONEXCLAMATION);
                            if (pathType == PATH_TYPE_ARCHIVE && (backslashAtEnd || mustBePath))
                            {
                                SalPathAddBackslash(path, pathBuffer.Capacity());
                            }
                        }
                        if (tgtPath != NULL)
                        {
                            UpdateWindow(MainWindow->HWindow);
                            delete[] (data.Indexes);
                            EndStopRefresh();
                            return;
                        }
                        goto _DLG_AGAIN;
                    }
                }
            }
            else
            {
                if (tgtPath != NULL)
                {
                    UpdateWindow(MainWindow->HWindow);
                    delete[] (data.Indexes);
                    EndStopRefresh();
                    return;
                }
                goto _DLG_AGAIN;
            }

            //---  refresh directories that are not automatically refreshed
            // changes on the target path and its subdirectories (creating new directories and unpacking
            // files/directories)
            if (changesRoot[0] != 0)
                MainWindow->PostChangeOnPathNotification(changesRoot, TRUE);
            // change in the directory containing the archive (should not occur during unpack, but refresh just in case it does)
            MainWindow->PostChangeOnPathNotification(archiveSource.Directory.c_str(), FALSE);
        }
    }
    else // delete
    {
        //---  ask whether the user is sure they want to delete
        HICON hIcon = (HICON)HANDLES(LoadImage(Shell32DLL, MAKEINTRESOURCE(WindowsVistaAndLater ? 16777 : 161), // delete icon
                                               IMAGE_ICON, 32, 32, IconLRFlags));
        if (!Configuration.CnfrmFileDirDel ||
            CMessageBox(HWindow, MSGBOXEX_YESNO | MSGBOXEX_ESCAPEENABLED | MSGBOXEX_SILENT,
                        LoadStr(IDS_CONFIRM_DELETE_TITLE), &str, NULL,
                        NULL, hIcon, 0, NULL, NULL, NULL, NULL)
                    .Execute() == IDYES)
        {
            UpdateWindow(MainWindow->HWindow);
            //---  finding non-empty directories - ask about deleting them if needed
            BOOL cancel = FALSE;
            if (Configuration.CnfrmNEDirDel)
            {
                int i;
                for (i = 0; i < data.IndexesCount; i++)
                {
                    if (data.Indexes[i] < Dirs->Count)
                    {
                        int dirsCount = 0;
                        int filesCount = 0;
                        GetArchiveDir()->GetDirSize(GetZIPPath(), Dirs->At(data.Indexes[i]).Name,
                                                    &dirsCount, &filesCount);
                        if (dirsCount + filesCount > 0)
                        {
                            char name[2 * MAX_PATH];
                            strcpy(name, GetZIPArchive());
                            if (GetZIPPath()[0] != 0)
                            {
                                if (GetZIPPath()[0] != '\\')
                                    strcat(name, "\\");
                                strcat(name, GetZIPPath());
                            }
                            strcat(name, "\\");
                            strcat(name, Dirs->At(data.Indexes[i]).Name);

                            char text[2 * MAX_PATH + 100];
                            sprintf(text, LoadStr(IDS_NONEMPTYDIRDELCONFIRM), name);
                            int res = SalMessageBox(HWindow, text, LoadStr(IDS_QUESTION),
                                                    MB_YESNOCANCEL | MB_ICONQUESTION);
                            if (res == IDCANCEL)
                            {
                                cancel = TRUE;
                                break;
                            }
                            if (res == IDNO)
                            {
                                memmove(data.Indexes + i, data.Indexes + i + 1, (data.IndexesCount - i - 1) * sizeof(int));
                                data.IndexesCount--;
                                i--;
                            }
                        }
                    }
                }
                if (data.IndexesCount == 0)
                    cancel = TRUE;
            }
            //---  actual deletion
            if (!cancel)
            {
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
                if (PackDelFromArc(MainWindow->HWindow, this, GetZIPArchive(), PluginData.GetInterface(),
                                   GetZIPPath(), PanelSalEnumSelection, &data))
                {                                                   // deletion succeeded
                    SetSel(FALSE, -1, TRUE);                        // explicit redraw
                    PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // sel-change notify
                }
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

                //---  refresh directories that are not automatically refreshed
                // change in the directory containing the archive
                MainWindow->PostChangeOnPathNotification(GetPath(), FALSE);
            }
        }
        HANDLES(DestroyIcon(hIcon));
    }

    UpdateWindow(MainWindow->HWindow);
    delete[] (data.Indexes);

    //---  if any Salamander window is active, suspend mode ends
    EndStopRefresh();
}

void CFilesWindow::DeleteFromZIPArchive()
{
    CALL_STACK_MESSAGE1("CFilesWindow::DeleteFromZIPArchive()");
    if (IsProtectedZip())
        return;
    UnpackZIPArchive(NULL, TRUE); // almost the same operation
}

BOOL _ReadDirectoryTree(HWND parent, char (&path)[SAL_MAX_PATH], const char* name, CSalamanderDirectory* dir,
                        int* errorOccured, BOOL getLinkTgtFileSize, BOOL* errGetFileSizeOfLnkTgtIgnAll,
                        int* containsDirLinks, char* linkName, BOOL wideNames)
{
    const size_t originalPathLength = strlen(path);
    struct CDirectoryPathScope
    {
        char* Path;
        size_t Length;
        ~CDirectoryPathScope() { Path[Length] = 0; }
    } pathScope = {path, originalPathLength};
    try
    {
    CALL_STACK_MESSAGE4("_ReadDirectoryTree(, %s, %s, , , %d, , ,)", path, name, getLinkTgtFileSize);
    char* end = path + strlen(path);
    CPathBuffer text(2 * SAL_MAX_PATH + 512);
    if ((end - path) + (*(end - 1) != '\\' ? 1 : 0) + strlen(name) + 2 >= _countof(path))
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
        return FALSE; // Never report an omitted subtree as complete.
    }
    if (*(end - 1) != '\\')
    {
        *end++ = '\\';
        *end = 0;
    }
    strcpy_s(end, _countof(path) - (end - path), name);
    strcat_s(end, _countof(path) - (end - path), "\\*");

    WIN32_FIND_DATAA legacyFile = {};
    WIN32_FIND_DATAW file = {};
    const std::wstring ioMask = wideNames ? SalPathAddExtendedPrefixW(
        SalMultiByteToWidePath(path, IsValidPathUtf8Text(path) ? CP_UTF8 : CP_ACP).c_str()) : std::wstring();
    HANDLE find = wideNames ? HANDLES_Q(FindFirstFileW(ioMask.c_str(), &file)) : HANDLES_Q(FindFirstFileA(path, &legacyFile));
    struct CDirectoryFindScope
    {
        HANDLE Handle;
        ~CDirectoryFindScope() { if (Handle != INVALID_HANDLE_VALUE) HANDLES(FindClose(Handle)); }
    } findScope = {find};
    *end = 0; // restore the path
    if (find == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        if (err != ERROR_FILE_NOT_FOUND && err != ERROR_NO_MORE_FILES)
        {
            if (errorOccured != NULL)
                *errorOccured = SALENUM_ERROR;
            strcpy_s(end, _countof(path) - (end - path), name);
            if (!Salamander::LocalizedText::Write(text.Data(), static_cast<size_t>(text.Capacity()), LoadStr(IDS_CANNOTREADDIR), path, GetErrorText(err)))
            {
                if (parent != NULL)
                    SalMessageBox(parent, GetErrorText(ERROR_INVALID_DATA), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                SetLastError(ERROR_INVALID_DATA);
                return FALSE;
            }
            *end = 0; // restore the path
            if (parent != NULL &&
                SalMessageBox(parent, text.Data(), LoadStr(IDS_ERRORTITLE),
                              MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
            {
                if (errorOccured != NULL)
                    *errorOccured = SALENUM_CANCEL;
                return FALSE; // user wants to quit
            }
        }
        return TRUE; // user wants to continue
    }
    else
    {
        strcpy_s(end, _countof(path) - (end - path), name);
        char* end2 = end + strlen(end);
        BOOL ok = TRUE;
        CFileData newF = {0}; // we no longer work with these items
        if (dir != NULL)
        {
            newF.PluginData = -1; // -1 is arbitrary, ignored
            newF.Association = 0;
            newF.Selected = 0;
            newF.Shared = 0;
            newF.Archive = 0;
            newF.SizeValid = 0;
            newF.Dirty = 0; // unnecessary, just to keep structure consistent
            newF.CutToClip = 0;
            newF.IconOverlayIndex = ICONOVERLAYINDEX_NOTUSED;
            newF.IconOverlayDone = 0;
        }
        else
            memset(&newF, 0, sizeof(newF));
        BOOL testFindNextErr = TRUE;

        do
        {
            const std::string fileName = wideNames ? SalWideToMultiBytePath(file.cFileName, CP_UTF8) : legacyFile.cFileName;
            const std::string alternateName = wideNames ? SalWideToMultiBytePath(file.cAlternateFileName, CP_UTF8) : legacyFile.cAlternateFileName;
            if (!wideNames)
            {
                file.dwFileAttributes = legacyFile.dwFileAttributes;
                file.nFileSizeLow = legacyFile.nFileSizeLow;
                file.nFileSizeHigh = legacyFile.nFileSizeHigh;
                file.ftLastWriteTime = legacyFile.ftLastWriteTime;
            }
            if (wideNames && (fileName.empty() || SalMultiByteToWidePath(fileName.c_str(), CP_UTF8) != file.cFileName ||
                SalMultiByteToWidePath(alternateName.c_str(), CP_UTF8) != file.cAlternateFileName))
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                ok = FALSE;
                testFindNextErr = FALSE;
                break;
            }
            if (fileName.c_str()[0] == 0 ||
                fileName.c_str()[0] == '.' &&
                    (fileName.c_str()[1] == 0 || (fileName.c_str()[1] == '.' && fileName.c_str()[2] == 0)))
                continue; // "." a ".."

            static DWORD lastBreakCheck = 0;
            if (containsDirLinks != NULL && GetTickCount() - lastBreakCheck > 200)
            {
                lastBreakCheck = GetTickCount();
                if (UserWantsToCancelSafeWaitWindow())
                {
                    *containsDirLinks = 2; // after interruption simulate an error to ensure the search immediately ends
                    ok = FALSE;
                    testFindNextErr = FALSE;
                    break;
                }
            }

            if (dir != NULL)
            {
                newF.Size = CQuadWord(file.nFileSizeLow, file.nFileSizeHigh);

                BOOL cancel = FALSE;
                if (getLinkTgtFileSize &&
                    (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&   // it's a file
                    (file.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) // it's a link
                {                                                                // for a symlink determine the target file size
                    CQuadWord size;
                    if (SalPathAppend(path, fileName.c_str(), _countof(path)))
                    { // only if the path is not too long (any resulting error will be reported during enumeration)
                        if (GetLinkTgtFileSize(parent, path, NULL, &size, &cancel, errGetFileSizeOfLnkTgtIgnAll))
                            newF.Size = size;
                        else
                        {
                            if (cancel && errorOccured != NULL)
                                *errorOccured = SALENUM_CANCEL;
                        }
                    }
                    *end2 = 0; // restore 'path' to its original state
                }

                newF.Name = !cancel ? DupStr(fileName.c_str()) : NULL;
                newF.DosName = NULL;
                if (cancel || newF.Name == NULL)
                {
                    ok = FALSE;
                    testFindNextErr = FALSE;
                    break;
                }
                newF.NameLen = static_cast<int>(strlen(newF.Name));
                if (!Configuration.SortDirsByExt && (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) // directory, so it is certainly a disk
                {
                    newF.Ext = newF.Name + newF.NameLen; // directories have no extensions
                }
                else
                {
                    newF.Ext = strrchr(newF.Name, '.');
                    if (newF.Ext == NULL)
                        newF.Ext = newF.Name + newF.NameLen; // ".cvspass" is treated as an extension in Windows ...
                                                             //        if (newF.Ext == NULL || newF.Ext == newF.Name) newF.Ext = newF.Name + newF.NameLen;
                    else
                        newF.Ext++;
                }

                if (alternateName.c_str()[0] != 0)
                {
                    newF.DosName = DupStr(alternateName.c_str());
                    if (newF.DosName == NULL)
                    {
                        free(newF.Name);
                        ok = FALSE;
                        testFindNextErr = FALSE;
                        break;
                    }
                }

                newF.Attr = file.dwFileAttributes;
                newF.LastWrite = file.ftLastWriteTime;
                newF.Hidden = newF.Attr & FILE_ATTRIBUTE_HIDDEN ? 1 : 0;
                newF.IsOffline = newF.Attr & FILE_ATTRIBUTE_OFFLINE ? 1 : 0;
            }

            if (file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // directory, so it is certainly a disk
            {
                CSalamanderDirectory* salDir = NULL;
                if (dir != NULL)
                {
                    newF.IsLink = (newF.Attr & FILE_ATTRIBUTE_REPARSE_POINT) ? 1 : 0; // volume mount point or junction point = show the directory with a link overlay
                    BOOL addDirOK = dir->AddDir("", newF, NULL);
                    if (addDirOK)
                        salDir = dir->GetSalamanderDir(newF.Name, FALSE); // allocate a sal-dir for the record
                    else
                    {
                        free(newF.Name);
                        if (newF.DosName != NULL)
                            free(newF.DosName);
                    }
                    if (salDir == NULL)
                    {
                        ok = FALSE;
                        testFindNextErr = FALSE;
                        break;
                    }
                }
                else // we are only looking for the first directory link
                {
                    if ((file.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                    {
                        *containsDirLinks = 1; // after finding one simulate an error to end the search immediately
                        *end2 = 0;
                        _snprintf_s(linkName, SAL_MAX_PATH, _TRUNCATE, "%s\\%s", path, fileName.c_str()); // truncation is fine, it is only for the message text
                        ok = FALSE;
                        testFindNextErr = FALSE;
                        break;
                    }
                }
                if (!_ReadDirectoryTree(parent, path, fileName.c_str(), salDir, errorOccured, getLinkTgtFileSize,
                                        errGetFileSizeOfLnkTgtIgnAll, containsDirLinks, linkName, wideNames))
                {
                    ok = FALSE;
                    testFindNextErr = FALSE;
                    break;
                }
            }
            else // file
            {
                if (dir != NULL)
                {
                    if (newF.Attr & FILE_ATTRIBUTE_REPARSE_POINT)
                        newF.IsLink = 1; // if the file is a reparse point (maybe impossible) display it with a link overlay
                    else
                        newF.IsLink = IsFileLink(newF.Ext);

                    if (!dir->AddFile("", newF, NULL))
                    {
                        free(newF.Name);
                        if (newF.DosName != NULL)
                            free(newF.DosName);
                        ok = FALSE;
                        testFindNextErr = FALSE;
                        break;
                    }
                }
            }
        } while (wideNames ? FindNextFileW(find, &file) : FindNextFileA(find, &legacyFile));
        DWORD err = GetLastError();
        *end = 0; // restore the path

        if (testFindNextErr && err != ERROR_NO_MORE_FILES)
        {
            if (errorOccured != NULL)
                *errorOccured = SALENUM_ERROR;
            strcpy_s(end, _countof(path) - (end - path), name);
            if (!Salamander::LocalizedText::Write(text.Data(), static_cast<size_t>(text.Capacity()), LoadStr(IDS_CANNOTREADDIR), path, GetErrorText(err)))
            {
                if (parent != NULL)
                    SalMessageBox(parent, GetErrorText(ERROR_INVALID_DATA), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
                SetLastError(ERROR_INVALID_DATA);
                return FALSE;
            }
            *end = 0; // restore the path
            if (parent != NULL &&
                SalMessageBox(parent, text.Data(), LoadStr(IDS_ERRORTITLE),
                              MB_OKCANCEL | MB_ICONEXCLAMATION) == IDCANCEL)
            {
                if (errorOccured != NULL)
                    *errorOccured = SALENUM_CANCEL;
                return FALSE; // user wants to quit
            }
        }

        if (!ok)
        {
            if (errorOccured != NULL && *errorOccured == SALENUM_SUCCESS)
                *errorOccured = SALENUM_ERROR;
            return FALSE;
        }
    }
    return TRUE;
    }
    catch (const std::bad_alloc&)
    {
        path[originalPathLength] = 0;
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
        return FALSE;
    }
}

CSalamanderDirectory* ReadDirectoryTree(HWND parent, CPanelTmpEnumData* data, int* errorOccured,
                                        BOOL getLinkTgtFileSize, int* containsDirLinks, char* linkName)
{
    CALL_STACK_MESSAGE2("ReadDirectoryTree(, , , %d, ,)", getLinkTgtFileSize);
    if (errorOccured != NULL)
        *errorOccured = SALENUM_SUCCESS;
    if (containsDirLinks != NULL)
        *containsDirLinks = 0;
    if (!data->HasCurrentContext())
    {
        SetLastError(ERROR_RETRY);
        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
        return NULL;
    }

    BOOL cancel = FALSE;
    if (data->CurrentIndex >= data->IndexesCount || data->WorkPath[0] == 0)
    {
        TRACE_E("Unexpected situation in ReadDirectoryTree().");
        if (errorOccured != NULL)
            *errorOccured = SALENUM_ERROR;
        return NULL; // nothing to do
    }

    BOOL errGetFileSizeOfLnkTgtIgnAll = parent == NULL; // silent mode = do not show an error, return success

    CSalamanderDirectory* dir = containsDirLinks == NULL ? new CSalamanderDirectory(TRUE) : NULL;
    if (dir == NULL && containsDirLinks == NULL)
    {
        if (errorOccured != NULL)
            *errorOccured = SALENUM_ERROR;
        return NULL; // out of memory
    }

    int index = data->CurrentIndex;
    CFileData newF = {0};
    if (dir != NULL)
    {
        newF.PluginData = -1; // -1 is arbitrary, ignored
        newF.Association = 0;
        newF.Selected = 0;
        newF.Shared = 0;
        newF.Archive = 0;
        newF.SizeValid = 0;
        newF.Dirty = 0; // unnecessary, just to keep structure consistent
        newF.CutToClip = 0;
        newF.IconOverlayIndex = ICONOVERLAYINDEX_NOTUSED;
        newF.IconOverlayDone = 0;
    }
    else
        memset(&newF, 0, sizeof(newF));
    while (index < data->IndexesCount)
    {
        if (!data->HasCurrentContext())
        {
            if (dir != NULL) delete dir;
            SetLastError(ERROR_RETRY);
            if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
            return NULL;
        }
        int i = data->Indexes[index++];
        BOOL isDir = i < data->Dirs->Count;
        CFileData* f = &(isDir ? data->Dirs->At(i) : data->Files->At(i - data->Dirs->Count));
        // skip ".." as a precaution (there was a bug report in "1.6 beta 5" about this; unclear how it could occur here)
        if (f->Name[0] == '.' && f->Name[1] == '.' && f->Name[2] == 0)
            continue;

        if (dir != NULL)
        {
            newF.Name = DupStr(f->Name);
            newF.DosName = NULL;
            if (newF.Name == NULL)
                goto RETURN_ERROR;
            if (f->DosName != NULL)
            {
                newF.DosName = DupStr(f->DosName);
                if (newF.DosName == NULL)
                    goto RETURN_ERROR;
            }
            newF.NameLen = f->NameLen;
            newF.Ext = newF.Name + (f->Ext - f->Name);
            newF.Size = f->Size;
            newF.Attr = f->Attr;
            newF.LastWrite = f->LastWrite;
            newF.Hidden = f->Hidden;
            newF.IsLink = f->IsLink;
            newF.IsOffline = f->IsOffline;
        }

        char path[SAL_MAX_PATH];
        if (isDir) // directory
        {
            CSalamanderDirectory* salDir = NULL;
            if (dir != NULL)
            {
                BOOL addDirOK = dir->AddDir("", newF, NULL);
                if (addDirOK)
                {
                    newF.Name = NULL; // already in dir, must not call free() on it - in case of an error below
                    newF.DosName = NULL;
                    salDir = dir->GetSalamanderDir(f->Name, FALSE); // allocate a sal-dir for the record
                }
                if (salDir == NULL)
                    goto RETURN_ERROR;
            }
            else
            {
                if ((f->Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0) // directory link found, stop...
                {
                    *containsDirLinks = 1;
                    strcpy(path, data->WorkPath);
                    SalPathRemoveBackslash(path);
                    _snprintf_s(linkName, SAL_MAX_PATH, _TRUNCATE, "%s\\%s", path, f->Name); // truncation is fine, it is only for the message text
                    break;
                }
            }

            strcpy(path, data->WorkPath);
            if (!_ReadDirectoryTree(parent, path, f->Name, salDir, errorOccured, getLinkTgtFileSize,
                                    &errGetFileSizeOfLnkTgtIgnAll, containsDirLinks, linkName, data->WideDiskNames))
            {
                goto RETURN_ERROR;
            }
        }
        else // file
        {
            if (dir != NULL)
            {
                if (getLinkTgtFileSize && (newF.Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
                { // for a symlink determine the target file size
                    CQuadWord size;
                    strcpy(path, data->WorkPath);
                    if (SalPathAppend(path, newF.Name, _countof(path)))
                    { // only if the path is not too long (any resulting error will be reported during enumeration)
                        if (GetLinkTgtFileSize(parent, path, NULL, &size, &cancel, &errGetFileSizeOfLnkTgtIgnAll))
                            newF.Size = size;
                        else
                        {
                            if (cancel && errorOccured != NULL)
                                *errorOccured = SALENUM_CANCEL;
                        }
                    }
                }
                if (cancel || !dir->AddFile("", newF, NULL))
                {
                RETURN_ERROR:

                    if (errorOccured != NULL && *errorOccured == SALENUM_SUCCESS)
                        *errorOccured = SALENUM_ERROR;
                    if (newF.Name != NULL)
                        free(newF.Name);
                    if (newF.DosName != NULL)
                        free(newF.DosName);
                    if (dir != NULL)
                        delete dir;
                    return NULL;
                }
            }
        }
    }
    return dir;
}

const char* WINAPI PanelEnumDiskSelection(HWND parent, int enumFiles, const char** dosName, BOOL* isDir,
                                          CQuadWord* size, DWORD* attr, FILETIME* lastWrite, void* param,
                                          int* errorOccured)
{
    CALL_STACK_MESSAGE_NONE
    CPanelTmpEnumData* data = (CPanelTmpEnumData*)param;
    if (errorOccured != NULL)
        *errorOccured = SALENUM_SUCCESS;

    if (enumFiles == -1)
    {
        if (dosName != NULL)
            *dosName = NULL;
        if (isDir != NULL)
            *isDir = FALSE;
        if (size != NULL)
            *size = CQuadWord(0, 0);
        if (attr != NULL)
            *attr = 0;
        if (lastWrite != NULL)
            memset(lastWrite, 0, sizeof(FILETIME));
        data->Reset();
        return NULL;
    }

    if (!data->HasCurrentContext())
    {
        SetLastError(ERROR_RETRY);
        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
        return NULL;
    }

    if (data->SourcePanel != NULL && data->ExactFileSelection)
    {
        if (data->CurrentIndex >= data->IndexesCount)
            return NULL;
        const int selectionIndex = data->CurrentIndex++;
        const int index = data->Indexes[selectionIndex];
        if (index < data->Dirs->Count || index >= data->Dirs->Count + data->Files->Count)
        {
            if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
            return NULL;
        }
        const CFileData& file = data->Files->At(index - data->Dirs->Count);
        CQuadWord fileSize = file.Size;
        try
        {
            const std::wstring relativePath = data->SourcePanel->GetItemRelativePathW(file);
            data->BranchEnumName = SalWideToMultiBytePath(relativePath.c_str(), CP_UTF8);
            if (relativePath.empty() || data->BranchEnumName.empty())
            {
                if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
                SetLastError(ERROR_INVALID_DATA);
                return NULL;
            }
            if (enumFiles == 3 && (file.Attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            {
                if (data->BranchEnumLinkSizeValid.size() != static_cast<size_t>(data->IndexesCount))
                {
                    data->BranchEnumLinkSizes.resize(data->IndexesCount);
                    data->BranchEnumLinkSizeValid.assign(data->IndexesCount, FALSE);
                }
                if (data->BranchEnumLinkSizeValid[selectionIndex])
                    fileSize = data->BranchEnumLinkSizes[selectionIndex];
                else
                {
                    const std::wstring fullPath = data->SourcePanel->GetItemFullPathW(file);
                    const std::string encodedPath = SalWideToMultiBytePath(fullPath.c_str(), CP_UTF8);
                    if (fullPath.empty() || encodedPath.empty())
                    {
                        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
                        SetLastError(ERROR_INVALID_DATA);
                        return NULL;
                    }
                    BOOL cancel = FALSE;
                    CQuadWord targetSize;
                    if (GetLinkTgtFileSize(parent, encodedPath.c_str(), NULL, &targetSize,
                                          &cancel, &data->ErrGetFileSizeOfLnkTgtIgnAll))
                        fileSize = targetSize;
                    else if (cancel)
                    {
                        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
                        return NULL;
                    }
                    if (!data->HasCurrentContext())
                    {
                        SetLastError(ERROR_RETRY);
                        if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
                        return NULL;
                    }
                    // A move packer may enumerate again after deleting its
                    // sources. Retain the resolved (or ignored) size on reset,
                    // just as ordinary enumeration retains DiskDirectoryTree.
                    data->BranchEnumLinkSizes[selectionIndex] = fileSize;
                    data->BranchEnumLinkSizeValid[selectionIndex] = TRUE;
                }
            }
        }
        catch (const std::bad_alloc&)
        {
            if (errorOccured != NULL) *errorOccured = SALENUM_CANCEL;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        if (dosName != NULL) *dosName = NULL;
        if (isDir != NULL) *isDir = FALSE;
        if (size != NULL) *size = fileSize;
        if (attr != NULL) *attr = file.Attr;
        if (lastWrite != NULL) *lastWrite = file.LastWrite;
        return data->BranchEnumName.c_str();
    }

    if (enumFiles > 0)
    {
        if (data->DiskDirectoryTree == NULL)
        {
            data->DiskDirectoryTree = ReadDirectoryTree(parent, data, errorOccured, enumFiles == 3, NULL, NULL);
            if (data->DiskDirectoryTree == NULL)
                return NULL; // error, stop
        }
        const CFileData* f = NULL;
        const char* ret = _PanelSalEnumSelection(enumFiles, dosName, isDir, size, &f, data, parent, errorOccured);
        if (ret != NULL)
        {
            if (f != NULL)
            {
                if (attr != NULL)
                    *attr = f->Attr;
                if (lastWrite != NULL)
                    *lastWrite = f->LastWrite;
            }
            else
            {
                if (attr != NULL)
                    *attr = 0;
                if (lastWrite != NULL)
                    memset(lastWrite, 0, sizeof(FILETIME));
            }
        }
        return ret;
    }
    else
    {
        if (data->CurrentIndex >= data->IndexesCount)
            return NULL;
        int i = data->Indexes[data->CurrentIndex++];
        if (isDir != NULL)
            *isDir = i < data->Dirs->Count;
        CFileData* f = &(i < data->Dirs->Count ? data->Dirs->At(i) : data->Files->At(i - data->Dirs->Count));
        if (dosName != NULL)
            *dosName = (f->DosName == NULL) ? f->Name : f->DosName;
        if (size != NULL)
            *size = f->Size;
        if (attr != NULL)
            *attr = f->Attr;
        if (lastWrite != NULL)
            *lastWrite = f->LastWrite;
        return f->Name;
    }
}


static BOOL UnpackArchiveToPluginFSViaTemp(CFilesWindow* source, CFilesWindow* target,
                                           CPanelTmpEnumData* data, int sourceFiles, int sourceDirs,
                                           char* targetPath, BOOL& invalidPathOrCancel)
{
    CALL_STACK_MESSAGE2("UnpackArchiveToPluginFSViaTemp(, , , , , %s,)", targetPath);

    invalidPathOrCancel = FALSE;
    if (!data->HasCurrentContext())
    { SetLastError(ERROR_RETRY); invalidPathOrCancel = TRUE; return FALSE; }

    if (target == NULL || !target->Is(ptPluginFS) || !target->GetPluginFS()->NotEmpty() ||
        !target->GetPluginFS()->IsServiceSupported(FS_SERVICE_COPYFROMDISKTOFS))
    {
        return FALSE;
    }

    int pathType;
    BOOL pathIsDir;
    char* secondPart;
    if (!source->ParsePath(targetPath, pathType, pathIsDir, secondPart, LoadStr(IDS_ERRORCOPY), NULL, NULL, MAX_PATH) ||
        pathType != PATH_TYPE_FS)
    {
        return FALSE;
    }

    char fsName[MAX_PATH];
    int fsNameLen = (int)((secondPart - targetPath) - 1);
    if (fsNameLen <= 0 || fsNameLen >= _countof(fsName))
        return FALSE;
    memcpy(fsName, targetPath, fsNameLen);
    fsName[fsNameLen] = 0;

    int fsNameIndex;
    if (!target->GetPluginFS()->IsFSNameFromSamePluginAsThisFS(fsName, fsNameIndex))
        return FALSE;

    // From this point the path is a valid path for the active target plug-in FS.
    // Any later failure is reported by extraction/upload code, not by the old "disk only" fallback.
    invalidPathOrCancel = TRUE;

    char tempRoot[MAX_PATH];
    if (!SalGetTempFileName(NULL, "ARC", tempRoot, FALSE))
    {
        SalMessageBox(source->HWindow, LoadStr(IDS_TMPDIRERROR), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        invalidPathOrCancel = TRUE;
        return FALSE;
    }

    BOOL done = FALSE;
    data->Reset();
    data->WorkPath[0] = 0;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    if (data->HasCurrentContext() && PackUncompress(MainWindow->HWindow, source, source->GetZIPArchive(), source->PluginData.GetInterface(),
                       tempRoot, source->GetZIPPath(), PanelSalEnumSelection, data, NULL, data->ContextIsCurrent, data->Context) && data->HasCurrentContext())
    {
        data->Reset();
        lstrcpyn(data->WorkPath, tempRoot, _countof(data->WorkPath));

        char internalTargetPath[2 * MAX_PATH];
        lstrcpyn(internalTargetPath, targetPath, 2 * MAX_PATH);
        target->GetPluginFS()->GetPluginInterfaceForFS()->ConvertPathToInternal(fsName, fsNameIndex,
                                                                                 internalTargetPath + strlen(fsName) + 1);

        BOOL moveTempToTarget = target->GetPluginFS()->IsServiceSupported(FS_SERVICE_MOVEFROMDISKTOFS);
        BOOL pluginInvalidPathOrCancel = FALSE;
        if (data->HasCurrentContext() && target->GetPluginFS()->CopyOrMoveFromDiskToFS(!moveTempToTarget, 3, target->GetPluginFS()->GetPluginFSName(),
                                                          source->HWindow, tempRoot, PanelEnumDiskSelection, data,
                                                          sourceFiles, sourceDirs, internalTargetPath,
                                                          &pluginInvalidPathOrCancel))
        {
            done = !pluginInvalidPathOrCancel;
        }
        else if (pluginInvalidPathOrCancel)
        {
            target->GetPluginFS()->GetPluginInterfaceForFS()->ConvertPathToExternal(fsName, fsNameIndex,
                                                                                     internalTargetPath + strlen(fsName) + 1);
            lstrcpyn(targetPath, internalTargetPath, MAX_PATH);
            invalidPathOrCancel = TRUE;
        }
    }
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    if (!done)
        RemoveTemporaryDir(tempRoot);
    data->Reset();
    data->WorkPath[0] = 0;

    return done;
}


static BOOL UnpackArchiveToArchiveViaTemp(CFilesWindow* source, CPanelTmpEnumData* data,
                                         char* targetPath, BOOL& invalidPathOrCancel)
{
    CALL_STACK_MESSAGE2("UnpackArchiveToArchiveViaTemp(, , %s,)", targetPath);

    invalidPathOrCancel = FALSE;
    if (!data->HasCurrentContext())
    { SetLastError(ERROR_RETRY); invalidPathOrCancel = TRUE; return FALSE; }

    char archivePath[2 * MAX_PATH];
    lstrcpyn(archivePath, targetPath, 2 * MAX_PATH);

    int pathType;
    BOOL pathIsDir;
    char* secondPart;
    if (!source->ParsePath(archivePath, pathType, pathIsDir, secondPart, LoadStr(IDS_ERRORCOPY), NULL, NULL, MAX_PATH) ||
        pathType != PATH_TYPE_ARCHIVE)
    {
        return FALSE;
    }

    if (strlen(secondPart) >= SAL_MAX_PATH)
    {
        SalMessageBox(source->HWindow, LoadStr(IDS_TOOLONGPATH), LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
        invalidPathOrCancel = TRUE;
        return FALSE;
    }

    char archiveRoot[MAX_PATH];
    archiveRoot[0] = 0;
    if (*secondPart != 0)
    {
        lstrcpyn(archiveRoot, secondPart + 1, MAX_PATH);
        char* opMask = strrchr(archiveRoot, '\\');
        char* opMask2 = strrchr(archiveRoot, '/');
        if (opMask == NULL || opMask2 > opMask)
            opMask = opMask2;
        if (opMask == NULL)
            opMask = archiveRoot;
        else
            opMask++;

        if (strcmp(opMask, "*.*") == 0 || strcmp(opMask, "*") == 0)
        {
            if (opMask == archiveRoot)
                archiveRoot[0] = 0;
            else
                *opMask = 0; // keep the archive subdirectory path including the trailing backslash
        }
        else
        {
            if (strchr(archiveRoot, '*') != NULL || strchr(archiveRoot, '?') != NULL)
            {
                SalMessageBox(source->HWindow, LoadStr(IDS_MOVECOPY_OPMASKSNOTSUP),
                              LoadStr(IDS_ERRORCOPY), MB_OK | MB_ICONEXCLAMATION);
                invalidPathOrCancel = TRUE;
                return FALSE;
            }
        }
    }
    *secondPart = 0;

    // From this point the path is a syntactically valid archive target.
    // Any later failure is reported by the unpack/pack code; do not fall back to the old "disk only" message.
    invalidPathOrCancel = TRUE;

    int format = PackerFormatConfig.PackIsArchive(archivePath);
    if (format == 0 || !PackerFormatConfig.GetUsePacker(format - 1))
        return FALSE;

    char tempRoot[MAX_PATH];
    if (!SalGetTempFileName(NULL, "ARC", tempRoot, FALSE))
    {
        SalMessageBox(source->HWindow, LoadStr(IDS_TMPDIRERROR), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        invalidPathOrCancel = TRUE;
        return FALSE;
    }

    BOOL done = FALSE;
    data->Reset();
    data->WorkPath[0] = 0;

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    if (data->HasCurrentContext() && PackUncompress(MainWindow->HWindow, source, source->GetZIPArchive(), source->PluginData.GetInterface(),
                       tempRoot, source->GetZIPPath(), PanelSalEnumSelection, data, NULL, data->ContextIsCurrent, data->Context) && data->HasCurrentContext())
    {
        data->Reset();
        lstrcpyn(data->WorkPath, tempRoot, _countof(data->WorkPath));

        SetCurrentDirectory(tempRoot);
        done = PackCompress(source->HWindow, source, archivePath, archiveRoot,
                            FALSE, tempRoot, PanelEnumDiskSelection, data);
        SetCurrentDirectoryToSystem();

        if (!done)
            invalidPathOrCancel = TRUE; // PackCompress already reported/cancelled; do not show the old unsupported fallback.

        if (done)
        {
            char changedPath[MAX_PATH];
            lstrcpyn(changedPath, archivePath, MAX_PATH);
            CutDirectory(changedPath);
            MainWindow->PostChangeOnPathNotification(changedPath, FALSE);
        }
    }
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    RemoveTemporaryDir(tempRoot);
    data->Reset();
    data->WorkPath[0] = 0;

    invalidPathOrCancel = TRUE;
    return done;
}

// Qualified Pack overwrite uses the exact wide destination and clears only RO.
static BOOL RemoveQualifiedPackArchive(const char* archiveFile)
try
{
    std::wstring path = SalMultiByteToWidePath(archiveFile, IsValidPathUtf8Text(archiveFile) ? CP_UTF8 : CP_ACP);
    if (path.empty()) { SetLastError(ERROR_INVALID_NAME); return FALSE; }
    path = SalPathAddExtendedPrefixW(path.c_str());
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) return FALSE;
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0 &&
        !SetFileAttributesW(path.c_str(), attributes & ~FILE_ATTRIBUTE_READONLY)) return FALSE;
    return DeleteFileW(path.c_str());
}
catch (const std::bad_alloc&)
{ SetLastError(ERROR_NOT_ENOUGH_MEMORY); return FALSE; }

// S27e2 staged activation: only the bundled ZIP/7-Zip packers are verified.
static BOOL IsVerifiedBranchPacker()
{
    const int index = PackerConfig.GetPreferedPacker();
    if (index < 0 || index >= PackerConfig.GetPackersCount())
        return FALSE;
    const int type = PackerConfig.GetPackerType(index);
    if (type >= 0)
        return FALSE;
    const CPluginData* plugin = Plugins.Get(-(type + 1));
    return plugin != NULL && plugin->SupportCustomPack && plugin->DLLName != NULL &&
        (_stricmp(plugin->DLLName, "zip\\zip.spl") == 0 ||
         _stricmp(plugin->DLLName, "7zip\\7zip.spl") == 0);
}

void CFilesWindow::Pack(CFilesWindow* target, int pluginIndex, const char* pluginName, int delFilesAfterPacking)
try
{
    if (CFilesActionDiskTarget::FindLive(MainWindow, this) == NULL ||
        CFilesActionDiskTarget::FindLive(MainWindow, target) == NULL) return;
    const bool qualifiedPack = IsBranchView() || target->IsBranchView();
    if (qualifiedPack && (!Is(ptDisk) || !target->Is(ptDisk) || FilesActionInProgress)) return;
    const HWND operationParent = MainWindow->HWindow;
    CFilesActionPackContext packContext;
    if (qualifiedPack && !packContext.Capture(MainWindow, this, target))
    {
        SalMessageBox(operationParent, GetErrorText(ERROR_RETRY), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    struct CPackSourceBusyScope
    {
        CFilesActionPackContext* Context;
        BOOL Previous;
        CPackSourceBusyScope(CFilesActionPackContext* context) : Context(context), Previous(FALSE)
        { if (Context != NULL) { Previous = Context->Source.Panel->FilesActionInProgress; Context->Source.Panel->FilesActionInProgress = TRUE; } }
        ~CPackSourceBusyScope()
        {
            if (Context != NULL && CFilesActionDiskTarget::FindLive(MainWindow, Context->Source.Panel) != NULL &&
                Context->Source.Panel->GetPanelTabId() == Context->Source.Tab)
                Context->Source.Panel->FilesActionInProgress = Previous;
        }
    } sourceBusy(qualifiedPack ? &packContext : NULL);
    const auto contextCurrent = [&]() { return !qualifiedPack || packContext.IsCurrent(MainWindow); };
    const auto reportChangedContext = [&]() {
        SalMessageBox(operationParent, GetErrorText(ERROR_RETRY), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
    };

    CALL_STACK_MESSAGE4("CFilesWindow::Pack(, %d, %s, %d)", pluginIndex, pluginName, delFilesAfterPacking);
    if (Files->Count + Dirs->Count == 0)
        return;

    // restore DefaultDir
    MainWindow->UpdateDefaultDir(MainWindow->GetActivePanel() == this);

    BeginStopRefresh(); // the snooper takes a break
    struct CPackRefreshScope { ~CPackRefreshScope() { EndStopRefresh(); } } refreshScope;

    //---  obtain the files and directories to work with
    CPathBuffer subjectBuffer(SAL_MAX_PATH + 100);
    char* subject = subjectBuffer.Data(); // text for the Unpack dialog (that is being unpacked)
    CPathBuffer pathBuffer(SAL_MAX_PATH);
    char* path = pathBuffer.Data();
    char text[1000];
    BOOL nameByItem;
    CPanelTmpEnumData data;
    std::unique_ptr<int[]> indexesOwner;
    BOOL subDir;
    if (Dirs->Count > 0)
        subDir = (strcmp(Dirs->At(0).Name, "..") == 0);
    else
        subDir = FALSE;
    data.IndexesCount = GetSelCount();
    CPathBuffer expandedBuffer(SAL_MAX_PATH + 100);
    char* expanded = expandedBuffer.Data();
    CPathBuffer archiveBaseNameBuffer(SAL_MAX_PATH);
    char* archiveBaseName = archiveBaseNameBuffer.Data();
    archiveBaseName[0] = 0;
    int files = 0;             // number of selected files
    if (data.IndexesCount > 1) // valid selection
    {
        nameByItem = FALSE;
#ifdef new
#undef new
#define RESTORE_BRANCH_PACK_SELECTION_DEBUG_NEW_MACRO
#endif
        data.Indexes = new (std::nothrow) int[data.IndexesCount];
        indexesOwner.reset(data.Indexes);
#ifdef RESTORE_BRANCH_PACK_SELECTION_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_BRANCH_PACK_SELECTION_DEBUG_NEW_MACRO
#endif
        if (data.Indexes == NULL)
        {
            TRACE_E(LOW_MEMORY);

            return;
        }
        else
        {
            GetSelItems(data.IndexesCount, data.Indexes);
            int i = data.IndexesCount;
            while (i--)
            {
                BOOL isDir = data.Indexes[i] < Dirs->Count;
                CFileData* f = isDir ? &Dirs->At(data.Indexes[i]) : &Files->At(data.Indexes[i] - Dirs->Count);
                if (!isDir)
                    files++;
            }
        }
        // build the subject for the dialog
        ExpandPluralFilesDirs(expanded, expandedBuffer.Capacity(), files, data.IndexesCount - files, epfdmNormal, FALSE);
    }
    else // take the selected file or directory
    {
        int index;
        if (data.IndexesCount == 0)
        {
            index = GetCaretIndex();
            nameByItem = TRUE; // for compatibility with Sal 2.0
        }
        else
        {
            GetSelItems(1, &index);
            nameByItem = FALSE; // for compatibility with Sal 2.0
        }

        // note about compatibility with 2.0
        // the current implementation is illogical: with a single selected item
        // Salamander behaves differently than with a single focused item, but it has one advantage:
        // the user can choose the suggested file name

        if (subDir && index == 0)
        {

            return;           // nothing to do
        }
        else
        {
#ifdef new
#undef new
#define RESTORE_BRANCH_PACK_SELECTION_DEBUG_NEW_MACRO
#endif
            data.Indexes = new (std::nothrow) int[1];
            indexesOwner.reset(data.Indexes);
#ifdef RESTORE_BRANCH_PACK_SELECTION_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_BRANCH_PACK_SELECTION_DEBUG_NEW_MACRO
#endif
            if (data.Indexes == NULL)
            {
                TRACE_E(LOW_MEMORY);

                return;
            }
            else
            {
                data.Indexes[0] = index;
                data.IndexesCount = 1;
                // build the subject for the dialog
                BOOL isDir = index < Dirs->Count;
                if (!isDir)
                    files = 1;
                CFileData* f = isDir ? &Dirs->At(index) : &Files->At(index - Dirs->Count);
                lstrcpyn(archiveBaseName, f->Name, archiveBaseNameBuffer.Capacity());
                AlterFileName(path, f->Name, -1, Configuration.FileNameFormat, 0, index < Dirs->Count);
                strcpy(expanded, LoadStr(isDir ? IDS_QUESTION_DIRECTORY : IDS_QUESTION_FILE));
            }
        }
    }
    sprintf(subject, LoadStr(IDS_PACKTOARCHIVE), expanded);
    CTruncatedString str;
    str.Set(subject, data.IndexesCount > 1 ? NULL : path);

    data.CurrentIndex = 0;
    data.ZIPPath = GetZIPPath();
    data.Dirs = Dirs;
    data.Files = Files;
    if (qualifiedPack)
    {
        data.Context = &packContext;
        data.WideDiskNames = TRUE;
        data.ContextIsCurrent = [](void* context) -> BOOL {
            return static_cast<CFilesActionPackContext*>(context)->IsCurrent(MainWindow);
        };
    }
    data.SourcePanel = this;
    data.ExactFileSelection = Is(ptDisk);
    for (int item = 0; item < data.IndexesCount && data.ExactFileSelection; ++item)
        if (data.Indexes[item] < Dirs->Count) data.ExactFileSelection = FALSE;
    data.ArchiveDir = GetArchiveDir();
    lstrcpyn(data.WorkPath, GetPath(), _countof(data.WorkPath));
    data.EnumLastDir = NULL;
    data.EnumLastIndex = -1;

    //---  we are packing into a new file, ask for its name
    CPathBuffer fileBufBuffer(SAL_MAX_PATH);
    char* fileBuf = fileBufBuffer.Data();    // file we will pack into
    CPathBuffer fileBufAltBuffer(SAL_MAX_PATH);
    char* fileBufAlt = fileBufAltBuffer.Data(); // alternative name shown in the Pack dialog combo box

    if (data.IndexesCount == 1 && archiveBaseName[0] != 0) // if only one item (file/directory) is selected, the archive inherits its name
    {
        const char* itemName = archiveBaseName[0] != 0 ? archiveBaseName : path;
        const char* ext = strrchr(itemName, '.');
        if (data.Indexes[0] < Dirs->Count || ext == NULL) // ".cvspass" is treated as an extension in Windows ...
                                                          //  if (data.Indexes[0] < Dirs->Count || ext == NULL || ext == itemName)
        {                                                 // subdirectory or no extension
            lstrcpyn(fileBuf, itemName, fileBufBuffer.Capacity());
            if (strlen(fileBuf) + 1 < static_cast<size_t>(fileBufBuffer.Capacity()))
                strcat(fileBuf, ".");
        }
        else
        {
            memcpy(fileBuf, itemName, ext + 1 - itemName);
            fileBuf[ext + 1 - itemName] = 0;
        }
    }
    else
    {
        // build the default archive name
        const char* end = GetPath() + strlen(GetPath());
        if (end > GetPath() && *(end - 1) == '\\')
            end--;
        const char* dir = end;
        CPathBuffer rootBuffer(SAL_MAX_PATH);
        char* root = rootBuffer.Data();
        GetRootPath(root, GetPath());
        const char* min = GetPath() + strlen(root);
        while (dir > min && *(dir - 1) != '\\')
            dir--;
        if (dir < end)
        {
            memcpy(fileBuf, dir, end - dir);
            fileBuf[end - dir] = '.';
            fileBuf[end - dir + 1] = 0;
        }
        else
            strcpy(fileBuf, LoadStr(IDS_NEW_ARCHIVE));
    }

    if (pluginIndex != -1)
    {
        int i;
        for (i = 0; i < PackerConfig.GetPackersCount(); i++)
        {
            if (PackerConfig.GetPackerType(i) == -pluginIndex - 1)
            {
                PackerConfig.SetPreferedPacker(i);
                break;
            }
        }
        if (i == PackerConfig.GetPackersCount()) // the requested plugin was not found
        {
            sprintf(subject, LoadStr(IDS_PLUGINPACKERNOTFOUND), pluginName);
            SalMessageBox(HWindow, subject, LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
            indexesOwner.reset();

            return;
        }
    }

    if (PackerConfig.GetPreferedPacker() == -1)
    { // if no preferred packer is set, choose the first one so users do not stare at an empty combo box
        PackerConfig.SetPreferedPacker(0);
    }
    if (PackerConfig.GetPreferedPacker() != -1) // necessary even after Set (it might have failed -> still returns -1)
        strcat(fileBuf, PackerConfig.GetPackerExt(PackerConfig.GetPreferedPacker()));

    strcpy(fileBufAlt, fileBuf);

    if (target->Is(ptDisk))
    {
        // based on the configuration adjust one of the paths so it goes to the target panel
        char* buff = Configuration.UseAnotherPanelForPack ? fileBuf : fileBufAlt;

        if (Configuration.UseAnotherPanelForPack)
            target->UserWorkedOnThisPath = TRUE; // default action = work with the path in the target panel

        const char* targetRoot = qualifiedPack ? packContext.Target.DefaultPath.c_str() : target->GetPath();
        int l = (int)strlen(targetRoot);
        if (l > 0 && targetRoot[l - 1] == '\\')
            l--;
        int ll = (int)strlen(buff);
        if (l + 2 + ll < SAL_MAX_PATH)
        {
            memmove(buff + l + 1, buff, ll + 1);
            buff[l] = '\\';
            memcpy(buff, targetRoot, l);
        }
        else if (qualifiedPack)
        {
            SalMessageBox(operationParent, LoadStr(IDS_TOOLONGPATH), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
            indexesOwner.reset();

            return;
        }
    }

    // if no item is selected, choose the focused item and store its name
    CPanelTemporarySelection temporarySelected;
    temporarySelected.Clear();
    SelectFocusedItemAndGetName(temporarySelected);

    if (delFilesAfterPacking == 1)
        PackerConfig.Move = TRUE;
    if (delFilesAfterPacking == 0 || // Petr: changed default - user must always enable deletion, it is too risky
        delFilesAfterPacking == 2)
    {
        PackerConfig.Move = FALSE;
    }

    BOOL first = TRUE;

_PACK_AGAIN:

    if (qualifiedPack) packContext.Backend = NULL; // A confirmed retry may choose a different packer.
    if (!contextCurrent())
    {
        reportChangedContext();
        indexesOwner.reset();

        return;
    }

    CPackDialog dlg(HWindow, fileBuf, fileBufAlt, &str, &PackerConfig);

    // Since Windows Vista Microsoft introduced an odd behavior: quick rename selects only the name without the dot and extension
    // the same code is in another place as well
    int selectionEnd = -1;
    if (first)
    {
        if (!Configuration.QuickRenameSelectAll)
        {
            const char* dot = strrchr(fileBuf, '.');
            if (dot != NULL && dot > fileBuf) // although ".cvspass" is technically an extension in Windows, Explorer selects the entire name, so we do the same
                                              //    if (dot != NULL)
                selectionEnd = (int)(dot - fileBuf);
            dlg.SetSelectionEnd(selectionEnd);
        }
        first = FALSE; // after an error we get the full filename, so select it entirely
    }

    if (dlg.Execute() == IDOK)
    {
        UpdateWindow(MainWindow->HWindow);
        if (!contextCurrent())
        { reportChangedContext(); indexesOwner.reset(); return; }
        if (qualifiedPack && !IsVerifiedBranchPacker())
        {
            SalMessageBox(HWindow, GetErrorText(ERROR_NOT_SUPPORTED), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
            goto _PACK_AGAIN;
        }
        if (qualifiedPack && !packContext.BindBackend(MainWindow))
        {
            SalMessageBox(HWindow, GetErrorText(GetLastError()), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
            goto _PACK_AGAIN;
        }
        //--- adjust the archive name to its full form
        int errTextID;
        BOOL empty = FALSE;
        CPathBuffer nextFocusBuffer(SAL_MAX_PATH);
        char* nextFocus = nextFocusBuffer.Data();
        nextFocus[0] = 0;
        if (SalGetFullName(fileBuf, &errTextID, Is(ptDisk) ? GetPath() : NULL, nextFocus, NULL, SAL_MAX_PATH))
        {
            //---  searching for a directory link in the packing source; cannot be combined with "delete files after packing"
            BOOL performPack = TRUE;
            if (PackerConfig.Move)
            {
                int containsDirLinks = 0;
                SetCurrentDirectory(GetPath());
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

                GetAsyncKeyState(VK_ESCAPE); // initialize GetAsyncKeyState - see help
                CreateSafeWaitWindow(LoadStr(IDS_ANALYSINGDIRTREEESC), NULL, 3000, TRUE, NULL);

                // try to find the first directory link; if found, simulate an error to stop the search
                CPathBuffer linkNameBuffer(SAL_MAX_PATH);
                char* linkName = linkNameBuffer.Data();
                linkName[0] = 0;
                ReadDirectoryTree(NULL /* silent mode */, &data, NULL, FALSE, &containsDirLinks, linkName);

                DestroySafeWaitWindow();

                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                SetCurrentDirectoryToSystem();
                // the directory contains a link and cannot be combined with "delete files after packing":
                // I can't handle the situation where packing a single file from the directory fails
                // (e.g., if the file is locked or access is denied), and I entered the directory via the link.
                // Deleting the whole link is wrong because it won't show that packing failed,
                // and deleting everything except one file after traversing the link is also wrong,
                // because it alters the original directory content, which users report as a bug since it's unexpected.
                if (containsDirLinks == 1)
                {
                    _snprintf_s(text, _TRUNCATE, LoadStr(IDS_DELFILESAFTERPACKINGNOLINKS), linkName);
                    SalMessageBox(HWindow, text, LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
                    PackerConfig.Move = FALSE;
                    goto _PACK_AGAIN;
                }
                if (containsDirLinks == 2) // user canceled loading (ESC pressed or closed the wait window)
                    performPack = FALSE;
            }

            if (!contextCurrent())
            { reportChangedContext(); indexesOwner.reset(); return; }

            //--- confirmation for adding (updating) to an existing archive
            if (performPack && Configuration.CnfrmAddToArchive && (qualifiedPack ?
                GetFileAttributesW(SalPathAddExtendedPrefixW(SalMultiByteToWidePath(fileBuf, IsValidPathUtf8Text(fileBuf) ? CP_UTF8 : CP_ACP).c_str()).c_str()) != INVALID_FILE_ATTRIBUTES : FileExists(fileBuf)))
            {
                BOOL dontShow = !Configuration.CnfrmAddToArchive;

                char filesDirs[200];
                ExpandPluralFilesDirs(filesDirs, 200, files, data.IndexesCount - files, epfdmNormal, FALSE);

                char buff[3 * MAX_PATH];
                sprintf(buff, LoadStr(IDS_CONFIRM_ADDTOARCHIVE), "%s", filesDirs);

                char* namePart = strrchr(fileBuf, '\\');
                if (namePart != NULL)
                    namePart++;
                else
                    namePart = fileBuf;
                CTruncatedString str2;
                str2.Set(buff, namePart);

                char alias[200];
                sprintf(alias, "%d\t%s\t%d\t%s",
                        DIALOG_YES, LoadStr(IDS_CONFIRM_ADDTOARCHIVE_ADD),
                        DIALOG_NO, LoadStr(IDS_CONFIRM_ADDTOARCHIVE_OVER));
                CMessageBox msgBox(HWindow,
                                   MSGBOXEX_YESNOCANCEL | MSGBOXEX_ESCAPEENABLED | MSGBOXEX_ICONQUESTION | MSGBOXEX_SILENT | MSGBOXEX_HINT,
                                   LoadStr(IDS_QUESTION),
                                   &str2,
                                   LoadStr(IDS_CONFIRM_ADDTOARCHIVE_NOASK),
                                   &dontShow,
                                   NULL, 0, NULL, alias, NULL, NULL);
                int msgBoxRed = msgBox.Execute();
                if (!contextCurrent())
                { reportChangedContext(); indexesOwner.reset(); return; }
                performPack = (msgBoxRed == IDYES);
                if (msgBoxRed == IDNO) // OVERWRITE
                {
                    if (!qualifiedPack) ClearReadOnlyAttr(fileBuf); // so it can be deleted...
                    if (!(qualifiedPack ? RemoveQualifiedPackArchive(fileBuf) : DeleteFile(fileBuf)))
                    {
                        DWORD err;
                        err = GetLastError();
                        SalMessageBox(HWindow, GetErrorText(err), LoadStr(IDS_ERROROVERWRITINGFILE), MB_OK | MB_ICONEXCLAMATION);
                        // fall through to _PACK_AGAIN
                    }
                    else
                        performPack = TRUE;
                }
                Configuration.CnfrmAddToArchive = !dontShow;
                if (!performPack)
                    goto _PACK_AGAIN;
            }

            //---  actual packing
            if (performPack)
            {
                if (!contextCurrent())
                { reportChangedContext(); indexesOwner.reset(); return; }
                const char* sourceRoot = qualifiedPack ? packContext.Source.DefaultPath.c_str() : GetPath();
                SetCurrentDirectory(sourceRoot);
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
                if (PackerConfig.ExecutePacker(this, fileBuf, PackerConfig.Move, sourceRoot,
                                               PanelEnumDiskSelection, &data, data.ContextIsCurrent, data.Context) && contextCurrent())
                { // packing succeeded
                    // if (nextFocus[0] != 0) strcpy(NextFocusName, nextFocus);
                    FocusFirstNewItem = TRUE; // focus also archives renamed by a plugin (e.g. SFX -> archive.exe)

                    SetSel(FALSE, -1, TRUE);                        // explicit redraw
                    PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0); // sel-change notify
                }
                SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
                SetCurrentDirectoryToSystem();

                //---  refresh directories that are not automatically refreshed
                // changes in the directory where the new archive is located
                CutDirectory(fileBuf); // may fail, but we do not handle this case (an extra refresh is harmless)
                MainWindow->PostChangeOnPathNotification(fileBuf, FALSE);
                // moving from disk to archive -> also a change on disk (files/directories removed)
                if (PackerConfig.Move)
                {
                    // changes in the current directory in the panel including its subdirectories
                    MainWindow->PostChangeOnPathNotification(qualifiedPack ? packContext.Source.DefaultPath.c_str() : GetPath(), TRUE);
                }
            }
        }
        else
        {
            SalMessageBox(HWindow, LoadStr(errTextID), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
            goto _PACK_AGAIN;
        }
    }

    // if we selected an item, deselect it again
    if (contextCurrent()) UnselectItemWithName(temporarySelected);

    UpdateWindow(MainWindow->HWindow);
    indexesOwner.reset();

    //---  if any Salamander window is active, suspend mode ends

}
catch (const std::bad_alloc&)
{
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    SalMessageBox(MainWindow != NULL ? MainWindow->HWindow : NULL,
                  GetErrorText(ERROR_NOT_ENOUGH_MEMORY), LoadStr(IDS_PACKTITLE), MB_OK | MB_ICONEXCLAMATION);
}

// Capture the actual archive identity before modal callbacks; display names are not I/O paths.
static BOOL PrepareUnpackArchiveSource(CFilesWindow* panel, const CFileData& file,
                                       std::string& source, int& errorTextID, std::string* sourceDirectory = NULL)
{
    source.clear();
    if (sourceDirectory != NULL) sourceDirectory->clear();
    errorTextID = IDS_PATHISINVALID;
    const std::wstring exact = panel->GetItemFullPathW(file);
    if (exact.empty()) return FALSE;
    std::string encoded = SalWideToMultiBytePath(exact.c_str(), CP_UTF8);
    if (encoded.empty() || SalMultiByteToWidePath(encoded.c_str(), CP_UTF8) != exact)
        return FALSE;
    if (encoded.size() >= SAL_MAX_PATH)
    {
        errorTextID = IDS_TOOLONGPATH; // Legacy archiver API byte capacity; never pass a shortened identity.
        return FALSE;
    }
    if (sourceDirectory != NULL)
    {
        // Keep the parent of this exact item, including its drive/share root separator.
        const wchar_t* leaf = SalPathFindFileNameW(exact.c_str());
        if (leaf == exact.c_str() || *leaf == 0) return FALSE;
        const std::wstring parent(exact.c_str(), (size_t)(leaf - exact.c_str()));
        std::string parentText = SalWideToMultiBytePath(parent.c_str(), CP_UTF8);
        if (parentText.empty() || SalMultiByteToWidePath(parentText.c_str(), CP_UTF8) != parent)
            return FALSE;
        sourceDirectory->swap(parentText);
    }
    source.swap(encoded);
    return TRUE;
}


void CFilesWindow::Unpack(CFilesWindow* target, int pluginIndex, const char* pluginName, const char* unpackMask)
{
    if (CFilesActionDiskTarget::FindLive(MainWindow, this) == NULL ||
        CFilesActionDiskTarget::FindLive(MainWindow, target) == NULL) return;
    if (target == NULL || !Is(ptDisk) ||
        ((IsBranchView() || target->IsBranchView()) && !target->Is(ptDisk))) return; // Branch operations require disk source and target.

    CALL_STACK_MESSAGE4("CFilesWindow::Unpack(, %d, %s, %s)", pluginIndex, pluginName, unpackMask);

    if (FilesActionInProgress || pluginIndex < -1) return;
    CFilesActionDiskTarget sourceSnapshot, diskTarget;
    const bool watchDiskTarget = target->Is(ptDisk);
    if (!sourceSnapshot.Capture(MainWindow, this, TRUE) ||
        (watchDiskTarget && !diskTarget.Capture(MainWindow, target, TRUE))) return;
    const bool branchOperation = sourceSnapshot.Branch || (watchDiskTarget && diskTarget.Branch);
    const HWND parent = MainWindow->HWindow;
    const auto panelsAreCurrent = [&]() {
        return sourceSnapshot.IsCurrent(MainWindow) && (!watchDiskTarget || diskTarget.IsCurrent(MainWindow));
    };
    const auto reportChangedContext = [&]() {
        SalMessageBox(parent, GetErrorText(ERROR_RETRY), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
    };
    const auto notifyPath = [&](const char* path, BOOL recursive) {
        if (MainWindow != NULL) MainWindow->PostChangeOnPathNotification(path, recursive);
    };

    // restore DefaultDir
    MainWindow->UpdateDefaultDir(MainWindow->GetActivePanel() == this);

    if (!panelsAreCurrent()) { reportChangedContext(); return; }

    int i = GetCaretIndex();
    if (i >= Dirs->Count && i < Dirs->Count + Files->Count)
    {
        BeginStopRefresh();
        struct CUnpackRefreshScope
        {
            ~CUnpackRefreshScope() { EndStopRefresh(); }
        } refreshScope;

        CFileData* file = &Files->At(i - Dirs->Count);
        std::string archiveSource, sourceDirectory;
        int sourceErrorTextID;
        if (!PrepareUnpackArchiveSource(this, *file, archiveSource, sourceErrorTextID, &sourceDirectory))
        {
            SalMessageBox(parent, LoadStr(sourceErrorTextID), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
            return;
        }
        CPathBuffer pathBuffer, alternateBuffer, fileNameBuffer, subjectBuffer(SAL_MAX_PATH + 100);
        char* path = pathBuffer.Data();
        char* pathAlt = alternateBuffer.Data();
        char* fileName = fileNameBuffer.Data();
        char* subject = subjectBuffer.Data();
        char mask[MAX_PATH]; // file mask, not a filesystem path
        path[0] = 0;
        pathAlt[0] = 0;
        if (target->Is(ptDisk))
        {
            const std::wstring targetW = target->GetPathW();
            const std::string targetText = SalWideToMultiBytePath(targetW.c_str(), CP_UTF8);
            if (targetW.empty() || targetText.size() >= (size_t)pathBuffer.Capacity() ||
                SalMultiByteToWidePath(targetText.c_str(), CP_UTF8) != targetW)
            {
                SalMessageBox(parent, LoadStr(IDS_PATHISINVALID), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                return;
            }
            if (Configuration.UseAnotherPanelForUnpack)
            {
                target->UserWorkedOnThisPath = TRUE; // default action = work with the path in the target panel
                strcpy(path, targetText.c_str());
            }
            else
                strcpy(pathAlt, targetText.c_str());
        }
        std::wstring fileNameW;
        if (file->UseWideName())
            fileNameW = file->NameW; // preserve the exact wide basename rather than interpreting UTF8 bytes through ACP case tables
        else
        {
            AlterFileName(fileName, file->Name, -1, Configuration.FileNameFormat, 0, FALSE);
            fileNameW = SalMultiByteToWidePath(fileName, IsValidPathUtf8Text(fileName) ? CP_UTF8 : CP_ACP);
        }
        const std::string displayName = SalWideToMultiBytePath(fileNameW.c_str(), CP_UTF8);
        if (displayName.empty() || displayName.size() >= (size_t)fileNameBuffer.Capacity() ||
            SalMultiByteToWidePath(displayName.c_str(), CP_UTF8) != fileNameW)
        {
            SalMessageBox(parent, LoadStr(IDS_PATHISINVALID), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
            return;
        }
        strcpy(fileName, displayName.c_str());
        if (Configuration.UseSubdirNameByArchiveForUnpack)
        {
            std::string stem = displayName;
            const size_t dot = stem.find_last_of('.');
            if (dot != std::string::npos && dot + 1 < stem.size()) stem.resize(dot);
            for (int index = 0; index < 2; ++index)
            {
                char* buffer = index == 0 ? path : pathAlt;
                if (!stem.empty() && !SalPathAppend(buffer, stem.c_str(), pathBuffer.Capacity()))
                {
                    SalMessageBox(parent, LoadStr(IDS_TOOLONGPATH), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                    return;
                }
            }
        }
        if (unpackMask != NULL)
            lstrcpyn(mask, unpackMask, MAX_PATH);
        else
            strcpy(mask, "*.*");
        CTruncatedString str;
        const char* prompt = LoadStr(IDS_UNPACKARCHIVE);
        const std::wstring promptW = SalMultiByteToWidePath(prompt, IsValidPathUtf8Text(prompt) ? CP_UTF8 : CP_ACP);
        str.SetW(promptW.c_str(), fileNameW.c_str());

        if (pluginIndex != -1)
        {
            int i2;
            for (i2 = 0; i2 < UnpackerConfig.GetUnpackersCount(); i2++)
            {
                if (UnpackerConfig.GetUnpackerType(i2) == -pluginIndex - 1)
                {
                    UnpackerConfig.SetPreferedUnpacker(i2);
                    break;
                }
            }
            if (i2 == UnpackerConfig.GetUnpackersCount()) // requested plugin not found
            {
                const bool haveMessage = Salamander::LocalizedText::Write(subject, static_cast<size_t>(subjectBuffer.Capacity()), LoadStr(IDS_PLUGINUNPACKERNOTFOUND), pluginName);
                SalMessageBox(parent, haveMessage ? subject : GetErrorText(ERROR_INVALID_DATA), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                return;
            }
        }
        else // choose the unpacker based on the extension
        {
            CMaskGroup tmpmask;
            if (UnpackerConfig.GetPreferedUnpacker() == -1)
            { // if none is preferred, pick the first one (so users do not stare at an empty combo box)
                UnpackerConfig.SetPreferedUnpacker(0);
            }
            if (UnpackerConfig.GetPreferedUnpacker() != -1)
            {
                tmpmask.SetMasksString(UnpackerConfig.GetUnpackerExt(UnpackerConfig.GetPreferedUnpacker()), TRUE);
            }
            else
            {
                tmpmask.SetMasksString("", TRUE);
            }
            int errpos = 0;
            tmpmask.PrepareMasks(errpos);
            if (!(file->UseWideName() ? tmpmask.AgreeMasksW(file->NameW, NULL) : tmpmask.AgreeMasks(file->Name, file->Ext)))
            {
                int i2;
                for (i2 = 0; i2 < UnpackerConfig.GetUnpackersCount(); i2++)
                {
                    tmpmask.SetMasksString(UnpackerConfig.GetUnpackerExt(i2), TRUE);
                    tmpmask.PrepareMasks(errpos);
                    if ((file->UseWideName() ? tmpmask.AgreeMasksW(file->NameW, NULL) : tmpmask.AgreeMasks(file->Name, file->Ext)))
                    {
                        UnpackerConfig.SetPreferedUnpacker(i2);
                        break;
                    }
                }
            }
        }
        const std::string offeredDiskTarget = watchDiskTarget ?
            (Configuration.UseAnotherPanelForUnpack ? path : pathAlt) : "";
        BOOL delArchiveWhenDone = FALSE;
        while (true)
        {
            if (!panelsAreCurrent()) { reportChangedContext(); return; }
            CUnpackDialog dialog(parent, path, pathBuffer.Capacity(), pathAlt, mask, &str, &UnpackerConfig, &delArchiveWhenDone, TRUE);
            if (dialog.Execute() != IDOK) break;

            CWholeArchiveUnpackerSnapshot backendSnapshot;
            if (!panelsAreCurrent() || !dialog.IsSelectionCurrent() || !backendSnapshot.Capture(&UnpackerConfig, TRUE))
            {
                reportChangedContext();
                return;
            }
            if (branchOperation && !backendSnapshot.IsVerifiedBranchSourceUnpacker())
            {
                SalMessageBox(parent, GetErrorText(ERROR_NOT_SUPPORTED), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                return;
            }
            bool branchTargetBound = false;
            const auto contextIsCurrent = [&]() {
                return panelsAreCurrent() && dialog.IsSelectionCurrent() && backendSnapshot.IsCurrent() &&
                       (!branchOperation || backendSnapshot.IsVerifiedBranchSourceUnpacker()) &&
                       (!branchTargetBound || backendSnapshot.ValidateBranchTarget(&diskTarget, path));
            };
            UpdateWindow(parent);
            if (!contextIsCurrent()) { reportChangedContext(); return; }

            // Only the offered panel destination is an implicit root; manual paths retain normal resolution.
            const bool useOfferedDiskTarget = watchDiskTarget && !offeredDiskTarget.empty() &&
                strcmp(path, offeredDiskTarget.c_str()) == 0;
            const auto offeredRootExists = [&]() {
                if (!useOfferedDiskTarget) return true;
                const std::wstring root = diskTarget.Root.length() >= MAX_PATH ?
                    SalPathAddExtendedPrefixW(diskTarget.Root.c_str()) : diskTarget.Root;
                const DWORD attrs = GetFileAttributesW(root.c_str());
                return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
            };
            int errTextID;
            CPathBuffer nextFocusBuffer;
            char* nextFocus = nextFocusBuffer.Data();
            nextFocus[0] = 0;
            const char* errorText = NULL;
            if (!SalGetFullName(path, &errTextID, sourceDirectory.c_str(), nextFocus, NULL, pathBuffer.Capacity()))
            {
                if (errTextID == IDS_EMPTYNAMENOTALLOWED)
                    strcpy(path, sourceDirectory.c_str());
                else
                    errorText = LoadStr(errTextID);
            }
            if (errorText != NULL)
            {
                SalMessageBox(parent, errorText, LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                continue;
            }
            if (watchDiskTarget && diskTarget.Branch)
            {
                if (!useOfferedDiskTarget || strcmp(path, offeredDiskTarget.c_str()) != 0)
                {
                    SalMessageBox(parent, GetErrorText(ERROR_NOT_SUPPORTED), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                    return;
                }
                if (!backendSnapshot.BindBranchTarget(&diskTarget, path)) { reportChangedContext(); return; }
                branchTargetBound = true;
            }
            if (!contextIsCurrent() || !offeredRootExists()) { reportChangedContext(); return; }
            if (!backendSnapshot.ValidateExternalPaths(archiveSource.c_str(), path))
            {
                const DWORD error = GetLastError();
                SalMessageBox(parent, GetErrorText(error), LoadStr(IDS_ERRORUNPACK), MB_OK | MB_ICONEXCLAMATION);
                continue;
            }
            const std::wstring pathW = SalMultiByteToWidePath(path, IsValidPathUtf8Text(path) ? CP_UTF8 : CP_ACP);
            std::wstring newDirW;
            const BOOL createdOK = !pathW.empty() && CheckAndCreateDirectoryW(pathW.c_str(), parent, TRUE, &newDirW, TRUE);
            std::string newDir = SalWideToMultiBytePath(newDirW.c_str(), CP_UTF8);
            if (!contextIsCurrent() || !offeredRootExists())
            {
                if (!newDir.empty()) RemoveEmptyUnpackDirectoriesW(newDirW);
                reportChangedContext();
                return;
            }
            if (!createdOK)
            {
                if (!newDir.empty())
                {
                    CutDirectory(newDir.data());
                    notifyPath(newDir.c_str(), TRUE);
                }
                continue;
            }

            CDynamicStringImp archiveVolumes;
            BOOL unpacked;
            DWORD unpackError;
            {
                struct CUnpackPriorityScope
                {
                    CUnpackPriorityScope() { SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL); }
                    ~CUnpackPriorityScope() { SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL); }
                } priorityScope;
                SetLastError(ERROR_SUCCESS);
                unpacked = UnpackerConfig.ExecuteUnpacker(MainWindow->HWindow, this, archiveSource.c_str(), mask,
                    path, delArchiveWhenDone, delArchiveWhenDone ? &archiveVolumes : NULL,
                    &backendSnapshot, &sourceSnapshot, watchDiskTarget ? &diskTarget : NULL);
                unpackError = GetLastError();
            }
            if (!unpacked)
            {
                if (!newDir.empty()) RemoveEmptyUnpackDirectoriesW(newDirW);
            }
            else if (contextIsCurrent())
            {
                if (delArchiveWhenDone && archiveVolumes.Length > 0)
                {
                    std::vector<CUnpackArchiveVolume> volumes;
                    if (!PrepareUnpackArchiveVolumes(archiveVolumes.Text, archiveVolumes.Length, archiveVolumes.Allocated,
                                                     archiveSource.c_str(), backendSnapshot.IsVerifiedBranchSourceUnpacker(), volumes))
                    {
                        const DWORD error = GetLastError();
                        nextFocus[0] = 0;
                        SalMessageBox(parent, GetErrorText(error), LoadStr(IDS_ERRORDELETINGFILE), MB_OK | MB_ICONEXCLAMATION);
                    }
                    else
                    {
                        BOOL skipAll = FALSE;
                        bool stopDeleting = false;
                        for (const auto& volume : volumes)
                        {
                            while (true)
                            {
                                if (!contextIsCurrent()) { stopDeleting = true; nextFocus[0] = 0; break; }
                                if (!DeleteUnpackArchiveVolumeW(volume.Path.c_str()) && !skipAll)
                                {
                                    const DWORD error = GetLastError();
                                    if (error == ERROR_FILE_NOT_FOUND) break;
                                    const int response = (int)CFileErrorDlg(parent, LoadStr(IDS_ERRORDELETINGFILE), volume.Display.c_str(), GetErrorText(error)).Execute();
                                    if (!contextIsCurrent()) { stopDeleting = true; nextFocus[0] = 0; break; }
                                    if (response == IDB_SKIPALL) skipAll = TRUE;
                                    if (response == IDB_SKIPALL || response == IDB_SKIP) break;
                                    if (response == IDCANCEL) { stopDeleting = true; nextFocus[0] = 0; break; }
                                }
                                else break;
                            }
                            if (stopDeleting) break;
                        }
                    }
                }
                // A volume error dialog may have dispatched callbacks since the success check above.
                if (contextIsCurrent() && !sourceSnapshot.Branch)
                {
                    NextFocusName[0] = 0;
                    if (nextFocus[0] != 0 && strlen(nextFocus) < sizeof(NextFocusName))
                    {
                        strcpy(NextFocusName, nextFocus);
                        PostMessage(HWindow, WM_USER_DONEXTFOCUS, 0, 0);
                    }
                }
            }
            const bool changedContext = !contextIsCurrent();
            // No source members/getters after callbacks: the original directory is owned by the snapshot.
            notifyPath(sourceSnapshot.DefaultPath.c_str(), FALSE);
            if (!newDir.empty())
            {
                CutDirectory(newDir.data());
                notifyPath(newDir.c_str(), TRUE);
            }
            else notifyPath(path, TRUE);
            if (changedContext || (!unpacked && unpackError == ERROR_RETRY)) reportChangedContext();
            break;
        }
        UpdateWindow(parent);
    }
}

// countSizeMode - 0 normal calculation, 1 for the selected item, 2 for all subdirectories
void CFilesWindow::CalculateOccupiedZIPSpace(int countSizeMode)
{
    CALL_STACK_MESSAGE2("CFilesWindow::CalculateOccupiedZIPSpace(%d)", countSizeMode);
    if (Is(ptZIPArchive) && (ValidFileData & VALID_DATA_SIZE)) // only if CFileData::Size is valid (sizes defined via plugin data are unlikely for archives, so we ignore them here for now)
    {
        BeginStopRefresh(); // the snooper will wait

        TDirectArray<CQuadWord> sizes(200, 400);
        CQuadWord totalSize(0, 0); // calculated size
        int files = 0;
        int dirs = 0;

        int selIndex = -1;
        BOOL upDir;
        if (Dirs->Count > 0)
            upDir = (strcmp(Dirs->At(0).Name, "..") == 0);
        else
            upDir = FALSE;
        int count = GetSelCount();
        if (countSizeMode == 0 && count != 0 || countSizeMode == 2) // valid selection
        {
            if (countSizeMode == 2)
                count = Dirs->Count - (upDir ? 1 : 0);
            int* indexes = new int[count];
            if (indexes == NULL)
            {
                TRACE_E(LOW_MEMORY);
                EndStopRefresh(); // the snooperresumes now
                return;
            }
            else
            {
                if (countSizeMode == 2)
                {
                    int i = (upDir ? 1 : 0);
                    int j;
                    for (j = 0; j < count; j++)
                        indexes[j] = i++;
                }
                else
                    GetSelItems(count, indexes);
                int i = count;
                while (i--)
                {
                    CFileData* f = (indexes[i] < Dirs->Count) ? &Dirs->At(indexes[i]) : &Files->At(indexes[i] - Dirs->Count);
                    if (indexes[i] < Dirs->Count)
                    {
                        f->SizeValid = 1;
                        f->Size = GetArchiveDir()->GetDirSize(GetZIPPath(), f->Name, &dirs, &files, &sizes);
                        totalSize += f->Size;
                        dirs++;
                    }
                    else
                    {
                        sizes.Add(f->Size); // adding errors are handled only in the output dialog
                        totalSize += f->Size;
                        files++;
                    }
                }
            }
            delete[] (indexes);
        }
        else // take the selected file or directory
        {
            selIndex = GetCaretIndex();
            if (upDir && selIndex == 0)
            {
                EndStopRefresh(); // the snooper resumes now
                return;           // nothing to do
            }
            else
            {
                if (countSizeMode == 0)
                {
                    SetSel(TRUE, selIndex);
                    PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
                }
                CFileData* f = (selIndex < Dirs->Count) ? &Dirs->At(selIndex) : &Files->At(selIndex - Dirs->Count);
                if (selIndex < Dirs->Count)
                {
                    f->SizeValid = 1;
                    f->Size = GetArchiveDir()->GetDirSize(GetZIPPath(), f->Name, &dirs, &files, &sizes);
                    totalSize += f->Size;
                    dirs++;
                }
                else
                {
                    sizes.Add(f->Size); // adding errors are handled only in the output dialog
                    totalSize += f->Size;
                    files++;
                }
            }
        }

        // sort if we counted over more than one selected directory or over all of them
        if (((countSizeMode == 0 && dirs > 1) || countSizeMode == 2) && SortType == stSize)
        {
            ChangeSortType(stSize, FALSE, TRUE);
        }
        RefreshListBox(-1, -1, FocusedIndex, FALSE, FALSE); // recalculate column widths
        if (countSizeMode == 0)
        {
            CSizeResultsDlg(HWindow, totalSize, CQuadWord(-1, -1), CQuadWord(-1, -1),
                            files, dirs, &sizes)
                .Execute();
            //      CZIPSizeResultsDlg(HWindow, totalSize, files, dirs).Execute();
        }
        if (selIndex != -1 && countSizeMode == 0)
        {
            SetSel(FALSE, selIndex);
            PostMessage(HWindow, WM_USER_SELCHANGED, 0, 0);
        }
        RepaintListBox(DRAWFLAG_DIRTY_ONLY | DRAWFLAG_SKIP_VISTEST);
        UpdateWindow(MainWindow->HWindow);

        EndStopRefresh(); // the snooper resumes now
    }
}

void CFilesWindow::AcceptChangeOnPathNotification(const char* path, BOOL includingSubdirs)
{
    CALL_STACK_MESSAGE3("CFilesWindow::AcceptChangeOnPathNotification(%s, %d)",
                        path, includingSubdirs);

    if (IsBranchView())
    {
        std::wstring changed = SalMultiByteToWidePath(path, CP_UTF8);
        if (changed.empty())
            changed = SalMultiByteToWidePath(path, CP_ACP);
        std::wstring root = GetPathW();
        while (!root.empty() && root.back() == L'\\') root.pop_back();
        while (!changed.empty() && changed.back() == L'\\') changed.pop_back();
        const BOOL descendant = changed.size() >= root.size() &&
            CompareStringOrdinal(changed.c_str(), (int)root.size(), root.c_str(), (int)root.size(), TRUE) == CSTR_EQUAL &&
            (changed.size() == root.size() || changed[root.size()] == L'\\');
        const BOOL ancestor = includingSubdirs && root.size() >= changed.size() &&
            CompareStringOrdinal(root.c_str(), (int)changed.size(), changed.c_str(), (int)changed.size(), TRUE) == CSTR_EQUAL &&
            (root.size() == changed.size() || root[changed.size()] == L'\\');
        if (!changed.empty() && (descendant || ancestor))
        {
            HANDLES(EnterCriticalSection(&TimeCounterSection));
            const int stamp = MyTimeCounter++;
            HANDLES(LeaveCriticalSection(&TimeCounterSection));
            PostMessage(HWindow, WM_USER_REFRESH_DIR, TRUE, stamp);
        }
        RefreshDiskFreeSpace(TRUE, TRUE);
        return;
    }
    BOOL refresh = FALSE;
    if ((Is(ptDisk) || Is(ptZIPArchive)) && (!AutomaticRefresh || GetNetworkDrive()))
    {
        // test the equality of paths or at least their prefix (we only care about disk paths,
        // FS paths in 'path' are automatically excluded because they can never match GetPath())
        char path1[MAX_PATH];
        char path2[MAX_PATH];
        lstrcpyn(path1, path, MAX_PATH);
        lstrcpyn(path2, GetPath(), MAX_PATH); // for archives this is the path to the archive
        SalPathRemoveBackslash(path1);
        SalPathRemoveBackslash(path2);
        int len1 = (int)strlen(path1);
        refresh = !includingSubdirs && StrICmp(path1, path2) == 0 ||       // exact match
                  includingSubdirs && StrNICmp(path1, path2, len1) == 0 && // prefix match
                      (path2[len1] == 0 || path2[len1] == '\\');
        if (Is(ptDisk) && !refresh && CutDirectory(path1)) // pointless for archives
        {
            SalPathRemoveBackslash(path1);
            // on NTFS the last subdirectory timestamp also changes (unfortunately visible only after entering
            // that subdirectory, but perhaps it will be fixed eventually, so refresh proactively)
            refresh = StrICmp(path1, path2) == 0;
        }
        if (refresh)
        {
            HANDLES(EnterCriticalSection(&TimeCounterSection));
            int t1 = MyTimeCounter++;
            HANDLES(LeaveCriticalSection(&TimeCounterSection));
            PostMessage(HWindow, WM_USER_REFRESH_DIR, 0, t1);
        }
    }
    else
    {
        if (Is(ptPluginFS) && GetPluginFS()->NotEmpty()) // send notification to the FS
        {
            // the EnterPlugin+LeavePlugin section must be exposed up to here (not wrapped inside the interface)
            EnterPlugin();
            GetPluginFS()->AcceptChangeOnPathNotification(GetPluginFS()->GetPluginFSName(), path, includingSubdirs);
            LeavePlugin();
        }
    }

    if (Is(ptDisk) && !refresh &&           // only disks have free space (archives do not and FS is handled elsewhere)
        HasTheSameRootPath(path, GetPath()) // same root -> possible change in free disk space size

        /* && (!AutomaticRefresh ||  // commented out because notifications do not arrive for subdirectory changes on auto-refreshed paths causing the free space info to remain invalid
       !IsTheSamePath(path, GetPath()))*/
        ) // this path is not monitored for changes -> refresh will definitely not arrive
    {
        RefreshDiskFreeSpace(TRUE, TRUE);
    }
}

void CFilesWindow::IconOverlaysChangedOnPath(const char* path)
{
    //  if ((int)(GetTickCount() - NextIconOvrRefreshTime) < 0)
    //    TRACE_I("CFilesWindow::IconOverlaysChangedOnPath: skipping notification for: " << path);
    if ((int)(GetTickCount() - NextIconOvrRefreshTime) >= 0 &&             // refresh of icon overlays occurs at NextIconOvrRefreshTime; before that it makes no sense to track changes
        !IconOvrRefreshTimerSet && !NeedIconOvrRefreshAfterIconsReading && // icon overlay refresh not scheduled yet
        Configuration.EnableCustomIconOverlays && Is(ptDisk) &&
        (UseSystemIcons || UseThumbnails) && IconCache != NULL &&
        IsTheSamePath(path, GetPath()))
    {
        DWORD elapsed = GetTickCount() - LastIconOvrRefreshTime;
        if (elapsed < ICONOVR_REFRESH_PERIOD) // wait before the next icon overlay refresh so we do not refresh too often
        {
            // TRACE_I("CFilesWindow::IconOverlaysChangedOnPath: setting timer for refresh");
            if (SetTimer(HWindow, IDT_ICONOVRREFRESH, max(200, ICONOVR_REFRESH_PERIOD - elapsed), NULL))
            {
                IconOvrRefreshTimerSet = TRUE;
                return;
            }
            // if the timer fails, attempt an immediate refresh...
        }
        // try to refresh immediately (as long as it didn't come too soon after the previous one)
        if (!IconCacheValid) // perform after icons finish loading (they may or may not load correctly)
        {
            // TRACE_I("CFilesWindow::IconOverlaysChangedOnPath: delaying refresh till end of reading of icons");
            NeedIconOvrRefreshAfterIconsReading = TRUE;
        }
        else // refresh immediately
        {
            // TRACE_I("CFilesWindow::IconOverlaysChangedOnPath: doing refresh: sleeping icon reader");
            SleepIconCacheThread();
            WaitOneTimeBeforeReadingIcons = 200; // during this time the icon reader waits before starting overlay loading; subsequent notifications from Tortoise SVN within 200 ms can be ignored
            LastIconOvrRefreshTime = GetTickCount();
            NextIconOvrRefreshTime = LastIconOvrRefreshTime + WaitOneTimeBeforeReadingIcons;
            WakeupIconCacheThread();
            // TRACE_I("CFilesWindow::IconOverlaysChangedOnPath: doing refresh: icon reader is awake again");
        }
    }
}
