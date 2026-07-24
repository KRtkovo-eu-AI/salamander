// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    salamatrixbridge.cpp
    Thin Automation-side consumer bridge for the Salamatrix runtime plugin.
*/

#include "precomp.h"
#include "salamatrixbridge.h"
#include "engassoc.h"
#include "knownengines.h"

#include <vector>

namespace
{
static bool AppendQuotedArgument(std::wstring& command, const wchar_t* value)
{
    if (value == NULL || wcschr(value, L'"') != NULL)
        return false;
    command.push_back(L'"');
    command.append(value);
    command.push_back(L'"');
    return true;
}

static void SetRuntimeText(
    const std::string& value,
    wchar_t* output,
    int outputCount,
    DWORD* outputLength)
{
    if (output == NULL || outputCount <= 0)
        return;

    int converted = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.c_str(),
        static_cast<int>(value.size()), output, outputCount - 1);
    if (converted == 0)
    {
        converted = MultiByteToWideChar(
            CP_ACP, 0, value.c_str(), static_cast<int>(value.size()),
            output, outputCount - 1);
    }
    if (converted < 0)
        converted = 0;
    output[converted] = L'\0';
    if (outputLength != NULL)
        *outputLength = static_cast<DWORD>(converted);
}

static void SetRuntimeFailure(
    Salamatrix::Runtime::RuntimeExecutionResult* result,
    HRESULT errorCode,
    const wchar_t* message)
{
    result->Status = Salamatrix::Runtime::RuntimeExecutionStatusFailed;
    result->ErrorCode = errorCode;
    result->ExitCode = 0;
    result->ProcessId = 0;
    result->OutputLength = 0;
    result->Output[0] = L'\0';
    StringCchCopyW(result->Message, _countof(result->Message), message);
}
} // namespace

CAutomationSalamatrixBridge::CAutomationSalamatrixBridge()
    : m_bQueried(false),
      m_pGeneral(NULL),
      m_pScriptRoot(NULL),
      m_pUIService(NULL),
      m_pCommandService(NULL),
      m_pFileOperationsService(NULL),
      m_pRuntimeService(NULL),
      m_pSidesService(NULL),
      m_pEventsService(NULL),
      m_pExtensionsService(NULL),
      m_pStorageService(NULL),
      m_dwAutomationVersion(0),
      m_dwUIVersion(0),
      m_dwCommandsVersion(0),
      m_dwFileOperationsVersion(0),
      m_dwRuntimeVersion(0),
      m_dwSidesVersion(0),
      m_dwEventsVersion(0),
      m_dwExtensionsVersion(0),
      m_dwStorageVersion(0),
      m_oJScriptRuntime("Automation.JScript", "Legacy Windows JScript", "javascript", ".js", _T(".js"), CLSID_JScript),
      m_oVBScriptRuntime("Automation.VBScript", "Legacy Windows VBScript", "vbscript", ".vbs", _T(".vbs"), CLSID_VBScript),
      m_oPythonRuntime("Automation.ActivePython", "Legacy ActivePython", "python", ".pys", _T(".pys"), CLSID_Python),
      m_oPHPRuntime("Automation.PHPScript", "Legacy PHPScript", "php", ".phps", _T(".phps"), CLSID_PHPScript),
      m_oCPythonRuntime(
          "Python.CPython",
          "CPython process runtime",
          "python",
          ".py",
          L"SALAMATRIX_PYTHON",
          L"python.exe",
          L"python3.exe",
          CAutomationProcessRuntimeAdapter::ProcessKindPython),
      m_oPowerShellRuntime(
          "PowerShell",
          "PowerShell process runtime",
          "powershell",
          ".ps1",
          L"SALAMATRIX_POWERSHELL",
          L"pwsh.exe",
          L"powershell.exe",
          CAutomationProcessRuntimeAdapter::ProcessKindPowerShell),
      m_oPHPCliRuntime(
          "PHP.CLI",
          "PHP CLI process runtime",
          "php",
          ".php",
          L"SALAMATRIX_PHP",
          L"php.exe",
          NULL,
          CAutomationProcessRuntimeAdapter::ProcessKindPhp),
      m_bRuntimeAdaptersRegistered(false)
{
    Reset();
}

