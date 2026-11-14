// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Measure partial directory identity tracking memory. | Report growth independently of bounded wire page size. | #731 | GPT-6.1 Terra |
#include "../branch_view.h"
#include <psapi.h>
#include <cstdio>
using namespace Salamander::BranchView;
static SIZE_T PrivateBytes()
{
    PROCESS_MEMORY_COUNTERS_EX memory{}; memory.cb = sizeof(memory);
    return GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)) ? memory.PrivateUsage : 0;
}
int wmain()
{
    DeniedDirectory parent; parent.Verified = true; parent.Volume = 1; parent.FileId[0] = 2;
    PartialDirectoryRows rows(parent);
    const SIZE_T baseline = PrivateBytes();
    if (!baseline) return 2;
    const ULONGLONG start = GetTickCount64();
    for (unsigned i = 0; i != 1000000; ++i)
    {
        WIN32_FIND_DATAW row{}; swprintf_s(row.cFileName, L"file-%010u-unicode-東.txt", i);
        row.dwFileAttributes = FILE_ATTRIBUTE_NORMAL; BYTE id[16]{};
        CopyMemory(id, &i, sizeof(i));
        if (!rows.ObserveOriginal(row, id)) return 3;
        if (i + 1 == 10000 || i + 1 == 100000 || i + 1 == 500000 || i + 1 == 1000000)
        {
            const SIZE_T current = PrivateBytes();
            if (!current || current < baseline) return 4;
            std::printf("rows=%u private_delta_bytes=%llu elapsed_ms=%llu\n", i+1,
                        static_cast<unsigned long long>(current-baseline),
                        static_cast<unsigned long long>(GetTickCount64()-start));
        }
    }
    if (!rows.BeginRetry(parent.Volume, parent.FileId)) return 5;
    const SIZE_T withIndex = PrivateBytes();
    if (withIndex < baseline) return 6;
    std::printf("rows=1000000 retry_index_private_delta_bytes=%llu elapsed_ms=%llu\n",
                static_cast<unsigned long long>(withIndex-baseline),
                static_cast<unsigned long long>(GetTickCount64()-start));
    return 0;
}
