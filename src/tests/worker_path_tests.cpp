// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>
#include "../workerpath.h"
#include "native_path_test_support.h"
static int Failed = 0;
static void Check(bool ok, const char* name)
{
    std::printf("%s: %s (error %lu)\n", ok ? "PASS" : "FAIL", name, ok ? 0UL : GetLastError());
    if (!ok) ++Failed;
}
static std::string Utf8(const std::wstring& path)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, NULL, 0, NULL, NULL);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, &result[0], size, NULL, NULL);
    result.resize(size - 1);
    return result;
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 1 && argc != 2) return 2;
    std::wstring root = NativePathTestParent(argc, argv);
    if (root.empty()) return 2;
    root += L"\\worker-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    std::vector<std::wstring> dirs;
    auto make = [&dirs](const std::wstring& path) {
        DWORD error = 0;
        // A test binary without longPathAware can hit CreateDirectoryW's older
        // 248-character limit before MAX_PATH; explicit wide fixture paths avoid
        // making setup depend on the length of the runner's temporary directory.
        const std::wstring fixturePath = L"\\\\?\\" + path;
        BOOL ok = WorkerCreateDirectory(Utf8(path).c_str(), &error, fixturePath.c_str());
        if (ok) dirs.push_back(path);
        else SetLastError(error);
        return ok != FALSE;
    };
    if (!make(root)) return 3;
    std::wstring unicodeDir = root + L"\\\u65e5\u672c-\u010desk\u00fd-\U0001f600";
    Check(make(unicodeDir), "create directory with CJK, diacritics and surrogate pair");
    DWORD error = 0;
    Check(!WorkerCreateDirectory(Utf8(unicodeDir).c_str(), &error) && error == ERROR_ALREADY_EXISTS,
          "existing directory preserves collision result");
    std::wstring byteLongDir = root + L"\\" + std::wstring(100, L'\u65e5');
    Check(Utf8(byteLongDir).size() > MAX_PATH && byteLongDir.size() < MAX_PATH && make(byteLongDir),
          "create short UTF-16 directory whose UTF-8 exceeds MAX_PATH");
    std::wstring longDir = unicodeDir;
    bool madeLong = true;
    for (int i = 0; i < 8; ++i)
    {
        longDir += L"\\" + std::wstring(45, L'\u65e5') + std::to_wstring(i);
        if (!make(longDir)) { madeLong = false; break; }
    }
    Check(madeLong && longDir.size() > MAX_PATH, "create nested Win32 long path");
    std::vector<std::wstring> parents = {root, unicodeDir, byteLongDir, longDir};
    bool ioOk = true;
    for (const auto& parent : parents)
    {
        std::wstring file = parent + L"\\\u65e5\U0001f600.txt";
        std::wstring io = WorkerOperationPathW(Utf8(file).c_str());
        HANDLE h = CreateFileW(io.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) { ioOk = false; continue; }
        FILETIME time; GetSystemTimeAsFileTime(&time);
        ioOk = SetFileTime(h, &time, &time, &time) != FALSE && ioOk;
        CloseHandle(h);
        ioOk = SetFileAttributesW(io.c_str(), FILE_ATTRIBUTE_READONLY) != FALSE && ioOk;
        WorkerClearReadOnlyW(io);
        ioOk = (GetFileAttributesW(io.c_str()) & FILE_ATTRIBUTE_READONLY) == 0 && ioOk;
        ioOk = DeleteFileW(io.c_str()) != FALSE && ioOk;
    }
    Check(ioOk, "Unicode and long-path attributes, timestamps, readonly clear and delete");
    std::wstring exact = unicodeDir + L"\\owned-wide";
    Check(WorkerCreateDirectory("incorrect-legacy-name", &error, exact.c_str()) != FALSE,
          "directory operation prefers owned wide path over legacy text");
    dirs.push_back(exact);
    std::wstring trailing = root + L"\\exact.";
    Check(make(trailing) && GetFileAttributesW((root + L"\\exact").c_str()) == INVALID_FILE_ATTRIBUTES,
          "trailing-dot directory is never normalized to another name");
    Check(WorkerOperationPathW("ignored", (L"\\\\server\\share\\" + std::wstring(300, L'a')).c_str()).rfind(L"\\\\?\\UNC\\server\\share\\", 0) == 0,
          "long UNC path receives the UNC extended prefix");
    bool cleanup = true;
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it)
    {
        if (it->rfind(root, 0) != 0) { cleanup = false; continue; }
        cleanup = RemoveDirectoryW(WorkerOperationPathW(Utf8(*it).c_str()).c_str()) != FALSE && cleanup;
    }
    Check(cleanup, "remove only directories created under this isolated test root");
    return Failed == 0 ? 0 : 1;
}
