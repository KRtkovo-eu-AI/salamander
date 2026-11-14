/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Add a one-shot UAC bootstrap experiment. | Test peer binding and token identification before introducing file commands. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Report early helper failures to the host. | Distinguish cross-account startup errors from a pipe timeout. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Exercise both directions of pipe loss. | Verify the one-shot lifecycle before adding file commands. | | GPT-6.1 Sol (Codex) |
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <sddl.h>
#include <shellapi.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <vector>

namespace
{
constexpr DWORD timeoutMs = 5000;
constexpr DWORD protocolV1 = 0x53414c01;
constexpr DWORD acknowledge = 0x53414c02;
constexpr wchar_t pipePrefix[] = LR"(\\.\pipe\Samandarin-UacLab-)";

struct Handle
{
    HANDLE value;
    explicit Handle(HANDLE h = nullptr) : value(h) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    ~Handle()
    {
        if (valid())
            CloseHandle(value);
    }
    bool valid() const { return value && value != INVALID_HANDLE_VALUE; }
};

int Fail(const wchar_t* step, DWORD error = GetLastError())
{
    std::fwprintf(stderr, L"%ls failed (%lu)\n", step, error);
    return 1;
}

bool FinishIo(HANDLE pipe, OVERLAPPED& io, DWORD& bytes)
{
    const DWORD wait = WaitForSingleObject(io.hEvent, timeoutMs);
    if (wait == WAIT_OBJECT_0)
        return GetOverlappedResult(pipe, &io, &bytes, FALSE) != FALSE;

    const DWORD error = wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
    CancelIoEx(pipe, &io);
    // Drain cancellation before releasing the OVERLAPPED and its buffer.
    GetOverlappedResult(pipe, &io, &bytes, TRUE);
    SetLastError(error);
    return false;
}

bool Connect(HANDLE pipe)
{
    Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
        return false;
    OVERLAPPED io{};
    io.hEvent = event.value;
    if (ConnectNamedPipe(pipe, &io))
        return true;
    const DWORD error = GetLastError();
    if (error == ERROR_PIPE_CONNECTED)
        return true;
    DWORD unused = 0;
    return error == ERROR_IO_PENDING && FinishIo(pipe, io, unused);
}

bool Transfer(HANDLE pipe, DWORD& message, bool write)
{
    Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event.valid())
        return false;
    OVERLAPPED io{};
    io.hEvent = event.value;
    DWORD bytes = 0;
    const BOOL done = write ? WriteFile(pipe, &message, sizeof(message), &bytes, &io)
                            : ReadFile(pipe, &message, sizeof(message), &bytes, &io);
    if (!done && (GetLastError() != ERROR_IO_PENDING || !FinishIo(pipe, io, bytes)))
        return false;
    if (bytes != sizeof(message))
    {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    return true;
}

bool SameSession(DWORD peerPid)
{
    DWORD selfSession = 0;
    DWORD peerSession = 0;
    return ProcessIdToSessionId(GetCurrentProcessId(), &selfSession) &&
           ProcessIdToSessionId(peerPid, &peerSession) &&
           selfSession == peerSession;
}

constexpr DWORD openHostFailure = 0x51000000;
constexpr DWORD openPipeFailure = 0x52000000;
constexpr DWORD failureKindMask = 0xff000000;
constexpr DWORD failureErrorMask = 0x00ffffff;

int ReportEarlyClientFailure(const wchar_t* step, DWORD kind)
{
    const DWORD error = GetLastError();
    Fail(step, error);
    return static_cast<int>(kind | (error & failureErrorMask));
}

void ReportEarlyClientExit(HANDLE child)
{
    DWORD code = 0;
    if (WaitForSingleObject(child, 0) != WAIT_OBJECT_0 || !GetExitCodeProcess(child, &code))
        return;
    switch (code & failureKindMask)
    {
    case openHostFailure:
        Fail(L"Helper: open host process", code & failureErrorMask);
        break;
    case openPipeFailure:
        Fail(L"Helper: open pipe", code & failureErrorMask);
        break;
    default:
        Fail(L"Helper exited before connecting", code);
        break;
    }
}

enum class Drop { None, Host, Helper };
constexpr int hostDropObserved = 51;
constexpr int helperDropInjected = 52;

bool PipeWasClosed(DWORD error)
{
    return error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA || error == ERROR_PIPE_NOT_CONNECTED;
}

int Client(const wchar_t* pipeName, DWORD hostPid, Drop drop)
{
    Handle host(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, hostPid));
    if (!host.valid())
        return ReportEarlyClientFailure(L"Open host process", openHostFailure);
    Handle pipe(CreateFileW(pipeName, FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE, 0,
                            nullptr, OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                            nullptr));
    if (!pipe.valid())
        return ReportEarlyClientFailure(L"Open pipe", openPipeFailure);
    ULONG serverPid = 0;
    if (!GetNamedPipeServerProcessId(pipe.value, &serverPid))
        return Fail(L"Query pipe server");
    if (serverPid != hostPid || !SameSession(hostPid) ||
        WaitForSingleObject(host.value, 0) != WAIT_TIMEOUT)
    {
        Fail(L"Verify pipe server", ERROR_ACCESS_DENIED);
        return ERROR_ACCESS_DENIED;
    }

