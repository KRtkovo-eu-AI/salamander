// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

BOOL CRenamerDialog::MoveFile(char* sourceName, char* targetName, char* newPart,
                              BOOL overwrite, BOOL isDir, BOOL& skip)
try
{
    CALL_STACK_MESSAGE4("CRenamerDialog::MoveFile(, , , %d, %d, %d)", overwrite,
                        isDir, skip);
    // create the path
    std::vector<char> directory(strlen(targetName) + 2, 0);
    memcpy(directory.data(), targetName, strlen(targetName) + 1);
    char* dir = directory.data();
    SG->CutDirectory(dir);
    if (!CheckAndCreateDirectory(dir, dir + (newPart - targetName), skip))
        return FALSE;

    // perform the move
    if (RenamerIO::SameRoot(sourceName, targetName))
    {
        while (1)
        {
            DWORD err = 0;
            if (RenamerPaths::Compare(sourceName, targetName) == 0 &&
                    strcmp(
                        SG->SalPathFindFileName(sourceName),
                        SG->SalPathFindFileName(targetName)) == 0 ||
                RenamerIO::Move(sourceName, targetName, &err))
                return TRUE; // success

            if ((err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) &&
                RenamerPaths::Compare(sourceName, targetName) != 0)
            {
                DWORD attr = RenamerIO::Attributes(targetName);
                if (attr != 0xFFFFFFFF && attr & FILE_ATTRIBUTE_DIRECTORY) // cannot overwrite a directory
                {
                    return FileError(HWindow, targetName,
                                     isDir ? IDS_DIRDIR : IDS_FILEDIR,
                                     FALSE, &skip, &SkipAllFileDir, IDS_ERROR);
                }

                if (!overwrite &&
                    !FileOverwrite(HWindow, targetName, NULL, sourceName, NULL, -1,
                                   IDS_CNFRM_SHOVERWRITE, IDS_OVEWWRITETITLE, &skip, &Silent))
                    return FALSE;

                RenamerIO::ClearReadOnly(targetName); // so it can be deleted ...
                while (1)
                {
                    if (RenamerIO::Delete(targetName))
                        break;

                    if (!FileError(HWindow, targetName, IDS_OVERWRITEERROR,
                                   TRUE, &skip, &SkipAllOverwrite, IDS_ERROR))
                        return FALSE;
                }
            }
            else
            {
                if (!FileError(HWindow, sourceName, IDS_MOVEERROR,
                               TRUE, &skip, &SkipAllMove, IDS_ERROR))
                    return FALSE;
            }
        }
    }
    else
    {
        if (isDir)
        {
            TRACE_E("Error in the script.");
            return skip = FALSE;
        }
        if (!CopyFile(sourceName, targetName, overwrite, skip))
            return FALSE;
        // we still need to clean up the file from the sources
        RenamerIO::ClearReadOnly(sourceName); // so it can be deleted ...
        while (1)
        {
            if (RenamerIO::Delete(sourceName))
                break;

            if (!FileError(HWindow, sourceName, IDS_DELETEERROR,
                           TRUE, &skip, &SkipAllDeleteErr, IDS_ERROR))
                return skip;
        }
        return TRUE;
    }
}

catch (const std::bad_alloc&)
{
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    skip = FALSE;
    Error(IDS_LOWMEM);
    return FALSE;
}

