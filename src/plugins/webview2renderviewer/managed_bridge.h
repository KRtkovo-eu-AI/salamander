// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

bool ManagedBridge_EnsureInitialized(HWND parent);
void ManagedBridge_Shutdown();
bool ManagedBridge_RequestShutdown(HWND parent, bool forceClose);
bool ManagedBridge_ViewDocument(HWND parent, const char* filePath, const RECT& placement,
                                UINT showCmd, BOOL alwaysOnTop, HANDLE fileLock, bool asynchronous);
