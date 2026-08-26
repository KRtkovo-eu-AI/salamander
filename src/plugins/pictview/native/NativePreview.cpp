// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace PictView::Native
{
namespace Detail
{
namespace
{

Status DecodeEpsAsciiPreview(const BYTE* data, size_t size, DecodedImage& image)
{
    const char* beginTag = "%%BeginPreview:";
    const size_t tagLen = strlen(beginTag);
    for (size_t i = 0; i + tagLen < size && i < 65536; ++i)
    {
        if (memcmp(data + i, beginTag, tagLen) != 0)
        {
            continue;
        }
        size_t pos = i + tagLen;
        auto skipSpace = [&]() {
            while (pos < size && (data[pos] == ' ' || data[pos] == '\t'))
            {
                ++pos;
            }
        };
        auto readInt = [&](UINT& value) -> bool {
            skipSpace();
            if (pos >= size || data[pos] < '0' || data[pos] > '9')
            {
                return false;
            }
            value = 0;
            while (pos < size && data[pos] >= '0' && data[pos] <= '9')
            {
                value = value * 10 + (data[pos++] - '0');
                if (value > kMaxDimension)
                {
                    return false;
                }
            }
            return value > 0;
        };
        UINT width = 0;
        UINT height = 0;
        UINT depth = 0;
        UINT lines = 0;
        if (!readInt(width) || !readInt(height) || !readInt(depth) || !readInt(lines))
        {
            return Status::Unsupported;
        }
        while (pos < size && data[pos] != '\n')
        {
            ++pos;
        }
        if (pos < size)
        {
            ++pos;
        }
        Frame frame;
        Status st = MakeFrame(frame, width, height, depth, depth <= 8 ? (1u << depth) : PV_COLOR_TC24);
        if (st != Status::Ok)
        {
            return st;
        }
        std::vector<BYTE> hex;
        const UINT rowBytes = (width * std::max(1u, depth) + 7) / 8;
        hex.reserve(static_cast<size_t>(rowBytes) * height);
        auto hexVal = [](BYTE c) -> int {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            return -1;
        };
        UINT decodedLines = 0;
        while (pos < size && decodedLines < height)
        {
            if (pos + 14 <= size && memcmp(data + pos, "%%EndPreview", 12) == 0)
            {
                break;
            }
            int hi = -1;
            while (pos < size)
            {
                const int value = hexVal(data[pos++]);
                if (value >= 0)
                {
                    if (hi < 0)
                    {
                        hi = value;
                    }
                    else
                    {
                        hex.push_back(static_cast<BYTE>((hi << 4) | value));
                        hi = -1;
                        if (rowBytes > 0 && hex.size() % rowBytes == 0)
                        {
                            ++decodedLines;
                            break;
                        }
                    }
                }
                else if (data[pos - 1] == '\n' || data[pos - 1] == '\r')
                {
                    break;
                }
            }
        }
        for (UINT y = 0; y < height; ++y)
        {
            const UINT srcY = height - 1 - y;
            if (static_cast<size_t>(srcY + 1) * rowBytes > hex.size())
            {
                break;
            }
            const BYTE* row = hex.data() + static_cast<size_t>(srcY) * rowBytes;
            for (UINT x = 0; x < width; ++x)
            {
                BYTE g = 255;
                if (depth == 1)
                {
                    const BYTE bit = (row[x / 8] >> (7 - (x % 8))) & 1;
                    g = bit ? 0 : 255;
                }
                else if (depth == 8 && x < rowBytes)
                {
                    g = row[x];
                }
                SetPixel(frame, x, y, g, g, g, 255);
            }
        }
        (void)lines;
        image.format = PVF_EPS;
        image.formatLabel = "EPS";
        image.frames.clear();
        image.frames.push_back(std::move(frame));
        return Status::Ok;
    }
    return Status::Unsupported;
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

void SkipPdfWs(const BYTE* data, size_t size, size_t& pos)
{
    while (pos < size)
    {
        const BYTE c = data[pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\0' || c == '\f')
        {
            ++pos;
            continue;
        }
        if (c == '%')
        {
            while (pos < size && data[pos] != '\n' && data[pos] != '\r')
            {
                ++pos;
            }
            continue;
        }
        break;
    }
}

bool ReadPdfInt(const BYTE* data, size_t size, size_t& pos, int& value)
{
    SkipPdfWs(data, size, pos);
    if (pos >= size)
    {
        return false;
    }
    const bool negative = data[pos] == '-';
    if (data[pos] == '+' || data[pos] == '-')
    {
        ++pos;
    }
    if (pos >= size || data[pos] < '0' || data[pos] > '9')
    {
        return false;
    }
    int parsed = 0;
    while (pos < size && data[pos] >= '0' && data[pos] <= '9')
    {
        parsed = parsed * 10 + (data[pos++] - '0');
        if (parsed > 100000000)
        {
            return false;
        }
    }
    value = negative ? -parsed : parsed;
    return true;
}

constexpr UINT kMaxPdfPreviewEdge = 4096;
constexpr size_t kMaxPdfPayload = 8ull * 1024ull * 1024ull;
constexpr int kMaxPdfInflateAttempts = 12;

bool PdfLengthIsIndirect(const BYTE* dict, size_t dictSize, size_t pos)
{
    SkipPdfWs(dict, dictSize, pos);
    if (pos >= dictSize || dict[pos] < '0' || dict[pos] > '9')
    {
        return false;
    }
    while (pos < dictSize && dict[pos] >= '0' && dict[pos] <= '9')
    {
        ++pos;
    }
    SkipPdfWs(dict, dictSize, pos);
    return pos < dictSize && dict[pos] == 'R';
}

bool FindPdfKeyInt(const BYTE* dict, size_t dictSize, const char* key, int& value, bool* indirect = nullptr)
{
    const size_t keyLen = strlen(key);
    if (indirect)
    {
        *indirect = false;
    }
    for (size_t i = 0; i + keyLen < dictSize; ++i)
    {
        if (memcmp(dict + i, key, keyLen) != 0)
        {
            continue;
        }
        size_t pos = i + keyLen;
        int parsed = 0;
        if (!ReadPdfInt(dict, dictSize, pos, parsed) || parsed <= 0)
        {
            continue;
        }
        if (PdfLengthIsIndirect(dict, dictSize, pos))
        {
            if (indirect)
            {
                *indirect = true;
            }
            return false;
        }
        value = parsed;
        return true;
    }
    return false;
}

bool DictHas(const BYTE* dict, size_t dictSize, const char* token)
{
    const size_t tokenLen = strlen(token);
    if (tokenLen == 0 || dictSize < tokenLen)
    {
        return false;
    }
    for (size_t i = 0; i + tokenLen <= dictSize; ++i)
    {
        if (memcmp(dict + i, token, tokenLen) == 0)
        {
            return true;
        }
    }
    return false;
}

bool ApplyPdfPredictor(std::vector<BYTE>& raw, UINT width, UINT height, int channels, int predictor)
{
    if (predictor <= 1)
    {
        return raw.size() >= static_cast<size_t>(width) * height * static_cast<size_t>(channels);
    }
    const size_t stride = static_cast<size_t>(width) * static_cast<size_t>(channels);
    if (predictor == 2)
    {
        if (raw.size() < stride * height)
        {
            return false;
        }
        for (UINT y = 0; y < height; ++y)
        {
            BYTE* row = raw.data() + static_cast<size_t>(y) * stride;
            for (size_t i = static_cast<size_t>(channels); i < stride; ++i)
            {
                row[i] = static_cast<BYTE>(row[i] + row[i - static_cast<size_t>(channels)]);
            }
        }
        return true;
    }
    if (predictor < 10 || predictor > 15)
    {
        return false;
    }
    if (raw.size() < (stride + 1) * height)
    {
        return false;
    }
    std::vector<BYTE> recon;
    recon.resize(stride * height);
    std::vector<BYTE> prev(stride, 0);
    size_t src = 0;
    for (UINT y = 0; y < height; ++y)
    {
        const BYTE filter = raw[src++];
        BYTE* dest = recon.data() + static_cast<size_t>(y) * stride;
        memcpy(dest, raw.data() + src, stride);
        src += stride;
        for (size_t i = 0; i < stride; ++i)
        {
            const BYTE a = i >= static_cast<size_t>(channels) ? dest[i - static_cast<size_t>(channels)] : 0;
            const BYTE b = prev[i];
            const BYTE c = i >= static_cast<size_t>(channels) ? prev[i - static_cast<size_t>(channels)] : 0;
            BYTE value = dest[i];
            switch (filter)
            {
            case 1:
                value = static_cast<BYTE>(value + a);
                break;
            case 2:
                value = static_cast<BYTE>(value + b);
                break;
            case 3:
                value = static_cast<BYTE>(value + ((a + b) / 2));
                break;
            case 4:
                value = static_cast<BYTE>(value + Paeth(a, b, c));
                break;
            case 0:
                break;
            default:
                return false;
            }
            dest[i] = value;
        }
        memcpy(prev.data(), dest, stride);
    }
    raw.swap(recon);
    return true;
}

Status DecodePdfFlateImages(const BYTE* data, size_t size, DecodedImage& image)
{
    Frame best;
    bool found = false;
    int inflates = 0;
    for (size_t i = 0; i + 16 < size;)
    {
        const size_t remaining = size - i;
        const BYTE* slash = static_cast<const BYTE*>(memchr(data + i, '/', remaining));
        if (slash == nullptr)
        {
            break;
        }
        i = static_cast<size_t>(slash - data);
        if (i + 16 >= size)
        {
            break;
        }
        if (memcmp(data + i, "/Subtype", 8) != 0)
        {
            ++i;
            continue;
        }
        size_t pos = i + 8;
        SkipPdfWs(data, size, pos);
        if (pos < size && data[pos] == '/')
        {
            ++pos;
        }
        if (pos + 5 > size || memcmp(data + pos, "Image", 5) != 0)
        {
            ++i;
            continue;
        }
        if (pos + 9 <= size && memcmp(data + pos, "ImageMask", 9) == 0)
        {
            ++i;
            continue;
        }
        size_t streamAt = pos;
        while (streamAt + 6 < size && memcmp(data + streamAt, "stream", 6) != 0)
        {
            ++streamAt;
            if (streamAt > i + 4096)
            {
                break;
            }
        }
        if (streamAt + 6 >= size || memcmp(data + streamAt, "stream", 6) != 0)
        {
            ++i;
            continue;
        }
        size_t dictFrom = i;
        if (i >= 2)
        {
            size_t look = i;
            const size_t minLook = i > 2048 ? i - 2048 : 0;
            while (look > minLook + 1)
            {
                --look;
                if (data[look] == '<' && data[look + 1] == '<')
                {
                    dictFrom = look;
                    break;
                }
            }
        }
        const BYTE* dict = data + dictFrom;
        const size_t dictSize = streamAt - dictFrom;
        if (DictHas(dict, dictSize, "/DCTDecode") || DictHas(dict, dictSize, "/CCITTFaxDecode") ||
            DictHas(dict, dictSize, "/JBIG2Decode") || DictHas(dict, dictSize, "/JPXDecode"))
        {
            i = streamAt + 6;
            continue;
        }
        if (!DictHas(dict, dictSize, "/FlateDecode"))
        {
            i = streamAt + 6;
            continue;
        }
        int width = 0;
        int height = 0;
        int bits = 8;
        int length = 0;
        int predictor = 1;
        int colors = 0;
        bool lengthIndirect = false;
        if (!FindPdfKeyInt(dict, dictSize, "/Width", width) || !FindPdfKeyInt(dict, dictSize, "/Height", height))
        {
            i = streamAt + 6;
            continue;
        }
        FindPdfKeyInt(dict, dictSize, "/BitsPerComponent", bits);
        FindPdfKeyInt(dict, dictSize, "/Length", length, &lengthIndirect);
        FindPdfKeyInt(dict, dictSize, "/Predictor", predictor);
        FindPdfKeyInt(dict, dictSize, "/Colors", colors);
        if (bits != 8 || width <= 0 || height <= 0 ||
            static_cast<UINT>(width) > kMaxPdfPreviewEdge || static_cast<UINT>(height) > kMaxPdfPreviewEdge)
        {
            i = streamAt + 6;
            continue;
        }
        const UINT pixels = static_cast<UINT>(width) * static_cast<UINT>(height);
        if (found && pixels <= best.width * best.height)
        {
            i = streamAt + 6;
            continue;
        }
        int channels = 0;
        if (DictHas(dict, dictSize, "/DeviceRGB"))
        {
            channels = 3;
        }
        else if (DictHas(dict, dictSize, "/DeviceGray"))
        {
            channels = 1;
        }
        else
        {
            i = streamAt + 6;
            continue;
        }
        if (colors == 1 || colors == 3)
        {
            channels = colors;
        }
        pos = streamAt + 6;
        if (pos < size && data[pos] == '\r')
        {
            ++pos;
        }
        if (pos < size && data[pos] == '\n')
        {
            ++pos;
        }
        size_t payloadSize = 0;
        if (!lengthIndirect && length > 0 && static_cast<size_t>(length) <= kMaxPdfPayload &&
            pos + static_cast<size_t>(length) <= size)
        {
            payloadSize = static_cast<size_t>(length);
        }
        else
        {
            size_t end = pos;
            while (end + 9 < size && memcmp(data + end, "endstream", 9) != 0)
            {
                ++end;
                if (end > pos + kMaxPdfPayload)
                {
                    break;
                }
            }
            if (end + 9 >= size || memcmp(data + end, "endstream", 9) != 0 || end <= pos)
            {
                i = streamAt + 6;
                continue;
            }
            payloadSize = end - pos;
        }
        if (payloadSize == 0 || payloadSize > kMaxPdfPayload)
        {
            i = pos + payloadSize;
            if (i <= streamAt)
            {
                i = streamAt + 6;
            }
            continue;
        }
        if (inflates >= kMaxPdfInflateAttempts)
        {
            break;
        }
        ++inflates;
        std::vector<BYTE> inflated;
        if (!InflateZlib(data + pos, payloadSize, inflated, kMaxFileBytes) &&
            !InflateRaw(data + pos, payloadSize, inflated, kMaxFileBytes))
        {
            i = pos + payloadSize;
            continue;
        }
        if (!ApplyPdfPredictor(inflated, static_cast<UINT>(width), static_cast<UINT>(height), channels, predictor))
        {
            i = pos + payloadSize;
            continue;
        }
        const size_t needed = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(channels);
        if (inflated.size() < needed)
        {
            i = pos + payloadSize;
            continue;
        }
        Frame frame;
        Status st = MakeFrame(frame, static_cast<UINT>(width), static_cast<UINT>(height), 32, PV_COLOR_TC32);
        if (st != Status::Ok)
        {
            return st;
        }
        for (int y = 0; y < height; ++y)
        {
            const BYTE* row = inflated.data() + static_cast<size_t>(y) * static_cast<size_t>(width) * channels;
            for (int x = 0; x < width; ++x)
            {
                const BYTE* p = row + static_cast<size_t>(x) * channels;
                if (channels == 1)
                {
                    SetPixel(frame, static_cast<UINT>(x), static_cast<UINT>(y), p[0], p[0], p[0], 255);
                }
                else
                {
                    SetPixel(frame, static_cast<UINT>(x), static_cast<UINT>(y), p[2], p[1], p[0], 255);
                }
            }
        }
        if (!found || frame.width * frame.height > best.width * best.height)
        {
            best = std::move(frame);
            found = true;
        }
        i = pos + payloadSize;
        if (i <= streamAt)
        {
            i = streamAt + 6;
        }
    }
    if (!found)
    {
        return Status::Unsupported;
    }
    const bool pdf = size >= 5 && memcmp(data, "%PDF-", 5) == 0;
    return FinishSingle(image, std::move(best), PVF_EPS, pdf ? "PDF" : "EPS");
}

} // namespace

Status DecodePreview(Reader& reader, DecodedImage& image)
{
    const BYTE* data = reader.Data();
    const size_t size = reader.Size();
    if (data == nullptr || size < 4 || !LooksLikePsOrPdf(data, size))
    {
        return Status::Unsupported;
    }

    size_t psOffset = 0;
    size_t psSize = size;
    const bool dosEps = size >= 30 && data[0] == 0xC5 && data[1] == 0xD0 && data[2] == 0xD3 && data[3] == 0xC6;
    if (dosEps)
    {
        const UINT32 headerPsOffset = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);
        const UINT32 headerPsLength = data[8] | (data[9] << 8) | (data[10] << 16) | (data[11] << 24);
        const UINT32 wmfOffset = data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24);
        const UINT32 wmfLength = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);
        const UINT32 tiffOffset = data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24);
        const UINT32 tiffLength = data[24] | (data[25] << 8) | (data[26] << 16) | (data[27] << 24);

        if (wmfOffset != 0 && wmfLength >= 8 && static_cast<size_t>(wmfOffset) + wmfLength <= size)
        {
            Frame frame;
            const Status st = RasterizeMetafile(data + wmfOffset, wmfLength, frame);
            if (st == Status::Ok)
            {
                return FinishSingle(image, std::move(frame), PVF_EPS, "EPS");
            }
            if (st == Status::OutOfMemory)
            {
                return st;
            }
        }

        if (tiffOffset != 0 && tiffLength >= 8 && static_cast<size_t>(tiffOffset) + tiffLength <= size)
        {
            const BYTE* slice = data + tiffOffset;
            if (LooksLikePlaceableWmf(slice, tiffLength) || LooksLikeEmf(slice, tiffLength) ||
                LooksLikeStandardWmf(slice, tiffLength))
            {
                Frame frame;
                const Status st = RasterizeMetafile(slice, tiffLength, frame);
                if (st == Status::Ok)
                {
                    return FinishSingle(image, std::move(frame), PVF_EPS, "EPS");
                }
                if (st == Status::OutOfMemory)
                {
                    return st;
                }
            }
        }

        if (headerPsOffset != 0 && static_cast<size_t>(headerPsOffset) < size)
        {
            psOffset = headerPsOffset;
            psSize = headerPsLength != 0 ? static_cast<size_t>(headerPsLength) : (size - psOffset);
            if (psOffset + psSize > size)
            {
                psSize = size - psOffset;
            }
        }
        else if (size > 30)
        {
            psOffset = 30;
            psSize = size - 30;
        }
    }

    const Status ascii = DecodeEpsAsciiPreview(data + psOffset, psSize, image);
    if (ascii == Status::Ok || ascii == Status::OutOfMemory)
    {
        return ascii;
    }

    const BYTE* flateData = dosEps ? (data + psOffset) : data;
    const size_t flateSize = dosEps ? psSize : size;
    return DecodePdfFlateImages(flateData, flateSize, image);
}

} // namespace Detail
} // namespace PictView::Native
