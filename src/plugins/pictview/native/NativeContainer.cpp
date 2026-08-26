// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

namespace PictView::Native
{
namespace Detail
{
namespace
{

bool DecodeBase64(const char* text, size_t length, std::vector<BYTE>& out)
{
    out.clear();
    int accum = 0;
    int bits = 0;
    for (size_t i = 0; i < length; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '=' || c == '\r' || c == '\n' || c == ' ' || c == '\t')
        {
            continue;
        }
        int value = -1;
        if (c >= 'A' && c <= 'Z')
        {
            value = c - 'A';
        }
        else if (c >= 'a' && c <= 'z')
        {
            value = c - 'a' + 26;
        }
        else if (c >= '0' && c <= '9')
        {
            value = c - '0' + 52;
        }
        else if (c == '+')
        {
            value = 62;
        }
        else if (c == '/')
        {
            value = 63;
        }
        if (value < 0)
        {
            continue;
        }
        accum = (accum << 6) | value;
        bits += 6;
        if (bits >= 8)
        {
            bits -= 8;
            out.push_back(static_cast<BYTE>((accum >> bits) & 0xFF));
        }
    }
    return !out.empty();
}

BYTE Paeth(BYTE a, BYTE b, BYTE c)
{
    const int p = static_cast<int>(a) + static_cast<int>(b) - static_cast<int>(c);
    const int pa = std::abs(p - static_cast<int>(a));
    const int pb = std::abs(p - static_cast<int>(b));
    const int pc = std::abs(p - static_cast<int>(c));
    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    if (pb <= pc)
    {
        return b;
    }
    return c;
}

UINT32 Be32(const BYTE* p)
{
    return (static_cast<UINT32>(p[0]) << 24) | (static_cast<UINT32>(p[1]) << 16) |
           (static_cast<UINT32>(p[2]) << 8) | p[3];
}

Status CompositeOver(Frame& dest, const Frame& src, int offsetX, int offsetY)
{
    for (UINT y = 0; y < src.height; ++y)
    {
        const int dy = offsetY + static_cast<int>(y);
        if (dy < 0 || dy >= static_cast<int>(dest.height))
        {
            continue;
        }
        for (UINT x = 0; x < src.width; ++x)
        {
            const int dx = offsetX + static_cast<int>(x);
            if (dx < 0 || dx >= static_cast<int>(dest.width))
            {
                continue;
            }
            const BYTE* s = Row(src, y) + static_cast<size_t>(x) * 4;
            BYTE* d = Row(dest, static_cast<UINT>(dy)) + static_cast<size_t>(dx) * 4;
            const BYTE a = s[3];
            if (a == 0)
            {
                continue;
            }
            if (a == 255)
            {
                memcpy(d, s, 4);
                continue;
            }
            d[0] = static_cast<BYTE>((s[0] * a + d[0] * (255 - a) + 127) / 255);
            d[1] = static_cast<BYTE>((s[1] * a + d[1] * (255 - a) + 127) / 255);
            d[2] = static_cast<BYTE>((s[2] * a + d[2] * (255 - a) + 127) / 255);
            const int outA = a + ((255 - a) * d[3] + 127) / 255;
            d[3] = static_cast<BYTE>(outA > 255 ? 255 : outA);
        }
    }
    NoteAlpha(dest);
    return Status::Ok;
}

} // namespace

Status DecodePng8(const BYTE* data, size_t size, Frame& frame)
{
    if (data == nullptr || size < 33 || memcmp(data, "\x89PNG\r\n\x1a\n", 8) != 0)
    {
        return Status::Unsupported;
    }
    UINT width = 0;
    UINT height = 0;
    BYTE bitDepth = 0;
    BYTE colorType = 0;
    std::vector<BYTE> idat;
    BYTE palette[256 * 3]{};
    BYTE trns[256]{};
    UINT paletteCount = 0;
    bool hasTrns = false;
    size_t pos = 8;
    while (pos + 12 <= size)
    {
        const UINT32 length = Be32(data + pos);
        if (pos + 12 + length > size)
        {
            return Status::Invalid;
        }
        const BYTE* type = data + pos + 4;
        const BYTE* chunk = data + pos + 8;
        if (memcmp(type, "IHDR", 4) == 0)
        {
            if (length < 13)
            {
                return Status::Invalid;
            }
            width = Be32(chunk);
            height = Be32(chunk + 4);
            bitDepth = chunk[8];
            colorType = chunk[9];
            if (chunk[10] != 0 || chunk[11] != 0 || chunk[12] != 0 || bitDepth != 8)
            {
                return Status::Unsupported;
            }
        }
        else if (memcmp(type, "PLTE", 4) == 0)
        {
            paletteCount = length / 3;
            if (paletteCount > 256)
            {
                return Status::Invalid;
            }
            memcpy(palette, chunk, paletteCount * 3);
        }
        else if (memcmp(type, "tRNS", 4) == 0)
        {
            hasTrns = true;
            memset(trns, 255, sizeof(trns));
            memcpy(trns, chunk, std::min<size_t>(length, 256));
        }
        else if (memcmp(type, "IDAT", 4) == 0)
        {
            idat.insert(idat.end(), chunk, chunk + length);
        }
        else if (memcmp(type, "IEND", 4) == 0)
        {
            break;
        }
        pos += 12 + length;
    }
    if (width == 0 || height == 0 || idat.empty())
    {
        return Status::Invalid;
    }
    int channels = 0;
    switch (colorType)
    {
    case 0:
        channels = 1;
        break;
    case 2:
        channels = 3;
        break;
    case 3:
        channels = 1;
        break;
    case 4:
        channels = 2;
        break;
    case 6:
        channels = 4;
        break;
    default:
        return Status::Unsupported;
    }
    std::vector<BYTE> raw;
    if (!InflateZlib(idat.data(), idat.size(), raw, kMaxFileBytes))
    {
        return Status::Invalid;
    }
    const size_t stride = static_cast<size_t>(width) * static_cast<size_t>(channels);
    if (raw.size() < (stride + 1) * height)
    {
        return Status::Invalid;
    }
    Status st = MakeFrame(frame, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }
    std::vector<BYTE> curr(stride);
    std::vector<BYTE> prev(stride, 0);
    size_t src = 0;
    for (UINT y = 0; y < height; ++y)
    {
        const BYTE filter = raw[src++];
        memcpy(curr.data(), raw.data() + src, stride);
        src += stride;
        for (size_t i = 0; i < stride; ++i)
        {
            const BYTE a = i >= static_cast<size_t>(channels) ? curr[i - channels] : 0;
            const BYTE b = prev[i];
            const BYTE c = i >= static_cast<size_t>(channels) ? prev[i - channels] : 0;
            BYTE recon = curr[i];
            switch (filter)
            {
            case 1:
                recon = static_cast<BYTE>(recon + a);
                break;
            case 2:
                recon = static_cast<BYTE>(recon + b);
                break;
            case 3:
                recon = static_cast<BYTE>(recon + ((a + b) / 2));
                break;
            case 4:
                recon = static_cast<BYTE>(recon + Paeth(a, b, c));
                break;
            case 0:
                break;
            default:
                return Status::Unsupported;
            }
            curr[i] = recon;
        }
        for (UINT x = 0; x < width; ++x)
        {
            BYTE r = 0;
            BYTE g = 0;
            BYTE b = 0;
            BYTE a = 255;
            const BYTE* p = curr.data() + static_cast<size_t>(x) * channels;
            if (colorType == 0)
            {
                r = g = b = p[0];
            }
            else if (colorType == 2)
            {
                r = p[0];
                g = p[1];
                b = p[2];
            }
            else if (colorType == 3)
            {
                const BYTE index = p[0];
                if (index < paletteCount)
                {
                    r = palette[index * 3];
                    g = palette[index * 3 + 1];
                    b = palette[index * 3 + 2];
                }
                a = hasTrns ? trns[index] : 255;
            }
            else if (colorType == 4)
            {
                r = g = b = p[0];
                a = p[1];
            }
            else
            {
                r = p[0];
                g = p[1];
                b = p[2];
                a = p[3];
            }
            SetPixel(frame, x, y, b, g, r, a);
        }
        prev.swap(curr);
        std::fill(curr.begin(), curr.end(), 0);
        // After swap, prev holds current row; curr was previous (now zeroed). Restore prev contents:
        // We swapped so prev is the reconstructed row. Zeroing curr is correct for next iteration's
        // working buffer, but we just zeroed the reconstructed data after swap... wait.
        // swap: prev<->curr, then fill curr with 0. prev keeps reconstructed row. Good.
    }
    return Status::Ok;
}

Status DecodePackedDib(const BYTE* data, size_t size, Frame& frame)
{
    if (data == nullptr || size < 40)
    {
        return Status::Unsupported;
    }
    size_t offset = 0;
    UINT32 bitsOffset = 0;
    if (data[0] == 'B' && data[1] == 'M' && size >= 14)
    {
        bitsOffset = data[10] | (data[11] << 8) | (data[12] << 16) | (data[13] << 24);
        offset = 14;
    }
    else if (size >= 48)
    {
        const INT32 maybeWidth = static_cast<INT32>(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
        const INT32 maybeHeight = static_cast<INT32>(data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24));
        const UINT32 biSize = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
        if (maybeWidth > 0 && maybeHeight != 0 && (biSize == 40 || biSize == 108 || biSize == 124))
        {
            offset = 8;
        }
    }
    if (offset + 16 > size)
    {
        return Status::Unsupported;
    }
    const UINT32 headerSize = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
    if (headerSize != 40 && headerSize != 12 && headerSize != 108 && headerSize != 124)
    {
        return Status::Unsupported;
    }
    INT32 width = 0;
    INT32 height = 0;
    UINT16 bitCount = 0;
    UINT32 compression = 0;
    UINT32 colorsUsed = 0;
    if (headerSize == 12)
    {
        width = static_cast<INT16>(data[offset + 4] | (data[offset + 5] << 8));
        height = static_cast<INT16>(data[offset + 6] | (data[offset + 7] << 8));
        bitCount = static_cast<UINT16>(data[offset + 10] | (data[offset + 11] << 8));
    }
    else
    {
        width = static_cast<INT32>(data[offset + 4] | (data[offset + 5] << 8) | (data[offset + 6] << 16) | (data[offset + 7] << 24));
        height = static_cast<INT32>(data[offset + 8] | (data[offset + 9] << 8) | (data[offset + 10] << 16) | (data[offset + 11] << 24));
        bitCount = static_cast<UINT16>(data[offset + 14] | (data[offset + 15] << 8));
        compression = data[offset + 16] | (data[offset + 17] << 8) | (data[offset + 18] << 16) | (data[offset + 19] << 24);
        colorsUsed = data[offset + 32] | (data[offset + 33] << 8) | (data[offset + 34] << 16) | (data[offset + 35] << 24);
    }
    if (width <= 0 || height == 0 || compression != 0)
    {
        return Status::Unsupported;
    }
    const bool topDown = height < 0;
    const UINT absHeight = static_cast<UINT>(topDown ? -height : height);
    const UINT absWidth = static_cast<UINT>(width);
    if (bitsOffset == 0)
    {
        UINT paletteEntries = 0;
        if (bitCount <= 8)
        {
            paletteEntries = colorsUsed != 0 ? colorsUsed : (1u << bitCount);
        }
        const UINT paletteBytes = headerSize == 12 ? paletteEntries * 3 : paletteEntries * 4;
        bitsOffset = static_cast<UINT32>(offset + headerSize + paletteBytes);
    }
    if (bitsOffset >= size)
    {
        return Status::Invalid;
    }
    Status st = MakeFrame(frame, absWidth, absHeight, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }
    const UINT srcStride = ((absWidth * bitCount + 31) / 32) * 4;
    BYTE palette[256][4]{};
    if (bitCount <= 8)
    {
        UINT paletteEntries = colorsUsed != 0 ? colorsUsed : (1u << bitCount);
        if (paletteEntries > 256)
        {
            paletteEntries = 256;
        }
        size_t palPos = offset + headerSize;
        for (UINT i = 0; i < paletteEntries && palPos < size; ++i)
        {
            if (headerSize == 12)
            {
                if (palPos + 3 > size)
                {
                    break;
                }
                palette[i][0] = data[palPos++];
                palette[i][1] = data[palPos++];
                palette[i][2] = data[palPos++];
                palette[i][3] = 255;
            }
            else
            {
                if (palPos + 4 > size)
                {
                    break;
                }
                palette[i][0] = data[palPos++];
                palette[i][1] = data[palPos++];
                palette[i][2] = data[palPos++];
                palette[i][3] = 255;
                palPos++;
            }
        }
    }
    for (UINT y = 0; y < absHeight; ++y)
    {
        const UINT srcY = topDown ? y : (absHeight - 1 - y);
        const size_t rowPos = static_cast<size_t>(bitsOffset) + static_cast<size_t>(srcY) * srcStride;
        if (rowPos + srcStride > size && bitCount >= 8)
        {
            if (rowPos >= size)
            {
                break;
            }
        }
        for (UINT x = 0; x < absWidth; ++x)
        {
            BYTE b = 0;
            BYTE g = 0;
            BYTE r = 0;
            BYTE a = 255;
            if (bitCount == 32)
            {
                const size_t p = rowPos + static_cast<size_t>(x) * 4;
                if (p + 4 > size)
                {
                    continue;
                }
                b = data[p];
                g = data[p + 1];
                r = data[p + 2];
                a = data[p + 3];
            }
            else if (bitCount == 24)
            {
                const size_t p = rowPos + static_cast<size_t>(x) * 3;
                if (p + 3 > size)
                {
                    continue;
                }
                b = data[p];
                g = data[p + 1];
                r = data[p + 2];
            }
            else if (bitCount == 16)
            {
                const size_t p = rowPos + static_cast<size_t>(x) * 2;
                if (p + 2 > size)
                {
                    continue;
                }
                const UINT16 pix = static_cast<UINT16>(data[p] | (data[p + 1] << 8));
                r = static_cast<BYTE>(((pix >> 10) & 31) * 255 / 31);
                g = static_cast<BYTE>(((pix >> 5) & 31) * 255 / 31);
                b = static_cast<BYTE>((pix & 31) * 255 / 31);
            }
            else if (bitCount == 8)
            {
                if (rowPos + x >= size)
                {
                    continue;
                }
                const BYTE index = data[rowPos + x];
                b = palette[index][0];
                g = palette[index][1];
                r = palette[index][2];
            }
            else if (bitCount == 1)
            {
                const BYTE bit = (data[rowPos + x / 8] >> (7 - (x % 8))) & 1;
                b = g = r = bit ? 255 : 0;
            }
            else
            {
                return Status::Unsupported;
            }
            SetPixel(frame, x, y, b, g, r, a);
        }
    }
    ForceUnusedAlphaOpaque(frame);
    return Status::Ok;
}

Status DecodePdn(Reader& reader, DecodedImage& image)
{
    const BYTE* data = reader.Data();
    const size_t size = reader.Size();
    if (data == nullptr || size < 8 || memcmp(data, "PDN3", 4) != 0)
    {
        return Status::Unsupported;
    }
    const UINT32 xmlLen = data[4] | (data[5] << 8) | (data[6] << 16);
    if (xmlLen == 0 || static_cast<size_t>(7) + xmlLen > size)
    {
        return Status::Invalid;
    }
    const std::string xml(reinterpret_cast<const char*>(data + 7), xmlLen);
    const size_t thumb = xml.find("png=\"");
    if (thumb == std::string::npos)
    {
        return Status::Unsupported;
    }
    const size_t start = thumb + 5;
    const size_t end = xml.find('"', start);
    if (end == std::string::npos)
    {
        return Status::Invalid;
    }
    std::vector<BYTE> png;
    if (!DecodeBase64(xml.c_str() + start, end - start, png))
    {
        return Status::Invalid;
    }
    Frame frame;
    const Status st = DecodePng8(png.data(), png.size(), frame);
    if (st != Status::Ok)
    {
        return st;
    }
    return FinishSingle(image, std::move(frame), PVF_PDN, "PDN");
}

namespace
{

UINT32 XcfU32(Reader& reader)
{
    return reader.U32BE();
}

UINT64 XcfOffset(Reader& reader, int version)
{
    if (version >= 11)
    {
        const UINT32 hi = reader.U32BE();
        const UINT32 lo = reader.U32BE();
        return (static_cast<UINT64>(hi) << 32) | lo;
    }
    return XcfU32(reader);
}

bool SkipXcfString(Reader& reader)
{
    const UINT32 length = XcfU32(reader);
    if (length == 0)
    {
        return !reader.Failed();
    }
    reader.Skip(length);
    return !reader.Failed();
}

Status DecodeXcfRle(Reader& reader, BYTE* dest, size_t count)
{
    size_t filled = 0;
    while (filled < count)
    {
        const BYTE opcode = reader.U8();
        if (reader.Failed())
        {
            return Status::Invalid;
        }
        if (opcode <= 126)
        {
            const size_t n = static_cast<size_t>(opcode) + 1;
            if (filled + n > count || !reader.Read(dest + filled, n))
            {
                return Status::Invalid;
            }
            filled += n;
        }
        else if (opcode == 127)
        {
            const UINT16 n = reader.U16BE();
            if (filled + n > count || !reader.Read(dest + filled, n))
            {
                return Status::Invalid;
            }
            filled += n;
        }
        else if (opcode == 128)
        {
            const UINT16 n = reader.U16BE();
            const BYTE value = reader.U8();
            if (filled + n > count)
            {
                return Status::Invalid;
            }
            memset(dest + filled, value, n);
            filled += n;
        }
        else
        {
            const size_t n = 257u - opcode;
            const BYTE value = reader.U8();
            if (filled + n > count)
            {
                return Status::Invalid;
            }
            memset(dest + filled, value, n);
            filled += n;
        }
    }
    return Status::Ok;
}

Status DecodeXcfTile(Reader& reader, BYTE* dest, size_t bytes, int compression)
{
    if (compression == 0)
    {
        return reader.Read(dest, bytes) ? Status::Ok : Status::Invalid;
    }
    if (compression == 1)
    {
        return DecodeXcfRle(reader, dest, bytes);
    }
    if (compression == 2)
    {
        const BYTE* start = reader.Peek(1);
        if (start == nullptr)
        {
            return Status::Invalid;
        }
        const size_t remaining = reader.Remaining();
        std::vector<BYTE> inflated;
        if (!InflateZlib(start, remaining, inflated, bytes + 64) &&
            !InflateRaw(start, remaining, inflated, bytes + 64))
        {
            return Status::Invalid;
        }
        if (inflated.size() < bytes)
        {
            return Status::Invalid;
        }
        memcpy(dest, inflated.data(), bytes);
        // zlib streams in XCF are per-tile; consume the compressed bytes we cannot size exactly.
        // Advance using the zlib/gzip consumed amount is not tracked; skip remaining of this tile
        // pointer range at the caller.
        return Status::Ok;
    }
    return Status::Unsupported;
}

Status DecodeXcfLayer(Reader& reader, int version, int compression, int imageType, Frame& layer, INT32& offsetX, INT32& offsetY)
{
    offsetX = 0;
    offsetY = 0;
    const UINT width = XcfU32(reader);
    const UINT height = XcfU32(reader);
    const UINT type = XcfU32(reader);
    if (!SkipXcfString(reader) || reader.Failed())
    {
        return Status::Invalid;
    }
    int bpp = 0;
    bool hasAlpha = false;
    if (type == 0)
    {
        bpp = 3;
    }
    else if (type == 1)
    {
        bpp = 4;
        hasAlpha = true;
    }
    else if (type == 2)
    {
        bpp = 1;
    }
    else if (type == 3)
    {
        bpp = 2;
        hasAlpha = true;
    }
    else
    {
        return Status::Unsupported;
    }
    (void)imageType;
    bool visible = true;
    for (;;)
    {
        const UINT32 prop = XcfU32(reader);
        const UINT32 payload = XcfU32(reader);
        if (reader.Failed())
        {
            return Status::Invalid;
        }
        if (prop == 0)
        {
            break;
        }
        const size_t start = reader.Position();
        if (prop == 6 && payload >= 8)
        {
            offsetX = static_cast<INT32>(XcfU32(reader));
            offsetY = static_cast<INT32>(XcfU32(reader));
        }
        else if (prop == 8 && payload >= 4)
        {
            visible = XcfU32(reader) != 0;
        }
        reader.Seek(start + payload);
    }
    const UINT64 hierarchy = XcfOffset(reader, version);
    if (reader.Failed() || hierarchy == 0 || hierarchy >= reader.Size())
    {
        return Status::Invalid;
    }
    if (!visible || width == 0 || height == 0)
    {
        layer.width = 0;
        return Status::Ok;
    }
    Status st = MakeFrame(layer, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }
    reader.Seek(static_cast<size_t>(hierarchy));
    const UINT hWidth = XcfU32(reader);
    const UINT hHeight = XcfU32(reader);
    const UINT hBpp = XcfU32(reader);
    const UINT64 level = XcfOffset(reader, version);
    if (reader.Failed() || hWidth != width || hHeight != height || hBpp != static_cast<UINT>(bpp) || level == 0)
    {
        return Status::Invalid;
    }
    reader.Seek(static_cast<size_t>(level));
    XcfU32(reader); // level width
    XcfU32(reader); // level height
    std::vector<UINT64> tiles;
    for (;;)
    {
        const UINT64 tile = XcfOffset(reader, version);
        if (reader.Failed())
        {
            return Status::Invalid;
        }
        if (tile == 0)
        {
            break;
        }
        tiles.push_back(tile);
    }
    const UINT tilesX = (width + 63) / 64;
    const UINT tilesY = (height + 63) / 64;
    if (tiles.size() < static_cast<size_t>(tilesX) * tilesY)
    {
        return Status::Invalid;
    }
    size_t tileIndex = 0;
    std::vector<BYTE> tileBytes;
    for (UINT ty = 0; ty < tilesY; ++ty)
    {
        for (UINT tx = 0; tx < tilesX; ++tx)
        {
            const UINT tw = std::min(64u, width - tx * 64);
            const UINT th = std::min(64u, height - ty * 64);
            const size_t bytes = static_cast<size_t>(tw) * th * bpp;
            tileBytes.assign(bytes, 0);
            reader.Seek(static_cast<size_t>(tiles[tileIndex++]));
            st = DecodeXcfTile(reader, tileBytes.data(), bytes, compression);
            if (st != Status::Ok)
            {
                return st;
            }
            for (UINT py = 0; py < th; ++py)
            {
                for (UINT px = 0; px < tw; ++px)
                {
                    const BYTE* p = tileBytes.data() + (static_cast<size_t>(py) * tw + px) * bpp;
                    BYTE r = p[0];
                    BYTE g = p[0];
                    BYTE b = p[0];
                    BYTE a = 255;
                    if (bpp >= 3)
                    {
                        r = p[0];
                        g = p[1];
                        b = p[2];
                        if (bpp == 4)
                        {
                            a = p[3];
                        }
                    }
                    else if (bpp == 2)
                    {
                        a = p[1];
                    }
                    SetPixel(layer, tx * 64 + px, ty * 64 + py, b, g, r, a);
                }
            }
        }
    }
    (void)hasAlpha;
    return Status::Ok;
}

} // namespace

Status DecodeXcf(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 26 || !reader.StartsWith("gimp xcf ", 9))
    {
        return Status::Unsupported;
    }
    const BYTE* magic = reader.Data();
    int version = 0;
    if (memcmp(magic + 9, "file", 4) == 0)
    {
        version = 0;
    }
    else if (magic[9] == 'v')
    {
        version = (magic[10] - '0') * 100 + (magic[11] - '0') * 10 + (magic[12] - '0');
        if (version < 0)
        {
            return Status::Unsupported;
        }
    }
    else
    {
        return Status::Unsupported;
    }
    reader.Seek(14);
    const UINT width = XcfU32(reader);
    const UINT height = XcfU32(reader);
    const UINT baseType = XcfU32(reader);
    if (version >= 4)
    {
        XcfU32(reader); // precision
    }
    if (reader.Failed() || (baseType != 0 && baseType != 1))
    {
        return baseType == 2 ? Status::Unsupported : Status::Invalid;
    }
    int compression = 1;
    for (;;)
    {
        const UINT32 prop = XcfU32(reader);
        const UINT32 payload = XcfU32(reader);
        if (reader.Failed())
        {
            return Status::Invalid;
        }
        if (prop == 0)
        {
            break;
        }
        const size_t start = reader.Position();
        if (prop == 17 && payload >= 1)
        {
            compression = reader.U8();
        }
        reader.Seek(start + payload);
    }
    std::vector<UINT64> layers;
    for (;;)
    {
        const UINT64 offset = XcfOffset(reader, version);
        if (reader.Failed())
        {
            return Status::Invalid;
        }
        if (offset == 0)
        {
            break;
        }
        layers.push_back(offset);
    }
    if (layers.empty())
    {
        return Status::Invalid;
    }
    Frame canvas;
    Status st = MakeFrame(canvas, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }
    memset(canvas.bgra.data(), 0, canvas.bgra.size());
    canvas.hasTransparency = true;
    // XCF stores the bottom-most layer first.
    for (UINT64 offset : layers)
    {
        reader.Seek(static_cast<size_t>(offset));
        Frame layer;
        INT32 ox = 0;
        INT32 oy = 0;
        st = DecodeXcfLayer(reader, version, compression, static_cast<int>(baseType), layer, ox, oy);
        if (st == Status::Unsupported)
        {
            continue;
        }
        if (st != Status::Ok)
        {
            return st;
        }
        if (layer.width == 0)
        {
            continue;
        }
        CompositeOver(canvas, layer, ox, oy);
    }
    return FinishSingle(image, std::move(canvas), PVF_XCF, "XCF");
}

