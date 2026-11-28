// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Restrict elevated package installs to trusted catalog origins. | The post-exit helper revalidates unsigned package contents against HTTPS catalog data. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Keep catalog status text localized. | Parser diagnostics may include untranslated implementation details. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Port catalog loading, installed package scanning, and version matching to C++. | Remove the managed catalog service and retain its update classifications. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_catalog.h"
#include "native_http.h"
#include "native_json.h"
#include "native_utils.h"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <winver.h>

#pragma comment(lib, "version.lib")

extern "C" int __stdcall Samandarin_ExportInstalledPlugins(char* buffer, int cchBuffer);
extern "C" int __stdcall Samandarin_ExportInstalledExtensions(wchar_t* buffer, int cchBuffer);

namespace SamandarinNative
{
namespace
{
using SamandarinJson::Value;

const wchar_t* const kStablePluginSource = L"https://samandarin.krtkovo.eu/catalogs/plugins-stable.json";
const wchar_t* const kStableExtensionSource = L"https://samandarin.krtkovo.eu/catalogs/extensions-stable.json";
const wchar_t* const kUnofficialPluginSource = L"https://samandarin.krtkovo.eu/catalogs/plugins-unofficial.json";
const wchar_t* const kExtensionRuntimeSource = L"https://samandarin.krtkovo.eu/catalogs/extension-runtimes.json";

struct InstalledPackage
{
    std::wstring Id;
    std::wstring Name;
    std::wstring Version;
    std::wstring IconPath;
    std::wstring InstallDirectory;
    PackageKind Kind = PackageKind::Unknown;
};

struct CatalogEntry
{
    std::wstring Id;
    std::wstring PackageType;
    std::wstring Name;
    std::wstring Author;
    std::wstring Description;
    std::wstring LatestVersion;
    std::wstring VersionScheme;
    std::wstring Homepage;
    std::wstring DownloadPage;
    std::wstring PackageSha256;
    std::wstring Icon;
    std::wstring Source;
    std::wstring SourceUrl;
    std::vector<std::wstring> Dependencies;
    CatalogSecurity Security;
    PackageKind Kind = PackageKind::Unknown;
};

std::wstring Trim(std::wstring value)
{
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return std::wstring();
    const size_t last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::wstring LowerAscii(std::wstring value)
{
    for (wchar_t& c : value)
    {
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    }
    return value;
}

bool EqualsIgnoreCase(const std::wstring& left, const std::wstring& right)
{
    return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
}

bool StartsWithIgnoreCase(const std::wstring& value, const std::wstring& prefix)
{
    return value.size() >= prefix.size() &&
           CompareStringOrdinal(value.data(), static_cast<int>(prefix.size()),
                                prefix.data(), static_cast<int>(prefix.size()), TRUE) == CSTR_EQUAL;
}

std::wstring GetStringValue(const Value* object, const wchar_t* name)
{
    if (object == nullptr) return std::wstring();
    const Value* value = object->Find(name);
    return value == nullptr ? std::wstring() : value->StringOrEmpty();
}

std::vector<std::wstring> Split(const std::wstring& value, wchar_t delimiter)
{
    std::vector<std::wstring> parts;
    size_t start = 0;
    for (;;)
    {
        const size_t end = value.find(delimiter, start);
        parts.push_back(value.substr(start, end == std::wstring::npos ? std::wstring::npos : end - start));
        if (end == std::wstring::npos) break;
        start = end + 1;
    }
    return parts;
}

std::wstring NormalizeLanguageKey(std::wstring value)
{
    std::wstring result;
    for (wchar_t c : value)
    {
        if (c == L'-' || c == L'_' || c == L' ') continue;
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
        result.push_back(c);
    }
    if (result == L"cs" || result == L"cz" || result == L"czech") return L"czech";
    if (result == L"de" || result == L"ger" || result == L"deu" || result == L"german") return L"german";
    if (result == L"fr" || result == L"fre" || result == L"fra" || result == L"french") return L"french";
    if (result == L"nl" || result == L"dut" || result == L"nld" || result == L"dutch") return L"dutch";
    if (result == L"hu" || result == L"hun" || result == L"hungarian") return L"hungarian";
    if (result == L"ro" || result == L"rum" || result == L"ron" || result == L"romanian") return L"romanian";
    if (result == L"ru" || result == L"rus" || result == L"russian") return L"russian";
    if (result == L"sk" || result == L"slo" || result == L"slk" || result == L"slovak") return L"slovak";
    if (result == L"es" || result == L"spa" || result == L"spanish") return L"spanish";
    if (result == L"zh" || result == L"zho" || result == L"chi" || result == L"chinese" ||
        result == L"chinesesimplified" || result == L"chs") return L"chinesesimplified";
    if (result == L"en" || result == L"eng" || result == L"english") return L"english";
    return result;
}

std::vector<std::wstring> GetLanguageCandidates()
{
    std::vector<std::wstring> candidates;
    const std::wstring modulePath = SamandarinNative::GetModulePath(HLanguage);
    if (!modulePath.empty())
    {
        candidates.push_back(GetFileNameWithoutExtension(modulePath));
        candidates.push_back(GetFileName(GetParentDirectory(modulePath)));
    }
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH] = {};
    if (LCIDToLocaleName(GetUserDefaultUILanguage(), localeName, _countof(localeName), 0) != 0)
    {
        wchar_t language[32] = {};
        if (GetLocaleInfoEx(localeName, LOCALE_SISO639LANGNAME, language, _countof(language)) > 0)
            candidates.push_back(language);
    }
    return candidates;
}

std::wstring ResolveLocalizedText(const Value* value)
{
    if (value == nullptr) return std::wstring();
    if (value->Kind == Value::Type::String) return value->Text;
    if (value->Kind != Value::Type::Object) return value->StringOrEmpty();

    std::set<std::wstring> tried;
    for (const std::wstring& candidate : GetLanguageCandidates())
    {
        const std::wstring normalized = NormalizeLanguageKey(candidate);
        if (!normalized.empty() && tried.insert(normalized).second)
        {
            const Value* localized = value->Find(normalized);
            if (localized != nullptr && !Trim(localized->StringOrEmpty()).empty()) return localized->StringOrEmpty();
        }
        const std::wstring raw = LowerAscii(candidate);
        if (!raw.empty() && tried.insert(raw).second)
        {
            const Value* localized = value->Find(raw);
            if (localized != nullptr && !Trim(localized->StringOrEmpty()).empty()) return localized->StringOrEmpty();
        }
    }
    for (const wchar_t* english : {L"english", L"en"})
    {
        const Value* localized = value->Find(english);
        if (localized != nullptr && !Trim(localized->StringOrEmpty()).empty()) return localized->StringOrEmpty();
    }
    for (const auto& item : value->Object)
    {
        const std::wstring text = item.second.StringOrEmpty();
        if (!Trim(text).empty()) return text;
    }
    return std::wstring();
}

bool IsYes(const std::wstring& value)
{
    return EqualsIgnoreCase(value, L"yes");
}

bool IsYesOrPossible(const std::wstring& value)
{
    return IsYes(value) || EqualsIgnoreCase(value, L"possible");
}

CatalogSecurity ParseSecurity(const Value* value)
{
    CatalogSecurity security;
    security.NetworkAccess = GetStringValue(value, L"networkAccess");
    security.ExternalProcesses = GetStringValue(value, L"externalProcesses");
    security.ScriptExecution = GetStringValue(value, L"scriptExecution");
    security.ActiveWebContent = GetStringValue(value, L"activeWebContent");
    security.Elevation = GetStringValue(value, L"elevation");
    return security;
}

PackageKind GetPackageKind(const std::wstring& packageType)
{
    if (EqualsIgnoreCase(packageType, L"plugin")) return PackageKind::Plugin;
    if (EqualsIgnoreCase(packageType, L"extension")) return PackageKind::Extension;
    return PackageKind::Unknown;
}

bool IsHexSha256(const std::wstring& value)
{
    if (value.size() != 64) return false;
    for (wchar_t c : value)
    {
        if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F'))) return false;
    }
    return true;
}

std::vector<CatalogSource> DefaultSources()
{
    return {{kStablePluginSource, true}, {kStableExtensionSource, true},
            {kUnofficialPluginSource, true}, {kExtensionRuntimeSource, true}};
}

bool ReplaceSourceUrl(std::vector<CatalogSource>& sources, const std::wstring& oldUrl, const std::wstring& newUrl)
{
    bool changed = false;
    for (CatalogSource& source : sources)
    {
        if (EqualsIgnoreCase(source.Url, oldUrl))
        {
            source.Url = newUrl;
            changed = true;
        }
    }
    return changed;
}

bool NormalizeSources(std::vector<CatalogSource>& sources)
{
    bool changed = false;
    changed |= ReplaceSourceUrl(sources, L"https://krtkovo-eu-ai.github.io/salamander/catalogs/plugins-stable.json", kStablePluginSource);
    changed |= ReplaceSourceUrl(sources, L"https://krtkovo-eu-ai.github.io/salamander/catalogs/plugins-unofficial.json", kUnofficialPluginSource);
    changed |= ReplaceSourceUrl(sources, L"https://samandarin.net/catalogs/plugins-stable.json", kStablePluginSource);
    changed |= ReplaceSourceUrl(sources, L"https://samandarin.net/catalogs/extensions-stable.json", kStableExtensionSource);
    changed |= ReplaceSourceUrl(sources, L"https://samandarin.net/catalogs/plugins-unofficial.json", kUnofficialPluginSource);
    changed |= ReplaceSourceUrl(sources, L"https://samandarin.net/catalogs/extension-runtimes.json", kExtensionRuntimeSource);
    for (const CatalogSource& defaults : DefaultSources())
    {
        if (std::none_of(sources.begin(), sources.end(), [&](const CatalogSource& source)
                         { return EqualsIgnoreCase(source.Url, defaults.Url); }))
        {
            sources.push_back(defaults);
            changed = true;
        }
    }
    std::set<std::wstring> seen;
    for (size_t index = 0; index < sources.size();)
    {
        if (!seen.insert(LowerAscii(Trim(sources[index].Url))).second)
        {
            sources.erase(sources.begin() + index);
            changed = true;
        }
        else
        {
            ++index;
        }
    }
    return changed;
}

bool SaveSettings(NativeUpdateSettings& settings)
{
    return Samandarin_SaveSettings(&settings) != FALSE;
}

std::wstring ResolvePluginPath(const std::wstring& value)
{
    const std::wstring trimmed = Trim(value);
    if (trimmed.empty()) return std::wstring();
    if ((trimmed.size() >= 3 && trimmed[1] == L':' && (trimmed[2] == L'\\' || trimmed[2] == L'/')) ||
        trimmed.rfind(L"\\\\", 0) == 0)
    {
        return FullPath(trimmed);
    }
    return FullPath(CombinePath(CombinePath(GetExecutableDirectory(), L"plugins"), trimmed));
}

std::wstring AddCurrentPlatform(const std::wstring& version)
{
    std::wstring trimmed = Trim(version);
    const std::wstring lower = LowerAscii(trimmed);
    if (lower.size() >= 5 && (lower.substr(lower.size() - 5) == L"(x86)" ||
                              lower.substr(lower.size() - 5) == L"(x64)")) return trimmed;
#ifdef _WIN64
    const wchar_t* suffix = L" (x64)";
#else
    const wchar_t* suffix = L" (x86)";
#endif
    return trimmed.empty() ? trimmed : trimmed + suffix;
}

std::wstring RemovePlatformSuffix(std::wstring version)
{
    version = Trim(version);
    const std::wstring lower = LowerAscii(version);
    for (const wchar_t* suffix : {L" (x86)", L" (x64)"})
    {
        const size_t length = wcslen(suffix);
        if (lower.size() >= length && lower.compare(lower.size() - length, length, suffix) == 0)
        {
            version.resize(version.size() - length);
            break;
        }
    }
    return version;
}

bool ParseUnsigned(const std::wstring& value, ULONGLONG* result)
{
    if (value.empty() || result == nullptr) return false;
    ULONGLONG parsed = 0;
    for (wchar_t c : value)
    {
        if (c < L'0' || c > L'9') return false;
        const unsigned digit = static_cast<unsigned>(c - L'0');
        if (parsed > ((std::numeric_limits<ULONGLONG>::max)() - digit) / 10) return false;
        parsed = parsed * 10 + digit;
    }
    *result = parsed;
    return true;
}

bool TryCompareDecimal(const std::wstring& left, const std::wstring& right, int* result)
{
    const size_t leftDot = left.find(L'.');
    const size_t rightDot = right.find(L'.');
    if (leftDot == std::wstring::npos || rightDot == std::wstring::npos ||
        left.find(L'.', leftDot + 1) != std::wstring::npos ||
        right.find(L'.', rightDot + 1) != std::wstring::npos) return false;
    const std::wstring leftMajor = left.substr(0, leftDot);
    const std::wstring rightMajor = right.substr(0, rightDot);
    std::wstring leftMinor = left.substr(leftDot + 1);
    std::wstring rightMinor = right.substr(rightDot + 1);
    if (leftMajor.empty() || rightMajor.empty() || leftMinor.empty() || rightMinor.empty()) return false;
    const auto isDigits = [](const std::wstring& value)
    {
        return std::all_of(value.begin(), value.end(), [](wchar_t c) { return c >= L'0' && c <= L'9'; });
    };
    if (!isDigits(leftMajor) || !isDigits(rightMajor) || !isDigits(leftMinor) || !isDigits(rightMinor)) return false;
    const auto compareDigits = [](std::wstring a, std::wstring b) -> int
    {
        while (a.size() > 1 && a[0] == L'0') a.erase(a.begin());
        while (b.size() > 1 && b[0] == L'0') b.erase(b.begin());
        if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
        const int comparison = a.compare(b);
        return comparison < 0 ? -1 : comparison > 0 ? 1 : 0;
    };
    int comparison = compareDigits(leftMajor, rightMajor);
    if (comparison != 0)
    {
        *result = comparison;
        return true;
    }
    while (leftMinor.size() < rightMinor.size()) leftMinor.push_back(L'0');
    while (rightMinor.size() < leftMinor.size()) rightMinor.push_back(L'0');
    *result = compareDigits(leftMinor, rightMinor);
    return true;
}

struct VersionToken
{
    bool Numeric = false;
    ULONGLONG Number = 0;
    std::wstring Text;
};

std::vector<VersionToken> TokenizeVersion(const std::wstring& value)
{
    std::vector<VersionToken> tokens;
    size_t start = 0;
    for (size_t index = 0; index <= value.size(); ++index)
    {
        if (index != value.size() && value[index] != L'.' && value[index] != L'-' && value[index] != L'_') continue;
        if (index > start)
        {
            VersionToken token;
            token.Text = value.substr(start, index - start);
            token.Numeric = ParseUnsigned(token.Text, &token.Number);
            if (!token.Numeric) token.Text = LowerAscii(token.Text);
            tokens.push_back(std::move(token));
        }
        start = index + 1;
    }
    return tokens;
}

int CompareToken(const VersionToken& left, const VersionToken& right)
{
    if (left.Numeric && right.Numeric)
        return left.Number < right.Number ? -1 : left.Number > right.Number ? 1 : 0;
    const std::wstring leftText = left.Numeric ? std::to_wstring(left.Number) : left.Text;
    const std::wstring rightText = right.Numeric ? std::to_wstring(right.Number) : right.Text;
    const int comparison = CompareStringOrdinal(leftText.c_str(), static_cast<int>(leftText.size()),
                                                rightText.c_str(), static_cast<int>(rightText.size()), TRUE);
    return comparison == CSTR_LESS_THAN ? -1 : comparison == CSTR_GREATER_THAN ? 1 : 0;
}

std::wstring GetVersionFromFile(const std::wstring& path)
{
    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(AddLongPathPrefix(path).c_str(), &ignored);
    if (size == 0) return std::wstring();
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(AddLongPathPrefix(path).c_str(), 0, size, data.data())) return std::wstring();
    VS_FIXEDFILEINFO* info = nullptr;
    UINT infoSize = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &infoSize) ||
        info == nullptr || infoSize < sizeof(VS_FIXEDFILEINFO) || info->dwSignature != 0xFEEF04BD)
    {
        return std::wstring();
    }
    wchar_t version[96] = {};
    _snwprintf_s(version, _countof(version), _TRUNCATE, L"%u.%u.%u.%u",
                 HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                 HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS));
    return version;
}

