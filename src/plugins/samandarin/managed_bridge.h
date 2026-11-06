// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

// Native callbacks hold this guard while they use host pointers/resources.
// Process exit can proceed only after all admitted callbacks have returned.
class CManagedBridgeNativeCall
{
public:
    CManagedBridgeNativeCall();
    ~CManagedBridgeNativeCall();
    bool IsAllowed() const { return Allowed; }
    CManagedBridgeNativeCall(const CManagedBridgeNativeCall&) = delete;
    CManagedBridgeNativeCall& operator=(const CManagedBridgeNativeCall&) = delete;
private:
    bool Allowed;
};
bool ManagedBridge_IsProcessExiting();
bool ManagedBridge_EnsureInitialized(HWND parent);
bool ManagedBridge_BeginInitialize(HWND parent);
void ManagedBridge_Shutdown();
bool ManagedBridge_ShowConfiguration(HWND parent);
void ManagedBridge_NotifyColorsChanged();
bool ManagedBridge_CheckNow(HWND parent);
bool ManagedBridge_ShowPluginUpdates(HWND parent);