namespace
{

constexpr UINT32 kTcodeShort = 0x80000000u;
constexpr UINT32 kTcodeCrc = 0x8000u;
constexpr UINT32 kTcodeTable = 0x10000000u;
constexpr UINT32 kTcodePropertiesTable = kTcodeTable | 0x0014u;
constexpr UINT32 kTcodePreview = 0x20000000u | kTcodeCrc | 0x0023u;
constexpr UINT32 kTcodeCompressedPreview = 0x20000000u | kTcodeCrc | 0x0025u;
constexpr UINT32 kTcodeAnonymousChunk = 0x40008000u; // TCODE_USER | TCODE_CRC
constexpr UINT32 kTcodeEndOfTable = 0xFFFFFFFFu;

int Parse3dmArchiveVersion(const BYTE* data, size_t size)
{
    if (data == nullptr || size < 32)
    {
        return 0;
    }
    int version = 0;
    for (size_t i = 24; i < 32; ++i)
    {
        const unsigned char c = data[i];
        if (c >= '0' && c <= '9')
        {
            version = version * 10 + (c - '0');
        }
        else if (c != ' ' && version != 0)
        {
            break;
        }
    }
    return version;
}

bool Read3dmInt64(Reader& reader, INT64& value)
{
    const UINT32 lo = reader.U32LE();
    const UINT32 hi = reader.U32LE();
    if (reader.Failed())
    {
        return false;
    }
    value = static_cast<INT64>(static_cast<UINT64>(lo) | (static_cast<UINT64>(hi) << 32));
    return true;
}

bool Read3dmChunk(Reader& reader, UINT32& tcode, std::vector<BYTE>& payload, size_t chunkValueBytes)
{
    tcode = reader.U32LE();
    INT64 length = 0;
    if (chunkValueBytes == 8)
    {
        if (!Read3dmInt64(reader, length))
        {
            return false;
        }
    }
    else
    {
        const INT32 value = static_cast<INT32>(reader.U32LE());
        if (reader.Failed())
        {
            return false;
        }
        length = value;
        if (length < 0)
        {
            if (!Read3dmInt64(reader, length))
            {
                return false;
            }
        }
    }
    if (reader.Failed())
    {
        return false;
    }
    payload.clear();
    if (tcode & kTcodeShort)
    {
        return true;
    }
    if (length < 0 || static_cast<size_t>(length) > reader.Remaining())
    {
        return false;
    }
    payload.resize(static_cast<size_t>(length));
    if (length > 0 && !reader.Read(payload.data(), static_cast<size_t>(length)))
    {
        return false;
    }
    if ((tcode & kTcodeCrc) && payload.size() >= 4)
    {
        payload.resize(payload.size() - 4);
    }
    return true;
}

bool SliceOpenNurbsZlib(const BYTE* src, size_t avail, size_t chunkValueBytes, const BYTE*& zlib,
                        size_t& zlibLen)
{
    zlib = src;
    zlibLen = avail;
    if (src == nullptr || avail < 2)
    {
        return false;
    }
    const UINT32 tcode = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
    if (tcode != kTcodeAnonymousChunk)
    {
        return true;
    }

    auto tryValueBytes = [&](size_t valueBytes) -> bool {
        if (avail < 4 + valueBytes)
        {
            return false;
        }
        UINT64 length = 0;
        if (valueBytes == 8)
        {
            const UINT32 lo = src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24);
            const UINT32 hi = src[8] | (src[9] << 8) | (src[10] << 16) | (src[11] << 24);
            length = static_cast<UINT64>(lo) | (static_cast<UINT64>(hi) << 32);
        }
        else
        {
            length = src[4] | (src[5] << 8) | (src[6] << 16) | (src[7] << 24);
        }
        if (length < 4 || 4 + valueBytes + static_cast<size_t>(length) > avail)
        {
            return false;
        }
        zlib = src + 4 + valueBytes;
        zlibLen = static_cast<size_t>(length) - 4;
        return zlibLen >= 2;
    };

