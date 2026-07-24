// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "managed_bridge.h"
#include "../../darkmode.h"
#include "../../common/winlibdpi.h"

#include <metahost.h>
#include <mscoree.h>
#include <strsafe.h>

#pragma comment(lib, "mscoree.lib")

namespace
{
ICLRRuntimeHost* gRuntimeHost = nullptr;
std::wstring gAssemblyPath;
const wchar_t* const kManagedType = L"OpenSalamander.HyperVM.EntryPoint";
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
    CWinLibDPIContext dpiContext;
    HRESULT hr = gRuntimeHost->ExecuteInDefaultAppDomain(gAssemblyPath.c_str(), kManagedType, kManagedMethod,
                                                         argument.c_str(), &returnValue);
    if (FAILED(hr))
    {
        wchar_t message[256];
        StringCchPrintfW(message, _countof(message), L"Failed to execute managed command '%s' (0x%08X).", command, hr);
        MessageBoxW(parent, message, L"Hyper-V Machines Plugin", MB_ICONERROR | MB_OK);
        return false;
    }

    return returnValue == 0;
}

void ShowLoadError(HWND parent, const wchar_t* text)
{
    MessageBoxW(parent, text, L"Hyper-V Machines Plugin", MB_ICONERROR | MB_OK);
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
        ShowLoadError(parent, L"Failed to start CLR runtime.");
        gRuntimeHost->Release();
        gRuntimeHost = nullptr;
        return false;
    }

    wchar_t modulePath[MAX_PATH] = {0};
    if (GetModuleFileNameW(DLLInstance, modulePath, _countof(modulePath)) == 0)
    {
        ShowLoadError(parent, L"Failed to determine plugin path.");
        ManagedBridge_Shutdown();
        return false;
    }

    wchar_t* lastSlash = wcsrchr(modulePath, L'\\');
    if (lastSlash != nullptr)
    {
        *(lastSlash + 1) = L'\0';
    }

    gAssemblyPath.assign(modulePath);
    gAssemblyPath.append(L"HyperVM.Managed.dll");

    return true;
}

void ManagedBridge_Shutdown()
{
    if (gRuntimeHost != nullptr)
    {
        gRuntimeHost->Stop();
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

void ManagedBridge_NotifyColorsChanged()
{
    if (!ManagedBridge_EnsureInitialized(nullptr))
    {
        return;
    }

    ExecuteCommand(L"ColorsChanged", nullptr, nullptr);
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

bool ManagedBridge_RunMenuCommandW(HWND parent, const wchar_t* command)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }

    return ExecuteCommand(L"Menu", parent, command);
}

extern "C" __declspec(dllexport) UINT32 __stdcall HyperVM_GetCurrentColor(int color)
{
    if (SalamanderGeneral == nullptr)
    {
        return 0;
    }

    return SalamanderGeneral->GetCurrentColor(color);
}

extern "C" __declspec(dllexport) void __stdcall HyperVM_SetDarkModeState(BOOL enabled)
{
    DarkModeSetEnabled(enabled != FALSE);
}

extern "C" __declspec(dllexport) void __stdcall HyperVM_ApplyDarkModeTree(HWND hwnd)
{
    DarkModeAllowDarkScrollbars(hwnd);
    DarkModeApplyTree(hwnd);
}

extern "C" __declspec(dllexport) void __stdcall HyperVM_UpdateListViewDarkMode(HWND hwnd)
{
    DarkModeAllowDarkScrollbars(hwnd);
    DarkModeApplyTree(hwnd);
    DarkModeUpdateListViewColors(hwnd, RGB(0xFF, 0xFF, 0xFF), RGB(56, 56, 56), true);
}

extern "C" __declspec(dllexport) int __stdcall HyperVM_LoadString(int stringId, wchar_t* buffer, int bufferLength)
{
    if (HLanguage == nullptr || buffer == nullptr || bufferLength <= 0)
    {
        return 0;
    }

    int copied = LoadStringW(HLanguage, stringId, buffer, bufferLength);
    if (copied <= 0)
    {
        buffer[0] = L'\0';
        return 0;
    }

    buffer[bufferLength - 1] = L'\0';
    return copied;
}
