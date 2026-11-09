// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Recheck default-only destinations before retiring an F6 source. | Detect late alternate streams before removing the guarded source. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Admit compressed named F6 streams. | Keep their exclusive locks while verifying their separate compression formats. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Admit sparse named streams under the F6 source lock. | Preserve their allocation instead of refusing the combined case. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Apply the cross-volume limit to each stream check. | Include named bytes before opening streams and after publication. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Publish guarded named streams before retiring the source. | Check the protected target inventory and finish the last unlink after closing the stream handles. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Hold and recheck the named streams of a sealed source. | Keep each stream locked and bind the inventory to the same NTFS identity. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Probe earlier opens on an individual NTFS data stream. | Separate per-stream exclusion from the file-wide source DACL. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Create the recovery link on the already guarded file. | Keep verification and link creation on a single handle. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Guard compressed move sources throughout retirement. | Apply the same metadata contract when reopening the recovery link. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Allow guarded sparse F6 publication. | Recheck the sparse source under the same seal until unlink. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Compare an existing F6 copy with its guarded source. | Prove the destination bytes before permitting a late source removal. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Retire both F6 source links after the destination receipt. | Preserve a recoverable source until the last unlink. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Publish a copy of a guarded move source. | Read the overlapped source without releasing its lock and retain the exact destination handle. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Guard the default stream of an F6 source. | Exclude earlier opens and refuse unsupported streams or metadata. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "copyleaf.h"
#include "movesourcejournal.h"

namespace UacMoveSourceJournal
{
// Call only after sealing the source and closing the broker's original handle.
// The reservation can then reject preexisting opens without admitting new writers
// when the filter handle is released and the broker reopens its guarded handle.
inline DWORD CheckNoPriorSourceOpens(HANDLE parent, std::wstring_view leaf, const FILE_ID_INFO& expected,
                                    std::wstring_view namedStream = {})
{
    if (!UacDirectoryLeaf::IsComponent(leaf) ||
        (!namedStream.empty() && !UacCopyLeaf::Detail::IsNamedDataStream(namedStream)))
        return ERROR_INVALID_NAME;
    std::wstring streamPath;
    if (!namedStream.empty())
    {
        streamPath.assign(leaf);
        streamPath.append(namedStream);
        leaf = streamPath;
    }
    const Detail::Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr), CloseHandle);
    if (!event)
        return GetLastError();
    constexpr DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE raw = nullptr;
    const DWORD opened = UacRenameLeaf::Detail::OpenFile(parent, leaf, raw, 0, share, FILE_OPEN,
                                                         FILE_RESERVE_OPFILTER, nullptr, false, true);
    if (opened)
        return opened;
    const Detail::Handle filter(raw, CloseHandle);
    OVERLAPPED io{};
    io.hEvent = event.get();
    DWORD bytes = 0;
    const BOOL complete = DeviceIoControl(filter.get(), FSCTL_REQUEST_FILTER_OPLOCK, nullptr, 0, nullptr, 0, &bytes, &io);
    const DWORD status = complete ? ERROR_OPLOCK_NOT_GRANTED : GetLastError();
    if (status != ERROR_IO_PENDING)
        return status;
    const DWORD result = Detail::CheckIdentity(filter.get(), expected);
    const DWORD state = WaitForSingleObject(event.get(), 0);
    const DWORD error = state == WAIT_FAILED ? GetLastError() : ERROR_SHARING_VIOLATION;
    CancelIoEx(filter.get(), &io);
    GetOverlappedResult(filter.get(), &io, &bytes, TRUE);
    return result ? result : state == WAIT_TIMEOUT ? ERROR_SUCCESS : error;
}

