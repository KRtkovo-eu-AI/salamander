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

#include <deque>
#include <vector>

namespace
{
static bool GetEnvironmentString(
    const wchar_t* name,
    std::wstring& value)
{
    value.clear();
    DWORD required = GetEnvironmentVariableW(name, NULL, 0);
    if (required == 0)
        return false;
    std::vector<wchar_t> buffer(static_cast<size_t>(required));
    DWORD length = GetEnvironmentVariableW(
        name, &buffer[0], required);
    if (length == 0 || length >= required)
        return false;
    value.assign(&buffer[0], length);
    return true;
}

static bool GetModulePathString(HMODULE module, std::wstring& value)
{
    value.clear();
    DWORD capacity = SAL_MAX_PATH;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        std::vector<wchar_t> buffer(static_cast<size_t>(capacity));
        DWORD length = GetModuleFileNameW(
            module, &buffer[0], capacity);
        if (length == 0)
            return false;
        if (length < capacity - 1)
        {
            value.assign(&buffer[0], length);
            return true;
        }
        capacity *= 2;
    }
    return false;
}

static bool SearchPathString(
    const wchar_t* fileName,
    std::wstring& value)
{
    value.clear();
    DWORD capacity = SAL_MAX_PATH;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        std::vector<wchar_t> buffer(static_cast<size_t>(capacity));
        DWORD length = SearchPathW(
            NULL, fileName, NULL, capacity, &buffer[0], NULL);
        if (length == 0)
            return false;
        if (length < capacity)
        {
            value.assign(&buffer[0], length);
            return true;
        }
        capacity = length + 1;
    }
    return false;
}

static std::wstring ToWin32Path(const std::wstring& value)
{
    if (value.size() < MAX_PATH || value.compare(0, 4, L"\\\\?\\") == 0)
        return value;
    if (value.size() >= 2 && value[0] == L'\\' && value[1] == L'\\')
        return L"\\\\?\\UNC\\" + value.substr(2);
    return L"\\\\?\\" + value;
}

static bool AppendQuotedArgument(std::wstring& command, const wchar_t* value)
{
    if (value == NULL)
        return false;
    command.push_back(L'"');
    size_t backslashes = 0;
    for (const wchar_t* current = value; *current != L'\0'; ++current)
    {
        if (*current == L'\\') { ++backslashes; continue; }
        if (*current == L'"')
        {
            command.append(backslashes * 2 + 1, L'\\');
            command.push_back(L'"');
        }
        else
        {
            command.append(backslashes, L'\\');
            command.push_back(*current);
        }
        backslashes = 0;
    }
    command.append(backslashes * 2, L'\\');
    command.push_back(L'"');
    return true;
}

static bool AppendUtf8QuotedArgument(std::wstring& command, const char* value)
{
    if (value == NULL)
        return false;
    int required = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, NULL, 0);
    UINT codePage = CP_UTF8;
    if (required <= 0)
    {
        required = MultiByteToWideChar(CP_ACP, 0, value, -1, NULL, 0);
        codePage = CP_ACP;
    }
    if (required <= 0)
        return false;
    std::vector<wchar_t> converted(static_cast<size_t>(required));
    if (MultiByteToWideChar(
            codePage,
            codePage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
            value,
            -1,
            &converted[0],
            required) <= 0)
        return false;
    return AppendQuotedArgument(command, &converted[0]);
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

class CAutomationProcessRuntimeSession : public Salamatrix::Runtime::IRuntimeSession
{
private:
    HANDLE m_hProcess;
    HANDLE m_hThread;
    HANDLE m_hInput;
    HANDLE m_hOutput;
    Salamatrix::Runtime::RuntimeExecutionRequest::RuntimeHostDispatchProc
        m_pHostDispatch;
    void* m_pHostDispatchContext;
    mutable CRITICAL_SECTION m_lock;
    std::string m_pending;
    std::deque<std::string> m_queuedFrames;
    Salamatrix::Runtime::RuntimeSessionState m_lastState;
    DWORD m_lastProcessId;
    DWORD m_lastExitCode;
    HRESULT m_lastErrorCode;
    enum { MaxQueuedFrames = 128 };

