// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "WicBackend.h"

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>

#include <objbase.h>
#include <shlwapi.h>
#include <strsafe.h>

#include "../Thumbnailer.h"

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std::string_literals;

namespace PictView::Wic
{
namespace
{
constexpr DWORD kBackendVersion = PV_VERSION_156;

std::mutex g_errorMutex;
std::unordered_map<DWORD, std::string> g_errorTexts = {
    {PVC_OK, "OK"},
    {PVC_CANNOT_OPEN_FILE, "Unable to open image."},
    {PVC_UNSUP_FILE_TYPE, "Image format is not supported by the WIC backend."},
    {PVC_UNSUP_OUT_PARAMS, "Requested output parameters are not supported by the WIC backend."},
    {PVC_OUT_OF_MEMORY, "Out of memory."},
    {PVC_INVALID_DIMENSIONS, "Requested dimensions are invalid."},
    {PVC_CANCELED, "Operation canceled."},
    {PVC_GDI_ERROR, "A GDI call failed."},
};

const char* LookupError(DWORD code)
{
    std::lock_guard<std::mutex> lock(g_errorMutex);
    const auto it = g_errorTexts.find(code);
    if (it != g_errorTexts.end())
    {
        return it->second.c_str();
    }
    static std::string fallback = "Unknown WIC error.";
    return fallback.c_str();
}

struct GuidMapping
{
    DWORD format;
    GUID container;
    GUID pixelFormat;
};

const GuidMapping kEncoderMappings[] = {
    {PVF_BMP, GUID_ContainerFormatBmp, GUID_WICPixelFormat32bppBGRA},
    {PVF_PNG, GUID_ContainerFormatPng, GUID_WICPixelFormat32bppBGRA},
    {PVF_JPG, GUID_ContainerFormatJpeg, GUID_WICPixelFormat24bppBGR},
    {PVF_TIFF, GUID_ContainerFormatTiff, GUID_WICPixelFormat32bppBGRA},
    {PVF_GIF, GUID_ContainerFormatGif, GUID_WICPixelFormat8bppIndexed},
    {PVF_ICO, GUID_ContainerFormatIco, GUID_WICPixelFormat32bppBGRA},
};

HRESULT CreateDecoder(Backend& backend, const std::wstring& path, IWICBitmapDecoder** decoder)
{
    auto factory = backend.Factory();
    if (!factory)
    {
        return E_POINTER;
    }
    return factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder);
}

HRESULT EnsureConverter(ImageHandle& handle, size_t index)
{
    if (index >= handle.frames.size())
    {
        return E_INVALIDARG;
    }
    FrameData& frame = handle.frames[index];
    if (frame.converter)
    {
        return S_OK;
    }
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    IWICImagingFactory* factory = handle.backend->Factory();
    if (!factory)
    {
        return E_FAIL;
    }
    HRESULT hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = converter->Initialize(frame.frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr))
    {
        return hr;
    }
    frame.converter = converter;
    return S_OK;
}

HRESULT DecodeFrame(ImageHandle& handle, size_t index)
{
    if (index >= handle.frames.size())
    {
        return E_INVALIDARG;
    }
    FrameData& frame = handle.frames[index];
    if (frame.decoded)
    {
        return S_OK;
    }

    HRESULT hr = EnsureConverter(handle, index);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = frame.frame->GetSize(&frame.width, &frame.height);
    if (FAILED(hr))
    {
        return hr;
    }
    frame.stride = frame.width * 4;
    const size_t bufferSize = static_cast<size_t>(frame.stride) * static_cast<size_t>(frame.height);
    frame.pixels.resize(bufferSize);

    WICRect rect{0, 0, static_cast<INT>(frame.width), static_cast<INT>(frame.height)};
    hr = frame.converter->CopyPixels(&rect, frame.stride, static_cast<UINT>(frame.pixels.size()), frame.pixels.data());
    if (FAILED(hr))
    {
        frame.pixels.clear();
        return hr;
    }

    frame.linePointers.resize(frame.height);
    for (UINT y = 0; y < frame.height; ++y)
    {
        frame.linePointers[y] = frame.pixels.data() + static_cast<size_t>(y) * frame.stride;
    }
    frame.palette.clear();

    frame.bmi.biSize = sizeof(BITMAPINFOHEADER);
    frame.bmi.biWidth = static_cast<LONG>(frame.width);
    frame.bmi.biHeight = -static_cast<LONG>(frame.height);
    frame.bmi.biPlanes = 1;
    frame.bmi.biBitCount = 32;
    frame.bmi.biCompression = BI_RGB;
    frame.bmi.biSizeImage = static_cast<DWORD>(frame.pixels.size());
    frame.bmi.biXPelsPerMeter = 0;
    frame.bmi.biYPelsPerMeter = 0;

    void* bits = nullptr;
    BITMAPINFO bmi{};
    bmi.bmiHeader = frame.bmi;
    frame.hbitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!frame.hbitmap)
    {
        return E_OUTOFMEMORY;
    }
    if (bits)
    {
        memcpy(bits, frame.pixels.data(), frame.pixels.size());
    }

    frame.decoded = true;
    return S_OK;
}

