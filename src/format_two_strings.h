// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Format two localized string arguments without printf. | Keep external strings out of format execution. | #731 | GPT-6 |
#pragma once
#include <string>

inline std::string FormatTwoStrings(const char* format, const char* first, const char* second)
{
    std::string result;
    unsigned argument = 0;
    for (const char* p = format; p && *p; ++p)
    {
        if (p[0] == '%' && p[1] == 's' && argument < 2)
        {
            result += argument++ == 0 ? first : second;
            ++p;
        }
        else if (p[0] == '%' && p[1] == '%')
        {
            result += '%';
            ++p;
        }
        else result += *p;
    }
    return result;
}
