/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-12 | Enumerate bounded identity-bearing directory pages. | Retain one verified handle, original row IDs and native short names. | #731 | GPT-6 (Codex) |
| 2026-11-12 | Retain pending I/O across unexpected wait failures. | The kernel may still own the status block and buffer. | #731 | GPT-6 (Codex) |
*/
#pragma once

#include "listing.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <vector>

namespace UacListing
{
namespace PagedDetail
{
// FILE_ID_EXTD_BOTH_DIR_INFORMATION / FileIdExtdBothDirectoryInformation (63).
// This driver ABI is not declared by the user-mode Windows SDK. The layout is
// identical on Win32 and Win64 and is validated against the public ntifs ABI.
struct alignas(8) NativeRow
{
    ULONG next;
    ULONG index;
    LARGE_INTEGER created;
    LARGE_INTEGER accessed;
    LARGE_INTEGER written;
    LARGE_INTEGER changed;
    LARGE_INTEGER eof;
    LARGE_INTEGER allocated;
    ULONG attributes;
    ULONG nameBytes;
    ULONG eaSize;
    ULONG reparseTag;
    BYTE id[16];
    signed char shortBytes;
    WCHAR shortName[12];
    WCHAR name[1];
};
static_assert(sizeof(WCHAR) == 2 && alignof(NativeRow) == 8, "unexpected native row alignment");
static_assert(offsetof(NativeRow, created) == 8 && offsetof(NativeRow, attributes) == 56, "unexpected native row times");
static_assert(offsetof(NativeRow, nameBytes) == 60 && offsetof(NativeRow, reparseTag) == 68, "unexpected native row metadata");
static_assert(offsetof(NativeRow, id) == 72 && offsetof(NativeRow, shortBytes) == 88, "unexpected native row identity");
static_assert(offsetof(NativeRow, shortName) == 90 && offsetof(NativeRow, name) == 114 && sizeof(NativeRow) == 120,
              "unexpected native row string layout");

using QueryDirectory = NTSTATUS (NTAPI *)(HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK,
                                          PVOID, ULONG, FILE_INFORMATION_CLASS, BOOLEAN,
                                          PUNICODE_STRING, BOOLEAN);
constexpr NTSTATUS statusPending = static_cast<NTSTATUS>(0x00000103L);
constexpr NTSTATUS statusNoMoreFiles = static_cast<NTSTATUS>(0x80000006UL);
constexpr FILE_INFORMATION_CLASS nativeInfoClass = static_cast<FILE_INFORMATION_CLASS>(63);
constexpr DWORD bufferBytes = 65536;

#ifdef SAM_UAC_LISTING_PAGE_TEST
using WaitFunction = DWORD (WINAPI *)(HANDLE, DWORD);
static WaitFunction testWaitFunction = nullptr;
#endif

inline void WaitForPending(HANDLE event)
{
    bool reported = false;
    for (;;)
    {
#ifdef SAM_UAC_LISTING_PAGE_TEST
        const DWORD result = testWaitFunction ? testWaitFunction(event, INFINITE) : WaitForSingleObject(event, INFINITE);
#else
        const DWORD result = WaitForSingleObject(event, INFINITE);
#endif
        if (result == WAIT_OBJECT_0) return;
        // An unexpected wait failure says nothing about whether the kernel
        // completed the request. Retain the event, directory, IO_STATUS_BLOCK
        // and buffer, even if this means this worker cannot finish promptly.
        if (!reported)
        {
            OutputDebugStringW(L"UAC directory enumeration: pending I/O wait failed; retaining I/O state until completion.\n");
            reported = true;
        }
        Sleep(10);
    }
}

inline DWORD QueryError(NTSTATUS status)
{
    if (status == static_cast<NTSTATUS>(0xC0000002UL) || // STATUS_NOT_IMPLEMENTED
        status == static_cast<NTSTATUS>(0xC0000003UL) || // STATUS_INVALID_INFO_CLASS
        status == static_cast<NTSTATUS>(0xC0000010UL) || // STATUS_INVALID_DEVICE_REQUEST
        status == static_cast<NTSTATUS>(0xC00000BBUL))   // STATUS_NOT_SUPPORTED
        return ERROR_NOT_SUPPORTED;
    return RtlNtStatusToDosError(status);
}

// Decode only the actual bytes returned by the kernel, never the unused tail.
// `nextOffset` is zero for the last record; a dot entry is skipped explicitly.
inline DWORD Decode(const BYTE* bytes, size_t size, size_t offset, UacF7::PagedListingEntry& entry,
                    size_t& nextOffset, bool& skip)
{
    entry = {};
    nextOffset = 0;
    skip = false;
    constexpr size_t header = offsetof(NativeRow, name);
    if (!bytes || offset > size || offset % alignof(NativeRow) ||
        reinterpret_cast<uintptr_t>(bytes + offset) % alignof(NativeRow) || size - offset < header)
        return ERROR_INVALID_DATA;
    const auto* row = reinterpret_cast<const NativeRow*>(bytes + offset);
    const size_t available = size - offset;
    const size_t length = row->next ? static_cast<size_t>(row->next) : available;
    if (length < header || length > available ||
        (row->next && (length % alignof(NativeRow) || available - length < header)) ||
        row->nameBytes % sizeof(WCHAR) || row->nameBytes > length - header ||
        row->nameBytes / sizeof(WCHAR) > UacDirectoryLeaf::maxComponent ||
        row->shortBytes < 0 || row->shortBytes % sizeof(WCHAR) ||
        static_cast<size_t>(row->shortBytes) > sizeof(row->shortName) ||
        static_cast<size_t>(row->shortBytes) >= sizeof(entry.data.cAlternateFileName) || row->eof.QuadPart < 0)
        return ERROR_INVALID_DATA;
    const std::wstring_view name(row->name, row->nameBytes / sizeof(WCHAR));
    if (name == L"." || name == L"..")
        skip = true;
    else if (!UacDirectoryLeaf::IsComponent(name))
        return ERROR_INVALID_NAME;
    const auto shortChars = static_cast<size_t>(row->shortBytes) / sizeof(WCHAR);
    if (shortChars && !UacDirectoryLeaf::IsComponent(std::wstring_view(row->shortName, shortChars)))
        return ERROR_INVALID_DATA;
    const bool reparse = (row->attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
    if (reparse && !row->reparseTag)
        return ERROR_REPARSE_TAG_INVALID;
    const BYTE empty[16]{};
    if (!skip && !std::memcmp(row->id, empty, sizeof(empty)))
        return ERROR_INVALID_DATA;
    if (!skip)
    {
        entry.data.dwFileAttributes = row->attributes;
        entry.data.ftCreationTime = Time(row->created);
        entry.data.ftLastAccessTime = Time(row->accessed);
        entry.data.ftLastWriteTime = Time(row->written);
        entry.data.nFileSizeHigh = static_cast<DWORD>(row->eof.HighPart);
        entry.data.nFileSizeLow = row->eof.LowPart;
        entry.data.dwReserved0 = reparse ? row->reparseTag : 0;
        std::memcpy(entry.data.cFileName, name.data(), row->nameBytes);
        entry.data.cFileName[name.size()] = L'\0';
        std::memcpy(entry.data.cAlternateFileName, row->shortName, static_cast<size_t>(row->shortBytes));
        entry.data.cAlternateFileName[shortChars] = L'\0';
        std::memcpy(entry.id, row->id, sizeof(entry.id));
    }
    nextOffset = row->next ? offset + length : 0;
    return ERROR_SUCCESS;
}
}

// This cursor owns one independently opened synchronous directory handle. It
// cannot be copied or restarted; every returned ID belongs to that handle's
// enumeration. A page is committed only after all its rows have been checked.
class PagedCursor
{
public:
    PagedCursor() = default;
    ~PagedCursor() { Close(); }
    PagedCursor(const PagedCursor&) = delete;
    PagedCursor& operator=(const PagedCursor&) = delete;

