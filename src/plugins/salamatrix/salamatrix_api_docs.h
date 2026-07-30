// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "../shared/spl_gen.h"

namespace Salamatrix
{
namespace Documentation
{

inline BOOL GetAutomationApiReferencePath(
    CSalamanderGeneralAbstract* general,
    char* path,
    int pathCapacity)
{
    if (general == NULL || path == NULL || pathCapacity <= 0 ||
        GetModuleFileNameA(NULL, path, pathCapacity) == 0 ||
        !general->CutDirectory(path) ||
        !general->SalPathAppend(path, "plugins", pathCapacity) ||
        !general->SalPathAppend(path, "salamatrix", pathCapacity) ||
        !general->SalPathAppend(path, "salamatrix-automation-api.html",
                                pathCapacity))
        return FALSE;
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline BOOL OpenAutomationApiReference(
    CSalamanderGeneralAbstract* general,
    HWND parent)
{
    char path[SAL_MAX_PATH];
    return GetAutomationApiReferencePath(general, path, _countof(path)) &&
           general->OpenFileInConfiguredViewer(parent, path);
}

} // namespace Documentation
} // namespace Salamatrix
