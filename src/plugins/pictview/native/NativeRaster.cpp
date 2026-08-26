// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace PictView::Native
{
namespace Detail
{
namespace
{

void Expand16Bit555(UINT16 packed, BYTE& b, BYTE& g, BYTE& r)
{
    r = static_cast<BYTE>(((packed >> 10) & 31) * 255 / 31);
    g = static_cast<BYTE>(((packed >> 5) & 31) * 255 / 31);
    b = static_cast<BYTE>((packed & 31) * 255 / 31);
}

void Expand16Bit565(UINT16 packed, BYTE& b, BYTE& g, BYTE& r)
{
    r = static_cast<BYTE>(((packed >> 11) & 31) * 255 / 31);
    g = static_cast<BYTE>(((packed >> 5) & 63) * 255 / 63);
    b = static_cast<BYTE>((packed & 31) * 255 / 31);
}

int SkipPnmWhitespaceAndComments(Reader& reader)
{
    for (;;)
    {
        if (reader.Remaining() == 0)
        {
            return -1;
        }
        const BYTE ch = reader.U8();
        if (ch == '#')
        {
            while (reader.Remaining() > 0)
            {
                const BYTE next = reader.U8();
                if (next == '\n' || next == '\r')
                {
                    break;
                }
            }
            continue;
        }
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            continue;
        }
        return ch;
    }
}

bool ReadPnmToken(Reader& reader, unsigned int& value)
{
    int ch = SkipPnmWhitespaceAndComments(reader);
    if (ch < 0 || ch < '0' || ch > '9')
    {
        return false;
    }
    unsigned int parsed = static_cast<unsigned int>(ch - '0');
    while (reader.Remaining() > 0)
    {
        const BYTE* peek = reader.Peek(1);
        if (peek == nullptr)
        {
            break;
        }
        if (*peek < '0' || *peek > '9')
        {
            break;
        }
        parsed = parsed * 10u + static_cast<unsigned int>(reader.U8() - '0');
        if (parsed > kMaxDimension)
        {
            return false;
        }
    }
    value = parsed;
    return !reader.Failed();
}

} // namespace

Status DecodeTga(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 18)
    {
        return Status::Unsupported;
    }
    reader.Seek(0);
    const BYTE idLength = reader.U8();
    const BYTE colorMapType = reader.U8();
    const BYTE imageType = reader.U8();
    const UINT16 cmFirst = reader.U16LE();
    const UINT16 cmLength = reader.U16LE();
    const BYTE cmEntrySize = reader.U8();
    reader.U16LE(); // x origin
    reader.U16LE(); // y origin
    const UINT16 width = reader.U16LE();
    const UINT16 height = reader.U16LE();
    const BYTE bpp = reader.U8();
    const BYTE descriptor = reader.U8();
    if (reader.Failed())
    {
        return Status::Unsupported;
    }

    const bool rle = imageType == 9 || imageType == 10 || imageType == 11;
    const BYTE baseType = rle ? static_cast<BYTE>(imageType - 8) : imageType;
    if (baseType != 1 && baseType != 2 && baseType != 3)
    {
        return Status::Unsupported;
    }
    if (width == 0 || height == 0)
    {
        return Status::Unsupported;
    }
    if (baseType == 2 && bpp != 16 && bpp != 24 && bpp != 32)
    {
        return Status::Unsupported;
    }
    if (baseType == 3 && bpp != 8 && bpp != 16)
    {
        return Status::Unsupported;
    }
    if (baseType == 1 && (colorMapType != 1 || bpp != 8 || (cmEntrySize != 16 && cmEntrySize != 24 && cmEntrySize != 32)))
    {
        return Status::Unsupported;
    }

    reader.Skip(idLength);
    std::vector<RGBQUAD> palette;
    if (colorMapType == 1 && cmLength > 0)
    {
        if (cmLength > 256)
        {
            return Status::Unsupported;
        }
        palette.resize(cmLength);
        for (UINT16 i = 0; i < cmLength; ++i)
        {
            BYTE b = 0;
            BYTE g = 0;
            BYTE r = 0;
            BYTE a = 255;
            if (cmEntrySize == 16)
            {
                Expand16Bit555(reader.U16LE(), b, g, r);
            }
            else if (cmEntrySize == 24)
            {
                b = reader.U8();
                g = reader.U8();
                r = reader.U8();
            }
            else
            {
                b = reader.U8();
                g = reader.U8();
                r = reader.U8();
                a = reader.U8();
            }
            palette[i] = RGBQUAD{b, g, r, a};
        }
    }
    if (reader.Failed())
    {
        return Status::Invalid;
    }

    Frame frame;
    const UINT bitDepth = bpp;
    const DWORD colors = bpp <= 8 ? 256u : (bpp == 16 ? PV_COLOR_HC16 : (bpp == 24 ? PV_COLOR_TC24 : PV_COLOR_TC32));
    Status st = MakeFrame(frame, width, height, bitDepth, colors);
    if (st != Status::Ok)
    {
        return st;
    }

    const bool topOrigin = (descriptor & 0x20) != 0;
    const UINT pixelBytes = (bpp + 7) / 8;
    auto decodePixel = [&](BYTE* dest) -> bool {
        BYTE b = 0;
        BYTE g = 0;
        BYTE r = 0;
        BYTE a = 255;
        if (baseType == 1)
        {
            const BYTE index = reader.U8();
            const UINT palIndex = static_cast<UINT>(index) - cmFirst;
            if (palIndex < palette.size())
            {
                b = palette[palIndex].rgbBlue;
                g = palette[palIndex].rgbGreen;
                r = palette[palIndex].rgbRed;
                a = palette[palIndex].rgbReserved;
            }
        }
        else if (baseType == 3)
        {
            if (bpp == 8)
            {
                b = g = r = reader.U8();
            }
            else
            {
                const UINT16 gray = reader.U16LE();
                b = g = r = static_cast<BYTE>(gray >> 8);
            }
        }
        else if (bpp == 16)
        {
            Expand16Bit555(reader.U16LE(), b, g, r);
        }
        else if (bpp == 24)
        {
            b = reader.U8();
            g = reader.U8();
            r = reader.U8();
        }
        else
        {
            b = reader.U8();
            g = reader.U8();
            r = reader.U8();
            a = reader.U8();
        }
        dest[0] = b;
        dest[1] = g;
        dest[2] = r;
        dest[3] = a;
        return !reader.Failed();
    };

    const UINT pixelCount = static_cast<UINT>(width) * height;
    std::vector<BYTE> linear;
    try
    {
        linear.resize(static_cast<size_t>(pixelCount) * 4);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    if (!rle)
    {
        for (UINT i = 0; i < pixelCount; ++i)
        {
            if (!decodePixel(linear.data() + static_cast<size_t>(i) * 4))
            {
                return Status::Invalid;
            }
        }
    }
    else
    {
        UINT decoded = 0;
        while (decoded < pixelCount)
        {
            const BYTE packet = reader.U8();
            const UINT count = (packet & 0x7F) + 1;
            if (decoded + count > pixelCount)
            {
                return Status::Invalid;
            }
            if (packet & 0x80)
            {
                BYTE pixel[4];
                if (!decodePixel(pixel))
                {
                    return Status::Invalid;
                }
                for (UINT i = 0; i < count; ++i)
                {
                    memcpy(linear.data() + static_cast<size_t>(decoded + i) * 4, pixel, 4);
                }
            }
            else
            {
                for (UINT i = 0; i < count; ++i)
                {
                    if (!decodePixel(linear.data() + static_cast<size_t>(decoded + i) * 4))
                    {
                        return Status::Invalid;
                    }
                }
            }
            decoded += count;
            (void)pixelBytes;
        }
    }

    for (UINT y = 0; y < height; ++y)
    {
        const UINT srcY = topOrigin ? y : (height - 1 - y);
        memcpy(Row(frame, y), linear.data() + static_cast<size_t>(srcY) * width * 4, static_cast<size_t>(width) * 4);
    }
    NoteAlpha(frame);
    return FinishSingle(image, std::move(frame), PVF_TGA, "TGA");
}

Status DecodePcx(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 128)
    {
        return Status::Unsupported;
    }
    reader.Seek(0);
    if (reader.U8() != 0x0A)
    {
        return Status::Unsupported;
    }
    const BYTE version = reader.U8();
    const BYTE encoding = reader.U8();
    const BYTE bpp = reader.U8();
    const UINT16 xMin = reader.U16LE();
    const UINT16 yMin = reader.U16LE();
    const UINT16 xMax = reader.U16LE();
    const UINT16 yMax = reader.U16LE();
    reader.U16LE();
    reader.U16LE();
    BYTE headerPalette[48];
    if (!reader.Read(headerPalette, 48))
    {
        return Status::Unsupported;
    }
    reader.U8();
    const BYTE planes = reader.U8();
    const UINT16 bytesPerLine = reader.U16LE();
    reader.U16LE(); // palette info
    reader.Seek(128);
    if (reader.Failed() || encoding > 1 || bpp == 0 || planes == 0)
    {
        return Status::Unsupported;
    }
    if (xMax < xMin || yMax < yMin)
    {
        return Status::Unsupported;
    }
    const UINT width = static_cast<UINT>(xMax) - xMin + 1;
    const UINT height = static_cast<UINT>(yMax) - yMin + 1;
    if (bytesPerLine == 0)
    {
        return Status::Unsupported;
    }
    const UINT bits = static_cast<UINT>(bpp) * planes;
    if (bits != 1 && bits != 2 && bits != 4 && bits != 8 && bits != 24)
    {
        return Status::Unsupported;
    }

    std::vector<BYTE> planeRows;
    try
    {
        planeRows.resize(static_cast<size_t>(bytesPerLine) * planes);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    Frame frame;
    const DWORD colors = bits <= 8 ? (1u << bits) : PV_COLOR_TC24;
    Status st = MakeFrame(frame, width, height, bits, colors);
    if (st != Status::Ok)
    {
        return st;
    }

    auto readScan = [&]() -> bool {
        size_t produced = 0;
        const size_t needed = planeRows.size();
        while (produced < needed)
        {
            const BYTE value = reader.U8();
            if (reader.Failed())
            {
                return false;
            }
            if (encoding == 1 && (value & 0xC0) == 0xC0)
            {
                const UINT run = value & 0x3F;
                const BYTE data = reader.U8();
                if (reader.Failed() || produced + run > needed)
                {
                    return false;
                }
                memset(planeRows.data() + produced, data, run);
                produced += run;
            }
            else
            {
                planeRows[produced++] = value;
            }
        }
        return true;
    };

    RGBQUAD palette[256]{};
    if (bits <= 8)
    {
        for (UINT i = 0; i < 16; ++i)
        {
            palette[i].rgbRed = headerPalette[i * 3];
            palette[i].rgbGreen = headerPalette[i * 3 + 1];
            palette[i].rgbBlue = headerPalette[i * 3 + 2];
            palette[i].rgbReserved = 255;
        }
        if (bits == 8 && reader.Size() >= 769)
        {
            const size_t palPos = reader.Size() - 769;
            if (reader.Data()[palPos] == 0x0C)
            {
                for (UINT i = 0; i < 256; ++i)
                {
                    palette[i].rgbRed = reader.Data()[palPos + 1 + i * 3];
                    palette[i].rgbGreen = reader.Data()[palPos + 2 + i * 3];
                    palette[i].rgbBlue = reader.Data()[palPos + 3 + i * 3];
                    palette[i].rgbReserved = 255;
                }
            }
        }
        if (bits == 1 && palette[0].rgbRed == 0 && palette[0].rgbGreen == 0 && palette[0].rgbBlue == 0 &&
            palette[1].rgbRed == 0 && palette[1].rgbGreen == 0 && palette[1].rgbBlue == 0)
        {
            palette[1].rgbRed = palette[1].rgbGreen = palette[1].rgbBlue = 255;
        }
    }

    for (UINT y = 0; y < height; ++y)
    {
        if (!readScan())
        {
            return Status::Invalid;
        }
        BYTE* dest = Row(frame, y);
        if (bits == 24 && planes == 3)
        {
            for (UINT x = 0; x < width; ++x)
            {
                dest[x * 4 + 2] = planeRows[x];
                dest[x * 4 + 1] = planeRows[bytesPerLine + x];
                dest[x * 4 + 0] = planeRows[static_cast<size_t>(bytesPerLine) * 2 + x];
                dest[x * 4 + 3] = 255;
            }
        }
        else if (planes == 1)
        {
            for (UINT x = 0; x < width; ++x)
            {
                UINT index = 0;
                if (bpp == 8)
                {
                    index = planeRows[x];
                }
                else if (bpp == 4)
                {
                    const BYTE packed = planeRows[x / 2];
                    index = (x & 1) ? (packed & 0x0F) : (packed >> 4);
                }
                else if (bpp == 2)
                {
                    const BYTE packed = planeRows[x / 4];
                    index = (packed >> (6 - (x % 4) * 2)) & 0x03;
                }
                else
                {
                    const BYTE packed = planeRows[x / 8];
                    index = (packed >> (7 - (x % 8))) & 1;
                }
                dest[x * 4 + 0] = palette[index].rgbBlue;
                dest[x * 4 + 1] = palette[index].rgbGreen;
                dest[x * 4 + 2] = palette[index].rgbRed;
                dest[x * 4 + 3] = 255;
            }
        }
        else
        {
            for (UINT x = 0; x < width; ++x)
            {
                BYTE index = 0;
                for (BYTE plane = 0; plane < planes && plane < 8; ++plane)
                {
                    const BYTE packed = planeRows[static_cast<size_t>(plane) * bytesPerLine + x / 8];
                    if ((packed >> (7 - (x % 8))) & 1)
                    {
                        index = static_cast<BYTE>(index | (1u << plane));
                    }
                }
                dest[x * 4 + 0] = palette[index].rgbBlue;
                dest[x * 4 + 1] = palette[index].rgbGreen;
                dest[x * 4 + 2] = palette[index].rgbRed;
                dest[x * 4 + 3] = 255;
            }
        }
    }

    (void)version;
    return FinishSingle(image, std::move(frame), PVF_PCX, "PCX");
}

Status DecodeDcx(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 8 || reader.U32LE() != 0x3ADE68B1)
    {
        reader.Seek(0);
        return Status::Unsupported;
    }
    UINT32 offsets[1024];
    if (!reader.Read(offsets, sizeof(offsets)))
    {
        return Status::Invalid;
    }
    DecodedImage combined;
    combined.format = PVF_DCX;
    combined.formatLabel = "DCX";
    for (UINT i = 0; i < 1023 && i < kMaxFrames; ++i)
    {
        if (offsets[i] == 0)
        {
            break;
        }
        if (offsets[i] >= reader.Size())
        {
            return Status::Invalid;
        }
        Reader page(reader.Data() + offsets[i], reader.Size() - offsets[i]);
        DecodedImage pageImage;
        const Status st = DecodePcx(page, pageImage);
        if (st != Status::Ok || pageImage.frames.empty())
        {
            return st == Status::Ok ? Status::Invalid : st;
        }
        combined.frames.push_back(std::move(pageImage.frames[0]));
    }
    if (combined.frames.empty())
    {
        return Status::Invalid;
    }
    image = std::move(combined);
    return Status::Ok;
}