std::wstring GetExtensionPackageDirectory(const std::wstring& entryPoint)
{
    if (entryPoint.empty()) return std::wstring();
    std::wstring current = entryPoint;
    bool directory = false;
    if (!PathExists(current, &directory) || !directory) current = GetParentDirectory(current);
    while (!current.empty())
    {
        if (PathExists(CombinePath(current, L"extension.json"))) return current;
        const std::wstring parent = GetParentDirectory(current);
        if (parent.empty() || EqualsIgnoreCase(parent, current)) break;
        current = parent;
    }
    return std::wstring();
}

std::wstring ResolveApplicationPath(const std::wstring& path)
{
    const std::wstring trimmed = Trim(path);
    if (trimmed.empty()) return std::wstring();
    if ((trimmed.size() >= 3 && trimmed[1] == L':' && (trimmed[2] == L'\\' || trimmed[2] == L'/')) ||
        trimmed.rfind(L"\\\\", 0) == 0)
        return FullPath(trimmed);
    return FullPath(CombinePath(GetExecutableDirectory(), trimmed));
}

bool IsDemoExtensionId(const std::wstring& id)
{
    return StartsWithIgnoreCase(id, L"Salamatrix.Demo.") ||
           EqualsIgnoreCase(id, L"Salamatrix.JScript.ProgressDemo");
}

