/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Add the one-file basic metadata broker command. | Authenticate and validate a bounded attributes and timestamps request. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Reconstruct the pipe path from a canonical GUID. | Keep elevated file opens within the local broker pipe namespace and address CodeQL findings. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add the authenticated one-file rename command. | Reuse the protected broker launch while keeping mkdir and rename separately validated. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add the one-shot UAC broker and its diagnostic host. | Verify a protected launch, peer identity and shutdown before introducing commands. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add the authenticated one-leaf F7 pilot. | Bind consent to the verified NTFS directory and one request. | | GPT-6.1 Sol (Codex) |
*/

#ifdef SAM_UAC_F7_HOST
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
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
#include "f7pilot.h"
#ifdef SAM_UAC_F7_HOST
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uuid.lib")
#pragma comment(lib, "ntdll.lib")
#endif
#if defined(SAM_UAC_BROKER) || defined(SAM_UAC_F7_HOST)
#include "metadataleaf.h"
#endif

namespace
{
struct CloseHandleDeleter
{
    void operator()(void* h) const { CloseHandle(h); }
};
using Handle = std::unique_ptr<void, CloseHandleDeleter>;

Handle Adopt(HANDLE h) { return Handle(h != INVALID_HANDLE_VALUE ? h : nullptr); }

constexpr std::wstring_view pipePrefix = LR"(\\.\pipe\OpenSamandarin-UAC-)";
constexpr size_t guidLength = 38;
constexpr wchar_t probeImage[] = L"uacbrokerprobe.exe";
[[maybe_unused]] constexpr wchar_t appImage[] = L"salamand.exe";
constexpr wchar_t brokerImage[] = L"uacbroker.exe";
constexpr DWORD pipeAccess = FILE_READ_DATA | FILE_WRITE_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE;
constexpr DWORD processAccess = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
constexpr DWORD timeoutMs = 5000;
constexpr DWORD protocolVersion = 1;
constexpr DWORD capabilities = 0;
[[maybe_unused]] constexpr DWORD mkdirCapability = 1;
[[maybe_unused]] constexpr DWORD renameCapability = 2;
[[maybe_unused]] constexpr DWORD metadataCapability = 4;
[[maybe_unused]] constexpr DWORD appCapabilities = mkdirCapability | renameCapability | metadataCapability;

std::wstring PipeName(const GUID& id)
{
    wchar_t guid[guidLength + 1]{};
    if (!StringFromGUID2(id, guid, ARRAYSIZE(guid)))
        return {};
    return std::wstring(pipePrefix) + guid;
}

[[maybe_unused]] std::wstring ParsePipeName(std::wstring_view argument)
{
    if (argument.size() != pipePrefix.size() + guidLength || argument.substr(0, pipePrefix.size()) != pipePrefix)
        return {};
    const std::wstring guid(argument.substr(pipePrefix.size()));
    GUID id{};
    if (guid.front() != L'{' || guid.back() != L'}' || FAILED(CLSIDFromString(guid.c_str(), &id)))
        return {};
    std::wstring canonical = PipeName(id);
    if (canonical.empty() || CompareStringOrdinal(argument.data(), static_cast<int>(argument.size()),
                                                   canonical.c_str(), -1, TRUE) != CSTR_EQUAL)
        return {};
    return canonical;
}

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

DWORD Exchange(HANDLE pipe, Frame& frame, bool send, ULONGLONG deadline, bool byteStream = false)
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
                               : ReadFile(pipe, input, byteStream ? sizeof(frame) : sizeof(input), &count, &io);
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

bool Matches(const Frame& frame, Phase phase, DWORD capability = capabilities)
{
    return frame.version == protocolVersion && frame.capabilityMask == capability && frame.phase == phase;
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

#if defined(SAM_UAC_BROKER) || defined(SAM_UAC_F7_HOST)
enum class Operation : DWORD { Mkdir = 1, Rename = 2, Metadata = 3 };
struct RequestHeader
{
    ULONGLONG serial;
    FILE_ID_128 parentId;
    FILE_ID_128 sourceId;
    Operation operation;
    DWORD relativeLength;
    DWORD leafLength;
    DWORD sourceLength;
};
static_assert(sizeof(RequestHeader) == 56);

constexpr std::wstring_view volumePrefix = LR"(\\?\Volume)";
constexpr size_t volumeLength = volumePrefix.size() + 38;

Handle OpenVolumeRoot(std::wstring_view volume)
{
    if (volume.size() != volumeLength ||
        !UacDirectoryLeaf::Equal(volume.substr(0, volumePrefix.size()), volumePrefix))
        return {};
    const std::wstring guid(volume.substr(volumePrefix.size()));
    GUID id{};
    if (guid.front() != L'{' || guid.back() != L'}' || FAILED(CLSIDFromString(guid.c_str(), &id)))
        return {};
    const std::wstring name = std::wstring(volume) + L"\\";
    if (GetDriveTypeW(name.c_str()) != DRIVE_FIXED)
        return {};
    return Adopt(CreateFileW(name.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
}

DWORD OpenRequestParent(HANDLE root, std::wstring_view relative, Handle& parent, UacDirectoryLeaf::ParentIdentity& identity)
{
    if (relative.empty())
    {
        HANDLE copy = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), root, GetCurrentProcess(), &copy, 0, FALSE, DUPLICATE_SAME_ACCESS))
            return GetLastError();
        parent = Adopt(copy);
        return UacDirectoryLeaf::ReadParentIdentity(parent.get(), identity);
    }
    HANDLE raw = nullptr;
    const DWORD error = UacDirectoryLeaf::OpenParentBelowRoot(root, relative, raw, identity);
    parent = Adopt(raw);
    return error;
}

// The client reads an exact number of bytes in byte mode. The message-mode
// server rejects larger replies with ERROR_MORE_DATA.
DWORD TransferRequest(HANDLE pipe, void* buffer, DWORD length, bool send, ULONGLONG deadline)
{
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    const auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!event)
        return GetLastError();
    OVERLAPPED io{};
    io.hEvent = event.get();
    DWORD count = 0;
    const BOOL complete = send ? WriteFile(pipe, buffer, length, &count, &io)
                               : ReadFile(pipe, buffer, length, &count, &io);
    if (!complete)
    {
        const DWORD error = GetLastError();
        const DWORD finished = error == ERROR_IO_PENDING ? FinishIo(pipe, io, count, deadline) : error;
        if (finished)
            return finished;
    }
    return count == length ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}