    if (tryValueBytes(chunkValueBytes))
    {
        return true;
    }
    return tryValueBytes(chunkValueBytes == 8 ? 4u : 8u);
}

bool ReadOpenNurbsCompressedBuffer(Reader& reader, size_t uncompressed, std::vector<BYTE>& dest,
                                   size_t chunkValueBytes)
{
    dest.clear();
    if (uncompressed == 0)
    {
        return true;
    }
    if (uncompressed > kMaxFileBytes)
    {
        return false;
    }
    reader.U32LE(); // CRC of the uncompressed bytes; not required to display the preview
    const BYTE method = reader.U8();
    if (reader.Failed() || (method != 0 && method != 1))
    {
        return false;
    }
    if (method == 0)
    {
        try
        {
            dest.resize(uncompressed);
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }
        return reader.Read(dest.data(), uncompressed);
    }
    const size_t avail = reader.Remaining();
    if (avail < 2)
    {
        return false;
    }
    const BYTE* src = reader.Peek(avail);
    if (src == nullptr)
    {
        return false;
    }
    const BYTE* zlib = src;
    size_t zlibLen = avail;
    if (!SliceOpenNurbsZlib(src, avail, chunkValueBytes, zlib, zlibLen))
    {
        return false;
    }
    if (!InflateZlib(zlib, zlibLen, dest, uncompressed))
    {
        dest.clear();
        return false;
    }
    return dest.size() >= uncompressed;
}

