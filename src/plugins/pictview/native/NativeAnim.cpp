// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <cstring>
#include <vector>

namespace PictView::Native
{
namespace Detail
{
namespace
{

Status DecodeIcoFrame(const BYTE* data, size_t size, UINT widthHint, UINT heightHint, Frame& frame)
{
    if (size < 40)
    {
        return Status::Unsupported;
    }
    Reader reader(data, size);
    const UINT32 headerSize = reader.U32LE();
    if (headerSize == 0x28)
    {
        const INT32 widthSigned = static_cast<INT32>(reader.U32LE());
        const INT32 heightSigned = static_cast<INT32>(reader.U32LE());
        reader.U16LE(); // planes
        const UINT16 bitCount = reader.U16LE();
        const UINT32 compression = reader.U32LE();
        reader.Skip(20);
        if (reader.Failed() || compression != 0)
        {
            return Status::Unsupported;
        }
        const UINT width = static_cast<UINT>(widthSigned < 0 ? -widthSigned : widthSigned);
        const UINT xorHeight = static_cast<UINT>(heightSigned < 0 ? -heightSigned : heightSigned);
        const UINT height = xorHeight / 2 != 0 && heightHint != 0 && xorHeight == heightHint * 2 ? heightHint : (xorHeight > heightHint && heightHint != 0 ? heightHint : xorHeight);
        const UINT storedHeight = xorHeight >= height ? xorHeight : height;
        if (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24 && bitCount != 32)
        {
            return Status::Unsupported;
        }
        RGBQUAD palette[256]{};
        const UINT palCount = bitCount <= 8 ? (1u << bitCount) : 0;
        for (UINT i = 0; i < palCount; ++i)
        {
            palette[i].rgbBlue = reader.U8();
            palette[i].rgbGreen = reader.U8();
            palette[i].rgbRed = reader.U8();
            reader.U8();
            palette[i].rgbReserved = 255;
        }
        Status st = MakeFrame(frame, width, height, bitCount, bitCount <= 8 ? palCount : (bitCount == 24 ? PV_COLOR_TC24 : PV_COLOR_TC32));
        if (st != Status::Ok)
        {
            return st;
        }
        const UINT xorStride = ((width * bitCount + 31) / 32) * 4;
        const UINT andStride = ((width + 31) / 32) * 4;
        std::vector<BYTE> xorBits(static_cast<size_t>(xorStride) * height);
        if (!reader.Read(xorBits.data(), xorBits.size()))
        {
            return Status::Invalid;
        }
        std::vector<BYTE> andBits;
        if (bitCount < 32)
        {
            andBits.resize(static_cast<size_t>(andStride) * height);
            reader.Read(andBits.data(), andBits.size());
        }
        for (UINT y = 0; y < height; ++y)
        {
            const UINT srcY = height - 1 - y;
            const BYTE* src = xorBits.data() + static_cast<size_t>(srcY) * xorStride;
            for (UINT x = 0; x < width; ++x)
            {
                BYTE b = 0;
                BYTE g = 0;
                BYTE r = 0;
                BYTE a = 255;
                if (bitCount == 32)
                {
                    b = src[x * 4];
                    g = src[x * 4 + 1];
                    r = src[x * 4 + 2];
                    a = src[x * 4 + 3];
                }
                else if (bitCount == 24)
                {
                    b = src[x * 3];
                    g = src[x * 3 + 1];
                    r = src[x * 3 + 2];
                }
                else
                {
                    UINT index = 0;
                    if (bitCount == 8)
                    {
                        index = src[x];
                    }
                    else if (bitCount == 4)
                    {
                        const BYTE packed = src[x / 2];
                        index = (x & 1) ? (packed & 0x0F) : (packed >> 4);
                    }
                    else
                    {
                        const BYTE packed = src[x / 8];
                        index = (packed >> (7 - (x % 8))) & 1;
                    }
                    b = palette[index].rgbBlue;
                    g = palette[index].rgbGreen;
                    r = palette[index].rgbRed;
                }
                if (bitCount < 32 && !andBits.empty())
                {
                    const BYTE packed = andBits[static_cast<size_t>(srcY) * andStride + x / 8];
                    if ((packed >> (7 - (x % 8))) & 1)
                    {
                        a = 0;
                    }
                }
                SetPixel(frame, x, y, b, g, r, a);
            }
        }
        (void)storedHeight;
        (void)widthHint;
        return Status::Ok;
    }
    if (data[0] == 0x89 && size >= 8 && memcmp(data, "\x89PNG\r\n\x1a\n", 8) == 0)
    {
        return Status::Unsupported;
    }
    return Status::Unsupported;
}

} // namespace

Status DecodeIff(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 12 || !reader.StartsWith("FORM", 4))
    {
        return Status::Unsupported;
    }
    reader.Seek(8);
    const UINT32 type = reader.U32BE();
    const bool isIlbm = type == FourCCBE("ILBM");
    const bool isPbm = type == FourCCBE("PBM ");
    if (!isIlbm && !isPbm)
    {
        return Status::Unsupported;
    }

