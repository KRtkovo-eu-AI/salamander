// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "ShellPreviewHost.h"
#include "../previewhost/PreviewHostProtocol.h"

#include <new>
#include <strsafe.h>

#include <shlwapi.h>
#include <shlguid.h>
#include <shobjidl.h>
#include <propsys.h>
#include <exdisp.h>
#include <shldisp.h>
#include <sddl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

using Microsoft::WRL::ComPtr;

namespace PictView::Wic
{
namespace
{

// Microsoft 3D Viewer packaged COM preview handler (AppxManifest DesktopPreviewHandler
// CLSID {4834AC27-23F1-420A-888D-85DC70B903C5} + ShellPreviewHandler3D.dll,
// SystemSurrogate=PreviewHost). Explorer uses this for .stl when 3D Viewer is the
// default app — there is often no HKCR shellex on .stl.
constexpr CLSID kClsid3DViewerPreview = {
    0x4834AC27, 0x23F1, 0x420A, {0x88, 0x8D, 0x85, 0xDC, 0x70, 0xB9, 0x03, 0xC5}};

constexpr wchar_t kPreviewHandlerShellEx[] = L"{8895b1c6-b41f-4c1c-a562-0d564250836f}";

// Shortcuts that still belong to PictView while IPreviewHandler has focus.
// Zoom, pan, crop and tool keys stay with the 3D viewer.
const ACCEL kPreviewHostAccels[] = {
    {FVIRTKEY | FNOINVERT, VK_ESCAPE, CMD_CLOSE},
    {FVIRTKEY | FNOINVERT, VK_SPACE, CMD_FILE_NEXT},
    {FVIRTKEY | FNOINVERT | FCONTROL, VK_SPACE, CMD_FILE_NEXTSELFILE},
    {FVIRTKEY | FNOINVERT | FSHIFT, VK_SPACE, CMD_FILE_LAST},
    {FVIRTKEY | FNOINVERT, VK_BACK, CMD_FILE_PREV},
    {FVIRTKEY | FNOINVERT | FCONTROL, VK_BACK, CMD_FILE_PREVSELFILE},
    {FVIRTKEY | FNOINVERT | FSHIFT, VK_BACK, CMD_FILE_FIRST},
    {FVIRTKEY | FNOINVERT, VK_F1, CMD_HELP_CONTENTS},
    {FVIRTKEY | FNOINVERT, VK_F2, CMD_IMG_RENAME},
    {FVIRTKEY | FNOINVERT, VK_F3, CMD_IMG_PROP},
    {FVIRTKEY | FNOINVERT, VK_F11, CMD_FULLSCREEN},
    {FVIRTKEY | FNOINVERT, VK_DELETE, CMD_IMG_DELETE},
    {FVIRTKEY | FNOINVERT | FSHIFT, VK_DELETE, CMD_IMG_DELETE},
    {FVIRTKEY | FNOINVERT, VK_INSERT, CMD_FILE_TOGGLESELECT},
    {FVIRTKEY | FNOINVERT | FCONTROL, 'O', CMD_OPEN},
    {FVIRTKEY | FNOINVERT | FCONTROL, 'R', CMD_RELOAD},
    {FVIRTKEY | FNOINVERT | FCONTROL, 'S', CMD_SAVEAS},
    {FVIRTKEY | FNOINVERT | FCONTROL, 'C', CMD_COPY},
    {FVIRTKEY | FNOINVERT, 'F', CMD_IMG_FOCUS},
    {FVIRTKEY | FNOINVERT, 'I', CMD_IMG_PROP},
    {FVIRTKEY | FNOINVERT, 'X', CMD_IMG_COPYTO},
};

HACCEL CreatePreviewHostAccel()
{
    return CreateAcceleratorTable(const_cast<ACCEL*>(kPreviewHostAccels),
                                  static_cast<int>(ARRAYSIZE(kPreviewHostAccels)));
}

WORD CommandFromPreviewKey(const MSG* pmsg)
{
    if (pmsg == nullptr || (pmsg->message != WM_KEYDOWN && pmsg->message != WM_SYSKEYDOWN))
    {
        return 0;
    }
    const BOOL shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const BOOL control = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    switch (pmsg->wParam)
    {
    case VK_ESCAPE:
        return CMD_CLOSE;
    case VK_SPACE:
        if (control && !shift)
            return CMD_FILE_NEXTSELFILE;
        if (!control && shift)
            return CMD_FILE_LAST;
        if (!control && !shift)
            return CMD_FILE_NEXT;
        return 0;
    case VK_BACK:
        if (control && !shift)
            return CMD_FILE_PREVSELFILE;
        if (!control && shift)
            return CMD_FILE_FIRST;
        if (!control && !shift)
            return CMD_FILE_PREV;
        return 0;
    case VK_F1:
        return control || shift ? 0 : CMD_HELP_CONTENTS;
    case VK_F2:
        return control || shift ? 0 : CMD_IMG_RENAME;
    case VK_F3:
        return control || shift ? 0 : CMD_IMG_PROP;
    case VK_F11:
        return control || shift ? 0 : CMD_FULLSCREEN;
    case VK_DELETE:
        return control ? 0 : CMD_IMG_DELETE;
    case VK_INSERT:
        return control || shift ? 0 : CMD_FILE_TOGGLESELECT;
    case 'O':
        return control && !shift ? CMD_OPEN : 0;
    case 'R':
        return control && !shift ? CMD_RELOAD : 0;
    case 'S':
        return control && !shift ? CMD_SAVEAS : 0;
    case 'C':
        return control && !shift ? CMD_COPY : 0;
    case 'F':
        return !control && !shift ? CMD_IMG_FOCUS : 0;
    case 'I':
        return !control && !shift ? CMD_IMG_PROP : 0;
    case 'X':
        return !control && !shift ? CMD_IMG_COPYTO : 0;
    default:
        return 0;
    }
}

class PreviewHandlerFrame final : public IPreviewHandlerFrame, public IServiceProvider
{
public:
    PreviewHandlerFrame() = default;
    PreviewHandlerFrame(const PreviewHandlerFrame&) = delete;
    PreviewHandlerFrame& operator=(const PreviewHandlerFrame&) = delete;

