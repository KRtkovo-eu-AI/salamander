// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <string>

// Policy and scheduling are independent of Win32 so tests can supply a clock
// and fake probes, including probes that never complete.
namespace DriveFreeSpaceDetail
{
constexpr std::uint64_t Lifetime = 60000;
constexpr std::uint64_t Deadline = 3000;
constexpr unsigned MaxProbes = 2;

struct Entry
{
    std::wstring Identity;
    std::uint64_t Token = 0;
    std::uint64_t Bytes = 0;
    std::uint64_t SuccessTime = 0;
    std::uint64_t NextAttempt = 0;
    std::uint64_t Started = 0;
    unsigned Mode = 0;
    bool Available = false;
    bool Attempted = false;
    bool Queued = false;
    bool Running = false;
    bool Stale = false;
    bool Failed = false;
};

class Cache
{
    Entry Entries[26];
    std::uint64_t Serial = 0;

public:
    const Entry& Get(unsigned drive) const { return Entries[drive]; }

    void Reset(unsigned drive)
    {
        Entries[drive] = Entry();
        Entries[drive].Token = ++Serial;
    }

    void Invalidate(int drive = -1)
    {
        for (unsigned i = 0; i < 26; ++i)
            if (drive < 0 || static_cast<unsigned>(drive) == i)
                Reset(i);
    }

    void MarkStale(int drive = -1)
    {
        for (unsigned i = 0; i < 26; ++i)
            if (drive < 0 || static_cast<unsigned>(drive) == i)
            {
                Entries[i].Stale = Entries[i].Available;
                // A failed request retains its retry backoff even if a file
                // operation or repeated panel refresh marks the display stale.
                if (Entries[i].Available && !Entries[i].Failed)
                    Entries[i].NextAttempt = 0;
            }
    }

    const Entry& Query(unsigned drive, const std::wstring& identity,
                       unsigned mode, std::uint64_t now, bool disconnected)
    {
        Entry& e = Entries[drive];
        if (mode == 0)
        {
            if (e.Mode != 0)
                Reset(drive);
            return e;
        }
        if (e.Identity != identity)
        {
            Reset(drive);
            e.Identity = identity;
        }
        e.Mode = mode;
        if (disconnected)
        {
            e.Stale = e.Available;
            if (e.Queued || e.Running)
            {
                e.Token = ++Serial;
                e.Queued = e.Running = false;
                e.Failed = true;
                e.NextAttempt = now + Lifetime;
            }
        }
        if (mode == 1 || (e.Available && now >= e.NextAttempt))
            e.Stale = e.Available;
        if (!e.Queued && !e.Running && (!e.Attempted || (mode == 2 && now >= e.NextAttempt)))
        {
            e.Attempted = true; // Once consumes failures as well as successes.
            e.Token = ++Serial;
            if (disconnected)
            {
                e.Stale = e.Available;
                e.Failed = true;
                e.NextAttempt = now + Lifetime;
            }
            else
                e.Queued = true;
        }
        return e;
    }

    int Reserve(std::uint64_t now)
    {
        unsigned running = 0;
        for (const Entry& e : Entries)
            running += e.Running ? 1 : 0;
        if (running >= MaxProbes)
            return -1;
        for (unsigned i = 0; i < 26; ++i)
            if (Entries[i].Queued)
            {
                Entries[i].Queued = false;
                Entries[i].Running = true;
                Entries[i].Started = now;
                return static_cast<int>(i);
            }
        return -1;
    }

    bool IsCurrent(unsigned drive, std::uint64_t token) const
    {
        return Entries[drive].Running && Entries[drive].Token == token;
    }

    bool Expired(unsigned drive, std::uint64_t token, std::uint64_t now) const
    {
        return IsCurrent(drive, token) && now - Entries[drive].Started >= Deadline;
    }

    bool Complete(unsigned drive, std::uint64_t token, bool success,
                  std::uint64_t bytes, std::uint64_t now, std::uint64_t wallTime)
    {
        if (!IsCurrent(drive, token))
            return false;
        Entry& e = Entries[drive];
        e.Running = false;
        e.NextAttempt = now + Lifetime;
        e.Failed = !success;
        if (success)
        {
            e.Available = true;
            e.Bytes = bytes;
            e.SuccessTime = wallTime;
            e.Stale = e.Mode == 1;
        }
        else
            e.Stale = e.Available;
        return true;
    }
};
}
