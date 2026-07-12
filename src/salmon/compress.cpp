// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"


static void AppendPowerShellQuotedString(char* text, int textSize, const char* value)
{
    lstrcat(text, "'");
    for (const char* s = value; *s != 0 && (int)strlen(text) < textSize - 3; s++)
    {
        if (*s == '\'')
            lstrcat(text, "''");
        else
        {
            int len = (int)strlen(text);
            text[len] = *s;
            text[len + 1] = 0;
        }
    }
    lstrcat(text, "'");
}

static BOOL WriteTextFile(const char* fileName, const char* text)
{
    HANDLE hFile = CreateFile(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD written;
    DWORD size = (DWORD)strlen(text);
    BOOL ret = WriteFile(hFile, text, size, &written, NULL) && written == size;
    CloseHandle(hFile);
    return ret;
}

static BOOL RunHiddenProcessAndWait(char* commandLine, const char* currentDir)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcess(NULL, commandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, currentDir, &si, &pi))
        return FALSE;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return exitCode == 0;
}

static BOOL CompressBugReportToZip(const char* reportName, char* errorMessage, int errorMessageSize)
{
    char archive[SAL_MAX_PATH];
    sprintf(archive, "%s%s.ZIP", BugReportPath, reportName);
    DeleteFile(archive);

    char scriptPath[SAL_MAX_PATH];
    sprintf(scriptPath, "%s~salmon-compress-%u.ps1", BugReportPath, GetTickCount());

    char script[30000];
    lstrcpy(script, "$ErrorActionPreference = 'Stop'\r\nCompress-Archive -LiteralPath @(\r\n");

    char mask[SAL_MAX_PATH];
    sprintf(mask, "%s%s.*", BugReportPath, reportName);

    BOOL first = TRUE;
    WIN32_FIND_DATA find;
    HANDLE hFind = HANDLES_Q(FindFirstFile(mask, &find));
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            const char* ext = strrchr(find.cFileName, '.');
            if (ext != NULL && (_stricmp(ext, ".7Z") == 0 || _stricmp(ext, ".ZIP") == 0))
                continue;

            char fileName[SAL_MAX_PATH];
            sprintf(fileName, "%s%s", BugReportPath, find.cFileName);
            if (!first)
                lstrcat(script, ",\r\n");
            lstrcat(script, "  ");
            AppendPowerShellQuotedString(script, sizeof(script), fileName);
            first = FALSE;
        } while (FindNextFile(hFind, &find));
        HANDLES(FindClose(hFind));
    }

    if (first)
    {
        lstrcpyn(errorMessage, "No bug report files were found for compression.", errorMessageSize);
        return FALSE;
    }

    lstrcat(script, "\r\n) -DestinationPath ");
    AppendPowerShellQuotedString(script, sizeof(script), archive);
    lstrcat(script, " -Force\r\n");

    if (!WriteTextFile(scriptPath, script))
    {
        lstrcpyn(errorMessage, "Cannot create PowerShell compression script.", errorMessageSize);
        return FALSE;
    }

    char commandLine[SAL_MAX_PATH + 200];
    sprintf(commandLine, "powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"%s\"", scriptPath);
    BOOL ret = RunHiddenProcessAndWait(commandLine, BugReportPath);
    DeleteFile(scriptPath);

    if (!ret)
        lstrcpyn(errorMessage, "Can not compress the bug report into a ZIP archive.", errorMessageSize);
    return ret;
}

static BOOL CompressBugReportsToZip(CCompressParams* compressParams)
{
    BOOL ret = TRUE;
    compressParams->ErrorMessage[0] = 0;
    for (int i = 0; i < BugReports.Count; i++)
    {
        BOOL res = CompressBugReportToZip(BugReports[i].Name, compressParams->ErrorMessage, 2 * MAX_PATH - 1);
        ret &= res;
        if (!res || !ReportOldBugs)
            break;
    }
    if (ret)
        compressParams->ErrorMessage[0] = 0;
    return ret;
}

//------------------------------------------------------------------------------------------------
//
// CompresBugReports()
//

BOOL CompresBugReports(CCompressParams* compressParams)
{
    return CompressBugReportsToZip(compressParams);
}

DWORD WINAPI CompressThreadF(void* param)
{
    CCompressParams* compressParams = (CCompressParams*)param;
    compressParams->Result = CompresBugReports(compressParams);
    return EXIT_SUCCESS;
}

HANDLE HCompressThread = NULL;

BOOL StartCompressThread(CCompressParams* params)
{
    if (HCompressThread != NULL)
        return FALSE;
    DWORD id;
    HCompressThread = CreateThread(NULL, 0, CompressThreadF, params, 0, &id);
    return HCompressThread != NULL;
}

BOOL IsCompressThreadRunning()
{
    if (HCompressThread == NULL)
        return FALSE;
    DWORD res = WaitForSingleObject(HCompressThread, 0);
    if (res != WAIT_TIMEOUT)
    {
        CloseHandle(HCompressThread);
        HCompressThread = NULL;
        return FALSE;
    }
    return TRUE;
}
