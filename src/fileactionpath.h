// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include "viewerpath.h"
#include <vector>

namespace Salamander { namespace FileActionPaths {
inline std::string Utf8(const std::wstring& path)
{
    if (path.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, NULL, 0, NULL, NULL);
    if (size <= 0) return std::string();
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &result[0], size, NULL, NULL);
    result.resize(size - 1);
    return result;
}
inline std::string Normalize(const char* text)
{
    return Utf8(ViewerPaths::Decode(text));
}
inline std::string ShortPath(const char* path)
{
    std::wstring wide = ViewerPaths::Decode(path);
    if (wide.size() >= MAX_PATH && wide.compare(0, 4, L"\\\\?\\") != 0)
        wide = wide.compare(0, 2, L"\\\\") == 0 ? L"\\\\?\\UNC\\" + wide.substr(2) : L"\\\\?\\" + wide;
    DWORD size = GetShortPathNameW(wide.c_str(), NULL, 0);
    if (size == 0) return std::string();
    std::wstring result(size, L'\0');
    DWORD written = GetShortPathNameW(wide.c_str(), &result[0], size);
    if (written == 0 || written >= size) return std::string();
    result.resize(written);
    // Expansion callbacks parse drive/UNC roots; keep their ordinary path syntax.
    if (result.compare(0, 8, L"\\\\?\\UNC\\") == 0) result = L"\\\\" + result.substr(8);
    else if (result.compare(0, 4, L"\\\\?\\") == 0) result.erase(0, 4);
    return Utf8(result);
}
inline std::wstring InitialDirectory(const char* path)
{
    std::wstring wide = ViewerPaths::Decode(path);
    // Windows process creation still rejects an over-MAX_PATH current directory,
    // even with longPathAware and an extended prefix. A DOS alias names the same
    // directory; keep the exact original path if the volume has no usable alias.
    if (wide.size() >= MAX_PATH)
    {
        std::wstring shortName = ViewerPaths::Decode(ShortPath(path).c_str());
        if (!shortName.empty() && shortName.size() < MAX_PATH) return shortName;
        if (wide.compare(0, 4, L"\\\\?\\") != 0)
            wide = wide.compare(0, 2, L"\\\\") == 0 ? L"\\\\?\\UNC\\" + wide.substr(2) : L"\\\\?\\" + wide;
    }
    return wide;
}
inline BOOL LaunchProcess(const char* command, const char* directory, DWORD flags,
                          STARTUPINFOW* startup, PROCESS_INFORMATION* process)
{
    std::wstring commandW = ViewerPaths::Decode(command);
    std::wstring directoryW = InitialDirectory(directory);
    if (commandW.empty() || commandW.size() >= 32767)
    {
        SetLastError(commandW.empty() ? ERROR_INVALID_PARAMETER : ERROR_FILENAME_EXCED_RANGE);
        return FALSE;
    }
    return CreateProcessW(NULL, &commandW[0], NULL, NULL, FALSE, flags, NULL,
                          directoryW.empty() ? NULL : directoryW.c_str(), startup, process);
}
}} // namespace Salamander::FileActionPaths