CAutomationActiveScriptRuntimeAdapter::CAutomationActiveScriptRuntimeAdapter(
    const char* runtimeId,
    const char* displayName,
    const char* languageId,
    const char* fileExtension,
    PCTSTR nativeFileExtension,
    const CLSID& engineClsid)
    : m_pszFileExtension(nativeFileExtension),
      m_clsidEngine(engineClsid)
{
    m_oDescriptor.RuntimeId = runtimeId;
    m_oDescriptor.DisplayName = displayName;
    m_oDescriptor.LanguageId = languageId;
    m_oDescriptor.FileExtensions = fileExtension;
    m_oDescriptor.RuntimeVersion = 0x00010000;
    m_oDescriptor.Flags = Salamatrix::Runtime::RuntimeAdapterFlagInProcess |
                          Salamatrix::Runtime::RuntimeAdapterFlagCompatibility;
}

const Salamatrix::Runtime::RuntimeAdapterDescriptor* WINAPI
CAutomationActiveScriptRuntimeAdapter::GetDescriptor() const
{
    return &m_oDescriptor;
}

BOOL WINAPI CAutomationActiveScriptRuntimeAdapter::IsAvailable() const
{
    CLSID associatedEngine = CLSID_NULL;
    return g_oScriptAssociations.FindEngineByExt(m_pszFileExtension, &associatedEngine) &&
           IsEqualCLSID(associatedEngine, m_clsidEngine);
}

BOOL WINAPI CAutomationActiveScriptRuntimeAdapter::SupportsEntryPoint(const char* entryPoint) const
{
    if (entryPoint == NULL)
        return FALSE;

    const char* extension = strrchr(entryPoint, '.');
    if (extension == NULL)
        return FALSE;

#ifdef UNICODE
    wchar_t nativeExtension[16];
    if (MultiByteToWideChar(CP_UTF8, 0, extension, -1, nativeExtension, _countof(nativeExtension)) == 0 &&
        MultiByteToWideChar(CP_ACP, 0, extension, -1, nativeExtension, _countof(nativeExtension)) == 0)
    {
        return FALSE;
    }
#else
    const char* nativeExtension = extension;
#endif

    return _tcsicmp(nativeExtension, m_pszFileExtension) == 0 && IsAvailable();
}

BOOL WINAPI CAutomationActiveScriptRuntimeAdapter::Execute(
    const Salamatrix::Runtime::RuntimeExecutionRequest* request,
    Salamatrix::Runtime::RuntimeExecutionResult* result)
{
    if (result == NULL || result->StructSize < sizeof(*result))
        return FALSE;

    result->Status = Salamatrix::Runtime::RuntimeExecutionStatusFailed;
    result->ErrorCode = E_INVALIDARG;
    result->Message[0] = L'\0';

    if (request == NULL || request->StructSize < sizeof(*request) ||
        request->EntryPoint == NULL || request->CompatibilityExecute == NULL)
    {
        StringCchCopyW(result->Message, _countof(result->Message),
                       L"The ActiveScript compatibility adapter requires a host execution callback.");
        return FALSE;
    }

    BOOL executed = request->CompatibilityExecute(request->CompatibilityContext, result);
    if (executed && result->Status == Salamatrix::Runtime::RuntimeExecutionStatusNotStarted)
        result->Status = Salamatrix::Runtime::RuntimeExecutionStatusSucceeded;
    else if (!executed && result->Status == Salamatrix::Runtime::RuntimeExecutionStatusNotStarted)
        result->Status = Salamatrix::Runtime::RuntimeExecutionStatusFailed;
    return executed;
}

CAutomationProcessRuntimeAdapter::CAutomationProcessRuntimeAdapter(
    const char* runtimeId,
    const char* displayName,
    const char* languageId,
    const char* fileExtension,
    const wchar_t* environmentVariable,
    const wchar_t* candidateOne,
    const wchar_t* candidateTwo,
    ProcessKind kind)
    : m_pszExtension(fileExtension),
      m_pszEnvironmentVariable(environmentVariable),
      m_pszCandidateOne(candidateOne),
      m_pszCandidateTwo(candidateTwo),
      m_kind(kind)
{
    m_oDescriptor.RuntimeId = runtimeId;
    m_oDescriptor.DisplayName = displayName;
    m_oDescriptor.LanguageId = languageId;
    m_oDescriptor.FileExtensions = fileExtension;
    m_oDescriptor.RuntimeVersion = 0x00010000;
    m_oDescriptor.Flags = Salamatrix::Runtime::RuntimeAdapterFlagOutOfProcess;
}

