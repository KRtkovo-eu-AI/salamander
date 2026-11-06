// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <string>

// Only the item temporarily selected for a dialog belongs to this snapshot.
// An explicit selection leaves both fields empty and is never undone here.
struct CPanelTemporarySelection
{
    std::string Name;
    std::wstring BranchIdentity;
    void Clear() { Name.clear(); BranchIdentity.clear(); }
};

// Resolve against the current listing: refresh/sorting may have replaced or
// reordered its rows. A missing item must never fall back to its basename.
template<class IdentityAt>
int FindBranchTemporarySelection(const CPanelTemporarySelection& selection,
                                 int count, IdentityAt identityAt)
{
    if (selection.BranchIdentity.empty()) return -1;
    for (int i = 0; i < count; ++i)
        if (identityAt(i) == selection.BranchIdentity) return i;
    return -1;
}
