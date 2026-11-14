// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Expose scoped Find bytes as a read-only COM stream. | Read keywords in the host without copying protected files. | | GPT-6.1 Sol (Codex) |
#pragma once

#include <windows.h>
#include <objidl.h>
#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <new>
#include <utility>

class CFindTagStream final : public IStream
{
public:
    using ReadBlock = DWORD (*)(void*, ULONGLONG, void*, DWORD);

    static CFindTagStream* Create(ULONGLONG size, DWORD blockSize, ReadBlock read, void* context)
    {
        if (!blockSize || !read)
            return nullptr;
        try
        {
            return new CFindTagStream(std::make_shared<Source>(size, blockSize, read, context), 0);
        }
        catch (const std::bad_alloc&)
        {
            return nullptr;
        }
    }

    DWORD Detach()
    {
        std::lock_guard<std::mutex> lock(source->mutex);
        source->read = nullptr;
        source->context = nullptr;
        return source->error;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** value) override
    {
        if (!value)
            return E_POINTER;
        *value = nullptr;
        if (iid != IID_IUnknown && iid != IID_ISequentialStream && iid != IID_IStream)
            return E_NOINTERFACE;
        *value = static_cast<IStream*>(this);
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto remaining = --refs;
        if (!remaining)
            delete this;
        return remaining;
    }

    HRESULT STDMETHODCALLTYPE Read(void* data, ULONG bytes, ULONG* read) override
    {
        if (read)
            *read = 0;
        if (!data && bytes)
            return STG_E_INVALIDPOINTER;
        std::lock_guard<std::mutex> lock(source->mutex);
        if (!source->read)
            return STG_E_REVERTED;
        if (source->error)
            return HRESULT_FROM_WIN32(source->error);
        const auto available = position < source->size ? source->size - position : 0;
        const auto wanted = static_cast<ULONG>(std::min<ULONGLONG>(bytes, available));
        auto* out = static_cast<BYTE*>(data);
        ULONG count = 0;
        while (count < wanted)
        {
            const DWORD chunk = std::min<DWORD>(source->blockSize, wanted - count);
            if (chunk > source->remaining)
                source->error = ERROR_FILE_TOO_LARGE;
            else
                source->error = source->read(source->context, position, out + count, chunk);
            if (source->error)
                break;
            source->remaining -= chunk;
            count += chunk;
            position += chunk;
        }
        if (read)
            *read = count;
        return source->error ? HRESULT_FROM_WIN32(source->error) : count == bytes ? S_OK : S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER move, DWORD origin, ULARGE_INTEGER* updated) override
    {
        std::lock_guard<std::mutex> lock(source->mutex);
        ULONGLONG base = 0;
        switch (origin)
        {
        case STREAM_SEEK_SET: break;
        case STREAM_SEEK_CUR: base = position; break;
        case STREAM_SEEK_END: base = source->size; break;
        default: return STG_E_INVALIDFUNCTION;
        }
        if (move.QuadPart < 0)
        {
            const auto distance = 0ULL - static_cast<ULONGLONG>(move.QuadPart);
            if (distance > base)
                return STG_E_SEEKERROR;
            position = base - distance;
        }
        else
        {
            const auto distance = static_cast<ULONGLONG>(move.QuadPart);
            if (distance > MAXULONGLONG - base)
                return STG_E_SEEKERROR;
            position = base + distance;
        }
        if (updated)
            updated->QuadPart = position;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE CopyTo(IStream* output, ULARGE_INTEGER size, ULARGE_INTEGER* read, ULARGE_INTEGER* written) override
    {
        if (read)
            read->QuadPart = 0;
        if (written)
            written->QuadPart = 0;
        if (!output)
            return STG_E_INVALIDPOINTER;

        BYTE buffer[8192];
        ULONGLONG remaining = size.QuadPart;
        while (remaining)
        {
            const ULONG count = static_cast<ULONG>(std::min<ULONGLONG>(sizeof(buffer), remaining));
            ULONG got = 0;
            const HRESULT hr = Read(buffer, count, &got);
            if (read)
                read->QuadPart += got;
            if (FAILED(hr) || !got)
                return hr;
            ULONG put = 0;
            const HRESULT write = output->Write(buffer, got, &put);
            if (written)
                written->QuadPart += put;
            if (FAILED(write))
                return write;
            if (put != got)
                return STG_E_MEDIUMFULL;
            remaining -= got;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Stat(STATSTG* stat, DWORD flags) override
    {
        if (!stat)
            return STG_E_INVALIDPOINTER;
        if (flags & ~(STATFLAG_NONAME | STATFLAG_NOOPEN))
            return STG_E_INVALIDFLAG;
        *stat = {};
        stat->type = STGTY_STREAM;
        stat->cbSize.QuadPart = source->size;
        stat->grfMode = STGM_READ;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Clone(IStream** stream) override
    {
        if (!stream)
            return STG_E_INVALIDPOINTER;
        std::lock_guard<std::mutex> lock(source->mutex);
        *stream = nullptr;
        try
        {
            *stream = new CFindTagStream(source, position);
            return S_OK;
        }
        catch (const std::bad_alloc&)
        {
            return E_OUTOFMEMORY;
        }
    }

    HRESULT STDMETHODCALLTYPE Write(const void*, ULONG, ULONG* written) override
    {
        if (written)
            *written = 0;
        return STG_E_ACCESSDENIED;
    }
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER) override { return STG_E_ACCESSDENIED; }
    HRESULT STDMETHODCALLTYPE Commit(DWORD) override { return STG_E_ACCESSDENIED; }
    HRESULT STDMETHODCALLTYPE Revert() override { return STG_E_INVALIDFUNCTION; }
    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return STG_E_INVALIDFUNCTION; }
    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return STG_E_INVALIDFUNCTION; }

private:
    struct Source
    {
        Source(ULONGLONG length, DWORD block, ReadBlock callback, void* argument)
            : size(length), blockSize(block), read(callback), context(argument) {}
        const ULONGLONG size;
        const DWORD blockSize;
        ReadBlock read;
        void* context;
        DWORD error = ERROR_SUCCESS;
        DWORD remaining = 64 * 1024 * 1024;
        std::mutex mutex;
    };

    CFindTagStream(std::shared_ptr<Source> source, ULONGLONG position)
        : source(std::move(source)), position(position) {}
    std::shared_ptr<Source> source;
    ULONGLONG position;
    std::atomic<ULONG> refs{1};
};
