/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Negotiate preservation of named streams in default protected F5. | Require protected staging before writing the copied streams. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Route the main-stream-only copy over the authenticated broker. | Negotiate the new request and retain all existing copy validation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Advertise support for basic directory attributes. | Match the host and broker before offering a metadata write. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Pin directory timestamp requests before consent. | Verify the visible directory and its volume-relative identity. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Send tree cancellation without giving up the authenticated reply. | Resolve cancellation races using the broker result. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Register additional protected tree-copy destinations. | Bind each destination to a protected path and an identity-specific recovery journal. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Hold the protected tree journal exclusively. | Serialize recovery and copying under the registered target. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Bind authenticated tree copy to a registered target. | Recheck both directory identities after UAC. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Serve strict recursive inventory on a listing session. | Keep the request anchored inside the approved root. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Dispatch a same-volume move over the authenticated transfer request. | Use the verified parents and return a terminal operation result. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Authenticate and dispatch a single verified copy. | Bind the request to both directory identities and monitor transport loss. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add an authenticated rooted directory listing session. | Support repeated read-only requests under one approved root. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Route one permanent file deletion through the authenticated broker. | Execute only the verified leaf and keep recycle operations separate. | | GPT-6.1 Sol (Codex) |
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
#include "deleteleaf.h"
#include "listing.h"
#include "copyleaf.h"
#ifdef SAM_UAC_BROKER
#include "copytree.h"
#endif
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
[[maybe_unused]] constexpr DWORD copyTimeoutMs = 30000;
constexpr DWORD protocolVersion = 1;
constexpr DWORD capabilities = 0;
[[maybe_unused]] constexpr DWORD mkdirCapability = 1;
[[maybe_unused]] constexpr DWORD renameCapability = 2;
[[maybe_unused]] constexpr DWORD metadataCapability = 4;
[[maybe_unused]] constexpr DWORD deleteCapability = 8;
[[maybe_unused]] constexpr DWORD listingCapability = 16;
[[maybe_unused]] constexpr DWORD copyCapability = 32;
[[maybe_unused]] constexpr DWORD moveCapability = 64;
[[maybe_unused]] constexpr DWORD inventoryCapability = 128;
[[maybe_unused]] constexpr DWORD copyTreeCapability = 256;
[[maybe_unused]] constexpr DWORD cancelTreeCapability = 512;
[[maybe_unused]] constexpr DWORD directoryTimesCapability = 1024;
[[maybe_unused]] constexpr DWORD directoryAttrsCapability = 2048;
[[maybe_unused]] constexpr DWORD copyIgnoreAdsCapability = 4096;
[[maybe_unused]] constexpr DWORD copyPreserveAdsCapability = 8192;
[[maybe_unused]] constexpr DWORD appCapabilities = mkdirCapability | renameCapability | metadataCapability |
                                                 deleteCapability | listingCapability | copyCapability | moveCapability |
                                                 inventoryCapability | copyTreeCapability | cancelTreeCapability |
                                                 directoryTimesCapability | directoryAttrsCapability | copyIgnoreAdsCapability |
                                                 copyPreserveAdsCapability;

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
    HANDLE DirectoryHandle() const { return directory.get(); }

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
using UacF7::Operation;
struct RequestHeader
{
    ULONGLONG serial;
    FILE_ID_128 parentId;
    FILE_ID_128 sourceId;
    Operation operation;
    DWORD relativeLength;
    DWORD leafLength;
    DWORD sourceLength;
    ULONGLONG targetSerial;
    FILE_ID_128 targetParentId;
    DWORD targetRelativeLength;
    DWORD targetLeafLength;
    ULONGLONG copySize;
};
static_assert(sizeof(RequestHeader) == 96);
struct ListingReply { DWORD error; DWORD count; };

constexpr std::wstring_view volumePrefix = LR"(\\?\Volume)";
constexpr size_t volumeLength = volumePrefix.size() + 38;
constexpr wchar_t treeTargetName[] = L"UacTreeTarget";
constexpr wchar_t treeRegistrationName[] = L"uac-tree-target.id";
constexpr unsigned treeTargetSlots = 8;

struct RegisteredTreeTarget
{
    UacDirectoryLeaf::ParentIdentity identity;
    std::wstring path;
    std::wstring relative;
};

std::wstring TreeSlotName(unsigned slot)
{
    return L"uac-tree-target-" + std::to_wstring(slot) + L".v1";
}

std::wstring_view TrimTreePath(std::wstring_view path)
{
    if (path.substr(0, 4) == LR"(\\?\)")
        path.remove_prefix(4);
    if (!path.empty() && path.back() == L'\\')
        path.remove_suffix(1);
    return path;
}

bool IsTreePath(std::wstring_view path)
{
    if (path.size() < 4 || path.size() > UacDirectoryLeaf::maxNativeName ||
        !((path[0] >= L'A' && path[0] <= L'Z') || (path[0] >= L'a' && path[0] <= L'z')) ||
        path[1] != L':' || path[2] != L'\\')
        return false;
    path.remove_prefix(3);
    for (unsigned depth = 0; depth < 32; ++depth)
    {
        const auto split = path.find(L'\\');
        if (!UacDirectoryLeaf::IsComponent(path.substr(0, split)))
            return false;
        if (split == std::wstring_view::npos)
            return true;
        path.remove_prefix(split + 1);
    }
    return false;
}

