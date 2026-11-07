/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Add the one-shot UAC broker and its diagnostic host. | Verify a protected launch, peer identity and shutdown before introducing commands. | | GPT-6.1 Sol (Codex) |
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <shellapi.h>
#include <shlobj.h>

#include <cstdio>
#include <cwchar>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace
{
struct CloseHandleDeleter
{
    void operator()(void* h) const { CloseHandle(h); }
};
using Handle = std::unique_ptr<void, CloseHandleDeleter>;

Handle Adopt(HANDLE h) { return Handle(h != INVALID_HANDLE_VALUE ? h : nullptr); }

constexpr wchar_t pipePrefix[] = LR"(\\.\pipe\OpenSamandarin-UAC-)";
constexpr wchar_t probeImage[] = L"uacbrokerprobe.exe";
constexpr wchar_t brokerImage[] = L"uacbroker.exe";
constexpr DWORD pipeAccess = FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
constexpr DWORD processAccess = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
constexpr DWORD timeoutMs = 5000;
constexpr DWORD protocolVersion = 1;
constexpr DWORD capabilities = 0;

enum class Phase : DWORD { Hello = 1, Accept = 2, Done = 3 };
struct Frame
{
    DWORD version;
    DWORD capabilityMask;
    Phase phase;
};
static_assert(sizeof(Frame) == 12);

enum ExitCode { Ok, Failed, Cancelled, Untrusted, ProtocolError, Disconnected, TimedOut };

int Report(ExitCode code, const wchar_t* stage, DWORD error)
{
    std::fwprintf(stderr, L"%ls (%lu)\n", stage, error);
    return code;
}

int IoError(const wchar_t* stage, DWORD error)
{
    if (error == ERROR_BROKEN_PIPE || error == ERROR_NO_DATA || error == ERROR_PIPE_NOT_CONNECTED)
        return Report(Disconnected, stage, error);
    return Report(error == ERROR_TIMEOUT ? TimedOut : Failed, stage, error);
}

DWORD FinishIo(HANDLE pipe, OVERLAPPED& io, DWORD& count, ULONGLONG deadline)
{
    const auto now = GetTickCount64();
    const DWORD remaining = now < deadline ? static_cast<DWORD>(deadline - now) : 0;
    const DWORD wait = WaitForSingleObject(io.hEvent, remaining);
    if (wait == WAIT_OBJECT_0)
        return GetOverlappedResult(pipe, &io, &count, FALSE) ? ERROR_SUCCESS : GetLastError();
    const DWORD error = wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : GetLastError();
    CancelIoEx(pipe, &io);
    // Drain the overlapped operation before its stack storage is released.
    GetOverlappedResult(pipe, &io, &count, TRUE);
    return error;
}

[[maybe_unused]] DWORD Connect(HANDLE pipe, ULONGLONG deadline)
{
    const auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event)
        return GetLastError();
    OVERLAPPED io{};
    io.hEvent = event.get();
    if (ConnectNamedPipe(pipe, &io))
        return ERROR_SUCCESS;
    const DWORD error = GetLastError();
    if (error == ERROR_PIPE_CONNECTED)
        return ERROR_SUCCESS;
    DWORD unused = 0;
    return error == ERROR_IO_PENDING ? FinishIo(pipe, io, unused, deadline) : error;
}

DWORD Exchange(HANDLE pipe, Frame& frame, bool send, ULONGLONG deadline)
{
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    const auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event)
        return GetLastError();
    OVERLAPPED io{};
    io.hEvent = event.get();
    DWORD count = 0;
    BYTE input[sizeof(Frame) + 1]{};
    const BOOL complete = send ? WriteFile(pipe, &frame, sizeof(frame), &count, &io)
                               : ReadFile(pipe, input, sizeof(input), &count, &io);
    if (!complete)
    {
        const DWORD error = GetLastError();
        const DWORD finished = error == ERROR_IO_PENDING ? FinishIo(pipe, io, count, deadline) : error;
        if (finished != ERROR_SUCCESS)
            return finished;
    }
    if (count != sizeof(frame))
        return ERROR_INVALID_DATA;
    if (!send)
        std::memcpy(&frame, input, sizeof(frame));
    return ERROR_SUCCESS;
}

