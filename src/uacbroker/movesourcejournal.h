// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Recover read-only move links and the original Archive bit. | Preserve source attributes and identity after cancellation or hard process exit. | | GPT-6.1 Sol |
// | 2026-11-09 | Restore the original Archive bit after a compressed F6 rollback. | Record the source write time and repair the attributes on the pinned source. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Separate the source seal from recovery link creation. | Make the durable DACL record recoverable before any hard link exists. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Retire both F6 source links after the destination receipt. | Preserve a recoverable source until the last unlink. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Persist the verified F6 destination. | A completed move requires the exact published target even after process loss. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Journal the original DACL before sealing a move source. | Recover the exact inherited or protected ACL after an interrupted move. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Journal an exact source recovery link. | Restore an interrupted move without replacing a different file. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "renameleaf.h"
#include "recoveryjournal.h"

namespace UacMoveSourceJournal
{
namespace Detail
{
using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;

constexpr auto volumeLength = std::wstring_view(LR"(\\?\Volume{00000000-0000-0000-0000-000000000000})").size();

struct Record
{
    ULONGLONG version = 1;
    GUID nonce{};
    FILE_ID_INFO originalParent{}, privateParent{}, file{};
    DWORD length = 0;
    wchar_t volume[volumeLength]{};
    wchar_t leaf[UacDirectoryLeaf::maxComponent]{};
};

struct DaclSnapshot
{
    DWORD protectedDacl = 0;
    BYTE owner[SECURITY_MAX_SID_SIZE]{}, group[SECURITY_MAX_SID_SIZE]{};
    alignas(DWORD) BYTE storage[0x10000]{};

    PACL Acl() { return reinterpret_cast<PACL>(storage); }
    bool Valid()
    {
        return protectedDacl <= 1 && IsValidSid(owner) && IsValidSid(group) &&
               Acl()->AclSize >= sizeof(ACL) && IsValidAcl(Acl());
    }

    DWORD Read(HANDLE file)
    {
        PACL acl = nullptr;
        PSECURITY_DESCRIPTOR raw = nullptr;
        PSID rawOwner = nullptr, rawGroup = nullptr;
        const DWORD error = GetSecurityInfo(file, SE_FILE_OBJECT,
                                            DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION,
                                            &rawOwner, &rawGroup, &acl, nullptr, &raw);
        const std::unique_ptr<void, decltype(&LocalFree)> descriptor(raw, LocalFree);
        if (error)
            return error;
        SECURITY_DESCRIPTOR_CONTROL control{};
        DWORD revision = 0;
        if (!acl || !rawOwner || !rawGroup || !IsValidAcl(acl) || !IsValidSid(rawOwner) || !IsValidSid(rawGroup) ||
            !GetSecurityDescriptorControl(raw, &control, &revision) ||
            !CopySid(sizeof(owner), owner, rawOwner) || !CopySid(sizeof(group), group, rawGroup))
            return ERROR_NOT_SUPPORTED;
        protectedDacl = (control & SE_DACL_PROTECTED) != 0;
        std::memcpy(storage, acl, acl->AclSize);
        return ERROR_SUCCESS;
    }

    bool Matches(DaclSnapshot& other)
    {
        ACL_SIZE_INFORMATION a{}, b{};
        return protectedDacl == other.protectedDacl && EqualSid(owner, other.owner) && EqualSid(group, other.group) &&
               Acl()->AclRevision == other.Acl()->AclRevision &&
               GetAclInformation(Acl(), &a, sizeof(a), AclSizeInformation) &&
               GetAclInformation(other.Acl(), &b, sizeof(b), AclSizeInformation) &&
               a.AclBytesInUse == b.AclBytesInUse && a.AceCount == b.AceCount &&
               a.AclBytesInUse >= sizeof(ACL) &&
               std::memcmp(Acl() + 1, other.Acl() + 1, a.AclBytesInUse - sizeof(ACL)) == 0;
    }

