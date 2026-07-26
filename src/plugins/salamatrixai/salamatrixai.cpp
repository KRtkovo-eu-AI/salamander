// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrixai.h"
#include "versinfo.rh2"
#include <strsafe.h>
#include <algorithm>
#include <winhttp.h>

HINSTANCE DLLInstance = NULL;
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
CSalamanderGUIAbstract* SalamanderGUI = NULL;
CSalamanderDebugAbstract* SalamanderDebug = NULL;
int SalamanderVersion = 0;
CPluginInterface PluginInterface;

namespace
{
CAIPluginMenuExt g_menu;
CLocalAssistantProvider g_provider;
CLocalHttpAssistantProvider g_httpProvider;
Salamatrix::AI::IAssistantService* g_ai = NULL;
Salamatrix::UI::IUIService* g_ui = NULL;
Salamatrix::Runtime::IRuntimeService* g_runtime = NULL;
Salamatrix::Automation::IScriptRunner* g_runner = NULL;
Salamatrix::Sides::ISidesService* g_sides = NULL;
bool g_released = false;

static bool IsCurrentService(const char* serviceId, DWORD minimumVersion, const void* expected)
{
    if (g_released || SalamanderGeneral == NULL || serviceId == NULL || expected == NULL)
        return false;

    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = serviceId;
    query.MinimumVersion = minimumVersion;
    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));
    return SalamanderGeneral->QueryService(&query, &result) && result.Interface == expected;
}

static const char* GetAssistantMenuCaption()
{
    if (SalamanderGeneral == NULL || DLLInstance == NULL)
        return "Ask Salamatrix AI...";
    const char* caption = SalamanderGeneral->LoadStr(DLLInstance, IDS_AI_ASSISTANT_MENU);
    return (caption == NULL || caption[0] == '\0') ? "Ask Salamatrix AI..." : caption;
}

static const char* AssistantStringFallback(UINT resourceId)
{
    switch (resourceId)
    {
    case IDS_AI_PROMPT: return "What should the generated script do?";
    case IDS_AI_PREVIEW_TITLE: return "Salamatrix AI preview (script copied to clipboard)";
    case IDS_AI_PREVIEW_SUMMARY: return "The generated script is ready for review.";
    case IDS_AI_SAVEQUESTION: return "Save the generated script to a file now?";
    case IDS_AI_SAVE_SUCCEEDED: return "The generated script was saved.";
    case IDS_AI_SAVE_FAILED: return "The generated script could not be saved.";
    case IDS_AI_EFFECTS_READONLY: return "No delete, external-process, or network effects were declared; review is still required.";
    case IDS_AI_EFFECTS_REVIEW: return "The declared effects require explicit review; no script was run.";
    case IDS_AI_GENERATE_FAILED: return "The local assistant did not return a valid script.";
    case IDS_AI_TITLE: return "Salamatrix AI";
    case IDS_AI_RUN_QUESTION: return "Run the generated script now?";
    case IDS_AI_RUN_SUCCEEDED: return "The generated script finished successfully.";
    case IDS_AI_RUN_FAILED: return "The generated script failed.";
    case IDS_AI_RUNTIME_MISSING: return "The generated script does not specify a supported runtime.";
    case IDS_AI_EXT_QUESTION: return "Save this script as a Salamatrix extension package?";
    case IDS_AI_EXT_SUCCEEDED: return "The Salamatrix extension package was saved.";
    case IDS_AI_EXT_FAILED: return "The Salamatrix extension package could not be saved.";
    case IDS_AI_EXT_INVALID: return "The generated response has no supported runtime for an extension package.";
    case IDS_AI_REFINE_QUESTION: return "Refine the generated script before previewing it?";
    case IDS_AI_REFINE_PROMPT: return "What should be changed in the generated script?";
    case IDS_AI_PREVIEW_LABEL: return "Preview";
    case IDS_AI_RUN_LABEL: return "Run";
    case IDS_AI_EXPORT_LABEL: return "Export";
    case IDS_AI_ASK_LABEL: return "Ask";
    case IDS_AI_SAVE_LABEL: return "Save";
    case IDS_AI_REFINE_ACCEPT: return "Use feedback";
    case IDS_AI_REFINE_CANCEL: return "Cancel";
    default: return "";
    }
}

static std::string LoadAssistantString(UINT resourceId)
{
    if (SalamanderGeneral == NULL || DLLInstance == NULL)
        return std::string(AssistantStringFallback(resourceId));
    const char* value = SalamanderGeneral->LoadStr(DLLInstance, resourceId);
    return value == NULL || value[0] == '\0'
               ? std::string(AssistantStringFallback(resourceId))
               : std::string(value);
}

static std::string EscapeJson(const char* value)
{
    std::string result;
    if (value == NULL) return result;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value); *p; ++p)
    {
        switch (*p)
        {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default: result.push_back(static_cast<char>(*p)); break;
        }
    }
    return result;
}

static std::string EscapeAssistantContext(const char* value)
{
    std::string escaped;
    if (value == NULL)
        return escaped;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(value);
         *p != '\0'; ++p)
    {
        if (*p == '\\')
            escaped.append("\\\\");
        else if (*p == '"')
            escaped.append("\\\"");
        else if (*p == '\n')
            escaped.append("\\n");
        else if (*p == '\r')
            escaped.append("\\r");
        else
            escaped.push_back(static_cast<char>(*p));
    }
    return escaped;
}

static std::wstring AssistantWin32Path(const std::wstring& value);

static std::string BuildAssistantPanelContext(
    Salamatrix::Sides::ISidesService* sides)
{
    if (sides == NULL)
        return "{}";
    const Salamatrix::Sides::SideReference side =
        Salamatrix::Sides::SideReferenceSource;
    std::vector<char> path(SALAMATRIX_SIDE_ITEM_PATH_CAPACITY);
    int pathType = 0;
    sides->GetPath(side, path.data(), static_cast<int>(path.size()), &pathType);
    std::string result =
        std::string("{\"source\":{\"path\":\"") +
        EscapeAssistantContext(&path[0]) + "\",\"pathType\":" +
        std::to_string(pathType) + ",\"selectedItems\":[";
    const int selectedCount = sides->GetSelectedItemCount(side);
    const int emitted = selectedCount < 32 ? selectedCount : 32;
    int emittedItems = 0;
    Salamatrix::Sides::ItemInfo item;
    item.StructSize = sizeof(item);
    for (int index = 0; index < emitted; ++index)
    {
        if (!sides->GetSelectedItem(side, index, &item))
            continue;
        if (emittedItems != 0)
            result.append(",");
        result += std::string("{\"name\":\"") +
                  EscapeAssistantContext(item.Name) + "\",\"path\":\"" +
                  EscapeAssistantContext(item.Path) + "\",\"isDirectory\":" +
                  (item.IsDirectory ? "true" : "false") + "}";
        ++emittedItems;
    }
    result += "]} }";
    return result;
}

static BOOL SaveAssistantStringToUtf8File(const std::wstring& path, const char* text)
{
    if (text == NULL)
        return FALSE;
    const std::wstring win32Path = AssistantWin32Path(path);
    HANDLE file = CreateFileW(
        win32Path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;
    DWORD length = static_cast<DWORD>(strlen(text));
    DWORD written = 0;
    BOOL result = length <= MAXDWORD &&
                  WriteFile(file, text, length, &written, NULL) &&
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

static BOOL CreateAssistantTemporaryScript(const char* script, const char* extension,
                                         AssistantTemporaryScript& temporary)
{
    temporary.Cleanup();
    if (script == NULL || extension == NULL || extension[0] != '.')
        return FALSE;

    std::vector<wchar_t> tempRoot(SAL_MAX_PATH);
    DWORD rootLength = GetTempPathW(static_cast<DWORD>(tempRoot.size()), tempRoot.data());
    if (rootLength == 0 || rootLength >= tempRoot.size())
        return FALSE;

    std::vector<wchar_t> tempDir(SAL_MAX_PATH);
    if (GetTempFileNameW(tempRoot.data(), L"smx", 0, tempDir.data()) == 0)
        return FALSE;
    DeleteFileW(tempDir.data());
    if (!CreateDirectoryW(tempDir.data(), NULL))
        return FALSE;

    temporary.Directory.assign(tempDir.data());
    temporary.ScriptPath = temporary.Directory + L"\\generated";
    int extensionLength =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, extension, -1, NULL, 0);
    if (extensionLength <= 0)
    {
        temporary.Cleanup();
        return FALSE;
    }
    std::vector<wchar_t> wideExtension(static_cast<size_t>(extensionLength));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, extension, -1,
                           wideExtension.data(), extensionLength) <= 0)
    {
        temporary.Cleanup();
        return FALSE;
    }
    temporary.ScriptPath.append(wideExtension.data());
    if (!SaveAssistantStringToUtf8File(temporary.ScriptPath, script))
    {
        temporary.Cleanup();
        return FALSE;
    }
    return TRUE;
}

