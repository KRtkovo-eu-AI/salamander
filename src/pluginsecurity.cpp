// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "pluginsecurity.h"
#include "consts.h"

#include <softpub.h>
#include <wincrypt.h>
#include <wintrust.h>

#include <vector>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

struct CPackageSecurity
{
    char Id[128];
    char Network[32];
    char Processes[32];
    char Script[32];
    char Web[32];
    char Elevation[32];
};

struct CPackageReceipt
{
    char Id[128];
    char SourceUrl[1024];
    char Sha256[80];
    char Signer[256];
};

static std::vector<CPackageSecurity> Capabilities;
static std::vector<CPackageReceipt> Receipts;
static std::vector<CPackageReceipt> BundledMetadata;
static BOOL CapabilitiesLoaded = FALSE;
static BOOL ReceiptsLoaded = FALSE;
static BOOL BundledMetadataLoaded = FALSE;

static void CopyField(char* dest, int destSize, const char* source)
{
    if (dest == NULL || destSize <= 0)
        return;
    dest[0] = 0;
    if (source != NULL)
        lstrcpyn(dest, source, destSize);
}

static const char* SkipWs(const char* text)
{
    while (text != NULL && *text != 0 && (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n'))
        text++;
    return text;
}

static BOOL ExtractJsonString(const char* json, const char* key, char* dest, int destSize)
{
    if (json == NULL || key == NULL || dest == NULL || destSize <= 0)
        return FALSE;
    dest[0] = 0;
    char pattern[128];
    _snprintf_s(pattern, _TRUNCATE, "\"%s\"", key);
    const char* found = strstr(json, pattern);
    if (found == NULL)
        return FALSE;
    found = strchr(found + (int)strlen(pattern), ':');
    if (found == NULL)
        return FALSE;
    found = SkipWs(found + 1);
    if (*found == 'n' && strncmp(found, "null", 4) == 0)
        return TRUE;
    if (*found != '"')
        return FALSE;
    found++;
    int index = 0;
    while (*found != 0 && *found != '"' && index < destSize - 1)
    {
        if (*found == '\\' && found[1] != 0)
            found++;
        dest[index++] = *found++;
    }
    dest[index] = 0;
    return TRUE;
}

static void GetInstallFilePath(const char* fileName, char* path, int pathSize)
{
    DWORD len = GetModuleFileName(NULL, path, pathSize);
    if (len == 0 || len >= (DWORD)pathSize)
    {
        path[0] = 0;
        return;
    }
    char* slash = strrchr(path, '\\');
    if (slash != NULL)
        slash++;
    else
        slash = path;
    lstrcpyn(slash, fileName, pathSize - (int)(slash - path));
}

static char* ReadTextFile(const char* path)
{
    HANDLE file = HANDLES_Q(CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    if (file == INVALID_HANDLE_VALUE)
        return NULL;
    LARGE_INTEGER size;
    size.QuadPart = 0;
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 8 * 1024 * 1024)
    {
        HANDLES(CloseHandle(file));
        return NULL;
    }
    char* buffer = (char*)malloc((size_t)size.QuadPart + 1);
    if (buffer == NULL)
    {
        HANDLES(CloseHandle(file));
        return NULL;
    }
    DWORD read = 0;
    BOOL ok = ReadFile(file, buffer, (DWORD)size.QuadPart, &read, NULL);
    HANDLES(CloseHandle(file));
    if (!ok)
    {
        free(buffer);
        return NULL;
    }
    buffer[read] = 0;
    return buffer;
}

static void LoadCapabilities()
{
    if (CapabilitiesLoaded)
        return;
    CapabilitiesLoaded = TRUE;
    char path[SAL_MAX_PATH];
    GetInstallFilePath("plugin-capabilities.json", path, _countof(path));
    char* json = ReadTextFile(path);
    if (json == NULL)
        return;
    const char* cursor = json;
    while ((cursor = strstr(cursor, "\"id\"")) != NULL)
    {
        const char* objectStart = cursor;
        while (objectStart > json && *objectStart != '{')
            objectStart--;
        const char* objectEnd = strchr(cursor, '}');
        if (objectEnd == NULL)
            break;
        const char* security = strstr(objectStart, "\"security\"");
        const char* windowEnd = objectEnd;
        if (security != NULL)
        {
            const char* nested = strchr(security, '}');
            if (nested != NULL)
                windowEnd = nested;
        }
        int windowLen = (int)(windowEnd - objectStart) + 1;
        if (windowLen <= 1 || windowLen >= 2048)
        {
            cursor = objectEnd + 1;
            continue;
        }
        char window[2049];
        memcpy(window, objectStart, windowLen);
        window[windowLen] = 0;

        CPackageSecurity item;
        memset(&item, 0, sizeof(item));
        ExtractJsonString(window, "id", item.Id, _countof(item.Id));
        ExtractJsonString(window, "networkAccess", item.Network, _countof(item.Network));
        ExtractJsonString(window, "externalProcesses", item.Processes, _countof(item.Processes));
        ExtractJsonString(window, "scriptExecution", item.Script, _countof(item.Script));
        ExtractJsonString(window, "activeWebContent", item.Web, _countof(item.Web));
        ExtractJsonString(window, "elevation", item.Elevation, _countof(item.Elevation));
        if (item.Id[0] != 0)
            Capabilities.push_back(item);
        cursor = windowEnd + 1;
    }
    free(json);
}

static void LoadReceipts()
{
    if (ReceiptsLoaded)
        return;
    ReceiptsLoaded = TRUE;
    char path[SAL_MAX_PATH];
    GetInstallFilePath("plugin-receipts.json", path, _countof(path));
    char* json = ReadTextFile(path);
    if (json == NULL)
        return;
    const char* cursor = json;
    while ((cursor = strstr(cursor, "\"id\"")) != NULL)
    {
        const char* objectStart = cursor;
        while (objectStart > json && *objectStart != '{')
            objectStart--;
        const char* objectEnd = strchr(cursor, '}');
        if (objectEnd == NULL)
            break;
        int windowLen = (int)(objectEnd - objectStart) + 1;
        if (windowLen <= 1 || windowLen >= 2048)
        {
            cursor = objectEnd + 1;
            continue;
        }
        char window[2049];
        memcpy(window, objectStart, windowLen);
        window[windowLen] = 0;
        CPackageReceipt item;
        memset(&item, 0, sizeof(item));
        ExtractJsonString(window, "id", item.Id, _countof(item.Id));
        ExtractJsonString(window, "sourceUrl", item.SourceUrl, _countof(item.SourceUrl));
        ExtractJsonString(window, "packageSha256", item.Sha256, _countof(item.Sha256));
        ExtractJsonString(window, "signer", item.Signer, _countof(item.Signer));
        if (item.Id[0] != 0)
            Receipts.push_back(item);
        cursor = objectEnd + 1;
    }
    free(json);
}

static void LoadBundledMetadata()
{
    if (BundledMetadataLoaded)
        return;
    BundledMetadataLoaded = TRUE;
    char path[SAL_MAX_PATH];
    GetInstallFilePath("bundled-plugin-metadata.json", path, _countof(path));
    char* json = ReadTextFile(path);
    if (json == NULL)
        return;
    const char* cursor = json;
    while ((cursor = strstr(cursor, "\"id\"")) != NULL)
    {
        const char* objectStart = cursor;
        while (objectStart > json && *objectStart != '{')
            objectStart--;
        const char* objectEnd = strchr(cursor, '}');
        if (objectEnd == NULL)
            break;
        int windowLen = (int)(objectEnd - objectStart) + 1;
        if (windowLen > 1 && windowLen < 2048)
        {
            char window[2049];
            memcpy(window, objectStart, windowLen);
            window[windowLen] = 0;
            CPackageReceipt item;
            memset(&item, 0, sizeof(item));
            ExtractJsonString(window, "id", item.Id, _countof(item.Id));
            ExtractJsonString(window, "packageSha256", item.Sha256, _countof(item.Sha256));
            ExtractJsonString(window, "signer", item.Signer, _countof(item.Signer));
            if (item.Id[0] != 0 && strlen(item.Sha256) == 64)
                BundledMetadata.push_back(item);
        }
        cursor = objectEnd + 1;
    }
    free(json);
}

static const CPackageReceipt* FindBundledMetadata(const char* id)
{
    LoadBundledMetadata();
    if (id == NULL || id[0] == 0)
        return NULL;
    for (size_t i = 0; i < BundledMetadata.size(); i++)
    {
        if (stricmp(BundledMetadata[i].Id, id) == 0)
            return &BundledMetadata[i];
    }
    return NULL;
}

static const CPackageSecurity* FindCapability(const char* id)
{
    LoadCapabilities();
    if (id == NULL || id[0] == 0)
        return NULL;
    for (size_t i = 0; i < Capabilities.size(); i++)
    {
        if (stricmp(Capabilities[i].Id, id) == 0)
            return &Capabilities[i];
    }
    return NULL;
}

static const CPackageReceipt* FindReceipt(const char* id)
{
    LoadReceipts();
    if (id == NULL || id[0] == 0)
        return NULL;
    for (size_t i = 0; i < Receipts.size(); i++)
    {
        if (stricmp(Receipts[i].Id, id) == 0)
            return &Receipts[i];
    }
    return NULL;
}

static const char* FormatFlag(const char* value, BOOL elevation)
{
    if (value == NULL || value[0] == 0)
        return LoadStr(IDS_PLUGINSEC_UNKNOWN);
    if (elevation)
    {
        if (stricmp(value, "install") == 0)
            return LoadStr(IDS_PLUGINSEC_ELEVATION_INSTALL);
        if (stricmp(value, "never") == 0)
            return LoadStr(IDS_PLUGINSEC_NO);
    }
    if (stricmp(value, "yes") == 0)
        return LoadStr(IDS_PLUGINSEC_YES);
    if (stricmp(value, "no") == 0)
        return LoadStr(IDS_PLUGINSEC_NO);
    if (stricmp(value, "possible") == 0)
        return LoadStr(IDS_PLUGINSEC_POSSIBLE);
    return value;
}

static BOOL PathToWide(const char* path, wchar_t* widePath, int wideSize)
{
    if (path == NULL || widePath == NULL || wideSize <= 0)
        return FALSE;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, widePath, wideSize) > 0)
        return TRUE;
    return MultiByteToWideChar(CP_ACP, 0, path, -1, widePath, wideSize) > 0;
}