bool Matches(const Frame& frame, Phase phase)
{
    return frame.version == protocolVersion && frame.capabilityMask == capabilities && frame.phase == phase;
}

bool LiveInSession(HANDLE peer, DWORD pid)
{
    DWORD selfSession = 0, peerSession = 0;
    return WaitForSingleObject(peer, 0) == WAIT_TIMEOUT &&
           ProcessIdToSessionId(GetCurrentProcessId(), &selfSession) &&
           ProcessIdToSessionId(pid, &peerSession) && selfSession == peerSession;
}

bool ExpectedPipePeer(HANDLE pipe, HANDLE process, DWORD pid, bool server)
{
    ULONG actual = 0;
    const BOOL queried = server ? GetNamedPipeServerProcessId(pipe, &actual)
                                : GetNamedPipeClientProcessId(pipe, &actual);
    return queried && actual == pid && GetProcessId(process) == pid && LiveInSession(process, pid);
}

struct LocalFreeDeleter
{
    void operator()(void* p) const { LocalFree(p); }
};

bool TrustedSid(PSID sid, PSID installer)
{
    return IsWellKnownSid(sid, WinLocalSystemSid) || IsWellKnownSid(sid, WinBuiltinAdministratorsSid) ||
           EqualSid(sid, installer);
}

bool ProtectedObject(HANDLE handle, bool directory)
{
    FILE_ATTRIBUTE_TAG_INFO info{};
    if (!GetFileInformationByHandleEx(handle, FileAttributeTagInfo, &info, sizeof(info)) ||
        (info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        ((info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) != directory)
        return false;

    PSID owner = nullptr;
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR raw = nullptr;
    const DWORD status = GetSecurityInfo(handle, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                         &owner, nullptr, &dacl, nullptr, &raw);
    const std::unique_ptr<void, LocalFreeDeleter> security(raw);
    PSID installerRaw = nullptr;
    if (status != ERROR_SUCCESS || !owner || !dacl ||
        !ConvertStringSidToSidW(L"S-1-5-80-956008885-3418522649-1831038044-1853292631-2271478464", &installerRaw))
        return false;
    const std::unique_ptr<void, LocalFreeDeleter> installer(installerRaw);
    if (!TrustedSid(owner, installer.get()))
        return false;

    // Inherit-only ACEs describe descendants, which we check independently.
    constexpr DWORD change = GENERIC_ALL | GENERIC_WRITE | WRITE_DAC | WRITE_OWNER | DELETE |
                             FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA |
                             FILE_WRITE_ATTRIBUTES | FILE_DELETE_CHILD;
    for (DWORD i = 0; i < dacl->AceCount; ++i)
    {
        void* entry = nullptr;
        if (!GetAce(dacl, i, &entry))
            return false;
        const auto* header = static_cast<const ACE_HEADER*>(entry);
        if (header->AceFlags & INHERIT_ONLY_ACE)
            continue;
        if (header->AceType == ACCESS_DENIED_ACE_TYPE)
            continue;
        if (header->AceType != ACCESS_ALLOWED_ACE_TYPE)
            return false;
        auto* allow = static_cast<ACCESS_ALLOWED_ACE*>(entry);
        if ((allow->Mask & change) && !TrustedSid(&allow->SidStart, installer.get()))
            return false;
    }
    return true;
}

class InstallLocation
{
public:
    bool Open()
    {
        PWSTR folder = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT_PATH, nullptr, &folder)))
            return false;
        const std::wstring programFiles(folder);
        CoTaskMemFree(folder);
        if (programFiles.size() < 3 || programFiles[1] != L':' || programFiles[2] != L'\\' ||
            GetDriveTypeW(programFiles.substr(0, 3).c_str()) != DRIVE_FIXED)
            return false;
        root = OpenDirectory(programFiles);
        if (!root)
            return false;
        path = programFiles + L"\\Open Samandarin UAC";
        directory = OpenDirectory(path);
        return directory != nullptr;
    }

    Handle OpenImage(const wchar_t* image) const
    {
        auto file = Adopt(CreateFileW(ImagePath(image).c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES | READ_CONTROL,
                                      FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        return file && ProtectedObject(file.get(), false) ? std::move(file) : Handle{};
    }

    std::wstring ImagePath(const wchar_t* image) const { return path + L"\\" + image; }

    bool MatchesProcess(HANDLE process, const wchar_t* image) const
    {
        std::wstring name(32768, L'\0');
        DWORD length = static_cast<DWORD>(name.size());
        if (!QueryFullProcessImageNameW(process, 0, name.data(), &length))
            return false;
        return _wcsicmp(name.c_str(), ImagePath(image).c_str()) == 0;
    }

    const std::wstring& Directory() const { return path; }

private:
    static Handle OpenDirectory(const std::wstring& name)
    {
        auto dir = Adopt(CreateFileW(name.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | READ_CONTROL,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                     FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr));
        return dir && ProtectedObject(dir.get(), true) ? std::move(dir) : Handle{};
    }

    std::wstring path;
    Handle root, directory;
};

bool Elevated(HANDLE token)
{
    TOKEN_ELEVATION elevation{};
    DWORD bytes = 0;
    return GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes) &&
           elevation.TokenIsElevated;
}

