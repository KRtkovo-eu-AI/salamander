// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "NativeDecoder.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace PictView::Native
{
namespace Detail
{

constexpr UINT kMaxDimension = 16384;
constexpr size_t kMaxFileBytes = 256ull * 1024ull * 1024ull;
constexpr UINT kMaxFrames = 256;
constexpr UINT kBytesPerPixel = 4;

inline bool MulOverflow(size_t a, size_t b, size_t& result)
{
    if (a != 0 && b > (std::numeric_limits<size_t>::max)() / a)
    {
        return true;
    }
    result = a * b;
    return false;
}

inline Status MakeFrame(Frame& frame, UINT width, UINT height, UINT bitDepth, DWORD colors)
{
    if (width == 0 || height == 0 || width > kMaxDimension || height > kMaxDimension)
    {
        return Status::Invalid;
    }
    size_t bytes = 0;
    if (MulOverflow(static_cast<size_t>(width), kBytesPerPixel, bytes) ||
        MulOverflow(bytes, static_cast<size_t>(height), bytes))
    {
        return Status::OutOfMemory;
    }
    try
    {
        frame.bgra.assign(bytes, 0);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }
    frame.width = width;
    frame.height = height;
    frame.bitDepth = bitDepth;
    frame.colors = colors;
    frame.hasTransparency = false;
    return Status::Ok;
}

inline BYTE* Row(Frame& frame, UINT y)
{
    return frame.bgra.data() + static_cast<size_t>(y) * frame.width * kBytesPerPixel;
}

inline const BYTE* Row(const Frame& frame, UINT y)
{
    return frame.bgra.data() + static_cast<size_t>(y) * frame.width * kBytesPerPixel;
}

inline void SetPixel(Frame& frame, UINT x, UINT y, BYTE b, BYTE g, BYTE r, BYTE a)
{
    BYTE* p = Row(frame, y) + static_cast<size_t>(x) * kBytesPerPixel;
    p[0] = b;
    p[1] = g;
    p[2] = r;
    p[3] = a;
    if (a < 255)
    {
        frame.hasTransparency = true;
    }
}

inline void NoteAlpha(Frame& frame)
{
    const size_t count = static_cast<size_t>(frame.width) * frame.height;
    for (size_t i = 0; i < count; ++i)
    {
        if (frame.bgra[i * 4 + 3] < 255)
        {
            frame.hasTransparency = true;
            return;
        }
    }
}

// BI_RGB 32-bpp DIBs and Explorer thumbnails often store an unused fourth byte.
// If no pixel is actually opaque (alpha 255), treat the plane as unused so
// premultiplied display does not wipe the colour channels.
inline void ForceUnusedAlphaOpaque(Frame& frame)
{
    const size_t count = static_cast<size_t>(frame.width) * frame.height;
    if (count == 0 || frame.bgra.size() < count * 4)
    {
        return;
    }
    for (size_t i = 0; i < count; ++i)
    {
        if (frame.bgra[i * 4 + 3] == 255)
        {
            return;
        }
    }
    for (size_t i = 0; i < count; ++i)
    {
        frame.bgra[i * 4 + 3] = 255;
    }
    frame.hasTransparency = false;
}

inline Status FinishSingle(DecodedImage& image, Frame&& frame, DWORD format, const char* label)
{
    image.format = format;
    image.formatLabel = label;
    image.frames.clear();
    image.frames.push_back(std::move(frame));
    return Status::Ok;
}

bool InflateZlib(const BYTE* src, size_t srcLen, std::vector<BYTE>& dest, size_t maxOut);
bool InflateRaw(const BYTE* src, size_t srcLen, std::vector<BYTE>& dest, size_t maxOut);
Status DecodePng8(const BYTE* data, size_t size, Frame& frame);
Status DecodePackedDib(const BYTE* data, size_t size, Frame& frame);

class Reader
{
public:
    Reader(const BYTE* data, size_t size)
        : m_data(data)
        , m_size(size)
        , m_pos(0)
        , m_failed(data == nullptr && size != 0)
    {
    }

    bool Failed() const { return m_failed; }
    size_t Position() const { return m_pos; }
    size_t Remaining() const { return m_pos <= m_size ? m_size - m_pos : 0; }
    const BYTE* Data() const { return m_data; }
    size_t Size() const { return m_size; }

    void Seek(size_t pos)
    {
        if (pos > m_size)
        {
            m_failed = true;
            return;
        }
        m_pos = pos;
    }

    void Skip(size_t count)
    {
        if (m_failed || count > Remaining())
        {
            m_failed = true;
            return;
        }
        m_pos += count;
    }

    BYTE U8()
    {
        if (m_failed || Remaining() < 1)
        {
            m_failed = true;
            return 0;
        }
        return m_data[m_pos++];
    }

    UINT16 U16LE()
    {
        const BYTE lo = U8();
        const BYTE hi = U8();
        return static_cast<UINT16>(lo | (static_cast<UINT16>(hi) << 8));
    }

    UINT16 U16BE()
    {
        const BYTE hi = U8();
        const BYTE lo = U8();
        return static_cast<UINT16>(lo | (static_cast<UINT16>(hi) << 8));
    }

    UINT32 U32LE()
    {
        const UINT16 lo = U16LE();
        const UINT16 hi = U16LE();
        return static_cast<UINT32>(lo | (static_cast<UINT32>(hi) << 16));
    }

    UINT32 U32BE()
    {
        const UINT16 hi = U16BE();
        const UINT16 lo = U16BE();
        return static_cast<UINT32>(lo | (static_cast<UINT32>(hi) << 16));
    }

    bool Read(void* dest, size_t count)
    {
        if (m_failed || count > Remaining())
        {
            m_failed = true;
            return false;
        }
        if (count > 0)
        {
            memcpy(dest, m_data + m_pos, count);
        }
        m_pos += count;
        return true;
    }

    const BYTE* Peek(size_t count)
    {
        if (m_failed || count > Remaining())
        {
            m_failed = true;
            return nullptr;
        }
        return m_data + m_pos;
    }

    bool StartsWith(const void* magic, size_t length) const
    {
        if (m_size < length)
        {
            return false;
        }
        return memcmp(m_data, magic, length) == 0;
    }

    bool HasAt(size_t offset, const void* magic, size_t length) const
    {
        if (offset + length > m_size)
        {
            return false;
        }
        return memcmp(m_data + offset, magic, length) == 0;
    }

private:
    const BYTE* m_data;
    size_t m_size;
    size_t m_pos;
    bool m_failed;
};

inline UINT32 FourCC(const char* s)
{
    return static_cast<UINT32>(static_cast<BYTE>(s[0])) |
           (static_cast<UINT32>(static_cast<BYTE>(s[1])) << 8) |
           (static_cast<UINT32>(static_cast<BYTE>(s[2])) << 16) |
           (static_cast<UINT32>(static_cast<BYTE>(s[3])) << 24);
}

inline UINT32 FourCCBE(const char* s)
{
    return (static_cast<UINT32>(static_cast<BYTE>(s[0])) << 24) |
           (static_cast<UINT32>(static_cast<BYTE>(s[1])) << 16) |
           (static_cast<UINT32>(static_cast<BYTE>(s[2])) << 8) |
           static_cast<UINT32>(static_cast<BYTE>(s[3]));
}

inline bool LooksLikePsOrPdf(const BYTE* data, size_t size)
{
    if (data == nullptr || size < 2)
    {
        return false;
    }
    if (data[0] == '%' && data[1] == '!')
    {
        return true;
    }
    if (size >= 5 && memcmp(data, "%PDF-", 5) == 0)
    {
        return true;
    }
    if (size >= 4 && data[0] == 0xC5 && data[1] == 0xD0 && data[2] == 0xD3 && data[3] == 0xC6)
    {
        return true;
    }
    return false;
}

inline bool LooksLikePlaceableWmf(const BYTE* data, size_t size)
{
    return data != nullptr && size >= 22 && data[0] == 0xD7 && data[1] == 0xCD && data[2] == 0xC6 &&
           data[3] == 0x9A;
}

inline bool LooksLikeEmf(const BYTE* data, size_t size)
{
    if (data == nullptr || size < 88)
    {
        return false;
    }
    const UINT32 type = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    const UINT32 signature = data[40] | (data[41] << 8) | (data[42] << 16) | (data[43] << 24);
    return type == 1 && signature == 0x464D4520; // ' EMF'
}

inline bool LooksLikeStandardWmf(const BYTE* data, size_t size)
{
    if (data == nullptr || size < 18)
    {
        return false;
    }
    const UINT16 type = static_cast<UINT16>(data[0] | (data[1] << 8));
    const UINT16 headerSize = static_cast<UINT16>(data[2] | (data[3] << 8));
    const UINT16 version = static_cast<UINT16>(data[4] | (data[5] << 8));
    return (type == 1 || type == 2) && headerSize == 9 && (version == 0x0100 || version == 0x0300);
}

Status DecodeTga(Reader& reader, DecodedImage& image);
Status DecodePcx(Reader& reader, DecodedImage& image);
Status DecodeDcx(Reader& reader, DecodedImage& image);
Status DecodePnm(Reader& reader, DecodedImage& image);
Status DecodeRas(Reader& reader, DecodedImage& image);
Status DecodeSgi(Reader& reader, DecodedImage& image);
Status DecodeWbmp(Reader& reader, DecodedImage& image);
Status DecodePsd(Reader& reader, DecodedImage& image);
Status DecodeDds(Reader& reader, DecodedImage& image);
Status RasterizeMetafile(const BYTE* data, size_t size, Frame& frame);
Status DecodeWmf(Reader& reader, DecodedImage& image);
Status DecodeIff(Reader& reader, DecodedImage& image);
Status DecodeAni(Reader& reader, DecodedImage& image);
Status DecodeIcoCur(Reader& reader, DecodedImage& image);
Status DecodeFli(Reader& reader, DecodedImage& image);
Status DecodeSvg(Reader& reader, DecodedImage& image);
Status DecodePreview(Reader& reader, DecodedImage& image);
Status DecodePdn(Reader& reader, DecodedImage& image);
Status DecodeXcf(Reader& reader, DecodedImage& image);
Status Decode3dm(Reader& reader, DecodedImage& image);
Status DecodeDwg(Reader& reader, DecodedImage& image);
Status DecodeSkp(Reader& reader, DecodedImage& image);
Status DecodeRiffPreview(Reader& reader, DecodedImage& image);
Status DecodeBlend(Reader& reader, DecodedImage& image);

} // namespace Detail
} // namespace PictView::Native