InstalledPackage ParseExtension(const std::vector<std::wstring>& fields)
{
    InstalledPackage result;
    if (fields.size() < 4) return result;
    const std::wstring manifestId = fields[0];
    result.Name = fields[1].empty() ? manifestId : fields[1];
    result.Version = fields[2];
    const std::wstring entryPoint = ResolveApplicationPath(fields[3]);
    const std::wstring packageDirectory = GetExtensionPackageDirectory(entryPoint);
    if (packageDirectory.empty())
    {
        result.Id = manifestId;
        result.InstallDirectory = GetParentDirectory(entryPoint);
    }
    else
    {
        const std::wstring manifestPath = CombinePath(packageDirectory, L"extension.json");
        std::string manifestText;
        if (ReadUtf8File(manifestPath, &manifestText, 4 * 1024 * 1024))
        {
            Value manifest;
            if (SamandarinJson::ParseUtf8(manifestText, &manifest))
            {
                const std::wstring version = GetStringValue(&manifest, L"version");
                if (!Trim(version).empty()) result.Version = Trim(version);
            }
        }
        if (IsDemoExtensionId(manifestId))
        {
            result.Id = L"salamatrixdemos";
            result.InstallDirectory = CombinePath(GetExecutableDirectory(), L"extensions\\demos");
        }
        else
        {
            result.Id = GetFileName(packageDirectory);
            if (result.Id.empty()) result.Id = manifestId;
            result.InstallDirectory = packageDirectory;
        }
    }
    if (fields.size() > 4) result.IconPath = ResolveApplicationPath(fields[4]);
    result.Kind = PackageKind::Extension;
    result.Version = AddCurrentPlatform(result.Version);
    return result;
}