    UINT16 width = 0;
    UINT16 height = 0;
    BYTE planes = 0;
    BYTE masking = 0;
    BYTE compression = 0;
    BYTE transparentIndex = 0;
    RGBQUAD palette[256]{};
    UINT paletteCount = 0;
    std::vector<BYTE> body;
    bool haveBmhd = false;

    while (reader.Remaining() >= 8 && !reader.Failed())
    {
        const UINT32 chunk = reader.U32BE();
        const UINT32 chunkSize = reader.U32BE();
        if (chunkSize > reader.Remaining())
        {
            return Status::Invalid;
        }
        const size_t chunkStart = reader.Position();
        if (chunk == FourCCBE("BMHD"))
        {
            width = reader.U16BE();
            height = reader.U16BE();
            reader.U16BE();
            reader.U16BE();
            planes = reader.U8();
            masking = reader.U8();
            compression = reader.U8();
            reader.U8();
            transparentIndex = static_cast<BYTE>(reader.U16BE());
            haveBmhd = true;
        }
        else if (chunk == FourCCBE("CMAP"))
        {
            paletteCount = chunkSize / 3;
            if (paletteCount > 256)
            {
                paletteCount = 256;
            }
            for (UINT i = 0; i < paletteCount; ++i)
            {
                palette[i].rgbRed = reader.U8();
                palette[i].rgbGreen = reader.U8();
                palette[i].rgbBlue = reader.U8();
                palette[i].rgbReserved = 255;
            }
        }
        else if (chunk == FourCCBE("BODY"))
        {
            body.resize(chunkSize);
            if (!reader.Read(body.data(), chunkSize))
            {
                return Status::Invalid;
            }
        }
        reader.Seek(chunkStart + chunkSize + (chunkSize & 1));
    }
    if (!haveBmhd || body.empty() || width == 0 || height == 0 || planes == 0 || planes > 8)
    {
        return Status::Unsupported;
    }

    Frame frame;
    Status st = MakeFrame(frame, width, height, planes, paletteCount ? paletteCount : (1u << planes));
    if (st != Status::Ok)
    {
        return st;
    }
    if (paletteCount == 0)
    {
        const UINT colors = 1u << planes;
        for (UINT i = 0; i < colors && i < 256; ++i)
        {
            const BYTE g = static_cast<BYTE>(i * 255 / (colors - 1));
            palette[i] = RGBQUAD{g, g, g, 255};
        }
        paletteCount = colors;
    }

    const UINT rowBytes = ((static_cast<UINT>(width) + 15) / 16) * 2;
    Reader bodyReader(body.data(), body.size());
    std::vector<BYTE> planeRows(static_cast<size_t>(rowBytes) * planes);

    auto readRow = [&](BYTE* dest, UINT bytes) -> bool {
        if (compression == 0)
        {
            return bodyReader.Read(dest, bytes);
        }
        UINT produced = 0;
        while (produced < bytes)
        {
            const signed char n = static_cast<signed char>(bodyReader.U8());
            if (bodyReader.Failed())
            {
                return false;
            }
            if (n >= 0)
            {
                const UINT count = static_cast<UINT>(n) + 1;
                if (produced + count > bytes || !bodyReader.Read(dest + produced, count))
                {
                    return false;
                }
                produced += count;
            }
            else if (n != -128)
            {
                const UINT count = static_cast<UINT>(-n) + 1;
                const BYTE value = bodyReader.U8();
                if (produced + count > bytes)
                {
                    return false;
                }
                memset(dest + produced, value, count);
                produced += count;
            }
        }
        return true;
    };

