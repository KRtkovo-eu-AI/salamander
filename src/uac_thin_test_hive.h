// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Isolate acceptance-only processes before their CRT starts. | Keep test host and reporter writes out of the user's HKCU. | #731 | GPT-6 |
#pragma once

// Test builds only. This header runs before the CRT: no C++ objects, heap,
// C runtime, tracing or handles wrappers may be used here. Each process maps
// HKCU independently; the mapping is not inherited by spawned processes.
#ifdef SAM_UAC_THIN_COPY_TEST
namespace UacThinTestHive
{
using LoadAppKey = LSTATUS(WINAPI*)(LPCWSTR, PHKEY, REGSAM, DWORD, DWORD);
using OverridePredefined = LSTATUS(WINAPI*)(HKEY, HKEY);

inline bool Enter(bool reporter)
{
    // The fixed hive is always inside this test executable's app directory;
    // a reporter lives in its utils child. Never accept an arbitrary hive path.
    static WCHAR path[32768];
    const DWORD length = GetModuleFileNameW(nullptr, path, _countof(path));
    if (!length || length >= _countof(path)) return false;
    DWORD end = length;
    while (end && path[end - 1] != L'\\') --end;
    if (!end) return false;
    --end; // drop final separator
    if (reporter)
    {
        DWORD start = end;
        while (start && path[start - 1] != L'\\') --start;
        if (!start || end - start != 5) return false;
        static const WCHAR utils[] = L"utils";
        for (DWORD i = 0; i < 5; ++i)
        {
            WCHAR value = path[start + i];
            if (value >= L'A' && value <= L'Z') value += L'a' - L'A';
            if (value != utils[i]) return false;
        }
        end = start - 1;
    }
    static const WCHAR suffix[] = L"\\uac-test-hkcu.dat";
    if (end + _countof(suffix) > _countof(path)) return false;
    for (DWORD i = 0; i < _countof(suffix); ++i) path[end + i] = suffix[i];

    // The first test host creates a fresh hive, its own reporter must open it.
    const DWORD attributes = GetFileAttributesW(path);
    if ((!reporter && (attributes != INVALID_FILE_ATTRIBUTES || GetLastError() != ERROR_FILE_NOT_FOUND)) ||
        (reporter && (attributes == INVALID_FILE_ATTRIBUTES ||
                      (attributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)))))
        return false;
    HMODULE advapi = GetModuleHandleW(L"advapi32.dll");
    if (!advapi) return false;
    const auto load = reinterpret_cast<LoadAppKey>(GetProcAddress(advapi, "RegLoadAppKeyW"));
    const auto overrideKey = reinterpret_cast<OverridePredefined>(GetProcAddress(advapi, "RegOverridePredefKey"));
    if (!load || !overrideKey) return false;
    HKEY hive = nullptr;
    if (load(path, &hive, KEY_ALL_ACCESS, 0, 0) != ERROR_SUCCESS) return false;
    const LSTATUS mapped = overrideKey(HKEY_CURRENT_USER, hive);
    RegCloseKey(hive); // the predefined mapping retains its own reference
    if (mapped != ERROR_SUCCESS) return false;

    // An unguessable marker proves which test processes reached this point.
    // The runner verifies it through the private hive and independently
    // verifies that the same key does not appear in its real HKCU.
    WCHAR nonce[33];
    if (GetEnvironmentVariableW(L"SAM_UAC_TEST_ISOLATION_NONCE", nonce, _countof(nonce)) != 32) return false;
    for (DWORD i = 0; i < 32; ++i)
        if (!((nonce[i] >= L'0' && nonce[i] <= L'9') || (nonce[i] >= L'a' && nonce[i] <= L'f')))
            return false;
    WCHAR keyName[96];
    static const WCHAR prefix[] = L"Software\\SamandarinThinProbe-";
    DWORD cursor = 0;
    for (; cursor + 1 < _countof(prefix); ++cursor) keyName[cursor] = prefix[cursor];
    for (DWORD i = 0; i < 32; ++i) keyName[cursor++] = nonce[i];
    keyName[cursor] = 0;
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, keyName, 0, nullptr, 0, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        return false;
    const DWORD value = 1;
    const LSTATUS written = RegSetValueExW(key, reporter ? L"Salmon" : L"Host", 0, REG_DWORD,
                                           reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
    return written == ERROR_SUCCESS;
}
}
#endif
