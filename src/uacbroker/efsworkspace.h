// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Share the private journal helpers with move recovery. | Keep access-control checks and durable writes consistent. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Reuse the anchored hard link helper. | Apply one component-validation boundary to NTFS aliases. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Journal the EFS workspace and its exact objects. | Recover interrupted raw copies without following replacement paths. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "copytree.h"
#include "recoveryjournal.h"

namespace UacEfsWorkspace
{
inline constexpr wchar_t sourceName[] = L"source.efs";
inline constexpr wchar_t payloadName[] = L"payload.efs";

namespace Detail
{
using UacCopyTree::Detail::Handle;
using UacRecoveryJournal::CheckPrivate;
using UacRecoveryJournal::ClearRecord;
using UacRecoveryJournal::WriteRecord;

struct Record
{
    ULONGLONG version = 1;
    GUID nonce{};
    FILE_ID_INFO parent{};
    FILE_ID_INFO directory{};
    FILE_ID_INFO source{};
    FILE_ID_INFO payload{};
};
static_assert(sizeof(Record) == 120);

inline std::wstring DirectoryName(const GUID& nonce)
{
    wchar_t guid[39]{};
    return StringFromGUID2(nonce, guid, ARRAYSIZE(guid)) ? std::wstring(L".samandarin-efs-") + guid : std::wstring{};
}

inline DWORD AllowReopen(HANDLE file)
{
    FILE_DISPOSITION_INFO_EX keep{FILE_DISPOSITION_FLAG_ON_CLOSE};
    return SetFileInformationByHandle(file, FileDispositionInfoEx, &keep, sizeof(keep)) ? ERROR_SUCCESS : GetLastError();
}

inline DWORD CheckFile(HANDLE file, const FILE_ID_INFO& expected, bool alias)
{
    FILE_ID_INFO id{};
    FILE_ATTRIBUTE_TAG_INFO attrs{};
    FILE_STANDARD_INFO standard{};
    if (!GetFileInformationByHandleEx(file, FileIdInfo, &id, sizeof(id)) ||
        !GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attrs, sizeof(attrs)) ||
        !GetFileInformationByHandleEx(file, FileStandardInfo, &standard, sizeof(standard)))
        return GetLastError();
    if ((attrs.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) || standard.DeletePending ||
        (!alias && standard.NumberOfLinks != 1) || !UacRenameLeaf::Detail::SameSource(id, expected))
        return ERROR_FILE_INVALID;
    return ERROR_SUCCESS;
}

inline DWORD OpenForCleanup(HANDLE directory, const wchar_t* name, const FILE_ID_INFO& expected,
                            bool alias, Handle& file)
{
    HANDLE raw = nullptr;
    const DWORD error = UacRenameLeaf::Detail::OpenSource(directory, name, raw, DELETE, FILE_SHARE_READ);
    if (error == ERROR_FILE_NOT_FOUND)
        return ERROR_SUCCESS;
    if (error)
        return error;
    file.reset(raw);
    return CheckFile(file.get(), expected, alias);
}

inline DWORD Cleanup(HANDLE directory, const Record& record)
{
    Handle alias(nullptr, CloseHandle), payload(nullptr, CloseHandle);
    DWORD error = OpenForCleanup(directory, sourceName, record.source, true, alias);
    if (!error)
        error = OpenForCleanup(directory, payloadName, record.payload, false, payload);
    if (error)
        return error;
    if (payload && (error = UacCopyTree::Detail::Remove(payload.get())) != ERROR_SUCCESS)
        return error;
    if (alias && (error = UacCopyTree::Detail::Remove(alias.get())) != ERROR_SUCCESS)
        return error;
    payload.reset();
    alias.reset();
    return UacCopyTree::Detail::Remove(directory);
}
}

