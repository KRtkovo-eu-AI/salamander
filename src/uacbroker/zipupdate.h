// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Preserve legacy disk journals after interruption. | Avoid claiming security-descriptor recovery without durable proof. | #731 | GPT-6.1 Terra |
// | 2026-11-11 | Prepare a default-stream-only replacement stage. | Keep ignored source streams out of a protected overwrite stage. | | GPT-6.1 Terra |
// | 2026-11-11 | Stream cancellable disk overwrite receipts and retire read-only backups atomically. | Support ordinary large attributed replacements while preserving journal and ZIP boundaries. | | GPT-6.1 Sol |
// | 2026-11-09 | Return a conflict after recovering the original name. | Require a new decision before replacing a restored file. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Bind named overwrite backups to their original contents. | Retire protected targets with named streams only after verifying a durable digest. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Bind overwrite recovery to every replacement stream. | Retain the backup when any verified stream changes. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Expose durable overwrite checkpoints to native fault tests. | Prove the actual recovery path after process termination. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Bind disk overwrite cleanup to both file revisions. | Retain a recovery backup when the target or backup has changed since publication. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Use the verified replacement journal for disk files. | Keep ZIP and disk backups in separate namespaces. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Use the shared staged-file exchange for ZIP writes. | Keep revision checks and journal lifecycle in the archive publisher. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Keep ZIP rollback scoped to the original archive name. | Remove parent-only recovery from the internal publication helper. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Restore an interrupted ZIP from its parent folder. | Derive the exact archive name from the verified journal. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Publish through the handle held throughout staging. | Verify the same original immediately before both renames. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Use the revision captured during brokered archive open. | Recheck the same fixed fields before publication. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Publish a prepared ZIP through verified file handles. | Preserve the original on collisions and support recovery after broker loss. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "copyleaf.h"
#include <algorithm>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace UacZipUpdate
{
using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;

using Revision = UacF7::ArchiveRevision;
enum class ReplacementKind { Zip, DiskCopy };

inline FILE_ID_INFO RevisionIdentity(const Revision& revision)
{
    FILE_ID_INFO id{revision.serial, {}};
    std::memcpy(id.FileId.Identifier, revision.fileId, sizeof(revision.fileId));
    return id;
}

inline void SetRevision(Revision& revision, const FILE_ID_INFO& id, const FILE_BASIC_INFO& basic, LARGE_INTEGER size)
{
    revision.serial = id.VolumeSerialNumber;
    std::memcpy(revision.fileId, id.FileId.Identifier, sizeof(revision.fileId));
    revision.size = size.QuadPart;
    revision.created = basic.CreationTime.QuadPart;
    revision.modified = basic.LastWriteTime.QuadPart;
    revision.changed = basic.ChangeTime.QuadPart;
    revision.attributes = basic.FileAttributes;
}

inline DWORD ReadRevision(HANDLE file, Revision& revision)
{
    FILE_ID_INFO id{};
    DWORD error = UacRenameLeaf::Detail::ReadSourceIdentity(file, id);
    if (error)
        return error;
    FILE_BASIC_INFO basic{};
    LARGE_INTEGER size{};
    if (!GetFileInformationByHandleEx(file, FileBasicInfo, &basic, sizeof(basic)) ||
        !GetFileSizeEx(file, &size))
        return GetLastError();
    SetRevision(revision, id, basic, size);
    return revision.size >= 0 ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

inline DWORD CheckRevision(HANDLE original, const Revision& expected);

namespace Detail
{
struct RecoveryRecord
{
    ULONGLONG version = 1;
    GUID nonce{};
    FILE_ID_INFO parent{};
    FILE_ID_INFO original{};
    FILE_ID_INFO replacement{};
    WCHAR leaf[UacDirectoryLeaf::maxComponent + 1]{};
};

struct DiskReplacementReceipt
{
    Revision original{};
    Revision replacement{};
    BYTE replacementDigest[32]{};
};

struct DiskRecoveryRecord
{
    RecoveryRecord exchange{};
    DiskReplacementReceipt receipt{};
};

struct NamedBackupRecord
{
    DiskRecoveryRecord disk{};
    BYTE originalDigest[32]{};
};

static_assert(offsetof(DiskRecoveryRecord, receipt) == sizeof(RecoveryRecord));
static_assert(offsetof(NamedBackupRecord, originalDigest) == sizeof(DiskRecoveryRecord));

inline bool IsZipLeaf(std::wstring_view leaf)
{
    return UacDirectoryLeaf::IsComponent(leaf) && leaf.size() > 4 &&
           UacDirectoryLeaf::Equal(leaf.substr(leaf.size() - 4), L".zip");
}

inline bool IsReplacementLeaf(std::wstring_view leaf, ReplacementKind kind)
{
    return kind == ReplacementKind::Zip ? IsZipLeaf(leaf) : UacDirectoryLeaf::IsComponent(leaf);
}

inline std::wstring BackupName(const GUID& nonce, ReplacementKind kind = ReplacementKind::Zip)
{
    WCHAR guid[39]{};
    if (!StringFromGUID2(nonce, guid, ARRAYSIZE(guid)))
        return {};
    return std::wstring(kind == ReplacementKind::Zip ? L".samandarin-zip-" : L".samandarin-overwrite-") + guid + L".bak";
}

inline DWORD Open(HANDLE parent, std::wstring_view leaf, HANDLE& file, ACCESS_MASK access)
{
    // A data access right makes the sharing check effective against rename and write.
    return UacRenameLeaf::Detail::OpenSource(parent, leaf, file, FILE_READ_DATA | access, FILE_SHARE_READ);
}

inline DWORD CheckIdentity(HANDLE file, const FILE_ID_INFO& expected)
{
    FILE_ID_INFO actual{};
    const DWORD error = UacRenameLeaf::Detail::ReadSourceIdentity(file, actual);
    return error ? error : UacRenameLeaf::Detail::SameSource(actual, expected) ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

inline DWORD CheckParent(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expected)
{
    UacDirectoryLeaf::ParentIdentity actual;
    const DWORD error = UacDirectoryLeaf::ReadParentIdentity(parent, actual);
    if (error)
        return error;
    return UacDirectoryLeaf::SameParent(expected, actual) ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

template <class Record>
inline DWORD WriteRecord(HANDLE journal, const Record& record)
{
    DWORD written = 0;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) ||
        !WriteFile(journal, &record, sizeof(record), &written, nullptr))
        return GetLastError();
    if (written != sizeof(record))
        return ERROR_WRITE_FAULT;
    return FlushFileBuffers(journal) ? ERROR_SUCCESS : GetLastError();
}

inline DWORD ClearRecord(HANDLE journal)
{
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) || !SetEndOfFile(journal) || !FlushFileBuffers(journal))
        return GetLastError();
    return ERROR_SUCCESS;
}

#ifdef SAM_UAC_DISK_RECEIPT_TEST
void DiskReceiptReadBlock();
#endif

inline DWORD HashReplacementBytes(BCRYPT_HASH_HANDLE hash, HANDLE file, ULONGLONG size,
                                  HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    if (!SetFilePointerEx(file, {}, nullptr, FILE_BEGIN))
        return GetLastError();
    BYTE block[4096];
    while (size)
    {
        if (const DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline))
            return error;
        const auto count = static_cast<DWORD>((std::min)(size, static_cast<ULONGLONG>(sizeof(block))));
        DWORD read = 0;
        if (!ReadFile(file, block, count, &read, nullptr))
            return GetLastError();
        if (read != count)
            return ERROR_HANDLE_EOF;
#ifdef SAM_UAC_DISK_RECEIPT_TEST
        DiskReceiptReadBlock();
#endif
        const auto status = BCryptHashData(hash, block, read, 0);
        if (status < 0)
            return RtlNtStatusToDosError(status);
        size -= read;
    }
    return ERROR_SUCCESS;
}

