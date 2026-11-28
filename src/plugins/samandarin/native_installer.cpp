// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Revalidate the official catalog and re-extract the hashed archive after elevation. | Do not trust mutable temporary metadata or unsigned package scripts. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Verify elevated installs from a protected package-root staging directory. | Prevent tampered temporary files and reparse paths from bypassing signature checks. | | openrouter/openai/gpt-6-luna-pro |
// | 2026-11-28 | Port official package staging, receipts, and post-exit installation to native C++. | Remove SharpCompress, Authenticode interop, and PowerShell from the updater. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_installer.h"
#include "native_archive.h"
#include "native_http.h"
#include "native_install_security.h"
#include "native_json.h"
#include "native_utils.h"

#include <algorithm>
#include <limits>
#include <set>
#include <shellapi.h>
#include <shlobj.h>
#include <wincrypt.h>
#include <vector>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "crypt32.lib")

namespace SamandarinNative
{
namespace
{
using SamandarinJson::Value;
constexpr size_t kMaximumDownloadBytes = 256ull * 1024ull * 1024ull;
constexpr size_t kMaximumMetadataBytes = 2ull * 1024ull * 1024ull;
constexpr DWORD kInstallerWaitMs = 30 * 60 * 1000;
const wchar_t* const kReceiptFileName = L"plugin-receipts.json";

struct TreeEntry
{
    std::wstring Path;
    std::wstring RelativePath;
    bool IsDirectory = false;
};

void SetError(std::wstring* error, const std::wstring& text)
{
    if (error != nullptr) *error = text;
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

bool IsHexSha256(const std::wstring& value)
{
    if (value.size() != 64) return false;
    for (wchar_t c : value)
    {
        if (!((c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') || (c >= L'A' && c <= L'F'))) return false;
    }
    return true;
}

std::wstring Lower(std::wstring value)
{
    for (wchar_t& c : value)
    {
        if (c >= L'A' && c <= L'Z') c = static_cast<wchar_t>(c - L'A' + L'a');
    }
    return value;
}

std::wstring Trim(std::wstring value)
{
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return std::wstring();
    return value.substr(first, value.find_last_not_of(L" \t\r\n") - first + 1);
}

Value MakeString(const std::wstring& text)
{
    Value value;
    value.Kind = Value::Type::String;
    value.Text = text;
    return value;
}

Value MakeNumber(ULONGLONG number)
{
    Value value;
    value.Kind = Value::Type::Number;
    value.Text = std::to_wstring(number);
    return value;
}

Value MakeNull()
{
    return Value();
}

Value MakeObject()
{
    Value value;
    value.Kind = Value::Type::Object;
    return value;
}

Value MakeArray()
{
    Value value;
    value.Kind = Value::Type::Array;
    return value;
}

std::wstring ReadString(const Value& object, const wchar_t* key)
{
    const Value* value = object.Find(key);
    return value == nullptr ? std::wstring() : value->StringOrEmpty();
}

bool ReadNumber(const Value& object, const wchar_t* key, ULONGLONG* number)
{
    if (number == nullptr) return false;
    const std::wstring value = ReadString(object, key);
    if (value.empty()) return false;
    ULONGLONG result = 0;
    for (wchar_t c : value)
    {
        if (c < L'0' || c > L'9') return false;
        const unsigned digit = static_cast<unsigned>(c - L'0');
        if (result > ((std::numeric_limits<ULONGLONG>::max)() - digit) / 10) return false;
        result = result * 10 + digit;
    }
    *number = result;
    return true;
}

bool VerifyCatalogPackageMetadata(const Value& metadata, PackageKind kind, std::wstring* error)
{
    const std::wstring catalogUrl = ReadString(metadata, L"catalogSourceUrl");
    if (!IsTrustedInstallCatalogUrl(catalogUrl))
    {
        SetError(error, L"The package catalog is not a trusted HTTPS source.");
        return false;
    }
    HttpResponse response;
    if (!HttpGet(catalogUrl, 16 * 1024 * 1024, &response, error, nullptr, true, true) ||
        !IsTrustedInstallCatalogUrl(response.FinalUrl))
    {
        SetError(error, L"The trusted package catalog could not be verified.");
        return false;
    }
    Value manifest;
    if (!SamandarinJson::ParseUtf8(response.Body, &manifest) || manifest.Kind != Value::Type::Object)
    {
        SetError(error, L"The trusted package catalog is invalid.");
        return false;
    }
    const Value* packages = manifest.Find(L"plugins");
    if (packages == nullptr || packages->Kind != Value::Type::Array)
    {
        SetError(error, L"The trusted package catalog does not contain package entries.");
        return false;
    }
    const std::wstring id = ReadString(metadata, L"id");
    const std::wstring packageType = kind == PackageKind::Extension ? L"extension" : L"plugin";
    const std::wstring downloadUrl = ReadString(metadata, L"sourceUrl");
    const std::wstring packageHash = ReadString(metadata, L"packageSha256");
    const std::wstring version = ReadString(metadata, L"version");
    PluginUpdateRow installPolicy;
    installPolicy.WebUrl = downloadUrl;
    installPolicy.PackageSha256 = packageHash;
    installPolicy.CatalogSourceUrl = catalogUrl;
    installPolicy.CatalogKind = kind;
    if (!CanInstallPluginRow(installPolicy))
    {
        SetError(error, L"The verified catalog entry is not an allowed official package.");
        return false;
    }
    for (const Value& item : packages->Array)
    {
        if (!EqualsIgnoreCase(ReadString(item, L"id"), id) ||
            !EqualsIgnoreCase(ReadString(item, L"packageType"), packageType) ||
            !EqualsIgnoreCase(ReadString(item, L"downloadPageUrl"), downloadUrl) ||
            !EqualsIgnoreCase(ReadString(item, L"packageSha256"), packageHash) ||
            !EqualsIgnoreCase(ReadString(item, L"latestVersion"), version))
            continue;
        return true;
    }
    SetError(error, L"The package is no longer listed with the staged URL and digest.");
    return false;
}

void Put(Value* object, const wchar_t* key, Value value)
{
    if (object != nullptr && object->Kind == Value::Type::Object) object->Object[key] = std::move(value);
}

std::wstring GetUniqueToken()
{
    GUID guid = {};
    if (FAILED(CoCreateGuid(&guid))) return std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    wchar_t text[40] = {};
    if (StringFromGUID2(guid, text, _countof(text)) <= 0) return std::to_wstring(GetTickCount64());
    std::wstring token = text;
    token.erase(std::remove(token.begin(), token.end(), L'{'), token.end());
    token.erase(std::remove(token.begin(), token.end(), L'}'), token.end());
    token.erase(std::remove(token.begin(), token.end(), L'-'), token.end());
    return token;
}

std::wstring GetUtcTimestamp()
{
    SYSTEMTIME now = {};
    GetSystemTime(&now);
    wchar_t text[40] = {};
    _snwprintf_s(text, _countof(text), _TRUNCATE, L"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
                 now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds);
    return text;
}

std::wstring GetTempDirectory()
{
    DWORD capacity = 256;
    for (;;)
    {
        std::vector<wchar_t> buffer(capacity, L'\0');
        const DWORD length = GetTempPathW(capacity, buffer.data());
        if (length == 0) return std::wstring();
        if (length < capacity) return std::wstring(buffer.data(), length);
        if (length >= 32768) return std::wstring();
        capacity = length + 1;
    }
}

std::wstring GetUpdateRoot()
{
    std::wstring root = GetTempDirectory();
    if (root.empty()) return root;
    root = CombinePath(root, L"OpenSamandarin");
    root = CombinePath(root, L"Samandarin");
    return CombinePath(root, L"Updates");
}

bool CreateUniqueStageDirectory(std::wstring* path)
{
    const std::wstring root = GetUpdateRoot();
    if (root.empty() || !CreateDirectories(root)) return false;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const std::wstring candidate = CombinePath(root, GetUniqueToken());
        if (CreateDirectoryW(AddLongPathPrefix(candidate).c_str(), nullptr))
        {
            *path = candidate;
            return true;
        }
        if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    return false;
}

bool CreateUniqueInstallDirectory(const std::wstring& packageRoot, std::wstring* path)
{
    if (path == nullptr || !CreateDirectories(packageRoot)) return false;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const std::wstring candidate = CombinePath(packageRoot, L".samandarin-install-" + GetUniqueToken());
        if (CreateDirectoryW(AddLongPathPrefix(candidate).c_str(), nullptr))
        {
            *path = candidate;
            return true;
        }
        if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    return false;
}

bool EnumerateTree(const std::wstring& root, std::vector<TreeEntry>* entries,
                   bool rejectReparse, std::wstring* error)
{
    if (entries == nullptr) return false;
    entries->clear();
    std::vector<std::wstring> directories{root};
    const std::wstring normalizedRoot = FullPath(root);
    if (normalizedRoot.empty())
    {
        SetError(error, L"The package directory path is invalid.");
        return false;
    }
    const DWORD rootAttributes = GetFileAttributesW(AddLongPathPrefix(normalizedRoot).c_str());
    if (rootAttributes == INVALID_FILE_ATTRIBUTES ||
        (rejectReparse && (rootAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0))
    {
        SetError(error, L"The package directory is missing or contains a reparse point.");
        return false;
    }
    while (!directories.empty())
    {
        const std::wstring directory = std::move(directories.back());
        directories.pop_back();
        WIN32_FIND_DATAW data = {};
        HANDLE find = FindFirstFileW(AddLongPathPrefix(CombinePath(directory, L"*")).c_str(), &data);
        if (find == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) continue;
            SetError(error, L"The package directory could not be scanned.");
            return false;
        }
        do
        {
            if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
            const std::wstring path = CombinePath(directory, data.cFileName);
            const bool isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            const bool reparse = (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (reparse && rejectReparse)
            {
                FindClose(find);
                SetError(error, L"Reparse points are not allowed in update packages.");
                return false;
            }
            if (!IsPathInside(path, normalizedRoot))
            {
                FindClose(find);
                SetError(error, L"The package contains a path outside its staging directory.");
                return false;
            }
            const std::wstring relative = path.substr(normalizedRoot.size() + 1);
            entries->push_back({path, relative, isDirectory});
            if (isDirectory && !reparse) directories.push_back(path);
        } while (FindNextFileW(find, &data));
        const DWORD status = GetLastError();
        FindClose(find);
        if (status != ERROR_NO_MORE_FILES)
        {
            SetError(error, L"The package directory could not be scanned.");
            return false;
        }
    }
    return true;
}

void DeleteTree(const std::wstring& path)
{
    std::vector<TreeEntry> entries;
    if (!EnumerateTree(path, &entries, true, nullptr))
    {
        RemoveDirectoryW(AddLongPathPrefix(path).c_str());
        return;
    }
    std::sort(entries.begin(), entries.end(), [](const TreeEntry& left, const TreeEntry& right)
              { return left.Path.size() > right.Path.size(); });
    for (const TreeEntry& entry : entries)
    {
        if (entry.IsDirectory)
            RemoveDirectoryW(AddLongPathPrefix(entry.Path).c_str());
        else
            DeleteFileW(AddLongPathPrefix(entry.Path).c_str());
    }
    RemoveDirectoryW(AddLongPathPrefix(path).c_str());
}

bool IsInsideProgramFiles(const std::wstring& path)
{
    bool inside = false;
    PWSTR knownFolder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, nullptr, &knownFolder)))
    {
        inside = IsPathInside(path, knownFolder);
        CoTaskMemFree(knownFolder);
    }
    knownFolder = nullptr;
    if (!inside && SUCCEEDED(SHGetKnownFolderPath(FOLDERID_ProgramFilesX86, KF_FLAG_DEFAULT, nullptr, &knownFolder)))
    {
        inside = IsPathInside(path, knownFolder);
        CoTaskMemFree(knownFolder);
    }
    return inside;
}

std::wstring GetSystemDirectory()
{
    UINT capacity = 256;
    for (;;)
    {
        std::vector<wchar_t> buffer(capacity, L'\0');
        const UINT length = GetSystemDirectoryW(buffer.data(), capacity);
        if (length == 0) return std::wstring();
        if (length < capacity) return std::wstring(buffer.data(), length);
        if (length >= 32768) return std::wstring();
        capacity = length + 1;
    }
}

std::string EncodeBase64(const std::string& bytes)
{
    DWORD required = 0;
    if (!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(bytes.data()), static_cast<DWORD>(bytes.size()),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &required))
        return std::string();
    std::string result(required, '\0');
    if (!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(bytes.data()), static_cast<DWORD>(bytes.size()),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &result[0], &required))
        return std::string();
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

bool DecodeBase64(const std::string& text, std::string* bytes)
{
    if (bytes == nullptr || text.empty()) return false;
    DWORD required = 0;
    if (!CryptStringToBinaryA(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64,
                              nullptr, &required, nullptr, nullptr)) return false;
    bytes->resize(required);
    return CryptStringToBinaryA(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64,
                                reinterpret_cast<BYTE*>(&(*bytes)[0]), &required, nullptr, nullptr) != FALSE;
}

bool SaveReceiptStore(const std::wstring& path, Value* store)
{
    if (store == nullptr) return false;
    if (store->Kind != Value::Type::Object) *store = MakeObject();
    Put(store, L"schemaVersion", MakeNumber(1));
    if (store->Find(L"receipts") == nullptr || store->Find(L"receipts")->Kind != Value::Type::Array)
        Put(store, L"receipts", MakeArray());
    const std::string json = SamandarinJson::StringifyUtf8(*store);
    return !json.empty() && WriteUtf8FileAtomic(path, json);
}

Value LoadReceiptStore(const std::wstring& path)
{
    std::string json;
    Value store;
    if (ReadUtf8File(path, &json, 16 * 1024 * 1024) && SamandarinJson::ParseUtf8(json, &store) &&
        store.Kind == Value::Type::Object)
    {
        if (store.Find(L"receipts") == nullptr || store.Find(L"receipts")->Kind != Value::Type::Array)
            Put(&store, L"receipts", MakeArray());
        return store;
    }
    store = MakeObject();
    Put(&store, L"schemaVersion", MakeNumber(1));
    Put(&store, L"lastInstallError", MakeNull());
    Put(&store, L"receipts", MakeArray());
    return store;
}

void UpsertReceipt(Value* store, const Value& receipt)
{
    Value* receipts = store == nullptr ? nullptr : &store->Object[L"receipts"];
    if (receipts == nullptr) return;
    if (receipts->Kind != Value::Type::Array) *receipts = MakeArray();
    const std::wstring id = ReadString(receipt, L"id");
    const std::wstring kind = ReadString(receipt, L"packageType");
    std::vector<Value> updated;
    for (const Value& existing : receipts->Array)
    {
        if (EqualsIgnoreCase(ReadString(existing, L"id"), id) &&
            EqualsIgnoreCase(ReadString(existing, L"packageType"), kind)) continue;
        updated.push_back(existing);
    }
    updated.push_back(receipt);
    receipts->Array = std::move(updated);
}

void SaveInstallFailure(const std::wstring& receiptPath, const std::wstring& error)
{
    if (receiptPath.empty()) return;
    Value store = LoadReceiptStore(receiptPath);
    Put(&store, L"lastInstallError", error.empty() ? MakeNull() : MakeString(error));
    SaveReceiptStore(receiptPath, &store);
}

bool CreateInstallMetadata(const std::wstring& stage, const std::wstring& archive,
                           const std::wstring& source, const std::wstring& target,
                           const std::wstring& packageRoot, const std::wstring& receiptPath,
                           const PluginUpdateRow& row, PackageKind kind,
                           const std::wstring& signer, const std::wstring& archiveHash,
                           const std::wstring& pluginRelativePath, bool appendPluginRecord,
                           const std::vector<std::wstring>& extensionManifestRecords,
                           std::wstring* metadataPath)
{
    Value metadata = MakeObject();
    Put(&metadata, L"schemaVersion", MakeNumber(1));
    Put(&metadata, L"hostProcessId", MakeNumber(GetCurrentProcessId()));
    Put(&metadata, L"hostExecutableDirectory", MakeString(GetExecutableDirectory()));
    Put(&metadata, L"stagingDirectory", MakeString(stage));
    Put(&metadata, L"archivePath", MakeString(archive));
    Put(&metadata, L"sourceDirectory", MakeString(source));
    Put(&metadata, L"targetDirectory", MakeString(target));
    Put(&metadata, L"packageRoot", MakeString(packageRoot));
    Put(&metadata, L"receiptPath", MakeString(receiptPath));
    Put(&metadata, L"id", MakeString(row.Id));
    Put(&metadata, L"packageType", MakeString(kind == PackageKind::Extension ? L"extension" : L"plugin"));
    Put(&metadata, L"sourceUrl", MakeString(row.WebUrl));
    Put(&metadata, L"catalogSourceUrl", MakeString(row.CatalogSourceUrl));
    Put(&metadata, L"packageSha256", MakeString(archiveHash));
    Put(&metadata, L"signer", MakeString(signer));
    Put(&metadata, L"version", MakeString(row.LatestVersion));
    Put(&metadata, L"verifiedAt", MakeString(GetUtcTimestamp()));
    Put(&metadata, L"pluginRelativePath", MakeString(pluginRelativePath));
    Put(&metadata, L"appendPluginRecord", Value{Value::Type::Boolean, appendPluginRecord});
    Value manifests = MakeArray();
    for (const std::wstring& record : extensionManifestRecords) manifests.Array.push_back(MakeString(record));
    Put(&metadata, L"extensionManifests", std::move(manifests));
    *metadataPath = CombinePath(stage, L"install.json");
    const std::string json = SamandarinJson::StringifyUtf8(metadata);
    return !json.empty() && json.size() <= kMaximumMetadataBytes && WriteUtf8FileAtomic(*metadataPath, json);
}

bool FindPluginRelativePath(const std::wstring& extractedRoot, const std::wstring& id,
                            std::wstring* relative, std::wstring* error)
{
    std::vector<TreeEntry> files;
    if (!EnumerateTree(extractedRoot, &files, true, error)) return false;
    std::vector<std::wstring> modules;
    for (const TreeEntry& entry : files)
    {
        if (!entry.IsDirectory && EqualsIgnoreCase(GetFileExtension(entry.Path), L".spl")) modules.push_back(entry.Path);
    }
    const std::wstring rootName = GetFileName(extractedRoot);
    auto selected = std::find_if(modules.begin(), modules.end(), [&](const std::wstring& path)
    {
        return EqualsIgnoreCase(GetFileNameWithoutExtension(path), id) ||
               EqualsIgnoreCase(GetFileNameWithoutExtension(path), rootName);
    });
    if (selected == modules.end())
    {
        if (modules.size() != 1)
        {
            SetError(error, L"The main plugin module cannot be identified in this package.");
            return false;
        }
        selected = modules.begin();
    }
    const std::wstring root = FullPath(extractedRoot);
    const std::wstring full = FullPath(*selected);
    if (!IsPathInside(full, root)) return false;
    *relative = full.substr(root.size() + 1);
    return !relative->empty();
}

bool BuildExtensionManifestRecords(const std::wstring& contentRoot, const std::wstring& targetDirectory,
                                   const std::wstring& packageRoot,
                                   std::vector<std::wstring>* records, std::wstring* error)
{
    std::vector<TreeEntry> files;
    if (!EnumerateTree(contentRoot, &files, true, error)) return false;
    const std::wstring contentPrefix = FullPath(contentRoot);
    const std::wstring packagePrefix = FullPath(packageRoot);
    for (const TreeEntry& entry : files)
    {
        if (entry.IsDirectory || !EqualsIgnoreCase(GetFileName(entry.Path), L"extension.json")) continue;
        const std::wstring manifestFull = FullPath(entry.Path);
        if (!IsPathInside(manifestFull, contentPrefix)) continue;
        const std::wstring relativeToContent = manifestFull.substr(contentPrefix.size() + 1);
        const std::wstring installedPath = FullPath(CombinePath(targetDirectory, relativeToContent));
        if (!IsPathInside(installedPath, packagePrefix))
        {
            SetError(error, L"The extension manifest path is invalid.");
            return false;
        }
        records->push_back(installedPath.substr(packagePrefix.size() + 1));
    }
    std::sort(records->begin(), records->end(), [](const std::wstring& left, const std::wstring& right)
              { return CompareStringOrdinal(left.c_str(), static_cast<int>(left.size()),
                                            right.c_str(), static_cast<int>(right.size()), TRUE) == CSTR_LESS_THAN; });
    return true;
}

bool HasReparseAncestor(const std::wstring& path, const std::wstring& root);

std::wstring ResolveExtensionContentRoot(const std::wstring& extractedRoot)
{
    std::vector<TreeEntry> files;
    if (!EnumerateTree(extractedRoot, &files, true, nullptr)) return extractedRoot;
    std::vector<std::wstring> manifests;
    for (const TreeEntry& entry : files)
    {
        if (!entry.IsDirectory && EqualsIgnoreCase(GetFileName(entry.Path), L"extension.json"))
            manifests.push_back(entry.Path);
    }
    return manifests.size() == 1 ? GetParentDirectory(manifests.front()) : extractedRoot;
}

bool GetProcessImagePath(HANDLE process, std::wstring* path)
{
    if (path == nullptr) return false;
    size_t capacity = 512;
    for (;;)
    {
        std::vector<wchar_t> buffer(capacity, L'\0');
        DWORD length = static_cast<DWORD>(buffer.size());
        if (QueryFullProcessImageNameW(process, 0, buffer.data(), &length))
        {
            path->assign(buffer.data(), length);
            return true;
        }
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || capacity >= 32768) return false;
        capacity = (std::min)(capacity * 2, static_cast<size_t>(32768));
    }
}

bool SafeMetadataPath(const std::wstring& metadataPath, const Value& metadata,
                      const std::wstring& modulePath, std::wstring* hostDirectory,
                      std::wstring* error)
{
    const std::wstring stage = ReadString(metadata, L"stagingDirectory");
    const std::wstring archive = ReadString(metadata, L"archivePath");
    const std::wstring source = ReadString(metadata, L"sourceDirectory");
    const std::wstring target = ReadString(metadata, L"targetDirectory");
    const std::wstring packageRoot = ReadString(metadata, L"packageRoot");
    const std::wstring receipt = ReadString(metadata, L"receiptPath");
    const std::wstring kind = ReadString(metadata, L"packageType");
    const std::wstring expectedHostDirectory = ReadString(metadata, L"hostExecutableDirectory");
    ULONGLONG schemaVersion = 0;
    const std::wstring updateRoot = FullPath(GetUpdateRoot());
    const std::wstring fullStage = FullPath(stage);
    const std::wstring payloadRoot = CombinePath(fullStage, L"payload");
    if (stage.empty() || archive.empty() || source.empty() || target.empty() || packageRoot.empty() ||
        receipt.empty() || expectedHostDirectory.empty() || metadataPath.empty() || updateRoot.empty() ||
        fullStage.empty() || !ReadNumber(metadata, L"schemaVersion", &schemaVersion) || schemaVersion != 1 ||
        (!EqualsIgnoreCase(kind, L"plugin") && !EqualsIgnoreCase(kind, L"extension")))
    {
        SetError(error, L"The installation manifest is incomplete.");
        return false;
    }
    if (!IsPathInside(fullStage, updateRoot) || EqualsIgnoreCase(fullStage, updateRoot) ||
        !EqualsIgnoreCase(FullPath(metadataPath), CombinePath(fullStage, L"install.json")) ||
        !EqualsIgnoreCase(FullPath(archive), CombinePath(fullStage, L"package.7z")) ||
        !IsPathInside(source, payloadRoot) || EqualsIgnoreCase(FullPath(source), FullPath(payloadRoot)) ||
        !IsPathInside(target, packageRoot) || EqualsIgnoreCase(FullPath(target), FullPath(packageRoot)))
    {
        SetError(error, L"The installation manifest contains an unsafe path.");
        return false;
    }
    if (HasReparseAncestor(fullStage, updateRoot) ||
        HasReparseAncestor(metadataPath, fullStage) ||
        HasReparseAncestor(archive, fullStage) ||
        HasReparseAncestor(source, payloadRoot) ||
        HasReparseAncestor(receipt, expectedHostDirectory))
    {
        SetError(error, L"The staging directory contains an unsafe reparse point.");
        return false;
    }
    const std::wstring pluginDirectory = GetParentDirectory(GetParentDirectory(modulePath));
    const std::wstring moduleHostDirectory = GetParentDirectory(pluginDirectory);
    if (!EqualsIgnoreCase(FullPath(moduleHostDirectory), FullPath(expectedHostDirectory)))
    {
        SetError(error, L"The installer module does not belong to the target installation.");
        return false;
    }
    const std::wstring expectedPackageRoot = CombinePath(expectedHostDirectory,
                                                          EqualsIgnoreCase(kind, L"extension") ? L"extensions" : L"plugins");
    if (!EqualsIgnoreCase(FullPath(packageRoot), FullPath(expectedPackageRoot)) ||
        !EqualsIgnoreCase(FullPath(receipt), FullPath(CombinePath(expectedHostDirectory, kReceiptFileName))))
    {
        SetError(error, L"The installation target is not inside the matching plugin directory.");
        return false;
    }
    *hostDirectory = FullPath(expectedHostDirectory);
    return true;
}

bool IsSafeRelativePath(const std::wstring& value)
{
    if (value.empty() || value.size() > 16000 || value.find(L':') != std::wstring::npos ||
        value.front() == L'\\' || value.front() == L'/') return false;
    std::wstring normalized = value;
    std::replace(normalized.begin(), normalized.end(), L'/', L'\\');
    size_t start = 0;
    for (size_t index = 0; index <= normalized.size(); ++index)
    {
        if (index != normalized.size() && normalized[index] != L'\\') continue;
        const std::wstring part = normalized.substr(start, index - start);
        if (part.empty() || part == L"." || part == L"..") return false;
        start = index + 1;
    }
    return true;
}

bool HasReparseAncestor(const std::wstring& path, const std::wstring& root)
{
    const std::wstring fullRoot = FullPath(root);
    const std::wstring fullPath = FullPath(path);
    if (fullRoot.empty() || fullPath.empty() || !IsPathInside(fullPath, fullRoot)) return true;
    const DWORD rootAttributes = GetFileAttributesW(AddLongPathPrefix(fullRoot).c_str());
    if (rootAttributes != INVALID_FILE_ATTRIBUTES && (rootAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) return true;
    if (EqualsIgnoreCase(fullPath, fullRoot)) return false;
    std::wstring current = fullRoot;
    const std::wstring relative = fullPath.substr(fullRoot.size() + 1);
    size_t start = 0;
    for (size_t index = 0; index <= relative.size(); ++index)
    {
        if (index != relative.size() && relative[index] != L'\\' && relative[index] != L'/') continue;
        current = CombinePath(current, relative.substr(start, index - start));
        const DWORD attributes = GetFileAttributesW(AddLongPathPrefix(current).c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) return true;
        start = index + 1;
    }
    return false;
}

bool CopyPackageTree(const std::wstring& source, const std::wstring& target, std::wstring* error)
{
    std::vector<TreeEntry> entries;
    if (!EnumerateTree(source, &entries, true, error)) return false;
    if (!CreateDirectories(target))
    {
        SetError(error, L"The target package directory could not be created.");
        return false;
    }
    std::sort(entries.begin(), entries.end(), [](const TreeEntry& left, const TreeEntry& right)
              { return left.RelativePath.size() < right.RelativePath.size(); });
    for (const TreeEntry& entry : entries)
    {
        const std::wstring destination = CombinePath(target, entry.RelativePath);
        if (!IsPathInside(destination, target) || HasReparseAncestor(destination, target))
        {
            SetError(error, L"The package target contains an unsafe reparse point.");
            return false;
        }
        if (entry.IsDirectory)
        {
            if (!CreateDirectories(destination))
            {
                SetError(error, L"A package directory could not be created.");
                return false;
            }
            continue;
        }
        CreateDirectories(GetParentDirectory(destination));
        const DWORD attributes = GetFileAttributesW(AddLongPathPrefix(destination).c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
            SetFileAttributesW(AddLongPathPrefix(destination).c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
        if (!CopyFileW(AddLongPathPrefix(entry.Path).c_str(), AddLongPathPrefix(destination).c_str(), FALSE))
        {
            SetError(error, L"A package file could not be copied (" + std::to_wstring(GetLastError()) + L").");
            return false;
        }
    }
    return true;
}

void RemoveExtraPackageFiles(const std::wstring& source, const std::wstring& target)
{
    std::vector<TreeEntry> destinationEntries;
    if (!EnumerateTree(target, &destinationEntries, true, nullptr)) return;
    std::sort(destinationEntries.begin(), destinationEntries.end(), [](const TreeEntry& left, const TreeEntry& right)
              { return left.RelativePath.size() > right.RelativePath.size(); });
    for (const TreeEntry& entry : destinationEntries)
    {
        const std::wstring expected = CombinePath(source, entry.RelativePath);
        if (PathExists(expected)) continue;
        if (entry.IsDirectory)
            RemoveDirectoryW(AddLongPathPrefix(entry.Path).c_str());
        else
        {
            const DWORD attributes = GetFileAttributesW(AddLongPathPrefix(entry.Path).c_str());
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_READONLY) != 0)
                SetFileAttributesW(AddLongPathPrefix(entry.Path).c_str(), attributes & ~FILE_ATTRIBUTE_READONLY);
            DeleteFileW(AddLongPathPrefix(entry.Path).c_str());
        }
    }
}

bool ReadRawFile(const std::wstring& path, std::vector<BYTE>* data)
{
    HANDLE file = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_READ,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size = {};
    bool success = GetFileSizeEx(file, &size) && size.QuadPart >= 0 && size.QuadPart <= 8 * 1024 * 1024;
    if (success)
    {
        data->resize(static_cast<size_t>(size.QuadPart));
        size_t offset = 0;
        while (offset < data->size())
        {
            DWORD read = 0;
            const DWORD chunk = static_cast<DWORD>((std::min)(data->size() - offset, static_cast<size_t>(0x7ffff000)));
            if (!ReadFile(file, data->data() + offset, chunk, &read, nullptr) || read == 0)
            {
                success = false;
                break;
            }
            offset += read;
        }
    }
    CloseHandle(file);
    if (!success) data->clear();
    return success;
}

std::vector<std::wstring> SplitLines(const std::wstring& text)
{
    std::vector<std::wstring> lines;
    size_t start = 0;
    for (size_t index = 0; index <= text.size(); ++index)
    {
        if (index != text.size() && text[index] != L'\r' && text[index] != L'\n') continue;
        if (index > start) lines.push_back(text.substr(start, index - start));
        if (index < text.size() && text[index] == L'\r' && index + 1 < text.size() && text[index + 1] == L'\n') ++index;
        start = index + 1;
    }
    return lines;
}

ULONGLONG NextVersionNumber(const std::vector<std::wstring>& lines)
{
    if (lines.empty()) return 1;
    const std::wstring header = Trim(lines.front());
    ULONGLONG value = 0;
    for (wchar_t c : header)
    {
        if (c < L'0' || c > L'9') break;
        const unsigned digit = static_cast<unsigned>(c - L'0');
        if (value > ((std::numeric_limits<ULONGLONG>::max)() - digit) / 10) return 1;
        value = value * 10 + digit;
    }
    return value + 1;
}

bool WritePluginVersionFile(const std::wstring& hostDirectory, const std::wstring& targetDirectory,
                            const std::wstring& pluginRelativePath, bool appendPluginRecord)
{
    const std::wstring pluginsRoot = CombinePath(hostDirectory, L"plugins");
    const std::wstring versionPath = CombinePath(pluginsRoot, L"plugins.ver");
    std::vector<BYTE> bytes;
    std::wstring content;
    if (ReadRawFile(versionPath, &bytes) && !bytes.empty())
    {
        int count = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<const char*>(bytes.data()),
                                        static_cast<int>(bytes.size()), nullptr, 0);
        if (count > 0)
        {
            content.resize(static_cast<size_t>(count));
            MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<const char*>(bytes.data()),
                                static_cast<int>(bytes.size()), content.data(), count);
        }
    }
    if (!content.empty() && content[0] == 0xFEFF) content.erase(content.begin());
    std::vector<std::wstring> lines = SplitLines(content);
    const ULONGLONG version = NextVersionNumber(lines);
    if (lines.empty()) lines.push_back(std::to_wstring(version));
    else lines[0] = std::to_wstring(version);
    if (appendPluginRecord && !pluginRelativePath.empty())
    {
        const std::wstring modulePath = FullPath(CombinePath(targetDirectory, pluginRelativePath));
        const std::wstring root = FullPath(pluginsRoot);
        if (!IsPathInside(modulePath, root)) return false;
        lines.push_back(std::to_wstring(version) + L":" + modulePath.substr(root.size() + 1));
    }
    std::wstring output;
    for (const std::wstring& line : lines)
    {
        if (!output.empty()) output.append(L"\r\n");
        output += line;
    }
    output.append(L"\r\n");
    return WriteUtf8FileAtomic(versionPath, WideToAnsi(output));
}

bool WriteExtensionVersionFile(const std::wstring& hostDirectory, const std::vector<std::wstring>& manifests)
{
    if (manifests.empty()) return true;
    const std::wstring extensionsRoot = CombinePath(hostDirectory, L"extensions");
    const std::wstring versionPath = CombinePath(extensionsRoot, L"extensions.ver");
    std::string bytes;
    ReadUtf8File(versionPath, &bytes, 8 * 1024 * 1024);
    std::wstring content = SamandarinNative::Utf8ToWide(bytes);
    if (!content.empty() && content[0] == 0xFEFF) content.erase(content.begin());
    std::vector<std::wstring> lines = SplitLines(content);
    const ULONGLONG version = NextVersionNumber(lines);
    if (lines.empty()) lines.push_back(std::to_wstring(version));
    else lines[0] = std::to_wstring(version);
    for (const std::wstring& relative : manifests)
    {
        if (!IsSafeRelativePath(relative)) return false;
        lines.push_back(std::to_wstring(version) + L":" + relative);
    }
    std::wstring output;
    for (const std::wstring& line : lines)
    {
        if (!output.empty()) output.push_back(L'\n');
        output += line;
    }
    output.push_back(L'\n');
    return WriteUtf8FileAtomic(versionPath, WideToUtf8(output));
}

bool ParseInstallArguments(const char* commandLine, DWORD* processId, std::wstring* metadataPath)
{
    if (commandLine == nullptr || processId == nullptr || metadataPath == nullptr) return false;
    std::string arguments(commandLine);
    size_t start = arguments.find_first_not_of(" \t\r\n\"");
    if (start == std::string::npos) return false;
    size_t end = arguments.find_first_of(" \t\r\n", start);
    if (end == std::string::npos) return false;
    const std::string pidText = arguments.substr(start, end - start);
    char* parseEnd = nullptr;
    const unsigned long parsedPid = strtoul(pidText.c_str(), &parseEnd, 10);
    if (parseEnd == nullptr || *parseEnd != '\0' || parsedPid == 0 || parsedPid > MAXDWORD) return false;
    start = arguments.find_first_not_of(" \t\r\n\"", end);
    if (start == std::string::npos) return false;
    end = arguments.find_first_of(" \t\r\n\"", start);
    const std::string encoded = arguments.substr(start, end == std::string::npos ? std::string::npos : end - start);
    std::string decoded;
    if (!DecodeBase64(encoded, &decoded)) return false;
    *metadataPath = Utf8ToWide(decoded);
    *processId = static_cast<DWORD>(parsedPid);
    return !metadataPath->empty();
}

bool WaitForHostExit(DWORD processId, const std::wstring& fallbackDirectory,
                     HANDLE* processHandle, std::wstring* hostDirectory,
                     std::wstring* error)
{
    *processHandle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (*processHandle == nullptr)
    {
        if (GetLastError() == ERROR_INVALID_PARAMETER)
        {
            *hostDirectory = FullPath(fallbackDirectory);
            return true;
        }
        SetError(error, L"The Open Samandarin process could not be identified.");
        return false;
    }
    std::wstring imagePath;
    if (GetProcessImagePath(*processHandle, &imagePath)) *hostDirectory = GetParentDirectory(imagePath);
    else *hostDirectory = FullPath(fallbackDirectory);
    const DWORD wait = WaitForSingleObject(*processHandle, kInstallerWaitMs);
    if (wait != WAIT_OBJECT_0)
    {
        SetError(error, wait == WAIT_TIMEOUT ? L"Timed out waiting for Open Samandarin to exit." :
                                               L"The Open Samandarin process could not be monitored.");
        return false;
    }
    Sleep(500);
    return true;
}

bool InstallAfterHostExit(const std::wstring& metadataPath, DWORD processId, HINSTANCE module)
{
    std::string json;
    Value metadata;
    std::wstring error;
    std::wstring receiptPath;
    if (!ReadUtf8File(metadataPath, &json, kMaximumMetadataBytes) ||
        !SamandarinJson::ParseUtf8(json, &metadata) || metadata.Kind != Value::Type::Object)
    {
        SetError(&error, L"The installation manifest could not be read.");
        return false;
    }
    receiptPath = ReadString(metadata, L"receiptPath");
    std::wstring validatedHostDirectory;
    const std::wstring modulePath = GetModulePath(module);
    if (!SafeMetadataPath(metadataPath, metadata, modulePath, &validatedHostDirectory, &error))
    {
        return false;
    }
    ULONGLONG metadataPid = 0;
    if (!ReadNumber(metadata, L"hostProcessId", &metadataPid) || metadataPid != processId)
    {
        error = L"The installation manifest has an invalid process identifier.";
        SaveInstallFailure(receiptPath, error);
        return false;
    }
    HANDLE process = nullptr;
    std::wstring hostDirectory;
    if (!WaitForHostExit(processId, validatedHostDirectory, &process, &hostDirectory, &error))
    {
        SaveInstallFailure(receiptPath, error);
        if (process != nullptr) CloseHandle(process);
        return false;
    }
    if (process != nullptr) CloseHandle(process);
    if (!EqualsIgnoreCase(FullPath(hostDirectory), FullPath(validatedHostDirectory)))
    {
        error = L"The running application does not match the installation manifest.";
        SaveInstallFailure(receiptPath, error);
        return false;
    }

    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"Local\\OpenSamandarin.PackageInstall");
    if (mutex == nullptr)
    {
        error = L"The package installation lock could not be created.";
        SaveInstallFailure(receiptPath, error);
        return false;
    }
    const DWORD lockResult = WaitForSingleObject(mutex, kInstallerWaitMs);
    if (lockResult != WAIT_OBJECT_0 && lockResult != WAIT_ABANDONED)
    {
        CloseHandle(mutex);
        error = L"Timed out waiting for another package installation.";
        SaveInstallFailure(receiptPath, error);
        return false;
    }