static BOOL GetPublisher(const char* path, char* publisher, int publisherSize)
{
    publisher[0] = 0;
    wchar_t widePath[SAL_MAX_PATH];
    if (!PathToWide(path, widePath, SAL_MAX_PATH))
        return FALSE;

    WINTRUST_FILE_INFO fileInfo;
    memset(&fileInfo, 0, sizeof(fileInfo));
    fileInfo.cbStruct = sizeof(fileInfo);
    fileInfo.pcwszFilePath = widePath;

    WINTRUST_DATA data;
    memset(&data, 0, sizeof(data));
    data.cbStruct = sizeof(data);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &fileInfo;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    data.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;

    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    LONG status = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &action, &data);
    data.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &action, &data);
    if (status != ERROR_SUCCESS)
        return FALSE;

    PCCERT_CONTEXT cert = NULL;
    DWORD encoding = 0;
    DWORD contentType = 0;
    DWORD formatType = 0;
    HCERTSTORE store = NULL;
    if (CryptQueryObject(CERT_QUERY_OBJECT_FILE, widePath, CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
                         CERT_QUERY_FORMAT_FLAG_BINARY, 0, &encoding, &contentType, &formatType,
                         &store, NULL, (const void**)&cert) &&
        cert != NULL)
    {
        char name[256];
        if (CertGetNameStringA(cert, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, name, _countof(name)) > 1)
            CopyField(publisher, publisherSize, name);
        CertFreeCertificateContext(cert);
    }
    if (store != NULL)
        CertCloseStore(store, 0);
    return publisher[0] != 0;
}