Status DecodePnm(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 3 || reader.U8() != 'P')
    {
        reader.Seek(0);
        return Status::Unsupported;
    }
    const BYTE kind = reader.U8();
    if (kind < '1' || kind > '6')
    {
        return Status::Unsupported;
    }
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int maxVal = 1;
    if (!ReadPnmToken(reader, width) || !ReadPnmToken(reader, height))
    {
        return Status::Invalid;
    }
    if (kind != '1' && kind != '4')
    {
        if (!ReadPnmToken(reader, maxVal) || maxVal == 0 || maxVal > 255)
        {
            return Status::Invalid;
        }
    }
    Frame frame;
    const bool gray = kind == '1' || kind == '2' || kind == '4' || kind == '5';
    Status st = MakeFrame(frame, width, height, gray ? 8 : 24, gray ? 256 : PV_COLOR_TC24);
    if (st != Status::Ok)
    {
        return st;
    }

    auto scale = [maxVal](unsigned int value) -> BYTE {
        if (maxVal == 255)
        {
            return static_cast<BYTE>(value);
        }
        return static_cast<BYTE>((value * 255u + maxVal / 2u) / maxVal);
    };

    if (kind == '1' || kind == '2' || kind == '3')
    {
        for (UINT y = 0; y < height; ++y)
        {
            for (UINT x = 0; x < width; ++x)
            {
                unsigned int v1 = 0;
                if (kind == '1')
                {
                    if (!ReadPnmToken(reader, v1))
                    {
                        return Status::Invalid;
                    }
                    const BYTE g = v1 ? 0 : 255;
                    SetPixel(frame, x, y, g, g, g, 255);
                }
                else if (kind == '2')
                {
                    if (!ReadPnmToken(reader, v1))
                    {
                        return Status::Invalid;
                    }
                    const BYTE g = scale(v1);
                    SetPixel(frame, x, y, g, g, g, 255);
                }
                else
                {
                    unsigned int v2 = 0;
                    unsigned int v3 = 0;
                    if (!ReadPnmToken(reader, v1) || !ReadPnmToken(reader, v2) || !ReadPnmToken(reader, v3))
                    {
                        return Status::Invalid;
                    }
                    SetPixel(frame, x, y, scale(v3), scale(v2), scale(v1), 255);
                }
            }
        }
    }
    else
    {
        if (kind == '4')
        {
            const UINT packedStride = (width + 7) / 8;
            for (UINT y = 0; y < height; ++y)
            {
                for (UINT x = 0; x < width; ++x)
                {
                    if (x % 8 == 0)
                    {
                        if (reader.Remaining() < 1)
                        {
                            return Status::Invalid;
                        }
                    }
                    const BYTE packed = reader.Data()[reader.Position() + x / 8];
                    const BYTE bit = (packed >> (7 - (x % 8))) & 1;
                    const BYTE g = bit ? 0 : 255;
                    SetPixel(frame, x, y, g, g, g, 255);
                }
                reader.Skip(packedStride);
            }
        }
        else if (kind == '5')
        {
            for (UINT y = 0; y < height; ++y)
            {
                for (UINT x = 0; x < width; ++x)
                {
                    const BYTE g = scale(reader.U8());
                    SetPixel(frame, x, y, g, g, g, 255);
                }
            }
        }
        else
        {
            for (UINT y = 0; y < height; ++y)
            {
                for (UINT x = 0; x < width; ++x)
                {
                    const BYTE r = scale(reader.U8());
                    const BYTE g = scale(reader.U8());
                    const BYTE b = scale(reader.U8());
                    SetPixel(frame, x, y, b, g, r, 255);
                }
            }
        }
    }
    if (reader.Failed())
    {
        return Status::Invalid;
    }
    return FinishSingle(image, std::move(frame), PVF_PNM, "PNM");
}

