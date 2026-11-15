/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-15 | Traverse protected size inventories by verified pages. | Avoid entry and recursion limits while retaining scoped identity checks. | #731 | GPT-6 |
*/
#pragma once

#include "listing_page.h"
#include <climits>
#include <set>

namespace UacListing
{
namespace SizeDetail
{
struct CloseHandleDeleter { void operator()(void* handle) const { if (handle) CloseHandle(handle); } };
using Handle = std::unique_ptr<void, CloseHandleDeleter>;

struct Frame
{
    Handle directory;
    PagedCursor cursor;
    std::vector<UacF7::PagedListingEntry> page;
    std::set<std::wstring> names;
    size_t offset = 0;
    size_t pathBefore = 0;
    bool done = false;
};

inline DWORD OpenFrame(HANDLE raw, const UacF7::MoveFileIdentity& expected, std::unique_ptr<Frame>& output)
{
    Handle owned(raw);
    auto frame = std::make_unique<Frame>();
    frame->directory = std::move(owned);
    HANDLE duplicate = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(), raw, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS))
        return GetLastError();
    UacF7::MoveFileIdentity actual{};
    // A malformed reparse tag remains a no-follow omission for size scans.
    const DWORD error = frame->cursor.Open(duplicate, &expected, actual, true);
    if (error) return error;
    output = std::move(frame);
    return ERROR_SUCCESS;
}

inline void Omit(UacF7::DirectoryInventory& inventory, DWORD error, const std::wstring& path,
                 std::wstring_view leaf, std::wstring* firstSkipped)
{
    if (inventory.firstOmission) return;
    inventory.firstOmission = error;
    if (!firstSkipped) return;
    *firstSkipped = path;
    if (!leaf.empty())
    {
        if (!firstSkipped->empty()) firstSkipped->push_back(L'\\');
        firstSkipped->append(leaf);
    }
}

inline bool IsCompatibilityAlias(HANDLE parent, const UacF7::PagedListingEntry& entry)
{
    FILE_ID_EXTD_DIR_INFO row{};
    row.FileAttributes = entry.data.dwFileAttributes;
    row.ReparsePointTag = entry.data.dwReserved0;
    std::memcpy(&row.FileId, entry.id, sizeof(entry.id));
    return Detail::VerifyCompatibilityAlias(parent, entry.data.cFileName, row);
}
}

