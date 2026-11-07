/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Add a directory creation primitive anchored to a verified NTFS handle. | Prove the P3a parent identity requirement before enabling privileged F7. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include <windows.h>
#include <winternl.h>
#include <cstring>
#include <string>
#include <string_view>

namespace UacDirectoryLeaf
{
constexpr size_t maxComponent = 255;
constexpr size_t maxNativeName = 32766;

inline bool Equal(std::wstring_view a, std::wstring_view b)
{
    return a.size() == b.size() &&
           CompareStringOrdinal(a.data(), static_cast<int>(a.size()), b.data(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL;
}

inline bool IsComponent(std::wstring_view name)
{
    if (name.empty() || name.size() > maxComponent || name.back() == L' ' || name.back() == L'.')
        return false;
    for (wchar_t ch : name)
        if (ch < L' ' || std::wstring_view(L"\\/:*?\"<>|").find(ch) != std::wstring_view::npos)
            return false;

    auto base = name.substr(0, name.find(L'.'));
    while (!base.empty() && base.back() == L' ')
        base.remove_suffix(1);
    for (const auto reserved : {L"CON", L"PRN", L"AUX", L"NUL", L"CONIN$", L"CONOUT$"})
        if (Equal(base, reserved))
            return false;
    return !(base.size() == 4 && (Equal(base.substr(0, 3), L"COM") || Equal(base.substr(0, 3), L"LPT")) &&
             std::wstring_view(L"123456789\u00b9\u00b2\u00b3").find(base.back()) != std::wstring_view::npos);
}

struct ParentIdentity
{
    FILE_ID_INFO file{};
    std::wstring volume;
};

inline DWORD ReadParentIdentity(HANDLE parent, ParentIdentity& identity)
{
    FILE_ATTRIBUTE_TAG_INFO attr{};
    if (!GetFileInformationByHandleEx(parent, FileAttributeTagInfo, &attr, sizeof(attr)))
        return GetLastError();
    if (attr.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        return ERROR_REPARSE_TAG_INVALID;
    if (!(attr.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return ERROR_DIRECTORY;
    wchar_t fs[16]{};
    if (!GetVolumeInformationByHandleW(parent, nullptr, 0, nullptr, nullptr, nullptr, fs, ARRAYSIZE(fs)))
        return GetLastError();
    if (!Equal(fs, L"NTFS"))
        return ERROR_NOT_SUPPORTED;

    constexpr DWORD flags = FILE_NAME_OPENED | VOLUME_NAME_GUID;
    const DWORD length = GetFinalPathNameByHandleW(parent, nullptr, 0, flags);
    if (!length)
        return GetLastError();
    if (length > maxNativeName)
        return ERROR_FILENAME_EXCED_RANGE;
    std::wstring name(length, L'\0');
    const DWORD written = GetFinalPathNameByHandleW(parent, name.data(), length, flags);
    if (!written)
        return GetLastError();
    if (written >= length)
        return ERROR_FILENAME_EXCED_RANGE;
    name.resize(written);
    constexpr std::wstring_view prefix = LR"(\\?\Volume{)";
    constexpr size_t close = prefix.size() + 36;
    if (name.size() <= close + 1 || !Equal(std::wstring_view(name).substr(0, prefix.size()), prefix) ||
        name[close] != L'}' || name[close + 1] != L'\\')
        return ERROR_NOT_SUPPORTED;
    if (!GetFileInformationByHandleEx(parent, FileIdInfo, &identity.file, sizeof(identity.file)))
        return GetLastError();
    identity.volume = name.substr(0, close + 1);
    return ERROR_SUCCESS;
}

inline bool SameParent(const ParentIdentity& a, const ParentIdentity& b)
{
    return a.file.VolumeSerialNumber == b.file.VolumeSerialNumber && Equal(a.volume, b.volume) &&
           std::memcmp(a.file.FileId.Identifier, b.file.FileId.Identifier, sizeof(a.file.FileId.Identifier)) == 0;
}

namespace Detail
{
inline DWORD NativeDirectory(HANDLE root, std::wstring_view name, ULONG disposition, HANDLE& directory)
{
    directory = nullptr;
    if (name.empty() || name.size() > maxNativeName)
        return ERROR_INVALID_NAME;
    const auto bytes = static_cast<USHORT>(name.size() * sizeof(wchar_t));
    UNICODE_STRING objectName{bytes, bytes, const_cast<wchar_t*>(name.data())};
    OBJECT_ATTRIBUTES attributes{};
    InitializeObjectAttributes(&attributes, &objectName, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, root, nullptr);
    IO_STATUS_BLOCK io{};
    const NTSTATUS result = NtCreateFile(&directory, FILE_READ_ATTRIBUTES | FILE_TRAVERSE | SYNCHRONIZE,
                                         &attributes, &io, nullptr, FILE_ATTRIBUTE_NORMAL,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, disposition,
                                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT,
                                         nullptr, 0);
    return result >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(result);
}

}

// root is a directory handle already opened and owned by the caller. No component may
// escape it. The kernel rejects traversal through intermediate reparse points.
inline DWORD OpenParentBelowRoot(HANDLE root, std::wstring_view relative, HANDLE& parent, ParentIdentity& identity)
{
    parent = nullptr;
    if (relative.empty() || relative.size() > maxNativeName)
        return ERROR_INVALID_NAME;
    auto remaining = relative;
    for (;;)
    {
        const size_t split = remaining.find(L'\\');
        if (!IsComponent(remaining.substr(0, split)))
            return ERROR_INVALID_NAME;
        if (split == std::wstring_view::npos)
            break;
        remaining.remove_prefix(split + 1);
    }
    ParentIdentity rootIdentity;
    DWORD error = ReadParentIdentity(root, rootIdentity);
    if (error != ERROR_SUCCESS)
        return error;
    error = Detail::NativeDirectory(root, relative, FILE_OPEN, parent);
    if (error == ERROR_SUCCESS)
    {
        error = ReadParentIdentity(parent, identity);
        if (error != ERROR_SUCCESS)
        {
            CloseHandle(parent);
            parent = nullptr;
        }
    }
    return error;
}

// One create-only operation. Do not turn the verified object back into a pathname.
inline DWORD CreateVerifiedLeaf(HANDLE parent, const ParentIdentity& expected, std::wstring_view leaf)
{
    if (!IsComponent(leaf))
        return ERROR_INVALID_NAME;
    ParentIdentity actual;
    const DWORD error = ReadParentIdentity(parent, actual);
    if (error != ERROR_SUCCESS)
        return error;
    if (!SameParent(expected, actual))
        return ERROR_FILE_INVALID;

    HANDLE created = nullptr;
    const DWORD result = Detail::NativeDirectory(parent, leaf, FILE_CREATE, created);
    if (result == ERROR_SUCCESS)
        CloseHandle(created);
    return result;
}
}