bool ReadTreeVolume(std::wstring_view path, std::wstring& volume)
{
    const std::wstring drive(path.substr(0, 3));
    wchar_t name[volumeLength + 2]{};
    if (GetDriveTypeW(drive.c_str()) != DRIVE_FIXED ||
        !GetVolumeNameForVolumeMountPointW(drive.c_str(), name, ARRAYSIZE(name)) ||
        wcslen(name) != volumeLength + 1 || name[volumeLength] != L'\\')
        return false;
    volume.assign(name, volumeLength);
    return true;
}

bool ReadTreeRegistration(const InstallLocation& install, UacDirectoryLeaf::ParentIdentity& registered,
                          std::wstring& relative)
{
    HANDLE raw = nullptr;
    if (UacRenameLeaf::Detail::OpenSource(install.DirectoryHandle(), treeRegistrationName, raw,
                                         FILE_READ_DATA | READ_CONTROL, FILE_SHARE_READ))
        return false;
    const auto record = Adopt(raw);
    LARGE_INTEGER size{};
    DWORD bytes = 0;
    if (!ProtectedObject(record.get(), false) || !GetFileSizeEx(record.get(), &size) ||
        size.QuadPart != static_cast<LONGLONG>(sizeof(FILE_ID_INFO)) ||
        !ReadFile(record.get(), &registered.file, sizeof(registered.file), &bytes, nullptr) || bytes != sizeof(registered.file))
        return false;

    UacDirectoryLeaf::ParentIdentity location;
    const auto& directory = install.Directory();
    wchar_t driveVolume[volumeLength + 2]{};
    if (UacDirectoryLeaf::ReadParentIdentity(install.DirectoryHandle(), location) ||
        location.file.VolumeSerialNumber != registered.file.VolumeSerialNumber ||
        !GetVolumeNameForVolumeMountPointW(directory.substr(0, 3).c_str(), driveVolume, ARRAYSIZE(driveVolume)) ||
        wcslen(driveVolume) != volumeLength + 1 || driveVolume[volumeLength] != L'\\' ||
        !UacDirectoryLeaf::Equal(std::wstring_view(driveVolume, volumeLength), location.volume))
        return false;
    registered.volume = std::move(location.volume);
    relative = directory.substr(3) + L"\\" + treeTargetName;
    return true;
}

bool ReadTreeSlot(const InstallLocation& install, unsigned slot, RegisteredTreeTarget& registered)
{
    HANDLE raw = nullptr;
    if (UacRenameLeaf::Detail::OpenSource(install.DirectoryHandle(), TreeSlotName(slot), raw,
                                         FILE_READ_DATA | READ_CONTROL, FILE_SHARE_READ))
        return false;
    const auto record = Adopt(raw);
    FILE_ID_INFO file{};
    LARGE_INTEGER size{};
    DWORD bytes = 0;
    constexpr LONGLONG headerSize = sizeof(file);
    if (!ProtectedObject(record.get(), false) || !GetFileSizeEx(record.get(), &size) ||
        size.QuadPart < headerSize + static_cast<LONGLONG>((volumeLength + 4) * sizeof(wchar_t)) ||
        size.QuadPart > headerSize + static_cast<LONGLONG>((volumeLength + UacDirectoryLeaf::maxNativeName) * sizeof(wchar_t)) ||
        (size.QuadPart - headerSize) % sizeof(wchar_t) ||
        !ReadFile(record.get(), &file, sizeof(file), &bytes, nullptr) || bytes != sizeof(file))
        return false;
    const auto textBytes = static_cast<DWORD>(size.QuadPart - headerSize);
    std::wstring text(textBytes / sizeof(wchar_t), L'\0');
    if (!ReadFile(record.get(), text.data(), textBytes, &bytes, nullptr) || bytes != textBytes)
        return false;
    const auto path = std::wstring_view(text).substr(volumeLength);
    std::wstring volume;
    if (!IsTreePath(path) || !ReadTreeVolume(path, volume) ||
        !UacDirectoryLeaf::Equal(volume, std::wstring_view(text).substr(0, volumeLength)))
        return false;
    registered = {{file, std::move(volume)}, std::wstring(path), std::wstring(path.substr(3))};
    return true;
}

// The host matches the entered DOS path. The broker matches the authenticated
// volume-relative request against the same administrator-owned registration.
bool FindTreeTarget(const InstallLocation& install, std::wstring_view path, std::wstring_view volume,
                    RegisteredTreeTarget& registered)
{
    const auto matches = [&](const RegisteredTreeTarget& candidate) {
        return volume.empty() ? UacDirectoryLeaf::Equal(path, candidate.path)
                              : path == candidate.relative && UacDirectoryLeaf::Equal(volume, candidate.identity.volume);
    };
    RegisteredTreeTarget candidate;
    candidate.path = install.ImagePath(treeTargetName);
    if (ReadTreeRegistration(install, candidate.identity, candidate.relative) && matches(candidate))
    {
        registered = std::move(candidate);
        return true;
    }
    for (unsigned slot = 1; slot <= treeTargetSlots; ++slot)
        if (ReadTreeSlot(install, slot, candidate) && matches(candidate))
        {
            registered = std::move(candidate);
            return true;
        }
    return false;
}

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
DWORD TransferRequest(HANDLE pipe, void* buffer, DWORD length, bool send, ULONGLONG deadline, HANDLE cancel = nullptr)
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
        if (error != ERROR_IO_PENDING)
            return error;
        if (cancel)
        {
            const HANDLE events[] = {io.hEvent, cancel};
            const auto now = GetTickCount64();
            const DWORD remaining = now < deadline ? static_cast<DWORD>(deadline - now) : 0;
            if (WaitForMultipleObjects(ARRAYSIZE(events), events, FALSE, remaining) == WAIT_OBJECT_0 + 1)
            {
                BYTE signal = UacRenameLeaf::Detail::treeCancelSignal;
                // Keep the read pending. A reply can confirm that publication won the race.
                TransferRequest(pipe, &signal, sizeof(signal), true, deadline);
            }
        }
        const DWORD finished = FinishIo(pipe, io, count, deadline);
        if (finished)
            return finished;
    }
    return count == length ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}
