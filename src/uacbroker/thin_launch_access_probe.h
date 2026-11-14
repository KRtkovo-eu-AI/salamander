// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

// Laboratory builds only. The caller must pre-create all events in the
// interactive session; no file, registry entry, or user setting is created.
// Event observations are diagnostic and are not peer authentication.
#if defined(SAM_UAC_F7_HOST) && defined(SAM_UAC_THIN_COPY_TEST)
namespace UacThinLaunchProbe
{
struct ObjectBasic
{
    ULONG attributes;
    ACCESS_MASK granted;
    ULONG handleCount, pointerCount, paged, nonPaged;
    ULONG reserved[3], nameInfo, typeInfo, securityInfo;
    LARGE_INTEGER created;
};
using QueryObject = LONG (NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);

inline bool Signal(const std::wstring& name)
{
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, name.c_str());
    if (!event) return false;
    const BOOL signaled = SetEvent(event);
    CloseHandle(event);
    return signaled != FALSE;
}

inline void Observe(HANDLE process) noexcept
{
    const DWORD saved = GetLastError();
    try
    {
        wchar_t base[192]{};
        const DWORD count = GetEnvironmentVariableW(L"SAM_UAC_TEST_LAUNCH_MASK_BASE", base, ARRAYSIZE(base));
        if (!count || count >= ARRAYSIZE(base)) { SetLastError(saved); return; }
        const std::wstring prefix(base, count);
        ObjectBasic basic{};
        ULONG returned = 0;
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        const auto query = ntdll ? reinterpret_cast<QueryObject>(GetProcAddress(ntdll, "NtQueryObject")) : nullptr;
        const bool valid = query && process && query(process, 0, &basic, sizeof(basic), &returned) >= 0 &&
                           returned >= sizeof(basic.attributes) + sizeof(basic.granted);
        bool complete = valid;
        if (valid)
            for (unsigned bit = 0; bit != 32; ++bit)
                if (basic.granted & (DWORD(1) << bit))
                    complete = Signal(prefix + L"-bit-" + std::to_wstring(bit)) && complete;
        if (!complete) Signal(prefix + L"-error");
        Signal(prefix + L"-ready"); // Written last; observer must also check -error.
    }
    catch (...) {} // Diagnostics must not change the broker launch outcome.
    SetLastError(saved);
}
}
#endif