inline DWORD DigestReplacement(HANDLE parent, std::wstring_view leaf, HANDLE file, const Revision& revision,
                               bool includeNamed, BYTE (&digest)[32],
                               HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    using namespace UacCopyLeaf::Detail;
    if (revision.size < 0)
        return ERROR_FILE_TOO_LARGE;
    if (const DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline))
        return error;
    const auto size = static_cast<ULONGLONG>(revision.size);
    std::vector<DataStream> inventory;
    std::vector<StreamHandle> named;
    if (includeNamed)
    {
        if (const DWORD error = ReadDataStreams(file, inventory, UacF7::maxNativeCopyBytes))
            return error;
        const auto identity = RevisionIdentity(revision);
        for (const auto& stream : inventory)
        {
            if (stream.suffix == L"::$DATA")
            {
                if (stream.size != size)
                    return ERROR_FILE_INVALID;
                continue;
            }
            StreamHandle opened(nullptr, CloseHandle);
            if (const DWORD error = OpenDataStream(parent, leaf, stream, identity, false, opened, false, true, true))
                return error;
            named.push_back(std::move(opened));
        }
    }

    BCRYPT_ALG_HANDLE provider = nullptr;
    auto status = BCryptOpenAlgorithmProvider(&provider, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status < 0)
        return RtlNtStatusToDosError(status);
    const auto closeProvider = [](BCRYPT_ALG_HANDLE handle) { BCryptCloseAlgorithmProvider(handle, 0); };
    const std::unique_ptr<void, decltype(closeProvider)> algorithm(provider, closeProvider);
    BCRYPT_HASH_HANDLE raw = nullptr;
    status = BCryptCreateHash(algorithm.get(), &raw, nullptr, 0, nullptr, 0, 0);
    if (status < 0)
        return RtlNtStatusToDosError(status);
    const std::unique_ptr<void, decltype(&BCryptDestroyHash)> hash(raw, BCryptDestroyHash);

    if (includeNamed)
    {
        // Sorted, length-prefixed names and sizes bind stream boundaries as
        // well as contents. Keep all named stream handles until revalidation.
        size_t index = 0;
        for (auto& stream : inventory)
        {
            ULONG nameBytes = static_cast<ULONG>(stream.suffix.size() * sizeof(wchar_t));
            status = BCryptHashData(hash.get(), reinterpret_cast<BYTE*>(&nameBytes), sizeof(nameBytes), 0);
            if (status >= 0)
                status = BCryptHashData(hash.get(), reinterpret_cast<BYTE*>(stream.suffix.data()), nameBytes, 0);
            if (status >= 0)
                status = BCryptHashData(hash.get(), reinterpret_cast<BYTE*>(&stream.size), sizeof(stream.size), 0);
            if (status < 0)
                return RtlNtStatusToDosError(status);
            const HANDLE input = stream.suffix == L"::$DATA" ? file : named[index++].get();
            if (const DWORD error = HashReplacementBytes(hash.get(), input, stream.size, connection, deadline))
                return error;
        }
        std::vector<DataStream> current;
        if (const DWORD error = ReadDataStreams(file, current, UacF7::maxNativeCopyBytes))
            return error;
        if (current != inventory)
            return ERROR_FILE_INVALID;
    }
    else if (const DWORD error = HashReplacementBytes(hash.get(), file, size, connection, deadline))
        return error;

    status = BCryptFinishHash(hash.get(), digest, sizeof(digest), 0);
    return status < 0 ? RtlNtStatusToDosError(status) : ERROR_SUCCESS;
}