PVCODE HResultToPvCode(HRESULT hr)
{
    if (SUCCEEDED(hr))
    {
        return PVC_OK;
    }
    switch (hr)
    {
    case E_OUTOFMEMORY:
        return PVC_OUT_OF_MEMORY;
    case WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT:
    case WINCODEC_ERR_COMPONENTNOTFOUND:
    case WINCODEC_ERR_UNSUPPORTEDOPERATION:
        return PVC_UNSUP_FILE_TYPE;
    default:
        return PVC_EXCEPTION;
    }
}

std::wstring Utf8ToWide(const char* path)
{
    if (!path)
    {
        return std::wstring();
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
    if (len <= 0)
    {
        len = MultiByteToWideChar(CP_ACP, 0, path, -1, nullptr, 0);
        if (len <= 0)
        {
            return std::wstring();
        }
        std::wstring wide;
        wide.resize(static_cast<size_t>(len));
        MultiByteToWideChar(CP_ACP, 0, path, -1, wide.data(), len);
        if (!wide.empty() && wide.back() == L'\0')
        {
            wide.pop_back();
        }
        return wide;
    }
    std::wstring wide;
    wide.resize(static_cast<size_t>(len));
    MultiByteToWideChar(CP_UTF8, 0, path, -1, wide.data(), len);
    if (!wide.empty() && wide.back() == L'\0')
    {
        wide.pop_back();
    }
    return wide;
}

PVCODE PopulateImageInfo(ImageHandle& handle, FrameData& frame, LPPVImageInfo info)
{
    if (!info)
    {
        return PVC_INVALID_HANDLE;
    }
    if (info->cbSize < sizeof(PVImageInfo))
    {
        return PVC_INVALID_HANDLE;
    }

    memset(info, 0, sizeof(PVImageInfo));
    info->cbSize = sizeof(PVImageInfo);
    info->Width = frame.width;
    info->Height = frame.height;
    info->BytesPerLine = frame.stride;
    info->Colors = PV_COLOR_TC32;
    info->Format = handle.baseInfo.Format;
    info->Flags = 0;
    info->ColorModel = PVCM_RGB;
    info->NumOfImages = static_cast<DWORD>(handle.frames.size());
    info->CurrentImage = 0;
    StringCchCopyA(info->Info1, PV_MAX_INFO_LEN, "WIC");
    info->TotalBitDepth = 32;
    return PVC_OK;
}

DWORD MapFormatToPvFormat(const GUID& container)
{
    if (container == GUID_ContainerFormatBmp)
        return PVF_BMP;
    if (container == GUID_ContainerFormatPng)
        return PVF_PNG;
    if (container == GUID_ContainerFormatJpeg)
        return PVF_JPG;
    if (container == GUID_ContainerFormatGif)
        return PVF_GIF;
    if (container == GUID_ContainerFormatTiff)
        return PVF_TIFF;
    if (container == GUID_ContainerFormatIco)
        return PVF_ICO;
    return PVF_BMP;
}

HRESULT CollectFrames(Backend& backend, IWICBitmapDecoder* decoder, ImageHandle& handle)
{
    UINT frameCount = 0;
    HRESULT hr = decoder->GetFrameCount(&frameCount);
    if (FAILED(hr))
    {
        return hr;
    }
    handle.frames.resize(frameCount);
    for (UINT i = 0; i < frameCount; ++i)
    {
        FrameData data;
        hr = decoder->GetFrame(i, &data.frame);
        if (FAILED(hr))
        {
            return hr;
        }
        handle.frames[i] = std::move(data);
    }
    GUID container = {};
    decoder->GetContainerFormat(&container);
    handle.baseInfo.Format = MapFormatToPvFormat(container);
    handle.baseInfo.NumOfImages = frameCount;
    return S_OK;
}

PVCODE DrawFrame(FrameData& frame, HDC dc, int x, int y, LPRECT rect)
{
    if (!dc)
    {
        return PVC_OK;
    }

    RECT dest{};
    if (rect)
    {
        dest = *rect;
    }
    else
    {
        dest.left = x;
        dest.top = y;
        dest.right = x + static_cast<int>(frame.width);
        dest.bottom = y + static_cast<int>(frame.height);
    }

    const int destWidth = dest.right - dest.left;
    const int destHeight = dest.bottom - dest.top;
    if (destWidth <= 0 || destHeight <= 0)
    {
        return PVC_OK;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader = frame.bmi;
    int result = StretchDIBits(dc, dest.left, dest.top, destWidth, destHeight, 0, 0, frame.width, frame.height,
                               frame.pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
    if (result == GDI_ERROR)
    {
        return PVC_GDI_ERROR;
    }
    return PVC_OK;
}

PVCODE CreateSequenceNodes(ImageHandle& handle, LPPVImageSequence* seq)
{
    if (!seq)
    {
        return PVC_INVALID_HANDLE;
    }
    *seq = nullptr;
    LPPVImageSequence head = nullptr;
    LPPVImageSequence* tail = seq;
    for (size_t i = 0; i < handle.frames.size(); ++i)
    {
        FrameData& frame = handle.frames[i];
        HRESULT hr = DecodeFrame(handle, i);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }
        auto node = std::make_unique<PVImageSequence>();
        node->pNext = nullptr;
        node->Rect.left = 0;
        node->Rect.top = 0;
        node->Rect.right = frame.width;
        node->Rect.bottom = frame.height;
        node->Delay = 0;
        node->DisposalMethod = PVDM_UNDEFINED;
        node->ImgHandle = frame.hbitmap;
        node->TransparentHandle = nullptr;
        *tail = node.release();
        tail = &((*tail)->pNext);
    }
    *tail = nullptr;
    return PVC_OK;
}

PVCODE SaveFrame(ImageHandle& handle, int imageIndex, const wchar_t* path, const GuidMapping& mapping,
                 LPPVSaveImageInfo info)
{
    if (imageIndex < 0 || static_cast<size_t>(imageIndex) >= handle.frames.size())
    {
        return PVC_INVALID_HANDLE;
    }
    FrameData& frame = handle.frames[static_cast<size_t>(imageIndex)];
    HRESULT hr = DecodeFrame(handle, static_cast<size_t>(imageIndex));
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    Microsoft::WRL::ComPtr<IWICBitmapEncoder> encoder;
    hr = handle.backend->Factory()->CreateEncoder(mapping.container, nullptr, &encoder);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    Microsoft::WRL::ComPtr<IWICStream> stream;
    hr = handle.backend->Factory()->CreateStream(&stream);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    hr = stream->InitializeFromFilename(path, GENERIC_WRITE);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    Microsoft::WRL::ComPtr<IWICBitmapFrameEncode> frameEncode;
    Microsoft::WRL::ComPtr<IPropertyBag2> bag;
    hr = encoder->CreateNewFrame(&frameEncode, &bag);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    hr = frameEncode->Initialize(bag.Get());
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    hr = frameEncode->SetSize(frame.width, frame.height);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    GUID pixelFormat = mapping.pixelFormat;
    hr = frameEncode->SetPixelFormat(&pixelFormat);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    if (pixelFormat != mapping.pixelFormat)
    {
        return PVC_UNSUP_FILE_TYPE;
    }

    if (mapping.pixelFormat == GUID_WICPixelFormat24bppBGR)
    {
        const UINT stride = frame.width * 3;
        std::vector<BYTE> rgb(stride * frame.height);
        for (UINT y = 0; y < frame.height; ++y)
        {
            const BYTE* src = frame.pixels.data() + y * frame.stride;
            BYTE* dst = rgb.data() + y * stride;
            for (UINT x = 0; x < frame.width; ++x)
            {
                dst[x * 3 + 0] = src[x * 4 + 0];
                dst[x * 3 + 1] = src[x * 4 + 1];
                dst[x * 3 + 2] = src[x * 4 + 2];
            }
        }
        hr = frameEncode->WritePixels(frame.height, stride, static_cast<UINT>(rgb.size()), rgb.data());
    }
    else if (mapping.pixelFormat == GUID_WICPixelFormat8bppIndexed)
    {
        Microsoft::WRL::ComPtr<IWICBitmap> bitmap;
        hr = handle.backend->Factory()->CreateBitmapFromMemory(frame.width, frame.height,
                                                               GUID_WICPixelFormat32bppBGRA, frame.stride,
                                                               static_cast<UINT>(frame.pixels.size()),
                                                               frame.pixels.data(), &bitmap);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        Microsoft::WRL::ComPtr<IWICPalette> palette;
        hr = handle.backend->Factory()->CreatePalette(&palette);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        hr = palette->InitializeFromBitmap(bitmap.Get(), 256, FALSE);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        hr = frameEncode->SetPalette(palette.Get());
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        Microsoft::WRL::ComPtr<IWICFormatConverter> gifConverter;
        hr = handle.backend->Factory()->CreateFormatConverter(&gifConverter);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        hr = gifConverter->Initialize(bitmap.Get(), GUID_WICPixelFormat8bppIndexed, WICBitmapDitherTypeErrorDiffusion,
                                      palette.Get(), 0.0, WICBitmapPaletteTypeCustom);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        const UINT stride = frame.width;
        std::vector<BYTE> indexed(static_cast<size_t>(stride) * frame.height);
        WICRect rect{0, 0, static_cast<INT>(frame.width), static_cast<INT>(frame.height)};
        hr = gifConverter->CopyPixels(&rect, stride, static_cast<UINT>(indexed.size()), indexed.data());
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        hr = frameEncode->WritePixels(frame.height, stride, static_cast<UINT>(indexed.size()), indexed.data());
    }
    else
    {
        hr = frameEncode->WritePixels(frame.height, frame.stride, static_cast<UINT>(frame.pixels.size()), frame.pixels.data());
    }
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    hr = frameEncode->Commit();
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    hr = encoder->Commit();
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    return PVC_OK;
}

DWORD MapPixelFormatToColors(const GUID& guid)
{
    if (guid == GUID_WICPixelFormat1bppIndexed)
        return 2;
    if (guid == GUID_WICPixelFormat4bppIndexed)
        return 16;
    if (guid == GUID_WICPixelFormat8bppIndexed)
        return 256;
    return 0;
}

} // namespace

ScopedCoInit::ScopedCoInit()
    : m_hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
    , m_needUninit(false)
{
    if (m_hr == S_OK || m_hr == S_FALSE)
    {
        m_needUninit = true;
    }
}

ScopedCoInit::~ScopedCoInit()
{
    if (m_needUninit)
    {
        CoUninitialize();
    }
}

Backend::Backend()
{
    if (!m_comScope.Succeeded())
    {
        return;
    }
    auto hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_factory));
    if (FAILED(hr))
    {
        m_factory.Reset();
    }
}