// Keep the original size semantics, but use an explicit stack of independently
// pinned directories and bounded pages instead of a recursive full-tree manifest.
template <typename Emit>
DWORD StreamInventory(HANDLE directory, ULONGLONG deadline, UacF7::DirectoryInventory& inventory,
                      UacF7::DirectorySizeDetails* details, std::wstring* firstSkipped, Emit&& emit)
{
    inventory = {};
    if (details) *details = {};
    if (firstSkipped) firstSkipped->clear();
    try
    {
        UacDirectoryLeaf::ParentIdentity root{};
        DWORD error = UacDirectoryLeaf::ReadParentIdentity(directory, root);
        if (error) return error;
        UacF7::MoveFileIdentity expected{root.file.VolumeSerialNumber};
        std::memcpy(expected.id, root.file.FileId.Identifier, sizeof(expected.id));
        HANDLE rootCopy = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), directory, GetCurrentProcess(), &rootCopy, 0, FALSE, DUPLICATE_SAME_ACCESS))
            return GetLastError();
        std::unique_ptr<SizeDetail::Frame> rootFrame;
        error = SizeDetail::OpenFrame(rootCopy, expected, rootFrame);
        if (error) return error;
        std::vector<std::unique_ptr<SizeDetail::Frame>> stack;
        stack.push_back(std::move(rootFrame));
        std::wstring path;
        UacF7::DirectorySizeDetails measured;
        Detail::SizeScan scan{measured};
        ULONGLONG lastHeartbeat = GetTickCount64();
        if (details)
        {
            const auto volume = root.volume + L"\\";
            DWORD sectors, bytes, freeClusters, totalClusters;
            if (GetDiskFreeSpaceW(volume.c_str(), &sectors, &bytes, &freeClusters, &totalClusters) &&
                sectors && bytes <= MAXDWORD / sectors)
                scan.cluster = sectors * bytes;
        }
        auto flush = [&]() -> DWORD {
            if (measured.fileSizes.empty()) return ERROR_SUCCESS;
            const DWORD sent = emit(UacF7::InventoryFrameKind::FileSizes, measured.fileSizes.data(),
                                    static_cast<DWORD>(measured.fileSizes.size()));
            if (!sent) { measured.fileSizes.clear(); lastHeartbeat = GetTickCount64(); }
            return sent;
        };
        bool deadlineExpired = false;
        while (!stack.empty())
        {
            if (GetTickCount64() >= deadline) { error = ERROR_TIMEOUT; deadlineExpired = true; break; }
            if (GetTickCount64() - lastHeartbeat >= 1000)
            {
                error = emit(UacF7::InventoryFrameKind::Heartbeat, nullptr, 0);
                if (error) break;
                lastHeartbeat = GetTickCount64();
            }
            auto& frame = *stack.back();
            if (frame.offset == frame.page.size())
            {
                if (frame.done)
                {
                    const bool child = stack.size() > 1;
                    const size_t previous = frame.pathBefore;
                    stack.pop_back();
                    path.resize(previous);
                    if (child)
                    {
                        if (inventory.directories == static_cast<DWORD>(INT_MAX)) { error = ERROR_ARITHMETIC_OVERFLOW; break; }
                        ++inventory.directories;
                    }
                    continue;
                }
                error = emit(UacF7::InventoryFrameKind::Heartbeat, nullptr, 0);
                if (error) break;
                lastHeartbeat = GetTickCount64();
                frame.page.clear();
                frame.offset = 0;
                error = frame.cursor.Next(frame.page, frame.done);
                if (error)
                {
                    if (!UacF7::IsSkippableSizeBranchError(error)) break;
                    SizeDetail::Omit(inventory, error, path, {}, firstSkipped);
                    error = ERROR_SUCCESS;
                    frame.done = true;
                }
                continue;
            }
            const auto row = frame.page[frame.offset++];
            const std::wstring_view name(row.data.cFileName);
            if (row.data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            {
                if (!SizeDetail::IsCompatibilityAlias(frame.directory.get(), row))
                    SizeDetail::Omit(inventory, ERROR_REPARSE_POINT_ENCOUNTERED, path, name, firstSkipped);
                continue;
            }
            if (!frame.names.emplace(name).second) { error = ERROR_FILE_INVALID; break; }
            const ULONGLONG bytes = (static_cast<ULONGLONG>(row.data.nFileSizeHigh) << 32) | row.data.nFileSizeLow;
            FILE_ID_128 fileId{};
            std::memcpy(fileId.Identifier, row.id, sizeof(row.id));
            if (row.data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                HANDLE raw = nullptr;
                UacDirectoryLeaf::ParentIdentity identity{};
                error = UacDirectoryLeaf::OpenParentBelowRoot(frame.directory.get(), name, raw, identity,
                                                              FILE_LIST_DIRECTORY, false);
                if (error && UacF7::IsSkippableSizeBranchError(error))
                {
                    SizeDetail::Omit(inventory, error, path, name, firstSkipped);
                    error = ERROR_SUCCESS;
                    continue;
                }
                if (error) break;
                const SizeDetail::Handle owned(raw);
                if (identity.file.VolumeSerialNumber != root.file.VolumeSerialNumber ||
                    !UacDirectoryLeaf::Equal(identity.volume, root.volume) || !Detail::SameId(identity.file.FileId, fileId))
                { error = ERROR_FILE_INVALID; break; }
                UacF7::MoveFileIdentity childExpected{identity.file.VolumeSerialNumber};
                std::memcpy(childExpected.id, identity.file.FileId.Identifier, sizeof(childExpected.id));
                HANDLE childCopy = nullptr;
                if (!DuplicateHandle(GetCurrentProcess(), owned.get(), GetCurrentProcess(), &childCopy, 0, FALSE, DUPLICATE_SAME_ACCESS))
                { error = GetLastError(); break; }
                std::unique_ptr<SizeDetail::Frame> child;
                error = SizeDetail::OpenFrame(childCopy, childExpected, child);
                if (error && UacF7::IsSkippableSizeBranchError(error))
                {
                    SizeDetail::Omit(inventory, error, path, name, firstSkipped);
                    error = ERROR_SUCCESS;
                    continue;
                }
                if (error) break;
                child->pathBefore = path.size();
                if (!path.empty()) path.push_back(L'\\');
                path.append(name);
                stack.push_back(std::move(child));
            }
            else
            {
                if (inventory.files == static_cast<DWORD>(INT_MAX) || bytes > MAXULONGLONG - inventory.bytes)
                { error = ERROR_ARITHMETIC_OVERFLOW; break; }
                inventory.bytes += bytes;
                ++inventory.files;
                if (details)
                {
                    const ManifestEntry item{manifestRoot, std::wstring(name), fileId, row.data.dwFileAttributes, bytes};
                    error = Detail::MeasureFile(frame.directory.get(), item, root.file.VolumeSerialNumber, scan);
                    if (error) break;
                    if (measured.fileSizes.size() == UacF7::maxInventoryFrameSizes && (error = flush())) break;
                }
            }
        }
        if (deadlineExpired && (inventory.files || inventory.directories))
        {
            SizeDetail::Omit(inventory, error, path, {}, firstSkipped);
            error = ERROR_SUCCESS;
        }
        if (!error) error = flush();
        if (!error && details) details->disk = measured.disk;
        return error;
    }
    catch (const std::bad_alloc&) { return ERROR_NOT_ENOUGH_MEMORY; }
}

