// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <stdlib.h>
#include <string.h>

// Quiet magic-byte helpers for CPluginInterfaceForArchiverAbstract::CanOpenArchive.
// No UI. Uses the same char* path the archiver SDK already passes to ListArchive.

inline HANDLE ArchiveProbeOpenRead(const char* fileName)
{
    if (fileName == NULL || fileName[0] == 0)
        return INVALID_HANDLE_VALUE;

    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fileName, -1, NULL, 0);
    if (n <= 0)
        n = MultiByteToWideChar(CP_ACP, 0, fileName, -1, NULL, 0);
    if (n <= 0)
        return INVALID_HANDLE_VALUE;

    wchar_t* wide = (wchar_t*)malloc((size_t)n * sizeof(wchar_t));
    if (wide == NULL)
        return INVALID_HANDLE_VALUE;
    int converted = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fileName, -1, wide, n);
    if (converted <= 0)
        converted = MultiByteToWideChar(CP_ACP, 0, fileName, -1, wide, n);
    HANDLE file = INVALID_HANDLE_VALUE;
    if (converted > 0)
    {
        const wchar_t* pathToOpen = wide;
        wchar_t* prefixed = NULL;
        size_t wideLen = wcslen(wide);
        if (wideLen >= MAX_PATH && wcsncmp(wide, L"\\\\?\\", 4) != 0)
        {
            if (wcsncmp(wide, L"\\\\", 2) == 0)
            {
                prefixed = (wchar_t*)malloc((wideLen + 8 + 1) * sizeof(wchar_t));
                if (prefixed != NULL)
                {
                    wcscpy(prefixed, L"\\\\?\\UNC\\");
                    wcscat(prefixed, wide + 2);
                    pathToOpen = prefixed;
                }
            }
            else
            {
                prefixed = (wchar_t*)malloc((wideLen + 4 + 1) * sizeof(wchar_t));
                if (prefixed != NULL)
                {
                    wcscpy(prefixed, L"\\\\?\\");
                    wcscat(prefixed, wide);
                    pathToOpen = prefixed;
                }
            }
        }
        file = CreateFileW(pathToOpen, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        free(prefixed);
    }
    free(wide);
    return file;
}

inline BOOL ArchiveProbeFindBytes(const unsigned char* data, DWORD dataLen,
                                  const unsigned char* magic, int magicLen)
{
    if (data == NULL || magic == NULL || magicLen <= 0 || dataLen < (DWORD)magicLen)
        return FALSE;
    const DWORD last = dataLen - (DWORD)magicLen;
    for (DWORD i = 0; i <= last; i++)
    {
        if (memcmp(data + i, magic, (size_t)magicLen) == 0)
            return TRUE;
    }
    return FALSE;
}

inline BOOL ArchiveProbeMatchAt(HANDLE file, ULONGLONG offset,
                                const unsigned char* magic, int magicLen)
{
    if (file == INVALID_HANDLE_VALUE || magic == NULL || magicLen <= 0)
        return FALSE;
    LARGE_INTEGER pos;
    pos.QuadPart = (LONGLONG)offset;
    if (!SetFilePointerEx(file, pos, NULL, FILE_BEGIN))
        return FALSE;
    unsigned char buf[64];
    if (magicLen > (int)sizeof(buf))
        return FALSE;
    DWORD read = 0;
    if (!ReadFile(file, buf, (DWORD)magicLen, &read, NULL) || read != (DWORD)magicLen)
        return FALSE;
    return memcmp(buf, magic, (size_t)magicLen) == 0;
}

inline BOOL ArchiveProbeScan(const char* fileName, const unsigned char* magic, int magicLen,
                             DWORD maxFromStart, DWORD maxFromEnd)
{
    HANDLE file = ArchiveProbeOpenRead(fileName);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    LARGE_INTEGER size = {};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0)
    {
        CloseHandle(file);
        return FALSE;
    }

    BOOL found = FALSE;
    if (maxFromStart > 0)
    {
        DWORD toRead = maxFromStart;
        if ((ULONGLONG)size.QuadPart < toRead)
            toRead = (DWORD)size.QuadPart;
        unsigned char* buf = (unsigned char*)malloc(toRead);
        if (buf != NULL)
        {
            LARGE_INTEGER zero = {};
            DWORD read = 0;
            if (SetFilePointerEx(file, zero, NULL, FILE_BEGIN) &&
                ReadFile(file, buf, toRead, &read, NULL))
            {
                found = ArchiveProbeFindBytes(buf, read, magic, magicLen);
            }
            free(buf);
        }
    }

    if (!found && maxFromEnd > 0)
    {
        DWORD toRead = maxFromEnd;
        if ((ULONGLONG)size.QuadPart < toRead)
            toRead = (DWORD)size.QuadPart;
        unsigned char* buf = (unsigned char*)malloc(toRead);
        if (buf != NULL)
        {
            LARGE_INTEGER pos;
            pos.QuadPart = size.QuadPart - toRead;
            DWORD read = 0;
            if (SetFilePointerEx(file, pos, NULL, FILE_BEGIN) &&
                ReadFile(file, buf, toRead, &read, NULL))
            {
                found = ArchiveProbeFindBytes(buf, read, magic, magicLen);
            }
            free(buf);
        }
    }

    CloseHandle(file);
    return found;
}
