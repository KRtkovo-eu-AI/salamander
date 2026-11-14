/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-10 | Allow create-only operations to retain a pinned directory. | Anchor subsequent requests to the newly created destination root. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Expose the access needed by the final nested destination handle. | Keep named-stream move verification on the same handle used for publication. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Allow transfer callers to pin an existing destination. | Reject renames while creating a verified missing suffix. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Create a missing directory chain below a verified ancestor. | Pin newly created components and remove only owned empty directories on rollback. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Allow an anchored metadata open without requesting traversal. | Let read-only size enumeration use its required access mask. | #731 | GPT-6.1 Sol |
| 2026-11-08 | Allow delete-on-close when creating an empty stage. | Cover interruption before the stage identity is journaled. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Allow a creation descriptor for native directories. | Protect the staged tree from the moment of creation. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Allow anchored directory opens to request enumeration access. | Reuse the no-reparse walk for approved listings. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add a directory creation primitive anchored to a verified NTFS handle. | Prove the P3a parent identity requirement before enabling privileged F7. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include <windows.h>
#include <winternl.h>
#include <cstring>
#include <new>
#include <string>
#include <string_view>
#include <vector>

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
inline DWORD NativeDirectory(HANDLE root, std::wstring_view name, ULONG disposition, HANDLE& directory,
                             ACCESS_MASK extraAccess = 0, PSECURITY_DESCRIPTOR security = nullptr,
                             ULONG sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ULONG options = 0,
                             bool requestTraverse = true)
{
    directory = nullptr;
    if (name.empty() || name.size() > maxNativeName)
        return ERROR_INVALID_NAME;
    const auto bytes = static_cast<USHORT>(name.size() * sizeof(wchar_t));
    UNICODE_STRING objectName{bytes, bytes, const_cast<wchar_t*>(name.data())};
    OBJECT_ATTRIBUTES attributes{};
    InitializeObjectAttributes(&attributes, &objectName, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE, root, security);
    IO_STATUS_BLOCK io{};
    const ACCESS_MASK access = FILE_READ_ATTRIBUTES | SYNCHRONIZE | extraAccess |
                               (requestTraverse ? FILE_TRAVERSE : 0);
    const NTSTATUS result = NtCreateFile(&directory, access,
                                         &attributes, &io, nullptr, FILE_ATTRIBUTE_NORMAL,
                                         sharing, disposition,
                                         FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT | options,
                                         nullptr, 0);
    return result >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(result);
}

}

inline bool IsRelativeDirectoryPath(std::wstring_view path, size_t* componentCount = nullptr)
{
    if (path.empty() || path.size() > maxNativeName)
        return false;
    size_t count = 0;
    for (;;)
    {
        const size_t split = path.find(L'\\');
        if (!IsComponent(path.substr(0, split)))
            return false;
        ++count;
        if (split == std::wstring_view::npos)
            break;
        path.remove_prefix(split + 1);
    }
    if (componentCount)
        *componentCount = count;
    return true;
}

inline DWORD VerifyParent(HANDLE parent, const ParentIdentity& expected)
{
    ParentIdentity actual;
    const DWORD error = ReadParentIdentity(parent, actual);
    if (error)
        return error;
    return SameParent(expected, actual) ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

// root is a directory handle already opened and owned by the caller. No component may
// escape it. The kernel rejects traversal through intermediate reparse points.
inline DWORD OpenParentBelowRoot(HANDLE root, std::wstring_view relative, HANDLE& parent, ParentIdentity& identity,
                                 ACCESS_MASK extraAccess = 0, bool requestTraverse = true,
                                 ULONG sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
{
    parent = nullptr;
    if (!IsRelativeDirectoryPath(relative))
        return ERROR_INVALID_NAME;
    ParentIdentity rootIdentity;
    DWORD error = ReadParentIdentity(root, rootIdentity);
    if (error != ERROR_SUCCESS)
        return error;
    error = Detail::NativeDirectory(root, relative, FILE_OPEN, parent, extraAccess, nullptr,
                                    sharing, 0, requestTraverse);
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
inline DWORD CreateVerifiedLeaf(HANDLE parent, const ParentIdentity& expected, std::wstring_view leaf,
                                HANDLE* pinned = nullptr)
{
    if (!IsComponent(leaf))
        return ERROR_INVALID_NAME;
    const DWORD error = VerifyParent(parent, expected);
    if (error)
        return error;

    HANDLE created = nullptr;
    const ULONG sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | (pinned ? 0 : FILE_SHARE_DELETE);
    const DWORD result = Detail::NativeDirectory(parent, leaf, FILE_CREATE, created, 0, nullptr, sharing);
    if (pinned)
        *pinned = created;
    else if (result == ERROR_SUCCESS)
        CloseHandle(created);
    return result;
}

// The caller owns and pins the verified existing ancestor. All requested
// components are new. Never open an object that appeared after preparation.
class PendingDirectoryChain
{
public:
    PendingDirectoryChain() = default;
    PendingDirectoryChain(const PendingDirectoryChain&) = delete;
    PendingDirectoryChain& operator=(const PendingDirectoryChain&) = delete;
    ~PendingDirectoryChain() { RollBack(); }

    DWORD Create(HANDLE ancestor, const ParentIdentity& expected, std::wstring_view suffix,
                 ULONGLONG deadline, ACCESS_MASK finalAccess = 0)
    {
        size_t count = 0;
        if (!created.empty() || !IsRelativeDirectoryPath(suffix, &count))
            return ERROR_INVALID_NAME;
        DWORD error = VerifyParent(ancestor, expected);
        if (error)
            return error;
        try
        {
            created.reserve(count);
        }
        catch (const std::bad_alloc&)
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        HANDLE parent = ancestor;
        for (;;)
        {
            if (GetTickCount64() >= deadline)
            {
                error = ERROR_TIMEOUT;
                break;
            }
            const size_t split = suffix.find(L'\\');
            HANDLE child = nullptr;
            const ACCESS_MASK access = DELETE | (split == std::wstring_view::npos ? finalAccess : 0);
            error = Detail::NativeDirectory(parent, suffix.substr(0, split), FILE_CREATE, child,
                                            access, nullptr, FILE_SHARE_READ | FILE_SHARE_WRITE);
            if (error)
                break;
            created.push_back(child);
            error = ReadParentIdentity(child, identity);
            if (error || split == std::wstring_view::npos)
                break;
            parent = child;
            suffix.remove_prefix(split + 1);
        }
        return error && RollBack() ? ERROR_PARTIAL_COPY : error;
    }

    HANDLE Directory() const { return created.empty() ? nullptr : created.back(); }
    const ParentIdentity& Identity() const { return identity; }
    void Commit() { Close(false); }
    DWORD RollBack() { return Close(true); }

private:
    DWORD Close(bool remove)
    {
        DWORD error = ERROR_SUCCESS;
        while (!created.empty())
        {
            const HANDLE directory = created.back();
            if (remove)
            {
                FILE_DISPOSITION_INFO disposition{TRUE};
                if (!SetFileInformationByHandle(directory, FileDispositionInfo, &disposition, sizeof(disposition)) && !error)
                    error = GetLastError();
            }
            CloseHandle(directory);
            created.pop_back();
        }
        return error;
    }

    std::vector<HANDLE> created;
    ParentIdentity identity;
};
}