inline DWORD ReadDiskReceipt(HANDLE parent, std::wstring_view leaf, HANDLE original, HANDLE replacement,
                             DiskReplacementReceipt& receipt, bool namedBackup = false,
                             HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    FILE_BASIC_INFO basic{};
    const auto policy = namedBackup ? UacCopyLeaf::StreamPolicy::PreserveNamed : UacCopyLeaf::StreamPolicy::RequireDefault;
    DWORD error = UacCopyLeaf::Detail::ReadPlainMetadata(original, basic, false, policy, UacF7::nativeCopyAttributes);
    if (!error)
        error = UacCopyLeaf::Detail::ReadPlainMetadata(replacement, basic, false, UacCopyLeaf::StreamPolicy::PreserveNamed, UacF7::nativeCopyAttributes);
    if (!error)
        error = ReadRevision(original, receipt.original);
    if (!error)
        error = ReadRevision(replacement, receipt.replacement);
    return error ? error : DigestReplacement(parent, leaf, replacement, receipt.replacement, true, receipt.replacementDigest, connection, deadline);
}

inline DWORD AttributesAfterBackupRename(DWORD expected, DWORD actual)
{
    return !(expected & FILE_ATTRIBUTE_ARCHIVE) && (actual & FILE_ATTRIBUTE_ARCHIVE)
        ? (expected & ~FILE_ATTRIBUTE_NORMAL) | FILE_ATTRIBUTE_ARCHIVE : expected;
}