    for (UINT y = 0; y < height; ++y)
    {
        if (isPbm)
        {
            if (!readRow(planeRows.data(), rowBytes))
            {
                return Status::Invalid;
            }
            for (UINT x = 0; x < width; ++x)
            {
                const BYTE index = planeRows[x];
                SetPixel(frame, x, y, palette[index].rgbBlue, palette[index].rgbGreen, palette[index].rgbRed,
                         (masking == 2 && index == transparentIndex) ? 0 : 255);
            }
            continue;
        }
        for (BYTE plane = 0; plane < planes; ++plane)
        {
            if (!readRow(planeRows.data() + static_cast<size_t>(plane) * rowBytes, rowBytes))
            {
                return Status::Invalid;
            }
        }
        if (masking == 1)
        {
            std::vector<BYTE> mask(rowBytes);
            if (!readRow(mask.data(), rowBytes))
            {
                return Status::Invalid;
            }
        }
        for (UINT x = 0; x < width; ++x)
        {
            UINT index = 0;
            for (BYTE plane = 0; plane < planes; ++plane)
            {
                const BYTE packed = planeRows[static_cast<size_t>(plane) * rowBytes + x / 8];
                if ((packed >> (7 - (x % 8))) & 1)
                {
                    index |= (1u << plane);
                }
            }
            const BYTE a = (masking == 2 && index == transparentIndex) ? 0 : 255;
            SetPixel(frame, x, y, palette[index].rgbBlue, palette[index].rgbGreen, palette[index].rgbRed, a);
        }
    }
    image.format = PVF_LBM;
    image.formatLabel = "IFF";
    image.frames.clear();
    image.frames.push_back(std::move(frame));
    return Status::Ok;
}

Status DecodeIcoCur(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 6)
    {
        return Status::Unsupported;
    }
    reader.Seek(0);
    if (reader.U16LE() != 0)
    {
        return Status::Unsupported;
    }
    const UINT16 type = reader.U16LE();
    const UINT16 count = reader.U16LE();
    if ((type != 1 && type != 2) || count == 0 || count > kMaxFrames)
    {
        return Status::Unsupported;
    }
    struct Entry
    {
        BYTE width;
        BYTE height;
        UINT32 offset;
        UINT32 size;
    };
    std::vector<Entry> entries(count);
    for (UINT16 i = 0; i < count; ++i)
    {
        entries[i].width = reader.U8();
        entries[i].height = reader.U8();
        reader.U8();
        reader.U8();
        reader.U16LE();
        reader.U16LE();
        entries[i].size = reader.U32LE();
        entries[i].offset = reader.U32LE();
    }
    image.frames.clear();
    image.format = type == 2 ? PVF_CUR : PVF_ICO;
    image.formatLabel = type == 2 ? "CUR" : "ICO";
    for (const Entry& entry : entries)
    {
        if (entry.offset >= reader.Size() || entry.size == 0 || entry.offset + entry.size > reader.Size())
        {
            continue;
        }
        Frame frame;
        const UINT width = entry.width == 0 ? 256u : entry.width;
        const UINT height = entry.height == 0 ? 256u : entry.height;
        const Status st = DecodeIcoFrame(reader.Data() + entry.offset, entry.size, width, height, frame);
        if (st == Status::Ok)
        {
            image.frames.push_back(std::move(frame));
        }
    }
    return image.frames.empty() ? Status::Unsupported : Status::Ok;
}

