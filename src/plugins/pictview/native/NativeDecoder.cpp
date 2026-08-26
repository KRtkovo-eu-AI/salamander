// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cstring>

namespace PictView::Native
{
namespace
{

void AddMask(std::vector<std::string>& masks, const char* mask)
{
    masks.emplace_back(mask);
}

Status ReadEntireFile(const wchar_t* path, std::vector<BYTE>& bytes)
{
    if (path == nullptr || path[0] == 0)
    {
        return Status::CannotOpen;
    }
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return Status::CannotOpen;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || static_cast<ULONGLONG>(size.QuadPart) > Detail::kMaxFileBytes)
    {
        CloseHandle(file);
        return size.QuadPart == 0 ? Status::Invalid : Status::OutOfMemory;
    }
    try
    {
        bytes.resize(static_cast<size_t>(size.QuadPart));
    }
    catch (const std::bad_alloc&)
    {
        CloseHandle(file);
        return Status::OutOfMemory;
    }
    DWORD read = 0;
    size_t offset = 0;
    while (offset < bytes.size())
    {
        const DWORD chunk = static_cast<DWORD>(std::min(bytes.size() - offset, static_cast<size_t>(1024 * 1024)));
        if (!ReadFile(file, bytes.data() + offset, chunk, &read, nullptr) || read == 0)
        {
            CloseHandle(file);
            return Status::CannotOpen;
        }
        offset += read;
    }
    CloseHandle(file);
    return Status::Ok;
}

} // namespace

void GetDecoderMasks(std::vector<std::string>& masks)
{
    masks.clear();
    // Wave 1
    AddMask(masks, "*.tga");
    AddMask(masks, "*.pcx");
    AddMask(masks, "*.pbm");
    AddMask(masks, "*.pgm");
    AddMask(masks, "*.ppm");
    AddMask(masks, "*.pnm");
    AddMask(masks, "*.ras");
    AddMask(masks, "*.sun");
    AddMask(masks, "*.sgi");
    AddMask(masks, "*.bw");
    AddMask(masks, "*.rgb");
    AddMask(masks, "*.wbmp");
    AddMask(masks, "*.jff");
    AddMask(masks, "*.jif");
    AddMask(masks, "*.thm");
    AddMask(masks, "*.thumb");
    AddMask(masks, "*.svg");
    // Wave 2
    AddMask(masks, "*.iff");
    AddMask(masks, "*.lbm");
    AddMask(masks, "*.ani");
    AddMask(masks, "*.cur");
    AddMask(masks, "*.dcx");
    AddMask(masks, "*.psd");
    AddMask(masks, "*.fli");
    AddMask(masks, "*.flc");
    AddMask(masks, "*.dtx");
    AddMask(masks, "*.dds");
    // Wave 3
    AddMask(masks, "*.eps");
    AddMask(masks, "*.ept");
    AddMask(masks, "*.ai");
    AddMask(masks, "*.mov");
    AddMask(masks, "*.hpi");
    AddMask(masks, "*.cdr");
    AddMask(masks, "*.cdt");
    AddMask(masks, "*.cmx");
    AddMask(masks, "*.xar");
    AddMask(masks, "*.web");
    AddMask(masks, "*.psp*");
    AddMask(masks, "*.zbr");
    AddMask(masks, "*.zmf");
    AddMask(masks, "*.zno");
    AddMask(masks, "*.xcf");
    AddMask(masks, "*.pdn");
    AddMask(masks, "*.3dm");
    AddMask(masks, "*.dwg");
    AddMask(masks, "*.skp");
    AddMask(masks, "*.blend");
    AddMask(masks, "*.wmf");
    AddMask(masks, "*.emf");
    AddMask(masks, "*.stl");
}

