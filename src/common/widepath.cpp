// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "widepath.h"

#include "../texts.rh2"

static const int SAL_MAX_EXTENDED_PATH = 32768;

CPathBuffer::CPathBuffer() : Buffer(SAL_MAX_EXTENDED_PATH, 0) {}
CPathBuffer::CPathBuffer(int reserveChars) : Buffer(max(reserveChars, 1), 0) {}
char* CPathBuffer::Data() { return Buffer.data(); }
const char* CPathBuffer::Data() const { return Buffer.data(); }
int CPathBuffer::Capacity() const { return (int)Buffer.size(); }
BOOL CPathBuffer::Ensure(int chars)
{
    if (chars <= 0)
        chars = 1;
    if ((int)Buffer.size() < chars)
        Buffer.resize(chars, 0);
    return TRUE;
}

CWidePathBuffer::CWidePathBuffer() : Buffer(SAL_MAX_EXTENDED_PATH, 0) {}
CWidePathBuffer::CWidePathBuffer(int reserveChars) : Buffer(max(reserveChars, 1), 0) {}
wchar_t* CWidePathBuffer::Data() { return Buffer.data(); }
const wchar_t* CWidePathBuffer::Data() const { return Buffer.data(); }
int CWidePathBuffer::Capacity() const { return (int)Buffer.size(); }
BOOL CWidePathBuffer::Ensure(int chars)
{
    if (chars <= 0)
        chars = 1;
    if ((int)Buffer.size() < chars)
        Buffer.resize(chars, 0);
    return TRUE;
}

BOOL SalIsExtendedLengthPathW(const wchar_t* path)
{
    return path != NULL && wcsncmp(path, L"\\\\?\\", 4) == 0;
}

std::wstring SalPathAddExtendedPrefixW(const wchar_t* path)
{
    if (path == NULL || *path == 0 || SalIsExtendedLengthPathW(path))
        return path != NULL ? std::wstring(path) : std::wstring();
    if (wcsncmp(path, L"\\\\", 2) == 0)
        return std::wstring(L"\\\\?\\UNC\\") + (path + 2);
    return std::wstring(L"\\\\?\\") + path;
}

std::wstring SalPathRemoveExtendedPrefixW(const wchar_t* path)
{
    if (path == NULL)
        return std::wstring();
    if (wcsncmp(path, L"\\\\?\\UNC\\", 8) == 0)
        return std::wstring(L"\\\\") + (path + 8);
    if (SalIsExtendedLengthPathW(path))
        return std::wstring(path + 4);
    return std::wstring(path);
}

BOOL IsValidPathUtf8Text(const char* text)
{
    if (text == NULL) return FALSE;
    return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0) != 0;
}

std::wstring SalMultiByteToWidePath(const char* path, UINT codePage)
{
    if (path == NULL || *path == 0)
        return std::wstring();
    if (codePage == CP_ACP && GetACP() == CP_UTF8)
        codePage = CP_UTF8;
    int len = MultiByteToWideChar(codePage, 0, path, -1, NULL, 0);
    if (len <= 0 && codePage != CP_ACP)
        len = MultiByteToWideChar(CP_ACP, 0, path, -1, NULL, 0), codePage = CP_ACP;
    if (len <= 0)
        return std::wstring();
    std::wstring ret(len, L'\0');
    MultiByteToWideChar(codePage, 0, path, -1, &ret[0], len);
    ret.resize(len - 1);
    return ret;
}

std::string SalWideToMultiBytePath(const wchar_t* path, UINT codePage)
{
    if (path == NULL || *path == 0)
        return std::string();
    if (codePage == CP_ACP && GetACP() == CP_UTF8)
        codePage = CP_UTF8;
    int len = WideCharToMultiByte(codePage, 0, path, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        return std::string();
    std::string ret(len - 1, '\0');
    WideCharToMultiByte(codePage, 0, path, -1, &ret[0], len, NULL, NULL);
    return ret;
}

BOOL SalPathAppendW(std::wstring& path, const wchar_t* name)
{
    if (name == NULL)
        return FALSE;
    if (!path.empty() && path[path.length() - 1] != L'\\' && path[path.length() - 1] != L'/')
        path += L'\\';
    path += name;
    return TRUE;
}

const wchar_t* SalPathFindFileNameW(const wchar_t* path)
{
    if (path == NULL)
        return NULL;
    const wchar_t* last = path;
    for (const wchar_t* s = path; *s != 0; s++)
        if (*s == L'\\' || *s == L'/' || *s == L':')
            last = s + 1;
    return last;
}

HANDLE SalFindFirstFileHW(const char* fileName, LPWIN32_FIND_DATAW findData)
{
    if (fileName == NULL || findData == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    std::wstring wideName = SalMultiByteToWidePath(fileName, CP_UTF8);
    if (wideName.empty() && GetACP() != CP_UTF8)
        wideName = SalMultiByteToWidePath(fileName, CP_ACP);
    if (wideName.empty())
    {
        SetLastError(ERROR_INVALID_NAME);
        return INVALID_HANDLE_VALUE;
    }

    if (wideName.length() >= MAX_PATH && !SalIsExtendedLengthPathW(wideName.c_str()))
        wideName = SalPathAddExtendedPrefixW(wideName.c_str());

    return HANDLES_Q(FindFirstFileW(wideName.c_str(), findData));
}

wchar_t* BuildNameW(const wchar_t* path, const wchar_t* name, const wchar_t* dosName,
                    BOOL* skip, BOOL* skipAll, const wchar_t* sourcePath)
{
    if (skip != NULL)
        *skip = FALSE;
    if (path == NULL || name == NULL)
        return NULL;
    std::wstring full(path);
    SalPathAppendW(full, name);
    if (full.length() >= MAX_PATH)
        full = SalPathAddExtendedPrefixW(full.c_str());
    wchar_t* ret = (wchar_t*)malloc((full.length() + 1) * sizeof(wchar_t));
    if (ret != NULL)
        wcscpy(ret, full.c_str());
    return ret;
}

BOOL SalGetFullNameW(std::wstring& name, int* errTextID, const wchar_t* curDir,
                     std::wstring* nextFocus, BOOL* callNethood, BOOL allowRelPathWithSpaces)
{
    if (errTextID != NULL)
        *errTextID = 0;
    if (callNethood != NULL)
        *callNethood = FALSE;
    if (name.empty())
    {
        if (errTextID != NULL)
            *errTextID = IDS_EMPTYNAMENOTALLOWED;
        return FALSE;
    }
    size_t first = allowRelPathWithSpaces ? 0 : name.find_first_not_of(L" \t\r\n");
    if (first != std::wstring::npos && first > 0)
        name.erase(0, first);
    if (SalIsExtendedLengthPathW(name.c_str()))
        return TRUE;
    if (name.length() >= 2 && name[1] == L':' || name.length() >= 2 && name[0] == L'\\' && name[1] == L'\\')
        return TRUE;
    if (curDir != NULL && *curDir != 0)
    {
        std::wstring full(curDir);
        SalPathAppendW(full, name.c_str());
        name = full;
        return TRUE;
    }
    return TRUE;
}
