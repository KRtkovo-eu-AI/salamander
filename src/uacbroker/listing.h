/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Inventory one approved subtree using strict enumeration. | Reject incomplete scans and reparse points. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Enumerate a bounded directory listing by handle. | Avoid resolving a privileged search path from text. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "directoryleaf.h"
#include "f7pilot.h"
#include <cstddef>
#include <memory>
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

inline DWORD Enumerate(HANDLE directory, std::vector<WIN32_FIND_DATAW>& entries, bool strict = false)
{
    entries.clear();
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
                if (entries.size() == maxEntries)
                    return ERROR_BUFFER_OVERFLOW;
                WIN32_FIND_DATAW entry{};
                entry.dwFileAttributes = row->FileAttributes;
                entry.ftCreationTime = Time(row->CreationTime);
                entry.ftLastAccessTime = Time(row->LastAccessTime);
                entry.ftLastWriteTime = Time(row->LastWriteTime);
                entry.nFileSizeHigh = static_cast<DWORD>(row->EndOfFile.HighPart);
                entry.nFileSizeLow = row->EndOfFile.LowPart;
                entry.dwReserved0 = row->ReparsePointTag;
                std::memcpy(entry.cFileName, name.data(), row->FileNameLength);
                entries.push_back(entry);
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

namespace Detail
{
inline DWORD ScanDirectory(HANDLE directory, unsigned depth, DWORD& remaining,
                           ULONGLONG deadline, UacF7::DirectoryInventory& inventory)
{
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    if (depth > maxInventoryDepth)
        return ERROR_BUFFER_OVERFLOW;

    std::vector<WIN32_FIND_DATAW> entries;
    DWORD error = Enumerate(directory, entries, true);
    if (error)
        return error;
    if (entries.size() > remaining)
        return ERROR_BUFFER_OVERFLOW;
    remaining -= static_cast<DWORD>(entries.size());

    for (const auto& entry : entries)
    {
        if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            HANDLE raw = nullptr;
            UacDirectoryLeaf::ParentIdentity identity;
            error = UacDirectoryLeaf::OpenParentBelowRoot(directory, entry.cFileName, raw, identity, FILE_LIST_DIRECTORY);
            if (error)
                return error;
            const std::unique_ptr<void, decltype(&CloseHandle)> child(raw, CloseHandle);
            ++inventory.directories;
            if ((error = ScanDirectory(child.get(), depth + 1, remaining, deadline, inventory)) != ERROR_SUCCESS)
                return error;
        }
        else
        {
            const ULONGLONG bytes = (static_cast<ULONGLONG>(entry.nFileSizeHigh) << 32) | entry.nFileSizeLow;
            if (bytes > MAXULONGLONG - inventory.bytes)
                return ERROR_ARITHMETIC_OVERFLOW;
            inventory.bytes += bytes;
            ++inventory.files;
        }
    }
    return GetTickCount64() < deadline ? ERROR_SUCCESS : ERROR_TIMEOUT;
}
}

// Publish the counts only if every entry could be visited and represented.
inline DWORD Inventory(HANDLE directory, ULONGLONG deadline, UacF7::DirectoryInventory& inventory)
{
    inventory = {};
    UacF7::DirectoryInventory complete;
    DWORD remaining = maxEntries;
    const DWORD error = Detail::ScanDirectory(directory, 0, remaining, deadline, complete);
    if (!error)
        inventory = complete;
    return error;
}
}
