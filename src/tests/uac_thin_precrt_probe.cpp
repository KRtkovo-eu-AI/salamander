// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Probe acceptance hive before CRT constructors. | Validate shared test-process isolation without starting the product. | #731 | GPT-6 |
#include <windows.h>
#define SAM_UAC_THIN_COPY_TEST
#include "../uac_thin_test_hive.h"

extern "C" int WinMainCRTStartup();
#pragma comment(linker, "/ENTRY:UacThinProbeEntry")

static bool Reporter()
{
    WCHAR value[2];
    return GetEnvironmentVariableW(L"SAM_UAC_TEST_PRECRT_REPORTER", value, 2) == 1 && value[0] == L'1';
}

extern "C" int UacThinProbeEntry()
{
    if (!UacThinTestHive::Enter(Reporter())) ExitProcess(70);
    return WinMainCRTStartup();
}

static bool Mark(const WCHAR* host, const WCHAR* reporter)
{
    WCHAR nonce[33];
    if (GetEnvironmentVariableW(L"SAM_UAC_TEST_ISOLATION_NONCE", nonce, _countof(nonce)) != 32) return false;
    WCHAR path[96];
    static const WCHAR prefix[] = L"Software\\SamandarinThinProbe-";
    DWORD index = 0;
    for (; index + 1 < _countof(prefix); ++index) path[index] = prefix[index];
    for (DWORD i = 0; i < 32; ++i) path[index++] = nonce[i];
    path[index] = 0;
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path, 0, KEY_WRITE, &key) != ERROR_SUCCESS) return false;
    const DWORD value = 1;
    const LSTATUS status = RegSetValueExW(key, Reporter() ? reporter : host, 0, REG_DWORD,
                                          reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

static bool constructed = false;
struct ProbeGlobal
{
    ProbeGlobal()
    {
        constructed = Mark(L"HostCtor", L"SalmonCtor");
        static const char ready[] = "READY\n";
        DWORD count = 0;
        if (!constructed || !WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), ready, sizeof(ready) - 1, &count, nullptr))
            ExitProcess(72);
        char next = 0;
        if (!ReadFile(GetStdHandle(STD_INPUT_HANDLE), &next, 1, &count, nullptr) || count != 1 || next != 'X')
            ExitProcess(73);
    }
    ~ProbeGlobal() { Mark(L"HostDtor", L"SalmonDtor"); }
};
static ProbeGlobal globalProbe;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return constructed && Mark(L"HostMain", L"SalmonMain") ? 0 : 74;
}