Status DecodeRas(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 32 || reader.U32BE() != 0x59A66A95)
    {
        reader.Seek(0);
        return Status::Unsupported;
    }
    const UINT32 width = reader.U32BE();
    const UINT32 height = reader.U32BE();
    const UINT32 depth = reader.U32BE();
    reader.U32BE(); // length
    const UINT32 type = reader.U32BE();
    const UINT32 mapType = reader.U32BE();
    const UINT32 mapLength = reader.U32BE();
    if (depth != 1 && depth != 8 && depth != 24 && depth != 32)
    {
        return Status::Unsupported;
    }
    std::vector<BYTE> colorMap;
    if (mapLength > 0)
    {
        if (mapLength > 3 * 256)
        {
            return Status::Unsupported;
        }
        colorMap.resize(mapLength);
        if (!reader.Read(colorMap.data(), mapLength))
        {
            return Status::Invalid;
        }
    }
    Frame frame;
    Status st = MakeFrame(frame, width, height, depth, depth <= 8 ? (1u << depth) : (depth == 24 ? PV_COLOR_TC24 : PV_COLOR_TC32));
    if (st != Status::Ok)
    {
        return st;
    }
    const UINT32 stride = ((width * depth + 15) / 16) * 2;
    std::vector<BYTE> raw;
    if (type == 2)
    {
        try
        {
            raw.resize(static_cast<size_t>(stride) * height);
        }
        catch (const std::bad_alloc&)
        {
            return Status::OutOfMemory;
        }
        size_t produced = 0;
        while (produced < raw.size())
        {
            const BYTE value = reader.U8();
            if (reader.Failed())
            {
                return Status::Invalid;
            }
            if (value == 0x80)
            {
                const BYTE count = reader.U8();
                if (count == 0)
                {
                    raw[produced++] = 0x80;
                    continue;
                }
                const BYTE data = reader.U8();
                const size_t run = static_cast<size_t>(count) + 1;
                if (produced + run > raw.size())
                {
                    return Status::Invalid;
                }
                memset(raw.data() + produced, data, run);
                produced += run;
            }
            else
            {
                raw[produced++] = value;
            }
        }
    }
    else
    {
        raw.resize(static_cast<size_t>(stride) * height);
        if (!reader.Read(raw.data(), raw.size()))
        {
            return Status::Invalid;
        }
    }

    for (UINT y = 0; y < height; ++y)
    {
        const BYTE* src = raw.data() + static_cast<size_t>(y) * stride;
        for (UINT x = 0; x < width; ++x)
        {
            BYTE b = 0;
            BYTE g = 0;
            BYTE r = 0;
            BYTE a = 255;
            if (depth == 1)
            {
                const BYTE bit = (src[x / 8] >> (7 - (x % 8))) & 1;
                b = g = r = bit ? 0 : 255;
            }
            else if (depth == 8)
            {
                const BYTE index = src[x];
                if (mapType == 1 && mapLength >= 256 * 3 && index < 256)
                {
                    r = colorMap[index];
                    g = colorMap[256 + index];
                    b = colorMap[512 + index];
                }
                else
                {
                    b = g = r = index;
                }
            }
            else if (depth == 24)
            {
                r = src[x * 3];
                g = src[x * 3 + 1];
                b = src[x * 3 + 2];
                if (type == 0)
                {
                    std::swap(r, b);
                }
            }
            else
            {
                a = src[x * 4];
                r = src[x * 4 + 1];
                g = src[x * 4 + 2];
                b = src[x * 4 + 3];
            }
            SetPixel(frame, x, y, b, g, r, a);
        }
    }
    (void)mapType;
    return FinishSingle(image, std::move(frame), PVF_RAS, "RAS");
}

