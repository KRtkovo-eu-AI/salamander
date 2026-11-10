// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-10 | Test protected metadata identities and mutation guards. | Keep file and directory reads separate from authorization and reject stale attributes or objects. | #731 | GPT-6 (Codex) |
// | 2026-11-10 | Test typed panel preparation for mkdir and rename. | Keep identity reads scoped and verify later mutations reject collisions and stale objects. | #731 | GPT-6 (Codex) |
// | 2026-11-10 | Test the panel metadata request against the real listing dispatcher. | Bind viewer consent to the selected file inside a protected listing. | #731 | Codex |
// | 2026-11-09 | Cancel one transfer while another is staged. | Verify that a second live broker session can publish and continue its own batch. | | Codex |
// | 2026-11-09 | Make active Find broker reads cancellable. | Stop waiting on the scoped read pipe and discard incomplete replies. | | Codex |
// | 2026-11-09 | Pause an actual overwrite during a live copy. | Verify the staged copy cannot fill the original name while the replacement holds the journal. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Lock the journal after a live copy writes its first block. | Reproduce a replacement starting after the copy's initial journal check. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise a competing protected overwrite journal. | Verify a live copy job does not publish while the target journal is locked. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Use an isolated install directory for dispatcher tests. | Exercise the absent disk-overwrite journal on a real pipe. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Test changing copy metadata options after consent. | Reject a changed operation even when both parent identities match. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Verify the exact-file Find wire scope. | Refuse enumeration, names and out-of-range requests on a live pipe. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Disconnect one of two protected Find content sessions. | Verify the other broker session continues reading. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise nonlocal volume names on the broker pipe. | Reject UNC and device aliases before copying or moving any file. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Reject malformed transfer progress on a live pipe. | Keep invalid progress out of the worker and require a terminal result. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Check the expanded progress wire frame. | Preserve the 64-bit cancellation and terminal reply regression. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise progress and cancellation on a local pipe. | Cover 64-bit counts and a late progress message. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Verify the final mutation frame on a real local pipe. | Distinguish undelivered requests from requests without a reply. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Verify F6 without the former pilot limits. | Exercise large request sizes and the active deadline policy. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise stale overwrite consent through the broker dispatcher. | Verify an externally replaced target survives and the job can continue. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Race two live copy jobs for the same target. | Verify no-replace publication across broker sessions. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Test the 64-bit F6 request size boundary. | Reject over-limit moves before any transfer and retain the lower F5 limit. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Check the source admission used for protected F6. | Cover sparse, exact compression and metadata combinations before broker launch. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Verify the host result policy for partial moves. | Keep one-shot replies and reusable jobs consistent. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise the compressed move request on the broker pipe. | Prove same-volume refusal with the new request type. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise the late F6 request framing. | Read the destination revision before evaluating the volume boundary. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Reject a cross-volume command on one volume. | Verify the broker dispatches the distinct F6 request safely. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Verify a queued cancellation on the live broker pipe. | Prove the dispatcher ends the scoped session without publishing the pending file. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Exercise scoped mutations through the real broker dispatcher. | Validate repeated requests and scope rejection on NTFS without interactive elevation. | | GPT-6.1 Sol (Codex) |

#define SAM_UAC_BROKER
#define SAM_UAC_BROKER_JOB_TEST
#define SAM_UAC_COPY_FAULT_TEST
#define SAM_UAC_REPLACEMENT_FAULT_TEST
#define wmain InstalledBrokerMain
#include "uacbroker.cpp"
#undef wmain

#include <initializer_list>
#include <thread>

namespace UacCopyLeaf::Detail
{
struct CopyPause
{
    HANDLE staged;
    HANDLE resume;
};
thread_local const CopyPause* activePause = nullptr;

void AfterCopyBlockForTest(HANDLE, HANDLE)
{
    const auto* pause = activePause;
    if (!pause)
        return;
    activePause = nullptr;
    if (SetEvent(pause->staged))
        WaitForSingleObject(pause->resume, timeoutMs);
}
}

namespace UacRenameLeaf::Detail
{
thread_local const UacCopyLeaf::Detail::CopyPause* activeReplacementPause = nullptr;

void PauseReplacementAt(ReplacementFaultPoint point)
{
    const auto* pause = activeReplacementPause;
    if (pause && point == ReplacementFaultPoint::BackupRenamed && SetEvent(pause->staged))
        WaitForSingleObject(pause->resume, timeoutMs);
}
}

namespace
{
int failures = 0;

void Check(bool condition, const char* message)
{
    std::printf("%s %s\n", condition ? "PASS" : "FAIL", message);
    if (!condition)
        ++failures;
}

struct Directory
{
    Handle handle;
    UacDirectoryLeaf::ParentIdentity identity;
    std::wstring path, relative;

    bool Open(const wchar_t* name)
    {
        if (!CreateDirectoryW(name, nullptr))
            return false;
        handle = Adopt(CreateFileW(name, FILE_TRAVERSE | FILE_ADD_FILE | FILE_READ_ATTRIBUTES,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        if (!handle || UacDirectoryLeaf::ReadParentIdentity(handle.get(), identity))
            return false;
        constexpr DWORD flags = VOLUME_NAME_GUID | FILE_NAME_OPENED;
        const DWORD capacity = GetFinalPathNameByHandleW(handle.get(), nullptr, 0, flags);
        if (!capacity)
            return false;
        path.resize(capacity);
        const DWORD length = GetFinalPathNameByHandleW(handle.get(), path.data(), capacity, flags);
        if (length <= identity.volume.size() || length >= capacity)
            return false;
        path.resize(length);
        relative = path.substr(identity.volume.size() + 1);
        return true;
    }

    std::wstring File(std::wstring_view leaf) const { return path + L"\\" + std::wstring(leaf); }
};

bool WriteByte(const std::wstring& path, BYTE value)
{
    const auto file = Adopt(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr));
    DWORD written = 0;
    return file && WriteFile(file.get(), &value, sizeof(value), &written, nullptr) && written == sizeof(value);
}

bool HasByte(const std::wstring& path, BYTE expected)
{
    const auto file = Adopt(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
    BYTE actual[2]{};
    DWORD bytes = 0;
    return file && ReadFile(file.get(), actual, sizeof(actual), &bytes, nullptr) && bytes == 1 && actual[0] == expected;
}

bool ReadBasic(const std::wstring& path, FILE_BASIC_INFO& basic)
{
    const auto file = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                       OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr));
    return file && GetFileInformationByHandleEx(file.get(), FileBasicInfo, &basic, sizeof(basic));
}

bool SameBasic(const FILE_BASIC_INFO& first, const FILE_BASIC_INFO& second)
{
    return first.CreationTime.QuadPart == second.CreationTime.QuadPart &&
           first.LastAccessTime.QuadPart == second.LastAccessTime.QuadPart &&
           first.LastWriteTime.QuadPart == second.LastWriteTime.QuadPart &&
           first.ChangeTime.QuadPart == second.ChangeTime.QuadPart && first.FileAttributes == second.FileAttributes;
}

// Test only: enter the dispatcher after the elevation handshake boundary.
// The production entry point still authenticates both images and the peer token.
class JobPipe
{
public:
    ~JobPipe() { Finish(); }

