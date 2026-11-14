// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Compare token ACL predictions with own-token access. | Evaluate a non-invasive launch-rights diagnostic. | #731 | GPT-6 |

#include <windows.h>
#include <aclapi.h>
#include <cstdio>

int main()
{
    HANDLE own = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE | READ_CONTROL, &own))
    {
        std::printf("SKIP cannot read own token: %lu\n", GetLastError());
        return 2;
    }
    TOKEN_ELEVATION elevation{};
    DWORD length = 0;
    if (!GetTokenInformation(own, TokenElevation, &elevation, sizeof(elevation), &length) || elevation.TokenIsElevated)
    {
        std::puts("SKIP test requires a non-elevated process");
        CloseHandle(own);
        return 2;
    }
    HANDLE caller = nullptr;
    if (!DuplicateToken(own, SecurityImpersonation, &caller))
    {
        std::printf("FAIL duplicate own caller token: %lu\n", GetLastError());
        CloseHandle(own);
        return 1;
    }
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    const DWORD secError = GetSecurityInfo(own, SE_KERNEL_OBJECT,
        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        nullptr, nullptr, nullptr, nullptr, &descriptor);
    if (secError != ERROR_SUCCESS || !descriptor)
    {
        std::printf("FAIL own token security descriptor: %lu\n", secError);
        CloseHandle(caller); CloseHandle(own);
        return 1;
    }
    const DWORD rights[] = {TOKEN_QUERY, TOKEN_DUPLICATE, TOKEN_ASSIGN_PRIMARY,
                            TOKEN_IMPERSONATE, TOKEN_ADJUST_PRIVILEGES, TOKEN_ADJUST_GROUPS,
                            TOKEN_ADJUST_DEFAULT, TOKEN_ADJUST_SESSIONID, READ_CONTROL, WRITE_DAC, WRITE_OWNER};
    GENERIC_MAPPING mapping{TOKEN_READ, TOKEN_WRITE, TOKEN_EXECUTE, TOKEN_ALL_ACCESS};
    bool ok = true;
    for (DWORD right : rights)
    {
        PRIVILEGE_SET privileges{};
        DWORD privilegeLength = sizeof(privileges), granted = 0;
        BOOL allowed = FALSE;
        const BOOL checked = AccessCheck(descriptor, caller, right, &mapping, &privileges,
                                         &privilegeLength, &granted, &allowed);
        HANDLE trial = nullptr;
        const BOOL actual = OpenProcessToken(GetCurrentProcess(), right, &trial);
        const DWORD actualError = actual ? ERROR_SUCCESS : GetLastError();
        if (trial) CloseHandle(trial);
        if (!checked || (!actual && actualError != ERROR_ACCESS_DENIED) || !!allowed != !!actual)
            ok = false;
        std::printf("right=0x%08lX acl=%d actual=%d error=%lu\n", right, allowed, actual, actualError);
    }
    LocalFree(descriptor);
    CloseHandle(caller);
    CloseHandle(own);
    std::puts(ok ? "PASS own non-elevated token ACL comparison" : "FAIL own-token ACL comparison");
    return ok ? 0 : 1;
}