static bool AssistantUtf8ToWide(const char* value, std::wstring& result)
{
    result.clear();
    if (value == NULL)
        return false;
    int length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, NULL, 0);
    if (length <= 0)
        return false;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, buffer.data(), length) <= 0)
        return false;
    result.assign(buffer.data());
    return true;
}

static bool GetAssistantRuntimeExtension(
    const Salamatrix::Runtime::IRuntimeAdapter* adapter,
    std::string& extension)
{
    extension.clear();
    if (adapter == NULL || adapter->GetDescriptor() == NULL ||
        adapter->GetDescriptor()->FileExtensions == NULL)
        return false;
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
        for (const unsigned char* p = reinterpret_cast<const unsigned char*>(title);
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

static void AssistantFailure(Salamatrix::AI::AssistantResponse* response,
                             Salamatrix::AI::AssistantStatus status,
                             HRESULT code, const wchar_t* message)
{
    response->Status = status;
    response->ErrorCode = code;
    response->OutputLength = 0;
    response->ResponseJson[0] = '\0';
    StringCchCopyW(response->Message, _countof(response->Message), message);
}

static bool StructuredJson(const std::string& value)
{
    size_t first = value.find_first_not_of(" \t\r\n");
    size_t last = value.find_last_not_of(" \t\r\n");
    return first != std::string::npos && last > first &&
           ((value[first] == '{' && value[last] == '}') ||
            (value[first] == '[' && value[last] == ']'));
}

static bool IsChatCompletionsProtocol(const std::wstring& protocol)
{
    return _wcsicmp(protocol.c_str(), L"chat") == 0 ||
           _wcsicmp(protocol.c_str(), L"chat-completions") == 0 ||
           _wcsicmp(protocol.c_str(), L"openai") == 0 ||
           _wcsicmp(protocol.c_str(), L"llama.cpp") == 0;
}

static bool ExtractChatCompletionContent(
    const std::string& response,
    std::string& content)
{
    std::string choices;
    if (!Salamatrix::Runtime::Protocol::Json::FindRawMember(
            response.c_str(), "choices", &choices))
        return false;

    size_t position = 0;
    Salamatrix::Runtime::Protocol::Json::SkipWhitespace(choices, &position);
    if (position >= choices.size() || choices[position] != '[')
        return false;
    ++position;
    Salamatrix::Runtime::Protocol::Json::SkipWhitespace(choices, &position);
    if (position >= choices.size() || choices[position] == ']')
        return false;
    size_t objectStart = position;
    if (!Salamatrix::Runtime::Protocol::Json::SkipValue(
            choices, &position) || position <= objectStart)
        return false;
    std::string choice = choices.substr(objectStart, position - objectStart);
    std::string message;
    if (!Salamatrix::Runtime::Protocol::Json::FindRawMember(
            choice.c_str(), "message", &message) ||
        !Salamatrix::Runtime::Protocol::Json::FindStringMember(
            message.c_str(), "content", &content))
        return false;
    return true;
}

static bool ReadEnvironmentString(const wchar_t* name, std::wstring& value)
{
    value.clear();
    wchar_t buffer[4096];
    DWORD length = GetEnvironmentVariableW(name, buffer, _countof(buffer));
    if (length == 0 || length >= _countof(buffer))
        return false;
    value.assign(buffer, length);
    return !value.empty();
}

static std::string WideToUtf8String(const std::wstring& value)
{
    if (value.empty())
        return std::string();
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                    value.c_str(), -1, NULL, 0, NULL, NULL);
    if (length <= 1)
        return std::string();
    std::vector<char> buffer(static_cast<size_t>(length));
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            value.c_str(), -1, buffer.data(), length,
                            NULL, NULL) <= 0)
        return std::string();
    return std::string(buffer.data());
}

static bool HttpGenerateRequest(
    const std::wstring& url,
    const std::wstring& model,
    const std::string& body,
    bool chatCompletions,
    DWORD timeoutMs,
    std::string& responseBody)
{
    responseBody.clear();
    if (url.empty() || model.empty() || body.empty())
        return false;

    URL_COMPONENTS components;
    memset(&components, 0, sizeof(components));
    components.dwStructSize = sizeof(components);
    // Use heap-backed bounds rather than MAX_PATH-sized URL scratch buffers.
    std::vector<wchar_t> host(4096, L'\0');
    std::vector<wchar_t> path(32768, L'\0');
    std::vector<wchar_t> extra(8192, L'\0');
    components.lpszHostName = host.data();
    components.dwHostNameLength = static_cast<DWORD>(host.size() - 1);
    components.lpszUrlPath = path.data();
    components.dwUrlPathLength = static_cast<DWORD>(path.size() - 1);
    components.lpszExtraInfo = extra.data();
    components.dwExtraInfoLength = static_cast<DWORD>(extra.size() - 1);
    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &components))
        return false;

    std::wstring requestPath(path.data(), components.dwUrlPathLength);
    requestPath.append(extra.data(), components.dwExtraInfoLength);
    if (requestPath.empty() || requestPath == L"/")
        requestPath = chatCompletions
                          ? L"/v1/chat/completions"
                          : L"/api/generate";
    HINTERNET session = WinHttpOpen(
        L"Open Salamander Salamatrix AI/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (session == NULL)
        return false;
    DWORD boundedTimeout = timeoutMs == 0 ? 120000 :
        (timeoutMs > 120000 ? 120000 : timeoutMs);
    WinHttpSetTimeouts(session, boundedTimeout, boundedTimeout,
                       boundedTimeout, boundedTimeout);
    HINTERNET connection = WinHttpConnect(
        session, host.data(), components.nPort, 0);
    if (connection == NULL)
    {
        WinHttpCloseHandle(session);
        return false;
    }
    DWORD flags = components.nScheme == INTERNET_SCHEME_HTTPS
                      ? WINHTTP_FLAG_SECURE
                      : 0;
    HINTERNET request = WinHttpOpenRequest(
        connection, L"POST", requestPath.c_str(), NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (request == NULL)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }
    const wchar_t headers[] = L"Content-Type: application/json\r\n";
    BOOL sent = WinHttpSendRequest(
        request, headers, static_cast<DWORD>(-1L),
        const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
        static_cast<DWORD>(body.size()), 0) && WinHttpReceiveResponse(
        request, NULL);
    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (sent)
        WinHttpQueryHeaders(request,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &status, &statusSize, WINHTTP_NO_HEADER_INDEX);

    bool ok = sent && status >= 200 && status < 300;
    while (ok)
    {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request, &available))
        {
            ok = false;
            break;
        }
        if (available == 0)
            break;
        if (responseBody.size() >= 1024 * 1024)
        {
            ok = false;
            break;
        }
        DWORD take = available;
        size_t remaining = 1024 * 1024 - responseBody.size();
        if (take > remaining)
            take = static_cast<DWORD>(remaining);
        std::vector<char> chunk(take);
        DWORD read = 0;
        if (!WinHttpReadData(request, chunk.data(), take, &read))
        {
            ok = false;
            break;
        }
        responseBody.append(chunk.data(), read);
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return ok;
}

static void* Query(const char* id, DWORD version)
{
    if (SalamanderGeneral == NULL) return NULL;
    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = id;
    query.MinimumVersion = version;
    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));
    return SalamanderGeneral->QueryService(&query, &result) ? result.Interface : NULL;
}

static void EnsureServices()
{
    if (g_released) return;
    if (g_ai == NULL)
        g_ai = static_cast<Salamatrix::AI::IAssistantService*>(Query(SALAMATRIX_SERVICE_AI, SALAMATRIX_AI_VERSION_1_0));
    if (g_ui == NULL)
        g_ui = static_cast<Salamatrix::UI::IUIService*>(Query(SALAMATRIX_SERVICE_UI, SALAMATRIX_UI_VERSION_1_0));
    if (g_runtime == NULL)
        g_runtime = static_cast<Salamatrix::Runtime::IRuntimeService*>(Query(SALAMATRIX_SERVICE_RUNTIME, SALAMATRIX_RUNTIME_VERSION_1_0));
    if (g_runner == NULL)
        g_runner = static_cast<Salamatrix::Automation::IScriptRunner*>(Query(
            SALAMATRIX_SERVICE_SCRIPT_RUNNER,
            SALAMATRIX_SCRIPT_RUNNER_VERSION_1_0));
    if (g_sides == NULL)
        g_sides = static_cast<Salamatrix::Sides::ISidesService*>(Query(
            SALAMATRIX_SERVICE_SIDES, SALAMATRIX_SIDES_VERSION_1_0));
    if (g_ai != NULL)
    {
        // Register descriptors even when their backend is currently
        // unavailable.  Generate() still skips unavailable providers, while
        // the chat can report what needs configuration or what is offline.
        g_ai->RegisterProvider(&g_provider);
        g_ai->RegisterProvider(&g_httpProvider);
    }
}