    void SetHostWindow(HWND hwnd) { m_host = hwnd; }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (ppv == nullptr)
        {
            return E_POINTER;
        }
        if (riid == IID_IUnknown || riid == IID_IPreviewHandlerFrame)
        {
            *ppv = static_cast<IPreviewHandlerFrame*>(this);
        }
        else if (riid == IID_IServiceProvider)
        {
            *ppv = static_cast<IServiceProvider*>(this);
        }
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return InterlockedIncrement(&m_ref);
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG ref = InterlockedDecrement(&m_ref);
        if (ref == 0)
        {
            delete this;
        }
        return ref;
    }

    HRESULT STDMETHODCALLTYPE GetWindowContext(PREVIEWHANDLERFRAMEINFO* info) override
    {
        if (info == nullptr)
        {
            return E_POINTER;
        }
        if (m_accel == nullptr)
        {
            m_accel = CreatePreviewHostAccel();
        }
        info->haccel = m_accel != nullptr ? m_accel : G.HAccel;
        HACCEL table = info->haccel;
        info->cAccelEntries = table != nullptr ? CopyAcceleratorTable(table, nullptr, 0) : 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG* pmsg) override
    {
        if (pmsg == nullptr)
        {
            return E_POINTER;
        }
        if (m_accel == nullptr)
        {
            m_accel = CreatePreviewHostAccel();
        }
        HACCEL table = m_accel != nullptr ? m_accel : G.HAccel;
        if (m_host != nullptr && table != nullptr && ::TranslateAccelerator(m_host, table, pmsg))
        {
            return S_OK;
        }
        const WORD cmd = CommandFromPreviewKey(pmsg);
        if (cmd != 0 && m_host != nullptr)
        {
            PostMessage(m_host, WM_COMMAND, MAKEWPARAM(cmd, 0), 0);
            return S_OK;
        }
        return S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void** ppv) override
    {
        if (guidService == IID_IPreviewHandlerFrame)
        {
            return QueryInterface(riid, ppv);
        }
        if (ppv != nullptr)
        {
            *ppv = nullptr;
        }
        return E_NOINTERFACE;
    }

private:
    ~PreviewHandlerFrame()
    {
        if (m_accel != nullptr)
        {
            DestroyAcceleratorTable(m_accel);
        }
    }
    LONG m_ref = 1;
    HWND m_host = nullptr;
    HACCEL m_accel = nullptr;
};

void BindPreviewFrameHost(IUnknown* site, HWND hwnd)
{
    if (site == nullptr || hwnd == nullptr)
    {
        return;
    }
    ComPtr<IPreviewHandlerFrame> frame;
    if (FAILED(site->QueryInterface(IID_PPV_ARGS(&frame))) || !frame)
    {
        return;
    }
    static_cast<PreviewHandlerFrame*>(frame.Get())->SetHostWindow(hwnd);
}

std::wstring ExtensionLower(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const size_t name = slash == std::wstring::npos ? 0 : slash + 1;
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || dot < name || dot + 1 >= path.size())
    {
        return std::wstring();
    }
    std::wstring ext = path.substr(dot + 1);
    for (wchar_t& ch : ext)
    {
        if (ch >= L'A' && ch <= L'Z')
        {
            ch = static_cast<wchar_t>(ch - L'A' + L'a');
        }
    }
    return ext;
}
constexpr DWORD kHelperConnectTimeoutMs = 10000;
constexpr DWORD kHelperIoTimeoutMs = 5000;