Status DecodeMemory(const BYTE* data, size_t size, DecodedImage& image)
{
    image = {};
    if (data == nullptr || size == 0)
    {
        return Status::Unsupported;
    }

    if (Detail::LooksLikePsOrPdf(data, size))
    {
        Detail::Reader reader(data, size);
        return Detail::DecodePreview(reader, image);
    }

    using DecoderFn = Status (*)(Detail::Reader&, DecodedImage&);
    const DecoderFn decoders[] = {
        Detail::DecodeDds,
        Detail::DecodePsd,
        Detail::DecodeRas,
        Detail::DecodeSgi,
        Detail::DecodeDcx,
        Detail::DecodePcx,
        Detail::DecodePnm,
        Detail::DecodeIff,
        Detail::DecodeAni,
        Detail::DecodeIcoCur,
        Detail::DecodeFli,
        Detail::DecodeSvg,
        Detail::DecodeWmf,
        Detail::DecodeTga,
        Detail::DecodeWbmp,
        Detail::DecodePdn,
        Detail::DecodeXcf,
        Detail::Decode3dm,
        Detail::DecodeDwg,
        Detail::DecodeSkp,
        Detail::DecodeBlend,
        Detail::DecodePreview,
    };

    for (DecoderFn decoder : decoders)
    {
        Detail::Reader reader(data, size);
        DecodedImage candidate;
        const Status st = decoder(reader, candidate);
        if (st == Status::Ok && !candidate.frames.empty())
        {
            image = std::move(candidate);
            return Status::Ok;
        }
        if (st == Status::OutOfMemory)
        {
            return st;
        }
    }
    return Status::Unsupported;
}

Status DecodeFile(const wchar_t* path, DecodedImage& image)
{
    std::vector<BYTE> bytes;
    const Status st = ReadEntireFile(path, bytes);
    if (st != Status::Ok)
    {
        return st;
    }
    return DecodeMemory(bytes.data(), bytes.size(), image);
}