    bool Open(HANDLE readyToDispatch = nullptr, HANDLE findRoot = nullptr,
              Operation findScope = Operation::FindContent,
              const UacCopyLeaf::Detail::CopyPause* copyPause = nullptr)
    {
        GUID id{};
        if (FAILED(CoCreateGuid(&id)))
            return false;
        const auto name = PipeName(id);
        server = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
            FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS,
            1, 4096, 4096, 0, nullptr));
        if (!server)
            return false;
        dispatcher = std::thread([this, name, readyToDispatch, findRoot, findScope, copyPause] {
            const ComApartment com;
            if (FAILED(com.result))
                return;
            const auto peer = Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
            if (peer && ExpectedPipePeer(peer.get(), GetCurrentProcess(), GetCurrentProcessId(), true) &&
                (!readyToDispatch || WaitForSingleObject(readyToDispatch, timeoutMs) == WAIT_OBJECT_0))
            {
                if (findRoot)
                {
                    dispatchError = ServeListings(peer.get(), GetCurrentProcess(), GetCurrentProcessId(), findRoot, findScope);
                    return;
                }
                const InstallLocation install(Adopt(CreateFileW(L".", FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)));
                if (!install.DirectoryHandle())
                    return;
                UacCopyLeaf::Detail::activePause = copyPause;
                dispatchError = ServeTransferJob(install, peer.get(), GetCurrentProcess(), GetCurrentProcessId(),
                                                 GetTickCount64() + timeoutMs);
            }
        });
        return !Connect(server.get(), GetTickCount64() + timeoutMs);
    }

    DWORD Finish()
    {
        server.reset();
        if (dispatcher.joinable())
            dispatcher.join();
        return dispatchError;
    }

    DWORD CancelIdle()
    {
        BYTE signal = UacRenameLeaf::Detail::transferCancelSignal;
        if (const DWORD error = TransferRequest(server.get(), &signal, sizeof(signal), true, GetTickCount64() + timeoutMs))
            return error;
        dispatcher.join();
        return Finish();
    }

    bool Request(Operation operation, const Directory& source, std::wstring_view leaf, DWORD& result,
                 const Directory* target = nullptr, UacF7::MetadataChange* change = nullptr,
                 UacF7::MetadataJobPolicy* policy = nullptr, bool stopBeforePayload = false, HANDLE cancelBeforeDispatch = nullptr,
                 UacF7::ArchiveRevision* revision = nullptr, ULONGLONG copySize = 1)
    {
        const auto file = Adopt(CreateFileW(source.File(leaf).c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        FILE_ID_INFO identity{};
        if (!file || !GetFileInformationByHandleEx(file.get(), FileIdInfo, &identity, sizeof(identity)))
            return false;
        RequestHeader header{};
        header.serial = source.identity.file.VolumeSerialNumber;
        header.parentId = source.identity.file.FileId;
        header.sourceId = identity.FileId;
        header.operation = operation;
        header.relativeLength = static_cast<DWORD>(source.relative.size());
        header.leafLength = static_cast<DWORD>(leaf.size());
        std::wstring payload = source.identity.volume + source.relative + std::wstring(leaf);
        if (target)
        {
            header.targetSerial = target->identity.file.VolumeSerialNumber;
            header.targetParentId = target->identity.file.FileId;
            header.targetRelativeLength = static_cast<DWORD>(target->relative.size());
            header.targetLeafLength = header.leafLength;
            header.copySize = copySize;
            payload += target->identity.volume + target->relative + std::wstring(leaf);
        }
        const ULONGLONG deadline = GetTickCount64() + timeoutMs;
        if (TransferRequest(server.get(), &header, sizeof(header), true, deadline))
            return false;
        if (stopBeforePayload)
            return true;
        if (TransferRequest(server.get(), payload.data(), static_cast<DWORD>(payload.size() * sizeof(wchar_t)), true, deadline) ||
            (change && TransferRequest(server.get(), change, sizeof(*change), true, deadline)) ||
            (policy && TransferRequest(server.get(), policy, sizeof(*policy), true, deadline)) ||
            (revision && TransferRequest(server.get(), revision, sizeof(*revision), true, deadline)))
            return false;
        if (cancelBeforeDispatch)
        {
            BYTE signal = UacRenameLeaf::Detail::transferCancelSignal;
            if (TransferRequest(server.get(), &signal, sizeof(signal), true, deadline) || !SetEvent(cancelBeforeDispatch))
                return false;
        }
        return !TransferRequest(server.get(), &result, sizeof(result), false, deadline);
    }

    bool ReadLeafIdentity(std::wstring_view path, DWORD& status, UacListing::LeafIdentity& identity,
                          bool parentOnly = false, bool directory = false)
    {
        status = ERROR_GEN_FAILURE;
        const auto deadline = GetTickCount64() + timeoutMs;
        const DWORD flags = UacListing::leafIdentityRequest | (parentOnly ? UacListing::parentIdentityRequest : 0) |
                            (directory ? UacListing::directoryIdentityRequest : 0);
        return SendFindRequest(flags, path, deadline) &&
               !TransferRequest(server.get(), &status, sizeof(status), false, deadline) &&
               (status || !TransferRequest(server.get(), &identity, sizeof(identity), false, deadline));
    }

    bool QueryLeafIdentity(std::wstring_view path, DWORD expected, UacListing::LeafIdentity& identity,
                           bool parentOnly = false, bool directory = false)
    {
        DWORD status = ERROR_GEN_FAILURE;
        return ReadLeafIdentity(path, status, identity, parentOnly, directory) && status == expected;
    }

    bool OpenFindContent(std::wstring_view leaf)
    {
        DWORD status = ERROR_GEN_FAILURE;
        UacF7::ArchiveRevision revision{};
        const auto deadline = GetTickCount64() + timeoutMs;
        return SendFindRequest(UacListing::contentRequest, leaf, deadline) &&
               !TransferRequest(server.get(), &status, sizeof(status), false, deadline) && !status &&
               !TransferRequest(server.get(), &revision, sizeof(revision), false, deadline) && revision.size == 1;
    }

    bool ReadAndCloseFindContent(BYTE expected)
    {
        UacF7::ArchiveReadRange range{0, 1, 0};
        DWORD status = ERROR_GEN_FAILURE;
        BYTE actual = 0;
        const auto deadline = GetTickCount64() + timeoutMs;
        if (TransferRequest(server.get(), &range, sizeof(range), true, deadline) ||
            TransferRequest(server.get(), &status, sizeof(status), false, deadline) || status ||
            TransferRequest(server.get(), &actual, sizeof(actual), false, deadline) || actual != expected)
            return false;
        range = {};
        return !TransferRequest(server.get(), &range, sizeof(range), true, deadline) &&
               !TransferRequest(server.get(), &status, sizeof(status), false, deadline) && !status;
    }

    bool RejectFindRequest(DWORD header)
    {
        return SendFindRequest(header, {}, GetTickCount64() + timeoutMs) && EndedWithoutFindReply();
    }

    bool RejectFindRange(UacF7::ArchiveReadRange range)
    {
        return !TransferRequest(server.get(), &range, sizeof(range), true, GetTickCount64() + timeoutMs) &&
               EndedWithoutFindReply();
    }

private:
    bool SendFindRequest(DWORD flags, std::wstring_view path, ULONGLONG deadline)
    {
        DWORD length = static_cast<DWORD>(path.size()) | flags;
        std::wstring relative(path);
        return !TransferRequest(server.get(), &length, sizeof(length), true, deadline) &&
               (relative.empty() || !TransferRequest(server.get(), relative.data(),
                   static_cast<DWORD>(relative.size() * sizeof(wchar_t)), true, deadline));
    }

    bool EndedWithoutFindReply()
    {
        DWORD reply = 0;
        return TransferRequest(server.get(), &reply, sizeof(reply), false, GetTickCount64() + timeoutMs) == ERROR_BROKEN_PIPE &&
               Finish() == ERROR_INVALID_DATA;
    }

    Handle server;
    std::thread dispatcher;
    DWORD dispatchError = ERROR_GEN_FAILURE;
};

bool IsAbsent(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
}

void CheckListedLeafIdentity(const Directory& source)
{
    constexpr wchar_t leaf[] = L"viewer-\u6771.txt";
    JobPipe job;
    if (!WriteByte(source.File(leaf), 13) || !job.Open(nullptr, source.handle.get(), Operation::List))
    {
        Check(false, "prepare the viewer identity fixture");
        return;
    }
    UacListing::LeafIdentity identity{};
    const auto file = Adopt(CreateFileW(source.File(leaf).c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO fileId{};
    Check(file && !UacRenameLeaf::Detail::ReadSourceIdentity(file.get(), fileId) &&
              job.QueryLeafIdentity(leaf, ERROR_SUCCESS, identity) &&
              !std::memcmp(&identity.parent, &source.identity.file.FileId, sizeof(identity.parent)) &&
              !std::memcmp(&identity.file, &fileId.FileId, sizeof(identity.file)),
          "the protected listing returns the exact parent and file IDs without granting content");
    Check(job.QueryLeafIdentity(L"..\\viewer-\u6771.txt", ERROR_INVALID_NAME, identity),
          "a viewer identity request cannot escape the approved listing root");
    Check(job.RejectFindRequest(UacListing::contentRequest | UacListing::leafIdentityRequest),
          "the listing rejects combined identity and content requests");
    JobPipe find;
    Check(find.Open(nullptr, source.handle.get()) && find.RejectFindRequest(UacListing::leafIdentityRequest),
          "the Find scope rejects the panel-only identity request");
}

void CheckPanelPreparationPaths(const Directory& source, const Directory& outside)
{
    Directory unicode, longFirst, longLast;
    const std::wstring unicodeName = L"preparation-\u6771-\U0001f642";
    const std::wstring longFirstName(110, L'\u6771');
    const std::wstring longLastName(110, L'\u4eac');
    if (!unicode.Open(source.File(unicodeName).c_str()) ||
        !longFirst.Open(unicode.File(longFirstName).c_str()) ||
        !longLast.Open(longFirst.File(longLastName).c_str()))
    {
        Check(false, "prepare Unicode and extended-length panel parent paths");
        return;
    }
    JobPipe job;
    if (!job.Open(nullptr, source.handle.get(), Operation::List))
    {
        Check(false, "open the read-only panel preparation session");
        return;
    }
    const std::pair<const Directory*, std::wstring> parents[] = {
        {&source, {}},
        {&unicode, unicodeName + L"\\"},
        {&longFirst, unicodeName + L"\\" + longFirstName + L"\\"},
        {&longLast, unicodeName + L"\\" + longFirstName + L"\\" + longLastName + L"\\"},
    };
    const FILE_ID_128 zeroId{};
    for (const auto& [directory, relative] : parents)
    {
        for (const auto leaf : {L"prepared-ascii", L"prepared-\u6771-\U0001f642"})
        {
            const auto path = directory->File(leaf);
            UacListing::LeafIdentity identity;
            std::memset(&identity, 0xA5, sizeof(identity));
            Check(IsAbsent(path) && job.QueryLeafIdentity(relative + leaf, ERROR_SUCCESS, identity, true) &&
                      !std::memcmp(&identity.parent, &directory->identity.file.FileId, sizeof(identity.parent)) &&
                      !std::memcmp(&identity.file, &zeroId, sizeof(identity.file)) && !identity.attributes && IsAbsent(path),
                  "mkdir preparation returns only the exact parent ID and creates no leaf");
            Check(job.QueryLeafIdentity(relative + leaf, ERROR_FILE_NOT_FOUND, identity),
                  "rename preparation requires an existing source and cannot create the missing leaf");
            if (!WriteByte(path, 41))
            {
                Check(false, "create the existing plain-file preparation fixture");
                continue;
            }
            const auto file = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
            FILE_ID_INFO actual{};
            FILE_BASIC_INFO before{}, after{};
            Check(file && !UacRenameLeaf::Detail::ReadSourceIdentity(file.get(), actual) &&
                      ReadBasic(path, before) &&
                      job.QueryLeafIdentity(relative + leaf, ERROR_SUCCESS, identity) &&
                      !std::memcmp(&identity.parent, &directory->identity.file.FileId, sizeof(identity.parent)) &&
                      !std::memcmp(&identity.file, &actual.FileId, sizeof(identity.file)) &&
                      identity.attributes == before.FileAttributes && ReadBasic(path, after) && SameBasic(before, after) &&
                      HasByte(path, 41) &&
                      IsAbsent(directory->File(std::wstring(leaf) + L"-renamed")),
                  "file preparation returns verified identity and attributes without changing data, names or metadata");
            std::memset(&identity, 0xA5, sizeof(identity));
            Check(job.QueryLeafIdentity(relative + leaf, ERROR_SUCCESS, identity, true) &&
                      !std::memcmp(&identity.parent, &directory->identity.file.FileId, sizeof(identity.parent)) &&
                      !std::memcmp(&identity.file, &zeroId, sizeof(identity.file)) && !identity.attributes && HasByte(path, 41),
                  "parent-only preparation accepts an occupied name without granting its identity or replacing it");
            DWORD status = ERROR_SUCCESS;
            Check(job.ReadLeafIdentity(relative + leaf, status, identity, false, true) && status != ERROR_SUCCESS &&
                      HasByte(path, 41), "directory preparation rejects a regular file without changing it");
        }
        constexpr wchar_t childName[] = L"metadata-directory-\u4eac-\U0001f642";
        Directory child;
        if (!child.Open(directory->File(childName).c_str()) || !WriteByte(child.File(L"untouched"), 43))
        {
            Check(false, "prepare a directory identity fixture with an untouched child");
            continue;
        }
        UacListing::LeafIdentity identity{};
        FILE_BASIC_INFO before{}, after{};
        Check(ReadBasic(child.path, before) &&
                  job.QueryLeafIdentity(relative + childName, ERROR_SUCCESS, identity, false, true) &&
                  !std::memcmp(&identity.parent, &directory->identity.file.FileId, sizeof(identity.parent)) &&
                  !std::memcmp(&identity.file, &child.identity.file.FileId, sizeof(identity.file)) &&
                  identity.attributes == before.FileAttributes && (identity.attributes & FILE_ATTRIBUTE_DIRECTORY) &&
                  ReadBasic(child.path, after) && SameBasic(before, after) && HasByte(child.File(L"untouched"), 43),
              "directory preparation returns verified IDs and attributes without changing its metadata or child");
        DWORD status = ERROR_SUCCESS;
        Check(job.ReadLeafIdentity(relative + childName, status, identity) && status != ERROR_SUCCESS,
              "file preparation cannot reinterpret an ordinary directory as a regular file");
        const auto childPath = child.path;
        Check(DeleteFileW(child.File(L"untouched").c_str()) != FALSE, "retire the owned directory preparation child");
        child.handle.reset();
        Check(RemoveDirectoryW(childPath.c_str()) != FALSE, "retire the owned directory preparation fixture");
    }

    constexpr wchar_t outsideLeaf[] = L"panel-preparation-sentinel";
    if (!WriteByte(outside.File(outsideLeaf), 47))
    {
        Check(false, "create the outside-scope preparation sentinel");
        return;
    }
    const std::wstring invalid[] = {
        L"..\\outside\\panel-preparation-sentinel", unicodeName + L"\\..\\prepared-ascii",
        L"\\prepared-ascii", outside.File(outsideLeaf), L"prepared-ascii:stream",
        unicodeName + L"\\", unicodeName + L"\\\\prepared-ascii", L"prepared-ascii.",
        L"NUL.txt", std::wstring(L"prepared\0hidden", 15),
    };
    for (const auto& relative : invalid)
    {
        UacListing::LeafIdentity identity{};
        for (const bool parentOnly : {false, true})
            Check(job.QueryLeafIdentity(relative, ERROR_INVALID_NAME, identity, parentOnly) &&
                      HasByte(outside.File(outsideLeaf), 47),
                  "typed preparation rejects traversal, absolute paths and ambiguous names inside one live session");
        Check(job.QueryLeafIdentity(relative, ERROR_INVALID_NAME, identity, false, true) &&
                  HasByte(outside.File(outsideLeaf), 47),
              "directory preparation rejects traversal, absolute paths and ambiguous names inside one live session");
    }
    UacListing::LeafIdentity identity{};
    Check(job.QueryLeafIdentity(L"prepared-ascii", ERROR_SUCCESS, identity) &&
              HasByte(source.File(L"prepared-ascii"), 41),
          "rejected preparation paths do not consume or widen the valid panel scope");
    // The Python runner's ordinary temporary-path cleanup need not support these
    // extended paths. Retire exactly our nested fixtures through their wide names.
    bool cleaned = true;
    for (auto* directory : {&longLast, &longFirst, &unicode})
    {
        for (const auto leaf : {L"prepared-ascii", L"prepared-\u6771-\U0001f642"})
            cleaned = (DeleteFileW(directory->File(leaf).c_str()) != FALSE) && cleaned;
        directory->handle.reset();
        cleaned = (RemoveDirectoryW(directory->path.c_str()) != FALSE) && cleaned;
    }
    Check(cleaned, "remove only the owned Unicode and extended-length preparation fixtures");
}

void CheckPanelPreparationFlags(const Directory& source)
{
    const DWORD leaf = UacListing::leafIdentityRequest;
    const DWORD parent = UacListing::parentIdentityRequest;
    const DWORD directory = UacListing::directoryIdentityRequest;
    const DWORD invalid[] = {
        parent | 1, leaf, leaf | parent, leaf | 0x02000000 | 1,
        directory | 1, leaf | directory, leaf | parent | directory | 1,
        leaf | UacListing::contentRequest | 1,
        leaf | UacListing::inventoryRequest | 1,
        leaf | UacListing::sizeDetailsRequest | 1,
        leaf | parent | UacListing::contentRequest | 1,
        leaf | parent | UacListing::inventoryRequest | 1,
        leaf | parent | UacListing::sizeDetailsRequest | 1,
        leaf | directory | UacListing::contentRequest | 1,
        leaf | directory | UacListing::inventoryRequest | 1,
        leaf | directory | UacListing::sizeDetailsRequest | 1,
    };
    for (const auto header : invalid)
    {
        JobPipe job;
        Check(job.Open(nullptr, source.handle.get(), Operation::List) && job.RejectFindRequest(header),
              "the panel closes malformed or incompatible preparation flags without replying or reading a payload");
    }
    for (const auto scope : {Operation::FindContent, Operation::FindFile})
    {
        for (const auto flags : {leaf, leaf | parent, leaf | directory})
        {
            JobPipe job;
            Check(job.Open(nullptr, source.handle.get(), scope) && job.RejectFindRequest(flags | 1),
                  "Find grants cannot use a typed panel preparation request");
        }
    }
    JobPipe job;
    Check(job.Open(nullptr, source.handle.get(), Operation::List) &&
              job.RejectFindRequest(UacListing::contentRequest | 1),
          "metadata preparation does not add content access to a List grant");
}

void CheckPanelPreparationMutationGuards(const Directory& source)
{
    JobPipe job;
    UacListing::LeafIdentity identity{};
    constexpr wchar_t mkdirLeaf[] = L"prepared-mkdir-race-\u6771";
    if (!job.Open(nullptr, source.handle.get(), Operation::List) ||
        !job.QueryLeafIdentity(mkdirLeaf, ERROR_SUCCESS, identity, true) ||
        !WriteByte(source.File(mkdirLeaf), 53))
    {
        Check(false, "prepare a mkdir name claimed after its parent identity was returned");
        return;
    }
    // These explicit mutation calls model the later separately authorized operation.
    // No mutation request is sent on the read-only panel connection.
    auto parent = source.identity;
    parent.file.FileId = identity.parent;
    DWORD error = UacDirectoryLeaf::CreateVerifiedLeaf(source.handle.get(), parent, mkdirLeaf);
    Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && HasByte(source.File(mkdirLeaf), 53),
          "mkdir execution rechecks absence atomically and preserves a collision after parent preparation");
    constexpr wchar_t from[] = L"prepared-rename-\u6771";
    constexpr wchar_t to[] = L"prepared-rename-target-\u4eac";
    constexpr wchar_t saved[] = L"prepared-rename-original";
    if (!WriteByte(source.File(from), 59) || !job.QueryLeafIdentity(from, ERROR_SUCCESS, identity) ||
        !WriteByte(source.File(to), 61))
    {
        Check(false, "prepare a rename target claimed after source identity preparation");
        return;
    }
    parent.file.FileId = identity.parent;
    FILE_ID_INFO expected{parent.file.VolumeSerialNumber, identity.file};
    error = UacRenameLeaf::RenameVerifiedFile(source.handle.get(), parent, expected, from, to);
    Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) &&
              HasByte(source.File(from), 59) && HasByte(source.File(to), 61),
          "rename execution never overwrites a target that appeared after identity preparation");
    if (!MoveFileExW(source.File(from).c_str(), source.File(saved).c_str(), 0) ||
        !WriteByte(source.File(from), 67))
    {
        Check(false, "replace the prepared rename source while retaining the original file");
        return;
    }
    constexpr wchar_t freshTarget[] = L"prepared-rename-fresh-target";
    error = UacRenameLeaf::RenameVerifiedFile(source.handle.get(), parent, expected, from, freshTarget);
    Check(error == ERROR_FILE_INVALID && HasByte(source.File(from), 67) && HasByte(source.File(saved), 59) &&
              IsAbsent(source.File(freshTarget)),
          "rename execution rejects a replaced source ID without moving either object");
    Check(job.QueryLeafIdentity(from, ERROR_SUCCESS, identity) &&
              std::memcmp(&identity.file, &expected.FileId, sizeof(identity.file)) != 0,
          "a later preparation observes the replacement identity rather than caching the old source");
}