std::vector<InstalledPackage> ScanInstalledExtensions()
{
    const int required = Samandarin_ExportInstalledExtensions(nullptr, 0);
    if (required <= 1) return {};
    std::vector<wchar_t> buffer(static_cast<size_t>(required), L'\0');
    if (Samandarin_ExportInstalledExtensions(buffer.data(), required) <= 1) return {};
    std::vector<InstalledPackage> raw;
    const std::wstring text(buffer.data());
    size_t lineStart = 0;
    while (lineStart < text.size())
    {
        const size_t lineEnd = text.find_first_of(L"\r\n", lineStart);
        const std::wstring line = text.substr(lineStart, lineEnd == std::wstring::npos ? std::wstring::npos : lineEnd - lineStart);
        if (!line.empty())
        {
            const std::vector<std::wstring> fields = Split(line, L'\t');
            InstalledPackage item = ParseExtension(fields);
            if (!item.Id.empty()) raw.push_back(std::move(item));
        }
        if (lineEnd == std::wstring::npos) break;
        lineStart = lineEnd + 1;
        if (text[lineEnd] == L'\r' && lineStart < text.size() && text[lineStart] == L'\n') ++lineStart;
    }

    std::map<std::wstring, std::vector<InstalledPackage>, SamandarinJson::CaseInsensitiveLess> grouped;
    for (InstalledPackage& item : raw) grouped[item.Id].push_back(std::move(item));
    std::vector<InstalledPackage> collapsed;
    for (auto& group : grouped)
    {
        InstalledPackage item = group.second.front();
        bool sameVersion = true;
        const std::wstring initialVersion = RemovePlatformSuffix(item.Version);
        for (const InstalledPackage& other : group.second)
        {
            if (!EqualsIgnoreCase(initialVersion, RemovePlatformSuffix(other.Version))) sameVersion = false;
            if (item.IconPath.empty() && !other.IconPath.empty()) item.IconPath = other.IconPath;
        }
        if (!sameVersion) item.Version.clear();
        collapsed.push_back(std::move(item));
    }
    return collapsed;
}