struct ChatContext
{
    Salamatrix::AI::IAssistantService* Ai;
    Salamatrix::Runtime::IRuntimeService* Runtime;
    Salamatrix::Automation::IScriptRunner* Runner;
    Salamatrix::UI::IDialog* Dialog;
    Salamatrix::UI::IControl* History;
    Salamatrix::UI::IControl* Prompt;
    Salamatrix::UI::IControl* RuntimeChoice;
    Salamatrix::UI::IControl* ProviderChoice;
    CSalamanderForOperationsAbstract* Operation;
    HWND Parent;
    Salamatrix::AI::AssistantResponse LastResponse;
    BOOL HasResponse;
    std::wstring LastSavedPath;
};

static bool SaveAssistantScript(
    HWND parent,
    const char* script,
    std::wstring& savedPath,
    Salamatrix::UI::IUIService* ui);

static bool RunAssistantScript(
    const char* runtimeId,
    const char* script,
    HWND parent,
    Salamatrix::Runtime::IRuntimeService* runtime,
    Salamatrix::Automation::IScriptRunner* runner);

struct RefinementContext
{
    Salamatrix::UI::IControl* Input;
    std::string Value;
    BOOL Accepted;

    RefinementContext()
        : Input(NULL), Accepted(FALSE)
    {
    }
};

static BOOL WINAPI RefinementEvent(
    void* context,
    const Salamatrix::UI::DialogEvent* event)
{
    RefinementContext* refinement = static_cast<RefinementContext*>(context);
    if (refinement == NULL || event == NULL || refinement->Input == NULL)
        return TRUE;
    if (strcmp(event->ControlId, "accept") != 0)
        return TRUE;
    char value[4096];
    if (refinement->Input->GetText(value, sizeof(value)) && value[0] != '\0')
    {
        refinement->Value.assign(value);
        refinement->Accepted = TRUE;
    }
    return TRUE;
}

static bool AskForRefinement(
    HWND parent,
    const char* title,
    const char* prompt,
    const char* accept,
    const char* cancel,
    std::string& value)
{
    value.clear();
    if (g_ui == NULL)
        return false;
    const std::string titleText = title != NULL ? title : "Salamatrix AI";
    const std::string promptText = prompt != NULL ? prompt : "";
    const std::string acceptText = accept != NULL ? accept : "OK";
    const std::string cancelText = cancel != NULL ? cancel : "Cancel";
    Salamatrix::UI::DialogOptions options;
    options.Title = titleText.c_str();
    options.Parent = parent;
    options.Width = 520;
    options.Height = 190;
    Salamatrix::UI::IDialog* dialog = g_ui->CreateSalamatrixDialog(options);
    if (dialog == NULL)
        return false;

    Salamatrix::UI::ControlLayout labelLayout;
    labelLayout.HasBounds = TRUE;
    labelLayout.X = 8; labelLayout.Y = 8; labelLayout.Width = 496; labelLayout.Height = 24;
    Salamatrix::UI::ControlOptions labelOptions;
    labelOptions.Id = "prompt-label";
    labelOptions.Text = promptText.c_str();
    dialog->AddControlEx(Salamatrix::UI::ControlKindLabel, labelOptions, labelLayout);

    Salamatrix::UI::ControlLayout inputLayout;
    inputLayout.HasBounds = TRUE;
    inputLayout.X = 8; inputLayout.Y = 34; inputLayout.Width = 496; inputLayout.Height = 78;
    Salamatrix::UI::ControlOptions inputOptions;
    inputOptions.Id = "feedback";
    inputOptions.Multiline = TRUE;
    RefinementContext refinement;
    refinement.Input = dialog->AddControlEx(
        Salamatrix::UI::ControlKindTextBox, inputOptions, inputLayout);

    Salamatrix::UI::ControlLayout acceptLayout;
    acceptLayout.HasBounds = TRUE;
    acceptLayout.X = 318; acceptLayout.Y = 124; acceptLayout.Width = 100; acceptLayout.Height = 24;
    Salamatrix::UI::ControlOptions acceptOptions;
    acceptOptions.Id = "accept";
    acceptOptions.Text = acceptText.c_str();
    acceptOptions.DialogResult = IDOK;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, acceptOptions, acceptLayout);

    Salamatrix::UI::ControlLayout cancelLayout;
    cancelLayout.HasBounds = TRUE;
    cancelLayout.X = 424; cancelLayout.Y = 124; cancelLayout.Width = 80; cancelLayout.Height = 24;
    Salamatrix::UI::ControlOptions cancelOptions;
    cancelOptions.Id = "cancel";
    cancelOptions.Text = cancelText.c_str();
    cancelOptions.DialogResult = IDCANCEL;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, cancelOptions, cancelLayout);

    dialog->SetEventCallback(RefinementEvent, &refinement);
    dialog->ShowModal();
    dialog->Release();
    if (!refinement.Accepted)
        return false;
    value.swap(refinement.Value);
    return !value.empty();
}

static bool Utf8ToWideString(const char* value, std::wstring& result)
{
    result.clear();
    if (value == NULL) return false;
    int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, NULL, 0);
    if (length <= 0) return false;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1,
                            buffer.data(), length) <= 0)
        return false;
    result.assign(buffer.data());
    return true;
}

static bool SaveResponseScript(ChatContext* chat)
{
    if (chat == NULL || !chat->HasResponse || chat->LastResponse.Summary.Script[0] == '\0')
        return false;
    return SaveAssistantScript(chat->Parent, chat->LastResponse.Summary.Script, chat->LastSavedPath, g_ui);
}

static bool SaveAssistantScript(
    HWND parent,
    const char* script,
    std::wstring& savedPath,
    Salamatrix::UI::IUIService* ui)
{
    if (script == NULL || ui == NULL || parent == NULL)
        return false;
    std::vector<char> path(32768, '\0');
    const char filter[] = "JavaScript|*.js;*.mjs|Python|*.py|PowerShell|*.ps1|PHP|*.php|All files|*.*";
    if (!ui->PickFile(parent, TRUE, "Save generated script",
                      filter, "", path.data(), static_cast<DWORD>(path.size())))
        return false;
    std::wstring widePath;
    if (!Utf8ToWideString(path.data(), widePath) ||
        !SaveAssistantStringToUtf8File(widePath, script))
        return false;
    savedPath = AssistantWin32Path(widePath);
    return true;
}

