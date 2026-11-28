// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Preserve catalog and installed icon references in update rows. | Recreate the original plugin-image column and its catalog-image fallback. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Declare the trusted catalog policy for elevated installs. | Share the post-exit package validation contract. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Define native catalog rows and source configuration APIs. | Share catalog results with native dialogs and installer. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include <string>
#include <vector>

namespace SamandarinNative
{
enum class PackageKind
{
    Unknown,
    Plugin,
    Extension,
};

enum class PluginUpdateStatus
{
    Other,
    UpdateAvailable,
    NotInCatalog,
    CatalogError,
};

struct CatalogSecurity
{
    std::wstring NetworkAccess;
    std::wstring ExternalProcesses;
    std::wstring ScriptExecution;
    std::wstring ActiveWebContent;
    std::wstring Elevation;

    bool IsHigherRisk() const;
};

struct PluginUpdateRow
{
    std::wstring Name;
    std::wstring InstalledVersion;
    std::wstring LatestVersion;
    std::wstring StatusText;
    std::wstring Author;
    std::wstring Homepage;
    PluginUpdateStatus Status = PluginUpdateStatus::Other;
    std::wstring Source;
    std::wstring WebUrl;
    std::wstring Description;
    std::wstring IconPath;
    std::wstring CatalogIconReference;
    std::wstring CatalogSourceUrl;
    std::wstring Id;
    PackageKind InstalledKind = PackageKind::Unknown;
    PackageKind CatalogKind = PackageKind::Unknown;
    std::wstring InstallDirectory;
    std::wstring PackageSha256;
    std::vector<std::wstring> Dependencies;
    CatalogSecurity Security;
};

struct CatalogIconImage
{
    std::wstring Reference;
    std::string Bytes;
};

struct CatalogSource
{
    std::wstring Url;
    bool Enabled = true;
};

bool FetchPluginCatalog(std::vector<PluginUpdateRow>* rows,
                        std::vector<std::wstring>* errors,
                        volatile LONG* cancelFlag = nullptr);
std::vector<CatalogSource> LoadCatalogSources();
bool SaveCatalogSources(const std::vector<CatalogSource>& sources);
bool IsTrustedInstallCatalogUrl(const std::wstring& url);
std::wstring ResolveCatalogIconReference(const PluginUpdateRow& row);
bool CanInstallPluginRow(const PluginUpdateRow& row);
int CompareVersions(const std::wstring& left, const std::wstring& right);
std::wstring GetLocalizedString(int resourceId);
std::wstring FormatLocalizedString(int resourceId, const std::vector<std::wstring>& arguments);
} // namespace SamandarinNative