bool IsProcessElevated()
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;
    TOKEN_ELEVATION elevation{};
    DWORD bytes = 0;
    const bool elevated = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &bytes) != FALSE &&
                          elevation.TokenIsElevated != 0;
    CloseHandle(token);
    return elevated;
}

std::wstring ModuleDirectory()
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&ModuleDirectory), &module))
        return {};
    std::vector<wchar_t> path(512);
    for (;;)
    {
        const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0)
            return {};
        if (length < path.size() - 1)
        {
            std::wstring result(path.data(), length);
            const size_t slash = result.find_last_of(L"\\/");
            return slash == std::wstring::npos ? std::wstring() : result.substr(0, slash);
        }
        if (path.size() >= 32768)
            return {};
        path.resize(path.size() * 2);
    }
}

HRESULT ShellExecuteUnelevated(const std::wstring& executable, const std::wstring& arguments,
                               const std::wstring& directory)
{
    ComPtr<IShellWindows> windows;
    HRESULT hr = CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(&windows));
    if (FAILED(hr)) return hr;
    VARIANT empty; VariantInit(&empty);
    long hwnd = 0;
    ComPtr<IDispatch> desktop;
    hr = windows->FindWindowSW(&empty, &empty, SWC_DESKTOP, &hwnd, SWFO_NEEDDISPATCH, &desktop);
    if (FAILED(hr)) return hr;
    ComPtr<IServiceProvider> provider;
    hr = desktop.As(&provider);
    if (FAILED(hr)) return hr;
    ComPtr<IShellBrowser> browser;
    hr = provider->QueryService(SID_STopLevelBrowser, IID_PPV_ARGS(&browser));
    if (FAILED(hr)) return hr;
    ComPtr<IShellView> view;
    hr = browser->QueryActiveShellView(&view);
    if (FAILED(hr)) return hr;
    ComPtr<IDispatch> background;
    hr = view->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&background));
    if (FAILED(hr)) return hr;
    ComPtr<IShellFolderViewDual> folderView;
    hr = background.As(&folderView);
    if (FAILED(hr)) return hr;
    ComPtr<IDispatch> application;
    hr = folderView->get_Application(&application);
    if (FAILED(hr)) return hr;
    ComPtr<IShellDispatch2> shell;
    hr = application.As(&shell);
    if (FAILED(hr)) return hr;
    VARIANT args{}, dir{}, verb{}, show{};
    args.vt = dir.vt = verb.vt = VT_BSTR;
    args.bstrVal = SysAllocString(arguments.c_str());
    dir.bstrVal = SysAllocString(directory.c_str());
    verb.bstrVal = SysAllocString(L"open");
    show.vt = VT_I4;
    show.lVal = SW_SHOWNORMAL;
    BSTR file = SysAllocString(executable.c_str());
    if (file == nullptr || args.bstrVal == nullptr || dir.bstrVal == nullptr || verb.bstrVal == nullptr)
        hr = E_OUTOFMEMORY;
    else
        hr = shell->ShellExecute(file, args, dir, verb, show);
    SysFreeString(file);
    VariantClear(&args);
    VariantClear(&dir);
    VariantClear(&verb);
    return hr;
}

