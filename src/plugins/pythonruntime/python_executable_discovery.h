// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace PythonExecutableDiscovery
{
typedef bool (*ExecutableValidator)(const std::wstring& path);

const DWORD PythonProbeTimeoutMs = 3000;

inline std::wstring ToWin32Path(const std::wstring& value)
{
    if (value.size() < MAX_PATH || value.compare(0, 4, L"\\\\?\\") == 0)
        return value;
    if (value.size() >= 2 && value[0] == L'\\' && value[1] == L'\\')
        return L"\\\\?\\UNC\\" + value.substr(2);
    return L"\\\\?\\" + value;
}

inline bool IsRegularFile(const std::wstring& path)
{
    if (path.empty())
        return false;
    std::wstring win32Path = ToWin32Path(path);
    DWORD attributes = GetFileAttributesW(win32Path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

inline bool AppendQuotedArgument(
    std::wstring& command,
    const std::wstring& value)
{
    if (value.find(L'"') != std::wstring::npos)
        return false;
    command.push_back(L'"');
    command.append(value);
    command.push_back(L'"');
    return true;
}

inline bool IsUsablePythonInterpreter(const std::wstring& path)
{
    if (!IsRegularFile(path))
        return false;

    std::wstring command;
    if (!AppendQuotedArgument(command, path))
        return false;
    // Windows' uninstalled Python Store placeholder returns an error when it
    // receives command-line arguments, instead of opening the Store UI.  A
    // working classic or Store Python executes this isolated version probe.
    command.append(
        L" -I -S -c \"import sys;raise SystemExit(0 if sys.version_info.major == 3 else 1)\"");
    std::vector<wchar_t> commandLine(command.begin(), command.end());
    commandLine.push_back(L'\0');

    STARTUPINFOW startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process = {};
    BOOL created = CreateProcessW(
        NULL,
        &commandLine[0],
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &startup,
        &process);
    if (!created)
        return false;

    DWORD waitResult = WaitForSingleObject(
        process.hProcess, PythonProbeTimeoutMs);
    if (waitResult != WAIT_OBJECT_0)
    {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, 1000);
    }

    DWORD exitCode = 1;
    bool usable = waitResult == WAIT_OBJECT_0 &&
                  GetExitCodeProcess(process.hProcess, &exitCode) &&
                  exitCode == 0;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return usable;
}

inline bool PathEquals(
    const std::wstring& left,
    const std::wstring& right)
{
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

inline bool TryCandidate(
    const std::wstring& candidate,
    ExecutableValidator validator,
    std::vector<std::wstring>& tested,
    std::wstring& result)
{
    if (candidate.empty())
        return false;
    for (size_t index = 0; index < tested.size(); ++index)
    {
        if (PathEquals(tested[index], candidate))
            return false;
    }
    tested.push_back(candidate);
    if (!IsRegularFile(candidate) ||
        (validator != NULL && !validator(candidate)))
        return false;
    result.assign(candidate);
    return true;
}

inline bool SearchPathString(
    const wchar_t* fileName,
    std::wstring& value)
{
    value.clear();
    DWORD capacity = MAX_PATH;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        std::vector<wchar_t> buffer(static_cast<size_t>(capacity));
        DWORD length = SearchPathW(
            NULL, fileName, NULL, capacity, &buffer[0], NULL);
        if (length == 0)
            return false;
        if (length < capacity)
        {
            value.assign(&buffer[0], length);
            return true;
        }
        capacity = length + 1;
    }
    return false;
}

inline std::wstring TrimPathEntry(const std::wstring& value)
{
    const wchar_t* whitespace = L" \t\r\n";
    size_t start = value.find_first_not_of(whitespace);
    if (start == std::wstring::npos)
        return std::wstring();
    size_t end = value.find_last_not_of(whitespace);
    std::wstring result = value.substr(start, end - start + 1);
    if (result.size() >= 2 && result.front() == L'"' &&
        result.back() == L'"')
        result = result.substr(1, result.size() - 2);
    return result;
}

inline bool FindUsableExecutableInPath(
    const wchar_t* fileName,
    const std::wstring& pathValue,
    ExecutableValidator validator,
    std::vector<std::wstring>& tested,
    std::wstring& result)
{
    if (fileName == NULL || fileName[0] == L'\0')
        return false;
    size_t start = 0;
    while (start <= pathValue.size())
    {
        size_t separator = pathValue.find(L';', start);
        std::wstring directory = TrimPathEntry(pathValue.substr(
            start,
            separator == std::wstring::npos
                ? std::wstring::npos
                : separator - start));
        if (!directory.empty())
        {
            if (directory.back() != L'\\' && directory.back() != L'/')
                directory.push_back(L'\\');
            std::wstring candidate = directory + fileName;
            if (TryCandidate(candidate, validator, tested, result))
                return true;
        }
        if (separator == std::wstring::npos)
            break;
        start = separator + 1;
    }
    return false;
}

inline bool FindUsableExecutable(
    const wchar_t* fileName,
    const std::wstring& pathValue,
    ExecutableValidator validator,
    std::wstring& result)
{
    result.clear();
    if (fileName == NULL || fileName[0] == L'\0')
        return false;

    std::vector<std::wstring> tested;
    if (TryCandidate(fileName, validator, tested, result))
        return true;

    const bool hasDirectory = wcschr(fileName, L'\\') != NULL ||
                              wcschr(fileName, L'/') != NULL ||
                              wcschr(fileName, L':') != NULL;
    if (hasDirectory)
        return false;

    std::wstring searchPathResult;
    if (SearchPathString(fileName, searchPathResult) &&
        TryCandidate(searchPathResult, validator, tested, result))
        return true;

    return FindUsableExecutableInPath(
        fileName, pathValue, validator, tested, result);
}
} // namespace PythonExecutableDiscovery
