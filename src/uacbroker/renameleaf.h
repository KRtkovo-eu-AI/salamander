/*
| Date | Summary | Reason | Ticket | Model |
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

inline DWORD OpenSource(HANDLE parent, std::wstring_view leaf, HANDLE& source)
{
    const auto bytes = static_cast<USHORT>(leaf.size() * sizeof(wchar_t));
    UNICODE_STRING objectName{bytes, bytes, const_cast<wchar_t*>(leaf.data())};
    OBJECT_ATTRIBUTES attributes{};
    InitializeObjectAttributes(&attributes, &objectName, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, parent, nullptr);
    IO_STATUS_BLOCK io{};
    const NTSTATUS status = NtCreateFile(&source, DELETE | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                         &attributes, &io, nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OPEN,
                                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                         nullptr, 0);
    return status >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(status);
}

inline DWORD VerifySource(HANDLE source, const FILE_ID_INFO& expected)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    FILE_STANDARD_INFO standard{};
    FILE_ID_INFO actual{};
    if (!GetFileInformationByHandleEx(source, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileInformationByHandleEx(source, FileStandardInfo, &standard, sizeof(standard)) ||
        !GetFileInformationByHandleEx(source, FileIdInfo, &actual, sizeof(actual)))
        return GetLastError();
    if (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        return ERROR_REPARSE_TAG_INVALID;
    if (standard.NumberOfLinks != 1 || standard.DeletePending)
        return ERROR_NOT_SUPPORTED;
    if (actual.VolumeSerialNumber != expected.VolumeSerialNumber ||
        std::memcmp(actual.FileId.Identifier, expected.FileId.Identifier, sizeof(actual.FileId.Identifier)) != 0)
        return ERROR_FILE_INVALID;
    return ERROR_SUCCESS;
}
}

// The caller keeps the original parent and source identity handles alive
// until completion. The same verified parent is used for both names.
inline DWORD RenameVerifiedFile(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                const FILE_ID_INFO& expectedSource, std::wstring_view from, std::wstring_view to)
{
    using namespace UacDirectoryLeaf;
    if (!IsComponent(from) || !IsComponent(to))
        return ERROR_INVALID_NAME;
    if (Equal(from, to))
        return ERROR_NOT_SUPPORTED;

    ParentIdentity actual;
    DWORD error = ReadParentIdentity(parent, actual);
    if (error != ERROR_SUCCESS)
        return error;
    if (!SameParent(expectedParent, actual) || expectedSource.VolumeSerialNumber != actual.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;

    HANDLE source = nullptr;
    error = Detail::OpenSource(parent, from, source);
    if (error != ERROR_SUCCESS)
        return error;
    error = Detail::VerifySource(source, expectedSource);
    if (error == ERROR_SUCCESS)
        error = Detail::NativeRename(source, parent, to);
    CloseHandle(source);
    return error;
}
}
