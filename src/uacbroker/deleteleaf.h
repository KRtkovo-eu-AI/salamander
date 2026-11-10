/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-10 | Delete only the verified empty directory by its exact handle. | Leave tree traversal and confirmations in the native worker. | #731 | GPT-6.1 Sol |
| 2026-11-09 | Share the verified deletion of an opened leaf. | Retire a copied source only while its exact destination remains pinned. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Detach a verified junction by handle. | Reuse the junction format check and reject other reparse types. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Detach a verified directory symbolic link by handle. | Use an anchored no-follow open and reject other directory reparse tags. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Delete a verified file symbolic link by handle. | Check the reparse tag and retain the ordinary-file rules. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Delete a single ordinary file by its verified NTFS handle. | Avoid reopening a path for deletion or changing file attributes. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "f7pilot.h"
#include "renameleaf.h"

#include <memory>

namespace UacDeleteLeaf
{
inline UacRenameLeaf::MoveKind SourceKind(UacF7::Operation operation)
{
    if (operation == UacF7::Operation::DeleteDirectory)
        return UacRenameLeaf::MoveKind::RenameDirectory;
    if (operation == UacF7::Operation::DeleteJunction)
        return UacRenameLeaf::MoveKind::RenameJunction;
    if (operation == UacF7::Operation::DeleteDirectorySymlink)
        return UacRenameLeaf::MoveKind::RenameDirectorySymlink;
    if (operation == UacF7::Operation::DeleteSymlink)
        return UacRenameLeaf::MoveKind::RenameSymlink;
    return UacRenameLeaf::MoveKind::RenameFile;
}

inline DWORD CheckAttributes(HANDLE file, UacF7::Operation operation)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if (!GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attributes, sizeof(attributes)))
        return GetLastError();
    if (operation == UacF7::Operation::DeleteDirectory)
        return UacF7::IsPlainDeleteDirectory(attributes.FileAttributes) ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
    const bool link = operation != UacF7::Operation::Delete;
    const bool junction = operation == UacF7::Operation::DeleteJunction;
    const DWORD expectedTag = junction ? IO_REPARSE_TAG_MOUNT_POINT : IO_REPARSE_TAG_SYMLINK;
    if (link && attributes.ReparseTag != expectedTag)
        return ERROR_REPARSE_TAG_INVALID;
    const bool directory = UacRenameLeaf::IsDirectory(SourceKind(operation));
    const bool supported = link ? UacF7::IsPlainDeleteLink(attributes.FileAttributes, directory)
                                : UacF7::IsPlainDeleteFile(attributes.FileAttributes);
    return supported ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
}

inline DWORD DeleteOpenedLeaf(HANDLE file, const FILE_ID_INFO& expectedSource, UacF7::Operation operation)
{
    FILE_ID_INFO actualSource{};
    DWORD error = UacRenameLeaf::Detail::ReadSourceIdentity(file, actualSource, SourceKind(operation));
    if (!error && !UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    if (!error)
        error = CheckAttributes(file, operation);
    if (error)
        return error;
    // Detach the verified name on close even while the host retains its identity handle.
    FILE_DISPOSITION_INFO_EX disposition{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS |
                                         FILE_DISPOSITION_FLAG_FORCE_IMAGE_SECTION_CHECK};
    return SetFileInformationByHandle(file, FileDispositionInfoEx, &disposition, sizeof(disposition))
        ? ERROR_SUCCESS : GetLastError();
}

inline DWORD DeleteCopiedSource(const std::wstring& source, const std::wstring& target,
                                const FILE_ID_INFO& expectedSource, const FILE_ID_INFO& expectedTarget)
{
    const HANDLE pinned = CreateFileW(target.c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                                      nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (pinned == INVALID_HANDLE_VALUE)
    {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? ERROR_FILE_INVALID : error;
    }
    const std::unique_ptr<void, decltype(&CloseHandle)> targetHandle(pinned, CloseHandle);
    FILE_ID_INFO actualTarget{};
    DWORD error = UacRenameLeaf::Detail::ReadSourceIdentity(pinned, actualTarget);
    if (error)
        return error;
    if (!UacRenameLeaf::Detail::SameSource(actualTarget, expectedTarget))
        return ERROR_FILE_INVALID;

    const HANDLE input = CreateFileW(source.c_str(), DELETE | FILE_READ_ATTRIBUTES, 0, nullptr,
                                      OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (input == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? ERROR_FILE_INVALID : error;
    }
    const std::unique_ptr<void, decltype(&CloseHandle)> sourceHandle(input, CloseHandle);
    return DeleteOpenedLeaf(input, expectedSource, UacF7::Operation::Delete);
}

inline DWORD DeleteVerifiedLeaf(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                const FILE_ID_INFO& expectedSource, std::wstring_view leaf,
                                UacF7::Operation operation = UacF7::Operation::Delete)
{
    if (!UacF7::IsLeafDelete(operation))
        return ERROR_INVALID_PARAMETER;
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;
    UacDirectoryLeaf::ParentIdentity actualParent;
    DWORD error = UacDirectoryLeaf::ReadParentIdentity(parent, actualParent);
    if (error != ERROR_SUCCESS)
        return error;
    if (!UacDirectoryLeaf::SameParent(expectedParent, actualParent) ||
        expectedSource.VolumeSerialNumber != actualParent.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;

    HANDLE file = nullptr;
    // A read-attributes identity handle can coexist with this exclusive data open.
    const auto kind = SourceKind(operation);
    error = UacRenameLeaf::Detail::OpenSource(parent, leaf, file, DELETE, 0, kind);
    if (error != ERROR_SUCCESS)
        return error;
    error = DeleteOpenedLeaf(file, expectedSource, operation);
    CloseHandle(file);
    return error;
}
}
