/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Set basic metadata once on an identity-verified NTFS file. | Avoid partially applying attributes and times before consent or on a retry. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "f7pilot.h"
#include "renameleaf.h"

namespace UacMetadataLeaf
{
inline bool IsOrdinary(DWORD attrs)
{
    return attrs == FILE_ATTRIBUTE_NORMAL || (attrs != 0 && (attrs & ~UacF7::basicAttributes) == 0);
}

inline bool IsValidTime(LONGLONG time)
{
    if (time < 0)
        return false;
    const FILETIME value{static_cast<DWORD>(time), static_cast<DWORD>(static_cast<ULONGLONG>(time) >> 32)};
    SYSTEMTIME converted{};
    return FileTimeToSystemTime(&value, &converted) != FALSE;
}

inline bool IsValid(const UacF7::MetadataChange& change)
{
    return IsOrdinary(change.expectedAttributes) &&
           (change.attributes == 0 || IsOrdinary(change.attributes)) &&
           (change.attributes || change.created || change.accessed || change.modified) &&
           IsValidTime(change.created) && IsValidTime(change.accessed) && IsValidTime(change.modified);
}

inline DWORD CheckAttributes(HANDLE file, const UacF7::MetadataChange& change)
{
    FILE_BASIC_INFO current{};
    if (!GetFileInformationByHandleEx(file, FileBasicInfo, &current, sizeof(current)))
        return GetLastError();
    if (current.FileAttributes != change.expectedAttributes)
        return ERROR_FILE_INVALID;
    return ERROR_SUCCESS;
}

inline DWORD SetVerifiedMetadata(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                 const FILE_ID_INFO& expectedSource, std::wstring_view leaf,
                                 const UacF7::MetadataChange& change)
{
    if (!UacDirectoryLeaf::IsComponent(leaf) || !IsValid(change))
        return ERROR_INVALID_DATA;
    UacDirectoryLeaf::ParentIdentity actualParent;
    DWORD error = UacDirectoryLeaf::ReadParentIdentity(parent, actualParent);
    if (error != ERROR_SUCCESS)
        return error;
    if (!UacDirectoryLeaf::SameParent(expectedParent, actualParent) ||
        expectedSource.VolumeSerialNumber != actualParent.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;

    HANDLE file = nullptr;
    error = UacRenameLeaf::Detail::OpenSource(parent, leaf, file, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ);
    if (error != ERROR_SUCCESS)
        return error;
    FILE_ID_INFO actualSource{};
    error = UacRenameLeaf::Detail::ReadSourceIdentity(file, actualSource);
    if (error == ERROR_SUCCESS && !UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    if (error == ERROR_SUCCESS)
        error = CheckAttributes(file, change);
    if (error == ERROR_SUCCESS)
    {
        FILE_BASIC_INFO basic{};
        basic.CreationTime.QuadPart = change.created;
        basic.LastAccessTime.QuadPart = change.accessed;
        basic.LastWriteTime.QuadPart = change.modified;
        basic.FileAttributes = change.attributes;
        if (!SetFileInformationByHandle(file, FileBasicInfo, &basic, sizeof(basic)))
            error = GetLastError();
    }
    CloseHandle(file);
    return error;
}
}
