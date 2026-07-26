// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SALAMATRIXAI_BUNDLED_PROVIDER_HAS_PRECOMP
#include "precomp.h"
#endif
#include "salamatrixai.h"
#include <strsafe.h>

namespace
{
static void AppendBundledOutput(
    Salamatrix::AI::AssistantResponse* response,
    const std::string& output)
{
    if (response == NULL || output.empty())
        return;

    const int byteCount = static_cast<int>(output.size() > 4096 ? 4096 : output.size());
    int wideCount = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        output.data(), byteCount, NULL, 0);
    UINT codePage = CP_UTF8;
    if (wideCount <= 0)
    {
        codePage = CP_ACP;
        wideCount = MultiByteToWideChar(codePage, 0, output.data(), byteCount, NULL, 0);
    }
    if (wideCount <= 0)
        return;

    std::vector<wchar_t> wide(static_cast<size_t>(wideCount) + 1, L'\0');
    if (MultiByteToWideChar(codePage, 0, output.data(), byteCount,
                            wide.data(), wideCount) <= 0)
        return;

    StringCchCatW(response->Message, _countof(response->Message),
                  L"\n\nllama output:\n");
    StringCchCatW(response->Message, _countof(response->Message), wide.data());
}

static void BundledFailure(Salamatrix::AI::AssistantResponse* response,
                           Salamatrix::AI::AssistantStatus status,
                           HRESULT error, const wchar_t* message,
                           const std::string* output = NULL)
{
    response->Status = status;
    response->ErrorCode = error;
    response->OutputLength = 0;
    response->ResponseJson[0] = '\0';
    StringCchCopyW(response->Message, _countof(response->Message), message);
    if (output != NULL)
        AppendBundledOutput(response, *output);
}

static std::wstring EnvironmentPath(const wchar_t* name)
{
    std::vector<wchar_t> value(4096, L'\0');
    DWORD length = GetEnvironmentVariableW(name, value.data(),
                                           static_cast<DWORD>(value.size()));
    if (length == 0)
        return std::wstring();
    if (length >= value.size())
    {
        value.resize(static_cast<size_t>(length) + 1, L'\0');
        length = GetEnvironmentVariableW(name, value.data(),
                                          static_cast<DWORD>(value.size()));
        if (length == 0 || length >= value.size())
            return std::wstring();
    }
    return std::wstring(value.data(), length);
}

static std::wstring RuntimeAsset(const wchar_t* name)
{
    std::vector<wchar_t> module(4096, L'\0');
    DWORD length = GetModuleFileNameW(DLLInstance, module.data(),
                                      static_cast<DWORD>(module.size()));
    if (length == 0 || length >= module.size())
        return std::wstring();
    std::wstring path(module.data(), length);
    const size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos)
        return std::wstring();
    path.resize(slash + 1);
    path += L"runtime\\";
    path += name;
    return path;
}

static std::wstring Quote(const std::wstring& value)
{
    std::wstring result = L"\"";
    size_t backslashes = 0;
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == L'\"')
        {
            result.append(backslashes * 2 + 1, L'\\');
            result += L'\"';
            backslashes = 0;
        }
        else if (value[i] == L'\\')
            ++backslashes;
        else
        {
            result.append(backslashes, L'\\');
            result += value[i];
            backslashes = 0;
        }
    }
    result.append(backslashes * 2, L'\\');
    result += L"\"";
    return result;
}

static std::string JsonString(const char* value)
{
    std::string result = "\"";
    const unsigned char* cursor = reinterpret_cast<const unsigned char*>(
        value != NULL ? value : "");
    for (; *cursor != '\0'; ++cursor)
    {
        switch (*cursor)
        {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\b': result += "\\b"; break;
        case '\f': result += "\\f"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default: result.push_back(static_cast<char>(*cursor)); break;
        }
    }
    result += "\"";
    return result;
}

