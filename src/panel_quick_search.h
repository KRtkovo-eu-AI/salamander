// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

namespace Salamander::Panel
{

struct QuickSearchCaretTextRange
{
    std::size_t Start = 0;
    std::size_t Length = 0;
};

// Returns the part of the matched file-name prefix that is rendered in the
// column containing the quick-search caret.  In Detailed mode the extension
// can be displayed separately, so characters from the Name column must not be
// measured again after the caret moves to the Ext column.
inline QuickSearchCaretTextRange GetQuickSearchCaretTextRange(
    const std::wstring& fileName, std::size_t matchedLength, bool extensionInSeparateColumn)
{
    matchedLength = (std::min)(matchedLength, fileName.length());
    QuickSearchCaretTextRange range = {0, matchedLength};
    if (!extensionInSeparateColumn)
        return range;

    std::size_t dot = fileName.find_last_of(L'.');
    if (dot == std::wstring::npos || dot == 0)
        return range;

    std::size_t extensionStart = dot + 1;
    if (matchedLength >= extensionStart)
    {
        range.Start = extensionStart;
        range.Length = matchedLength - extensionStart;
    }
    return range;
}

} // namespace Salamander::Panel