#endif

#ifdef SAM_UAC_BROKER
DWORD OpenProtectedTree(HANDLE volume, std::wstring_view relative, std::vector<Handle>& pinned,
                        UacDirectoryLeaf::ParentIdentity& identity)
{
    HANDLE parent = volume;
    for (;;)
    {
        const auto split = relative.find(L'\\');
        const auto component = relative.substr(0, split);
        if (!UacDirectoryLeaf::IsComponent(component) || pinned.size() == 32)
            return ERROR_INVALID_NAME;
        HANDLE raw = nullptr;
        const DWORD error = UacDirectoryLeaf::Detail::NativeDirectory(parent, component, FILE_OPEN, raw,
            READ_CONTROL, nullptr, FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (error)
            return error;
        auto directory = Adopt(raw);
        if (!ProtectedObject(directory.get(), true))
            return ERROR_ACCESS_DENIED;
        parent = directory.get();
        pinned.push_back(std::move(directory));
        if (split == std::wstring_view::npos)
            return UacDirectoryLeaf::ReadParentIdentity(parent, identity);
        relative.remove_prefix(split + 1);
    }
}

std::wstring TreeJournalName(const InstallLocation& install, const UacDirectoryLeaf::ParentIdentity& target)
{
    UacDirectoryLeaf::ParentIdentity legacy;
    std::wstring legacyRelative;
    if (ReadTreeRegistration(install, legacy, legacyRelative) && UacDirectoryLeaf::SameParent(legacy, target))
        return L"uac-tree-recovery.dat";
    constexpr wchar_t hex[] = L"0123456789abcdef";
    std::wstring name = L"uac-tree-recovery-" + target.volume.substr(volumePrefix.size());
    // Make the journal key invariant to the volume GUID's case, even in a
    // case-sensitive installation directory.
    for (auto& ch : name)
        if (ch >= L'A' && ch <= L'F')
            ch += L'a' - L'A';
    name += L'-';
    for (int shift = 60; shift >= 0; shift -= 4)
        name += hex[(target.file.VolumeSerialNumber >> shift) & 15];
    name += L'-';
    for (const BYTE value : target.file.FileId.Identifier)
    {
        name += hex[value >> 4];
        name += hex[value & 15];
    }
    return name + L".dat";
}

DWORD OpenTreeJournal(const InstallLocation& install, const UacDirectoryLeaf::ParentIdentity& target, Handle& journal)
{
    PSECURITY_DESCRIPTOR rawSecurity = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"O:BAD:P(A;;FA;;;BA)(A;;FA;;;SY)",
                                                               SDDL_REVISION_1, &rawSecurity, nullptr))
        return GetLastError();
    const std::unique_ptr<void, LocalFreeDeleter> security(rawSecurity);
    HANDLE raw = nullptr;
    const DWORD error = UacRenameLeaf::Detail::OpenFile(install.DirectoryHandle(), TreeJournalName(install, target), raw,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE, 0, FILE_OPEN_IF, 0, security.get());
    if (error)
        return error;
    journal = Adopt(raw);
    FILE_ID_INFO identity{};
    if (!ProtectedObject(journal.get(), false))
        return ERROR_ACCESS_DENIED;
    return UacRenameLeaf::Detail::ReadSourceIdentity(journal.get(), identity);
}

DWORD WriteTreeRegistration(const InstallLocation& install, std::wstring_view name, FILE_ID_INFO& file,
                            std::wstring_view text = {})
{
    PSECURITY_DESCRIPTOR rawSecurity = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"O:BAD:P(A;;FA;;;BA)(A;;FA;;;SY)(A;;FR;;;BU)",
                                                               SDDL_REVISION_1, &rawSecurity, nullptr))
        return GetLastError();
    const std::unique_ptr<void, LocalFreeDeleter> security(rawSecurity);
    HANDLE raw = nullptr;
    const DWORD error = UacRenameLeaf::Detail::OpenFile(install.DirectoryHandle(), name, raw, FILE_WRITE_DATA | DELETE,
                                                        0, FILE_CREATE, FILE_DELETE_ON_CLOSE, security.get());
    if (error)
        return error;
    const auto record = Adopt(raw);
    DWORD written = 0;
    if (!WriteFile(record.get(), &file, sizeof(file), &written, nullptr))
        return GetLastError();
    if (written != sizeof(file))
        return ERROR_WRITE_FAULT;
    if (!text.empty())
    {
        const auto bytes = static_cast<DWORD>(text.size() * sizeof(wchar_t));
        if (!WriteFile(record.get(), text.data(), bytes, &written, nullptr))
            return GetLastError();
        if (written != bytes)
            return ERROR_WRITE_FAULT;
    }
    if (!FlushFileBuffers(record.get()))
        return GetLastError();
    FILE_DISPOSITION_INFO_EX commit{FILE_DISPOSITION_FLAG_ON_CLOSE};
    return SetFileInformationByHandle(record.get(), FileDispositionInfoEx, &commit, sizeof(commit)) ? ERROR_SUCCESS : GetLastError();
}

