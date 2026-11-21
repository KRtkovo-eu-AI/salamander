// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-21 | Estimate compact opaque cache entries. | Match the raster retained after dropping the duplicate GDI bitmap. | | gpt-6.2-sol |
// | 2026-11-21 | Define five prefetch budgets and matching estimates. | Keep the Performance page and worker limits in agreement. | | gpt-6.2-sol |

#pragma once

#include <cstdint>

namespace PictView::PrefetchBudget
{
inline constexpr uint64_t MiB = 1024 * 1024;
inline constexpr unsigned Levels = 5;
inline constexpr unsigned DefaultLevel = 1;

struct Limit
{
    unsigned MemoryMiB;
    unsigned PerDirection;
    uint64_t Bytes() const { return MemoryMiB * MiB; }
};

inline Limit At(unsigned level)
{
    static constexpr Limit limits[Levels] = {
#ifdef _WIN64
        {256, 1}, {512, 2}, {1024, 4}, {1536, 6}, {2048, 8}
#else
        {128, 1}, {256, 2}, {384, 3}, {512, 4}, {768, 6}
#endif
    };
    return limits[level < Levels ? level : DefaultLevel];
}

// Allow for full raster copies, a temporary rotation/conversion, scaled
// pixels and the EXIF input. Codec-internal memory is implementation-dependent.
inline uint64_t WorkingBytes(uint64_t pixels, uint64_t displayPixels, uint64_t fileBytes)
{
    return 20 * pixels + 12 * displayPixels + 2 * fileBytes + MiB;
}

inline unsigned Estimate(Limit limit, unsigned megapixels)
{
    const uint64_t pixels = megapixels * UINT64_C(1000000);
    // Typical opaque raster with a 1 MP display bitmap and encoded bytes
    // around 1/4 byte per source pixel (for example ~6 MB at 24 MP).
    const auto working = WorkingBytes(pixels, 1000000, pixels / 4);
    const auto retained = 4 * pixels + 8 * UINT64_C(1000000) + MiB;
    if (working > limit.Bytes())
        return 0;
    const auto count = 1 + (limit.Bytes() - working) / retained;
    const unsigned cap = 2 * limit.PerDirection;
    return count < cap ? static_cast<unsigned>(count) : cap;
}
}