Backend& Backend::Instance()
{
    static Backend instance;
    return instance;
}

bool Backend::Populate(CPVW32DLL& table)
{
    if (!m_factory)
    {
        return false;
    }

    table.PVOpenImageEx = &Backend::sPVOpenImageEx;
    table.PVCloseImage = &Backend::sPVCloseImage;
    table.PVReadImage2 = &Backend::sPVReadImage2;
    table.PVDrawImage = &Backend::sPVDrawImage;
    table.PVGetErrorText = &Backend::sPVGetErrorText;
    table.PVSetBkHandle = &Backend::sPVSetBkHandle;
    table.PVGetDLLVersion = &Backend::sPVGetDLLVersion;
    table.PVSetStretchParameters = &Backend::sPVSetStretchParameters;
    table.PVLoadFromClipboard = &Backend::sPVLoadFromClipboard;
    table.PVGetImageInfo = &Backend::sPVGetImageInfo;
    table.PVSetParam = &Backend::sPVSetParam;
    table.PVGetHandles2 = &Backend::sPVGetHandles2;
    table.PVSaveImage = &Backend::sPVSaveImage;
    table.PVChangeImage = &Backend::sPVChangeImage;
    table.PVIsOutCombSupported = &Backend::sPVIsOutCombSupported;
    table.PVReadImageSequence = &Backend::sPVReadImageSequence;
    table.PVCropImage = &Backend::sPVCropImage;
    table.GetRGBAtCursor = &Backend::sGetRGBAtCursor;
    table.CalculateHistogram = &Backend::sCalculateHistogram;
    table.CreateThumbnail = &Backend::sCreateThumbnail;
    table.SimplifyImageSequence = &Backend::sSimplifyImageSequence;
    table.Handle = nullptr;
    StringCchCopyA(table.Version, SizeOf(table.Version), "WIC 1.0");
    return true;
}

