// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cstring>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

namespace PictView::Native
{
namespace Detail
{
namespace
{

constexpr UINT kMaxMetafileEdge = 2048;

INT16 ReadI16(const BYTE* data)
{
    return static_cast<INT16>(data[0] | (data[1] << 8));
}

Status RasterizeEnhMetaFile(HENHMETAFILE hemf, Frame& frame)
{
    if (hemf == nullptr)
    {
        return Status::Invalid;
    }
    ENHMETAHEADER header{};
    if (GetEnhMetaFileHeader(hemf, sizeof(header), &header) == 0)
    {
        return Status::Invalid;
    }

    LONG widthHimetric = header.rclFrame.right - header.rclFrame.left;
    LONG heightHimetric = header.rclFrame.bottom - header.rclFrame.top;
    if (widthHimetric <= 0 || heightHimetric <= 0)
    {
        widthHimetric = (header.rclBounds.right - header.rclBounds.left) * 2540 /
                        (std::max)(1L, header.szlDevice.cx > 0 ? header.szlDevice.cx : 96);
        heightHimetric = (header.rclBounds.bottom - header.rclBounds.top) * 2540 /
                         (std::max)(1L, header.szlDevice.cy > 0 ? header.szlDevice.cy : 96);
    }
    if (widthHimetric <= 0 || heightHimetric <= 0)
    {
        widthHimetric = (std::max)(1L, header.szlMillimeters.cx) * 100;
        heightHimetric = (std::max)(1L, header.szlMillimeters.cy) * 100;
    }

    HDC screen = GetDC(nullptr);
    const int dpiX = screen != nullptr ? GetDeviceCaps(screen, LOGPIXELSX) : 96;
    const int dpiY = screen != nullptr ? GetDeviceCaps(screen, LOGPIXELSY) : 96;
    if (screen != nullptr)
    {
        ReleaseDC(nullptr, screen);
    }

    UINT width = static_cast<UINT>((std::max)(1L, (widthHimetric * dpiX + 1270) / 2540));
    UINT height = static_cast<UINT>((std::max)(1L, (heightHimetric * dpiY + 1270) / 2540));
    if (width > kMaxMetafileEdge || height > kMaxMetafileEdge)
    {
        const double scale = (std::min)(static_cast<double>(kMaxMetafileEdge) / width,
                                        static_cast<double>(kMaxMetafileEdge) / height);
        width = (std::max)(1u, static_cast<UINT>(width * scale));
        height = (std::max)(1u, static_cast<UINT>(height * scale));
    }

    Status st = MakeFrame(frame, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(width);
    info.bmiHeader.biHeight = -static_cast<LONG>(height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC memDc = CreateCompatibleDC(nullptr);
    if (memDc == nullptr)
    {
        return Status::Invalid;
    }
    HBITMAP dib = CreateDIBSection(memDc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (dib == nullptr || bits == nullptr)
    {
        DeleteDC(memDc);
        return Status::OutOfMemory;
    }
    HGDIOBJ old = SelectObject(memDc, dib);
    RECT rc{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    HBRUSH background = CreateSolidBrush(RGB(255, 255, 255));
    if (background != nullptr)
    {
        FillRect(memDc, &rc, background);
        DeleteObject(background);
    }
    PlayEnhMetaFile(memDc, hemf, &rc);
    memcpy(frame.bgra.data(), bits, frame.bgra.size());
    const size_t count = static_cast<size_t>(width) * height;
    for (size_t i = 0; i < count; ++i)
    {
        frame.bgra[i * 4 + 3] = 255;
    }
    SelectObject(memDc, old);
    DeleteObject(dib);
    DeleteDC(memDc);
    return Status::Ok;
}

} // namespace

Status RasterizeMetafile(const BYTE* data, size_t size, Frame& frame)
{
    if (LooksLikeEmf(data, size))
    {
        HENHMETAFILE hemf = SetEnhMetaFileBits(static_cast<UINT>(size), data);
        if (hemf == nullptr)
        {
            return Status::Invalid;
        }
        const Status st = RasterizeEnhMetaFile(hemf, frame);
        DeleteEnhMetaFile(hemf);
        return st;
    }

    const BYTE* wmf = data;
    size_t wmfSize = size;
    METAFILEPICT pict{};
    pict.mm = MM_ANISOTROPIC;
    if (LooksLikePlaceableWmf(data, size))
    {
        const INT16 left = ReadI16(data + 6);
        const INT16 top = ReadI16(data + 8);
        const INT16 right = ReadI16(data + 10);
        const INT16 bottom = ReadI16(data + 12);
        const INT16 inch = ReadI16(data + 14);
        const INT width = right - left;
        const INT height = bottom - top;
        if (width > 0 && height > 0 && inch > 0)
        {
            pict.xExt = MulDiv(width, 2540, inch);
            pict.yExt = MulDiv(height, 2540, inch);
        }
        wmf = data + 22;
        wmfSize = size - 22;
    }
    else if (!LooksLikeStandardWmf(data, size))
    {
        return Status::Unsupported;
    }
    if (wmfSize < 18)
    {
        return Status::Invalid;
    }

    HDC hdc = GetDC(nullptr);
    HENHMETAFILE hemf = SetWinMetaFileBits(static_cast<UINT>(wmfSize), wmf, hdc, &pict);
    if (hdc != nullptr)
    {
        ReleaseDC(nullptr, hdc);
    }
    if (hemf == nullptr)
    {
        return Status::Invalid;
    }
    const Status st = RasterizeEnhMetaFile(hemf, frame);
    DeleteEnhMetaFile(hemf);
    return st;
}

Status DecodeWmf(Reader& reader, DecodedImage& image)
{
    const BYTE* data = reader.Data();
    const size_t size = reader.Size();
    const bool emf = LooksLikeEmf(data, size);
    if (!emf && !LooksLikePlaceableWmf(data, size) && !LooksLikeStandardWmf(data, size))
    {
        return Status::Unsupported;
    }
    Frame frame;
    const Status st = RasterizeMetafile(data, size, frame);
    if (st != Status::Ok)
    {
        return st;
    }
    return FinishSingle(image, std::move(frame), emf ? PVF_EMF : PVF_WMF, emf ? "EMF" : "WMF");
}

} // namespace Detail
} // namespace PictView::Native
