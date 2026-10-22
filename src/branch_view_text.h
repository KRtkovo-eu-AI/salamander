// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <cstdint>
#include <string>

namespace Salamander { namespace BranchViewText {
namespace Detail {

// Resource strings may expand only two ordered, exactly typed placeholders.
// Unknown or extra directives remain literal; inserted values are never parsed.
inline std::string ExpandTwo(const char* pattern, const char* token,
                             const std::string& first, const std::string& second)
{
    if (pattern == nullptr)
        return std::string();
    const std::string format(pattern);
    const std::size_t tokenLength = std::char_traits<char>::length(token);
    const std::string* values[] = {&first, &second};
    unsigned int argument = 0;
    std::string text;
    text.reserve(format.size());
    for (std::size_t i = 0; i < format.size();)
    {
        if (format[i] == '%' && i + 1 < format.size() && format[i + 1] == '%')
        {
            text += '%';
            i += 2;
        }
        else if (argument < 2 && format.compare(i, tokenLength, token) == 0)
        {
            text += *values[argument++];
            i += tokenLength;
        }
        else
            text += format[i++];
    }
    // Keep complete UTF-8 without a fixed-size intermediate or truncation.
    return text;
}

} // namespace Detail

inline std::string FormatStatus(const char* pattern, std::uint64_t files,
                                std::uint64_t errorsOrPrevious)
{
    return Detail::ExpandTwo(pattern, "%I64u", std::to_string(files),
                             std::to_string(errorsOrPrevious));
}

inline std::string FormatCompressedSizeError(const char* pattern, const char* path,
                                             const char* error)
{
    return Detail::ExpandTwo(pattern, "%s", path != nullptr ? path : "",
                             error != nullptr ? error : "");
}

} } // namespace Salamander::BranchViewText