bool FindEmbeddedPreview(const BYTE* data, size_t size, EmbeddedPreview& preview)
{
    preview = {};
    if (data == nullptr || size < 8)
    {
        return false;
    }

    auto jpegEnd = [](const BYTE* bytes, size_t total, size_t start) -> size_t {
        const size_t cap = start + (16ull * 1024ull * 1024ull);
        const size_t limit = cap < total ? cap : total;
        size_t i = start + 2;
        bool hasSof = false;
        while (i + 1 < limit)
        {
            if (bytes[i] != 0xFF)
            {
                ++i;
                continue;
            }
            const BYTE marker = bytes[i + 1];
            if (marker == 0xD9)
            {
                return hasSof ? i + 2 : 0;
            }
            if (marker >= 0xC0 && marker <= 0xC3)
            {
                hasSof = true;
            }
            if (marker == 0xD8 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7) || marker == 0xFF)
            {
                i += 2;
                continue;
            }
            if (i + 3 >= limit)
            {
                break;
            }
            const UINT16 length = static_cast<UINT16>((bytes[i + 2] << 8) | bytes[i + 3]);
            if (length < 2)
            {
                break;
            }
            i += 2u + length;
        }
        return 0;
    };

    auto consider = [&](size_t offset, size_t bytes, DWORD format, const char* label) {
        if (bytes > preview.size)
        {
            preview.offset = offset;
            preview.size = bytes;
            preview.format = format;
            preview.formatLabel = label;
        }
    };

    if (size >= 30 && data[0] == 0xC5 && data[1] == 0xD0 && data[2] == 0xD3 && data[3] == 0xC6)
    {
        const UINT32 tiffOffset = data[20] | (data[21] << 8) | (data[22] << 16) | (data[23] << 24);
        const UINT32 tiffLength = data[24] | (data[25] << 8) | (data[26] << 16) | (data[27] << 24);
        if (tiffOffset != 0 && tiffLength >= 8 && static_cast<size_t>(tiffOffset) + tiffLength <= size)
        {
            preview.offset = tiffOffset;
            preview.size = tiffLength;
            preview.format = PVF_EPS;
            preview.formatLabel = "EPS";
            return true;
        }
        const UINT32 wmfOffset = data[12] | (data[13] << 8) | (data[14] << 16) | (data[15] << 24);
        const UINT32 wmfLength = data[16] | (data[17] << 8) | (data[18] << 16) | (data[19] << 24);
        if (wmfOffset != 0 && wmfLength >= 8 && static_cast<size_t>(wmfOffset) + wmfLength <= size)
        {
            preview.offset = wmfOffset;
            preview.size = wmfLength;
            preview.format = PVF_EPS;
            preview.formatLabel = "EPS";
            return true;
        }
    }

    const bool pdfOrPs = (size >= 5 && memcmp(data, "%PDF-", 5) == 0) || (size >= 2 && data[0] == '%' && data[1] == '!');
    if (pdfOrPs)
    {
        const char* tag = "/DCTDecode";
        const size_t tagLen = 10;
        for (size_t i = 0; i + tagLen + 8 < size;)
        {
            const BYTE* slash = static_cast<const BYTE*>(memchr(data + i, '/', size - i));
            if (slash == nullptr)
            {
                break;
            }
            i = static_cast<size_t>(slash - data);
            if (i + tagLen + 8 >= size)
            {
                break;
            }
            if (memcmp(data + i, tag, tagLen) != 0)
            {
                ++i;
                continue;
            }
            size_t pos = i + tagLen;
            while (pos + 6 < size && memcmp(data + pos, "stream", 6) != 0)
            {
                ++pos;
                if (pos > i + 2048)
                {
                    break;
                }
            }
            if (pos + 6 >= size || memcmp(data + pos, "stream", 6) != 0)
            {
                i += tagLen;
                continue;
            }
            pos += 6;
            if (pos < size && data[pos] == '\r')
            {
                ++pos;
            }
            if (pos < size && data[pos] == '\n')
            {
                ++pos;
            }
            const size_t soiLimit = (std::min)(pos + 16, size);
            while (pos + 1 < soiLimit && !(data[pos] == 0xFF && data[pos + 1] == 0xD8))
            {
                ++pos;
            }
            if (pos + 3 < size && data[pos] == 0xFF && data[pos + 1] == 0xD8)
            {
                const size_t end = jpegEnd(data, size, pos);
                if (end > pos + 8)
                {
                    consider(pos, end - pos, PVF_JPG, "JPEG");
                    i = end;
                    continue;
                }
            }
            i += tagLen;
        }
        return preview.size >= 8;
    }

    const bool rhino3dm = size >= 23 && memcmp(data, "3D Geometry File Format", 23) == 0;
    const size_t limit = size;
    for (size_t i = 0; i + 8 < limit; ++i)
    {
        if (data[i] == 0xFF && data[i + 1] == 0xD8 && data[i + 2] == 0xFF)
        {
            const size_t end = jpegEnd(data, size, i);
            if (end > i + 8)
            {
                consider(i, end - i, PVF_JPG, "JPEG");
                i = end - 1;
            }
            else
            {
                i += 2;
            }
        }
        else if (data[i] == 0x89 && i + 24 < size && memcmp(data + i, "\x89PNG\r\n\x1a\n", 8) == 0)
        {
            const size_t remaining = size - i;
            if (remaining <= 16ull * 1024ull * 1024ull)
            {
                consider(i, remaining, PVF_PNG, "PNG");
            }
        }
        else if (data[i] == 'B' && data[i + 1] == 'M' && i + 14 < size)
        {
            const UINT32 fileSize = data[i + 2] | (data[i + 3] << 8) | (data[i + 4] << 16) | (data[i + 5] << 24);
            if (fileSize >= 14 && fileSize <= 16u * 1024u * 1024u && i + fileSize <= size)
            {
                consider(i, fileSize, PVF_BMP, "BMP");
            }
        }
        else if (!rhino3dm && Detail::LooksLikePlaceableWmf(data + i, size - i))
        {
            size_t bytes = 0;
            if (i + 28 <= size)
            {
                const UINT32 words = data[i + 22 + 6] | (data[i + 22 + 7] << 8) | (data[i + 22 + 8] << 16) |
                                     (data[i + 22 + 9] << 24);
                const size_t wmfBytes = 22ull + static_cast<size_t>(words) * 2ull;
                if (words >= 9 && wmfBytes >= 40 && wmfBytes <= 8ull * 1024ull * 1024ull && i + wmfBytes <= size)
                {
                    bytes = wmfBytes;
                }
            }
            if (bytes >= 40)
            {
                consider(i, bytes, PVF_WMF, "WMF");
            }
        }
        else if (!rhino3dm && Detail::LooksLikeEmf(data + i, size - i))
        {
            const UINT32 nBytes = data[i + 48] | (data[i + 49] << 8) | (data[i + 50] << 16) | (data[i + 51] << 24);
            if (nBytes >= 88 && nBytes <= 8u * 1024u * 1024u && i + nBytes <= size)
            {
                consider(i, nBytes, PVF_EMF, "EMF");
            }
        }
        else if (!rhino3dm && ((data[i] == 'I' && data[i + 1] == 'I' && data[i + 2] == 42 && data[i + 3] == 0) ||
                              (data[i] == 'M' && data[i + 1] == 'M' && data[i + 2] == 0 && data[i + 3] == 42)))
        {
            const size_t remaining = size - i;
            if (remaining >= 8 && remaining <= 16ull * 1024ull * 1024ull)
            {
                consider(i, remaining, PVF_TIFF, "TIFF");
            }
        }
    }
    return preview.size >= 8;
}

} // namespace PictView::Native
