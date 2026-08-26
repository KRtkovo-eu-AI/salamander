// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "ShellPreviewHost.h"

#include <new>

#include <shlwapi.h>
#include <shobjidl.h>
#include <propsys.h>

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

class PreviewHandlerFrame final : public IPreviewHandlerFrame, public IServiceProvider
{
public:
    PreviewHandlerFrame() = default;
    PreviewHandlerFrame(const PreviewHandlerFrame&) = delete;
    PreviewHandlerFrame& operator=(const PreviewHandlerFrame&) = delete;

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
        info->haccel = nullptr;
        info->cAccelEntries = 0;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE TranslateAccelerator(MSG* /*pmsg*/) override
    {
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
    ~PreviewHandlerFrame() = default;
    LONG m_ref = 1;
};

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

bool IsStlExtension(const std::wstring& path)
{
    return ExtensionLower(path) == L"stl";
}

bool LookupRegisteredPreviewClsid(const wchar_t* assoc, CLSID& clsid)
{
    WCHAR text[80]{};
    DWORD chars = static_cast<DWORD>(sizeof(text) / sizeof(text[0]));
    const HRESULT hr = AssocQueryStringW(ASSOCF_NOTRUNCATE, ASSOCSTR_SHELLEXTENSION, assoc,
                                         kPreviewHandlerShellEx, text, &chars);
    if (FAILED(hr) || text[0] == 0)
    {
        return false;
    }
    return SUCCEEDED(CLSIDFromString(text, &clsid));
}

HRESULT CoCreatePreviewHandler(const CLSID& clsid, IPreviewHandler** handler)
{
    return CoCreateInstance(clsid, nullptr,
                            CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER | CLSCTX_INPROC_HANDLER,
                            IID_PPV_ARGS(handler));
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

} // namespace

bool HandleHasInteractivePreview(const ImageHandle& handle)
{
    return handle.previewHandler != nullptr;
}

void ReleaseInteractivePreview(ImageHandle& handle)
{
    if (handle.previewHandler)
    {
        ComPtr<IPreviewHandler> preview;
        if (SUCCEEDED(handle.previewHandler.As(&preview)) && preview)
        {
            preview->Unload();
        }
        ComPtr<IObjectWithSite> site;
        if (SUCCEEDED(handle.previewHandler.As(&site)) && site)
        {
            site->SetSite(nullptr);
        }
    }
    handle.previewHandler.Reset();
    handle.previewSite.Reset();
    handle.previewVisible = false;
}

HRESULT TryOpenInteractivePreview(ImageHandle& handle)
{
    if (handle.fileName.empty() || !IsStlExtension(handle.fileName))
    {
        return WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
    }
    if ((handle.openFlags & PVFF_FAST) != 0 || (handle.openFlags & PVOF_THUMBNAIL) != 0)
    {
        return WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
    }

    CLSID clsid{};
    const bool haveAssoc = LookupRegisteredPreviewClsid(L".stl", clsid);

    ComPtr<IPreviewHandler> preview;
    HRESULT hr = E_FAIL;
    if (haveAssoc)
    {
        hr = CoCreatePreviewHandler(clsid, preview.GetAddressOf());
    }
    if (FAILED(hr))
    {
        preview.Reset();
        hr = CoCreatePreviewHandler(kClsid3DViewerPreview, preview.GetAddressOf());
    }
    if (FAILED(hr) || !preview)
    {
        return FAILED(hr) ? hr : WINCODEC_ERR_UNKNOWNIMAGEFORMAT;
    }

#if defined(_DEBUG)
#undef new
#define PICTVIEW_PREVIEW_NEW
#endif
    PreviewHandlerFrame* frame = new (std::nothrow) PreviewHandlerFrame();
#if defined(PICTVIEW_PREVIEW_NEW)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef PICTVIEW_PREVIEW_NEW
#endif
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

    hr = InitializePreviewHandler(preview.Get(), handle.fileName.c_str());
    if (FAILED(hr))
    {
        if (objectWithSite)
        {
            objectWithSite->SetSite(nullptr);
        }
        preview->Unload();
        return hr;
    }

    handle.frames.clear();
    hr = AdoptDummyFrame(handle);
    if (FAILED(hr))
    {
        if (objectWithSite)
        {
            objectWithSite->SetSite(nullptr);
        }
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
    if (hwnd == nullptr || handle.previewHandler == nullptr)
    {
        return E_INVALIDARG;
    }
    ComPtr<IPreviewHandler> preview;
    HRESULT hr = handle.previewHandler.As(&preview);
    if (FAILED(hr) || !preview)
    {
        return FAILED(hr) ? hr : E_NOINTERFACE;
    }

    ComPtr<IPreviewHandlerVisuals> visuals;
    if (SUCCEEDED(preview.As(&visuals)) && visuals)
    {
        visuals->SetBackgroundColor(background);
        const BYTE r = GetRValue(background);
        const BYTE g = GetGValue(background);
        const BYTE b = GetBValue(background);
        const bool dark = (static_cast<unsigned>(r) + g + b) < 384u;
        visuals->SetTextColor(dark ? RGB(255, 255, 255) : RGB(0, 0, 0));
    }

    hr = preview->SetWindow(hwnd, &rect);
    if (FAILED(hr))
    {
        return hr;
    }
    if (!handle.previewVisible)
    {
        hr = preview->DoPreview();
        if (FAILED(hr))
        {
            return hr;
        }
        handle.previewVisible = true;
    }
    hr = preview->SetRect(&rect);
    if (FAILED(hr))
    {
        return hr;
    }
    preview->SetFocus();
    return S_OK;
}

HRESULT ResizeInteractivePreview(ImageHandle& handle, const RECT& rect)
{
    if (handle.previewHandler == nullptr || !handle.previewVisible)
    {
        return S_FALSE;
    }
    ComPtr<IPreviewHandler> preview;
    const HRESULT hr = handle.previewHandler.As(&preview);
    if (FAILED(hr) || !preview)
    {
        return FAILED(hr) ? hr : E_NOINTERFACE;
    }
    return preview->SetRect(&rect);
}

} // namespace PictView::Wic