static bool CreateUtf8PromptFile(const std::string& prompt, std::wstring* path)
{
    if (path == NULL)
        return false;
    path->clear();
    std::vector<wchar_t> tempRoot(SAL_MAX_PATH, L'\0');
    DWORD rootLength = GetTempPathW(static_cast<DWORD>(tempRoot.size()), tempRoot.data());
    if (rootLength == 0 || rootLength >= tempRoot.size())
        return false;
    std::vector<wchar_t> tempName(SAL_MAX_PATH, L'\0');
    if (GetTempFileNameW(tempRoot.data(), L"smx", 0, tempName.data()) == 0)
        return false;
    HANDLE file = CreateFileW(tempName.data(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        DeleteFileW(tempName.data());
        return false;
    }
    bool written = prompt.size() <= MAXDWORD;
    DWORD count = 0;
    if (written && !prompt.empty())
        written = WriteFile(file, prompt.data(), static_cast<DWORD>(prompt.size()), &count, NULL) &&
                  count == prompt.size();
    CloseHandle(file);
    if (!written)
    {
        DeleteFileW(tempName.data());
        return false;
    }
    *path = tempName.data();
    return true;
}

static bool ExtractJsonObject(const std::string& value, size_t* first, size_t* last)
{
    if (first == NULL || last == NULL)
        return false;

    for (size_t start = 0; start < value.size(); ++start)
    {
        if (value[start] != '{')
            continue;
        int depth = 0;
        bool inString = false;
        bool escaped = false;
        for (size_t end = start; end < value.size(); ++end)
        {
            const char character = value[end];
            if (inString)
            {
                if (escaped)
                    escaped = false;
                else if (character == '\\')
                    escaped = true;
                else if (character == '"')
                    inString = false;
                continue;
            }
            if (character == '"')
                inString = true;
            else if (character == '{')
                ++depth;
            else if (character == '}' && --depth == 0)
            {
                const std::string candidate = value.substr(start, end - start + 1);
                std::string title;
                std::string script;
                if (Salamatrix::Runtime::Protocol::Json::FindStringMember(
                        candidate.c_str(), "title", &title) &&
                    Salamatrix::Runtime::Protocol::Json::FindStringMember(
                        candidate.c_str(), "script", &script))
                {
                    *first = start;
                    *last = end;
                    return true;
                }
                break;
            }
        }
    }

    *first = std::string::npos;
    *last = std::string::npos;
    return false;
}

static bool IsJsonObject(const std::string& value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    const size_t last = value.find_last_not_of(" \t\r\n");
    return first != std::string::npos && last > first && value[first] == '{' && value[last] == '}';
}

static void ReadAvailablePipe(
    HANDLE parentOut,
    std::string& output,
    Salamatrix::AI::AssistantOutputCallback callback,
    void* callbackContext)
{
    DWORD available = 0;
    while (PeekNamedPipe(parentOut, NULL, 0, NULL, &available, NULL) && available)
    {
        char buffer[4096];
        DWORD count = 0;
        const DWORD take = available < sizeof(buffer) ? available : sizeof(buffer);
        if (!ReadFile(parentOut, buffer, take, &count, NULL) || !count)
            break;
        if (callback != NULL)
            callback(callbackContext, buffer, count);
        if (output.size() < 1024 * 1024)
            output.append(buffer, buffer +
                (count < 1024 * 1024 - output.size() ? count : 1024 * 1024 - output.size()));
    }
}

static Salamatrix::AI::AssistantOutputCallback GetOutputCallback(
    const Salamatrix::AI::AssistantRequest* request,
    void** context)
{
    if (context != NULL)
        *context = NULL;
    if (request == NULL || request->StructSize <
        offsetof(Salamatrix::AI::AssistantRequest, OutputContext) + sizeof(void*))
        return NULL;
    if (context != NULL)
        *context = request->OutputContext;
    return request->OutputCallback;
}

static bool IsRegularFile(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static std::wstring ResolveBundledAsset(const wchar_t* name)
{
    std::wstring primary = RuntimeAsset(name);
    if (IsRegularFile(primary))
        return primary;

    // Keep installations produced by the first companion-plugin packaging
    // layout working while the next build moves assets below its own runtime
    // directory. Environment overrides still take precedence in the caller.
    const wchar_t* legacyRoots[] = {
        L"..\\salamatrixai\\runtime\\",
        L"..\\salamatrixai\\"
    };
    for (size_t index = 0; index < _countof(legacyRoots); ++index)
    {
        std::wstring relative = legacyRoots[index];
        relative += name;
        std::wstring legacy = RuntimeAsset(relative.c_str());
        if (IsRegularFile(legacy))
            return legacy;
    }
    return primary;
}

static std::string InstalledApiDescription(const char* prompt)
{
    if (SalamanderGeneral == NULL)
        return "{}";
    CSalamanderServiceQuery query = {};
    query.ServiceId = SALAMATRIX_SERVICE_AI;
    query.MinimumVersion = SALAMATRIX_AI_VERSION_1_0;
    CSalamanderServiceResult result = {};
    if (!SalamanderGeneral->QueryService(&query, &result) || result.Interface == NULL)
        return "{}";
    Salamatrix::AI::IAssistantService* service =
        static_cast<Salamatrix::AI::IAssistantService*>(result.Interface);
    return Salamatrix::AI::BuildRelevantApiDescription(service, prompt);
}
}

CLocalBundledAssistantProvider::CLocalBundledAssistantProvider()
{
    m_descriptor.ProviderId = "local.bundled";
    m_descriptor.DisplayName = "Bundled local model";
    m_descriptor.ProviderVersion = 0x00010000;
    m_descriptor.Flags = 0;
}

void CLocalBundledAssistantProvider::ResolveConfiguration() const
{
    m_command = EnvironmentPath(L"SALAMATRIX_AI_BUNDLED_COMMAND");
    if (m_command.empty())
        m_command = ResolveBundledAsset(L"llama-cli.exe");
    m_model = EnvironmentPath(L"SALAMATRIX_AI_BUNDLED_MODEL");
    if (m_model.empty())
        m_model = ResolveBundledAsset(L"salamatrix.gguf");
}

const Salamatrix::AI::AssistantProviderDescriptor* WINAPI
CLocalBundledAssistantProvider::GetDescriptor() const
{
    return &m_descriptor;
}

BOOL WINAPI CLocalBundledAssistantProvider::IsAvailable() const
{
    ResolveConfiguration();
    return !m_command.empty() && !m_model.empty() &&
           IsRegularFile(m_command) && IsRegularFile(m_model);
}

BOOL WINAPI CLocalBundledAssistantProvider::Generate(
    const Salamatrix::AI::AssistantRequest* request,
    Salamatrix::AI::AssistantResponse* response)
{
    if (response == NULL || response->StructSize < sizeof(*response))
        return FALSE;
    *response = Salamatrix::AI::AssistantResponse();
    if (request == NULL || request->StructSize < sizeof(*request))
    {
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed, E_INVALIDARG,
                       L"The assistant request is invalid.");
        return FALSE;
    }
    if (!IsAvailable())
    {
        BundledFailure(response, Salamatrix::AI::AssistantStatusUnavailable,
                       HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
                       L"The bundled llama.cpp executable or GGUF model is missing.");
        return FALSE;
    }

    const std::string apiDescription = InstalledApiDescription(request->Prompt);
    std::string prompt =
        "You generate one Salamander automation script. The installed API "
        "reference is included below for context only. Do not copy or repeat "
        "the reference objects in your answer.\n\n"
        "BEGIN INSTALLED SALAMANDER API REFERENCE\n" +
        apiDescription +
        "\nEND INSTALLED SALAMANDER API REFERENCE\n\n"
        "USER TASK:\n";
    prompt += (request->Prompt != NULL ? request->Prompt : "");
    if (request->RuntimeId != NULL && request->RuntimeId[0] != '\0')
        prompt += "\nTarget runtime: " + std::string(request->RuntimeId);
    if (request->ContextJson != NULL && request->ContextJson[0] != '\0')
        prompt += "\nCurrent Salamander context (JSON):\n" + std::string(request->ContextJson);
    if (request->ExistingScript != NULL && request->ExistingScript[0] != '\0')
        prompt += "\nExisting script to repair:\n" + std::string(request->ExistingScript);
    if (request->Feedback != NULL && request->Feedback[0] != '\0')
        prompt += "\nRepair feedback:\n" + std::string(request->Feedback);
    prompt +=
        "\n\nFINAL OUTPUT RULE: Return exactly one JSON object and no markdown, "
        "explanation, API reference, or schema. The object must contain the "
        "string fields title, description, and script. capabilities MUST be "
        "a JSON array containing ONLY these installed capability names: "
        "panels.read, panels.write, ui.dialogs, commands, file-operations, "
        "storage, events, ai, clipboard, runtimes. estimatedEffects MUST be "
        "a JSON object whose values are booleans, never an array or a string. "
        "canImplement and missingCapabilities are top-level properties, never "
        "members of estimatedEffects. Include runtime when known and use exactly the requested runtime. "
        "The script field must contain source code for that runtime, not an "
        "error message or a command description. If the task cannot be "
        "implemented, set canImplement to false and list missingCapabilities at "
        "the top level. The script field must still be present. Always include "
        "canImplement and all eight estimatedEffects keys: readSelection, "
        "readMetadata, renameFiles, moveFiles, deleteFiles, modifyContents, "
        "executeExternal, network. The runtime field must exactly equal the "
        "Target runtime value above. When canImplement is true, script must be "
        "complete executable source code; it must never be empty, an ellipsis, "
        "a placeholder, or a prose description.\n"
        "Do not invent a Salamander API object, method, property, event, or "
        "option: every Salamander identifier must exist in the installed "
        "reference. You may and should use normal syntax, built-in features, "
        "standard-library functions, and third-party libraries in the selected "
        "runtime when they are appropriate for the task. The user is responsible "
        "for installing or configuring such language libraries; their possible "
        "absence is not a missing Salamander capability and must not by itself "
        "cause canImplement=false. If the Salamander reference does not provide "
        "a required framework operation, return canImplement=false, describe the "
        "missing Salamander capability in missingCapabilities, and do not pretend "
        "that an invented Salamander API implements it. Any filesystem, "
        "external-process, or network operation performed by the generated "
        "language code must be declared by the corresponding estimatedEffects "
        "boolean.";

    std::wstring promptFile;
    if (!CreateUtf8PromptFile(prompt, &promptFile))
    {
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed,
                       HRESULT_FROM_WIN32(ERROR_WRITE_FAULT),
                       L"Unable to create the bundled model prompt file.");
        return FALSE;
    }
    std::string outputSchema =
        "{"
        "\"type\":\"object\","
        "\"additionalProperties\":false,"
        "\"required\":[\"title\",\"description\",\"capabilities\","
        "\"estimatedEffects\",\"runtime\",\"canImplement\","
        "\"missingCapabilities\",\"script\"],"
        "\"properties\":{"
        "\"title\":{\"type\":\"string\",\"minLength\":1},"
        "\"description\":{\"type\":\"string\",\"minLength\":1},"
        "\"capabilities\":{\"type\":\"array\",\"maxItems\":10,\"items\":{"
        "\"type\":\"string\",\"enum\":[\"panels.read\",\"panels.write\","
        "\"ui.dialogs\",\"commands\",\"file-operations\",\"storage\","
        "\"events\",\"ai\",\"clipboard\",\"runtimes\"]}},"
        "\"estimatedEffects\":{\"type\":\"object\",\"additionalProperties\":false,"
        "\"required\":[\"readSelection\",\"readMetadata\",\"renameFiles\","
        "\"moveFiles\",\"deleteFiles\",\"modifyContents\",\"executeExternal\","
        "\"network\"],\"properties\":{"
        "\"readSelection\":{\"type\":\"boolean\"},"
        "\"readMetadata\":{\"type\":\"boolean\"},"
        "\"renameFiles\":{\"type\":\"boolean\"},"
        "\"moveFiles\":{\"type\":\"boolean\"},"
        "\"deleteFiles\":{\"type\":\"boolean\"},"
        "\"modifyContents\":{\"type\":\"boolean\"},"
        "\"executeExternal\":{\"type\":\"boolean\"},"
        "\"network\":{\"type\":\"boolean\"}}},"
        "\"runtime\":{\"type\":\"string\",\"minLength\":1";
    if (request->RuntimeId != NULL && request->RuntimeId[0] != '\0')
        outputSchema += ",\"const\":" + JsonString(request->RuntimeId);
    outputSchema += "},"
        "\"canImplement\":{\"type\":\"boolean\"},"
        "\"missingCapabilities\":{\"type\":\"array\",\"maxItems\":16,"
        "\"items\":{\"type\":\"string\"}},"
        "\"script\":{\"type\":\"string\"}"
        "}}";
    std::wstring schemaFile;
    if (!CreateUtf8PromptFile(outputSchema, &schemaFile))
    {
        DeleteFileW(promptFile.c_str());
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed,
                       HRESULT_FROM_WIN32(ERROR_WRITE_FAULT),
                       L"Unable to create the bundled model output schema.");
        return FALSE;
    }

    SECURITY_ATTRIBUTES security = { sizeof(security), NULL, TRUE };
    HANDLE parentIn = NULL, childIn = NULL, parentOut = NULL, childOut = NULL;
    HANDLE parentErr = NULL, childErr = NULL;
    if (!CreatePipe(&parentIn, &childIn, &security, 0) ||
        !SetHandleInformation(parentIn, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&parentOut, &childOut, &security, 0) ||
        !SetHandleInformation(parentOut, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&parentErr, &childErr, &security, 0) ||
        !SetHandleInformation(parentErr, HANDLE_FLAG_INHERIT, 0))
    {
        if (parentIn) CloseHandle(parentIn); if (childIn) CloseHandle(childIn);
        if (parentOut) CloseHandle(parentOut); if (childOut) CloseHandle(childOut);
        if (parentErr) CloseHandle(parentErr); if (childErr) CloseHandle(childErr);
        DeleteFileW(promptFile.c_str());
        DeleteFileW(schemaFile.c_str());
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed,
                       HRESULT_FROM_WIN32(GetLastError()), L"Unable to create bundled model pipes.");
        return FALSE;
    }
    std::wstring command = Quote(m_command) + L" -m " + Quote(m_model) +
                           L" -f " + Quote(promptFile) +
                           L" --json-schema-file " + Quote(schemaFile) +
                           L" --single-turn --no-conversation --no-jinja --simple-io"
                           L" --no-display-prompt --no-perf"
                           L" --temp 0 --top-k 1 --seed 0 -n 4096";
    std::vector<wchar_t> commandLine(command.begin(), command.end());
    commandLine.push_back(L'\0');
    STARTUPINFOW startup = {}; PROCESS_INFORMATION process = {};
    startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childIn; startup.hStdOutput = childOut;
    // llama.cpp reports model/loading diagnostics on stderr. Capture it
    // separately so diagnostics cannot interfere with the JSON stdout stream.
    startup.hStdError = childErr;
    BOOL created = CreateProcessW(NULL, commandLine.data(), NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, NULL, &startup, &process);
    CloseHandle(childIn); CloseHandle(childOut);
    CloseHandle(childErr);
    if (!created)
    {
        CloseHandle(parentIn); CloseHandle(parentOut); CloseHandle(parentErr);
        DeleteFileW(promptFile.c_str());
        DeleteFileW(schemaFile.c_str());
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed,
                       HRESULT_FROM_WIN32(GetLastError()), L"Unable to start bundled llama.cpp.");
        return FALSE;
    }
    CloseHandle(parentIn);

    std::string output;
    std::string diagnostics;
    void* outputContext = NULL;
    const Salamatrix::AI::AssistantOutputCallback outputCallback =
        GetOutputCallback(request, &outputContext);
    const DWORD timeout = request->TimeoutMs == 0 ? 120000 :
        (request->TimeoutMs > 120000 ? 120000 : request->TimeoutMs);
    const ULONGLONG start = GetTickCount64();
    bool timedOut = false;
    for (;;)
    {
        ReadAvailablePipe(parentOut, output, outputCallback, outputContext);
        ReadAvailablePipe(parentErr, diagnostics, outputCallback, outputContext);
        if (WaitForSingleObject(process.hProcess, 10) == WAIT_OBJECT_0) break;
        if (GetTickCount64() - start >= timeout)
        { timedOut = true; TerminateProcess(process.hProcess, 1); break; }
    }
    WaitForSingleObject(process.hProcess, 1000);
    ReadAvailablePipe(parentOut, output, outputCallback, outputContext);
    ReadAvailablePipe(parentErr, diagnostics, outputCallback, outputContext);
    DWORD exitCode = 1; GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    CloseHandle(parentOut); CloseHandle(parentErr);
    DeleteFileW(promptFile.c_str());
    DeleteFileW(schemaFile.c_str());
    std::string failureOutput;
    size_t first = std::string::npos, last = std::string::npos;
    const bool hasJsonObject = ExtractJsonObject(output, &first, &last);
    if (hasJsonObject && last >= first)
        failureOutput = output.substr(first, last - first + 1);
    if (timedOut)
    { BundledFailure(response, Salamatrix::AI::AssistantStatusCancelled, HRESULT_FROM_WIN32(ERROR_TIMEOUT), L"The bundled model timed out.", &failureOutput); return FALSE; }
    if (exitCode != 0 || !hasJsonObject || last - first + 1 >= sizeof(response->ResponseJson))
    { BundledFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse, HRESULT_FROM_WIN32(ERROR_INVALID_DATA), L"The bundled model returned invalid structured JSON.", &failureOutput); return FALSE; }
    const std::string json = output.substr(first, last - first + 1);
    if (!IsJsonObject(json))
    { BundledFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse, HRESULT_FROM_WIN32(ERROR_INVALID_DATA), L"The bundled model returned invalid structured JSON.", &failureOutput); return FALSE; }
    const size_t length = json.size();
    memcpy(response->ResponseJson, json.data(), length);
    response->ResponseJson[length] = '\0'; response->OutputLength = static_cast<DWORD>(length);
    response->Status = Salamatrix::AI::AssistantStatusSucceeded;
    response->ErrorCode = S_OK;
    return TRUE;
}
