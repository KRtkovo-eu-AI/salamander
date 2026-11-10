/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-10 | Add a metadata-only listing request for F3. | Bind viewer consent to the selected file inside a protected listing. | #731 | Codex |
| 2026-11-10 | Capture a child directory ID from its parent listing. | Identify directories that deny even metadata opens. | #731 | Codex |
| 2026-11-09 | Remove the size-only Find content reply. | Use the shared file revision for each verified open. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Separate single-file and rooted Find frames. | Deny listing, inventory and other names on an exact-file grant. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Define a bounded Find content request. | Scope the byte budget to an authorized listing session. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Collect allocation and per-file sizes for protected Ctrl+Q. | Match ordinary cluster rounding and query sparse and compressed files by verified handle. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Capture and recheck subtree identities. | Prepare a bounded source manifest for recursive copy. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Inventory one approved subtree using strict enumeration. | Reject incomplete scans and reparse points. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Enumerate a bounded directory listing by handle. | Avoid resolving a privileged search path from text. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "directoryleaf.h"
#include "f7pilot.h"
#include "renameleaf.h"
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
constexpr DWORD sizeDetailsRequest = 0x40000000;
constexpr DWORD contentRequest = 0x20000000;
constexpr DWORD viewerIdentityRequest = 0x10000000;

struct ViewerIdentity
{
    FILE_ID_128 parent;
    FILE_ID_128 file;
};
static_assert(sizeof(ViewerIdentity) == 32);
constexpr ULONGLONG contentBudget = 64ULL * 1024 * 1024 * 1024;

inline bool ValidRequestScope(UacF7::Operation scope, DWORD pathLength, bool inventory, bool content)
{
    if (scope == UacF7::Operation::FindFile)
        return content && !inventory && pathLength == 0;
    return !content || (scope == UacF7::Operation::FindContent && !inventory && pathLength != 0);
}
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