std::vector<InstalledPackage> ScanInstalledPlugins()
{
    const int required = Samandarin_ExportInstalledPlugins(nullptr, 0);
    if (required <= 1) return {};
    std::vector<char> buffer(static_cast<size_t>(required), '\0');
    if (Samandarin_ExportInstalledPlugins(buffer.data(), required) <= 1) return {};
    const std::string text(buffer.data());
    std::vector<InstalledPackage> result;
    size_t lineStart = 0;
    while (lineStart < text.size())
    {
        const size_t lineEnd = text.find_first_of("\r\n", lineStart);
        const std::string line = text.substr(lineStart, lineEnd == std::string::npos ? std::string::npos : lineEnd - lineStart);
        if (!line.empty())
        {
            std::vector<std::string> fields;
            size_t fieldStart = 0;
            for (;;)
            {
                const size_t tab = line.find('\t', fieldStart);
                fields.push_back(line.substr(fieldStart, tab == std::string::npos ? std::string::npos : tab - fieldStart));
                if (tab == std::string::npos) break;
                fieldStart = tab + 1;
            }
            if (fields.size() >= 3 && !fields[0].empty())
            {
                InstalledPackage item;
                const std::wstring registeredPath = AnsiOrUtf8ToWide(fields[0]);
                item.Name = AnsiOrUtf8ToWide(fields[1]);
                item.Version = AnsiOrUtf8ToWide(fields[2]);
                item.Id = GetFileNameWithoutExtension(registeredPath);
                if (item.Name.empty()) item.Name = item.Id;
                const std::wstring modulePath = ResolvePluginPath(registeredPath);
                if (item.Version.empty()) item.Version = GetVersionFromFile(modulePath);
                item.Version = AddCurrentPlatform(item.Version);
                item.IconPath = modulePath;
                item.InstallDirectory = GetParentDirectory(modulePath);
                item.Kind = PackageKind::Plugin;
                if (!item.Id.empty()) result.push_back(std::move(item));
            }
        }
        if (lineEnd == std::string::npos) break;
        lineStart = lineEnd + 1;
        if (text[lineEnd] == '\r' && lineStart < text.size() && text[lineStart] == '\n') ++lineStart;
    }
    return result;
}

bool IsTotalCommanderProxyInstance(const InstalledPackage& plugin)
{
    const std::wstring prefix = L"Total Commander Proxy (";
    if (!StartsWithIgnoreCase(plugin.Name, prefix) || plugin.Name.size() <= prefix.size() + 1 || plugin.Name.back() != L')') return false;
    const size_t dot = plugin.Name.find_last_of(L'.');
    if (dot == std::wstring::npos || dot + 1 >= plugin.Name.size() - 1) return false;
    const std::wstring extension = LowerAscii(plugin.Name.substr(dot + 1, plugin.Name.size() - dot - 2));
    for (const wchar_t* base : {L"wcx", L"wfx", L"wlx", L"wdx"})
    {
        if (extension == base || extension == std::wstring(base) + L"6" || extension == std::wstring(base) + L"64") return true;
    }
    return false;
}

bool IsProxyCatalogEntry(const CatalogEntry& entry)
{
    return EqualsIgnoreCase(entry.Id, L"x-tc-proxy");
}

bool IsCatalogMatch(const CatalogEntry& entry, const InstalledPackage& plugin)
{
    if (entry.Kind != PackageKind::Unknown && plugin.Kind != PackageKind::Unknown && entry.Kind != plugin.Kind) return false;
    if (EqualsIgnoreCase(entry.Id, plugin.Id)) return true;
    return IsProxyCatalogEntry(entry) && IsTotalCommanderProxyInstance(plugin);
}