Status DecodeAni(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 12 || !reader.StartsWith("RIFF", 4))
    {
        return Status::Unsupported;
    }
    reader.Seek(8);
    if (reader.U32LE() != FourCC("ACON"))
    {
        return Status::Unsupported;
    }
    image.format = PVF_ANI;
    image.formatLabel = "ANI";
    image.frames.clear();
    DWORD defaultDelay = 100;
    while (reader.Remaining() >= 8 && !reader.Failed() && image.frames.size() < kMaxFrames)
    {
        const UINT32 chunk = reader.U32LE();
        const UINT32 chunkSize = reader.U32LE();
        if (chunkSize > reader.Remaining())
        {
            break;
        }
        const size_t chunkStart = reader.Position();
        if (chunk == FourCC("LIST"))
        {
            const UINT32 listType = reader.U32LE();
            if (listType == FourCC("fram") || listType == FourCC("INFO"))
            {
                while (reader.Position() + 8 <= chunkStart + chunkSize && image.frames.size() < kMaxFrames)
                {
                    const UINT32 inner = reader.U32LE();
                    const UINT32 innerSize = reader.U32LE();
                    const size_t innerStart = reader.Position();
                    if (inner == FourCC("icon") && innerSize <= reader.Remaining())
                    {
                        Reader icon(reader.Data() + innerStart, innerSize);
                        DecodedImage iconImage;
                        if (DecodeIcoCur(icon, iconImage) == Status::Ok && !iconImage.frames.empty())
                        {
                            iconImage.frames[0].delayMs = defaultDelay;
                            image.frames.push_back(std::move(iconImage.frames[0]));
                        }
                    }
                    reader.Seek(innerStart + innerSize + (innerSize & 1));
                }
            }
        }
        else if (chunk == FourCC("icon"))
        {
            Reader icon(reader.Data() + chunkStart, chunkSize);
            DecodedImage iconImage;
            if (DecodeIcoCur(icon, iconImage) == Status::Ok && !iconImage.frames.empty())
            {
                iconImage.frames[0].delayMs = defaultDelay;
                image.frames.push_back(std::move(iconImage.frames[0]));
            }
        }
        else if (chunk == FourCC("anih") && chunkSize >= 36)
        {
            reader.U32LE();
            reader.U32LE();
            reader.U32LE();
            reader.U32LE();
            reader.U32LE();
            reader.U32LE();
            reader.U32LE();
            const UINT32 displayRate = reader.U32LE();
            if (displayRate > 0)
            {
                defaultDelay = displayRate * 1000 / 60;
                if (defaultDelay == 0)
                {
                    defaultDelay = 16;
                }
            }
        }
        reader.Seek(chunkStart + chunkSize + (chunkSize & 1));
    }
    return image.frames.empty() ? Status::Unsupported : Status::Ok;
}