ImageHandle* Backend::FromHandle(LPPVHandle handle)
{
    return reinterpret_cast<ImageHandle*>(handle);
}

PVCODE WINAPI Backend::sPVOpenImageEx(LPPVHandle* Img, LPPVOpenImageExInfo pOpenExInfo, LPPVImageInfo pImgInfo, int size)
{
    if (!Img || !pOpenExInfo)
    {
        return PVC_INVALID_HANDLE;
    }
    if (!(pOpenExInfo->Flags & PVOF_ATTACH_TO_HANDLE) && !pOpenExInfo->FileName)
    {
        return PVC_UNSUP_FILE_TYPE;
    }

    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }

    auto& backend = Backend::Instance();
    auto image = std::make_unique<ImageHandle>();
    image->backend = &backend;
    image->openFlags = pOpenExInfo->Flags;

    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    if (pOpenExInfo->Flags & PVOF_ATTACH_TO_HANDLE)
    {
        return PVC_UNSUP_FILE_TYPE;
    }
    else
    {
        image->fileName = Utf8ToWide(pOpenExInfo->FileName);
        if (image->fileName.empty())
        {
            return PVC_CANNOT_OPEN_FILE;
        }
        HRESULT hr = CreateDecoder(backend, image->fileName, &decoder);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }
        hr = CollectFrames(backend, decoder.Get(), *image);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }
    }

    if (image->frames.empty())
    {
        return PVC_UNSUP_FILE_TYPE;
    }

    HRESULT hr = DecodeFrame(*image, 0);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    if (pImgInfo)
    {
        PopulateImageInfo(*image, image->frames[0], pImgInfo);
    }

    *Img = reinterpret_cast<LPPVHandle>(image.release());
    return PVC_OK;
}

