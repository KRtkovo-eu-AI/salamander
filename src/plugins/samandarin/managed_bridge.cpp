// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Keep the CLR loaded after Samandarin shutdown and report Start failures. | ICLRRuntimeHost::Stop prevents later managed plugins from starting in this process. | | Grok 4.7 |

#include "precomp.h"
#include "managed_bridge.h"
#include "../../darkmode.h"
#include "../../common/winlibdpi.h"

#include <metahost.h>
#include <mscoree.h>
#include <strsafe.h>

#pragma comment(lib, "mscoree.lib")

extern HINSTANCE DLLInstance;

namespace
{
ICLRRuntimeHost* gRuntimeHost = nullptr;
std::wstring gAssemblyPath;
std::wstring gCurrentVersion;
bool gIsInitialized = false;
SRWLOCK gRuntimeLock = SRWLOCK_INIT;
HANDLE gInitializationThread = nullptr;
DWORD gInitializationThreadId = 0;
LONG gInitializationResult = 0;
const wchar_t* const kManagedType = L"OpenSalamander.Samandarin.EntryPoint";
const wchar_t* const kManagedMethod = L"Dispatch";
const wchar_t* const kPluginCaption = L"Samandarin Update Notifier";

HWND ResolveOwnerWindow(HWND parent)
{
    HWND foreground = GetForegroundWindow();
    if (foreground != nullptr)
    {
        HWND root = GetAncestor(foreground, GA_ROOT);
        if (root != nullptr)
        {
            foreground = root;
        }

        DWORD windowProcessId = 0;
        GetWindowThreadProcessId(foreground, &windowProcessId);
        if (windowProcessId == GetCurrentProcessId() && IsWindowVisible(foreground) && !IsIconic(foreground))
        {
            return foreground;
        }
    }

    return parent;
}

std::wstring BuildArgument(const wchar_t* command, HWND parent, const wchar_t* payload)
{
    std::wstring argument = command;
    argument.push_back(L';');

    wchar_t buffer[32];
    ULONGLONG handleValue = reinterpret_cast<ULONGLONG>(parent);
    StringCchPrintfW(buffer, _countof(buffer), L"%llu", handleValue);
    argument.append(buffer);

    argument.push_back(L';');
    if (payload != nullptr)
    {
        argument.append(payload);
    }

    return argument;
}

bool ExecuteCommand(const wchar_t* command, HWND parent, const wchar_t* payload)
{
    if (gRuntimeHost == nullptr)
    {
        return false;
    }

    HWND owner = ResolveOwnerWindow(parent);
    DWORD returnValue = 0;
    std::wstring argument = BuildArgument(command, owner, payload);
    CWinLibDPIContext dpiContext;
    HRESULT hr = gRuntimeHost->ExecuteInDefaultAppDomain(gAssemblyPath.c_str(), kManagedType, kManagedMethod,
                                                         argument.c_str(), &returnValue);
    if (FAILED(hr))
    {
        wchar_t message[256];
        StringCchPrintfW(message, _countof(message), L"Failed to execute managed command '%s' (0x%08X).", command, hr);
        MessageBoxW(owner, message, kPluginCaption, MB_ICONERROR | MB_OK);
        return false;
    }

    return returnValue == 0;
}

void ShowLoadError(HWND parent, const wchar_t* text)
{
    MessageBoxW(ResolveOwnerWindow(parent), text, kPluginCaption, MB_ICONERROR | MB_OK);
}

// HOST_E_INVALIDOPERATION: ICLRRuntimeHost::Stop already ran in this process.
constexpr HRESULT kClrHostAlreadyStopped = static_cast<HRESULT>(0x80131022L);

void ShowClrLoadError(HWND parent, HRESULT hr)
{
    wchar_t message[320];
    StringCchPrintfW(message, _countof(message),
                     L"Failed to load the CLR runtime. This process requires .NET Framework 4.8 (0x%08X).",
                     static_cast<unsigned int>(hr));
    ShowLoadError(parent, message);
}

void ShowClrStartError(HWND parent, HRESULT hr)
{
    wchar_t message[320];
    if (hr == kClrHostAlreadyStopped)
    {
        StringCchPrintfW(message, _countof(message),
                         L"Failed to start CLR runtime (0x%08X). The CLR was already stopped in this process. Restart Salamander.",
                         static_cast<unsigned int>(hr));
    }
    else
    {
        StringCchPrintfW(message, _countof(message),
                         L"Failed to start CLR runtime (0x%08X).",
                         static_cast<unsigned int>(hr));
    }
    ShowLoadError(parent, message);
}

std::wstring QueryPluginDirectory()
{
    DWORD capacity = 32768;
    std::wstring modulePath(static_cast<size_t>(capacity), L'\0');
    for (;;)
    {
        const DWORD length = GetModuleFileNameW(DLLInstance, &modulePath[0], capacity);
        if (length == 0)
        {
            return std::wstring();
        }
        if (length < capacity)
        {
            modulePath.resize(length);
            break;
        }
        if (capacity >= 1024u * 1024u)
        {
            return std::wstring();
        }
        capacity *= 2;
        modulePath.resize(capacity);
    }

    const size_t slash = modulePath.find_last_of(L'\\');
    if (slash != std::wstring::npos)
    {
        modulePath.resize(slash + 1);
    }
    return modulePath;
}

std::wstring BuildCurrentVersion()
{
    std::string version = VERSINFO_SALAMANDER_VERSION;
    const size_t platformSuffix = version.find(" (");
    if (platformSuffix != std::string::npos)
    {
        version.resize(platformSuffix);
    }

    version += VERSINFO_SAMANDARIN_SUFFIX;

    int required = MultiByteToWideChar(CP_UTF8, 0, version.c_str(), -1, nullptr, 0);
    if (required <= 0)
    {
        return std::wstring();
    }

    std::wstring result;
    result.resize(static_cast<size_t>(required));
    int converted = MultiByteToWideChar(CP_UTF8, 0, version.c_str(), -1, result.data(), required);
    if (converted <= 0)
    {
        return std::wstring();
    }

    result.resize(static_cast<size_t>(converted - 1));
    return result;
}

void ResetRuntimeLocked()
{
    if (gRuntimeHost != nullptr)
    {
        if (gIsInitialized)
        {
            ExecuteCommand(L"Shutdown", nullptr, nullptr);
            gIsInitialized = false;
        }
        // Leave the CLR loaded. Stop() is process-wide and cannot be undone.
        gRuntimeHost->Release();
        gRuntimeHost = nullptr;
    }
    gAssemblyPath.clear();
    gCurrentVersion.clear();
}

bool InitializeRuntimeLocked(HWND parent)
{
    if (gRuntimeHost != nullptr)
    {
        return true;
    }

    ICLRMetaHost* metaHost = nullptr;
    HRESULT hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&metaHost));
    if (FAILED(hr))
    {
        ShowLoadError(parent, L"Failed to load CLR meta host.");
        return false;
    }

    ICLRRuntimeInfo* runtimeInfo = nullptr;
    hr = metaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&runtimeInfo));
    metaHost->Release();
    if (FAILED(hr))
    {
        ShowLoadError(parent, L"Failed to locate CLR v4 runtime.");
        return false;
    }

    BOOL loadable = FALSE;
    hr = runtimeInfo->IsLoadable(&loadable);
    if (FAILED(hr) || loadable == FALSE)
    {
        runtimeInfo->Release();
        if (SUCCEEDED(hr))
        {
            hr = E_FAIL;
        }
        ShowClrLoadError(parent, hr);
        return false;
    }

    hr = runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&gRuntimeHost));
    runtimeInfo->Release();
    if (FAILED(hr))
    {
        ShowLoadError(parent, L"Failed to create CLR runtime host.");
        return false;
    }

    hr = gRuntimeHost->Start();
    if (FAILED(hr))
    {
        ShowClrStartError(parent, hr);
        gRuntimeHost->Release();
        gRuntimeHost = nullptr;
        return false;
    }

    const std::wstring moduleDirectory = QueryPluginDirectory();
    if (moduleDirectory.empty())
    {
        ShowLoadError(parent, L"Failed to determine plugin path.");
        ResetRuntimeLocked();
        return false;
    }

    gAssemblyPath = moduleDirectory;
    gAssemblyPath.append(L"Samandarin.Managed.dll");

    gCurrentVersion = BuildCurrentVersion();

    if (!gIsInitialized)
    {
        gIsInitialized = ExecuteCommand(L"Initialize", parent, gCurrentVersion.c_str());
        if (!gIsInitialized)
        {
            ResetRuntimeLocked();
            return false;
        }
    }

    return true;
}

