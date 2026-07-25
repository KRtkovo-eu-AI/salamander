// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "powershellruntime.h"
#include <strsafe.h>
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

class CPowerShellRuntimeSession : public Salamatrix::Runtime::IRuntimeSession
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

    CPowerShellRuntimeSession(
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
          m_pHostDispatchContext(hostDispatchContext)
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
    static CPowerShellRuntimeSession* Create(
        HANDLE process,
        HANDLE thread,
        HANDLE input,
        HANDLE output,
        Salamatrix::Runtime::RuntimeExecutionRequest::RuntimeHostDispatchProc
            hostDispatch,
        void* hostDispatchContext)
    {
        return new CPowerShellRuntimeSession(
            process,
            thread,
            input,
            output,
            hostDispatch,
            hostDispatchContext);
    }

    virtual ~CPowerShellRuntimeSession()
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

        char response[8192];
        DWORD responseLength = 0;
        BOOL dispatched = m_pHostDispatch(
            m_pHostDispatchContext,
            frame.Type,
            frame.Id,
            frame.PayloadJson.c_str(),
            response,
            _countof(response),
            &responseLength);
        if (responseLength == 0)
        {
            const char* fallback =
                dispatched ? "{\"ok\":true}" :
                             "{\"ok\":false,\"error\":\"host_dispatch_failed\"}";
            responseLength = static_cast<DWORD>(strlen(fallback));
            memcpy(response, fallback, responseLength);
        }

        std::string encoded;
        if (!Salamatrix::Runtime::Protocol::LineCodec::Encode(
                dispatched
                    ? Salamatrix::Runtime::Protocol::MessageResult
                    : Salamatrix::Runtime::Protocol::MessageError,
                frame.Id,
                std::string(response, responseLength),
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
            if (WaitForSingleObject(m_hProcess, 0) == WAIT_TIMEOUT)
            {
                TerminateProcess(m_hProcess, 1);
                WaitForSingleObject(m_hProcess, 1000);
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

CPowerShellRuntimeAdapter::CPowerShellRuntimeAdapter(
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
    CPowerShellRuntimeAdapter::ProcessKind kind,
    std::wstring* path)
{
    if (path == NULL)
        return FALSE;

    const wchar_t* fileName = NULL;
    switch (kind)
    {
    case CPowerShellRuntimeAdapter::ProcessKindPython:
        fileName = L"salamatrix_worker.py";
        break;
    case CPowerShellRuntimeAdapter::ProcessKindPowerShell:
        fileName = L"salamatrix_worker.ps1";
        break;
    case CPowerShellRuntimeAdapter::ProcessKindPhp:
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

void CPowerShellRuntimeAdapter::ResolveInterpreter() const
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
CPowerShellRuntimeAdapter::GetDescriptor() const
{
    return &m_oDescriptor;
}

BOOL WINAPI CPowerShellRuntimeAdapter::IsAvailable() const
{
    ResolveInterpreter();
    return m_executablePath.empty() ? FALSE : TRUE;
}

BOOL WINAPI CPowerShellRuntimeAdapter::SupportsEntryPoint(
    const char* entryPoint) const
{
    if (entryPoint == NULL || !IsAvailable())
        return FALSE;
    const char* extension = strrchr(entryPoint, '.');
    return extension != NULL && _stricmp(extension, m_pszExtension) == 0;
}

BOOL WINAPI CPowerShellRuntimeAdapter::Execute(
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
    if (timeout > 120000)
        timeout = 120000;

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

BOOL WINAPI CPowerShellRuntimeAdapter::StartPersistent(
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

    *session = CPowerShellRuntimeSession::Create(
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


CPluginInterface PluginInterface;
HINSTANCE DLLInstance = NULL;
HINSTANCE HLanguage = NULL;
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;
static CPowerShellRuntimeAdapter PowerShellRuntime(
    "PowerShell",
    "CPowerShell runtime provider",
    "powershell",
    ".ps1",
    L"SALAMATRIX_POWERSHELL",
    L"pwsh.exe",
    L"powershell.exe",
    CPowerShellRuntimeAdapter::ProcessKindPowerShell);
static Salamatrix::Runtime::RuntimeProviderRegistration PowerShellRegistration;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        DLLInstance = hinstDLL;
    return TRUE;
}

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(
    CSalamanderPluginEntryAbstract* salamander)
{
    if (salamander == NULL ||
        salamander->GetVersion() < LAST_VERSION_OF_SALAMANDER)
        return NULL;
    SalamanderGeneral = salamander->GetSalamanderGeneral();
    if (SalamanderGeneral == NULL)
        return NULL;
    salamander->SetBasicPluginData(
        "PowerShell Runtime",
        FUNCTION_AUTOMATIONFRAMEWORK,
        VERSINFO_VERSION_NO_PLATFORM,
        VERSINFO_COPYRIGHT,
        VERSINFO_DESCRIPTION,
        "POWERSHELL.RUNTIME",
        NULL,
        NULL);
    CSalamanderServiceQuery query;
    CSalamanderServiceResult serviceResult;
    memset(&query, 0, sizeof(query));
    memset(&serviceResult, 0, sizeof(serviceResult));
    query.ServiceId = Salamatrix::Runtime::SALAMATRIX_SERVICE_RUNTIME;
    query.MinimumVersion = Salamatrix::Runtime::SALAMATRIX_RUNTIME_VERSION_1_0;
    if (!SalamanderGeneral->QueryService(&query, &serviceResult) ||
        serviceResult.Interface == NULL)
        return NULL;
    Salamatrix::Runtime::IRuntimeService* runtime =
        static_cast<Salamatrix::Runtime::IRuntimeService*>(
            serviceResult.Interface);
    if (runtime->FindAdapter("PowerShell", 0) != NULL)
        return NULL;
    if (!PowerShellRegistration.Register(runtime, &PowerShellRuntime))
        return NULL;
    return &PluginInterface;
}

void WINAPI CPluginInterface::About(HWND parent)
{
    char text[512];
    _snprintf_s(text, _countof(text), _TRUNCATE,
        "PowerShell Runtime provider\\n\\nRegistered: %s\\nInterpreter available: %s",
        PowerShellRegistration.IsRegistered() ? "yes" : "no",
        PowerShellRuntime.IsAvailable() ? "yes" : "no");
    if (SalamanderGeneral != NULL)
        SalamanderGeneral->SalMessageBox(parent, text, "PowerShell Runtime",
                                         MB_OK | MB_ICONINFORMATION);
}

BOOL WINAPI CPluginInterface::Release(HWND, BOOL)
{
    PowerShellRegistration.Unregister();
    SalamanderGeneral = NULL;
    return TRUE;
}

void WINAPI CPluginInterface::LoadConfiguration(HWND, HKEY, CSalamanderRegistryAbstract*) {}
void WINAPI CPluginInterface::SaveConfiguration(HWND, HKEY, CSalamanderRegistryAbstract*) {}
void WINAPI CPluginInterface::Connect(HWND, CSalamanderConnectAbstract*) {}
void WINAPI CPluginInterface::Event(int, DWORD) {}