static void FormatSecurityText(const char* id, const char* binaryPath,
                               const CPackageSecurity* declared,
                               char* buffer, int bufferSize)
{
    buffer[0] = 0;
    const CPackageSecurity* security = declared;
    if (security == NULL ||
        (security->Network[0] == 0 && security->Processes[0] == 0 &&
         security->Script[0] == 0 && security->Web[0] == 0 &&
         security->Elevation[0] == 0))
    {
        security = FindCapability(id);
    }
    const CPackageReceipt* receipt = FindReceipt(id);
    const CPackageReceipt* bundledMetadata = receipt == NULL ? FindBundledMetadata(id) : NULL;
    char publisher[256];
    publisher[0] = 0;
    if (binaryPath != NULL && binaryPath[0] != 0)
        GetPublisher(binaryPath, publisher, _countof(publisher));

    const char* signer = publisher[0] != 0
                             ? publisher
                             : (receipt != NULL && receipt->Signer[0] != 0
                                    ? receipt->Signer
                                    : (bundledMetadata != NULL && bundledMetadata->Signer[0] != 0
                                           ? bundledMetadata->Signer
                                           : LoadStr(IDS_PLUGINSEC_UNKNOWN)));
    const BOOL bundled = receipt == NULL && security != NULL;
    const char* hash = receipt != NULL && receipt->Sha256[0] != 0
                           ? receipt->Sha256
                           : (bundledMetadata != NULL && bundledMetadata->Sha256[0] != 0
                                  ? bundledMetadata->Sha256
                                  : (bundled ? LoadStr(IDS_PLUGINSEC_BUNDLED) : LoadStr(IDS_PLUGINSEC_UNKNOWN)));
    const char* source = receipt != NULL && receipt->SourceUrl[0] != 0
                             ? receipt->SourceUrl
                             : (bundled ? LoadStr(IDS_PLUGINSEC_BUNDLED) : LoadStr(IDS_PLUGINSEC_UNKNOWN));

    char line[2048];
    _snprintf_s(line, _TRUNCATE, "%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s\n%s: %s",
                LoadStr(IDS_PLUGINSEC_NETWORK),
                FormatFlag(security != NULL ? security->Network : NULL, FALSE),
                LoadStr(IDS_PLUGINSEC_PROCESSES),
                FormatFlag(security != NULL ? security->Processes : NULL, FALSE),
                LoadStr(IDS_PLUGINSEC_SCRIPT),
                FormatFlag(security != NULL ? security->Script : NULL, FALSE),
                LoadStr(IDS_PLUGINSEC_WEB),
                FormatFlag(security != NULL ? security->Web : NULL, FALSE),
                LoadStr(IDS_PLUGINSEC_ELEVATION),
                FormatFlag(security != NULL ? security->Elevation : NULL, TRUE),
                LoadStr(IDS_PLUGINSEC_SIGNER), signer,
                LoadStr(IDS_PLUGINSEC_HASH), hash,
                LoadStr(IDS_PLUGINSEC_SOURCE), source);
    lstrcpyn(buffer, line, bufferSize);
}

