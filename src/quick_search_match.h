// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <string>

namespace Salamander::Panel
{
namespace QuickSearchMatchDetail
{
inline std::wstring Normalize(const wchar_t* text, int length)
{
    const int required = NormalizeString(NormalizationC, text, length, NULL, 0);
    if (required <= 0)
        return std::wstring(text, length);
    std::wstring result(required, L'\0');
    const int written = NormalizeString(NormalizationC, text, length, &result[0], required);
    if (written <= 0)
        return std::wstring(text, length);
    result.resize(written);
    return result;
}

inline int ScalarLength(const wchar_t* text)
{
    return text[0] >= 0xD800 && text[0] <= 0xDBFF &&
           text[1] >= 0xDC00 && text[1] <= 0xDFFF ? 2 : 1;
}

inline bool Agree(const wchar_t* filename, bool hasExtension, const wchar_t* base,
                  const wchar_t* mask, bool wholeString, int& offset)
{
    while (*filename != 0)
    {
        if (!wholeString && *mask == 0)
        {
            offset = (int)(filename - base);
            return true;
        }
        const int nameLength = ScalarLength(filename);
        const int maskLength = *mask != 0 ? ScalarLength(mask) : 0;
        // Windows ordinal casing is independent of the CRT's current locale.
        // Compare complete scalars, so neither matching nor wildcard stepping
        // can stop between the halves of a supplementary character.
        if (maskLength != 0 &&
            CompareStringOrdinal(filename, nameLength, mask, maskLength, TRUE) == CSTR_EQUAL)
        {
            filename += nameLength;
            mask += maskLength;
        }
        else if (*mask == L'/')
        {
            ++mask;
            while (*filename != 0)
            {
                if (Agree(filename, hasExtension, base, mask, wholeString, offset))
                    return true;
                filename += ScalarLength(filename);
            }
            break;
        }
        else
            return false;
    }
    if (*mask == 0 || (!hasExtension && *mask == L'.' && mask[1] == 0))
    {
        offset = (int)(filename - base);
        return true;
    }
    return false;
}

inline int OriginalOffset(const std::wstring& original, const std::wstring& normalized,
                          int normalizedOffset)
{
    if (normalizedOffset <= 0)
        return 0;
    if (original == normalized)
        return normalizedOffset;
    // Equal prefix lengths alone are insufficient: "e" and "e + acute" both
    // normalize to one code unit, but only the latter matches the final "e acute".
    // Compare the actual normalized prefix and return an original UTF-16 boundary.
    for (size_t end = 0; end < original.length();)
    {
        end += ScalarLength(original.c_str() + end);
        const std::wstring prefix = Normalize(original.c_str(), (int)end);
        if (prefix.length() >= (size_t)normalizedOffset &&
            prefix.compare(0, normalizedOffset, normalized, 0, normalizedOffset) == 0)
            return (int)end;
    }
    return (int)original.length();
}
} // namespace QuickSearchMatchDetail

// Quick-search only: preserve '/' wildcard and extensionless trailing-dot rules.
// The successful offset always refers to the original filename's UTF-16 units.
inline bool MatchQuickSearchWide(const wchar_t* filename, bool hasExtension,
                                 const wchar_t* mask, bool wholeString, int& offset)
{
    offset = 0;
    if (filename == NULL || mask == NULL)
        return false;
    const std::wstring original(filename);
    const std::wstring normalizedName = QuickSearchMatchDetail::Normalize(filename, (int)original.length());
    const std::wstring normalizedMask = QuickSearchMatchDetail::Normalize(mask, (int)wcslen(mask));
    int normalizedOffset = 0;
    if (!QuickSearchMatchDetail::Agree(normalizedName.c_str(), hasExtension,
                                      normalizedName.c_str(), normalizedMask.c_str(),
                                      wholeString, normalizedOffset))
        return false;
    offset = QuickSearchMatchDetail::OriginalOffset(original, normalizedName, normalizedOffset);
    return true;
}
} // namespace Salamander::Panel
