// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Request target attribute writes only for read-only retirement. | Avoid unnecessary access denial for otherwise deletable ordinary targets. | #731 | GPT-6.1 Sol |
// | 2026-11-11 | Share NTFS backup rename attribute normalization. | Recover a normal target when closing the renamed backup sets Archive. | #731 | GPT-6.1 Sol |
// | 2026-11-11 | Replace a verified move target without copying a same-volume source. | Preserve source identity and security, and recover only the recorded exact objects. | #731 | GPT-6.1 Sol |
#pragma once

#include "zipupdate.h"
#include "movesourceguard.h"

namespace UacMoveOverwrite
{
using Handle = UacZipUpdate::Handle;
using Revision = UacZipUpdate::Revision;
using ParentIdentity = UacDirectoryLeaf::ParentIdentity;

struct Result
{
    DWORD error;
    DWORD recoveryError = ERROR_SUCCESS;
    bool moved = false;
    FILE_ID_INFO identity{};
};

#ifdef SAM_UAC_MOVE_OVERWRITE_FAULT_TEST
enum class FaultPoint { JournalFlushed, BackupRenamed, SourceRenamed, BackupRetired };
void PauseAt(FaultPoint point);
#endif

namespace Detail
{
struct Record
{
    ULONGLONG version = 0x53414d4d4f564501ull;
    GUID nonce{};
    FILE_ID_INFO sourceParent{}, targetParent{}, source{};
    Revision target{};
    BYTE targetDigest[32]{};
    WCHAR sourceLeaf[UacDirectoryLeaf::maxComponent + 1]{};
    WCHAR targetLeaf[UacDirectoryLeaf::maxComponent + 1]{};
};

inline std::wstring_view Name(const WCHAR (&name)[UacDirectoryLeaf::maxComponent + 1])
{
    return {name, static_cast<size_t>(std::find(std::begin(name), std::end(name), L'\0') - name)};
}

inline std::wstring BackupName(const GUID& nonce)
{
    WCHAR guid[39]{};
    return StringFromGUID2(nonce, guid, ARRAYSIZE(guid))
        ? std::wstring(L".samandarin-move-") + guid + L".bak" : std::wstring{};
}

inline DWORD Open(HANDLE parent, std::wstring_view name, Handle& file, ACCESS_MASK access)
{
    HANDLE raw = nullptr;
    const DWORD error = UacRenameLeaf::Detail::OpenSource(parent, name, raw,
        FILE_READ_DATA | FILE_READ_EA | access, FILE_SHARE_READ);
    file.reset(raw);
    return error;
}

inline DWORD CheckFile(HANDLE file)
{
    FILE_BASIC_INFO basic{};
    return UacCopyLeaf::Detail::ReadPlainMetadata(file, basic, false,
        UacCopyLeaf::StreamPolicy::PreserveNamed, UacF7::nativeCopyAttributes);
}

inline DWORD CheckBackup(HANDLE parent, std::wstring_view name, HANDLE file,
                         const Record& record, UacCopyLeaf::StageVerifier verify,
                         HANDLE connection, ULONGLONG deadline)
{
    Revision current;
    DWORD error = UacZipUpdate::ReadRevision(file, current);
    if (error) return error;
    if (!UacRenameLeaf::Detail::SameSource(UacZipUpdate::RevisionIdentity(current),
            UacZipUpdate::RevisionIdentity(record.target)) ||
        current.created != record.target.created || current.modified != record.target.modified ||
        current.size != record.target.size ||
        current.attributes != UacZipUpdate::Detail::AttributesAfterBackupRename(record.target.attributes, current.attributes))
        return ERROR_FILE_INVALID;
    std::vector<UacCopyLeaf::Detail::DataStream> streams;
    if ((error = UacCopyLeaf::Detail::ReadDataStreams(file, streams, UacF7::maxNativeCopyBytes)))
        return error;
    if (streams.size() > 1 && (!verify || !verify(file, false)))
        return ERROR_ACCESS_DENIED;
    BYTE digest[32]{};
    if ((error = UacZipUpdate::Detail::DigestReplacement(parent, name, file, current, true,
                                                        digest, connection, deadline)))
        return error;
    return std::memcmp(digest, record.targetDigest, sizeof(digest)) ? ERROR_FILE_INVALID : ERROR_SUCCESS;
}

inline DWORD CheckParents(HANDLE sourceParent, const ParentIdentity& expectedSourceParent,
                          HANDLE targetParent, const ParentIdentity& expectedTargetParent)
{
    DWORD error = UacZipUpdate::Detail::CheckParent(sourceParent, expectedSourceParent);
    if (!error) error = UacZipUpdate::Detail::CheckParent(targetParent, expectedTargetParent);
    if (!error && expectedSourceParent.file.VolumeSerialNumber != expectedTargetParent.file.VolumeSerialNumber)
        error = ERROR_NOT_SAME_DEVICE;
    return error;
}
}

// A separate per-item namespace must be used for this record, never the ZIP or
// disk-copy journal. The source object ID makes unrelated moves independent.
inline std::wstring JournalName(const ParentIdentity& sourceParent, const FILE_ID_INFO& source,
                                const ParentIdentity& targetParent, std::wstring_view targetLeaf)
{
    if (!UacDirectoryLeaf::IsComponent(targetLeaf)) return {};
    BCRYPT_ALG_HANDLE provider = nullptr;
    if (BCryptOpenAlgorithmProvider(&provider, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return {};
    const auto closeProvider = [](BCRYPT_ALG_HANDLE handle) { BCryptCloseAlgorithmProvider(handle, 0); };
    const std::unique_ptr<void, decltype(closeProvider)> algorithm(provider, closeProvider);
    BCRYPT_HASH_HANDLE raw = nullptr;
    if (BCryptCreateHash(provider, &raw, nullptr, 0, nullptr, 0, 0) < 0) return {};
    const std::unique_ptr<void, decltype(&BCryptDestroyHash)> hash(raw, BCryptDestroyHash);
    const FILE_ID_INFO ids[]{sourceParent.file, source, targetParent.file};
    if (BCryptHashData(raw, reinterpret_cast<PUCHAR>(const_cast<FILE_ID_INFO*>(ids)), sizeof(ids), 0) < 0 ||
        BCryptHashData(raw, reinterpret_cast<PUCHAR>(const_cast<WCHAR*>(targetLeaf.data())),
                       static_cast<ULONG>(targetLeaf.size() * sizeof(WCHAR)), 0) < 0) return {};
    BYTE digest[32]{};
    if (BCryptFinishHash(raw, digest, sizeof(digest), 0) < 0) return {};
    std::wstring result = L"uac-move-overwrite-";
    for (BYTE byte : digest)
    {
        result += L"0123456789abcdef"[byte >> 4];
        result += L"0123456789abcdef"[byte & 15];
    }
    return result + L".dat";
}

// Recovery never deletes or rewrites the moved source. It either restores the
// exact old target to a free name, or retires its unchanged backup after proving
// that the original source object is present at the final destination.
inline DWORD RecoverSameVolume(HANDLE sourceParent, const ParentIdentity& expectedSourceParent,
                                const FILE_ID_INFO& sourceId, std::wstring_view sourceLeaf,
                                HANDLE targetParent, const ParentIdentity& expectedTargetParent,
                                std::wstring_view targetLeaf, HANDLE journal,
                                HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG,
                                UacCopyLeaf::StageVerifier verify = nullptr, bool* completed = nullptr)
{
    using namespace Detail;
    using UacRenameLeaf::Detail::SameSource;
    if (completed) *completed = false;
    if (!UacDirectoryLeaf::IsComponent(sourceLeaf) || !UacDirectoryLeaf::IsComponent(targetLeaf))
        return ERROR_INVALID_NAME;
    DWORD error = CheckParents(sourceParent, expectedSourceParent, targetParent, expectedTargetParent);
    if (error) return error;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size)) return GetLastError();
    if (!size.QuadPart) return ERROR_SUCCESS;
    Record record;
    DWORD read = 0;
    if (size.QuadPart != sizeof(record)) return ERROR_INVALID_DATA;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) ||
        !ReadFile(journal, &record, sizeof(record), &read, nullptr)) return GetLastError();
    if (read != sizeof(record) || record.version != Record{}.version ||
        !SameSource(record.sourceParent, expectedSourceParent.file) ||
        !SameSource(record.targetParent, expectedTargetParent.file) || !SameSource(record.source, sourceId) ||
        Name(record.sourceLeaf) != sourceLeaf || Name(record.targetLeaf) != targetLeaf ||
        SameSource(record.source, UacZipUpdate::RevisionIdentity(record.target))) return ERROR_INVALID_DATA;
    const auto backupName = BackupName(record.nonce);
    if (backupName.empty()) return ERROR_INVALID_DATA;
    Handle source(nullptr, CloseHandle), final(nullptr, CloseHandle), backup(nullptr, CloseHandle);
    error = Open(sourceParent, sourceLeaf, source, 0);
    if (error && error != ERROR_FILE_NOT_FOUND) return error;
    if (source && (error = UacZipUpdate::Detail::CheckIdentity(source.get(), sourceId))) return error;
    error = Open(targetParent, targetLeaf, final, 0);
    if (error && error != ERROR_FILE_NOT_FOUND) return error;
    const ACCESS_MASK backupAccess = DELETE | READ_CONTROL |
        ((record.target.attributes & FILE_ATTRIBUTE_READONLY) ? FILE_WRITE_ATTRIBUTES : 0);
    error = Open(targetParent, backupName, backup, backupAccess);
    if (error && error != ERROR_FILE_NOT_FOUND) return error;
    const bool moved = !source && final && !UacZipUpdate::Detail::CheckIdentity(final.get(), sourceId);
    if (!backup)
    {
        const bool untouched = source && final &&
            !UacZipUpdate::CheckRevision(final.get(), record.target);
        error = moved || untouched ? UacZipUpdate::Detail::ClearRecord(journal) : ERROR_FILE_INVALID;
        if (!error && completed) *completed = moved;
        return error;
    }
    if ((error = CheckBackup(targetParent, backupName, backup.get(), record, verify, connection, deadline)))
        return error;
    if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline))) return error;
    if (source && !final)
        error = UacRenameLeaf::Detail::NativeRename(backup.get(), targetParent, targetLeaf);
    else if (moved)
        error = UacZipUpdate::Detail::RemoveBackup(backup.get(), UacZipUpdate::ReplacementKind::DiskCopy);
    else return ERROR_FILE_INVALID;
    if (error) return error;
    backup.reset();
    error = UacZipUpdate::Detail::ClearRecord(journal);
    if (!error && completed) *completed = moved;
    return error;
}