    const std::wstring archive = ReadString(metadata, L"archivePath");
    const std::wstring expectedHash = ReadString(metadata, L"packageSha256");
    const std::wstring target = ReadString(metadata, L"targetDirectory");
    const std::wstring packageRoot = ReadString(metadata, L"packageRoot");
    const std::wstring stage = ReadString(metadata, L"stagingDirectory");
    const std::wstring packageType = ReadString(metadata, L"packageType");
    const std::wstring id = ReadString(metadata, L"id");
    const PackageKind kind = EqualsIgnoreCase(packageType, L"extension")
                                 ? PackageKind::Extension
                                 : PackageKind::Plugin;
    std::wstring actualHash;
    std::wstring verifiedPublisher;
    std::wstring isolatedRoot;
    std::wstring extractedRoot;
    std::wstring contentRoot;
    std::wstring pluginRelativePath;
    std::vector<std::wstring> extensionManifests;
    bool success = VerifyCatalogPackageMetadata(metadata, kind, &error);
    if (success && !IsHexSha256(expectedHash))
    {
        success = false;
        error = L"The package digest is invalid.";
    }
    if (success && HasReparseAncestor(packageRoot, hostDirectory))
    {
        success = false;
        error = L"The package root contains an unsafe reparse point.";
    }
    if (success && PathExists(target))
    {
        std::vector<TreeEntry> existingTarget;
        if (!EnumerateTree(target, &existingTarget, true, &error))
        {
            success = false;
            if (error.empty()) error = L"The package target contains an unsafe reparse point.";
        }
    }
    if (success && !CreateUniqueInstallDirectory(packageRoot, &isolatedRoot))
    {
        success = false;
        error = L"A protected package staging directory could not be created.";
    }
    const std::wstring trustedArchive = CombinePath(isolatedRoot, L"package.7z");
    if (success && !CopyFileW(AddLongPathPrefix(archive).c_str(),
                              AddLongPathPrefix(trustedArchive).c_str(), TRUE))
    {
        success = false;
        error = L"The verified package archive could not be copied.";
    }
    if (success)
    {
        success = ComputeSha256(trustedArchive, &actualHash, &error) &&
                  EqualsIgnoreCase(actualHash, expectedHash);
        if (!success && error.empty()) error = L"The staged package hash no longer matches the catalog.";
    }
    const std::wstring trustedPayload = CombinePath(isolatedRoot, L"payload");
    if (success && !CreateDirectories(trustedPayload))
    {
        success = false;
        error = L"The verified package payload directory could not be created.";
    }
    if (success)
        success = ExtractAndValidatePackage(trustedArchive, trustedPayload, kind, &extractedRoot, &error);
    if (success)
        contentRoot = kind == PackageKind::Extension ? ResolveExtensionContentRoot(extractedRoot) : extractedRoot;
    if (success)
    {
        success = VerifyExtractedPackage(extractedRoot, kind, &verifiedPublisher, &error);
        const std::wstring expectedPublisher = ReadString(metadata, L"signer");
        if (success && !expectedPublisher.empty() && !EqualsIgnoreCase(expectedPublisher, verifiedPublisher))
        {
            success = false;
            error = L"The package signer changed after validation.";
        }
    }
    if (success && kind == PackageKind::Plugin)
        success = FindPluginRelativePath(extractedRoot, id, &pluginRelativePath, &error);
    if (success && kind == PackageKind::Extension)
    {
        success = BuildExtensionManifestRecords(contentRoot, target, packageRoot, &extensionManifests, &error);
        if (success && extensionManifests.empty())
        {
            success = false;
            error = L"The extension package does not contain a registerable manifest.";
        }
    }
    if (success && PathExists(target))
    {
        if (kind == PackageKind::Plugin)
        {
            if (!PathExists(CombinePath(target, pluginRelativePath)))
            {
                success = false;
                error = L"The existing target does not contain the matching plugin module.";
            }
        }
        else
        {
            const std::wstring targetName = GetFileName(target);
            const bool isDemoPackage = EqualsIgnoreCase(id, L"salamatrixdemos") &&
                                       EqualsIgnoreCase(targetName, L"demos");
            if (!isDemoPackage && !EqualsIgnoreCase(targetName, id))
            {
                success = false;
                error = L"The existing target does not match the extension package identifier.";
            }
        }
    }
    else if (success)
    {
        const std::wstring expectedTarget = FullPath(CombinePath(packageRoot, GetFileName(contentRoot)));
        if (!EqualsIgnoreCase(FullPath(target), expectedTarget))
        {
            success = false;
            error = L"The new target does not match the verified package directory.";
        }
    }
    if (success) success = CopyPackageTree(contentRoot, target, &error);
    if (success)
    {
        RemoveExtraPackageFiles(contentRoot, target);
        if (kind == PackageKind::Plugin)
        {
            const bool append = metadata.Find(L"appendPluginRecord") != nullptr &&
                                metadata.Find(L"appendPluginRecord")->Kind == Value::Type::Boolean &&
                                metadata.Find(L"appendPluginRecord")->Boolean;
            success = WritePluginVersionFile(hostDirectory, target,
                                             pluginRelativePath, append);
            if (!success) error = L"plugins.ver could not be updated.";
        }
        else
        {
            if (!WriteExtensionVersionFile(hostDirectory, extensionManifests))
            {
                success = false;
                error = L"extensions.ver could not be updated.";
            }
        }
    }

