// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-09 | Provide the ZIP range test with SDK tracing. | Exercise the real plug-in export outside the file manager. | | Codex |

#include <windows.h>
#include <commctrl.h>
#include <cstdarg>

#pragma warning(push)
#pragma warning(disable : 4121 4201) // Packed SDK structures and anonymous unions.
#include "../plugins/shared/spl_com.h"
#pragma warning(pop)
#include "../plugins/shared/spl_vers.h"
#include "../plugins/shared/spl_base.h"

namespace
{
class RangeTestDebug final : public CSalamanderDebugAbstract
{
    void WINAPI TraceI(const char*, int, const char*) override {}
    void WINAPI TraceIW(const WCHAR*, int, const WCHAR*) override {}
    void WINAPI TraceE(const char*, int, const char*) override {}
    void WINAPI TraceEW(const WCHAR*, int, const WCHAR*) override {}
    void WINAPI TraceAttachThread(HANDLE, unsigned) override {}
    void WINAPI TraceSetThreadName(const char*) override {}
    void WINAPI TraceSetThreadNameW(const WCHAR*) override {}
    unsigned WINAPI CallWithCallStack(unsigned(WINAPI* body)(void*), void* arg) override
    {
        return body(arg);
    }
    void WINAPI Push(const char*, va_list, CCallStackMsgContext*, BOOL) override {}
    void WINAPI Pop(CCallStackMsgContext*) override {}
    void WINAPI SetThreadNameInVC(const char*) override {}
    void WINAPI SetThreadNameInVCAndTrace(const char*) override {}
    void WINAPI TraceConnectToServer() override {}
    void WINAPI AddModuleWithPossibleMemoryLeaks(const char*) override {}
};

class RangeTestEntry final : public CSalamanderPluginEntryAbstract
{
public:
    bool LanguageRequested = false;

private:
    RangeTestDebug Debug;
    int WINAPI GetVersion() override { return LAST_VERSION_OF_SALAMANDER; }
    HWND WINAPI GetParentWindow() override { return nullptr; }
    CSalamanderDebugAbstract* WINAPI GetSalamanderDebug() override { return &Debug; }
    HINSTANCE WINAPI LoadLanguageModule(HWND, const char*) override
    {
        // The range export needs tracing only. Stop before the panel services
        // and language resources that a full Salamander host supplies.
        LanguageRequested = true;
        return nullptr;
    }
    BOOL WINAPI SetBasicPluginData(const char*, DWORD, const char*, const char*,
                                   const char*, const char*, const char*, const char*) override { return FALSE; }
    CSalamanderGeneralAbstract* WINAPI GetSalamanderGeneral() override { return nullptr; }
    DWORD WINAPI GetLoadInformation() override { return 0; }
    WORD WINAPI GetCurrentSalamanderLanguageID() override { return 0; }
    CSalamanderGUIAbstract* WINAPI GetSalamanderGUI() override { return nullptr; }
    CSalamanderSafeFileAbstract* WINAPI GetSalamanderSafeFile() override { return nullptr; }
    void WINAPI SetPluginHomePageURL(const char*) override {}
    BOOL WINAPI AddFSName(const char*, int*) override { return FALSE; }
};
}

extern "C" __declspec(dllexport) BOOL InitializeZipRangeProbe(FSalamanderPluginEntry entry)
{
    static RangeTestEntry host;
    host.LanguageRequested = false;
    return entry(&host) == nullptr && host.LanguageRequested;
}
