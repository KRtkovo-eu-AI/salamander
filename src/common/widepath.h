// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

class CPathBuffer
{
public:
    CPathBuffer();
    explicit CPathBuffer(int reserveChars);
    char* Data();
    const char* Data() const;
    int Capacity() const;
    BOOL Ensure(int chars);

private:
    std::vector<char> Buffer;
};

class CWidePathBuffer
{
public:
    CWidePathBuffer();
    explicit CWidePathBuffer(int reserveChars);
    wchar_t* Data();
    const wchar_t* Data() const;
    int Capacity() const;
    BOOL Ensure(int chars);

private:
    std::vector<wchar_t> Buffer;
};

BOOL SalIsExtendedLengthPathW(const wchar_t* path);
std::wstring SalPathAddExtendedPrefixW(const wchar_t* path);
std::wstring SalPathRemoveExtendedPrefixW(const wchar_t* path);
BOOL IsValidPathUtf8Text(const char* text);
std::wstring SalMultiByteToWidePath(const char* path, UINT codePage = CP_ACP);
std::string SalWideToMultiBytePath(const wchar_t* path, UINT codePage = CP_ACP);
BOOL SalPathAppendW(std::wstring& path, const wchar_t* name);
const wchar_t* SalPathFindFileNameW(const wchar_t* path);
HANDLE SalFindFirstFileHW(const char* fileName, LPWIN32_FIND_DATAW findData);
wchar_t* BuildNameW(const wchar_t* path, const wchar_t* name, const wchar_t* dosName,
                    BOOL* skip, BOOL* skipAll, const wchar_t* sourcePath);
BOOL SalGetFullNameW(std::wstring& name, int* errTextID, const wchar_t* curDir,
                     std::wstring* nextFocus, BOOL* callNethood,
                     BOOL allowRelPathWithSpaces);
