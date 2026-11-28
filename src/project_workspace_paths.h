// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Resolve project-workspace path containment across long paths and reparse points. | Keep workspace navigation inside the fixed physical root. | https://github.com/KRtkovo-eu-AI/salamander/issues/853 | openai/gpt-6-luna-pro |
#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace Salamander { namespace ProjectWorkspacePaths {

inline bool Normalize(const wchar_t* path, std::wstring& normalized)
{
    normalized.clear();
    if (path == NULL || *path == 0)
        return false;

    std::wstring input(path);
    if (input.size() >= 8 &&
        CompareStringOrdinal(input.c_str(), 8, L"\\\\?\\UNC\\", 8, TRUE) == CSTR_EQUAL)
        input = L"\\\\" + input.substr(8);
    else if (input.size() >= 4 &&
             CompareStringOrdinal(input.c_str(), 4, L"\\\\?\\", 4, TRUE) == CSTR_EQUAL)
        input.erase(0, 4);
    else if (input.size() >= 4 &&
             CompareStringOrdinal(input.c_str(), 4, L"\\\\.\\", 4, TRUE) == CSTR_EQUAL)
        return false;

    for (size_t i = 0; i < input.size(); ++i)
        if (input[i] == L'/')
            input[i] = L'\\';

    std::wstring root;
    size_t position = 0;
    if (input.size() >= 3 && input[1] == L':' && input[2] == L'\\')
    {
        root.assign(input, 0, 3);
        position = 3;
    }
    else if (input.size() >= 2 && input[0] == L'\\' && input[1] == L'\\' &&
             (input.size() == 2 || input[2] != L'\\'))
    {
        const size_t serverStart = 2;
        const size_t serverEnd = input.find(L'\\', serverStart);
        if (serverEnd == std::wstring::npos || serverEnd == serverStart)
            return false;

        size_t shareStart = serverEnd + 1;
        while (shareStart < input.size() && input[shareStart] == L'\\')
            ++shareStart;
        if (shareStart == input.size())
            return false;

        const size_t shareEnd = input.find(L'\\', shareStart);
        const size_t shareLength = (shareEnd == std::wstring::npos ? input.size() : shareEnd) - shareStart;
        if (shareLength == 0)
            return false;

        root = L"\\\\" + input.substr(serverStart, serverEnd - serverStart) + L"\\" +
               input.substr(shareStart, shareLength);
        position = shareEnd == std::wstring::npos ? input.size() : shareEnd + 1;
    }
    else
        return false;

    std::vector<std::wstring> components;
    while (position < input.size())
    {
        while (position < input.size() && input[position] == L'\\')
            ++position;
        if (position == input.size())
            break;

        const size_t end = input.find(L'\\', position);
        const size_t length = (end == std::wstring::npos ? input.size() : end) - position;
        const std::wstring component = input.substr(position, length);
        if (component == L"..")
        {
            if (!components.empty())
                components.pop_back();
        }
        else if (!component.empty() && component != L".")
            components.push_back(component);

        position = end == std::wstring::npos ? input.size() : end + 1;
    }

    normalized = root;
    for (size_t i = 0; i < components.size(); ++i)
    {
        if (!normalized.empty() && normalized[normalized.size() - 1] != L'\\')
            normalized += L'\\';
        normalized += components[i];
    }
    return true;
}

inline bool RootLength(const std::wstring& path, size_t& length)
{
    if (path.size() >= 3 && path[1] == L':' && path[2] == L'\\')
    {
        length = 3;
        return true;
    }
    if (path.size() < 5 || path[0] != L'\\' || path[1] != L'\\')
        return false;

    const size_t serverEnd = path.find(L'\\', 2);
    if (serverEnd == std::wstring::npos || serverEnd == 2)
        return false;
    const size_t shareEnd = path.find(L'\\', serverEnd + 1);
    if (shareEnd == serverEnd + 1)
        return false;
    length = shareEnd == std::wstring::npos ? path.size() : shareEnd;
    return true;
}

inline bool IsWithinNormalized(const std::wstring& root, const std::wstring& path)
{
    const size_t rootLength = root.size();
    if (path.size() < rootLength ||
        CompareStringOrdinal(root.c_str(), static_cast<int>(rootLength),
                             path.c_str(), static_cast<int>(rootLength), TRUE) != CSTR_EQUAL)
        return false;
    return path.size() == rootLength || root[rootLength - 1] == L'\\' || path[rootLength] == L'\\';
}

inline bool IsSameLexically(const wchar_t* first, const wchar_t* second)
{
    std::wstring normalizedFirst, normalizedSecond;
    return Normalize(first, normalizedFirst) && Normalize(second, normalizedSecond) &&
           CompareStringOrdinal(normalizedFirst.c_str(), -1,
                                normalizedSecond.c_str(), -1, TRUE) == CSTR_EQUAL;
}

inline bool IsWithinLexically(const wchar_t* root, const wchar_t* path)
{
    std::wstring normalizedRoot, normalizedPath;
    return Normalize(root, normalizedRoot) && Normalize(path, normalizedPath) &&
           IsWithinNormalized(normalizedRoot, normalizedPath);
}