    if (success)
    {
        Value store = LoadReceiptStore(receiptPath);
        Value receipt = MakeObject();
        Put(&receipt, L"id", MakeString(ReadString(metadata, L"id")));
        Put(&receipt, L"packageType", MakeString(ReadString(metadata, L"packageType")));
        Put(&receipt, L"sourceUrl", MakeString(ReadString(metadata, L"sourceUrl")));
        Put(&receipt, L"packageSha256", MakeString(actualHash));
        Put(&receipt, L"signer", MakeString(verifiedPublisher));
        Put(&receipt, L"version", MakeString(ReadString(metadata, L"version")));
        Put(&receipt, L"verifiedAt", MakeString(ReadString(metadata, L"verifiedAt")));
        UpsertReceipt(&store, receipt);
        Put(&store, L"lastInstallError", MakeNull());
        success = SaveReceiptStore(receiptPath, &store);
        if (!success) error = L"The package receipt could not be saved.";
    }
    if (!success)
    {
        SaveInstallFailure(receiptPath, error);
    }
    if (!isolatedRoot.empty()) DeleteTree(isolatedRoot);
    if (success) DeleteTree(stage);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return success;
}
} // namespace

bool StageAndSchedulePackage(const PluginUpdateRow& row, DWORD hostProcessId,
                             volatile LONG* cancelFlag, std::wstring* status,
                             std::wstring* error)
{
    if (status != nullptr) status->clear();
    if (error != nullptr) error->clear();
    if (!IsHexSha256(row.PackageSha256))
    {
        SetError(error, GetLocalizedString(IDS_PLUGIN_INSTALL_MISSING_HASH));
        return false;
    }
    if (!CanInstallPluginRow(row))
    {
        SetError(error, GetLocalizedString(IDS_PLUGIN_INSTALL_UNAVAILABLE));
        return false;
    }
    const PackageKind kind = row.CatalogKind != PackageKind::Unknown
                                 ? row.CatalogKind
                                 : StartsWithIgnoreCase(GetFileName(row.WebUrl), L"extension")
                                       ? PackageKind::Extension
                                       : PackageKind::Plugin;
    const std::wstring hostDirectory = GetExecutableDirectory();
    const std::wstring packageRoot = CombinePath(hostDirectory, kind == PackageKind::Extension ? L"extensions" : L"plugins");
    std::wstring stage;
    if (!CreateUniqueStageDirectory(&stage))
    {
        SetError(error, L"The package staging directory could not be created.");
        return false;
    }
    const std::wstring payload = CombinePath(stage, L"payload");
    const std::wstring archive = CombinePath(stage, L"package.7z");
    if (!CreateDirectories(payload))
    {
        SetError(error, L"The package staging directory could not be created.");
        DeleteTree(stage);
        return false;
    }
    if (!HttpDownloadToFile(row.WebUrl, archive, kMaximumDownloadBytes, error, cancelFlag))
    {
        DeleteTree(stage);
        return false;
    }
    std::wstring actualHash;
    if (!ComputeSha256(archive, &actualHash, error))
    {
        DeleteTree(stage);
        return false;
    }
    if (!EqualsIgnoreCase(actualHash, row.PackageSha256))
    {
        SetError(error, GetLocalizedString(IDS_PLUGIN_INSTALL_HASH_MISMATCH));
        DeleteTree(stage);
        return false;
    }

    std::wstring extractedRoot;
    if (!ExtractAndValidatePackage(archive, payload, kind, &extractedRoot, error))
    {
        DeleteTree(stage);
        return false;
    }
    std::wstring signer;
    if (!VerifyExtractedPackage(extractedRoot, kind, &signer, error))
    {
        DeleteTree(stage);
        return false;
    }
    const std::wstring contentRoot = kind == PackageKind::Extension
                                         ? ResolveExtensionContentRoot(extractedRoot)
                                         : extractedRoot;
    const bool installed = !row.InstallDirectory.empty();
    const std::wstring target = FullPath(installed ? row.InstallDirectory
                                                   : CombinePath(packageRoot, GetFileName(contentRoot)));
    if (target.empty() || !IsPathInside(target, packageRoot) || EqualsIgnoreCase(target, FullPath(packageRoot)) ||
        EqualsIgnoreCase(target, FullPath(stage)))
    {
        SetError(error, L"The installed package path is not a safe package directory.");
        DeleteTree(stage);
        return false;
    }
    std::wstring pluginRelative;
    std::vector<std::wstring> extensionManifests;
    if (kind == PackageKind::Plugin)
    {
        if (!FindPluginRelativePath(extractedRoot, row.Id, &pluginRelative, error))
        {
            DeleteTree(stage);
            return false;
        }
    }
    else
    {
        if (!BuildExtensionManifestRecords(contentRoot, target, packageRoot, &extensionManifests, error) ||
            (extensionManifests.empty() && IsPathInside(target, packageRoot)))
        {
            if (error != nullptr && error->empty()) *error = L"The extension package does not contain a registerable manifest.";
            DeleteTree(stage);
            return false;
        }
    }
    const std::wstring receiptPath = CombinePath(hostDirectory, kReceiptFileName);
    std::wstring metadataPath;
    if (!CreateInstallMetadata(stage, archive, contentRoot, target, packageRoot, receiptPath,
                               row, kind, signer, actualHash, pluginRelative,
                               kind == PackageKind::Plugin && !installed, extensionManifests, &metadataPath))
    {
        SetError(error, L"The installation manifest could not be written.");
        DeleteTree(stage);
        return false;
    }

    const std::wstring systemDirectory = GetSystemDirectory();
    const std::wstring rundll32 = CombinePath(systemDirectory, L"rundll32.exe");
    const std::wstring pluginDll = GetModulePath(DLLInstance);
    const std::string metadataUtf8 = WideToUtf8(metadataPath);
    const std::string encoded = EncodeBase64(metadataUtf8);
    if (systemDirectory.empty() || pluginDll.empty() || encoded.empty())
    {
        SetError(error, L"The native package installer could not be started.");
        DeleteTree(stage);
        return false;
    }
    std::wstring parameters = L"\"" + pluginDll + L"\",Samandarin_InstallAfterExit " +
                              std::to_wstring(hostProcessId) + L" " +
                              SamandarinNative::Utf8ToWide(encoded);
    SHELLEXECUTEINFOW execute = {};
    execute.cbSize = sizeof(execute);
    execute.fMask = SEE_MASK_NOASYNC;
    execute.lpVerb = IsInsideProgramFiles(target) || IsInsideProgramFiles(packageRoot) ? L"runas" : nullptr;
    execute.lpFile = rundll32.c_str();
    execute.lpParameters = parameters.c_str();
    execute.nShow = SW_HIDE;
    if (!ShellExecuteExW(&execute))
    {
        SetError(error, L"The native package installer could not be started (" + std::to_wstring(GetLastError()) + L").");
        DeleteTree(stage);
        return false;
    }
    if (execute.hProcess != nullptr) CloseHandle(execute.hProcess);
    if (status != nullptr)
        *status = installed ? GetLocalizedString(IDS_PLUGIN_UPDATE_SCHEDULED)
                            : GetLocalizedString(IDS_PLUGIN_INSTALL_SCHEDULED);
    return true;
}