DWORD WINAPI InitializeRuntimeThread(void* parameter)
{
    AcquireSRWLockExclusive(&gRuntimeLock);
    const bool initialized = InitializeRuntimeLocked(static_cast<HWND>(parameter));
    InterlockedExchange(&gInitializationResult, initialized ? 1 : -1);
    ReleaseSRWLockExclusive(&gRuntimeLock);
    return initialized ? 0 : 1;
}

void WaitForBackgroundInitialization()
{
    HANDLE thread = nullptr;
    DWORD threadId = 0;
    AcquireSRWLockShared(&gRuntimeLock);
    thread = gInitializationThread;
    threadId = gInitializationThreadId;
    ReleaseSRWLockShared(&gRuntimeLock);
    if (thread != nullptr && threadId != GetCurrentThreadId())
        WaitForSingleObject(thread, INFINITE);
}
} // namespace

bool ManagedBridge_BeginInitialize(HWND parent)
{
    AcquireSRWLockExclusive(&gRuntimeLock);
    if (gIsInitialized || gInitializationThread != nullptr)
    {
        ReleaseSRWLockExclusive(&gRuntimeLock);
        return true;
    }
    InterlockedExchange(&gInitializationResult, 0);
    gInitializationThread = CreateThread(nullptr, 0, InitializeRuntimeThread, parent, 0,
                                         &gInitializationThreadId);
    const bool started = gInitializationThread != nullptr;
    ReleaseSRWLockExclusive(&gRuntimeLock);
    return started;
}

