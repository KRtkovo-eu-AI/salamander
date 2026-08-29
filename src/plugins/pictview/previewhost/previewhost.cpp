// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <propsys.h>
#include <wrl/client.h>

#include <cstdint>
#include <cwchar>
#include <new>
#include <string>
#include <vector>

#include "PreviewHostProtocol.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

using Microsoft::WRL::ComPtr;
using namespace PictViewPreviewProtocol;

namespace
{
constexpr wchar_t PreviewShellExtension[] = L"{8895b1c6-b41f-4c1c-a562-0d564250836f}";
constexpr CLSID ThreeDViewerPreview = {0x4834ac27, 0x23f1, 0x420a, {0x88, 0x8d, 0x85, 0xdc, 0x70, 0xb9, 0x03, 0xc5}};
constexpr wchar_t WindowClassName[] = L"OpenSalamander.PictView.PreviewHost";

HRESULT ErrorHresult(DWORD error = GetLastError())
{
    return HRESULT_FROM_WIN32(error == ERROR_SUCCESS ? ERROR_GEN_FAILURE : error);
}

bool ReadExact(HANDLE handle, void* data, DWORD bytes)
{
    BYTE* cursor = static_cast<BYTE*>(data);
    while (bytes != 0)
    {
        DWORD done = 0;
        if (!ReadFile(handle, cursor, bytes, &done, nullptr) || done == 0)
            return false;
        cursor += done;
        bytes -= done;
    }
    return true;
}

bool WriteExact(HANDLE handle, const void* data, DWORD bytes)
{
    const BYTE* cursor = static_cast<const BYTE*>(data);
    while (bytes != 0)
    {
        DWORD done = 0;
        if (!WriteFile(handle, cursor, bytes, &done, nullptr) || done == 0)
            return false;
        cursor += done;
        bytes -= done;
    }
    return true;
}

bool ReadRegistryDefault(const std::wstring& keyName, std::wstring& value)
{
    value.clear();
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyName.c_str(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS)
        return false;
    DWORD type = 0;
    DWORD bytes = 0;
    LONG status = RegQueryValueExW(key, nullptr, nullptr, &type, nullptr, &bytes);
    if (status != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(wchar_t))
    {
        RegCloseKey(key);
        return false;
    }
    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
    status = RegQueryValueExW(key, nullptr, nullptr, &type, reinterpret_cast<BYTE*>(buffer.data()), &bytes);
    RegCloseKey(key);
    if (status != ERROR_SUCCESS || buffer[0] == L'\0')
        return false;
    value.assign(buffer.data());
    return true;
}

bool LookupPreviewClsid(const std::wstring& association, CLSID& clsid)
{
    wchar_t value[80]{};
    DWORD chars = ARRAYSIZE(value);
    if (SUCCEEDED(AssocQueryStringW(ASSOCF_NOTRUNCATE, ASSOCSTR_SHELLEXTENSION, association.c_str(),
                                    PreviewShellExtension, value, &chars)) && value[0] != L'\0' &&
        SUCCEEDED(CLSIDFromString(value, &clsid)))
        return true;
    std::wstring registered;
    return ReadRegistryDefault(association + L"\\shellex\\" + PreviewShellExtension, registered) &&
           SUCCEEDED(CLSIDFromString(registered.c_str(), &clsid));
}

std::wstring ExtensionOf(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash) || dot + 1 == path.size())
        return {};
    std::wstring extension = path.substr(dot);
    CharLowerBuffW(extension.data(), static_cast<DWORD>(extension.size()));
    return extension;
}