Status DecodeSgi(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 512 || reader.U16BE() != 0x01DA)
    {
        reader.Seek(0);
        return Status::Unsupported;
    }
    const BYTE storage = reader.U8();
    const BYTE bytesPerChannel = reader.U8();
    const UINT16 dimension = reader.U16BE();
    const UINT16 width = reader.U16BE();
    const UINT16 height = reader.U16BE();
    const UINT16 channels = reader.U16BE();
    reader.Seek(512);
    if (bytesPerChannel != 1 || width == 0 || height == 0 || channels == 0 || channels > 4)
    {
        return Status::Unsupported;
    }
    (void)dimension;
    Frame frame;
    Status st = MakeFrame(frame, width, height, static_cast<UINT>(channels) * 8,
                          channels == 1 ? 256 : (channels == 3 ? PV_COLOR_TC24 : PV_COLOR_TC32));
    if (st != Status::Ok)
    {
        return st;
    }

    std::vector<BYTE> channelData;
    try
    {
        channelData.resize(static_cast<size_t>(width) * height * channels);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    if (storage == 0)
    {
        if (!reader.Read(channelData.data(), channelData.size()))
        {
            return Status::Invalid;
        }
    }
    else if (storage == 1)
    {
        const UINT tabLen = static_cast<UINT>(height) * channels;
        std::vector<UINT32> startTab(tabLen);
        std::vector<UINT32> lengthTab(tabLen);
        for (UINT i = 0; i < tabLen; ++i)
        {
            startTab[i] = reader.U32BE();
        }
        for (UINT i = 0; i < tabLen; ++i)
        {
            lengthTab[i] = reader.U32BE();
        }
        if (reader.Failed())
        {
            return Status::Invalid;
        }
        for (UINT channel = 0; channel < channels; ++channel)
        {
            for (UINT y = 0; y < height; ++y)
            {
                const UINT index = channel * height + y;
                if (startTab[index] >= reader.Size())
                {
                    return Status::Invalid;
                }
                Reader rowReader(reader.Data() + startTab[index],
                                 std::min<size_t>(lengthTab[index], reader.Size() - startTab[index]));
                UINT x = 0;
                BYTE* dest = channelData.data() + (static_cast<size_t>(channel) * height + y) * width;
                while (x < width)
                {
                    const BYTE pixel = rowReader.U8();
                    UINT count = pixel & 0x7F;
                    if (count == 0)
                    {
                        break;
                    }
                    if (x + count > width)
                    {
                        return Status::Invalid;
                    }
                    if (pixel & 0x80)
                    {
                        for (UINT i = 0; i < count; ++i)
                        {
                            dest[x++] = rowReader.U8();
                        }
                    }
                    else
                    {
                        const BYTE value = rowReader.U8();
                        memset(dest + x, value, count);
                        x += count;
                    }
                    if (rowReader.Failed())
                    {
                        return Status::Invalid;
                    }
                }
            }
        }
    }
    else
    {
        return Status::Unsupported;
    }

    auto channelAt = [&](UINT channel, UINT x, UINT y) -> BYTE {
        const UINT srcY = height - 1 - y;
        return channelData[(static_cast<size_t>(channel) * height + srcY) * width + x];
    };
    for (UINT y = 0; y < height; ++y)
    {
        for (UINT x = 0; x < width; ++x)
        {
            BYTE r = 0;
            BYTE g = 0;
            BYTE b = 0;
            BYTE a = 255;
            if (channels == 1)
            {
                r = g = b = channelAt(0, x, y);
            }
            else
            {
                r = channelAt(0, x, y);
                g = channels > 1 ? channelAt(1, x, y) : r;
                b = channels > 2 ? channelAt(2, x, y) : r;
                a = channels > 3 ? channelAt(3, x, y) : 255;
            }
            SetPixel(frame, x, y, b, g, r, a);
        }
    }
    const char* label = channels == 1 ? "SGI" : (channels == 3 ? "RGB" : "SGI");
    return FinishSingle(image, std::move(frame), PVF_SGI, label);
}