inline DWORD VerifyDiskReceipt(HANDLE parent, std::wstring_view leaf, HANDLE original, HANDLE replacement,
                               const DiskReplacementReceipt& receipt, bool includeNamed,
                               std::wstring_view backupName = {}, const BYTE (*originalDigest)[32] = nullptr,
                               UacCopyLeaf::StageVerifier verify = nullptr,
                               HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    FILE_BASIC_INFO basic{};
    DWORD error = CheckRevision(replacement, receipt.replacement);
    if (error)
        return error;
    BYTE actualDigest[sizeof(receipt.replacementDigest)]{};
    const auto policy = includeNamed ? UacCopyLeaf::StreamPolicy::PreserveNamed : UacCopyLeaf::StreamPolicy::RequireDefault;
    if ((error = UacCopyLeaf::Detail::ReadPlainMetadata(replacement, basic, false, policy, UacF7::nativeCopyAttributes)) ||
        (error = DigestReplacement(parent, leaf, replacement, receipt.replacement, includeNamed, actualDigest, connection, deadline)))
        return error;
    if (std::memcmp(actualDigest, receipt.replacementDigest, sizeof(actualDigest)) != 0)
        return ERROR_FILE_INVALID;
    const auto originalPolicy = originalDigest ? UacCopyLeaf::StreamPolicy::PreserveNamed
                                               : UacCopyLeaf::StreamPolicy::RequireDefault;
    if ((error = CheckRevision(replacement, receipt.replacement)) ||
        (error = UacCopyLeaf::Detail::ReadPlainMetadata(original, basic, false, originalPolicy, UacF7::nativeCopyAttributes)))
        return error;
    if (originalDigest)
    {
        if (!verify || !verify(original, false))
            return ERROR_ACCESS_DENIED;
        if ((error = DigestReplacement(parent, backupName, original, receipt.original, true, actualDigest, connection, deadline)))
            return error;
        if (std::memcmp(actualDigest, *originalDigest, sizeof(actualDigest)) != 0)
            return ERROR_FILE_INVALID;
    }
    // NTFS may defer setting Archive for the backup rename until its last
    // write-capable handle closes. Permit only that one-way metadata change;
    // content, identity and every timestamp remain bound to the receipt.
    Revision expectedBackup = receipt.original;
    expectedBackup.attributes = AttributesAfterBackupRename(expectedBackup.attributes, basic.FileAttributes);
    return CheckRevision(original, expectedBackup);
}

inline DWORD RemoveBackup(HANDLE file, ReplacementKind kind = ReplacementKind::Zip)
{
    if (kind == ReplacementKind::DiskCopy)
    {
        FILE_BASIC_INFO basic{};
        if (!GetFileInformationByHandleEx(file, FileBasicInfo, &basic, sizeof(basic)))
            return GetLastError();
        if (basic.FileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            // Do not clear the backup attribute: failure and cancellation must
            // preserve its complete original state until the verified unlink.
            FILE_DISPOSITION_INFO_EX remove{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE};
            return SetFileInformationByHandle(file, FileDispositionInfoEx, &remove, sizeof(remove))
                ? ERROR_SUCCESS : GetLastError();
        }
    }
    FILE_DISPOSITION_INFO remove{TRUE};
    return SetFileInformationByHandle(file, FileDispositionInfo, &remove, sizeof(remove))
               ? ERROR_SUCCESS : GetLastError();
}
}