template <typename Emit>
DWORD StreamInventoryBelowRoot(HANDLE root, std::wstring_view relative, ULONGLONG deadline,
                               UacF7::DirectoryInventory& inventory, UacF7::DirectorySizeDetails* details,
                               std::wstring* firstSkipped, Emit&& emit)
{
    inventory = {};
    if (details) *details = {};
    if (firstSkipped) firstSkipped->clear();
    if (relative.empty())
        return StreamInventory(root, deadline, inventory, details, firstSkipped, emit);
    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity{};
    const DWORD opened = UacDirectoryLeaf::OpenParentBelowRoot(root, relative, raw, identity, FILE_LIST_DIRECTORY, false);
    const SizeDetail::Handle directory(raw);
    if (!opened)
        return StreamInventory(directory.get(), deadline, inventory, details, firstSkipped, emit);
    return CheckInventoryCompatibilityAlias(root, relative, deadline, opened, inventory,
        [&]() -> DWORD { return emit(UacF7::InventoryFrameKind::Heartbeat, nullptr, 0); });
}

// The existing native callers receive the same aggregate and optional per-file
// lengths; the broker uses the streamed form to keep its own buffers bounded.
inline DWORD Inventory(HANDLE directory, ULONGLONG deadline, UacF7::DirectoryInventory& inventory,
                       UacF7::DirectorySizeDetails* details = nullptr, std::wstring* firstSkipped = nullptr)
{
    UacF7::DirectorySizeDetails measured;
    std::vector<ULONGLONG> sizes;
    const auto emit = [&](UacF7::InventoryFrameKind kind, const ULONGLONG* data, DWORD count) -> DWORD {
        if (kind == UacF7::InventoryFrameKind::FileSizes)
            sizes.insert(sizes.end(), data, data + count);
        return ERROR_SUCCESS;
    };
    try
    {
        const DWORD error = StreamInventory(directory, deadline, inventory, details ? &measured : nullptr, firstSkipped, emit);
        if (error) { inventory = {}; if (firstSkipped) firstSkipped->clear(); return error; }
        if (details) { measured.fileSizes.swap(sizes); *details = std::move(measured); }
        return ERROR_SUCCESS;
    }
    catch (const std::bad_alloc&) { inventory = {}; return ERROR_NOT_ENOUGH_MEMORY; }
}

inline DWORD InventoryBelowRoot(HANDLE root, std::wstring_view relative, ULONGLONG deadline,
                                UacF7::DirectoryInventory& inventory, UacF7::DirectorySizeDetails* details,
                                std::wstring* firstSkipped = nullptr)
{
    inventory = {};
    if (details) *details = {};
    if (firstSkipped) firstSkipped->clear();
    if (relative.empty()) return Inventory(root, deadline, inventory, details, firstSkipped);
    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity{};
    const DWORD opened = UacDirectoryLeaf::OpenParentBelowRoot(root, relative, raw, identity, FILE_LIST_DIRECTORY, false);
    const SizeDetail::Handle directory(raw);
    if (!opened) return Inventory(directory.get(), deadline, inventory, details, firstSkipped);
    return CheckInventoryCompatibilityAlias(root, relative, deadline, opened, inventory,
        []() -> DWORD { return ERROR_SUCCESS; });
}
}