void CheckPanelMetadataGuards(const Directory& source)
{
    for (const bool directory : {false, true})
    {
        const std::wstring leaf = directory ? L"metadata-guard-dir-\u6771" : L"metadata-guard-file-\u6771";
        const std::wstring saved = leaf + L"-original";
        const auto path = source.File(leaf);
        const auto create = [&]() {
            return directory ? CreateDirectoryW(path.c_str(), nullptr) != FALSE : WriteByte(path, 79);
        };
        JobPipe listing;
        UacListing::LeafIdentity identity{};
        if (!create() || !listing.Open(nullptr, source.handle.get(), Operation::List) ||
            !listing.QueryLeafIdentity(leaf, ERROR_SUCCESS, identity, false, directory))
        {
            Check(false, "prepare a file or directory for metadata identity guard tests");
            continue;
        }
        auto parent = source.identity;
        parent.file.FileId = identity.parent;
        FILE_ID_INFO expected{parent.file.VolumeSerialNumber, identity.file};
        UacF7::MetadataChange change{132000000000000000LL, 0, 0, identity.attributes, FILE_ATTRIBUTE_ARCHIVE};
        FILE_BASIC_INFO before{}, after{};
        auto staleParent = parent;
        staleParent.file.FileId.Identifier[0] ^= 1;
        // These mutation helpers model the separately authorized write. The
        // listing request itself must never apply any part of MetadataChange.
        Check(ReadBasic(path, before) &&
                  UacMetadataLeaf::SetVerifiedMetadata(source.handle.get(), staleParent, expected, leaf, change) ==
                      ERROR_FILE_INVALID && ReadBasic(path, after) && SameBasic(before, after),
              "metadata execution rejects a stale parent identity before changing a file or directory");
        if (!SetFileAttributesW(path.c_str(), identity.attributes | FILE_ATTRIBUTE_HIDDEN))
        {
            Check(false, "change attributes after their verified preparation snapshot");
            continue;
        }
        Check(ReadBasic(path, before) &&
                  UacMetadataLeaf::SetVerifiedMetadata(source.handle.get(), parent, expected, leaf, change) ==
                      ERROR_FILE_INVALID && ReadBasic(path, after) && SameBasic(before, after),
              "metadata execution rejects changed expected attributes without partially writing timestamps");
        if (!listing.QueryLeafIdentity(leaf, ERROR_SUCCESS, identity, false, directory) ||
            !MoveFileExW(path.c_str(), source.File(saved).c_str(), 0) || !create() ||
            !SetFileAttributesW(path.c_str(), identity.attributes))
        {
            Check(false, "replace a prepared metadata target while keeping its original object");
            continue;
        }
        expected.FileId = identity.file;
        change.expectedAttributes = identity.attributes;
        FILE_BASIC_INFO savedBefore{}, savedAfter{};
        Check(ReadBasic(path, before) && ReadBasic(source.File(saved), savedBefore) &&
                  UacMetadataLeaf::SetVerifiedMetadata(source.handle.get(), parent, expected, leaf, change) ==
                      ERROR_FILE_INVALID && ReadBasic(path, after) && ReadBasic(source.File(saved), savedAfter) &&
                  SameBasic(before, after) && SameBasic(savedBefore, savedAfter),
              "metadata execution rejects a substituted target with matching attributes and preserves both objects");
        if (!listing.QueryLeafIdentity(leaf, ERROR_SUCCESS, identity, false, directory))
        {
            Check(false, "prepare the replacement metadata target explicitly");
            continue;
        }
        Check(std::memcmp(&identity.file, &expected.FileId, sizeof(identity.file)) != 0 &&
                  identity.attributes == before.FileAttributes,
              "new metadata preparation returns the replacement identity and its current attributes");
        expected.FileId = identity.file;
        change.expectedAttributes = identity.attributes;
        const DWORD desiredAttributes = FILE_ATTRIBUTE_ARCHIVE | (directory ? FILE_ATTRIBUTE_DIRECTORY : 0);
        Check(!UacMetadataLeaf::SetVerifiedMetadata(source.handle.get(), parent, expected, leaf, change) &&
                  ReadBasic(path, after) && after.CreationTime.QuadPart == change.created &&
                  after.FileAttributes == desiredAttributes &&
                  ReadBasic(source.File(saved), savedAfter) && SameBasic(savedBefore, savedAfter) &&
                  (directory || (HasByte(path, 79) && HasByte(source.File(saved), 79))),
              "a separately authorized metadata write applies to the newly prepared object and leaves the original intact");
    }

    constexpr wchar_t leaf[] = L"metadata-list-authority";
    JobPipe listing;
    UacListing::LeafIdentity identity{};
    if (!WriteByte(source.File(leaf), 83) || !listing.Open(nullptr, source.handle.get(), Operation::List) ||
        !listing.QueryLeafIdentity(leaf, ERROR_SUCCESS, identity))
    {
        Check(false, "prepare a live List connection for the mutation authorization boundary");
        return;
    }
    UacF7::MetadataChange change{132000000000000000LL, 0, 0, identity.attributes, FILE_ATTRIBUTE_HIDDEN};
    UacF7::MetadataJobPolicy policy{FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_ARCHIVE};
    DWORD error = ERROR_SUCCESS;
    FILE_BASIC_INFO before{}, after{};
    Check(ReadBasic(source.File(leaf), before) &&
              !listing.Request(Operation::Metadata, source, leaf, error, nullptr, &change, &policy) &&
              listing.Finish() != ERROR_SUCCESS && ReadBasic(source.File(leaf), after) && SameBasic(before, after) &&
              HasByte(source.File(leaf), 83),
          "a successful List identity query never authorizes a mutation frame on that read-only connection");
}

