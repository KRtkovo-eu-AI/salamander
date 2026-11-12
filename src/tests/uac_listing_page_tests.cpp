// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise bounded identity-bearing directory pages. | Check real NTFS enumeration and reject malformed native records. | #731 | GPT-6 (Codex) |
#define SAM_UAC_LISTING_PAGE_TEST
#include "../uacbroker/listing_page.h"
#include <cstdio>
#include <string>
#include <vector>

static int failures = 0;
static int skipped = 0;
static void Check(bool value, const char* label)
{
    std::printf("%s: %s\n", value ? "PASS" : "FAIL", label);
    if (!value) ++failures;
}
static void Skip(const char* label)
{
    ++skipped;
    std::printf("SKIP: %s\n", label);
}
static HANDLE OpenDirectory(const std::wstring& path)
{
    return CreateFileW(path.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
}
static bool MakeFile(const std::wstring& path)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    CloseHandle(file);
    return true;
}
static std::wstring Item(unsigned i)
{
    wchar_t name[64]{};
    swprintf_s(name, L"\\entry-%04u.txt", i);
    return name;
}
static unsigned fakeCalls = 0;
static bool fakeMalformed = false;
static PIO_STATUS_BLOCK pendingIo = nullptr;
static PVOID pendingBuffer = nullptr;
static unsigned pendingWaits = 0;
static bool pendingStateLive = true;
static NTSTATUS NTAPI PendingQuery(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK io,
    PVOID output, ULONG, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN)
{
    pendingIo = io;
    pendingBuffer = output;
    io->Status = UacListing::PagedDetail::statusPending;
    return UacListing::PagedDetail::statusPending;
}
static DWORD WINAPI PendingWait(HANDLE event, DWORD timeout)
{
    DWORD flags = 0;
    pendingStateLive = pendingStateLive && pendingIo && pendingBuffer &&
        pendingIo->Status == UacListing::PagedDetail::statusPending &&
        GetHandleInformation(event, &flags) && timeout == INFINITE;
    if (++pendingWaits < 3)
        return WAIT_FAILED;
    pendingIo->Status = UacListing::PagedDetail::statusNoMoreFiles;
    pendingIo->Information = 0;
    SetEvent(event);
    return WaitForSingleObject(event, 0);
}
static NTSTATUS NTAPI FakeQuery(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK io,
    PVOID output, ULONG bytes, FILE_INFORMATION_CLASS, BOOLEAN, PUNICODE_STRING, BOOLEAN)
{
    if (fakeCalls++ != 0) return static_cast<NTSTATUS>(0xC0000022UL); // STATUS_ACCESS_DENIED
    if (bytes < 2 * sizeof(UacListing::PagedDetail::NativeRow)) return static_cast<NTSTATUS>(0xC000000DL);
    auto* first = static_cast<UacListing::PagedDetail::NativeRow*>(output);
    std::memset(output, 0, 2 * sizeof(*first));
    first->id[0] = 1;
    first->attributes = FILE_ATTRIBUTE_ARCHIVE;
    first->nameBytes = sizeof(WCHAR);
    first->name[0] = L'a';
    first->next = fakeMalformed ? sizeof(*first) : 0;
    if (fakeMalformed)
    {
        auto* second = reinterpret_cast<UacListing::PagedDetail::NativeRow*>(
            static_cast<BYTE*>(output) + sizeof(*first));
        second->nameBytes = 1; // invalid UTF-16 byte count
    }
    io->Status = 0;
    io->Information = fakeMalformed ? 2 * sizeof(*first) : sizeof(*first);
    return 0;
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    using namespace UacListing;
    using namespace PagedDetail;
    NativeRow row{};
    row.id[0] = 7;
    row.attributes = FILE_ATTRIBUTE_ARCHIVE;
    row.nameBytes = sizeof(WCHAR);
    row.name[0] = L'x';
    row.shortBytes = 2 * sizeof(WCHAR);
    row.shortName[0] = L'X'; row.shortName[1] = L'1';
    UacF7::PagedListingEntry decoded{};
    size_t next = 0;
    bool skip = false;
    const auto decode = [&]() { return Decode(reinterpret_cast<const BYTE*>(&row), sizeof(row), 0, decoded, next, skip); };
    Check(decode() == ERROR_SUCCESS && !skip && wcscmp(decoded.data.cFileName, L"x") == 0 &&
          wcscmp(decoded.data.cAlternateFileName, L"X1") == 0 && decoded.id[0] == 7,
          "native row retains complete name, short name and 128-bit ID");
    row.shortBytes = 1;
    Check(decode() == ERROR_INVALID_DATA, "odd short-name byte length is rejected");
    row.shortBytes = 26;
    Check(decode() == ERROR_INVALID_DATA, "short name exceeding native field is rejected");
    row.shortBytes = 2;
    row.shortName[0] = 0;
    Check(decode() == ERROR_INVALID_DATA, "embedded null in short name is rejected");
    row.shortName[0] = L':';
    Check(decode() == ERROR_INVALID_DATA, "short name containing a stream separator is rejected");
    row.shortBytes = 0;
    row.nameBytes = 1;
    Check(decode() == ERROR_INVALID_DATA, "odd long-name byte length is rejected");
    row.nameBytes = static_cast<ULONG>(sizeof(row));
    Check(decode() == ERROR_INVALID_DATA, "long name extending past actual reply is rejected");
    row.nameBytes = sizeof(WCHAR);
    row.next = 8;
    Check(decode() == ERROR_INVALID_DATA, "next record shorter than header is rejected");
    row.next = 0;
    row.attributes |= FILE_ATTRIBUTE_REPARSE_POINT;
    Check(decode() == ERROR_REPARSE_TAG_INVALID, "reparse record without a tag is rejected");
    row.reparseTag = IO_REPARSE_TAG_SYMLINK;
    Check(decode() == ERROR_SUCCESS && (decoded.data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
          decoded.data.dwReserved0 == IO_REPARSE_TAG_SYMLINK, "valid reparse type and tag are preserved without following");
    row.attributes = FILE_ATTRIBUTE_ARCHIVE;
    Check(decode() == ERROR_SUCCESS && decoded.data.dwReserved0 == 0, "irrelevant tag is cleared for an ordinary file");
    row.id[0] = 0;
    Check(decode() == ERROR_INVALID_DATA, "zero file identity is rejected");
    row.id[0] = 7;
    row.name[0] = L'.';
    Check(decode() == ERROR_SUCCESS && skip, "dot entry is skipped without a published row");
    Check(Decode(reinterpret_cast<const BYTE*>(&row), offsetof(NativeRow, name) - 1, 0, decoded, next, skip)
          == ERROR_INVALID_DATA, "short native buffer is rejected");
    Check(Decode(reinterpret_cast<const BYTE*>(&row) + 1, sizeof(row) - 1, 0, decoded, next, skip)
          == ERROR_INVALID_DATA, "misaligned native buffer is rejected");
    Check(QueryError(static_cast<NTSTATUS>(0xC0000003UL)) == ERROR_NOT_SUPPORTED &&
          QueryError(static_cast<NTSTATUS>(0xC00000BBUL)) == ERROR_NOT_SUPPORTED &&
          QueryError(static_cast<NTSTATUS>(0xC0000022UL)) == ERROR_ACCESS_DENIED,
          "unsupported filesystem class is distinguished from access denied");

    const std::wstring root = std::wstring(argv[1]) + L"\\listing-page-" + std::to_wstring(GetCurrentProcessId());
    if (!CreateDirectoryW(root.c_str(), nullptr)) return 3;
    const std::wstring unicode = root + L"\\東-long-name-for-alternate-name-123.txt";
    unsigned created = 0;
    while (created < 4103 && MakeFile(root + Item(created))) ++created;
    const bool unicodeCreated = MakeFile(unicode);
    if (created != 4103 || !unicodeCreated)
    {
        for (unsigned i = 0; i < created; ++i) DeleteFileW((root + Item(i)).c_str());
        if (unicodeCreated) DeleteFileW(unicode.c_str());
        RemoveDirectoryW(root.c_str());
        return 4;
    }
    WIN32_FIND_DATAW find{};
    HANDLE search = FindFirstFileW(unicode.c_str(), &find);
    if (search == INVALID_HANDLE_VALUE) return 5;
    FindClose(search);
    HANDLE file = CreateFileW(unicode.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    FILE_ID_INFO fileId{};
    if (file == INVALID_HANDLE_VALUE || !GetFileInformationByHandleEx(file, FileIdInfo, &fileId, sizeof(fileId))) return 6;
    CloseHandle(file);
    PagedCursor cursor;
    UacF7::MoveFileIdentity actual{};
    const DWORD opened = cursor.Open(OpenDirectory(root), nullptr, actual);
    if (opened) std::printf("OpenError=%lu\n", static_cast<unsigned long>(opened));
    Check(opened == ERROR_SUCCESS, "real NTFS directory opens with verified ID");
    std::vector<UacF7::PagedListingEntry> page;
    bool done = false, unicodeFound = false, bounded = true, matchedId = false, matchedAlternate = false, matchedMetadata = false;
    unsigned total = 0, pages = 0;
    DWORD error = opened;
    while (!error && !done && pages < 1000)
    {
        error = cursor.Next(page, done);
        ++pages;
        bounded = bounded && page.size() <= UacF7::maxPagedListingEntries;
        total += static_cast<unsigned>(page.size());
        for (const auto& entry : page)
            if (wcscmp(entry.data.cFileName, find.cFileName) == 0)
            {
                unicodeFound = true;
                matchedId = std::memcmp(entry.id, fileId.FileId.Identifier, sizeof(entry.id)) == 0;
                matchedAlternate = wcscmp(entry.data.cAlternateFileName, find.cAlternateFileName) == 0;
                matchedMetadata = entry.data.dwFileAttributes == find.dwFileAttributes &&
                    !std::memcmp(&entry.data.ftCreationTime, &find.ftCreationTime, sizeof(FILETIME)) &&
                    !std::memcmp(&entry.data.ftLastWriteTime, &find.ftLastWriteTime, sizeof(FILETIME)) &&
                    entry.data.nFileSizeLow == find.nFileSizeLow && entry.data.nFileSizeHigh == find.nFileSizeHigh;
            }
    }
    Check(!error && done && bounded && total == 4104 && pages > 64,
          "more than 4096 entries complete through bounded pages without restart");
    Check(unicodeFound && matchedId, "Unicode name carries ID of the actual opened file");
    Check(unicodeFound && matchedMetadata, "attributes, size and timestamps match FindFirstFileW");
    if (find.cAlternateFileName[0])
        Check(unicodeFound && matchedAlternate, "8.3 short name matches FindFirstFileW");
    else
        Skip("NTFS short names are disabled or unavailable for this fixture");
    Check(cursor.Next(page, done) == ERROR_SUCCESS && done && page.empty(), "EOF is stable and does not restart");
    cursor.Close();
    UacF7::MoveFileIdentity wrong = actual;
    ++wrong.serial;
    UacF7::MoveFileIdentity refused{};
    Check(cursor.Open(OpenDirectory(root), &wrong, refused) == ERROR_FILE_INVALID,
          "wrong expected volume identity rejects the directory");
    Check(cursor.Next(page, done) == ERROR_INVALID_HANDLE && page.empty(), "failed open does not leave a usable cursor");
    HANDLE source = OpenDirectory(root), twin = nullptr;
    if (source == INVALID_HANDLE_VALUE || !DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(),
        &twin, 0, FALSE, DUPLICATE_SAME_ACCESS)) return 13;
    PagedCursor firstCursor, secondCursor;
    UacF7::MoveFileIdentity firstParent{}, secondParent{};
    const DWORD firstOpen = firstCursor.Open(source, &actual, firstParent);
    const DWORD secondOpen = secondCursor.Open(twin, &actual, secondParent);
    std::vector<UacF7::PagedListingEntry> firstPage, secondPage;
    bool firstDone = false, secondDone = false;
    const DWORD firstRead = firstOpen ? firstOpen : firstCursor.Next(firstPage, firstDone);
    const DWORD secondRead = secondOpen ? secondOpen : secondCursor.Next(secondPage, secondDone);
    bool sameFirst = !firstRead && !secondRead && firstPage.size() == secondPage.size() && !firstPage.empty();
    for (size_t i = 0; sameFirst && i < firstPage.size(); ++i)
        sameFirst = !wcscmp(firstPage[i].data.cFileName, secondPage[i].data.cFileName) &&
                    !std::memcmp(firstPage[i].id, secondPage[i].id, sizeof(firstPage[i].id));
    Check(sameFirst, "duplicated input handles become independent enumeration cursors");
    firstCursor.Close(); secondCursor.Close();
    fakeCalls = 0; fakeMalformed = false;
    Check(cursor.Open(OpenDirectory(root), &actual, refused) == ERROR_SUCCESS, "prepare injected denied-read test");
    cursor.TestSetQuery(FakeQuery);
    Check(cursor.Next(page, done) == ERROR_ACCESS_DENIED && page.empty(),
          "later denied read discards the uncommitted partial page");
    Check(cursor.Next(page, done) == ERROR_INVALID_HANDLE, "denied cursor cannot be resumed past missing rows");
    fakeCalls = 0; fakeMalformed = true;
    Check(cursor.Open(OpenDirectory(root), &actual, refused) == ERROR_SUCCESS, "prepare injected malformed-row test");
    cursor.TestSetQuery(FakeQuery);
    Check(cursor.Next(page, done) == ERROR_INVALID_DATA && page.empty(),
          "malformed later row discards the whole uncommitted page");
    Check(cursor.Open(OpenDirectory(root), &actual, refused) == ERROR_SUCCESS,
          "prepare pending query with unexpected wait failures");
    cursor.TestSetQuery(PendingQuery);
    PagedDetail::testWaitFunction = PendingWait;
    const DWORD pendingResult = cursor.Next(page, done);
    PagedDetail::testWaitFunction = nullptr;
    Check(pendingResult == ERROR_SUCCESS && done && page.empty() && pendingStateLive && pendingWaits == 3,
          "pending I/O state stays alive through wait failures until completion signals");
    cursor.Close();
    HANDLE asyncDirectory = CreateFileW(root.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OVERLAPPED, nullptr);
    const DWORD asyncOpen = cursor.Open(asyncDirectory, nullptr, refused);
    const DWORD asyncRead = asyncOpen ? asyncOpen : cursor.Next(page, done);
    Check(!asyncOpen && !asyncRead && !page.empty(),
          "asynchronous input is reopened as an independent synchronous directory cursor");
    cursor.Close();
    UacF7::MoveFileIdentity aliased = actual;
    Check(cursor.Open(OpenDirectory(root), &aliased, aliased) == ERROR_SUCCESS && aliased.serial == actual.serial &&
          !std::memcmp(aliased.id, actual.id, sizeof(actual.id)), "expected and actual identity may safely alias");
    cursor.Close();
    HANDLE ordinaryFile = CreateFileW(unicode.c_str(), FILE_READ_ATTRIBUTES | SYNCHRONIZE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD unusedFlags = 0;
    const DWORD rejectedFile = cursor.Open(ordinaryFile, nullptr, refused);
    Check(rejectedFile != ERROR_SUCCESS && !GetHandleInformation(ordinaryFile, &unusedFlags),
          "failed open consumes the caller-owned non-directory handle");

    // Test the identity bound to a handle even if the visible directory name
    // can be replaced. Some NTFS configurations disallow the rename while an
    // enumeration handle is open; that variant is reported as SKIP.
    const std::wstring moved = root + L"-moved";
    Check(cursor.Open(OpenDirectory(root), &actual, refused) == ERROR_SUCCESS, "same directory ID opens a new cursor");
    if (MoveFileW(root.c_str(), moved.c_str()))
    {
        if (!CreateDirectoryW(root.c_str(), nullptr) || !MakeFile(root + L"\\replacement-marker.txt")) return 7;
        bool noReplacement = true;
        unsigned oldCount = 0;
        done = false;
        error = ERROR_SUCCESS;
        while (!done && !error)
        {
            error = cursor.Next(page, done);
            for (const auto& entry : page)
            {
                ++oldCount;
                if (wcscmp(entry.data.cFileName, L"replacement-marker.txt") == 0) noReplacement = false;
            }
        }
        Check(!error && noReplacement && oldCount == 4104,
              "same-name replacement cannot change an already pinned enumeration");
        cursor.Close();
        Check(cursor.Open(OpenDirectory(root), &actual, refused) == ERROR_FILE_INVALID,
              "fresh cursor refuses same-name replacement with old directory ID");
        if (!DeleteFileW((root + L"\\replacement-marker.txt").c_str()) || !RemoveDirectoryW(root.c_str()) ||
            !MoveFileW(moved.c_str(), root.c_str())) return 8;
    }
    else
    {
        Skip("NTFS refused directory rename while its enumeration handle was open");
        cursor.Close();
    }

    const std::wstring sensitive = root + L"\\case-sensitive";
    if (!CreateDirectoryW(sensitive.c_str(), nullptr)) return 9;
    HANDLE caseHandle = CreateFileW(sensitive.c_str(), FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    const ULONG caseSensitive = 1;
    const bool caseEnabled = caseHandle != INVALID_HANDLE_VALUE && SetFileInformationByHandle(caseHandle,
        static_cast<FILE_INFO_BY_HANDLE_CLASS>(23), const_cast<ULONG*>(&caseSensitive), sizeof(caseSensitive));
    if (caseHandle != INVALID_HANDLE_VALUE) CloseHandle(caseHandle);
    if (caseEnabled)
    {
        const std::wstring lower = sensitive + L"\\case.txt";
        const std::wstring upper = sensitive + L"\\CASE.txt";
        if (!MakeFile(lower) || !MakeFile(upper)) return 10;
        Check(cursor.Open(OpenDirectory(sensitive), nullptr, refused) == ERROR_SUCCESS,
              "case-sensitive NTFS directory opens for enumeration");
        bool lowerFound = false, upperFound = false;
        BYTE lowerId[16]{}, upperId[16]{};
        done = false;
        error = ERROR_SUCCESS;
        while (!done && !error)
        {
            error = cursor.Next(page, done);
            for (const auto& entry : page)
            {
                if (!wcscmp(entry.data.cFileName, L"case.txt")) { lowerFound = true; std::memcpy(lowerId, entry.id, 16); }
                if (!wcscmp(entry.data.cFileName, L"CASE.txt")) { upperFound = true; std::memcpy(upperId, entry.id, 16); }
            }
        }
        Check(!error && lowerFound && upperFound && std::memcmp(lowerId, upperId, 16),
              "case-distinct names retain separate exact IDs");
        cursor.Close();
        if (!DeleteFileW(lower.c_str()) || !DeleteFileW(upper.c_str())) return 11;
    }
    else
        Skip("case sensitivity could not be enabled for the test directory");
    if (!RemoveDirectoryW(sensitive.c_str())) return 12;

    for (unsigned i = 0; i < created; ++i)
        if (!DeleteFileW((root + Item(i)).c_str())) ++failures;
    if (!DeleteFileW(unicode.c_str()) || !RemoveDirectoryW(root.c_str())) ++failures;
    std::printf("Failures=%d Skips=%d\n", failures, skipped);
    return failures ? 1 : 0;
}
