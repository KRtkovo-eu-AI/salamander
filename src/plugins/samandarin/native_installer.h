// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Define native package staging and receipt APIs. | Replace SharpCompress, Authenticode interop, and PowerShell installation. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include "native_catalog.h"

namespace SamandarinNative
{
struct InstallReceipt
{
    std::wstring Signer;
    std::wstring PackageSha256;
    std::wstring Version;
};

bool StageAndSchedulePackage(const PluginUpdateRow& row, DWORD hostProcessId,
                             volatile LONG* cancelFlag, std::wstring* status,
                             std::wstring* error);
bool GetInstallReceipt(const std::wstring& id, PackageKind kind, InstallReceipt* receipt);
bool TakeLastInstallError(std::wstring* error);
} // namespace SamandarinNative

extern "C" void CALLBACK Samandarin_InstallAfterExit(HWND hwnd, HINSTANCE instance,
                                                       LPSTR commandLine, int showCommand);