inline DWORD RequestExclusiveStreamOplock(HANDLE file, OVERLAPPED& io, REQUEST_OPLOCK_OUTPUT_BUFFER& reply)
{
    REQUEST_OPLOCK_INPUT_BUFFER request{REQUEST_OPLOCK_CURRENT_VERSION, sizeof(request),
        OPLOCK_LEVEL_CACHE_READ | OPLOCK_LEVEL_CACHE_WRITE | OPLOCK_LEVEL_CACHE_HANDLE, REQUEST_OPLOCK_INPUT_FLAG_REQUEST};
    DWORD bytes = 0;
    const BOOL complete = DeviceIoControl(file, FSCTL_REQUEST_OPLOCK, &request, sizeof(request),
                                          &reply, sizeof(reply), &bytes, &io);
    const DWORD status = complete ? ERROR_OPLOCK_NOT_GRANTED : GetLastError();
    return status == ERROR_IO_PENDING ? ERROR_SUCCESS : status;
}

// The caller seals the file before locking its named streams, so new writes
// cannot start between each stream's filter reservation and exclusive open.
class NamedSourceStreams
{
    struct Lock
    {
        Detail::Handle file{nullptr, CloseHandle};
        Detail::Handle event{nullptr, CloseHandle};
        OVERLAPPED io{};
        REQUEST_OPLOCK_OUTPUT_BUFFER reply{};
        bool pending = false;

        ~Lock()
        {
            if (pending)
            {
                DWORD bytes = 0;
                CancelIoEx(file.get(), &io);
                GetOverlappedResult(file.get(), &io, &bytes, TRUE);
            }
        }
    };

public:
    DWORD Open(HANDLE main, HANDLE parent, std::wstring_view leaf, const FILE_ID_INFO& expected)
    {
        if (!inventory.empty())
            return ERROR_ALREADY_INITIALIZED;
        if (!UacDirectoryLeaf::IsComponent(leaf))
            return ERROR_INVALID_NAME;
        DWORD error = Detail::CheckIdentity(main, expected);
        if (error)
            return error;
        std::vector<UacCopyLeaf::Detail::DataStream> snapshot;
        error = UacCopyLeaf::Detail::ReadDataStreams(main, snapshot, UacF7::maxCrossVolumeMoveBytes);
        if (error)
            return error;

        std::vector<std::unique_ptr<Lock>> acquired;
        for (const auto& stream : snapshot)
        {
            if (stream.suffix == L"::$DATA")
                continue;
            if ((error = CheckNoPriorSourceOpens(parent, leaf, expected, stream.suffix)) != ERROR_SUCCESS)
                return error;
            auto lock = std::make_unique<Lock>();
            lock->event.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            if (!lock->event)
                return GetLastError();
            lock->io.hEvent = lock->event.get();
            const auto name = std::wstring(leaf) + stream.suffix;
            HANDLE raw = nullptr;
            error = UacRenameLeaf::Detail::OpenFile(parent, name, raw, FILE_READ_DATA | SYNCHRONIZE,
                FILE_SHARE_READ | FILE_SHARE_DELETE, FILE_OPEN, FILE_OPEN_REQUIRING_OPLOCK, nullptr, false, true);
            if (error)
                return error;
            lock->file.reset(raw);
            error = RequestExclusiveStreamOplock(raw, lock->io, lock->reply);
            if (error)
                return error;
            lock->pending = true;
            if ((error = Detail::CheckIdentity(raw, expected)) != ERROR_SUCCESS)
                return error;
            FILE_BASIC_INFO basic{};
            LARGE_INTEGER size{};
            if (!GetFileInformationByHandleEx(raw, FileBasicInfo, &basic, sizeof(basic)) || !GetFileSizeEx(raw, &size))
                return GetLastError();
            if (!UacF7::IsDefaultCopyFile(basic.FileAttributes) && !UacF7::IsCompressedCopyFile(basic.FileAttributes))
                return ERROR_NOT_SUPPORTED;
            if (size.QuadPart < 0 || static_cast<ULONGLONG>(size.QuadPart) != stream.size)
                return ERROR_FILE_INVALID;
            acquired.push_back(std::move(lock));
        }
        inventory = std::move(snapshot);
        locks = std::move(acquired);
        identity = expected;
        error = Check(main);
        if (error)
        {
            locks.clear();
            inventory.clear();
        }
        return error;
    }