Status DecodeWbmp(Reader& reader, DecodedImage& image)
{
    reader.Seek(0);
    const BYTE type = reader.U8();
    const BYTE header = reader.U8();
    if (type != 0 || header != 0)
    {
        return Status::Unsupported;
    }
    auto readMulti = [&](UINT& value) -> bool {
        value = 0;
        for (int i = 0; i < 5; ++i)
        {
            const BYTE b = reader.U8();
            value = (value << 7) | (b & 0x7F);
            if ((b & 0x80) == 0)
            {
                return !reader.Failed() && value > 0 && value <= kMaxDimension;
            }
        }
        return false;
    };
    UINT width = 0;
    UINT height = 0;
    if (!readMulti(width) || !readMulti(height))
    {
        return Status::Unsupported;
    }
    Frame frame;
    Status st = MakeFrame(frame, width, height, 1, 2);
    if (st != Status::Ok)
    {
        return st;
    }
    const UINT stride = (width + 7) / 8;
    for (UINT y = 0; y < height; ++y)
    {
        for (UINT x = 0; x < width; ++x)
        {
            if (x % 8 == 0 && reader.Remaining() < 1)
            {
                return Status::Invalid;
            }
            const BYTE packed = reader.Data()[reader.Position() + x / 8];
            const BYTE bit = (packed >> (7 - (x % 8))) & 1;
            const BYTE g = bit ? 255 : 0;
            SetPixel(frame, x, y, g, g, g, 255);
        }
        reader.Skip(stride);
    }
    return FinishSingle(image, std::move(frame), PVF_WBMP, "WBMP");
}

