// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include <string>

namespace Salamander { namespace ViewerPaths {

inline std::wstring Decode(const char* path)
{
    if (path == NULL || *path == 0) return std::wstring();
    UINT codePage = CP_UTF8;
    int length = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
    if (length == 0)
    {
        codePage = CP_ACP;
        length = MultiByteToWideChar(codePage, 0, path, -1, NULL, 0);
    }
    if (length == 0) return std::wstring();
    std::wstring result(length, L'\0');
    MultiByteToWideChar(codePage, codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
                        path, -1, &result[0], length);
    result.resize(length - 1);
    return result;
}

inline std::wstring FullPath(const wchar_t* path)
{
    if (path == NULL || *path == 0) return std::wstring();
    DWORD length = GetFullPathNameW(path, 0, NULL, NULL);
    if (length == 0) return std::wstring();
    std::wstring result(length, L'\0');
    DWORD written = GetFullPathNameW(path, length, &result[0], NULL);
    if (written == 0 || written >= length) return std::wstring();
    result.resize(written);
    return result;
}

// No normalization/folding: two directory entries are separate identities even
// on case-sensitive disks or when their Unicode names are canonically equivalent.
template<class GetPath>
int FindIndex(int index, int count, const std::wstring& lastName, GetPath getPath)
{
    if (lastName.empty()) return -1;
    if (index >= 0 && index < count && getPath(index) == lastName) return index;
    for (int i = 0; i < count; ++i)
        if (getPath(i) == lastName) return i;
    return -1;
}

inline BOOL CopyResult(const std::wstring& path, wchar_t* buffer, int capacity)
{
    if (buffer == NULL || capacity <= 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    buffer[0] = 0;
    if (path.size() >= static_cast<size_t>(capacity))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    memcpy(buffer, path.c_str(), (path.size() + 1) * sizeof(wchar_t));
    return TRUE;
}

}} // namespace Salamander::ViewerPaths
