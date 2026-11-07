// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>
#include "../workerpath.h"
#include "native_path_test_support.h"
#include "../fileactionpath.h"
using Salamander::FileActionPaths::Utf8;
static int Failed = 0;
static void Check(bool ok, const char* name)
{
    std::printf("%s: %s (error %lu)\n", ok ? "PASS" : "FAIL", name, ok ? 0UL : GetLastError());
    if (!ok) ++Failed;
}
struct NativeStreamInfo
{
    ULONG NextEntry, NameLength;
    LARGE_INTEGER Size, AllocationSize;
    WCHAR Name[1];
};
static bool Streams(const std::wstring& file, std::vector<std::wstring>& names)
{
    const std::string utf8 = Utf8(file);
    HANDLE input = WorkerOpenFile(utf8.c_str(), NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);
    if (input == INVALID_HANDLE_VALUE) return false;
    typedef LONG(NTAPI* QueryInfo)(HANDLE, PVOID, PVOID, ULONG, ULONG);
    QueryInfo query = reinterpret_cast<QueryInfo>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationFile"));
    ULONG_PTR status[2] = {};
    std::vector<BYTE> bytes(65535);
    const LONG result = query(input, status, bytes.data(), (ULONG)bytes.size(), 22 /*FileStreamInformation*/);
    CloseHandle(input);
    if (result != 0) return false;
    if (status[1] == 0) return true;
    NativeStreamInfo* info = reinterpret_cast<NativeStreamInfo*>(bytes.data());
    for (;;)
    {
        names.push_back(std::wstring(info->Name, info->NameLength / sizeof(wchar_t)));
        if (info->NextEntry == 0) break;
        info = reinterpret_cast<NativeStreamInfo*>(reinterpret_cast<BYTE*>(info) + info->NextEntry);
    }
    return true;
}
static bool Write(const std::wstring& file, const std::string& content)
{
    HANDLE output = WorkerOpenFile(Utf8(file).c_str(), file.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if (output == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    BOOL ok = WriteFile(output, content.data(), (DWORD)content.size(), &written, NULL);
    CloseHandle(output);
    return ok && written == content.size();
}
static bool Read(const std::wstring& file, std::string& content)
{
    HANDLE input = WorkerOpenFile(Utf8(file).c_str(), NULL, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (input == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(input, NULL), read = 0;
    if (size == INVALID_FILE_SIZE || size > 1024 * 1024) { CloseHandle(input); return false; }
    content.resize(size);
    BOOL ok = size == 0 || ReadFile(input, &content[0], size, &read, NULL);
    CloseHandle(input);
    return ok && read == size;
}
static bool CopyReopen(const std::wstring& source, const std::wstring& target)
{
    // First inspect source as the ADS preflight does, close it, then reopen for data.
    HANDLE input = WorkerOpenFile(Utf8(source).c_str(), source.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);
    if (input == INVALID_HANDLE_VALUE) return false;
    CloseHandle(input);
    std::string original, copied;
    if (!Read(source, original) || !Write(target, original)) return false;
    // Simulate the overwrite/retry path: reopen both files, truncate and rewrite.
    if (!Read(source, copied) || copied != original || !Write(target, copied)) return false;
    return Read(target, copied) && copied == original;
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 1 && argc != 3) return 2;
    const std::wstring parent = NativePathTestParent(argc, argv);
    if (parent.empty()) return 2;
    const std::wstring root = parent + L"\\copy-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64());
    std::wstring actualFixture = argc == 3 ? argv[2] : L"";
    std::vector<std::wstring> dirs, files;
    auto make = [&dirs](const std::wstring& path) {
        DWORD error = 0;
        BOOL ok = WorkerCreateDirectory(Utf8(path).c_str(), &error, (L"\\\\?\\" + path).c_str());
        if (ok) dirs.push_back(path);
        return ok != FALSE;
    };
    if (!make(root)) return 3;
    std::wstring deep = root;
    bool made = true;
    for (int i = 0; i < 8; ++i)
    {
        deep += L"\\日本-žluťoučký-😀-" + std::wstring(65, L'a') + std::to_wstring(i);
        if (!make(deep)) { made = false; break; }
    }
    Check(made && deep.size() > 2 * MAX_PATH, "fixture exceeds old ADS full-path buffer");
    if (actualFixture.empty())
    {
        actualFixture = deep + L"\\fixture-日本-č-😀.txt";
        if (Write(actualFixture, "independent long Unicode regression fixture\n"))
            files.push_back(actualFixture);
        else
            Check(false, "create independent long Unicode source fixture");
    }
    std::vector<std::wstring> nativeNames;
    Check(Streams(actualFixture, nativeNames), "GUI regression fixture opens for native ADS preflight");
    const std::wstring actualTarget = deep + L"\\long-unicode-č.txt";
    files.push_back(actualTarget);
    Check(CopyReopen(actualFixture, actualTarget), "GUI source copied, reopened and overwritten at exact long target");
    const std::wstring source = deep + L"\\source-日本-😀.txt";
    const std::wstring target = deep + L"\\target-日本-😀.txt";
    files.push_back(source); files.push_back(target);
    Check(Write(source, "default stream exact bytes\n"), "create deep Unicode default data stream");
    const std::wstring stream = L":保護-č-😀:$DATA";
    const std::wstring sourceStream = WorkerOperationPathW(Utf8(source).c_str()) + stream;
    const std::wstring targetStream = WorkerOperationPathW(Utf8(target).c_str()) + stream;
    Check(Write(sourceStream, "named stream must survive\n"), "create deep Unicode named data stream");
    nativeNames.clear();
    bool named = false;
    if (Streams(source, nativeNames)) for (const auto& name : nativeNames) if (name == stream) named = true;
    Check(named, "NtQueryInformationFile reports exact Unicode stream name on long source");
    Check(CopyReopen(source, target), "copy/retry/overwrite default stream with UTF-8 fallback and owned-wide opens");
    Check(CopyReopen(sourceStream, targetStream), "copy/retry Unicode ADS beyond the old 2*MAX_PATH limit");
    const std::wstring targetApi = WorkerOperationPathW(Utf8(target).c_str());
    bool attrs = SetFileAttributesW(targetApi.c_str(), FILE_ATTRIBUTE_READONLY) != FALSE;
    attrs = WorkerClearReadOnlyW(targetApi) != FALSE && attrs;
    Check(attrs && CopyReopen(source, target), "readonly overwrite recovery keeps the exact target");
    Check(Write(targetStream, "stream after overwrite") && DeleteFileW(targetStream.c_str()) != FALSE,
          "delete overwritten ADS using a dynamic wide full path");
    bool clean = true;
    for (const auto& file : files) clean = DeleteFileW(WorkerOperationPathW(Utf8(file).c_str()).c_str()) != FALSE && clean;
    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) clean = RemoveDirectoryW((L"\\\\?\\" + *it).c_str()) != FALSE && clean;
    Check(clean, "cleanup only isolated created files and directories");
    return Failed == 0 ? 0 : 1;
}