Status DecodePsd(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 26 || !reader.StartsWith("8BPS", 4))
    {
        return Status::Unsupported;
    }
    reader.Seek(4);
    const UINT16 version = reader.U16BE();
    reader.Skip(6);
    const UINT16 channels = reader.U16BE();
    const UINT32 height = reader.U32BE();
    const UINT32 width = reader.U32BE();
    const UINT16 depth = reader.U16BE();
    const UINT16 colorMode = reader.U16BE();
    if (reader.Failed() || (version != 1 && version != 2) || depth != 8 || channels == 0 || channels > 4)
    {
        return Status::Unsupported;
    }
    if (colorMode != 1 && colorMode != 3) // grayscale or RGB
    {
        return Status::Unsupported;
    }
    const UINT32 colorModeLen = reader.U32BE();
    reader.Skip(colorModeLen);
    const UINT32 resourceLen = reader.U32BE();
    reader.Skip(resourceLen);
    const UINT32 layerLen = version == 1 ? reader.U32BE() : 0;
    if (version == 2)
    {
        const UINT64 layerLen64 = (static_cast<UINT64>(reader.U32BE()) << 32) | reader.U32BE();
        if (layerLen64 > reader.Remaining())
        {
            return Status::Invalid;
        }
        reader.Skip(static_cast<size_t>(layerLen64));
    }
    else
    {
        reader.Skip(layerLen);
    }
    const UINT16 compression = reader.U16BE();
    if (reader.Failed())
    {
        return Status::Invalid;
    }

    Frame frame;
    Status st = MakeFrame(frame, width, height, depth * channels,
                          colorMode == 1 ? 256 : (channels >= 4 ? PV_COLOR_TC32 : PV_COLOR_TC24));
    if (st != Status::Ok)
    {
        return st;
    }

    std::vector<BYTE> channelData;
    try
    {
        channelData.resize(static_cast<size_t>(width) * height * channels);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    if (compression == 0)
    {
        if (!reader.Read(channelData.data(), channelData.size()))
        {
            return Status::Invalid;
        }
    }
    else if (compression == 1)
    {
        std::vector<UINT16> rowSizes(static_cast<size_t>(height) * channels);
        for (auto& rowSize : rowSizes)
        {
            rowSize = reader.U16BE();
        }
        size_t dest = 0;
        for (UINT channel = 0; channel < channels; ++channel)
        {
            for (UINT y = 0; y < height; ++y)
            {
                UINT decoded = 0;
                while (decoded < width)
                {
                    const signed char n = static_cast<signed char>(reader.U8());
                    if (reader.Failed())
                    {
                        return Status::Invalid;
                    }
                    if (n >= 0)
                    {
                        const UINT count = static_cast<UINT>(n) + 1;
                        if (decoded + count > width || dest + count > channelData.size())
                        {
                            return Status::Invalid;
                        }
                        if (!reader.Read(channelData.data() + dest, count))
                        {
                            return Status::Invalid;
                        }
                        dest += count;
                        decoded += count;
                    }
                    else if (n != -128)
                    {
                        const UINT count = static_cast<UINT>(-n) + 1;
                        const BYTE value = reader.U8();
                        if (decoded + count > width || dest + count > channelData.size())
                        {
                            return Status::Invalid;
                        }
                        memset(channelData.data() + dest, value, count);
                        dest += count;
                        decoded += count;
                    }
                }
            }
        }
        (void)rowSizes;
    }
    else
    {
        return Status::Unsupported;
    }

    const size_t plane = static_cast<size_t>(width) * height;
    for (UINT y = 0; y < height; ++y)
    {
        for (UINT x = 0; x < width; ++x)
        {
            const size_t pixel = static_cast<size_t>(y) * width + x;
            BYTE r = channelData[pixel];
            BYTE g = r;
            BYTE b = r;
            BYTE a = 255;
            if (channels >= 3)
            {
                g = channelData[plane + pixel];
                b = channelData[plane * 2 + pixel];
            }
            if (channels >= 4)
            {
                a = channelData[plane * 3 + pixel];
            }
            else if (channels == 2)
            {
                a = channelData[plane + pixel];
            }
            SetPixel(frame, x, y, b, g, r, a);
        }
    }
    return FinishSingle(image, std::move(frame), PVF_PSD, "PSD");
}

namespace
{

void DecodeBc1Block(const BYTE block[8], BYTE dest[16 * 4], bool punchThrough)
{
    const UINT16 c0 = static_cast<UINT16>(block[0] | (block[1] << 8));
    const UINT16 c1 = static_cast<UINT16>(block[2] | (block[3] << 8));
    BYTE colors[4][4];
    Expand16Bit565(c0, colors[0][0], colors[0][1], colors[0][2]);
    colors[0][3] = 255;
    Expand16Bit565(c1, colors[1][0], colors[1][1], colors[1][2]);
    colors[1][3] = 255;
    if (c0 > c1 || !punchThrough)
    {
        for (int i = 0; i < 3; ++i)
        {
            colors[2][i] = static_cast<BYTE>((2 * colors[0][i] + colors[1][i]) / 3);
            colors[3][i] = static_cast<BYTE>((colors[0][i] + 2 * colors[1][i]) / 3);
        }
        colors[2][3] = colors[3][3] = 255;
    }
    else
    {
        for (int i = 0; i < 3; ++i)
        {
            colors[2][i] = static_cast<BYTE>((colors[0][i] + colors[1][i]) / 2);
            colors[3][i] = 0;
        }
        colors[2][3] = 255;
        colors[3][3] = 0;
    }
    UINT32 lookup = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);
    for (int i = 0; i < 16; ++i)
    {
        const UINT index = lookup & 3;
        lookup >>= 2;
        memcpy(dest + i * 4, colors[index], 4);
    }
}