    CAutomationProcessRuntimeSession(
        HANDLE process,
        HANDLE thread,
        HANDLE input,
        HANDLE output,
        Salamatrix::Runtime::RuntimeExecutionRequest::RuntimeHostDispatchProc
            hostDispatch,
        void* hostDispatchContext)
        : m_hProcess(process),
          m_hThread(thread),
          m_hInput(input),
          m_hOutput(output),
          m_pHostDispatch(hostDispatch),
          m_pHostDispatchContext(hostDispatchContext),
          m_lastState(Salamatrix::Runtime::RuntimeSessionStateRunning),
          m_lastProcessId(process == NULL ? 0 : GetProcessId(process)),
          m_lastExitCode(0),
          m_lastErrorCode(S_OK)
    {
        InitializeCriticalSection(&m_lock);
    }

    static BOOL ReadAvailable(
        HANDLE output,
        std::string& pending)
    {
        DWORD available = 0;
        while (PeekNamedPipe(output, NULL, 0, NULL, &available, NULL) &&
               available != 0)
        {
            char buffer[2048];
            DWORD toRead = available < sizeof(buffer) ? available : sizeof(buffer);
            DWORD read = 0;
            if (!ReadFile(output, buffer, toRead, &read, NULL) || read == 0)
                return FALSE;
            if (pending.size() + read > Salamatrix::Runtime::Protocol::MaxFrameBytes)
                return FALSE;
            pending.append(buffer, buffer + read);
        }
        return TRUE;
    }

public:
    static CAutomationProcessRuntimeSession* Create(
        HANDLE process,
        HANDLE thread,
        HANDLE input,
        HANDLE output,
        Salamatrix::Runtime::RuntimeExecutionRequest::RuntimeHostDispatchProc
            hostDispatch,
        void* hostDispatchContext)
    {
        return new CAutomationProcessRuntimeSession(
            process,
            thread,
            input,
            output,
            hostDispatch,
            hostDispatchContext);
    }

    virtual ~CAutomationProcessRuntimeSession()
    {
        Stop();
        DeleteCriticalSection(&m_lock);
    }

    virtual BOOL WINAPI IsAlive() const
    {
        EnterCriticalSection(&m_lock);
        BOOL alive = m_hProcess != NULL &&
                     WaitForSingleObject(m_hProcess, 0) == WAIT_TIMEOUT;
        LeaveCriticalSection(&m_lock);
        return alive;
    }

    void UpdateLifecycleStateLocked()
    {
        if (m_hProcess == NULL)
            return;

        m_lastProcessId = GetProcessId(m_hProcess);
        DWORD waitResult = WaitForSingleObject(m_hProcess, 0);
        if (waitResult == WAIT_TIMEOUT)
        {
            m_lastState = Salamatrix::Runtime::RuntimeSessionStateRunning;
            return;
        }

        if (waitResult == WAIT_OBJECT_0)
        {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(m_hProcess, &exitCode))
            {
                m_lastExitCode = exitCode;
                if (exitCode == 0)
                {
                    m_lastState = Salamatrix::Runtime::RuntimeSessionStateExited;
                    m_lastErrorCode = S_OK;
                }
                else
                {
                    m_lastState = Salamatrix::Runtime::RuntimeSessionStateFailed;
                    m_lastErrorCode = HRESULT_FROM_WIN32(exitCode);
                }
                return;
            }
        }

