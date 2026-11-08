// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Check the host connection during EFS transfer. | Stop raw callbacks after a disconnect. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Wrap bounded EFS Raw transfers. | Keep encrypted bytes intact and isolate the path-dependent open from transfer. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "renameleaf.h"
#include <windows.h>
#include <winefs.h>
#include <algorithm>
#include <cstring>
#include <new>
#include <vector>

namespace UacEfsRaw
{
// EFS Raw takes paths. Callers must control the namespace independently;
// a successful open does not expose a file handle for identity verification.
class Context
{
public:
    Context() = default;
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    ~Context()
    {
        if (value)
            CloseEncryptedFileRaw(value);
    }

    DWORD Open(const wchar_t* path, ULONG flags)
    {
        if (value)
            return ERROR_ALREADY_INITIALIZED;
        PVOID opened = nullptr;
        const DWORD error = OpenEncryptedFileRawW(path, flags, &opened);
        if (!error)
            value = opened;
        return error;
    }

    DWORD Read(std::vector<BYTE>& bytes, size_t maximum, ULONGLONG deadline, HANDLE connection = nullptr)
    {
        bytes.clear();
        if (!value)
            return ERROR_INVALID_HANDLE;
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        ExportState state{bytes, maximum, deadline, connection};
        const DWORD result = ReadEncryptedFileRaw(Export, &state, value);
        DWORD error = state.error ? state.error : result;
        if (!error && bytes.empty())
            error = ERROR_INVALID_DATA;
        if (!error && GetTickCount64() >= deadline)
            error = ERROR_TIMEOUT;
        if (error)
            bytes.clear();
        return error;
    }

    DWORD Write(const std::vector<BYTE>& bytes, ULONGLONG deadline, HANDLE connection = nullptr)
    {
        if (!value)
            return ERROR_INVALID_HANDLE;
        if (bytes.empty())
            return ERROR_INVALID_DATA;
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        ImportState state{bytes, deadline, connection};
        const DWORD error = WriteEncryptedFileRaw(Import, &state, value);
        if (state.error)
            return state.error;
        if (error)
            return error;
        if (GetTickCount64() >= deadline)
            return ERROR_TIMEOUT;
        return state.offset == bytes.size() ? ERROR_SUCCESS : ERROR_INVALID_DATA;
    }

private:
    struct ExportState
    {
        std::vector<BYTE>& bytes;
        size_t maximum;
        ULONGLONG deadline;
        HANDLE connection;
        DWORD error = ERROR_SUCCESS;
    };

    struct ImportState
    {
        const std::vector<BYTE>& bytes;
        ULONGLONG deadline;
        HANDLE connection;
        size_t offset = 0;
        DWORD error = ERROR_SUCCESS;
    };

    static DWORD CALLBACK Export(PBYTE block, PVOID context, ULONG length)
    {
        auto& state = *static_cast<ExportState*>(context);
        if (state.error)
            return state.error;
        if (const DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(state.connection, state.deadline))
            return state.error = error;
        if (length > state.maximum - state.bytes.size() || length > state.bytes.max_size() - state.bytes.size())
            return state.error = ERROR_FILE_TOO_LARGE;
        if (!length)
            return ERROR_SUCCESS;
        try
        {
            state.bytes.insert(state.bytes.end(), block, block + length);
        }
        catch (const std::bad_alloc&)
        {
            return state.error = ERROR_NOT_ENOUGH_MEMORY;
        }
        return ERROR_SUCCESS;
    }

    static DWORD CALLBACK Import(PBYTE block, PVOID context, PULONG length)
    {
        auto& state = *static_cast<ImportState*>(context);
        if (state.error)
            return state.error;
        if (const DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(state.connection, state.deadline))
            return state.error = error;
        *length = static_cast<ULONG>(std::min<size_t>(*length, state.bytes.size() - state.offset));
        if (*length)
        {
            std::memcpy(block, state.bytes.data() + state.offset, *length);
            state.offset += *length;
        }
        return ERROR_SUCCESS;
    }

    PVOID value = nullptr;
};
}
