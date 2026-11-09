// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Check the source admission used for protected F6. | Cover sparse, exact compression and metadata combinations before broker launch. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Verify the host result policy for partial moves. | Keep one-shot replies and reusable jobs consistent. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise the compressed move request on the broker pipe. | Prove same-volume refusal with the new request type. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Exercise the late F6 request framing. | Read the destination revision before evaluating the volume boundary. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Reject a cross-volume command on one volume. | Verify the broker dispatches the distinct F6 request safely. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Verify a queued cancellation on the live broker pipe. | Prove the dispatcher ends the scoped session without publishing the pending file. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Exercise scoped mutations through the real broker dispatcher. | Validate repeated requests and scope rejection on NTFS without interactive elevation. | | GPT-6.1 Sol (Codex) |

#define SAM_UAC_BROKER
#define wmain InstalledBrokerMain
#include "uacbroker.cpp"
#undef wmain

#include <thread>

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
    const auto file = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ, nullptr,
                                       OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    return file && GetFileInformationByHandleEx(file.get(), FileBasicInfo, &basic, sizeof(basic));
}

// Test only: enter the dispatcher after the elevation handshake boundary.
// The production entry point still authenticates both images and the peer token.
class JobPipe
{
public:
    ~JobPipe() { Finish(); }

    bool Open(HANDLE readyToDispatch = nullptr)
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
        dispatcher = std::thread([this, name, readyToDispatch] {
            const ComApartment com;
            if (FAILED(com.result))
                return;
            const auto peer = Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
            if (peer && ExpectedPipePeer(peer.get(), GetCurrentProcess(), GetCurrentProcessId(), true) &&
                (!readyToDispatch || WaitForSingleObject(readyToDispatch, timeoutMs) == WAIT_OBJECT_0))
            {
                const InstallLocation install;
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
                 UacF7::ArchiveRevision* revision = nullptr)
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
            header.copySize = 1;
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

private:
    Handle server;
    std::thread dispatcher;
    DWORD dispatchError = ERROR_GEN_FAILURE;
};

bool IsAbsent(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
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

void CheckCancelledCopyJob(const Directory& source, const Directory& target)
{
    const auto ready = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    JobPipe job;
    constexpr wchar_t pending[] = L"pending-cancel";
    if (!ready || !WriteByte(source.File(pending), 4) || !job.Open(ready.get()))
    {
        Check(false, "prepare the cancellable copy job");
        return;
    }
    DWORD error = ERROR_GEN_FAILURE;
    const bool cancelled = job.Request(Operation::Copy, source, pending, error, &target, nullptr, nullptr, false, ready.get());
    Check(cancelled && error == ERROR_CANCELLED &&
              !job.Request(Operation::Copy, source, pending, error, &target) && job.Finish() == ERROR_SUCCESS &&
              IsAbsent(target.File(pending)) && HasByte(source.File(pending), 4),
          "a cancel control keeps the source, leaves no target and ends the scoped broker job");
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
    CheckCopyJob(source, target, outside);
    CheckCancelledCopyJob(source, target);
    CheckLateCopyCancellation(source, target);
    CheckCrossVolumeBoundary(source, target);
    CheckTransferReplies();
    CheckMoveSourceEligibility();
    CheckMetadataJob(source);
    CheckDeleteJob(source);
    return failures ? 1 : 0;
}
