// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	scriptlist.cpp
	List of the scripts stored in the repositories.
*/

#include "precomp.h"
#include "scriptlist.h"
#include "extensionmanifest.h"
#include "scriptsite.h"
#include "aututils.h"
#include "engassoc.h"
#include "knownengines.h"
#include "automationplug.h"
#include "shim.h"
#include "abortpalette.h"
#include "abortmodal.h"
#include "lang\lang.rh"

extern HINSTANCE g_hInstance;
extern HINSTANCE g_hLangInst;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CAutomationPluginInterface g_oAutomationPlugin;

static BOOL ReadSmallTextFile(PCTSTR path, char* buffer, DWORD bufferSize)
{
    HANDLE hFile = HANDLES_Q(CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD bytesRead = 0;
    BOOL ok = ReadFile(hFile, buffer, bufferSize - 1, &bytesRead, NULL);
    HANDLES(CloseHandle(hFile));

    if (!ok || bytesRead == 0)
        return FALSE;

    buffer[bytesRead] = 0;
    return TRUE;
}

static BOOL ReadManifestTextFile(PCTSTR path, std::string& text)
{
    HANDLE hFile = HANDLES_Q(CreateFile(
        path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL));
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    LARGE_INTEGER fileSize;
    BOOL validSize =
        GetFileSizeEx(hFile, &fileSize) &&
        fileSize.QuadPart > 0 &&
        fileSize.QuadPart <= 1024 * 1024;
    if (!validSize)
    {
        HANDLES(CloseHandle(hFile));
        return FALSE;
    }

    text.resize(static_cast<size_t>(fileSize.QuadPart));
    DWORD bytesRead = 0;
    BOOL read = ReadFile(
        hFile, &text[0], static_cast<DWORD>(text.size()), &bytesRead, NULL);
    HANDLES(CloseHandle(hFile));
    if (!read || bytesRead != static_cast<DWORD>(text.size()))
    {
        text.clear();
        return FALSE;
    }
    return TRUE;
}

static BOOL Utf8ToNative(
    const std::string& value,
    PTSTR output,
    int outputCount)
{
    if (output == NULL || outputCount <= 0)
        return FALSE;

#ifdef UNICODE
    int converted = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, output, outputCount);
    if (converted == 0)
        output[0] = L'\0';
    return converted != 0;
#else
    int wideLength = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
    if (wideLength == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    std::vector<wchar_t> wideValue(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(), -1,
            &wideValue[0], wideLength) == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    int converted = WideCharToMultiByte(
        CP_ACP, 0, &wideValue[0], -1, output, outputCount, NULL, NULL);
    if (converted == 0)
        output[0] = '\0';
    return converted != 0;
#endif
}

static BOOL NativeToUtf8(PCTSTR value, char* output, int outputCount)
{
    if (value == NULL || output == NULL || outputCount <= 0)
        return FALSE;

#ifdef UNICODE
    int converted = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, output, outputCount,
        NULL, NULL);
    if (converted == 0)
        output[0] = '\0';
    return converted != 0;
#else
    int wideLength = MultiByteToWideChar(CP_ACP, 0, value, -1, NULL, 0);
    if (wideLength == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    std::vector<wchar_t> wideValue(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(
            CP_ACP, 0, value, -1, &wideValue[0], wideLength) == 0)
    {
        output[0] = '\0';
        return FALSE;
    }
    int converted = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, &wideValue[0], -1,
        output, outputCount, NULL, NULL);
    if (converted == 0)
        output[0] = '\0';
    return converted != 0;
#endif
}

static BOOL PathsEqual(PCTSTR first, PCTSTR second)
{
    TCHAR firstFull[MAX_PATH];
    TCHAR secondFull[MAX_PATH];
    DWORD firstLength = GetFullPathName(first, _countof(firstFull), firstFull, NULL);
    DWORD secondLength = GetFullPathName(second, _countof(secondFull), secondFull, NULL);
    if (firstLength == 0 || firstLength >= _countof(firstFull) ||
        secondLength == 0 || secondLength >= _countof(secondFull))
    {
        return FALSE;
    }
    return _tcsicmp(firstFull, secondFull) == 0;
}

static BOOL LoadManifestForEntryPoint(
    PCTSTR entryPointPath,
    CExtensionManifest& manifest)
{
    TCHAR directory[MAX_PATH];
    StringCchCopy(directory, _countof(directory), entryPointPath);
    if (!PathRemoveFileSpec(directory))
        return FALSE;

    for (int level = 0; level < 32; ++level)
    {
        TCHAR manifestPath[MAX_PATH];
        StringCchCopy(manifestPath, _countof(manifestPath), directory);
        if (!SalamanderGeneral->SalPathAppend(
                manifestPath, _T("extension.json"), _countof(manifestPath)))
        {
            return FALSE;
        }

        std::string json;
        if (ReadManifestTextFile(manifestPath, json))
        {
            CExtensionManifest candidate;
            CExtensionManifestError error;
            if (candidate.Parse(json.data(), json.size(), error))
            {
                TCHAR nativeEntryPoint[MAX_PATH];
                if (Utf8ToNative(candidate.EntryPoint, nativeEntryPoint, _countof(nativeEntryPoint)))
                {
                    TCHAR resolvedEntryPoint[MAX_PATH];
                    StringCchCopy(resolvedEntryPoint, _countof(resolvedEntryPoint), directory);
                    if (SalamanderGeneral->SalPathAppend(
                            resolvedEntryPoint, nativeEntryPoint, _countof(resolvedEntryPoint)) &&
                        PathsEqual(resolvedEntryPoint, entryPointPath))
                    {
                        manifest = candidate;
                        return TRUE;
                    }
                }
            }
        }

        TCHAR parent[MAX_PATH];
        StringCchCopy(parent, _countof(parent), directory);
        if (!PathRemoveFileSpec(parent) || _tcsicmp(parent, directory) == 0)
            break;
        StringCchCopy(directory, _countof(directory), parent);
    }
    return FALSE;
}

CScriptInfo::CScriptInfo(
    PCTSTR pszFileName,
    CScriptContainer* pContainer)
{
    PCTSTR pszNameStart, pszNameEnd;

    StringCchCopy(m_szFileName, _countof(m_szFileName), pszFileName);

    pszNameStart = PathFindFileName(pszFileName);
    pszNameEnd = PathFindExtension(pszNameStart);
    StringCchCopyN(m_szDisplayName, _countof(m_szDisplayName), pszNameStart, pszNameEnd - pszNameStart);
    m_szSalamatrixCommandId[0] = _T('\0');
    m_szSalamatrixExtensionId[0] = '\0';
    m_szSalamatrixRuntimeId[0] = '\0';
    m_dwSalamatrixMinimumRuntimeVersion = 0;
    m_bShowInPluginMenu = true;
    m_bShowInContextMenu = false;
    m_dwMenuEventOrMask = MENU_EVENT_TRUE;
    m_dwMenuEventAndMask = MENU_EVENT_TRUE;
    LoadSalamatrixMetadata();
    LoadSalamatrixManifestMetadata();

    m_clsidEngine = CLSID_NULL;

    m_pScript = NULL;
    m_pSite = NULL;
    m_pHardError = NULL;

    m_cExecuted = 0;

    m_pExecInfo = NULL;

    m_bAbortPending = false;
    m_hAbortEvent = NULL;
    m_hwndAbortTarget = NULL;
    m_pRuntimeSession = NULL;
    m_hRuntimePumpThread = NULL;
    memset(m_runtimeEventSubscriptions, 0, sizeof(m_runtimeEventSubscriptions));
    m_nRuntimeEventSubscriptions = 0;

    m_pNext = NULL;
    m_pNextHash = NULL;
    m_pContainer = pContainer;
    m_nId = 0;

    m_bDirty = false;

    m_pShim = NULL;
}

CScriptInfo::~CScriptInfo()
{
    ReleaseRuntimeSession();
    if (m_pScript != NULL)
    {
        m_pScript->Release();
    }
}

void CScriptInfo::ApplySalamatrixPlacement(PCTSTR pszValue)
{
    if (_tcsicmp(pszValue, _T("plugin")) == 0)
    {
        m_bShowInPluginMenu = true;
        m_bShowInContextMenu = false;
    }
    else if (_tcsicmp(pszValue, _T("context")) == 0)
    {
        m_bShowInPluginMenu = false;
        m_bShowInContextMenu = true;
    }
    else if (_tcsicmp(pszValue, _T("both")) == 0)
    {
        m_bShowInPluginMenu = true;
        m_bShowInContextMenu = true;
    }
    else if (_tcsicmp(pszValue, _T("none")) == 0)
    {
        m_bShowInPluginMenu = false;
        m_bShowInContextMenu = false;
    }
}

void CScriptInfo::ApplySalamatrixContextMenu(bool value)
{
    m_bShowInContextMenu = value;
}

