// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../drivefreespace.h"
#include "../drivefreespace_cache.h"
#include <cstdio>
#include <string>
#include <vector>
#include <tlhelp32.h>

using namespace DriveFreeSpaceDetail;

namespace
{
int Failures = 0;
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        ++Failures;
        std::printf("FAIL: %s\n", description);
    }
}

void PolicyTests()
{
    Cache cache;
    const std::wstring medium = L"\\Device\\HarddiskVolume42:media-1";
    Check(!cache.Query(4, medium, 0, 0, false).Queued, "Off never schedules a probe");
    Check(cache.Reserve(0) == -1, "Off has no background work");
    cache.Query(4, medium, 1, 0, false);
    Check(cache.Reserve(10) == 4, "Once schedules first attempt");
    std::uint64_t token = cache.Get(4).Token;
    Check(cache.Complete(4, token, false, 0, 20, 0), "failed probe completes");
    Check(!cache.Query(4, medium, 1, 1000000, false).Queued,
          "Once does not retry a failed first attempt");
    cache.MarkStale(4);
    Check(!cache.Query(4, medium, 1, 2000000, false).Queued,
          "manual refresh cannot turn Once into Updated");

    cache.Invalidate(4);
    cache.Query(4, medium, 1, 0, false);
    cache.Reserve(0);
    token = cache.Get(4).Token;
    cache.Complete(4, token, true, 123, 10, 456);
    const Entry& once = cache.Query(4, medium, 1, 900000, false);
    Check(once.Available && once.Stale && once.Bytes == 123 && once.SuccessTime == 456,
          "Once retains timestamped stale snapshot");
    Check(!once.Queued, "Once success is never refreshed automatically");

    const Entry& updated = cache.Query(4, medium, 2, 900000, false);
    Check(updated.Queued && updated.Bytes == 123 && updated.Stale,
          "Updated keeps last known value while refreshing");
    cache.Reserve(900000);
    token = cache.Get(4).Token;
    cache.Complete(4, token, true, 789, 900010, 654);
    Check(!cache.Query(4, medium, 2, 960009, false).Queued, "Updated TTL is sixty seconds");
    Check(cache.Query(4, medium, 2, 960010, false).Queued, "Updated refreshes at TTL");
    cache.Reserve(960010);
    token = cache.Get(4).Token;
    cache.Complete(4, token, false, 0, 960020, 0);
    const Entry& stale = cache.Get(4);
    Check(stale.Available && stale.Stale && stale.Bytes == 789 && stale.SuccessTime == 654,
          "failure preserves last successful value and its timestamp");
    cache.MarkStale(4);
    Check(!cache.Query(4, medium, 2, 960030, false).Queued,
          "refresh notifications respect failure retry backoff");
    Check(cache.Query(4, medium, 2, 1020020, false).Queued,
          "Updated retries after failure backoff");
}

void IdentityAndCancellationTests()
{
    Cache cache;
    cache.Query(5, L"\\Device\\VolumeA", 1, 0, false);
    cache.Reserve(0);
    std::uint64_t old = cache.Get(5).Token;
    cache.Query(5, L"\\Device\\VolumeB", 1, 1, false);
    Check(!cache.Complete(5, old, true, 999, 2, 2), "remapped letter rejects old completion");
    Check(cache.Get(5).Queued && !cache.Get(5).Available, "new volume does not inherit old free space");
    cache.Reserve(2);
    old = cache.Get(5).Token;
    cache.Invalidate(5);
    Check(!cache.Complete(5, old, true, 999, 3, 3), "media epoch rejects in-flight completion");
    cache.Query(5, L"\\Device\\VolumeB", 1, 4, false);
    Check(cache.Get(5).Queued, "new medium under same NT device allows one new attempt");
    cache.Query(6, L"\\server\\share", 1, 4, false);
    cache.Reserve(4);
    cache.Reserve(4);
    std::uint64_t other = cache.Get(6).Token;
    cache.Invalidate(5);
    Check(cache.IsCurrent(6, other), "targeted invalidation preserves unrelated drive");
    cache.Query(6, L"\\server\\share", 0, 5, false);
    Check(!cache.IsCurrent(6, other), "Off cancels an active request");
    Check(!cache.Complete(6, other, true, 1, 6, 6), "late cancelled success is ignored");

    cache.Query(7, L"\\server\\offline", 1, 0, true);
    Check(!cache.Get(7).Queued && cache.Get(7).Attempted,
          "disconnected share is not touched and consumes Once attempt");
    Check(!cache.Query(7, L"\\server\\offline", 1, 70000, false).Queued,
          "Once does not reconnect remembered share on a later menu open");
    cache.Query(8, L"\\server\\offline", 2, 0, true);
    Check(!cache.Query(8, L"\\server\\offline", 2, 59000, false).Queued,
          "disconnected Updated share backs off");
    Check(cache.Query(8, L"\\server\\offline", 2, 60000, false).Queued,
          "Updated can probe after a connection is independently restored");
    cache.Invalidate();
    cache.Query(9, L"connected", 2, 0, false);
    cache.Reserve(0);
    std::uint64_t connected = cache.Get(9).Token;
    cache.Complete(9, connected, true, 42, 10, 10);
    Check(cache.Query(9, L"connected", 2, 20, true).Stale,
          "disconnect marks a fresh cached result stale before TTL expires");
    cache.Invalidate();
    cache.Query(9, L"connected", 1, 0, false);
    cache.Reserve(0);
    connected = cache.Get(9).Token;
    cache.Query(9, L"connected", 1, 1, true);
    Check(!cache.IsCurrent(9, connected) && !cache.Get(9).Queued && !cache.Get(9).Running,
          "disconnect immediately cancels an in-flight probe");
    Check(!cache.Complete(9, connected, true, 99, 2, 2),
          "disconnect rejects an in-flight successful result");
    Check(!cache.Query(9, L"connected", 1, 70000, false).Queued,
          "disconnect preserves Once attempted state");
}

