// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | 2026-11-08 | Declare the private panel listing export. | Pass the standard directory and plugin data within the host process. | | GPT-6.1 Sol (Codex) |
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Declare the private ZIP entry reader. | Stream verified output to a host-owned destination. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Define the private ZIP range reader interface. | Keep the public plugin SDK unchanged. | | GPT-6.1 Sol (Codex) |
#pragma once

#include <windows.h>

struct SalZipReadSourceV1
{
    ULONGLONG Size;
    void* Context;
    DWORD(WINAPI* Read)(void* context, ULONGLONG offset, void* bytes, DWORD count);
    DWORD(WINAPI* Entry)(void* context, const char* utf8Name, DWORD length);
};

class CSalamanderDirectoryAbstract;
class CPluginDataInterfaceAbstract;
using SalListZipPanelV1 = DWORD(WINAPI*)(const SalZipReadSourceV1*, CSalamanderDirectoryAbstract*,
                                       CPluginDataInterfaceAbstract**);

using SalListZipSourceV1 = DWORD(WINAPI*)(const SalZipReadSourceV1*);

struct CFileData;
using SalZipWrite = DWORD(WINAPI*)(void* context, const BYTE* bytes, DWORD count);
using SalReadZipEntryV1 = DWORD(WINAPI*)(const SalZipReadSourceV1*, const char*, const CFileData*,
                                        ULONGLONG, SalZipWrite, void*);