static bool SaveAssistantExtensionPackage(
    HWND parent,
    Salamatrix::UI::IUIService* ui,
    Salamatrix::Runtime::IRuntimeService* runtime,
    const Salamatrix::AI::AssistantResponse& response)
{
    if (parent == NULL || ui == NULL || runtime == NULL ||
        response.Summary.Script[0] == '\0' || response.Summary.Title[0] == '\0')
        return false;

    std::string runtimeId;
    if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(response.ResponseJson, "runtime", &runtimeId) ||
        runtimeId.empty())
    {
        ui->ShowMessageBox(parent, LoadAssistantString(IDS_AI_RUNTIME_MISSING).c_str(),
                           LoadAssistantString(IDS_AI_TITLE).c_str(),
                           MB_OK | MB_ICONWARNING);
        return false;
    }
    Salamatrix::Runtime::IRuntimeAdapter* adapter = runtime->FindAdapter(runtimeId.c_str(), 0);
    if (adapter == NULL || !adapter->IsAvailable())
        return false;

    std::string extension;
    if (!GetAssistantRuntimeExtension(adapter, extension))
        return false;

    std::vector<char> selectedFolder(32768, '\0');
    if (!ui->PickFolder(parent, "Choose a directory for the extension package",
                        "", selectedFolder.data(), static_cast<DWORD>(selectedFolder.size())))
        return false;

    std::string extensionId =
        MakeAssistantExtensionId(response.Summary.Title);
    std::wstring parentPath;
    if (!Utf8ToWideString(selectedFolder.data(), parentPath))
        return false;
    parentPath = AssistantWin32Path(parentPath);

    std::wstring packagePath = parentPath;
    if (!packagePath.empty() && packagePath[packagePath.size() - 1] != L'\\')
        packagePath.push_back(L'\\');
    std::wstring extensionIdPath;
    if (!AssistantUtf8ToWide(extensionId.c_str(), extensionIdPath))
        return false;
    packagePath += extensionIdPath;
    packagePath = AssistantWin32Path(packagePath);

    if (GetFileAttributesW(packagePath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        ui->ShowMessageBox(parent, LoadAssistantString(IDS_AI_EXT_INVALID).c_str(),
                           LoadAssistantString(IDS_AI_TITLE).c_str(),
                           MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!CreateDirectoryW(packagePath.c_str(), NULL))
        return false;

    const std::string* capabilitiesRaw = NULL;
    std::string capabilities = "[]";
    Salamatrix::Runtime::Protocol::Json::FindRawMember(
        response.ResponseJson, "capabilities", &capabilities);
    if (capabilities.size() >= 2 && capabilities.front() == '[' &&
        capabilities.back() == ']')
    {
        capabilitiesRaw = &capabilities;
    }
    const std::string id = MakeAssistantExtensionId(response.Summary.Title);
    const std::string title = response.Summary.Title;
    const std::string description = response.Summary.Description;
    const std::string entryPoint = std::string("main") + extension;
    const std::string manifest =
        std::string("{\n  \"schemaVersion\": 1,\n") +
        "  \"id\": \"" + EscapeAssistantContext(id.c_str()) +
        "\",\n  \"name\": \"" + EscapeAssistantContext(title.c_str()) +
        "\",\n  \"version\": \"1.0.0\",\n  \"description\": \"" +
        EscapeAssistantContext(description.c_str()) +
        "\",\n  \"runtime\": \"" +
        EscapeAssistantContext(runtimeId.c_str()) +
        "\",\n  \"entryPoint\": \"" +
        EscapeAssistantContext(entryPoint.c_str()) +
        "\",\n  \"capabilities\": " +
        (capabilitiesRaw != NULL ? *capabilitiesRaw : "[]") +
        ",\n  \"commands\": [{\"id\": \"" +
        EscapeAssistantContext(id.c_str()) +
        "\", \"title\": \"" + EscapeAssistantContext(response.Summary.Title) +
        "\", \"handler\": \"main\", \"menu\": \"plugin\", \"requires\": \"any\"}]\n}\n";

    const std::wstring manifestPath = packagePath + L"\\extension.json";
    std::wstring extensionWide;
    if (!AssistantUtf8ToWide(extension.c_str(), extensionWide))
    {
        RemoveDirectoryW(packagePath.c_str());
        return false;
    }
    const std::wstring scriptPath = packagePath + L"\\main" + extensionWide;
    if (!SaveAssistantStringToUtf8File(manifestPath, manifest.c_str()) ||
        !SaveAssistantStringToUtf8File(scriptPath, response.Summary.Script))
    {
        DeleteFileW(manifestPath.c_str());
        DeleteFileW(scriptPath.c_str());
        RemoveDirectoryW(packagePath.c_str());
        return false;
    }
    if (g_runner != NULL &&
        IsCurrentService(SALAMATRIX_SERVICE_SCRIPT_RUNNER,
                         SALAMATRIX_SCRIPT_RUNNER_VERSION_1_0,
                         g_runner))
        g_runner->RefreshExtensions();
    if (SalamanderGeneral != NULL)
        SalamanderGeneral->PostPluginMenuChanged();
    return TRUE;
}

static const char* RuntimeIdFromChoice(ChatContext* chat)
{
    static char id[128];
    id[0] = '\0';
    if (chat == NULL || chat->RuntimeChoice == NULL)
        return id;
    chat->RuntimeChoice->GetText(id, sizeof(id));
    return id;
}

static const char* ProviderIdFromChoice(ChatContext* chat)
{
    static char id[128];
    id[0] = '\0';
    if (chat == NULL || chat->ProviderChoice == NULL ||
        !chat->ProviderChoice->GetText(id, sizeof(id)) ||
        id[0] == '\0' || _stricmp(id, "auto") == 0)
        return NULL;
    return id;
}

static const char* ConfiguredProviderId()
{
    static char provider[128];
    DWORD length = GetEnvironmentVariableA(
        "SALAMATRIX_AI_PROVIDER", provider, _countof(provider));
    if (length == 0 || length >= _countof(provider))
    {
        provider[0] = '\0';
        return NULL;
    }
    return provider;
}

static bool RunResponseScript(ChatContext* chat)
{
    if (chat == NULL || !chat->HasResponse || chat->LastResponse.Summary.Script[0] == '\0')
        return false;
    if (!Salamatrix::AI::IsSafeToRun(chat->LastResponse.Summary))
        return false;
    if (chat->LastSavedPath.empty() && !SaveResponseScript(chat))
        return false;
    return RunAssistantScript(RuntimeIdFromChoice(chat),
                             chat->LastResponse.Summary.Script,
                             chat->Parent, chat->Runtime, chat->Runner);
}

static bool RunAssistantScript(
    const char* runtimeId,
    const char* script,
    HWND parent,
    Salamatrix::Runtime::IRuntimeService* runtime,
    Salamatrix::Automation::IScriptRunner* runner)
{
    if (script == NULL || parent == NULL)
        return false;
    AssistantTemporaryScript temporary;
    std::string extension;
    Salamatrix::Runtime::IRuntimeAdapter* adapter = NULL;
    bool canUseRunner = false;
    if (runtime == NULL || runtimeId == NULL || runtimeId[0] == '\0')
        return false;
    if (runtimeId != NULL && runtimeId[0] != '\0')
    {
        adapter = runtime->FindAdapter(runtimeId, 0);
        if (adapter == NULL || !adapter->IsAvailable() ||
            !GetAssistantRuntimeExtension(adapter, extension))
            return false;
        canUseRunner = runner != NULL;
    }
    if (extension.empty())
        extension = ".js";
    if (!CreateAssistantTemporaryScript(script, extension.c_str(), temporary))
        return false;

    if (canUseRunner && runtimeId != NULL && runtimeId[0] != '\0')
    {
        Salamatrix::Automation::GeneratedScriptRequest request;
        request.EntryPoint = temporary.ScriptPath.c_str();
        request.RuntimeId = runtimeId;
        request.ExtensionId = "salamatrix.ai.generated";
        request.ParentWindow = parent;
        request.Operation = NULL;
        request.TimeoutMs = 120000;
        Salamatrix::Automation::GeneratedScriptResult result;
        const bool executed = runner->ExecuteGenerated(&request, &result) &&
                             result.Succeeded != FALSE;
        temporary.Cleanup();
        return executed;
    }

    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.ExtensionId = "salamatrix.ai.generated";
    request.CommandId = "generated";
    request.ParentWindow = parent;
    request.TimeoutMs = 120000;
    request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap |
                    Salamatrix::Runtime::RuntimeExecutionFlagOneShotWorker;
    const Salamatrix::Runtime::RuntimeAdapterDescriptor* descriptor =
        adapter->GetDescriptor();
    if (descriptor != NULL && descriptor->RuntimeId != NULL &&
        _strnicmp(descriptor->RuntimeId, "Automation.", 10) == 0)
        request.Flags = Salamatrix::Runtime::RuntimeExecutionFlagNone;
    request.EntryPoint = temporary.ScriptPath.c_str();
    Salamatrix::Runtime::RuntimeExecutionResult result;
    const bool executed = adapter->Execute(&request, &result) &&
                          result.Status == Salamatrix::Runtime::RuntimeExecutionStatusSucceeded;
    temporary.Cleanup();
    return executed;
}

static BOOL WINAPI ChatEvent(void* context, const Salamatrix::UI::DialogEvent* event)
{
    ChatContext* chat = static_cast<ChatContext*>(context);
    if (chat == NULL || event == NULL || event->ControlId[0] == '\0')
        return TRUE;
    if (strcmp(event->ControlId, "preview") == 0)
    {
        if (chat->HasResponse && g_ui != NULL)
        {
            std::string preview = std::string(chat->LastResponse.Summary.Title) +
                "\n\n" + chat->LastResponse.Summary.Description +
                "\n\n" + LoadAssistantString(IDS_AI_PREVIEW_SUMMARY) +
                "\n\n" + chat->LastResponse.Summary.Script;
            g_ui->ShowMessageBox(chat->Parent, preview.c_str(),
                                 LoadAssistantString(IDS_AI_PREVIEW_TITLE).c_str(),
                                 MB_OK | MB_ICONINFORMATION);
        }
        return TRUE;
    }
    if (strcmp(event->ControlId, "export") == 0)
    {
        const bool exported = chat != NULL && chat->HasResponse &&
                             SaveAssistantExtensionPackage(chat->Parent, g_ui, chat->Runtime, chat->LastResponse);
        if (g_ui != NULL)
        {
            const std::string& message = LoadAssistantString(exported ? IDS_AI_EXT_SUCCEEDED : IDS_AI_EXT_FAILED);
            g_ui->ShowMessageBox(
                chat->Parent, message.c_str(),
                LoadAssistantString(IDS_AI_TITLE).c_str(),
                MB_OK | (exported ? MB_ICONINFORMATION : MB_ICONWARNING));
        }
        return TRUE;
    }
    if (strcmp(event->ControlId, "save") == 0)
    {
        const bool saved = SaveResponseScript(chat);
        if (g_ui != NULL)
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(saved ? IDS_AI_SAVE_SUCCEEDED : IDS_AI_SAVE_FAILED).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_OK | (saved ? MB_ICONINFORMATION : MB_ICONWARNING));
        return TRUE;
    }
    if (strcmp(event->ControlId, "run") == 0)
    {
        const bool ran = RunResponseScript(chat);
        if (g_ui != NULL)
            g_ui->ShowMessageBox(
                chat->Parent, LoadAssistantString(ran ? IDS_AI_RUN_SUCCEEDED : IDS_AI_RUN_FAILED).c_str(),
                LoadAssistantString(IDS_AI_TITLE).c_str(),
                MB_OK | (ran ? MB_ICONINFORMATION : MB_ICONWARNING));
        return TRUE;
    }
    if (strcmp(event->ControlId, "ask") != 0 ||
        chat->Prompt == NULL || chat->History == NULL || chat->Ai == NULL)
        return TRUE;
    char prompt[4096];
    if (!chat->Prompt->GetText(prompt, sizeof(prompt)) || prompt[0] == '\0')
        return TRUE;
    std::string line = std::string("You: ") + prompt;
    chat->History->AddItem(line.c_str());
    Salamatrix::AI::AssistantResponse response;
    const char* selectedProvider = ProviderIdFromChoice(chat);
    if (selectedProvider == NULL)
        selectedProvider = ConfiguredProviderId();

    const std::string panelContext = BuildAssistantPanelContext(g_sides);
    std::string existingScript;
    std::string feedback;
    if (chat->HasResponse && chat->LastResponse.Summary.Script[0] != '\0')
    {
        existingScript.assign(chat->LastResponse.Summary.Script);
        feedback.assign(prompt);
    }
    std::string runtimeId;
    bool generated = false;
    for (int iteration = 0; iteration < 3; ++iteration)
    {
        Salamatrix::AI::AssistantRequest request;
        request.Prompt = prompt;
        request.ContextJson = panelContext.c_str();
        request.RuntimeId = runtimeId.empty() ? RuntimeIdFromChoice(chat) : runtimeId.c_str();
        request.ExistingScript = existingScript.empty() ? NULL : existingScript.c_str();
        request.Feedback = feedback.empty() ? NULL : feedback.c_str();
        request.MaxOutputBytes = 65536;
        if (!chat->Ai->GenerateWithRepair(selectedProvider, &request, &response, 2) ||
            response.Status != Salamatrix::AI::AssistantStatusSucceeded)
        {
            generated = false;
            break;
        }
        generated = true;
        runtimeId.clear();
        if (response.ResponseJson[0] != '\0')
            Salamatrix::Runtime::Protocol::Json::FindStringMember(response.ResponseJson, "runtime", &runtimeId);
        if (iteration == 2)
            break;
        if (g_ui == NULL ||
            g_ui->ShowMessageBox(chat->Parent, LoadAssistantString(IDS_AI_REFINE_QUESTION).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_YESNO | MB_ICONQUESTION) != IDYES)
            break;
        std::string refinement;
        if (!AskForRefinement(
                chat->Parent,
                LoadAssistantString(IDS_AI_TITLE).c_str(),
                LoadAssistantString(IDS_AI_REFINE_PROMPT).c_str(),
                LoadAssistantString(IDS_AI_REFINE_ACCEPT).c_str(),
                LoadAssistantString(IDS_AI_REFINE_CANCEL).c_str(),
                refinement))
            break;
        existingScript.assign(response.Summary.Script);
        feedback.swap(refinement);
    }

    if (!generated)
    {
        if (g_ui != NULL)
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(IDS_AI_GENERATE_FAILED).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_OK | MB_ICONWARNING);
        chat->Prompt->SetText("");
        return TRUE;
    }

    std::string summary =
        std::string(response.Summary.Title) + "\n\n" +
        response.Summary.Description + "\n\n" +
        LoadAssistantString(IDS_AI_PREVIEW_SUMMARY) + "\n";
    if (Salamatrix::AI::IsSafeToRun(response.Summary))
        summary += LoadAssistantString(IDS_AI_EFFECTS_READONLY);
    else
        summary += LoadAssistantString(IDS_AI_EFFECTS_REVIEW);
    chat->History->AddItem((std::string("AI: ") + summary).c_str());

    if (g_ui != NULL)
    {
        g_ui->CopyTextToClipboard(response.Summary.Script, TRUE, chat->Parent);
        g_ui->ShowMessageBox(chat->Parent, summary.c_str(),
                             LoadAssistantString(IDS_AI_PREVIEW_TITLE).c_str(),
                             MB_OK | MB_ICONINFORMATION);
        if (response.Summary.Script[0] != '\0' && Salamatrix::AI::IsSafeToRun(response.Summary) &&
            !runtimeId.empty() &&
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(IDS_AI_RUN_QUESTION).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            const bool ran = RunAssistantScript(
                runtimeId.empty() ? RuntimeIdFromChoice(chat) : runtimeId.c_str(),
                response.Summary.Script,
                chat->Parent,
                chat->Runtime,
                chat->Runner);
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(ran ? IDS_AI_RUN_SUCCEEDED : IDS_AI_RUN_FAILED).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_OK | (ran ? MB_ICONINFORMATION : MB_ICONWARNING));
        }
        if (g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(IDS_AI_SAVEQUESTION).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            const bool saved = SaveResponseScript(chat);
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(saved ? IDS_AI_SAVE_SUCCEEDED : IDS_AI_SAVE_FAILED).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_OK | (saved ? MB_ICONINFORMATION : MB_ICONWARNING));
        }
        if (!runtimeId.empty() &&
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(IDS_AI_EXT_QUESTION).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_YESNO | MB_ICONQUESTION) == IDYES)
        {
            const bool exported = SaveAssistantExtensionPackage(chat->Parent, g_ui, chat->Runtime, response);
            g_ui->ShowMessageBox(chat->Parent,
                                 LoadAssistantString(exported ? IDS_AI_EXT_SUCCEEDED : IDS_AI_EXT_FAILED).c_str(),
                                 LoadAssistantString(IDS_AI_TITLE).c_str(),
                                 MB_OK | (exported ? MB_ICONINFORMATION : MB_ICONWARNING));
        }
    }

    chat->LastResponse = response;
    chat->HasResponse = TRUE;
    chat->Prompt->SetText("");
    return TRUE;
}

