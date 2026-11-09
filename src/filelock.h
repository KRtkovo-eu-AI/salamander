// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Detect and terminate processes holding operation targets. | Offer recovery directly from file operation errors without requiring an extension. | https://github.com/KRtkovo-eu-AI/salamander/issues/836 | GPT-6 (Codex) |
#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct CFileLockingProcess
{
    DWORD ProcessId;
    FILETIME ProcessStartTime;
    std::wstring Name;
    BOOL IsService;
};

BOOL GetFileLockingProcesses(const std::wstring& path, std::vector<CFileLockingProcess>& processes);
BOOL TerminateFileLockingProcesses(const std::vector<CFileLockingProcess>& processes);
