/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Enumerate a bounded directory listing by handle. | Avoid resolving a privileged search path from text. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "directoryleaf.h"
#include <cstddef>
#include <vector>

namespace UacListing
{
constexpr DWORD maxEntries = 4096;
static_assert(sizeof(WIN32_FIND_DATAW) == 592);
inline constexpr wchar_t marker[] = L"listing";

inline FILETIME Time(LARGE_INTEGER input)
{
    return {input.LowPart, static_cast<DWORD>(input.HighPart)};
}

inline DWORD Enumerate(HANDLE directory, std::vector<WIN32_FIND_DATAW>& entries)
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
            if (!next)
                break;
            offset += next;
        }
    }
    const DWORD error = GetLastError();
    return error == ERROR_NO_MORE_FILES ? ERROR_SUCCESS : error;
}
}