bool ParseCatalogEntry(const Value& value, const std::wstring& catalogName,
                       const std::wstring& sourceUrl, CatalogEntry* entry)
{
    if (entry == nullptr || value.Kind != Value::Type::Object) return false;
    entry->Id = Trim(GetStringValue(&value, L"id"));
    if (entry->Id.empty()) return false;
    entry->PackageType = GetStringValue(&value, L"packageType");
    entry->Kind = GetPackageKind(entry->PackageType);
    entry->Name = ResolveLocalizedText(value.Find(L"name"));
    entry->Author = GetStringValue(&value, L"author");
    entry->Description = ResolveLocalizedText(value.Find(L"description"));
    entry->LatestVersion = GetStringValue(&value, L"latestVersion");
    entry->VersionScheme = GetStringValue(&value, L"versionScheme");
    entry->Homepage = GetStringValue(&value, L"homepageUrl");
    entry->DownloadPage = GetStringValue(&value, L"downloadPageUrl");
    entry->PackageSha256 = GetStringValue(&value, L"packageSha256");
    entry->Icon = GetStringValue(&value, L"icon");
    entry->Source = catalogName.empty() ? sourceUrl : catalogName;
    entry->SourceUrl = sourceUrl;
    entry->Security = ParseSecurity(value.Find(L"security"));
    const Value* dependencies = value.Find(L"dependencies");
    if (dependencies != nullptr && dependencies->Kind == Value::Type::Array)
    {
        std::set<std::wstring> seen;
        for (const Value& dependency : dependencies->Array)
        {
            const std::wstring id = Trim(dependency.StringOrEmpty());
            if (!id.empty() && seen.insert(LowerAscii(id)).second) entry->Dependencies.push_back(id);
        }
    }
    return true;
}

bool FetchCatalogSource(const CatalogSource& source, std::vector<CatalogEntry>* entries, std::wstring* error,
                       volatile LONG* cancelFlag)
{
    std::string json;
    if (IsAbsoluteHttpUrl(source.Url))
    {
        HttpResponse response;
        if (!HttpGet(source.Url, 16 * 1024 * 1024, &response, error, cancelFlag, true, false)) return false;
        json = std::move(response.Body);
    }
    else if (!ReadUtf8File(source.Url, &json, 16 * 1024 * 1024))
    {
        if (error != nullptr) *error = L"The catalog file could not be read.";
        return false;
    }
    Value manifest;
    std::wstring parseError;
    if (!SamandarinJson::ParseUtf8(json, &manifest, &parseError))
    {
        if (error != nullptr) *error = parseError;
        return false;
    }
    const std::wstring catalogName = GetStringValue(&manifest, L"catalogName");
    const Value* plugins = manifest.Find(L"plugins");
    if (plugins == nullptr || plugins->Kind != Value::Type::Array)
    {
        if (error != nullptr) *error = L"The catalog does not contain a plugins array.";
        return false;
    }
    for (const Value& item : plugins->Array)
    {
        CatalogEntry entry;
        if (ParseCatalogEntry(item, catalogName, source.Url, &entry)) entries->push_back(std::move(entry));
    }
    return true;
}

PluginUpdateRow BuildNotInCatalogRow(const InstalledPackage& package)
{
    PluginUpdateRow row;
    row.Name = package.Name;
    row.InstalledVersion = package.Version;
    row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_NOT_IN_CATALOG);
    row.Status = PluginUpdateStatus::NotInCatalog;
    row.Id = package.Id;
    row.InstalledKind = package.Kind;
    row.InstallDirectory = package.InstallDirectory;
    return row;
}

PluginUpdateRow BuildCatalogRow(const CatalogEntry& entry, const InstalledPackage* package)
{
    PluginUpdateRow row;
    row.Name = entry.Name.empty() ? entry.Id : entry.Name;
    row.InstalledVersion = package == nullptr ? std::wstring() : package->Version;
    row.LatestVersion = entry.LatestVersion;
    row.Author = entry.Author;
    row.Homepage = entry.Homepage;
    row.Status = PluginUpdateStatus::Other;
    row.Source = entry.Source;
    row.WebUrl = entry.DownloadPage.empty() ? entry.Homepage : entry.DownloadPage;
    row.Description = entry.Description;
    row.CatalogSourceUrl = entry.SourceUrl;
    row.Id = entry.Id;
    row.InstalledKind = package == nullptr ? PackageKind::Unknown : package->Kind;
    row.CatalogKind = entry.Kind;
    row.InstallDirectory = package == nullptr ? std::wstring() : package->InstallDirectory;
    row.PackageSha256 = entry.PackageSha256;
    row.Dependencies = entry.Dependencies;
    row.Security = entry.Security;
    if (package == nullptr)
    {
        row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_NOT_INSTALLED);
        return row;
    }
    if (IsProxyCatalogEntry(entry) && IsTotalCommanderProxyInstance(*package)) row.Name = package->Name;
    else if (!entry.Name.empty()) row.Name = entry.Name;
    const int comparison = CompareVersions(RemovePlatformSuffix(package->Version), RemovePlatformSuffix(entry.LatestVersion));
    if (package->Version.empty() || entry.LatestVersion.empty())
    {
        row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_UNKNOWN_VERSION);
    }
    else if (comparison == 0)
    {
        row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_CURRENT);
    }
    else if (comparison < 0)
    {
        row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_UPDATE);
        row.Status = PluginUpdateStatus::UpdateAvailable;
    }
    else
    {
        row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_DIFFERENT);
    }
    return row;
}

PluginUpdateRow BuildCatalogErrorRow(const std::wstring& source, const std::wstring& error)
{
    PluginUpdateRow row;
    row.Name = source;
    row.Source = source;
    row.Status = PluginUpdateStatus::CatalogError;
    row.StatusText = GetLocalizedString(IDS_PLUGIN_STATUS_CATALOG_ERROR);
    (void)error;
    return row;
}

