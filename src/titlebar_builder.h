// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef TITLEBAR_BUILDER_H
#define TITLEBAR_BUILDER_H

#include <string>

static std::wstring BuildMainWindowTitleText(const std::wstring& prefix,
                                             const std::wstring& path,
                                             const std::wstring& suffix)
{
    std::wstring title;
    if (!prefix.empty())
    {
        title += prefix;
        title += L" - ";
    }
    if (!path.empty())
    {
        title += path;
        title += L" - ";
    }
    title += suffix;
    return title;
}

#endif // TITLEBAR_BUILDER_H