    DWORD message = protocolV1;
    if (!Transfer(pipe.value, message, true))
        return Fail(L"Send hello");
    if (!Transfer(pipe.value, message, false))
    {
        const DWORD error = GetLastError();
        Fail(L"Receive acknowledgement", error);
        return drop == Drop::Host && PipeWasClosed(error) ? hostDropObserved : 1;
    }
    if (message != acknowledge || drop == Drop::Host)
        return Fail(L"Check acknowledgement", ERROR_INVALID_DATA);
    // In the helper-loss case, exit without returning the test receipt.
    return drop == Drop::Helper ? helperDropInjected : 0;
}

bool QueryTokenUser(HANDLE token, std::vector<BYTE>& buffer)
{
    DWORD bytes = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &bytes);
    if (!bytes)
        return false;
    buffer.resize(bytes);
    return GetTokenInformation(token, TokenUser, buffer.data(), bytes, &bytes) != FALSE;
}

bool InspectPipeClient(HANDLE pipe, PSID hostSid, bool& elevated, bool& sameUser)
{
    if (!ImpersonateNamedPipeClient(pipe))
        return false;
    HANDLE rawToken = nullptr;
    const BOOL opened = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &rawToken);
    const DWORD openError = opened ? ERROR_SUCCESS : GetLastError();
    if (!RevertToSelf())
        std::abort();
    Handle token(rawToken);
    if (!opened)
    {
        SetLastError(openError);
        return false;
    }

    SECURITY_IMPERSONATION_LEVEL level{};
    TOKEN_ELEVATION elevation{};
    DWORD bytes = 0;
    if (!GetTokenInformation(token.value, TokenImpersonationLevel, &level, sizeof(level), &bytes) ||
        !GetTokenInformation(token.value, TokenElevation, &elevation, sizeof(elevation), &bytes))
        return false;
    if (level != SecurityIdentification)
    {
        SetLastError(ERROR_ACCESS_DENIED);
        return false;
    }
    std::vector<BYTE> user;
    if (!QueryTokenUser(token.value, user))
        return false;
    elevated = elevation.TokenIsElevated != 0;
    sameUser = EqualSid(hostSid, reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid) != FALSE;
    return true;
}