bool LoadSourcesFromSettings(std::vector<CatalogSource>* sources, NativeUpdateSettings* settings)
{
    if (sources == nullptr || settings == nullptr) return false;
    if (!Samandarin_LoadSettings(settings)) return false;
    const std::wstring saved = AnsiOrUtf8ToWide(settings->PluginCatalogSources);
    if (Trim(saved).empty())
    {
        *sources = DefaultSources();
        return true;
    }
    std::set<std::wstring> seen;
    for (const std::wstring& raw : Split(saved, L'\n'))
    {
        const std::wstring line = Trim(raw);
        if (line.empty()) continue;
        const size_t pipe = line.find(L'|');
        CatalogSource source;
        if (pipe == std::wstring::npos)
        {
            source.Enabled = true;
            source.Url = line;
        }
        else
        {
            source.Enabled = Trim(line.substr(0, pipe)) == L"1";
            source.Url = Trim(line.substr(pipe + 1));
        }
        const std::wstring unique = LowerAscii(source.Url);
        if (!source.Url.empty() && seen.insert(unique).second) sources->push_back(std::move(source));
    }
    if (sources->empty()) *sources = DefaultSources();
    return true;
}
} // namespace

bool CatalogSecurity::IsHigherRisk() const
{
    return IsYesOrPossible(NetworkAccess) || IsYes(ScriptExecution) ||
           IsYes(ActiveWebContent) || IsYes(ExternalProcesses);
}

std::wstring GetLocalizedString(int resourceId)
{
    std::vector<wchar_t> buffer(4096, L'\0');
    const int length = HLanguage != nullptr ? LoadStringW(HLanguage, resourceId, buffer.data(), static_cast<int>(buffer.size())) : 0;
    if (length <= 0) return std::to_wstring(resourceId);
    return std::wstring(buffer.data(), static_cast<size_t>(length));
}

std::wstring FormatLocalizedString(int resourceId, const std::vector<std::wstring>& arguments)
{
    std::wstring result = GetLocalizedString(resourceId);
    for (size_t index = 0; index < arguments.size(); ++index)
    {
        const std::wstring marker = L"{" + std::to_wstring(index) + L"}";
        size_t at = 0;
        while ((at = result.find(marker, at)) != std::wstring::npos)
        {
            result.replace(at, marker.size(), arguments[index]);
            at += arguments[index].size();
        }
    }
    return result;
}

std::vector<CatalogSource> LoadCatalogSources()
{
    std::vector<CatalogSource> sources;
    NativeUpdateSettings settings = {};
    if (!LoadSourcesFromSettings(&sources, &settings)) return DefaultSources();
    if (NormalizeSources(sources))
    {
        std::wstring serialized;
        for (const CatalogSource& source : sources)
        {
            if (!serialized.empty()) serialized.push_back(L'\n');
            serialized += source.Enabled ? L"1|" : L"0|";
            serialized += Trim(source.Url);
        }
        const std::string encoded = WideToAnsi(serialized);
        memset(settings.PluginCatalogSources, 0, sizeof(settings.PluginCatalogSources));
        if (!encoded.empty())
        {
            const size_t copy = (std::min)(encoded.size(), sizeof(settings.PluginCatalogSources) - 1);
            memcpy(settings.PluginCatalogSources, encoded.data(), copy);
        }
        SaveSettings(settings);
    }
    return sources;
}

bool SaveCatalogSources(const std::vector<CatalogSource>& input)
{
    NativeUpdateSettings settings = {};
    if (!Samandarin_LoadSettings(&settings)) return false;
    std::vector<CatalogSource> sources;
    std::set<std::wstring> seen;
    for (const CatalogSource& source : input)
    {
        const std::wstring url = Trim(source.Url);
        if (url.empty() || !seen.insert(LowerAscii(url)).second) continue;
        sources.push_back({url, source.Enabled});
    }
    NormalizeSources(sources);
    std::wstring serialized;
    for (const CatalogSource& source : sources)
    {
        if (!serialized.empty()) serialized.push_back(L'\n');
        serialized += source.Enabled ? L"1|" : L"0|";
        serialized += source.Url;
    }
    const std::string encoded = WideToAnsi(serialized);
    if (encoded.size() >= sizeof(settings.PluginCatalogSources)) return false;
    memset(settings.PluginCatalogSources, 0, sizeof(settings.PluginCatalogSources));
    if (!encoded.empty()) memcpy(settings.PluginCatalogSources, encoded.data(), encoded.size());
    return SaveSettings(settings);
}

bool IsTrustedInstallCatalogUrl(const std::wstring& url)
{
    const std::wstring prefix = L"https://samandarin.krtkovo.eu/catalogs/";
    if (url.size() <= prefix.size() ||
        CompareStringOrdinal(url.c_str(), static_cast<int>(prefix.size()),
                             prefix.c_str(), static_cast<int>(prefix.size()), TRUE) != CSTR_EQUAL)
        return false;
    const std::wstring file = url.substr(prefix.size());
    if (file.find_first_of(L"/?#%\\:") != std::wstring::npos || file == L"." || file == L"..") return false;
    return file.size() > 5 && EqualsIgnoreCase(file.substr(file.size() - 5), L".json");
}

