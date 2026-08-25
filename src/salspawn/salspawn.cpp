// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>

#include "lstrfix.h"

#pragma warning(3 : 4706) // warning C4706: assignment within conditional expression

/*
SALSPAWN errorcodes:
  err -> External prg err
  retBase -> bad options
  retBase + 1 -> no executable
  retBase * 2 + err -> CreateProcess err
  retBase * 3 + err -> WaitForSingleObject err
  retBase * 4 + err -> GetExitCode err
*/

BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
    // Ignore Ctrl+C, Ctrl+Break, and other console-close reasons: the spawned
    // archiver must exit first, otherwise it keeps running after Salamander has
    // already reported that compression/decompression finished.
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        return TRUE;

    default:
        return FALSE;
    }
}

// ****************************************************************************
// EnableExceptionsOn64
//

// Report SEH exceptions even on x64 Windows 7 SP1 and later.
// http://blog.paulbetts.org/index.php/2010/07/20/the-case-of-the-disappearing-onload-exception-user-mode-callback-exceptions-in-x64/
// http://connect.microsoft.com/VisualStudio/feedback/details/550944/hardware-exceptions-on-x64-machines-are-silently-caught-in-wndproc-messages
// http://support.microsoft.com/kb/976038
void EnableExceptionsOn64()
{
    typedef BOOL(WINAPI * FSetProcessUserModeExceptionPolicy)(DWORD dwFlags);
    typedef BOOL(WINAPI * FGetProcessUserModeExceptionPolicy)(LPDWORD dwFlags);
    typedef BOOL(WINAPI * FIsWow64Process)(HANDLE, PBOOL);
#define PROCESS_CALLBACK_FILTER_ENABLED 0x1

    HINSTANCE hDLL = LoadLibrary("KERNEL32.DLL");
    if (hDLL != NULL)
    {
        FIsWow64Process isWow64 = (FIsWow64Process)GetProcAddress(hDLL, "IsWow64Process");                                                      // Min: XP SP2
        FSetProcessUserModeExceptionPolicy set = (FSetProcessUserModeExceptionPolicy)GetProcAddress(hDLL, "SetProcessUserModeExceptionPolicy"); // Min: Vista with hotfix
        FGetProcessUserModeExceptionPolicy get = (FGetProcessUserModeExceptionPolicy)GetProcAddress(hDLL, "GetProcessUserModeExceptionPolicy"); // Min: Vista with hotfix
        if (isWow64 != NULL && set != NULL && get != NULL)
        {
            BOOL bIsWow64;
            if (isWow64(GetCurrentProcess(), &bIsWow64) && bIsWow64)
            {
                DWORD dwFlags;
                if (get(&dwFlags))
                    set(dwFlags & ~PROCESS_CALLBACK_FILTER_ENABLED);
            }
        }
        FreeLibrary(hDLL);
    }
}

void mainCRTStartup()
{
    EnableExceptionsOn64();
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE);
    BOOL help = FALSE;
    BOOL error = FALSE;
    int retBase = 10000;
    const int exeNameChars = 32768;
    wchar_t* exeName = (wchar_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                           exeNameChars * sizeof(wchar_t));
    wchar_t* cmdline;
    DWORD exitCode;

    if (exeName == NULL)
        ExitProcess(retBase);

    // Do not show critical errors such as "no disk in drive A:".
    SetErrorMode(SetErrorMode(0) | SEM_FAILCRITICALERRORS);

    cmdline = GetCommandLineW();
    // skip leading spaces
    while (*cmdline == L' ' || *cmdline == L'\t')
        cmdline++;
    // skip exe name
    if (*cmdline == L'"')
    {
        cmdline++;
        while (*cmdline != L'\0' && *cmdline != L'"')
            cmdline++;
        if (*cmdline == L'"')
            cmdline++;
    }
    else
        while (*cmdline != L'\0' && *cmdline != L' ' && *cmdline != L'\t')
            cmdline++;
    // get params
    while (1)
    {
        // skip spaces
        while (*cmdline == L' ' || *cmdline == L'\t')
            cmdline++;
        if (*cmdline == L'\0')
            break;
        // is it a switch ?
        if (*cmdline == L'-' || *cmdline == L'/')
        {
            cmdline += 2;
            switch (*(cmdline - 1))
            {
            case '?':
            case 'h':
            case 'H':
                help = TRUE;
                break;
            case 'c':
                if (*cmdline > L'9' || *cmdline < L'0')
                {
                    help = TRUE;
                    break;
                }
                retBase = 0;
                while (*cmdline <= L'9' && *cmdline >= L'0')
                    retBase = retBase * 10 + *cmdline++ - L'0';
                if (*cmdline != L' ' && *cmdline != L'\t' && *cmdline != L'\0')
                {
                    help = TRUE;
                    break;
                }
                break;
            default:
                ExitProcess(retBase);
            }
        }
        // if not, it must be a line to execute
        else
        {
            int len = 0;
            while (len < exeNameChars - 1 && *cmdline != L'\0')
                exeName[len++] = *cmdline++;
            exeName[len] = L'\0';
        }
    }

    if (exeName[0] == L'\0' || help)
    {
        DWORD written;
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
                  "SALSPAWN: Spawn for Open Salamander, Copyright (C) 1998-2026 Open Salamander Authors\n\nUsage: salspawn [-|/<switch>] <executable> [exe params]\n\nAvailable switches:\n  ?,h,H - this help screen\n  c<num> - sets base of SALSPAWN error level to <num>\n\n",
                  221, &written, NULL);
        ExitProcess(retBase + 1);
    }

    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.lpTitle = NULL;
    si.lpDesktop = NULL;
    si.cbReserved2 = 0;
    si.lpReserved2 = 0;
    si.dwFlags = 0;
    if (!CreateProcessW(NULL, exeName, NULL, NULL, TRUE, CREATE_NEW_PROCESS_GROUP,
                       NULL, NULL, &si, &pi))
    {
        ExitProcess(GetLastError() + retBase * 2);
    }

    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED)
    {
        exitCode = GetLastError() + retBase * 3;
        goto EXIT1;
    }

    if (!GetExitCodeProcess(pi.hProcess, &exitCode))
    {
        exitCode = GetLastError() + retBase * 4;
    }
EXIT1:
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    ExitProcess(exitCode);
}
