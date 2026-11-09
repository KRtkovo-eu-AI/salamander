// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Detect and terminate processes holding operation targets. | Offer recovery directly from file operation errors without requiring an extension. | https://github.com/KRtkovo-eu-AI/salamander/issues/836 | GPT-6 (Codex) |

#include "precomp.h"
#include "filelock.h"

#include <RestartManager.h>
#pragma comment(lib, "Rstrtmgr.lib")

BOOL GetFileLockingProcesses(const std::wstring& path, std::vector<CFileLockingProcess>& processes)
{
    processes.clear();
    if (path.empty()) return FALSE;

    DWORD session = 0;
    WCHAR sessionKey[CCH_RM_SESSION_KEY + 1] = {};
    if (RmStartSession(&session, 0, sessionKey) != ERROR_SUCCESS) return FALSE;

    BOOL result = FALSE;
    LPCWSTR paths[] = { path.c_str() };
    if (RmRegisterResources(session, 1, paths, 0, NULL, 0, NULL) == ERROR_SUCCESS)
    {
        UINT needed = 0;
        UINT count = 0;
        DWORD rebootReasons = 0;
        DWORD status = RmGetList(session, &needed, &count, NULL, &rebootReasons);
        if (status == ERROR_MORE_DATA && needed != 0)
        {
            std::vector<RM_PROCESS_INFO> infos(needed);
            count = needed;
            status = RmGetList(session, &needed, &count, &infos[0], &rebootReasons);
            if (status == ERROR_SUCCESS)
            {
                DWORD currentPid = GetCurrentProcessId();
                for (UINT i = 0; i < count; ++i)
                {
                    const RM_PROCESS_INFO& info = infos[i];
                    if (info.Process.dwProcessId == 0 || info.Process.dwProcessId == currentPid)
                        continue;
                    CFileLockingProcess process;
                    process.ProcessId = info.Process.dwProcessId;
                    process.ProcessStartTime = info.Process.ProcessStartTime;
                    process.Name = info.strAppName;
                    process.IsService = info.ApplicationType == RmService || info.ApplicationType == RmCritical;
                    processes.push_back(process);
                }
                result = !processes.empty();
            }
        }
        else if (status == ERROR_SUCCESS)
            result = TRUE;
    }

    RmEndSession(session);
    return result;
}

BOOL TerminateFileLockingProcesses(const std::vector<CFileLockingProcess>& processes)
{
    BOOL terminatedAny = FALSE;
    for (size_t i = 0; i < processes.size(); ++i)
    {
        const CFileLockingProcess& process = processes[i];
        if (process.IsService) continue;

        HANDLE handle = OpenProcess(PROCESS_TERMINATE | PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE,
                                    FALSE, process.ProcessId);
        if (handle == NULL) continue;

        FILETIME creation, exit, kernel, user;
        BOOL sameProcess = GetProcessTimes(handle, &creation, &exit, &kernel, &user) &&
                           CompareFileTime(&creation, &process.ProcessStartTime) == 0;
        if (sameProcess && TerminateProcess(handle, ERROR_CANCELLED))
        {
            WaitForSingleObject(handle, 5000);
            terminatedAny = TRUE;
        }
        CloseHandle(handle);
    }
    return terminatedAny;
}