Status Decode3dmCompressedPreview(const BYTE* data, size_t size, Frame& frame, size_t chunkValueBytes)
{
    // OpenNURBS ON_WindowsBitmap::ReadCompressed: BITMAPINFOHEADER fields, then
    // a WriteCompressedBuffer blob of palette+bits.
    if (data == nullptr || size < 40 + 4 + 4 + 1)
    {
        return Status::Unsupported;
    }
    Reader reader(data, size);
    const UINT32 biSize = reader.U32LE();
    const INT32 width = static_cast<INT32>(reader.U32LE());
    const INT32 height = static_cast<INT32>(reader.U32LE());
    const UINT16 planes = reader.U16LE();
    const UINT16 bitCount = reader.U16LE();
    const UINT32 compression = reader.U32LE();
    reader.U32LE(); // biSizeImage
    reader.U32LE(); // biXPelsPerMeter
    reader.U32LE(); // biYPelsPerMeter
    const UINT32 colorsUsed = reader.U32LE();
    reader.U32LE(); // biClrImportant
    if (reader.Failed() || biSize != 40 || planes != 1 || compression != 0 || width <= 0 || height == 0)
    {
        return Status::Unsupported;
    }
    const UINT absWidth = static_cast<UINT>(width);
    const UINT absHeight = static_cast<UINT>(height < 0 ? -height : height);
    if (absWidth > kMaxDimension || absHeight > kMaxDimension)
    {
        return Status::Unsupported;
    }
    if (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 16 && bitCount != 24 && bitCount != 32)
    {
        return Status::Unsupported;
    }
    UINT paletteEntries = 0;
    if (bitCount <= 8)
    {
        paletteEntries = colorsUsed != 0 ? colorsUsed : (1u << bitCount);
        if (paletteEntries > 256)
        {
            paletteEntries = 256;
        }
    }
    const size_t paletteBytes = static_cast<size_t>(paletteEntries) * 4u;
    const size_t stride = ((static_cast<size_t>(absWidth) * bitCount + 31) / 32) * 4;
    const size_t imageBytes = stride * absHeight;
    if (imageBytes == 0 || imageBytes > kMaxFileBytes)
    {
        return Status::Unsupported;
    }

    const UINT32 bodyUncompressed = reader.U32LE();
    if (reader.Failed() || bodyUncompressed == 0 || bodyUncompressed > kMaxFileBytes)
    {
        return Status::Unsupported;
    }
    std::vector<BYTE> body;
    if (!ReadOpenNurbsCompressedBuffer(reader, bodyUncompressed, body, chunkValueBytes))
    {
        return Status::Unsupported;
    }
    if (body.size() > bodyUncompressed)
    {
        body.resize(bodyUncompressed);
    }
    if (body.size() < paletteBytes + imageBytes)
    {
        return Status::Unsupported;
    }

    std::vector<BYTE> dib;
    try
    {
        dib.assign(40 + paletteBytes + imageBytes, 0);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }
    memcpy(dib.data(), data, 40);
    memcpy(dib.data() + 40, body.data(), paletteBytes + imageBytes);
    return DecodePackedDib(dib.data(), dib.size(), frame);
}

