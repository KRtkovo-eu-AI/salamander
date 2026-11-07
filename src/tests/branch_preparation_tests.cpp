// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../branch_view.h"
#include <cstdio>

using Salamander::BranchView::AssociationPreparation;
static int Checks = 0;
static int Failures = 0;
static void Check(bool ok, const char* text)
{
    ++Checks;
    printf("%s %s\n", ok ? "PASS" : "FAIL", text);
    if (!ok) ++Failures;
}
static void AddExtensions(AssociationPreparation& preparation, int count)
{
    for (int i = 0; i < count; ++i) preparation.Add("extension" + std::to_string(i));
}
int main()
{
    ULONGLONG now = 1000;
    auto clock = [&]() { return now; };
    AssociationPreparation empty;
    int calls = 0;
    empty.RunSlice(clock, [&](const std::string&) { ++calls; return true; });
    Check(!empty.HasPending() && calls == 0, "empty preparation never calls the shell resolver");

    AssociationPreparation dedup;
    for (int i = 0; i < 10000; ++i) { dedup.Add("png"); dedup.Add("txt"); }
    std::vector<std::string> resolved;
    dedup.RunSlice(clock, [&](const std::string& extension) { resolved.push_back(extension); return true; });
    Check(resolved == std::vector<std::string>({"png", "txt"}) && !dedup.HasPending(),
          "repeated rows resolve each extension once in insertion order");
    dedup.Add("png");
    Check(!dedup.HasPending(), "already completed extensions remain deduplicated");

    AssociationPreparation budget;
    AddExtensions(budget, 10);
    calls = 0;
    budget.RunSlice(clock, [&](const std::string&) { ++calls; now += 3; return true; });
    Check(calls == 3 && budget.HasPending(), "8 ms default budget yields after three 3 ms queries");
    while (budget.HasPending()) budget.RunSlice(clock, [&](const std::string&) { ++calls; now += 3; return true; });
    Check(calls == 10, "time-sliced resume neither skips nor repeats completed queries");

    AssociationPreparation slow;
    AddExtensions(slow, 2);
    calls = 0;
    slow.RunSlice(clock, [&](const std::string&) { ++calls; now += 100; return true; });
    Check(calls == 1 && slow.HasPending(), "one slow query yields before starting another query");

    AssociationPreparation capped;
    AddExtensions(capped, 130);
    calls = 0;
    capped.RunSlice(clock, [&](const std::string&) { ++calls; return true; });
    Check(calls == 64 && capped.HasPending(), "64-query cap bounds a slice even with a stationary clock");
    capped.RunSlice(clock, [&](const std::string&) { ++calls; return true; }, 8, 3);
    Check(calls == 67 && capped.HasPending(), "explicit query cap is honored when resuming");
    while (capped.HasPending()) capped.RunSlice(clock, [&](const std::string&) { ++calls; return true; });
    Check(calls == 130, "count-limited slices eventually finish the complete collection");

    AssociationPreparation stopped;
    stopped.Add("first"); stopped.Add("second");
    calls = 0;
    stopped.RunSlice(clock, [&](const std::string&) { ++calls; return false; });
    Check(calls == 1 && stopped.HasPending(), "resolver cancellation stops immediately without losing pending work");
    resolved.clear();
    stopped.RunSlice(clock, [&](const std::string& extension) { resolved.push_back(extension); return true; });
    Check(resolved == std::vector<std::string>({"first", "second"}) && !stopped.HasPending(),
          "cancelled query can be retried without skipping its extension");

    AssociationPreparation cleared;
    cleared.Add(std::string(200, 'x')); cleared.Add("old-tail");
    calls = 0;
    bool copiedArgumentSurvived = false;
    cleared.RunSlice(clock, [&](const std::string& extension) {
        ++calls;
        cleared.Clear();
        copiedArgumentSurvived = extension == std::string(200, 'x');
        cleared.Add("replacement");
        return true;
    });
    Check(calls == 1 && copiedArgumentSurvived && cleared.HasPending(),
          "clear-and-replace during callback preserves its copied argument and stops the old slice");
    resolved.clear();
    cleared.RunSlice(clock, [&](const std::string& extension) { resolved.push_back(extension); return true; });
    Check(resolved == std::vector<std::string>({"replacement"}) && !cleared.HasPending(),
          "replacement preparation starts at its first item after old callback returns");

    AssociationPreparation restarted;
    restarted.Add("a"); restarted.Add("b"); restarted.Add("c");
    restarted.RunSlice(clock, [](const std::string&) { return true; }, 8, 1);
    calls = 0;
    restarted.RunSlice(clock, [&](const std::string& extension) {
        ++calls;
        Check(extension == "b", "restart exercise reaches the original second item");
        restarted.Restart();
        return true;
    });
    Check(calls == 1 && restarted.HasPending(), "restart during callback invalidates the active slice");
    resolved.clear();
    restarted.RunSlice(clock, [&](const std::string& extension) { resolved.push_back(extension); return true; });
    Check(resolved == std::vector<std::string>({"a", "b", "c"}) && !restarted.HasPending(),
          "invalidated association generation re-resolves every extension from the beginning");
    restarted.Restart();
    restarted.Clear();
    Check(!restarted.HasPending(), "clear cancels restarted preparation and releases pending work");

    printf("%d checks, %d failures\n", Checks, Failures);
    return Failures == 0 ? 0 : 1;
}