inline DWORD CaptureChildDirectoryId(HANDLE parent, std::wstring_view child, FILE_ID_128& id)
{
    if (!UacDirectoryLeaf::IsComponent(child))
        return ERROR_INVALID_NAME;

    bool found = false;
    DWORD count = 0;
    FILE_ID_128 candidate{};
    const DWORD error = Detail::EnumerateRows(parent, false,
        [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
            if (++count > maxEntries)
                return ERROR_BUFFER_OVERFLOW;
            if (!UacDirectoryLeaf::Equal(name, child))
                return ERROR_SUCCESS;
            if (found)
                return ERROR_DUP_NAME;
            if (row.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                return ERROR_REPARSE_TAG_INVALID;
            if (!(row.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                return ERROR_DIRECTORY;
            candidate = row.FileId;
            found = true;
            return ERROR_SUCCESS;
        });
    if (error)
        return error;
    if (!found)
        return ERROR_FILE_NOT_FOUND;
    id = candidate;
    return ERROR_SUCCESS;
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

struct SizeScan
{
    UacF7::DirectorySizeDetails& details;
    DWORD cluster = 0;
};

inline DWORD MeasureFile(HANDLE parent, const ManifestEntry& entry, ULONGLONG serial, SizeScan& scan)
{
    constexpr DWORD special = FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE;
    scan.details.fileSizes.push_back(entry.bytes);
    ULONGLONG stored = entry.bytes;
    if (entry.attributes & special)
    {
        HANDLE raw = nullptr;
        const DWORD error = UacRenameLeaf::Detail::OpenSource(parent, entry.name, raw, 0,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        const std::unique_ptr<void, decltype(&CloseHandle)> file(raw, CloseHandle);
        FILE_ID_INFO identity{};
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        FILE_STANDARD_INFO size{};
        FILE_COMPRESSION_INFO compression{};
        if (error || !GetFileInformationByHandleEx(file.get(), FileIdInfo, &identity, sizeof(identity)) ||
            !GetFileInformationByHandleEx(file.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
            !GetFileInformationByHandleEx(file.get(), FileStandardInfo, &size, sizeof(size)))
            stored = UacF7::unavailableSize;
        else if (identity.VolumeSerialNumber != serial || !SameId(entry.id, identity.FileId) ||
                 (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) || size.Directory || size.DeletePending ||
                 (attributes.FileAttributes & special) != (entry.attributes & special) ||
                 static_cast<ULONGLONG>(size.EndOfFile.QuadPart) != entry.bytes)
            return ERROR_FILE_INVALID;
        else if (!GetFileInformationByHandleEx(file.get(), FileCompressionInfo, &compression, sizeof(compression)) ||
                 compression.CompressedFileSize.QuadPart < 0)
            stored = UacF7::unavailableSize;
        else
            stored = static_cast<ULONGLONG>(compression.CompressedFileSize.QuadPart);
    }

    auto& disk = scan.details.disk;
    const auto add = [](ULONGLONG& total, ULONGLONG value) {
        if (value >= UacF7::unavailableSize - total)
            total = UacF7::unavailableSize;
        else
            total += value;
    };
    add(disk.compressed, stored);
    if (!scan.cluster || stored == UacF7::unavailableSize)
        disk.occupied = UacF7::unavailableSize;
    else
    {
        const ULONGLONG padding = (scan.cluster - stored % scan.cluster) % scan.cluster;
        add(disk.occupied, stored);
        add(disk.occupied, padding);
    }
    return ERROR_SUCCESS;
}

inline DWORD ScanDirectory(HANDLE directory, unsigned depth, size_t parent, DWORD& remaining,
                           ULONGLONG deadline, SubtreeManifest& manifest, SizeScan* scan = nullptr)
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
            if ((error = ScanDirectory(child.get(), depth + 1, index, remaining, deadline, manifest, scan)) != ERROR_SUCCESS)
                return error;
        }
        else
        {
            const ULONGLONG bytes = recorded.bytes;
            if (bytes > MAXULONGLONG - inventory.bytes)
                return ERROR_ARITHMETIC_OVERFLOW;
            inventory.bytes += bytes;
            ++inventory.files;
            if (scan && (error = MeasureFile(directory, recorded, manifest.root.file.VolumeSerialNumber, *scan)) != ERROR_SUCCESS)
                return error;
        }
    }
    return GetTickCount64() < deadline ? ERROR_SUCCESS : ERROR_TIMEOUT;
}
}

// The manifest describes a live scan. A subsequent copy must still verify the
// handles it actually uses for I/O and apply its own stream and metadata policy.
inline DWORD CaptureManifest(HANDLE directory, ULONGLONG deadline, SubtreeManifest& manifest,
                             UacF7::DirectorySizeDetails* details = nullptr)
{
    manifest = {};
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    SubtreeManifest complete;
    DWORD error = UacDirectoryLeaf::ReadParentIdentity(directory, complete.root);
    if (error)
        return error;
    DWORD remaining = maxEntries;
    UacF7::DirectorySizeDetails measured;
    Detail::SizeScan scan{measured};
    if (details)
    {
        const auto volume = complete.root.volume + L"\\";
        DWORD sectors, bytes, freeClusters, totalClusters;
        if (GetDiskFreeSpaceW(volume.c_str(), &sectors, &bytes, &freeClusters, &totalClusters) &&
            sectors && bytes <= MAXDWORD / sectors)
            scan.cluster = sectors * bytes;
    }
    error = Detail::ScanDirectory(directory, 0, manifestRoot, remaining, deadline, complete, details ? &scan : nullptr);
    if (!error && details)
        *details = std::move(measured);
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

inline DWORD Inventory(HANDLE directory, ULONGLONG deadline, UacF7::DirectoryInventory& inventory,
                       UacF7::DirectorySizeDetails* details = nullptr)
{
    if (details)
        *details = {};
    SubtreeManifest manifest;
    const DWORD error = CaptureManifest(directory, deadline, manifest, details);
    inventory = manifest.inventory;
    return error;
}
}