DWORD RegisterTreeSlot(const InstallLocation& install, unsigned slot, std::wstring_view supplied)
{
    const auto path = TrimTreePath(supplied);
    std::wstring volume;
    if (slot < 1 || slot > treeTargetSlots || !IsTreePath(path) || !ReadTreeVolume(path, volume))
        return ERROR_INVALID_PARAMETER;
    if (UacDirectoryLeaf::Equal(path, install.ImagePath(treeTargetName)))
        return ERROR_ALREADY_EXISTS; // The original registration retains its original recovery journal.
    const auto root = OpenVolumeRoot(volume);
    if (!root)
        return ERROR_NOT_SUPPORTED;
    std::vector<Handle> pinned;
    UacDirectoryLeaf::ParentIdentity identity;
    DWORD error = OpenProtectedTree(root.get(), path.substr(3), pinned, identity);
    if (error)
        return error;
    if ((error = UacCopyTree::Detail::StageProtection(true).Check(pinned.back().get(), true)) != ERROR_SUCCESS)
        return error;

    // Ensure that the path shown to the user resolves to the pinned object.
    constexpr DWORD pathFlags = FILE_NAME_OPENED | VOLUME_NAME_DOS;
    const DWORD capacity = GetFinalPathNameByHandleW(pinned.back().get(), nullptr, 0, pathFlags);
    if (!capacity || capacity > UacDirectoryLeaf::maxNativeName + 5)
        return ERROR_FILE_INVALID;
    std::wstring visible(capacity, L'\0');
    const DWORD length = GetFinalPathNameByHandleW(pinned.back().get(), visible.data(), capacity, pathFlags);
    if (!length || length >= capacity ||
        !UacDirectoryLeaf::Equal(path, TrimTreePath(std::wstring_view(visible.data(), length))) ||
        !UacDirectoryLeaf::Equal(identity.volume, volume))
        return ERROR_FILE_INVALID;

    RegisteredTreeTarget existing;
    if (ReadTreeSlot(install, slot, existing))
        return UacDirectoryLeaf::Equal(existing.path, path) && UacDirectoryLeaf::SameParent(existing.identity, identity)
            ? ERROR_SUCCESS : ERROR_FILE_EXISTS;
    return WriteTreeRegistration(install, TreeSlotName(slot), identity.file, volume + std::wstring(path));
}

DWORD RegisterTreeTarget(const InstallLocation& install)
{
    PSECURITY_DESCRIPTOR rawSecurity = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"O:BAS:(ML;OICI;NWNR;;;HI)", SDDL_REVISION_1,
                                                               &rawSecurity, nullptr))
        return GetLastError();
    const std::unique_ptr<void, LocalFreeDeleter> targetSecurity(rawSecurity);
    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity;
    DWORD error = UacDirectoryLeaf::OpenParentBelowRoot(install.DirectoryHandle(), treeTargetName, raw, identity, READ_CONTROL);
    if (error == ERROR_FILE_NOT_FOUND)
        error = UacDirectoryLeaf::Detail::NativeDirectory(install.DirectoryHandle(), treeTargetName, FILE_CREATE, raw,
                                                          READ_CONTROL, targetSecurity.get());
    const auto target = Adopt(raw);
    if (error)
        return error;
    if (!ProtectedObject(target.get(), true))
        return ERROR_ACCESS_DENIED;
    if ((error = UacDirectoryLeaf::ReadParentIdentity(target.get(), identity)) != ERROR_SUCCESS)
        return error;
    if ((error = UacCopyTree::Detail::StageProtection(true).Check(target.get(), true)) != ERROR_SUCCESS)
        return error;

    UacDirectoryLeaf::ParentIdentity existing;
    std::wstring relative;
    if (ReadTreeRegistration(install, existing, relative))
        return UacDirectoryLeaf::SameParent(existing, identity) ? ERROR_SUCCESS : ERROR_FILE_INVALID;

    return WriteTreeRegistration(install, treeRegistrationName, identity.file);
}