void FakeSlowProbeTests()
{
    Cache cache;
    for (unsigned drive = 0; drive < 10; ++drive)
        cache.Query(drive, L"fake-provider:" + std::to_wstring(drive), 2, 0, false);
    int first = cache.Reserve(100);
    int second = cache.Reserve(100);
    Check(first == 0 && second == 1 && cache.Reserve(100) == -1,
          "fake never-ending probes cannot exceed concurrency two");
    std::uint64_t firstToken = cache.Get(first).Token;
    Check(!cache.Expired(first, firstToken, 3099), "slow probe still within deadline");
    Check(cache.Expired(first, firstToken, 3100), "slow probe expires at three seconds");
    cache.Complete(first, firstToken, false, 0, 3100, 0);
    Check(!cache.Get(first).Running && cache.Get(first).Failed,
          "timed-out fake probe becomes unavailable");
    Check(cache.Reserve(3101) == 2, "next queued drive can use reclaimed slot");
    Check(!cache.Complete(first, firstToken, true, 123, 3102, 3102),
          "late success after timeout cannot overwrite the cache");
    cache.Invalidate();
    Check(cache.Reserve(4000) == -1, "shutdown/invalidate cancels all queued fake probes");
}

unsigned ChildCount()
{
    std::vector<wchar_t> module(32768);
    DWORD moduleLength = GetModuleFileNameW(nullptr, module.data(), static_cast<DWORD>(module.size()));
    if (moduleLength == 0 || moduleLength >= module.size())
        return 0;
    const wchar_t* name = wcsrchr(module.data(), L'\\');
    name = name == nullptr ? module.data() : name + 1;
    unsigned count = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry))
        do
        {
            if (entry.th32ParentProcessID == GetCurrentProcessId() && _wcsicmp(entry.szExeFile, name) == 0)
            {
                HANDLE child = OpenProcess(SYNCHRONIZE, FALSE, entry.th32ProcessID);
                if (child != nullptr)
                {
                    // A debugger/security monitor can retain an exited process
                    // object. Count running children, not retained PID entries.
                    if (WaitForSingleObject(child, 0) == WAIT_TIMEOUT)
                        ++count;
                    CloseHandle(child);
                }
            }
        } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return count;
}

bool WaitForChildren(unsigned expected)
{
    for (unsigned i = 0; i < 80; ++i)
    {
        if (ChildCount() == expected)
            return true;
        Sleep(25);
    }
    return false;
}

