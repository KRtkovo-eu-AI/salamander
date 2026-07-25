// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>

// The SDK's debug headers redirect memcpy to this symbol when TRACE_ENABLE is
// active.  Runtime provider plug-ins intentionally do not link the full
// debugger object (which expects Salamander's process-wide debug singleton),
// but still need the overlap-checking entry point to keep their debug builds
// linkable.
extern "C" void* _sal_safe_memcpy(void* destination,
                                  const void* source,
                                  size_t count)
{
    return std::memcpy(destination, source, count);
}