Status DecodeFli(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 128)
    {
        return Status::Unsupported;
    }
    reader.Seek(4);
    const UINT16 magic = reader.U16LE();
    if (magic != 0xAF11 && magic != 0xAF12 && magic != 0xAF43)
    {
        return Status::Unsupported;
    }
    const UINT16 frames = reader.U16LE();
    const UINT16 width = reader.U16LE();
    const UINT16 height = reader.U16LE();
    reader.U16LE(); // depth
    reader.U16LE(); // flags
    const UINT16 speed = reader.U16LE();
    reader.Seek(128);
    if (width == 0 || height == 0 || frames == 0)
    {
        return Status::Unsupported;
    }

    RGBQUAD palette[256]{};
    for (UINT i = 0; i < 256; ++i)
    {
        palette[i] = RGBQUAD{static_cast<BYTE>(i), static_cast<BYTE>(i), static_cast<BYTE>(i), 255};
    }
    std::vector<BYTE> indexed;
    try
    {
        indexed.assign(static_cast<size_t>(width) * height, 0);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    const DWORD delayMs = magic == 0xAF11 ? (speed * 1000 / 70) : speed;
    image.format = PVF_FLI;
    image.formatLabel = magic == 0xAF11 ? "FLI" : "FLC";
    image.frames.clear();

    auto pushFrame = [&]() -> Status {
        if (image.frames.size() >= kMaxFrames)
        {
            return Status::Ok;
        }
        Frame frame;
        Status st = MakeFrame(frame, width, height, 8, 256);
        if (st != Status::Ok)
        {
            return st;
        }
        frame.delayMs = delayMs == 0 ? 80 : delayMs;
        for (UINT y = 0; y < height; ++y)
        {
            for (UINT x = 0; x < width; ++x)
            {
                const BYTE index = indexed[static_cast<size_t>(y) * width + x];
                SetPixel(frame, x, y, palette[index].rgbBlue, palette[index].rgbGreen, palette[index].rgbRed, 255);
            }
        }
        image.frames.push_back(std::move(frame));
        return Status::Ok;
    };

    const UINT frameLimit = std::min<UINT>(frames, kMaxFrames);
    for (UINT frameIndex = 0; frameIndex < frameLimit && reader.Remaining() >= 16; ++frameIndex)
    {
        const size_t frameStart = reader.Position();
        const UINT32 frameSize = reader.U32LE();
        const UINT16 frameMagic = reader.U16LE();
        const UINT16 chunks = reader.U16LE();
        reader.Skip(8);
        if (frameMagic != 0xF1FA && frameMagic != 0xF100)
        {
            if (frameSize < 16)
            {
                break;
            }
            reader.Seek(frameStart + frameSize);
            continue;
        }
        for (UINT16 chunk = 0; chunk < chunks && reader.Position() < frameStart + frameSize; ++chunk)
        {
            const size_t chunkStart = reader.Position();
            const UINT32 chunkSize = reader.U32LE();
            const UINT16 chunkType = reader.U16LE();
            if (chunkType == 4 || chunkType == 11) // 256-level or 64-level palette
            {
                UINT16 packets = reader.U16LE();
                UINT index = 0;
                for (UINT16 p = 0; p < packets && index < 256; ++p)
                {
                    index += reader.U8();
                    UINT count = reader.U8();
                    if (count == 0)
                    {
                        count = 256;
                    }
                    for (UINT i = 0; i < count && index < 256; ++i, ++index)
                    {
                        BYTE r = reader.U8();
                        BYTE g = reader.U8();
                        BYTE b = reader.U8();
                        if (chunkType == 11)
                        {
                            r = static_cast<BYTE>(r * 4);
                            g = static_cast<BYTE>(g * 4);
                            b = static_cast<BYTE>(b * 4);
                        }
                        palette[index] = RGBQUAD{b, g, r, 255};
                    }
                }
            }
            else if (chunkType == 15) // BRUN
            {
                for (UINT y = 0; y < height; ++y)
                {
                    reader.U8(); // packet count, unused for FLI
                    UINT x = 0;
                    while (x < width)
                    {
                        const signed char n = static_cast<signed char>(reader.U8());
                        if (n >= 0)
                        {
                            const UINT count = static_cast<UINT>(n);
                            const BYTE value = reader.U8();
                            if (x + count > width)
                            {
                                return Status::Invalid;
                            }
                            memset(indexed.data() + static_cast<size_t>(y) * width + x, value, count);
                            x += count;
                        }
                        else
                        {
                            const UINT count = static_cast<UINT>(-n);
                            if (x + count > width || !reader.Read(indexed.data() + static_cast<size_t>(y) * width + x, count))
                            {
                                return Status::Invalid;
                            }
                            x += count;
                        }
                    }
                }
            }
            else if (chunkType == 12) // LC
            {
                const UINT16 first = reader.U16LE();
                const UINT16 count = reader.U16LE();
                for (UINT i = 0; i < count; ++i)
                {
                    const UINT y = first + i;
                    UINT packets = reader.U8();
                    UINT x = 0;
                    for (UINT p = 0; p < packets; ++p)
                    {
                        x += reader.U8();
                        const signed char n = static_cast<signed char>(reader.U8());
                        if (n >= 0)
                        {
                            const UINT run = static_cast<UINT>(n);
                            if (x + run > width || !reader.Read(indexed.data() + static_cast<size_t>(y) * width + x, run))
                            {
                                return Status::Invalid;
                            }
                            x += run;
                        }
                        else
                        {
                            const UINT run = static_cast<UINT>(-n);
                            const BYTE value = reader.U8();
                            if (x + run > width)
                            {
                                return Status::Invalid;
                            }
                            memset(indexed.data() + static_cast<size_t>(y) * width + x, value, run);
                            x += run;
                        }
                    }
                }
            }
            else if (chunkType == 16) // COPY
            {
                if (!reader.Read(indexed.data(), indexed.size()))
                {
                    return Status::Invalid;
                }
            }
            else if (chunkType == 13) // BLACK
            {
                memset(indexed.data(), 0, indexed.size());
            }
            reader.Seek(chunkStart + std::max<UINT32>(chunkSize, 6));
        }
        if (pushFrame() != Status::Ok)
        {
            return Status::OutOfMemory;
        }
        reader.Seek(frameStart + std::max<UINT32>(frameSize, 16));
    }
    return image.frames.empty() ? Status::Invalid : Status::Ok;
}

} // namespace Detail
} // namespace PictView::Native