void CheckPanelPreparationReparse(const Directory& source, const Directory& outside)
{
    constexpr wchar_t fileLink[] = L"prepared-file-link-\u6771";
    constexpr wchar_t directoryLink[] = L"prepared-directory-link-\u6771";
    constexpr wchar_t referent[] = L"prepared-link-sentinel";
    if (!WriteByte(outside.File(referent), 71))
    {
        Check(false, "create the reparse preparation sentinel");
        return;
    }
    if (!CreateSymbolicLinkW(source.File(fileLink).c_str(), outside.File(referent).c_str(),
                            SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        const DWORD error = GetLastError();
        if (error == ERROR_PRIVILEGE_NOT_HELD)
            std::puts("SKIP: panel preparation link fixtures require Developer Mode or SeCreateSymbolicLinkPrivilege");
        else
            Check(false, "create the panel preparation file link");
        return;
    }
    if (!CreateSymbolicLinkW(source.File(directoryLink).c_str(), outside.path.c_str(),
                            SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        Check(false, "create the panel preparation directory link");
        DeleteFileW(source.File(fileLink).c_str());
        return;
    }
    JobPipe job;
    if (job.Open(nullptr, source.handle.get(), Operation::List))
    {
        UacListing::LeafIdentity identity{};
        DWORD status = ERROR_SUCCESS;
        Check(job.ReadLeafIdentity(fileLink, status, identity) && status != ERROR_SUCCESS &&
                  HasByte(outside.File(referent), 71),
              "plain-file rename preparation rejects a symbolic link without following its referent");
        const FILE_ID_128 zeroId{};
        Check(job.QueryLeafIdentity(fileLink, ERROR_SUCCESS, identity, true) &&
                  !std::memcmp(&identity.parent, &source.identity.file.FileId, sizeof(identity.parent)) &&
                  !std::memcmp(&identity.file, &zeroId, sizeof(identity.file)) && !identity.attributes &&
                  HasByte(outside.File(referent), 71),
              "parent-only preparation returns the local parent without opening an occupied symbolic-link leaf");
        FILE_BASIC_INFO before{}, after{};
        Check(ReadBasic(outside.path, before) &&
                  job.ReadLeafIdentity(directoryLink, status, identity, false, true) && status != ERROR_SUCCESS &&
                  ReadBasic(outside.path, after) && SameBasic(before, after),
              "directory metadata preparation rejects a directory link without changing the outside referent");
        Check(job.ReadLeafIdentity(fileLink, status, identity, false, true) && status != ERROR_SUCCESS &&
                  HasByte(outside.File(referent), 71),
              "directory metadata preparation also rejects a file symbolic link");
        constexpr wchar_t child[] = L"metadata-reparse-child";
        const auto outsideChild = outside.File(child);
        Check(CreateDirectoryW(outsideChild.c_str(), nullptr) &&
                  job.ReadLeafIdentity(std::wstring(directoryLink) + L"\\" + child, status, identity, false, true) &&
                  status != ERROR_SUCCESS && RemoveDirectoryW(outsideChild.c_str()),
              "directory metadata preparation cannot traverse an intermediate link to an existing outside directory");
        for (const bool parentOnly : {false, true})
            Check(job.ReadLeafIdentity(std::wstring(directoryLink) + L"\\" + referent,
                                       status, identity, parentOnly) && status != ERROR_SUCCESS &&
                      HasByte(outside.File(referent), 71),
                  "neither preparation type traverses a directory link beyond the approved root");
        Check(job.QueryLeafIdentity(L"prepared-ascii", ERROR_SUCCESS, identity),
              "reparse rejection leaves the read-only panel preparation session usable");
    }
    else
        Check(false, "open the reparse preparation session");
    Check(DeleteFileW(source.File(fileLink).c_str()) && RemoveDirectoryW(source.File(directoryLink).c_str()),
          "remove the test links without touching their outside-scope referents");
}

void CheckFindContentDisconnect(const Directory& first, const Directory& second)
{
    constexpr wchar_t leaf[] = L"\u6771-find-content";
    JobPipe interrupted, remaining;
    if (!WriteByte(first.File(leaf), 17) || !WriteByte(second.File(leaf), 23) ||
        !interrupted.Open(nullptr, first.handle.get()) || !remaining.Open(nullptr, second.handle.get()) ||
        !interrupted.OpenFindContent(leaf) || !remaining.OpenFindContent(leaf))
    {
        Check(false, "prepare two live scoped Find content streams");
        return;
    }
    Check(interrupted.Finish() == ERROR_BROKEN_PIPE, "a disconnected Find content stream ends its scoped session");
    Check(remaining.ReadAndCloseFindContent(23) && remaining.OpenFindContent(leaf) &&
              remaining.ReadAndCloseFindContent(23),
          "another Find session reads the correct file and remains usable after the disconnect");
}

void CheckStoppedFindReply()
{
    GUID id{};
    if (FAILED(CoCreateGuid(&id)))
    {
        Check(false, "prepare an unresponsive Find reply");
        return;
    }
    const auto name = PipeName(id);
    auto server = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 4096, 4096, 0, nullptr));
    const auto stop = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const auto peer = server ? Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING,
                                                FILE_FLAG_OVERLAPPED, nullptr)) : Handle{};
    const auto deadline = GetTickCount64() + timeoutMs;
    if (!stop || !peer || Connect(server.get(), deadline))
    {
        Check(false, "prepare an unresponsive Find reply");
        return;
    }

    DWORD peerError = ERROR_GEN_FAILURE;
    std::thread broker([&] {
        DWORD pathSize = MAXDWORD;
        peerError = TransferRequest(peer.get(), &pathSize, sizeof(pathSize), false, deadline);
        if (peerError || pathSize != 0 || !SetEvent(stop.get()))
            return;
        BYTE extra = 0;
        peerError = TransferRequest(peer.get(), &extra, sizeof(extra), false, deadline);
    });
    DWORD root = 0;
    DWORD error = TransferRequest(server.get(), &root, sizeof(root), true, deadline);
    ListingReply reply{};
    if (!error)
        error = TransferRequest(server.get(), &reply, sizeof(reply), false, deadline, nullptr, nullptr, nullptr, stop.get());
    server.reset();
    broker.join();
    Check(error == ERROR_CANCELLED && peerError == ERROR_BROKEN_PIPE,
          "stop an active Find listing wait and close its pipe without a transfer cancellation byte");
}