inline std::wstring ExtendedPath(const std::wstring& path)
{
    if (path.compare(0, 4, L"\\\\?\\") == 0)
        return path;
    if (path.compare(0, 2, L"\\\\") == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);
    return L"\\\\?\\" + path;
}

inline bool TryGetFinalPath(const std::wstring& path, std::wstring& finalPath, DWORD* failureError = NULL)
{
    if (failureError != NULL)
        *failureError = ERROR_SUCCESS;

    const std::wstring extended = ExtendedPath(path);
    HANDLE handle = CreateFileW(extended.c_str(), FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        if (failureError != NULL)
            *failureError = GetLastError();
        return false;
    }

    const DWORD flags = FILE_NAME_NORMALIZED | VOLUME_NAME_DOS;
    const DWORD required = GetFinalPathNameByHandleW(handle, NULL, 0, flags);
    if (required == 0)
    {
        if (failureError != NULL)
            *failureError = ERROR_GEN_FAILURE;
        CloseHandle(handle);
        return false;
    }
    if (required > 32768)
    {
        if (failureError != NULL)
            *failureError = ERROR_FILENAME_EXCED_RANGE;
        CloseHandle(handle);
        return false;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(required) + 1, L'\0');
    const DWORD written = GetFinalPathNameByHandleW(handle, buffer.data(),
                                                    static_cast<DWORD>(buffer.size()), flags);
    CloseHandle(handle);
    if (written == 0 || written >= buffer.size())
    {
        if (failureError != NULL)
            *failureError = written == 0 ? ERROR_GEN_FAILURE : ERROR_INSUFFICIENT_BUFFER;
        return false;
    }

    std::wstring result;
    if (!Normalize(std::wstring(buffer.data(), written).c_str(), result))
    {
        if (failureError != NULL)
            *failureError = ERROR_INVALID_NAME;
        return false;
    }
    finalPath.swap(result);
    return true;
}

enum ReparsePointStatus
{
    ReparsePointMissing,
    ReparsePointNo,
    ReparsePointYes,
    ReparsePointUnknown
};

inline ReparsePointStatus InspectReparsePoint(const std::wstring& path)
{
    const std::wstring extended = ExtendedPath(path);
    HANDLE handle = CreateFileW(extended.c_str(), FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (handle == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND
            ? ReparsePointMissing
            : ReparsePointUnknown;
    }

    BY_HANDLE_FILE_INFORMATION information;
    const BOOL gotInformation = GetFileInformationByHandle(handle, &information);
    CloseHandle(handle);
    if (!gotInformation)
        return ReparsePointUnknown;
    return (information.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0
        ? ReparsePointYes
        : ReparsePointNo;
}

inline bool ResolveThroughExistingAncestor(const std::wstring& path, std::wstring& resolvedPath)
{
    std::wstring candidate;
    if (!Normalize(path.c_str(), candidate))
        return false;

    size_t rootLength = 0;
    if (!RootLength(candidate, rootLength))
        return false;

    std::wstring unresolvedSuffix;
    for (;;)
    {
        std::wstring finalPath;
        DWORD error = ERROR_SUCCESS;
        if (TryGetFinalPath(candidate, finalPath, &error))
        {
            if (!unresolvedSuffix.empty())
            {
                if (!finalPath.empty() && finalPath[finalPath.size() - 1] == L'\\' &&
                    unresolvedSuffix[0] == L'\\')
                    unresolvedSuffix.erase(0, 1);
                finalPath += unresolvedSuffix;
            }
            return Normalize(finalPath.c_str(), resolvedPath);
        }

        // Only a genuinely missing suffix may inherit its existing parent's path.
        // Never turn an inaccessible or unresolved reparse point into a lexical in-root path.
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            return false;
        const ReparsePointStatus reparseStatus = InspectReparsePoint(candidate);
        if (reparseStatus == ReparsePointYes || reparseStatus == ReparsePointUnknown)
            return false;

        if (candidate.size() <= rootLength)
            return false;
        const size_t separator = candidate.find_last_of(L'\\');
        if (separator == std::wstring::npos)
            return false;
        unresolvedSuffix = candidate.substr(separator) + unresolvedSuffix;
        if (separator < rootLength)
            candidate.resize(rootLength);
        else
            candidate.resize(separator);
    }
}

inline bool IsWithinProjectRoot(const wchar_t* rootPath, const wchar_t* targetPath)
{
    std::wstring root, target;
    if (!Normalize(rootPath, root) || !Normalize(targetPath, target))
        return false;

    std::wstring resolvedRoot, resolvedTarget;
    if (!TryGetFinalPath(root, resolvedRoot) || !ResolveThroughExistingAncestor(target, resolvedTarget))
        return false;
    return IsWithinNormalized(resolvedRoot, resolvedTarget);
}

} } // namespace Salamander::ProjectWorkspacePaths