void CAutomationProcessRuntimeAdapter::ResolveInterpreter() const
{
    if (!m_executablePath.empty())
        return;

    wchar_t configured[MAX_PATH * 4];
    DWORD configuredLength = GetEnvironmentVariableW(
        m_pszEnvironmentVariable,
        configured,
        _countof(configured));
    if (configuredLength != 0 && configuredLength < _countof(configured))
    {
        DWORD attributes = GetFileAttributesW(configured);
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            m_executablePath.assign(configured);
            return;
        }

        wchar_t resolved[MAX_PATH * 4];
        DWORD resolvedLength = SearchPathW(
            NULL, configured, NULL, _countof(resolved), resolved, NULL);
        if (resolvedLength != 0 && resolvedLength < _countof(resolved))
        {
            m_executablePath.assign(resolved);
            return;
        }
    }

    const wchar_t* candidates[] = {m_pszCandidateOne, m_pszCandidateTwo};
    for (int index = 0; index < _countof(candidates); ++index)
    {
        if (candidates[index] == NULL)
            continue;
        wchar_t resolved[MAX_PATH * 4];
        DWORD resolvedLength = SearchPathW(
            NULL,
            candidates[index],
            NULL,
            _countof(resolved),
            resolved,
            NULL);
        if (resolvedLength != 0 && resolvedLength < _countof(resolved))
        {
            m_executablePath.assign(resolved);
            return;
        }
    }
}

const Salamatrix::Runtime::RuntimeAdapterDescriptor* WINAPI
CAutomationProcessRuntimeAdapter::GetDescriptor() const
{
    return &m_oDescriptor;
}

BOOL WINAPI CAutomationProcessRuntimeAdapter::IsAvailable() const
{
    ResolveInterpreter();
    return m_executablePath.empty() ? FALSE : TRUE;
}

BOOL WINAPI CAutomationProcessRuntimeAdapter::SupportsEntryPoint(
    const char* entryPoint) const
{
    if (entryPoint == NULL || !IsAvailable())
        return FALSE;
    const char* extension = strrchr(entryPoint, '.');
    return extension != NULL && _stricmp(extension, m_pszExtension) == 0;
}

