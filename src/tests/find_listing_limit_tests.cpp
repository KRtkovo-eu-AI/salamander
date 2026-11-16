// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Exercise the real per-directory listing limit on NTFS. | Keep the independent broker limit after removing Find tree budget. | #731 | GPT-6.1 Terra |
#include "../uacbroker/listing.h"
#include <cstdio>
#include <string>
#include <vector>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root = std::wstring(argv[1]) + L"\\find-limit-" + std::to_wstring(GetCurrentProcessId()) +
                              L"-" + std::to_wstring(GetTickCount64());
    if (!CreateDirectoryW(root.c_str(), nullptr)) { std::printf("fixture create failed: %lu\n", GetLastError()); return 2; }
    unsigned created = 0;
    DWORD fixtureError = ERROR_SUCCESS;
    for (; created != UacListing::maxEntries + 1; ++created)
    {
        const std::wstring name = root + L"\\f" + std::to_wstring(created);
        HANDLE file = CreateFileW(name.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { fixtureError = GetLastError(); break; }
        CloseHandle(file);
    }
    int failures = 0;
    if (fixtureError) { std::printf("fixture file %u failed: %lu\n", created, fixtureError); ++failures; }
    if (!fixtureError)
    {
        const std::wstring last = root + L"\\f" + std::to_wstring(UacListing::maxEntries);
        if (!DeleteFileW(last.c_str())) { std::printf("fixture trim failed: %lu\n", GetLastError()); ++failures; }
        else
        {
            --created;
            HANDLE directory = CreateFileW(root.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (directory == INVALID_HANDLE_VALUE) { std::printf("fixture open failed: %lu\n", GetLastError()); ++failures; }
            else
            {
                std::vector<WIN32_FIND_DATAW> rows;
                const DWORD atLimit = UacListing::Enumerate(directory, rows);
                const bool exact = atLimit == ERROR_SUCCESS && rows.size() == UacListing::maxEntries;
                std::printf("%s: actual listing of 4096 entries succeeds\n", exact ? "PASS" : "FAIL");
                if (!exact) ++failures;
                HANDLE added = CreateFileW(last.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                           nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (added == INVALID_HANDLE_VALUE) { std::printf("fixture restore failed: %lu\n", GetLastError()); ++failures; }
                else
                {
                    CloseHandle(added); ++created;
                    const DWORD overflow = UacListing::Enumerate(directory, rows);
                    const bool rejected = overflow == ERROR_BUFFER_OVERFLOW && rows.size() == UacListing::maxEntries;
                    std::printf("%s: actual listing of 4097 entries rejects overflow\n", rejected ? "PASS" : "FAIL");
                    if (!rejected) ++failures;
                }
                CloseHandle(directory);
            }
        }
    }
    for (unsigned i = 0; i < created; ++i)
    {
        const std::wstring name = root + L"\\f" + std::to_wstring(i);
        if (!DeleteFileW(name.c_str())) { std::printf("cleanup file failed: %lu\n", GetLastError()); ++failures; }
    }
    if (!RemoveDirectoryW(root.c_str())) { std::printf("cleanup directory failed: %lu\n", GetLastError()); ++failures; }
    std::printf("Real NTFS Find listing limit: %d failures\n", failures);
    return failures ? 1 : 0;
}
