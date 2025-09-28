// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

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
    result.resize(required - 1);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, result.data(), required - 1);
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
    gAssemblyPath.append(L"CSDemo.Managed.dll");

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

bool ManagedBridge_RunMenuCommand(HWND parent, const char* command)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }

    std::wstring payload = ToWide(command);
    return ExecuteCommand(L"Menu", parent, payload.c_str());
}