BOOL CRenamerDialog::CheckAndCreateDirectory(char* directory, char* newPart, BOOL& skip)
try
{
    CALL_STACK_MESSAGE2("CRenamerDialog::CheckAndCreateDirectory(, , %d)", skip);
    WIN32_FIND_DATAW fd;
    HANDLE f;
    char* directoryEnd = directory + strlen(directory);

    // skip drives
    char *start = directory, *end;
    if (start[0] == '\\' && start[1] == '\\') // UNC
    {
        start += 2;
        while (*start != 0 && *start != '\\')
            start++;
        if (*start != 0)
            start++; // '\\'
        while (*start != 0 && *start != '\\')
            start++;
        start++;
    }
    else
        start += 3;

    start = max(start, newPart);

    // existing part
    while (start < directoryEnd)
    {
        end = (char*)GetNextPathComponent(start);
        *end = 0;
        f = RenamerIO::FindFirst(directory, &fd);
        if (f == INVALID_HANDLE_VALUE)
            goto CREATE_PATH;
        else
        {
            FindClose(f);
            // adjust the case of the name
            const std::string actualName = RenamerPaths::ToUtf8(fd.cFileName);
            if (strcmp(start, actualName.c_str()))
            {
                const std::string oldPath = std::string(directory, start - directory) + actualName;
                std::vector<char> oldBuffer(oldPath.begin(), oldPath.end());
                oldBuffer.push_back(0);
                char* old = oldBuffer.data();
                while (1)
                {
                    if (RenamerIO::Move(old, directory, NULL))
                    {
                        if (!Undoing)
                            UndoStack.Add(new CUndoStackEntry(directory, old, NULL, FALSE, FALSE));
                        break;
                    }

                    if (!FileError(HWindow, old, IDS_DIRCASEERROR,
                                   TRUE, &skip, &SkipAllDirChangeCase, IDS_ERROR))
                    {
                        if (!skip)
                            return FALSE;
                        break;
                    }
                }
            }
        }
        *end = '\\';
        start = end + 1;
    }
    // non-existing part
    while (start < directoryEnd)
    {
        end = (char*)GetNextPathComponent(start);
        *end = 0;
    CREATE_PATH:

        while (1)
        {
            if (RenamerIO::CreateDir(directory, NULL))
            {
                if (!Undoing)
                    UndoStack.Add(new CUndoStackEntry(directory, NULL, NULL, FALSE, FALSE));
                break;
            }

            if (!FileError(HWindow, directory, IDS_CREATEDIR,
                           TRUE, &skip, &SkipAllCreateDir, IDS_ERROR))
                return FALSE;
        }
        *end = '\\';
        start = end + 1;
    }
    return TRUE;
}

catch (const std::bad_alloc&)
{
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    skip = FALSE;
    Error(IDS_LOWMEM);
    return FALSE;
}