int RunHost(bool elevate, bool rejectPeer, bool rejectServer = false, Drop drop = Drop::None)
{
    Handle selfToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &selfToken.value))
        return Fail(L"Open host token");
    DWORD bytes = 0;
    TOKEN_ELEVATION hostElevation{};
    if (!GetTokenInformation(selfToken.value, TokenElevation, &hostElevation, sizeof(hostElevation), &bytes))
        return Fail(L"Query host elevation");
    if (elevate && hostElevation.TokenIsElevated)
        return Fail(L"Start this test from an unelevated console", ERROR_INVALID_STATE);
    std::vector<BYTE> userBuffer;
    if (!QueryTokenUser(selfToken.value, userBuffer))
        return Fail(L"Get host SID");
    const PSID hostSid = reinterpret_cast<TOKEN_USER*>(userBuffer.data())->User.Sid;
    wchar_t* sid = nullptr;
    if (!ConvertSidToStringSidW(hostSid, &sid))
        return Fail(L"Format host SID");

    // The host creates the first and only instance. The elevated client needs
    // read/write data, read attributes and synchronize, never FILE_CREATE_PIPE_INSTANCE.
    std::wstring sddl = L"D:P(A;;0x00100083;;;BA)(A;;GA;;;";
    sddl += sid;
    sddl += L")";
    LocalFree(sid);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
        return Fail(L"Build pipe DACL");

    GUID id{};
    wchar_t guid[39]{};
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, ARRAYSIZE(guid)))
    {
        LocalFree(descriptor);
        return Fail(L"Generate pipe name", ERROR_GEN_FAILURE);
    }
    const std::wstring name = pipePrefix + std::wstring(guid);
    SECURITY_ATTRIBUTES security{sizeof(security), descriptor, FALSE};
    Handle pipe(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE |
                                 FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                                 PIPE_REJECT_REMOTE_CLIENTS, 1, 256, 256, 0, &security));
    const DWORD pipeError = pipe.valid() ? ERROR_SUCCESS : GetLastError();
    LocalFree(descriptor);
    if (!pipe.valid())
        return Fail(L"Create pipe", pipeError);

    std::wstring exe(32768, L'\0');
    const DWORD chars = GetModuleFileNameW(nullptr, &exe[0], static_cast<DWORD>(exe.size()));
    if (!chars || chars >= exe.size())
        return Fail(L"Get lab executable path");
    exe.resize(chars);
    const wchar_t* mode = L"--client ";
    if (rejectServer)
        mode = L"--reject-server-client ";
    else if (drop == Drop::Host)
        mode = L"--client-drop-host ";
    else if (drop == Drop::Helper)
        mode = L"--client-drop-helper ";
    const std::wstring args = mode + name + L" " + std::to_wstring(GetCurrentProcessId());
    Handle child;
    if (elevate)
    {
        SHELLEXECUTEINFOW launch{sizeof(launch)};
        launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
        launch.hwnd = GetConsoleWindow();
        launch.lpVerb = L"runas";
        launch.lpFile = exe.c_str();
        launch.lpParameters = args.c_str();
        launch.nShow = SW_HIDE;
        const HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (FAILED(com))
            return Fail(L"Initialize COM", static_cast<DWORD>(com));
        const BOOL started = ShellExecuteExW(&launch);
        const DWORD error = started ? ERROR_SUCCESS : GetLastError();
        CoUninitialize();
        if (!started)
            return Fail(error == ERROR_CANCELLED ? L"UAC consent cancelled" : L"Launch helper", error);
        child.value = launch.hProcess;
    }
    else
    {
        std::wstring command = L"\"" + exe + L"\" " + args;
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(exe.c_str(), &command[0], nullptr, nullptr, FALSE, 0,
                            nullptr, nullptr, &startup, &process))
            return Fail(L"Launch local helper");
        child.value = process.hProcess;
        CloseHandle(process.hThread);
    }

    if (rejectServer)
    {
        // The negative-control child expects its own PID as the server PID.
        // Check a dedicated rejection code, independent of pipe closure timing.
        DWORD exitCode = 0;
        if (!child.valid() || WaitForSingleObject(child.value, timeoutMs) != WAIT_OBJECT_0 ||
            !GetExitCodeProcess(child.value, &exitCode) || exitCode != ERROR_ACCESS_DENIED)
            return Fail(L"Check rejected server exit", ERROR_INVALID_DATA);
        std::wprintf(L"Unexpected server rejected before sending hello.\n");
        return 0;
    }

    const DWORD childPid = child.valid() ? GetProcessId(child.value) : 0;
    if (!childPid)
        return Fail(L"Get launched process identity");
    if (!Connect(pipe.value))
    {
        const DWORD error = GetLastError();
        ReportEarlyClientExit(child.value);
        return Fail(L"Connect to helper", error);
    }
    ULONG clientPid = 0;
    if (!GetNamedPipeClientProcessId(pipe.value, &clientPid))
        return Fail(L"Query pipe client");

    // Negative control: the connected process is not the expected process.
    const DWORD expectedPid = rejectPeer ? GetCurrentProcessId() : childPid;
    if (clientPid != expectedPid)
    {
        if (!rejectPeer || clientPid != childPid)
            return Fail(L"Verify pipe client", ERROR_ACCESS_DENIED);
        CloseHandle(pipe.value);
        pipe.value = nullptr;
        DWORD exitCode = 0;
        if (WaitForSingleObject(child.value, timeoutMs) != WAIT_OBJECT_0 ||
            !GetExitCodeProcess(child.value, &exitCode) || exitCode != 1)
            return Fail(L"Check rejected client exit", ERROR_INVALID_DATA);
        std::wprintf(L"Unexpected peer rejected before reading its hello.\n");
        return 0;
    }
    if (rejectPeer || !SameSession(childPid) || WaitForSingleObject(child.value, 0) != WAIT_TIMEOUT)
        return Fail(L"Verify live pipe client", ERROR_ACCESS_DENIED);
    DWORD message = 0;
    if (!Transfer(pipe.value, message, false))
        return Fail(L"Read helper hello");
    if (message != protocolV1)
        return Fail(L"Verify protocol", ERROR_INVALID_DATA);

    bool elevated = false;
    bool sameUser = false;
    if (!InspectPipeClient(pipe.value, hostSid, elevated, sameUser))
        return Fail(L"Identify helper token");
    if (elevate && !elevated)
        return Fail(L"Verify helper elevation", ERROR_ACCESS_DENIED);

    if (drop == Drop::Host)
    {
        // The authenticated client is waiting for its acknowledgement.
        CloseHandle(pipe.value);
        pipe.value = nullptr;
        DWORD code = 0;
        if (WaitForSingleObject(child.value, timeoutMs) != WAIT_OBJECT_0 ||
            !GetExitCodeProcess(child.value, &code) || code != hostDropObserved)
            return Fail(L"Check client exit after host pipe loss", ERROR_INVALID_DATA);
        std::wprintf(L"Host pipe closed after verified hello. Client observed loss and exited. Elevated: %ls.\n",
                     elevated ? L"yes" : L"no");
        return 0;
    }

    message = acknowledge;
    if (!Transfer(pipe.value, message, true))
        return Fail(L"Acknowledge helper");
    if (drop == Drop::Helper)
    {
        if (Transfer(pipe.value, message, false))
            return Fail(L"Unexpected receipt", ERROR_INVALID_DATA);
        const DWORD error = GetLastError();
        DWORD code = 0;
        if (!PipeWasClosed(error) || WaitForSingleObject(child.value, timeoutMs) != WAIT_OBJECT_0 ||
            !GetExitCodeProcess(child.value, &code) || code != helperDropInjected)
            return Fail(L"Check helper pipe loss", error);
        std::wprintf(L"Helper exited before receipt. Host observed pipe error %lu. Elevated: %ls.\n",
                     error, elevated ? L"yes" : L"no");
        return 0;
    }
    DWORD exitCode = 0;
    if (WaitForSingleObject(child.value, timeoutMs) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child.value, &exitCode) || exitCode != 0)
        return Fail(L"Check helper exit", ERROR_INVALID_DATA);
    std::wprintf(L"Handshake passed. Client token elevated: %ls. Same user: %ls.\n",
                 elevated ? L"yes" : L"no", sameUser ? L"yes" : L"no");
    return 0;
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && !std::wcscmp(argv[1], L"--local"))
        return RunHost(false, false);
    if (argc == 2 && !std::wcscmp(argv[1], L"--reject-peer"))
        return RunHost(false, true);
    if (argc == 2 && !std::wcscmp(argv[1], L"--runas"))
        return RunHost(true, false);
    if (argc == 2 && !std::wcscmp(argv[1], L"--reject-server"))
        return RunHost(false, false, true);
    if (argc == 2 && !std::wcscmp(argv[1], L"--drop-host"))
        return RunHost(false, false, false, Drop::Host);
    if (argc == 2 && !std::wcscmp(argv[1], L"--drop-helper"))
        return RunHost(false, false, false, Drop::Helper);
    if (argc == 2 && !std::wcscmp(argv[1], L"--runas-drop-host"))
        return RunHost(true, false, false, Drop::Host);
    if (argc == 2 && !std::wcscmp(argv[1], L"--runas-drop-helper"))
        return RunHost(true, false, false, Drop::Helper);

    const bool rejectServer = argc == 4 && !std::wcscmp(argv[1], L"--reject-server-client");
    Drop drop = Drop::None;
    if (argc == 4 && !std::wcscmp(argv[1], L"--client-drop-host"))
        drop = Drop::Host;
    else if (argc == 4 && !std::wcscmp(argv[1], L"--client-drop-helper"))
        drop = Drop::Helper;
    if (argc == 4 && (rejectServer || drop != Drop::None || !std::wcscmp(argv[1], L"--client")) &&
        !std::wcsncmp(argv[2], pipePrefix, ARRAYSIZE(pipePrefix) - 1) &&
        std::wcslen(argv[2]) == ARRAYSIZE(pipePrefix) - 1 + 38 &&
        std::wcspbrk(argv[2] + ARRAYSIZE(pipePrefix) - 1, L"\\/:") == nullptr)
    {
        wchar_t* end = nullptr;
        const unsigned long pid = std::wcstoul(argv[3], &end, 10);
        if (pid && !*end)
            return Client(argv[2], rejectServer ? GetCurrentProcessId() : pid, drop);
    }
    std::fwprintf(stderr, L"Usage: uac-bootstrap-lab --local | --reject-peer | --reject-server | --runas\n"
                          L"       --drop-host | --drop-helper | --runas-drop-host | --runas-drop-helper\n");
    return 2;
}