Status FrameFromPreviewBytes(const BYTE* data, size_t size, Frame& frame)
{
    Status st = DecodePackedDib(data, size, frame);
    if (st == Status::Ok)
    {
        return st;
    }
    st = DecodePng8(data, size, frame);
    if (st == Status::Ok)
    {
        return st;
    }
    st = RasterizeMetafile(data, size, frame);
    if (st == Status::Ok)
    {
        return st;
    }
    EmbeddedPreview preview;
    if (FindEmbeddedPreview(data, size, preview) && preview.offset + preview.size <= size)
    {
        const BYTE* slice = data + preview.offset;
        if (preview.format == PVF_PNG)
        {
            return DecodePng8(slice, preview.size, frame);
        }
        if (preview.format == PVF_BMP)
        {
            return DecodePackedDib(slice, preview.size, frame);
        }
        if (preview.format == PVF_WMF || preview.format == PVF_EMF || preview.format == PVF_EPS)
        {
            return RasterizeMetafile(slice, preview.size, frame);
        }
    }
    return Status::Unsupported;
}

// 3DM whole-file scans must not treat a coincidental WMF/EMF/TIFF signature as the
// model preview: that blocked the original JPEG/BMP/Explorer path.
Status FrameFromRasterPreviewBytes(const BYTE* data, size_t size, Frame& frame)
{
    Status st = DecodePackedDib(data, size, frame);
    if (st == Status::Ok)
    {
        return st;
    }
    st = DecodePng8(data, size, frame);
    if (st == Status::Ok)
    {
        return st;
    }
    EmbeddedPreview preview;
    if (FindEmbeddedPreview(data, size, preview) && preview.offset + preview.size <= size)
    {
        const BYTE* slice = data + preview.offset;
        if (preview.format == PVF_PNG)
        {
            return DecodePng8(slice, preview.size, frame);
        }
        if (preview.format == PVF_BMP)
        {
            return DecodePackedDib(slice, preview.size, frame);
        }
    }
    return Status::Unsupported;
}