        m_lastState = Salamatrix::Runtime::RuntimeSessionStateFailed;
        m_lastErrorCode = HRESULT_FROM_WIN32(GetLastError());
    }

    virtual BOOL WINAPI GetDiagnostic(
        Salamatrix::Runtime::RuntimeSessionDiagnostic* diagnostic) const
    {
        if (diagnostic == NULL)
            return FALSE;

        *diagnostic = Salamatrix::Runtime::RuntimeSessionDiagnostic();
        EnterCriticalSection(&m_lock);
        const_cast<CAutomationProcessRuntimeSession*>(this)
            ->UpdateLifecycleStateLocked();
        diagnostic->State = m_lastState;
        diagnostic->ProcessId = m_lastProcessId;
        diagnostic->ExitCode = m_lastExitCode;
        diagnostic->ErrorCode = m_lastErrorCode;
        LeaveCriticalSection(&m_lock);

        if (diagnostic->State == Salamatrix::Runtime::RuntimeSessionStateStopped)
        {
            StringCchCopyW(
                diagnostic->Message,
                _countof(diagnostic->Message),
                L"Runtime worker was stopped by the host.");
        }
        else if (diagnostic->ExitCode != 0)
        {
            StringCchPrintfW(
                diagnostic->Message,
                _countof(diagnostic->Message),
                L"Runtime worker exited with code %lu.",
                diagnostic->ExitCode);
        }
        else if (diagnostic->ErrorCode != S_OK)
        {
            StringCchPrintfW(
                diagnostic->Message,
                _countof(diagnostic->Message),
                L"Runtime worker lifecycle failed (HRESULT 0x%08lX).",
                static_cast<unsigned long>(diagnostic->ErrorCode));
        }
        return TRUE;
    }

    BOOL FlushQueuedFrames()
    {
        for (;;)
        {
            std::string frame;
            EnterCriticalSection(&m_lock);
            if (m_queuedFrames.empty())
            {
                LeaveCriticalSection(&m_lock);
                return TRUE;
            }
            frame.swap(m_queuedFrames.front());
            m_queuedFrames.pop_front();
            LeaveCriticalSection(&m_lock);
            if (!SendFrame(frame.c_str(), static_cast<DWORD>(frame.size())))
                return FALSE;
        }
    }

    virtual BOOL WINAPI GetExitCode(DWORD* exitCode) const
    {
        if (exitCode == NULL)
            return FALSE;
        *exitCode = 0;
        EnterCriticalSection(&m_lock);
        if (m_hProcess == NULL ||
            WaitForSingleObject(m_hProcess, 0) == WAIT_TIMEOUT)
        {
            LeaveCriticalSection(&m_lock);
            return FALSE;
        }
        BOOL result = GetExitCodeProcess(m_hProcess, exitCode);
        LeaveCriticalSection(&m_lock);
        return result;
    }

    virtual BOOL WINAPI SendFrame(const char* bytes, DWORD count)
    {
        if (bytes == NULL || count == 0 ||
            count > Salamatrix::Runtime::Protocol::MaxFrameBytes)
            return FALSE;

        EnterCriticalSection(&m_lock);
        if (m_hInput == NULL || m_hProcess == NULL ||
            WaitForSingleObject(m_hProcess, 0) != WAIT_TIMEOUT)
        {
            LeaveCriticalSection(&m_lock);
            return FALSE;
        }

        DWORD offset = 0;
        BOOL result = TRUE;
        while (offset < count)
        {
            DWORD written = 0;
            if (!WriteFile(m_hInput, bytes + offset, count - offset, &written, NULL) ||
                written == 0)
            {
                result = FALSE;
                break;
            }
            offset += written;
        }
        LeaveCriticalSection(&m_lock);
        return result;
    }

    virtual BOOL WINAPI QueueFrame(const char* bytes, DWORD count)
    {
        if (bytes == NULL || count == 0 ||
            count > Salamatrix::Runtime::Protocol::MaxFrameBytes)
            return FALSE;
        EnterCriticalSection(&m_lock);
        if (m_hInput == NULL || m_hProcess == NULL ||
            WaitForSingleObject(m_hProcess, 0) != WAIT_TIMEOUT ||
            m_queuedFrames.size() >= MaxQueuedFrames)
        {
            LeaveCriticalSection(&m_lock);
            return FALSE;
        }
        m_queuedFrames.push_back(std::string(bytes, count));
        LeaveCriticalSection(&m_lock);
        return TRUE;
    }

    virtual BOOL WINAPI ReceiveFrame(
        char* bytes,
        DWORD capacity,
        DWORD timeoutMs,
        DWORD* received)
    {
        if (received != NULL)
            *received = 0;
        if (bytes == NULL || capacity < 2 || received == NULL)
            return FALSE;

        ULONGLONG start = GetTickCount64();
        for (;;)
        {
            EnterCriticalSection(&m_lock);
            if (m_hOutput == NULL || !ReadAvailable(m_hOutput, m_pending))
            {
                LeaveCriticalSection(&m_lock);
                return FALSE;
            }

            std::string::size_type newline = m_pending.find('\n');
            if (newline != std::string::npos)
            {
                size_t frameBytes = newline + 1;
                if (frameBytes >= capacity)
                {
                    m_pending.clear();
                    LeaveCriticalSection(&m_lock);
                    return FALSE;
                }
                memcpy(bytes, m_pending.data(), frameBytes);
                bytes[frameBytes] = '\0';
                *received = static_cast<DWORD>(frameBytes);
                m_pending.erase(0, frameBytes);
                LeaveCriticalSection(&m_lock);
                return TRUE;
            }
            BOOL alive = m_hProcess != NULL &&
                         WaitForSingleObject(m_hProcess, 0) == WAIT_TIMEOUT;
            LeaveCriticalSection(&m_lock);
            if (!alive)
                return FALSE;
            if (timeoutMs != INFINITE && GetTickCount64() - start >= timeoutMs)
                return FALSE;
            Sleep(5);
        }
    }

    virtual BOOL WINAPI Pump(DWORD timeoutMs)
    {
        if (m_pHostDispatch == NULL)
            return FALSE;

        if (!FlushQueuedFrames())
            return FALSE;

        std::vector<char> frameBytes(Salamatrix::Runtime::Protocol::MaxFrameBytes);
        DWORD received = 0;
        if (!ReceiveFrame(
                &frameBytes[0],
                static_cast<DWORD>(frameBytes.size()),
                timeoutMs,
                &received))
        {
            return FALSE;
        }

        Salamatrix::Runtime::Protocol::LineCodec codec;
        Salamatrix::Runtime::Protocol::Frame frame;
        BOOL complete = FALSE;
        if (!codec.Append(&frameBytes[0], received, &frame, &complete) ||
            !complete)
        {
            return FALSE;
        }
        if (frame.Type == Salamatrix::Runtime::Protocol::MessageShutdown)
        {
            Stop();
            return FALSE;
        }

        // Keep host responses comfortably above the original small JSON
        // buffer; tab enumeration and UI snapshots can legitimately contain
        // several paths. The vector avoids a large stack allocation.
        std::vector<char> response(65536);
        DWORD responseLength = 0;
        BOOL dispatched = m_pHostDispatch(
            m_pHostDispatchContext,
            frame.Type,
            frame.Id,
            frame.PayloadJson.c_str(),
            &response[0],
            static_cast<DWORD>(response.size()),
            &responseLength);
        if (responseLength == 0)
        {
            const char* fallback =
                dispatched ? "{\"ok\":true}" :
                             "{\"ok\":false,\"error\":\"host_dispatch_failed\"}";
            responseLength = static_cast<DWORD>(strlen(fallback));
            memcpy(&response[0], fallback, responseLength);
        }

        std::string encoded;
        if (!Salamatrix::Runtime::Protocol::LineCodec::Encode(
                dispatched
                    ? Salamatrix::Runtime::Protocol::MessageResult
                    : Salamatrix::Runtime::Protocol::MessageError,
                frame.Id,
                std::string(&response[0], responseLength),
                &encoded))
        {
            return FALSE;
        }
        return SendFrame(encoded.c_str(), static_cast<DWORD>(encoded.size()));
    }

    virtual void WINAPI Stop()
    {
        EnterCriticalSection(&m_lock);
        if (m_hInput != NULL)
        {
            CloseHandle(m_hInput);
            m_hInput = NULL;
        }
        if (m_hProcess != NULL)
        {
            DWORD waitResult = WaitForSingleObject(m_hProcess, 0);
            if (waitResult == WAIT_TIMEOUT)
            {
                m_lastState = Salamatrix::Runtime::RuntimeSessionStateStopped;
                m_lastErrorCode = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                TerminateProcess(m_hProcess, 1);
                WaitForSingleObject(m_hProcess, 1000);
            }
            else if (waitResult == WAIT_OBJECT_0)
            {
                UpdateLifecycleStateLocked();
            }
            else
            {
                m_lastState = Salamatrix::Runtime::RuntimeSessionStateFailed;
                m_lastErrorCode = HRESULT_FROM_WIN32(GetLastError());
            }
            CloseHandle(m_hProcess);
            m_hProcess = NULL;
        }
        if (m_hThread != NULL)
        {
            CloseHandle(m_hThread);
            m_hThread = NULL;
        }
        if (m_hOutput != NULL)
        {
            CloseHandle(m_hOutput);
            m_hOutput = NULL;
        }
        LeaveCriticalSection(&m_lock);
    }

    virtual void WINAPI Release()
    {
        delete this;
    }
};

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
      m_pAssistantService(NULL),
      m_dwAutomationVersion(0),
      m_dwUIVersion(0),
      m_dwCommandsVersion(0),
      m_dwFileOperationsVersion(0),
      m_dwRuntimeVersion(0),
      m_dwSidesVersion(0),
      m_dwEventsVersion(0),
      m_dwExtensionsVersion(0),
      m_dwStorageVersion(0),
      m_dwAssistantVersion(0),
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

