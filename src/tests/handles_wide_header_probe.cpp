// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
// Compile the actual common header in a separate Unicode translation unit.
#define UNICODE
#define _UNICODE
#include "handles_wide_test_support/precomp.h"
#include <type_traits>
#include "handles.h"
#ifdef HANDLES_ENABLE
static_assert(std::is_same_v<decltype(&C__Handles::LoadImage), HANDLE (C__Handles::*)(HINSTANCE, LPCWSTR, UINT, int, int, UINT)>);
static_assert(std::is_same_v<decltype(&C__Handles::FindFirstChangeNotification), HANDLE (C__Handles::*)(LPCWSTR, BOOL, DWORD)>);
static_assert(std::is_same_v<decltype(&C__Handles::LoadImageA), HANDLE (C__Handles::*)(HINSTANCE, LPCSTR, UINT, int, int, UINT)>);
static_assert(std::is_same_v<decltype(&C__Handles::FindFirstChangeNotificationA), HANDLE (C__Handles::*)(LPCSTR, BOOL, DWORD)>);
#endif
static_assert(std::is_same_v<decltype(HANDLES_Q(LoadImage(NULL, MAKEINTRESOURCE(32512), IMAGE_ICON, 32, 32, 0))), HANDLE>);
