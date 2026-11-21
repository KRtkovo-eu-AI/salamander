// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-21 | Add opt-in anonymous viewer phase timings. | Measure navigation before introducing prefetch. | | GPT-6 (Codex) |

#pragma once

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

namespace PictView::Perf
{
enum class Phase { Other, Navigation, Open, Decode, Exif, Transform, Scale, Paint, Count };
enum class Counter { Decode, Scale, Transform, Draw, Count };

// Read once per process. The caller must supply an absolute CSV path in an
// existing directory. Never collect image names, paths, hashes or EXIF fields.
struct Configuration
{
    wchar_t* path = nullptr;
    LARGE_INTEGER frequency{};
    Configuration()
    {
        const DWORD length = GetEnvironmentVariableW(L"PICTVIEW_PERF_LOG", nullptr, 0);
        if (length < 4 || length > 32760 || !QueryPerformanceFrequency(&frequency))
            return;
        wchar_t* buffer = static_cast<wchar_t*>(HeapAlloc(GetProcessHeap(), 0, (static_cast<size_t>(length) + 8) * sizeof(wchar_t)));
        if (!buffer)
            return;
        const DWORD copied = GetEnvironmentVariableW(L"PICTVIEW_PERF_LOG", buffer, length);
        if (copied == 0 || copied >= length ||
            !((buffer[0] && buffer[1] == L':' && buffer[2] == L'\\') || (buffer[0] == L'\\' && buffer[1] == L'\\')))
        {
            HeapFree(GetProcessHeap(), 0, buffer);
            return;
        }
        if (copied >= MAX_PATH && wcsncmp(buffer, L"\\\\?\\", 4) != 0)
        {
            if (buffer[0] == L'\\' && buffer[1] == L'\\')
            {
                memmove(buffer + 8, buffer + 2, (static_cast<size_t>(copied) - 1) * sizeof(wchar_t));
                memcpy(buffer, L"\\\\?\\UNC\\", 8 * sizeof(wchar_t));
            }
            else
            {
                memmove(buffer + 4, buffer, (static_cast<size_t>(copied) + 1) * sizeof(wchar_t));
                memcpy(buffer, L"\\\\?\\", 4 * sizeof(wchar_t));
            }
        }
        path = buffer;
    }
    ~Configuration() { if (path) HeapFree(GetProcessHeap(), 0, path); }
    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;
};

inline const Configuration& Config() { static const Configuration config; return config; }
inline LONGLONG Now() { LARGE_INTEGER value{}; QueryPerformanceCounter(&value); return value.QuadPart; }

struct Sample
{
    void* owner = nullptr;
    ULONG_PTR serial = 0;
    bool active = false;
    bool opened = false;
    const void* expectedHandle = nullptr;
    const char* action = "open";
    Phase phase = Phase::Other;
    LONGLONG started = 0, checkpoint = 0;
    LONGLONG ticks[static_cast<int>(Phase::Count)]{};
    unsigned attempts[static_cast<int>(Counter::Count)]{};
    unsigned paintCalls = 0;
    unsigned pendingTransforms = 0;
    UINT sourceWidth = 0, sourceHeight = 0, displayWidth = 0, displayHeight = 0;
    ULONGLONG fileBytes = 0;
};

// Each viewer owns a UI thread. Inline-local TLS is shared across translation
// units; thumbnail threads have no active sample and cannot pollute the log.
inline Sample& Current() { static thread_local Sample sample; return sample; }
inline void Checkpoint(Sample& sample, LONGLONG now)
{
    sample.ticks[static_cast<int>(sample.phase)] += now - sample.checkpoint;
    sample.checkpoint = now;
}
inline bool Active(void* owner) { const auto& s = Current(); return s.active && s.owner == owner; }
inline ULONG_PTR Serial(void* owner) { return Active(owner) ? Current().serial : 0; }

class Scope
{
    ULONG_PTR serial = 0;
    Phase previous = Phase::Other;
public:
    explicit Scope(Phase phase)
    {
        auto& s = Current();
        if (s.active)
        {
            serial = s.serial;
            previous = s.phase;
            Checkpoint(s, Now());
            s.phase = phase;
        }
    }
    ~Scope()
    {
        auto& s = Current();
        if (serial && s.active && s.serial == serial)
        {
            Checkpoint(s, Now());
            s.phase = previous;
        }
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
};

inline void Count(Counter counter) { auto& s = Current(); if (s.active) ++s.attempts[static_cast<int>(counter)]; }
inline void SetImageSize(UINT w, UINT h) { auto& s = Current(); if (s.active) { s.sourceWidth = w; s.sourceHeight = h; } }
inline void SetDisplaySize(UINT w, UINT h) { auto& s = Current(); if (s.active) { s.displayWidth = w; s.displayHeight = h; } }
inline void SetFileBytes(ULONGLONG bytes) { if (Current().active) Current().fileBytes = bytes; }

inline void Finish(void* owner, const char* outcome, bool flushGdi = false)
{
    auto& s = Current();
    if (!Active(owner))
        return;
    if (flushGdi)
    {
        Scope paint(Phase::Paint);
        if (!GdiFlush()) // Diagnostic runs only; not a DWM/vsync or monitor timestamp.
            outcome = "gdi_flush_failed";
    }
    const LONGLONG ended = Now();
    Checkpoint(s, ended);
    s.active = false; // File I/O and formatting are outside the measured interval.
    const double ms = 1000.0 / static_cast<double>(Config().frequency.QuadPart);
    char row[1024];
    const int length = _snprintf_s(row, sizeof(row), _TRUNCATE,
        "%lu,%lu,%llu,%s,%s,%llu,%u,%u,%u,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u\r\n",
        GetCurrentProcessId(), GetCurrentThreadId(), static_cast<unsigned long long>(s.serial), s.action, outcome,
        static_cast<unsigned long long>(s.fileBytes), s.sourceWidth, s.sourceHeight, s.displayWidth, s.displayHeight,
        (ended - s.started) * ms,
        s.ticks[static_cast<int>(Phase::Navigation)] * ms, s.ticks[static_cast<int>(Phase::Open)] * ms,
        s.ticks[static_cast<int>(Phase::Decode)] * ms, s.ticks[static_cast<int>(Phase::Exif)] * ms,
        s.ticks[static_cast<int>(Phase::Transform)] * ms, s.ticks[static_cast<int>(Phase::Scale)] * ms,
        s.ticks[static_cast<int>(Phase::Paint)] * ms, s.ticks[static_cast<int>(Phase::Other)] * ms,
        s.attempts[0], s.attempts[1], s.attempts[2], s.attempts[3], s.paintCalls);
    if (length <= 0)
        return;
    static SRWLOCK lock = SRWLOCK_INIT;
    AcquireSRWLockExclusive(&lock);
    HANDLE output = CreateFileW(Config().path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size{};
        DWORD written = 0;
        if (GetFileSizeEx(output, &size) && size.QuadPart == 0)
        {
            static const char header[] = "pid,tid,sample,action,outcome,file_bytes,width,height,display_width,display_height,total_ms,navigation_ms,open_other_ms,decode_ms,exif_ms,transform_ms,scale_ms,paint_ms,other_ms,decode_attempts,scale_attempts,transform_attempts,draw_attempts,paint_calls\r\n";
            WriteFile(output, header, static_cast<DWORD>(sizeof(header) - 1), &written, nullptr);
        }
        WriteFile(output, row, static_cast<DWORD>(length), &written, nullptr);
        CloseHandle(output);
    }
    else
        OutputDebugStringA("PictView performance log could not be opened.\n");
    ReleaseSRWLockExclusive(&lock);
}

inline void Begin(void* owner, const char* action)
{
    if (!Config().path)
        return;
    auto& s = Current();
    if (s.active)
        Finish(s.owner, "superseded");
    ULONG_PTR serial = s.serial + 1;
    if (serial == 0) serial = 1;
    s = Sample{};
    s.owner = owner;
    s.serial = serial;
    s.active = true;
    s.action = action;
    s.started = s.checkpoint = Now();
}

inline void BeginOpen(void* owner)
{
    if (!Active(owner) || Current().opened)
        Begin(owner, "open");
    if (Active(owner)) Current().opened = true;
}

inline LPARAM PendingTransforms(void* owner, unsigned count)
{
    if (!Active(owner)) return 0;
    Current().pendingTransforms = count;
    return static_cast<LPARAM>(Current().serial);
}

class AutoTransform
{
    void* owner;
    ULONG_PTR serial;
public:
    AutoTransform(void* owner, LPARAM cookie) : owner(owner), serial(0)
    {
        if (Active(owner) && cookie != 0 && static_cast<ULONG_PTR>(cookie) == Current().serial && Current().pendingTransforms)
            serial = Current().serial;
    }
    ~AutoTransform()
    {
        if (serial && Active(owner) && Current().serial == serial && Current().pendingTransforms)
            --Current().pendingTransforms;
    }
};

inline void ArmPaint(void* owner, const void* handle)
{
    if (Active(owner)) Current().expectedHandle = handle;
}

inline void Painted(void* owner, ULONG_PTR serial, const void* handle, bool success)
{
    if (serial && Active(owner) && Current().serial == serial && success &&
        handle && Current().expectedHandle == handle && !Current().pendingTransforms)
        Finish(owner, "painted", true);
}
}