Status TryDecode3dmProperties(Reader& reader, DecodedImage& image, size_t chunkValueBytes)
{
    reader.Seek(32);
    const size_t headerBytes = 4 + chunkValueBytes;
    while (reader.Remaining() >= headerBytes)
    {
        UINT32 tcode = 0;
        std::vector<BYTE> payload;
        if (!Read3dmChunk(reader, tcode, payload, chunkValueBytes))
        {
            break;
        }
        if (tcode == kTcodePropertiesTable)
        {
            Reader table(payload.data(), payload.size());
            while (table.Remaining() >= headerBytes)
            {
                UINT32 rec = 0;
                std::vector<BYTE> recPayload;
                if (!Read3dmChunk(table, rec, recPayload, chunkValueBytes))
                {
                    break;
                }
                if (rec == kTcodeEndOfTable)
                {
                    break;
                }
                Frame frame;
                Status st = Status::Unsupported;
                if (rec == kTcodePreview)
                {
                    st = FrameFromRasterPreviewBytes(recPayload.data(), recPayload.size(), frame);
                }
                else if (rec == kTcodeCompressedPreview && !recPayload.empty())
                {
                    st = Decode3dmCompressedPreview(recPayload.data(), recPayload.size(), frame,
                                                    chunkValueBytes);
                    if (st != Status::Ok)
                    {
                        std::vector<BYTE> inflated;
                        if (InflateZlib(recPayload.data(), recPayload.size(), inflated, kMaxFileBytes) ||
                            InflateRaw(recPayload.data(), recPayload.size(), inflated, kMaxFileBytes))
                        {
                            st = FrameFromRasterPreviewBytes(inflated.data(), inflated.size(), frame);
                        }
                    }
                }
                if (st == Status::Ok)
                {
                    return FinishSingle(image, std::move(frame), PVF_3DM, "3DM");
                }
            }
            return Status::Unsupported;
        }
        if (tcode == 0x00007FFF || tcode == kTcodeEndOfTable)
        {
            break;
        }
    }
    return Status::Unsupported;
}

} // namespace