PVCODE WINAPI Backend::sPVCloseImage(LPPVHandle Img)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    for (auto& frame : handle->frames)
    {
        if (frame.hbitmap)
        {
            DeleteObject(frame.hbitmap);
            frame.hbitmap = nullptr;
        }
    }
    delete handle;
    return PVC_OK;
}

PVCODE WINAPI Backend::sPVReadImage2(LPPVHandle Img, HDC paintDC, RECT* dRect, TProgressProc /*progress*/, void* /*appSpecific*/,
                                     int imageIndex)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    if (imageIndex < 0 || static_cast<size_t>(imageIndex) >= handle->frames.size())
    {
        return PVC_INVALID_HANDLE;
    }
    HRESULT hr = DecodeFrame(*handle, static_cast<size_t>(imageIndex));
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    return DrawFrame(handle->frames[static_cast<size_t>(imageIndex)], paintDC, dRect ? dRect->left : 0,
                     dRect ? dRect->top : 0, dRect);
}

PVCODE WINAPI Backend::sPVDrawImage(LPPVHandle Img, HDC paintDC, int x, int y, LPRECT rect)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    HRESULT hr = DecodeFrame(*handle, 0);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    return DrawFrame(handle->frames[0], paintDC, x, y, rect);
}

const char* WINAPI Backend::sPVGetErrorText(DWORD errorCode)
{
    return LookupError(errorCode);
}

PVCODE WINAPI Backend::sPVSetBkHandle(LPPVHandle Img, COLORREF bkColor)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    handle->background = bkColor;
    return PVC_OK;
}

DWORD WINAPI Backend::sPVGetDLLVersion()
{
    return kBackendVersion;
}

PVCODE WINAPI Backend::sPVSetStretchParameters(LPPVHandle Img, DWORD width, DWORD height, DWORD mode)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    handle->stretchWidth = width;
    handle->stretchHeight = height;
    handle->stretchMode = mode;
    return PVC_OK;
}

PVCODE WINAPI Backend::sPVLoadFromClipboard(LPPVHandle* /*Img*/, LPPVImageInfo /*pImgInfo*/, int /*size*/)
{
    return PVC_UNSUP_FILE_TYPE;
}

