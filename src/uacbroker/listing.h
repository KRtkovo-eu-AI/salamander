/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-12 | Stream exact thin-copy child identity checks. | Check large directories without losing duplicate detection or cancellation. | #731 | GPT-6 |
| 2026-11-12 | Give paged directory requests cursor tokens. | Keep nested Find traversal scoped to its exact open handles. | #731 | GPT-6.1 Terra |
| 2026-11-12 | Define bounded verified directory page frames. | Keep streamed metadata confined to a CompareList session. | #731 | GPT-6 (Codex) |
| 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
| 2026-11-11 | Measure one pinned file without a directory grant. | Keep standalone file consent limited to metadata for that exact object. | #731 | GPT-6.1 Terra |
| 2026-11-11 | Measure allocation for one verified file within a read-only scope. | Resolve a denied compressed size without rescanning or exposing file contents. | #731 | GPT-6.1 Terra |
| 2026-11-11 | Capture a complete replacement revision from a verified parent listing. | Support inaccessible targets without relaxing no-follow or revision checks. | #731 | GPT-6.1 Terra |
| 2026-11-10 | Carry an exact link type in listing identity replies. | Allow the verified delete path to identify a link without following it. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Enumerate copy entries with their NTFS IDs. | Share row decoding and keep the existing listing bounds. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Define verified comparison listing requests. | Reject unrelated actions on a comparison grant. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Read the omitted name from its current manifest slot. | Keep diagnostic paths valid after nested manifest growth. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Return verified partial inventory and recognize a selected alias. | Keep readable siblings after access denial without following reparse targets. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Request only list and metadata access in read-only inventory. | Avoid a denial when a directory permits listing but refuses an explicit traverse handle. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Exclude compatibility aliases from read-only size totals. | Pin and check the link metadata without resolving its target or changing mutation manifests. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Restrict inventory sessions to size queries. | Separate size consent from listing and content access. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Capture the exact ordinary file identity from a readable parent. | Support a verified delete when even the child metadata open is denied. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Report attributes with the verified leaf identity. | Prepare basic metadata for ordinary files and directories. | #731 | GPT-6 (Codex) |
| 2026-11-10 | Share the listing identity query with leaf operations. | Identify a verified parent for a single directory creation. | #731 | GPT-6 (Codex) |
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
#include "compatjunction.h"
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
constexpr DWORD leafIdentityRequest = 0x10000000;
constexpr DWORD parentIdentityRequest = 0x08000000;
constexpr DWORD directoryIdentityRequest = 0x04000000;
constexpr DWORD verifiedListingRequest = 0x02000000;
constexpr DWORD linkIdentityRequest = 0x01000000;
constexpr DWORD fileAllocationRequest = 0x00800000;
constexpr DWORD pagedListingRequest = 0x00400000;

enum class PagedListingOp : DWORD { Open = 1, Next = 2, Close = 3 };
struct PagedListingFrame
{
    PagedListingOp operation;
    DWORD reserved;
    ULONGLONG cursor;
};
struct PagedListingReply
{
    DWORD error;
    DWORD count;
    DWORD done;
    DWORD reserved;
    UacF7::MoveFileIdentity identity;
};
static_assert(sizeof(PagedListingFrame) == 16);
static_assert(sizeof(PagedListingReply) == 40);

struct LeafIdentity
{
    FILE_ID_128 parent;
    FILE_ID_128 file;
    DWORD attributes;
    DWORD reparseTag;
};
static_assert(sizeof(LeafIdentity) == 40);
constexpr ULONGLONG contentBudget = 64ULL * 1024 * 1024 * 1024;

inline bool ValidRequestScope(UacF7::Operation scope, DWORD pathLength, bool inventory, bool content)
{
    if (scope == UacF7::Operation::Inventory)
        return inventory && !content;
    if (scope == UacF7::Operation::FileAllocation)
        return false; // Exact allocation permits only its separately typed request.
    if (scope == UacF7::Operation::FindFile)
        return content && !inventory && pathLength == 0;
    if (UacF7::IsComparisonScope(scope))
        return !inventory && (!content || (scope == UacF7::Operation::CompareContent && pathLength != 0));
    return !content || (scope == UacF7::Operation::FindContent && !inventory && pathLength != 0);
}

