// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

void PluginSecurityFormatForPlugin(const char* pluginId, const char* dllName, char* buffer, int bufferSize);
void PluginSecurityFormatForExtension(const char* extensionId,
                                      const char* networkAccess,
                                      const char* externalProcesses,
                                      const char* scriptExecution,
                                      const char* activeWebContent,
                                      const char* elevation,
                                      char* buffer, int bufferSize);
