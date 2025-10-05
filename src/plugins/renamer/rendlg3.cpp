﻿// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

BOOL CRenamerDialog::MoveFile(char* sourceName, char* targetName, char* newPart,
                              BOOL overwrite, BOOL isDir, BOOL& skip)
{
    CALL_STACK_MESSAGE4("CRenamerDialog::MoveFile(, , , %d, %d, %d)", overwrite,
                        isDir, skip);
    // create the path
    char dir[MAX_PATH];
    strcpy(dir, targetName);
    SG->CutDirectory(dir);
    if (!CheckAndCreateDirectory(dir, dir + (newPart - targetName), skip))
        return FALSE;

    // perform the move
    if (SG->HasTheSameRootPath(sourceName, targetName))
    {
        while (1)
        {
            DWORD err = 0;
            if (SG->StrICmp(sourceName, targetName) == 0 &&
                    strcmp(
                        SG->SalPathFindFileName(sourceName),
                        SG->SalPathFindFileName(targetName)) == 0 ||
                SG->SalMoveFile(sourceName, targetName, &err))
                return TRUE; // success

            if ((err == ERROR_ALREADY_EXISTS || err == ERROR_FILE_EXISTS) &&
                SG->StrICmp(sourceName, targetName) != 0)
            {
                DWORD attr = SG->SalGetFileAttributes(targetName);
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

                SG->ClearReadOnlyAttr(targetName); // so it can be deleted ...
                while (1)
                {
                    if (DeleteFile(targetName))
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
        SG->ClearReadOnlyAttr(sourceName); // so it can be deleted ...
        while (1)
        {
            if (DeleteFile(sourceName))
                break;

            if (!FileError(HWindow, sourceName, IDS_DELETEERROR,
                           TRUE, &skip, &SkipAllDeleteErr, IDS_ERROR))
                return skip;
        }
        return TRUE;
    }
}

BOOL CRenamerDialog::CheckAndCreateDirectory(char* directory, char* newPart, BOOL& skip)
{
    CALL_STACK_MESSAGE2("CRenamerDialog::CheckAndCreateDirectory(, , %d)", skip);
    WIN32_FIND_DATA fd;
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
        f = FindFirstFile(directory, &fd);
        if (f == INVALID_HANDLE_VALUE)
            goto CREATE_PATH;
        else
        {
            FindClose(f);
            // adjust the case of the name
            if (strcmp(start, fd.cFileName))
            {
                char old[MAX_PATH];
                memcpy(old, directory, start - directory);
                strcpy(old + (start - directory), fd.cFileName);
                while (1)
                {
                    if (SG->SalMoveFile(old, directory, NULL))
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
            if (CreateDirectory(directory, NULL))
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
        in = CreateFile(sourceName, GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        if (in != INVALID_HANDLE_VALUE)
        {
            HANDLE out;
            while (1)
            {
                out = CreateFile(targetName, GENERIC_WRITE, 0, NULL,
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
                                        DeleteFile(targetName);
                                        return FALSE;
                                    }

                                    // retry
                                    if (out != NULL)
                                        CloseHandle(out); // close the invalid handle
                                    out = CreateFile(targetName, GENERIC_WRITE, 0, NULL,
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
                                            DeleteFile(targetName);
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
                                                DeleteFile(targetName);
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
                                    DeleteFile(targetName);
                                    return FALSE;
                                }

                                if (in != NULL)
                                    CloseHandle(in); // close the invalid handle
                                in = CreateFile(sourceName, GENERIC_READ,
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
                                        DeleteFile(targetName);
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
                                            DeleteFile(targetName);
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
                    attr = SG->SalGetFileAttributes(sourceName);
                    if (attr != -1)
                        SetFileAttributes(targetName, attr | FILE_ATTRIBUTE_ARCHIVE);
                    return TRUE;
                }
                else
                {
                    DWORD err = GetLastError();
                    DWORD attr = SG->SalGetFileAttributes(targetName);
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
                            SetFileAttributes(targetName, attr & (~FILE_ATTRIBUTE_READONLY));
                        }

                        out = CreateFile(targetName, GENERIC_WRITE, 0, NULL,
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
                                SetFileAttributes(targetName, attr);
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
