// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "renamer_paths.h"

namespace RenamerIO
{
// Panel paths and generated targets are absolute. Always use the extended form
// so long paths and existing trailing-dot/space names address the exact item.
inline std::wstring Path(const char* name)
{
    std::wstring path = RenamerPaths::ToWide(name);
    if (path.empty())
    {
        SetLastError(ERROR_INVALID_NAME);
        return path;
    }
    for (size_t i = 0; i < path.size(); ++i)
        if (path[i] == L'/')
            path[i] = L'\\';
    const bool extended = path.compare(0, 4, L"\\\\?\\") == 0;
    const bool device = path.compare(0, 4, L"\\\\.\\") == 0;
    std::wstring ordinary = path;
    if (extended)
    {
        if (path.size() >= 8 && CompareStringOrdinal(path.c_str(), 8, L"\\\\?\\UNC\\", 8, TRUE) == CSTR_EQUAL)
            ordinary = L"\\\\" + path.substr(8);
        else
            ordinary = path.substr(4);
    }
    // A generated relative/full-path name can navigate with '.' or '..'. Extended
    // APIs do not resolve these components. Normalize only this case: applying
    // Win32 normalization to every source would alias real trailing-dot names.
    size_t component = 0;
    if (ordinary.compare(0, 2, L"\\\\") == 0)
    {
        const size_t serverEnd = ordinary.find(L'\\', 2);
        const size_t shareEnd = serverEnd == std::wstring::npos ? std::wstring::npos : ordinary.find(L'\\', serverEnd + 1);
        component = shareEnd == std::wstring::npos ? ordinary.size() : shareEnd + 1;
    }
    else if (ordinary.size() >= 3 && ordinary[1] == L':' && ordinary[2] == L'\\')
        component = 3;
    bool navigation = false;
    while (!device && component < ordinary.size())
    {
        size_t end = ordinary.find(L'\\', component);
        if (end == std::wstring::npos)
            end = ordinary.size();
        const size_t length = end - component;
        if ((length == 1 && ordinary[component] == L'.') ||
            (length == 2 && ordinary[component] == L'.' && ordinary[component + 1] == L'.'))
        {
            navigation = true;
            break;
        }
        component = end + 1;
    }
    if (navigation)
    {
        const DWORD needed = GetFullPathNameW(ordinary.c_str(), 0, NULL, NULL);
        if (needed == 0)
            return std::wstring();
        std::vector<wchar_t> full((size_t)needed + 1, 0);
        const DWORD length = GetFullPathNameW(ordinary.c_str(), (DWORD)full.size(), full.data(), NULL);
        if (length == 0 || length >= full.size())
        {
            if (length >= full.size())
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return std::wstring();
        }
        path.assign(full.data(), length);
    }
    else if (extended || device)
        return path;
    if (path.size() >= 3 && path[1] == L':' && path[2] == L'\\')
        return L"\\\\?\\" + path;
    if (path.compare(0, 2, L"\\\\") == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);
    return path;
}


// Compare drive or complete UNC server/share roots, never a basename or an
// ACP-folded byte prefix. Prefix spelling does not change the filesystem root.
inline BOOL SameRoot(const char* first, const char* second)
{
    try
    {
        const auto root = [](const std::wstring& path) -> std::wstring {
            if (path.size() >= 8 && CompareStringOrdinal(path.c_str(), 8, L"\\\\?\\UNC\\", 8, TRUE) == CSTR_EQUAL)
            {
                const size_t serverEnd = path.find(L'\\', 8);
                if (serverEnd == std::wstring::npos || serverEnd + 1 == path.size())
                    return std::wstring();
                const size_t shareEnd = path.find(L'\\', serverEnd + 1);
                return path.substr(0, shareEnd);
            }
            if (path.size() >= 7 && path.compare(0, 4, L"\\\\?\\") == 0 && path[5] == L':' && path[6] == L'\\')
                return path.substr(0, 7);
            return std::wstring();
        };
        const std::wstring a = root(Path(first));
        const std::wstring b = root(Path(second));
        return !a.empty() && !b.empty() &&
               CompareStringOrdinal(a.c_str(), (int)a.size(), b.c_str(), (int)b.size(), TRUE) == CSTR_EQUAL;
    }
    catch (const std::bad_alloc&)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
}

template <class Result, class Operation>
inline Result WithPath(const char* name, Result failure, Operation operation)
{
    try
    {
        const std::wstring path = Path(name);
        return path.empty() ? failure : operation(path.c_str());
    }
    catch (const std::bad_alloc&)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return failure;
    }
}