bool OverlappedTransfer(HANDLE pipe, bool write, void* buffer, DWORD bytes)
{
    BYTE* cursor = static_cast<BYTE*>(buffer);
    DWORD remaining = bytes;
    const ULONGLONG deadline = GetTickCount64() + kHelperIoTimeoutMs;
    while (remaining != 0)
    {
        OVERLAPPED operation{};
        operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (operation.hEvent == nullptr) return false;
        DWORD transferred = 0;
        BOOL ok = write ? WriteFile(pipe, cursor, remaining, &transferred, &operation)
                        : ReadFile(pipe, cursor, remaining, &transferred, &operation);
        const bool pending = !ok && GetLastError() == ERROR_IO_PENDING;
        if (pending)
        {
            const ULONGLONG now = GetTickCount64();
            const DWORD waitMs = now < deadline ? static_cast<DWORD>(deadline - now) : 0;
            ok = WaitForSingleObject(operation.hEvent, waitMs) == WAIT_OBJECT_0 &&
                 GetOverlappedResult(pipe, &operation, &transferred, FALSE);
        }
        if (!ok)
        {
            if (pending)
            {
                CancelIoEx(pipe, &operation);
                DWORD ignored = 0;
                if (!GetOverlappedResult(pipe, &operation, &ignored, TRUE) &&
                    GetLastError() != ERROR_OPERATION_ABORTED)
                {
                    CloseHandle(operation.hEvent);
                    return false;
                }
            }
            CloseHandle(operation.hEvent);
            return false;
        }
        CloseHandle(operation.hEvent);
        if (transferred == 0) return false;
        cursor += transferred;
        remaining -= transferred;
    }
    return true;
}

HRESULT SendHelperRequest(ImageHandle& handle, PictViewPreviewProtocol::Command command,
                          const void* payload = nullptr, DWORD payloadBytes = 0)
{
    if (handle.previewPipe == INVALID_HANDLE_VALUE) return E_HANDLE;
    PictViewPreviewProtocol::RequestHeader request{PictViewPreviewProtocol::Magic,
        PictViewPreviewProtocol::Version, command, payloadBytes, ++handle.previewRequestId};
    if (!OverlappedTransfer(handle.previewPipe, true, &request, sizeof(request)) ||
        (payloadBytes != 0 && !OverlappedTransfer(handle.previewPipe, true, const_cast<void*>(payload), payloadBytes)))
        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    PictViewPreviewProtocol::Response response{};
    if (!OverlappedTransfer(handle.previewPipe, false, &response, sizeof(response)))
        return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    if (response.magic != PictViewPreviewProtocol::Magic || response.version != PictViewPreviewProtocol::Version ||
        response.requestId != request.requestId)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    return response.result;
}

HRESULT StartPreviewHelper(ImageHandle& handle)
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return HRESULT_FROM_WIN32(GetLastError());
    DWORD bytes = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &bytes);
    std::vector<BYTE> userBytes(bytes);
    if (!GetTokenInformation(token, TokenUser, userBytes.data(), bytes, &bytes))
    { DWORD error = GetLastError(); CloseHandle(token); return HRESULT_FROM_WIN32(error); }
    CloseHandle(token);
    LPWSTR sidText = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(userBytes.data())->User.Sid, &sidText))
        return HRESULT_FROM_WIN32(GetLastError());
    std::wstring sddl = L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;" + std::wstring(sidText) + L")S:(ML;;NW;;;ME)";
    LocalFree(sidText);
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &descriptor, nullptr))
        return HRESULT_FROM_WIN32(GetLastError());
    GUID id{}; CoCreateGuid(&id);
    wchar_t guid[64]{}; StringFromGUID2(id, guid, ARRAYSIZE(guid));
    const std::wstring pipeName = L"\\\\.\\pipe\\OpenSalamander.PictView." +
                                  std::to_wstring(GetCurrentProcessId()) + L"." + guid;
    SECURITY_ATTRIBUTES security{sizeof(security), descriptor, FALSE};
    HANDLE pipe = CreateNamedPipeW(pipeName.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS, 1, 4096, 4096, 0, &security);
    LocalFree(descriptor);
    if (pipe == INVALID_HANDLE_VALUE) return HRESULT_FROM_WIN32(GetLastError());
    OVERLAPPED connect{}; connect.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (connect.hEvent == nullptr) { CloseHandle(pipe); return HRESULT_FROM_WIN32(GetLastError()); }
    BOOL connected = ConnectNamedPipe(pipe, &connect);
    DWORD connectError = connected ? ERROR_SUCCESS : GetLastError();
    const std::wstring directory = ModuleDirectory();
    const std::wstring executable = directory + L"\\pictview-previewhost.exe";
    const std::wstring arguments = L"\"" + pipeName + L"\" " + std::to_wstring(GetCurrentProcessId());
    HRESULT hr = ShellExecuteUnelevated(executable, arguments, directory);
    if (SUCCEEDED(hr))
    {
        if (!connected && connectError == ERROR_PIPE_CONNECTED) connected = TRUE;
        else if (!connected && connectError == ERROR_IO_PENDING)
            connected = WaitForSingleObject(connect.hEvent, kHelperConnectTimeoutMs) == WAIT_OBJECT_0 &&
                        GetOverlappedResult(pipe, &connect, &bytes, FALSE);
        else if (!connected) hr = HRESULT_FROM_WIN32(connectError);
    }
    if (!connected && connectError == ERROR_IO_PENDING)
    {
        CancelIoEx(pipe, &connect);
        DWORD ignored = 0;
        if (!GetOverlappedResult(pipe, &connect, &ignored, TRUE) && GetLastError() != ERROR_OPERATION_ABORTED)
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    CloseHandle(connect.hEvent);
    if (FAILED(hr) || !connected) { CloseHandle(pipe); return FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_TIMEOUT); }
    ULONG clientPid = 0;
    DWORD parentSession = 0, clientSession = 0;
    if (!GetNamedPipeClientProcessId(pipe, &clientPid) ||
        !ProcessIdToSessionId(GetCurrentProcessId(), &parentSession) ||
        !ProcessIdToSessionId(clientPid, &clientSession) || parentSession != clientSession)
    { CloseHandle(pipe); return E_ACCESSDENIED; }
    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, clientPid);
    if (process == nullptr) { CloseHandle(pipe); return E_ACCESSDENIED; }
    handle.previewPipe = pipe;
    handle.previewProcess = process;
    handle.previewHelper = true;
    const DWORD pathBytes = static_cast<DWORD>((handle.fileName.size() + 1) * sizeof(wchar_t));
    hr = SendHelperRequest(handle, PictViewPreviewProtocol::Command::Open, handle.fileName.c_str(), pathBytes);
    if (FAILED(hr))
    {
        CloseHandle(handle.previewProcess); handle.previewProcess = nullptr;
        CloseHandle(handle.previewPipe); handle.previewPipe = INVALID_HANDLE_VALUE;
        handle.previewHelper = false;
    }
    return hr;
}

} // namespace

