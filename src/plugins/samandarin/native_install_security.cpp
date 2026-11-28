// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Verify package SHA-256 and Authenticode signatures with native Windows APIs. | Replace managed SHA-256 and WinVerifyTrust wrappers. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_install_security.h"
#include "native_utils.h"

#include <bcrypt.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <softpub.h>
#include <algorithm>
#include <limits>
#include <set>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

namespace SamandarinNative
{
namespace
{
const GUID kAuthenticodeAction = {0x00AAC56B, 0xCD44, 0x11D0, {0x8C, 0xC2, 0x00, 0xC0, 0x4F, 0xC2, 0x95, 0xEE}};
const wchar_t* const kExpectedPublishers[] = {L"Open Source Developer", L"Kotas", L"KRtkovo"};

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

bool ContainsIgnoreCase(const std::wstring& value, const std::wstring& needle)
{
    if (needle.empty()) return true;
    for (size_t index = 0; index + needle.size() <= value.size(); ++index)
    {
        if (CompareStringOrdinal(value.data() + index, static_cast<int>(needle.size()),
                                 needle.data(), static_cast<int>(needle.size()), TRUE) == CSTR_EQUAL)
            return true;
    }
    return false;
}

bool IsExpectedPublisher(const std::wstring& publisher)
{
    for (const wchar_t* needle : kExpectedPublishers)
    {
        if (ContainsIgnoreCase(publisher, needle)) return true;
    }
    return false;
}

bool IsUnsignedRedistributable(const std::wstring& name)
{
    static const wchar_t* const names[] = {
        L"7za.dll", L"7zwrapper.dll", L"unrar.dll", L"chmlib.dll", L"sqlite.dll",
        L"libeay32.dll", L"ssleay32.dll", L"Newtonsoft.Json.dll", L"WebView2Loader.dll",
        L"dbghelp.dll", L"ucrtbase.dll", L"vcruntime140.dll", L"vcruntime140_1.dll",
        L"msvcp140.dll", L"concrt140.dll", L"lua.dll", L"lua.exe", L"python.exe",
        L"python3.dll", L"python311.dll", L"python312.dll", L"python313.dll",
    };
    for (const wchar_t* known : names)
    {
        if (EqualsIgnoreCase(name, known)) return true;
    }
    return StartsWithIgnoreCase(name, L"api-ms-win-") || StartsWithIgnoreCase(name, L"System.") ||
           StartsWithIgnoreCase(name, L"Microsoft.Web.WebView2.") || StartsWithIgnoreCase(name, L"WebView2");
}

bool IsSignedFileExtension(const std::wstring& extension)
{
    return EqualsIgnoreCase(extension, L".spl") || EqualsIgnoreCase(extension, L".slg") ||
           EqualsIgnoreCase(extension, L".dll") || EqualsIgnoreCase(extension, L".exe");
}

bool VerifyFileTrust(const std::wstring& path, std::wstring* publisher)
{
    const std::wstring trustPath = AddLongPathPrefix(path);
    WINTRUST_FILE_INFO fileInfo = {};
    fileInfo.cbStruct = sizeof(fileInfo);
    fileInfo.pcwszFilePath = trustPath.c_str();
    WINTRUST_DATA trust = {};
    trust.cbStruct = sizeof(trust);
    trust.dwUIChoice = WTD_UI_NONE;
    trust.fdwRevocationChecks = WTD_REVOKE_NONE;
    trust.dwUnionChoice = WTD_CHOICE_FILE;
    trust.pFile = &fileInfo;
    trust.dwStateAction = WTD_STATEACTION_VERIFY;
    trust.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;
    GUID action = kAuthenticodeAction;
    const HWND noOwner = reinterpret_cast<HWND>(INVALID_HANDLE_VALUE);
    const LONG result = WinVerifyTrust(noOwner, &action, &trust);
    trust.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(noOwner, &action, &trust);
    if (result != ERROR_SUCCESS) return false;

    DWORD encoding = 0, content = 0, format = 0;
    HCERTSTORE store = nullptr;
    HCRYPTMSG message = nullptr;
    PCCERT_CONTEXT certificate = nullptr;
    const BOOL queried = CryptQueryObject(CERT_QUERY_OBJECT_FILE, AddLongPathPrefix(path).c_str(),
                                          CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                                          CERT_QUERY_FORMAT_FLAG_BINARY, 0, &encoding, &content,
                                          &format, &store, &message,
                                          reinterpret_cast<const void**>(&certificate));
    bool success = queried != FALSE && certificate != nullptr;
    if (success)
    {
        const DWORD required = CertGetNameStringW(certificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0,
                                                   nullptr, nullptr, 0);
        if (required > 1)
        {
            std::vector<wchar_t> name(required, L'\0');
            if (CertGetNameStringW(certificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, nullptr,
                                   name.data(), required) > 1)
            {
                publisher->assign(name.data());
            }
        }
        if (publisher->empty())
        {
            const DWORD requiredSubject = CertGetNameStringW(certificate, CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                                              CERT_NAME_ISSUER_FLAG, nullptr, nullptr, 0);
            if (requiredSubject > 1)
            {
                std::vector<wchar_t> subject(requiredSubject, L'\0');
                if (CertGetNameStringW(certificate, CERT_NAME_SIMPLE_DISPLAY_TYPE, CERT_NAME_ISSUER_FLAG,
                                       nullptr, subject.data(), requiredSubject) > 1)
                    publisher->assign(subject.data());
            }
        }
    }
    if (certificate != nullptr) CertFreeCertificateContext(certificate);
    if (message != nullptr) CryptMsgClose(message);
    if (store != nullptr) CertCloseStore(store, 0);
    return success;
}

bool EnumerateFiles(const std::wstring& root, std::vector<std::wstring>* files, std::wstring* error)
{
    std::vector<std::wstring> directories{root};
    while (!directories.empty())
    {
        const std::wstring directory = std::move(directories.back());
        directories.pop_back();
        WIN32_FIND_DATAW data = {};
        HANDLE find = FindFirstFileW(AddLongPathPrefix(CombinePath(directory, L"*")).c_str(), &data);
        if (find == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND) continue;
            SetError(error, L"The extracted package could not be scanned.");
            return false;
        }
        do
        {
            if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
            const std::wstring path = CombinePath(directory, data.cFileName);
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
            {
                FindClose(find);
                SetError(error, L"Reparse points are not allowed in update packages.");
                return false;
            }
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                directories.push_back(path);
            else
                files->push_back(path);
        } while (FindNextFileW(find, &data));
        const DWORD status = GetLastError();
        FindClose(find);
        if (status != ERROR_NO_MORE_FILES)
        {
            SetError(error, L"The extracted package could not be scanned.");
            return false;
        }
    }
    return true;
}
} // namespace