enum class RecoveryIntent { Replace, CopyToNewName };

// Only use a broker-owned, exclusively held journal. Recovery is scoped to
// the same verified parent and exact archive name. It never replaces a name.
inline DWORD Recover(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expected,
                     std::wstring_view leaf, HANDLE journal, ReplacementKind kind = ReplacementKind::Zip,
                     UacCopyLeaf::StageVerifier verify = nullptr, RecoveryIntent intent = RecoveryIntent::Replace,
                     HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    using namespace Detail;
    using UacRenameLeaf::Detail::SameSource;
    if (!IsReplacementLeaf(leaf, kind))
        return ERROR_INVALID_NAME;
    DWORD error = CheckParent(parent, expected);
    if (error)
        return error;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size))
        return GetLastError();
    if (!size.QuadPart)
        return ERROR_SUCCESS;

    RecoveryRecord record;
    DWORD read = 0;
    const bool namedBackup = kind == ReplacementKind::DiskCopy && size.QuadPart == sizeof(NamedBackupRecord);
    const bool diskRecord = kind == ReplacementKind::DiskCopy && (namedBackup || size.QuadPart == sizeof(DiskRecoveryRecord));
    if (!diskRecord && size.QuadPart != sizeof(record))
        return ERROR_INVALID_DATA;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) ||
        !ReadFile(journal, &record, sizeof(record), &read, nullptr))
        return GetLastError();
    const auto* end = std::find(std::begin(record.leaf), std::end(record.leaf), L'\0');
    const std::wstring_view recorded(record.leaf, end - record.leaf);
    const bool versionMatches = namedBackup ? record.version == 4
        : diskRecord ? record.version == 2 || record.version == 3 : record.version == 1;
    if (read != sizeof(record) || !versionMatches || !IsReplacementLeaf(recorded, kind) ||
        !SameSource(record.parent, expected.file) ||
        record.original.VolumeSerialNumber != expected.file.VolumeSerialNumber ||
        record.replacement.VolumeSerialNumber != expected.file.VolumeSerialNumber ||
        SameSource(record.original, record.replacement))
        return ERROR_INVALID_DATA;
    if (intent == RecoveryIntent::CopyToNewName)
    {
        if (kind != ReplacementKind::DiskCopy)
            return ERROR_INVALID_PARAMETER;
        if (!UacDirectoryLeaf::Equal(recorded, leaf))
            return ERROR_SUCCESS;
        leaf = recorded;
    }
    else if (recorded != leaf)
        return ERROR_INVALID_DATA;
    // Disk journal versions 1-4 do not bind the original security descriptor.
    // Clear only a journal that still names the original object and has no
    // backup: this changes no data and leaves any later overwrite to obtain a
    // fresh approval. Every other state must remain available for review.
    if (kind == ReplacementKind::DiskCopy)
    {
        if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) != ERROR_SUCCESS)
            return error;
        const auto unopenedBackup = BackupName(record.nonce, kind);
        if (unopenedBackup.empty())
            return ERROR_INVALID_DATA;
        HANDLE rawBackupCheck = nullptr;
        error = Open(parent, unopenedBackup, rawBackupCheck, 0);
        Handle backupCheck(rawBackupCheck, CloseHandle);
        if (error == ERROR_FILE_NOT_FOUND)
        {
            HANDLE rawOriginal = nullptr;
            error = Open(parent, leaf, rawOriginal, 0);
            Handle originalCheck(rawOriginal, CloseHandle);
            if (!error && !CheckIdentity(originalCheck.get(), record.original) && !CheckParent(parent, expected))
            {
                error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline);
                return error ? error : ClearRecord(journal);
            }
        }
        return ERROR_PARTIAL_COPY;
    }
    const auto backupName = BackupName(record.nonce, kind);
    if (backupName.empty())
        return ERROR_INVALID_DATA;

    HANDLE rawFinal = nullptr;
    error = Open(parent, leaf, rawFinal, 0);
    if (error && error != ERROR_FILE_NOT_FOUND)
        return error;
    Handle finalFile(rawFinal, CloseHandle);
    HANDLE rawBackup = nullptr;
    error = Open(parent, backupName, rawBackup, DELETE | (namedBackup ? READ_CONTROL : 0));
    if (error == ERROR_FILE_NOT_FOUND)
    {
        if (!finalFile)
            return ERROR_FILE_INVALID;
        if (CheckIdentity(finalFile.get(), record.original) &&
            CheckIdentity(finalFile.get(), record.replacement))
            return ERROR_FILE_INVALID;
        return ClearRecord(journal);
    }
    if (error)
        return error;
    Handle backup(rawBackup, CloseHandle);
    if ((error = CheckIdentity(backup.get(), record.original)) != ERROR_SUCCESS)
        return error;
    if (kind == ReplacementKind::DiskCopy)
    {
        FILE_BASIC_INFO basic{};
        if (!GetFileInformationByHandleEx(backup.get(), FileBasicInfo, &basic, sizeof(basic)))
            return GetLastError();
        if (basic.FileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            backup.reset();
            rawBackup = nullptr;
            error = Open(parent, backupName, rawBackup, DELETE | FILE_WRITE_ATTRIBUTES | (namedBackup ? READ_CONTROL : 0));
            if (error)
                return error;
            backup.reset(rawBackup);
            if ((error = CheckIdentity(backup.get(), record.original)) != ERROR_SUCCESS)
                return error;
        }
    }
    if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) != ERROR_SUCCESS)
        return error;
    if (!finalFile)
    {
        error = UacRenameLeaf::Detail::NativeRename(backup.get(), parent, leaf);
    }
    else if ((error = CheckIdentity(finalFile.get(), record.replacement)) == ERROR_SUCCESS)
    {
        if (kind == ReplacementKind::DiskCopy)
        {
            DiskReplacementReceipt receipt;
            // A legacy or interrupted record can identify both names without
            // proving which revisions are safe to retire.
            if (!diskRecord || !ReadFile(journal, &receipt, sizeof(receipt), &read, nullptr) ||
                read != sizeof(receipt) || !SameSource(RevisionIdentity(receipt.original), record.original) ||
                !SameSource(RevisionIdentity(receipt.replacement), record.replacement))
                return ERROR_PARTIAL_COPY;
            BYTE originalDigest[32]{};
            if (namedBackup && (!ReadFile(journal, originalDigest, sizeof(originalDigest), &read, nullptr) ||
                                read != sizeof(originalDigest)))
                return ERROR_PARTIAL_COPY;
            const DWORD verified = VerifyDiskReceipt(parent, leaf, backup.get(), finalFile.get(), receipt, record.version >= 3,
                                                      backupName, namedBackup ? &originalDigest : nullptr, verify, connection, deadline);
            if (verified)
                return verified == ERROR_CANCELLED || verified == ERROR_TIMEOUT ? verified : ERROR_PARTIAL_COPY;
        }
        error = RemoveBackup(backup.get(), kind);
    }
    if (error)
        return error;
    backup.reset();
    error = ClearRecord(journal);
    // A copy to a free name has no approval to overwrite the recovered file.
    return !error && !finalFile && intent == RecoveryIntent::CopyToNewName ? ERROR_ALREADY_EXISTS : error;
}