class PreviewFrame final : public IPreviewHandlerFrame, public IServiceProvider
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override
    {
        if (result == nullptr)
            return E_POINTER;
        *result = nullptr;
        if (iid == IID_IUnknown || iid == IID_IPreviewHandlerFrame)
            *result = static_cast<IPreviewHandlerFrame*>(this);
        else if (iid == IID_IServiceProvider)
            *result = static_cast<IServiceProvider*>(this);
        else
            return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&references_); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        ULONG remaining = InterlockedDecrement(&references_);
        if (remaining == 0)
            delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE GetWindowContext(PREVIEWHANDLERFRAMEINFO* info) override
    {
        if (info == nullptr)
            return E_POINTER;
        info->haccel = nullptr;
        info->cAccelEntries = 0;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG*) override { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID service, REFIID iid, void** result) override
    {
        return service == IID_IPreviewHandlerFrame ? QueryInterface(iid, result) : E_NOINTERFACE;
    }
private:
    ~PreviewFrame() = default;
    LONG references_ = 1;
};

HRESULT InitializeHandler(IPreviewHandler* handler, const std::wstring& path)
{
    ComPtr<IShellItem> item;
    HRESULT hr = SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&item));
    ComPtr<IInitializeWithItem> itemInitializer;
    if (SUCCEEDED(hr) && SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&itemInitializer))) &&
        SUCCEEDED(hr = itemInitializer->Initialize(item.Get(), STGM_READ)))
        return S_OK;
    ComPtr<IInitializeWithFile> fileInitializer;
    if (SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&fileInitializer))) &&
        SUCCEEDED(hr = fileInitializer->Initialize(path.c_str(), STGM_READ)))
        return S_OK;
    ComPtr<IInitializeWithStream> streamInitializer;
    if (FAILED(handler->QueryInterface(IID_PPV_ARGS(&streamInitializer))))
        return FAILED(hr) ? hr : E_NOINTERFACE;
    ComPtr<IStream> stream;
    hr = SHCreateStreamOnFileEx(path.c_str(), STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, nullptr, &stream);
    return FAILED(hr) ? hr : streamInitializer->Initialize(stream.Get(), STGM_READ);
}

class PreviewHost
{
public:
    ~PreviewHost() { Close(); if (window_ != nullptr) DestroyWindow(window_); }

    HRESULT Create(HINSTANCE instance)
    {
        WNDCLASSEXW wc{sizeof(wc)};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = WindowClassName;
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return ErrorHresult();
        window_ = CreateWindowExW(WS_EX_NOACTIVATE, WindowClassName, L"", WS_POPUP | WS_CLIPCHILDREN,
                                  0, 0, 1, 1, nullptr, nullptr, instance, this);
        return window_ != nullptr ? S_OK : ErrorHresult();
    }

    HRESULT Dispatch(Command command, const std::vector<BYTE>& payload)
    {
        switch (command)
        {
        case Command::Open: return Open(payload);
        case Command::Attach: return Attach(payload);
        case Command::Resize: return Resize(payload);
        case Command::Focus: return Focus();
        case Command::Close: return Close();
        default: return E_INVALIDARG;
        }
    }

private:
    HRESULT Open(const std::vector<BYTE>& payload)
    {
        if (payload.size() < sizeof(wchar_t) || payload.size() % sizeof(wchar_t) != 0)
            return E_INVALIDARG;
        const wchar_t* text = reinterpret_cast<const wchar_t*>(payload.data());
        const size_t count = payload.size() / sizeof(wchar_t);
        if (text[count - 1] != L'\0' || wcsnlen_s(text, count) != count - 1)
            return E_INVALIDARG;
        Close();
        std::wstring path(text, count - 1);
        const std::wstring extension = ExtensionOf(path);
        if (extension.empty())
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        std::vector<std::wstring> associations{extension, L"SystemFileAssociations\\" + extension};
        std::wstring progId;
        if (ReadRegistryDefault(extension, progId) && !progId.empty())
            associations.push_back(progId);

        HRESULT last = REGDB_E_CLASSNOTREG;
        for (const std::wstring& association : associations)
        {
            CLSID clsid{};
            if (!LookupPreviewClsid(association, clsid))
                continue;
            last = CreateAndInitialize(clsid, path);
            if (SUCCEEDED(last))
                return last;
        }
        if (_wcsicmp(extension.c_str(), L".stl") == 0)
            last = CreateAndInitialize(ThreeDViewerPreview, path);
        return last;
    }

