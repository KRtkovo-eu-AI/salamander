// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Publish a copy of a guarded move source. | Read the overlapped source without releasing its lock and retain the exact destination handle. | | GPT-6.1 Sol (Codex) |
// | 2026-11-09 | Guard the default stream of an F6 source. | Exclude earlier opens and refuse unsupported streams or metadata. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "copyleaf.h"
#include "movesourcejournal.h"

namespace UacMoveSourceJournal
{
// Call only after sealing the source and closing the broker's original handle.
// The reservation can then reject preexisting opens without admitting new writers
// when the filter handle is released and the broker reopens its guarded handle.
inline DWORD CheckNoPriorSourceOpens(HANDLE parent, std::wstring_view leaf, const FILE_ID_INFO& expected)
{
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;
    const Detail::Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr), CloseHandle);
    if (!event)
        return GetLastError();
    constexpr DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    HANDLE raw = nullptr;
    const DWORD opened = UacRenameLeaf::Detail::OpenFile(parent, leaf, raw, 0, share, FILE_OPEN,
                                                         FILE_RESERVE_OPFILTER, nullptr, false, true);
    if (opened)
        return opened;
    const Detail::Handle filter(raw, CloseHandle);
    OVERLAPPED io{};
    io.hEvent = event.get();
    DWORD bytes = 0;
    const BOOL complete = DeviceIoControl(filter.get(), FSCTL_REQUEST_FILTER_OPLOCK, nullptr, 0, nullptr, 0, &bytes, &io);
    const DWORD status = complete ? ERROR_OPLOCK_NOT_GRANTED : GetLastError();
    if (status != ERROR_IO_PENDING)
        return status;
    const DWORD result = Detail::CheckIdentity(filter.get(), expected);
    const DWORD state = WaitForSingleObject(event.get(), 0);
    const DWORD error = state == WAIT_FAILED ? GetLastError() : ERROR_SHARING_VIOLATION;
    CancelIoEx(filter.get(), &io);
    GetOverlappedResult(filter.get(), &io, &bytes, TRUE);
    return result ? result : state == WAIT_TIMEOUT ? ERROR_SUCCESS : error;
}

// The oplock covers the default stream. A file-wide DACL seal is still
// required before committing a move so another name cannot gain a new stream.
class SourceGuard
{
public:
    SourceGuard() = default;
    SourceGuard(const SourceGuard&) = delete;
    SourceGuard& operator=(const SourceGuard&) = delete;
    ~SourceGuard() { Close(); }

    DWORD Open(HANDLE parent, std::wstring_view leaf, const FILE_ID_INFO& expected, DWORD links)
    {
        if (source)
            return ERROR_ALREADY_INITIALIZED;
        if (!UacDirectoryLeaf::IsComponent(leaf))
            return ERROR_INVALID_NAME;
        event.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!event)
            return GetLastError();
        io = {};
        io.hEvent = event.get();
        constexpr ACCESS_MASK access = FILE_READ_DATA | FILE_READ_EA | DELETE | READ_CONTROL | WRITE_DAC | SYNCHRONIZE;
        HANDLE raw = nullptr;
        DWORD error = UacRenameLeaf::Detail::OpenFile(parent, leaf, raw, access, 0, FILE_OPEN,
                                                      FILE_OPEN_REQUIRING_OPLOCK, nullptr, false, true);
        if (error)
            return error;
        source.reset(raw);

        REQUEST_OPLOCK_INPUT_BUFFER request{REQUEST_OPLOCK_CURRENT_VERSION, sizeof(request),
            OPLOCK_LEVEL_CACHE_READ | OPLOCK_LEVEL_CACHE_WRITE | OPLOCK_LEVEL_CACHE_HANDLE, REQUEST_OPLOCK_INPUT_FLAG_REQUEST};
        DWORD bytes = 0;
        const BOOL complete = DeviceIoControl(source.get(), FSCTL_REQUEST_OPLOCK, &request, sizeof(request),
                                              &reply, sizeof(reply), &bytes, &io);
        error = complete ? ERROR_OPLOCK_NOT_GRANTED : GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            source.reset();
            return error;
        }
        identity = expected;
        error = Check(links);
        if (error)
            Close();
        return error;
    }

    HANDLE File() const { return source.get(); }

    DWORD PublishCopy(HANDLE targetParent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                      std::wstring_view leaf, ULONGLONG size, HANDLE cancel, HANDLE connection, ULONGLONG deadline,
                      Detail::Handle& target, FILE_ID_INFO& targetId) const
    {
        if (target)
            return ERROR_INVALID_PARAMETER;
        targetId = {};
        if (!UacDirectoryLeaf::IsComponent(leaf))
            return ERROR_INVALID_NAME;
        UacDirectoryLeaf::ParentIdentity actual;
        DWORD error = UacDirectoryLeaf::ReadParentIdentity(targetParent, actual);
        if (error)
            return error;
        if (!UacDirectoryLeaf::SameParent(actual, expectedParent))
            return ERROR_FILE_INVALID;
        if ((error = Check(2)) != ERROR_SUCCESS)
            return error;
        FILE_BASIC_INFO basic{};
        FILE_ID_INFO publishedId{};
        error = UacCopyLeaf::Detail::CopyOpenedFile(source.get(), targetParent, leaf, cancel, connection, deadline,
            size, basic, &publishedId, nullptr, UacCopyLeaf::StreamPolicy::RequireDefault, nullptr, false, nullptr, &target);
        if (error)
            return error;
        targetId = publishedId;
        return Check(2) ? ERROR_PARTIAL_COPY : ERROR_SUCCESS;
    }

    DWORD Check(DWORD links) const
    {
        if (!source)
            return ERROR_INVALID_HANDLE;
        if (DWORD error = Detail::CheckIdentity(source.get(), identity))
            return error;
        FILE_STANDARD_INFO standard{};
        if (!GetFileInformationByHandleEx(source.get(), FileStandardInfo, &standard, sizeof(standard)))
            return GetLastError();
        if (standard.NumberOfLinks != links)
            return ERROR_FILE_INVALID;
        FILE_BASIC_INFO metadata{};
        if (DWORD error = UacCopyLeaf::Detail::ReadPlainMetadata(source.get(), metadata))
            return error;
        const DWORD state = WaitForSingleObject(event.get(), 0);
        if (state == WAIT_TIMEOUT)
            return ERROR_SUCCESS;
        return state == WAIT_OBJECT_0 ? ERROR_SHARING_VIOLATION : GetLastError();
    }

private:
    void Close()
    {
        if (source)
        {
            CancelIoEx(source.get(), &io);
            DWORD bytes = 0;
            GetOverlappedResult(source.get(), &io, &bytes, TRUE);
            source.reset();
        }
    }

    Detail::Handle source{nullptr, CloseHandle};
    Detail::Handle event{nullptr, CloseHandle};
    FILE_ID_INFO identity{};
    OVERLAPPED io{};
    REQUEST_OPLOCK_OUTPUT_BUFFER reply{};
};
}
