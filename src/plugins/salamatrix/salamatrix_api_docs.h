// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "../shared/spl_gen.h"
#include <string>
#include <vector>
#include <new>

namespace Salamatrix
{
namespace Documentation
{

inline BOOL GetAutomationApiReferencePath(
    CSalamanderGeneralAbstract* general,
    char* path,
    int pathCapacity)
{
    if (path != NULL && pathCapacity > 0) path[0] = 0;
    if (general == NULL || path == NULL || pathCapacity <= 0) return FALSE;
    try
    {
        std::vector<wchar_t> module(SAL_MAX_PATH);
        const DWORD length = GetModuleFileNameW(NULL, module.data(), static_cast<DWORD>(module.size()));
        if (length == 0 || length >= module.size()) return FALSE;
        std::wstring reference(module.data(), length);
        const size_t slash = reference.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return FALSE;
        reference.resize(slash + 1);
        reference += L"plugins\\salamatrix\\salamatrix-automation-api.html";
        if (reference.size() >= SAL_MAX_PATH) return FALSE;
        std::wstring checked = reference;
        if (checked.size() >= MAX_PATH && checked.compare(0, 4, L"\\\\?\\") != 0)
            checked = checked.compare(0, 2, L"\\\\") == 0 ? L"\\\\?\\UNC\\" + checked.substr(2) : L"\\\\?\\" + checked;
        const DWORD attributes = GetFileAttributesW(checked.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
        const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, reference.c_str(), -1,
                                               NULL, 0, NULL, NULL);
        if (count <= 0 || count > pathCapacity) return FALSE;
        return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, reference.c_str(), -1,
                                    path, pathCapacity, NULL, NULL) != 0;
    }
    catch (const std::bad_alloc&) { return FALSE; }
}

inline BOOL OpenAutomationApiReference(
    CSalamanderGeneralAbstract* general,
    HWND parent)
{
    try
    {
        std::vector<char> path(SAL_MAX_PATH);
        return GetAutomationApiReferencePath(general, path.data(), static_cast<int>(path.size())) &&
               general->OpenFileInConfiguredViewer(parent, path.data());
    }
    catch (const std::bad_alloc&) { return FALSE; }
}

} // namespace Documentation
} // namespace Salamatrix
