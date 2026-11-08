// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Share private recovery journal primitives. | Verify owner and DACL and flush records before mutating a name. | | GPT-6.1 Sol (Codex) |
#pragma once

#include <windows.h>
#include <aclapi.h>
#include <cstring>
#include <memory>

namespace UacRecoveryJournal
{
template <class Record>
inline DWORD WriteRecord(HANDLE journal, const Record& record)
{
    DWORD written = 0;
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) ||
        !WriteFile(journal, &record, sizeof(record), &written, nullptr))
        return GetLastError();
    if (written != sizeof(record))
        return ERROR_WRITE_FAULT;
    return FlushFileBuffers(journal) ? ERROR_SUCCESS : GetLastError();
}

inline DWORD ClearRecord(HANDLE journal)
{
    if (!SetFilePointerEx(journal, {}, nullptr, FILE_BEGIN) || !SetEndOfFile(journal) || !FlushFileBuffers(journal))
        return GetLastError();
    return ERROR_SUCCESS;
}

inline DWORD CheckPrivate(HANDLE object, PSECURITY_DESCRIPTOR expected)
{
    PACL expectedDacl = nullptr;
    PSID expectedOwner = nullptr;
    BOOL present = FALSE, defaulted = FALSE;
    SECURITY_DESCRIPTOR_CONTROL expectedControl{};
    DWORD revision = 0;
    if (!expected || !GetSecurityDescriptorDacl(expected, &present, &expectedDacl, &defaulted) ||
        !present || !expectedDacl || !GetSecurityDescriptorOwner(expected, &expectedOwner, &defaulted) ||
        !expectedOwner || !GetSecurityDescriptorControl(expected, &expectedControl, &revision) ||
        !(expectedControl & SE_DACL_PROTECTED))
        return ERROR_INVALID_SECURITY_DESCR;

    PACL dacl = nullptr;
    PSID owner = nullptr;
    PSECURITY_DESCRIPTOR raw = nullptr;
    const DWORD error = GetSecurityInfo(object, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                         &owner, nullptr, &dacl, nullptr, &raw);
    const std::unique_ptr<void, decltype(&LocalFree)> descriptor(raw, LocalFree);
    if (error)
        return error;
    SECURITY_DESCRIPTOR_CONTROL control{};
    if (!owner || !EqualSid(owner, expectedOwner) || !dacl || dacl->AclSize != expectedDacl->AclSize ||
        std::memcmp(dacl, expectedDacl, dacl->AclSize) ||
        !GetSecurityDescriptorControl(raw, &control, &revision) || !(control & SE_DACL_PROTECTED))
        return ERROR_ACCESS_DENIED;
    return ERROR_SUCCESS;
}
}