void DecodeBc4Values(const BYTE block[8], BYTE dest[16])
{
    BYTE values[8];
    values[0] = block[0];
    values[1] = block[1];
    if (values[0] > values[1])
    {
        for (int i = 1; i <= 6; ++i)
        {
            values[i + 1] = static_cast<BYTE>(((7 - i) * values[0] + i * values[1]) / 7);
        }
    }
    else
    {
        for (int i = 1; i <= 4; ++i)
        {
            values[i + 1] = static_cast<BYTE>(((5 - i) * values[0] + i * values[1]) / 5);
        }
        values[6] = 0;
        values[7] = 255;
    }
    UINT64 bits = 0;
    memcpy(&bits, block + 2, 6);
    for (int i = 0; i < 16; ++i)
    {
        dest[i] = values[bits & 7];
        bits >>= 3;
    }
}

void DecodeBc3Block(const BYTE block[16], BYTE dest[16 * 4])
{
    DecodeBc1Block(block + 8, dest, false);
    BYTE alpha[16];
    DecodeBc4Values(block, alpha);
    for (int i = 0; i < 16; ++i)
    {
        dest[i * 4 + 3] = alpha[i];
    }
}

BYTE ReconstructNormalZ(BYTE r, BYTE g)
{
    const float x = (static_cast<float>(r) / 255.0f) * 2.0f - 1.0f;
    const float y = (static_cast<float>(g) / 255.0f) * 2.0f - 1.0f;
    float z2 = 1.0f - x * x - y * y;
    if (z2 < 0.0f)
    {
        z2 = 0.0f;
    }
    const int b = static_cast<int>((std::sqrt(z2) * 0.5f + 0.5f) * 255.0f + 0.5f);
    if (b < 0)
    {
        return 0;
    }
    if (b > 255)
    {
        return 255;
    }
    return static_cast<BYTE>(b);
}

Status CopyBcSurface(Reader& reader, Frame& frame, int mode)
{
    const UINT blocksX = (frame.width + 3) / 4;
    const UINT blocksY = (frame.height + 3) / 4;
    BYTE decoded[16 * 4];
    for (UINT by = 0; by < blocksY; ++by)
    {
        for (UINT bx = 0; bx < blocksX; ++bx)
        {
            BYTE block[16]{};
            const UINT blockSize = (mode == 1 || mode == 4) ? 8u : 16u;
            if (!reader.Read(block, blockSize))
            {
                return Status::Invalid;
            }
            if (mode == 1)
            {
                DecodeBc1Block(block, decoded, true);
            }
            else if (mode == 2)
            {
                DecodeBc1Block(block + 8, decoded, false);
                for (int i = 0; i < 16; ++i)
                {
                    const BYTE nibble = (block[i / 2] >> ((i & 1) * 4)) & 0x0F;
                    decoded[i * 4 + 3] = static_cast<BYTE>(nibble * 17);
                }
            }
            else if (mode == 4)
            {
                BYTE values[16];
                DecodeBc4Values(block, values);
                for (int i = 0; i < 16; ++i)
                {
                    decoded[i * 4 + 0] = values[i];
                    decoded[i * 4 + 1] = values[i];
                    decoded[i * 4 + 2] = values[i];
                    decoded[i * 4 + 3] = 255;
                }
            }
            else if (mode == 5)
            {
                BYTE red[16];
                BYTE green[16];
                DecodeBc4Values(block, red);
                DecodeBc4Values(block + 8, green);
                for (int i = 0; i < 16; ++i)
                {
                    decoded[i * 4 + 0] = ReconstructNormalZ(red[i], green[i]);
                    decoded[i * 4 + 1] = green[i];
                    decoded[i * 4 + 2] = red[i];
                    decoded[i * 4 + 3] = 255;
                }
            }
            else
            {
                DecodeBc3Block(block, decoded);
            }
            for (UINT py = 0; py < 4; ++py)
            {
                const UINT y = by * 4 + py;
                if (y >= frame.height)
                {
                    continue;
                }
                for (UINT px = 0; px < 4; ++px)
                {
                    const UINT x = bx * 4 + px;
                    if (x >= frame.width)
                    {
                        continue;
                    }
                    const BYTE* p = decoded + (py * 4 + px) * 4;
                    SetPixel(frame, x, y, p[0], p[1], p[2], p[3]);
                }
            }
        }
    }
    return Status::Ok;
}

Status CopyMaskedRgb(Reader& reader, Frame& frame, UINT rgbBits, UINT32 rMask, UINT32 gMask, UINT32 bMask, UINT32 aMask)
{
    const UINT bytes = (rgbBits + 7) / 8;
    if (bytes == 0 || bytes > 4)
    {
        return Status::Unsupported;
    }
    auto channel = [](UINT32 pixel, UINT32 mask) -> BYTE {
        if (mask == 0)
        {
            return 0;
        }
        UINT shift = 0;
        UINT32 m = mask;
        while ((m & 1) == 0)
        {
            m >>= 1;
            ++shift;
        }
        UINT bits = 0;
        while (m & 1)
        {
            m >>= 1;
            ++bits;
        }
        const UINT32 value = (pixel >> shift) & (mask >> shift);
        if (bits >= 8)
        {
            return static_cast<BYTE>(value >> (bits - 8));
        }
        return static_cast<BYTE>((value * 255) / ((1u << bits) - 1));
    };
    const UINT stride = ((frame.width * bytes + 3) / 4) * 4;
    std::vector<BYTE> row(stride);
    for (UINT y = 0; y < frame.height; ++y)
    {
        if (!reader.Read(row.data(), stride))
        {
            return Status::Invalid;
        }
        for (UINT x = 0; x < frame.width; ++x)
        {
            UINT32 pixel = 0;
            memcpy(&pixel, row.data() + static_cast<size_t>(x) * bytes, bytes);
            const BYTE r = channel(pixel, rMask ? rMask : 0x00FF0000);
            const BYTE g = channel(pixel, gMask ? gMask : 0x0000FF00);
            const BYTE b = channel(pixel, bMask ? bMask : 0x000000FF);
            const BYTE a = aMask ? channel(pixel, aMask) : 255;
            SetPixel(frame, x, y, b, g, r, a);
        }
    }
    return Status::Ok;
}