static void ShowChat(HWND parent, CSalamanderForOperationsAbstract* operation)
{
    EnsureServices();
    if (g_ai == NULL || g_ui == NULL)
    {
        if (SalamanderGeneral != NULL)
            SalamanderGeneral->SalMessageBox(parent,
                "Salamatrix Framework is not loaded.", "Salamatrix AI",
                MB_OK | MB_ICONWARNING);
        return;
    }
    Salamatrix::UI::DialogOptions options;
    options.Title = "Salamatrix AI";
    options.Parent = parent;
    options.Width = 760;
    options.Height = 560;
    Salamatrix::UI::IDialog* dialog = g_ui->CreateSalamatrixDialog(options);
    if (dialog == NULL) return;
    Salamatrix::UI::ControlLayout historyLayout;
    historyLayout.HasBounds = TRUE;
    historyLayout.X = 8; historyLayout.Y = 8; historyLayout.Width = 744; historyLayout.Height = 330;
    Salamatrix::UI::ControlOptions historyOptions;
    historyOptions.Id = "history";
    Salamatrix::UI::IControl* history = dialog->AddControlEx(
        Salamatrix::UI::ControlKindListView, historyOptions, historyLayout);
    Salamatrix::UI::ControlLayout promptLayout;
    promptLayout.HasBounds = TRUE;
    promptLayout.X = 8; promptLayout.Y = 345; promptLayout.Width = 548; promptLayout.Height = 76;
    Salamatrix::UI::ControlOptions promptOptions;
    promptOptions.Id = "prompt";
    promptOptions.Multiline = TRUE;
    Salamatrix::UI::IControl* prompt = dialog->AddControlEx(
        Salamatrix::UI::ControlKindTextBox, promptOptions, promptLayout);
    Salamatrix::UI::ControlLayout runtimeLayout;
    runtimeLayout.HasBounds = TRUE;
    runtimeLayout.X = 568; runtimeLayout.Y = 345; runtimeLayout.Width = 184; runtimeLayout.Height = 24;
    Salamatrix::UI::ControlOptions runtimeOptions;
    runtimeOptions.Id = "runtime";
    Salamatrix::UI::IControl* runtimeChoice = dialog->AddControlEx(
        Salamatrix::UI::ControlKindComboBox, runtimeOptions, runtimeLayout);

    Salamatrix::UI::ControlLayout providerLayout;
    providerLayout.HasBounds = TRUE;
    providerLayout.X = 568; providerLayout.Y = 375; providerLayout.Width = 184; providerLayout.Height = 24;
    Salamatrix::UI::ControlOptions providerOptions;
    providerOptions.Id = "provider";
    Salamatrix::UI::IControl* providerChoice = dialog->AddControlEx(
        Salamatrix::UI::ControlKindComboBox, providerOptions, providerLayout);
    if (providerChoice != NULL)
    {
        providerChoice->AddItem("auto");
        const char* configured = ConfiguredProviderId();
        int configuredIndex = 0;
        for (int index = 0; index < g_ai->GetProviderCount(); ++index)
        {
            Salamatrix::AI::IAssistantProvider* provider = g_ai->GetProvider(index);
            const Salamatrix::AI::AssistantProviderDescriptor* descriptor =
                provider != NULL ? provider->GetDescriptor() : NULL;
            if (descriptor == NULL || descriptor->ProviderId == NULL ||
                !provider->IsAvailable())
                continue;
            providerChoice->AddItem(descriptor->ProviderId);
            if (configured != NULL &&
                _stricmp(configured, descriptor->ProviderId) == 0)
                configuredIndex = providerChoice->GetItemCount() - 1;
        }
        providerChoice->SetSelectedIndex(configuredIndex);
    }
    std::string providerStatus = "Providers: ";
    for (int index = 0; index < g_ai->GetProviderCount(); ++index)
    {
        Salamatrix::AI::IAssistantProvider* provider = g_ai->GetProvider(index);
        const Salamatrix::AI::AssistantProviderDescriptor* descriptor =
            provider != NULL ? provider->GetDescriptor() : NULL;
        if (descriptor == NULL || descriptor->ProviderId == NULL)
            continue;
        if (providerStatus.size() > strlen("Providers: "))
            providerStatus += ", ";
        providerStatus += descriptor->ProviderId;
        providerStatus += provider->IsAvailable() ? " (ready)" : " (unavailable)";
    }
    Salamatrix::UI::ControlLayout statusLayout;
    statusLayout.HasBounds = TRUE;
    statusLayout.X = 568; statusLayout.Y = 500; statusLayout.Width = 184; statusLayout.Height = 45;
    Salamatrix::UI::ControlOptions statusOptions;
    statusOptions.Id = "provider-status";
    statusOptions.Text = providerStatus.c_str();
    statusOptions.ReadOnly = TRUE;
    statusOptions.Multiline = TRUE;
    dialog->AddControlEx(Salamatrix::UI::ControlKindTextBox, statusOptions, statusLayout);
    if (runtimeChoice != NULL && g_runtime != NULL)
    {
        for (int index = 0; index < g_runtime->GetAdapterCount(); ++index)
        {
            Salamatrix::Runtime::IRuntimeAdapter* adapter = g_runtime->GetAdapter(index);
            const Salamatrix::Runtime::RuntimeAdapterDescriptor* descriptor =
                adapter != NULL ? adapter->GetDescriptor() : NULL;
            if (descriptor != NULL && descriptor->RuntimeId != NULL)
                runtimeChoice->AddItem(descriptor->RuntimeId);
        }
        if (runtimeChoice->GetItemCount() > 0)
            runtimeChoice->SetSelectedIndex(0);
    }
    Salamatrix::UI::ControlLayout askLayout;
    askLayout.HasBounds = TRUE;
    askLayout.X = 568; askLayout.Y = 405; askLayout.Width = 86; askLayout.Height = 24;
    Salamatrix::UI::ControlOptions askOptions;
    askOptions.Id = "ask";
    askOptions.Text = LoadAssistantString(IDS_AI_ASK_LABEL).c_str();
    askOptions.KeepOpen = TRUE;
    askOptions.DialogResult = 0;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, askOptions, askLayout);
    Salamatrix::UI::ControlOptions previewOptions;
    previewOptions.Id = "preview"; previewOptions.Text = LoadAssistantString(IDS_AI_PREVIEW_LABEL).c_str(); previewOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout previewLayout;
    previewLayout.HasBounds = TRUE; previewLayout.X = 660; previewLayout.Y = 405; previewLayout.Width = 92; previewLayout.Height = 24;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, previewOptions, previewLayout);
    Salamatrix::UI::ControlOptions saveOptions;
    saveOptions.Id = "save"; saveOptions.Text = LoadAssistantString(IDS_AI_SAVE_LABEL).c_str(); saveOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout saveLayout;
    saveLayout.HasBounds = TRUE; saveLayout.X = 568; saveLayout.Y = 435; saveLayout.Width = 86; saveLayout.Height = 24;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, saveOptions, saveLayout);
    Salamatrix::UI::ControlOptions runOptions;
    runOptions.Id = "run"; runOptions.Text = LoadAssistantString(IDS_AI_RUN_LABEL).c_str(); runOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout runLayout;
    runLayout.HasBounds = TRUE; runLayout.X = 660; runLayout.Y = 435; runLayout.Width = 92; runLayout.Height = 24;
    Salamatrix::UI::ControlOptions exportOptions;
    exportOptions.Id = "export";
    exportOptions.Text = LoadAssistantString(IDS_AI_EXPORT_LABEL).c_str();
    exportOptions.KeepOpen = TRUE;
    Salamatrix::UI::ControlLayout exportLayout;
    exportLayout.HasBounds = TRUE; exportLayout.X = 568; exportLayout.Y = 465; exportLayout.Width = 184; exportLayout.Height = 24;
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, exportOptions, exportLayout);
    dialog->AddControlEx(Salamatrix::UI::ControlKindButton, runOptions, runLayout);
    if (history != NULL) history->AddColumn("Conversation", 700);
    ChatContext chat = { g_ai, g_runtime, g_runner, dialog, history, prompt, runtimeChoice, providerChoice,
                         operation, parent, Salamatrix::AI::AssistantResponse(), FALSE, std::wstring() };
    dialog->SetEventCallback(ChatEvent, &chat);
    dialog->ShowModal();
    dialog->Release();
}
} // namespace

