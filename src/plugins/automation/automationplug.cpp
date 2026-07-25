// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
	Automation Plugin for Open Salamander
	
	Copyright (c) 2009-2026 Milan Kase <manison@manison.cz>
	Copyright (c) 2010-2026 Open Salamander Authors
	
	automationplug.cpp
	Automation plugin main object and menu extension.
*/

#include "precomp.h"
#include "automationplug.h"
#include "salamatrixrunner.h"
#include "scriptlist.h"
#include "extensionmanifest.h"
#include "automation.rh2"
#include "lang\lang.rh"
#include "engassoc.h"
#include "cfgdlg.h"
#include "abortpalette.h"
#include "versinfo.rh2"
#include "persistence.h"
#include "abortmodal.h"

#include <vector>

#pragma comment(lib, "UxTheme.lib")

CAutomationMenuExtInterface g_oMenuExtInterface;
extern CSalamanderGeneralAbstract* SalamanderGeneral;
extern CSalamanderGUIAbstract* SalamanderGUI;
extern HINSTANCE g_hInstance;
extern HINSTANCE g_hLangInst;
extern CAutomationPluginInterface g_oAutomationPlugin;
extern CGeneratedScriptRunner g_oGeneratedScriptRunner;
CWindowQueue AbortPaletteWindowQueue("Automation Abort Palette Window");

static std::string EscapeAssistantContext(const char* value)
{
    std::string escaped;
    if (value == NULL)
        return escaped;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value);
         *p != '\0'; ++p)
    {
        if (*p == '\\') escaped.append("\\\\");
        else if (*p == '"') escaped.append("\\\"");
        else if (*p == '\n') escaped.append("\\n");
        else if (*p == '\r') escaped.append("\\r");
        else escaped.push_back(static_cast<char>(*p));
    }
    return escaped;
}

static std::string LoadAssistantString(UINT resourceId)
{
    PCTSTR value = SalamanderGeneral->LoadStr(g_hLangInst, resourceId);
    if (value == NULL)
        return std::string();
#ifdef UNICODE
    int length = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, NULL, 0, NULL, NULL);
    if (length <= 0)
        return std::string();
    std::vector<char> buffer(static_cast<size_t>(length));
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
            &buffer[0], length, NULL, NULL) <= 0)
        return std::string();
    return std::string(&buffer[0]);
#else
    return std::string(value);
#endif
}

static BOOL AppendFocusedItemName(PTSTR path, int pathCapacity,
                                  const CFileData* file)
{
    if (path == NULL || pathCapacity <= 0 || file == NULL)
        return FALSE;
#ifdef UNICODE
    std::vector<wchar_t> name;
    if (file->NameW != NULL && file->NameW[0] != L'\0')
    {
        name.assign(file->NameW, file->NameW + wcslen(file->NameW) + 1);
    }
    else if (file->Name != NULL)
    {
        int length = MultiByteToWideChar(CP_ACP, 0, file->Name, -1, NULL, 0);
        if (length <= 0)
            return FALSE;
        name.resize(static_cast<size_t>(length));
        if (MultiByteToWideChar(CP_ACP, 0, file->Name, -1,
                                &name[0], length) <= 0)
            return FALSE;
    }
    else
    {
        return FALSE;
    }
    return PathAppendW(path, &name[0]);
#else
    return file->Name != NULL && PathAppendA(path, file->Name);
#endif
}

static std::string BuildAssistantPanelContext(
    Salamatrix::Sides::ISidesService* sides)
{
    if (sides == NULL)
        return "{}";
    const Salamatrix::Sides::SideReference side =
        Salamatrix::Sides::SideReferenceSource;
    std::vector<char> path(SALAMATRIX_SIDE_ITEM_PATH_CAPACITY);
    int pathType = 0;
    sides->GetPath(side, &path[0], static_cast<int>(path.size()), &pathType);
    std::string result =
        std::string("{\"source\":{\"path\":\"") +
        EscapeAssistantContext(&path[0]) + "\",\"pathType\":" +
        std::to_string(pathType) + ",\"selectedItems\":[";
    int selectedCount = sides->GetSelectedItemCount(side);
    int emitted = selectedCount < 32 ? selectedCount : 32;
    std::vector<Salamatrix::Sides::ItemInfo> itemBuffer(1);
    for (int index = 0; index < emitted; ++index)
    {
        Salamatrix::Sides::ItemInfo& item = itemBuffer[0];
        if (!sides->GetSelectedItem(side, index, &item))
            continue;
        if (index != 0)
            result.append(",");
        result += std::string("{\"name\":\"") +
                  EscapeAssistantContext(item.Name) + "\",\"path\":\"" +
                  EscapeAssistantContext(item.Path) + "\",\"isDirectory\":" +
                  (item.IsDirectory ? "true" : "false") + "}";
    }
    result += "]}}";
    return result;
}