void CScriptInfo::ApplySalamatrixRequires(PCTSTR pszValue)
{
    if (_tcsicmp(pszValue, _T("any")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_TRUE;
        m_dwMenuEventAndMask = MENU_EVENT_TRUE;
    }
    else if (_tcsicmp(pszValue, _T("disk")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_TRUE;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
    else if (_tcsicmp(pszValue, _T("focused")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_FILE_FOCUSED | MENU_EVENT_DIR_FOCUSED;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
    else if (_tcsicmp(pszValue, _T("file")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_FILE_FOCUSED | MENU_EVENT_FILES_SELECTED;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
    else if (_tcsicmp(pszValue, _T("selection")) == 0)
    {
        m_dwMenuEventOrMask = MENU_EVENT_FILES_SELECTED | MENU_EVENT_DIRS_SELECTED;
        m_dwMenuEventAndMask = MENU_EVENT_DISK;
    }
}

void CScriptInfo::ApplySalamatrixMetadataLine(PCTSTR pszLine)
{
    static const TCHAR COMMAND_ID_PREFIX[] = _T("// Salamatrix.CommandId:");
    static const TCHAR COMMAND_TITLE_PREFIX[] = _T("// Salamatrix.CommandTitle:");
    static const TCHAR COMMAND_MENU_PREFIX[] = _T("// Salamatrix.CommandMenu:");
    static const TCHAR COMMAND_CONTEXT_MENU_PREFIX[] = _T("// Salamatrix.CommandContextMenu:");
    static const TCHAR COMMAND_REQUIRES_PREFIX[] = _T("// Salamatrix.CommandRequires:");

    while (*pszLine == _T(' ') || *pszLine == _T('\t'))
        ++pszLine;

    if (_tcsnicmp(pszLine, COMMAND_ID_PREFIX, _countof(COMMAND_ID_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_ID_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        StringCchCopy(m_szSalamatrixCommandId, _countof(m_szSalamatrixCommandId), pszLine);
    }
    else if (_tcsnicmp(pszLine, COMMAND_TITLE_PREFIX, _countof(COMMAND_TITLE_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_TITLE_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        StringCchCopy(m_szDisplayName, _countof(m_szDisplayName), pszLine);
    }
    else if (_tcsnicmp(pszLine, COMMAND_MENU_PREFIX, _countof(COMMAND_MENU_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_MENU_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        ApplySalamatrixPlacement(pszLine);
    }
    else if (_tcsnicmp(pszLine, COMMAND_CONTEXT_MENU_PREFIX, _countof(COMMAND_CONTEXT_MENU_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_CONTEXT_MENU_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        ApplySalamatrixContextMenu(_tcsicmp(pszLine, _T("true")) == 0);
    }
    else if (_tcsnicmp(pszLine, COMMAND_REQUIRES_PREFIX, _countof(COMMAND_REQUIRES_PREFIX) - 1) == 0)
    {
        pszLine += _countof(COMMAND_REQUIRES_PREFIX) - 1;
        while (*pszLine == _T(' ') || *pszLine == _T('\t'))
            ++pszLine;
        ApplySalamatrixRequires(pszLine);
    }
}

void CScriptInfo::ApplySalamatrixManifestValue(const char* key, const char* value)
{
    if (value == NULL || value[0] == 0)
        return;

    TCHAR converted[256];
    if (!Utf8ToNative(value, converted, _countof(converted)))
        return;

    if (strcmp(key, "id") == 0)
    {
        StringCchCopy(m_szSalamatrixCommandId, _countof(m_szSalamatrixCommandId), converted);
    }
    else if (strcmp(key, "title") == 0 || strcmp(key, "name") == 0)
    {
        StringCchCopy(m_szDisplayName, _countof(m_szDisplayName), converted);
    }
    else if (strcmp(key, "menu") == 0 || strcmp(key, "placement") == 0)
    {
        ApplySalamatrixPlacement(converted);
    }
    else if (strcmp(key, "requires") == 0)
    {
        ApplySalamatrixRequires(converted);
    }
}

void CScriptInfo::LoadSalamatrixManifestMetadata()
{
    CExtensionManifest manifest;
    if (!LoadManifestForEntryPoint(m_szFileName, manifest))
        return;

    StringCchCopyA(
        m_szSalamatrixExtensionId,
        _countof(m_szSalamatrixExtensionId),
        manifest.Id.c_str());
    StringCchCopyA(
        m_szSalamatrixRuntimeId,
        _countof(m_szSalamatrixRuntimeId),
        manifest.RuntimeId.c_str());
    m_dwSalamatrixMinimumRuntimeVersion = manifest.MinimumRuntimeVersion;

    if (manifest.Commands.empty())
        return;

    const CExtensionManifestCommand& command = manifest.Commands[0];
    ApplySalamatrixManifestValue("id", command.Id.c_str());
    ApplySalamatrixManifestValue("title", command.Title.c_str());
    ApplySalamatrixManifestValue("menu", command.Menu.c_str());
    ApplySalamatrixContextMenu(command.ContextMenu);
    ApplySalamatrixManifestValue("requires", command.Requires.c_str());
}

void CScriptInfo::LoadSalamatrixMetadata()
{
    char buffer[4097];
    if (!ReadSmallTextFile(m_szFileName, buffer, sizeof(buffer)))
        return;

    char* line = buffer;
    while (line != NULL && *line != 0)
    {
        char* next = strchr(line, '\n');
        if (next != NULL)
        {
            *next = 0;
            ++next;
        }

        char* end = line + strlen(line);
        while (end > line && (end[-1] == '\r' || end[-1] == '\n'))
        {
            --end;
            *end = 0;
        }

#ifdef UNICODE
        TCHAR wideLine[512];
        MultiByteToWideChar(CP_ACP, 0, line, -1, wideLine, _countof(wideLine));
        wideLine[_countof(wideLine) - 1] = 0;
        ApplySalamatrixMetadataLine(wideLine);
#else
        ApplySalamatrixMetadataLine(line);
#endif

        line = next;
    }
}

PCTSTR CScriptInfo::GetFileExt() const
{
    return PathFindExtension(m_szFileName);
}

bool CScriptInfo::Execute(__inout EXECUTION_INFO& info)
{
    if (m_szSalamatrixRuntimeId[0] != '\0')
        return ExecuteThroughRuntime(info);

    return ExecuteLegacy(info);
}

bool CScriptInfo::ExecuteLegacy(__inout EXECUTION_INFO& info)
{
    if (!EnsureEngineAssociation())
    {
        return false;
    }

    //return ExecuteInSeparateThread(&info);
    return ExecuteWorker(&info);
}

struct CCompatibilityExecutionContext
{
    CScriptInfo* Script;
    CScriptInfo::EXECUTION_INFO* Info;
};

BOOL WINAPI CScriptInfo::ExecuteCompatibilityRuntime(
    void* context,
    Salamatrix::Runtime::RuntimeExecutionResult* result)
{
    CCompatibilityExecutionContext* execution =
        static_cast<CCompatibilityExecutionContext*>(context);
    if (execution == NULL || execution->Script == NULL ||
        execution->Info == NULL || result == NULL)
    {
        return FALSE;
    }

    bool succeeded = execution->Script->ExecuteLegacy(*execution->Info);
    result->Status = succeeded
                         ? Salamatrix::Runtime::RuntimeExecutionStatusSucceeded
                         : Salamatrix::Runtime::RuntimeExecutionStatusFailed;
    result->ErrorCode = succeeded ? S_OK : E_FAIL;
    return succeeded ? TRUE : FALSE;
}

bool CScriptInfo::ExecuteThroughRuntime(__inout EXECUTION_INFO& info)
{
    CAutomationSalamatrixBridge* bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasRuntimeBroker())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }

    Salamatrix::Runtime::IRuntimeService* runtimeService = bridge->GetRuntimeService();
    Salamatrix::Runtime::IRuntimeAdapter* adapter =
        runtimeService != NULL
            ? runtimeService->FindAdapter(
                  m_szSalamatrixRuntimeId,
                  m_dwSalamatrixMinimumRuntimeVersion)
            : NULL;

    char entryPointUtf8[MAX_PATH * 3];
    if (adapter == NULL ||
        !NativeToUtf8(m_szFileName, entryPointUtf8, _countof(entryPointUtf8)) ||
        !adapter->IsAvailable() ||
        !adapter->SupportsEntryPoint(entryPointUtf8))
    {
        TCHAR message[512];
        TCHAR runtimeId[128];
        Utf8ToNative(m_szSalamatrixRuntimeId, runtimeId, _countof(runtimeId));
        StringCchPrintf(
            message,
            _countof(message),
            _T("The extension requires runtime '%s', but no compatible and available adapter is registered."),
            runtimeId);
        SalamanderGeneral->SalMessageBox(
            SalamanderGeneral->GetMsgBoxParent(),
            message,
            SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
            MB_OK | MB_ICONERROR);
        return false;
    }

    wchar_t entryPointWide[MAX_PATH];
#ifdef UNICODE
    StringCchCopyW(entryPointWide, _countof(entryPointWide), m_szFileName);
#else
    if (MultiByteToWideChar(
            CP_ACP, 0, m_szFileName, -1, entryPointWide, _countof(entryPointWide)) == 0)
    {
        return false;
    }
#endif

    CCompatibilityExecutionContext compatibilityContext = {this, &info};
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = m_szSalamatrixExtensionId;
    request.CommandId = m_szSalamatrixCommandId;
    request.EntryPoint = entryPointWide;
    request.ParentWindow = SalamanderGeneral->GetMsgBoxParent();
    request.CompatibilityExecute = ExecuteCompatibilityRuntime;
    request.CompatibilityContext = &compatibilityContext;

    Salamatrix::Runtime::RuntimeExecutionResult result;
    BOOL executed = adapter->Execute(&request, &result);
    if (!executed &&
        result.Status != Salamatrix::Runtime::RuntimeExecutionStatusCancelled &&
        result.Message[0] != L'\0')
    {
        TCHAR message[_countof(result.Message)];
#ifdef UNICODE
        StringCchCopyW(message, _countof(message), result.Message);
#else
        WideCharToMultiByte(
            CP_ACP, 0, result.Message, -1, message, _countof(message), NULL, NULL);
        message[_countof(message) - 1] = '\0';
#endif
        SalamanderGeneral->SalMessageBox(
            request.ParentWindow,
            message,
            SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
            MB_OK | MB_ICONERROR);
    }
    return executed != FALSE &&
           result.Status == Salamatrix::Runtime::RuntimeExecutionStatusSucceeded;
}

static std::string JsonEscapeRuntimeText(const char* value)
{
    std::string escaped;
    if (value == NULL)
        return escaped;
    for (const unsigned char* character =
             reinterpret_cast<const unsigned char*>(value);
         *character != 0;
         ++character)
    {
        switch (*character)
        {
        case '"':
            escaped.append("\\\"");
            break;
        case '\\':
            escaped.append("\\\\");
            break;
        case '\b':
            escaped.append("\\b");
            break;
        case '\f':
            escaped.append("\\f");
            break;
        case '\n':
            escaped.append("\\n");
            break;
        case '\r':
            escaped.append("\\r");
            break;
        case '\t':
            escaped.append("\\t");
            break;
        default:
            if (*character < 0x20)
                escaped.push_back(' ');
            else
                escaped.push_back(static_cast<char>(*character));
            break;
        }
    }
    return escaped;
}

static BOOL CopyRuntimeHostResult(
    const std::string& value,
    char* output,
    DWORD outputCapacity,
    DWORD* outputLength)
{
    if (output == NULL || outputLength == NULL ||
        value.size() + 1 > outputCapacity)
        return FALSE;
    memcpy(output, value.c_str(), value.size());
    output[value.size()] = '\0';
    *outputLength = static_cast<DWORD>(value.size());
    return TRUE;
}

static const char* RuntimeEventName(
    Salamatrix::Events::EventKind kind)
{
    switch (kind)
    {
    case Salamatrix::Events::EventKindHostStartup:
        return "hostStartup";
    case Salamatrix::Events::EventKindHostShutdown:
        return "hostShutdown";
    case Salamatrix::Events::EventKindSettingsChanged:
        return "settingsChanged";
    case Salamatrix::Events::EventKindConfigurationChanged:
        return "configurationChanged";
    case Salamatrix::Events::EventKindColorsChanged:
        return "colorsChanged";
    case Salamatrix::Events::EventKindPanelsSwapped:
        return "panelsSwapped";
    case Salamatrix::Events::EventKindActivePanelChanged:
        return "activePanelChanged";
    default:
        return NULL;
    }
}

static BOOL RuntimeEventKindFromName(
    const std::string& name,
    Salamatrix::Events::EventKind* kind)
{
    if (kind == NULL)
        return FALSE;
    for (int value = Salamatrix::Events::EventKindHostStartup;
         value <= Salamatrix::Events::EventKindActivePanelChanged;
         ++value)
    {
        Salamatrix::Events::EventKind candidate =
            static_cast<Salamatrix::Events::EventKind>(value);
        const char* candidateName = RuntimeEventName(candidate);
        if (candidateName != NULL && _stricmp(name.c_str(), candidateName) == 0)
        {
            *kind = candidate;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL WINAPI CScriptInfo::RuntimeEventCallback(
    void* context,
    const Salamatrix::Events::EventPayload* payload)
{
    CScriptInfo* script = static_cast<CScriptInfo*>(context);
    if (script == NULL || payload == NULL || script->m_pRuntimeSession == NULL)
        return FALSE;

    const char* name = RuntimeEventName(payload->Kind);
    if (name == NULL)
        return FALSE;
    char tabId[32];
    _ui64toa_s(payload->ActiveTabId, tabId, _countof(tabId), 10);
    std::string eventJson =
        std::string("{\"event\":\"") + name +
        "\",\"parameter\":" + std::to_string(payload->Parameter) +
        ",\"activePanel\":" + std::to_string(payload->ActivePanel) +
        ",\"tabId\":\"" + tabId +
        "\",\"pathType\":" + std::to_string(payload->PathType) +
        ",\"path\":\"" + JsonEscapeRuntimeText(payload->Path) + "\"}";
    std::string frame;
    if (!Salamatrix::Runtime::Protocol::LineCodec::Encode(
            Salamatrix::Runtime::Protocol::MessageEvent,
            0,
            eventJson,
            &frame))
        return FALSE;
    return script->m_pRuntimeSession->SendFrame(
        frame.c_str(), static_cast<DWORD>(frame.size()));
}

BOOL WINAPI CScriptInfo::RuntimeHostDispatch(
    void* context,
    Salamatrix::Runtime::Protocol::MessageType type,
    ULONGLONG requestId,
    const char* payloadJson,
    char* resultJson,
    DWORD resultCapacity,
    DWORD* resultLength)
{
    (void)requestId;
    CScriptInfo* script = static_cast<CScriptInfo*>(context);
    if (script == NULL || resultJson == NULL || resultLength == NULL)
        return FALSE;
    *resultLength = 0;

    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->WasQueried())
        g_oAutomationPlugin.RefreshSalamatrixServices();

    if (type == Salamatrix::Runtime::Protocol::MessageHello)
    {
        std::string response =
            "{\"ok\":true,\"protocol\":1,\"extensionId\":\"" +
            JsonEscapeRuntimeText(script->m_szSalamatrixExtensionId) +
            "\",\"services\":[\"commands\",\"sides\",\"storage\"]}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (type != Salamatrix::Runtime::Protocol::MessageCall ||
        payloadJson == NULL)
        return FALSE;

    std::string method;
    if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "method", &method))
        return FALSE;

    if (method == "runtime.ready")
    {
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"ready\":true}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    if (method == "salamander.ui.messageBox")
    {
        std::string message;
        std::string title;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "message", &message))
            return FALSE;
        Salamatrix::Runtime::Protocol::Json::FindStringMember(
            payloadJson, "title", &title);
        if (title.empty())
            title = "Salamatrix";
        int result = MessageBoxA(
            SalamanderGeneral->GetMsgBoxParent(),
            message.c_str(),
            title.c_str(),
            MB_OK | MB_ICONINFORMATION);
        std::string response =
            "{\"ok\":true,\"result\":" + std::to_string(result) + "}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.events.subscribe")
    {
        std::string eventName;
        Salamatrix::Events::EventKind kind;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "event", &eventName) ||
            !RuntimeEventKindFromName(eventName, &kind) ||
            script->m_nRuntimeEventSubscriptions >=
                static_cast<int>(_countof(script->m_runtimeEventSubscriptions)))
            return FALSE;
        Salamatrix::Events::IEventsService* events = bridge->GetEventsService();
        if (events == NULL)
            return FALSE;
        ULONGLONG subscriptionId = 0;
        if (!events->Subscribe(
                kind,
                CScriptInfo::RuntimeEventCallback,
                script,
                &subscriptionId))
            return FALSE;
        script->m_runtimeEventSubscriptions[
            script->m_nRuntimeEventSubscriptions++] = subscriptionId;
        char id[32];
        _ui64toa_s(subscriptionId, id, _countof(id), 10);
        std::string response =
            std::string("{\"ok\":true,\"subscriptionId\":\"") + id + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.events.unsubscribe")
    {
        std::string idText;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "subscriptionId", &idText))
            return FALSE;
        char* end = NULL;
        ULONGLONG subscriptionId = _strtoui64(idText.c_str(), &end, 10);
        if (end == idText.c_str() || *end != '\0')
            return FALSE;
        Salamatrix::Events::IEventsService* events = bridge->GetEventsService();
        if (events == NULL || !events->Unsubscribe(subscriptionId))
            return FALSE;
        for (int index = 0; index < script->m_nRuntimeEventSubscriptions; ++index)
        {
            if (script->m_runtimeEventSubscriptions[index] == subscriptionId)
            {
                for (int move = index;
                     move + 1 < script->m_nRuntimeEventSubscriptions;
                     ++move)
                {
                    script->m_runtimeEventSubscriptions[move] =
                        script->m_runtimeEventSubscriptions[move + 1];
                }
                --script->m_nRuntimeEventSubscriptions;
                break;
            }
        }
        return CopyRuntimeHostResult(
            "{\"ok\":true}", resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.commands.execute")
    {
        std::string commandId;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "commandId", &commandId))
            return FALSE;
        Salamatrix::Commands::ICommandService* commands =
            bridge->GetCommandService();
        if (commands == NULL)
            return FALSE;
        Salamatrix::Commands::ExecuteOptions options;
        options.Parent = SalamanderGeneral->GetMsgBoxParent();
        Salamatrix::Runtime::OperationResult operation =
            commands->Execute(commandId.c_str(), options);
        const char* resultName =
            operation == Salamatrix::Runtime::OperationResultOk
                ? "ok"
                : operation == Salamatrix::Runtime::OperationResultNotAvailable
                      ? "not_available"
                      : "error";
        std::string response =
            "{\"ok\":" +
            std::string(operation == Salamatrix::Runtime::OperationResultOk
                            ? "true"
                            : "false") +
            ",\"result\":\"" + resultName + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.sides.activeTab")
    {
        std::string sideName;
        Salamatrix::Sides::SideReference side =
            Salamatrix::Sides::SideReferenceSource;
        if (Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "side", &sideName))
        {
            if (_stricmp(sideName.c_str(), "left") == 0)
                side = Salamatrix::Sides::SideReferenceLeft;
            else if (_stricmp(sideName.c_str(), "right") == 0)
                side = Salamatrix::Sides::SideReferenceRight;
            else if (_stricmp(sideName.c_str(), "target") == 0)
                side = Salamatrix::Sides::SideReferenceTarget;
        }
        Salamatrix::Sides::ISidesService* sides = bridge->GetSidesService();
        if (sides == NULL)
            return FALSE;
        Salamatrix::Sides::TabInfo info;
        if (!sides->GetActiveTabInfo(side, &info))
            return FALSE;
        char path[32768];
        int pathType = info.PathType;
        if (!sides->GetTabPath(info.TabId, path, _countof(path), &pathType))
            path[0] = '\0';
        char id[32];
        _ui64toa_s(info.TabId, id, _countof(id), 10);
        std::string response =
            std::string("{\"ok\":true,\"id\":\"") + id +
            "\",\"index\":" + std::to_string(info.Index) +
            ",\"pathType\":" + std::to_string(pathType) +
            ",\"path\":\"" + JsonEscapeRuntimeText(path) + "\"}";
        return CopyRuntimeHostResult(
            response, resultJson, resultCapacity, resultLength);
    }

    if (method == "salamander.storage.get" ||
        method == "salamander.storage.set")
    {
        std::string key;
        if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                payloadJson, "key", &key))
            return FALSE;
        Salamatrix::Storage::IStorageService* storage =
            bridge->GetStorageService();
        if (storage == NULL || script->m_szSalamatrixExtensionId[0] == '\0')
            return FALSE;
        if (method == "salamander.storage.set")
        {
            std::string value;
            if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    payloadJson, "value", &value) ||
                !storage->SetString(
                    script->m_szSalamatrixExtensionId,
                    key.c_str(),
                    value.c_str()))
                return FALSE;
            return CopyRuntimeHostResult(
                "{\"ok\":true}", resultJson, resultCapacity, resultLength);
        }
        char value[16385];
        int required = 0;
        if (storage->GetString(
                script->m_szSalamatrixExtensionId,
                key.c_str(),
                value,
                _countof(value),
                &required))
        {
            std::string response =
                "{\"ok\":true,\"type\":\"string\",\"value\":\"" +
                JsonEscapeRuntimeText(value) + "\"}";
            return CopyRuntimeHostResult(
                response, resultJson, resultCapacity, resultLength);
        }
        return CopyRuntimeHostResult(
            "{\"ok\":true,\"type\":\"missing\"}",
            resultJson,
            resultCapacity,
            resultLength);
    }

    return FALSE;
}

BOOL WINAPI CScriptInfo::RuntimeLifecycleCallback(
    void* context,
    Salamatrix::Extensions::ExtensionAction action,
    const Salamatrix::Extensions::ExtensionInfo* info)
{
    CScriptInfo* script = static_cast<CScriptInfo*>(context);
    if (script == NULL || info == NULL)
        return FALSE;

    if (action == Salamatrix::Extensions::ExtensionActionDeactivate)
    {
        script->ReleaseRuntimeSession();
        return TRUE;
    }

    if (action != Salamatrix::Extensions::ExtensionActionActivate)
        return FALSE;
    if (script->m_pRuntimeSession != NULL)
    {
        if (script->m_pRuntimeSession->IsAlive())
            return TRUE;
        script->ReleaseRuntimeSession();
    }

    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasRuntimeBroker())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }
    Salamatrix::Runtime::IRuntimeService* runtime =
        bridge->GetRuntimeService();
    Salamatrix::Runtime::IRuntimeAdapter* adapter =
        runtime != NULL
            ? runtime->FindAdapter(
                  script->m_szSalamatrixRuntimeId,
                  script->m_dwSalamatrixMinimumRuntimeVersion)
            : NULL;
    if (adapter == NULL)
        return FALSE;

    wchar_t entryPoint[MAX_PATH];
#ifdef UNICODE
    if (StringCchCopyW(entryPoint, _countof(entryPoint), script->m_szFileName) != S_OK)
        return FALSE;
#else
    if (MultiByteToWideChar(
            CP_ACP, 0, script->m_szFileName, -1,
            entryPoint, _countof(entryPoint)) == 0)
        return FALSE;
#endif

    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = script->m_szSalamatrixExtensionId;
    request.EntryPoint = entryPoint;
    request.ParentWindow = SalamanderGeneral->GetMsgBoxParent();
    request.Flags =
        Salamatrix::Runtime::RuntimeExecutionFlagPersistentWorker |
        Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap;
    request.HostDispatch = CScriptInfo::RuntimeHostDispatch;
    request.HostDispatchContext = script;

    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    if (!adapter->StartPersistent(&request, &session) || session == NULL)
        return FALSE;
    if (!session->IsAlive())
    {
        session->Release();
        return FALSE;
    }
    script->m_pRuntimeSession = session;
    script->m_hRuntimePumpThread = CreateThread(
        NULL, 0, CScriptInfo::RuntimePumpProc, script, 0, NULL);
    if (script->m_hRuntimePumpThread == NULL)
    {
        script->ReleaseRuntimeSession();
        return FALSE;
    }
    return TRUE;
}

bool CScriptInfo::EnsureEngineAssociation()
{
    if (IsEqualGUID(m_clsidEngine, GUID_NULL))
    {
        if (!g_oScriptAssociations.FindEngineByExt(
                PathFindExtension(m_szFileName),
                &m_clsidEngine))
        {
            // TODO: report a message
            return false;
        }
    }

    return true;
}

bool CScriptInfo::CreateEngine(EXECUTION_INFO* info)
{
    HRESULT hr;
    IActiveScriptParse* pParse;

    if (m_pScript)
    {
        return true;
    }

    hr = CoCreateInstance(m_clsidEngine, NULL, CLSCTX_INPROC_SERVER,
                          __uuidof(m_pScript), (void**)&m_pScript);
    if (SUCCEEDED(hr))
    {
        _ASSERTE(m_pSite == NULL);
        m_pSite = new CScriptSite(this);
        _ASSERTE(m_pSite);

        m_pShim = CScriptEngineShim::Create(this);

        if (info->bEnableDebugger)
        {
            InitializeDebugger(&info->dbgInfo);
        }

        m_hAbortEvent = HANDLES(CreateEvent(NULL, TRUE, FALSE, NULL));

        // BUG 103: ActiveRuby calls OnEnterScript very early
        // during parsing.
        m_pSite->SetExecutionInfo(info);

        hr = m_pScript->SetScriptSite(m_pSite);

        if (SUCCEEDED(hr))
        {
            hr = m_pScript->QueryInterface(&pParse);
            if (SUCCEEDED(hr))
            {
                hr = pParse->InitNew();

                if (SUCCEEDED(hr))
                {
                    hr = m_pScript->AddNamedItem(
                        L"Salamander",
                        SCRIPTITEM_ISVISIBLE); // These are the flags cscript.exe uses.

                    if (SUCCEEDED(hr))
                    {
                        hr = LoadScript(pParse, info);
                    }
                }

                pParse->Release();
            }
        }
    }

    if (FAILED(hr))
    {
        delete m_pShim;

        if (m_pScript != NULL)
        {
            m_pScript->Release();
            m_pScript = NULL;
        }

        if (m_pSite != NULL)
        {
            m_pSite->Release();
            m_pSite = NULL;
        }

        HANDLES(CloseHandle(m_hAbortEvent));
        m_hAbortEvent = NULL;

        TRACE_E("CreateEngine failed with error " << std::hex << hr);

        return false;
    }

    return true;
}

HRESULT CScriptInfo::LoadScript(IActiveScriptParse* pParse, EXECUTION_INFO* info)
{
    HRESULT hr;
    LPOLESTR pszCode;
    EXCEPINFO ei;
    ULONG cch;
    TCHAR szExpanded[MAX_PATH];

    if (!g_oAutomationPlugin.ExpandPath(m_szFileName, szExpanded,
                                        _countof(szExpanded)))
    {
        return HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND);
    }

    hr = LoadOleStringFromFile(szExpanded, pszCode, &cch);
    if (FAILED(hr))
    {
        TCHAR szMessage[256];
        TCHAR szError[192];

        FormatErrorText(hr, szError, _countof(szError));
        StringCchPrintf(szMessage, _countof(szMessage),
                        SalamanderGeneral->LoadStr(g_hLangInst, IDS_LOADERRFMT),
                        PathFindFileName(m_szFileName), szError);

        SalamanderGeneral->ShowMessageBox(
            szMessage,
            SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
            MSGBOX_ERROR);

        return hr;
    }

    if (SUCCEEDED(hr) && info->dbgInfo.pDbgDocHelper != NULL)
    {
        HRESULT hrdbg;
        IDebugDocumentHelper* pddh = info->dbgInfo.pDbgDocHelper;

        IDebugDocumentHost* phost;
        hrdbg = m_pSite->QueryInterface(__uuidof(phost), (void**)&phost);
        if (SUCCEEDED(hrdbg))
        {
            hrdbg = pddh->SetDebugDocumentHost(phost);
            phost->Release();
        }

        hrdbg = pddh->AddUnicodeText(pszCode);
        if (SUCCEEDED(hrdbg))
        {
            hrdbg = pddh->DefineScriptBlock(0, cch, m_pScript, FALSE, &info->dbgInfo.dwSourceContext);
        }
    }

    hr = pParse->ParseScriptText(pszCode, NULL, NULL, NULL, 0, 0, SCRIPTTEXT_HOSTMANAGESSOURCE, NULL, &ei);

    FreeOleString(pszCode);

    if (FAILED(hr) && hr != SCRIPT_E_REPORTED)
    {
        DisplayException(ei);
    }

    return hr;
}

HRESULT CScriptInfo::LoadOleStringFromFile(PCTSTR pszFileName, __out LPOLESTR& s, __out_opt ULONG* cch)
{
    HANDLE hFile, hMapping;
    DWORD cbSize;
    char* pszCodeA;
    HRESULT hr;
    int cchRequired, cchConverted;

    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    cbSize = GetFileSize(hFile, NULL);
    if (cbSize == 0)
    {
        CloseHandle(hFile);
        s = (LPOLESTR)malloc(sizeof(WCHAR));
        if (s == NULL)
        {
            return E_OUTOFMEMORY;
        }

        *s = L'\0';
        return S_OK;
    }

    hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hFile);
    if (hMapping == NULL)
    {
        return hr;
    }

    pszCodeA = (char*)MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
    hr = HRESULT_FROM_WIN32(GetLastError());
    CloseHandle(hMapping);
    if (pszCodeA == NULL)
    {
        return hr;
    }

    cchRequired = MultiByteToWideChar(CP_ACP, 0, pszCodeA, cbSize, NULL, 0);
    if (cchRequired <= 0)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        UnmapViewOfFile(pszCodeA);
        return hr;
    }

    s = (LPOLESTR)malloc((cchRequired + 1) * sizeof(WCHAR));
    if (s == NULL)
    {
        UnmapViewOfFile(pszCodeA);
        return E_OUTOFMEMORY;
    }

    cchConverted = MultiByteToWideChar(CP_ACP, 0, pszCodeA, cbSize, s, cchRequired);
    hr = HRESULT_FROM_WIN32(GetLastError());
    UnmapViewOfFile(pszCodeA);
    if (cchConverted <= 0)
    {
        free(s);
        return hr;
    }

    // nul terminate
    s[cchConverted] = L'\0';

    if (cch != NULL)
    {
        *cch = cchConverted;
    }

    return S_OK;
}