CLocalAssistantProvider::CLocalAssistantProvider()
{
    m_descriptor.ProviderId = "local.command";
    m_descriptor.DisplayName = "Local command assistant";
    m_descriptor.ProviderVersion = 0x00010000;
    m_descriptor.Flags = 0;
}

void CLocalAssistantProvider::ResolveCommand() const
{
    wchar_t value[4096];
    DWORD length = GetEnvironmentVariableW(L"SALAMATRIX_AI_COMMAND", value, _countof(value));
    if (length == 0 || length >= _countof(value)) m_commandLine.clear();
    else m_commandLine.assign(value, length);
}

const Salamatrix::AI::AssistantProviderDescriptor* WINAPI
CLocalAssistantProvider::GetDescriptor() const { return &m_descriptor; }

BOOL WINAPI CLocalAssistantProvider::IsAvailable() const
{
    ResolveCommand();
    return m_commandLine.empty() ? FALSE : TRUE;
}

BOOL WINAPI CLocalAssistantProvider::Generate(
    const Salamatrix::AI::AssistantRequest* request,
    Salamatrix::AI::AssistantResponse* response)
{
    if (response == NULL || response->StructSize < sizeof(*response)) return FALSE;
    *response = Salamatrix::AI::AssistantResponse();
    if (request == NULL || request->StructSize < sizeof(*request))
    {
        AssistantFailure(response, Salamatrix::AI::AssistantStatusFailed, E_INVALIDARG,
                         L"The assistant request is invalid.");
        return FALSE;
    }
    if (!IsAvailable())
    {
        AssistantFailure(response, Salamatrix::AI::AssistantStatusUnavailable,
                         HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
                         L"Set SALAMATRIX_AI_COMMAND to a local assistant wrapper.");
        return FALSE;
    }
    std::string json = std::string("{\"apiVersion\":\"") +
        EscapeJson(request->ApiVersion != NULL ? request->ApiVersion : "1.0") +
        "\",\"prompt\":\"" + EscapeJson(request->Prompt != NULL ? request->Prompt : "") +
        "\",\"context\":" + (request->ContextJson != NULL ? request->ContextJson : "{}") +
        ",\"runtime\":\"" + EscapeJson(request->RuntimeId != NULL ? request->RuntimeId : "") +
        "\",\"existingScript\":\"" + EscapeJson(request->ExistingScript != NULL ? request->ExistingScript : "") +
        "\",\"feedback\":\"" + EscapeJson(request->Feedback != NULL ? request->Feedback : "") +
        "\",\"maxOutputBytes\":" + std::to_string(request->MaxOutputBytes ? request->MaxOutputBytes : 65536) + "}\n";
    SECURITY_ATTRIBUTES security = { sizeof(security), NULL, TRUE };
    HANDLE childIn = NULL, parentIn = NULL, childOut = NULL, parentOut = NULL;
    if (!CreatePipe(&parentIn, &childIn, &security, 0) || !SetHandleInformation(parentIn, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&parentOut, &childOut, &security, 0) || !SetHandleInformation(parentOut, HANDLE_FLAG_INHERIT, 0))
    {
        if (parentIn) CloseHandle(parentIn); if (childIn) CloseHandle(childIn);
        if (parentOut) CloseHandle(parentOut); if (childOut) CloseHandle(childOut);
        AssistantFailure(response, Salamatrix::AI::AssistantStatusFailed, HRESULT_FROM_WIN32(GetLastError()),
                         L"Unable to create assistant pipes.");
        return FALSE;
    }
    STARTUPINFOW startup = {}; PROCESS_INFORMATION process = {};
    startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childIn; startup.hStdOutput = childOut; startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    std::vector<wchar_t> command(m_commandLine.begin(), m_commandLine.end()); command.push_back(L'\0');
    BOOL created = CreateProcessW(NULL, command.data(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup, &process);
    CloseHandle(childIn); CloseHandle(childOut);
    if (!created)
    {
        CloseHandle(parentIn); CloseHandle(parentOut);
        AssistantFailure(response, Salamatrix::AI::AssistantStatusFailed, HRESULT_FROM_WIN32(GetLastError()),
                         L"Unable to start the local assistant command.");
        return FALSE;
    }
    DWORD written = 0;
    if (!WriteFile(parentIn, json.data(), static_cast<DWORD>(json.size()), &written, NULL) || written != json.size())
        TerminateProcess(process.hProcess, 1);
    CloseHandle(parentIn);
    std::string output; ULONGLONG start = GetTickCount64();
    DWORD timeout = request->TimeoutMs > 120000 ? 120000 : (request->TimeoutMs ? request->TimeoutMs : 120000);
    bool timedOut = false;
    for (;;)
    {
        DWORD available = 0;
        while (PeekNamedPipe(parentOut, NULL, 0, NULL, &available, NULL) && available)
        {
            char buffer[4096]; DWORD count = 0, take = available < sizeof(buffer) ? available : sizeof(buffer);
            if (!ReadFile(parentOut, buffer, take, &count, NULL) || !count) break;
            if (output.size() < 1024 * 1024) output.append(buffer, buffer + (count < 1024 * 1024 - output.size() ? count : 1024 * 1024 - output.size()));
        }
        if (WaitForSingleObject(process.hProcess, 10) == WAIT_OBJECT_0) break;
        if (GetTickCount64() - start >= timeout) { timedOut = true; TerminateProcess(process.hProcess, 1); break; }
    }
    WaitForSingleObject(process.hProcess, 1000);
    for (;;)
    {
        DWORD available = 0;
        if (!PeekNamedPipe(parentOut, NULL, 0, NULL, &available, NULL) || !available)
            break;
        char buffer[4096]; DWORD count = 0;
        DWORD take = available < sizeof(buffer) ? available : sizeof(buffer);
        if (!ReadFile(parentOut, buffer, take, &count, NULL) || !count)
            break;
        if (output.size() < 1024 * 1024)
        {
            size_t remaining = 1024 * 1024 - output.size();
            output.append(buffer, buffer + (count < remaining ? count : remaining));
        }
    }
    DWORD exitCode = 1; GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread); CloseHandle(process.hProcess); CloseHandle(parentOut);
    if (timedOut) { AssistantFailure(response, Salamatrix::AI::AssistantStatusCancelled, HRESULT_FROM_WIN32(ERROR_TIMEOUT), L"The assistant timed out."); return FALSE; }
    if (exitCode != 0) { AssistantFailure(response, Salamatrix::AI::AssistantStatusFailed, HRESULT_FROM_WIN32(exitCode), L"The assistant command failed."); return FALSE; }
    size_t first = output.find_first_not_of(" \t\r\n"), last = output.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last <= first || last - first + 1 >= sizeof(response->ResponseJson) || !StructuredJson(output))
    { AssistantFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse, HRESULT_FROM_WIN32(ERROR_INVALID_DATA), L"The assistant returned invalid JSON."); return FALSE; }
    size_t length = last - first + 1; memcpy(response->ResponseJson, output.data() + first, length); response->ResponseJson[length] = '\0';
    response->OutputLength = static_cast<DWORD>(length); response->Status = Salamatrix::AI::AssistantStatusSucceeded; response->ErrorCode = S_OK; return TRUE;
}

