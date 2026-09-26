// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <string>

namespace Salamander { namespace DriveFreeSpaceText {

// Localized tooltip templates contain two ordered %s placeholders. Expand only
// those text placeholders; never treat resource text as a printf format string.
// Unsupported directives and excess placeholders remain literal. Values are
// copied verbatim, so percent characters in a value are not interpreted again.
inline std::string FormatDescription(const char* pattern, const char* size,
                                      const char* timestamp)
{
    if (pattern == NULL)
        return std::string();
    const char* values[] = {size != NULL ? size : "", timestamp != NULL ? timestamp : ""};
    unsigned int argument = 0;
    std::string description;
    for (const char* current = pattern; *current != 0; ++current)
    {
        if (current[0] == '%' && current[1] == '%')
        {
            description += '%';
            ++current;
        }
        else if (current[0] == '%' && current[1] == 's' && argument < 2)
        {
            description += values[argument++];
            ++current;
        }
        else
            description += *current;
    }
    // Keep complete UTF-8 here; the tooltip's bounded copy truncates safely.
    return description;
}

} }
