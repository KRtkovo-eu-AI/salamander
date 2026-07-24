// SPDX-License-Identifier: GPL-2.0-or-later

#include "../precomp.h"
#include <strsafe.h>
#include <cstdio>

#include "../salamatrixbridge.h"

namespace
{
int Failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}

bool FindProgram(const wchar_t* name, wchar_t* path, int pathCount)
{
    return SearchPathW(NULL, name, NULL, pathCount, path, NULL) != 0;
}

bool WriteScript(const wchar_t* path, const char* text)
{
    HANDLE file = CreateFileW(
        path,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    DWORD written = 0;
    bool result = WriteFile(
        file,
        text,
        static_cast<DWORD>(strlen(text)),
        &written,
        NULL) != FALSE;
    CloseHandle(file);
    return result && written == strlen(text);
}

void MakePath(const wchar_t* extension, wchar_t* path, int pathCount)
{
    wchar_t tempPath[MAX_PATH];
    DWORD length = GetTempPathW(_countof(tempPath), tempPath);
    if (length == 0 || length >= _countof(tempPath))
    {
        path[0] = L'\0';
        return;
    }
    StringCchPrintfW(
        path,
        pathCount,
        L"%ssalamatrix-runtime-%lu%s",
        tempPath,
        GetCurrentProcessId(),
        extension);
}

void RunPythonTests()
{
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"python.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: python.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON", interpreter);

    wchar_t script[MAX_PATH];
    MakePath(L".py", script, _countof(script));
    Check(WriteScript(script, "print('salamatrix-python-ok')\n"), "write python script");

    CAutomationProcessRuntimeAdapter adapter(
        "Python.CPython",
        "CPython",
        "python",
        ".py",
        L"SALAMATRIX_PYTHON",
        L"python.exe",
        L"python3.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPython);
    Check(adapter.IsAvailable() != FALSE, "python adapter available");
    Check(adapter.SupportsEntryPoint("sample.py") != FALSE, "python extension support");

    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = script;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "python execution succeeds");
    Check(result.Status == Salamatrix::Runtime::RuntimeExecutionStatusSucceeded,
          "python result status");
    Check(wcsstr(result.Output, L"salamatrix-python-ok") != NULL,
          "python output captured");

    Check(WriteScript(script, "import time\ntime.sleep(2)\n"), "write timeout script");
    request.TimeoutMs = 100;
    result = Salamatrix::Runtime::RuntimeExecutionResult();
    Check(adapter.Execute(&request, &result) == FALSE, "python timeout returns false");
    Check(result.Status == Salamatrix::Runtime::RuntimeExecutionStatusCancelled,
          "python timeout status");
    DeleteFileW(script);
}

void RunPowerShellTest()
{
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"pwsh.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: pwsh.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_POWERSHELL", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L".ps1", script, _countof(script));
    Check(WriteScript(script, "Write-Output 'salamatrix-powershell-ok'\n"),
          "write powershell script");
    CAutomationProcessRuntimeAdapter adapter(
        "PowerShell",
        "PowerShell",
        "powershell",
        ".ps1",
        L"SALAMATRIX_POWERSHELL",
        L"pwsh.exe",
        L"powershell.exe",
        CAutomationProcessRuntimeAdapter::ProcessKindPowerShell);
    Check(adapter.IsAvailable() != FALSE, "powershell adapter available");
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = script;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "powershell execution succeeds");
    Check(wcsstr(result.Output, L"salamatrix-powershell-ok") != NULL,
          "powershell output captured");
    DeleteFileW(script);
}

void RunPhpTest()
{
    wchar_t interpreter[MAX_PATH * 4];
    if (!FindProgram(L"php.exe", interpreter, _countof(interpreter)))
    {
        std::fprintf(stderr, "SKIPPED: php.exe was not found.\n");
        return;
    }
    SetEnvironmentVariableW(L"SALAMATRIX_PHP", interpreter);
    wchar_t script[MAX_PATH];
    MakePath(L".php", script, _countof(script));
    Check(WriteScript(script, "<?php echo 'salamatrix-php-ok\\n'; ?>\n"),
          "write php script");
    CAutomationProcessRuntimeAdapter adapter(
        "PHP.CLI",
        "PHP",
        "php",
        ".php",
        L"SALAMATRIX_PHP",
        L"php.exe",
        NULL,
        CAutomationProcessRuntimeAdapter::ProcessKindPhp);
    Check(adapter.IsAvailable() != FALSE, "php adapter available");
    Salamatrix::Runtime::RuntimeExecutionRequest request;
    request.EntryPoint = script;
    request.TimeoutMs = 5000;
    Salamatrix::Runtime::RuntimeExecutionResult result;
    Check(adapter.Execute(&request, &result) != FALSE, "php execution succeeds");
    Check(wcsstr(result.Output, L"salamatrix-php-ok") != NULL,
          "php output captured");
    DeleteFileW(script);
}
} // namespace

int main()
{
    RunPythonTests();
    RunPowerShellTest();
    RunPhpTest();
    if (Failures != 0)
    {
        std::fprintf(stderr, "%d process runtime test(s) failed.\n", Failures);
        return 1;
    }
    std::fprintf(stderr, "All process runtime tests passed.\n");
    return 0;
}
