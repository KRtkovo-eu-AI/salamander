// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Pass lock recovery paths through a typed dialog payload. | Avoid reinterpretation of narrow data as a wide path. | https://github.com/KRtkovo-eu-AI/salamander/pull/842 | GPT-6 (Codex) |
// | 2026-11-09 | Detect and terminate processes holding operation targets. | Offer recovery directly from file operation errors without requiring an extension. | https://github.com/KRtkovo-eu-AI/salamander/issues/836 | GPT-6 (Codex) |
#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct CFileLockRecoveryDialogData
{
    int* Result;
    const char* Caption;
    const char* FileName;
    const char* Error;
    const wchar_t* ExactPath;
};

struct CFileLockingProcess
{
    DWORD ProcessId;
    FILETIME ProcessStartTime;
    std::wstring Name;
    BOOL IsService;
};

BOOL GetFileLockingProcesses(const std::wstring& path, std::vector<CFileLockingProcess>& processes);
BOOL TerminateFileLockingProcesses(const std::vector<CFileLockingProcess>& processes);