bool CScriptInfo::ExecuteWorker(EXECUTION_INFO* info)
{
    HRESULT hr = S_OK;

    CALL_STACK_MESSAGE2("CScriptInfo::ExecuteWorker() (file name = \"%s\")", m_szFileName);

    info->bDeselect = false;

    m_pExecInfo = info;
    m_bAbortPending = false;

    _ASSERTE(m_pHardError == NULL);

    m_bSiteErrorDisplayed = false;
    if (!CreateEngine(info))
    {
        // If the error occured during parsing and was already displayed
        // through the site's OnScriptError event, don't display it here.
        // Otherwise display generic error message here.
        if (!m_bSiteErrorDisplayed)
        {
            SalamanderGeneral->SalMessageBox(
                SalamanderGeneral->GetMsgBoxParent(),
                SalamanderGeneral->LoadStr(g_hLangInst, IDS_ENGINECREATEFAIL),
                SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
                MB_OK | MB_ICONERROR);
        }

        return false;
    }

    // workaround for WshShell.SendKeys to work properly (by john):
    // Salamander is not responding properly on SendKeys (http://msdn.microsoft.com/en-us/library/8c6yea83%28VS.85%29.aspx)
    // when Ctrl/Shift/Alt is still pressed (for example when script was started using Ctrl+Shift+Z hot key).
    // Miranda global hot key (Ctrl+Shift+A) is triggered on (salamander ->) Ctrl+Shift+Z -> (script started ->) SendKeys("a").
    // Attempt to release pressed Ctrl/Shift/Alt using SetKeyboardState() doesn't work.
    // Note: it seems that SendKeys is using API ::SendInput() beacause it doesn't work too (Miranda is activated).
    // Fortunately, following hack works pretty well.
    ResetKeyboardState();

    m_pShim->BeginExecution();

    // Run the script.
    // We used to have only SetScriptState(SCRIPTSTATE_CONNECTED) here, but tracing
    // the cscript.exe revealed, that it calls SCRIPTSTATE_INITIALIZED immediately
    // followed by SCRIPTSTATE_STARTED. We changed the logic here to match the one
    // of cscript.exe more closely. This was done primarily because of RScript22
    // won't execute a script without SCRIPTSTATE_STARTED. Hopefully, this won't
    // break other engines (JScript and VBScript seems fine).
    hr = m_pScript->SetScriptState(SCRIPTSTATE_INITIALIZED);
    if (SUCCEEDED(hr))
    {
        hr = m_pScript->SetScriptState(SCRIPTSTATE_STARTED);
    }

    // ActivePython returns SCRIPT_E_REPORTED if there was parse error.
    // If debugging is enabled, the SCRIPT_E_PROPAGATE may be returned
    // if debugger is detached while an exception is being debugged.
    _ASSERTE(SUCCEEDED(hr) || hr == SCRIPT_E_REPORTED || hr == SCRIPT_E_PROPAGATE || m_pHardError);

    m_pSite->SetExecutionInfo(NULL);
    UninitializeDebugger(&info->dbgInfo);
    m_pShim->EndExecution();

    // cscript.exe doesn't call SetScriptState at all at the end of the execution. But it can
    // afford not calling it because the process ends afterwards. We must deal with buggy
    // engines and do need to uninitialize it.
    hr = m_pScript->SetScriptState(SCRIPTSTATE_INITIALIZED);
    _ASSERTE(SUCCEEDED(hr));

    if (m_pHardError)
    {
        DisplayException(m_pHardError, m_pShim);
        m_pHardError->Release();
        m_pHardError = NULL;
    }

    hr = m_pShim->ReleaseEngine(m_pScript);
    _ASSERTE(SUCCEEDED(hr));

    m_pScript = NULL;

    m_pSite->Release();
    m_pSite = NULL;

    m_pExecInfo = NULL;

    HANDLES(CloseHandle(m_hAbortEvent));
    m_hAbortEvent = NULL;

    delete m_pShim;
    m_pShim = NULL;

    return SUCCEEDED(hr);
}