inline Result MoveSameVolume(HANDLE sourceParent, const ParentIdentity& expectedSourceParent,
                              const FILE_ID_INFO& sourceId, std::wstring_view sourceLeaf,
                              HANDLE targetParent, const ParentIdentity& expectedTargetParent,
                              std::wstring_view targetLeaf, const Revision& targetRevision, HANDLE journal,
                              HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG,
                              UacCopyLeaf::StageVerifier verify = nullptr)
{
    using namespace Detail;
    if (!UacDirectoryLeaf::IsComponent(sourceLeaf) || !UacDirectoryLeaf::IsComponent(targetLeaf))
        return {ERROR_INVALID_NAME};
    if (UacRenameLeaf::Detail::SameSource(sourceId, UacZipUpdate::RevisionIdentity(targetRevision)))
        return {ERROR_NOT_SUPPORTED};
    DWORD error = CheckParents(sourceParent, expectedSourceParent, targetParent, expectedTargetParent);
    if (error) return {error};
    if (sourceId.VolumeSerialNumber != expectedSourceParent.file.VolumeSerialNumber ||
        targetRevision.serial != expectedTargetParent.file.VolumeSerialNumber) return {ERROR_FILE_INVALID};
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size)) return {GetLastError()};
    if (size.QuadPart) return {ERROR_BUSY};
    Handle source(nullptr, CloseHandle), target(nullptr, CloseHandle);
    const ACCESS_MASK targetAccess = DELETE | READ_CONTROL |
        ((targetRevision.attributes & FILE_ATTRIBUTE_READONLY) ? FILE_WRITE_ATTRIBUTES : 0);
    if ((error = Open(sourceParent, sourceLeaf, source, DELETE)) ||
        (error = UacZipUpdate::Detail::CheckIdentity(source.get(), sourceId)) ||
        (error = CheckFile(source.get())) ||
        (error = Open(targetParent, targetLeaf, target, targetAccess)) ||
        (error = UacZipUpdate::CheckRevision(target.get(), targetRevision)) ||
        (error = CheckFile(target.get()))) return {error};
    Record record;
    record.sourceParent = expectedSourceParent.file;
    record.targetParent = expectedTargetParent.file;
    record.source = sourceId;
    record.target = targetRevision;
    std::copy(sourceLeaf.begin(), sourceLeaf.end(), record.sourceLeaf);
    std::copy(targetLeaf.begin(), targetLeaf.end(), record.targetLeaf);
    if (FAILED(CoCreateGuid(&record.nonce))) return {ERROR_GEN_FAILURE};
    const auto backupName = BackupName(record.nonce);
    if (backupName.empty()) return {ERROR_GEN_FAILURE};
    std::vector<UacCopyLeaf::Detail::DataStream> streams;
    if ((error = UacCopyLeaf::Detail::ReadDataStreams(target.get(), streams, UacF7::maxNativeCopyBytes))) return {error};
    if (streams.size() > 1 && (!verify || !verify(target.get(), false))) return {ERROR_ACCESS_DENIED};
    if ((error = UacZipUpdate::Detail::DigestReplacement(targetParent, targetLeaf, target.get(), targetRevision,
            true, record.targetDigest, connection, deadline)) ||
        (error = UacZipUpdate::CheckRevision(target.get(), targetRevision)) ||
        (error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline))) return {error};
    if ((error = UacZipUpdate::Detail::WriteRecord(journal, record)))
        return {error, UacZipUpdate::Detail::ClearRecord(journal)};
