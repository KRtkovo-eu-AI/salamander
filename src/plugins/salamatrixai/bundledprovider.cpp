// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrixai.h"
#include <strsafe.h>

namespace
{
static void BundledFailure(Salamatrix::AI::AssistantResponse* response,
                           Salamatrix::AI::AssistantStatus status,
                           HRESULT error, const wchar_t* message)
{
    response->Status = status;
    response->ErrorCode = error;
    response->OutputLength = 0;
    response->ResponseJson[0] = '\0';
    StringCchCopyW(response->Message, _countof(response->Message), message);
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
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == L'\"')
            result += L"\\\"";
        else
            result += value[i];
    }
    result += L"\"";
    return result;
}

static bool IsJsonObject(const std::string& value)
{
    const size_t first = value.find_first_not_of(" \t\r\n");
    const size_t last = value.find_last_not_of(" \t\r\n");
    return first != std::string::npos && last > first && value[first] == '{' && value[last] == '}';
}

static bool IsRegularFile(const std::wstring& path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
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
        m_command = RuntimeAsset(L"llama-cli.exe");
    m_model = EnvironmentPath(L"SALAMATRIX_AI_BUNDLED_MODEL");
    if (m_model.empty())
        m_model = RuntimeAsset(L"salamatrix.gguf");
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

    std::string prompt =
        "Return only one JSON object with title, description, capabilities, "
        "estimatedEffects, and script. Include runtime when known. You may "
        "set canImplement to false and list missingCapabilities when the "
        "installed API cannot perform the task. Generate a Salamander script "
        "using only the installed Salamander API described here: " +
        InstalledApiDescription(request->Prompt) + "\nTask: ";
    prompt += (request->Prompt != NULL ? request->Prompt : "");
    if (request->RuntimeId != NULL && request->RuntimeId[0] != '\0')
        prompt += "\nTarget runtime: " + std::string(request->RuntimeId);
    if (request->ContextJson != NULL && request->ContextJson[0] != '\0')
        prompt += "\nCurrent Salamander context (JSON):\n" + std::string(request->ContextJson);
    if (request->ExistingScript != NULL && request->ExistingScript[0] != '\0')
        prompt += "\nExisting script to repair:\n" + std::string(request->ExistingScript);
    if (request->Feedback != NULL && request->Feedback[0] != '\0')
        prompt += "\nRepair feedback:\n" + std::string(request->Feedback);

    SECURITY_ATTRIBUTES security = { sizeof(security), NULL, TRUE };
    HANDLE parentIn = NULL, childIn = NULL, parentOut = NULL, childOut = NULL;
    if (!CreatePipe(&parentIn, &childIn, &security, 0) ||
        !SetHandleInformation(parentIn, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&parentOut, &childOut, &security, 0) ||
        !SetHandleInformation(parentOut, HANDLE_FLAG_INHERIT, 0))
    {
        if (parentIn) CloseHandle(parentIn); if (childIn) CloseHandle(childIn);
        if (parentOut) CloseHandle(parentOut); if (childOut) CloseHandle(childOut);
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed,
                       HRESULT_FROM_WIN32(GetLastError()), L"Unable to create bundled model pipes.");
        return FALSE;
    }
    std::wstring command = Quote(m_command) + L" -m " + Quote(m_model) +
                           L" --simple-io --no-display-prompt -n 4096";
    std::vector<wchar_t> commandLine(command.begin(), command.end());
    commandLine.push_back(L'\0');
    STARTUPINFOW startup = {}; PROCESS_INFORMATION process = {};
    startup.cb = sizeof(startup); startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childIn; startup.hStdOutput = childOut;
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    BOOL created = CreateProcessW(NULL, commandLine.data(), NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, NULL, &startup, &process);
    CloseHandle(childIn); CloseHandle(childOut);
    if (!created)
    {
        CloseHandle(parentIn); CloseHandle(parentOut);
        BundledFailure(response, Salamatrix::AI::AssistantStatusFailed,
                       HRESULT_FROM_WIN32(GetLastError()), L"Unable to start bundled llama.cpp.");
        return FALSE;
    }
    DWORD written = 0;
    if (!WriteFile(parentIn, prompt.data(), static_cast<DWORD>(prompt.size()), &written, NULL) ||
        written != prompt.size())
        TerminateProcess(process.hProcess, 1);
    CloseHandle(parentIn);

    std::string output;
    const DWORD timeout = request->TimeoutMs == 0 ? 120000 :
        (request->TimeoutMs > 120000 ? 120000 : request->TimeoutMs);
    const ULONGLONG start = GetTickCount64();
    bool timedOut = false;
    for (;;)
    {
        DWORD available = 0;
        while (PeekNamedPipe(parentOut, NULL, 0, NULL, &available, NULL) && available)
        {
            char buffer[4096]; DWORD count = 0;
            const DWORD take = available < sizeof(buffer) ? available : sizeof(buffer);
            if (!ReadFile(parentOut, buffer, take, &count, NULL) || !count) break;
            if (output.size() < 1024 * 1024)
                output.append(buffer, buffer + (count < 1024 * 1024 - output.size() ? count : 1024 * 1024 - output.size()));
        }
        if (WaitForSingleObject(process.hProcess, 10) == WAIT_OBJECT_0) break;
        if (GetTickCount64() - start >= timeout)
        { timedOut = true; TerminateProcess(process.hProcess, 1); break; }
    }
    WaitForSingleObject(process.hProcess, 1000);
    DWORD available = 0;
    while (PeekNamedPipe(parentOut, NULL, 0, NULL, &available, NULL) && available)
    {
        char buffer[4096]; DWORD count = 0;
        const DWORD take = available < sizeof(buffer) ? available : sizeof(buffer);
        if (!ReadFile(parentOut, buffer, take, &count, NULL) || !count) break;
        if (output.size() < 1024 * 1024)
            output.append(buffer, buffer + (count < 1024 * 1024 - output.size() ? count : 1024 * 1024 - output.size()));
    }
    DWORD exitCode = 1; GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread); CloseHandle(process.hProcess); CloseHandle(parentOut);
    const size_t first = output.find_first_not_of(" \t\r\n");
    const size_t last = output.find_last_not_of(" \t\r\n");
    if (timedOut)
    { BundledFailure(response, Salamatrix::AI::AssistantStatusCancelled, HRESULT_FROM_WIN32(ERROR_TIMEOUT), L"The bundled model timed out."); return FALSE; }
    if (exitCode != 0 || first == std::string::npos || last <= first ||
        last - first + 1 >= sizeof(response->ResponseJson) || !IsJsonObject(output))
    { BundledFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse, HRESULT_FROM_WIN32(ERROR_INVALID_DATA), L"The bundled model returned invalid structured JSON."); return FALSE; }
    const size_t length = last - first + 1;
    memcpy(response->ResponseJson, output.data() + first, length);
    response->ResponseJson[length] = '\0'; response->OutputLength = static_cast<DWORD>(length);
    response->Status = Salamatrix::AI::AssistantStatusSucceeded;
    response->ErrorCode = S_OK;
    return TRUE;
}