struct ComApartment
{
    const HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ~ComApartment() { if (SUCCEEDED(result)) CoUninitialize(); }
};

#ifdef SAM_UAC_BROKER
int RunBroker(const InstallLocation& install, const wchar_t* pipeName, DWORD hostPid)
{
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken))
        return Report(Untrusted, L"Open broker token", GetLastError());
    const auto token = Adopt(rawToken);
    if (!Elevated(token.get()))
        return Report(Untrusted, L"Broker requires elevation", ERROR_ELEVATION_REQUIRED);
    const auto host = Adopt(OpenProcess(processAccess, FALSE, hostPid));
    const auto hostImage = install.OpenImage(probeImage);
    if (!host || !hostImage || !LiveInSession(host.get(), hostPid) ||
        !install.MatchesProcess(host.get(), probeImage))
        return Report(Untrusted, L"Verify host image and session", ERROR_ACCESS_DENIED);

    const auto pipe = Adopt(CreateFileW(pipeName, pipeAccess, 0, nullptr, OPEN_EXISTING,
                                        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                                        nullptr));
    if (!pipe)
        return IoError(L"Open broker pipe", GetLastError());
    if (!ExpectedPipePeer(pipe.get(), host.get(), hostPid, true))
        return Report(Untrusted, L"Verify pipe server", ERROR_ACCESS_DENIED);
    const auto deadline = GetTickCount64() + timeoutMs;
    Frame frame{protocolVersion, capabilities, Phase::Hello};
    DWORD error = Exchange(pipe.get(), frame, true, deadline);
    if (error)
        return IoError(L"Send hello", error);
    error = Exchange(pipe.get(), frame, false, deadline);
    if (error)
        return IoError(L"Read accept", error);
    if (!Matches(frame, Phase::Accept))
        return Report(ProtocolError, L"Validate accept", ERROR_INVALID_DATA);
    if (!LiveInSession(host.get(), hostPid))
        return Report(Untrusted, L"Host exited", ERROR_ACCESS_DENIED);
    frame = {protocolVersion, capabilities, Phase::Done};
    error = Exchange(pipe.get(), frame, true, deadline);
    return error ? IoError(L"Send done", error) : Ok;
}
#else
Handle CreateEndpoint(std::wstring& name)
{
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken))
        return {};
    const auto token = Adopt(rawToken);
    DWORD bytes = 0;
    GetTokenInformation(token.get(), TokenUser, nullptr, 0, &bytes);
    if (!bytes)
        return {};
    std::vector<BYTE> user(bytes);
    if (!GetTokenInformation(token.get(), TokenUser, user.data(), bytes, &bytes))
        return {};
    PWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(user.data())->User.Sid, &sid))
        return {};
    wchar_t access[16]{};
    swprintf_s(access, L"0x%08lX", pipeAccess);
    std::wstring sddl = std::wstring(L"D:P(A;;") + access + L";;;BA)(A;;GA;;;" + sid + L")";
    LocalFree(sid);
    PSECURITY_DESCRIPTOR raw = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &raw, nullptr))
        return {};
    const std::unique_ptr<void, LocalFreeDeleter> descriptor(raw);

    GUID id{};
    wchar_t guid[39]{};
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, ARRAYSIZE(guid)))
        return {};
    name = pipePrefix + std::wstring(guid);
    SECURITY_ATTRIBUTES security{sizeof(security), descriptor.get(), FALSE};
    return Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE |
                                  FILE_FLAG_OVERLAPPED, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                                  PIPE_REJECT_REMOTE_CLIENTS, 1, 256, 256, 0, &security));
}