bool CScriptInfo::ExecuteInSeparateThread(EXECUTION_INFO* info)
{
    HANDLE hThread;

    info->pInstance = this;
    info->bAsyncResult = false;

    hThread = CreateThread(NULL, 0, ExecuteEntryProc, info, 0, NULL);
    if (hThread != NULL)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    return info->bAsyncResult;
}

DWORD WINAPI CScriptInfo::ExecuteEntryProc(void* arg)
{
    HRESULT hr;
    CScriptInfo* that;
    EXECUTION_INFO* info;

    info = reinterpret_cast<EXECUTION_INFO*>(arg);
    that = reinterpret_cast<CScriptInfo*>(info->pInstance);

    hr = CoInitializeEx(NULL, /*COINIT_MULTITHREADED*/ COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr))
    {
        info->bAsyncResult = that->ExecuteWorker(info);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        CoUninitialize();
    }

    return 0;
}

void CScriptInfo::InitializeDebugger(DEBUG_INFO* dbgInfo)
{
    HRESULT hr;

    hr = CoCreateInstance(
        CLSID_ProcessDebugManager,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        __uuidof(IProcessDebugManager),
        (void**)&dbgInfo->pProcDbgMgr);
    if (FAILED(hr))
        return;

    hr = dbgInfo->pProcDbgMgr->GetDefaultApplication(&dbgInfo->pDbgApp);
    if (FAILED(hr))
        return;

    hr = dbgInfo->pProcDbgMgr->CreateDebugDocumentHelper(NULL, &dbgInfo->pDbgDocHelper);
    if (FAILED(hr))
        return;

    OLECHAR szUrl[2 * MAX_PATH];
    DWORD cchUrl = _countof(szUrl);
    A2OLE sFileNameW(m_szFileName);
    if (FAILED(UrlCreateFromPathW(A2OLE(sFileNameW), szUrl, &cchUrl, 0)))
    {
        StringCchCopyW(szUrl, _countof(szUrl), sFileNameW);
    }
    hr = dbgInfo->pDbgDocHelper->Init(dbgInfo->pDbgApp,
                                      A2OLE(GetDisplayName()), szUrl, TEXT_DOC_ATTR_READONLY);
    if (FAILED(hr))
        return;

    hr = dbgInfo->pDbgDocHelper->Attach(NULL);
}