    DWORD Apply(HANDLE file)
    {
        const SECURITY_INFORMATION protection = protectedDacl ? PROTECTED_DACL_SECURITY_INFORMATION :
                                                                  UNPROTECTED_DACL_SECURITY_INFORMATION;
        return SetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | protection, nullptr, nullptr, Acl(), nullptr);
    }
};

struct DaclRecord
{
    DaclSnapshot original, sealed;
};

constexpr ULONGLONG sealedVersion = 2;
constexpr ULONGLONG clearArchiveVersion = 3;

inline DWORD RestoreClearArchive(HANDLE source, LONGLONG recordedWrite)
{
    FILE_BASIC_INFO before{};
    if (!GetFileInformationByHandleEx(source, FileBasicInfo, &before, sizeof(before)))
        return GetLastError();
    if (!(before.FileAttributes & FILE_ATTRIBUTE_ARCHIVE) || before.LastWriteTime.QuadPart != recordedWrite)
        return ERROR_SUCCESS;
    constexpr DWORD allowed = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY |
                              FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE;
    if (before.FileAttributes & ~allowed)
        return ERROR_FILE_INVALID;
    FILE_BASIC_INFO change{};
    change.FileAttributes = before.FileAttributes & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
    if (!change.FileAttributes)
        change.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    FILE_BASIC_INFO after{};
    if (!SetFileInformationByHandle(source, FileBasicInfo, &change, sizeof(change)) ||
        !GetFileInformationByHandleEx(source, FileBasicInfo, &after, sizeof(after)))
        return GetLastError();
    const DWORD expected = before.FileAttributes & ~FILE_ATTRIBUTE_ARCHIVE;
    return after.FileAttributes == (expected ? expected : FILE_ATTRIBUTE_NORMAL) &&
           after.LastWriteTime.QuadPart == recordedWrite ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

inline DWORD CheckOrRestoreDacl(HANDLE originalName, DaclRecord& record, bool restore,
                               const LONGLONG* clearArchiveAt = nullptr)
{
    const auto current = std::make_unique<DaclSnapshot>();
    DWORD error = current->Read(originalName);
    if (error)
        return error;
    if (current->Matches(record.original))
        return restore && clearArchiveAt ? RestoreClearArchive(originalName, *clearArchiveAt) : ERROR_SUCCESS;
    if (!current->Matches(record.sealed))
        return ERROR_FILE_INVALID;
    if (!restore)
        return ERROR_SUCCESS;
    if ((error = record.original.Apply(originalName)) || (error = current->Read(originalName)))
        return error;
    if (!current->Matches(record.original))
        return ERROR_FILE_INVALID;
    return clearArchiveAt ? RestoreClearArchive(originalName, *clearArchiveAt) : ERROR_SUCCESS;
}

struct DestinationRecord
{
    GUID nonce{};
    FILE_ID_INFO parent{}, file{};
    DWORD length = 0;
    wchar_t path[UacDirectoryLeaf::maxNativeName]{};
};

constexpr LONGLONG sealedRecordSize = sizeof(Record) + sizeof(DaclRecord);
inline LONGLONG SealedRecordSize(ULONGLONG version)
{
    return sealedRecordSize + (version == clearArchiveVersion ? sizeof(LONGLONG) : 0);
}

inline DWORD OpenedPath(HANDLE file, std::wstring& path)
{
    constexpr DWORD flags = FILE_NAME_OPENED | VOLUME_NAME_GUID;
    const DWORD length = GetFinalPathNameByHandleW(file, nullptr, 0, flags);
    if (!length)
        return GetLastError();
    if (length > UacDirectoryLeaf::maxNativeName)
        return ERROR_FILENAME_EXCED_RANGE;
    path.resize(length);
    const DWORD actual = GetFinalPathNameByHandleW(file, path.data(), length, flags);
    if (!actual)
        return GetLastError();
    if (actual >= length)
        return ERROR_FILENAME_EXCED_RANGE;
    path.resize(actual);
    return ERROR_SUCCESS;
}

inline DWORD CheckOpenedLeaf(HANDLE parent, std::wstring_view leaf, HANDLE file, std::wstring& path)
{
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;
    std::wstring parentPath;
    DWORD error = OpenedPath(parent, parentPath);
    if (error)
        return error;
    if ((error = OpenedPath(file, path)) != ERROR_SUCCESS)
        return error;
    if (parentPath.back() != L'\\')
        parentPath += L'\\';
    parentPath += leaf;
    return UacDirectoryLeaf::Equal(parentPath, path) ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

inline std::wstring RecoveryName(const GUID& nonce)
{
    wchar_t guid[39]{};
    return StringFromGUID2(nonce, guid, ARRAYSIZE(guid)) ? std::wstring(L".samandarin-move-") + guid : std::wstring{};
}

inline DWORD ValidateStorage(HANDLE parent, HANDLE storage, HANDLE journal, PSECURITY_DESCRIPTOR security,
                             UacDirectoryLeaf::ParentIdentity& original, UacDirectoryLeaf::ParentIdentity& recovery,
                             FILE_ID_INFO& journalId)
{
    DWORD error = UacDirectoryLeaf::ReadParentIdentity(parent, original);
    if (!error)
        error = UacDirectoryLeaf::ReadParentIdentity(storage, recovery);
    if (error)
        return error;
    if (!UacDirectoryLeaf::Equal(original.volume, recovery.volume) ||
        original.file.VolumeSerialNumber != recovery.file.VolumeSerialNumber)
        return ERROR_NOT_SAME_DEVICE;
    if ((error = UacRecoveryJournal::CheckPrivate(storage, security)) != ERROR_SUCCESS)
        return error;
    if (UacRenameLeaf::Detail::SameSource(original.file, recovery.file))
        return ERROR_INVALID_PARAMETER;
    if ((error = UacRenameLeaf::Detail::ReadSourceIdentity(journal, journalId)) != ERROR_SUCCESS)
        return error;
    return UacRecoveryJournal::CheckPrivate(journal, security);
}

inline DWORD CheckIdentity(HANDLE file, const FILE_ID_INFO& expected)
{
    FILE_ID_INFO actual{};
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    FILE_STANDARD_INFO standard{};
    if (!GetFileInformationByHandleEx(file, FileIdInfo, &actual, sizeof(actual)) ||
        !GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileInformationByHandleEx(file, FileStandardInfo, &standard, sizeof(standard)))
        return GetLastError();
    return !(attributes.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) &&
                   !standard.DeletePending && UacRenameLeaf::Detail::SameSource(actual, expected) ?
               ERROR_SUCCESS : ERROR_FILE_INVALID;
}

inline DWORD DeleteMoveLinkOnClose(HANDLE file)
{
    FILE_BASIC_INFO attributes{};
    if (!GetFileInformationByHandleEx(file, FileBasicInfo, &attributes, sizeof(attributes)))
        return GetLastError();
    FILE_DISPOSITION_INFO_EX remove{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_FORCE_IMAGE_SECTION_CHECK};
    if (attributes.FileAttributes & FILE_ATTRIBUTE_READONLY)
        remove.Flags |= FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE;
    return SetFileInformationByHandle(file, FileDispositionInfoEx, &remove, sizeof(remove))
        ? ERROR_SUCCESS : GetLastError();
}

inline DWORD VerifyDestination(const DestinationRecord& record, const GUID& nonce, Handle& target)
{
    if (record.nonce != nonce || record.length <= volumeLength + 1 || record.length > ARRAYSIZE(record.path) ||
        record.file.VolumeSerialNumber != record.parent.VolumeSerialNumber)
        return ERROR_INVALID_DATA;
    const std::wstring_view path(record.path, record.length);
    constexpr std::wstring_view prefix = LR"(\\?\Volume)";
    const auto volume = path.substr(0, volumeLength);
    GUID id{};
    if (!UacDirectoryLeaf::Equal(volume.substr(0, prefix.size()), prefix) ||
        FAILED(CLSIDFromString(std::wstring(volume.substr(prefix.size())).c_str(), &id)) ||
        path[volumeLength] != L'\\')
        return ERROR_INVALID_DATA;
    const size_t last = path.find_last_of(L'\\');
    const auto leaf = path.substr(last + 1);
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_DATA;

    const std::wstring rootPath(path.substr(0, volumeLength + 1));
    const HANDLE rawRoot = CreateFileW(rootPath.c_str(), FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (rawRoot == INVALID_HANDLE_VALUE)
        return GetLastError();
    const Handle root(rawRoot, CloseHandle);
    Handle parent(nullptr, CloseHandle);
    UacDirectoryLeaf::ParentIdentity actual;
    DWORD error;
    if (last == volumeLength)
        error = UacDirectoryLeaf::ReadParentIdentity(root.get(), actual);
    else
    {
        HANDLE rawParent = nullptr;
        const auto relative = path.substr(volumeLength + 1, last - volumeLength - 1);
        error = UacDirectoryLeaf::OpenParentBelowRoot(root.get(), relative, rawParent, actual);
        parent.reset(rawParent);
    }
    if (error)
        return error;
    if (!UacDirectoryLeaf::SameParent(actual, {record.parent, std::wstring(volume)}))
        return ERROR_FILE_INVALID;

    HANDLE raw = nullptr;
    error = UacRenameLeaf::Detail::OpenSource(parent ? parent.get() : root.get(), leaf, raw,
                                              FILE_READ_DATA, FILE_SHARE_READ);
    if (error)
        return error;
    target.reset(raw);
    return CheckIdentity(target.get(), record.file);
}

inline DWORD OpenOriginal(HANDLE parent, std::wstring_view leaf, const FILE_ID_INFO& expected, Handle& file, bool pin = false)
{
    HANDLE raw = nullptr;
    const DWORD error = UacRenameLeaf::Detail::OpenSource(parent, leaf, raw, pin ? FILE_READ_DATA : 0,
        pin ? FILE_SHARE_READ : FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    if (error)
        return error;
    file.reset(raw);
    return CheckIdentity(file.get(), expected);
}
}

// The caller holds an exclusive journal and the verified original file
// handle, and supplies the exact security descriptor of the private storage.
// A supplied seal must allow the recovery identity to reopen the file for
// READ_CONTROL, WRITE_DAC and DELETE. The caller controls concurrent writers.
// No recovery name is created until the caller can exclude earlier opens.
inline DWORD PrepareRecovery(HANDLE parent, std::wstring_view leaf, HANDLE source, const FILE_ID_INFO& expected,
                 HANDLE storage, HANDLE journal, PSECURITY_DESCRIPTOR security, std::wstring& recoveryName, PACL seal = nullptr)
{
    using namespace Detail;
    recoveryName.clear();
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;
    UacDirectoryLeaf::ParentIdentity original, recovery;
    FILE_ID_INFO journalId{};
    DWORD error = ValidateStorage(parent, storage, journal, security, original, recovery, journalId);
    if (error)
        return error;

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size))
        return GetLastError();
    if (size.QuadPart)
        return ERROR_BUSY;

    FILE_ID_INFO actual{};
    if ((error = UacRenameLeaf::Detail::ReadSourceIdentity(source, actual)) != ERROR_SUCCESS)
        return error;
    if (!UacRenameLeaf::Detail::SameSource(actual, expected) || UacRenameLeaf::Detail::SameSource(actual, journalId) ||
        actual.VolumeSerialNumber != original.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;
    Handle anchored(nullptr, CloseHandle);
    if ((error = OpenOriginal(parent, leaf, expected, anchored)) != ERROR_SUCCESS)
        return error;

    Record record{};
    std::unique_ptr<DaclRecord> dacl;
    FILE_BASIC_INFO metadata{};
    if (seal)
    {
        if (!IsValidAcl(seal))
            return ERROR_INVALID_ACL;
        dacl = std::make_unique<DaclRecord>();
        if ((error = dacl->original.Read(source)) != ERROR_SUCCESS)
            return error;
        dacl->sealed = dacl->original;
        dacl->sealed.protectedDacl = 1;
        std::memcpy(dacl->sealed.storage, seal, seal->AclSize);
        if (!GetFileInformationByHandleEx(source, FileBasicInfo, &metadata, sizeof(metadata)))
            return GetLastError();
        record.version = !(metadata.FileAttributes & FILE_ATTRIBUTE_ARCHIVE) ? clearArchiveVersion : sealedVersion;
    }
    if (FAILED(CoCreateGuid(&record.nonce)))
        return ERROR_GEN_FAILURE;
    auto alias = RecoveryName(record.nonce);
    if (alias.empty())
        return ERROR_GEN_FAILURE;
    record.originalParent = original.file;
    record.privateParent = recovery.file;
    record.file = actual;
    if (original.volume.size() != ARRAYSIZE(record.volume))
        return ERROR_FILE_INVALID;
    std::memcpy(record.volume, original.volume.data(), sizeof(record.volume));
    record.length = static_cast<DWORD>(leaf.size());
    std::memcpy(record.leaf, leaf.data(), leaf.size() * sizeof(wchar_t));
    if ((error = UacRecoveryJournal::WriteRecord(journal, record)) != ERROR_SUCCESS)
        return error;
    if (dacl)
    {
        DWORD written = 0;
        if (!WriteFile(journal, dacl.get(), sizeof(*dacl), &written, nullptr))
            return GetLastError();
        if (written != sizeof(*dacl))
            return ERROR_WRITE_FAULT;
        if (record.version == clearArchiveVersion)
        {
            if (!WriteFile(journal, &metadata.LastWriteTime.QuadPart, sizeof(LONGLONG), &written, nullptr))
                return GetLastError();
            if (written != sizeof(LONGLONG))
                return ERROR_WRITE_FAULT;
        }
        if (!FlushFileBuffers(journal))
            return GetLastError();
    }
    if (dacl && (error = dacl->sealed.Apply(source)) != ERROR_SUCCESS)
        return error;
    recoveryName = std::move(alias);
    return ERROR_SUCCESS;
}

inline DWORD Arm(HANDLE parent, std::wstring_view leaf, HANDLE source, const FILE_ID_INFO& expected,
                 HANDLE storage, HANDLE journal, PSECURITY_DESCRIPTOR security, std::wstring& recoveryName, PACL seal = nullptr)
{
    const DWORD error = PrepareRecovery(parent, leaf, source, expected, storage, journal, security, recoveryName, seal);
    return error ? error : UacRenameLeaf::Detail::NativeHardLink(source, storage, recoveryName);
}

// The caller still holds the published destination without write or delete
// sharing. A flushed receipt must precede any irreversible source removal.
inline DWORD RememberDestination(HANDLE journal, PSECURITY_DESCRIPTOR journalSecurity, const FILE_ID_INFO& sourceId,
                                 HANDLE targetParent, std::wstring_view targetLeaf, HANDLE target,
                                 const FILE_ID_INFO& targetId)
{
    using namespace Detail;
    if (!UacDirectoryLeaf::IsComponent(targetLeaf))
        return ERROR_INVALID_NAME;
    DWORD error = UacRecoveryJournal::CheckPrivate(journal, journalSecurity);
    if (error)
        return error;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size))
        return GetLastError();
    Record record{};
    DWORD bytes = 0;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) || !ReadFile(journal, &record, sizeof(record), &bytes, nullptr))
        return GetLastError();
    if (bytes != sizeof(record) || (record.version != sealedVersion && record.version != clearArchiveVersion) ||
        size.QuadPart != SealedRecordSize(record.version) || !UacRenameLeaf::Detail::SameSource(record.file, sourceId))
        return ERROR_FILE_INVALID;

    UacDirectoryLeaf::ParentIdentity parent;
    if ((error = UacDirectoryLeaf::ReadParentIdentity(targetParent, parent)) || (error = CheckIdentity(target, targetId)))
        return error;
    if (targetId.VolumeSerialNumber != parent.file.VolumeSerialNumber ||
        UacRenameLeaf::Detail::SameSource(record.file, targetId))
        return ERROR_FILE_INVALID;
    std::wstring path;
    if ((error = CheckOpenedLeaf(targetParent, targetLeaf, target, path)) != ERROR_SUCCESS)
        return error;

    const auto receipt = std::make_unique<DestinationRecord>();
    receipt->nonce = record.nonce;
    receipt->parent = parent.file;
    receipt->file = targetId;
    receipt->length = static_cast<DWORD>(path.size());
    std::memcpy(receipt->path, path.data(), path.size() * sizeof(wchar_t));
    if (!SetFilePointerEx(journal, size, nullptr, FILE_BEGIN) ||
        !WriteFile(journal, receipt.get(), sizeof(*receipt), &bytes, nullptr))
        return GetLastError();
    if (bytes != sizeof(*receipt))
        return ERROR_WRITE_FAULT;
    return FlushFileBuffers(journal) ? ERROR_SUCCESS : GetLastError();
}