    HRESULT CreateAndInitialize(REFCLSID clsid, const std::wstring& path)
    {
        ComPtr<IPreviewHandler> candidate;
        HRESULT hr = CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&candidate));
        if (FAILED(hr))
        {
            hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&candidate));
            if (FAILED(hr))
                return hr;
        }
        PreviewFrame* rawFrame = new (std::nothrow) PreviewFrame();
        if (rawFrame == nullptr)
            return E_OUTOFMEMORY;
        ComPtr<IUnknown> frame;
        hr = rawFrame->QueryInterface(IID_PPV_ARGS(&frame));
        rawFrame->Release();
        if (FAILED(hr))
            return hr;
        ComPtr<IObjectWithSite> site;
        if (SUCCEEDED(candidate.As(&site)))
            site->SetSite(frame.Get());
        hr = InitializeHandler(candidate.Get(), path);
        if (FAILED(hr))
        {
            if (site) site->SetSite(nullptr);
            candidate->Unload();
            return hr;
        }
        handler_ = std::move(candidate);
        site_ = std::move(site);
        frame_ = std::move(frame);
        return ApplyWindow();
    }

    HRESULT Attach(const std::vector<BYTE>& payload)
    {
        if (payload.size() != sizeof(AttachPayload))
            return E_INVALIDARG;
        const AttachPayload& attach = *reinterpret_cast<const AttachPayload*>(payload.data());
        HWND parent = reinterpret_cast<HWND>(static_cast<ULONG_PTR>(attach.parentWindow));
        if (parent != nullptr && !IsWindow(parent))
            return HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE);
        parent_ = parent;
        rectangle_ = attach.rectangle;
        background_ = attach.background;
        SetWindowLongPtrW(window_, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(parent_));
        InvalidateRect(window_, nullptr, TRUE);
        return ApplyWindow();
    }

    HRESULT Resize(const std::vector<BYTE>& payload)
    {
        if (payload.size() != sizeof(ResizePayload))
            return E_INVALIDARG;
        rectangle_ = reinterpret_cast<const ResizePayload*>(payload.data())->rectangle;
        return ApplyWindow();
    }

    HRESULT ApplyWindow()
    {
        if (window_ == nullptr)
            return E_UNEXPECTED;
        const int width = rectangle_.right > rectangle_.left ? rectangle_.right - rectangle_.left : 0;
        const int height = rectangle_.bottom > rectangle_.top ? rectangle_.bottom - rectangle_.top : 0;
        SetWindowPos(window_, HWND_TOP, rectangle_.left, rectangle_.top, width, height,
                     SWP_NOACTIVATE | (parent_ == nullptr ? SWP_HIDEWINDOW : SWP_SHOWWINDOW));
        if (!handler_)
            return S_OK;
        RECT client{0, 0, width, height};
        HRESULT hr = handler_->SetWindow(window_, &client);
        if (SUCCEEDED(hr)) hr = handler_->SetRect(&client);
        if (SUCCEEDED(hr) && !previewStarted_)
        {
            hr = handler_->DoPreview();
            if (SUCCEEDED(hr)) previewStarted_ = true;
        }
        return hr;
    }

    HRESULT Focus()
    {
        if (!handler_)
            return E_UNEXPECTED;
        return handler_->SetFocus();
    }

    HRESULT Close()
    {
        if (handler_)
            handler_->Unload();
        if (site_)
            site_->SetSite(nullptr);
        handler_.Reset();
        site_.Reset();
        frame_.Reset();
        previewStarted_ = false;
        if (window_ != nullptr)
            ShowWindow(window_, SW_HIDE);
        return S_OK;
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        PreviewHost* self = reinterpret_cast<PreviewHost*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            self = static_cast<PreviewHost*>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (message == WM_ERASEBKGND && self != nullptr)
        {
            RECT rect{};
            GetClientRect(window, &rect);
            HBRUSH brush = CreateSolidBrush(self->background_);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, brush);
            DeleteObject(brush);
            return 1;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    HWND window_ = nullptr;
    HWND parent_ = nullptr;
    RECT rectangle_{};
    COLORREF background_ = RGB(255, 255, 255);
    ComPtr<IPreviewHandler> handler_;
    ComPtr<IObjectWithSite> site_;
    ComPtr<IUnknown> frame_;
    bool previewStarted_ = false;
};

