// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Decode catalog, package, and default plugin icons into native image lists. | Match the original WinForms updater's 16-pixel image column without blocking network work on the UI thread. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_icons.h"
#include "native_utils.h"

#include <objbase.h>
#include <shellapi.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace SamandarinNative
{
namespace
{
using Microsoft::WRL::ComPtr;

class ScopedComInitialization
{
public:
    ScopedComInitialization() : Result(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))
    {
        Uninitialize = SUCCEEDED(Result);
    }

    ~ScopedComInitialization()
    {
        if (Uninitialize) CoUninitialize();
    }

    bool IsUsable() const { return SUCCEEDED(Result) || Result == RPC_E_CHANGED_MODE; }

private:
    HRESULT Result = E_FAIL;
    bool Uninitialize = false;
};

bool CreateImagingFactory(ComPtr<IWICImagingFactory>* factory)
{
    if (factory == nullptr) return false;
    return SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                     IID_PPV_ARGS(factory->GetAddressOf())));
}

HBITMAP CreateIconBitmap(IWICImagingFactory* factory, IWICBitmapSource* source, int iconSize)
{
    if (factory == nullptr || source == nullptr || iconSize <= 0) return nullptr;
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (FAILED(source->GetSize(&sourceWidth, &sourceHeight)) || sourceWidth == 0 || sourceHeight == 0)
        return nullptr;

    const double scale = (std::min)(static_cast<double>(iconSize) / sourceWidth,
                                    static_cast<double>(iconSize) / sourceHeight);
    const UINT targetWidth = static_cast<UINT>((std::max)(1L, std::lround(sourceWidth * scale)));
    const UINT targetHeight = static_cast<UINT>((std::max)(1L, std::lround(sourceHeight * scale)));

    ComPtr<IWICBitmapScaler> scaler;
    if (FAILED(factory->CreateBitmapScaler(scaler.GetAddressOf())) ||
        FAILED(scaler->Initialize(source, targetWidth, targetHeight, WICBitmapInterpolationModeFant)))
        return nullptr;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
        FAILED(converter->Initialize(scaler.Get(), GUID_WICPixelFormat32bppPBGRA,
                                     WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom)))
        return nullptr;

    const UINT stride = targetWidth * 4;
    const UINT bufferSize = stride * targetHeight;
    std::vector<BYTE> pixels(bufferSize);
    if (FAILED(converter->CopyPixels(nullptr, stride, bufferSize, pixels.data()))) return nullptr;

    BITMAPINFO bitmapInfo = {};
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = iconSize;
    bitmapInfo.bmiHeader.biHeight = -iconSize;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    void* destination = nullptr;
    HBITMAP bitmap = CreateDIBSection(nullptr, &bitmapInfo, DIB_RGB_COLORS,
                                      &destination, nullptr, 0);
    if (bitmap == nullptr || destination == nullptr)
    {
        if (bitmap != nullptr) DeleteObject(bitmap);
        return nullptr;
    }

    auto* output = static_cast<BYTE*>(destination);
    memset(output, 0, static_cast<size_t>(iconSize) * iconSize * 4);
    const UINT offsetX = (iconSize - targetWidth) / 2;
    const UINT offsetY = (iconSize - targetHeight) / 2;
    for (UINT row = 0; row < targetHeight; ++row)
    {
        const size_t destinationOffset =
            (static_cast<size_t>(row + offsetY) * iconSize + offsetX) * 4;
        memcpy(output + destinationOffset, pixels.data() + static_cast<size_t>(row) * stride,
               stride);
    }
    return bitmap;
}

int AddBitmap(HIMAGELIST imageList, HBITMAP bitmap)
{
    if (imageList == nullptr || bitmap == nullptr) return -1;
    const int index = ImageList_Add(imageList, bitmap, nullptr);
    DeleteObject(bitmap);
    return index;
}

int AddDecodedStream(HIMAGELIST imageList, IWICImagingFactory* factory, IStream* stream, int iconSize)
{
    if (imageList == nullptr || factory == nullptr || stream == nullptr) return -1;
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnLoad,
                                                decoder.GetAddressOf())))
        return -1;
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf()))) return -1;
    return AddBitmap(imageList, CreateIconBitmap(factory, frame.Get(), iconSize));
}