#ifdef SAM_UAC_MOVE_OVERWRITE_FAULT_TEST
    PauseAt(FaultPoint::JournalFlushed);
#endif
    if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) ||
        (error = UacRenameLeaf::Detail::NativeRename(target.get(), targetParent, backupName)))
        return {error, UacZipUpdate::Detail::ClearRecord(journal)};
#ifdef SAM_UAC_MOVE_OVERWRITE_FAULT_TEST
    PauseAt(FaultPoint::BackupRenamed);
#endif
    if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) ||
        (error = UacRenameLeaf::Detail::NativeRename(source.get(), targetParent, targetLeaf)))
    {
        const DWORD rollback = UacRenameLeaf::Detail::NativeRename(target.get(), targetParent, targetLeaf);
        return {error, rollback ? rollback : UacZipUpdate::Detail::ClearRecord(journal)};
    }
#ifdef SAM_UAC_MOVE_OVERWRITE_FAULT_TEST
    PauseAt(FaultPoint::SourceRenamed);
#endif
    // A rename moved the original object. Never set deletion disposition on it,
    // including when the host disconnects or the journal cleanup fails.
    if ((error = UacZipUpdate::Detail::CheckIdentity(source.get(), sourceId)) ||
        (error = CheckBackup(targetParent, backupName, target.get(), record, verify, connection, deadline)) ||
        (error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) ||
        (error = UacZipUpdate::Detail::RemoveBackup(target.get(), UacZipUpdate::ReplacementKind::DiskCopy)))
        return {ERROR_SUCCESS, error, true, sourceId};
    target.reset();