bool IsStlExtension(const std::wstring& path)
{
    const size_t slash = path.find_last_of(L"\\/");
    const size_t dot = path.find_last_of(L'.');
    if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash))
    {
        return false;
    }
    return path.size() - dot == 4 &&
           (path[dot + 1] == L's' || path[dot + 1] == L'S') &&
           (path[dot + 2] == L't' || path[dot + 2] == L'T') &&
           (path[dot + 3] == L'l' || path[dot + 3] == L'L');
}

namespace
{

bool ReadDefaultRegistryString(const std::wstring& keyPath, std::wstring& value)
{
    value.clear();
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
    {
        return false;
    }
    DWORD type = 0;
    DWORD bytes = 0;
    LONG result = RegQueryValueExW(key, nullptr, nullptr, &type, nullptr, &bytes);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ) || bytes < sizeof(wchar_t))
    {
        RegCloseKey(key);
        return false;
    }
    std::vector<wchar_t> buffer(bytes / sizeof(wchar_t) + 1, L'\0');
    result = RegQueryValueExW(key, nullptr, nullptr, &type, reinterpret_cast<BYTE*>(buffer.data()), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || buffer[0] == L'\0')
    {
        return false;
    }
    value.assign(buffer.data());
    return true;
}

bool LookupRegisteredPreviewClsid(const wchar_t* assoc, CLSID& clsid)
{
    WCHAR text[80]{};
    DWORD chars = static_cast<DWORD>(sizeof(text) / sizeof(text[0]));
    if (SUCCEEDED(AssocQueryStringW(ASSOCF_NOTRUNCATE, ASSOCSTR_SHELLEXTENSION, assoc,
                                    kPreviewHandlerShellEx, text, &chars)) && text[0] != 0 &&
        SUCCEEDED(CLSIDFromString(text, &clsid)))
    {
        return true;
    }

    std::wstring value;
    const std::wstring directKey = std::wstring(assoc) + L"\\shellex\\" + kPreviewHandlerShellEx;
    if (!ReadDefaultRegistryString(directKey, value))
    {
        return false;
    }
    return SUCCEEDED(CLSIDFromString(value.c_str(), &clsid));
}

HRESULT CoCreatePreviewHandler(const CLSID& clsid, IPreviewHandler** handler)
{
    // Shell preview handlers run in the PreviewHost surrogate. Never load an
    // arbitrary preview DLL into PictView; deterministic out-of-process
    // activation also keeps both Salamander instances on the same code path.
    return CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER, IID_PPV_ARGS(handler));
}