struct PublishResult
{
    DWORD error;
    DWORD recoveryError = ERROR_SUCCESS;
    bool published = false;
};

inline DWORD CheckRevision(HANDLE original, const Revision& expected)
{
    Revision actual;
    const DWORD error = ReadRevision(original, actual);
    if (error)
        return error;
    return UacRenameLeaf::Detail::SameSource(RevisionIdentity(actual), RevisionIdentity(expected)) &&
           actual.created == expected.created && actual.modified == expected.modified &&
           actual.changed == expected.changed && actual.attributes == expected.attributes &&
           actual.size == expected.size ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

// Prepares only a fresh, exclusively held stage. The caller must keep both
// handles open until publication and use the journalled target replacement.
// This helper does not authorize either the source or an overwrite.
inline DWORD CopyIgnoredNamedStreamsToStage(HANDLE source, const Revision& expectedSource,
                                            HANDLE stage, UacCopyLeaf::StageVerifier verify,
                                            HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG,
                                            const UacCopyLeaf::Detail::CopyProgress* progress = nullptr)
{
    using namespace UacCopyLeaf;
    using namespace UacCopyLeaf::Detail;
    if (!source || !stage || !verify || expectedSource.size < 0)
        return ERROR_INVALID_PARAMETER;
    if (!verify(stage, false))
        return ERROR_ACCESS_DENIED;
    DWORD error = CheckRevision(source, expectedSource);
    if (error)
        return error;
    FILE_BASIC_INFO sourceBasic{}, stageBasic{};
    if ((error = ReadPlainMetadata(source, sourceBasic, false, StreamPolicy::IgnoreNamed,
                                   UacF7::nativeCopyAttributes)) ||
        (error = ReadPlainMetadata(stage, stageBasic, false, StreamPolicy::RequireDefault,
                                   UacF7::nativeCopyAttributes)))
        return error;
    LARGE_INTEGER stageLength{};
    if (!GetFileSizeEx(stage, &stageLength))
        return GetLastError();
    if (stageLength.QuadPart != 0)
        return ERROR_FILE_INVALID;
    const LARGE_INTEGER beginning{};
    if (!SetFilePointerEx(source, beginning, nullptr, FILE_BEGIN) ||
        !SetFilePointerEx(stage, beginning, nullptr, FILE_BEGIN))
        return GetLastError();
    if ((error = CopyData(source, stage, static_cast<ULONGLONG>(expectedSource.size), nullptr,
                          connection, deadline, false, 0, progress)) ||
        (error = CheckRevision(source, expectedSource)) ||
        (error = ReadPlainMetadata(stage, stageBasic, false, StreamPolicy::RequireDefault,
                                   UacF7::nativeCopyAttributes)))
        return error;
    return verify(stage, false) ? ERROR_SUCCESS : ERROR_ACCESS_DENIED;
}

// The caller supplies the original opened for delete without sharing write
// or delete, and a fully written, exclusive delete-on-close stage in its
// parent. The journal must be trusted, exclusive and empty.
inline PublishResult Publish(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                             std::wstring_view leaf, const Revision& expectedOriginal, Handle& original,
                             HANDLE stage, HANDLE journal, HANDLE connection, ULONGLONG deadline,
                             FILE_ID_INFO* published = nullptr, ReplacementKind kind = ReplacementKind::Zip,
                             UacCopyLeaf::StageVerifier verify = nullptr)
{
    using namespace Detail;
    using namespace UacRenameLeaf::Detail;
    if (!IsReplacementLeaf(leaf, kind))
        return {ERROR_INVALID_NAME};
    DWORD error = CheckParent(parent, expectedParent);
    if (error)
        return {error};
    LARGE_INTEGER journalSize{};
    if (!GetFileSizeEx(journal, &journalSize))
        return {GetLastError()};
    if (journalSize.QuadPart)
        return {ERROR_INVALID_DATA};

    if ((error = CheckRevision(original.get(), expectedOriginal)) != ERROR_SUCCESS)
        return {error};

    const bool disk = kind == ReplacementKind::DiskCopy;
    NamedBackupRecord backupRecord;
    auto& diskRecord = backupRecord.disk;
    bool namedBackup = false;
    if (disk)
    {
        using namespace UacCopyLeaf::Detail;
        std::vector<DataStream> streams;
        if ((error = CheckPlainStreamsAndEa(original.get(), false, UacCopyLeaf::StreamPolicy::PreserveNamed)) ||
            (error = ReadDataStreams(original.get(), streams, UacF7::maxNativeCopyBytes)))
            return {error};
        namedBackup = streams.size() > 1;
        if (namedBackup)
        {
            if (!verify || !verify(original.get(), false))
                return {ERROR_ACCESS_DENIED};
            if ((error = DigestReplacement(parent, leaf, original.get(), expectedOriginal, true, backupRecord.originalDigest, connection, deadline)) ||
                (error = CheckRevision(original.get(), expectedOriginal)))
                return {error};
        }
    }
    auto& record = diskRecord.exchange;
    record.version = namedBackup ? 4u : disk ? 3u : 1u;
    record.parent = expectedParent.file;
    record.original = RevisionIdentity(expectedOriginal);
    std::copy(leaf.begin(), leaf.end(), record.leaf);
    if (!GetFileInformationByHandleEx(stage, FileIdInfo, &record.replacement, sizeof(record.replacement)) ||
        !FlushFileBuffers(stage))
        return {GetLastError()};
    if (record.replacement.VolumeSerialNumber != record.original.VolumeSerialNumber ||
        SameSource(record.replacement, record.original))
        return {ERROR_FILE_INVALID};
    if (FAILED(CoCreateGuid(&record.nonce)))
        return {ERROR_GEN_FAILURE};
    const auto backupName = BackupName(record.nonce, kind);
    if (backupName.empty())
        return {ERROR_GEN_FAILURE};
    if ((error = CheckTransferConnection(connection, deadline)) != ERROR_SUCCESS)
        return {error};
    const auto writeRecord = [&] {
        if (namedBackup)
            return WriteRecord(journal, backupRecord);
        return disk ? WriteRecord(journal, diskRecord) : WriteRecord(journal, record);
    };
    error = writeRecord();
    if (error)
        return {error, ClearRecord(journal)};
#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
    PauseReplacementAt(ReplacementFaultPoint::JournalFlushed);
#endif
    const auto exchange = ReplaceStagedFile(parent, original.get(), stage, leaf, backupName, connection, deadline);
    if (exchange.error)
        return {exchange.error, exchange.rollbackError ? exchange.rollbackError : ClearRecord(journal)};

    if (published)
        *published = record.replacement;
    DWORD cleanup = ERROR_SUCCESS;
    if (disk)
    {
        auto& receipt = diskRecord.receipt;
        cleanup = ReadDiskReceipt(parent, leaf, original.get(), stage, receipt, namedBackup, connection, deadline);
        // Renaming the original can update ChangeTime and set Archive.
        // Its data revision and other attributes must still match consent.
        if (!cleanup && (!SameSource(RevisionIdentity(receipt.original), record.original) ||
            !SameSource(RevisionIdentity(receipt.replacement), record.replacement) ||
            receipt.original.created != expectedOriginal.created || receipt.original.modified != expectedOriginal.modified ||
            receipt.original.size != expectedOriginal.size ||
            receipt.original.attributes != AttributesAfterBackupRename(expectedOriginal.attributes, receipt.original.attributes)))
            cleanup = ERROR_FILE_INVALID;
        if (!cleanup)
            cleanup = writeRecord();
#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
        if (!cleanup)
            PauseReplacementAt(ReplacementFaultPoint::ReceiptFlushed);
#endif
        if (!cleanup)
            cleanup = VerifyDiskReceipt(parent, leaf, original.get(), stage, receipt, true, backupName,
                                        namedBackup ? &backupRecord.originalDigest : nullptr, verify, connection, deadline);
    }
    if (!cleanup)
        cleanup = RemoveBackup(original.get(), kind);
    original.reset();
#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
    if (disk && !cleanup)
        PauseReplacementAt(ReplacementFaultPoint::BackupRetired);
#endif
    if (!cleanup)
        cleanup = ClearRecord(journal);
    return {ERROR_SUCCESS, cleanup, true};
}
}