Status CopyDxgiUnorm32(Reader& reader, Frame& frame, bool bgra, bool hasAlpha)
{
    const UINT stride = frame.width * 4;
    std::vector<BYTE> row(stride);
    for (UINT y = 0; y < frame.height; ++y)
    {
        if (!reader.Read(row.data(), stride))
        {
            return Status::Invalid;
        }
        for (UINT x = 0; x < frame.width; ++x)
        {
            const BYTE* p = row.data() + static_cast<size_t>(x) * 4;
            if (bgra)
            {
                SetPixel(frame, x, y, p[0], p[1], p[2], hasAlpha ? p[3] : 255);
            }
            else
            {
                SetPixel(frame, x, y, p[2], p[1], p[0], hasAlpha ? p[3] : 255);
            }
        }
    }
    return Status::Ok;
}

} // namespace

Status DecodeDds(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 128 || !reader.StartsWith("DDS ", 4))
    {
        return Status::Unsupported;
    }
    reader.Seek(4);
    const UINT32 headerSize = reader.U32LE();
    reader.U32LE(); // flags
    const UINT32 height = reader.U32LE();
    const UINT32 width = reader.U32LE();
    reader.U32LE(); // pitch
    reader.U32LE(); // depth
    reader.U32LE(); // mipmaps
    reader.Skip(11 * 4); // reserved
    const UINT32 pfSize = reader.U32LE();
    const UINT32 pfFlags = reader.U32LE();
    const UINT32 fourCC = reader.U32LE();
    const UINT32 rgbBits = reader.U32LE();
    const UINT32 rMask = reader.U32LE();
    const UINT32 gMask = reader.U32LE();
    const UINT32 bMask = reader.U32LE();
    const UINT32 aMask = reader.U32LE();
    reader.Seek(4 + headerSize);
    if (reader.Failed() || headerSize < 124 || pfSize < 32)
    {
        return Status::Invalid;
    }

    int bcMode = 0;
    bool dxgiBgra = false;
    bool dxgiHasAlpha = true;
    bool dxgiUnorm32 = false;
    if (fourCC == FourCC("DX10"))
    {
        const UINT32 dxgiFormat = reader.U32LE();
        reader.U32LE(); // resource dimension
        reader.U32LE(); // misc
        const UINT32 arraySize = reader.U32LE();
        reader.U32LE(); // misc2
        if (reader.Failed() || arraySize > 1)
        {
            return Status::Unsupported;
        }
        switch (dxgiFormat)
        {
        case 71: // DXGI_FORMAT_BC1_UNORM
        case 72: // DXGI_FORMAT_BC1_UNORM_SRGB
            bcMode = 1;
            break;
        case 74: // DXGI_FORMAT_BC2_UNORM
        case 75: // DXGI_FORMAT_BC2_UNORM_SRGB
            bcMode = 2;
            break;
        case 77: // DXGI_FORMAT_BC3_UNORM
        case 78: // DXGI_FORMAT_BC3_UNORM_SRGB
            bcMode = 3;
            break;
        case 80: // DXGI_FORMAT_BC4_UNORM
        case 81: // DXGI_FORMAT_BC4_SNORM
            bcMode = 4;
            break;
        case 83: // DXGI_FORMAT_BC5_UNORM
        case 84: // DXGI_FORMAT_BC5_SNORM
            bcMode = 5;
            break;
        case 28: // DXGI_FORMAT_R8G8B8A8_UNORM
        case 29: // DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
            dxgiUnorm32 = true;
            dxgiBgra = false;
            dxgiHasAlpha = true;
            break;
        case 87: // DXGI_FORMAT_B8G8R8A8_UNORM
        case 91: // DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
            dxgiUnorm32 = true;
            dxgiBgra = true;
            dxgiHasAlpha = true;
            break;
        case 88: // DXGI_FORMAT_B8G8R8X8_UNORM
        case 93: // DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
            dxgiUnorm32 = true;
            dxgiBgra = true;
            dxgiHasAlpha = false;
            break;
        default:
            return Status::Unsupported;
        }
    }
    else if (fourCC == FourCC("DXT1"))
    {
        bcMode = 1;
    }
    else if (fourCC == FourCC("DXT3"))
    {
        bcMode = 2;
    }
    else if (fourCC == FourCC("DXT5"))
    {
        bcMode = 3;
    }
    else if (fourCC == FourCC("ATI1") || fourCC == FourCC("BC4U") || fourCC == FourCC("BC4S"))
    {
        bcMode = 4;
    }
    else if (fourCC == FourCC("ATI2") || fourCC == FourCC("BC5U") || fourCC == FourCC("BC5S") ||
             fourCC == FourCC("DXN "))
    {
        bcMode = 5;
    }

    Frame frame;
    Status st = MakeFrame(frame, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }
    if (bcMode != 0)
    {
        st = CopyBcSurface(reader, frame, bcMode);
    }
    else if (dxgiUnorm32)
    {
        st = CopyDxgiUnorm32(reader, frame, dxgiBgra, dxgiHasAlpha);
    }
    else if ((pfFlags & 0x40) != 0 || fourCC == 0)
    {
        st = CopyMaskedRgb(reader, frame, rgbBits, rMask, gMask, bMask, aMask);
    }
    else
    {
        return Status::Unsupported;
    }
    if (st != Status::Ok)
    {
        return st;
    }
    return FinishSingle(image, std::move(frame), PVF_DDS, "DDS");
}

} // namespace Detail
} // namespace PictView::Native