HRESULT InitializePreviewHandler(IPreviewHandler* handler, const wchar_t* path)
{
    ComPtr<IShellItem> item;
    HRESULT hr = SHCreateItemFromParsingName(path, nullptr, IID_PPV_ARGS(&item));
    if (SUCCEEDED(hr) && item)
    {
        ComPtr<IInitializeWithItem> initItem;
        if (SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&initItem))))
        {
            hr = initItem->Initialize(item.Get(), STGM_READ);
            if (SUCCEEDED(hr))
            {
                return S_OK;
            }
        }
    }

    ComPtr<IInitializeWithFile> initFile;
    if (SUCCEEDED(handler->QueryInterface(IID_PPV_ARGS(&initFile))))
    {
        hr = initFile->Initialize(path, STGM_READ);
        if (SUCCEEDED(hr))
        {
            return S_OK;
        }
    }

    ComPtr<IInitializeWithStream> initStream;
    if (FAILED(handler->QueryInterface(IID_PPV_ARGS(&initStream))))
    {
        return E_NOINTERFACE;
    }
    ComPtr<IStream> stream;
    hr = SHCreateStreamOnFileEx(path, STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, nullptr, &stream);
    if (FAILED(hr) || !stream)
    {
        return FAILED(hr) ? hr : E_FAIL;
    }
    return initStream->Initialize(stream.Get(), STGM_READ);
}