#endif

#ifdef SAM_UAC_BROKER
DWORD ReceiveRequest(HANDLE pipe, HANDLE host, DWORD hostPid, ULONGLONG deadline, DWORD& result)
{
    RequestHeader header{};
    DWORD error = TransferRequest(pipe, &header, sizeof(header), false, deadline);
    if (error)
        return error;
    const bool rename = header.operation == Operation::Rename;
    const bool mkdir = header.operation == Operation::Mkdir;
    const bool setMetadata = header.operation == Operation::Metadata;
    if (header.relativeLength > UacDirectoryLeaf::maxNativeName || !header.leafLength ||
        header.leafLength > UacDirectoryLeaf::maxComponent || header.sourceLength > UacDirectoryLeaf::maxComponent ||
        (!rename && !mkdir && !setMetadata) || rename != (header.sourceLength != 0))
        return ERROR_INVALID_DATA;
    std::wstring payload(volumeLength + header.relativeLength + header.leafLength + header.sourceLength, L'\0');
    error = TransferRequest(pipe, payload.data(), static_cast<DWORD>(payload.size() * sizeof(wchar_t)), false, deadline);
    if (error)
        return error;
    UacF7::MetadataChange metadata{};
    if (setMetadata)
    {
        error = TransferRequest(pipe, &metadata, sizeof(metadata), false, deadline);
        if (error)
            return error;
        if (!UacMetadataLeaf::IsValid(metadata))
            return ERROR_INVALID_DATA;
    }

    const std::wstring_view data(payload);
    const auto volume = data.substr(0, volumeLength);
    const auto relative = data.substr(volumeLength, header.relativeLength);
    const auto leaf = data.substr(volumeLength + header.relativeLength, header.leafLength);
    const auto source = data.substr(volumeLength + header.relativeLength + header.leafLength);
    if (!UacDirectoryLeaf::IsComponent(leaf) || (rename && !UacDirectoryLeaf::IsComponent(source)) ||
        volume.find(L'\0') != std::wstring_view::npos)
        return ERROR_INVALID_DATA;
    const auto root = OpenVolumeRoot(volume);
    if (!root)
    {
        result = ERROR_NOT_SUPPORTED;
        return ERROR_SUCCESS;
    }
    Handle parent;
    UacDirectoryLeaf::ParentIdentity actual;
    result = OpenRequestParent(root.get(), relative, parent, actual);
    if (result)
        return ERROR_SUCCESS;

    UacDirectoryLeaf::ParentIdentity expected;
    expected.file.VolumeSerialNumber = header.serial;
    expected.file.FileId = header.parentId;
    expected.volume.assign(volume);
    if (!LiveInSession(host, hostPid) || GetTickCount64() >= deadline)
        return ERROR_OPERATION_ABORTED;
    const FILE_ID_INFO sourceIdentity{header.serial, header.sourceId};
    if (rename)
    {
        result = UacRenameLeaf::RenameVerifiedFile(parent.get(), expected, sourceIdentity, source, leaf);
    }
    else if (setMetadata)
        result = UacMetadataLeaf::SetVerifiedMetadata(parent.get(), expected, sourceIdentity, leaf, metadata);
    else
        result = UacDirectoryLeaf::CreateVerifiedLeaf(parent.get(), expected, leaf);
    return ERROR_SUCCESS;
}

