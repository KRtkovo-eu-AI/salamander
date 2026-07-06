// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TITLEBAR_BUILDER_H
#define TITLEBAR_BUILDER_H

#include <string>

static size_t TitleBarSafePrefixLength(const std::wstring& text, size_t maxPrefixLength)
{
    if (text.length() <= maxPrefixLength)
        return text.length();

    size_t cutAt = maxPrefixLength;
    if (cutAt > 0 && cutAt < text.length() &&
        text[cutAt - 1] >= 0xD800 && text[cutAt - 1] <= 0xDBFF &&
        text[cutAt] >= 0xDC00 && text[cutAt] <= 0xDFFF)
    {
        cutAt--;
    }
    return cutAt;
}

static std::wstring BuildMainWindowTitleText(const std::wstring& prefix,
                                             const std::wstring& path,
                                             const std::wstring& suffix,
                                             size_t maxPrefixLength = 60)
{
    std::wstring titlePrefix;
    if (!prefix.empty())
    {
        titlePrefix += prefix;
        titlePrefix += L" - ";
    }
    if (!path.empty())
    {
        titlePrefix += path;
    }

    std::wstring title;
    if (!titlePrefix.empty())
    {
        size_t cutAt = TitleBarSafePrefixLength(titlePrefix, maxPrefixLength);
        if (cutAt < titlePrefix.length())
            title += titlePrefix.substr(0, cutAt) + L"...";
        else
            title += titlePrefix;
        title += L" - ";
    }
    title += suffix;
    return title;
}

#endif // TITLEBAR_BUILDER_H