PVCODE WINAPI Backend::sPVGetImageInfo(LPPVHandle Img, LPPVImageInfo pImgInfo, int /*size*/, int imageIndex)
{
    auto handle = FromHandle(Img);
    if (!handle || !pImgInfo)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    if (imageIndex < 0 || static_cast<size_t>(imageIndex) >= handle->frames.size())
    {
        return PVC_INVALID_HANDLE;
    }
    HRESULT hr = DecodeFrame(*handle, static_cast<size_t>(imageIndex));
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    return PopulateImageInfo(*handle, handle->frames[static_cast<size_t>(imageIndex)], pImgInfo);
}

PVCODE WINAPI Backend::sPVSetParam(LPPVHandle /*Img*/)
{
    return PVC_OK;
}

PVCODE WINAPI Backend::sPVGetHandles2(LPPVHandle Img, LPPVImageHandles* pHandles)
{
    auto handle = FromHandle(Img);
    if (!handle || !pHandles)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    HRESULT hr = DecodeFrame(*handle, 0);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    auto& frame = handle->frames[0];
    PVImageHandles& handles = handle->handles;
    ZeroMemory(&handles, sizeof(PVImageHandles));
    handles.TransparentHandle = frame.hbitmap;
    handles.TransparentBackgroundHandle = frame.hbitmap;
    handles.StretchedHandle = frame.hbitmap;
    handles.StretchedTransparentHandle = frame.hbitmap;
    handles.Palette = frame.palette.empty() ? nullptr : frame.palette.data();
    handles.pLines = frame.linePointers.empty() ? nullptr : frame.linePointers.data();
    *pHandles = &handles;
    return PVC_OK;
}

PVCODE WINAPI Backend::sPVSaveImage(LPPVHandle Img, const char* outFileName, LPPVSaveImageInfo pSii, TProgressProc /*progress*/,
                                    void* /*appSpecific*/, int imageIndex)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    if (!outFileName)
    {
        return PVC_UNSUP_OUT_PARAMS;
    }
    const auto fileName = Utf8ToWide(outFileName);
    const GuidMapping* mapping = nullptr;
    for (const auto& item : kEncoderMappings)
    {
        if (item.format == pSii->Format)
        {
            mapping = &item;
            break;
        }
    }
    if (!mapping)
    {
        return PVC_UNSUP_OUT_PARAMS;
    }
    return SaveFrame(*handle, imageIndex, fileName.c_str(), *mapping, pSii);
}

PVCODE WINAPI Backend::sPVChangeImage(LPPVHandle Img, DWORD flags)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    if (handle->frames.empty())
    {
        return PVC_INVALID_HANDLE;
    }
    FrameData& frame = handle->frames[0];
    HRESULT hr = DecodeFrame(*handle, 0);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    if (!(flags & (PVCF_ROTATE90CW | PVCF_ROTATE90CCW)))
    {
        return PVC_OK;
    }

    const UINT newWidth = frame.height;
    const UINT newHeight = frame.width;
    std::vector<BYTE> rotated(frame.pixels.size());
    for (UINT y = 0; y < frame.height; ++y)
    {
        for (UINT x = 0; x < frame.width; ++x)
        {
            BYTE* src = frame.pixels.data() + y * frame.stride + x * 4;
            UINT dstX = flags & PVCF_ROTATE90CW ? (frame.height - 1 - y) : y;
            UINT dstY = flags & PVCF_ROTATE90CW ? x : (frame.width - 1 - x);
            BYTE* dst = rotated.data() + dstY * newWidth * 4 + dstX * 4;
            memcpy(dst, src, 4);
        }
    }
    frame.width = newWidth;
    frame.height = newHeight;
    frame.stride = frame.width * 4;
    frame.pixels.swap(rotated);
    frame.bmi.biWidth = static_cast<LONG>(frame.width);
    frame.bmi.biHeight = -static_cast<LONG>(frame.height);

    if (frame.hbitmap)
    {
        DeleteObject(frame.hbitmap);
    }
    void* bits = nullptr;
    BITMAPINFO bmi{};
    bmi.bmiHeader = frame.bmi;
    frame.hbitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!frame.hbitmap)
    {
        return PVC_GDI_ERROR;
    }
    if (bits)
    {
        memcpy(bits, frame.pixels.data(), frame.pixels.size());
    }
    return PVC_OK;
}

DWORD WINAPI Backend::sPVIsOutCombSupported(int format, int /*compression*/, int /*colors*/, int /*colorModel*/)
{
    for (const auto& item : kEncoderMappings)
    {
        if (item.format == static_cast<DWORD>(format))
        {
            return 0;
        }
    }
    return static_cast<DWORD>(-1);
}