void CheckExactFindScope(const Directory& source)
{
    const auto path = source.File(L"exact-find-file");
    if (!WriteByte(path, 37))
    {
        Check(false, "prepare an exact-file Find source");
        return;
    }
    const auto file = Adopt(CreateFileW(path.c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                                        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (!file)
    {
        Check(false, "pin the exact-file Find source");
        return;
    }
    {
        JobPipe exact;
        Check(exact.Open(nullptr, file.get(), Operation::FindFile) && exact.OpenFindContent({}) &&
                  exact.ReadAndCloseFindContent(37),
              "an exact-file Find grant returns bytes from its pinned file");
    }
    const std::pair<DWORD, const char*> rejected[] = {
        {0, "an exact-file Find grant rejects directory listing without a reply"},
        {UacListing::inventoryRequest, "an exact-file Find grant rejects inventory without a reply"},
        {UacListing::contentRequest | 1, "an exact-file Find grant rejects named content without a reply"},
    };
    for (const auto& [header, reason] : rejected)
    {
        JobPipe exact;
        Check(exact.Open(nullptr, file.get(), Operation::FindFile) && exact.RejectFindRequest(header), reason);
    }
    JobPipe exact;
    Check(exact.Open(nullptr, file.get(), Operation::FindFile) && exact.OpenFindContent({}) &&
              exact.RejectFindRange({1, 1, 0}),
          "an exact-file Find grant closes the pipe without returning bytes beyond EOF");
}

void CheckCopyJob(const Directory& source, const Directory& target, const Directory& outside)
{
    JobPipe job;
    constexpr wchar_t collision[] = L"exists";
    constexpr wchar_t second[] = L"\u6771-second";
    constexpr wchar_t third[] = L"outside";
    if (!WriteByte(source.File(collision), 1) || !WriteByte(target.File(collision), 9) ||
        !WriteByte(source.File(second), 2) || !WriteByte(source.File(third), 3) || !job.Open())
    {
        Check(false, "prepare the real NTFS copy job");
        return;
    }
    DWORD error = ERROR_GEN_FAILURE;
    Check(job.Request(Operation::Copy, source, collision, error, &target) &&
              (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && HasByte(target.File(collision), 9),
          "a scoped copy reports the collision and preserves the existing destination");
    Check(job.Request(Operation::Copy, source, second, error, &target) && !error && HasByte(target.File(second), 2),
          "the same broker accepts a later eligible item after the collision");
    Check(!job.Request(Operation::Copy, source, third, error, &outside) && job.Finish() == ERROR_ACCESS_DENIED &&
              IsAbsent(outside.File(third)) && HasByte(source.File(third), 3),
          "a different destination parent is refused before mutation");
}

void CheckCopyOptionScope(const Directory& source, const Directory& target)
{
    constexpr wchar_t approved[] = L"copy-option-approved";
    constexpr wchar_t pending[] = L"copy-option-pending";
    if (!WriteByte(source.File(approved), 41) || !WriteByte(source.File(pending), 43))
    {
        Check(false, "prepare files for checking changes to approved copy options");
        return;
    }
    const std::pair<Operation, const char*> differentOptions[] = {
        {Operation::CopyIgnoreAds, "ordinary copy consent cannot omit named streams in a later item"},
        {Operation::CopyCompressedSecurity, "ordinary copy consent cannot add compression and security in a later item"},
    };
    for (const auto& [operation, reason] : differentOptions)
    {
        JobPipe job;
        DWORD error = ERROR_GEN_FAILURE;
        const bool alreadyCopied = !IsAbsent(target.File(approved));
        const bool ordinaryCopy = job.Open() && job.Request(Operation::Copy, source, approved, error, &target) &&
            (alreadyCopied ? (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) : error == ERROR_SUCCESS);
        Check(ordinaryCopy && !job.Request(operation, source, pending, error, &target) &&
                  job.Finish() == ERROR_ACCESS_DENIED && HasByte(target.File(approved), 41) &&
                  HasByte(source.File(pending), 43) && IsAbsent(target.File(pending)),
              reason);
    }
}

void CheckCompetingCopyJobs(const Directory& first, const Directory& second, const Directory& target)
{
    constexpr wchar_t leaf[] = L"\u6771-shared-target";
    const auto start = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    JobPipe firstJob, secondJob;
    if (!start || !WriteByte(first.File(leaf), 6) || !WriteByte(second.File(leaf), 7) ||
        !firstJob.Open() || !secondJob.Open())
    {
        Check(false, "prepare concurrent copy jobs for one destination");
        return;
    }

    DWORD firstError = ERROR_GEN_FAILURE, secondError = ERROR_GEN_FAILURE;
    bool firstReply = false, secondReply = false;
    std::thread firstRequest([&] {
        firstReply = WaitForSingleObject(start.get(), timeoutMs) == WAIT_OBJECT_0 &&
                     firstJob.Request(Operation::Copy, first, leaf, firstError, &target);
    });
    std::thread secondRequest([&] {
        secondReply = WaitForSingleObject(start.get(), timeoutMs) == WAIT_OBJECT_0 &&
                      secondJob.Request(Operation::Copy, second, leaf, secondError, &target);
    });
    const bool started = SetEvent(start.get()) != FALSE;
    firstRequest.join();
    secondRequest.join();

    const auto isCollision = [](DWORD error) {
        return error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS;
    };
    const bool firstWon = firstError == ERROR_SUCCESS && isCollision(secondError) && HasByte(target.File(leaf), 6);
    const bool secondWon = secondError == ERROR_SUCCESS && isCollision(firstError) && HasByte(target.File(leaf), 7);
    Check(started && firstReply && secondReply && (firstWon || secondWon) &&
              HasByte(first.File(leaf), 6) && HasByte(second.File(leaf), 7),
          "two scoped copy jobs publish exactly one destination and preserve both sources");
}

void CheckActiveOverwriteJournal(const Directory& source, const Directory& target)
{
    constexpr wchar_t leaf[] = L"locked-overwrite-journal";
    const auto journalName = DiskOverwriteJournalName(target.identity);
    auto journal = Adopt(CreateFileW(journalName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    JobPipe job;
    if (!journal || !WriteByte(source.File(leaf), 47) || !job.Open())
    {
        Check(false, "prepare a copy while a competing overwrite holds the parent journal");
        return;
    }

    DWORD error = ERROR_GEN_FAILURE;
    Check(job.Request(Operation::Copy, source, leaf, error, &target) &&
              error == ERROR_SHARING_VIOLATION && IsAbsent(target.File(leaf)) && HasByte(source.File(leaf), 47),
          "an already locked overwrite journal blocks publication by a competing broker");
    journal.reset();
    Check(DeleteFileW(journalName.c_str()) &&
              job.Request(Operation::Copy, source, leaf, error, &target) && !error &&
              HasByte(target.File(leaf), 47) && HasByte(source.File(leaf), 47),
          "the same job can copy after the competing journal is released");
}

void CheckLateOverwriteJournal(const Directory& source, const Directory& target)
{
    constexpr wchar_t leaf[] = L"late-overwrite-journal";
    const auto path = target.File(leaf);
    const auto journalName = DiskOverwriteJournalName(target.identity);
    const auto staged = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const auto resume = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const auto backupRenamed = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const auto finishReplacement = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const UacCopyLeaf::Detail::CopyPause pause{staged.get(), resume.get()};
    const UacCopyLeaf::Detail::CopyPause replacementPause{backupRenamed.get(), finishReplacement.get()};
    JobPipe job;
    if (!staged || !resume || !backupRenamed || !finishReplacement || !WriteByte(source.File(leaf), 51) ||
        !job.Open(nullptr, nullptr, Operation::FindContent, &pause))
    {
        Check(false, "prepare a copy that pauses after passing the initial journal check");
        return;
    }

    DWORD error = ERROR_GEN_FAILURE;
    bool replied = false;
    std::thread copy([&] { replied = job.Request(Operation::Copy, source, leaf, error, &target); });
    const bool copying = WaitForSingleObject(staged.get(), timeoutMs) == WAIT_OBJECT_0;
    UacZipUpdate::PublishResult publication{ERROR_GEN_FAILURE};
    std::thread overwrite;
    if (copying)
    {
        overwrite = std::thread([&] {
            const ComApartment com;
            const InstallLocation install(Adopt(CreateFileW(L".", FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)));
            Handle journal;
            if (FAILED(com.result) || !install.DirectoryHandle() || !WriteByte(path, 61) ||
                OpenProtectedJournal(install, journalName, FILE_OPEN_IF, true, journal))
                return;
            const RemoveEmptyZipJournal retire{journal};
            HANDLE raw = nullptr;
            if (UacZipUpdate::Detail::Open(target.handle.get(), leaf, raw, DELETE))
                return;
            UacZipUpdate::Handle original(raw, CloseHandle);
            UacF7::ArchiveRevision revision{};
            std::wstring stageName;
            if (UacZipUpdate::ReadRevision(original.get(), revision) ||
                UacCopyLeaf::Detail::StageFile(target.handle.get(), raw, stageName, false, nullptr, true))
                return;
            const auto stage = Adopt(raw);
            const BYTE replacement = 71;
            DWORD written = 0;
            if (!WriteFile(stage.get(), &replacement, sizeof(replacement), &written, nullptr) ||
                written != sizeof(replacement))
                return;
            UacRenameLeaf::Detail::activeReplacementPause = &replacementPause;
            publication = UacZipUpdate::Publish(target.handle.get(), target.identity, leaf, revision, original,
                stage.get(), journal.get(), nullptr, MAXULONGLONG, nullptr, UacZipUpdate::ReplacementKind::DiskCopy);
        });
    }
    const bool inGap = copying && WaitForSingleObject(backupRenamed.get(), timeoutMs) == WAIT_OBJECT_0 &&
        IsAbsent(path);
    const bool released = SetEvent(resume.get()) != FALSE;
    copy.join();
    const bool copyRefused = inGap && released && replied && error == ERROR_SHARING_VIOLATION && IsAbsent(path);
    SetEvent(finishReplacement.get());
    if (overwrite.joinable())
        overwrite.join();
    Check(copyRefused && HasByte(source.File(leaf), 51),
          "an actual late overwrite keeps the staged copy out of the temporary name gap");
    Check(!publication.error && !publication.recoveryError && HasByte(path, 71) && IsAbsent(journalName),
          "the competing overwrite completes and clears the journal");
    Check(job.Request(Operation::Copy, source, leaf, error, &target) &&
              (error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && HasByte(path, 71),
          "the original copy session sees the newly published file as an ordinary collision");
}

void CheckStaleOverwriteJob(const Directory& source, const Directory& target)
{
    constexpr wchar_t leaf[] = L"\u6771-overwrite";
    const auto targetPath = target.File(leaf);
    const auto formerPath = target.File(L"former-overwrite");
    UacF7::ArchiveRevision approved{};
    const auto readRevision = [&] {
        const auto file = Adopt(CreateFileW(targetPath.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                                            nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        return file && !UacZipUpdate::ReadRevision(file.get(), approved);
    };
    JobPipe job;
    if (!WriteByte(source.File(leaf), 8) || !WriteByte(targetPath, 9) || !readRevision() ||
        !MoveFileExW(targetPath.c_str(), formerPath.c_str(), 0) || !WriteByte(targetPath, 10) || !job.Open())
    {
        Check(false, "prepare a replaced overwrite target after capturing its revision");
        return;
    }

    DWORD error = ERROR_GEN_FAILURE;
    Check(job.Request(Operation::CopyOverwrite, source, leaf, error, &target,
                      nullptr, nullptr, false, nullptr, &approved) &&
              error == ERROR_FILE_INVALID && HasByte(targetPath, 10) && HasByte(formerPath, 9) && HasByte(source.File(leaf), 8),
          "an overwrite approval cannot replace a newly substituted file");
    Check(readRevision() && SetFileAttributesW(targetPath.c_str(), FILE_ATTRIBUTE_HIDDEN) &&
              job.Request(Operation::CopyOverwrite, source, leaf, error, &target,
                          nullptr, nullptr, false, nullptr, &approved) &&
              error == ERROR_FILE_INVALID && HasByte(targetPath, 10) &&
              (GetFileAttributesW(targetPath.c_str()) & FILE_ATTRIBUTE_HIDDEN),
          "the same broker rejects a revised target with the same file identity");
}

void CheckCancelledCopyJob(const Directory& source, const Directory& outside, const Directory& target)
{
    const auto ready = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const auto staged = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const auto resume = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    const UacCopyLeaf::Detail::CopyPause pause{staged.get(), resume.get()};
    JobPipe job, independent;
    constexpr wchar_t pending[] = L"pending-cancel";
    constexpr wchar_t other[] = L"other-active-copy";
    constexpr wchar_t next[] = L"other-next-copy";
    if (!ready || !staged || !resume || !WriteByte(source.File(pending), 4) ||
        !WriteByte(outside.File(other), 71) || !WriteByte(outside.File(next), 72) ||
        !job.Open(ready.get()) || !independent.Open(nullptr, nullptr, Operation::FindContent, &pause))
    {
        Check(false, "prepare cancellable and independent copy jobs");
        return;
    }

    DWORD independentError = ERROR_GEN_FAILURE;
    bool independentReply = false;
    std::thread copy([&] { independentReply = independent.Request(Operation::Copy, outside, other, independentError, &target); });
    const bool otherStaged = WaitForSingleObject(staged.get(), timeoutMs) == WAIT_OBJECT_0;
    DWORD error = ERROR_GEN_FAILURE;
    const bool cancelled = job.Request(Operation::Copy, source, pending, error, &target, nullptr, nullptr, false, ready.get());
    Check(cancelled && error == ERROR_CANCELLED &&
              !job.Request(Operation::Copy, source, pending, error, &target) && job.Finish() == ERROR_SUCCESS &&
              IsAbsent(target.File(pending)) && HasByte(source.File(pending), 4),
          "a cancel control keeps the source, leaves no target and ends the scoped broker job");

    const bool otherUnpublished = IsAbsent(target.File(other));
    const bool resumed = SetEvent(resume.get()) != FALSE;
    copy.join();
    Check(otherStaged && otherUnpublished && resumed && independentReply && independentError == ERROR_SUCCESS &&
              HasByte(target.File(other), 71) && HasByte(outside.File(other), 71),
          "cancelling a different live job leaves the staged copy and its source intact");
    Check(independent.Request(Operation::Copy, outside, next, independentError, &target) &&
              independentError == ERROR_SUCCESS && HasByte(target.File(next), 72) && HasByte(outside.File(next), 72) &&
              IsAbsent(target.File(pending)) && independent.CancelIdle() == ERROR_SUCCESS,
          "the independent job retains its scope for another item and ends cleanly");
}

void CheckLateCopyCancellation(const Directory& source, const Directory& target)
{
    JobPipe job;
    constexpr wchar_t finished[] = L"late-cancel";
    if (!WriteByte(source.File(finished), 5) || !job.Open())
    {
        Check(false, "prepare cancellation after publication");
        return;
    }
    DWORD error = ERROR_GEN_FAILURE;
    Check(job.Request(Operation::Copy, source, finished, error, &target) && !error &&
              job.CancelIdle() == ERROR_SUCCESS && HasByte(target.File(finished), 5) && HasByte(source.File(finished), 5),
          "a late cancel ends the job cleanly and preserves the already published copy");
}

void CheckMoveSourceEligibility()
{
    constexpr DWORD plain = FILE_ATTRIBUTE_ARCHIVE;
    constexpr DWORD sparse = plain | FILE_ATTRIBUTE_SPARSE_FILE;
    constexpr DWORD compressed = plain | FILE_ATTRIBUTE_COMPRESSED;
    Check(UacF7::IsEligibleMoveFile(plain, false, false) && UacF7::IsEligibleMoveFile(sparse, false, false) &&
              UacF7::IsEligibleMoveFile(compressed, true, false) &&
              !UacF7::IsEligibleMoveFile(compressed, false, false),
          "admit plain and sparse move sources and require the explicit compression option");
    Check(!UacF7::IsEligibleMoveFile(sparse, true, false) && !UacF7::IsEligibleMoveFile(plain, false, true) &&
              !UacF7::IsEligibleMoveFile(plain | FILE_ATTRIBUTE_ENCRYPTED, false, false) &&
              !UacF7::IsEligibleMoveFile(plain | FILE_ATTRIBUTE_REPARSE_POINT, false, false),
          "refuse unsupported metadata and link combinations at move preparation");
}

void CheckRejectedVolumeAliases(const Directory& source, const Directory& target)
{
    constexpr wchar_t leaf[] = L"nonlocal-volume";
    if (!WriteByte(source.File(leaf), 4))
    {
        Check(false, "prepare the volume-alias fixture");
        return;
    }

    const auto check = [&](Operation operation, const Directory& original, const wchar_t* volume,
                           bool aliasSource, const char* message) {
        Directory alias;
        alias.identity = original.identity;
        alias.identity.volume = volume;
        alias.identity.volume.resize(volumeLength, L'x');
        alias.relative = original.relative;
        alias.path = original.path; // Query the real file ID before substituting the wire volume.
        JobPipe job;
        DWORD error = ERROR_GEN_FAILURE;
        Check(job.Open() && job.Request(operation, aliasSource ? alias : source, leaf, error,
                                        aliasSource ? &target : &alias) &&
                  error == ERROR_NOT_SUPPORTED && HasByte(source.File(leaf), 4) && IsAbsent(target.File(leaf)),
              message);
    };

    check(Operation::Copy, source, LR"(\\localhost\sample\)", true,
          "refuse a UNC source volume even with valid local file and parent IDs");
    check(Operation::Copy, target, LR"(\\?\UNC\localhost\sample\)", false,
          "refuse an extended UNC destination before publication");
    check(Operation::MoveAcrossVolume, target, LR"(\\?\GLOBALROOT\Device\HarddiskVolume1)", false,
          "refuse a device namespace destination without retiring the source");
}

void CheckTransferReplies()
{
    const auto oneShotMove = UacF7::TransferReply(Operation::MoveAcrossVolume, ERROR_PARTIAL_COPY);
    const auto oneShotCompressed = UacF7::TransferReply(Operation::MoveCompressed, ERROR_PARTIAL_COPY);
    const auto lateMove = UacF7::TransferReply(Operation::FinishCrossVolumeMove, ERROR_PARTIAL_COPY);
    Check(oneShotMove.unknown && oneShotMove.error == ERROR_PARTIAL_COPY &&
              oneShotCompressed.unknown && lateMove.unknown,
          "retain a partial result for every cross-volume move after the one-shot done frame");
    const auto encrypted = UacF7::TransferReply(Operation::CopyEncrypted, ERROR_PARTIAL_COPY);
    Check(encrypted.unknown && !UacF7::CanContinueTransferJob(encrypted.error),
          "retain encrypted copy uncertainty and stop the job on a partial reply");
    const auto cancelled = UacF7::TransferReply(Operation::MoveAcrossVolume, ERROR_CANCELLED, true);
    const auto collision = UacF7::TransferReply(Operation::MoveAcrossVolume, ERROR_ALREADY_EXISTS);
    Check(!cancelled.unknown && cancelled.error == ERROR_CANCELLED && !UacF7::CanContinueTransferJob(cancelled.error) &&
              !collision.unknown && collision.error == ERROR_ALREADY_EXISTS && UacF7::CanContinueTransferJob(collision.error),
          "distinguish a confirmed cancellation or collision from a partial move");
}

void CheckUnboundedTransferCancellation(Operation operation)
{
    GUID id{};
    if (FAILED(CoCreateGuid(&id)))
    {
        Check(false, "prepare the cancellable unbounded transfer");
        return;
    }
    const auto name = PipeName(id);
    const auto server = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
        FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS,
        1, 4096, 4096, 0, nullptr));
    const auto cancel = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!server || !cancel)
    {
        Check(false, "prepare the cancellable unbounded transfer");
        return;
    }
    DWORD peerError = ERROR_GEN_FAILURE;
    std::thread receiver([&] {
        const auto peer = Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
        if (!peer)
            return;
        BYTE signal = 0;
        const auto deadline = GetTickCount64() + timeoutMs;
        const auto sendProgress = [&](ULONGLONG bytes) {
            DWORD marker = transferProgressFrame;
            const DWORD error = TransferRequest(peer.get(), &marker, sizeof(marker), true, deadline);
            TransferProgressUpdate update{bytes, 0};
            return error ? error : TransferRequest(peer.get(), &update, sizeof(update), true, deadline);
        };
        peerError = sendProgress(4ULL * 1024 * 1024 * 1024 + 1);
        if (!peerError)
            peerError = TransferRequest(peer.get(), &signal, sizeof(signal), false, deadline);
        if (!peerError && signal != UacRenameLeaf::Detail::transferCancelSignal)
            peerError = ERROR_INVALID_DATA;
        if (!peerError)
        {
            peerError = sendProgress(4ULL * 1024 * 1024 * 1024 + 2);
        }
        if (!peerError)
        {
            DWORD status = ERROR_CANCELLED;
            peerError = TransferRequest(peer.get(), &status, sizeof(status), true, deadline);
        }
    });
    DWORD status = ERROR_GEN_FAILURE;
    DWORD error = Connect(server.get(), GetTickCount64() + timeoutMs);
    struct Progress
    {
        HANDLE cancel;
        ULONGLONG bytes = 0;
        unsigned updates = 0;
    } progress{cancel.get()};
    if (!error)
        error = ReadTransferResult(server.get(), status, MAXULONGLONG, cancel.get(),
            [](void* context, ULONGLONG bytes, ULONGLONG) {
                auto& progress = *static_cast<Progress*>(context);
                progress.bytes = bytes;
                ++progress.updates;
                SetEvent(progress.cancel);
            }, &progress, operation);
    receiver.join();
    Check(!error && !peerError && status == ERROR_CANCELLED &&
              progress.updates == 1 && progress.bytes == 4ULL * 1024 * 1024 * 1024 + 1,
          "read 64-bit progress, send one cancellation, and wait for the terminal reply");
}

void CheckMalformedTransferProgress()
{
    const auto check = [](Operation operation, std::initializer_list<TransferProgressUpdate> updates,
                          size_t expectedCallbacks, bool truncateLast, bool subscribe, const char* message) {
        GUID id{};
        if (FAILED(CoCreateGuid(&id)))
        {
            Check(false, message);
            return;
        }
        const auto name = PipeName(id);
        const auto host = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 4096, 4096, 0, nullptr));
        const auto peer = host ? Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING,
                                                    FILE_FLAG_OVERLAPPED, nullptr)) : Handle{};
        const auto deadline = GetTickCount64() + timeoutMs;
        if (!peer || Connect(host.get(), deadline))
        {
            Check(false, message);
            return;
        }

        size_t frames = 0;
        for (auto update : updates)
        {
            DWORD marker = transferProgressFrame;
            const DWORD length = truncateLast && frames + 1 == updates.size() ? sizeof(update.bytes) : sizeof(update);
            if (TransferRequest(peer.get(), &marker, sizeof(marker), true, deadline) ||
                TransferRequest(peer.get(), &update, length, true, deadline))
            {
                Check(false, message);
                return;
            }
            ++frames;
        }

        size_t callbacks = 0;
        DWORD result = ERROR_GEN_FAILURE;
        const UacF7::TransferProgress countUpdate = [](void* context, ULONGLONG, ULONGLONG) {
            ++*static_cast<size_t*>(context);
        };
        const DWORD error = ReadTransferResult(host.get(), result, deadline, nullptr,
                                               subscribe ? countUpdate : nullptr, &callbacks, operation);
        Check(error == ERROR_INVALID_DATA && callbacks == expectedCallbacks && result == transferProgressFrame, message);
    };

    check(Operation::CopyTree, {{1, 10}, {1, 10}}, 1, false, true, "reject repeated transfer progress");
    check(Operation::CopyTree, {{2, 10}, {1, 10}}, 1, false, true, "reject decreasing transfer progress");
    check(Operation::CopyTree, {{1, 10}, {2, 11}}, 1, false, true, "reject a changed total within one transfer");
    check(Operation::CopyTree, {{1, 0}}, 0, false, true, "require an explicit total for tree progress");
    check(Operation::CopyEncrypted, {{2, 1}}, 0, false, true, "reject encrypted progress beyond its total");
    check(Operation::CopyOverwrite, {{1, 2}}, 0, false, true, "reject an unexpected explicit total for overwrite progress");
    check(Operation::MoveAcrossVolume, {{UacF7::maxCrossVolumeMoveBytes + 1, 0}}, 0, false, true,
          "reject progress beyond the representable F6 range");
    check(Operation::CopyTree, {{1, 10}}, 0, true, true, "reject a truncated progress frame");
    check(Operation::CopyTree, {{1, 10}}, 0, false, false, "reject unsolicited progress frames");
}

void CheckRequestDelivery()
{
    GUID id{};
    if (FAILED(CoCreateGuid(&id)))
    {
        Check(false, "create a request-delivery pipe name");
        return;
    }
    const auto name = PipeName(id);
    const auto sender = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 4096, 4096, 0, nullptr));
    auto receiver = sender ? Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING,
                                                     FILE_FLAG_OVERLAPPED, nullptr)) : Handle{};
    const auto deadline = GetTickCount64() + timeoutMs;
    if (!receiver || Connect(sender.get(), deadline))
    {
        Check(false, "open the request-delivery pipe");
        return;
    }

    RequestHeader header{};
    header.operation = Operation::WriteZipUpdate;
    std::wstring payload = L"fixture.zip";
    UacF7::Result delivery;
    const auto roundTrip = [&](void* extra, DWORD extraBytes, UacF7::MetadataJobPolicy* policy, bool more) {
        RequestHeader received{};
        std::wstring path(payload.size(), L'\0');
        return !SendOperationRequest(sender.get(), header, payload, extra, extraBytes, policy, more, deadline, delivery) &&
               !TransferRequest(receiver.get(), &received, sizeof(received), false, deadline) &&
               received.operation == header.operation &&
               !TransferRequest(receiver.get(), path.data(), static_cast<DWORD>(path.size() * sizeof(wchar_t)), false, deadline) &&
               path == payload;
    };
    Check(roundTrip(nullptr, 0, nullptr, true) && !delivery.unknown,
          "a ZIP update request prefix cannot publish before the commit frame");
    header.operation = Operation::Metadata;
    UacF7::MetadataChange change{};
    UacF7::MetadataJobPolicy policy{FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_HIDDEN};
    UacF7::MetadataJobPolicy receivedPolicy{};
    UacF7::MetadataChange receivedChange{};
    Check(roundTrip(&change, sizeof(change), &policy, false) && delivery.unknown &&
              !TransferRequest(receiver.get(), &receivedChange, sizeof(receivedChange), false, deadline) &&
              !TransferRequest(receiver.get(), &receivedPolicy, sizeof(receivedPolicy), false, deadline) &&
              receivedPolicy.set == policy.set && receivedPolicy.clear == policy.clear,
          "the metadata policy is the last frame before a scoped mutation can run");

    UacF7::Result rejected;
    Check(SendOperationRequest(sender.get(), header, payload, &change, sizeof(change), &policy,
                               false, GetTickCount64(), rejected) == ERROR_TIMEOUT && !rejected.unknown,
          "an expired deadline before the header leaves the mutation unsubmitted");

    receiver.reset();
    DWORD reply = ERROR_SUCCESS;
    Check(TransferRequest(sender.get(), &reply, sizeof(reply), false, deadline) == ERROR_BROKEN_PIPE && delivery.unknown,
          "a disconnect after the complete mutation request leaves the result unconfirmed");
    Check(SendOperationRequest(sender.get(), header, payload, &change, sizeof(change), &policy,
                               false, deadline, rejected) != ERROR_SUCCESS && !rejected.unknown,
          "a disconnect before the next request header leaves that item unsubmitted");
}

