// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
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
inline DWORD Arm(HANDLE parent, std::wstring_view leaf, HANDLE source, const FILE_ID_INFO& expected,
                 HANDLE storage, HANDLE journal, PSECURITY_DESCRIPTOR security, std::wstring& recoveryName)
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
    error = UacRenameLeaf::Detail::NativeHardLink(source, storage, alias);
    if (!error)
        recoveryName = std::move(alias);
    return error;
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
    if (size.QuadPart != sizeof(Record))
        return ERROR_INVALID_DATA;

    Record record{};
    DWORD bytes = 0;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) || !ReadFile(journal, &record, sizeof(record), &bytes, nullptr))
        return GetLastError();
    if (bytes != sizeof(record) || record.version != 1 || !record.length || record.length > ARRAYSIZE(record.leaf) ||
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

    HANDLE raw = nullptr;
    error = UacRenameLeaf::Detail::OpenSource(storage, name, raw, DELETE | FILE_READ_DATA, FILE_SHARE_READ);
    if (error && error != ERROR_FILE_NOT_FOUND)
        return error;
    Handle retained(raw, CloseHandle);
    if (retained && (error = CheckIdentity(retained.get(), record.file)) != ERROR_SUCCESS)
        return error;

    Handle source(nullptr, CloseHandle);
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
        FILE_DISPOSITION_INFO remove{TRUE};
        if (!SetFileInformationByHandle(retained.get(), FileDispositionInfo, &remove, sizeof(remove)))
            return GetLastError();
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
    return UacRecoveryJournal::ClearRecord(journal);
}
}
