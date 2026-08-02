// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <string>
#include <vector>
#include <windows.h>

#include "../python_executable_discovery.h"

namespace
{
int Failures = 0;
std::wstring AcceptedPath;
std::vector<std::wstring> ValidatedPaths;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}

bool AcceptOnlySelectedPath(const std::wstring& path)
{
    ValidatedPaths.push_back(path);
    return _wcsicmp(path.c_str(), AcceptedPath.c_str()) == 0;
}

std::wstring CurrentExecutablePath()
{
    std::vector<wchar_t> buffer(MAX_PATH);
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        DWORD length = GetModuleFileNameW(
            NULL, &buffer[0], static_cast<DWORD>(buffer.size()));
        if (length == 0)
            return std::wstring();
        if (length < buffer.size() - 1)
            return std::wstring(&buffer[0], length);
        buffer.resize(buffer.size() * 2);
    }
    return std::wstring();
}

std::wstring JoinPath(
    const std::wstring& directory,
    const wchar_t* name)
{
    std::wstring result(directory);
    if (!result.empty() && result.back() != L'\\')
        result.push_back(L'\\');
    result.append(name);
    return result;
}

void TestProbe(const std::wstring& executable)
{
    SetEnvironmentVariableW(
        L"SALAMATRIX_PYTHON_PROBE_TEST_MODE", L"success");
    Check(
        PythonExecutableDiscovery::IsUsablePythonInterpreter(executable),
        "successful Python probe fixture was rejected");

    SetEnvironmentVariableW(
        L"SALAMATRIX_PYTHON_PROBE_TEST_MODE", L"failure");
    Check(
        !PythonExecutableDiscovery::IsUsablePythonInterpreter(executable),
        "nonzero Python probe fixture was accepted");

    SetEnvironmentVariableW(
        L"SALAMATRIX_PYTHON_PROBE_TEST_MODE", L"timeout");
    ULONGLONG started = GetTickCount64();
    Check(
        !PythonExecutableDiscovery::IsUsablePythonInterpreter(executable),
        "timed out Python probe fixture was accepted");
    ULONGLONG elapsed = GetTickCount64() - started;
    Check(elapsed >= PythonExecutableDiscovery::PythonProbeTimeoutMs,
          "Python probe returned before its timeout");
    Check(elapsed < PythonExecutableDiscovery::PythonProbeTimeoutMs + 2000,
          "Python probe did not terminate its timed out child promptly");
    SetEnvironmentVariableW(L"SALAMATRIX_PYTHON_PROBE_TEST_MODE", NULL);
}

void TestPathContinuation(const std::wstring& executable)
{
    wchar_t temporaryPath[MAX_PATH] = {};
    Check(GetTempPathW(_countof(temporaryPath), temporaryPath) != 0,
          "temporary path is unavailable");
    wchar_t directoryName[96] = {};
    _snwprintf_s(
        directoryName,
        _countof(directoryName),
        _TRUNCATE,
        L"salamatrix-python-discovery-%lu-%llu",
        GetCurrentProcessId(),
        static_cast<unsigned long long>(GetTickCount64()));
    std::wstring root = JoinPath(temporaryPath, directoryName);
    std::wstring firstDirectory = JoinPath(root, L"first");
    std::wstring secondDirectory = JoinPath(root, L"second");
    Check(CreateDirectoryW(root.c_str(), NULL) != FALSE,
          "test root directory was not created");
    Check(CreateDirectoryW(firstDirectory.c_str(), NULL) != FALSE,
          "first PATH directory was not created");
    Check(CreateDirectoryW(secondDirectory.c_str(), NULL) != FALSE,
          "second PATH directory was not created");

    std::wstring firstPython = JoinPath(firstDirectory, L"python.exe");
    std::wstring secondPython = JoinPath(secondDirectory, L"python.exe");
    Check(CopyFileW(executable.c_str(), firstPython.c_str(), FALSE) != FALSE,
          "first Python fixture was not copied");
    Check(CopyFileW(executable.c_str(), secondPython.c_str(), FALSE) != FALSE,
          "second Python fixture was not copied");

    AcceptedPath = secondPython;
    ValidatedPaths.clear();
    std::wstring pathValue = L"\"" + firstDirectory + L"\";" +
                             secondDirectory;
    std::wstring result;
    Check(
        PythonExecutableDiscovery::FindUsableExecutable(
            L"python.exe",
            pathValue,
            AcceptOnlySelectedPath,
            result),
        "PATH search stopped after the first unusable executable");
    Check(_wcsicmp(result.c_str(), secondPython.c_str()) == 0,
          "PATH search did not return the later usable executable");
    Check(ValidatedPaths.size() >= 2,
          "PATH search did not validate both executable candidates");

    DeleteFileW(firstPython.c_str());
    DeleteFileW(secondPython.c_str());
    RemoveDirectoryW(firstDirectory.c_str());
    RemoveDirectoryW(secondDirectory.c_str());
    RemoveDirectoryW(root.c_str());
}
} // namespace

int wmain(int argc, wchar_t** argv)
{
    if (argc > 1 && wcscmp(argv[1], L"-I") == 0)
    {
        wchar_t mode[32] = {};
        GetEnvironmentVariableW(
            L"SALAMATRIX_PYTHON_PROBE_TEST_MODE", mode, _countof(mode));
        if (wcscmp(mode, L"success") == 0)
            return 0;
        if (wcscmp(mode, L"timeout") == 0)
            Sleep(PythonExecutableDiscovery::PythonProbeTimeoutMs + 5000);
        return 7;
    }

    std::wstring executable = CurrentExecutablePath();
    Check(!executable.empty(), "test executable path is unavailable");
    if (!executable.empty())
    {
        TestProbe(executable);
        TestPathContinuation(executable);
    }

    if (Failures != 0)
    {
        std::fprintf(stderr, "%d Python executable discovery test(s) failed.\n",
                     Failures);
        return 1;
    }
    std::puts("Python executable discovery tests passed.");
    return 0;
}