BOOL CRenamerDialog::CopyFile(char* sourceName, char* targetName, BOOL overwrite,
                              BOOL& skip)
{
    CALL_STACK_MESSAGE3("CRenamerDialog::CopyFile(, , %d, %d)", overwrite, skip);
    char buffer[OPERATION_BUFFER];

    CQuadWord operationDone;

COPY_AGAIN:

    operationDone.Set(0, 0);
    HANDLE in;

    while (1)
    {
        in = RenamerIO::Open(sourceName, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (in != INVALID_HANDLE_VALUE)
        {
            HANDLE out;
            while (1)
            {
                out = RenamerIO::Open(targetName, GENERIC_WRITE, 0, NULL,
                                 CREATE_NEW, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
                if (out != INVALID_HANDLE_VALUE)
                {
                COPY:

                    DWORD readed;
                    while (1)
                    {
                        if (ReadFile(in, buffer, OPERATION_BUFFER, &readed, NULL))
                        {
                            DWORD writen;
                            if (readed == 0)
                                break; // EOF

                            while (1)
                            {
                                if (WriteFile(out, buffer, readed, &writen, NULL) &&
                                    readed == writen)
                                    break;

                                while (1)
                                {
                                    if (!FileError(HWindow, targetName, IDS_WRITEERROR,
                                                   TRUE, &skip, &SkipAllBadWrite, IDS_ERROR))
                                    {
                                        if (in != NULL)
                                            CloseHandle(in);
                                        if (out != NULL)
                                            CloseHandle(out);
                                        RenamerIO::Delete(targetName);
                                        return FALSE;
                                    }

                                    // retry
                                    if (out != NULL)
                                        CloseHandle(out); // close the invalid handle
                                    out = RenamerIO::Open(targetName, GENERIC_WRITE, 0, NULL,
                                                     OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
                                    if (out != INVALID_HANDLE_VALUE) // opened, now set the offset
                                    {
                                        CQuadWord size;
                                        DWORD err;
                                        if (!SG->SalGetFileSize(out, size, err) ||
                                            size < operationDone)
                                        { // cannot get the size or the file is too small, start over
                                            CloseHandle(in);
                                            CloseHandle(out);
                                            RenamerIO::Delete(targetName);
                                            goto COPY_AGAIN;
                                        }
                                        else // success (the file is large enough), set the offset
                                        {
                                            LONG lo, hi;
                                            lo = operationDone.LoDWord;
                                            hi = operationDone.HiDWord;
                                            lo = SetFilePointer(out, lo, &hi, FILE_BEGIN);
                                            if (lo == 0xFFFFFFFF && GetLastError() != NO_ERROR ||
                                                lo != (LONG)operationDone.LoDWord ||
                                                hi != (LONG)operationDone.HiDWord)
                                            { // cannot set the offset, start over
                                                CloseHandle(in);
                                                CloseHandle(out);
                                                RenamerIO::Delete(targetName);
                                                goto COPY_AGAIN;
                                            }
                                            break;
                                        }
                                    }
                                    else // cannot open it, the problem persists ...
                                    {
                                        out = NULL;
                                    }
                                }
                            }

                            operationDone.Value += readed;
                        }
                        else
                        {
                            while (1)
                            {
                                if (!FileError(HWindow, targetName, IDS_READERROR,
                                               TRUE, &skip, &SkipAllBadRead, IDS_ERROR))
                                {
                                    if (in != NULL)
                                        CloseHandle(in);
                                    if (out != NULL)
                                        CloseHandle(out);
                                    RenamerIO::Delete(targetName);
                                    return FALSE;
                                }

                                if (in != NULL)
                                    CloseHandle(in); // close the invalid handle
                                in = RenamerIO::Open(sourceName, GENERIC_READ,
                                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                                OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
                                if (in != INVALID_HANDLE_VALUE) // opened, now set the offset
                                {
                                    CQuadWord size;
                                    DWORD err;
                                    if (!SG->SalGetFileSize(out, size, err) ||
                                        size < operationDone)
                                    { // cannot get the size or the file is too small, start over
                                        CloseHandle(in);
                                        CloseHandle(out);
                                        RenamerIO::Delete(targetName);
                                        goto COPY_AGAIN;
                                    }
                                    else // success (the file is large enough), set the offset
                                    {
                                        LONG lo, hi;
                                        lo = operationDone.LoDWord;
                                        hi = operationDone.HiDWord;
                                        lo = SetFilePointer(in, lo, &hi, FILE_BEGIN);
                                        if (lo == 0xFFFFFFFF && GetLastError() != NO_ERROR ||
                                            lo != (LONG)operationDone.LoDWord ||
                                            hi != (LONG)operationDone.HiDWord)
                                        { // cannot set the offset, start over
                                            CloseHandle(in);
                                            CloseHandle(out);
                                            RenamerIO::Delete(targetName);
                                            goto COPY_AGAIN;
                                        }
                                        break;
                                    }
                                }
                                else // cannot open it, the problem persists ...
                                {
                                    in = NULL;
                                }
                            }
                        }
                    }

                    FILETIME creation, lastAccess, lastWrite;
                    GetFileTime(in, &creation, &lastAccess, &lastWrite);
                    SetFileTime(out, &creation, &lastAccess, &lastWrite);

                    CloseHandle(in);
                    CloseHandle(out);

                    DWORD attr;
                    attr = RenamerIO::Attributes(sourceName);
                    if (attr != -1)
                        RenamerIO::SetAttributes(targetName, attr | FILE_ATTRIBUTE_ARCHIVE);
                    return TRUE;
                }
                else
                {
                    DWORD err = GetLastError();
                    DWORD attr = RenamerIO::Attributes(targetName);
                    if (err == ERROR_FILE_EXISTS || err == ERROR_ALREADY_EXISTS)
                    {
                        // overwrite the file?
                        if (!overwrite &&
                            !FileOverwrite(HWindow, targetName, NULL, sourceName, NULL, attr,
                                           IDS_CNFRM_SHOVERWRITE, IDS_OVEWWRITETITLE, &skip, &Silent))
                        {
                            CloseHandle(in);
                            return FALSE;
                        }

                        // so it can be overwritten
                        BOOL readonly = FALSE;
                        if (attr != 0xFFFFFFFF && (attr & FILE_ATTRIBUTE_READONLY))
                        {
                            readonly = TRUE;
                            RenamerIO::SetAttributes(targetName, attr & (~FILE_ATTRIBUTE_READONLY));
                        }

                        out = RenamerIO::Open(targetName, GENERIC_WRITE, 0, NULL,
                                         OPEN_ALWAYS, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

                        if (out != INVALID_HANDLE_VALUE)
                        {
                            // write from the start of the file (this seek was forced by Windows XP)
                            SetFilePointer(out, 0, NULL, FILE_BEGIN);

                            SetEndOfFile(out); // reset the file length to zero
                            goto COPY;
                        }
                        else
                        {
                            err = GetLastError();
                            if (readonly)
                                RenamerIO::SetAttributes(targetName, attr);
                            goto NORMAL_ERROR;
                        }
                    }
                    else // plain error
                    {
                    NORMAL_ERROR:

                        if (!FileError(HWindow, targetName, IDS_OPENFILEERROR,
                                       TRUE, &skip, &SkipAllOpenOut, IDS_ERROR))
                        {
                            CloseHandle(in);
                            return FALSE;
                        }
                    }
                }
            }
        }
        else
        {
            if (!FileError(HWindow, sourceName, IDS_OPENFILEERROR,
                           TRUE, &skip, &SkipAllOpenIn, IDS_ERROR))
                return FALSE;
        }
    }
}
