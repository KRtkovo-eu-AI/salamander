// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Distinguish native installer errors from release-check errors. | Show the correct localized status after a scheduled installation fails. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Define native updater worker and UI handoff contracts. | Keep network work off the Salamander UI thread. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include "samandarin.h"
#include "native_catalog.h"

namespace SamandarinNative
{
struct CheckCompletion
{
    HWND Owner = nullptr;
    std::wstring CurrentVersion;
    std::wstring LatestVersion;
    std::wstring Error;
    std::wstring CatalogError;
    bool UserInitiated = false;
    bool ShowIfCurrent = false;
    bool NoPublishedReleases = false;
    bool NotifyRelease = false;
    bool InstallError = false;
    std::vector<PluginUpdateRow> PluginRows;
};

struct CatalogCompletion
{
    HWND Target = nullptr;
    std::vector<PluginUpdateRow> Rows;
    std::vector<std::wstring> Errors;
    bool Success = false;
};

struct InstallCompletion
{
    HWND Target = nullptr;
    std::wstring Status;
    std::wstring Error;
    std::vector<std::wstring> WebUrls;
    bool Success = false;
};

bool NativeUi_Initialize(HWND parent);
void NativeUi_Shutdown();
HWND NativeUi_GetMessageWindow();
BOOL NativeUi_ShowConfiguration(HWND parent, const NativeUpdateSettings& settings,
                                const std::wstring& currentVersion);
BOOL NativeUi_ShowPluginUpdates(HWND parent);
void NativeUi_NotifyColorsChanged();
void NativeUi_PostCheckCompletion(CheckCompletion* completion);
void NativeUi_PostCatalogCompletion(CatalogCompletion* completion);
void NativeUi_PostInstallCompletion(InstallCompletion* completion);
} // namespace SamandarinNative

namespace SamandarinNative
{
void NativeUpdater_RequestCatalogRefresh(HWND target);
void NativeUpdater_QueueInstallPlan(HWND target, std::vector<PluginUpdateRow> packages,
                                    std::vector<std::wstring> webUrls);
}