BOOL WINAPI CAutomationProcessRuntimeAdapter::Execute(
    const Salamatrix::Runtime::RuntimeExecutionRequest* request,
    Salamatrix::Runtime::RuntimeExecutionResult* result)
{
    if (result == NULL || result->StructSize < sizeof(*result))
        return FALSE;
    *result = Salamatrix::Runtime::RuntimeExecutionResult();

    if (request == NULL || request->StructSize < sizeof(*request) ||
        request->EntryPoint == NULL || !IsAvailable())
    {
        SetRuntimeFailure(
            result,
            E_INVALIDARG,
            L"The external runtime adapter received an invalid request or is unavailable.");
        return FALSE;
    }

    std::wstring command;
    if (!AppendQuotedArgument(command, m_executablePath.c_str()))
    {
        SetRuntimeFailure(result, E_INVALIDARG, L"The runtime executable path is invalid.");
        return FALSE;
    }

    switch (m_kind)
    {
    case ProcessKindPython:
        command.append(L" -u ");
        break;
    case ProcessKindPowerShell:
        command.append(L" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File ");
        break;
    case ProcessKindPhp:
        command.append(L" -f ");
        break;
    }
    if (!AppendQuotedArgument(command, request->EntryPoint))
    {
        SetRuntimeFailure(result, E_INVALIDARG, L"The script entry point is invalid.");
        return FALSE;
    }

    SECURITY_ATTRIBUTES security;
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;

    HANDLE outputRead = NULL;
    HANDLE outputWrite = NULL;
    HANDLE inputHandle = NULL;
    if (!CreatePipe(&outputRead, &outputWrite, &security, 0) ||
        !SetHandleInformation(outputRead, HANDLE_FLAG_INHERIT, 0))
    {
        if (outputRead != NULL)
            CloseHandle(outputRead);
        if (outputWrite != NULL)
            CloseHandle(outputWrite);
        SetRuntimeFailure(
            result,
            HRESULT_FROM_WIN32(GetLastError()),
            L"Unable to create the runtime output pipe.");
        return FALSE;
    }

    inputHandle = CreateFileW(
        L"NUL",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security,
        OPEN_EXISTING,
        0,
        NULL);
    if (inputHandle == INVALID_HANDLE_VALUE)
        inputHandle = NULL;

    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = inputHandle;
    startup.hStdOutput = outputWrite;
    startup.hStdError = outputWrite;

    std::vector<wchar_t> commandLine(command.begin(), command.end());
    commandLine.push_back(L'\0');

    std::wstring workingDirectory;
    if (request->WorkingDirectory != NULL && request->WorkingDirectory[0] != L'\0')
    {
        workingDirectory.assign(request->WorkingDirectory);
    }
    else
    {
        workingDirectory.assign(request->EntryPoint);
        std::wstring::size_type slash = workingDirectory.find_last_of(L"\\/");
        if (slash != std::wstring::npos)
            workingDirectory.resize(slash);
        else
            workingDirectory.clear();
    }

    BOOL created = CreateProcessW(
        NULL,
        &commandLine[0],
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        workingDirectory.empty() ? NULL : workingDirectory.c_str(),
        &startup,
        &process);
    CloseHandle(outputWrite);
    outputWrite = NULL;
    if (inputHandle != NULL)
        CloseHandle(inputHandle);

    if (!created)
    {
        DWORD error = GetLastError();
        CloseHandle(outputRead);
        SetRuntimeFailure(
            result,
            HRESULT_FROM_WIN32(error),
            L"Unable to start the external script runtime.");
        return FALSE;
    }

    result->ProcessId = process.dwProcessId;
    std::string capturedOutput;
    const size_t maxCapturedOutput = 1024 * 1024;
    bool timedOut = false;
    ULONGLONG startedAt = GetTickCount64();
    DWORD timeout = request->TimeoutMs == 0 ? 120000 : request->TimeoutMs;
    if (timeout > 3600000)
        timeout = 3600000;

    for (;;)
    {
        DWORD available = 0;
        while (PeekNamedPipe(outputRead, NULL, 0, NULL, &available, NULL) &&
               available != 0)
        {
            char buffer[2048];
            DWORD toRead = available < sizeof(buffer) ? available : sizeof(buffer);
            DWORD read = 0;
            if (!ReadFile(outputRead, buffer, toRead, &read, NULL) || read == 0)
                break;
            if (capturedOutput.size() < maxCapturedOutput)
            {
                size_t remaining = maxCapturedOutput - capturedOutput.size();
                size_t toAppend = read < remaining ? read : remaining;
                capturedOutput.append(buffer, buffer + toAppend);
            }
        }

        DWORD wait = WaitForSingleObject(process.hProcess, 10);
        if (wait == WAIT_OBJECT_0)
            break;
        if (wait == WAIT_FAILED)
        {
            TerminateProcess(process.hProcess, 1);
            break;
        }
        if (GetTickCount64() - startedAt >= timeout)
        {
            timedOut = true;
            TerminateProcess(process.hProcess, 1);
            WaitForSingleObject(process.hProcess, 1000);
            break;
        }
    }

    for (;;)
    {
        DWORD available = 0;
        if (!PeekNamedPipe(outputRead, NULL, 0, NULL, &available, NULL) ||
            available == 0)
            break;
        char buffer[2048];
        DWORD toRead = available < sizeof(buffer) ? available : sizeof(buffer);
        DWORD read = 0;
        if (!ReadFile(outputRead, buffer, toRead, &read, NULL) || read == 0)
            break;
        if (capturedOutput.size() < maxCapturedOutput)
        {
            size_t remaining = maxCapturedOutput - capturedOutput.size();
            size_t toAppend = read < remaining ? read : remaining;
            capturedOutput.append(buffer, buffer + toAppend);
        }
    }

    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(outputRead);

    SetRuntimeText(
        capturedOutput,
        result->Output,
        _countof(result->Output),
        &result->OutputLength);
    result->ExitCode = exitCode;

    if (timedOut)
    {
        result->Status = Salamatrix::Runtime::RuntimeExecutionStatusCancelled;
        result->ErrorCode = HRESULT_FROM_WIN32(ERROR_TIMEOUT);
        StringCchCopyW(
            result->Message,
            _countof(result->Message),
            L"The external script runtime exceeded its execution timeout.");
        return FALSE;
    }

    if (exitCode != 0)
    {
        result->Status = Salamatrix::Runtime::RuntimeExecutionStatusFailed;
        result->ErrorCode = HRESULT_FROM_WIN32(exitCode);
        StringCchPrintfW(
            result->Message,
            _countof(result->Message),
            L"The external script runtime exited with code %lu.",
            exitCode);
        return FALSE;
    }

    result->Status = Salamatrix::Runtime::RuntimeExecutionStatusSucceeded;
    result->ErrorCode = S_OK;
    return TRUE;
}

