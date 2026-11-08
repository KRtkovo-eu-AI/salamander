/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Delete a verified file symbolic link by handle. | Check the reparse tag and retain the ordinary-file rules. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Delete a single ordinary file by its verified NTFS handle. | Avoid reopening a path for deletion or changing file attributes. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "f7pilot.h"
#include "renameleaf.h"

namespace UacDeleteLeaf
{
inline DWORD CheckAttributes(HANDLE file, bool symlink = false)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if (!GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attributes, sizeof(attributes)))
        return GetLastError();
    if (symlink && attributes.ReparseTag != IO_REPARSE_TAG_SYMLINK)
        return ERROR_REPARSE_TAG_INVALID;
    const bool supported = symlink ? UacF7::IsPlainDeleteSymlink(attributes.FileAttributes)
                                   : UacF7::IsPlainDeleteFile(attributes.FileAttributes);
    return supported ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
}

inline DWORD DeleteVerifiedFile(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                const FILE_ID_INFO& expectedSource, std::wstring_view leaf, bool symlink = false)
{
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
    error = UacRenameLeaf::Detail::OpenSource(parent, leaf, file, DELETE, 0);
    if (error != ERROR_SUCCESS)
        return error;
    FILE_ID_INFO actualSource{};
    const auto kind = symlink ? UacRenameLeaf::MoveKind::RenameSymlink : UacRenameLeaf::MoveKind::RenameFile;
    error = UacRenameLeaf::Detail::ReadSourceIdentity(file, actualSource, kind);
    if (error == ERROR_SUCCESS && !UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    if (error == ERROR_SUCCESS)
        error = CheckAttributes(file, symlink);
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
