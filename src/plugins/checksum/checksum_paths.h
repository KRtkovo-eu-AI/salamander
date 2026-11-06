// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

namespace ChecksumPaths
{
inline std::wstring Wide(const char* path)
{
    return SalamanderDiskSelection::WideFromPath(path);
}
inline std::string Utf8(const wchar_t* path)
{
    return SalamanderDiskSelection::Utf8FromWide(path);
}
inline std::wstring Join(const std::wstring& parent, const std::wstring& name)
{
    if (name.size() >= 2 && (name[1] == L':' || (name[0] == L'\\' && name[1] == L'\\')))
        return name;
    std::wstring result = parent;
    if (!result.empty() && result.back() != L'\\') result += L'\\';
    return result + name;
}
inline std::wstring IoPath(const std::wstring& path)
{
    if (path.size() < MAX_PATH || path.compare(0, 4, L"\\\\?\\") == 0) return path;
    DWORD capacity = GetFullPathNameW(path.c_str(), 0, NULL, NULL);
    if (!capacity || capacity > SAL_MAX_PATH) { SetLastError(ERROR_FILENAME_EXCED_RANGE); return std::wstring(); }
    std::vector<wchar_t> buffer(capacity);
    DWORD length = GetFullPathNameW(path.c_str(), capacity, buffer.data(), NULL);
    if (!length || length >= capacity) return std::wstring();
    return PluginPathAddExtendedPrefixW(buffer.data());
}
inline std::wstring IoPath(const char* path) { return IoPath(Wide(path)); }
inline std::wstring Parent(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L'\\');
    return slash == std::wstring::npos ? std::wstring() :
        path.substr(0, slash > 0 && path[slash - 1] == L':' ? slash + 1 : slash);
}
inline std::wstring Base(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L'\\');
    return path.substr(slash == std::wstring::npos ? 0 : slash + 1);
}
inline std::string Resolve(const char* directory, const char* entry)
{
    return Utf8(Join(Wide(directory), Wide(entry)).c_str());
}
}