static BOOL SaveAssistantScript(
    HWND parent,
    const char* script,
    TCHAR* savedPath,
    int savedPathCapacity)
{
    if (script == NULL || savedPath == NULL || savedPathCapacity <= 0)
        return FALSE;
    StringCchCopy(savedPath, savedPathCapacity, _T("salamatrix-script"));
    static const TCHAR filter[] =
        _T("Script files\0*.js;*.py;*.ps1;*.php\0All files\0*.*\0");
    OPENFILENAME ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parent;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = savedPath;
    ofn.nMaxFile = savedPathCapacity;
    ofn.lpstrDefExt = _T("js");
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    if (!SalamanderGeneral->SafeGetSaveFileName(&ofn))
        return FALSE;
    std::wstring widePath;
#ifdef UNICODE
    widePath.assign(savedPath);
#else
    int wideLength = MultiByteToWideChar(
        CP_ACP, 0, savedPath, -1, NULL, 0);
    if (wideLength <= 0)
        return FALSE;
    std::vector<wchar_t> converted(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(
            CP_ACP, 0, savedPath, -1, &converted[0], wideLength) <= 0)
        return FALSE;
    widePath.assign(&converted[0]);
#endif
    if (widePath.size() >= MAX_PATH &&
        widePath.compare(0, 4, L"\\\\?\\") != 0)
    {
        if (widePath.size() >= 2 && widePath[0] == L'\\' &&
            widePath[1] == L'\\')
            widePath = L"\\\\?\\UNC\\" + widePath.substr(2);
        else
            widePath = L"\\\\?\\" + widePath;
    }
    HANDLE file = CreateFileW(
        widePath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD length = static_cast<DWORD>(strlen(script));
    DWORD written = 0;
    BOOL result = WriteFile(file, script, length, &written, NULL) &&
                  written == length;
    CloseHandle(file);
    return result;
}

struct AssistantTemporaryScript
{
    std::wstring Directory;
    std::wstring ScriptPath;

    void Cleanup()
    {
        if (!ScriptPath.empty())
            DeleteFileW(ScriptPath.c_str());
        if (!Directory.empty())
            RemoveDirectoryW(Directory.c_str());
        ScriptPath.clear();
        Directory.clear();
    }
};

static std::wstring AssistantWin32Path(const std::wstring& value)
{
    if (value.size() < MAX_PATH || value.compare(0, 4, L"\\\\?\\") == 0)
        return value;
    if (value.size() >= 2 && value[0] == L'\\' && value[1] == L'\\')
        return L"\\\\?\\UNC\\" + value.substr(2);
    return L"\\\\?\\" + value;
}

static BOOL WriteAssistantUtf8File(
    const std::wstring& path,
    const char* text)
{
    if (text == NULL)
        return FALSE;
    std::wstring win32Path = AssistantWin32Path(path);
    HANDLE file = CreateFileW(
        win32Path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    const size_t length = strlen(text);
    BOOL result = length <= MAXDWORD;
    DWORD written = 0;
    if (result)
        result = WriteFile(
            file,
            text,
            static_cast<DWORD>(length),
            &written,
            NULL) &&
            written == static_cast<DWORD>(length);
    CloseHandle(file);
    return result;
}

static BOOL CreateAssistantTemporaryScript(
    const char* script,
    const char* extension,
    AssistantTemporaryScript& temporary)
{
    temporary.Cleanup();
    if (script == NULL || extension == NULL || extension[0] != '.')
        return FALSE;

    std::vector<wchar_t> tempRoot(SAL_MAX_PATH);
    DWORD rootLength = GetTempPathW(
        static_cast<DWORD>(tempRoot.size()), &tempRoot[0]);
    if (rootLength == 0 || rootLength >= tempRoot.size())
        return FALSE;

    std::vector<wchar_t> uniquePath(SAL_MAX_PATH);
    if (GetTempFileNameW(
            &tempRoot[0], L"smx", 0,
            &uniquePath[0]) == 0)
        return FALSE;
    DeleteFileW(&uniquePath[0]);
    if (!CreateDirectoryW(&uniquePath[0], NULL))
        return FALSE;

    temporary.Directory.assign(&uniquePath[0]);
    temporary.ScriptPath = temporary.Directory + L"\\generated";
    int extensionLength = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, extension, -1, NULL, 0);
    if (extensionLength <= 0)
    {
        temporary.Cleanup();
        return FALSE;
    }
    std::vector<wchar_t> extensionWide(static_cast<size_t>(extensionLength));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, extension, -1,
            &extensionWide[0], extensionLength) <= 0)
    {
        temporary.Cleanup();
        return FALSE;
    }
    temporary.ScriptPath.append(&extensionWide[0]);
    if (!WriteAssistantUtf8File(temporary.ScriptPath, script))
    {
        temporary.Cleanup();
        return FALSE;
    }
    return TRUE;
}

static BOOL GetAssistantRuntimeExtension(
    const Salamatrix::Runtime::IRuntimeAdapter* adapter,
    std::string& extension)
{
    extension.clear();
    if (adapter == NULL || adapter->GetDescriptor() == NULL ||
        adapter->GetDescriptor()->FileExtensions == NULL)
        return FALSE;
    const char* value = adapter->GetDescriptor()->FileExtensions;
    const char* end = strchr(value, ';');
    extension.assign(value, end != NULL ? end - value : strlen(value));
    return extension.size() >= 2 && extension[0] == '.';
}

static std::string MakeAssistantExtensionId(const char* title)
{
    std::string id;
    if (title != NULL)
    {
        for (const unsigned char* p =
                 reinterpret_cast<const unsigned char*>(title);
             *p != '\0' && id.size() < 96; ++p)
        {
            if ((*p >= 'A' && *p <= 'Z') ||
                (*p >= 'a' && *p <= 'z') ||
                (*p >= '0' && *p <= '9'))
            {
                char value = static_cast<char>(*p);
                if (value >= 'A' && value <= 'Z')
                    value = static_cast<char>(value - 'A' + 'a');
                id.push_back(value);
            }
            else if (!id.empty() && id[id.size() - 1] != '-')
            {
                id.push_back('-');
            }
        }
    }
    while (!id.empty() && id[id.size() - 1] == '-')
        id.erase(id.size() - 1);
    if (id.empty())
        id = "generated-extension";
    if (id[0] >= '0' && id[0] <= '9')
        id = "generated-" + id;
    return id;
}

static BOOL AssistantUtf8ToWide(
    const char* value,
    std::wstring& result)
{
    result.clear();
    const char* safeValue = value != NULL ? value : "";
    int length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, safeValue, -1, NULL, 0);
    if (length <= 0)
        return FALSE;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, safeValue, -1,
            &buffer[0], length) <= 0)
        return FALSE;
    result.assign(&buffer[0]);
    return TRUE;
}