    DWORD Check(HANDLE main) const
    {
        if (inventory.empty())
            return ERROR_INVALID_HANDLE;
        if (DWORD error = Detail::CheckIdentity(main, identity))
            return error;
        std::vector<UacCopyLeaf::Detail::DataStream> current;
        if (DWORD error = UacCopyLeaf::Detail::ReadDataStreams(main, current, UacF7::maxCrossVolumeMoveBytes))
            return error;
        if (current != inventory)
            return ERROR_FILE_INVALID;
        for (const auto& lock : locks)
        {
            const DWORD state = WaitForSingleObject(lock->event.get(), 0);
            if (state != WAIT_TIMEOUT)
                return state == WAIT_OBJECT_0 ? ERROR_SHARING_VIOLATION : GetLastError();
        }
        return ERROR_SUCCESS;
    }

    bool HasNamed() const { return !locks.empty(); }
    const std::vector<UacCopyLeaf::Detail::DataStream>& Inventory() const { return inventory; }

    std::vector<HANDLE> Inputs() const
    {
        std::vector<HANDLE> result;
        result.reserve(locks.size());
        for (const auto& lock : locks)
            result.push_back(lock->file.get());
        return result;
    }

    DWORD CheckTarget(HANDLE target) const
    {
        std::vector<UacCopyLeaf::Detail::DataStream> current;
        if (DWORD error = UacCopyLeaf::Detail::ReadDataStreams(target, current, UacF7::maxCrossVolumeMoveBytes))
            return error;
        return current == inventory ? ERROR_SUCCESS : ERROR_FILE_INVALID;
    }

private:
    FILE_ID_INFO identity{};
    std::vector<UacCopyLeaf::Detail::DataStream> inventory;
    std::vector<std::unique_ptr<Lock>> locks;
};

// The oplock covers the default stream. A file-wide DACL seal is still
// required before committing a move so another name cannot gain a new stream.
class SourceGuard
{
public:
    explicit SourceGuard(DWORD compressedAttributes = 0, UacCopyLeaf::StageVerifier verifyNamedTarget = nullptr)
        : originalCompressedAttributes(compressedAttributes), verifyStage(verifyNamedTarget) {}
    SourceGuard(const SourceGuard&) = delete;
    SourceGuard& operator=(const SourceGuard&) = delete;
    ~SourceGuard() { Close(); }

    DWORD Open(HANDLE parent, std::wstring_view leaf, const FILE_ID_INFO& expected, DWORD links)
    {
        if (source)
            return ERROR_ALREADY_INITIALIZED;
        if (!UacDirectoryLeaf::IsComponent(leaf))
            return ERROR_INVALID_NAME;
        event.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!event)
            return GetLastError();
        io = {};
        io.hEvent = event.get();
        constexpr ACCESS_MASK access = FILE_READ_DATA | FILE_READ_EA | DELETE | READ_CONTROL | WRITE_DAC | SYNCHRONIZE;
        HANDLE raw = nullptr;
        DWORD error = UacRenameLeaf::Detail::OpenFile(parent, leaf, raw, access, 0, FILE_OPEN,
                                                      FILE_OPEN_REQUIRING_OPLOCK, nullptr, false, true);
        if (error)
            return error;
        source.reset(raw);