DWORD ReceiveRequest(const InstallLocation& install, HANDLE pipe, HANDLE host, DWORD hostPid,
                     ULONGLONG& deadline, DWORD& result, Handle& listingRoot)
{
    RequestHeader header{};
    DWORD error = TransferRequest(pipe, &header, sizeof(header), false, deadline);
    if (error)
        return error;
    const bool rename = header.operation == Operation::Rename;
    const bool mkdir = header.operation == Operation::Mkdir;
    const bool setMetadata = header.operation == Operation::Metadata;
    const bool deleteFile = header.operation == Operation::Delete;
    const bool list = header.operation == Operation::List;
    const bool copy = UacF7::IsFileCopy(header.operation);
    const bool move = header.operation == Operation::Move;
    const bool tree = header.operation == Operation::CopyTree;
    const bool transfer = copy || move || tree;
    if (header.relativeLength > UacDirectoryLeaf::maxNativeName || !header.leafLength ||
        header.leafLength > UacDirectoryLeaf::maxComponent || header.sourceLength > UacDirectoryLeaf::maxComponent ||
        (!rename && !mkdir && !setMetadata && !deleteFile && !list && !transfer) || rename != (header.sourceLength != 0) ||
        (list && !header.relativeLength) || transfer != (header.targetLeafLength != 0) ||
        header.targetRelativeLength > UacDirectoryLeaf::maxNativeName || header.targetLeafLength > UacDirectoryLeaf::maxComponent ||
        header.copySize > UacF7::maxPilotCopyBytes || (!copy && header.copySize) || (!transfer && header.targetRelativeLength))
        return ERROR_INVALID_DATA;
    const size_t primarySize = volumeLength + header.relativeLength + header.leafLength + header.sourceLength;
    const size_t targetSize = transfer ? volumeLength + header.targetRelativeLength + header.targetLeafLength : 0;
    std::wstring payload(primarySize + targetSize, L'\0');
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
    const auto source = data.substr(volumeLength + header.relativeLength + header.leafLength, header.sourceLength);
    if (!UacDirectoryLeaf::IsComponent(leaf) || (rename && !UacDirectoryLeaf::IsComponent(source)) ||
        (list && leaf != UacListing::marker) ||
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
    if (list)
    {
        HANDLE raw = nullptr;
        result = UacDirectoryLeaf::OpenParentBelowRoot(root.get(), relative, raw, actual, FILE_LIST_DIRECTORY);
        parent = Adopt(raw);
    }
    else
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
    else if (transfer)
    {
        const auto targetVolume = data.substr(primarySize, volumeLength);
        const auto targetRelative = data.substr(primarySize + volumeLength, header.targetRelativeLength);
        const auto targetLeaf = data.substr(primarySize + volumeLength + header.targetRelativeLength, header.targetLeafLength);
        if (targetVolume.find(L'\0') != std::wstring_view::npos || !UacDirectoryLeaf::IsComponent(targetLeaf))
            return ERROR_INVALID_DATA;
        const UacDirectoryLeaf::ParentIdentity expectedTarget{{header.targetSerial, header.targetParentId},
                                                              std::wstring(targetVolume)};
        if (tree)
        {
            RegisteredTreeTarget registered;
            if (targetLeaf != leaf || !FindTreeTarget(install, targetRelative, targetVolume, registered) ||
                !UacDirectoryLeaf::SameParent(registered.identity, expectedTarget) ||
                !UacDirectoryLeaf::SameParent(actual, expected))
            {
                result = ERROR_FILE_INVALID;
                return ERROR_SUCCESS;
            }
        }
        const auto targetRoot = OpenVolumeRoot(targetVolume);
        if (!targetRoot)
        {
            result = ERROR_NOT_SUPPORTED;
            return ERROR_SUCCESS;
        }
        Handle targetParent;
        UacDirectoryLeaf::ParentIdentity targetIdentity;
        if (tree)
        {
            std::vector<Handle> pinned;
            result = OpenProtectedTree(targetRoot.get(), targetRelative, pinned, targetIdentity);
            if (!result && !UacDirectoryLeaf::SameParent(expectedTarget, targetIdentity))
                result = ERROR_FILE_INVALID;
            if (!result)
            {
                HANDLE rawSource = nullptr;
                UacDirectoryLeaf::ParentIdentity sourceDirectory;
                result = UacDirectoryLeaf::OpenParentBelowRoot(parent.get(), leaf, rawSource, sourceDirectory,
                                                               FILE_LIST_DIRECTORY | FILE_READ_EA);
                const auto sourceHandle = Adopt(rawSource);
                if (!result && (!UacRenameLeaf::Detail::SameSource(sourceIdentity, sourceDirectory.file) ||
                                !UacDirectoryLeaf::Equal(expected.volume, sourceDirectory.volume)))
                    result = ERROR_FILE_INVALID;
                if (!result)
                {
                    Handle journal;
                    result = OpenTreeJournal(install, expectedTarget, journal);
                    if (!result)
                    {
                        const auto copyDeadline = GetTickCount64() + copyTimeoutMs;
                        deadline = copyDeadline + timeoutMs;
                        const auto copied = UacCopyTree::CopyVerifiedTree(sourceHandle.get(), sourceDirectory, pinned.back().get(),
                                                                        expectedTarget, targetLeaf, nullptr, pipe, copyDeadline,
                                                                        journal.get());
                        result = copied.cleanupError ? ERROR_PARTIAL_COPY : copied.error;
                    }
                }
            }
            return ERROR_SUCCESS;
        }
        result = OpenRequestParent(targetRoot.get(), targetRelative, targetParent, targetIdentity);
        if (!result)
        {
            if (move)
            {
                result = UacRenameLeaf::MoveVerifiedFile(parent.get(), expected, sourceIdentity, leaf,
                                                         targetParent.get(), expectedTarget, targetLeaf, true, pipe, deadline);
            }
            else
            {
                const auto copyDeadline = GetTickCount64() + copyTimeoutMs;
                deadline = copyDeadline + timeoutMs;
                result = UacCopyLeaf::CopyVerifiedFile(parent.get(), expected, sourceIdentity, leaf,
                                                       targetParent.get(), expectedTarget, targetLeaf,
                                                       nullptr, pipe, copyDeadline, header.copySize,
                                                       header.operation == Operation::CopyIgnoreAds
                                                           ? UacCopyLeaf::StreamPolicy::IgnoreNamed
                                                           : UacCopyLeaf::StreamPolicy::PreserveNamed,
                                                       ProtectedObject);
            }
        }
    }
    else if (setMetadata)
        result = UacMetadataLeaf::SetVerifiedMetadata(parent.get(), expected, sourceIdentity, leaf, metadata);
    else if (deleteFile)
        result = UacDeleteLeaf::DeleteVerifiedFile(parent.get(), expected, sourceIdentity, leaf);
    else if (list)
    {
        if (UacDirectoryLeaf::SameParent(expected, actual))
            listingRoot = std::move(parent);
        else
            result = ERROR_FILE_INVALID;
    }
    else
        result = UacDirectoryLeaf::CreateVerifiedLeaf(parent.get(), expected, leaf);
    return ERROR_SUCCESS;
}

DWORD ServeListings(HANDLE pipe, HANDLE host, DWORD hostPid, HANDLE root)
{
    constexpr ULONGLONG idleTimeoutMs = 15 * 60 * 1000;
    std::vector<WIN32_FIND_DATAW> entries;
    for (;;)
    {
        DWORD length = 0;
        DWORD error = TransferRequest(pipe, &length, sizeof(length), false, GetTickCount64() + idleTimeoutMs);
        if (error)
            return error;
        const bool inventoryRequest = (length & UacListing::inventoryRequest) != 0;
        length &= ~UacListing::inventoryRequest;
        if (length > UacDirectoryLeaf::maxNativeName || !LiveInSession(host, hostPid))
            return ERROR_INVALID_DATA;
        const auto deadline = GetTickCount64() + timeoutMs;
        std::wstring relative(length, L'\0');
        if (length)
        {
            error = TransferRequest(pipe, relative.data(), length * static_cast<DWORD>(sizeof(wchar_t)), false, deadline);
            if (error)
                return error;
        }
        UacDirectoryLeaf::ParentIdentity identity;
        Handle child;
        if (length)
        {
            HANDLE raw = nullptr;
            error = UacDirectoryLeaf::OpenParentBelowRoot(root, relative, raw, identity, FILE_LIST_DIRECTORY);
            child = Adopt(raw);
        }
        else
            error = UacDirectoryLeaf::ReadParentIdentity(root, identity);
        UacF7::DirectoryInventory inventory;
        if (!error)
        {
            HANDLE directory = child ? child.get() : root;
            error = inventoryRequest ? UacListing::Inventory(directory, deadline - 250, inventory)
                                     : UacListing::Enumerate(directory, entries);
        }
        ListingReply reply{error, error || inventoryRequest ? 0 : static_cast<DWORD>(entries.size())};
        error = TransferRequest(pipe, &reply, sizeof(reply), true, deadline);
        if (!error && !reply.error && inventoryRequest)
            error = TransferRequest(pipe, &inventory, sizeof(inventory), true, deadline);
        if (!error && reply.count)
            error = TransferRequest(pipe, entries.data(), reply.count * static_cast<DWORD>(sizeof(WIN32_FIND_DATAW)), true, deadline);
        if (error)
            return error;
    }
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
    auto deadline = GetTickCount64() + timeoutMs;
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
        Handle listingRoot;
        error = ReceiveRequest(install, pipe.get(), host.get(), hostPid, deadline, result, listingRoot);
        if (error)
            return IoError(L"Receive request", error);
        error = TransferRequest(pipe.get(), &result, sizeof(result), true, deadline);
        if (error)
            return IoError(L"Send request result", error);
        if (listingRoot)
            return IoError(L"Listing session ended", ServeListings(pipe.get(), host.get(), hostPid, listingRoot.get()));
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
             , HWND owner, RequestHeader* request, std::wstring* payload, UacF7::MetadataChange* metadata, UacF7::Result* result,
               HANDLE* listingPipe = nullptr, HANDLE cancel = nullptr
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
    auto deadline = GetTickCount64() + timeoutMs;
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

        const bool tree = request->operation == Operation::CopyTree;
        const bool copy = UacF7::IsFileCopy(request->operation);
        if (copy || tree)
            deadline = GetTickCount64() + copyTimeoutMs + 2 * timeoutMs;
        error = TransferRequest(pipe.get(), &operationError, sizeof(operationError), false, deadline, tree ? cancel : nullptr);
        if (!error && (copy || request->operation == Operation::Move || tree))
        {
            // The authenticated reply is sent only after the operation handles close.
            result->error = operationError;
            result->unknown = tree && operationError == ERROR_PARTIAL_COPY;
        }
        if (error)
            return IoError(L"Read request result", error);
        if (listingPipe && operationError == ERROR_SUCCESS)
        {
            *listingPipe = pipe.release();
            result->error = ERROR_SUCCESS;
            result->unknown = false;
            return Ok;
        }
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
    result->unknown = request->operation == Operation::CopyTree && operationError == ERROR_PARTIAL_COPY;
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
    return PrepareTarget(path, renameTo.empty() ? Operation::Mkdir : Operation::Rename, renameTo, nullptr);
}

bool PreparedLeaf::PrepareMetadata(const std::wstring& path, const MetadataChange& change)
{
    return UacMetadataLeaf::IsValid(change) && PrepareTarget(path, Operation::Metadata, {}, &change);
}

bool PreparedLeaf::PrepareDelete(const std::wstring& path)
{
    return PrepareTarget(path, Operation::Delete, {}, nullptr);
}

bool PreparedLeaf::PrepareTarget(const std::wstring& path, Operation requested, const std::wstring& renameTo,
                                 const MetadataChange* change)
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
    const bool rename = requested == Operation::Rename;
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
    if (requested == Operation::CopyTree || (change && UacMetadataLeaf::IsDirectory(*change)))
    {
        HANDLE raw = nullptr;
        UacDirectoryLeaf::ParentIdentity anchored, visibleDirectory;
        if (UacDirectoryLeaf::OpenParentBelowRoot(opened.get(), finalName, raw, anchored))
            return false;
        openedSource = Adopt(raw);
        const std::wstring sourcePath = LR"(\\?\)" + std::wstring(input);
        const auto visibleSource = Adopt(CreateFileW(sourcePath.c_str(), FILE_READ_ATTRIBUTES,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                                      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                                      nullptr));
        if (!visibleSource || UacDirectoryLeaf::ReadParentIdentity(visibleSource.get(), visibleDirectory) ||
            !UacDirectoryLeaf::SameParent(anchored, visibleDirectory) ||
            !UacDirectoryLeaf::Equal(snapshot.volume, anchored.volume))
            return false;
        if (change && UacMetadataLeaf::CheckAttributes(openedSource.get(), *change))
            return false;
        std::memcpy(sourceId, anchored.file.FileId.Identifier, sizeof(sourceId));
    }
    else if (requested != Operation::Mkdir && requested != Operation::List)
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
        if (requested == Operation::Delete && UacDeleteLeaf::CheckAttributes(openedSource.get()))
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
    operation = requested;
    if (change)
        metadata = *change;
    return true;
}