Status Decode3dm(Reader& reader, DecodedImage& image)
{
    const BYTE* data = reader.Data();
    if (data == nullptr || reader.Size() < 40 || memcmp(data, "3D Geometry File Format", 23) != 0)
    {
        return Status::Unsupported;
    }
    // openNURBS: archives with version >= 50 store 8-byte chunk values.
    const size_t preferred = Parse3dmArchiveVersion(data, reader.Size()) >= 50 ? 8u : 4u;
    if (TryDecode3dmProperties(reader, image, preferred) == Status::Ok)
    {
        return Status::Ok;
    }
    const size_t fallback = preferred == 8 ? 4u : 8u;
    if (TryDecode3dmProperties(reader, image, fallback) == Status::Ok)
    {
        return Status::Ok;
    }
    Frame frame;
    const Status st = FrameFromRasterPreviewBytes(data, reader.Size(), frame);
    if (st != Status::Ok)
    {
        return Status::Unsupported;
    }
    return FinishSingle(image, std::move(frame), PVF_3DM, "3DM");
}

Status DecodeDwg(Reader& reader, DecodedImage& image)
{
    const BYTE* data = reader.Data();
    const size_t size = reader.Size();
    if (data == nullptr || size < 32 || data[0] != 'A' || data[1] != 'C')
    {
        return Status::Unsupported;
    }
    UINT32 previewAt = 0;
    if (size > 0x11)
    {
        previewAt = data[0x0D] | (data[0x0E] << 8) | (data[0x0F] << 16) | (data[0x10] << 24);
    }
    auto decodeSlice = [&](size_t start, size_t bytes, BYTE type, Frame& frame) -> Status {
        if (bytes == 0 || start >= size || start + bytes > size)
        {
            return Status::Unsupported;
        }
        const BYTE* slice = data + start;
        if (type == 1)
        {
            return Status::Unsupported;
        }
        if (type == 3 || type == 4 || LooksLikePlaceableWmf(slice, bytes) || LooksLikeStandardWmf(slice, bytes) ||
            LooksLikeEmf(slice, bytes))
        {
            const Status st = RasterizeMetafile(slice, bytes, frame);
            if (st == Status::Ok)
            {
                return st;
            }
        }
        return FrameFromPreviewBytes(slice, bytes, frame);
    };
    auto tryParse = [&](size_t offset) -> Status {
        if (offset + 24 > size)
        {
            return Status::Unsupported;
        }
        if (data[offset] != 0x1F || data[offset + 1] != 0x25 || data[offset + 2] != 0x6D || data[offset + 3] != 0x07)
        {
            return Status::Unsupported;
        }
        size_t pos = offset + 16;
        if (pos + 5 > size)
        {
            pos = offset + 4;
        }
        if (pos + 8 > size)
        {
            return Status::Unsupported;
        }
        pos += 4; // overall size
        const BYTE present = data[pos++];
        int entries = present;
        if (entries > 8)
        {
            entries = 8;
        }
        Frame best;
        bool found = false;
        for (int i = 0; i < entries && pos + 9 <= size; ++i)
        {
            const BYTE type = data[pos++];
            const UINT32 start = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
            pos += 4;
            const UINT32 bytes = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
            pos += 4;
            auto consider = [&](size_t candidate) {
                Frame frame;
                if (decodeSlice(candidate, bytes, type, frame) != Status::Ok)
                {
                    return;
                }
                if (!found || frame.width * frame.height > best.width * best.height)
                {
                    best = std::move(frame);
                    found = true;
                }
            };
            consider(start);
            if (start != 0 && offset + static_cast<size_t>(start) < size &&
                offset + static_cast<size_t>(start) != start)
            {
                consider(offset + start);
            }
        }
        if (!found)
        {
            return Status::Unsupported;
        }
        return FinishSingle(image, std::move(best), PVF_DWG, "DWG");
    };
    Status st = Status::Unsupported;
    if (previewAt != 0)
    {
        st = tryParse(previewAt);
    }
    if (st != Status::Ok)
    {
        for (size_t i = 0; i + 4 < std::min(size, static_cast<size_t>(1024 * 1024)); ++i)
        {
            if (data[i] == 0x1F && data[i + 1] == 0x25 && data[i + 2] == 0x6D && data[i + 3] == 0x07)
            {
                st = tryParse(i);
                if (st == Status::Ok)
                {
                    return st;
                }
            }
        }
    }
    return st;
}

bool ExtractZipPreviewBytes(const BYTE* data, size_t size, std::vector<BYTE>& bytes, DWORD& format,
                            const char*& formatLabel)
{
    bytes.clear();
    format = PVF_SKP;
    formatLabel = "SKP";
    if (data == nullptr || size < 30 || data[0] != 'P' || data[1] != 'K' || data[2] != 3 || data[3] != 4)
    {
        return false;
    }
    auto endsWith = [](const std::string& name, const char* ext) -> bool {
        const size_t extLen = strlen(ext);
        return name.size() >= extLen && name.compare(name.size() - extLen, extLen, ext) == 0;
    };
    size_t pos = 0;
    std::vector<BYTE> best;
    size_t bestScore = 0;
    bool bestPng = false;
    bool saw3mf = false;
    bool sawCdr = false;
    while (pos + 30 <= size && data[pos] == 'P' && data[pos + 1] == 'K' && data[pos + 2] == 3 && data[pos + 3] == 4)
    {
        const UINT16 method = static_cast<UINT16>(data[pos + 8] | (data[pos + 9] << 8));
        const UINT32 comp = data[pos + 18] | (data[pos + 19] << 8) | (data[pos + 20] << 16) | (data[pos + 21] << 24);
        const UINT32 uncomp = data[pos + 22] | (data[pos + 23] << 8) | (data[pos + 24] << 16) | (data[pos + 25] << 24);
        const UINT16 nameLen = static_cast<UINT16>(data[pos + 26] | (data[pos + 27] << 8));
        const UINT16 extraLen = static_cast<UINT16>(data[pos + 28] | (data[pos + 29] << 8));
        const size_t nameAt = pos + 30;
        if (nameAt + nameLen + extraLen > size)
        {
            break;
        }
        std::string name(reinterpret_cast<const char*>(data + nameAt), nameLen);
        for (char& c : name)
        {
            if (c >= 'A' && c <= 'Z')
            {
                c = static_cast<char>(c - 'A' + 'a');
            }
            if (c == '\\')
            {
                c = '/';
            }
        }
        const size_t dataAt = nameAt + nameLen + extraLen;
        const size_t dataSize = comp != 0 ? static_cast<size_t>(comp) : static_cast<size_t>(uncomp);
        if (dataAt + dataSize > size)
        {
            break;
        }
        if (name == "metadata/thumbnail.png" || name.find("3d/3dmodel") != std::string::npos ||
            name == "[content_types].xml")
        {
            saw3mf = true;
        }
        if (name.find("metadata/thumbnails") != std::string::npos || endsWith(name, ".cdr"))
        {
            sawCdr = true;
        }
        const bool isPng = endsWith(name, ".png");
        const bool isJpeg = endsWith(name, ".jpg") || endsWith(name, ".jpeg");
        const bool isBmp = endsWith(name, ".bmp") || endsWith(name, ".dib");
        const bool isTiff = endsWith(name, ".tif") || endsWith(name, ".tiff");
        const bool isWmf = endsWith(name, ".wmf") || endsWith(name, ".emf");
        const bool isImage = isPng || isJpeg || isBmp || isTiff || isWmf;
        const bool nameHint = name.find("thumb") != std::string::npos || name.find("preview") != std::string::npos ||
                              name.find("thumbnail") != std::string::npos;
        const bool inPreviewDir = name.find("metadata/thumbnail") != std::string::npos ||
                                  name.find("/previews/") != std::string::npos || name.find("previews/") == 0;
        const bool thumb = isImage && (nameHint || inPreviewDir);
        pos = dataAt + dataSize;
        if (!thumb)
        {
            continue;
        }
        std::vector<BYTE> extracted;
        if (method == 0)
        {
            extracted.assign(data + dataAt, data + dataAt + dataSize);
        }
        else if (method == 8)
        {
            if (!InflateRaw(data + dataAt, dataSize, extracted, uncomp != 0 ? uncomp : kMaxFileBytes))
            {
                continue;
            }
        }
        else
        {
            continue;
        }
        if (extracted.size() < 8)
        {
            continue;
        }
        const size_t score = extracted.size();
        if (score > bestScore || (score == bestScore && isPng && !bestPng))
        {
            best = std::move(extracted);
            bestScore = score;
            bestPng = isPng;
        }
    }
    if (best.empty())
    {
        return false;
    }
    if (saw3mf)
    {
        format = PVF_3MF;
        formatLabel = "3MF";
    }
    else if (sawCdr)
    {
        format = PVF_CDR;
        formatLabel = "CDR";
    }
    bytes = std::move(best);
    return true;
}

