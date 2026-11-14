/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-11 | Retire a read-only copied source only while its exact destination is pinned. | Keep F6 receipt identity checks and avoid clearing source attributes. | | GPT-6.1 Sol |
| 2026-11-11 | Delete verified objects with common attributes without clearing ReadOnly. | Use atomic disposition and retain strict copied-source retirement. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Classify supported links from their attributes and reparse tag. | Share link type validation between listing and deletion. | #731 | GPT-6.1 Sol |
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
inline UacF7::Operation LinkOperation(DWORD attributes, DWORD tag)
{
    using UacF7::Operation;
    const bool directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (!UacF7::IsEligibleDeleteLink(attributes, directory))
        return Operation::Delete; // Unsupported as a link.
    if (tag == IO_REPARSE_TAG_SYMLINK)
        return directory ? Operation::DeleteDirectorySymlink : Operation::DeleteSymlink;
    if (tag == IO_REPARSE_TAG_MOUNT_POINT && directory)
        return Operation::DeleteJunction;
    return Operation::Delete;
}

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

inline DWORD CheckAttributes(HANDLE file, UacF7::Operation operation, DWORD* actualAttributes = nullptr)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if (!GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attributes, sizeof(attributes)))
        return GetLastError();
    if (actualAttributes)
        *actualAttributes = attributes.FileAttributes;
    if (operation == UacF7::Operation::DeleteDirectory)
        return UacF7::IsEligibleDeleteDirectory(attributes.FileAttributes) ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
    const bool link = operation != UacF7::Operation::Delete;
    const bool junction = operation == UacF7::Operation::DeleteJunction;
    const DWORD expectedTag = junction ? IO_REPARSE_TAG_MOUNT_POINT : IO_REPARSE_TAG_SYMLINK;
    if (link && attributes.ReparseTag != expectedTag)
        return ERROR_REPARSE_TAG_INVALID;
    const bool directory = UacRenameLeaf::IsDirectory(SourceKind(operation));
    const bool supported = link ? UacF7::IsEligibleDeleteLink(attributes.FileAttributes, directory)
                                : UacF7::IsEligibleDeleteFile(attributes.FileAttributes);
    return supported ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
}

inline DWORD ValidateOpenedLeaf(HANDLE file, const FILE_ID_INFO& expectedSource,
                                 UacF7::Operation operation, DWORD& attributes)
{
    FILE_ID_INFO actualSource{};
    DWORD error = UacRenameLeaf::Detail::ReadSourceIdentity(file, actualSource, SourceKind(operation));
    if (!error && !UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    return error ? error : CheckAttributes(file, operation, &attributes);
}

inline DWORD DeleteOpenedLeaf(HANDLE file, const FILE_ID_INFO& expectedSource, UacF7::Operation operation,
                               bool copiedSource = false)
{
    DWORD attributes = 0;
    DWORD error = ValidateOpenedLeaf(file, expectedSource, operation, attributes);
    if (error)
        return error;
    // F6 source retirement keeps its existing receipt and representation contract.
    if (copiedSource && !UacF7::IsNativePlainCopyFile(attributes))
        return ERROR_NOT_SUPPORTED;
    // Detach the verified name on close even while the host retains its identity handle.
    // The atomic flag avoids clearing ReadOnly on cancellation or a failed unlink.
    FILE_DISPOSITION_INFO_EX disposition{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS |
                                         FILE_DISPOSITION_FLAG_FORCE_IMAGE_SECTION_CHECK};
    if (attributes & FILE_ATTRIBUTE_READONLY)
        disposition.Flags |= FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE;
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

    HANDLE input = CreateFileW(source.c_str(), DELETE | FILE_READ_ATTRIBUTES, 0, nullptr,
                                      OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (input == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? ERROR_FILE_INVALID : error;
    }
    std::unique_ptr<void, decltype(&CloseHandle)> sourceHandle(input, CloseHandle);
    DWORD attributes = 0;
    error = ValidateOpenedLeaf(input, expectedSource, UacF7::Operation::Delete, attributes);
    if (error)
        return error;
    if (attributes & FILE_ATTRIBUTE_READONLY)
    {
        sourceHandle.reset();
        input = CreateFileW(source.c_str(), DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, 0, nullptr,
                            OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (input == INVALID_HANDLE_VALUE)
            return GetLastError();
        sourceHandle.reset(input);
    }
    return DeleteOpenedLeaf(sourceHandle.get(), expectedSource, UacF7::Operation::Delete, true);
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
    DWORD attributes = 0;
    error = ValidateOpenedLeaf(file, expectedSource, operation, attributes);
    if (!error && (attributes & FILE_ATTRIBUTE_READONLY))
    {
        // NTFS requires FILE_WRITE_ATTRIBUTES for IGNORE_READONLY_ATTRIBUTE.
        // Request it only for ReadOnly objects so ordinary DELETE-only access still works.
        // The new handle is anchored and DeleteOpenedLeaf checks identity and type again.
        CloseHandle(file);
        file = nullptr;
        error = UacRenameLeaf::Detail::OpenSource(parent, leaf, file, DELETE | FILE_WRITE_ATTRIBUTES, 0, kind);
    }
    if (!error)
        error = DeleteOpenedLeaf(file, expectedSource, operation);
    if (file)
        CloseHandle(file);
    return error;
}
}
