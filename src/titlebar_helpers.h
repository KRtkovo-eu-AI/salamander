// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TITLEBAR_HELPERS_H
#define TITLEBAR_HELPERS_H

#include <string>

static int WCharVisualWidth(wchar_t ch)
{
    if (ch < 0x80)
        return 1;
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
        w += WCharVisualWidth(s[i]);
    return w;
}

static void EnsureAppNameSuffixInTitle(std::wstring& title, const std::wstring& appSuffix)
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

    int suffixVW = StringVisualWidth(appSuffix);
    int sepVW = StringVisualWidth(separator);
    int totalVWLimit = 68;
    int maxPrefixVW = totalVWLimit - suffixVW - sepVW - 3;
    if (maxPrefixVW < 10)
        maxPrefixVW = 10;

    int vw = 0;
    size_t cutAt = 0;
    for (size_t i = 0; i < prefix.length(); i++)
    {
        int cw = WCharVisualWidth(prefix[i]);
        if (vw + cw > maxPrefixVW)
            break;
        vw += cw;
        cutAt = i + 1;
    }

    if (cutAt == 0)
        cutAt = 1;

    if (cutAt < prefix.length())
    {
        if (cutAt > 0 && cutAt < prefix.length())
        {
            wchar_t lastChar = prefix[cutAt - 1];
            if (lastChar >= 0xD800 && lastChar <= 0xDBFF)
            {
                wchar_t nextChar = prefix[cutAt];
                if (nextChar >= 0xDC00 && nextChar <= 0xDFFF)
                    cutAt++;
            }
        }
        title = prefix.substr(0, cutAt) + L"..." + separator + appSuffix;
    }
}

#endif // TITLEBAR_HELPERS_H