        error = RequestExclusiveStreamOplock(source.get(), io, reply);
        if (error)
        {
            source.reset();
            return error;
        }
        identity = expected;
        error = Check(links);
        if (error)
            Close();
        return error;
    }

    HANDLE File() const { return source.get(); }

    DWORD AddRecoveryLink(HANDLE storage, std::wstring_view leaf) const
    {
        DWORD error = Check(1);
        if (!error)
            error = UacRenameLeaf::Detail::NativeHardLink(source.get(), storage, leaf);
        return error ? error : Check(2);
    }

    DWORD LockStreams(HANDLE storage, std::wstring_view alias)
    {
        if (!verifyStage || named || !source)
            return ERROR_INVALID_PARAMETER;
        auto streams = std::make_unique<NamedSourceStreams>();
        if (DWORD error = streams->Open(source.get(), storage, alias, identity))
            return error;
        named = std::move(streams);
        return ERROR_SUCCESS;
    }

    DWORD PublishCopy(HANDLE targetParent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                      std::wstring_view leaf, ULONGLONG size, HANDLE cancel, HANDLE connection, ULONGLONG deadline,
                      Detail::Handle& target, FILE_ID_INFO& targetId) const
    {
        if (target)
            return ERROR_INVALID_PARAMETER;
        targetId = {};
        if (!UacDirectoryLeaf::IsComponent(leaf))
            return ERROR_INVALID_NAME;
        UacDirectoryLeaf::ParentIdentity actual;
        DWORD error = UacDirectoryLeaf::ReadParentIdentity(targetParent, actual);
        if (error)
            return error;
        if (!UacDirectoryLeaf::SameParent(actual, expectedParent))
            return ERROR_FILE_INVALID;
        if ((error = Check(2)) != ERROR_SUCCESS)
            return error;
        FILE_BASIC_INFO basic{};
        FILE_ID_INFO publishedId{};
        if (verifyStage && !named)
            return ERROR_INVALID_HANDLE;
        if (named && named->HasNamed() && !verifyStage(targetParent, true))
            return ERROR_ACCESS_DENIED;
        const auto inputs = named ? named->Inputs() : std::vector<HANDLE>{};
        const UacCopyLeaf::Detail::CopySourceLocation location{nullptr, {}, identity, verifyStage,
            &inputs, named ? &named->Inventory() : nullptr};
        error = UacCopyLeaf::Detail::CopyOpenedFile(source.get(), targetParent, leaf, cancel, connection, deadline,
            size, basic, &publishedId, nullptr, StreamPolicy(), named ? &location : nullptr,
            originalCompressedAttributes != 0, nullptr, &target, originalCompressedAttributes);
        if (error)
            return error;
        targetId = publishedId;
        return Check(2) ? ERROR_PARTIAL_COPY : ERROR_SUCCESS;
    }

    // The destination is already present and exclusively held. This check
    // does not modify either file or publish a new destination.
    DWORD VerifyExistingCopy(HANDLE target, ULONGLONG size, HANDLE connection, ULONGLONG deadline) const
    {
        DWORD error = Check(2);
        if (error)
            return error;
        FILE_BASIC_INFO basic{};
        if ((error = UacCopyLeaf::Detail::ReadPlainMetadata(target, basic)) != ERROR_SUCCESS)
            return error;
        FILE_BASIC_INFO sourceBasic{};
        if ((error = UacCopyLeaf::Detail::ReadPlainMetadata(source.get(), sourceBasic)) != ERROR_SUCCESS)
            return error;
        if (sourceBasic.LastWriteTime.QuadPart != basic.LastWriteTime.QuadPart)
            return ERROR_FILE_INVALID;
        LARGE_INTEGER from{}, to{};
        if (!GetFileSizeEx(source.get(), &from) || !GetFileSizeEx(target, &to))
            return GetLastError();
        if (from.QuadPart < 0 || from.QuadPart != to.QuadPart || static_cast<ULONGLONG>(from.QuadPart) != size)
            return ERROR_FILE_INVALID;
        if (!SetFilePointerEx(target, {}, nullptr, FILE_BEGIN))
            return GetLastError();
        const Detail::Handle readEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr), CloseHandle);
        if (!readEvent)
            return GetLastError();
        constexpr DWORD block = 64 * 1024;
        std::vector<BYTE> original(block), copy(block);
        ULONGLONG offset = 0;
        while (offset < size)
        {
            if ((error = UacCopyLeaf::Detail::CheckCancelled(nullptr, connection, deadline)) != ERROR_SUCCESS)
                return error;
            const DWORD length = static_cast<DWORD>((std::min)(size - offset, static_cast<ULONGLONG>(block)));
            DWORD read = 0, readCopy = 0;
            error = UacCopyLeaf::Detail::ReadOverlappedBlock(source.get(), original.data(), length, offset, readEvent.get(),
                                                          read, nullptr, connection, deadline);
            if (error)
                return error;
            if (!ReadFile(target, copy.data(), length, &readCopy, nullptr))
                return GetLastError();
            if (read != length || readCopy != length || std::memcmp(original.data(), copy.data(), length))
                return ERROR_FILE_INVALID;
            offset += length;
        }
        if ((error = Check(2)) != ERROR_SUCCESS)
            return error;
        if ((error = UacCopyLeaf::Detail::CheckCancelled(nullptr, connection, deadline)) != ERROR_SUCCESS)
            return error;
        return FlushFileBuffers(target) ? ERROR_SUCCESS : GetLastError();
    }

    // The caller retains the exclusive published target until this returns.
    // On error the caller can close the guard and recover any surviving link.
    DWORD RetirePublishedSource(HANDLE parent, HANDLE storage, HANDLE journal, PSECURITY_DESCRIPTOR journalSecurity,
                                HANDLE targetParent, std::wstring_view targetLeaf, HANDLE target,
                                const FILE_ID_INFO& targetId, HANDLE cancel, HANDLE connection, ULONGLONG deadline)
    {
        using namespace Detail;
        DWORD error = Check(2);
        if (error)
            return error;

        UacDirectoryLeaf::ParentIdentity original, privateParent;
        FILE_ID_INFO journalId{};
        error = ValidateStorage(parent, storage, journal, journalSecurity, original, privateParent, journalId);
        if (error)
            return error;
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(journal, &length))
            return GetLastError();
        if (length.QuadPart != sealedRecordSize)
            return ERROR_INVALID_DATA;
        Record record{};
        DWORD bytes = 0;
        if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) || !ReadFile(journal, &record, sizeof(record), &bytes, nullptr))
            return GetLastError();
        if (bytes != sizeof(record) || record.version != 2 || !record.length || record.length > ARRAYSIZE(record.leaf) ||
            !UacRenameLeaf::Detail::SameSource(record.file, identity) ||
            UacRenameLeaf::Detail::SameSource(identity, journalId) ||
            !UacRenameLeaf::Detail::SameSource(record.originalParent, original.file) ||
            !UacRenameLeaf::Detail::SameSource(record.privateParent, privateParent.file) ||
            !UacDirectoryLeaf::Equal(original.volume, std::wstring_view(record.volume, ARRAYSIZE(record.volume))))
            return ERROR_FILE_INVALID;

        const std::wstring_view leaf(record.leaf, record.length);
        const auto alias = RecoveryName(record.nonce);
        if (alias.empty())
            return ERROR_INVALID_DATA;
        std::wstring path;
        if ((error = CheckOpenedLeaf(parent, leaf, source.get(), path)) != ERROR_SUCCESS)
            return error;
        const auto dacl = std::make_unique<DaclRecord>();
        if (!ReadFile(journal, dacl.get(), sizeof(*dacl), &bytes, nullptr))
            return GetLastError();
        if (bytes != sizeof(*dacl) || !dacl->original.Valid() || !dacl->sealed.Valid() || !dacl->sealed.protectedDacl)
            return ERROR_INVALID_DATA;
        const auto current = std::make_unique<DaclSnapshot>();
        auto verifySeal = [&](HANDLE file) -> DWORD {
            const DWORD status = current->Read(file);
            return status ? status : current->Matches(dacl->sealed) ? ERROR_SUCCESS : ERROR_FILE_INVALID;
        };
        if ((error = verifySeal(source.get())) != ERROR_SUCCESS)
            return error;
        {
            Handle retained(nullptr, CloseHandle);
            if ((error = OpenOriginal(storage, alias, identity, retained)) != ERROR_SUCCESS)
                return error;
        }

        if ((error = UacCopyLeaf::Detail::CheckCancelled(cancel, connection, deadline)) ||
            (error = CheckPublishedStreams(targetParent, target)) ||
            (error = RememberDestination(journal, journalSecurity, identity, targetParent, targetLeaf, target, targetId)) ||
            (error = RemoveLink(parent, leaf, 2, cancel, connection, deadline)))
            return error;

        // Until the final unlink, the protected alias still contains the source.
        if ((error = CheckNoPriorSourceOpens(storage, alias, identity)) ||
            (error = Open(storage, alias, identity, 1)) ||
            (error = verifySeal(source.get())) ||
            (error = CheckPublishedStreams(targetParent, target)))
            return error;
        return RemoveLink(storage, alias, 1, cancel, connection, deadline);
    }

    DWORD Check(DWORD links) const
    {
        if (!source)
            return ERROR_INVALID_HANDLE;
        if (DWORD error = Detail::CheckIdentity(source.get(), identity))
            return error;
        FILE_STANDARD_INFO standard{};
        if (!GetFileInformationByHandleEx(source.get(), FileStandardInfo, &standard, sizeof(standard)))
            return GetLastError();
        if (standard.NumberOfLinks != links)
            return ERROR_FILE_INVALID;
        FILE_BASIC_INFO metadata{};
        const DWORD allowed = originalCompressedAttributes ? UacF7::compressedCopyAttributes : UacF7::crossVolumeMoveAttributes;
        if (DWORD error = UacCopyLeaf::Detail::ReadPlainMetadata(source.get(), metadata, false,
                StreamPolicy(), allowed))
            return error;
        // The seal and recovery link can set Archive. Other attribute
        // changes invalidate the approved compressed move.
        if (originalCompressedAttributes &&
            ((metadata.FileAttributes ^ originalCompressedAttributes) & ~FILE_ATTRIBUTE_ARCHIVE))
            return ERROR_FILE_INVALID;
        const DWORD state = WaitForSingleObject(event.get(), 0);
        if (state == WAIT_TIMEOUT)
            return named ? named->Check(source.get()) : ERROR_SUCCESS;
        return state == WAIT_OBJECT_0 ? ERROR_SHARING_VIOLATION : GetLastError();
    }

