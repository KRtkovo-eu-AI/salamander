/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Capture and recheck subtree identities. | Prepare a bounded source manifest for recursive copy. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Inventory one approved subtree using strict enumeration. | Reject incomplete scans and reparse points. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Enumerate a bounded directory listing by handle. | Avoid resolving a privileged search path from text. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "directoryleaf.h"
#include "f7pilot.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace UacListing
{
constexpr DWORD maxEntries = 4096;
constexpr unsigned maxInventoryDepth = 32;
constexpr DWORD inventoryRequest = 0x80000000;
static_assert(sizeof(WIN32_FIND_DATAW) == 592);
inline constexpr wchar_t marker[] = L"listing";

inline FILETIME Time(LARGE_INTEGER input)
{
    return {input.LowPart, static_cast<DWORD>(input.HighPart)};
}

namespace Detail
{
template <typename Accept>
DWORD EnumerateRows(HANDLE directory, bool strict, Accept accept)
{
    std::vector<ULONGLONG> buffer(65536 / sizeof(ULONGLONG));
    const DWORD bytes = static_cast<DWORD>(buffer.size() * sizeof(ULONGLONG));
    auto mode = FileIdExtdDirectoryRestartInfo;
    while (GetFileInformationByHandleEx(directory, mode, buffer.data(), bytes))
    {
        mode = FileIdExtdDirectoryInfo;
        DWORD offset = 0;
        for (;;)
        {
            constexpr DWORD header = offsetof(FILE_ID_EXTD_DIR_INFO, FileName);
            const auto* row = reinterpret_cast<const FILE_ID_EXTD_DIR_INFO*>(
                reinterpret_cast<const BYTE*>(buffer.data()) + offset);
            const DWORD next = row->NextEntryOffset;
            const DWORD remaining = next ? next : bytes - offset;
            if (remaining < header || remaining > bytes - offset ||
                (next && (next % alignof(ULONGLONG) || bytes - offset - next < header)) ||
                row->FileNameLength > remaining - header || row->FileNameLength % sizeof(wchar_t) ||
                row->FileNameLength / sizeof(wchar_t) > UacDirectoryLeaf::maxComponent)
                return ERROR_INVALID_DATA;
            const std::wstring_view name(row->FileName, row->FileNameLength / sizeof(wchar_t));
            if (UacDirectoryLeaf::IsComponent(name))
            {
                if (strict && (row->FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
                    return ERROR_REPARSE_TAG_INVALID;
                if (strict && row->EndOfFile.QuadPart < 0)
                    return ERROR_INVALID_DATA;
                const DWORD error = accept(*row, name);
                if (error)
                    return error;
            }
            else if (strict && name != L"." && name != L"..")
                return ERROR_INVALID_NAME;
            if (!next)
                break;
            offset += next;
        }
    }
    const DWORD error = GetLastError();
    return error == ERROR_NO_MORE_FILES ? ERROR_SUCCESS : error;
}
}

inline DWORD Enumerate(HANDLE directory, std::vector<WIN32_FIND_DATAW>& entries)
{
    entries.clear();
    return Detail::EnumerateRows(directory, false, [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
        if (entries.size() == maxEntries)
            return ERROR_BUFFER_OVERFLOW;
        WIN32_FIND_DATAW entry{};
        entry.dwFileAttributes = row.FileAttributes;
        entry.ftCreationTime = Time(row.CreationTime);
        entry.ftLastAccessTime = Time(row.LastAccessTime);
        entry.ftLastWriteTime = Time(row.LastWriteTime);
        entry.nFileSizeHigh = static_cast<DWORD>(row.EndOfFile.HighPart);
        entry.nFileSizeLow = row.EndOfFile.LowPart;
        entry.dwReserved0 = row.ReparsePointTag;
        std::memcpy(entry.cFileName, name.data(), row.FileNameLength);
        entries.push_back(entry);
        return ERROR_SUCCESS;
    });
}

constexpr size_t manifestRoot = static_cast<size_t>(-1);

struct ManifestEntry
{
    size_t parent;
    std::wstring name;
    FILE_ID_128 id;
    DWORD attributes;
    ULONGLONG bytes;
};

struct SubtreeManifest
{
    UacDirectoryLeaf::ParentIdentity root;
    std::vector<ManifestEntry> entries;
    UacF7::DirectoryInventory inventory{};
};

namespace Detail
{
inline bool SameId(const FILE_ID_128& first, const FILE_ID_128& second)
{
    return std::memcmp(first.Identifier, second.Identifier, sizeof(first.Identifier)) == 0;
}

inline DWORD ScanDirectory(HANDLE directory, unsigned depth, size_t parent, DWORD& remaining,
                           ULONGLONG deadline, SubtreeManifest& manifest)
{
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    if (depth > maxInventoryDepth)
        return ERROR_BUFFER_OVERFLOW;

    std::vector<ManifestEntry> entries;
    DWORD error = EnumerateRows(directory, true, [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        if (!remaining)
            return ERROR_BUFFER_OVERFLOW;
        --remaining;
        const auto bytes = (row.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : row.EndOfFile.QuadPart;
        entries.push_back({parent, std::wstring(name), row.FileId, row.FileAttributes, static_cast<ULONGLONG>(bytes)});
        return ERROR_SUCCESS;
    });
    if (error)
        return error;
    std::sort(entries.begin(), entries.end(),
              [](const ManifestEntry& first, const ManifestEntry& second) { return first.name < second.name; });
    if (std::adjacent_find(entries.begin(), entries.end(), [](const ManifestEntry& first, const ManifestEntry& second) {
            return first.name == second.name;
        }) != entries.end())
        return ERROR_FILE_INVALID;

    auto& inventory = manifest.inventory;
    for (auto& entry : entries)
    {
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        const size_t index = manifest.entries.size();
        manifest.entries.push_back(std::move(entry));
        const auto& recorded = manifest.entries.back();
        if (recorded.attributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            HANDLE raw = nullptr;
            UacDirectoryLeaf::ParentIdentity identity;
            error = UacDirectoryLeaf::OpenParentBelowRoot(directory, recorded.name, raw, identity, FILE_LIST_DIRECTORY);
            if (error)
                return error;
            const std::unique_ptr<void, decltype(&CloseHandle)> child(raw, CloseHandle);
            if (identity.file.VolumeSerialNumber != manifest.root.file.VolumeSerialNumber ||
                !UacDirectoryLeaf::Equal(identity.volume, manifest.root.volume) ||
                !SameId(identity.file.FileId, recorded.id))
                return ERROR_FILE_INVALID;
            ++inventory.directories;
            if ((error = ScanDirectory(child.get(), depth + 1, index, remaining, deadline, manifest)) != ERROR_SUCCESS)
                return error;
        }
        else
        {
            const ULONGLONG bytes = recorded.bytes;
            if (bytes > MAXULONGLONG - inventory.bytes)
                return ERROR_ARITHMETIC_OVERFLOW;
            inventory.bytes += bytes;
            ++inventory.files;
        }
    }
    return GetTickCount64() < deadline ? ERROR_SUCCESS : ERROR_TIMEOUT;
}
}

// The manifest describes a live scan. A subsequent copy must still verify the
// handles it actually uses for I/O and apply its own stream and metadata policy.
inline DWORD CaptureManifest(HANDLE directory, ULONGLONG deadline, SubtreeManifest& manifest)
{
    manifest = {};
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    SubtreeManifest complete;
    DWORD error = UacDirectoryLeaf::ReadParentIdentity(directory, complete.root);
    if (error)
        return error;
    DWORD remaining = maxEntries;
    error = Detail::ScanDirectory(directory, 0, manifestRoot, remaining, deadline, complete);
    if (!error)
        manifest = std::move(complete);
    return error;
}

inline DWORD VerifyManifest(HANDLE directory, ULONGLONG deadline, const SubtreeManifest& expected)
{
    SubtreeManifest current;
    const DWORD error = CaptureManifest(directory, deadline, current);
    if (error)
        return error;
    if (!UacDirectoryLeaf::SameParent(expected.root, current.root) ||
        expected.entries.size() != current.entries.size())
        return ERROR_FILE_INVALID;
    for (size_t i = 0; i < current.entries.size(); ++i)
    {
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        const auto& first = expected.entries[i];
        const auto& second = current.entries[i];
        if (first.parent != second.parent || first.name != second.name || !Detail::SameId(first.id, second.id) ||
            first.attributes != second.attributes || first.bytes != second.bytes)
            return ERROR_FILE_INVALID;
    }
    return ERROR_SUCCESS;
}

inline DWORD Inventory(HANDLE directory, ULONGLONG deadline, UacF7::DirectoryInventory& inventory)
{
    SubtreeManifest manifest;
    const DWORD error = CaptureManifest(directory, deadline, manifest);
    inventory = manifest.inventory;
    return error;
}
}