#if 0 // Local assistant ownership moved to the standalone Salamatrix AI plugin.
CAutomationLocalAssistantProvider::CAutomationLocalAssistantProvider()
{
    m_oDescriptor.ProviderId = "local.command";
    m_oDescriptor.DisplayName = "Local command assistant";
    m_oDescriptor.ProviderVersion = 0x00010000;
    m_oDescriptor.Flags = 0;
}

void CAutomationLocalAssistantProvider::ResolveCommand() const
{
    wchar_t value[4096];
    DWORD length = GetEnvironmentVariableW(
        L"SALAMATRIX_AI_COMMAND", value, _countof(value));
    if (length == 0 || length >= _countof(value))
    {
        m_commandLine.clear();
        return;
    }
    m_commandLine.assign(value, length);
}

const Salamatrix::AI::AssistantProviderDescriptor* WINAPI
CAutomationLocalAssistantProvider::GetDescriptor() const
{
    return &m_oDescriptor;
}

BOOL WINAPI CAutomationLocalAssistantProvider::IsAvailable() const
{
    ResolveCommand();
    return m_commandLine.empty() ? FALSE : TRUE;
}

BOOL WINAPI CAutomationLocalAssistantProvider::Generate(
    const Salamatrix::AI::AssistantRequest* request,
    Salamatrix::AI::AssistantResponse* response)
{
    if (response == NULL || response->StructSize < sizeof(*response))
        return FALSE;
    *response = Salamatrix::AI::AssistantResponse();
    if (request == NULL || request->StructSize < sizeof(*request))
    {
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusFailed,
                            E_INVALIDARG, L"The assistant request is invalid.");
        return FALSE;
    }
    if (!IsAvailable())
    {
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusUnavailable,
                            HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
                            L"No local assistant command is configured.");
        return FALSE;
    }

    std::string requestJson =
        std::string("{\"apiVersion\":\"") +
        EscapeAssistantJson(request->ApiVersion != NULL ? request->ApiVersion : "1.0") +
        "\",\"prompt\":\"" +
        EscapeAssistantJson(request->Prompt != NULL ? request->Prompt : "") +
        "\",\"context\":" +
        (request->ContextJson != NULL && request->ContextJson[0] != '\0'
             ? request->ContextJson
             : "{}") +
        ",\"runtime\":\"" +
        EscapeAssistantJson(request->RuntimeId != NULL ? request->RuntimeId : "") +
        "\",\"existingScript\":\"" +
        EscapeAssistantJson(request->ExistingScript != NULL ? request->ExistingScript : "") +
        "\",\"feedback\":\"" +
        EscapeAssistantJson(request->Feedback != NULL ? request->Feedback : "") +
        "\",\"maxOutputBytes\":" +
        std::to_string(request->MaxOutputBytes == 0 ? 65535 : request->MaxOutputBytes) +
        "}\n";

    SECURITY_ATTRIBUTES security;
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;
    HANDLE childInput = NULL;
    HANDLE parentInput = NULL;
    HANDLE childOutput = NULL;
    HANDLE parentOutput = NULL;
    if (!CreatePipe(&parentInput, &childInput, &security, 0) ||
        !SetHandleInformation(parentInput, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&parentOutput, &childOutput, &security, 0) ||
        !SetHandleInformation(parentOutput, HANDLE_FLAG_INHERIT, 0))
    {
        if (parentInput != NULL) CloseHandle(parentInput);
        if (childInput != NULL) CloseHandle(childInput);
        if (parentOutput != NULL) CloseHandle(parentOutput);
        if (childOutput != NULL) CloseHandle(childOutput);
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusFailed,
                            HRESULT_FROM_WIN32(GetLastError()),
                            L"Unable to create local assistant pipes.");
        return FALSE;
    }

    HANDLE errorHandle = CreateFileW(
        L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, OPEN_EXISTING, 0, NULL);
    if (errorHandle == INVALID_HANDLE_VALUE)
        errorHandle = NULL;

    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childInput;
    startup.hStdOutput = childOutput;
    startup.hStdError = errorHandle;
    std::vector<wchar_t> commandLine(m_commandLine.begin(), m_commandLine.end());
    commandLine.push_back(L'\0');
    BOOL created = CreateProcessW(
        NULL, &commandLine[0], NULL, NULL, TRUE, CREATE_NO_WINDOW,
        NULL, NULL, &startup, &process);
    CloseHandle(childInput);
    CloseHandle(childOutput);
    if (errorHandle != NULL)
        CloseHandle(errorHandle);
    if (!created)
    {
        CloseHandle(parentInput);
        CloseHandle(parentOutput);
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusFailed,
                            HRESULT_FROM_WIN32(GetLastError()),
                            L"Unable to start the local assistant command.");
        return FALSE;
    }

    DWORD written = 0;
    BOOL writeOk = WriteFile(parentInput, requestJson.data(),
                             static_cast<DWORD>(requestJson.size()), &written, NULL);
    CloseHandle(parentInput);
    parentInput = NULL;
    if (!writeOk || written != requestJson.size())
        TerminateProcess(process.hProcess, 1);

    std::string output;
    const size_t maxOutput = 1024 * 1024;
    bool timedOut = false;
    ULONGLONG startedAt = GetTickCount64();
    // Keep the local assistant bounded to the same two-minute ceiling used by
    // the public request default. A provider must never turn a malformed or
    // untrusted request into an unbounded child process.
    DWORD timeout = request->TimeoutMs == 0 ? 120000 : request->TimeoutMs;
    if (timeout > 120000)
        timeout = 120000;
    for (;;)
    {
        DWORD available = 0;
        while (PeekNamedPipe(parentOutput, NULL, 0, NULL, &available, NULL) &&
               available != 0)
        {
            char buffer[4096];
            DWORD toRead = available < sizeof(buffer) ? available : sizeof(buffer);
            DWORD read = 0;
            if (!ReadFile(parentOutput, buffer, toRead, &read, NULL) || read == 0)
                break;
            size_t remaining = output.size() < maxOutput ? maxOutput - output.size() : 0;
            if (remaining != 0)
                output.append(buffer, buffer + (read < remaining ? read : remaining));
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
        if (!PeekNamedPipe(parentOutput, NULL, 0, NULL, &available, NULL) ||
            available == 0)
            break;
        char buffer[4096];
        DWORD toRead = available < sizeof(buffer) ? available : sizeof(buffer);
        DWORD read = 0;
        if (!ReadFile(parentOutput, buffer, toRead, &read, NULL) || read == 0)
            break;
        size_t remaining = output.size() < maxOutput ? maxOutput - output.size() : 0;
        if (remaining != 0)
            output.append(buffer, buffer + (read < remaining ? read : remaining));
    }
    DWORD exitCode = 1;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    CloseHandle(parentOutput);

    if (timedOut)
    {
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusCancelled,
                            HRESULT_FROM_WIN32(ERROR_TIMEOUT),
                            L"The local assistant exceeded its execution timeout.");
        return FALSE;
    }
    if (exitCode != 0)
    {
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusFailed,
                            HRESULT_FROM_WIN32(exitCode),
                            L"The local assistant command failed.");
        return FALSE;
    }
    size_t first = output.find_first_not_of(" \t\r\n");
    size_t last = output.find_last_not_of(" \t\r\n");
    if (first == std::string::npos || last < first ||
        last - first + 1 >= _countof(response->ResponseJson) ||
        !IsStructuredJson(output))
    {
        SetAssistantFailure(response, Salamatrix::AI::AssistantStatusInvalidResponse,
                            HRESULT_FROM_WIN32(ERROR_INVALID_DATA),
                            L"The local assistant did not return structured JSON.");
        return FALSE;
    }
    size_t length = last - first + 1;
    memcpy(response->ResponseJson, output.data() + first, length);
    response->ResponseJson[length] = '\0';
    response->OutputLength = static_cast<DWORD>(length);
    response->Status = Salamatrix::AI::AssistantStatusSucceeded;
    response->ErrorCode = S_OK;
    response->Message[0] = L'\0';
    return TRUE;
}