// Root checks remain exclusive to CompareList; the Find roles can verify an
// already-listed descendant without gaining a broader root-level operation.
inline bool ValidVerifiedListingScope(UacF7::Operation scope, DWORD pathLength)
{
    return (UacF7::IsComparisonScope(scope) && (pathLength != 0 || scope == UacF7::Operation::CompareList)) ||
           (pathLength != 0 && (scope == UacF7::Operation::List || scope == UacF7::Operation::FindContent));
}

inline bool MatchesListingIdentity(const UacDirectoryLeaf::ParentIdentity& actual, const UacF7::MoveFileIdentity& expected)
{
    return actual.file.VolumeSerialNumber == expected.serial &&
           std::memcmp(actual.file.FileId.Identifier, expected.id, sizeof(expected.id)) == 0;
}

inline bool ValidAllocationScope(UacF7::Operation scope, DWORD pathLength)
{
    return (pathLength == 0 && scope == UacF7::Operation::FileAllocation) ||
           (pathLength != 0 && (scope == UacF7::Operation::List || scope == UacF7::Operation::Inventory));
}

inline DWORD MeasureOpenedFileAllocation(HANDLE file, const UacF7::FileAllocationRequest& expected, ULONGLONG& bytes)
{
    constexpr DWORD special = FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_SPARSE_FILE;
    if (expected.reserved || !(expected.attributes & special) ||
        (expected.attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        return ERROR_INVALID_PARAMETER;
    FILE_ID_INFO identity{};
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    FILE_STANDARD_INFO standard{};
    FILE_COMPRESSION_INFO compression{};
    if (!GetFileInformationByHandleEx(file, FileIdInfo, &identity, sizeof(identity)) ||
        !GetFileInformationByHandleEx(file, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileInformationByHandleEx(file, FileStandardInfo, &standard, sizeof(standard)))
        return GetLastError();
    if (identity.VolumeSerialNumber != expected.serial ||
        std::memcmp(identity.FileId.Identifier, expected.fileId, sizeof(expected.fileId)) ||
        (attributes.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
        (attributes.FileAttributes & special) != (expected.attributes & special) || standard.Directory ||
        standard.DeletePending || standard.EndOfFile.QuadPart < 0 ||
        static_cast<ULONGLONG>(standard.EndOfFile.QuadPart) != expected.logicalSize)
        return ERROR_FILE_INVALID;
    if (!GetFileInformationByHandleEx(file, FileCompressionInfo, &compression, sizeof(compression)))
        return GetLastError();
    if (compression.CompressedFileSize.QuadPart < 0)
        return ERROR_INVALID_DATA;
    bytes = static_cast<ULONGLONG>(compression.CompressedFileSize.QuadPart);
    return ERROR_SUCCESS;
}

inline DWORD MeasureFileAllocation(HANDLE root, std::wstring_view relative,
                                   const UacF7::FileAllocationRequest& expected, ULONGLONG& bytes)
{
    const auto split = relative.find_last_of(L'\\');
    const auto parentPath = split == std::wstring_view::npos ? std::wstring_view{} : relative.substr(0, split);
    const auto leaf = split == std::wstring_view::npos ? relative : relative.substr(split + 1);
    if ((split != std::wstring_view::npos && parentPath.empty()) || !UacDirectoryLeaf::IsComponent(leaf) ||
        (!parentPath.empty() && !UacDirectoryLeaf::IsRelativeDirectoryPath(parentPath)))
        return ERROR_INVALID_NAME;
    HANDLE rawParent = nullptr;
    UacDirectoryLeaf::ParentIdentity parentIdentity;
    DWORD error = parentPath.empty() ? UacDirectoryLeaf::ReadParentIdentity(root, parentIdentity) :
        UacDirectoryLeaf::OpenParentBelowRoot(root, parentPath, rawParent, parentIdentity, 0, false);
    const std::unique_ptr<void, decltype(&CloseHandle)> ownedParent(rawParent, CloseHandle);
    if (error)
        return error;
    if (parentIdentity.file.VolumeSerialNumber != expected.serial ||
        std::memcmp(parentIdentity.file.FileId.Identifier, expected.parentId, sizeof(expected.parentId)))
        return ERROR_FILE_INVALID;
    HANDLE rawFile = nullptr;
    error = UacRenameLeaf::Detail::OpenSource(rawParent ? rawParent : root, leaf, rawFile, 0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    const std::unique_ptr<void, decltype(&CloseHandle)> file(rawFile, CloseHandle);
    if (error)
        return error;
    return MeasureOpenedFileAllocation(rawFile, expected, bytes);
}
static_assert(sizeof(WIN32_FIND_DATAW) == 592);
inline constexpr wchar_t marker[] = L"listing";

inline FILETIME Time(LARGE_INTEGER input)
{
    return {input.LowPart, static_cast<DWORD>(input.HighPart)};
}

namespace Detail
{
// Read only the link object's metadata using the already anchored parent.
// The target is never resolved or opened by this check.
inline bool VerifyCompatibilityAlias(HANDLE directory, std::wstring_view name, const FILE_ID_EXTD_DIR_INFO& row)
{
    if (!HasCompatibilityLinkAttributes(row.FileAttributes) ||
        (row.ReparsePointTag != IO_REPARSE_TAG_MOUNT_POINT && row.ReparsePointTag != IO_REPARSE_TAG_SYMLINK))
        return false;
    HANDLE raw = nullptr;
    if (UacRenameLeaf::Detail::OpenSource(directory, name, raw, READ_CONTROL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, UacRenameLeaf::MoveKind::RenameDirectory))
        return false;
    const std::unique_ptr<void, decltype(&CloseHandle)> link(raw, CloseHandle);
    FILE_ID_INFO identity{};
    return GetFileInformationByHandleEx(link.get(), FileIdInfo, &identity, sizeof(identity)) &&
           std::memcmp(&identity.FileId, &row.FileId, sizeof(row.FileId)) == 0 &&
           IsCompatibilityLink(link.get());
}

template <typename Accept, typename BeforePage>
DWORD EnumerateRowsChecked(HANDLE directory, bool strict, Accept&& accept, BeforePage&& beforePage)
{
    std::vector<ULONGLONG> buffer(65536 / sizeof(ULONGLONG));
    const DWORD bytes = static_cast<DWORD>(buffer.size() * sizeof(ULONGLONG));
    auto mode = FileIdExtdDirectoryRestartInfo;
    for (;;)
    {
        const DWORD interrupted = beforePage();
        if (interrupted)
            return interrupted;
        if (!GetFileInformationByHandleEx(directory, mode, buffer.data(), bytes))
            break;
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

template <typename Accept>
DWORD EnumerateRows(HANDLE directory, bool strict, Accept accept)
{
    return EnumerateRowsChecked(directory, strict, accept, []() -> DWORD { return ERROR_SUCCESS; });
}
}

inline DWORD CaptureChildId(HANDLE parent, std::wstring_view child, bool directory, FILE_ID_128& id,
                            DWORD& attributes, DWORD expectedReparseTag = 0)
{
    if (!UacDirectoryLeaf::IsComponent(child))
        return ERROR_INVALID_NAME;

    bool found = false;
    DWORD count = 0;
    FILE_ID_128 candidate{};
    DWORD candidateAttributes = 0;
    const DWORD error = Detail::EnumerateRows(parent, false,
        [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
            if (++count > maxEntries)
                return ERROR_BUFFER_OVERFLOW;
            if (!UacDirectoryLeaf::Equal(name, child))
                return ERROR_SUCCESS;
            if (found)
                return ERROR_DUP_NAME;
            const bool reparse = (row.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (reparse != (expectedReparseTag != 0) || (reparse && row.ReparsePointTag != expectedReparseTag))
                return ERROR_REPARSE_TAG_INVALID;
            if (((row.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) != directory)
                return ERROR_DIRECTORY;
            candidate = row.FileId;
            candidateAttributes = row.FileAttributes;
            found = true;
            return ERROR_SUCCESS;
        });
    if (error)
        return error;
    if (!found)
        return ERROR_FILE_NOT_FOUND;
    id = candidate;
    attributes = candidateAttributes;
    return ERROR_SUCCESS;
}

#ifdef SAM_UAC_THIN_COPY_TEST
// The ordinary bounded listing API remains unchanged. A thin-copy caller may
// instead scan all pages with constant memory; its callback must check the
// authenticated connection and deadline without changing the directory.
// The caller MUST provide its own independently opened file object, not a
// duplicate of a host-controlled handle sharing an enumeration cursor.
// Enumeration is not an atomic snapshot and an individual OS call may block.
template <typename BeforePage>
DWORD CaptureChildIdStreaming(HANDLE parent, std::wstring_view child, bool directory, FILE_ID_128& id,
                              DWORD& attributes, BeforePage&& beforePage, DWORD expectedReparseTag = 0)
{
    if (!UacDirectoryLeaf::IsComponent(child))
        return ERROR_INVALID_NAME;

    bool found = false;
    FILE_ID_128 candidate{};
    DWORD candidateAttributes = 0;
    const DWORD error = Detail::EnumerateRowsChecked(parent, false,
        [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
            if (!UacDirectoryLeaf::Equal(name, child))
                return ERROR_SUCCESS;
            if (found)
                return ERROR_DUP_NAME;
            const bool reparse = (row.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
            if (reparse != (expectedReparseTag != 0) || (reparse && row.ReparsePointTag != expectedReparseTag))
                return ERROR_REPARSE_TAG_INVALID;
            if (((row.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) != directory)
                return ERROR_DIRECTORY;
            candidate = row.FileId;
            candidateAttributes = row.FileAttributes;
            found = true;
            return ERROR_SUCCESS;
        }, beforePage);
    if (error)
        return error;
    const DWORD interrupted = beforePage();
    if (interrupted)
        return interrupted;
    if (!found)
        return ERROR_FILE_NOT_FOUND;
    id = candidate;
    attributes = candidateAttributes;
    return ERROR_SUCCESS;
}
#endif

inline DWORD CaptureChildDirectoryId(HANDLE parent, std::wstring_view child, FILE_ID_128& id)
{
    DWORD attributes = 0;
    return CaptureChildId(parent, child, true, id, attributes);
}

// Capture the complete revision shown by a readable, pinned parent. The broker
// still opens the child without following links and compares every field before
// replacing it; a directory entry alone never authorizes a name-only overwrite.
inline DWORD CaptureChildRevision(HANDLE parent, std::wstring_view child, ULONGLONG serial,
                                  UacF7::ArchiveRevision& revision)
{
    if (!UacDirectoryLeaf::IsComponent(child))
        return ERROR_INVALID_NAME;
    bool found = false;
    DWORD count = 0;
    UacF7::ArchiveRevision candidate{};
    const DWORD error = Detail::EnumerateRows(parent, true,
        [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
            if (++count > maxEntries)
                return ERROR_BUFFER_OVERFLOW;
            if (!UacDirectoryLeaf::Equal(name, child))
                return ERROR_SUCCESS;
            if (found)
                return ERROR_DUP_NAME;
            if ((row.FileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
                !UacF7::IsNativePlainCopyFile(row.FileAttributes))
                return ERROR_NOT_SUPPORTED;
            candidate.serial = serial;
            std::memcpy(candidate.fileId, row.FileId.Identifier, sizeof(candidate.fileId));
            candidate.size = row.EndOfFile.QuadPart;
            candidate.created = row.CreationTime.QuadPart;
            candidate.modified = row.LastWriteTime.QuadPart;
            candidate.changed = row.ChangeTime.QuadPart;
            candidate.attributes = row.FileAttributes;
            found = true;
            return ERROR_SUCCESS;
        });
    if (error)
        return error;
    if (!found)
        return ERROR_FILE_NOT_FOUND;
    revision = candidate;
    return ERROR_SUCCESS;
}

inline WIN32_FIND_DATAW FindData(const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name)
{
    WIN32_FIND_DATAW entry{};
    entry.dwFileAttributes = row.FileAttributes;
    entry.ftCreationTime = Time(row.CreationTime);
    entry.ftLastAccessTime = Time(row.LastAccessTime);
    entry.ftLastWriteTime = Time(row.LastWriteTime);
    entry.nFileSizeHigh = static_cast<DWORD>(row.EndOfFile.HighPart);
    entry.nFileSizeLow = row.EndOfFile.LowPart;
    entry.dwReserved0 = row.ReparsePointTag;
    std::memcpy(entry.cFileName, name.data(), name.size() * sizeof(wchar_t));
    return entry;
}

inline DWORD Enumerate(HANDLE directory, std::vector<WIN32_FIND_DATAW>& entries)
{
    entries.clear();
    return Detail::EnumerateRows(directory, false, [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
        if (entries.size() == maxEntries)
            return ERROR_BUFFER_OVERFLOW;
        entries.push_back(FindData(row, name));
        return ERROR_SUCCESS;
    });
}

inline DWORD EnumerateTreeSource(HANDLE directory, std::vector<UacF7::TreeCopySourceEntry>& entries,
                                 ULONGLONG deadline)
{
    return Detail::EnumerateRows(directory, true, [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        if (entries.size() == maxEntries)
            return ERROR_BUFFER_OVERFLOW;
        entries.push_back({FindData(row, name), row.FileId});
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
inline void RecordOmission(SubtreeManifest& manifest, DWORD error, size_t parent, std::wstring_view leaf,
                           std::wstring* firstSkipped)
{
    if (manifest.inventory.firstOmission)
        return;
    manifest.inventory.firstOmission = error;
    if (!firstSkipped)
        return;
    firstSkipped->assign(leaf);
    for (; parent != manifestRoot; parent = manifest.entries[parent].parent)
    {
        if (!firstSkipped->empty())
            firstSkipped->insert(0, 1, L'\\');
        firstSkipped->insert(0, manifest.entries[parent].name);
    }
}

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
                           ULONGLONG deadline, SubtreeManifest& manifest, SizeScan* scan = nullptr,
                           bool sizeInventory = false, std::wstring* firstSkipped = nullptr)
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
        if (row.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            if (!sizeInventory)
                return ERROR_REPARSE_TAG_INVALID;
            if (!VerifyCompatibilityAlias(directory, name, row))
                RecordOmission(manifest, ERROR_REPARSE_TAG_INVALID, parent, name, firstSkipped);
            return ERROR_SUCCESS;
        }
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
            error = UacDirectoryLeaf::OpenParentBelowRoot(directory, recorded.name, raw, identity,
                                                         FILE_LIST_DIRECTORY, !sizeInventory);
            if (sizeInventory && error == ERROR_ACCESS_DENIED)
            {
                RecordOmission(manifest, error, parent, recorded.name, firstSkipped);
                continue;
            }
            if (error)
                return error;
            const std::unique_ptr<void, decltype(&CloseHandle)> child(raw, CloseHandle);
            if (identity.file.VolumeSerialNumber != manifest.root.file.VolumeSerialNumber ||
                !UacDirectoryLeaf::Equal(identity.volume, manifest.root.volume) ||
                !SameId(identity.file.FileId, recorded.id))
                return ERROR_FILE_INVALID;
            error = ScanDirectory(child.get(), depth + 1, index, remaining, deadline, manifest, scan, sizeInventory, firstSkipped);
            if (sizeInventory && error == ERROR_ACCESS_DENIED)
            {
                RecordOmission(manifest, error, parent, manifest.entries[index].name, firstSkipped);
                continue;
            }
            if (error)
                return error;
            ++inventory.directories;
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
inline DWORD CaptureTree(HANDLE directory, ULONGLONG deadline, SubtreeManifest& manifest,
                         UacF7::DirectorySizeDetails* details, bool sizeInventory,
                         std::wstring* firstSkipped = nullptr)
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
    error = Detail::ScanDirectory(directory, 0, manifestRoot, remaining, deadline, complete,
                                  details ? &scan : nullptr, sizeInventory, firstSkipped);
    // A timeout or entry limit can end a safe read early. Keep the verified
    // prefix only for size scans. Identity and malformed-data errors stay fatal.
    if (sizeInventory && (error == ERROR_BUFFER_OVERFLOW || error == ERROR_TIMEOUT) &&
        (complete.inventory.files || complete.inventory.directories))
    {
        Detail::RecordOmission(complete, error, manifestRoot, {}, firstSkipped);
        error = ERROR_SUCCESS;
    }
    if (!error && details)
        *details = std::move(measured);
    if (!error)
        manifest = std::move(complete);
    return error;
}

inline DWORD CaptureManifest(HANDLE directory, ULONGLONG deadline, SubtreeManifest& manifest)
{
    return CaptureTree(directory, deadline, manifest, nullptr, false);
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
                       UacF7::DirectorySizeDetails* details = nullptr, std::wstring* firstSkipped = nullptr)
{
    if (details)
        *details = {};
    if (firstSkipped)
        firstSkipped->clear();
    SubtreeManifest manifest;
    const DWORD error = CaptureTree(directory, deadline, manifest, details, true, firstSkipped);
    inventory = manifest.inventory;
    if (error && firstSkipped)
        firstSkipped->clear();
    return error;
}

// Open the selected root without traversing links. An exact compatibility
// alias is a zero-size exclusion only after rechecking its listing identity.
inline DWORD InventoryBelowRoot(HANDLE root, std::wstring_view relative, ULONGLONG deadline,
                                UacF7::DirectoryInventory& inventory, UacF7::DirectorySizeDetails* details,
                                std::wstring* firstSkipped = nullptr)
{
    inventory = {};
    if (details)
        *details = {};
    if (firstSkipped)
        firstSkipped->clear();
    if (relative.empty())
        return Inventory(root, deadline, inventory, details, firstSkipped);

    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity;
    const DWORD opened = UacDirectoryLeaf::OpenParentBelowRoot(root, relative, raw, identity, FILE_LIST_DIRECTORY, false);
    const std::unique_ptr<void, decltype(&CloseHandle)> directory(raw, CloseHandle);
    if (!opened)
        return Inventory(directory.get(), deadline, inventory, details, firstSkipped);

    const size_t split = relative.find_last_of(L'\\');
    const auto leaf = split == std::wstring_view::npos ? relative : relative.substr(split + 1);
    if (!UacDirectoryLeaf::IsComponent(leaf) || split == 0)
        return opened;
    raw = nullptr;
    if (split != std::wstring_view::npos &&
        UacDirectoryLeaf::OpenParentBelowRoot(root, relative.substr(0, split), raw, identity, FILE_LIST_DIRECTORY, false))
        return opened;
    const std::unique_ptr<void, decltype(&CloseHandle)> parent(raw, CloseHandle);
    const HANDLE handle = parent ? parent.get() : root;
    bool alias = false;
    const DWORD listed = Detail::EnumerateRows(handle, false, [&](const FILE_ID_EXTD_DIR_INFO& row, std::wstring_view name) -> DWORD {
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        if (name != leaf)
            return ERROR_SUCCESS;
        alias = Detail::VerifyCompatibilityAlias(handle, name, row);
        return ERROR_NO_MORE_ITEMS;
    });
    if (listed != ERROR_NO_MORE_ITEMS || !alias)
        return opened;
    inventory.flags = UacF7::inventoryCompatibilityAlias;
    return ERROR_SUCCESS;
}
}
