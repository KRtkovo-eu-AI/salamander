// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Internal host adapter for the existing SDK multi-string volume result.
// The plan owns paths before any readonly/delete I/O; it is not a filesystem transaction.
struct CUnpackArchiveVolume
{
    std::string Display;
    std::wstring Path;
};

static bool UnpackVolumeDiskPathW(const std::wstring& input, std::wstring& path)
{
    path = SalPathRemoveExtendedPrefixW(input.c_str());
    const size_t length = path.size();
    size_t start;
    size_t minimumComponents;
    if (length > 3 && ((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')) &&
        path[1] == L':' && path[2] == L'\\')
    {
        start = 3;
        minimumComponents = 1;
    }
    else if (length > 2 && path[0] == L'\\' && path[1] == L'\\')
    {
        start = 2;
        minimumComponents = 3; // server, share and a file
    }
    else { SetLastError(ERROR_INVALID_NAME); return false; }
    size_t components = 0;
    for (size_t i = start; i <= length; ++i)
    {
        if (i != length && path[i] != L'\\')
        {
            if (path[i] < 32 || wcschr(L"/:*?\"<>|", path[i]) != NULL)
            { SetLastError(ERROR_INVALID_NAME); return false; }
            continue;
        }
        if (i == start || path[i - 1] == L'.' || path[i - 1] == L' ')
        { SetLastError(ERROR_INVALID_NAME); return false; }
        ++components;
        start = i + 1;
    }
    if (components < minimumComponents || length >= SAL_MAX_PATH)
    { SetLastError(ERROR_INVALID_NAME); return false; }
    return true;
}

static bool PrepareUnpackArchiveVolumes(const char* text, int length, int allocated,
                                       const char* source, bool utf8First,
                                       std::vector<CUnpackArchiveVolume>& result)
{
    result.clear();
    if (length < 0 || allocated < 0 || length >= allocated || text == NULL)
    {
        if (length == 0 && allocated == 0 && text == NULL) return true;
        SetLastError(ERROR_INVALID_DATA); return false;
    }
    if (length == 0) return true; // legacy empty result; producer completeness is a separate contract
    try
    {
        if (source == NULL || !IsValidPathUtf8Text(source))
        { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return false; }
        std::wstring sourcePath(L"");
        if (!UnpackVolumeDiskPathW(SalMultiByteToWidePath(source, CP_UTF8), sourcePath)) return false;
        std::vector<CUnpackArchiveVolume> plan(0);
        bool containsSource = false;
        for (size_t offset = 0; offset < static_cast<size_t>(length);)
        {
            const char* begin = text + offset;
            const char* end = static_cast<const char*>(memchr(begin, 0, static_cast<size_t>(length) - offset));
            if (end == NULL || end == begin || end - begin >= SAL_MAX_PATH)
            { SetLastError(ERROR_INVALID_DATA); return false; }
            const std::string bytes(begin, end);
            const UINT encoding = utf8First && IsValidPathUtf8Text(bytes.c_str()) ? CP_UTF8 : CP_ACP;
            const std::wstring decoded = SalMultiByteToWidePath(bytes.c_str(), encoding);
            if (decoded.empty() || SalWideToMultiBytePath(decoded.c_str(), encoding) != bytes)
            { SetLastError(ERROR_NO_UNICODE_TRANSLATION); return false; }
            std::wstring path(L"");
            if (!UnpackVolumeDiskPathW(decoded, path)) return false;
            if (path == sourcePath) containsSource = true;
            // Exact spelling only: case-sensitive Windows directories may contain both names.
            bool duplicate = false;
            for (const auto& volume : plan)
                if (SalPathRemoveExtendedPrefixW(volume.Path.c_str()) == path) { duplicate = true; break; }
            if (!duplicate)
                plan.push_back({SalWideToMultiBytePath(decoded.c_str(), CP_UTF8),
                                path.length() >= MAX_PATH ? SalPathAddExtendedPrefixW(path.c_str()) : path});
            offset += bytes.size() + 1;
        }
        if (!containsSource) { SetLastError(ERROR_INVALID_DATA); return false; }
        result.swap(plan);
        return true;
    }
    catch (const std::bad_alloc&) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return false; }
}

static BOOL DeleteUnpackArchiveVolumeW(const wchar_t* path)
{
    const DWORD attributes = GetFileAttributesW(path);
    if (attributes == INVALID_FILE_ATTRIBUTES)
        return GetLastError() == ERROR_FILE_NOT_FOUND; // preserve already-removed-file behavior
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    { SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    // Preserve all attributes except readonly; never replace unknown attributes with ARCHIVE.
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0 &&
        !SetFileAttributesW(path, attributes & ~FILE_ATTRIBUTE_READONLY)) return FALSE;
    return DeleteFileW(path);
}
