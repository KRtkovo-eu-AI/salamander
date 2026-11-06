// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#define UNICODE
#define _UNICODE
#include "plugin_handles_wide_test_support/precomp.h"
#include <type_traits>
#ifdef MHANDLES_ENABLE
static_assert(std::is_same_v<decltype(&C__Handles::CreateFile),HANDLE(C__Handles::*)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE)>);
static_assert(std::is_same_v<decltype(&C__Handles::FindFirstFile),HANDLE(C__Handles::*)(LPCWSTR,LPWIN32_FIND_DATAW)>);
static_assert(std::is_same_v<decltype(&C__Handles::FindFirstFileA),HANDLE(C__Handles::*)(LPCSTR,LPWIN32_FIND_DATAA)>);
#endif
static_assert(std::is_same_v<decltype(HANDLES_Q(FindFirstFile(L"compile-only",(LPWIN32_FIND_DATAW)nullptr))),HANDLE>);
