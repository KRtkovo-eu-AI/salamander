// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

namespace Salamander::Panel
{

// A panel receives UTF-16 WM_CHAR/WM_SYSCHAR units from its Unicode window.
// Incomplete pairs belong to that panel and never enter a search mask.
inline bool AppendQuickSearchCodeUnit(wchar_t& pendingHighSurrogate,
                                      std::uint32_t value, std::wstring& text)
{
    text.clear();
    if (value >= 0xD800 && value <= 0xDBFF)
    {
        pendingHighSurrogate = (wchar_t)value;
        return false;
    }
    wchar_t high = pendingHighSurrogate;
    pendingHighSurrogate = 0;
    if (value >= 0xDC00 && value <= 0xDFFF)
    {
        if (high == 0)
            return false;
        text += high;
        text += (wchar_t)value;
        return true;
    }
    if (value <= 31 || value > 0xFFFF)
        return false;
    text += (wchar_t)value;
    return true;
}

inline std::wstring QuickSearchCodePointText(std::uint32_t value)
{
    if (value > 0x10FFFF || value >= 0xD800 && value <= 0xDFFF)
        return std::wstring();
    if (value <= 0xFFFF)
        return std::wstring(1, (wchar_t)value);
    value -= 0x10000;
    std::wstring result(1, (wchar_t)(0xD800 + (value >> 10)));
    result += (wchar_t)(0xDC00 + (value & 0x3FF));
    return result;
}

inline std::size_t PreviousQuickSearchCharacter(const std::wstring& text, std::size_t end)
{
    if (end == 0)
        return 0;
    --end;
    if (end > 0 && text[end] >= 0xDC00 && text[end] <= 0xDFFF &&
        text[end - 1] >= 0xD800 && text[end - 1] <= 0xDBFF)
        --end;
    return end;
}

inline void RemoveLastQuickSearchCharacter(std::wstring& text)
{
    text.resize(PreviousQuickSearchCharacter(text, text.length()));
}

inline bool IsQuickSearchWildcard(wchar_t ch)
{
    return ch == L'/' || ch == L'\\' || ch == L'<';
}

inline void ShortenQuickSearchPrefix(std::wstring& prefix, std::wstring& mask)
{
    RemoveLastQuickSearchCharacter(prefix);
    std::size_t last = PreviousQuickSearchCharacter(mask, mask.length());
    std::size_t previous = PreviousQuickSearchCharacter(mask, last);
    if (last > 0 && !IsQuickSearchWildcard(mask[last]) && !IsQuickSearchWildcard(mask[previous]))
        mask.resize(last);
    else
        mask = prefix;
}

inline bool ExtendQuickSearchPrefix(std::wstring& prefix, std::wstring& mask,
                                    const std::wstring& name)
{
    std::size_t start = prefix.length();
    if (start >= name.length())
        return false;
    std::size_t count = 1;
    if (name[start] >= 0xD800 && name[start] <= 0xDBFF && start + 1 < name.length() &&
        name[start + 1] >= 0xDC00 && name[start + 1] <= 0xDFFF)
        count = 2;
    prefix.append(name, start, count);
    mask.append(name, start, count);
    return true;
}

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