bool InspectPipeClient(HANDLE pipe, bool& elevated)
{
    if (!ImpersonateNamedPipeClient(pipe))
        return false;
    HANDLE rawToken = nullptr;
    const BOOL opened = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &rawToken);
    if (!RevertToSelf())
        std::abort();
    const auto token = Adopt(rawToken);
    SECURITY_IMPERSONATION_LEVEL level{};
    DWORD bytes = 0;
    TOKEN_ELEVATION elevation{};
    if (!opened || !GetTokenInformation(token.get(), TokenImpersonationLevel, &level, sizeof(level), &bytes) ||
        level != SecurityIdentification ||
        !GetTokenInformation(token.get(), TokenElevation, &elevation, sizeof(elevation), &bytes))
        return false;
    elevated = elevation.TokenIsElevated != 0;
    return true;
}

int RunProbe(const InstallLocation& install, bool drop, bool badVersion)
{
    auto pipeName = std::wstring();
    auto pipe = CreateEndpoint(pipeName);
    if (!pipe)
        return Report(Failed, L"Create local pipe", GetLastError());
    const auto brokerImageLock = install.OpenImage(brokerImage);
    if (!brokerImageLock)
        return Report(Untrusted, L"Verify installed broker", ERROR_ACCESS_DENIED);
    const auto exe = install.ImagePath(brokerImage);
    const auto args = pipeName + L" " + std::to_wstring(GetCurrentProcessId());
    SHELLEXECUTEINFOW launch{sizeof(launch)};
    launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    launch.hwnd = GetConsoleWindow();
    launch.lpVerb = L"runas";
    launch.lpFile = exe.c_str();
    launch.lpParameters = args.c_str();
    launch.lpDirectory = install.Directory().c_str();
    launch.nShow = SW_HIDE;
    if (!ShellExecuteExW(&launch))
    {
        const DWORD error = GetLastError();
        return Report(error == ERROR_CANCELLED ? Cancelled : Failed, L"Launch broker", error);
    }
    const auto child = Adopt(launch.hProcess);
    const DWORD pid = child ? GetProcessId(child.get()) : 0;
    if (!pid)
        return Report(Untrusted, L"Identify launched broker", ERROR_ACCESS_DENIED);
    const auto deadline = GetTickCount64() + timeoutMs;
    DWORD error = Connect(pipe.get(), deadline);
    if (error)
    {
        DWORD code = 0;
        if (WaitForSingleObject(child.get(), 0) == WAIT_OBJECT_0 && GetExitCodeProcess(child.get(), &code))
            std::fwprintf(stderr, L"Broker exited before connection (%lu)\n", code);
        return IoError(L"Connect broker", error);
    }
    if (!ExpectedPipePeer(pipe.get(), child.get(), pid, false))
        return Report(Untrusted, L"Verify pipe client process", ERROR_ACCESS_DENIED);
    Frame frame{};
    error = Exchange(pipe.get(), frame, false, deadline);
    if (error)
        return IoError(L"Read hello", error);
    if (!Matches(frame, Phase::Hello))
        return Report(ProtocolError, L"Validate hello", ERROR_INVALID_DATA);
    bool elevated = false;
    if (!InspectPipeClient(pipe.get(), elevated) || !elevated || !LiveInSession(child.get(), pid))
        return Report(Untrusted, L"Verify elevated identification token", ERROR_ACCESS_DENIED);

    if (drop)
        pipe.reset();
    else
    {
        frame = {badVersion ? protocolVersion + 1 : protocolVersion, capabilities, Phase::Accept};
        error = Exchange(pipe.get(), frame, true, deadline);
        if (error)
            return IoError(L"Send accept", error);
        if (!badVersion)
        {
            error = Exchange(pipe.get(), frame, false, deadline);
            if (error)
                return IoError(L"Read done", error);
            if (!Matches(frame, Phase::Done))
                return Report(ProtocolError, L"Validate done", ERROR_INVALID_DATA);
        }
    }
    DWORD code = 0;
    const DWORD expected = drop ? Disconnected : badVersion ? ProtocolError : Ok;
    if (WaitForSingleObject(child.get(), timeoutMs) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child.get(), &code) || code != expected)
        return Report(Failed, L"Unexpected broker exit", code);
    std::wprintf(L"Authenticated elevated broker exited (%lu), expected (%lu).\n", code, expected);
    return Ok;
}
#endif
}