#ifdef SAM_UAC_MOVE_OVERWRITE_FAULT_TEST
    PauseAt(FaultPoint::BackupRetired);
#endif
    return {ERROR_SUCCESS, UacZipUpdate::Detail::ClearRecord(journal), true, sourceId};
}

using SecurityVerifier = DWORD (*)(HANDLE file, void* context);

// The caller prepares and seals the source with the existing source journal.
// This step publishes a full copy while keeping that original source guarded.
// Only a successful publication (including receipt cleanup) may be followed by
// SourceGuard::RetirePublishedSource, with this same pinned destination handle.
inline UacZipUpdate::PublishResult PublishAcrossVolume(
    UacMoveSourceJournal::SourceGuard& source, HANDLE targetParent, const ParentIdentity& expectedTargetParent,
    std::wstring_view targetLeaf, const Revision& targetRevision, Handle& original,
    PSECURITY_DESCRIPTOR targetSecurity, HANDLE targetJournal, UacCopyLeaf::StageVerifier verify,
    ULONGLONG size, HANDLE connection, ULONGLONG deadline, Handle& published, FILE_ID_INFO& publishedId,
    const UacCopyLeaf::Detail::CopyProgress* progress = nullptr,
    SecurityVerifier verifySecurity = nullptr, void* securityContext = nullptr, bool destinationAudit = true)
{
    publishedId = {};
    if (published || !original || !targetSecurity || !IsValidSecurityDescriptor(targetSecurity) || !verifySecurity)
        return {ERROR_INVALID_PARAMETER};
    if (!UacDirectoryLeaf::IsComponent(targetLeaf)) return {ERROR_INVALID_NAME};
    DWORD error = UacZipUpdate::Detail::CheckParent(targetParent, expectedTargetParent);
    if (!error) error = UacZipUpdate::CheckRevision(original.get(), targetRevision);
    if (!error) error = Detail::CheckFile(original.get());
    if (!error) error = Detail::CheckFile(source.File());
    if (!error) error = verifySecurity(original.get(), securityContext);
    if (error) return {error};
    GUID nonce{};
    if (FAILED(CoCreateGuid(&nonce))) return {ERROR_GEN_FAILURE};
    const auto stageLeaf = Detail::BackupName(nonce) + L".tmp";
    Handle stage(nullptr, CloseHandle);
    FILE_ID_INFO stageId{};
    error = source.PublishCopy(targetParent, expectedTargetParent, stageLeaf, size, nullptr, connection, deadline,
                               stage, stageId, progress, true, targetSecurity, destinationAudit);
    if (error) return {error};
    // Existing-file copies retain the target's creation time and access policy.
    FILE_BASIC_INFO timestamps{};
    timestamps.CreationTime.QuadPart = targetRevision.created;
    if (!SetFileInformationByHandle(stage.get(), FileBasicInfo, &timestamps, sizeof(timestamps)))
        return {GetLastError()};
    if ((error = verifySecurity(stage.get(), securityContext)) ||
        (error = verifySecurity(original.get(), securityContext))) return {error};
    const auto result = UacZipUpdate::Publish(targetParent, expectedTargetParent, targetLeaf, targetRevision,
        original, stage.get(), targetJournal, connection, deadline, &publishedId,
        UacZipUpdate::ReplacementKind::DiskCopy, verify);
    if (result.published) published = std::move(stage);
    return result;
}
}