    DWORD Open(HANDLE ownedDirectory, const UacF7::MoveFileIdentity* expected, UacF7::MoveFileIdentity& actual)
    {
        Close();
        const bool hasExpected = expected != nullptr;
        const UacF7::MoveFileIdentity expectedValue = hasExpected ? *expected : UacF7::MoveFileIdentity{};
        actual = {};
        if (!ownedDirectory || ownedDirectory == INVALID_HANDLE_VALUE)
            return ERROR_INVALID_HANDLE;
        directory_ = ownedDirectory; // ownership transfers even if validation fails
        try
        {
            UacDirectoryLeaf::ParentIdentity identity;
            DWORD error = UacDirectoryLeaf::ReadParentIdentity(directory_, identity);
            FILE_STANDARD_INFO standard{};
            if (!error && !GetFileInformationByHandleEx(directory_, FileStandardInfo, &standard, sizeof(standard)))
                error = GetLastError();
            if (!error && (!standard.Directory || standard.DeletePending))
                error = ERROR_FILE_INVALID;
            const BYTE empty[16]{};
            if (!error && !std::memcmp(identity.file.FileId.Identifier, empty, sizeof(empty)))
                error = ERROR_INVALID_DATA;
            if (!error && hasExpected && (identity.file.VolumeSerialNumber != expectedValue.serial ||
                std::memcmp(identity.file.FileId.Identifier, expectedValue.id, sizeof(expectedValue.id))))
                error = ERROR_FILE_INVALID;
            // Open a fresh file object, not a duplicated cursor and not an
            // overlapped handle. Keep the supplied object pinned until its ID
            // has been compared with the independently reopened object.
            if (!error)
            {
                HANDLE reopened = nullptr;
                WCHAR emptyName[] = L"";
                UNICODE_STRING objectName{0, 0, emptyName};
                OBJECT_ATTRIBUTES attributes{};
                InitializeObjectAttributes(&attributes, &objectName, OBJ_CASE_INSENSITIVE | OBJ_DONT_REPARSE,
                                           directory_, nullptr);
                IO_STATUS_BLOCK io{};
                const NTSTATUS status = NtCreateFile(&reopened, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                    &attributes, &io, nullptr, FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT, nullptr, 0);
                if (status < 0)
                    error = RtlNtStatusToDosError(status);
                else
                {
                    UacDirectoryLeaf::ParentIdentity reopenedIdentity;
                    error = UacDirectoryLeaf::ReadParentIdentity(reopened, reopenedIdentity);
                    if (!error && !UacDirectoryLeaf::SameParent(identity, reopenedIdentity))
                        error = ERROR_FILE_INVALID;
                    if (error)
                        CloseHandle(reopened);
                    else
                    {
                        CloseHandle(directory_);
                        directory_ = reopened;
                    }
                }
            }
            const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
            query_ = ntdll ? reinterpret_cast<PagedDetail::QueryDirectory>(GetProcAddress(ntdll, "NtQueryDirectoryFile")) : nullptr;
            if (!error && !query_)
                error = ERROR_PROC_NOT_FOUND;
            if (!error)
            {
                buffer_.reset(new (std::nothrow) ULONGLONG[PagedDetail::bufferBytes / sizeof(ULONGLONG)]);
                if (!buffer_)
                    error = ERROR_NOT_ENOUGH_MEMORY;
            }
            if (error)
            {
                Close();
                return error;
            }
            actual.serial = identity.file.VolumeSerialNumber;
            std::memcpy(actual.id, identity.file.FileId.Identifier, sizeof(actual.id));
            return ERROR_SUCCESS;
        }
        catch (const std::bad_alloc&)
        {
            Close();
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    DWORD Next(std::vector<UacF7::PagedListingEntry>& entries, bool& done)
    {
        entries.clear();
        done = false;
        if (!directory_ || !query_ || !buffer_)
            return ERROR_INVALID_HANDLE;
        if (complete_)
        {
            done = true;
            return ERROR_SUCCESS;
        }
        try
        {
            std::vector<UacF7::PagedListingEntry> candidate;
            candidate.reserve(UacF7::maxPagedListingEntries);
            while (candidate.size() < UacF7::maxPagedListingEntries)
            {
                if (!haveRows_)
                {
                    HANDLE event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
                    if (!event) return Fail(GetLastError());
                    IO_STATUS_BLOCK io{};
                    NTSTATUS status = query_(directory_, event, nullptr, nullptr, &io, buffer_.get(),
                        PagedDetail::bufferBytes, PagedDetail::nativeInfoClass, FALSE, nullptr, restart_ ? TRUE : FALSE);
                    restart_ = false;
                    if (status == PagedDetail::statusPending)
                    {
                        PagedDetail::WaitForPending(event);
                        status = io.Status;
                    }
                    CloseHandle(event);
                    if (status == PagedDetail::statusNoMoreFiles)
                    {
                        complete_ = true;
                        break;
                    }
                    if (status < 0)
                        return Fail(PagedDetail::QueryError(status));
                    if (!io.Information || io.Information > PagedDetail::bufferBytes)
                        return Fail(ERROR_INVALID_DATA);
                    used_ = static_cast<size_t>(io.Information);
                    offset_ = 0;
                    haveRows_ = true;
                }
                UacF7::PagedListingEntry entry{};
                size_t next = 0;
                bool skip = false;
                const DWORD error = PagedDetail::Decode(reinterpret_cast<const BYTE*>(buffer_.get()), used_, offset_, entry, next, skip);
                if (error)
                    return Fail(error);
                if (!skip)
                    candidate.push_back(entry);
                if (next)
                    offset_ = next;
                else
                    haveRows_ = false;
            }
            entries.swap(candidate);
            done = complete_;
            return ERROR_SUCCESS;
        }
        catch (const std::bad_alloc&)
        {
            return Fail(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    void Close()
    {
        if (directory_ && directory_ != INVALID_HANDLE_VALUE)
            CloseHandle(directory_);
        directory_ = nullptr;
        query_ = nullptr;
        buffer_.reset();
        used_ = offset_ = 0;
        restart_ = true;
        haveRows_ = complete_ = false;
    }

#ifdef SAM_UAC_LISTING_PAGE_TEST
    // Narrow fixture seam: the ordinary build always calls ntdll directly.
    void TestSetQuery(PagedDetail::QueryDirectory query) { query_ = query; }
#endif

private:
    DWORD Fail(DWORD error) { Close(); return error ? error : ERROR_GEN_FAILURE; }
    HANDLE directory_ = nullptr;
    PagedDetail::QueryDirectory query_ = nullptr;
    std::unique_ptr<ULONGLONG[]> buffer_;
    size_t used_ = 0;
    size_t offset_ = 0;
    bool restart_ = true;
    bool haveRows_ = false;
    bool complete_ = false;
};
}
