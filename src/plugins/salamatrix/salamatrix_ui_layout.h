// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Salamatrix
{
namespace UI
{

struct FilePickerLayoutMetrics
{
    int EditWidth;
    int BrowseX;
    int BrowseWidth;
};

inline FilePickerLayoutMetrics ComputeFilePickerLayout(int x, int width)
{
    if (width < 1)
        width = 1;
    int editWidth = width - 28;
    if (editWidth < 1)
        editWidth = 1;
    int browseX = x + editWidth + 4;
    int browseWidth = width - editWidth - 4;
    if (browseWidth < 1)
        browseWidth = 1;
    return FilePickerLayoutMetrics{editWidth, browseX, browseWidth};
}

inline int ScaleDialogMetric(int value, unsigned oldDpi, unsigned newDpi)
{
    if (oldDpi == 0)
        oldDpi = 96;
    if (newDpi == 0)
        newDpi = oldDpi;
    return static_cast<int>(
        (static_cast<long long>(value) * newDpi + oldDpi / 2) / oldDpi);
}

} // namespace UI
} // namespace Salamatrix