static BOOL SaveAssistantExtensionPackage(
    HWND parent,
    CAutomationSalamatrixBridge* bridge,
    Salamatrix::UI::IUIService* ui,
    const char* runtimeId,
    const Salamatrix::AI::AssistantResponse& response)
{
    if (bridge == NULL || ui == NULL || runtimeId == NULL ||
        runtimeId[0] == '\0' || response.Summary.Script[0] == '\0')
        return FALSE;

    Salamatrix::Runtime::IRuntimeService* runtime =
        bridge->GetRuntimeService();
    Salamatrix::Runtime::IRuntimeAdapter* adapter =
        runtime != NULL ? runtime->FindAdapter(runtimeId, 0) : NULL;
    if (adapter == NULL || !adapter->IsAvailable())
        return FALSE;
    std::string extension;
    if (!GetAssistantRuntimeExtension(adapter, extension))
        return FALSE;

    std::vector<char> selectedFolder(SAL_MAX_PATH * 3);
    if (!ui->PickFolder(
            parent,
            "Choose a directory for the extension package",
            "",
            &selectedFolder[0],
            static_cast<DWORD>(selectedFolder.size())))
        return FALSE;

    std::wstring parentWide;
    std::wstring idWide;
    std::wstring extensionWide;
    if (!AssistantUtf8ToWide(
            &selectedFolder[0], parentWide) ||
        !AssistantUtf8ToWide(
            MakeAssistantExtensionId(response.Summary.Title).c_str(), idWide) ||
        !AssistantUtf8ToWide(extension.c_str(), extensionWide))
        return FALSE;

    std::string extensionId =
        MakeAssistantExtensionId(response.Summary.Title);
    std::wstring packagePath = parentWide;
    if (!packagePath.empty() && packagePath[packagePath.size() - 1] != L'\\')
        packagePath.push_back(L'\\');
    packagePath += idWide;
    packagePath = AssistantWin32Path(packagePath);
    DWORD attributes = GetFileAttributesW(packagePath.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
        return FALSE;
    if (!CreateDirectoryW(packagePath.c_str(), NULL))
        return FALSE;

    std::string capabilities = "[]";
    Salamatrix::Runtime::Protocol::Json::FindRawMember(
        response.ResponseJson, "capabilities", &capabilities);
    if (capabilities.size() < 2 || capabilities[0] != '[' ||
        capabilities[capabilities.size() - 1] != ']')
        capabilities = "[]";

    std::string entryPoint = "main" + extension;
    std::string manifest =
        std::string("{\n  \"schemaVersion\": 1,\n") +
        "  \"id\": \"" + EscapeAssistantContext(extensionId.c_str()) +
        "\",\n  \"name\": \"" +
        EscapeAssistantContext(response.Summary.Title) +
        "\",\n  \"version\": \"1.0.0\",\n  \"description\": \"" +
        EscapeAssistantContext(response.Summary.Description) +
        "\",\n  \"runtime\": \"" +
        EscapeAssistantContext(runtimeId) +
        "\",\n  \"entryPoint\": \"" +
        EscapeAssistantContext(entryPoint.c_str()) +
        "\",\n  \"capabilities\": " + capabilities +
        ",\n  \"commands\": [{\"id\": \"" +
        EscapeAssistantContext(extensionId.c_str()) +
        "\", \"title\": \"" +
        EscapeAssistantContext(response.Summary.Title) +
        "\", \"handler\": \"main\", \"menu\": \"plugin\", \"requires\": \"any\"}]\n}\n";

    CExtensionManifest parsedManifest;
    CExtensionManifestError manifestError;
    if (!parsedManifest.Parse(
            manifest.c_str(), manifest.size(), manifestError))
    {
        RemoveDirectoryW(packagePath.c_str());
        return FALSE;
    }

    std::wstring manifestPath = packagePath + L"\\extension.json";
    std::wstring scriptPath = packagePath + L"\\main" + extensionWide;
    if (!WriteAssistantUtf8File(manifestPath, manifest.c_str()) ||
        !WriteAssistantUtf8File(scriptPath, response.Summary.Script))
    {
        DeleteFileW(manifestPath.c_str());
        DeleteFileW(scriptPath.c_str());
        RemoveDirectoryW(packagePath.c_str());
        return FALSE;
    }
    return TRUE;
}

static BOOL RunAssistantScript(
    CAutomationSalamatrixBridge* bridge,
    const char* runtimeId,
    const char* script,
    HWND parent)
{
    if (bridge == NULL || runtimeId == NULL || runtimeId[0] == '\0' ||
        script == NULL)
        return FALSE;
    Salamatrix::Runtime::IRuntimeService* runtime =
        bridge->GetRuntimeService();
    Salamatrix::Runtime::IRuntimeAdapter* adapter =
        runtime != NULL ? runtime->FindAdapter(runtimeId, 0) : NULL;
    if (adapter == NULL || !adapter->IsAvailable())
        return FALSE;

    std::string extension;
    if (!GetAssistantRuntimeExtension(adapter, extension))
        return FALSE;
    AssistantTemporaryScript temporary;
    if (!CreateAssistantTemporaryScript(script, extension.c_str(), temporary))
        return FALSE;

#ifdef UNICODE
    CScriptInfo hostScript(temporary.ScriptPath.c_str(), NULL);
#else
    int nativeLength = WideCharToMultiByte(
        CP_ACP, 0, temporary.ScriptPath.c_str(), -1, NULL, 0, NULL, NULL);
    if (nativeLength <= 0)
    {
        temporary.Cleanup();
        return FALSE;
    }
    std::vector<TCHAR> nativePath(static_cast<size_t>(nativeLength));
    if (WideCharToMultiByte(
            CP_ACP, 0, temporary.ScriptPath.c_str(), -1,
            &nativePath[0], nativeLength, NULL, NULL) <= 0)
    {
        temporary.Cleanup();
        return FALSE;
    }
    CScriptInfo hostScript(&nativePath[0], NULL);
#endif

    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = "org.opensalamander.ai.generated";
    request.CommandId = "run";
    request.EntryPoint = temporary.ScriptPath.c_str();
    request.ParentWindow = parent;
    request.TimeoutMs = 120000;
    request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap |
                    Salamatrix::Runtime::RuntimeExecutionFlagOneShotWorker;
    request.CompatibilityExecute =
        CScriptInfo::DispatchCompatibilityRuntimeForScript;
    request.CompatibilityContext = &hostScript;
    request.HostDispatch = CScriptInfo::DispatchRuntimeHostCall;
    request.HostDispatchContext = &hostScript;

    BOOL executed = FALSE;
    Salamatrix::Runtime::IRuntimeSession* session = NULL;
    if (adapter->StartPersistent(&request, &session) && session != NULL)
    {
        const ULONGLONG startedAt = GetTickCount64();
        while (session->IsAlive())
        {
            if (GetTickCount64() - startedAt >= request.TimeoutMs)
                break;
            session->Pump(250);
        }
        DWORD exitCode = 1;
        executed = session->GetExitCode(&exitCode) && exitCode == 0;
        if (session->IsAlive())
            session->Stop();
        session->Release();
    }
    else if (adapter->GetDescriptor() != NULL &&
             adapter->GetDescriptor()->RuntimeId != NULL &&
             _strnicmp(adapter->GetDescriptor()->RuntimeId,
                       "Automation.", 10) == 0)
    {
        request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagNone;
        Salamatrix::Runtime::RuntimeExecutionResult result;
        executed = adapter->Execute(&request, &result);
    }
    temporary.Cleanup();
    return executed;
}

static const TCHAR CONFIG_VERSION[] = TEXT("Version");
static const UINT CURRENT_CONFIG_VERSION = 1;
static const TCHAR CONFIG_ENABLEDEBUGGER[] = TEXT("EnableDebugger");
static const TCHAR CONFIG_DIRECTORIES[] = TEXT("Directories");
static const TCHAR CONFIG_SCRIPTS[] = TEXT("Scripts");
static const TCHAR CONFIG_PERSISTENCE[] = TEXT("Persistent");

CScriptLookup g_oScriptLookup;
CPersistentValueStorage g_oPersistentStorage;

CAutomationMenuExtInterface::CAutomationMenuExtInterface()
{
    m_bDeferredPopup = false;
}

BOOL WINAPI CAutomationMenuExtInterface::ExecuteMenuItem(
    CSalamanderForOperationsAbstract* salamander,
    HWND parent,
    int id,
    DWORD eventMask)
{
    CScriptInfo::EXECUTION_INFO info;
    bool bExecuted = false;
    bool bRunScript = false;

    g_oAutomationPlugin.RefreshSalamatrixServices();

    info.pOperation = salamander;
    info.bEnableDebugger = g_oAutomationPlugin.IsDebuggerEnabled();

    if (id == CmdRunFocusedScript)
    {
        std::vector<TCHAR> szFullName(SAL_MAX_PATH);
        const CFileData* pFocusedFile;

        SalamanderGeneral->GetPanelPath(
            PANEL_SOURCE, &szFullName[0], static_cast<int>(szFullName.size()), NULL, NULL);
        pFocusedFile = SalamanderGeneral->GetPanelFocusedItem(PANEL_SOURCE, NULL);
        AppendFocusedItemName(
            &szFullName[0], static_cast<int>(szFullName.size()), pFocusedFile);

        CScriptInfo scriptInfo(&szFullName[0], NULL);
        bExecuted = scriptInfo.Execute(info);
    }
    else if (id == CmdScriptPopupMenu)
    {
        m_bDeferredPopup = true;
        SalamanderGeneral->PostPluginMenuChanged();
    }
    else if (id == CmdOpenPopupMenu)
    {
        id = ExecuteScriptMenu();
        return ExecuteMenuItem(salamander, parent, id, eventMask);
    }
    else if (id == CmdAskAssistant)
    {
        CAutomationSalamatrixBridge* bridge =
            g_oAutomationPlugin.GetSalamatrixBridge();
        Salamatrix::AI::IAssistantService* assistant =
            bridge != NULL ? bridge->GetAssistantService() : NULL;
        Salamatrix::UI::IUIService* ui =
            bridge != NULL ? bridge->GetUIService() : NULL;
        const std::string aiTitle = LoadAssistantString(IDS_AITITLE);
        const std::string aiPrompt = LoadAssistantString(IDS_AIPROMPT);
        const std::string aiGenerateFailed =
            LoadAssistantString(IDS_AIGENERATEFAILED);
        const std::string aiRefineQuestion =
            LoadAssistantString(IDS_AIREFINEQUESTION);
        const std::string aiRefinePrompt =
            LoadAssistantString(IDS_AIREFINEPROMPT);
        char prompt[4096];
        if (assistant == NULL || ui == NULL ||
            !ShowRuntimeInputBox(
                parent,
                aiTitle.c_str(),
                aiPrompt.c_str(),
                "",
                prompt,
                _countof(prompt)))
            return FALSE;
        std::string context = BuildAssistantPanelContext(
            bridge->GetSidesService());
        Salamatrix::AI::AssistantRequest request;
        request.Prompt = prompt;
        request.ContextJson = context.c_str();
        Salamatrix::AI::AssistantResponse response;
        std::string generatedRuntime;
        std::string existingScript;
        std::string feedback;
        BOOL generated = FALSE;
        for (int iteration = 0; iteration < 3; ++iteration)
        {
            request.ExistingScript =
                existingScript.empty() ? NULL : existingScript.c_str();
            request.Feedback = feedback.empty() ? NULL : feedback.c_str();
            Salamatrix::AI::AssistantResponse candidate;
            BOOL attempt = assistant->Generate(NULL, &request, &candidate);
            if (!attempt)
            {
                generated = iteration != 0;
                break;
            }
            response = candidate;
            generated = TRUE;
            generatedRuntime.clear();
            if (generated && response.ResponseJson[0] != '\0')
                Salamatrix::Runtime::Protocol::Json::FindStringMember(
                    response.ResponseJson, "runtime", &generatedRuntime);
            if (!generated || iteration == 2)
                break;

            int refineChoice = ui->ShowMessageBox(
                parent,
                aiRefineQuestion.c_str(),
                aiTitle.c_str(),
                MB_YESNO | MB_ICONQUESTION);
            if (refineChoice != IDYES)
                break;
            char refinement[4096];
            if (!ShowRuntimeInputBox(
                    parent,
                    aiTitle.c_str(),
                    aiRefinePrompt.c_str(),
                    "",
                    refinement,
                    _countof(refinement)))
                break;
            existingScript.assign(response.Summary.Script);
            feedback.assign(refinement);
        }
        if (!generated)
        {
            ui->ShowMessageBox(
                parent,
                aiGenerateFailed.c_str(),
                aiTitle.c_str(),
                MB_OK | MB_ICONWARNING);
            return FALSE;
        }
        std::string summary =
            std::string(response.Summary.Title) + "\n\n" +
            response.Summary.Description +
            "\n\n" + LoadAssistantString(IDS_AIPREVIEWSUMMARY) + "\n";
        if (Salamatrix::AI::IsSafeToRun(response.Summary))
            summary += LoadAssistantString(IDS_AIEFFECTSREADONLY);
        else
            summary += LoadAssistantString(IDS_AIEFFECTSREVIEW);
        if (generatedRuntime.empty())
        {
            summary += "\n\n" + LoadAssistantString(IDS_AIRUNTIME);
        }
        const std::string previewTitle =
            LoadAssistantString(IDS_AIPREVIEWTITLE);
        const std::string saveQuestion =
            LoadAssistantString(IDS_AISAVEQUESTION);
        const std::string saveSucceeded =
            LoadAssistantString(IDS_AISAVESUCCEEDED);
        const std::string saveFailed = LoadAssistantString(IDS_AISAVEFAILED);
        const std::string extensionQuestion =
            LoadAssistantString(IDS_AIEXTQUESTION);
        const std::string extensionSucceeded =
            LoadAssistantString(IDS_AIEXTSUCCEEDED);
        const std::string extensionFailed =
            LoadAssistantString(IDS_AIEXTFAILED);
        ui->CopyTextToClipboard(response.Summary.Script, TRUE, parent);
        ui->ShowMessageBox(
            parent,
            summary.c_str(),
            previewTitle.c_str(),
            MB_OK | MB_ICONINFORMATION);
        if (!generatedRuntime.empty() &&
            Salamatrix::AI::IsSafeToRun(response.Summary))
        {
            const std::string runQuestion =
                LoadAssistantString(IDS_AIRUNQUESTION);
            int runChoice = ui->ShowMessageBox(
                parent,
                runQuestion.c_str(),
                aiTitle.c_str(),
                MB_YESNO | MB_ICONQUESTION);
            if (runChoice == IDYES)
            {
                const BOOL ran = RunAssistantScript(
                    bridge,
                    generatedRuntime.c_str(),
                    response.Summary.Script,
                    parent);
                const std::string runMessage = LoadAssistantString(
                    ran ? IDS_AIRUNSUCCEEDED : IDS_AIRUNFAILED);
                ui->ShowMessageBox(
                    parent,
                    runMessage.c_str(),
                    aiTitle.c_str(),
                    ran ? (MB_OK | MB_ICONINFORMATION)
                        : (MB_OK | MB_ICONWARNING));
            }
        }
        int saveChoice = ui->ShowMessageBox(
            parent,
            saveQuestion.c_str(),
            aiTitle.c_str(),
            MB_YESNO | MB_ICONQUESTION);
        if (saveChoice == IDYES)
        {
            std::vector<TCHAR> savedPath(SAL_MAX_PATH);
            if (SaveAssistantScript(
                    parent,
                    response.Summary.Script,
                    &savedPath[0],
                    static_cast<int>(savedPath.size())))
            {
                ui->ShowMessageBox(
                    parent,
                    saveSucceeded.c_str(),
                    aiTitle.c_str(),
                    MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                ui->ShowMessageBox(
                    parent,
                    saveFailed.c_str(),
                    aiTitle.c_str(),
                    MB_OK | MB_ICONWARNING);
            }
        }
        if (!generatedRuntime.empty())
        {
            int extensionChoice = ui->ShowMessageBox(
                parent,
                extensionQuestion.c_str(),
                aiTitle.c_str(),
                MB_YESNO | MB_ICONQUESTION);
            if (extensionChoice == IDYES)
            {
                const BOOL packaged = SaveAssistantExtensionPackage(
                    parent,
                    bridge,
                    ui,
                    generatedRuntime.c_str(),
                    response);
                if (packaged)
                    g_oScriptLookup.Refresh(TRUE);
                ui->ShowMessageBox(
                    parent,
                    (packaged ? extensionSucceeded : extensionFailed).c_str(),
                    aiTitle.c_str(),
                    packaged ? (MB_OK | MB_ICONINFORMATION)
                             : (MB_OK | MB_ICONWARNING));
            }
        }
        return FALSE;
    }
    else
    {
        bRunScript = true;
    }

    if (bRunScript)
    {
        CScriptInfo* pScript = g_oScriptLookup.LookupScript(id);
        if (pScript == NULL)
            pScript = g_oScriptLookup.LookupRuntimeCommand(id);
        if (pScript)
        {
            bExecuted = pScript->Execute(info);
        }
    }

    return bExecuted ? info.bDeselect : FALSE;
}

DWORD WINAPI CAutomationMenuExtInterface::GetMenuItemState(
    int id,
    DWORD eventMask)
{
    if (id == CmdRunFocusedScript)
    {
        if ((eventMask & (MENU_EVENT_DISK | MENU_EVENT_FILE_FOCUSED)) != (MENU_EVENT_DISK | MENU_EVENT_FILE_FOCUSED))
        {
            // no file-on-disk focused
            return 0;
        }

        return CanExecuteFocusedItem() ? MENU_ITEM_STATE_ENABLED : 0;
    }
    else if (id == CmdScriptPopupMenu)
    {
        return MENU_ITEM_STATE_ENABLED;
    }
    else if (id == CmdAskAssistant)
    {
        g_oAutomationPlugin.RefreshSalamatrixServices();
        const CAutomationSalamatrixBridge* bridge =
            g_oAutomationPlugin.GetSalamatrixBridge();
        return bridge != NULL && bridge->HasAssistant() && bridge->HasUI()
                   ? MENU_ITEM_STATE_ENABLED
                   : 0;
    }
    else
    {
        // preloaded script item
        CScriptInfo* pScript = g_oScriptLookup.LookupScript(id);
        if (pScript == NULL)
            pScript = g_oScriptLookup.LookupRuntimeCommand(id);
        if (pScript == NULL)
            return 0;

        int runtimeIndex = pScript->GetRuntimeCommandIndexByMenuId(id);
        if (runtimeIndex >= 0)
        {
            const CScriptInfo::RUNTIME_COMMAND_INFO* command =
                pScript->GetRuntimeCommand(runtimeIndex);
            if (command == NULL ||
                (eventMask & command->MenuEventAndMask) !=
                    command->MenuEventAndMask ||
                (eventMask & command->MenuEventOrMask) == 0)
                return 0;
            return MENU_ITEM_STATE_ENABLED;
        }

        if ((eventMask & pScript->GetMenuEventAndMask()) != pScript->GetMenuEventAndMask())
            return 0;

        if ((eventMask & pScript->GetMenuEventOrMask()) == 0)
            return 0;

        return MENU_ITEM_STATE_ENABLED;
    }
}

int CAutomationMenuExtInterface::ExecuteScriptMenu()
{
    POINT pt;
    int nCmd;
    CGUIMenuPopupAbstract* pMenu;
    MENU_ITEM_INFO mii;
    TCHAR szText[100];
    TCHAR szHotKeyText[64];

    pMenu = SalamanderGUI->CreateMenuPopup();

    SalamanderGeneral->GetFocusedItemMenuPos(&pt);

    /* used for the export_mnu.py script, which generates salmenu.mnu for Translator
   keep synchronized with the InsertItem() call below...
MENU_TEMPLATE_ITEM ExecuteScriptMenu[] =
{
        {MNTT_PB, 0
        {MNTT_IT, IDS_RUNFOCUSED
        {MNTT_PE, 0
};
*/
    // run focused script menu item
    memset(&mii, 0, sizeof(MENU_ITEM_INFO));
    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_STATE | MENU_MASK_IMAGEINDEX;
    mii.Type = MENU_TYPE_STRING;
    mii.ID = CmdRunFocusedScript;
    mii.State = CanExecuteFocusedItem() ? 0 : MENU_STATE_GRAYED;
    LoadString(g_hLangInst, IDS_RUNFOCUSED, szText, _countof(szText));
    if (SalamanderGeneral->GetMenuItemHotKey(mii.ID, NULL, szHotKeyText, _countof(szHotKeyText)))
    {
        StringCchCat(szText, _countof(szText), TEXT("\t"));
        StringCchCat(szText, _countof(szText), szHotKeyText);
    }
    mii.String = szText;
    mii.ImageIndex = PluginIconRun;
    pMenu->InsertItem(0, TRUE, &mii);

    if (g_oScriptLookup.GetCount() > 0)
    {
        // separator
        memset(&mii, 0, sizeof(MENU_ITEM_INFO));
        mii.Mask = MENU_MASK_TYPE;
        mii.Type = MENU_TYPE_SEPARATOR;
        pMenu->InsertItem(1, TRUE, &mii);

        const CScriptContainer* pRootContainer = g_oScriptLookup.GetRootContainer();
        _ASSERTE(pRootContainer != NULL);
        AddScriptContainerToPopup(pRootContainer, pMenu, 0);
    }

    // The image lists are applied on existing submenus only,
    // so setup the image list after the menu structure is
    // built completely.
    pMenu->SetImageList(g_oAutomationPlugin.GetImageList(false), TRUE);
    pMenu->SetHotImageList(g_oAutomationPlugin.GetImageList(true), TRUE);

    nCmd = pMenu->Track(MENU_TRACK_RETURNCMD | MENU_TRACK_NONOTIFY,
                        pt.x, pt.y, SalamanderGeneral->GetMainWindowHWND(), NULL);

    SalamanderGUI->DestroyMenuPopup(pMenu);

    return nCmd;
}

void CAutomationMenuExtInterface::AddScriptContainerToPopup(
    const CScriptContainer* pContainer,
    CGUIMenuPopupAbstract* pMenu,
    int nLevel)
{
    MENU_ITEM_INFO mii = {
        0,
    };
    int i = 0;
    CGUIMenuPopupAbstract* pSubMenu = NULL;
    TCHAR szDisplayName[256];

    const CScriptContainer* pSubContainer;
    const CScriptInfo* pScript;

    if (nLevel > 1)
    {
        pSubMenu = SalamanderGUI->CreateMenuPopup();

        mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_SUBMENU;
        mii.Type = MENU_TYPE_STRING;
        mii.ID = 0;
        mii.SubMenu = pSubMenu;

        StringCchCopy(szDisplayName, _countof(szDisplayName), pContainer->GetName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));
        mii.String = szDisplayName;

        pMenu->InsertItem(INT_MAX, TRUE, &mii);
    }

    pSubContainer = pContainer->FirstChild();
    while (pSubContainer)
    {
        AddScriptContainerToPopup(pSubContainer, pSubMenu ? pSubMenu : pMenu, nLevel + 1);
        pSubContainer = pSubContainer->NextSibling();
    }

    mii.Mask = MENU_MASK_TYPE | MENU_MASK_ID | MENU_MASK_STRING | MENU_MASK_IMAGEINDEX;
    mii.Type = MENU_TYPE_STRING;
    mii.ImageIndex = PluginIconScript;
    for (pScript = pContainer->FirstScript(); pScript; pScript = pScript->Next(), i++)
    {
        if (pScript->GetRuntimeCommandCount() > 0)
        {
            for (int commandIndex = 0;
                 commandIndex < pScript->GetRuntimeCommandCount();
                 ++commandIndex)
            {
                const CScriptInfo::RUNTIME_COMMAND_INFO* command =
                    pScript->GetRuntimeCommand(commandIndex);
                if (command == NULL || !command->PluginMenu)
                    continue;
                mii.ID = command->MenuId;
                StringCchCopy(
                    szDisplayName,
                    _countof(szDisplayName),
                    command->Title);
                SalamanderGeneral->DuplicateAmpersands(
                    szDisplayName, _countof(szDisplayName));
                mii.String = szDisplayName;
                if (pSubMenu != NULL)
                    pSubMenu->InsertItem(INT_MAX, TRUE, &mii);
                else
                    pMenu->InsertItem(INT_MAX, TRUE, &mii);
            }
            continue;
        }
        if (!pScript->ShowInPluginMenu())
            continue;

        mii.ID = pScript->GetId();

        StringCchCopy(szDisplayName, _countof(szDisplayName), pScript->GetDisplayName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));
        TCHAR szHotKeyText[100];
        if (SalamanderGeneral->GetMenuItemHotKey(pScript->GetId(), NULL, szHotKeyText, 100))
        {
            StringCchCat(szDisplayName, _countof(szDisplayName), "\t");
            StringCchCat(szDisplayName, _countof(szDisplayName), szHotKeyText);
        }
        mii.String = szDisplayName;

        if (pSubMenu != NULL)
        {
            pSubMenu->InsertItem(INT_MAX, TRUE, &mii);
        }
        else
        {
            pMenu->InsertItem(INT_MAX, TRUE, &mii);
        }
    }
}

