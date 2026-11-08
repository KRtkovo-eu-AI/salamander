/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Allow a creation descriptor for native files. | Apply mandatory integrity before the staged file is visible. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Move one verified file across directories on the same volume. | Reuse the no-replace native rename without changing ordinary F6. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share anchored leaf opens with the staging file. | Keep temporary file creation relative to the verified target. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Share the anchored source open with basic metadata changes. | Keep access and sharing specific to each operation. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Share the anchored source open and identity check with the panel host. | Capture the file identity before requesting elevation. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add an NTFS rename primitive for a single file under a verified parent. | Establish a no-overwrite command boundary before broker and panel integration. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "directoryleaf.h"
#include <cstddef>

extern "C" NTSYSAPI NTSTATUS NTAPI NtSetInformationFile(
    HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

namespace UacRenameLeaf
{
namespace Detail
{
inline DWORD NativeRename(HANDLE source, HANDLE targetParent, std::wstring_view leaf)
{
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;

    // FILE_RENAME_INFORMATION from ntifs.h. Its variable-length tail is
    // backed by storage for one NTFS component.
    struct
    {
        BOOLEAN replaceIfExists;
        HANDLE rootDirectory;
        ULONG fileNameLength;
        WCHAR fileName[UacDirectoryLeaf::maxComponent + 1];
    } info{};
    static_assert(offsetof(decltype(info), rootDirectory) == offsetof(FILE_RENAME_INFO, RootDirectory));
    static_assert(offsetof(decltype(info), fileNameLength) == offsetof(FILE_RENAME_INFO, FileNameLength));
    static_assert(offsetof(decltype(info), fileName) == offsetof(FILE_RENAME_INFO, FileName));
    info.rootDirectory = targetParent;
    info.fileNameLength = static_cast<ULONG>(leaf.size() * sizeof(wchar_t));
    std::memcpy(info.fileName, leaf.data(), info.fileNameLength);
    IO_STATUS_BLOCK io{};
    constexpr auto renameClass = static_cast<FILE_INFORMATION_CLASS>(10); // FileRenameInformation
    const NTSTATUS status = NtSetInformationFile(source, &io, &info, sizeof(info), renameClass);
    return status >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(status);
}

inline DWORD OpenFile(HANDLE parent, std::wstring_view leaf, HANDLE& source, ACCESS_MASK extraAccess, ULONG sharing,
                      ULONG disposition, ULONG options, PSECURITY_DESCRIPTOR security = nullptr)
{
    const auto bytes = static_cast<USHORT>(leaf.size() * sizeof(wchar_t));
    UNICODE_STRING objectName{bytes, bytes, const_cast<wchar_t*>(leaf.data())};
    OBJECT_ATTRIBUTES attributes{};
    InitializeObjectAttributes(&attributes, &objectName, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, parent, security);
    IO_STATUS_BLOCK io{};
    const ACCESS_MASK access = FILE_READ_ATTRIBUTES | SYNCHRONIZE | extraAccess;
    const NTSTATUS status = NtCreateFile(&source, access, &attributes, &io, nullptr, FILE_ATTRIBUTE_NORMAL, sharing, disposition,
                                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT | options,
                                         nullptr, 0);
    return status >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(status);
}

inline DWORD OpenSource(HANDLE parent, std::wstring_view leaf, HANDLE& source, ACCESS_MASK extraAccess, ULONG sharing)
{
    return OpenFile(parent, leaf, source, extraAccess, sharing, FILE_OPEN, 0);
}

inline DWORD ReadSourceIdentity(HANDLE source, FILE_ID_INFO& identity)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    FILE_STANDARD_INFO standard{};
    if (!GetFileInformationByHandleEx(source, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileInformationByHandleEx(source, FileStandardInfo, &standard, sizeof(standard)) ||
        !GetFileInformationByHandleEx(source, FileIdInfo, &identity, sizeof(identity)))
        return GetLastError();
    if (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        return ERROR_REPARSE_TAG_INVALID;
    if (standard.Directory || standard.NumberOfLinks != 1 || standard.DeletePending)
        return ERROR_NOT_SUPPORTED;
    return ERROR_SUCCESS;
}

inline DWORD CheckTransferConnection(HANDLE connection, ULONGLONG deadline)
{
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    if (!connection)
        return ERROR_SUCCESS;
    DWORD pending = 0;
    if (!PeekNamedPipe(connection, nullptr, 0, nullptr, &pending, nullptr))
        return GetLastError();
    return pending ? ERROR_INVALID_DATA : ERROR_SUCCESS;
}

inline bool SameSource(const FILE_ID_INFO& actual, const FILE_ID_INFO& expected)
{
    return actual.VolumeSerialNumber == expected.VolumeSerialNumber &&
           std::memcmp(actual.FileId.Identifier, expected.FileId.Identifier, sizeof(actual.FileId.Identifier)) == 0;
}
}

// The caller keeps the original parent and source identity handles alive
// until completion. A cross-directory move also pins its destination parent.
inline DWORD MoveVerifiedFile(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                              const FILE_ID_INFO& expectedSource, std::wstring_view from,
                              HANDLE targetParent, const UacDirectoryLeaf::ParentIdentity& expectedTarget,
                              std::wstring_view to, bool ordinaryOnly = true,
                              HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    using namespace UacDirectoryLeaf;
    if (!IsComponent(from) || !IsComponent(to))
        return ERROR_INVALID_NAME;
    if (!Equal(expectedParent.volume, expectedTarget.volume) ||
        expectedParent.file.VolumeSerialNumber != expectedTarget.file.VolumeSerialNumber)
        return ERROR_NOT_SAME_DEVICE;
    if (SameParent(expectedParent, expectedTarget) && Equal(from, to))
        return ERROR_NOT_SUPPORTED;

    ParentIdentity actual;
    DWORD error = ReadParentIdentity(parent, actual);
    if (error != ERROR_SUCCESS)
        return error;
    if (!SameParent(expectedParent, actual) || expectedSource.VolumeSerialNumber != actual.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;

    if (parent != targetParent)
    {
        ParentIdentity actualTarget;
        error = ReadParentIdentity(targetParent, actualTarget);
        if (error)
            return error;
        if (!SameParent(expectedTarget, actualTarget))
            return ERROR_FILE_INVALID;
    }
    else if (!SameParent(expectedTarget, actual))
        return ERROR_FILE_INVALID;

    HANDLE source = nullptr;
    error = Detail::OpenSource(parent, from, source, DELETE, FILE_SHARE_READ);
    if (error != ERROR_SUCCESS)
        return error;
    FILE_ID_INFO actualSource{};
    error = Detail::ReadSourceIdentity(source, actualSource);
    if (error == ERROR_SUCCESS && !Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    if (error == ERROR_SUCCESS && ordinaryOnly)
    {
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        if (!GetFileInformationByHandleEx(source, FileAttributeTagInfo, &attributes, sizeof(attributes)))
            error = GetLastError();
        else if (attributes.FileAttributes != FILE_ATTRIBUTE_NORMAL && attributes.FileAttributes != FILE_ATTRIBUTE_ARCHIVE)
            error = ERROR_NOT_SUPPORTED;
    }
    if (error == ERROR_SUCCESS)
        error = Detail::CheckTransferConnection(connection, deadline);
    if (error == ERROR_SUCCESS)
        error = Detail::NativeRename(source, targetParent, to);
    CloseHandle(source);
    return error;
}

inline DWORD RenameVerifiedFile(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                const FILE_ID_INFO& expectedSource, std::wstring_view from, std::wstring_view to)
{
    return MoveVerifiedFile(parent, expectedParent, expectedSource, from, parent, expectedParent, to, false);
}
}