void CScriptInfo::UninitializeDebugger(DEBUG_INFO* dbgInfo)
{
    if (dbgInfo->pDbgDocHelper)
    {
        dbgInfo->pDbgDocHelper->Detach();
        dbgInfo->pDbgDocHelper->Release();
    }

    if (dbgInfo->pDbgApp)
    {
        dbgInfo->pDbgApp->Release();
    }

    if (dbgInfo->pProcDbgMgr)
    {
        dbgInfo->pProcDbgMgr->Release();
    }

    memset(dbgInfo, 0, sizeof(DEBUG_INFO));
}

HRESULT CScriptInfo::AbortScript()
{
    HRESULT hr;

    _ASSERTE(m_pScript);
    _ASSERTE(m_pShim);

    hr = m_pShim->InterruptScript(m_pScript);

    if (!m_bAbortPending)
    {
        m_bAbortPending = true;

        // Cooperatively abort native execution (e.g. quit modal
        // message boxes, exit GUI loops, cancel sleeps, break
        // enumerators etc.).

        // Set the abort event, so the kernel waits can exit (e.g.
        // Salamander.Sleep()).
        _ASSERTE(m_hAbortEvent != NULL);
        SetEvent(m_hAbortEvent);

        HWND hwndAbortTarget = *(volatile HWND*)&m_hwndAbortTarget;
        if (hwndAbortTarget != NULL)
        {
            PostMessage(hwndAbortTarget, WM_SAL_ABORTMODAL, 0, 0);
        }
    }

    return hr;
}