CLocalHttpAssistantProvider::CLocalHttpAssistantProvider()
{
    m_descriptor.ProviderId = "local.ollama";
    m_descriptor.DisplayName = "Local model endpoint";
    m_descriptor.ProviderVersion = 0x00010000;
    m_descriptor.Flags = 0;
}

void CLocalHttpAssistantProvider::ResolveConfiguration() const
{
    m_url.clear();
    m_model.clear();
    m_protocol = L"ollama";
    std::wstring configuredProtocol;
    const bool protocolConfigured =
        ReadEnvironmentString(L"SALAMATRIX_AI_PROTOCOL", configuredProtocol);
    if (protocolConfigured)
        m_protocol = configuredProtocol;
    ReadEnvironmentString(L"SALAMATRIX_AI_OLLAMA_URL", m_url);
    if (m_url.empty())
        ReadEnvironmentString(L"SALAMATRIX_AI_HTTP_URL", m_url);
    std::wstring llamaUrl;
    if (ReadEnvironmentString(L"SALAMATRIX_AI_LLAMA_URL", llamaUrl))
    {
        m_url = llamaUrl;
        if (!protocolConfigured)
            m_protocol = L"chat-completions";
    }
    ReadEnvironmentString(L"SALAMATRIX_AI_MODEL", m_model);
    if (m_model.empty())
        ReadEnvironmentString(L"SALAMATRIX_AI_OLLAMA_MODEL", m_model);
    // Do not advertise an unreachable provider by default.  Setting only a
    // model opts into the conventional local Ollama endpoint.
    if (m_url.empty() && !m_model.empty())
    {
        m_url = IsChatCompletionsProtocol(m_protocol)
                    ? L"http://127.0.0.1:8080/v1/chat/completions"
                    : L"http://127.0.0.1:11434/api/generate";
    }
    if (!m_url.empty() && m_model.empty())
        m_model = IsChatCompletionsProtocol(m_protocol)
                      ? L"local-model"
                      : L"llama3.2";
}

const Salamatrix::AI::AssistantProviderDescriptor* WINAPI
CLocalHttpAssistantProvider::GetDescriptor() const
{
    return &m_descriptor;
}

BOOL WINAPI CLocalHttpAssistantProvider::IsAvailable() const
{
    ResolveConfiguration();
    return (!m_url.empty() && !m_model.empty()) ? TRUE : FALSE;
}

