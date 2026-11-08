// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
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

using SalListZipSourceV1 = DWORD(WINAPI*)(const SalZipReadSourceV1*);
