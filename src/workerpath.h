// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <string>
#include <cstring>
#include <cwchar>

// Operation paths are UTF-8 when supplied by a panel, with an ACP fallback for
// legacy callers. Preserve an already-owned wide path whenever the operation has
// one, and protect exact trailing-dot/space names from Win32 normalization.
inline std::wstring WorkerOperationPathW(const char* name, const wchar_t* exactName = NULL)
{
    std::wstring path;
    if (exactName != NULL)
        path = exactName;
    else if (name != NULL && *name != 0)
    {
        UINT cp = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, NULL, 0) != 0 ? CP_UTF8 : CP_ACP;
        int length = MultiByteToWideChar(cp, 0, name, -1, NULL, 0);
        if (length > 0)
        {
            path.resize(length);
            MultiByteToWideChar(cp, 0, name, -1, &path[0], length);
            path.resize(length - 1);
        }
    }
    if (!path.empty() && (path.size() >= MAX_PATH || path.back() <= L' ' || path.back() == L'.') &&
        path.compare(0, 4, L"\\\\?\\") != 0)
    {
        if (path.compare(0, 2, L"\\\\") == 0)
            path = L"\\\\?\\UNC\\" + path.substr(2);
        else
            path = L"\\\\?\\" + path;
    }
    return path;
}

// All copy opens (including ADS inspection and retry after dialogs) use the same
// UTF-8/owned-wide boundary; ANSI CreateFile cannot reopen Win32 long paths.
inline HANDLE WorkerOpenFile(const char* name, const wchar_t* exactName,
                             DWORD access, DWORD share, LPSECURITY_ATTRIBUTES security,
                             DWORD disposition, DWORD flags, HANDLE templateFile)
{
    const std::wstring path = WorkerOperationPathW(name, exactName);
    return CreateFileW(path.c_str(), access, share, security, disposition, flags, templateFile);
}

inline BOOL WorkerClearReadOnlyW(const std::wstring& name, DWORD attrs = INVALID_FILE_ATTRIBUTES)
{
    if (attrs == INVALID_FILE_ATTRIBUTES)
        attrs = GetFileAttributesW(name.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES)
    {
        SetFileAttributesW(name.c_str(), FILE_ATTRIBUTE_ARCHIVE);
        return TRUE;
    }
    if ((attrs & FILE_ATTRIBUTE_READONLY) != 0)
    {
        SetFileAttributesW(name.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
        return TRUE;
    }
    return FALSE;
}

// Preserve the legacy read-only move retry used while resolving an 8.3 alias.
inline BOOL WorkerMoveFileW(const wchar_t* source, const wchar_t* target)
{
    if (MoveFileW(source, target)) return TRUE;
    DWORD error = GetLastError();
    if (error == ERROR_ACCESS_DENIED)
    {
        DWORD attrs = GetFileAttributesW(source);
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY))
        {
            SetFileAttributesW(source, FILE_ATTRIBUTE_ARCHIVE);
            if (MoveFileW(source, target))
            {
                SetFileAttributesW(target, attrs);
                return TRUE;
            }
            error = GetLastError();
            SetFileAttributesW(source, attrs);
        }
    }
    SetLastError(error);
    return FALSE;
}

inline BOOL WorkerCreateDirectory(const char* name, DWORD* err, const wchar_t* exactName = NULL)
{
    if (err != NULL) *err = ERROR_SUCCESS;
    std::wstring nameW = WorkerOperationPathW(name, exactName);
    if (CreateDirectoryW(nameW.c_str(), NULL)) return TRUE;
    DWORD errLoc = GetLastError();
    size_t nameLength = name != NULL ? strlen(name) : 0;
    BOOL hasTrailingDotSpace = nameLength != 0 &&
        ((unsigned char)name[nameLength - 1] <= ' ' || name[nameLength - 1] == '.');
    if (!hasTrailingDotSpace && (errLoc == ERROR_FILE_EXISTS || errLoc == ERROR_ALREADY_EXISTS))
    {
        WIN32_FIND_DATAW data;
        HANDLE find = FindFirstFileW(nameW.c_str(), &data);
        if (find != INVALID_HANDLE_VALUE)
        {
            FindClose(find);
            size_t separator = nameW.find_last_of(L"\\/");
            std::wstring parent = separator == std::wstring::npos ? L"" : nameW.substr(0, separator + 1);
            const wchar_t* targetName = nameW.c_str() + (separator == std::wstring::npos ? 0 : separator + 1);
            if (_wcsicmp(targetName, data.cAlternateFileName) == 0 &&
                _wcsicmp(targetName, data.cFileName) != 0)
            {
                std::wstring original = parent + data.cFileName;
                std::wstring temporary;
                DWORD originalAttrs = GetFileAttributesW(original.c_str());
                DWORD num = (GetTickCount() / 10) % 0xFFF;
                for (;;)
                {
                    wchar_t leaf[32];
                    swprintf_s(leaf, _countof(leaf), L"sal%03X", num++);
                    temporary = parent + leaf;
                    if (WorkerMoveFileW(original.c_str(), temporary.c_str())) break;
                    DWORD moveError = GetLastError();
                    if (moveError != ERROR_FILE_EXISTS && moveError != ERROR_ALREADY_EXISTS)
                    {
                        temporary.clear();
                        break;
                    }
                }
                if (!temporary.empty())
                {
                    BOOL created = CreateDirectoryW(nameW.c_str(), NULL);
                    if (!WorkerMoveFileW(temporary.c_str(), original.c_str()))
                    {
                        if (created)
                        {
                            if (RemoveDirectoryW(nameW.c_str())) created = FALSE;
                            WorkerMoveFileW(temporary.c_str(), original.c_str());
                        }
                    }
                    else if (originalAttrs != INVALID_FILE_ATTRIBUTES && (originalAttrs & FILE_ATTRIBUTE_ARCHIVE) == 0)
                        SetFileAttributesW(original.c_str(), originalAttrs);
                    if (created) return TRUE;
                }
            }
        }
    }
    if (err != NULL) *err = errLoc;
    return FALSE;
}