// The caller must exclusively hold a trusted journal and a private parent
// on the source volume. It supplies the workspace security descriptor.
inline DWORD Recover(HANDLE parent, const FILE_ID_INFO& parentId, HANDLE journal, PSECURITY_DESCRIPTOR security)
{
    using namespace Detail;
    DWORD error = UacCopyTree::Detail::CheckDirectory(parent, parentId);
    if (!error)
        error = CheckPrivate(parent, security);
    if (error)
        return error;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(journal, &size))
        return GetLastError();
    if (!size.QuadPart)
        return ERROR_SUCCESS;
    if (size.QuadPart != sizeof(Record))
        return ERROR_INVALID_DATA;
    Record record;
    DWORD read = 0;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) ||
        !ReadFile(journal, &record, sizeof(record), &read, nullptr))
        return GetLastError();
    if (read != sizeof(record) || record.version != 1 ||
        !UacRenameLeaf::Detail::SameSource(record.parent, parentId) ||
        record.directory.VolumeSerialNumber != parentId.VolumeSerialNumber ||
        record.source.VolumeSerialNumber != parentId.VolumeSerialNumber)
        return ERROR_INVALID_DATA;
    const auto name = DirectoryName(record.nonce);
    if (name.empty())
        return ERROR_INVALID_DATA;
    Handle directory(nullptr, CloseHandle);
    error = UacCopyTree::Detail::OpenDirectory(parent, name, directory, DELETE | FILE_LIST_DIRECTORY | READ_CONTROL, FILE_OPEN,
                                                     nullptr, FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (error == ERROR_FILE_NOT_FOUND)
        return ClearRecord(journal);
    if (error)
        return error;
    if ((error = UacCopyTree::Detail::CheckDirectory(directory.get(), record.directory)) ||
        (error = CheckPrivate(directory.get(), security)) || (error = Cleanup(directory.get(), record)))
        return error;
    directory.reset();
    return ClearRecord(journal);
}

class Workspace
{
public:
    Workspace() = default;
    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;
    ~Workspace() { Cleanup(); }

    DWORD Create(HANDLE parent, const FILE_ID_INFO& parentId, HANDLE journalHandle, HANDLE source,
                 const FILE_ID_INFO& sourceId, PSECURITY_DESCRIPTOR security)
    {
        using namespace Detail;
        if (sourceId.VolumeSerialNumber != parentId.VolumeSerialNumber)
            return ERROR_NOT_SAME_DEVICE;
        DWORD error = CheckFile(source, sourceId, false);
        if (error)
            return error;
        FILE_ATTRIBUTE_TAG_INFO attrs{};
        if (!GetFileInformationByHandleEx(source, FileAttributeTagInfo, &attrs, sizeof(attrs)))
            return GetLastError();
        if (!(attrs.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED))
            return ERROR_FILE_NOT_ENCRYPTED;
        if ((error = Recover(parent, parentId, journalHandle, security)) != ERROR_SUCCESS)
            return error;
        if (FAILED(CoCreateGuid(&record.nonce)))
            return ERROR_GEN_FAILURE;
        const auto name = DirectoryName(record.nonce);
        if (name.empty())
            return ERROR_GEN_FAILURE;

        record.parent = parentId;
        record.source = sourceId;
        error = UacCopyTree::Detail::OpenDirectory(parent, name, directory,
            DELETE | FILE_ADD_FILE | FILE_LIST_DIRECTORY | READ_CONTROL, FILE_CREATE, security,
            FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_DELETE_ON_CLOSE);
        if (error)
            return error;
        if (!GetFileInformationByHandleEx(directory.get(), FileIdInfo, &record.directory, sizeof(record.directory)))
            error = GetLastError();
        else if (!(error = CheckPrivate(directory.get(), security)) && !(error = WriteRecord(journalHandle, record)))
            error = AllowReopen(directory.get());
        if (error)
        {
            directory.reset(); // still empty and armed for deletion
            return error;
        }
        journal = journalHandle;
        return UacRenameLeaf::Detail::NativeHardLink(source, directory.get(), sourceName);
    }

    DWORD CreatePayload(PSECURITY_DESCRIPTOR security)
    {
        using namespace Detail;
        if (!directory)
            return ERROR_INVALID_HANDLE;
        HANDLE raw = nullptr;
        const DWORD open = UacRenameLeaf::Detail::OpenFile(directory.get(), payloadName, raw, DELETE | FILE_WRITE_DATA | READ_CONTROL,
                                                          0, FILE_CREATE, FILE_DELETE_ON_CLOSE, security);
        if (open)
            return open;
        const Handle payload(raw, CloseHandle);
        if (const DWORD error = CheckPrivate(payload.get(), security))
            return error;
        if (!GetFileInformationByHandleEx(payload.get(), FileIdInfo, &record.payload, sizeof(record.payload)))
            return GetLastError();
        const DWORD error = WriteRecord(journal, record);
        return error ? error : AllowReopen(payload.get());
    }

    HANDLE Directory() const { return directory.get(); }
    const FILE_ID_INFO& PayloadId() const { return record.payload; }

    DWORD Cleanup()
    {
        if (!directory)
            return ERROR_SUCCESS;
        const DWORD error = Detail::Cleanup(directory.get(), record);
        directory.reset();
        return error ? error : Detail::ClearRecord(journal);
    }

private:
    Detail::Record record;
    Detail::Handle directory{nullptr, CloseHandle};
    HANDLE journal = nullptr;
};
}
