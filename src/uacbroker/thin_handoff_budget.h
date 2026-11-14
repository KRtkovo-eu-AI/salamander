// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Bound concurrent and uncertain thin handle deliveries. | Prevent repeated lost replies from leaking unbounded host handles. | #731 | gpt-6.1-terra |
#pragma once
#include <atomic>
inline constexpr unsigned thinHandoffProcessLimit = 256;

// A reservation covers one possible remote duplicate. A delivered or known
// failed request releases it; an uncertain result consumes it until process
// exit, because its numeric handle is unknown and cannot safely be closed.
class ThinHandoffBudget
{
public:
    explicit ThinHandoffBudget(unsigned limit) noexcept : limit_(limit) {}
    bool Acquire() noexcept
    {
        unsigned value = used_.load(std::memory_order_relaxed);
        while (value < limit_)
            if (used_.compare_exchange_weak(value, value + 1, std::memory_order_acquire,
                                            std::memory_order_relaxed)) return true;
        return false;
    }
    void Release() noexcept { used_.fetch_sub(1, std::memory_order_release); }
private:
    const unsigned limit_;
    std::atomic<unsigned> used_{0};
};

class ThinHandoffReservation
{
public:
    ThinHandoffReservation(ThinHandoffBudget& budget, bool needed, const bool& uncertain) noexcept
        : budget_(budget), uncertain_(uncertain), needed_(needed), acquired_(!needed || budget.Acquire()) {}
    ~ThinHandoffReservation() { if (needed_ && acquired_ && !uncertain_) budget_.Release(); }
    ThinHandoffReservation(const ThinHandoffReservation&) = delete;
    ThinHandoffReservation& operator=(const ThinHandoffReservation&) = delete;
    bool Acquired() const noexcept { return acquired_; }
private:
    ThinHandoffBudget& budget_;
    const bool& uncertain_;
    bool needed_, acquired_;
};