int RunBroker(const InstallLocation& install, const wchar_t* pipeName, DWORD hostPid)
{
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken))
        return Report(Untrusted, L"Open broker token", GetLastError());
    const auto token = Adopt(rawToken);
    if (!Elevated(token.get()))
        return Report(Untrusted, L"Broker requires elevation", ERROR_ELEVATION_REQUIRED);
    const auto host = Adopt(OpenProcess(processAccess, FALSE, hostPid));
    const bool appHost = host && install.MatchesProcess(host.get(), appImage);
    const wchar_t* hostName = appHost ? appImage : probeImage;
    const auto hostImage = install.OpenImage(hostName);
    if (!host || !hostImage || !LiveInSession(host.get(), hostPid) ||
        !install.MatchesProcess(host.get(), hostName))
        return Report(Untrusted, L"Verify host image and session", ERROR_ACCESS_DENIED);

    const auto pipe = Adopt(CreateFileW(pipeName, pipeAccess, 0, nullptr, OPEN_EXISTING,
                                        FILE_FLAG_OVERLAPPED | SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                                        nullptr));
    if (!pipe)
        return IoError(L"Open broker pipe", GetLastError());
    if (!ExpectedPipePeer(pipe.get(), host.get(), hostPid, true))
        return Report(Untrusted, L"Verify pipe server", ERROR_ACCESS_DENIED);
    const auto deadline = GetTickCount64() + timeoutMs;
    const DWORD capability = appHost ? appCapabilities : capabilities;
    Frame frame{protocolVersion, capability, Phase::Hello};
    DWORD error = Exchange(pipe.get(), frame, true, deadline);
    if (error)
        return IoError(L"Send hello", error);
    error = Exchange(pipe.get(), frame, false, deadline, appHost);
    if (error)
        return IoError(L"Read accept", error);
    if (!Matches(frame, Phase::Accept, capability))
        return Report(ProtocolError, L"Validate accept", ERROR_INVALID_DATA);
    if (!LiveInSession(host.get(), hostPid))
        return Report(Untrusted, L"Host exited", ERROR_ACCESS_DENIED);
    if (appHost)
    {
        DWORD result = ERROR_GEN_FAILURE;
        error = ReceiveRequest(pipe.get(), host.get(), hostPid, deadline, result);
        if (error)
            return IoError(L"Receive request", error);
        error = TransferRequest(pipe.get(), &result, sizeof(result), true, deadline);
        if (error)
            return IoError(L"Send request result", error);
    }
    frame = {protocolVersion, capability, Phase::Done};
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
    if (FAILED(CoCreateGuid(&id)))
        return {};
    name = PipeName(id);
    if (name.empty())
        return {};
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

