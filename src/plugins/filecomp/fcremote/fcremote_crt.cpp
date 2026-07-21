// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stddef.h>

extern "C" void* memcpy(void* dst, const void* src, size_t len);
#pragma function(memcpy)

extern "C" void* memcpy(void* dst, const void* src, size_t len)
{
    char* d = (char*)dst;
    const char* s = (const char*)src;
    while (len--)
        *d++ = *s++;
    return dst;
}