Status DecodeRasterPreviewBytes(const BYTE* data, size_t size, Frame& frame)
{
    if (DecodePng8(data, size, frame) == Status::Ok)
    {
        return Status::Ok;
    }
    if (DecodePackedDib(data, size, frame) == Status::Ok)
    {
        return Status::Ok;
    }
    return RasterizeMetafile(data, size, frame);
}

Status DecodeSkp(Reader& reader, DecodedImage& image)
{
    std::vector<BYTE> preview;
    DWORD format = PVF_SKP;
    const char* label = "SKP";
    if (!ExtractZipPreviewBytes(reader.Data(), reader.Size(), preview, format, label))
    {
        return Status::Unsupported;
    }
    Frame frame;
    if (DecodeRasterPreviewBytes(preview.data(), preview.size(), frame) != Status::Ok)
    {
        return Status::Unsupported;
    }
    return FinishSingle(image, std::move(frame), format, label);
}

Status DecodeRiffPreview(Reader& reader, DecodedImage& image)
{
    const BYTE* data = reader.Data();
    const size_t size = reader.Size();
    if (data == nullptr || size < 12 || memcmp(data, "RIFF", 4) != 0)
    {
        return Status::Unsupported;
    }
    if (!(memcmp(data + 8, "CDR", 3) == 0 || memcmp(data + 8, "cdr", 3) == 0 || memcmp(data + 8, "CMX", 3) == 0))
    {
        return Status::Unsupported;
    }
    Frame best;
    bool found = false;
    size_t pos = 12;
    while (pos + 8 <= size)
    {
        const UINT32 chunkSize = data[pos + 4] | (data[pos + 5] << 8) | (data[pos + 6] << 16) | (data[pos + 7] << 24);
        const size_t payloadAt = pos + 8;
        if (payloadAt > size)
        {
            break;
        }
        const size_t payloadSize = (std::min)(static_cast<size_t>(chunkSize), size - payloadAt);
        Frame frame;
        if (payloadSize >= 8 && DecodeRasterPreviewBytes(data + payloadAt, payloadSize, frame) == Status::Ok &&
            (!found || frame.width * frame.height > best.width * best.height))
        {
            best = std::move(frame);
            found = true;
        }
        else
        {
            EmbeddedPreview preview;
            if (FindEmbeddedPreview(data + payloadAt, payloadSize, preview) &&
                preview.offset + preview.size <= payloadSize)
            {
                if (DecodeRasterPreviewBytes(data + payloadAt + preview.offset, preview.size, frame) == Status::Ok &&
                    (!found || frame.width * frame.height > best.width * best.height))
                {
                    best = std::move(frame);
                    found = true;
                }
            }
        }
        const size_t step = 8ull + static_cast<size_t>(chunkSize) + (chunkSize & 1u);
        if (step < 8 || pos + step <= pos)
        {
            break;
        }
        pos += step;
    }
    if (!found)
    {
        EmbeddedPreview preview;
        if (!FindEmbeddedPreview(data, size, preview) || preview.offset + preview.size > size)
        {
            return Status::Unsupported;
        }
        Frame frame;
        if (DecodeRasterPreviewBytes(data + preview.offset, preview.size, frame) != Status::Ok)
        {
            return Status::Unsupported;
        }
        return FinishSingle(image, std::move(frame), PVF_CDR, "CDR");
    }
    return FinishSingle(image, std::move(best), PVF_CDR, "CDR");
}

Status DecodeBlend(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 12 || !reader.StartsWith("BLENDER", 7))
    {
        return Status::Unsupported;
    }
    EmbeddedPreview preview;
    if (!FindEmbeddedPreview(reader.Data(), reader.Size(), preview))
    {
        return Status::Unsupported;
    }
    Frame frame;
    Status st = Status::Unsupported;
    const BYTE* slice = reader.Data() + preview.offset;
    if (preview.format == PVF_PNG)
    {
        st = DecodePng8(slice, preview.size, frame);
    }
    else if (preview.format == PVF_BMP)
    {
        st = DecodePackedDib(slice, preview.size, frame);
    }
    if (st != Status::Ok)
    {
        return Status::Unsupported;
    }
    return FinishSingle(image, std::move(frame), PVF_BLEND, "BLEND");
}

} // namespace Detail

bool ExtractZipEmbeddedPreview(const BYTE* data, size_t size, std::vector<BYTE>& bytes, DWORD& format,
                               const char*& formatLabel)
{
    return Detail::ExtractZipPreviewBytes(data, size, bytes, format, formatLabel);
}

} // namespace PictView::Native