inline DWORD Attributes(const char* name)
{
    return WithPath<DWORD>(name, INVALID_FILE_ATTRIBUTES,
        [](const wchar_t* path) { return GetFileAttributesW(path); });
}

inline BOOL SetAttributes(const char* name, DWORD attributes)
{
    return WithPath<BOOL>(name, FALSE,
        [attributes](const wchar_t* path) { return SetFileAttributesW(path, attributes); });
}

// Preserve ClearReadOnlyAttr's return convention: TRUE means an update was
// attempted (including the legacy fallback when attributes cannot be read).
inline BOOL ClearReadOnly(const char* name)
{
    const DWORD attributes = Attributes(name);
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        SetAttributes(name, FILE_ATTRIBUTE_ARCHIVE);
        return TRUE;
    }
    if ((attributes & FILE_ATTRIBUTE_READONLY) == 0)
        return FALSE;
    SetAttributes(name, attributes & ~FILE_ATTRIBUTE_READONLY);
    return TRUE;
}

inline BOOL Delete(const char* name)
{
    return WithPath<BOOL>(name, FALSE,
        [](const wchar_t* path) { return DeleteFileW(path); });
}

inline BOOL CreateDir(const char* name, LPSECURITY_ATTRIBUTES security = NULL)
{
    return WithPath<BOOL>(name, FALSE,
        [security](const wchar_t* path) { return CreateDirectoryW(path, security); });
}

inline BOOL RemoveDir(const char* name)
{
    return WithPath<BOOL>(name, FALSE,
        [](const wchar_t* path) { return RemoveDirectoryW(path); });
}

inline HANDLE Open(const char* name, DWORD access, DWORD sharing,
                   LPSECURITY_ATTRIBUTES security, DWORD creation,
                   DWORD flags, HANDLE templateFile)
{
    return WithPath<HANDLE>(name, INVALID_HANDLE_VALUE,
        [=](const wchar_t* path) {
            return CreateFileW(path, access, sharing, security, creation, flags, templateFile);
        });
}

inline HANDLE FindFirst(const char* name, WIN32_FIND_DATAW* data)
{
    return WithPath<HANDLE>(name, INVALID_HANDLE_VALUE,
        [data](const wchar_t* path) { return FindFirstFileW(path, data); });
}

inline BOOL Move(const char* source, const char* target, DWORD* error = NULL)
{
    DWORD failure = ERROR_SUCCESS;
    try
    {
        const std::wstring from = Path(source);
        const std::wstring to = Path(target);
        if (from.empty() || to.empty())
            failure = ERROR_INVALID_NAME;
        else if (MoveFileW(from.c_str(), to.c_str()))
        {
            if (error != NULL)
                *error = ERROR_SUCCESS;
            return TRUE;
        }
        else
        {
            failure = GetLastError();
            if (failure == ERROR_ACCESS_DENIED)
            {
                const DWORD attributes = GetFileAttributesW(from.c_str());
                if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
                {
                    // Preserve SalMoveFile's retry for file systems that refuse
                    // moving read-only files, restoring attributes in both cases.
                    SetFileAttributesW(from.c_str(), FILE_ATTRIBUTE_ARCHIVE);
                    if (MoveFileW(from.c_str(), to.c_str()))
                    {
                        SetFileAttributesW(to.c_str(), attributes);
                        if (error != NULL)
                            *error = ERROR_SUCCESS;
                        return TRUE;
                    }
                    failure = GetLastError();
                    SetFileAttributesW(from.c_str(), attributes);
                }
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        failure = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (error != NULL)
        *error = failure;
    SetLastError(failure);
    return FALSE;
}
}
