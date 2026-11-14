// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>
#define SAM_UAC_F7_HOST
#define SAM_UAC_THIN_COPY_TEST
#include "../uacbroker/thin_launch_access_probe.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && (_wcsicmp(argv[1], L"--external") == 0 || _wcsicmp(argv[1], L"--external-invalid") == 0))
    {
        HANDLE restricted = nullptr;
        if (_wcsicmp(argv[1], L"--external") == 0 &&
            !DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &restricted,
                             PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, 0)) return 4;
        UacThinLaunchProbe::Observe(restricted);
        if (restricted) CloseHandle(restricted);
        return 0;
    }
    const std::wstring base = L"Local\\SamandarinLaunchMaskTest-" + std::to_wstring(GetCurrentProcessId());
    std::vector<HANDLE> events;
    auto create = [&](const std::wstring& name) {
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, name.c_str());
        if (!event || GetLastError() == ERROR_ALREADY_EXISTS) return false;
        events.push_back(event);
        return true;
    };
    for (unsigned bit = 0; bit != 32; ++bit)
        if (!create(base + L"-bit-" + std::to_wstring(bit))) return 2;
    if (!create(base + L"-error") || !create(base + L"-ready")) return 2;
    if (!SetEnvironmentVariableW(L"SAM_UAC_TEST_LAUNCH_MASK_BASE", base.c_str())) return 3;
    HANDLE restricted = nullptr;
    constexpr DWORD desired = PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE;
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &restricted,
                         desired, FALSE, 0)) return 4;
    SetLastError(ERROR_ACCESS_DENIED);
    UacThinLaunchProbe::Observe(restricted);
    const bool preserved = GetLastError() == ERROR_ACCESS_DENIED;
    bool maskMatches = true;
    for (unsigned bit = 0; bit != 32; ++bit)
        maskMatches = maskMatches && ((WaitForSingleObject(events[bit], 0) == WAIT_OBJECT_0) ==
                                      ((desired & (DWORD(1) << bit)) != 0));
    const bool ready = WaitForSingleObject(events[33], 0) == WAIT_OBJECT_0;
    const bool noError = WaitForSingleObject(events[32], 0) == WAIT_TIMEOUT;
    CloseHandle(restricted);
    ResetEvent(events[32]); ResetEvent(events[33]);
    SetLastError(ERROR_SHARING_VIOLATION);
    UacThinLaunchProbe::Observe(nullptr);
    const bool failure = GetLastError() == ERROR_SHARING_VIOLATION &&
                         WaitForSingleObject(events[32], 0) == WAIT_OBJECT_0 &&
                         WaitForSingleObject(events[33], 0) == WAIT_OBJECT_0;
    SetEnvironmentVariableW(L"SAM_UAC_TEST_LAUNCH_MASK_BASE", nullptr);
    for (HANDLE event : events) CloseHandle(event);
    const bool ok = preserved && maskMatches && ready && noError && failure;
    std::printf("%s original process handle mask and failed query are signaled without changing last error\n",
                ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
}