bool ParseArguments(std::wstring& pipeName, DWORD& ownerPid)
{
    int count = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &count);
    if (args == nullptr)
        return false;
    bool valid = count == 3 && wcsncmp(args[1], L"\\\\.\\pipe\\", 9) == 0;
    wchar_t* end = nullptr;
    unsigned long pid = wcstoul(count >= 3 ? args[2] : L"", &end, 10);
    valid = valid && end != nullptr && *end == L'\0' && pid != 0;
    if (valid)
    {
        pipeName = args[1];
        ownerPid = static_cast<DWORD>(pid);
    }
    LocalFree(args);
    return valid;
}

int Run(HINSTANCE instance)
{
    std::wstring pipeName;
    DWORD ownerPid = 0;
    if (!ParseArguments(pipeName, ownerPid))
        return ERROR_BAD_ARGUMENTS;
    HANDLE owner = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, ownerPid);
    if (owner == nullptr)
        return GetLastError();
    if (WaitForSingleObject(owner, 0) != WAIT_TIMEOUT)
    {
        CloseHandle(owner);
        return ERROR_PROCESS_ABORTED;
    }
    HANDLE pipe = CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        CloseHandle(owner);
        return error;
    }
    ULONG serverPid = 0;
    if (!GetNamedPipeServerProcessId(pipe, &serverPid) || serverPid != ownerPid)
    {
        CloseHandle(pipe);
        CloseHandle(owner);
        return ERROR_ACCESS_DENIED;
    }
    DWORD mode = PIPE_READMODE_BYTE;
    SetNamedPipeHandleState(pipe, &mode, nullptr, nullptr);
    PreviewHost host;
    HRESULT hr = host.Create(instance);
    if (FAILED(hr))
    {
        CloseHandle(pipe);
        CloseHandle(owner);
        return HRESULT_CODE(hr);
    }
    bool running = true;
    while (running)
    {
        DWORD wait = MsgWaitForMultipleObjects(1, &owner, FALSE, 25, QS_ALLINPUT);
        if (wait == WAIT_OBJECT_0) break;
        if (wait == WAIT_OBJECT_0 + 1)
        {
            MSG message{};
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT) { running = false; break; }
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
        if (!running) break;
        DWORD available = 0;
        if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr)) break;
        if (available < sizeof(RequestHeader)) continue;
        RequestHeader header{};
        if (!ReadExact(pipe, &header, sizeof(header))) break;
        HRESULT result = S_OK;
        std::vector<BYTE> payload;
        if (header.magic != Magic || header.version != Version || header.payloadBytes > MaximumPayloadBytes)
            result = E_INVALIDARG;
        else
        {
            try { payload.resize(header.payloadBytes); }
            catch (const std::bad_alloc&) { result = E_OUTOFMEMORY; }
            if (SUCCEEDED(result) && header.payloadBytes != 0 && !ReadExact(pipe, payload.data(), header.payloadBytes)) break;
            if (SUCCEEDED(result)) result = host.Dispatch(header.command, payload);
        }
        Response response{Magic, Version, 0, header.requestId, result};
        if (!WriteExact(pipe, &response, sizeof(response))) break;
        if (header.command == Command::Close) running = false;
    }
    CloseHandle(pipe);
    CloseHandle(owner);
    return 0;
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    HRESULT ole = OleInitialize(nullptr);
    if (FAILED(ole))
        return HRESULT_CODE(ole);
    int result = Run(instance);
    OleUninitialize();
    return result;
}