void WINAPI CAutomationMenuExtInterface::BuildMenu(
    HWND parent,
    CSalamanderBuildMenuAbstract* salamander)
{
    // refresh script list
    g_oScriptLookup.Refresh();

    /* used for the export_mnu.py script, which generates salmenu.mnu for Translator
   keep synchronized with the salamander->AddMenuItem() call below...
MENU_TEMPLATE_ITEM PluginMenu[] =
{
        {MNTT_PB, 0
        {MNTT_IT, IDS_RUNFOCUSED
        {MNTT_IT, IDS_SCRIPTPOPUPMENU
	{MNTT_PE, 0
};
*/

    // run focused script menu item
    salamander->AddMenuItem(
        PluginIconRun,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_RUNFOCUSED),
        0,
        CAutomationMenuExtInterface::CmdRunFocusedScript,
        TRUE, // callGetState
        0,    // or-mask (ignored id callGetState == TRUE)
        0,    // and-mask (ignored id callGetState == TRUE)
        MENU_SKILLLEVEL_ALL);

    // open script menu
    salamander->AddMenuItem(
        -1,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_SCRIPTPOPUPMENU),
        SALHOTKEY('A', HOTKEYF_CONTROL | HOTKEYF_SHIFT),
        CAutomationMenuExtInterface::CmdScriptPopupMenu,
        TRUE,
        0,
        0,
        MENU_SKILLLEVEL_ALL);

    salamander->AddMenuItem(
        PluginIconRun,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_ASKASSISTANT),
        0,
        CAutomationMenuExtInterface::CmdAskAssistant,
        TRUE,
        0,
        0,
        MENU_SKILLLEVEL_ALL);

    // for our menu items we set callGetState to TRUE, so the
    // GetMenuItemState will be always called for the items which
    // forces our plugin to be loaded before first menu popup
    // and the items get refreshed

    if (g_oScriptLookup.GetCount() > 0)
    {
        // separator
        salamander->AddMenuItem(
            -1, // icon index
            NULL,
            0,     // hotkey
            0,     // id
            FALSE, // callGetState
            0,     // or-mask
            0,     // and-mask
            MENU_SKILLLEVEL_ALL);

        const CScriptContainer* pRootContainer = g_oScriptLookup.GetRootContainer();
        _ASSERTE(pRootContainer != NULL);
        AddScriptContainerToMenu(pRootContainer, salamander, 0);
    }

    // Salamander manages the icon list itself, so we must everytime
    // create a copy of that list that we pass to menu builder.
    CGUIIconListAbstract* pListCopy = SalamanderGUI->CreateIconList();
    if (pListCopy)
    {
        if (pListCopy->CreateAsCopy(g_oAutomationPlugin.GetIconList(), FALSE))
        {
            salamander->SetIconListForMenu(pListCopy);
        }
        else
        {
            _ASSERTE(0);
            SalamanderGUI->DestroyIconList(pListCopy);
        }
    }
    else
    {
        _ASSERTE(0);
    }

    if (m_bDeferredPopup)
    {
        m_bDeferredPopup = false;
        SalamanderGeneral->PostMenuExtCommand(CmdOpenPopupMenu, TRUE);
    }
}