void CAutomationSalamatrixBridge::Reset()
{
    UnregisterRuntimeAdapters();
    m_bQueried = false;
    m_pGeneral = NULL;
    m_pScriptRoot = NULL;
    m_pUIService = NULL;
    m_pCommandService = NULL;
    m_pFileOperationsService = NULL;
    m_pRuntimeService = NULL;
    m_pSidesService = NULL;
    m_pEventsService = NULL;
    m_pExtensionsService = NULL;
    m_pStorageService = NULL;
    m_dwAutomationVersion = 0;
    m_dwUIVersion = 0;
    m_dwCommandsVersion = 0;
    m_dwFileOperationsVersion = 0;
    m_dwRuntimeVersion = 0;
    m_dwSidesVersion = 0;
    m_dwEventsVersion = 0;
    m_dwExtensionsVersion = 0;
    m_dwStorageVersion = 0;
    m_bRuntimeAdaptersRegistered = false;
}

void CAutomationSalamatrixBridge::RegisterRuntimeAdapters()
{
    if (m_pRuntimeService == NULL || m_bRuntimeAdaptersRegistered)
        return;

    bool registeredAny = false;
    if (m_oJScriptRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oJScriptRuntime) != FALSE;
    if (m_oVBScriptRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oVBScriptRuntime) != FALSE;
    if (m_oPythonRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oPythonRuntime) != FALSE;
    if (m_oPHPRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oPHPRuntime) != FALSE;
    if (m_oCPythonRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oCPythonRuntime) != FALSE;
    if (m_oPowerShellRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oPowerShellRuntime) != FALSE;
    if (m_oPHPCliRuntime.IsAvailable())
        registeredAny |= m_pRuntimeService->RegisterAdapter(&m_oPHPCliRuntime) != FALSE;

    m_bRuntimeAdaptersRegistered = registeredAny;
}

void CAutomationSalamatrixBridge::UnregisterRuntimeAdapters()
{
    if (m_pRuntimeService == NULL || m_pGeneral == NULL)
        return;

    void* currentService = QueryService(m_pGeneral, SALAMATRIX_SERVICE_RUNTIME,
                                        SALAMATRIX_RUNTIME_VERSION_1_0, NULL);
    if (currentService != m_pRuntimeService)
        return;

    m_pRuntimeService->UnregisterAdapter(&m_oPHPRuntime);
    m_pRuntimeService->UnregisterAdapter(&m_oPythonRuntime);
    m_pRuntimeService->UnregisterAdapter(&m_oVBScriptRuntime);
    m_pRuntimeService->UnregisterAdapter(&m_oJScriptRuntime);
    m_pRuntimeService->UnregisterAdapter(&m_oPHPCliRuntime);
    m_pRuntimeService->UnregisterAdapter(&m_oPowerShellRuntime);
    m_pRuntimeService->UnregisterAdapter(&m_oCPythonRuntime);
    m_bRuntimeAdaptersRegistered = false;
}