#endif

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

static BOOL ResolveWorkerBootstrapPath(
    CAutomationProcessRuntimeAdapter::ProcessKind kind,
    std::wstring* path)
{
    if (path == NULL)
        return FALSE;

    const wchar_t* fileName = NULL;
    switch (kind)
    {
    case CAutomationProcessRuntimeAdapter::ProcessKindPython:
        fileName = L"salamatrix_worker.py";
        break;
    case CAutomationProcessRuntimeAdapter::ProcessKindPowerShell:
        fileName = L"salamatrix_worker.ps1";
        break;
    case CAutomationProcessRuntimeAdapter::ProcessKindPhp:
        fileName = L"salamatrix_worker.php";
        break;
    default:
        return FALSE;
    }

    std::wstring root;
    if (!GetEnvironmentString(L"SALAMATRIX_WORKER_ROOT", root))
    {
        HMODULE module = NULL;
        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&ResolveWorkerBootstrapPath),
                &module))
            return FALSE;
        if (!GetModulePathString(module, root))
            return FALSE;
        std::wstring::size_type slash = root.find_last_of(L"\\/");
        if (slash == std::wstring::npos)
            return FALSE;
        root.resize(slash);
        root.append(L"\\runtime");
    }

    path->assign(root);
    if (!path->empty() && path->back() != L'\\' && path->back() != L'/')
        path->push_back(L'\\');
    path->append(fileName);
    std::wstring filePath = ToWin32Path(*path);
    DWORD attributes = GetFileAttributesW(filePath.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

void CAutomationProcessRuntimeAdapter::ResolveInterpreter() const
{
    if (!m_executablePath.empty())
        return;

    std::wstring configured;
    if (GetEnvironmentString(m_pszEnvironmentVariable, configured))
    {
        std::wstring configuredPath = ToWin32Path(configured);
        DWORD attributes = GetFileAttributesW(configuredPath.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            m_executablePath.assign(configured);
            return;
        }

        std::wstring resolved;
        if (SearchPathString(configured.c_str(), resolved))
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
        std::wstring resolved;
        if (SearchPathString(candidates[index], resolved))
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

BOOL WINAPI CAutomationProcessRuntimeAdapter::StartPersistent(
    const Salamatrix::Runtime::RuntimeExecutionRequest* request,
    Salamatrix::Runtime::IRuntimeSession** session)
{
    if (session != NULL)
        *session = NULL;
    if (session == NULL || request == NULL ||
        request->StructSize < sizeof(*request) ||
        request->EntryPoint == NULL || !IsAvailable())
    {
        return FALSE;
    }

    std::wstring command;
    if (!AppendQuotedArgument(command, m_executablePath.c_str()))
        return FALSE;
    if ((request->Flags &
         Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap) != 0)
    {
        std::wstring bootstrap;
        if (!ResolveWorkerBootstrapPath(m_kind, &bootstrap))
            return FALSE;
        switch (m_kind)
        {
        case ProcessKindPython:
            command.append(L" -u ");
            if (!AppendQuotedArgument(command, bootstrap.c_str()))
                return FALSE;
            command.append(L" --entry ");
            break;
        case ProcessKindPowerShell:
            command.append(L" -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File ");
            if (!AppendQuotedArgument(command, bootstrap.c_str()))
                return FALSE;
            command.append(L" -EntryPoint ");
            break;
        case ProcessKindPhp:
            command.append(L" -f ");
            if (!AppendQuotedArgument(command, bootstrap.c_str()))
                return FALSE;
            command.append(L" -- --entry ");
            break;
        }
    }
    else
    {
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
    }
    if (!AppendQuotedArgument(command, request->EntryPoint))
        return FALSE;
    if ((request->Flags &
         Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap) != 0 &&
        request->CommandId != NULL && request->CommandId[0] != '\0')
    {
        command.append(m_kind == ProcessKindPowerShell
                           ? L" -CommandId "
                           : L" --command-id ");
        if (!AppendUtf8QuotedArgument(command, request->CommandId))
            return FALSE;
    }
    if ((request->Flags &
         Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap) != 0 &&
        request->CommandHandler != NULL &&
        request->CommandHandler[0] != '\0')
    {
        command.append(m_kind == ProcessKindPowerShell
                           ? L" -CommandHandler "
                           : L" --command-handler ");
        if (!AppendUtf8QuotedArgument(command, request->CommandHandler))
            return FALSE;
    }
    if ((request->Flags & Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap) != 0 &&
        request->InvocationJson != NULL && request->InvocationJson[0] != '\0')
    {
        command.append(m_kind == ProcessKindPowerShell
                           ? L" -InvocationJson "
                           : L" --invocation-json ");
        if (!AppendUtf8QuotedArgument(command, request->InvocationJson))
            return FALSE;
    }
    if ((request->Flags &
         Salamatrix::Runtime::RuntimeExecutionFlagOneShotWorker) != 0 &&
        (request->Flags &
         Salamatrix::Runtime::RuntimeExecutionFlagUseWorkerBootstrap) != 0)
    {
        if (m_kind == ProcessKindPowerShell)
            command.append(L" -OneShot");
        else
            command.append(L" --one-shot");
    }

    SECURITY_ATTRIBUTES security;
    memset(&security, 0, sizeof(security));
    security.nLength = sizeof(security);
    security.bInheritHandle = TRUE;

    HANDLE childInput = NULL;
    HANDLE parentInput = NULL;
    HANDLE parentOutput = NULL;
    HANDLE childOutput = NULL;
    if (!CreatePipe(&childInput, &parentInput, &security, 0) ||
        !SetHandleInformation(parentInput, HANDLE_FLAG_INHERIT, 0) ||
        !CreatePipe(&parentOutput, &childOutput, &security, 0) ||
        !SetHandleInformation(parentOutput, HANDLE_FLAG_INHERIT, 0))
    {
        if (childInput != NULL)
            CloseHandle(childInput);
        if (parentInput != NULL)
            CloseHandle(parentInput);
        if (parentOutput != NULL)
            CloseHandle(parentOutput);
        if (childOutput != NULL)
            CloseHandle(childOutput);
        return FALSE;
    }

    HANDLE errorHandle = CreateFileW(
        L"NUL",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security,
        OPEN_EXISTING,
        0,
        NULL);
    if (errorHandle == INVALID_HANDLE_VALUE)
        errorHandle = NULL;

    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = childInput;
    startup.hStdOutput = childOutput;
    startup.hStdError = errorHandle;

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
    CloseHandle(childInput);
    CloseHandle(childOutput);
    if (errorHandle != NULL)
        CloseHandle(errorHandle);
    if (!created)
    {
        CloseHandle(parentInput);
        CloseHandle(parentOutput);
        return FALSE;
    }

    *session = CAutomationProcessRuntimeSession::Create(
        process.hProcess,
        process.hThread,
        parentInput,
        parentOutput,
        request->HostDispatch,
        request->HostDispatchContext);
    if (*session == NULL)
    {
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        CloseHandle(parentInput);
        CloseHandle(parentOutput);
        return FALSE;
    }
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
    m_pAssistantService = NULL;
    m_dwAutomationVersion = 0;
    m_dwUIVersion = 0;
    m_dwCommandsVersion = 0;
    m_dwFileOperationsVersion = 0;
    m_dwRuntimeVersion = 0;
    m_dwSidesVersion = 0;
    m_dwEventsVersion = 0;
    m_dwExtensionsVersion = 0;
    m_dwStorageVersion = 0;
    m_dwAssistantVersion = 0;
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
    m_pAssistantService = static_cast<Salamatrix::AI::IAssistantService*>(
        QueryService(salamander, SALAMATRIX_SERVICE_AI,
                     SALAMATRIX_AI_VERSION_1_0, &m_dwAssistantVersion));
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
                    TEXT("available (UI: %s, Commands: %s, FileOperations: %s, Sides: %s, Events: %s, Extensions: %s, Storage: %s, AI: %s, Runtime broker: %s, adapters: %d)"),
                    HasUI() ? TEXT("yes") : TEXT("no"),
                    HasCommands() ? TEXT("yes") : TEXT("no"),
                    HasFileOperations() ? TEXT("yes") : TEXT("no"),
                    HasSides() ? TEXT("yes") : TEXT("no"),
                    HasEvents() ? TEXT("yes") : TEXT("no"),
                    HasExtensions() ? TEXT("yes") : TEXT("no"),
                    HasStorage() ? TEXT("yes") : TEXT("no"),
                    HasAssistant() ? TEXT("yes") : TEXT("no"),
                    HasRuntimeBroker() ? TEXT("yes") : TEXT("no"),
                    HasRuntimeBroker() ? m_pRuntimeService->GetAdapterCount() : 0);
}