int RunProbe(const InstallLocation& install, bool drop, bool badVersion
#ifdef SAM_UAC_F7_HOST
             , HWND owner, RequestHeader* request, std::wstring* payload, UacF7::MetadataChange* metadata, UacF7::Result* result
#endif
)
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
#ifdef SAM_UAC_F7_HOST
    launch.hwnd = owner;
#else
    launch.hwnd = GetConsoleWindow();
#endif
    launch.lpVerb = L"runas";
    launch.lpFile = exe.c_str();
    launch.lpParameters = args.c_str();
    launch.lpDirectory = install.Directory().c_str();
    launch.nShow = SW_HIDE;
    if (!ShellExecuteExW(&launch))
    {
        const DWORD error = GetLastError();
#ifdef SAM_UAC_F7_HOST
        result->error = error;
#endif
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
#ifdef SAM_UAC_F7_HOST
    const DWORD capability = appCapabilities;
#else
    const DWORD capability = capabilities;
#endif
    if (!Matches(frame, Phase::Hello, capability))
        return Report(ProtocolError, L"Validate hello", ERROR_INVALID_DATA);
    bool elevated = false;
    if (!InspectPipeClient(pipe.get(), elevated) || !elevated || !LiveInSession(child.get(), pid))
        return Report(Untrusted, L"Verify elevated identification token", ERROR_ACCESS_DENIED);

#ifdef SAM_UAC_F7_HOST
    DWORD operationError = ERROR_GEN_FAILURE;
#endif
    if (drop)
        pipe.reset();
    else
    {
        frame = {badVersion ? protocolVersion + 1 : protocolVersion, capability, Phase::Accept};
        error = Exchange(pipe.get(), frame, true, deadline);
        if (error)
            return IoError(L"Send accept", error);
#ifdef SAM_UAC_F7_HOST
        result->unknown = true;
        error = TransferRequest(pipe.get(), request, sizeof(*request), true, deadline);
        if (!error)
            error = TransferRequest(pipe.get(), payload->data(), static_cast<DWORD>(payload->size() * sizeof(wchar_t)), true, deadline);
        if (!error && metadata)
            error = TransferRequest(pipe.get(), metadata, sizeof(*metadata), true, deadline);
        if (error)
            return IoError(L"Send request", error);

        error = TransferRequest(pipe.get(), &operationError, sizeof(operationError), false, deadline);
        if (error)
            return IoError(L"Read request result", error);
#endif
        if (!badVersion)
        {
            error = Exchange(pipe.get(), frame, false, deadline);
            if (error)
                return IoError(L"Read done", error);
            if (!Matches(frame, Phase::Done, capability))
                return Report(ProtocolError, L"Validate done", ERROR_INVALID_DATA);
        }
    }
    DWORD code = 0;
    const DWORD expected = drop ? Disconnected : badVersion ? ProtocolError : Ok;
    if (WaitForSingleObject(child.get(), timeoutMs) != WAIT_OBJECT_0 ||
        !GetExitCodeProcess(child.get(), &code) || code != expected)
        return Report(Failed, L"Unexpected broker exit", code);
#ifdef SAM_UAC_F7_HOST
    result->error = operationError;
    result->unknown = false;
#else
    std::wprintf(L"Authenticated elevated broker exited (%lu), expected (%lu).\n", code, expected);
#endif
    return Ok;
}
#endif
}