private:
    UacCopyLeaf::StreamPolicy StreamPolicy() const
    {
        return verifyStage ? UacCopyLeaf::StreamPolicy::PreserveNamed : UacCopyLeaf::StreamPolicy::RequireDefault;
    }

    DWORD CheckPublishedStreams(HANDLE parent, HANDLE target) const
    {
        if (!named || !named->HasNamed())
            return UacCopyLeaf::Detail::CheckPlainStreamsAndEa(target, false, UacCopyLeaf::StreamPolicy::RequireDefault);
        if (!verifyStage(parent, true) || !verifyStage(target, false))
            return ERROR_ACCESS_DENIED;
        return named->CheckTarget(target);
    }

    DWORD RemoveLink(HANDLE parent, std::wstring_view leaf, DWORD links,
                     HANDLE cancel, HANDLE connection, ULONGLONG deadline)
    {
        DWORD error = Check(links);
        if (error)
            return error;
        if ((error = UacCopyLeaf::Detail::CheckCancelled(cancel, connection, deadline)) != ERROR_SUCCESS)
            return error;
        FILE_DISPOSITION_INFO remove{TRUE};
        if (!SetFileInformationByHandle(source.get(), FileDispositionInfo, &remove, sizeof(remove)))
            return GetLastError();
        Close();
        if (links == 1)
            named.reset(); // NTFS completes the final unlink after the named opens close.

        HANDLE raw = nullptr;
        error = UacRenameLeaf::Detail::OpenSource(parent, leaf, raw, 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        const Detail::Handle remaining(raw, CloseHandle);
        return error == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : error ? error : ERROR_FILE_INVALID;
    }

    void Close()
    {
        if (source)
        {
            CancelIoEx(source.get(), &io);
            DWORD bytes = 0;
            GetOverlappedResult(source.get(), &io, &bytes, TRUE);
            source.reset();
        }
    }

    const DWORD originalCompressedAttributes;
    const UacCopyLeaf::StageVerifier verifyStage;
    std::unique_ptr<NamedSourceStreams> named;
    Detail::Handle source{nullptr, CloseHandle};
    Detail::Handle event{nullptr, CloseHandle};
    FILE_ID_INFO identity{};
    OVERLAPPED io{};
    REQUEST_OPLOCK_OUTPUT_BUFFER reply{};
};
}