void CheckCrossVolumeBoundary(const Directory& source, const Directory& target)
{
    JobPipe job;
    constexpr wchar_t leaf[] = L"cross-volume";
    if (!WriteByte(source.File(leaf), 5) || !job.Open())
    {
        Check(false, "prepare the cross-volume boundary fixture");
        return;
    }
    DWORD error = ERROR_GEN_FAILURE;
    Check(job.Request(Operation::MoveAcrossVolume, source, leaf, error, &target) &&
              error == ERROR_NOT_SAME_DEVICE && HasByte(source.File(leaf), 5) && IsAbsent(target.File(leaf)),
          "reject the cross-volume command on the same NTFS volume before modifying either parent");
    JobPipe boundary;
    constexpr ULONGLONG aboveFourGiB = (1ULL << 32) + 1;
    Check(boundary.Open() && boundary.Request(Operation::MoveAcrossVolume, source, leaf, error, &target,
              nullptr, nullptr, false, nullptr, nullptr, aboveFourGiB) &&
              error == ERROR_NOT_SAME_DEVICE && HasByte(source.File(leaf), 5) && IsAbsent(target.File(leaf)),
          "accept a move request beyond the former 4 GiB limit");
    Check(TransferDeadline(Operation::MoveAcrossVolume) == MAXULONGLONG &&
              TransferDeadline(Operation::MoveCompressed) == MAXULONGLONG &&
              TransferDeadline(Operation::FinishCrossVolumeMove) == MAXULONGLONG &&
              TransferDeadline(Operation::Copy) != MAXULONGLONG,
          "wait without a total deadline only for active cross-volume moves");
    JobPipe oversized;
    Check(oversized.Open() && !oversized.Request(Operation::MoveAcrossVolume, source, leaf, error, &target,
              nullptr, nullptr, false, nullptr, nullptr, UacF7::maxCrossVolumeMoveBytes + 1) &&
              oversized.Finish() == ERROR_INVALID_DATA && HasByte(source.File(leaf), 5) && IsAbsent(target.File(leaf)),
          "reject F6 above its limit before mutation");
    JobPipe copyLimit;
    Check(copyLimit.Open() && !copyLimit.Request(Operation::Copy, source, leaf, error, &target,
              nullptr, nullptr, false, nullptr, nullptr, UacF7::maxPilotCopyBytes + 1) &&
              copyLimit.Finish() == ERROR_INVALID_DATA && HasByte(source.File(leaf), 5) && IsAbsent(target.File(leaf)),
          "keep the existing limit for F5 requests");
    JobPipe compressed;
    Check(compressed.Open() && compressed.Request(Operation::MoveCompressed, source, leaf, error, &target) &&
              error == ERROR_NOT_SAME_DEVICE && HasByte(source.File(leaf), 5) && IsAbsent(target.File(leaf)),
          "reject a compressed cross-volume move on the same volume before mutation");
    JobPipe completion;
    UacF7::ArchiveRevision revision{};
    if (!WriteByte(target.File(leaf), 5) || !completion.Open())
    {
        Check(false, "prepare the existing-copy boundary fixture");
        return;
    }
    const auto file = Adopt(CreateFileW(target.File(leaf).c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    Check(file && !UacZipUpdate::ReadRevision(file.get(), revision) &&
              completion.Request(Operation::FinishCrossVolumeMove, source, leaf, error, &target,
                                 nullptr, nullptr, false, nullptr, &revision) &&
              error == ERROR_NOT_SAME_DEVICE && HasByte(source.File(leaf), 5) && HasByte(target.File(leaf), 5),
          "consume the late move target revision and reject a same-volume completion without changing either file");
}

void CheckMetadataJob(const Directory& source)
{
    JobPipe job;
    const wchar_t* first = L"meta-first";
    const wchar_t* second = L"meta-second";
    if (!WriteByte(source.File(first), 1) || !WriteByte(source.File(second), 2) ||
        !SetFileAttributesW(source.File(first).c_str(), FILE_ATTRIBUTE_HIDDEN) || !job.Open())
    {
        Check(false, "prepare the real NTFS metadata job");
        return;
    }
    FILE_BASIC_INFO initial{};
    if (!ReadBasic(source.File(first), initial))
    {
        Check(false, "read the initial metadata");
        return;
    }
    UacF7::MetadataJobPolicy policy{FILE_ATTRIBUTE_ARCHIVE, FILE_ATTRIBUTE_HIDDEN};
    UacF7::MetadataChange change{132000000000000000LL, 0, 0, initial.FileAttributes, FILE_ATTRIBUTE_ARCHIVE};
    DWORD error = ERROR_GEN_FAILURE;
    FILE_BASIC_INFO final{};
    Check(job.Request(Operation::Metadata, source, first, error, nullptr, &change, &policy) && !error &&
              ReadBasic(source.File(first), final) && final.FileAttributes == FILE_ATTRIBUTE_ARCHIVE &&
              final.CreationTime.QuadPart == change.created,
          "apply the first approved metadata request to its exact NTFS item");
    if (!ReadBasic(source.File(second), initial))
    {
        Check(false, "read the second initial metadata");
        return;
    }
    change.expectedAttributes = initial.FileAttributes;
    change.attributes = (initial.FileAttributes & UacF7::basicAttributes) == FILE_ATTRIBUTE_ARCHIVE ? 0 : FILE_ATTRIBUTE_ARCHIVE;
    Check(job.Request(Operation::Metadata, source, second, error, nullptr, &change, &policy) && !error &&
              ReadBasic(source.File(second), final) && final.CreationTime.QuadPart == change.created,
          "reuse metadata consent for an item with different starting attributes");
    change.expectedAttributes = FILE_ATTRIBUTE_ARCHIVE;
    change.attributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY;
    Check(!job.Request(Operation::Metadata, source, second, error, nullptr, &change, &policy) &&
              job.Finish() == ERROR_ACCESS_DENIED && ReadBasic(source.File(second), final) &&
              final.FileAttributes == FILE_ATTRIBUTE_ARCHIVE,
          "reject an attribute bit outside the consent before changing the item");
}

void CheckDeleteJob(const Directory& source)
{
    JobPipe job;
    const wchar_t* first = L"delete-first";
    const wchar_t* second = L"delete-second";
    const wchar_t* third = L"delete-interrupted";
    if (!WriteByte(source.File(first), 1) || !WriteByte(source.File(second), 2) || !WriteByte(source.File(third), 3) || !job.Open())
    {
        Check(false, "prepare the real NTFS deletion job");
        return;
    }
    DWORD error = ERROR_GEN_FAILURE;
    Check(job.Request(Operation::Delete, source, first, error) && !error && IsAbsent(source.File(first)) &&
              job.Request(Operation::Delete, source, second, error) && !error && IsAbsent(source.File(second)),
          "permanently delete two exact files on one broker connection");
    Check(job.Request(Operation::Delete, source, third, error, nullptr, nullptr, nullptr, true) &&
              job.Finish() == ERROR_BROKEN_PIPE && HasByte(source.File(third), 3),
          "disconnecting after the header leaves the pending item intact and ends the dispatcher");
}
}

int wmain()
{
    Directory source, target, outside;
    if (!source.Open(L"source") || !target.Open(L"target") || !outside.Open(L"outside"))
        return 2;
    CheckListedLeafIdentity(source);
    CheckPanelPreparationPaths(source, outside);
    CheckPanelPreparationFlags(source);
    CheckPanelPreparationMutationGuards(source);
    CheckPanelMetadataGuards(source);
    CheckPanelPreparationReparse(source, outside);
    CheckFindContentDisconnect(source, outside);
    CheckStoppedFindReply();
    CheckExactFindScope(source);
    CheckCopyJob(source, target, outside);
    CheckCopyOptionScope(source, target);
    CheckCompetingCopyJobs(source, outside, target);
    CheckActiveOverwriteJournal(source, target);
    CheckLateOverwriteJournal(source, target);
    CheckStaleOverwriteJob(source, target);
    CheckCancelledCopyJob(source, outside, target);
    CheckLateCopyCancellation(source, target);
    CheckCrossVolumeBoundary(source, target);
    CheckRejectedVolumeAliases(source, target);
    Check(CanReportTransferProgress(Operation::FinishCrossVolumeMove), "enable progress for F6 completion");
    CheckUnboundedTransferCancellation(Operation::MoveAcrossVolume);
    CheckUnboundedTransferCancellation(Operation::FinishCrossVolumeMove);
    CheckMalformedTransferProgress();
    CheckRequestDelivery();
    CheckTransferReplies();
    CheckMoveSourceEligibility();
    CheckMetadataJob(source);
    CheckDeleteJob(source);
    return failures ? 1 : 0;
}