int CompareVersions(const std::wstring& inputLeft, const std::wstring& inputRight)
{
    if (Trim(inputLeft).empty() && Trim(inputRight).empty()) return 0;
    if (Trim(inputLeft).empty()) return -1;
    if (Trim(inputRight).empty()) return 1;
    const std::wstring left = Trim(inputLeft);
    const std::wstring right = Trim(inputRight);
    if (EqualsIgnoreCase(left, right)) return 0;
    int decimal = 0;
    if (TryCompareDecimal(left, right, &decimal)) return decimal;
    const std::vector<VersionToken> leftTokens = TokenizeVersion(left);
    const std::vector<VersionToken> rightTokens = TokenizeVersion(right);
    const size_t count = (std::max)(leftTokens.size(), rightTokens.size());
    const VersionToken zero{true, 0, std::wstring()};
    for (size_t index = 0; index < count; ++index)
    {
        const VersionToken& a = index < leftTokens.size() ? leftTokens[index] : zero;
        const VersionToken& b = index < rightTokens.size() ? rightTokens[index] : zero;
        const int comparison = CompareToken(a, b);
        if (comparison != 0) return comparison;
    }
    return 0;
}

bool CanInstallPluginRow(const PluginUpdateRow& row)
{
    if (!IsHexSha256(row.PackageSha256) || row.WebUrl.empty() ||
        !IsTrustedInstallCatalogUrl(row.CatalogSourceUrl)) return false;
    const std::wstring prefix = L"https://github.com/KRtkovo-eu-AI/salamander-plugins/releases/download/";
    if (row.WebUrl.size() <= prefix.size() || row.WebUrl.compare(0, prefix.size(), prefix) != 0) return false;
    const size_t query = row.WebUrl.find_first_of(L"?#");
    const std::wstring path = row.WebUrl.substr(0, query);
    if (path.size() < 4 || !EqualsIgnoreCase(path.substr(path.size() - 3), L".7z")) return false;
    const std::wstring file = GetFileName(path);
    PackageKind packageKind = StartsWithIgnoreCase(file, L"plugin") ? PackageKind::Plugin :
                              StartsWithIgnoreCase(file, L"extension") ? PackageKind::Extension : PackageKind::Unknown;
    if (packageKind == PackageKind::Unknown) return false;
    if (row.CatalogKind != PackageKind::Unknown && row.CatalogKind != packageKind) return false;
    return row.InstalledKind == PackageKind::Unknown || row.InstalledKind == packageKind;
}

bool FetchPluginCatalog(std::vector<PluginUpdateRow>* rows,
                        std::vector<std::wstring>* errors,
                        volatile LONG* cancelFlag)
{
    if (rows == nullptr) return false;
    rows->clear();
    if (errors != nullptr) errors->clear();
    std::vector<InstalledPackage> installed = ScanInstalledPlugins();
    std::vector<InstalledPackage> extensions = ScanInstalledExtensions();
    installed.insert(installed.end(), std::make_move_iterator(extensions.begin()), std::make_move_iterator(extensions.end()));

    const std::vector<CatalogSource> sources = LoadCatalogSources();
    std::vector<CatalogEntry> catalog;
    std::vector<std::wstring> sourceErrors;
    for (const CatalogSource& source : sources)
    {
        if (!source.Enabled) continue;
        std::wstring error;
        if (!FetchCatalogSource(source, &catalog, &error, cancelFlag))
        {
            if (errors != nullptr) errors->push_back(source.Url + L": " + error);
            sourceErrors.push_back(source.Url + L": " + error);
        }
        if (cancelFlag != nullptr && InterlockedCompareExchange(const_cast<LONG*>(cancelFlag), 0, 0) != 0) return false;
    }

    for (const InstalledPackage& package : installed)
    {
        bool matched = false;
        for (const CatalogEntry& entry : catalog)
        {
            if (!IsCatalogMatch(entry, package)) continue;
            rows->push_back(BuildCatalogRow(entry, &package));
            matched = true;
        }
        if (!matched) rows->push_back(BuildNotInCatalogRow(package));
    }
    for (const CatalogEntry& entry : catalog)
    {
        const bool installedMatch = std::any_of(installed.begin(), installed.end(),
                                                [&](const InstalledPackage& package)
                                                { return IsCatalogMatch(entry, package); });
        if (!installedMatch) rows->push_back(BuildCatalogRow(entry, nullptr));
    }
    for (const std::wstring& sourceError : sourceErrors)
    {
        const size_t separator = sourceError.find(L": ");
        rows->push_back(BuildCatalogErrorRow(sourceError.substr(0, separator),
                                             separator == std::wstring::npos ? sourceError : sourceError.substr(separator + 2)));
    }
    std::stable_sort(rows->begin(), rows->end(), [](const PluginUpdateRow& left, const PluginUpdateRow& right)
                     { return CompareStringOrdinal(left.Name.c_str(), static_cast<int>(left.Name.size()),
                                                   right.Name.c_str(), static_cast<int>(right.Name.size()), TRUE) == CSTR_LESS_THAN; });
    return true;
}
} // namespace SamandarinNative
