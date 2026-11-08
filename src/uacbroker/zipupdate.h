// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
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

inline DWORD WriteRecord(HANDLE journal, const RecoveryRecord& record)
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

inline DWORD RemoveBackup(HANDLE file)
{
    FILE_DISPOSITION_INFO remove{TRUE};
    return SetFileInformationByHandle(file, FileDispositionInfo, &remove, sizeof(remove))
               ? ERROR_SUCCESS : GetLastError();
}
}

// Only use a broker-owned, exclusively held journal. Recovery is scoped to
// the same verified parent and exact archive name. It never replaces a name.
inline DWORD Recover(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expected,
                     std::wstring_view leaf, HANDLE journal, ReplacementKind kind = ReplacementKind::Zip)
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
    if (size.QuadPart != sizeof(record))
        return ERROR_INVALID_DATA;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) ||
        !ReadFile(journal, &record, sizeof(record), &read, nullptr))
        return GetLastError();
    const auto* end = std::find(std::begin(record.leaf), std::end(record.leaf), L'\0');
    const std::wstring_view recorded(record.leaf, end - record.leaf);
    if (read != sizeof(record) || record.version != 1 || !IsReplacementLeaf(recorded, kind) ||
        recorded != leaf || !SameSource(record.parent, expected.file) ||
        record.original.VolumeSerialNumber != expected.file.VolumeSerialNumber ||
        record.replacement.VolumeSerialNumber != expected.file.VolumeSerialNumber ||
        SameSource(record.original, record.replacement))
        return ERROR_INVALID_DATA;
    const auto backupName = BackupName(record.nonce, kind);
    if (backupName.empty())
        return ERROR_INVALID_DATA;

    HANDLE rawFinal = nullptr;
    error = Open(parent, leaf, rawFinal, 0);
    if (error && error != ERROR_FILE_NOT_FOUND)
        return error;
    Handle finalFile(rawFinal, CloseHandle);
    HANDLE rawBackup = nullptr;
    error = Open(parent, backupName, rawBackup, DELETE);
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
    if (!finalFile)
        error = UacRenameLeaf::Detail::NativeRename(backup.get(), parent, leaf);
    else if ((error = CheckIdentity(finalFile.get(), record.replacement)) == ERROR_SUCCESS)
        error = RemoveBackup(backup.get());
    if (error)
        return error;
    backup.reset();
    return ClearRecord(journal);
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

// The caller supplies the original opened for delete without sharing write
// or delete, and a fully written, exclusive delete-on-close stage in its
// parent. The journal must be trusted, exclusive and empty.
inline PublishResult Publish(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                             std::wstring_view leaf, const Revision& expectedOriginal, Handle& original,
                             HANDLE stage, HANDLE journal, HANDLE connection, ULONGLONG deadline,
                             FILE_ID_INFO* published = nullptr, ReplacementKind kind = ReplacementKind::Zip)
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

    RecoveryRecord record;
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
    if ((error = WriteRecord(journal, record)) != ERROR_SUCCESS)
        return {error, ClearRecord(journal)};
    const auto exchange = ReplaceStagedFile(parent, original.get(), stage, leaf, backupName, connection, deadline);
    if (exchange.error)
        return {exchange.error, exchange.rollbackError ? exchange.rollbackError : ClearRecord(journal)};

    if (published)
        *published = record.replacement;
    DWORD cleanup = RemoveBackup(original.get());
    original.reset();
    if (!cleanup)
        cleanup = ClearRecord(journal);
    return {ERROR_SUCCESS, cleanup, true};
}
}