bool GetInstallReceipt(const std::wstring& id, PackageKind kind, InstallReceipt* receipt)
{
    if (receipt == nullptr || id.empty()) return false;
    const std::wstring path = CombinePath(GetExecutableDirectory(), kReceiptFileName);
    Value store = LoadReceiptStore(path);
    const Value* entries = store.Find(L"receipts");
    if (entries == nullptr || entries->Kind != Value::Type::Array) return false;
    const std::wstring expectedKind = kind == PackageKind::Extension ? L"extension" : L"plugin";
    for (const Value& item : entries->Array)
    {
        if (!EqualsIgnoreCase(ReadString(item, L"id"), id) ||
            !EqualsIgnoreCase(ReadString(item, L"packageType"), expectedKind)) continue;
        receipt->Signer = ReadString(item, L"signer");
        receipt->PackageSha256 = ReadString(item, L"packageSha256");
        receipt->Version = ReadString(item, L"version");
        return true;
    }
    return false;
}

bool TakeLastInstallError(std::wstring* error)
{
    if (error == nullptr) return false;
    const std::wstring path = CombinePath(GetExecutableDirectory(), kReceiptFileName);
    Value store = LoadReceiptStore(path);
    const Value* lastError = store.Find(L"lastInstallError");
    const std::wstring message = lastError == nullptr ? std::wstring() : lastError->StringOrEmpty();
    if (message.empty()) return false;
    *error = message.substr(0, 4096);
    Put(&store, L"lastInstallError", MakeNull());
    SaveReceiptStore(path, &store);
    return true;
}
} // namespace SamandarinNative

extern "C" void CALLBACK Samandarin_InstallAfterExit(HWND hwnd, HINSTANCE instance,
                                                       LPSTR commandLine, int showCommand)
{
    (void)hwnd;
    (void)showCommand;
    DWORD processId = 0;
    std::wstring metadataPath;
    if (!SamandarinNative::ParseInstallArguments(commandLine, &processId, &metadataPath)) return;
    SamandarinNative::InstallAfterHostExit(metadataPath, processId, instance);
}
