// SPDX-FileCopyrightText: 2024 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "WicBackend.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <vector>

#include <objbase.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <propvarutil.h>
#include <wincodecsdk.h>

#include "../Thumbnailer.h"

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "propsys.lib")

using namespace std::string_literals;

namespace PictView::Wic
{
namespace
{
constexpr DWORD kBackendVersion = PV_VERSION_156;
constexpr UINT kBytesPerPixel = 4;
constexpr UINT kMaxGdiDimension = static_cast<UINT>(std::numeric_limits<int>::max());

HRESULT AllocatePixelStorage(FrameData& frame, UINT width, UINT height);
HRESULT FinalizeDecodedFrame(FrameData& frame);
PVCODE PopulateImageInfo(ImageHandle& handle, LPPVImageInfo info, DWORD bufferSize, bool hasPreviousImage,
                         DWORD previousImageIndex, int currentImage);
bool TryReadUnsignedMetadata(IWICMetadataQueryReader* reader, LPCWSTR name, UINT& value);
DWORD MapGifDisposalToPv(UINT disposal);
LONG ClampUnsignedToLong(ULONGLONG value);

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
    {PVC_READING_ERROR, "The image file appears to be corrupt or unreadable."},
    {PVC_WRITING_ERROR, "The image could not be written."},
    {PVC_UNEXPECTED_EOF, "The image data ended unexpectedly."},
};

bool PathLooksLikeExif(LPCWSTR path)
{
    if (!path)
    {
        return false;
    }
    if (wcsstr(path, L"exif") != nullptr)
    {
        return true;
    }
    if (wcsstr(path, L"{ushort=34665}") != nullptr)
    {
        return true;
    }
    return false;
}

bool QueryReaderContainsExif(IWICMetadataQueryReader* query)
{
    if (!query)
    {
        return false;
    }
    static const wchar_t* kProbePaths[] = {
        L"/ifd/exif:ExifVersion",
        L"/ifd/{ushort=34665}",
        L"/app1/ifd/exif:ExifVersion",
        L"/app1/{ushort=34665}"
    };
    PROPVARIANT value;
    for (const auto* path : kProbePaths)
    {
        PropVariantInit(&value);
        const HRESULT hr = query->GetMetadataByName(path, &value);
        PropVariantClear(&value);
        if (SUCCEEDED(hr))
        {
            return true;
        }
    }

    PropVariantInit(&value);
    const HRESULT ifdHr = query->GetMetadataByName(L"/ifd", &value);
    if (SUCCEEDED(ifdHr))
    {
        bool hasExif = true;
        if (value.vt == VT_UNKNOWN && value.punkVal)
        {
            ComPtr<IWICMetadataQueryReader> nested;
            if (SUCCEEDED(value.punkVal->QueryInterface(IID_PPV_ARGS(&nested))) && nested)
            {
                hasExif = QueryReaderContainsExif(nested.Get());
            }
        }
        PropVariantClear(&value);
        if (hasExif)
        {
            return true;
        }
    }
    else
    {
        PropVariantClear(&value);
    }

    ComPtr<IEnumString> names;
    if (SUCCEEDED(query->GetEnumerator(&names)) && names)
    {
        LPOLESTR rawName = nullptr;
        ULONG fetched = 0;
        while (names->Next(1, &rawName, &fetched) == S_OK)
        {
            if (rawName)
            {
                const bool looksLikeExif = PathLooksLikeExif(rawName);
                if (looksLikeExif)
                {
                    PROPVARIANT enumValue;
                    PropVariantInit(&enumValue);
                    const HRESULT hr = query->GetMetadataByName(rawName, &enumValue);
                    bool hasExif = false;
                    if (SUCCEEDED(hr))
                    {
                        if (enumValue.vt == VT_UNKNOWN && enumValue.punkVal)
                        {
                            ComPtr<IWICMetadataQueryReader> nested;
                            if (SUCCEEDED(enumValue.punkVal->QueryInterface(IID_PPV_ARGS(&nested))) && nested)
                            {
                                hasExif = QueryReaderContainsExif(nested.Get());
                            }
                        }
                        else
                        {
                            hasExif = true;
                        }
                    }
                    PropVariantClear(&enumValue);
                    CoTaskMemFree(rawName);
                    rawName = nullptr;
                    if (hasExif)
                    {
                        return true;
                    }
                }
                if (rawName)
                {
                    CoTaskMemFree(rawName);
                    rawName = nullptr;
                }
            }
        }
    }

    return false;
}

bool ReaderContainsExif(IWICMetadataReader* reader)
{
    if (!reader)
    {
        return false;
    }
    GUID format = {};
    if (SUCCEEDED(reader->GetMetadataFormat(&format)))
    {
        if (format == GUID_MetadataFormatExif || format == GUID_MetadataFormatIfd)
        {
            return true;
        }
    }

    ComPtr<IWICMetadataQueryReader> query;
    if (SUCCEEDED(reader->QueryInterface(IID_PPV_ARGS(&query))) && query)
    {
        if (QueryReaderContainsExif(query.Get()))
        {
            return true;
        }
    }

    ComPtr<IWICMetadataBlockReader> blockReader;
    if (SUCCEEDED(reader->QueryInterface(IID_PPV_ARGS(&blockReader))) && blockReader)
    {
        UINT count = 0;
        if (SUCCEEDED(blockReader->GetCount(&count)))
        {
            for (UINT i = 0; i < count; ++i)
            {
                ComPtr<IWICMetadataReader> child;
                if (SUCCEEDED(blockReader->GetReaderByIndex(i, &child)) && child)
                {
                    if (ReaderContainsExif(child.Get()))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool SourceContainsExif(IUnknown* source)
{
    if (!source)
    {
        return false;
    }
    ComPtr<IWICMetadataBlockReader> blockReader;
    if (FAILED(source->QueryInterface(IID_PPV_ARGS(&blockReader))) || !blockReader)
    {
        return false;
    }
    UINT count = 0;
    if (FAILED(blockReader->GetCount(&count)))
    {
        return false;
    }
    for (UINT i = 0; i < count; ++i)
    {
        ComPtr<IWICMetadataReader> reader;
        if (SUCCEEDED(blockReader->GetReaderByIndex(i, &reader)) && reader)
        {
            if (ReaderContainsExif(reader.Get()))
            {
                return true;
            }
        }
    }
    return false;
}

bool FrameContainsExif(IWICBitmapFrameDecode* frame)
{
    if (!frame)
    {
        return false;
    }
    if (SourceContainsExif(frame))
    {
        return true;
    }
    ComPtr<IWICMetadataQueryReader> query;
    if (SUCCEEDED(frame->GetMetadataQueryReader(&query)) && query)
    {
        if (QueryReaderContainsExif(query.Get()))
        {
            return true;
        }
    }
    return false;
}

bool TryExtractDelayHundredths(const PROPVARIANT& value, UINT& hundredths)
{
    switch (value.vt)
    {
    case VT_UI1:
        hundredths = value.bVal;
        return true;
    case VT_UI2:
        hundredths = value.uiVal;
        return true;
    case VT_UI4:
        hundredths = static_cast<UINT>(value.ulVal);
        return true;
    case VT_UI8:
        hundredths = static_cast<UINT>(std::min<ULONGLONG>(value.uhVal.QuadPart,
                                                           static_cast<ULONGLONG>(std::numeric_limits<UINT>::max())));
        return true;
    case VT_UINT:
        hundredths = value.uintVal;
        return true;
    case VT_R4:
        hundredths = static_cast<UINT>(value.fltVal);
        return true;
    case VT_R8:
        hundredths = static_cast<UINT>(value.dblVal);
        return true;
    case (VT_VECTOR | VT_UI1):
        if (value.caub.cElems > 0 && value.caub.pElems)
        {
            hundredths = value.caub.pElems[0];
            return true;
        }
        break;
    case (VT_VECTOR | VT_UI2):
        if (value.caui.cElems > 0 && value.caui.pElems)
        {
            hundredths = value.caui.pElems[0];
            return true;
        }
        break;
    case (VT_VECTOR | VT_UI4):
        if (value.caul.cElems > 0 && value.caul.pElems)
        {
            hundredths = static_cast<UINT>(value.caul.pElems[0]);
            return true;
        }
        break;
    case (VT_VECTOR | VT_UI8):
        if (value.cauh.cElems > 0 && value.cauh.pElems)
        {
            hundredths = static_cast<UINT>(std::min<ULONGLONG>(value.cauh.pElems[0].QuadPart,
                                                               static_cast<ULONGLONG>(std::numeric_limits<UINT>::max())));
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

bool TryReadDelayHundredths(IWICMetadataQueryReader* reader, LPCWSTR name, UINT& hundredths)
{
    if (!reader || !name)
    {
        return false;
    }
    PROPVARIANT value;
    PropVariantInit(&value);
    const HRESULT hr = reader->GetMetadataByName(name, &value);
    if (FAILED(hr))
    {
        PropVariantClear(&value);
        return false;
    }
    const bool extracted = TryExtractDelayHundredths(value, hundredths);
    PropVariantClear(&value);
    return extracted;
}

bool TryReadUnsignedMetadata(IWICMetadataQueryReader* reader, LPCWSTR name, UINT& value)
{
    if (!reader || !name)
    {
        return false;
    }

    PROPVARIANT rawValue;
    PropVariantInit(&rawValue);
    const HRESULT hr = reader->GetMetadataByName(name, &rawValue);
    if (FAILED(hr))
    {
        PropVariantClear(&rawValue);
        return false;
    }

    UINT extracted = 0;
    const HRESULT convertHr = PropVariantToUInt32(rawValue, &extracted);
    PropVariantClear(&rawValue);
    if (FAILED(convertHr))
    {
        return false;
    }

    value = extracted;
    return true;
}

DWORD ClampDelayHundredthsToMilliseconds(UINT hundredths)
{
    if (hundredths == 0)
    {
        hundredths = 10; // default to 100 ms when delay is unspecified
    }
    const ULONGLONG delayMs64 = static_cast<ULONGLONG>(hundredths) * 10ull;
    return delayMs64 > std::numeric_limits<DWORD>::max() ? std::numeric_limits<DWORD>::max()
                                                         : static_cast<DWORD>(delayMs64);
}

DWORD GetFrameDelayMilliseconds(IWICBitmapFrameDecode* frame)
{
    if (!frame)
    {
        return 0;
    }
    ComPtr<IWICMetadataQueryReader> query;
    if (FAILED(frame->GetMetadataQueryReader(&query)) || !query)
    {
        return 0;
    }

    UINT hundredths = 0;
    static constexpr const wchar_t* kDelayPaths[] = {
        L"/grctlext/DelayTime",            // GIF frame delay
        L"/ifd/{ushort=0x5100}",           // TIFF/PropertyTagFrameDelay
        L"/xmp/GIF:DelayTime",             // XMP GIF namespace (fallback)
        L"/xmp/MM:FrameDelay",             // Additional XMP metadata some encoders emit
        L"/xmp/extensibility/Animation/FrameDelay"
    };
    for (const auto* path : kDelayPaths)
    {
        if (TryReadDelayHundredths(query.Get(), path, hundredths))
        {
            return ClampDelayHundredthsToMilliseconds(hundredths);
        }
    }
    return 0;
}

DWORD MapGifDisposalToPv(UINT disposal)
{
    switch (disposal & 0x7u)
    {
    case 1:
        return PVDM_UNMODIFIED;
    case 2:
        return PVDM_BACKGROUND;
    case 3:
        return PVDM_PREVIOUS;
    default:
        return PVDM_UNDEFINED;
    }
}

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

ULONGLONG AbsoluteDimension(LONGLONG value)
{
    if (value >= 0)
    {
        return static_cast<ULONGLONG>(value);
    }
    if (value == std::numeric_limits<LONGLONG>::min())
    {
        return static_cast<ULONGLONG>(std::numeric_limits<LONGLONG>::max()) + 1ull;
    }
    return static_cast<ULONGLONG>(-(value + 1)) + 1;
}

LONG ClampUnsignedToLong(ULONGLONG value)
{
    return value > static_cast<ULONGLONG>(std::numeric_limits<LONG>::max())
               ? std::numeric_limits<LONG>::max()
               : static_cast<LONG>(value);
}

DWORD ClampToDword(ULONGLONG value)
{
    return value > static_cast<ULONGLONG>(std::numeric_limits<DWORD>::max())
               ? std::numeric_limits<DWORD>::max()
               : static_cast<DWORD>(value);
}

DWORD QueryFileSize(const std::wstring& path)
{
    if (path.empty())
    {
        return 0;
    }

    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attributes))
    {
        return 0;
    }

    ULARGE_INTEGER size;
    size.LowPart = attributes.nFileSizeLow;
    size.HighPart = attributes.nFileSizeHigh;
    return ClampToDword(size.QuadPart);
}

size_t NormalizeFrameIndex(const ImageHandle& handle, int requestedIndex, size_t fallbackIndex = 0)
{
    if (handle.frames.empty())
    {
        return 0;
    }

    size_t index = fallbackIndex;
    if (index >= handle.frames.size())
    {
        index = handle.frames.size() - 1;
    }

    if (requestedIndex >= 0)
    {
        index = static_cast<size_t>(requestedIndex);
        if (index >= handle.frames.size())
        {
            index = handle.frames.size() - 1;
        }
    }

    return index;
}

HRESULT PopulateFrameFromBitmapHandle(FrameData& frame, HBITMAP bitmap)
{
    if (!bitmap)
    {
        return E_INVALIDARG;
    }

    DIBSECTION dib{};
    const int objectSize = GetObjectW(bitmap, sizeof(dib), &dib);
    if (objectSize == 0)
    {
        const DWORD error = GetLastError();
        return HRESULT_FROM_WIN32(error != 0 ? error : ERROR_INVALID_DATA);
    }

    const LONG widthLong = dib.dsBm.bmWidth;
    const LONG heightLong = dib.dsBm.bmHeight;
    if (widthLong <= 0 || heightLong == 0)
    {
        return WINCODEC_ERR_INVALIDPARAMETER;
    }

    const UINT width = static_cast<UINT>(widthLong);
    const UINT height = static_cast<UINT>(heightLong < 0 ? -heightLong : heightLong);

    HRESULT hr = AllocatePixelStorage(frame, width, height);
    if (FAILED(hr))
    {
        return hr;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = static_cast<LONG>(width);
    bmi.bmiHeader.biHeight = -static_cast<LONG>(height);
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc)
    {
        const DWORD error = GetLastError();
        frame.pixels.clear();
        frame.stride = 0;
        return HRESULT_FROM_WIN32(error != 0 ? error : ERROR_NOT_ENOUGH_MEMORY);
    }

    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    const int lines = GetDIBits(dc, bitmap, 0, height, frame.pixels.data(), &bmi, DIB_RGB_COLORS);
    if (oldBitmap)
    {
        SelectObject(dc, oldBitmap);
    }
    DeleteDC(dc);

    if (lines == 0)
    {
        const DWORD error = GetLastError();
        frame.pixels.clear();
        frame.stride = 0;
        return HRESULT_FROM_WIN32(error != 0 ? error : ERROR_INVALID_DATA);
    }

    if (dib.dsBm.bmBitsPixel < 32)
    {
        BYTE* pixels = frame.pixels.data();
        const size_t pixelCount = static_cast<size_t>(width) * height;
        for (size_t i = 0; i < pixelCount; ++i)
        {
            pixels[i * 4 + 3] = 255;
        }
    }

    frame.rect.left = 0;
    frame.rect.top = 0;
    frame.rect.right = ClampUnsignedToLong(static_cast<ULONGLONG>(width));
    frame.rect.bottom = ClampUnsignedToLong(static_cast<ULONGLONG>(height));
    frame.disposal = PVDM_UNDEFINED;

    hr = FinalizeDecodedFrame(frame);
    if (FAILED(hr))
    {
        return hr;
    }
    return S_OK;
}

HRESULT AllocateBuffer(std::vector<BYTE>& buffer, size_t size)
{
    try
    {
        buffer.resize(size);
    }
    catch (const std::bad_alloc&)
    {
        buffer.clear();
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

HRESULT AllocatePixelStorage(FrameData& frame, UINT width, UINT height)
{
    if (width == 0 || height == 0)
    {
        return WINCODEC_ERR_INVALIDPARAMETER;
    }
    if (width > kMaxGdiDimension || height > kMaxGdiDimension)
    {
        return WINCODEC_ERR_INVALIDPARAMETER;
    }

    const ULONGLONG stride64 = static_cast<ULONGLONG>(width) * kBytesPerPixel;
    if (stride64 > std::numeric_limits<UINT>::max())
    {
        return E_OUTOFMEMORY;
    }

    const ULONGLONG buffer64 = stride64 * static_cast<ULONGLONG>(height);
    if (height != 0 && buffer64 / height != stride64)
    {
        return E_OUTOFMEMORY;
    }
    if (buffer64 > static_cast<ULONGLONG>(std::numeric_limits<UINT>::max()))
    {
        return E_OUTOFMEMORY;
    }
    if (buffer64 > static_cast<ULONGLONG>(std::numeric_limits<size_t>::max()))
    {
        return E_OUTOFMEMORY;
    }

    frame.width = width;
    frame.height = height;
    frame.stride = static_cast<UINT>(stride64);

    HRESULT hr = AllocateBuffer(frame.pixels, static_cast<size_t>(buffer64));
    if (FAILED(hr))
    {
        frame.stride = 0;
        return hr;
    }
    return S_OK;
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

bool IsIgnorableColorProfileError(HRESULT hr)
{
    switch (hr)
    {
    case WINCODEC_ERR_UNSUPPORTEDOPERATION:
    case WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT:
    case WINCODEC_ERR_PROPERTYNOTSUPPORTED:
    case WINCODEC_ERR_UNSUPPORTEDVERSION:
#ifdef WINCODEC_ERR_PROFILENOTASSOCIATED
    case WINCODEC_ERR_PROFILENOTASSOCIATED:
#endif
#ifdef WINCODEC_ERR_PROFILEINVALID
    case WINCODEC_ERR_PROFILEINVALID:
#endif
    case E_NOTIMPL:
        return true;
    default:
        return false;
    }
}

HRESULT ApplyEmbeddedColorProfile(ImageHandle& handle, FrameData& frame)
{
    if (frame.colorConvertedSource)
    {
        return S_OK;
    }

    IWICImagingFactory* factory = handle.backend->Factory();
    if (!factory)
    {
        return E_POINTER;
    }

    UINT contextCount = 0;
    HRESULT hr = frame.frame->GetColorContexts(0, nullptr, &contextCount);
    if (FAILED(hr))
    {
        return hr;
    }
    if (contextCount == 0)
    {
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    std::vector<Microsoft::WRL::ComPtr<IWICColorContext>> sourceContexts(contextCount);
    std::vector<IWICColorContext*> rawContexts(contextCount);
    for (UINT i = 0; i < contextCount; ++i)
    {
        hr = factory->CreateColorContext(&sourceContexts[i]);
        if (FAILED(hr))
        {
            return hr;
        }
        rawContexts[i] = sourceContexts[i].Get();
    }

    hr = frame.frame->GetColorContexts(contextCount, rawContexts.data(), &contextCount);
    if (FAILED(hr))
    {
        return hr;
    }
    if (contextCount == 0)
    {
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }

    Microsoft::WRL::ComPtr<IWICColorContext> destinationContext;
    hr = factory->CreateColorContext(&destinationContext);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = destinationContext->InitializeFromExifColorSpace(0x1); // sRGB
    if (FAILED(hr))
    {
        return hr;
    }

    Microsoft::WRL::ComPtr<IWICColorTransform> transform;
    static const GUID kCLSID_WICColorTransform =
        {0xB66F034F, 0xD0E2, 0x40AB, {0xB4, 0x36, 0x6D, 0xE3, 0x9E, 0x32, 0x1A, 0x94}};
    hr = CoCreateInstance(kCLSID_WICColorTransform, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&transform));
    if (FAILED(hr))
    {
        if (hr == REGDB_E_CLASSNOTREG)
        {
            return WINCODEC_ERR_UNSUPPORTEDOPERATION;
        }
        return hr;
    }

    if (rawContexts.empty())
    {
        return WINCODEC_ERR_UNSUPPORTEDOPERATION;
    }
    IWICColorContext* sourceContext = rawContexts[0];
    hr = transform->Initialize(frame.frame.Get(), sourceContext, destinationContext.Get(), GUID_WICPixelFormat32bppBGRA);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = transform.As(&frame.colorConvertedSource);
    return hr;
}

HRESULT CopyBgraFromSource(FrameData& frame, IWICBitmapSource* source)
{
    if (!source)
    {
        return E_POINTER;
    }

    UINT width = 0;
    UINT height = 0;
    HRESULT hr = source->GetSize(&width, &height);
    if (FAILED(hr))
    {
        return hr;
    }

    hr = AllocatePixelStorage(frame, width, height);
    if (FAILED(hr))
    {
        return hr;
    }

    WICRect rect{0, 0, static_cast<INT>(width), static_cast<INT>(height)};
    const UINT bufferSize = static_cast<UINT>(frame.pixels.size());
    hr = source->CopyPixels(&rect, frame.stride, bufferSize, frame.pixels.data());
    if (FAILED(hr))
    {
        frame.pixels.clear();
        frame.stride = 0;
        return hr;
    }

    return S_OK;
}

HRESULT FinalizeDecodedFrame(FrameData& frame)
{
    const size_t lineCount = static_cast<size_t>(frame.height);
    if (lineCount > frame.linePointers.max_size())
    {
        return E_OUTOFMEMORY;
    }
    try
    {
        frame.linePointers.resize(lineCount);
    }
    catch (const std::bad_alloc&)
    {
        frame.linePointers.clear();
        return E_OUTOFMEMORY;
    }

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
    const size_t pixelBytes = frame.pixels.size();
    frame.bmi.biSizeImage = pixelBytes > std::numeric_limits<DWORD>::max() ? 0
                                                                         : static_cast<DWORD>(pixelBytes);
    frame.bmi.biXPelsPerMeter = 0;
    frame.bmi.biYPelsPerMeter = 0;

    if (frame.hbitmap)
    {
        DeleteObject(frame.hbitmap);
        frame.hbitmap = nullptr;
    }

    void* bits = nullptr;
    BITMAPINFO bmi{};
    bmi.bmiHeader = frame.bmi;
    frame.hbitmap = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!frame.hbitmap)
    {
        return E_OUTOFMEMORY;
    }
    if (bits && !frame.pixels.empty())
    {
        memcpy(bits, frame.pixels.data(), frame.pixels.size());
    }

    frame.decoded = true;
    return S_OK;
}

inline BYTE CombineCmykChannel(BYTE component, BYTE black)
{
    const int c = 255 - component;
    const int k = 255 - black;
    const int value = c * k + 127;
    return static_cast<BYTE>(value / 255);
}

inline BYTE ToByteFromWord(UINT16 value)
{
    return static_cast<BYTE>((static_cast<unsigned int>(value) + 128u) / 257u);
}

HRESULT DecodeUnsupportedPixelFormat(FrameData& frame)
{
    GUID pixelFormat{};
    HRESULT hr = frame.frame->GetPixelFormat(&pixelFormat);
    if (FAILED(hr))
    {
        return hr;
    }

    if (pixelFormat == GUID_WICPixelFormat32bppCMYK)
    {
        UINT width = 0;
        UINT height = 0;
        hr = frame.frame->GetSize(&width, &height);
        if (FAILED(hr))
        {
            return hr;
        }

        const ULONGLONG sourceStride64 = static_cast<ULONGLONG>(width) * 4ull;
        if (sourceStride64 > std::numeric_limits<UINT>::max())
        {
            return E_OUTOFMEMORY;
        }
        const UINT sourceStride = static_cast<UINT>(sourceStride64);
        const ULONGLONG sourceSize64 = sourceStride64 * static_cast<ULONGLONG>(height);
        if (height != 0 && sourceSize64 / height != sourceStride64)
        {
            return E_OUTOFMEMORY;
        }
        if (sourceSize64 > static_cast<ULONGLONG>(std::numeric_limits<size_t>::max()) ||
            sourceSize64 > static_cast<ULONGLONG>(std::numeric_limits<UINT>::max()))
        {
            return E_OUTOFMEMORY;
        }

        std::vector<BYTE> cmyk;
        hr = AllocateBuffer(cmyk, static_cast<size_t>(sourceSize64));
        if (FAILED(hr))
        {
            return hr;
        }

        WICRect rect{0, 0, static_cast<INT>(width), static_cast<INT>(height)};
        hr = frame.frame->CopyPixels(&rect, sourceStride, static_cast<UINT>(cmyk.size()), cmyk.data());
        if (FAILED(hr))
        {
            return hr;
        }

        hr = AllocatePixelStorage(frame, width, height);
        if (FAILED(hr))
        {
            return hr;
        }

        for (UINT y = 0; y < frame.height; ++y)
        {
            const BYTE* src = cmyk.data() + static_cast<size_t>(y) * sourceStride;
            BYTE* dst = frame.pixels.data() + static_cast<size_t>(y) * frame.stride;
            for (UINT x = 0; x < frame.width; ++x)
            {
                const BYTE c = src[x * 4 + 0];
                const BYTE m = src[x * 4 + 1];
                const BYTE yComp = src[x * 4 + 2];
                const BYTE k = src[x * 4 + 3];
                dst[x * 4 + 0] = CombineCmykChannel(yComp, k);
                dst[x * 4 + 1] = CombineCmykChannel(m, k);
                dst[x * 4 + 2] = CombineCmykChannel(c, k);
                dst[x * 4 + 3] = 255;
            }
        }
        return S_OK;
    }

    if (pixelFormat == GUID_WICPixelFormat64bppCMYK)
    {
        UINT width = 0;
        UINT height = 0;
        hr = frame.frame->GetSize(&width, &height);
        if (FAILED(hr))
        {
            return hr;
        }

        const ULONGLONG sourceStride64 = static_cast<ULONGLONG>(width) * 8ull;
        if (sourceStride64 > std::numeric_limits<UINT>::max())
        {
            return E_OUTOFMEMORY;
        }
        const UINT sourceStride = static_cast<UINT>(sourceStride64);
        const ULONGLONG sourceSize64 = sourceStride64 * static_cast<ULONGLONG>(height);
        if (height != 0 && sourceSize64 / height != sourceStride64)
        {
            return E_OUTOFMEMORY;
        }
        if (sourceSize64 > static_cast<ULONGLONG>(std::numeric_limits<size_t>::max()) ||
            sourceSize64 > static_cast<ULONGLONG>(std::numeric_limits<UINT>::max()))
        {
            return E_OUTOFMEMORY;
        }

        std::vector<BYTE> cmyk;
        hr = AllocateBuffer(cmyk, static_cast<size_t>(sourceSize64));
        if (FAILED(hr))
        {
            return hr;
        }

        WICRect rect{0, 0, static_cast<INT>(width), static_cast<INT>(height)};
        hr = frame.frame->CopyPixels(&rect, sourceStride, static_cast<UINT>(cmyk.size()), cmyk.data());
        if (FAILED(hr))
        {
            return hr;
        }

        hr = AllocatePixelStorage(frame, width, height);
        if (FAILED(hr))
        {
            return hr;
        }

        for (UINT y = 0; y < frame.height; ++y)
        {
            const UINT16* src = reinterpret_cast<const UINT16*>(cmyk.data() + static_cast<size_t>(y) * sourceStride);
            BYTE* dst = frame.pixels.data() + static_cast<size_t>(y) * frame.stride;
            for (UINT x = 0; x < frame.width; ++x)
            {
                const BYTE c = ToByteFromWord(src[x * 4 + 0]);
                const BYTE m = ToByteFromWord(src[x * 4 + 1]);
                const BYTE yComp = ToByteFromWord(src[x * 4 + 2]);
                const BYTE k = ToByteFromWord(src[x * 4 + 3]);
                dst[x * 4 + 0] = CombineCmykChannel(yComp, k);
                dst[x * 4 + 1] = CombineCmykChannel(m, k);
                dst[x * 4 + 2] = CombineCmykChannel(c, k);
                dst[x * 4 + 3] = 255;
            }
        }
        return S_OK;
    }

    return WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT;
}

bool IsConverterFormatFailure(HRESULT hr)
{
    if (hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT)
    {
        return true;
    }
#if defined(WINCODEC_ERR_INVALIDPARAMETER)
    if (hr == WINCODEC_ERR_INVALIDPARAMETER)
    {
        return true;
    }
#endif
#if defined(WINCODEC_ERR_UNSUPPORTEDOPERATION)
    if (hr == WINCODEC_ERR_UNSUPPORTEDOPERATION)
    {
        return true;
    }
#endif
    return false;
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
    IWICImagingFactory* factory = handle.backend->Factory();
    if (!factory)
    {
        return E_FAIL;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    HRESULT hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = converter->Initialize(frame.frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr) && IsConverterFormatFailure(hr))
    {
        HRESULT profileHr = ApplyEmbeddedColorProfile(handle, frame);
        if (SUCCEEDED(profileHr) && frame.colorConvertedSource)
        {
            hr = converter->Initialize(frame.colorConvertedSource.Get(), GUID_WICPixelFormat32bppBGRA,
                                       WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
        }
        else if (FAILED(profileHr) && !IsIgnorableColorProfileError(profileHr))
        {
            return profileHr;
        }
    }
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
        if (IsConverterFormatFailure(hr))
        {
            hr = DecodeUnsupportedPixelFormat(frame);
            if (FAILED(hr))
            {
                return hr;
            }
            hr = FinalizeDecodedFrame(frame);
            return hr;
        }
        return hr;
    }

    hr = CopyBgraFromSource(frame, frame.converter.Get());
    if (FAILED(hr))
    {
        return hr;
    }

    return FinalizeDecodedFrame(frame);
}

PVCODE HResultToPvCode(HRESULT hr)
{
    if (SUCCEEDED(hr))
    {
        return PVC_OK;
    }
    if (hr == E_OUTOFMEMORY
#if defined(WINCODEC_ERR_OUTOFMEMORY)
        || hr == WINCODEC_ERR_OUTOFMEMORY
#endif
#if defined(WINCODEC_ERR_INSUFFICIENTBUFFER)
        || hr == WINCODEC_ERR_INSUFFICIENTBUFFER
#endif
    )
    {
        return PVC_OUT_OF_MEMORY;
    }
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) ||
        hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) || hr == STG_E_FILENOTFOUND ||
        hr == STG_E_ACCESSDENIED)
    {
        return PVC_CANNOT_OPEN_FILE;
    }
    if (hr == E_INVALIDARG
#if defined(WINCODEC_ERR_INVALIDPARAMETER)
        || hr == WINCODEC_ERR_INVALIDPARAMETER
#endif
#if defined(WINCODEC_ERR_VALUEOUTOFRANGE)
        || hr == WINCODEC_ERR_VALUEOUTOFRANGE
#endif
    )
    {
        return PVC_INVALID_DIMENSIONS;
    }
    if (
#if defined(WINCODEC_ERR_BADHEADER)
        hr == WINCODEC_ERR_BADHEADER ||
#endif
#if defined(WINCODEC_ERR_BADIMAGE)
        hr == WINCODEC_ERR_BADIMAGE ||
#endif
#if defined(WINCODEC_ERR_BADMETADATAHEADER)
        hr == WINCODEC_ERR_BADMETADATAHEADER ||
#endif
#if defined(WINCODEC_ERR_BADSTREAMDATA)
        hr == WINCODEC_ERR_BADSTREAMDATA ||
#endif
#if defined(WINCODEC_ERR_STREAMREAD)
        hr == WINCODEC_ERR_STREAMREAD ||
#endif
#if defined(WINCODEC_ERR_STREAMWRITE)
        hr == WINCODEC_ERR_STREAMWRITE ||
#endif
#if defined(WINCODEC_ERR_STREAMNOTAVAILABLE)
        hr == WINCODEC_ERR_STREAMNOTAVAILABLE ||
#endif
#if defined(WINCODEC_ERR_UNEXPECTEDMETADATAFORM)
        hr == WINCODEC_ERR_UNEXPECTEDMETADATAFORM ||
#endif
#if defined(WINCODEC_ERR_INTERNALERROR)
        hr == WINCODEC_ERR_INTERNALERROR ||
#endif
#if defined(WINCODEC_ERR_INVALIDPROGRESSIVELEVEL)
        hr == WINCODEC_ERR_INVALIDPROGRESSIVELEVEL ||
#endif
#if defined(WINCODEC_ERR_UNSUPPORTEDVERSION)
        hr == WINCODEC_ERR_UNSUPPORTEDVERSION ||
#endif
        hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF) || hr == HRESULT_FROM_WIN32(ERROR_CRC) || hr == E_FAIL)
    {
        return (hr == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF)) ? PVC_UNEXPECTED_EOF : PVC_READING_ERROR;
    }
    if (hr == WINCODEC_ERR_UNSUPPORTEDPIXELFORMAT
#if defined(WINCODEC_ERR_COMPONENTNOTFOUND)
        || hr == WINCODEC_ERR_COMPONENTNOTFOUND
#endif
#if defined(WINCODEC_ERR_UNSUPPORTEDOPERATION)
        || hr == WINCODEC_ERR_UNSUPPORTEDOPERATION
#endif
#if defined(WINCODEC_ERR_CODECNOTFOUND)
        || hr == WINCODEC_ERR_CODECNOTFOUND
#endif
#if defined(WINCODEC_ERR_DECODERNOTFOUND)
        || hr == WINCODEC_ERR_DECODERNOTFOUND
#endif
#if defined(WINCODEC_ERR_UNKNOWNIMAGEFORMAT)
        || hr == WINCODEC_ERR_UNKNOWNIMAGEFORMAT
#endif
#if defined(WINCODEC_ERR_PROPERTYNOTSUPPORTED)
        || hr == WINCODEC_ERR_PROPERTYNOTSUPPORTED
#endif
    )
    {
        return PVC_UNSUP_FILE_TYPE;
    }
    return PVC_EXCEPTION;
}

std::wstring Utf8ToWide(const char* path)
{
    if (!path)
    {
        return std::wstring();
    }
    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    if (len <= 0)
    {
        len = MultiByteToWideChar(CP_ACP, 0, path, -1, nullptr, 0);
        if (len <= 0)
        {
            return std::wstring();
        }
        std::wstring wide(static_cast<size_t>(len), L'\0');
        MultiByteToWideChar(CP_ACP, 0, path, -1, wide.data(), len);
        if (!wide.empty() && wide.back() == L'\0')
        {
            wide.pop_back();
        }
        return wide;
    }
    std::wstring wide(static_cast<size_t>(len), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide.data(), len);
    if (!wide.empty() && wide.back() == L'\0')
    {
        wide.pop_back();
    }
    return wide;
}

PVCODE PopulateImageInfo(ImageHandle& handle, LPPVImageInfo info, DWORD bufferSize, bool hasPreviousImage,
                         DWORD previousImageIndex, int currentImage)
{
    if (!info)
    {
        return PVC_INVALID_HANDLE;
    }
    if (bufferSize < sizeof(PVImageInfo))
    {
        return PVC_INVALID_HANDLE;
    }

    const DWORD bytesToClear = std::min<DWORD>(bufferSize, static_cast<DWORD>(sizeof(PVImageInfo)));
    ZeroMemory(info, bytesToClear);
    info->cbSize = sizeof(PVImageInfo);
    info->FileSize = handle.baseInfo.FileSize;
    info->Colors = PV_COLOR_TC32;
    info->Format = handle.baseInfo.Format;
    info->Flags = handle.baseInfo.Flags;
    info->ColorModel = PVCM_RGB;
    info->NumOfImages = static_cast<DWORD>(handle.frames.size());
    info->StretchMode = handle.stretchMode;
    info->TotalBitDepth = 32;
    info->FSI = handle.hasFormatSpecificInfo ? &handle.formatInfo : nullptr;

    struct FormatLabel
    {
        DWORD format;
        const char* label;
    };

    static constexpr FormatLabel kFormatLabels[] = {
        {PVF_BMP, "BMP"},
        {PVF_PNG, "PNG"},
        {PVF_JPG, "JPEG"},
        {PVF_TIFF, "TIFF"},
        {PVF_GIF, "GIF"},
        {PVF_ICO, "ICO"},
    };

    const char* info1 = "WIC";
    for (const auto& entry : kFormatLabels)
    {
        if (entry.format == info->Format)
        {
            info1 = entry.label;
            break;
        }
    }
    StringCchCopyA(info->Info1, PV_MAX_INFO_LEN, info1);

    if (handle.frames.empty())
    {
        info->CurrentImage = 0;
        info->Width = 0;
        info->Height = 0;
        info->BytesPerLine = 0;
        info->StretchedWidth = 0;
        info->StretchedHeight = 0;
        return PVC_OK;
    }

    size_t fallbackIndex = 0;
    if (!handle.frames.empty() && hasPreviousImage)
    {
        fallbackIndex = std::min(static_cast<size_t>(previousImageIndex), handle.frames.size() - 1);
    }

    const size_t normalized = NormalizeFrameIndex(handle, currentImage, fallbackIndex);
    const FrameData& frame = handle.frames[normalized];

    info->CurrentImage = static_cast<DWORD>(normalized);
    info->Width = frame.width;
    info->Height = frame.height;
    info->BytesPerLine = frame.stride;

    double dpiX = 0.0;
    double dpiY = 0.0;
    if (frame.frame)
    {
        if (SUCCEEDED(frame.frame->GetResolution(&dpiX, &dpiY)))
        {
            auto clampDpi = [](double value) -> DWORD {
                if (!std::isfinite(value) || value <= 0.0)
                {
                    return 0;
                }
                const double rounded = std::floor(value + 0.5);
                if (rounded <= 0.0)
                {
                    return 0;
                }
                if (rounded > static_cast<double>(std::numeric_limits<DWORD>::max()))
                {
                    return std::numeric_limits<DWORD>::max();
                }
                return static_cast<DWORD>(rounded);
            };

            info->HorDPI = clampDpi(dpiX);
            info->VerDPI = clampDpi(dpiY);
        }
    }

    const LONGLONG stretchWidthSigned = handle.stretchWidth ? static_cast<LONGLONG>(handle.stretchWidth)
                                                            : static_cast<LONGLONG>(frame.width);
    const LONGLONG stretchHeightSigned = handle.stretchHeight ? static_cast<LONGLONG>(handle.stretchHeight)
                                                              : static_cast<LONGLONG>(frame.height);
    const ULONGLONG stretchWidthAbs = AbsoluteDimension(stretchWidthSigned);
    const ULONGLONG stretchHeightAbs = AbsoluteDimension(stretchHeightSigned);
    info->StretchedWidth = stretchWidthAbs > std::numeric_limits<DWORD>::max()
                               ? std::numeric_limits<DWORD>::max()
                               : static_cast<DWORD>(stretchWidthAbs);
    info->StretchedHeight = stretchHeightAbs > std::numeric_limits<DWORD>::max()
                                 ? std::numeric_limits<DWORD>::max()
                                 : static_cast<DWORD>(stretchHeightAbs);
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
    handle.baseInfo.Flags = 0;

    HRESULT hr = decoder->GetFrameCount(&frameCount);
    if (FAILED(hr))
    {
        return hr;
    }
    handle.frames.resize(frameCount);
    handle.hasFormatSpecificInfo = false;
    handle.formatInfo = {};
    handle.formatInfo.cbSize = sizeof(PVFormatSpecificInfo);
    handle.formatInfo.GIF.DisposalMethod = PVDM_UNDEFINED;
    handle.baseInfo.FSI = nullptr;
    handle.canvasWidth = 0;
    handle.canvasHeight = 0;

    ComPtr<IWICMetadataQueryReader> decoderQuery;
    if (FAILED(decoder->GetMetadataQueryReader(&decoderQuery)))
    {
        decoderQuery = nullptr;
    }

    bool hasExif = SourceContainsExif(decoder);
    if (!hasExif && decoderQuery)
    {
        hasExif = QueryReaderContainsExif(decoderQuery.Get());
    }

    UINT logicalScreenWidth = 0;
    UINT logicalScreenHeight = 0;
    bool hasLogicalScreenWidth = false;
    bool hasLogicalScreenHeight = false;
    UINT backgroundIndex = 0;
    bool hasBackgroundIndex = false;
    if (decoderQuery)
    {
        UINT value = 0;
        if (TryReadUnsignedMetadata(decoderQuery.Get(), L"/logscrdesc/Width", value))
        {
            logicalScreenWidth = value;
            hasLogicalScreenWidth = true;
        }
        if (TryReadUnsignedMetadata(decoderQuery.Get(), L"/logscrdesc/Height", value))
        {
            logicalScreenHeight = value;
            hasLogicalScreenHeight = true;
        }
        if (TryReadUnsignedMetadata(decoderQuery.Get(), L"/logscrdesc/BackgroundColorIndex", value))
        {
            backgroundIndex = value;
            hasBackgroundIndex = true;
        }
    }

    COLORREF backgroundColor = RGB(0, 0, 0);
    if (hasBackgroundIndex)
    {
        ComPtr<IWICPalette> palette;
        if (SUCCEEDED(backend.Factory()->CreatePalette(&palette)) && palette)
        {
            if (SUCCEEDED(decoder->CopyPalette(palette.Get())))
            {
                UINT paletteCount = 0;
                if (SUCCEEDED(palette->GetColorCount(&paletteCount)) && paletteCount > backgroundIndex)
                {
                    std::vector<WICColor> colors(paletteCount);
                    UINT actualCount = paletteCount;
                    if (SUCCEEDED(palette->GetColors(paletteCount, colors.data(), &actualCount)) &&
                        actualCount > backgroundIndex)
                    {
                        const WICColor color = colors[backgroundIndex];
                        const BYTE r = static_cast<BYTE>((color >> 16) & 0xFF);
                        const BYTE g = static_cast<BYTE>((color >> 8) & 0xFF);
                        const BYTE b = static_cast<BYTE>(color & 0xFF);
                        backgroundColor = RGB(r, g, b);
                    }
                }
            }
        }
    }

    if (hasLogicalScreenWidth)
    {
        handle.canvasWidth = ClampUnsignedToLong(logicalScreenWidth);
    }
    if (hasLogicalScreenHeight)
    {
        handle.canvasHeight = ClampUnsignedToLong(logicalScreenHeight);
    }

    const auto clampEdge = [](LONG origin, LONG extent, LONG limit) -> LONG {
        if (extent <= 0)
        {
            return origin;
        }
        LONGLONG sum = static_cast<LONGLONG>(origin) + static_cast<LONGLONG>(extent);
        if (limit > 0)
        {
            if (origin >= limit)
            {
                return limit;
            }
            if (sum > limit)
            {
                sum = limit;
            }
        }
        if (sum > static_cast<LONGLONG>(std::numeric_limits<LONG>::max()))
        {
            sum = std::numeric_limits<LONG>::max();
        }
        return static_cast<LONG>(sum);
    };
    for (UINT i = 0; i < frameCount; ++i)
    {
        FrameData data;
        hr = decoder->GetFrame(i, &data.frame);
        if (FAILED(hr))
        {
            return hr;
        }
        UINT width = 0;
        UINT height = 0;
        hr = data.frame->GetSize(&width, &height);
        if (FAILED(hr))
        {
            return hr;
        }
        if (width == 0 || height == 0)
        {
            return WINCODEC_ERR_INVALIDPARAMETER;
        }
        data.width = width;
        data.height = height;
        data.delayMs = GetFrameDelayMilliseconds(data.frame.Get());
        if (frameCount > 1 && data.delayMs == 0)
        {
            data.delayMs = 100;
        }
        data.rect.left = 0;
        data.rect.top = 0;
        data.rect.right = ClampUnsignedToLong(static_cast<ULONGLONG>(width));
        data.rect.bottom = ClampUnsignedToLong(static_cast<ULONGLONG>(height));
        data.disposal = PVDM_UNDEFINED;

        ULONGLONG left64 = 0;
        ULONGLONG top64 = 0;
        bool leftSpecified = false;
        bool topSpecified = false;
        ULONGLONG rectWidth64 = static_cast<ULONGLONG>(width);
        ULONGLONG rectHeight64 = static_cast<ULONGLONG>(height);
        ComPtr<IWICMetadataQueryReader> frameQuery;
        if (SUCCEEDED(data.frame->GetMetadataQueryReader(&frameQuery)) && frameQuery)
        {
            UINT value = 0;
            if (TryReadUnsignedMetadata(frameQuery.Get(), L"/imgdesc/Left", value))
            {
                left64 = value;
                leftSpecified = true;
            }
            if (TryReadUnsignedMetadata(frameQuery.Get(), L"/imgdesc/Top", value))
            {
                top64 = value;
                topSpecified = true;
            }
            if (TryReadUnsignedMetadata(frameQuery.Get(), L"/imgdesc/Width", value) && value > 0)
            {
                rectWidth64 = value;
            }
            if (TryReadUnsignedMetadata(frameQuery.Get(), L"/imgdesc/Height", value) && value > 0)
            {
                rectHeight64 = value;
            }
            if (TryReadUnsignedMetadata(frameQuery.Get(), L"/grctlext/Disposal", value))
            {
                data.disposal = MapGifDisposalToPv(value);
            }
        }
        LONG rectLeft = ClampUnsignedToLong(leftSpecified ? left64 : 0ull);
        LONG rectTop = ClampUnsignedToLong(topSpecified ? top64 : 0ull);
        if (handle.canvasWidth > 0)
        {
            if (rectLeft < 0)
            {
                rectLeft = 0;
            }
            else if (rectLeft > handle.canvasWidth)
            {
                rectLeft = handle.canvasWidth;
            }
        }
        if (handle.canvasHeight > 0)
        {
            if (rectTop < 0)
            {
                rectTop = 0;
            }
            else if (rectTop > handle.canvasHeight)
            {
                rectTop = handle.canvasHeight;
            }
        }
        data.rect.left = rectLeft;
        data.rect.top = rectTop;
        const LONG rectWidthLong = ClampUnsignedToLong(rectWidth64);
        const LONG rectHeightLong = ClampUnsignedToLong(rectHeight64);
        data.rect.right = clampEdge(rectLeft, rectWidthLong, handle.canvasWidth);
        data.rect.bottom = clampEdge(rectTop, rectHeightLong, handle.canvasHeight);
        if (!hasExif && FrameContainsExif(data.frame.Get()))
        {
            hasExif = true;
        }
        handle.frames[i] = std::move(data);
    }
    if (handle.canvasWidth <= 0 && !handle.frames.empty())
    {
        handle.canvasWidth = ClampUnsignedToLong(static_cast<ULONGLONG>(handle.frames[0].width));
    }
    if (handle.canvasHeight <= 0 && !handle.frames.empty())
    {
        handle.canvasHeight = ClampUnsignedToLong(static_cast<ULONGLONG>(handle.frames[0].height));
    }
    GUID container = {};
    decoder->GetContainerFormat(&container);
    handle.baseInfo.Format = MapFormatToPvFormat(container);
    handle.baseInfo.NumOfImages = frameCount;
    handle.baseInfo.FileSize = QueryFileSize(handle.fileName);

    if (handle.baseInfo.Format == PVF_GIF)
    {
        handle.hasFormatSpecificInfo = true;
        const LONG screenWidth = std::max<LONG>(0, handle.canvasWidth);
        const LONG screenHeight = std::max<LONG>(0, handle.canvasHeight);
        handle.formatInfo.GIF.ScreenWidth = static_cast<unsigned>(screenWidth);
        handle.formatInfo.GIF.ScreenHeight = static_cast<unsigned>(screenHeight);
        handle.formatInfo.GIF.XPosition = 0;
        handle.formatInfo.GIF.YPosition = 0;
        handle.formatInfo.GIF.Delay = 0;
        handle.formatInfo.GIF.TranspIndex = 0;
        handle.formatInfo.GIF.BgColor = backgroundColor;
        handle.baseInfo.FSI = &handle.formatInfo;
    }
    else
    {
        handle.hasFormatSpecificInfo = false;
        handle.baseInfo.FSI = nullptr;
    }

    if (!hasExif && handle.baseInfo.Format == PVF_JPG)
    {
        CExifFileBuffer buffer;
        bool loaded = false;
#ifdef _UNICODE
        loaded = buffer.LoadFromFile(handle.fileName.c_str());
#else
        loaded = buffer.LoadFromWideFile(handle.fileName.c_str());
#endif
        if (loaded)
        {
            hasExif = buffer.HasExifData();
        }
    }
    if (hasExif)
    {
        handle.baseInfo.Flags |= PVFF_EXIF;
    }
    if (frameCount > 1 && handle.baseInfo.Format == PVF_GIF)
    {
        handle.baseInfo.Flags |= PVFF_IMAGESEQUENCE;
    }
    return S_OK;
}

PVCODE DrawFrame(ImageHandle& handle, FrameData& frame, HDC dc, int x, int y, LPRECT rect)
{
    if (!dc)
    {
        return PVC_OK;
    }

    const LONGLONG stretchWidthSigned = handle.stretchWidth ? static_cast<LONGLONG>(handle.stretchWidth)
                                                            : static_cast<LONGLONG>(frame.width);
    const LONGLONG stretchHeightSigned = handle.stretchHeight ? static_cast<LONGLONG>(handle.stretchHeight)
                                                              : static_cast<LONGLONG>(frame.height);
    const ULONGLONG stretchWidthAbs = AbsoluteDimension(stretchWidthSigned);
    const ULONGLONG stretchHeightAbs = AbsoluteDimension(stretchHeightSigned);
    if (stretchWidthAbs == 0 || stretchHeightAbs == 0)
    {
        return PVC_OK;
    }

    if (stretchWidthAbs > static_cast<ULONGLONG>(std::numeric_limits<int>::max()) ||
        stretchHeightAbs > static_cast<ULONGLONG>(std::numeric_limits<int>::max()))
    {
        return PVC_INVALID_DIMENSIONS;
    }
    if (frame.width > static_cast<UINT>(std::numeric_limits<int>::max()) ||
        frame.height > static_cast<UINT>(std::numeric_limits<int>::max()))
    {
        return PVC_INVALID_DIMENSIONS;
    }

    RECT imageRect;
    imageRect.left = x;
    imageRect.top = y;
    const LONGLONG imageRight = static_cast<LONGLONG>(x) + static_cast<LONGLONG>(stretchWidthAbs);
    const LONGLONG imageBottom = static_cast<LONGLONG>(y) + static_cast<LONGLONG>(stretchHeightAbs);
    if (imageRight > std::numeric_limits<LONG>::max() || imageRight < std::numeric_limits<LONG>::min() ||
        imageBottom > std::numeric_limits<LONG>::max() || imageBottom < std::numeric_limits<LONG>::min())
    {
        return PVC_INVALID_DIMENSIONS;
    }
    imageRect.right = static_cast<LONG>(imageRight);
    imageRect.bottom = static_cast<LONG>(imageBottom);

    RECT clipRect = imageRect;
    if (rect)
    {
        if (!IntersectRect(&clipRect, &imageRect, rect))
        {
            return PVC_OK;
        }
    }

    int savedState = 0;
    bool resetClip = false;
    if (rect)
    {
        savedState = SaveDC(dc);
        if (savedState == 0)
        {
            resetClip = true;
        }
        const int clipResult = IntersectClipRect(dc, clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);
        if (clipResult == ERROR)
        {
            if (savedState > 0)
            {
                RestoreDC(dc, savedState);
            }
            else if (resetClip)
            {
                SelectClipRgn(dc, nullptr);
            }
            return PVC_GDI_ERROR;
        }
        if (clipResult == NULLREGION)
        {
            if (savedState > 0)
            {
                RestoreDC(dc, savedState);
            }
            else if (resetClip)
            {
                SelectClipRgn(dc, nullptr);
            }
            return PVC_OK;
        }
    }

    int previousMode = SetStretchBltMode(dc, handle.stretchMode ? static_cast<int>(handle.stretchMode) : COLORONCOLOR);
    BITMAPINFO bmi{};
    bmi.bmiHeader = frame.bmi;

    const int destX = stretchWidthSigned >= 0 ? imageRect.left : imageRect.right - 1;
    const int destY = stretchHeightSigned >= 0 ? imageRect.top : imageRect.bottom - 1;
    const int destWidth = stretchWidthSigned >= 0 ? static_cast<int>(stretchWidthAbs)
                                                  : -static_cast<int>(stretchWidthAbs);
    const int destHeight = stretchHeightSigned >= 0 ? static_cast<int>(stretchHeightAbs)
                                                    : -static_cast<int>(stretchHeightAbs);

    const int result = StretchDIBits(dc, destX, destY, destWidth, destHeight, 0, 0, frame.width, frame.height,
                                     frame.pixels.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

    if (previousMode > 0)
    {
        SetStretchBltMode(dc, previousMode);
    }
    if (savedState > 0)
    {
        RestoreDC(dc, savedState);
    }
    else if (resetClip)
    {
        SelectClipRgn(dc, nullptr);
    }
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
        node->Rect = frame.rect;
        node->Delay = frame.delayMs;
        node->DisposalMethod = frame.disposal;
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
    if (handle.frames.empty())
    {
        return PVC_INVALID_HANDLE;
    }
    const size_t normalizedIndex = NormalizeFrameIndex(handle, imageIndex, 0);
    FrameData& frame = handle.frames[normalizedIndex];
    HRESULT hr = DecodeFrame(handle, normalizedIndex);
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
    if (m_hr == RPC_E_CHANGED_MODE)
    {
        m_hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    }
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
    *Img = nullptr;
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

    if (pOpenExInfo->Flags & PVOF_ATTACH_TO_HANDLE)
    {
        HBITMAP bitmap = reinterpret_cast<HBITMAP>(pOpenExInfo->Handle);
        if (!bitmap)
        {
            return PVC_INVALID_HANDLE;
        }

        FrameData frame;
        HRESULT hr = PopulateFrameFromBitmapHandle(frame, bitmap);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }

        image->frames.push_back(std::move(frame));
        image->baseInfo.Format = PVF_BMP;
        image->baseInfo.Flags = 0;
        image->baseInfo.NumOfImages = 1;
        image->baseInfo.FileSize = 0;
    }
    else
    {
        image->fileName = Utf8ToWide(pOpenExInfo->FileName);
        if (image->fileName.empty())
        {
            return PVC_CANNOT_OPEN_FILE;
        }
        image->baseInfo.FileSize = QueryFileSize(image->fileName);

        Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
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

    image->baseInfo.NumOfImages = static_cast<DWORD>(image->frames.size());

    HRESULT hr = DecodeFrame(*image, 0);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }

    if (pImgInfo)
    {
        const DWORD bufferSize = size > 0 ? static_cast<DWORD>(size) : 0;
        PopulateImageInfo(*image, pImgInfo, bufferSize, false, 0, 0);
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
        frame.converter.Reset();
        frame.colorConvertedSource.Reset();
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
    if (handle->frames.empty())
    {
        return PVC_INVALID_HANDLE;
    }
    const size_t normalizedIndex = NormalizeFrameIndex(*handle, imageIndex, 0);
    HRESULT hr = DecodeFrame(*handle, normalizedIndex);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    return DrawFrame(*handle, handle->frames[normalizedIndex], paintDC, dRect ? dRect->left : 0,
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
    return DrawFrame(*handle, handle->frames[0], paintDC, x, y, rect);
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
    const auto convert = [](DWORD value) -> LONG {
        if (value == 0 || value == 0x80000000u)
        {
            return 0;
        }
        const LONG signedValue = static_cast<LONG>(value);
        if ((value & 0x80000000u) != 0u)
        {
            return signedValue;
        }
        if (value > static_cast<DWORD>(std::numeric_limits<LONG>::max()))
        {
            return std::numeric_limits<LONG>::max();
        }
        return signedValue;
    };

    handle->stretchWidth = convert(width);
    handle->stretchHeight = convert(height);
    handle->stretchMode = mode;
    return PVC_OK;
}

PVCODE WINAPI Backend::sPVLoadFromClipboard(LPPVHandle* /*Img*/, LPPVImageInfo /*pImgInfo*/, int /*size*/)
{
    return PVC_UNSUP_FILE_TYPE;
}

PVCODE WINAPI Backend::sPVGetImageInfo(LPPVHandle Img, LPPVImageInfo pImgInfo, int size, int imageIndex)
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
    const DWORD bufferSize = size > 0 ? static_cast<DWORD>(size) : 0;
    DWORD previousIndex = 0;
    bool hasPreviousIndex = false;
    if (bufferSize >= sizeof(PVImageInfo) && pImgInfo->cbSize == sizeof(PVImageInfo))
    {
        previousIndex = pImgInfo->CurrentImage;
        hasPreviousIndex = true;
    }
    size_t fallbackIndex = 0;
    if (!handle->frames.empty())
    {
        const size_t lastIndex = handle->frames.size() - 1;
        fallbackIndex = std::min(static_cast<size_t>(previousIndex), lastIndex);
    }
    else
    {
        return PVC_INVALID_HANDLE;
    }
    const size_t normalizedIndex = NormalizeFrameIndex(*handle, imageIndex, fallbackIndex);
    HRESULT hr = DecodeFrame(*handle, normalizedIndex);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    return PopulateImageInfo(*handle, pImgInfo, bufferSize, hasPreviousIndex, previousIndex, imageIndex);
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
    frame.converter.Reset();
    frame.colorConvertedSource.Reset();
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
    HRESULT finalizeHr = FinalizeDecodedFrame(frame);
    if (FAILED(finalizeHr))
    {
        return HResultToPvCode(finalizeHr);
    }
    frame.rect.left = 0;
    frame.rect.top = 0;
    frame.rect.right = ClampUnsignedToLong(static_cast<ULONGLONG>(frame.width));
    frame.rect.bottom = ClampUnsignedToLong(static_cast<ULONGLONG>(frame.height));
    frame.disposal = PVDM_UNDEFINED;
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
    frame.converter.Reset();
    frame.colorConvertedSource.Reset();
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
    frame.width = static_cast<UINT>(width);
    frame.height = static_cast<UINT>(height);
    frame.stride = newStride;
    frame.pixels.swap(cropped);
    HRESULT finalizeHr = FinalizeDecodedFrame(frame);
    if (FAILED(finalizeHr))
    {
        return HResultToPvCode(finalizeHr);
    }
    frame.rect.left = 0;
    frame.rect.top = 0;
    frame.rect.right = ClampUnsignedToLong(static_cast<ULONGLONG>(frame.width));
    frame.rect.bottom = ClampUnsignedToLong(static_cast<ULONGLONG>(frame.height));
    frame.disposal = PVDM_UNDEFINED;
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
    if (handle->frames.empty())
    {
        return PVC_INVALID_HANDLE;
    }
    const size_t normalizedIndex = NormalizeFrameIndex(*handle, imageIndex, 0);
    HRESULT hr = DecodeFrame(*handle, normalizedIndex);
    if (FAILED(hr))
    {
        return HResultToPvCode(hr);
    }
    FrameData& frame = handle->frames[normalizedIndex];

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
        Microsoft::WRL::ComPtr<IWICBitmapSource> scaleSource;
        if (frame.converter)
        {
            scaleSource = frame.converter;
        }
        else
        {
            Microsoft::WRL::ComPtr<IWICBitmap> bitmap;
            hr = handle->backend->Factory()->CreateBitmapFromMemory(frame.width, frame.height, GUID_WICPixelFormat32bppBGRA,
                                                                    frame.stride, static_cast<UINT>(frame.pixels.size()),
                                                                    frame.pixels.data(), &bitmap);
            if (FAILED(hr))
            {
                return HResultToPvCode(hr);
            }
            scaleSource = bitmap;
        }

        Microsoft::WRL::ComPtr<IWICBitmapScaler> scaler;
        hr = handle->backend->Factory()->CreateBitmapScaler(&scaler);
        if (FAILED(hr))
        {
            return HResultToPvCode(hr);
        }
        hr = scaler->Initialize(scaleSource.Get(), desiredWidth, desiredHeight, WICBitmapInterpolationModeFant);
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