inline DWORD Recover(HANDLE parent, HANDLE storage, HANDLE journal, PSECURITY_DESCRIPTOR security)
{
    using namespace Detail;
    UacDirectoryLeaf::ParentIdentity original, recovery;
    FILE_ID_INFO journalId{};
    DWORD error = ValidateStorage(parent, storage, journal, security, original, recovery, journalId);
    if (error)
        return error;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size))
        return GetLastError();
    if (!size.QuadPart)
        return ERROR_SUCCESS;
    Record record{};
    DWORD bytes = 0;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) || !ReadFile(journal, &record, sizeof(record), &bytes, nullptr))
        return GetLastError();
    const bool clearArchive = record.version == clearArchiveVersion;
    const bool hasDacl = record.version == sealedVersion || clearArchive;
    const LONGLONG sealedBytes = SealedRecordSize(record.version);
    const LONGLONG completedBytes = sealedBytes + sizeof(DestinationRecord);
    if (bytes != sizeof(record) || (!hasDacl && record.version != 1) ||
        (hasDacl ? size.QuadPart < sealedBytes || size.QuadPart > completedBytes : size.QuadPart != sizeof(Record)) ||
        !record.length || record.length > ARRAYSIZE(record.leaf) ||
        record.file.VolumeSerialNumber != original.file.VolumeSerialNumber ||
        UacRenameLeaf::Detail::SameSource(record.file, journalId))
        return ERROR_INVALID_DATA;
    if (!UacDirectoryLeaf::Equal(original.volume, std::wstring_view(record.volume, ARRAYSIZE(record.volume))) ||
        !UacRenameLeaf::Detail::SameSource(original.file, record.originalParent) ||
        !UacRenameLeaf::Detail::SameSource(recovery.file, record.privateParent))
        return ERROR_FILE_INVALID;
    const std::wstring_view leaf(record.leaf, record.length);
    const auto name = RecoveryName(record.nonce);
    if (!UacDirectoryLeaf::IsComponent(leaf) || name.empty())
        return ERROR_INVALID_DATA;

    std::unique_ptr<DaclRecord> dacl;
    if (hasDacl)
    {
        dacl = std::make_unique<DaclRecord>();
        if (!ReadFile(journal, dacl.get(), sizeof(*dacl), &bytes, nullptr))
            return GetLastError();
        if (bytes != sizeof(*dacl) || !dacl->original.Valid() || !dacl->sealed.Valid() || !dacl->sealed.protectedDacl)
            return ERROR_INVALID_DATA;
    }

    LONGLONG recordedWrite = 0;
    if (clearArchive)
    {
        if (!ReadFile(journal, &recordedWrite, sizeof(recordedWrite), &bytes, nullptr))
            return GetLastError();
        if (bytes != sizeof(recordedWrite))
            return ERROR_INVALID_DATA;
    }

    HANDLE raw = nullptr;
    const DWORD access = hasDacl ? READ_CONTROL | WRITE_DAC | (clearArchive ? FILE_WRITE_ATTRIBUTES : 0) : FILE_READ_DATA;
    error = UacRenameLeaf::Detail::OpenSource(storage, name, raw, DELETE | access, FILE_SHARE_READ);
    if (error && error != ERROR_FILE_NOT_FOUND)
        return error;
    Handle retained(raw, CloseHandle);
    if (retained && (error = CheckIdentity(retained.get(), record.file)) != ERROR_SUCCESS)
        return error;
    if (retained && !(access & FILE_WRITE_ATTRIBUTES))
    {
        FILE_BASIC_INFO attributes{};
        if (!GetFileInformationByHandleEx(retained.get(), FileBasicInfo, &attributes, sizeof(attributes)))
            return GetLastError();
        if (attributes.FileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            // Retain the ordinary recovery access mask unless atomic read-only
            // unlink needs the extra right. Recheck the journaled identity.
            retained.reset();
            raw = nullptr;
            error = UacRenameLeaf::Detail::OpenSource(storage, name, raw,
                DELETE | access | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ);
            retained.reset(raw);
            if (error || (error = CheckIdentity(retained.get(), record.file)))
                return error;
        }
    }

    Handle source(nullptr, CloseHandle);
    Handle metadata(nullptr, CloseHandle);
    if (dacl)
    {
        raw = nullptr;
        error = UacRenameLeaf::Detail::OpenSource(parent, leaf, raw, access,
                                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        source.reset(raw);
        if (error == ERROR_FILE_NOT_FOUND && !retained && size.QuadPart == completedBytes)
        {
            const auto receipt = std::make_unique<DestinationRecord>();
            if (!ReadFile(journal, receipt.get(), sizeof(*receipt), &bytes, nullptr))
                return GetLastError();
            if (bytes != sizeof(*receipt))
                return ERROR_INVALID_DATA;
            Handle target(nullptr, CloseHandle);
            error = VerifyDestination(*receipt, record.nonce, target);
            return error ? error : UacRecoveryJournal::ClearRecord(journal);
        }
        if (error == ERROR_FILE_NOT_FOUND && retained)
        {
            if ((error = CheckOrRestoreDacl(retained.get(), *dacl, false)) != ERROR_SUCCESS)
                return error;
            // Restore the name before restoring an inherited ACL. Its original
            // parent, not the recovery directory, defines inheritance.
            error = UacRenameLeaf::Detail::NativeRename(retained.get(), parent, leaf);
            if (error)
                return error;
            source = std::move(retained);
            if (clearArchive)
            {
                // The rename handle includes DELETE; the later read pin must
                // exclude deletion. Reopen only the metadata rights while this
                // exact restored name is still pinned by the rename handle.
                raw = nullptr;
                error = UacRenameLeaf::Detail::OpenSource(parent, leaf, raw, access,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
                Handle restored(raw, CloseHandle);
                if (error || (error = CheckIdentity(restored.get(), record.file)))
                    return error;
                source = std::move(restored);
            }
        }
        if (error || (error = CheckIdentity(source.get(), record.file)) ||
            (error = CheckOrRestoreDacl(source.get(), *dacl, !clearArchive)))
            return error;
        if (clearArchive)
            metadata = std::move(source);
        else
            source.reset();
    }

    error = OpenOriginal(parent, leaf, record.file, source, true);
    if (error == ERROR_FILE_NOT_FOUND && retained)
    {
        // Rename the exact recovery link back. A late collision is still
        // rejected by NTFS without replacing the new file.
        error = UacRenameLeaf::Detail::NativeRename(retained.get(), parent, leaf);
    }
    else if (!error && retained)
    {
        // A read pin on the original name blocks POSIX replacement while
        // we delete the retained link to the same file.
        if ((error = DeleteMoveLinkOnClose(retained.get())) != ERROR_SUCCESS)
            return error;
        retained.reset();
        // Keep the original pin until the alias is actually absent. If an
        // older alias handle delays the unlink, leave the record for retry.
        raw = nullptr;
        error = UacRenameLeaf::Detail::OpenSource(storage, name, raw, 0,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        const Handle remaining(raw, CloseHandle);
        if (error != ERROR_FILE_NOT_FOUND)
            return error ? error : ERROR_FILE_INVALID;
        error = ERROR_SUCCESS;
    }
    if (error)
        return error;
    // Removing the alias and restoring the original DACL can set Archive.
    // Finish the repair after both changes, while the original data is pinned.
    if (metadata && (error = CheckOrRestoreDacl(metadata.get(), *dacl, true, &recordedWrite)))
        return error;
    return UacRecoveryJournal::ClearRecord(journal);
}
}