DWORD PreparedLeaf::ExecuteLocal() const
{
    if (!parent || (operation != Operation::Metadata && operation != Operation::Delete))
        return ERROR_INVALID_PARAMETER;
    UacDirectoryLeaf::ParentIdentity expectedParent{};
    expectedParent.file.VolumeSerialNumber = serial;
    std::memcpy(expectedParent.file.FileId.Identifier, fileId, sizeof(fileId));
    expectedParent.volume = volume;
    FILE_ID_INFO expectedSource{serial, {}};
    std::memcpy(expectedSource.FileId.Identifier, sourceId, sizeof(sourceId));
    if (operation == Operation::Delete)
        return UacDeleteLeaf::DeleteVerifiedFile(parent, expectedParent, expectedSource, leaf);
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
    RequestHeader header{serial, {}, {}, operation,
                         static_cast<DWORD>(relative.size()), static_cast<DWORD>(leaf.size()),
                         static_cast<DWORD>(sourceLeaf.size()), 0, {}, 0, 0, 0};
    std::memcpy(header.parentId.Identifier, fileId, sizeof(fileId));
    std::memcpy(header.sourceId.Identifier, sourceId, sizeof(sourceId));
    std::wstring payload = volume + relative + leaf + sourceLeaf;
    MetadataChange requestMetadata = metadata;
    RunProbe(install, false, false, owner, &header, &payload, operation == Operation::Metadata ? &requestMetadata : nullptr, &result);
    return result;
}

