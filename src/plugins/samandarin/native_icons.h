// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Declare native plugin icon-list and image decoding helpers. | Restore catalog and installed-package icons in the updates dialog. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include <commctrl.h>
#include <string>

namespace SamandarinNative
{
HIMAGELIST NativeIcons_CreateImageList(int iconSize, int* defaultImageIndex);
int NativeIcons_AddImage(HIMAGELIST imageList, const std::string& imageBytes);
int NativeIcons_AddImageFromPath(HIMAGELIST imageList, const std::wstring& path);
} // namespace SamandarinNative