void PluginSecurityFormatForPlugin(const char* pluginId, const char* dllName, char* buffer, int bufferSize)
{
    char path[SAL_MAX_PATH];
    path[0] = 0;
    if (dllName != NULL && dllName[0] != 0)
    {
        if (dllName[0] == '\\' || (dllName[0] != 0 && dllName[1] == ':'))
            lstrcpyn(path, dllName, _countof(path));
        else
        {
            GetModuleFileName(NULL, path, _countof(path));
            char* slash = strrchr(path, '\\');
            if (slash != NULL)
                *(slash + 1) = 0;
            strncat_s(path, "plugins\\", _TRUNCATE);
            strncat_s(path, dllName, _TRUNCATE);
        }
    }
    char id[128];
    CopyField(id, _countof(id), pluginId);
    if (id[0] == 0 && dllName != NULL)
    {
        const char* name = strrchr(dllName, '\\');
        name = name != NULL ? name + 1 : dllName;
        lstrcpyn(id, name, _countof(id));
        char* dot = strrchr(id, '.');
        if (dot != NULL)
            *dot = 0;
    }
    FormatSecurityText(id, path, NULL, buffer, bufferSize);
}

void PluginSecurityFormatForExtension(const char* extensionId,
                                      const char* networkAccess,
                                      const char* externalProcesses,
                                      const char* scriptExecution,
                                      const char* activeWebContent,
                                      const char* elevation,
                                      char* buffer, int bufferSize)
{
    CPackageSecurity declared;
    memset(&declared, 0, sizeof(declared));
    CopyField(declared.Id, _countof(declared.Id), extensionId);
    CopyField(declared.Network, _countof(declared.Network), networkAccess);
    CopyField(declared.Processes, _countof(declared.Processes), externalProcesses);
    CopyField(declared.Script, _countof(declared.Script), scriptExecution);
    CopyField(declared.Web, _countof(declared.Web), activeWebContent);
    CopyField(declared.Elevation, _countof(declared.Elevation), elevation);
    const BOOL hasDeclared =
        declared.Network[0] != 0 || declared.Processes[0] != 0 ||
        declared.Script[0] != 0 || declared.Web[0] != 0 ||
        declared.Elevation[0] != 0;
    FormatSecurityText(
        extensionId, NULL, hasDeclared ? &declared : NULL, buffer, bufferSize);
}