int GetImageListSize(HIMAGELIST imageList)
{
    int width = 0;
    int height = 0;
    if (imageList == nullptr || !ImageList_GetIconSize(imageList, &width, &height)) return 0;
    return (std::min)(width, height);
}

int AddResourceDefaultImage(HIMAGELIST imageList)
{
    HRSRC resource = FindResourceW(DLLInstance,
                                   MAKEINTRESOURCEW(IDR_SAMANDARIN_DEFAULT_PLUGIN_IMAGE),
                                   MAKEINTRESOURCEW(10));
    if (resource != nullptr)
    {
        const DWORD size = SizeofResource(DLLInstance, resource);
        HGLOBAL loaded = LoadResource(DLLInstance, resource);
        const void* bytes = loaded != nullptr ? LockResource(loaded) : nullptr;
        if (bytes != nullptr && size != 0)
        {
            const std::string imageBytes(static_cast<const char*>(bytes), size);
            const int index = NativeIcons_AddImage(imageList, imageBytes);
            if (index >= 0) return index;
        }
    }

    HICON icon = LoadIconW(DLLInstance, MAKEINTRESOURCEW(IDI_PLUGINICON));
    return icon != nullptr ? ImageList_AddIcon(imageList, icon) : -1;
}
} // namespace

HIMAGELIST NativeIcons_CreateImageList(int iconSize, int* defaultImageIndex)
{
    iconSize = (std::max)(1, iconSize);
    HIMAGELIST imageList = ImageList_Create(iconSize, iconSize,
                                            ILC_COLOR32 | ILC_MASK, 16, 8);
    if (imageList == nullptr)
    {
        if (defaultImageIndex != nullptr) *defaultImageIndex = -1;
        return nullptr;
    }
    const int defaultIndex = AddResourceDefaultImage(imageList);
    if (defaultImageIndex != nullptr) *defaultImageIndex = defaultIndex;
    return imageList;
}

int NativeIcons_AddImage(HIMAGELIST imageList, const std::string& imageBytes)
{
    if (imageList == nullptr || imageBytes.empty() || imageBytes.size() > MAXDWORD) return -1;
    const int iconSize = GetImageListSize(imageList);
    if (iconSize <= 0) return -1;
    ScopedComInitialization com;
    if (!com.IsUsable()) return -1;
    ComPtr<IWICImagingFactory> factory;
    if (!CreateImagingFactory(&factory)) return -1;
    ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(stream.GetAddressOf())) ||
        FAILED(stream->InitializeFromMemory(
            reinterpret_cast<BYTE*>(const_cast<char*>(imageBytes.data())),
            static_cast<DWORD>(imageBytes.size()))))
        return -1;
    return AddDecodedStream(imageList, factory.Get(), stream.Get(), iconSize);
}

int NativeIcons_AddImageFromPath(HIMAGELIST imageList, const std::wstring& path)
{
    if (imageList == nullptr || path.empty()) return -1;
    const int iconSize = GetImageListSize(imageList);
    if (iconSize <= 0) return -1;
    const std::wstring extendedPath = AddLongPathPrefix(path);
    HICON largeIcon = nullptr;
    HICON smallIcon = nullptr;
    if (ExtractIconExW(extendedPath.c_str(), 0, &largeIcon, &smallIcon, 1) != 0)
    {
        const HICON icon = smallIcon != nullptr ? smallIcon : largeIcon;
        const int index = icon != nullptr ? ImageList_AddIcon(imageList, icon) : -1;
        if (smallIcon != nullptr) DestroyIcon(smallIcon);
        if (largeIcon != nullptr && largeIcon != smallIcon) DestroyIcon(largeIcon);
        if (index >= 0) return index;
    }
    else
    {
        if (smallIcon != nullptr) DestroyIcon(smallIcon);
        if (largeIcon != nullptr) DestroyIcon(largeIcon);
    }

    ScopedComInitialization com;
    if (!com.IsUsable()) return -1;
    ComPtr<IWICImagingFactory> factory;
    if (!CreateImagingFactory(&factory)) return -1;
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(extendedPath.c_str(), nullptr, GENERIC_READ,
                                                   WICDecodeMetadataCacheOnLoad,
                                                   decoder.GetAddressOf())))
        return -1;
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf()))) return -1;
    return AddBitmap(imageList, CreateIconBitmap(factory.Get(), frame.Get(), iconSize));
}
} // namespace SamandarinNative
