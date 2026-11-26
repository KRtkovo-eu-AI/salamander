// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Exercise concurrent and uncertain handoff reservations. | Bound lost handles without retaining slots for known outcomes. | #731 | gpt-6.1-terra |
#include "../uacbroker/thin_handoff_budget.h"
#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

static int failures = 0;
static void Check(bool ok, const char* name)
{
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok) ++failures;
}

int main()
{
    ThinHandoffBudget budget(3);
    bool known = false, unknownA = false, unknownB = false, unknownC = false;
    {
        ThinHandoffReservation a(budget, true, unknownA), b(budget, true, unknownB), c(budget, true, known);
        Check(a.Acquired() && b.Acquired() && c.Acquired(), "reserve concurrent possible deliveries");
        ThinHandoffReservation blocked(budget, true, known), control(budget, false, known);
        Check(!blocked.Acquired() && control.Acquired(), "capacity blocks new deliveries but not control requests");
        unknownA = true; unknownB = true;
    }
    {
        ThinHandoffReservation c(budget, true, unknownC);
        Check(c.Acquired(), "known outcome returns its reservation");
        unknownC = true;
    }
    ThinHandoffReservation afterLoss(budget, true, known), control(budget, false, known);
    Check(!afterLoss.Acquired() && control.Acquired(), "three uncertain outcomes remain bounded");
    ThinHandoffBudget processBudget(thinHandoffProcessLimit);
    unsigned acquired = 0;
    while (acquired < thinHandoffProcessLimit && processBudget.Acquire()) ++acquired;
    Check(acquired == thinHandoffProcessLimit && !processBudget.Acquire(), "process limit bounds reservations at its configured value");
    while (acquired) { processBudget.Release(); --acquired; }
    Check(processBudget.Acquire(), "process capacity returns after known outcomes");
    processBudget.Release();
    ThinHandoffBudget parallel(3);
    std::atomic<unsigned> live{0}, peak{0};
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 12; ++i)
        threads.emplace_back([&] {
            for (unsigned n = 0; n < 1000; ++n)
            {
                bool uncertain = false;
                ThinHandoffReservation slot(parallel, true, uncertain);
                if (!slot.Acquired()) { std::this_thread::yield(); continue; }
                const unsigned current = live.fetch_add(1) + 1;
                unsigned old = peak.load();
                while (old < current && !peak.compare_exchange_weak(old, current)) {}
                std::this_thread::yield();
                live.fetch_sub(1);
            }
        });
    for (auto& thread : threads) thread.join();
    bool clean = false;
    ThinHandoffReservation p(parallel, true, clean), q(parallel, true, clean), r(parallel, true, clean), s(parallel, true, clean);
    Check(peak.load() <= 3 && !live.load() && p.Acquired() && q.Acquired() && r.Acquired() && !s.Acquired(),
          "concurrent known outcomes never exceed or leak capacity");
    return failures ? 1 : 0;
}