bool ManagedBridge_EnsureInitialized(HWND parent)
{
    WaitForBackgroundInitialization();
    AcquireSRWLockExclusive(&gRuntimeLock);
    if (gInitializationThread != nullptr)
    {
        CloseHandle(gInitializationThread);
        gInitializationThread = nullptr;
        gInitializationThreadId = 0;
    }
    bool initialized = gIsInitialized;
    if (!initialized && InterlockedCompareExchange(&gInitializationResult, 0, 0) >= 0)
        initialized = InitializeRuntimeLocked(parent);
    ReleaseSRWLockExclusive(&gRuntimeLock);
    return initialized;
}

void ManagedBridge_Shutdown()
{
    WaitForBackgroundInitialization();
    AcquireSRWLockExclusive(&gRuntimeLock);
    if (gInitializationThread != nullptr)
    {
        CloseHandle(gInitializationThread);
        gInitializationThread = nullptr;
        gInitializationThreadId = 0;
    }
    ResetRuntimeLocked();
    InterlockedExchange(&gInitializationResult, 0);
    ReleaseSRWLockExclusive(&gRuntimeLock);
}

bool ManagedBridge_ShowConfiguration(HWND parent)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }
    return ExecuteCommand(L"Configure", parent, gCurrentVersion.c_str());
}

void ManagedBridge_NotifyColorsChanged()
{
    if (!ManagedBridge_EnsureInitialized(nullptr))
    {
        return;
    }

    ExecuteCommand(L"ColorsChanged", nullptr, nullptr);
}

bool ManagedBridge_CheckNow(HWND parent)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }

    return ExecuteCommand(L"CheckNow", parent, gCurrentVersion.c_str());
}

bool ManagedBridge_ShowPluginUpdates(HWND parent)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }

    return ExecuteCommand(L"PluginUpdates", parent, gCurrentVersion.c_str());
}

extern "C" __declspec(dllexport) UINT32 __stdcall Samandarin_GetCurrentColor(int color)
{
    if (SalamanderGeneral == nullptr)
    {
        return 0;
    }

    return SalamanderGeneral->GetCurrentColor(color);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_SetDarkModeState(BOOL enabled)
{
    DarkModeSetEnabled(enabled != FALSE);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_ApplyDarkModeTree(HWND hwnd)
{
    DarkModeAllowDarkScrollbars(hwnd);
    DarkModeApplyTree(hwnd);
}

extern "C" __declspec(dllexport) void __stdcall Samandarin_UpdateListViewDarkMode(HWND hwnd)
{
    DarkModeAllowDarkScrollbars(hwnd);
    DarkModeApplyTree(hwnd);

    if (SalamanderGeneral != nullptr)
    {
        const COLORREF text = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_FG_NORMAL);
        const COLORREF background = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL);
        const bool useCustomColors = text != GetSysColor(COLOR_WINDOWTEXT) ||
                                     background != GetSysColor(COLOR_WINDOW);
        DarkModeUpdateListViewColors(hwnd, text, background, useCustomColors);
        return;
    }

    DarkModeUpdateListViewColors(hwnd);
}


extern "C" __declspec(dllexport) int __stdcall Samandarin_GetLanguageModulePath(wchar_t* buffer, int bufferLength)
{
    if (HLanguage == nullptr || buffer == nullptr || bufferLength <= 0)
    {
        return 0;
    }

    buffer[0] = L'\0';
    DWORD length = GetModuleFileNameW(HLanguage, buffer, static_cast<DWORD>(bufferLength));
    if (length == 0 || length >= static_cast<DWORD>(bufferLength))
    {
        buffer[0] = L'\0';
        return 0;
    }

    return static_cast<int>(length);
}

extern "C" __declspec(dllexport) int __stdcall Samandarin_LoadString(int resourceId, wchar_t* buffer, int bufferLength)
{
    if (HLanguage == nullptr || buffer == nullptr || bufferLength <= 0)
    {
        return 0;
    }

    buffer[0] = L'\0';
    return LoadStringW(HLanguage, resourceId, buffer, bufferLength);
}
