// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
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
#define wmain InstalledBrokerMain
#include "uacbroker.cpp"
#undef wmain

#include <initializer_list>
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

    bool Open(HANDLE readyToDispatch = nullptr, HANDLE findRoot = nullptr,
              Operation findScope = Operation::FindContent)
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
        dispatcher = std::thread([this, name, readyToDispatch, findRoot, findScope] {
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

void CheckUnboundedTransferCancellation()
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
            }, &progress, Operation::MoveAcrossVolume);
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
    CheckFindContentDisconnect(source, outside);
    CheckExactFindScope(source);
    CheckCopyJob(source, target, outside);
    CheckCompetingCopyJobs(source, outside, target);
    CheckStaleOverwriteJob(source, target);
    CheckCancelledCopyJob(source, target);
    CheckLateCopyCancellation(source, target);
    CheckCrossVolumeBoundary(source, target);
    CheckRejectedVolumeAliases(source, target);
    CheckUnboundedTransferCancellation();
    CheckMalformedTransferProgress();
    CheckRequestDelivery();
    CheckTransferReplies();
    CheckMoveSourceEligibility();
    CheckMetadataJob(source);
    CheckDeleteJob(source);
    return failures ? 1 : 0;
}