void RealHelperSmokeTest()
{
    std::vector<wchar_t> systemDirectory(256);
    UINT length = GetSystemDirectoryW(systemDirectory.data(), static_cast<UINT>(systemDirectory.size()));
    if (length >= systemDirectory.size())
    {
        systemDirectory.resize(static_cast<size_t>(length) + 1);
        length = GetSystemDirectoryW(systemDirectory.data(), static_cast<UINT>(systemDirectory.size()));
    }
    if (length < 3 || length >= systemDirectory.size())
    {
        Check(false, "test system drive discovered");
        return;
    }
    wchar_t letter = systemDirectory[0];
    ULONGLONG started = GetTickCount64();
    DriveFreeSpaceValue value = DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr,
                                                    false, dfsOnce, nullptr, 0);
    Check(GetTickCount64() - started < 250, "real query returns without waiting for child probe");
    for (unsigned i = 0; value.Pending && i < 180; ++i)
    {
        Sleep(25);
        value = DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr, false, dfsOnce, nullptr, 0);
    }
    Check(value.Available && value.Bytes > 0 && value.Stale && !value.Pending,
          "real isolated helper returns quota-aware free space and Once snapshot");
    Check(value.LastSuccess.dwLowDateTime != 0 || value.LastSuccess.dwHighDateTime != 0,
          "real helper records successful wall clock timestamp");

    // The test executable itself stands in for an unresponsive filesystem
    // provider. Production service code and its process launcher are unchanged.
    SetEnvironmentVariableW(L"SAL_TEST_DRIVE_FREE_SPACE_HANG", L"1");
    DriveFreeSpaceInvalidate(letter);
    started = GetTickCount64();
    value = DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr, false, dfsUpdated, nullptr, 0);
    Check(WaitForChildren(1), "one isolated fake hung provider process starts");
    for (unsigned i = 0; value.Pending && i < 200; ++i)
    {
        Sleep(25);
        value = DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr, false, dfsUpdated, nullptr, 0);
    }
    ULONGLONG elapsed = GetTickCount64() - started;
    Check(!value.Pending && !value.Available && elapsed >= Deadline && elapsed < 5000,
          "actual never-returning helper is timed out within bounded deadline");
    Check(WaitForChildren(0), "timed-out process exits and leaves no child behind");
    std::printf("Real hung helper deadline: %llu ms\n", elapsed);

    DriveFreeSpaceInvalidate(letter);
    DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr, false, dfsOnce, nullptr, 0);
    Check(WaitForChildren(1), "second fake hung provider process starts");
    started = GetTickCount64();
    DriveFreeSpaceSetPolicies(dfsOff, dfsOff);
    Check(WaitForChildren(0), "Off promptly terminates an actual running helper");
    std::printf("Real Off cancellation: %llu ms\n", GetTickCount64() - started);
    value = DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr, false, dfsOff, nullptr, 0);
    Check(!value.Available && !value.Pending, "Off clears cached display and queued work");

    DriveFreeSpaceQuery(letter, DRIVE_REMOVABLE, nullptr, false, dfsUpdated, nullptr, 0);
    Check(WaitForChildren(1), "shutdown fake hung provider process starts");
    started = GetTickCount64();
    DriveFreeSpaceShutdown();
    Check(WaitForChildren(0), "shutdown kills an actual running helper without orphaning it");
    Check(GetTickCount64() - started < 2000, "shutdown does not wait for a filesystem provider");
    std::printf("Real shutdown: %llu ms\n", GetTickCount64() - started);
    SetEnvironmentVariableW(L"SAL_TEST_DRIVE_FREE_SPACE_HANG", nullptr);
}
}

int wmain(int argc, wchar_t** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    int helperResult = 0;
    if (wcsstr(GetCommandLineW(), L"--sal-drive-free-space-helper") != nullptr &&
        GetEnvironmentVariableW(L"SAL_TEST_DRIVE_FREE_SPACE_HANG", nullptr, 0) != 0)
        Sleep(30000); // Bounded test-only fake; the real service kills it at 3s.
    if (DriveFreeSpaceHelperDispatch(helperResult))
        return helperResult;
    // ProcessStartInfo/ShellExecute may reject long executable paths before
    // calling CreateProcess. Exercise the same explicit wide application path
    // used by the production launcher instead.
    if (argc == 3 && wcscmp(argv[1], L"--launch-test-executable") == 0)
    {
        std::wstring command = L"\"" + std::wstring(argv[2]) + L"\"";
        std::vector<wchar_t> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back(0);
        STARTUPINFOW startup = {};
        startup.cb = sizeof(startup);
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        PROCESS_INFORMATION process = {};
        if (!CreateProcessW(argv[2], mutableCommand.data(), nullptr, nullptr, TRUE,
                            CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        {
            std::printf("Long-path launch failed: %lu\n", GetLastError());
            return 1;
        }
        CloseHandle(process.hThread);
        DWORD result = 1;
        if (WaitForSingleObject(process.hProcess, 15000) == WAIT_OBJECT_0)
            GetExitCodeProcess(process.hProcess, &result);
        else
            TerminateProcess(process.hProcess, ERROR_TIMEOUT);
        CloseHandle(process.hProcess);
        std::printf("Explicit wide application-path child: %s\n", result == 0 ? "PASS" : "FAIL");
        return result == 0 ? 0 : 1;
    }
    PolicyTests();
    IdentityAndCancellationTests();
    FakeSlowProbeTests();
    RealHelperSmokeTest();
    std::printf("Drive free space tests: %s (%d failures)\n", Failures == 0 ? "PASS" : "FAIL", Failures);
    return Failures == 0 ? 0 : 1;
}
