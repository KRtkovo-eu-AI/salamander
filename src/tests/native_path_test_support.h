// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <string>
#include <vector>

// The caller creates a uniquely named child and removes only its own fixtures.
inline std::wstring NativePathTestParent(int argc, wchar_t** argv)
{
    if (argc > 1) return argv[1];
    DWORD capacity = GetTempPathW(0, NULL);
    if (capacity == 0) return std::wstring();
    std::vector<wchar_t> buffer(capacity + 1);
    DWORD length = GetTempPathW((DWORD)buffer.size(), buffer.data());
    if (length == 0 || length >= buffer.size()) return std::wstring();
    std::wstring result(buffer.data(), length);
    // Some Windows CI runners expose TEMP through an 8.3 alias. Compare actual
    // child-process working directories against the expanded spelling.
    DWORD longCapacity = GetLongPathNameW(result.c_str(), NULL, 0);
    if (longCapacity != 0)
    {
        std::vector<wchar_t> longPath(longCapacity);
        DWORD longLength = GetLongPathNameW(result.c_str(), longPath.data(), longCapacity);
        if (longLength != 0 && longLength < longCapacity) result.assign(longPath.data(), longLength);
    }
    while (result.size() > 3 && result.back() == L'\\') result.pop_back();
    return result;
}
