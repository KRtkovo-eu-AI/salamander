// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "managed_bridge.h"

#include <metahost.h>
#include <mscoree.h>
#include <strsafe.h>
#include <wincrypt.h>

#pragma comment(lib, "mscoree.lib")
#pragma comment(lib, "crypt32.lib")

namespace
{
ICLRRuntimeHost* gRuntimeHost = nullptr;
std::wstring gAssemblyPath;
const wchar_t* const kManagedType = L"OpenSalamander.TextViewer.EntryPoint";
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

std::wstring ConvertMultiByteToWide(const char* text, UINT codePage)
{
    if (text == nullptr)
    {
        return std::wstring();
    }

    DWORD flags = (codePage == CP_UTF8) ? MB_ERR_INVALID_CHARS : 0;
    int required = MultiByteToWideChar(codePage, flags, text, -1, nullptr, 0);
    if (required <= 0)
    {
        return std::wstring();
    }

    std::wstring result;
    result.resize(static_cast<size_t>(required));
    int converted = MultiByteToWideChar(codePage, flags, text, -1, result.data(), required);
    if (converted <= 0)
    {
        return std::wstring();
    }

    result.resize(static_cast<size_t>(converted) - 1);
    return result;
}

std::wstring AnsiToWide(const char* text)
{
    if (text == nullptr)
    {
        return std::wstring();
    }

    std::wstring result = ConvertMultiByteToWide(text, CP_ACP);
    if (!result.empty())
    {
        return result;
    }

    result = ConvertMultiByteToWide(text, CP_UTF8);
    if (!result.empty())
    {
        return result;
    }

    // As a last resort, map bytes directly to Unicode code points so the
    // managed side still receives something meaningful to work with.
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(text);
    while (*bytes != '\0')
    {
        result.push_back(static_cast<wchar_t>(*bytes));
        ++bytes;
    }

    return result;
}

std::wstring EncodeBase64FromWide(const std::wstring& value)
{
    if (value.empty())
    {
        return std::wstring();
    }

    int utf8Length = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (utf8Length <= 1)
    {
        return std::wstring();
    }

    std::string utf8;
    utf8.resize(utf8Length - 1);
    if (WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, utf8.data(), utf8Length - 1, nullptr, nullptr) <= 0)
    {
        return std::wstring();
    }

    DWORD base64Length = 0;
    if (!CryptBinaryToStringW(reinterpret_cast<const BYTE*>(utf8.data()), static_cast<DWORD>(utf8.size()),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &base64Length))
    {
        return std::wstring();
    }

    std::wstring result;
    result.resize(base64Length);
    if (!CryptBinaryToStringW(reinterpret_cast<const BYTE*>(utf8.data()), static_cast<DWORD>(utf8.size()),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, result.data(), &base64Length))
    {
        return std::wstring();
    }

    if (!result.empty() && result.back() == L'\0')
    {
        result.pop_back();
    }
    else
    {
        result.resize(base64Length);
    }

    return result;
}

std::wstring ExtractFileName(const std::wstring& path)
{
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}

void AppendKeyValue(std::wstring& payload, const wchar_t* key, const wchar_t* value)
{
    if (!payload.empty())
    {
        payload.push_back(L'|');
    }
    payload.append(key);
    payload.push_back(L'=');
    if (value != nullptr)
    {
        payload.append(value);
    }
}

void AppendUInt(std::wstring& payload, const wchar_t* key, unsigned long value)
{
    wchar_t buffer[32];
    StringCchPrintfW(buffer, _countof(buffer), L"%lu", value);
    AppendKeyValue(payload, key, buffer);
}

void AppendInt(std::wstring& payload, const wchar_t* key, long value)
{
    wchar_t buffer[32];
    StringCchPrintfW(buffer, _countof(buffer), L"%ld", value);
    AppendKeyValue(payload, key, buffer);
}

void AppendHandle(std::wstring& payload, const wchar_t* key, HANDLE handle)
{
    wchar_t buffer[32];
    ULONGLONG value = reinterpret_cast<ULONGLONG>(handle);
    StringCchPrintfW(buffer, _countof(buffer), L"%llu", value);
    AppendKeyValue(payload, key, buffer);
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
        MessageBoxW(parent, message, L"Text Viewer .NET Plugin", MB_ICONERROR | MB_OK);
        return false;
    }

    return returnValue == 0;
}

void ShowLoadError(HWND parent, const wchar_t* text)
{
    MessageBoxW(parent, text, L"Text Viewer .NET Plugin", MB_ICONERROR | MB_OK);
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
    gAssemblyPath.append(L"TextViewer.Managed.dll");

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

bool ManagedBridge_RequestShutdown(HWND parent, bool forceClose)
{
    if (gRuntimeHost == nullptr)
    {
        return true;
    }

    std::wstring payload;
    AppendKeyValue(payload, L"force", forceClose ? L"1" : L"0");
    return ExecuteCommand(L"Release", parent, payload.c_str());
}

bool ManagedBridge_ViewTextFile(HWND parent, const char* filePath, const RECT& placement,
                                UINT showCmd, BOOL alwaysOnTop, HANDLE fileLock, bool asynchronous)
{
    if (!ManagedBridge_EnsureInitialized(parent))
    {
        return false;
    }

    std::wstring widePath = AnsiToWide(filePath);

    std::wstring encodedPath = EncodeBase64FromWide(widePath);
    if (encodedPath.empty())
    {
        encodedPath = widePath;
    }

    if (encodedPath.empty())
    {
        ShowLoadError(parent, L"Unable to prepare parameters for the text viewer.");
        return false;
    }

    std::wstring caption = ExtractFileName(widePath);
    if (caption.empty())
    {
        caption = widePath;
    }

    std::wstring encodedCaption = EncodeBase64FromWide(caption);
    if (encodedCaption.empty())
    {
        encodedCaption = caption;
    }

    std::wstring payload;
    AppendKeyValue(payload, L"path", encodedPath.c_str());
    AppendKeyValue(payload, L"caption", encodedCaption.c_str());
    AppendInt(payload, L"left", placement.left);
    AppendInt(payload, L"top", placement.top);
    AppendInt(payload, L"width", placement.right - placement.left);
    AppendInt(payload, L"height", placement.bottom - placement.top);
    AppendUInt(payload, L"show", showCmd);
    AppendKeyValue(payload, L"ontop", alwaysOnTop ? L"1" : L"0");
    AppendHandle(payload, L"close", fileLock);
    AppendKeyValue(payload, L"async", asynchronous ? L"1" : L"0");

    const wchar_t* command = asynchronous ? L"View" : L"ViewSync";
    return ExecuteCommand(command, parent, payload.c_str());
}

extern "C" __declspec(dllexport) UINT32 __stdcall TextViewer_GetCurrentColor(int color)
{
    if (SalamanderGeneral == NULL)
    {
        return 0;
    }

    return SalamanderGeneral->GetCurrentColor(color);
}