PVCODE WINAPI Backend::sPVReadImageSequence(LPPVHandle Img, LPPVImageSequence* seq)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    return CreateSequenceNodes(*handle, seq);
}

PVCODE WINAPI Backend::sPVCropImage(LPPVHandle Img, int left, int top, int width, int height)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    FrameData& frame = handle->frames[0];
    HRESULT hr = DecodeFrame(*handle, 0);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    if (left < 0 || top < 0 || width <= 0 || height <= 0 || left + width > static_cast<int>(frame.width) ||
        top + height > static_cast<int>(frame.height))
    {
        return PVC_INVALID_DIMENSIONS;
    }
    std::vector<BYTE> cropped(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);
    const UINT newStride = width * 4;
    for (int y = 0; y < height; ++y)
    {
        const BYTE* src = frame.pixels.data() + (top + y) * frame.stride + left * 4;
        BYTE* dst = cropped.data() + y * newStride;
        memcpy(dst, src, newStride);
    }
    frame.width = width;
    frame.height = height;
    frame.stride = newStride;
    frame.pixels.swap(cropped);
    frame.bmi.biWidth = width;
    frame.bmi.biHeight = -height;
    if (frame.hbitmap)
    {
        DeleteObject(frame.hbitmap);
    }
    void* bits = nullptr;
    BITMAPINFO bmi{};
    bmi.bmiHeader = frame.bmi;
    frame.hbitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!frame.hbitmap)
    {
        return PVC_GDI_ERROR;
    }
    if (bits)
    {
        memcpy(bits, frame.pixels.data(), frame.pixels.size());
    }
    return PVC_OK;
}

bool Backend::sGetRGBAtCursor(LPPVHandle Img, DWORD /*colors*/, int x, int y, RGBQUAD* rgb, int* /*index*/)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return false;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return false;
    }
    FrameData& frame = handle->frames[0];
    if (!frame.decoded)
    {
        if (FAILED(DecodeFrame(*handle, 0)))
        {
            return false;
        }
    }
    if (x < 0 || y < 0 || x >= static_cast<int>(frame.width) || y >= static_cast<int>(frame.height))
    {
        return false;
    }
    const BYTE* src = frame.pixels.data() + y * frame.stride + x * 4;
    if (rgb)
    {
        rgb->rgbBlue = src[0];
        rgb->rgbGreen = src[1];
        rgb->rgbRed = src[2];
        rgb->rgbReserved = src[3];
    }
    return true;
}

PVCODE Backend::sCalculateHistogram(LPPVHandle Img, const LPPVImageInfo /*info*/, LPDWORD luminosity, LPDWORD red, LPDWORD green,
                                    LPDWORD blue, LPDWORD rgb)
{
    auto handle = FromHandle(Img);
    if (!handle)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    FrameData& frame = handle->frames[0];
    if (!frame.decoded)
    {
        if (FAILED(DecodeFrame(*handle, 0)))
        {
            return PVC_EXCEPTION;
        }
    }
    std::fill(luminosity, luminosity + 256, 0);
    std::fill(red, red + 256, 0);
    std::fill(green, green + 256, 0);
    std::fill(blue, blue + 256, 0);
    std::fill(rgb, rgb + 256, 0);

    for (UINT y = 0; y < frame.height; ++y)
    {
        const BYTE* src = frame.pixels.data() + y * frame.stride;
        for (UINT x = 0; x < frame.width; ++x)
        {
            BYTE b = src[x * 4 + 0];
            BYTE g = src[x * 4 + 1];
            BYTE r = src[x * 4 + 2];
            BYTE l = static_cast<BYTE>((static_cast<int>(r) * 30 + static_cast<int>(g) * 59 + static_cast<int>(b) * 11) / 100);
            luminosity[l]++;
            red[r]++;
            green[g]++;
            blue[b]++;
            rgb[(r + g + b) / 3]++;
        }
    }
    return PVC_OK;
}

