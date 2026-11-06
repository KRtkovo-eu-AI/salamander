// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstddef>
#include <cstring>
#include <initializer_list>

namespace Salamander
{
namespace LocalizedText
{
// These messages contain narrow string inserts only. Do not pass translated
// resource text or inserted filenames to a printf-family format interpreter.
// Unknown directives, missing/extra arguments and insufficient storage fail
// without publishing a partial message. Caller storage must not alias inputs.
inline bool Measure(const char* pattern, std::initializer_list<const char*> values,
                    size_t& length) noexcept
{
    length = 0;
    if (pattern == nullptr) return false;
    auto next = values.begin();
    const size_t limit = static_cast<size_t>(-1) - 1; // retain room for NUL
    for (const char* p = pattern; *p != 0; ++p)
    {
        size_t count = 1;
        if (*p == '%')
        {
            ++p;
            if (*p == 's')
            {
                if (next == values.end() || *next == nullptr) return false;
                count = std::strlen(*next++);
            }
            else if (*p != '%') return false;
        }
        if (count > limit - length) return false;
        length += count;
    }
    return next == values.end();
}

inline bool Write(char* destination, size_t capacity, const char* pattern,
                  std::initializer_list<const char*> values) noexcept
{
    if (destination == nullptr || capacity == 0) return false;
    size_t length = 0;
    if (!Measure(pattern, values, length) || length >= capacity)
    {
        destination[0] = 0;
        return false;
    }
    const auto fail = [destination]() noexcept { destination[0] = 0; return false; };
    char* out = destination;
    auto next = values.begin();
    for (const char* p = pattern; *p != 0; ++p)
    {
        if (*p == '%')
        {
            ++p;
            if (*p == 's')
            {
                if (next == values.end() || *next == nullptr) return fail();
                const char* value = *next++;
                const size_t count = std::strlen(value);
                if (count >= capacity - static_cast<size_t>(out - destination)) return fail();
                memcpy(out, value, count);
                out += count;
            }
            else
            {
                if (*p != '%' || capacity - static_cast<size_t>(out - destination) <= 1) return fail();
                *out++ = '%';
            }
        }
        else
        {
            if (capacity - static_cast<size_t>(out - destination) <= 1) return fail();
            *out++ = *p;
        }
    }
    if (next != values.end()) return fail();
    *out = 0;
    return true;
}

inline bool Write(char* destination, size_t capacity, const char* pattern,
                  const char* first) noexcept
{
    return Write(destination, capacity, pattern, {first});
}

inline bool Write(char* destination, size_t capacity, const char* pattern,
                  const char* first, const char* second) noexcept
{
    return Write(destination, capacity, pattern, {first, second});
}
}
}
