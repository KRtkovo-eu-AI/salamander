// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Keep the CLR loaded after the C# demo shutdown and report Start failures. | ICLRRuntimeHost::Stop prevents later managed plugins from starting in this process. | | Grok 4.7 |

#include "precomp.h"
#include "managed_bridge.h"

#include <metahost.h>
#include <mscoree.h>
#include <strsafe.h>

#pragma comment(lib, "mscoree.lib")

namespace
{
ICLRRuntimeHost* gRuntimeHost = nullptr;
std::wstring gAssemblyPath;
const wchar_t* const kManagedType = L"OpenSalamander.CSDemo.EntryPoint";
const wchar_t* const kManagedMethod = L"Dispatch";

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

std::wstring ToWide(const char* text)
{
    if (text == nullptr)
    {
        return std::wstring();
    }

    int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (required <= 0)
    {
        return std::wstring();
    }

    std::wstring result;
    result.resize(required);
    int converted = MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), required);
    if (converted <= 0)
    {
        return std::wstring();
    }

    result.resize(converted - 1);
    return result;
}

bool ExecuteCommand(const wchar_t* command, HWND parent, const wchar_t* payload)
{
    if (gRuntimeHost == nullptr)
    {
        return false;
    }

    DWORD returnValue = 0;
    std::wstring argument = BuildArgument(command, parent, payload);
    HRESULT hr = gRuntimeHost->ExecuteInDefaultAppDomain(gAssemblyPath.c_str(), kManagedType, kManagedMethod,
                                                         argument.c_str(), &returnValue);
    if (FAILED(hr))
    {
        wchar_t message[256];
        StringCchPrintfW(message, _countof(message), L"Failed to execute managed command '%s' (0x%08X).", command, hr);
        MessageBoxW(parent, message, L"C# Demo Plugin", MB_ICONERROR | MB_OK);
        return false;
    }

    return returnValue == 0;
}

void ShowLoadError(HWND parent, const wchar_t* text)
{
    MessageBoxW(parent, text, L"C# Demo Plugin", MB_ICONERROR | MB_OK);
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

} // namespace

bool ManagedBridge_EnsureInitialized(HWND parent)
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
        ManagedBridge_Shutdown();
        return false;
    }

    gAssemblyPath = moduleDirectory;
    gAssemblyPath.append(L"CSDemo.Managed.dll");

    return true;
}

void ManagedBridge_Shutdown()
{
    if (gRuntimeHost != nullptr)
    {
        // Leave the CLR loaded. Stop() is process-wide and cannot be undone.
        gRuntimeHost->Release();
        gRuntimeHost = nullptr;
        gAssemblyPath.clear();
    }
}

bool ManagedBridge_ShowAbout(HWND parent)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }
    return ExecuteCommand(L"About", parent, nullptr);
}

bool ManagedBridge_ShowConfiguration(HWND parent)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }
    return ExecuteCommand(L"Configure", parent, nullptr);
}

bool ManagedBridge_RunMenuCommand(HWND parent, const char* command)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }

    std::wstring payload = ToWide(command);
    return ExecuteCommand(L"Menu", parent, payload.c_str());
}

extern "C" __declspec(dllexport) UINT32 __stdcall CSDemo_GetCurrentColor(int color)
{
    if (SalamanderGeneral == nullptr)
    {
        return 0;
    }

    return SalamanderGeneral->GetCurrentColor(color);
}