HRESULT AdoptDummyFrame(ImageHandle& handle)
{
    FrameData frame;
    frame.width = 1;
    frame.height = 1;
    frame.stride = 4;
    frame.displayStride = 4;
    try
    {
        frame.pixels.assign(4, 0);
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    frame.pixels[3] = 255;
    frame.decoded = true;
    frame.hasTransparency = false;
    frame.reportedColors = PV_COLOR_TC32;
    frame.reportedBitDepth = 32;
    frame.colorModel = PVCM_RGB;
    frame.bitsPerPixel = 32;
    try
    {
        handle.frames.push_back(std::move(frame));
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
    handle.baseInfo.Format = PVF_STL;
    handle.baseInfo.Flags = 0;
    handle.baseInfo.NumOfImages = 1;
    return S_OK;
}

PreviewHandlerFrame* NewPreviewHandlerFrame()
{
#if defined(_DEBUG)
#undef new
#define PICTVIEW_PREVIEW_NEW
#endif
    PreviewHandlerFrame* frame = new (std::nothrow) PreviewHandlerFrame();
#if defined(PICTVIEW_PREVIEW_NEW)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef PICTVIEW_PREVIEW_NEW
#endif
    return frame;
}

HRESULT CreateInitializedStlPreviewHandler(const wchar_t* path, IPreviewHandler** handler, IUnknown** siteOut)
{
    if (path == nullptr || handler == nullptr || siteOut == nullptr)
    {
        return E_POINTER;
    }
    *handler = nullptr;
    *siteOut = nullptr;

    CLSID clsid{};
    ComPtr<IPreviewHandler> preview;
    HRESULT hr = E_FAIL;
    bool haveRegisteredAssoc = false;

    // Preview handlers may be registered against the extension or the
    // SystemFileAssociations fallback. Try both standard association scopes
    // before using the packaged Microsoft 3D Viewer CLSID.
    const wchar_t* const associationScopes[] = {
        L".stl", L"SystemFileAssociations\\.stl"};
    for (const wchar_t* scope : associationScopes)
    {
        if (LookupRegisteredPreviewClsid(scope, clsid))
        {
            haveRegisteredAssoc = true;
            wchar_t diagnostic[160]{};
            StringCchPrintfW(diagnostic, ARRAYSIZE(diagnostic),
                             L"PictView STL preview candidate (%s): {%08lX-%04X-%04X-...}\n",
                             scope, static_cast<unsigned long>(clsid.Data1),
                             static_cast<unsigned>(clsid.Data2), static_cast<unsigned>(clsid.Data3));
            OutputDebugStringW(diagnostic);
            hr = CoCreatePreviewHandler(clsid, preview.GetAddressOf());
            if (SUCCEEDED(hr) && preview)
                break;
            preview.Reset();
        }
    }
    if (!preview)
    {
        std::wstring progId;
        if (ReadDefaultRegistryString(L".stl", progId) && !progId.empty() &&
            LookupRegisteredPreviewClsid(progId.c_str(), clsid))
        {
            hr = CoCreatePreviewHandler(clsid, preview.GetAddressOf());
        }
    }
    if (!preview)
    {
        preview.Reset();
        OutputDebugStringW(L"PictView STL preview candidate: Microsoft 3D Viewer fallback\n");
        hr = CoCreatePreviewHandler(kClsid3DViewerPreview, preview.GetAddressOf());
    }
    if (FAILED(hr) || !preview)
    {
        return FAILED(hr) ? hr : WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
    }

    PreviewHandlerFrame* frame = NewPreviewHandlerFrame();
    if (frame == nullptr)
    {
        preview->Unload();
        return E_OUTOFMEMORY;
    }
    ComPtr<IUnknown> site;
    const HRESULT qiHr = static_cast<IPreviewHandlerFrame*>(frame)->QueryInterface(IID_PPV_ARGS(&site));
    frame->Release();
    if (FAILED(qiHr) || !site)
    {
        preview->Unload();
        return FAILED(qiHr) ? qiHr : E_NOINTERFACE;
    }
    ComPtr<IObjectWithSite> objectWithSite;
    if (SUCCEEDED(preview.As(&objectWithSite)) && objectWithSite)
    {
        objectWithSite->SetSite(site.Get());
    }

    hr = InitializePreviewHandler(preview.Get(), path);
    if (FAILED(hr))
    {
        if (objectWithSite)
        {
            objectWithSite->SetSite(nullptr);
        }
        preview->Unload();
        preview.Reset();
        if (haveRegisteredAssoc && clsid != kClsid3DViewerPreview)
        {
            // A stale or broken .stl association must not suppress the
            // Microsoft 3D Viewer handler available on the same machine.
            hr = CoCreatePreviewHandler(kClsid3DViewerPreview, preview.GetAddressOf());
            if (SUCCEEDED(hr) && preview)
            {
                ComPtr<IObjectWithSite> fallbackSite;
                if (SUCCEEDED(preview.As(&fallbackSite)) && fallbackSite)
                    fallbackSite->SetSite(site.Get());
                hr = InitializePreviewHandler(preview.Get(), path);
                if (SUCCEEDED(hr))
                {
                    *handler = preview.Detach();
                    *siteOut = site.Detach();
                    return S_OK;
                }
                preview->Unload();
            }
        }
        return hr;
    }

    *handler = preview.Detach();
    *siteOut = site.Detach();
    return S_OK;
}

} // namespace

bool HandleHasInteractivePreview(const ImageHandle& handle)
{
    return handle.previewHandler != nullptr || handle.previewHelper;
}

void ReleaseInteractivePreview(ImageHandle& handle)
{
    if (handle.previewHelper)
        SendHelperRequest(handle, PictViewPreviewProtocol::Command::Close);
    if (handle.previewPipe != INVALID_HANDLE_VALUE)
    {
        CloseHandle(handle.previewPipe);
        handle.previewPipe = INVALID_HANDLE_VALUE;
    }
    if (handle.previewProcess != nullptr)
    {
        CloseHandle(handle.previewProcess);
        handle.previewProcess = nullptr;
    }
    handle.previewHelper = false;
    handle.previewWindow = nullptr;
    if (handle.previewHandler)
    {
        ComPtr<IPreviewHandler> preview;
        if (SUCCEEDED(handle.previewHandler.As(&preview)) && preview) preview->Unload();
        ComPtr<IObjectWithSite> site;
        if (SUCCEEDED(handle.previewHandler.As(&site)) && site) site->SetSite(nullptr);
    }
    handle.previewHandler.Reset();
    handle.previewSite.Reset();
    handle.previewVisible = false;
}

HRESULT TryOpenInteractivePreview(ImageHandle& handle)
{
    APTTYPE apartmentType = APTTYPE_CURRENT;
    APTTYPEQUALIFIER apartmentQualifier = APTTYPEQUALIFIER_NONE;
    const HRESULT apartmentHr = CoGetApartmentType(&apartmentType, &apartmentQualifier);
    if (FAILED(apartmentHr) || (apartmentType != APTTYPE_STA && apartmentType != APTTYPE_MAINSTA))
        return FAILED(apartmentHr) ? apartmentHr : RPC_E_CHANGED_MODE;
    if (handle.fileName.empty() || !IsStlExtension(handle.fileName) ||
        (handle.openFlags & (PVFF_FAST | PVOF_THUMBNAIL)) != 0)
        return WINCODEC_ERR_UNKNOWNIMAGEFORMAT;

    ComPtr<IPreviewHandler> preview;
    ComPtr<IUnknown> site;
    HRESULT hr = E_FAIL;
    const bool elevated = IsProcessElevated();
    if (!elevated)
        hr = CreateInitializedStlPreviewHandler(handle.fileName.c_str(), preview.GetAddressOf(), site.GetAddressOf());
    if (elevated)
    {
        hr = StartPreviewHelper(handle);
        if (SUCCEEDED(hr))
        {
            handle.frames.clear();
            hr = AdoptDummyFrame(handle);
            if (FAILED(hr)) ReleaseInteractivePreview(handle);
            return hr;
        }
    }
    if (FAILED(hr) || !preview)
        return FAILED(hr) ? hr : WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
    handle.frames.clear();
    hr = AdoptDummyFrame(handle);
    if (FAILED(hr))
    {
        ComPtr<IObjectWithSite> objectWithSite;
        if (SUCCEEDED(preview.As(&objectWithSite)) && objectWithSite) objectWithSite->SetSite(nullptr);
        preview->Unload();
        return hr;
    }
    handle.previewHandler = preview;
    handle.previewSite = site;
    handle.previewVisible = false;
    return S_OK;
}

HRESULT ShowInteractivePreview(ImageHandle& handle, HWND hwnd, const RECT& rect, COLORREF background)
{
    if (hwnd == nullptr) return E_INVALIDARG;
    if (handle.previewHelper)
    {
        POINT points[2] = {{rect.left, rect.top}, {rect.right, rect.bottom}};
        SetLastError(ERROR_SUCCESS);
        if (!MapWindowPoints(hwnd, nullptr, points, 2) && GetLastError() != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(GetLastError());
        RECT screenRect{points[0].x, points[0].y, points[1].x, points[1].y};
        PictViewPreviewProtocol::AttachPayload payload{static_cast<std::uint64_t>(reinterpret_cast<ULONG_PTR>(hwnd)), screenRect, background};
        HRESULT hr = SendHelperRequest(handle, PictViewPreviewProtocol::Command::Attach, &payload, sizeof(payload));
        if (SUCCEEDED(hr))
        {
            handle.previewWindow = hwnd;
            handle.previewVisible = true;
            SendHelperRequest(handle, PictViewPreviewProtocol::Command::Focus);
        }
        return hr;
    }
    if (handle.previewHandler == nullptr) return E_INVALIDARG;
    HWND host = GetParent(hwnd);
    BindPreviewFrameHost(handle.previewSite.Get(), host != nullptr ? host : hwnd);
    ComPtr<IPreviewHandler> preview;
    HRESULT hr = handle.previewHandler.As(&preview);
    if (FAILED(hr) || !preview) return FAILED(hr) ? hr : E_NOINTERFACE;
    ComPtr<IPreviewHandlerVisuals> visuals;
    if (SUCCEEDED(preview.As(&visuals)) && visuals)
    {
        visuals->SetBackgroundColor(background);
        const bool dark = static_cast<unsigned>(GetRValue(background)) + GetGValue(background) + GetBValue(background) < 384u;
        visuals->SetTextColor(dark ? RGB(255, 255, 255) : RGB(0, 0, 0));
    }
    hr = preview->SetWindow(hwnd, &rect);
    if (FAILED(hr)) return hr;
    if (!handle.previewVisible)
    {
        hr = preview->DoPreview();
        if (FAILED(hr)) return hr;
        handle.previewVisible = true;
    }
    hr = preview->SetRect(&rect);
    if (SUCCEEDED(hr)) preview->SetFocus();
    return hr;
}

HRESULT ResizeInteractivePreview(ImageHandle& handle, const RECT& rect)
{
    if (handle.previewHelper && handle.previewVisible && handle.previewWindow != nullptr)
    {
        POINT points[2] = {{rect.left, rect.top}, {rect.right, rect.bottom}};
        SetLastError(ERROR_SUCCESS);
        if (!MapWindowPoints(handle.previewWindow, nullptr, points, 2) && GetLastError() != ERROR_SUCCESS)
            return HRESULT_FROM_WIN32(GetLastError());
        PictViewPreviewProtocol::ResizePayload payload{{points[0].x, points[0].y, points[1].x, points[1].y}};
        return SendHelperRequest(handle, PictViewPreviewProtocol::Command::Resize, &payload, sizeof(payload));
    }
    if (handle.previewHandler == nullptr || !handle.previewVisible) return S_FALSE;
    ComPtr<IPreviewHandler> preview;
    const HRESULT hr = handle.previewHandler.As(&preview);
    return FAILED(hr) || !preview ? (FAILED(hr) ? hr : E_NOINTERFACE) : preview->SetRect(&rect);
}
} // namespace PictView::Wic