void CScriptInfo::ScriptEnter()
{
    HWND hwndPalette;

    _ASSERTE(m_pExecInfo);
    _ASSERTE(m_pExecInfo->pAbortPalette == NULL);

    if (m_pExecInfo->pAbortPalette == NULL)
    {
        m_pExecInfo->pAbortPalette = new CScriptAbortPalette(this);
        hwndPalette = m_pExecInfo->pAbortPalette->GetHwnd();
        _ASSERTE(IsWindow(hwndPalette));

        // Make the main window inaccessible, since the script may display
        // modeless window and the user can unload the whole plugin from
        // the main window in the mean time.
        // NOTE: 'hwndPalette' mechanism is not used because WS_EX_TOPMOST with process tree
        // checking using WindowBelongsToProcessID() look like better solution for now.
        // For example unpack script is starting several command prompt windows
        // so WS_EX_TOPMOST toolbar is better accessible.
        SalamanderGeneral->LockMainWindow(TRUE, NULL, SalamanderGeneral->LoadStr(g_hLangInst, IDS_MAINWINDOWLOCKED));
    }
}

void CScriptInfo::ScriptLeave()
{
    if (m_pExecInfo != NULL)
    {
        // Enable the main window again.
        SalamanderGeneral->LockMainWindow(FALSE, NULL, NULL);

        delete m_pExecInfo->pAbortPalette;
        m_pExecInfo->pAbortPalette = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////

CScriptContainer::CScriptContainer()
{
    m_pParent = NULL;
    m_pSibling = NULL;
    m_pChild = NULL;
    m_pScripts = NULL;
    m_szPath[0] = _T('\0');
    m_pszName = NULL;
}

CScriptContainer::CScriptContainer(
    CScriptContainer* pParent,
    PCTSTR pszPath,
    bool bFullPath)
{
    m_pParent = pParent;
    m_pSibling = NULL;
    m_pChild = NULL;
    m_pScripts = NULL;

    if (bFullPath)
    {
        StringCchCopy(m_szPath, _countof(m_szPath), pszPath);
    }
    else
    {
        _ASSERTE(pParent);
        StringCchCopy(m_szPath, _countof(m_szPath), pParent->m_szPath);
        SalamanderGeneral->SalPathAppend(m_szPath, pszPath, _countof(m_szPath));
    }

    SalamanderGeneral->SalPathRemoveBackslash(m_szPath);
    m_pszName = PathFindFileName(m_szPath);
}

CScriptContainer::~CScriptContainer()
{
}

CScriptContainer* CScriptContainer::FirstChild(
    PCTSTR pszPath,
    bool bFullPath)
{
    CScriptContainer* pIter;
    TCHAR szFullPath[MAX_PATH];

    if (m_pChild == NULL)
    {
        return NULL;
    }

    pIter = m_pChild;

    if (bFullPath)
    {
        StringCchCopy(szFullPath, _countof(szFullPath), pszPath);
    }
    else
    {
        StringCchCopy(szFullPath, _countof(szFullPath), m_szPath);
        SalamanderGeneral->SalPathAppend(szFullPath, pszPath, _countof(szFullPath));
    }

    while (pIter)
    {
        if (_tcsicmp(pIter->m_szPath, szFullPath) == 0)
        {
            return pIter;
        }

        pIter = pIter->m_pSibling;
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

CScriptLookup::CScriptLookup()
{
    m_pRootContainer = NULL;
    memset(m_apHashBins, 0, sizeof(m_apHashBins));
    m_cScriptsTotal = 0;
    m_bModified = false;
    m_dwLastRefreshTime = 0;
}

CScriptLookup::~CScriptLookup()
{
    if (m_pRootContainer)
    {
        CascadeDeleteContainer(m_pRootContainer);
    }

    CScriptInfo *pIter, *pTmp;
    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        pIter = m_apHashBins[iBin];
        while (pIter)
        {
            pTmp = pIter->m_pNextHash;
            delete pIter;
            pIter = pTmp;
        }
    }
}

bool CScriptLookup::Load(HKEY hKey, CSalamanderRegistryAbstract* registry)
{
    int cDirs;
    int iDir;

    if (m_pRootContainer == NULL)
    {
        m_pRootContainer = new CScriptContainer();
    }

    if (hKey != INVALID_HANDLE_VALUE)
    {
        // not a refresh
        m_bModified = false;
    }

    cDirs = g_oAutomationPlugin.GetScriptDirectoryCount();
    for (iDir = 0; iDir < cDirs; iDir++)
    {
        CScriptContainer* pContainer;
        bool bExisting;

        pContainer = m_pRootContainer->FirstChild(
            g_oAutomationPlugin.GetScriptDirectoryRaw(iDir), true);
        bExisting = (pContainer != NULL);

        if (!bExisting)
        {
            pContainer = new CScriptContainer(m_pRootContainer,
                                              g_oAutomationPlugin.GetScriptDirectoryRaw(iDir), true);
        }

        if (FillContainer(pContainer, hKey, registry) > 0)
        {
            if (!bExisting)
            {
                LinkContainer(pContainer, m_pRootContainer);
            }
        }
        else if (!bExisting)
        {
            delete pContainer;
        }
    }

    m_dwLastRefreshTime = GetTickCount();

    return true;
}

bool CScriptLookup::Save(HKEY hKey, CSalamanderRegistryAbstract* registry)
{
    _ASSERTE(hKey != NULL);
    _ASSERTE(registry != NULL);

    if (!registry->ClearKey(hKey))
    {
        return false;
    }

    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        if (m_apHashBins[iBin] != NULL)
        {
            SaveBin(m_apHashBins[iBin], hKey, registry);
        }
    }

    m_bModified = false;

    return true;
}

bool CScriptLookup::SaveBin(
    CScriptInfo* pFirst,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    TCHAR szName[8];
    HKEY hkSub = NULL;
    UINT nPrevHash = 0;
    UINT nHash;
    CScriptInfo* pIter;

    for (pIter = pFirst; pIter; pIter = pIter->m_pNextHash)
    {
        nHash = HashFromId(pIter->m_nId);
        if (nHash != nPrevHash)
        {
            if (hkSub != NULL)
            {
                registry->CloseKey(hkSub);
                hkSub = NULL;
            }

            StringCchPrintf(szName, _countof(szName), "%06X", nHash);
            if (!registry->CreateKey(hKey, szName, hkSub))
            {
                return false;
            }

            nPrevHash = nHash;
        }

        StringCchPrintf(szName, _countof(szName), "%02X", UniquierFromId(pIter->m_nId));
        registry->SetValue(hkSub, szName, REG_SZ, pIter->GetFileName(), -1);
    }

    if (hkSub != NULL)
    {
        registry->CloseKey(hkSub);
    }

    return true;
}

CScriptInfo* CScriptLookup::LookupScript(int nId)
{
    int iBin;
    CScriptInfo* pScript;

    iBin = HashFromId(nId) % _countof(m_apHashBins);
    pScript = m_apHashBins[iBin];
    while (pScript)
    {
        if (pScript->m_nId == nId)
        {
            return pScript;
        }
        else if (pScript->m_nId > nId)
        {
            break;
        }

        pScript = pScript->m_pNextHash;
    }

    return NULL;
}

int CScriptLookup::FillContainer(
    CScriptContainer* pContainer,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    HANDLE hFind;
    TCHAR szPattern[MAX_PATH];
    WIN32_FIND_DATA fd;
    int cScripts = 0;

    g_oAutomationPlugin.ExpandPath(pContainer->GetPath(), szPattern, _countof(szPattern));
    SalamanderGeneral->SalPathAppend(szPattern, _T("*"), _countof(szPattern));

    hFind = FindFirstFile(szPattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    do
    {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        {
            continue;
        }

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (fd.cFileName[0] != _T('.')) // exclude . and .. as well as unix style hidden dirs
            {
                int cSubScripts = 0;
                CScriptContainer* pSubContainer;
                bool bExisting;

                pSubContainer = pContainer->FirstChild(fd.cFileName, false);
                bExisting = (pSubContainer != NULL);
                if (!bExisting)
                {
                    pSubContainer = new CScriptContainer(pContainer, fd.cFileName, false);
                }

                cSubScripts = FillContainer(pSubContainer, hKey, registry);
                if (cSubScripts > 0)
                {
                    if (!bExisting)
                    {
                        LinkContainer(pSubContainer, pContainer);
                    }
                    cScripts += cSubScripts;
                }
                else if (!bExisting)
                {
                    delete pSubContainer;
                }
            }
        }
        else
        {
            PTSTR pszExt = PathFindExtension(fd.cFileName);
            if (pszExt && *pszExt)
            {
                TCHAR szFullPath[MAX_PATH];
                StringCchCopy(szFullPath, _countof(szFullPath), pContainer->GetPath());
                SalamanderGeneral->SalPathAppend(szFullPath, fd.cFileName, _countof(szFullPath));

                bool supported = g_oScriptAssociations.FindEngineByExt(pszExt);
                if (!supported)
                {
                    CExtensionManifest manifest;
                    supported = LoadManifestForEntryPoint(szFullPath, manifest) != FALSE;
                }
                if (!supported)
                {
                    const CAutomationSalamatrixBridge* bridge =
                        g_oAutomationPlugin.GetSalamatrixBridge();
                    Salamatrix::Runtime::IRuntimeService* runtimes =
                        bridge->GetRuntimeService();
                    char entryPointUtf8[MAX_PATH * 3];
                    supported =
                        runtimes != NULL &&
                        NativeToUtf8(
                            szFullPath, entryPointUtf8, _countof(entryPointUtf8)) &&
                        runtimes->FindAdapterForEntryPoint(entryPointUtf8) != NULL;
                }

                if (supported &&
                    AddScriptFromFile(pContainer, szFullPath, hKey, registry))
                {
                    ++cScripts;
                    ++m_cScriptsTotal;
                }
            }
        }
    } while (FindNextFile(hFind, &fd));

    FindClose(hFind);

    return (cScripts > 0);
}

void CScriptLookup::LinkContainer(CScriptContainer* pContainer, CScriptContainer* pParent)
{
    _ASSERTE(pParent != NULL);

    if (pParent->m_pChild == NULL)
    {
        pParent->m_pChild = pContainer;
    }
    else
    {
        CScriptContainer* pIter = pParent->m_pChild;
        CScriptContainer* pPrev = NULL;

        while (pIter && _tcsicmp(pContainer->m_pszName, pIter->m_pszName) >= 0)
        {
            pPrev = pIter;
            pIter = pIter->m_pSibling;
        }

        if (pPrev)
        {
            pContainer->m_pSibling = pPrev->m_pSibling;
            pPrev->m_pSibling = pContainer;
        }
        else
        {
            pContainer->m_pSibling = pParent->m_pChild;
            pParent->m_pChild = pContainer;
        }
    }
}

void CScriptLookup::UnlinkContainer(CScriptContainer* pContainer)
{
    _ASSERTE(pContainer != NULL);
    _ASSERTE(pContainer->m_pChild == NULL);
    _ASSERTE(pContainer->m_pScripts == NULL);

    CScriptContainer* pParent = pContainer->m_pParent;
    CScriptContainer *pIter, *pPrev = NULL;

    pIter = pParent->m_pChild;
    while (pIter != pContainer)
    {
        pPrev = pIter;
        pIter = pIter->m_pSibling;
    }

    if (pPrev == NULL)
    {
        pParent->m_pChild = pContainer->m_pSibling;
    }
    else
    {
        pPrev->m_pSibling = pContainer->m_pSibling;
    }

    pContainer->m_pSibling = NULL;
    pContainer->m_pParent = NULL;
}

void CScriptLookup::LinkScript(
    CScriptInfo* pScript)
{
    CScriptContainer* pParent = pScript->m_pContainer;

    if (pParent->m_pScripts == NULL)
    {
        pParent->m_pScripts = pScript;
    }
    else
    {
        CScriptInfo* pIter = pParent->m_pScripts;
        CScriptInfo* pPrev = NULL;

        while (pIter && _tcsicmp(pScript->GetDisplayName(), pIter->GetDisplayName()) >= 0)
        {
            pPrev = pIter;
            pIter = pIter->m_pNext;
        }

        if (pPrev)
        {
            pScript->m_pNext = pPrev->m_pNext;
            pPrev->m_pNext = pScript;
        }
        else
        {
            pScript->m_pNext = pParent->m_pScripts;
            pParent->m_pScripts = pScript;
        }
    }
}

void CScriptLookup::UnlinkScript(
    CScriptInfo* pScript)
{
    CScriptContainer* pParent = pScript->m_pContainer;
    CScriptInfo *pIter, *pPrev = NULL;

    pIter = pParent->m_pScripts;
    while (pIter != pScript)
    {
        pPrev = pIter;
        pIter = pIter->m_pNext;
    }

    if (pPrev == NULL)
    {
        pParent->m_pScripts = pScript->m_pNext;
    }
    else
    {
        pPrev->m_pNext = pScript->m_pNext;
    }

    pScript->m_pNext = NULL;
    pScript->m_pContainer = NULL;
}

void CScriptLookup::LinkScriptHash(CScriptInfo* pScript)
{
    int iBin;

    iBin = HashFromId(pScript->m_nId) % _countof(m_apHashBins);
    if (m_apHashBins[iBin] == NULL)
    {
        m_apHashBins[iBin] = pScript;
    }
    else
    {
        CScriptInfo* pIter = m_apHashBins[iBin];
        CScriptInfo* pPrev = NULL;

        while (pIter && pIter->m_nId <= pScript->m_nId)
        {
            pPrev = pIter;
            pIter = pIter->m_pNextHash;
        }

        if (pPrev)
        {
            pScript->m_pNextHash = pPrev->m_pNextHash;
            pPrev->m_pNextHash = pScript;
        }
        else
        {
            pScript->m_pNextHash = m_apHashBins[iBin];
            m_apHashBins[iBin] = pScript;
        }
    }
}

CScriptInfo* CScriptLookup::AddScriptFromFile(
    CScriptContainer* pContainer,
    PCTSTR pszFullPath,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    UINT nHash;
    int nUniquier;
    CScriptInfo* pScript;

    nHash = HashPath(pszFullPath);
    if (nHash == 0)
    {
        _ASSERTE(0);
        return NULL;
    }

    pScript = LookupScriptByPath(nHash, pszFullPath);
    if (pScript)
    {
        // script already in the hash map,
        // clear the dirty flag
        pScript->ResetDirty();
        return pScript;
    }

    nUniquier = GetUniquier(nHash, pszFullPath, hKey, registry);
    if (nUniquier < 0)
    {
        return NULL;
    }

    pScript = new CScriptInfo(pszFullPath, pContainer);
    pScript->m_nId = MakeId(nHash, nUniquier);
    LinkScript(pScript);
    LinkScriptHash(pScript);

    return pScript;
}

int CScriptLookup::GetUniquier(
    UINT nHash,
    __in_z PCTSTR pszPath,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    int iBin;
    CScriptInfo* pScript;
    int nUniquier;
    CUniquierBitmap bitmap;

    if (hKey != NULL && hKey != INVALID_HANDLE_VALUE)
    {
        HKEY hkSub;
        TCHAR szName[8];
        StringCchPrintf(szName, _countof(szName), "%06X", HashFromId(nHash));
        if (!registry->OpenKey(hKey, szName, hkSub))
        {
            // the hash key does not even exist in the registry,
            // return 1st available uniquier
            m_bModified = true;
            return 0;
        }

        LONG res = NO_ERROR;
        DWORD dwIndex = 0;
        DWORD cchName;
        DWORD dwType;
        TCHAR szPathRead[MAX_PATH];
        DWORD cbData;

        for (; res == NO_ERROR; dwIndex++)
        {
            cchName = _countof(szName);
            cbData = sizeof(szPathRead);
            res = RegEnumValue(hkSub, dwIndex, szName, &cchName,
                               NULL, &dwType, (LPBYTE)szPathRead, &cbData);
            if (res == NO_ERROR && dwType == REG_SZ)
            {
                nUniquier = _tcstol(szName, NULL, 16);

                // mark this uniquier as used in the free bitmap
                bitmap.MarkBusy(nUniquier);

                if (_tcsicmp(pszPath, szPathRead) == 0)
                {
                    // we found exact uniquier for this script
                    registry->CloseKey(hkSub);
                    return nUniquier;
                }
            }
        }

        registry->CloseKey(hkSub);

        // the script path was not found in the registry,
        // look if we can allocate a new uniquier for this path
        m_bModified = true;
        return bitmap.Alloc();
    }

    m_bModified = true;
    iBin = nHash % _countof(m_apHashBins);
    pScript = m_apHashBins[iBin];
    CScriptInfo* pFirstScript = NULL;
    while (pScript)
    {
        if (HashFromId(pScript->m_nId) == HashFromId(nHash))
        {
            pFirstScript = pScript;
            break;
        }
        else if (HashFromId(pScript->m_nId) > HashFromId(nHash))
        {
            break;
        }

        pScript = pScript->m_pNextHash;
    }

    if (pFirstScript != NULL)
    {
        while (pFirstScript && HashFromId(pFirstScript->m_nId) == HashFromId(nHash))
        {
            bitmap.MarkBusy(UniquierFromId(pFirstScript->m_nId));
            pFirstScript = pFirstScript->m_pNextHash;
        }

        return bitmap.Alloc();
    }

    // no uniquier for this hash yet, start counting at zero
    return 0;
}

UINT CScriptLookup::HashPath(__in_z PCTSTR pszPath)
{
    UINT nHash;
    TCHAR szCanonicalPath[MAX_PATH];

    StringCchCopy(szCanonicalPath, _countof(szCanonicalPath), pszPath);
    CharLower(szCanonicalPath);
    nHash = HashString(szCanonicalPath);
    if (nHash == 0)
    {
        nHash = ~0u;
    }

    nHash <<= HASH_SHIFT;
    nHash &= HASH_MASK;

    return nHash;
}

void CScriptLookup::CascadeDeleteContainer(
    CScriptContainer* pContainer)
{
    if (pContainer)
    {
        CascadeDeleteContainer(pContainer->m_pChild);

        while (pContainer->m_pSibling)
        {
            CScriptContainer* pTmp;
            pTmp = pContainer->m_pSibling->m_pSibling;
            CascadeDeleteContainer(pContainer->m_pSibling->m_pChild);
            delete pContainer->m_pSibling;
            pContainer->m_pSibling = pTmp;
        }

        delete pContainer;
    }
}

bool CScriptLookup::Refresh(bool bForce)
{
    bool res;

    if (GetTickCount() - m_dwLastRefreshTime < 5000 && !bForce)
    {
        // ignore refresh request if it comes too early
        // since the last one (this prevents excessive
        // disk activity when user repeatedly runs a script)
        return true;
    }

    // Keep the lifecycle registry in sync with the objects that are about to
    // be replaced by the refresh. Ownership is the CScriptInfo address, so
    // unregister before RemoveDirtyScripts deletes anything.
    UnpublishSalamatrixExtensions();

    // assume all existing scripts dirty
    MarkAllScriptsDirty();

    res = Load((HKEY)INVALID_HANDLE_VALUE, NULL);

    // remove scripts that remained dirty after the refresh
    RemoveDirtyScripts();

    // remove containers that remained empty
    RemoveEmptyContainers(m_pRootContainer);

    PublishSalamatrixExtensions();

    return res;
}

void CScriptLookup::UnpublishSalamatrixExtensions()
{
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasExtensions())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }

    Salamatrix::Extensions::IExtensionsService* service =
        bridge->GetExtensionsService();
    if (service == NULL)
        return;

    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        for (CScriptInfo* pScript = m_apHashBins[iBin];
             pScript != NULL;
             pScript = pScript->m_pNextHash)
        {
            if (pScript->GetSalamatrixExtensionId()[0] != '\0')
            {
                service->UnregisterExtension(
                    pScript->GetSalamatrixExtensionId(), pScript);
            }
        }
    }
}