bool PreparedTransfer::Prepare(const std::wstring& sourcePath, const std::wstring& targetPath,
                               ULONGLONG expectedSize, Operation operation)
{
    const bool moving = operation == Operation::Move;
    if ((!moving && (!IsFileCopy(operation) || expectedSize > maxPilotCopyBytes)) ||
        !source.PrepareTarget(sourcePath, operation, {}, nullptr) ||
        !target.Prepare(targetPath))
        return false;
    if (moving && (source.serial != target.serial || !UacDirectoryLeaf::Equal(source.volume, target.volume)))
        return false;
    size = moving ? 0 : expectedSize;
    return true;
}

bool PreparedTransfer::PrepareTree(const std::wstring& sourcePath, std::wstring_view targetDirectory, std::wstring& destination)
{
    targetDirectory = TrimTreePath(targetDirectory);
    InstallLocation install;
    if (!IsTreePath(targetDirectory) || !install.Open())
        return false;
    RegisteredTreeTarget registered;
    if (!FindTreeTarget(install, targetDirectory, {}, registered) ||
        !source.PrepareTarget(sourcePath, Operation::CopyTree, {}, nullptr))
        return false;
    target.relative = std::move(registered.relative);
    target.volume = std::move(registered.identity.volume);
    target.serial = registered.identity.file.VolumeSerialNumber;
    std::memcpy(target.fileId, registered.identity.file.FileId.Identifier, sizeof(target.fileId));
    target.leaf = source.leaf;
    destination = registered.path + L"\\" + target.leaf;
    return true;
}

Result PreparedTransfer::Execute(HWND owner, HANDLE cancel) const
{
    Result result;
    const ComApartment com;
    InstallLocation install;
    if (!source.source || (source.operation == Operation::CopyTree ? target.relative.empty() : !target.parent) ||
        FAILED(com.result) || !install.Open() ||
        !install.MatchesProcess(GetCurrentProcess(), appImage) || !install.OpenImage(appImage))
        return result;

    RequestHeader header{source.serial, {}, {}, source.operation,
                         static_cast<DWORD>(source.relative.size()), static_cast<DWORD>(source.leaf.size()), 0,
                         target.serial, {}, static_cast<DWORD>(target.relative.size()), static_cast<DWORD>(target.leaf.size()), size};
    std::memcpy(header.parentId.Identifier, source.fileId, sizeof(source.fileId));
    std::memcpy(header.sourceId.Identifier, source.sourceId, sizeof(source.sourceId));
    std::memcpy(header.targetParentId.Identifier, target.fileId, sizeof(target.fileId));
    std::wstring payload = source.volume + source.relative + source.leaf + target.volume + target.relative + target.leaf;
    RunProbe(install, false, false, owner, &header, &payload, nullptr, &result, nullptr, cancel);
    return result;
}

ListingSession::~ListingSession()
{
    Close();
}

void ListingSession::Close()
{
    if (pipe)
    {
        CloseHandle(pipe);
        pipe = nullptr;
    }
}

bool ListingSession::Prepare(const std::wstring& path)
{
    if (path.size() <= 3 || path.back() == L'\\' ||
        !prepared.PrepareTarget(path + L"\\" + UacListing::marker, Operation::List, {}, nullptr))
        return false;
    root = path;
    return true;
}

