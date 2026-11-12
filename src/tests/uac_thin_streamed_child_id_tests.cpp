// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise streamed child identity scans on NTFS. | Preserve duplicate and no-follow checks beyond the old listing limit. | #731 | GPT-6 |
#define SAM_UAC_THIN_COPY_TEST
#include "../uacbroker/listing.h"
#include <cstdio>
#include <cstring>
#include <string>

static int failures = 0;
static int skips = 0;
static void Check(bool value, const char* name)
{
    std::printf("%s: %s\n", value ? "PASS" : "FAIL", name);
    if (!value) ++failures;
}
static void Skip(const char* name)
{
    std::printf("SKIP: %s\n", name);
    ++skips;
}
static bool MakeFile(const std::wstring& path)
{
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    CloseHandle(file);
    return true;
}
static HANDLE OpenDirectory(const std::wstring& path, DWORD access = FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES)
{
    return CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
}
static std::wstring Item(unsigned index)
{
    wchar_t value[64]{};
    swprintf_s(value, L"\\entry-%05u.txt", index);
    return value;
}
static bool Sentinel(const FILE_ID_128& id, DWORD attrs)
{
    BYTE expected[16]; std::memset(expected, 0xA5, sizeof(expected));
    return attrs == 0xABCD1234 && std::memcmp(id.Identifier, expected, sizeof(expected)) == 0;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root = std::wstring(L"\\\\?\\") + argv[1];
    const std::wstring one = root + L"\\" + std::wstring(82, L'東');
    const std::wstring two = one + L"\\" + std::wstring(82, L'日');
    const std::wstring dir = two + L"\\" + std::wstring(82, L'語');
    if (!CreateDirectoryW(one.c_str(), nullptr) || !CreateDirectoryW(two.c_str(), nullptr) ||
        !CreateDirectoryW(dir.c_str(), nullptr))
    {
        std::printf("FAIL: create long Unicode test directory (%lu)\n", GetLastError());
        return 2;
    }
    const HANDLE listing = OpenDirectory(dir);
    if (listing == INVALID_HANDLE_VALUE) return 2;
    wchar_t fs[64]{};
    if (!GetVolumeInformationByHandleW(listing, nullptr, 0, nullptr, nullptr, nullptr, fs, _countof(fs)) ||
        _wcsicmp(fs, L"NTFS") != 0)
    {
        std::printf("FAIL: isolated test requires NTFS\n");
        CloseHandle(listing); return 2;
    }
    constexpr unsigned total = 4105;
    unsigned created = 0;
    for (; created < total; ++created)
        if (!MakeFile(dir + Item(created))) break;
    const std::wstring unicode = dir + L"\\東-child-日.txt";
    const bool madeUnicode = created == total && MakeFile(unicode);
    Check(madeUnicode && unicode.size() > 260, "create over 4096 files under a long Unicode path");
    if (madeUnicode)
    {
        const HANDLE child = CreateFileW(unicode.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        FILE_ID_INFO expected{};
        Check(child != INVALID_HANDLE_VALUE && GetFileInformationByHandleEx(child, FileIdInfo, &expected, sizeof(expected)),
              "read actual 128-bit file identity");
        if (child != INVALID_HANDLE_VALUE) CloseHandle(child);

        FILE_ID_128 id{}; DWORD attrs = 0;
        DWORD bounded = UacListing::CaptureChildId(listing, L"東-child-日.txt", false, id, attrs);
        Check(bounded == ERROR_BUFFER_OVERFLOW, "existing bounded API still refuses more than 4096 entries");
        unsigned pages = 0;
        const auto ok = [&]() -> DWORD { ++pages; return ERROR_SUCCESS; };
        DWORD result = UacListing::CaptureChildIdStreaming(listing, L"東-child-日.txt", false, id, attrs, ok);
        Check(result == ERROR_SUCCESS && pages > 2 &&
              std::memcmp(id.Identifier, expected.FileId.Identifier, sizeof(id.Identifier)) == 0 &&
              !(attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)),
              "stream all pages and return the exact Unicode file identity");

        // Simulate a host repeatedly resetting its own directory cursor while
        // the broker enumerates a separately opened object for the same ID.
        const HANDLE independent = OpenDirectory(dir);
        unsigned resets = 0;
        alignas(8) BYTE hostBuffer[8192]{};
        const auto interfere = [&]() -> DWORD {
            if (++resets > 100) return ERROR_CANCELLED;
            if (!GetFileInformationByHandleEx(listing, FileIdExtdDirectoryRestartInfo,
                                               hostBuffer, sizeof(hostBuffer)))
                return GetLastError();
            return ERROR_SUCCESS;
        };
        FILE_ID_128 independentId{}; DWORD independentAttrs = 0;
        const DWORD independentResult = independent == INVALID_HANDLE_VALUE ? ERROR_INVALID_HANDLE :
            UacListing::CaptureChildIdStreaming(independent, L"東-child-日.txt", false,
                                               independentId, independentAttrs, interfere);
        Check(independentResult == ERROR_SUCCESS && resets > 2 && resets < 100 &&
              std::memcmp(independentId.Identifier, expected.FileId.Identifier, sizeof(independentId.Identifier)) == 0,
              "host cursor resets cannot affect an independently opened scan");
        if (independent != INVALID_HANDLE_VALUE) CloseHandle(independent);

        FILE_ID_128 sentinel; std::memset(&sentinel, 0xA5, sizeof(sentinel));
        DWORD unchanged = 0xABCD1234;
        unsigned calls = 0;
        result = UacListing::CaptureChildIdStreaming(listing, L"東-child-日.txt", false, sentinel, unchanged,
            [&]() -> DWORD { return ++calls == 2 ? ERROR_CANCELLED : ERROR_SUCCESS; });
        Check(result == ERROR_CANCELLED && calls == 2 && Sentinel(sentinel, unchanged),
              "cancellation between pages never publishes a partial identity");
        calls = 0;
        result = UacListing::CaptureChildIdStreaming(listing, L"東-child-日.txt", false, sentinel, unchanged,
            [&]() -> DWORD { return ++calls == pages ? ERROR_TIMEOUT : ERROR_SUCCESS; });
        Check(result == ERROR_TIMEOUT && calls == pages && Sentinel(sentinel, unchanged),
              "timeout after scanning is checked before publishing identity");
        auto stateful = [seen = 0u, last = pages]() mutable -> DWORD {
            return ++seen == last ? ERROR_TIMEOUT : ERROR_SUCCESS;
        };
        result = UacListing::CaptureChildIdStreaming(listing, L"東-child-日.txt", false, sentinel, unchanged, stateful);
        Check(result == ERROR_TIMEOUT && Sentinel(sentinel, unchanged),
              "one stateful callback covers enumeration and the final publication check");
        calls = 0;
        result = UacListing::CaptureChildIdStreaming(listing, L"東-child-日.txt", false, sentinel, unchanged,
            [&]() -> DWORD { ++calls; return ERROR_BROKEN_PIPE; });
        Check(result == ERROR_BROKEN_PIPE && calls == 1 && Sentinel(sentinel, unchanged),
              "disconnected peer stops before enumeration without publishing");
        result = UacListing::CaptureChildIdStreaming(listing, L"not-present.txt", false, sentinel, unchanged, ok);
        Check(result == ERROR_FILE_NOT_FOUND && Sentinel(sentinel, unchanged), "missing child is rejected");
        result = UacListing::CaptureChildIdStreaming(listing, L"東-child-日.txt", true, sentinel, unchanged, ok);
        Check(result == ERROR_DIRECTORY && Sentinel(sentinel, unchanged), "unexpected file/directory type is rejected");
        result = UacListing::CaptureChildIdStreaming(listing, L"..\\bad", false, sentinel, unchanged, ok);
        Check(result == ERROR_INVALID_NAME && Sentinel(sentinel, unchanged), "relative traversal in child name is rejected");

        const std::wstring link = dir + L"\\sample-link.txt";
        if (CreateSymbolicLinkW(link.c_str(), unicode.c_str(), 0x2 /* ALLOW_UNPRIVILEGED_CREATE */))
        {
            result = UacListing::CaptureChildIdStreaming(listing, L"sample-link.txt", false, sentinel, unchanged, ok);
            Check(result == ERROR_REPARSE_TAG_INVALID && Sentinel(sentinel, unchanged),
                  "unexpected file symlink is rejected without following it");
            Check(DeleteFileW(link.c_str()) != FALSE, "remove isolated symlink");
        }
        else Skip("file symlink creation unavailable to current account");

        const std::wstring caseDir = dir + L"\\case-sensitive";
        bool caseTested = false;
        if (CreateDirectoryW(caseDir.c_str(), nullptr))
        {
            const HANDLE caseHandle = OpenDirectory(caseDir, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES);
            if (caseHandle != INVALID_HANDLE_VALUE)
            {
                FILE_CASE_SENSITIVE_INFO caseInfo{FILE_CS_FLAG_CASE_SENSITIVE_DIR};
                if (SetFileInformationByHandle(caseHandle, FileCaseSensitiveInfo, &caseInfo, sizeof(caseInfo)) &&
                    MakeFile(caseDir + L"\\aCase.txt") && MakeFile(caseDir + L"\\ACASE.txt"))
                {
                    result = UacListing::CaptureChildIdStreaming(caseHandle, L"aCase.txt", false, sentinel, unchanged, ok);
                    Check(result == ERROR_DUP_NAME && Sentinel(sentinel, unchanged),
                          "two case-fold equivalent names are rejected after full scan");
                    caseTested = true;
                }
                CloseHandle(caseHandle);
            }
            DeleteFileW((caseDir + L"\\aCase.txt").c_str());
            DeleteFileW((caseDir + L"\\ACASE.txt").c_str());
            Check(RemoveDirectoryW(caseDir.c_str()) != FALSE, "remove isolated case-sensitive fixture");
        }
        if (!caseTested) Skip("case-sensitive fixture unavailable to current account");
        Check(GetFileAttributesW(unicode.c_str()) != INVALID_FILE_ATTRIBUTES,
              "read-only checks preserve the source");
        Check(DeleteFileW(unicode.c_str()) != FALSE, "remove isolated Unicode child");
    }
    CloseHandle(listing);
    bool cleaned = true;
    for (unsigned i = 0; i < created; ++i)
        if (!DeleteFileW((dir + Item(i)).c_str())) cleaned = false;
    Check(cleaned && RemoveDirectoryW(dir.c_str()) && RemoveDirectoryW(two.c_str()) && RemoveDirectoryW(one.c_str()),
          "remove only the isolated fixture");
    std::printf("Failures: %d; skipped: %d\n", failures, skips);
    return failures ? 1 : 0;
}