void CAutomationMenuExtInterface::AddScriptContainerToMenu(
    const CScriptContainer* pContainer,
    CSalamanderBuildMenuAbstract* pMenuBuilder,
    int nLevel)
{
    const CScriptContainer* pSubContainer;
    const CScriptInfo* pScript;
    TCHAR szDisplayName[256];

    if (nLevel > 1)
    {
        StringCchCopy(szDisplayName, _countof(szDisplayName), pContainer->GetName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));

        pMenuBuilder->AddSubmenuStart(
            -1, // iconIndex
            szDisplayName,
            0,
            FALSE,
            MENU_EVENT_TRUE,
            MENU_EVENT_TRUE,
            MENU_SKILLLEVEL_ALL);
    }

    pSubContainer = pContainer->FirstChild();
    while (pSubContainer)
    {
        AddScriptContainerToMenu(pSubContainer, pMenuBuilder, nLevel + 1);
        pSubContainer = pSubContainer->NextSibling();
    }

    pScript = pContainer->FirstScript();
    while (pScript)
    {
        if (pScript->GetRuntimeCommandCount() > 0)
        {
            for (int commandIndex = 0;
                 commandIndex < pScript->GetRuntimeCommandCount();
                 ++commandIndex)
            {
                const CScriptInfo::RUNTIME_COMMAND_INFO* command =
                    pScript->GetRuntimeCommand(commandIndex);
                if (command == NULL ||
                    (!command->PluginMenu && !command->ContextMenu))
                    continue;
                StringCchCopy(
                    szDisplayName,
                    _countof(szDisplayName),
                    command->Title);
                SalamanderGeneral->DuplicateAmpersands(
                    szDisplayName, _countof(szDisplayName));
                pMenuBuilder->AddMenuItem(
                    PluginIconScript,
                    szDisplayName,
                    command->HotKey,
                    command->MenuId,
                    TRUE,
                    command->MenuEventOrMask,
                    command->MenuEventAndMask,
                    MENU_SKILLLEVEL_ALL);
            }
            pScript = pScript->Next();
            continue;
        }
        if (!pScript->ShowInPluginMenu() && !pScript->ShowInContextMenu())
        {
            pScript = pScript->Next();
            continue;
        }

        StringCchCopy(szDisplayName, _countof(szDisplayName), pScript->GetDisplayName());
        SalamanderGeneral->DuplicateAmpersands(szDisplayName, _countof(szDisplayName));

        pMenuBuilder->AddMenuItem(
            PluginIconScript, // icon index
            szDisplayName,
            0,                // hotkey
            pScript->GetId(), // id
            TRUE,                         // callGetState
            pScript->GetMenuEventOrMask(),  // or-mask
            pScript->GetMenuEventAndMask(), // and-mask
            MENU_SKILLLEVEL_ALL);

        pScript = pScript->Next();
    }

    if (nLevel > 1)
    {
        pMenuBuilder->AddSubmenuEnd();
    }
}

