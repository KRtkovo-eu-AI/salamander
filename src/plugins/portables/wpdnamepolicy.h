// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Decide portable overwrite and rename without a device. | Case, create, and folder-property choices must be testable without a phone. | | Grok 4.7 |

#pragma once

// Decisions shared by the Portable Devices transfer path.
// Name text is already UTF-8 on both sides. Comparison goes through the
// caller-supplied case-insensitive comparator (Salamander's StrICmp).

enum WpdRenamePropertyPlan
{
    // Folders accept WPD_OBJECT_NAME only. Setting ORIGINAL_FILE_NAME in the
    // same call makes some devices reject the whole update.
    WpdRenameFolderNameOnly = 1,
    // Files set WPD_OBJECT_NAME and WPD_OBJECT_ORIGINAL_FILE_NAME in separate
    // SetValues calls so one unsupported property does not fail the other.
    WpdRenameFileNameAndOriginalSeparately = 2
};

inline WpdRenamePropertyPlan WpdRenamePlanForObject(bool isDirectory)
{
    return isDirectory ? WpdRenameFolderNameOnly : WpdRenameFileNameAndOriginalSeparately;
}

// CreateObject* is refused while the destination name is still present.
// A failed create on Android MTP wedges the session until the device is
// unplugged and the process is restarted.
inline bool WpdMayCreateWhenNamePresent(bool nameStillPresent)
{
    return !nameStillPresent;
}

inline bool WpdNamesEqualInsensitive(const char* left, const char* right, int (*compare)(const char*, const char*))
{
    if (left == nullptr || right == nullptr || compare == nullptr)
        return false;
    return compare(left, right) == 0;
}

// A child matches when either stored name equals the wanted name.
inline bool WpdChildMatchesEitherName(const char* originalName, bool hasOriginal,
                                      const char* objectName, bool hasObject,
                                      const char* wantedName,
                                      int (*compare)(const char*, const char*))
{
    if (hasOriginal && WpdNamesEqualInsensitive(originalName, wantedName, compare))
        return true;
    if (hasObject && WpdNamesEqualInsensitive(objectName, wantedName, compare))
        return true;
    return false;
}
