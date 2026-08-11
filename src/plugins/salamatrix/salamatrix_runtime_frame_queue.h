// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <deque>
#include <string>

#include "salamatrix_runtime_api.h"

namespace Salamatrix
{
namespace Runtime
{

// A bounded, non-blocking producer queue for frames emitted by host event
// callbacks. Pipe writes happen only on the private writer thread, so a slow
// or re-entrant worker cannot stall Salamander's UI thread.
class RuntimeFrameQueue
{
private:
    enum
    {
        MaxFrames = 128,
        MaxBytes = Protocol::MaxFrameBytes
    };

    CRITICAL_SECTION Lock;
    HANDLE WakeEvent;
    HANDLE Thread;
    IRuntimeSession* Session;
    BOOL Stopping;
    size_t QueuedBytes;
    std::deque<std::string> Frames;

    RuntimeFrameQueue(const RuntimeFrameQueue&);
    RuntimeFrameQueue& operator=(const RuntimeFrameQueue&);

    static DWORD WINAPI ThreadProc(void* context)
    {
        RuntimeFrameQueue* queue =
            static_cast<RuntimeFrameQueue*>(context);
        return queue != NULL ? queue->Run() : 1;
    }

    DWORD Run()
    {
        for (;;)
        {
            WaitForSingleObject(WakeEvent, INFINITE);
            for (;;)
            {
                std::string frame;
                BOOL stopping = FALSE;
                EnterCriticalSection(&Lock);
                stopping = Stopping;
                if (!Frames.empty())
                {
                    frame.swap(Frames.front());
                    Frames.pop_front();
                    QueuedBytes -= frame.size();
                }
                LeaveCriticalSection(&Lock);

                if (frame.empty())
                {
                    if (stopping)
                        return 0;
                    break;
                }
                if (Session != NULL)
                    Session->SendFrame(
                        frame.c_str(), static_cast<DWORD>(frame.size()));
            }
        }
    }

public:
    RuntimeFrameQueue()
        : WakeEvent(NULL),
          Thread(NULL),
          Session(NULL),
          Stopping(FALSE),
          QueuedBytes(0)
    {
        InitializeCriticalSection(&Lock);
    }

    ~RuntimeFrameQueue()
    {
        Shutdown();
        DeleteCriticalSection(&Lock);
    }

    BOOL Start(IRuntimeSession* session)
    {
        if (session == NULL || Thread != NULL)
            return FALSE;
        Stopping = FALSE;
        Session = session;
        WakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (WakeEvent == NULL)
        {
            Session = NULL;
            return FALSE;
        }
        Thread = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
        if (Thread == NULL)
        {
            CloseHandle(WakeEvent);
            WakeEvent = NULL;
            Session = NULL;
            return FALSE;
        }
        return TRUE;
    }

    BOOL Queue(const char* bytes, DWORD count)
    {
        if (bytes == NULL || count == 0 || count > Protocol::MaxFrameBytes)
            return FALSE;
        EnterCriticalSection(&Lock);
        if (Thread == NULL || Stopping || Frames.size() >= MaxFrames ||
            QueuedBytes + count > MaxBytes)
        {
            LeaveCriticalSection(&Lock);
            return FALSE;
        }
        Frames.push_back(std::string(bytes, bytes + count));
        QueuedBytes += count;
        SetEvent(WakeEvent);
        LeaveCriticalSection(&Lock);
        return TRUE;
    }

    void Shutdown()
    {
        EnterCriticalSection(&Lock);
        HANDLE thread = Thread;
        if (thread != NULL)
        {
            Stopping = TRUE;
            Frames.clear();
            QueuedBytes = 0;
            SetEvent(WakeEvent);
        }
        LeaveCriticalSection(&Lock);
        if (thread != NULL)
        {
            // The writer can be inside a synchronous pipe WriteFile. Cancel it
            // before waiting so session shutdown cannot deadlock while trying
            // to acquire the session's write lock.
            CancelSynchronousIo(thread);
            // Once cancellation has been requested, the object cannot be
            // destroyed until the writer has really left: closing its handle
            // after a timeout would let it access a deleted critical section.
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
            EnterCriticalSection(&Lock);
            Thread = NULL;
            LeaveCriticalSection(&Lock);
        }
        if (WakeEvent != NULL)
        {
            CloseHandle(WakeEvent);
            WakeEvent = NULL;
        }
        Session = NULL;
    }
};

} // namespace Runtime
} // namespace Salamatrix