DWORD WINAPI CScriptInfo::RuntimePumpProc(void* arg)
{
    CScriptInfo* script = static_cast<CScriptInfo*>(arg);
    if (script == NULL)
        return 0;

    Salamatrix::Runtime::IRuntimeSession* session = script->m_pRuntimeSession;
    if (session == NULL)
        return 0;

    while (session->IsAlive())
    {
        if (!session->Pump(250) && !session->IsAlive())
            break;
    }
    return 0;
}

void CScriptInfo::ReleaseRuntimeEventSubscriptions()
{
    if (m_nRuntimeEventSubscriptions <= 0)
        return;
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    Salamatrix::Events::IEventsService* events =
        bridge != NULL ? bridge->GetEventsService() : NULL;
    if (events != NULL)
    {
        for (int index = 0; index < m_nRuntimeEventSubscriptions; ++index)
            events->Unsubscribe(m_runtimeEventSubscriptions[index]);
    }
    memset(m_runtimeEventSubscriptions, 0, sizeof(m_runtimeEventSubscriptions));
    m_nRuntimeEventSubscriptions = 0;
}

void CScriptInfo::ReleaseRuntimeSession()
{
    if (m_pRuntimeSession == NULL)
        return;
    ReleaseRuntimeEventSubscriptions();
    m_pRuntimeSession->Stop();
    if (m_hRuntimePumpThread != NULL)
    {
        WaitForSingleObject(m_hRuntimePumpThread, INFINITE);
        CloseHandle(m_hRuntimePumpThread);
        m_hRuntimePumpThread = NULL;
    }
    m_pRuntimeSession->Release();
    m_pRuntimeSession = NULL;
}