int wmain(int argc, wchar_t** argv)
{
#ifdef SAM_UAC_BROKER
    if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32))
        return Report(Failed, L"Restrict DLL lookup", GetLastError());
    if (argc != 3 || wcsncmp(argv[1], pipePrefix, ARRAYSIZE(pipePrefix) - 1) ||
        wcslen(argv[1]) != ARRAYSIZE(pipePrefix) - 1 + 38 ||
        wcspbrk(argv[1] + ARRAYSIZE(pipePrefix) - 1, L"\\/:"))
        return Untrusted;
    wchar_t* end = nullptr;
    const DWORD hostPid = wcstoul(argv[2], &end, 10);
    if (!hostPid || *end)
        return Untrusted;
#else
    const bool drop = argc == 2 && !wcscmp(argv[1], L"--drop");
    const bool badVersion = argc == 2 && !wcscmp(argv[1], L"--bad-version");
    if (argc != 1 && !drop && !badVersion)
        return Report(Failed, L"Usage: uacbrokerprobe [--drop | --bad-version]", ERROR_INVALID_PARAMETER);
#endif
    const ComApartment com;
    if (FAILED(com.result))
        return Report(Failed, L"Initialize COM", static_cast<DWORD>(com.result));
    InstallLocation install;
    if (!install.Open())
        return Report(Untrusted, L"Verify protected installation", ERROR_ACCESS_DENIED);
#ifdef SAM_UAC_BROKER
    if (!install.MatchesProcess(GetCurrentProcess(), brokerImage) || !install.OpenImage(brokerImage))
        return Report(Untrusted, L"Verify broker image", ERROR_ACCESS_DENIED);
    return RunBroker(install, argv[1], hostPid);
#else
    if (!install.MatchesProcess(GetCurrentProcess(), probeImage) || !install.OpenImage(probeImage))
        return Report(Untrusted, L"Verify probe image", ERROR_ACCESS_DENIED);
    return RunProbe(install, drop, badVersion);
#endif
}
