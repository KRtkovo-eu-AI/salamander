// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "Common/MyString.h"
#include "Common/StringConvert.h"

inline UString GetSalamanderUnicodeString(const char* text)
{
    if (text == NULL || *text == 0)
        return UString();
    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int len = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    if (len <= 0)
    {
        codePage = CP_ACP;
        flags = 0;
        len = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
    }
    if (len <= 0)
        return GetUnicodeString(text);
    std::wstring ret(len - 1, L'\0');
    MultiByteToWideChar(codePage, flags, text, -1, &ret[0], len);
    return UString(ret.c_str());
}

struct CUpdateInfo
{
    bool NewData;
    bool NewProperties;

    bool ExistsInArchive;
    int ArchiveItemIndex;

    bool ExistsOnDisk;
    int FileItemIndex;
    bool IsAnti;
};

// used in updatecallback
struct CFileItem
{
    UINT32 Attributes;
    FILETIME CreationTime;
    FILETIME LastAccessTime;
    FILETIME LastWriteTime;
    UINT64 Size;
    UString Name;
    UString FullPath;
    bool IsDir;

    BOOL CanDelete; // TRUE if Overwrite was chosen when updating the archive, otherwise FALSE

    CFileItem(const char* sourcePath, const char* archiveRoot, const char* name, DWORD attr, UINT64 size, FILETIME lastWrite, bool isDir)
    {
        // if archiveRoot is empty, the name must not start with a backslash '\'
        if (strlen(archiveRoot) > 0)
            Name = GetSalamanderUnicodeString(archiveRoot) + UString(L"\\") + GetSalamanderUnicodeString(name);
        else
            Name = GetSalamanderUnicodeString(name);

        FullPath = GetSalamanderUnicodeString(sourcePath) + UString(L"\\") + GetSalamanderUnicodeString(name);
        Attributes = attr;
        Size = size;
        LastWriteTime = CreationTime = LastAccessTime = lastWrite;
        IsDir = isDir;

        CanDelete = FALSE;
    }
};

// used in extractcallback
struct CArchiveItem
{
    UString NameInArchive; // in the archive (i.e. including the path)
    UString Name;
    DWORD Attr;
    FILETIME LastWrite;
    UINT64 Size;
    bool IsDir;
    UINT32 Idx;

    CArchiveItem(UINT32 idx, UString name, UINT64 size, DWORD attr, FILETIME lastWrite, bool isDir)
    {
        Idx = idx;
        //    NameInArchive = nameInArchive;
        Name = name;
        Size = size;
        Attr = attr;
        LastWrite = lastWrite;
        IsDir = isDir;
    }
};
