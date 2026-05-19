// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

bool ManagedBridge_EnsureInitialized(HWND parent);
void ManagedBridge_Shutdown();
bool ManagedBridge_ShowAbout(HWND parent);
bool ManagedBridge_ShowConfiguration(HWND parent);
bool ManagedBridge_RunMenuCommand(HWND parent, const char* command);