void CScriptLookup::PublishSalamatrixExtensions()
{
    CAutomationSalamatrixBridge* bridge =
        g_oAutomationPlugin.GetSalamatrixBridge();
    if (!bridge->HasExtensions())
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        bridge = g_oAutomationPlugin.GetSalamatrixBridge();
    }

    Salamatrix::Extensions::IExtensionsService* service =
        bridge->GetExtensionsService();
    if (service == NULL)
        return;

    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        for (CScriptInfo* pScript = m_apHashBins[iBin];
             pScript != NULL;
             pScript = pScript->m_pNextHash)
        {
            const char* extensionId = pScript->GetSalamatrixExtensionId();
            if (extensionId[0] == '\0')
                continue;

            Salamatrix::Extensions::ExtensionDescriptor descriptor;
            memset(&descriptor, 0, sizeof(descriptor));
            descriptor.StructSize = sizeof(descriptor);
            StringCchCopyA(
                descriptor.Id, _countof(descriptor.Id), extensionId);
            NativeToUtf8(
                pScript->GetDisplayName(),
                descriptor.Name,
                _countof(descriptor.Name));
            StringCchCopyA(
                descriptor.RuntimeId,
                _countof(descriptor.RuntimeId),
                pScript->GetSalamatrixRuntimeId());
            NativeToUtf8(
                pScript->GetFileName(),
                descriptor.EntryPoint,
                _countof(descriptor.EntryPoint));
            descriptor.Flags = Salamatrix::Extensions::ExtensionFlagManifest |
                               Salamatrix::Extensions::ExtensionFlagPersistent;

            // A failed registration (for example a duplicate manifest id)
            // is intentionally ignored here. The host registry remains
            // authoritative and malformed/duplicate entries never become
            // executable by accident.
            service->RegisterExtension(
                &descriptor,
                CScriptInfo::RuntimeLifecycleCallback,
                pScript);
        }
    }
}

void CScriptLookup::MarkAllScriptsDirty()
{
    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
        for (CScriptInfo* pIter = m_apHashBins[iBin];
             pIter != NULL;
             pIter = pIter->m_pNextHash)
        {
            pIter->SetDirty();
        }
    }
}

void CScriptLookup::RemoveDirtyScripts()
{
    for (int iBin = 0; iBin < _countof(m_apHashBins); iBin++)
    {
    restart:
        CScriptInfo* pIter = m_apHashBins[iBin];
        CScriptInfo* pPrev = NULL;

        while (pIter != NULL)
        {
            CScriptInfo* pNext = pIter->m_pNextHash;

            if (pIter->IsDirty())
            {
                // unlink from hash map
                if (pPrev)
                {
                    pPrev->m_pNextHash = pNext;
                }
                else
                {
                    m_apHashBins[iBin] = pNext;
                }

                // unlink from container
                UnlinkScript(pIter);
                m_bModified = true;

                delete pIter;

                goto restart;
            }

            pPrev = pIter;
            pIter = pNext;
        }
    }
}

void CScriptLookup::RemoveEmptyContainers(CScriptContainer* pContainer)
{
    _ASSERTE(pContainer != NULL);

    if (pContainer->m_pChild != NULL)
    {
        RemoveEmptyContainers(pContainer->m_pChild);
    }

    while (pContainer != NULL)
    {
        CScriptContainer* pNext = pContainer->m_pSibling;

        if (pContainer->m_pChild == NULL && pContainer->m_pScripts == NULL)
        {
            if (pContainer != m_pRootContainer)
            {
                UnlinkContainer(pContainer);
                delete pContainer;
            }
        }

        pContainer = pNext;
    }
}

CScriptInfo* CScriptLookup::LookupScriptByPath(
    UINT nHash,
    PCTSTR pszFullPath)
{
    _ASSERTE(nHash != 0);

    int iBin = HashFromId(nHash) % _countof(m_apHashBins);
    for (CScriptInfo* pIter = m_apHashBins[iBin];
         pIter != NULL;
         pIter = pIter->m_pNextHash)
    {
        if (_tcsicmp(pIter->GetFileName(), pszFullPath) == 0)
        {
            return pIter;
        }
    }

    return NULL;
}