PVCODE Backend::sCreateThumbnail(LPPVHandle Img, LPPVSaveImageInfo /*sii*/, int imageIndex, DWORD imgWidth, DWORD imgHeight,
                                  int thumbWidth, int thumbHeight, CSalamanderThumbnailMakerAbstract* thumbMaker,
                                  DWORD thumbFlags, TProgressProc progressProc, void* progressProcArg)
{
    auto handle = FromHandle(Img);
    if (!handle || !thumbMaker)
    {
        return PVC_INVALID_HANDLE;
    }
    ScopedCoInit init;
    if (!init.Succeeded())
    {
        return PVC_EXCEPTION;
    }
    if (imageIndex < 0 || static_cast<size_t>(imageIndex) >= handle->frames.size())
    {
        return PVC_INVALID_HANDLE;
    }
    HRESULT hr = DecodeFrame(*handle, static_cast<size_t>(imageIndex));
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    FrameData& frame = handle->frames[static_cast<size_t>(imageIndex)];

    int targetWidth = thumbWidth > 0 ? thumbWidth : static_cast<int>(frame.width);
    int targetHeight = thumbHeight > 0 ? thumbHeight : static_cast<int>(frame.height);
    if (targetWidth <= 0)
    {
        targetWidth = static_cast<int>(frame.width);
    }
    if (targetHeight <= 0)
    {
        targetHeight = static_cast<int>(frame.height);
    }

    const auto calculateThumbnailSize = [](int originalWidth, int originalHeight, int maxWidth, int maxHeight,
                                          int& thumbWidth, int& thumbHeight) {
        if (originalWidth <= maxWidth && originalHeight <= maxHeight)
        {
            thumbWidth = originalWidth;
            thumbHeight = originalHeight;
            return false;
        }

        const double aspect = static_cast<double>(originalWidth) / static_cast<double>(originalHeight);
        const double bounds = static_cast<double>(maxWidth) / static_cast<double>(maxHeight);
        if (bounds < aspect)
        {
            thumbWidth = maxWidth;
            thumbHeight = static_cast<int>(static_cast<double>(maxWidth) / aspect);
        }
        else
        {
            thumbHeight = maxHeight;
            thumbWidth = static_cast<int>(static_cast<double>(maxHeight) * aspect);
        }

        if (thumbWidth < 1)
        {
            thumbWidth = 1;
        }
        if (thumbHeight < 1)
        {
            thumbHeight = 1;
        }
        return true;
    };

    calculateThumbnailSize(static_cast<int>(imgWidth), static_cast<int>(imgHeight), targetWidth, targetHeight, targetWidth,
                           targetHeight);

    if (!thumbMaker->SetParameters(targetWidth, targetHeight, thumbFlags))
    {
        return PVC_OUT_OF_MEMORY;
    }

    const UINT desiredWidth = static_cast<UINT>(targetWidth);
    const UINT desiredHeight = static_cast<UINT>(targetHeight);

    const BYTE* source = frame.pixels.data();
    std::vector<BYTE> scaled;
    if (desiredWidth != frame.width || desiredHeight != frame.height)
    {
        Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
        hr = handle->backend->Factory()->CreateBitmapScaler(&scaler);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }
        hr = scaler->Initialize(frame.converter.Get(), desiredWidth, desiredHeight, WICBitmapInterpolationModeFant);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        scaled.resize(static_cast<size_t>(desiredWidth) * static_cast<size_t>(desiredHeight) * 4);
        WICRect rect{0, 0, static_cast<INT>(desiredWidth), static_cast<INT>(desiredHeight)};
        hr = scaler->CopyPixels(&rect, desiredWidth * 4, static_cast<UINT>(scaled.size()), scaled.data());
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        source = scaled.data();
    }

    if (progressProc && !progressProc(100, progressProcArg))
    {
        return PVC_CANCELED;
    }

    // The thumbnail maker expects rows in top-down order; feed them in manageable batches
    // so it can honour cancellation requests.
    const size_t rowBytes = static_cast<size_t>(desiredWidth) * 4;
    int processedRows = 0;
    while (processedRows < targetHeight)
    {
        if (thumbMaker->GetCancelProcessing())
        {
            return PVC_CANCELED;
        }

        const int batch = std::min<int>(32, targetHeight - processedRows);
        BYTE* chunk = const_cast<BYTE*>(source + static_cast<size_t>(processedRows) * rowBytes);
        thumbMaker->ProcessBuffer(chunk, batch);
        processedRows += batch;
    }
    return PVC_OK;
}

PVCODE Backend::sSimplifyImageSequence(LPPVHandle /*Img*/, HDC /*dc*/, int /*screenWidth*/, int /*screenHeight*/,
                                       LPPVImageSequence& /*seq*/, const COLORREF& /*bgColor*/)
{
    return PVC_OK;
}

} // namespace PictView::Wic