bool ComputeSha256(const std::wstring& path, std::wstring* hash, std::wstring* error)
{
    if (hash == nullptr) return false;
    hash->clear();
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hashHandle = nullptr;
    DWORD objectSize = 0, bytes = 0, hashSize = 0;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status < 0)
    {
        SetError(error, L"SHA-256 is unavailable.");
        return false;
    }
    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize),
                               sizeof(objectSize), &bytes, 0);
    if (status >= 0)
        status = BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashSize),
                                   sizeof(hashSize), &bytes, 0);
    std::vector<BYTE> object(objectSize);
    std::vector<BYTE> digest(hashSize);
    if (status >= 0) status = BCryptCreateHash(algorithm, &hashHandle, object.data(), objectSize, nullptr, 0, 0);
    HANDLE file = INVALID_HANDLE_VALUE;
    if (status >= 0)
    {
        file = CreateFileW(AddLongPathPrefix(path).c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) status = static_cast<NTSTATUS>(0xC0000001L);
    }
    std::vector<BYTE> buffer(1024 * 1024);
    while (status >= 0 && file != INVALID_HANDLE_VALUE)
    {
        DWORD read = 0;
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &read, nullptr))
        {
            status = static_cast<NTSTATUS>(0xC0000001L);
            break;
        }
        if (read == 0) break;
        status = BCryptHashData(hashHandle, buffer.data(), read, 0);
    }
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (status >= 0) status = BCryptFinishHash(hashHandle, digest.data(), hashSize, 0);
    if (hashHandle != nullptr) BCryptDestroyHash(hashHandle);
    if (algorithm != nullptr) BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0)
    {
        SetError(error, L"The package SHA-256 digest could not be calculated.");
        return false;
    }
    static const wchar_t hex[] = L"0123456789abcdef";
    hash->reserve(digest.size() * 2);
    for (BYTE value : digest)
    {
        hash->push_back(hex[value >> 4]);
        hash->push_back(hex[value & 0x0F]);
    }
    return true;
}

bool VerifyExtractedPackage(const std::wstring& root, PackageKind kind,
                            std::wstring* firstPartyPublisher, std::wstring* error)
{
    if (firstPartyPublisher != nullptr) firstPartyPublisher->clear();
    std::vector<std::wstring> files;
    if (!EnumerateFiles(root, &files, error)) return false;
    bool hasPluginModule = false;
    bool firstPartySigned = false;
    std::wstring publisher;
    for (const std::wstring& path : files)
    {
        const std::wstring extension = GetFileExtension(path);
        if (EqualsIgnoreCase(extension, L".spl")) hasPluginModule = true;
        if (!IsSignedFileExtension(extension)) continue;
        const bool firstParty = EqualsIgnoreCase(extension, L".spl") || EqualsIgnoreCase(extension, L".slg");
        if (!firstParty && IsUnsignedRedistributable(GetFileName(path))) continue;
        std::wstring currentPublisher;
        if (!VerifyFileTrust(path, &currentPublisher))
        {
            SetError(error, FormatLocalizedString(IDS_PLUGIN_INSTALL_UNSIGNED_NAMED, {GetFileName(path)}));
            return false;
        }
        if (firstParty)
        {
            if (!IsExpectedPublisher(currentPublisher))
            {
                SetError(error, FormatLocalizedString(IDS_PLUGIN_INSTALL_UNEXPECTED_PUBLISHER,
                                                      {GetFileName(path), currentPublisher}));
                return false;
            }
            firstPartySigned = true;
            if (publisher.empty()) publisher = currentPublisher;
        }
    }
    if (kind == PackageKind::Plugin && !hasPluginModule)
    {
        SetError(error, GetLocalizedString(IDS_PLUGIN_INSTALL_UNSIGNED));
        return false;
    }
    if (kind == PackageKind::Plugin && !firstPartySigned)
    {
        SetError(error, GetLocalizedString(IDS_PLUGIN_INSTALL_UNSIGNED));
        return false;
    }
    if (firstPartyPublisher != nullptr) *firstPartyPublisher = publisher;
    return true;
}
} // namespace SamandarinNative