BOOL WINAPI CLocalHttpAssistantProvider::Generate(
    const Salamatrix::AI::AssistantRequest* request,
    Salamatrix::AI::AssistantResponse* response)
{
    if (response == NULL || response->StructSize < sizeof(*response))
        return FALSE;
    *response = Salamatrix::AI::AssistantResponse();
    if (request == NULL || request->StructSize < sizeof(*request))
    {
        AssistantFailure(response, Salamatrix::AI::AssistantStatusFailed,
                         E_INVALIDARG, L"The assistant request is invalid.");
        return FALSE;
    }
    ResolveConfiguration();
    if (m_url.empty() || m_model.empty())
    {
        AssistantFailure(response, Salamatrix::AI::AssistantStatusUnavailable,
                         HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
                         L"Configure SALAMATRIX_AI_MODEL and a local endpoint (SALAMATRIX_AI_OLLAMA_URL or SALAMATRIX_AI_LLAMA_URL).");
        return FALSE;
    }

    std::string api = g_ai != NULL ? g_ai->GetApiDescriptionSlice("all") : "{}";
    std::string prompt = request->Prompt != NULL ? request->Prompt : "";
    if (request->ExistingScript != NULL && request->ExistingScript[0] != '\0')
    {
        prompt += "\n\nExisting script to repair or extend:\n";
        prompt += request->ExistingScript;
    }
    if (request->Feedback != NULL && request->Feedback[0] != '\0')
    {
        prompt += "\n\nUser feedback on the previous result:\n";
        prompt += request->Feedback;
    }
    if (request->ContextJson != NULL && request->ContextJson[0] != '\0')
    {
        prompt += "\n\nCurrent Salamander context (JSON):\n";
        prompt += request->ContextJson;
    }
    std::string system =
        "You are the local Open Salamander script assistant. Return only one "
        "JSON object with title, description, capabilities (array), "
        "estimatedEffects (object), and script. The script must use the "
        "Salamander API described below; do not invent privileged APIs. "
        "API description: " + api;
    const bool chatCompletions = IsChatCompletionsProtocol(m_protocol);
    std::string body;
    if (chatCompletions)
    {
        body = std::string("{\"model\":\"") +
               EscapeJson(WideToUtf8String(m_model).c_str()) +
               "\",\"messages\":[{\"role\":\"system\",\"content\":\"" +
               EscapeJson(system.c_str()) +
               "\"},{\"role\":\"user\",\"content\":\"" +
               EscapeJson(prompt.c_str()) +
               "\"}],\"temperature\":0.2,\"response_format\":{\"type\":\"json_object\"}}";
    }
    else
    {
        body = std::string("{\"model\":\"") +
               EscapeJson(WideToUtf8String(m_model).c_str()) +
               "\",\"system\":\"" + EscapeJson(system.c_str()) +
               "\",\"prompt\":\"" + EscapeJson(prompt.c_str()) +
               "\",\"stream\":false,\"format\":\"json\"}";
    }

    std::string transportResponse;
    if (!HttpGenerateRequest(m_url, m_model, body, chatCompletions,
                             request->TimeoutMs,
                             transportResponse))
    {
        AssistantFailure(response, Salamatrix::AI::AssistantStatusFailed,
                         HRESULT_FROM_WIN32(ERROR_BAD_NET_RESP),
                         L"The local model endpoint did not return a response.");
        return FALSE;
    }

    std::string generated;
    if (chatCompletions)
    {
        if (!ExtractChatCompletionContent(transportResponse, generated))
        {
            AssistantFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse,
                             HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                             L"The local chat-completions endpoint returned no message content.");
            return FALSE;
        }
    }
    else if (!Salamatrix::Runtime::Protocol::Json::FindStringMember(
                 transportResponse.c_str(), "response", &generated))
        generated = transportResponse;
    size_t first = generated.find_first_not_of(" \t\r\n");
    size_t last = generated.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last < first ||
        last - first + 1 >= sizeof(response->ResponseJson) ||
        !StructuredJson(generated))
    {
        AssistantFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse,
                         HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                         L"The local model returned invalid structured JSON.");
        return FALSE;
    }
    size_t length = last - first + 1;
    memcpy(response->ResponseJson, generated.data() + first, length);
    response->ResponseJson[length] = '\0';
    response->OutputLength = static_cast<DWORD>(length);
    response->Status = Salamatrix::AI::AssistantStatusSucceeded;
    response->ErrorCode = S_OK;
    return TRUE;
}

DWORD WINAPI CAIPluginMenuExt::GetMenuItemState(int id, DWORD eventMask)
{
    UNREFERENCED_PARAMETER(eventMask);
    return id == CmdOpenAssistant ? MENU_ITEM_STATE_ENABLED : 0;
}

BOOL WINAPI CAIPluginMenuExt::ExecuteMenuItem(CSalamanderForOperationsAbstract* salamander, HWND parent, int id, DWORD eventMask)
{
    UNREFERENCED_PARAMETER(eventMask);
    if (id == CmdOpenAssistant) { ShowChat(parent, salamander); return TRUE; }
    return FALSE;
}

BOOL WINAPI CAIPluginMenuExt::HelpForMenuItem(HWND parent, int id)
{ UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(id); return FALSE; }

void WINAPI CAIPluginMenuExt::BuildMenu(HWND parent, CSalamanderBuildMenuAbstract* salamander)
{
    UNREFERENCED_PARAMETER(parent);
    if (!g_released && salamander != NULL)
        salamander->AddMenuItem(-1, GetAssistantMenuCaption(), 0, CmdOpenAssistant, TRUE, 0, 0, MENU_SKILLLEVEL_ALL);
}

void WINAPI CPluginInterface::About(HWND parent)
{ SalamanderGeneral->SalMessageBox(parent, "Standalone local assistant and Salamatrix chat window.", "Salamatrix AI", MB_OK | MB_ICONINFORMATION); }
BOOL WINAPI CPluginInterface::Release(HWND parent, BOOL force)
{
    UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(force);
    if (!g_released && IsCurrentService(SALAMATRIX_SERVICE_AI, SALAMATRIX_AI_VERSION_1_0, g_ai))
    {
        g_ai->UnregisterProvider(&g_httpProvider);
        g_ai->UnregisterProvider(&g_provider);
    }
    g_released = true;
    g_ai = NULL;
    g_ui = NULL;
    g_runtime = NULL;
    g_runner = NULL;
    return TRUE;
}
void WINAPI CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{ UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(regKey); UNREFERENCED_PARAMETER(registry); }
void WINAPI CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{ UNREFERENCED_PARAMETER(parent); UNREFERENCED_PARAMETER(regKey); UNREFERENCED_PARAMETER(registry); }
void WINAPI CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    UNREFERENCED_PARAMETER(parent);

    if (SalamanderGUI != NULL)
    {
        CGUIIconListAbstract* iconList = SalamanderGUI->CreateIconList();
        if (iconList != NULL)
        {
            if (iconList->Create(16, 16, 1))
            {
                UINT loadFlags = SalamanderGeneral != NULL ? SalamanderGeneral->GetIconLRFlags() : LR_DEFAULTCOLOR;
                HICON hIcon = (HICON)LoadImage(DLLInstance, MAKEINTRESOURCE(IDI_PLUGINICON), IMAGE_ICON, 16, 16, loadFlags);
                if (hIcon != NULL)
                {
                    iconList->ReplaceIcon(0, hIcon);
                    DestroyIcon(hIcon);
                    salamander->SetIconListForGUI(iconList);
                    salamander->SetPluginIcon(0);
                    salamander->SetPluginMenuAndToolbarIcon(0);
                    iconList = NULL;
                }
            }

            if (iconList != NULL)
                SalamanderGUI->DestroyIconList(iconList);
        }
    }
}
void WINAPI CPluginInterface::Event(int event, DWORD param)
{
    UNREFERENCED_PARAMETER(event); UNREFERENCED_PARAMETER(param);
    EnsureServices();
}
CPluginInterfaceForMenuExtAbstract* WINAPI CPluginInterface::GetInterfaceForMenuExt() { return &g_menu; }

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(reserved);
    if (reason == DLL_PROCESS_ATTACH) { DLLInstance = instance; DisableThreadLibraryCalls(instance); }
    return TRUE;
}

int WINAPI SalamanderPluginGetReqVer() { return LAST_VERSION_OF_SALAMANDER; }

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    SalamanderDebug = salamander->GetSalamanderDebug();
    SalamanderVersion = salamander->GetVersion();
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    SalamanderGUI = salamander->GetSalamanderGUI();
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER) return NULL;
    salamander->SetBasicPluginData("Salamatrix AI",
        FUNCTION_AUTOMATIONFRAMEWORK | FUNCTION_DYNAMICMENUEXT,
        VERSINFO_VERSION_NO_PLATFORM,
        VERSINFO_COPYRIGHT,
        VERSINFO_DESCRIPTION,
        VERSINFO_INTERNAL,
        NULL,
        NULL);
    salamander->SetPluginHomePageURL("https://samandarin.krtkovo.eu/");
    PluginInterface.Event(0, 0);
    return &PluginInterface;
}
