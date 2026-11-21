// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-21 | Compare foreground misses with and without prefetch work. | Warn only when pausing prefetch measurably improves comparable opens. | | gpt-6.2-sol |

#pragma once

#include <cstdint>

namespace PictView::PrefetchHealth
{
enum class Activity { Quiet, SameDrive, Unclear };

struct Observation
{
    unsigned drive;
    unsigned format;
    std::uint64_t pixels;
    std::uint64_t bytes;
    std::uint64_t milliseconds;
    Activity activity;
};

class Monitor
{
    static constexpr unsigned RequiredMisses = 3;
    Observation First{};
    std::uint64_t FastestBusyMs = 0;
    unsigned SlowMisses = 0;
    unsigned UnclearProbes = 0;

    static bool Similar(std::uint64_t a, std::uint64_t b)
    {
        return a < b ? b - a <= a / 2 : a - b <= b / 2;
    }

public:
    bool NeedsQuietOpen() const { return SlowMisses == RequiredMisses; }

    bool Observe(const Observation& open)
    {
        const bool comparable = SlowMisses && open.drive == First.drive && open.format == First.format &&
                                Similar(open.pixels, First.pixels) && Similar(open.bytes, First.bytes);
        if (NeedsQuietOpen() && comparable)
        {
            if (open.activity == Activity::Quiet)
            {
                const bool faster = open.milliseconds <= FastestBusyMs / 2;
                *this = {};
                return faster;
            }
            if (++UnclearProbes < 2)
                return false;
            *this = {};
            return false;
        }

        constexpr std::uint64_t slowOpenMs = 750;
        if (open.activity != Activity::SameDrive || open.milliseconds < slowOpenMs)
        {
            *this = {};
            return false;
        }
        if (!comparable)
        {
            First = open;
            SlowMisses = 0;
            FastestBusyMs = open.milliseconds;
        }
        if (open.milliseconds < FastestBusyMs)
            FastestBusyMs = open.milliseconds;
        ++SlowMisses;
        return false;
    }
};
}
