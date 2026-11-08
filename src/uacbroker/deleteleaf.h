/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Detach a verified junction by handle. | Reuse the junction format check and reject other reparse types. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Detach a verified directory symbolic link by handle. | Use an anchored no-follow open and reject other directory reparse tags. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Delete a verified file symbolic link by handle. | Check the reparse tag and retain the ordinary-file rules. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Delete a single ordinary file by its verified NTFS handle. | Avoid reopening a path for deletion or changing file attributes. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "f7pilot.h"
#include "renameleaf.h"

namespace UacDeleteLeaf
{
inline UacRenameLeaf::MoveKind SourceKind(UacF7::Operation operation)
{
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
    const bool link = operation != UacF7::Operation::Delete;
    const bool junction = operation == UacF7::Operation::DeleteJunction;
    const DWORD expectedTag = junction ? IO_REPARSE_TAG_MOUNT_POINT : IO_REPARSE_TAG_SYMLINK;
    if (link && attributes.ReparseTag != expectedTag)
        return ERROR_REPARSE_TAG_INVALID;
    const bool directory = UacRenameLeaf::IsDirectoryLink(SourceKind(operation));
    const bool supported = link ? UacF7::IsPlainDeleteLink(attributes.FileAttributes, directory)
                                : UacF7::IsPlainDeleteFile(attributes.FileAttributes);
    return supported ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
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
    FILE_ID_INFO actualSource{};
    error = UacRenameLeaf::Detail::ReadSourceIdentity(file, actualSource, kind);
    if (error == ERROR_SUCCESS && !UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    if (error == ERROR_SUCCESS)
        error = CheckAttributes(file, operation);
    if (error == ERROR_SUCCESS)
    {
        // Detach the verified name on close even while the host retains its identity handle.
        FILE_DISPOSITION_INFO_EX disposition{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS |
                                             FILE_DISPOSITION_FLAG_FORCE_IMAGE_SECTION_CHECK};
        if (!SetFileInformationByHandle(file, FileDispositionInfoEx, &disposition, sizeof(disposition)))
            error = GetLastError();
    }
    CloseHandle(file);
    return error;
}
}