#ifdef SAM_UAC_F7_HOST
namespace UacF7
{
PreparedLeaf::~PreparedLeaf()
{
    if (source)
        CloseHandle(source);
    if (parent)
        CloseHandle(parent);
}

bool PreparedLeaf::Prepare(const std::wstring& path, const std::wstring& renameTo)
{
    return PrepareTarget(path, renameTo, nullptr);
}

bool PreparedLeaf::PrepareMetadata(const std::wstring& path, const MetadataChange& change)
{
    return UacMetadataLeaf::IsValid(change) && PrepareTarget(path, {}, &change);
}

bool PreparedLeaf::PrepareTarget(const std::wstring& path, const std::wstring& renameTo, const MetadataChange* change)
{
    // An ordinary or portable copy must never offer the pilot.
    InstallLocation install;
    if (parent || !install.Open() || !install.MatchesProcess(GetCurrentProcess(), appImage) ||
        !install.OpenImage(appImage) || !install.OpenImage(brokerImage))
        return false;

    std::wstring_view input(path);
    if (input.substr(0, 4) == LR"(\\?\)")
        input.remove_prefix(4);
    if (input.size() < 4 || input.size() > UacDirectoryLeaf::maxNativeName ||
        !((input[0] >= L'A' && input[0] <= L'Z') || (input[0] >= L'a' && input[0] <= L'z')) ||
        input[1] != L':' || input[2] != L'\\')
        return false;
    const size_t split = input.rfind(L'\\');
    const auto finalName = input.substr(split + 1);
    const bool rename = !renameTo.empty();
    if (!UacDirectoryLeaf::IsComponent(finalName) ||
        (rename && (!UacDirectoryLeaf::IsComponent(renameTo) || UacDirectoryLeaf::Equal(finalName, renameTo))))
        return false;
    const std::wstring drive(input.substr(0, 3));
    wchar_t guid[volumeLength + 2]{};
    if (GetDriveTypeW(drive.c_str()) != DRIVE_FIXED ||
        !GetVolumeNameForVolumeMountPointW(drive.c_str(), guid, ARRAYSIZE(guid)) ||
        wcslen(guid) != volumeLength + 1 || guid[volumeLength] != L'\\')
        return false;
    const std::wstring_view volumeName(guid, volumeLength);
    const auto root = OpenVolumeRoot(volumeName);
    if (!root)
        return false;

    const auto parentPath = split == 2 ? std::wstring_view{} : input.substr(3, split - 3);
    Handle opened;
    UacDirectoryLeaf::ParentIdentity snapshot;
    if (OpenRequestParent(root.get(), parentPath, opened, snapshot) ||
        !UacDirectoryLeaf::Equal(snapshot.volume, volumeName))
        return false;
    // Verify that the DOS path presented in the panel names the same object as
    // the volume-relative walk. Reject any redirected or substituted drive.
    const std::wstring originalPath = LR"(\\?\)" + std::wstring(input.substr(0, split == 2 ? 3 : split));
    const auto original = Adopt(CreateFileW(originalPath.c_str(), FILE_READ_ATTRIBUTES,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    UacDirectoryLeaf::ParentIdentity visible;
    if (!original || UacDirectoryLeaf::ReadParentIdentity(original.get(), visible) ||
        !UacDirectoryLeaf::SameParent(snapshot, visible))
        return false;
    Handle openedSource;
    FILE_ID_INFO sourceIdentity{};
    if (rename || change)
    {
        HANDLE raw = nullptr;
        if (UacRenameLeaf::Detail::OpenSource(opened.get(), finalName, raw, 0,
                                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE))
            return false;
        openedSource = Adopt(raw);
        const std::wstring sourcePath = LR"(\\?\)" + std::wstring(input);
        const auto visibleSource = Adopt(CreateFileW(sourcePath.c_str(), FILE_READ_ATTRIBUTES,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                      nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        FILE_ID_INFO visibleIdentity{};
        if (!visibleSource || UacRenameLeaf::Detail::ReadSourceIdentity(visibleSource.get(), visibleIdentity) ||
            UacRenameLeaf::Detail::ReadSourceIdentity(openedSource.get(), sourceIdentity) ||
            !UacRenameLeaf::Detail::SameSource(visibleIdentity, sourceIdentity) ||
            sourceIdentity.VolumeSerialNumber != snapshot.file.VolumeSerialNumber)
            return false;
        if (change && UacMetadataLeaf::CheckAttributes(openedSource.get(), *change))
            return false;
        if (rename)
            sourceLeaf.assign(finalName);
        std::memcpy(sourceId, sourceIdentity.FileId.Identifier, sizeof(sourceId));
    }
    volume = snapshot.volume;
    relative.assign(parentPath);
    leaf = rename ? renameTo : std::wstring(finalName);
    serial = snapshot.file.VolumeSerialNumber;
    std::memcpy(fileId, snapshot.file.FileId.Identifier, sizeof(fileId));
    parent = opened.release();
    source = openedSource.release();
    if (change)
    {
        metadata = *change;
        isMetadata = true;
    }
    return true;
}

DWORD PreparedLeaf::ExecuteLocalMetadata() const
{
    if (!parent || !isMetadata)
        return ERROR_INVALID_PARAMETER;
    UacDirectoryLeaf::ParentIdentity expectedParent{};
    expectedParent.file.VolumeSerialNumber = serial;
    std::memcpy(expectedParent.file.FileId.Identifier, fileId, sizeof(fileId));
    expectedParent.volume = volume;
    FILE_ID_INFO expectedSource{serial, {}};
    std::memcpy(expectedSource.FileId.Identifier, sourceId, sizeof(sourceId));
    return UacMetadataLeaf::SetVerifiedMetadata(parent, expectedParent, expectedSource, leaf, metadata);
}

Result PreparedLeaf::Execute(HWND owner) const
{
    Result result;
    const ComApartment com;
    InstallLocation install;
    if (!parent || FAILED(com.result) || !install.Open() ||
        !install.MatchesProcess(GetCurrentProcess(), appImage) || !install.OpenImage(appImage))
        return result;
    const Operation operation = isMetadata ? Operation::Metadata : source ? Operation::Rename : Operation::Mkdir;
    RequestHeader header{serial, {}, {}, operation,
                         static_cast<DWORD>(relative.size()), static_cast<DWORD>(leaf.size()),
                         static_cast<DWORD>(sourceLeaf.size())};
    std::memcpy(header.parentId.Identifier, fileId, sizeof(fileId));
    std::memcpy(header.sourceId.Identifier, sourceId, sizeof(sourceId));
    std::wstring payload = volume + relative + leaf + sourceLeaf;
    MetadataChange requestMetadata = metadata;
    RunProbe(install, false, false, owner, &header, &payload, isMetadata ? &requestMetadata : nullptr, &result);
    return result;
}
}
#else
int wmain(int argc, wchar_t** argv)
{
#ifdef SAM_UAC_BROKER
    if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32))
        return Report(Failed, L"Restrict DLL lookup", GetLastError());
    if (argc != 3)
        return Untrusted;
    const std::wstring pipeName = ParsePipeName(argv[1]);
    if (pipeName.empty())
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
    return RunBroker(install, pipeName.c_str(), hostPid);
#else
    if (!install.MatchesProcess(GetCurrentProcess(), probeImage) || !install.OpenImage(probeImage))
        return Report(Untrusted, L"Verify probe image", ERROR_ACCESS_DENIED);
    return RunProbe(install, drop, badVersion);
#endif
}
#endif
