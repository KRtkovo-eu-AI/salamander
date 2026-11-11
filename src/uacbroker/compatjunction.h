// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-10 | Recognize Windows compatibility links by metadata and ACL. | Avoid counting aliases or stopping the size scan at a deliberate list denial. | #731 | GPT-6.1 Sol |
#pragma once

#include <windows.h>
#include <aclapi.h>
#include <memory>

namespace UacListing
{
inline bool HasCompatibilityLinkAttributes(DWORD attributes)
{
    constexpr DWORD required = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT |
                               FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    return (attributes & required) == required;
}

// The caller opens the link itself. Windows marks profile aliases as hidden
// system links and denies Everyone permission to list the link.
inline bool IsCompatibilityLink(HANDLE link)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if (!GetFileInformationByHandleEx(link, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !HasCompatibilityLinkAttributes(attributes.FileAttributes) ||
        (attributes.ReparseTag != IO_REPARSE_TAG_MOUNT_POINT && attributes.ReparseTag != IO_REPARSE_TAG_SYMLINK))
        return false;

    PACL acl = nullptr;
    PSECURITY_DESCRIPTOR raw = nullptr;
    const DWORD error = GetSecurityInfo(link, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                        nullptr, nullptr, &acl, nullptr, &raw);
    const std::unique_ptr<void, decltype(&LocalFree)> descriptor(raw, LocalFree);
    if (error || !acl)
        return false;
    for (DWORD index = 0; index < acl->AceCount; ++index)
    {
        void* entry = nullptr;
        if (!GetAce(acl, index, &entry))
            return false;
        const auto* header = static_cast<const ACE_HEADER*>(entry);
        if (header->AceType != ACCESS_DENIED_ACE_TYPE ||
            (header->AceFlags & (INHERIT_ONLY_ACE | INHERITED_ACE)))
            continue;
        const auto* deny = static_cast<const ACCESS_DENIED_ACE*>(entry);
        if ((deny->Mask & FILE_LIST_DIRECTORY) && IsWellKnownSid(const_cast<DWORD*>(&deny->SidStart), WinWorldSid))
            return true;
    }
    return false;
}
}
