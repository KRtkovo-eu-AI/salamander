// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TITLEBAR_HELPERS_H
#define TITLEBAR_HELPERS_H

#include <string>

static int WCharVisualWidth(wchar_t ch)
{
    if (ch < 0x80)
        return 1;
    if (ch >= 0xD800 && ch <= 0xDFFF)
        return 2;
    if (ch >= 0xFF01 && ch <= 0xFF5E)
        return 2;
    if ((ch >= 0x2E80 && ch <= 0x9FFF) ||
        (ch >= 0xF900 && ch <= 0xFAFF) ||
        (ch >= 0xFE30 && ch <= 0xFE4F) ||
        (ch >= 0xFF60 && ch <= 0xFF63) ||
        (ch >= 0xFFE0 && ch <= 0xFFE6))
        return 3;
    return 1;
}

static int StringVisualWidth(const std::wstring& s)
{
    int w = 0;
    for (size_t i = 0; i < s.length(); i++)
    {
        if (s[i] >= 0xD800 && s[i] <= 0xDBFF &&
            i + 1 < s.length() && s[i + 1] >= 0xDC00 && s[i + 1] <= 0xDFFF)
        {
            w += 2;
            i++;
        }
        else
        {
            w += WCharVisualWidth(s[i]);
        }
    }
    return w;
}

static bool StringHasNonAscii(const std::wstring& s)
{
    for (size_t i = 0; i < s.length(); i++)
        if (s[i] >= 0x80)
            return true;
    return false;
}

static int LeadingAsciiVisualWidth(const std::wstring& s)
{
    int w = 0;
    for (size_t i = 0; i < s.length(); i++)
    {
        if (s[i] >= 0x80)
            break;
        w += WCharVisualWidth(s[i]);
    }
    return w;
}

static size_t TitlePrefixCutByVisualWidth(const std::wstring& prefix, int maxPrefixVW)
{
    if (maxPrefixVW <= 0)
        return 0;

    int vw = 0;
    size_t cutAt = 0;
    for (size_t i = 0; i < prefix.length(); i++)
    {
        size_t next = i + 1;
        int cw;
        if (prefix[i] >= 0xD800 && prefix[i] <= 0xDBFF &&
            next < prefix.length() && prefix[next] >= 0xDC00 && prefix[next] <= 0xDFFF)
        {
            cw = 2;
            next++;
        }
        else
        {
            cw = WCharVisualWidth(prefix[i]);
        }

        if (vw + cw > maxPrefixVW)
            break;
        vw += cw;
        cutAt = next;
        i = next - 1;
    }
    return cutAt;
}

static void EnsureAppNameSuffixInTitle(std::wstring& title, const std::wstring& appSuffix,
                                       int maxPrefixVW = 220, int maxUnicodePrefixVW = 2)
{
    if (title.empty() || appSuffix.empty() ||
        title.length() <= appSuffix.length() ||
        title.compare(title.length() - appSuffix.length(), appSuffix.length(), appSuffix) != 0)
    {
        return;
    }

    const std::wstring separator = L" - ";
    size_t prefixEnd = title.length() - appSuffix.length();
    if (prefixEnd >= separator.length() &&
        title.compare(prefixEnd - separator.length(), separator.length(), separator) == 0)
    {
        prefixEnd -= separator.length();
    }
    std::wstring prefix = title.substr(0, prefixEnd);
    if (prefix.empty())
        return;

    int effectiveMaxPrefixVW = maxPrefixVW;
    if (StringHasNonAscii(prefix))
    {
        int unicodePrefixVW = LeadingAsciiVisualWidth(prefix) + maxUnicodePrefixVW;
        effectiveMaxPrefixVW = effectiveMaxPrefixVW < unicodePrefixVW ? effectiveMaxPrefixVW : unicodePrefixVW;
    }

    size_t cutAt = TitlePrefixCutByVisualWidth(prefix, effectiveMaxPrefixVW);

    if (cutAt < prefix.length())
    {
        title = prefix.substr(0, cutAt) + L"..." + separator + appSuffix;
    }
}

#endif // TITLEBAR_HELPERS_H