bool CAutomationMenuExtInterface::CanExecuteFocusedItem()
{
    // check if we have script engine for the file extension
    const CFileData* pFocusedFile = SalamanderGeneral->GetPanelFocusedItem(PANEL_SOURCE, NULL);
    // _ASSERTE(pFocusedFile);  // Petr: if the panel is empty (e.g. when we are in the root of an empty disk)
    if (pFocusedFile == NULL || pFocusedFile->Ext == NULL || *pFocusedFile->Ext == _T('\0'))
    {
        return false;
    }

    if (!g_oScriptAssociations.FindEngineByExt(pFocusedFile->Ext - 1))
    {
        // there is no script engine registered for this extension
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

CAutomationPluginInterface::CAutomationPluginInterface() : m_aDirectories(4, 1)
{
    m_pIcons = NULL;
    m_himlHot = NULL;
    m_himlCold = NULL;
    m_bEnableDebugger = false;

    GetModuleFileName(NULL, m_szSalDir, _countof(m_szSalDir));
    PTSTR pszNamePart = PathFindFileName(m_szSalDir);
    if (pszNamePart)
    {
        *pszNamePart = _T('\0');
    }
}

CAutomationPluginInterface::~CAutomationPluginInterface()
{
}

void CAutomationPluginInterface::Connect(
    HWND parent,
    CSalamanderConnectAbstract* salamander)
{
    CGUIIconListAbstract* pIcons;
    BOOL bLoaded;

    RefreshSalamatrixServices();

    pIcons = SalamanderGUI->CreateIconList();
    _ASSERTE(pIcons);

    bLoaded = pIcons->CreateFromPNG(g_hInstance, MAKEINTRESOURCE(IDB_ICONSTRIP), 16);
    _ASSERTE(bLoaded);
    if (bLoaded)
    {
        salamander->SetIconListForGUI(pIcons);
        salamander->SetPluginIcon(PluginIconMain);
        salamander->SetPluginMenuAndToolbarIcon(PluginIconMain);

        m_pIcons = SalamanderGUI->CreateIconList();
        if (!m_pIcons->CreateAsCopy(pIcons, FALSE))
        {
            SalamanderGUI->DestroyIconList(m_pIcons);
            m_pIcons = NULL;
        }

        m_himlHot = pIcons->GetImageList();
        _ASSERTE(m_himlHot != NULL);

        m_himlCold = NULL;
        CGUIIconListAbstract* pColdIcons = SalamanderGUI->CreateIconList();
        _ASSERTE(pColdIcons);
        if (pColdIcons->CreateAsCopy(pIcons, TRUE))
        {
            m_himlCold = pColdIcons->GetImageList();
            _ASSERTE(m_himlCold);
        }
        else
        {
            _ASSERTE(0);
        }
        SalamanderGUI->DestroyIconList(pColdIcons);
    }
    else
    {
        SalamanderGUI->DestroyIconList(pIcons);
        pIcons = NULL;
    }
}

void CAutomationPluginInterface::RefreshSalamatrixServices()
{
    m_oSalamatrix.Refresh(SalamanderGeneral);
}

void CAutomationPluginInterface::About(HWND parent)
{
    TCHAR szMessage[512];
    TCHAR szSalamatrixStatus[256];

    RefreshSalamatrixServices();
    m_oSalamatrix.GetStatusText(szSalamatrixStatus, _countof(szSalamatrixStatus));

    StringCchPrintf(szMessage, _countof(szMessage),
                    TEXT("%s ") TEXT(VERSINFO_VERSION) TEXT("\n\n")
                        TEXT(VERSINFO_COPYRIGHT) TEXT("\n\n")
                            TEXT("%s\n\n")
                                TEXT("Salamatrix Framework: %s"),
                    SalamanderGeneral->LoadStr(g_hLangInst, IDS_PLUGINNAME),
                    SalamanderGeneral->LoadStr(g_hLangInst, IDS_DESCRIPTION),
                    szSalamatrixStatus);

    SalamanderGeneral->SalMessageBox(
        parent,
        szMessage,
        SalamanderGeneral->LoadStr(g_hLangInst, IDS_ABOUT),
        MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI CAutomationPluginInterface::Release(HWND parent, BOOL force)
{
    if (SalamanderGeneral != NULL)
        SalamanderGeneral->UnregisterServiceOwned(
            SALAMATRIX_SERVICE_SCRIPT_RUNNER,
            &g_oGeneratedScriptRunner,
            &g_oGeneratedScriptRunner);
    g_oScriptLookup.UnpublishSalamatrixExtensions();
    m_oSalamatrix.Reset();
    ReleaseWinLib(g_hInstance);
    UninitializeAbortableModalDialogWrapper();

    ImageList_Destroy(m_himlHot);
    m_himlHot = NULL;

    ImageList_Destroy(m_himlCold);
    m_himlCold = NULL;

    SalamanderGUI->DestroyIconList(m_pIcons);
    m_pIcons = NULL;

    return TRUE;
}

void WINAPI CAutomationPluginInterface::LoadConfiguration(
    HWND parent,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    DWORD dwVersion;
    DWORD dwArbitrary;
    DIRECTORY_INFO dir;

    CALL_STACK_MESSAGE1("CAutomationPluginInterface::LoadConfiguration(, ,)");

    if (!hKey)
    {
        dwVersion = CURRENT_CONFIG_VERSION;
    }
    else if (!registry->GetValue(hKey, CONFIG_VERSION, REG_DWORD, &dwVersion, sizeof(DWORD)))
    {
        dwVersion = 0;
    }

    if (hKey && registry->GetValue(hKey, CONFIG_ENABLEDEBUGGER, REG_DWORD, &dwArbitrary, sizeof(DWORD)))
    {
        m_bEnableDebugger = !!dwArbitrary;
    }
    else
    {
        m_bEnableDebugger = false;
    }

    bool bLoadDefaultDirs = true;

    if (hKey)
    {
        HKEY hkDirs;

        if (registry->OpenKey(hKey, CONFIG_DIRECTORIES, hkDirs))
        {
            TCHAR szValName[16];
            int iVal;

            bLoadDefaultDirs = false;

            for (iVal = 1;; iVal++)
            {
                _itot_s(iVal, szValName, _countof(szValName), 10);
                if (!registry->GetValue(hkDirs, szValName, REG_SZ, dir.szDirectory, _countof(dir.szDirectory)))
                {
                    break;
                }

                m_aDirectories.Add(dir);
            }

            registry->CloseKey(hkDirs);
        }
    }

    if (bLoadDefaultDirs)
    {
        dir.Set(_T("$[AppData]\\Open Salamander\\Automation\\scripts"));
        m_aDirectories.Add(dir);

        dir.Set(_T("$[AllUsersProfile]\\Open Salamander\\Automation\\scripts"));
        m_aDirectories.Add(dir);

        dir.Set(_T("$(SalDir)\\plugins\\automation\\scripts"));
        m_aDirectories.Add(dir);
    }

    bool bLookupLoaded = false;
    if (hKey)
    {
        HKEY hkScripts;

        if (registry->OpenKey(hKey, CONFIG_SCRIPTS, hkScripts))
        {
            bLookupLoaded = g_oScriptLookup.Load(hkScripts, registry);
            registry->CloseKey(hkScripts);
        }
    }

    if (!bLookupLoaded)
    {
        g_oScriptLookup.Load(NULL, registry);
    }

    g_oScriptLookup.PublishSalamatrixExtensions();

    if (hKey)
    {
        HKEY hkPersist;

        if (registry->OpenKey(hKey, CONFIG_PERSISTENCE, hkPersist))
        {
            g_oPersistentStorage.Load(hkPersist, registry);
            registry->CloseKey(hkPersist);
        }
    }
}

void WINAPI CAutomationPluginInterface::SaveConfiguration(
    HWND parent,
    HKEY hKey,
    CSalamanderRegistryAbstract* registry)
{
    CALL_STACK_MESSAGE1("CAutomationPluginInterface::SaveConfiguration(, ,)");

    if (hKey != NULL)
    {
        DWORD dwArbitrary;

        dwArbitrary = CURRENT_CONFIG_VERSION;
        registry->SetValue(hKey, CONFIG_VERSION, REG_DWORD, &dwArbitrary, sizeof(DWORD));

        dwArbitrary = m_bEnableDebugger;
        registry->SetValue(hKey, CONFIG_ENABLEDEBUGGER, REG_DWORD, &dwArbitrary, sizeof(DWORD));

        HKEY hkDirs;
        if (registry->CreateKey(hKey, CONFIG_DIRECTORIES, hkDirs))
        {
            TCHAR szValName[16];
            int iVal;

            registry->ClearKey(hkDirs);

            for (iVal = 0; iVal < m_aDirectories.Count; iVal++)
            {
                _itot_s(iVal + 1, szValName, _countof(szValName), 10);
                if (!registry->SetValue(hkDirs, szValName, REG_SZ,
                                        m_aDirectories.At(iVal).szDirectory, -1))
                {
                    break;
                }
            }

            registry->CloseKey(hkDirs);
        }

        if (g_oScriptLookup.IsModified())
        {
            HKEY hkScripts;

            if (registry->CreateKey(hKey, CONFIG_SCRIPTS, hkScripts))
            {
                g_oScriptLookup.Save(hkScripts, registry);
                registry->CloseKey(hkScripts);
            }
        }

        if (g_oPersistentStorage.IsModified())
        {
            HKEY hkPersist;

            if (registry->CreateKey(hKey, CONFIG_PERSISTENCE, hkPersist))
            {
                g_oPersistentStorage.Save(hkPersist, registry);
                registry->CloseKey(hkPersist);
            }
        }
    }
}

void WINAPI CAutomationPluginInterface::Configuration(HWND hwndParent)
{
    CALL_STACK_MESSAGE1("CAutomationPluginInterface::Configuration()");

    CAutomationConfigDialog(hwndParent).Execute();
}

bool CAutomationPluginInterface::ExpandPath(
    __in PCTSTR pszPath,
    __out_ecount(cchMax) PTSTR pszExpanded,
    __in int cchMax)
{
    static const CSalamanderVarStrEntry variables[] =
        {
            _T("SalDir"),
            ExpandSalDir,
            NULL,
            NULL,
        };

    if (!SalamanderGeneral->ExpandVarString(
            NULL, // hwndParent
            pszPath,
            pszExpanded,
            cchMax,
            variables,
            this))
    {
        return false;
    }

    return true;
}

/*static*/ PCTSTR CALLBACK CAutomationPluginInterface::ExpandSalDir(
    HWND hwndParent,
    void* pContext)
{
    CAutomationPluginInterface* that = reinterpret_cast<CAutomationPluginInterface*>(pContext);
    _ASSERTE(that);

    return that->m_szSalDir;
}

void CAutomationPluginInterface::Event(int event, DWORD param)
{
    switch (event)
    {
    case PLUGINEVENT_CONFIGURATIONCHANGED:
        // A runtime provider may be added after Automation. Re-publish and
        // activate manifest extensions so providers installed later become
        // usable without requiring a process restart.
        RefreshSalamatrixServices();
        g_oScriptLookup.PublishSalamatrixExtensions();
        break;
    case PLUGINEVENT_SETTINGCHANGE:
    {
        AbortPaletteWindowQueue.BroadcastMessage(CScriptAbortPaletteWindow::WM_USER_SETTINGCHANGE, 0, 0);
        break;
    }
    }
}