void* CAutomationSalamatrixBridge::QueryService(
    CSalamanderGeneralAbstract* salamander,
    const char* serviceName,
    DWORD minVersion,
    DWORD* actualVersion)
{
    if (actualVersion != NULL)
    {
        *actualVersion = 0;
    }

    if (salamander == NULL || serviceName == NULL)
    {
        return NULL;
    }

    CSalamanderServiceQuery query;
    memset(&query, 0, sizeof(query));
    query.ServiceId = serviceName;
    query.MinimumVersion = minVersion;

    CSalamanderServiceResult result;
    memset(&result, 0, sizeof(result));

    if (!salamander->QueryService(&query, &result))
    {
        return NULL;
    }

    if (actualVersion != NULL)
    {
        *actualVersion = result.Version;
    }

    return result.Interface;
}

void CAutomationSalamatrixBridge::Refresh(CSalamanderGeneralAbstract* salamander)
{
    Reset();
    m_bQueried = true;
    m_pGeneral = salamander;

    m_pScriptRoot = static_cast<Salamatrix::Automation::ScriptRootAdapter*>(
        QueryService(salamander, SALAMATRIX_SERVICE_AUTOMATION_ADAPTER,
                     SALAMATRIX_AUTOMATION_VERSION_1_0, &m_dwAutomationVersion));

    m_pUIService = static_cast<Salamatrix::UI::IUIService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_UI,
                     SALAMATRIX_UI_VERSION_1_0, &m_dwUIVersion));

    m_pCommandService = static_cast<Salamatrix::Commands::ICommandService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_COMMANDS,
                     SALAMATRIX_COMMANDS_VERSION_1_0, &m_dwCommandsVersion));

    m_pFileOperationsService = static_cast<Salamatrix::FileOperations::IFileOperationsService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_FILEOPERATIONS,
                     SALAMATRIX_FILEOPERATIONS_VERSION_1_0, &m_dwFileOperationsVersion));

    m_pRuntimeService = static_cast<Salamatrix::Runtime::IRuntimeService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_RUNTIME,
                     SALAMATRIX_RUNTIME_VERSION_1_0, &m_dwRuntimeVersion));

    m_pSidesService = static_cast<Salamatrix::Sides::ISidesService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_SIDES,
                     SALAMATRIX_SIDES_VERSION_1_0, &m_dwSidesVersion));

    m_pEventsService = static_cast<Salamatrix::Events::IEventsService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_EVENTS,
                     SALAMATRIX_EVENTS_VERSION_1_0, &m_dwEventsVersion));

    m_pExtensionsService = static_cast<Salamatrix::Extensions::IExtensionsService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_EXTENSIONS,
                     SALAMATRIX_EXTENSIONS_VERSION_1_0, &m_dwExtensionsVersion));

    m_pStorageService = static_cast<Salamatrix::Storage::IStorageService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_STORAGE,
                     SALAMATRIX_STORAGE_VERSION_1_0, &m_dwStorageVersion));
    RegisterRuntimeAdapters();
}

void CAutomationSalamatrixBridge::GetStatusText(PTSTR buffer, int cchBuffer) const
{
    if (buffer == NULL || cchBuffer <= 0)
    {
        return;
    }

    if (!m_bQueried)
    {
        StringCchCopy(buffer, cchBuffer, TEXT("not queried yet"));
        return;
    }

    if (!IsAvailable())
    {
        StringCchCopy(buffer, cchBuffer, TEXT("not available (install/load Salamatrix Framework)"));
        return;
    }

    StringCchPrintf(buffer, cchBuffer,
                    TEXT("available (UI: %s, Commands: %s, FileOperations: %s, Sides: %s, Events: %s, Extensions: %s, Storage: %s, Runtime broker: %s, adapters: %d)"),
                    HasUI() ? TEXT("yes") : TEXT("no"),
                    HasCommands() ? TEXT("yes") : TEXT("no"),
                    HasFileOperations() ? TEXT("yes") : TEXT("no"),
                    HasSides() ? TEXT("yes") : TEXT("no"),
                    HasEvents() ? TEXT("yes") : TEXT("no"),
                    HasExtensions() ? TEXT("yes") : TEXT("no"),
                    HasStorage() ? TEXT("yes") : TEXT("no"),
                    HasRuntimeBroker() ? TEXT("yes") : TEXT("no"),
                    HasRuntimeBroker() ? m_pRuntimeService->GetAdapterCount() : 0);
}
