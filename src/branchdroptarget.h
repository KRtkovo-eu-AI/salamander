// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "common/widepath.h"

namespace Salamander { namespace BranchView {
// Deferred destination work must still refer to the collection's real root.
inline bool IsRootDropTarget(const wchar_t* root, const char* targetPath)
{
    if (root == NULL || root[0] == 0 || targetPath == NULL)
        return false;
    const std::wstring target = SalMultiByteToWidePath(targetPath, IsValidPathUtf8Text(targetPath) ? CP_UTF8 : CP_ACP);
    return !target.empty() && target == root;
}
} }