DWORD ListingSession::Start(HWND owner)
{
    const ComApartment com;
    InstallLocation install;
    if (root.empty() || pipe || FAILED(com.result) || !install.Open() ||
        !install.MatchesProcess(GetCurrentProcess(), appImage) || !install.OpenImage(appImage))
        return ERROR_ACCESS_DENIED;
    RequestHeader header{prepared.serial, {}, {}, Operation::List,
                         static_cast<DWORD>(prepared.relative.size()), static_cast<DWORD>(prepared.leaf.size()), 0,
                         0, {}, 0, 0, 0};
    std::memcpy(header.parentId.Identifier, prepared.fileId, sizeof(prepared.fileId));
    std::wstring payload = prepared.volume + prepared.relative + prepared.leaf;
    Result result;
    RunProbe(install, false, false, owner, &header, &payload, nullptr, &result, &pipe);
    return result.error;
}

bool ListingSession::Contains(const std::wstring& path) const
{
    return pipe && path.compare(0, root.size(), root) == 0 &&
           (path.size() == root.size() || (path.size() > root.size() && path[root.size()] == L'\\'));
}

DWORD ListingSession::SendPath(const std::wstring& path, DWORD flags, ULONGLONG deadline)
{
    if (!Contains(path))
        return ERROR_ACCESS_DENIED;
    std::wstring relative = path.size() == root.size() ? std::wstring{} : path.substr(root.size() + 1);
    if (relative.size() > UacDirectoryLeaf::maxNativeName)
        return ERROR_FILENAME_EXCED_RANGE;
    DWORD request = static_cast<DWORD>(relative.size()) | flags;
    DWORD error = TransferRequest(pipe, &request, sizeof(request), true, deadline);
    if (!error && !relative.empty())
        error = TransferRequest(pipe, relative.data(), static_cast<DWORD>(relative.size() * sizeof(wchar_t)), true, deadline);
    if (error)
        Close();
    return error;
}

DWORD ListingSession::Inventory(const std::wstring& path, DirectoryInventory& inventory)
{
    inventory = {};
    const auto deadline = GetTickCount64() + timeoutMs;
    DWORD error = SendPath(path, UacListing::inventoryRequest, deadline);
    if (error)
        return error;
    ListingReply reply{};
    error = TransferRequest(pipe, &reply, sizeof(reply), false, deadline);
    if (!error && reply.count)
        error = ERROR_INVALID_DATA;
    if (!error && !reply.error)
    {
        DirectoryInventory complete;
        error = TransferRequest(pipe, &complete, sizeof(complete), false, deadline);
        if (!error && (complete.directories > UacListing::maxEntries ||
                       complete.files > UacListing::maxEntries - complete.directories))
            error = ERROR_INVALID_DATA;
        if (!error)
            inventory = complete;
    }
    if (error)
        Close();
    return error ? error : reply.error;
}

DWORD ListingSession::List(const std::wstring& path, std::vector<WIN32_FIND_DATAW>& entries)
{
    entries.clear();
    const auto deadline = GetTickCount64() + timeoutMs;
    DWORD error = SendPath(path, 0, deadline);
    if (error)
        return error;
    ListingReply reply{};
    if (!error)
        error = TransferRequest(pipe, &reply, sizeof(reply), false, deadline);
    if (!error && (reply.count > UacListing::maxEntries || (reply.error && reply.count)))
        error = ERROR_INVALID_DATA;
    if (!error && reply.count)
    {
        entries.resize(reply.count);
        error = TransferRequest(pipe, entries.data(), reply.count * static_cast<DWORD>(sizeof(WIN32_FIND_DATAW)), false, deadline);
        if (!error)
        {
            for (const auto& entry : entries)
                if (!UacDirectoryLeaf::IsComponent(std::wstring_view(entry.cFileName, wcsnlen_s(entry.cFileName, ARRAYSIZE(entry.cFileName)))))
                {
                    error = ERROR_INVALID_DATA;
                    break;
                }
        }
    }
    if (error)
    {
        entries.clear();
        Close();
        return error;
    }
    return reply.error;
}
}
#else
int wmain(int argc, wchar_t** argv)
{
#ifdef SAM_UAC_BROKER
    if (!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32))
        return Report(Failed, L"Restrict DLL lookup", GetLastError());
    const bool registerTree = (argc == 2 || argc == 4) && wcscmp(argv[1], L"--register-tree-target") == 0;
    if (!registerTree && argc != 3)
        return Untrusted;
    std::wstring pipeName;
    DWORD hostPid = 0;
    if (!registerTree)
    {
        pipeName = ParsePipeName(argv[1]);
        wchar_t* end = nullptr;
        hostPid = wcstoul(argv[2], &end, 10);
        if (pipeName.empty() || !hostPid || *end)
            return Untrusted;
    }
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
    if (registerTree)
    {
        unsigned slot = 0;
        if (argc == 4)
        {
            const std::wstring_view value(argv[2]);
            if (value.size() != 1 || value.front() < L'1' || value.front() > L'8')
                return Report(Failed, L"Invalid tree registration slot", ERROR_INVALID_PARAMETER);
            slot = static_cast<unsigned>(value.front() - L'0');
        }
        const DWORD error = slot ? RegisterTreeSlot(install, slot, argv[3]) : RegisterTreeTarget(install);
        return error ? Report(Failed, L"Register tree target", error) : Ok;
    }
    return RunBroker(install, pipeName.c_str(), hostPid);
#else
    if (!install.MatchesProcess(GetCurrentProcess(), probeImage) || !install.OpenImage(probeImage))
        return Report(Untrusted, L"Verify probe image", ERROR_ACCESS_DENIED);
    return RunProbe(install, drop, badVersion);
#endif
}
#endif
