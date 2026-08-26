// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../plugins/pictview/native/NativeDecoder.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace
{

int Fail(const char* message)
{
    std::cerr << message << '\n';
    return 1;
}

UINT32 Checksum(const PictView::Native::Frame& frame)
{
    UINT32 hash = 2166136261u;
    for (BYTE value : frame.bgra)
    {
        hash ^= value;
        hash *= 16777619u;
    }
    return hash;
}

int ExpectDecoded(const char* name, const std::vector<BYTE>& bytes, DWORD format)
{
    PictView::Native::DecodedImage image;
    const PictView::Native::Status status =
        PictView::Native::DecodeMemory(bytes.data(), bytes.size(), image);
    if (status != PictView::Native::Status::Ok)
    {
        std::cerr << name << ": decode failed (" << static_cast<int>(status) << ")\n";
        return 1;
    }
    if (image.format != format)
    {
        std::cerr << name << ": unexpected format " << image.format << "\n";
        return 1;
    }
    if (image.frames.empty() || image.frames[0].width == 0 || image.frames[0].height == 0)
    {
        std::cerr << name << ": empty frame\n";
        return 1;
    }
    std::cout << name << ": " << image.frames[0].width << "x" << image.frames[0].height
              << " checksum=" << Checksum(image.frames[0]) << "\n";
    return 0;
}

int ExpectImage(const char* name, const std::vector<BYTE>& bytes, DWORD format, UINT width, UINT height)
{
    PictView::Native::DecodedImage image;
    const PictView::Native::Status status =
        PictView::Native::DecodeMemory(bytes.data(), bytes.size(), image);
    if (status != PictView::Native::Status::Ok)
    {
        std::cerr << name << ": decode failed (" << static_cast<int>(status) << ")\n";
        return 1;
    }
    if (image.format != format)
    {
        std::cerr << name << ": unexpected format " << image.format << "\n";
        return 1;
    }
    if (image.frames.empty() || image.frames[0].width != width || image.frames[0].height != height)
    {
        std::cerr << name << ": unexpected size\n";
        return 1;
    }
    if (Checksum(image.frames[0]) == 0 && image.frames[0].bgra.size() > 4)
    {
        std::cerr << name << ": empty checksum\n";
        return 1;
    }
    std::cout << name << ": " << width << "x" << height << " checksum=" << Checksum(image.frames[0]) << "\n";
    return 0;
}

std::vector<BYTE> MakeTga2x1()
{
    std::vector<BYTE> data(18 + 6, 0);
    data[2] = 2; // truecolor
    data[12] = 2; // width
    data[13] = 0;
    data[14] = 1; // height
    data[15] = 0;
    data[16] = 24;
    data[17] = 0x20; // top origin
    data[18] = 0; // B
    data[19] = 0;
    data[20] = 255; // R
    data[21] = 255; // B
    data[22] = 0;
    data[23] = 0;
    return data;
}

std::vector<BYTE> MakePnm2x1()
{
    const char* header = "P6\n2 1\n255\n";
    std::vector<BYTE> data(header, header + 11);
    data.insert(data.end(), {255, 0, 0, 0, 255, 0});
    return data;
}

std::vector<BYTE> MakeWbmp2x2()
{
    return {0x00, 0x00, 0x02, 0x02, 0xC0, 0xC0};
}

std::vector<BYTE> MakeRas2x1()
{
    std::vector<BYTE> data(32 + 8, 0);
    data[0] = 0x59;
    data[1] = 0xA6;
    data[2] = 0x6A;
    data[3] = 0x95;
    data[7] = 2; // width
    data[11] = 1; // height
    data[15] = 24;
    data[19] = 8; // length
    data[23] = 1; // RGB format
    data[32] = 255;
    data[33] = 0;
    data[34] = 0;
    data[35] = 0;
    data[36] = 255;
    data[37] = 0;
    return data;
}

std::vector<BYTE> MakeSgi2x1()
{
    std::vector<BYTE> data(512 + 6, 0);
    data[0] = 0x01;
    data[1] = 0xDA;
    data[2] = 0; // verbatim
    data[3] = 1; // bytes per channel
    data[5] = 3; // dimension
    data[7] = 2; // width
    data[9] = 1; // height
    data[11] = 3; // channels
    data[512] = 255; // R
    data[513] = 0;
    data[514] = 0; // G
    data[515] = 255;
    data[516] = 0; // B
    data[517] = 0;
    return data;
}

std::vector<BYTE> MakePcx2x1()
{
    std::vector<BYTE> data(128 + 6, 0);
    data[0] = 0x0A;
    data[1] = 5;
    data[2] = 1; // RLE
    data[3] = 8;
    data[8] = 1; // xmax
    data[10] = 0; // ymax
    data[65] = 3; // planes
    data[66] = 2; // bytes per line
    data[67] = 0;
    // RLE: 0xC1, value for each plane row of 2 bytes. bytesPerLine=2, planes=3.
    data[128] = 0xC2;
    data[129] = 255;
    data[130] = 0xC2;
    data[131] = 0;
    data[132] = 0xC2;
    data[133] = 0;
    return data;
}

std::vector<BYTE> MakeSvg2x2()
{
    const char* svg =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"2\" height=\"2\">"
        "<rect width=\"2\" height=\"2\" fill=\"#ff0000\"/>"
        "</svg>";
    return std::vector<BYTE>(svg, svg + strlen(svg));
}

std::vector<BYTE> MakePsd2x1()
{
    std::vector<BYTE> data(26 + 12 + 2 + 6, 0);
    memcpy(data.data(), "8BPS", 4);
    data[5] = 1; // version 1
    data[13] = 3; // channels
    data[17] = 1; // height
    data[21] = 2; // width
    data[23] = 8; // depth
    data[25] = 3; // RGB
    data[38] = 0;
    data[39] = 0; // compression
    data[40] = 255; // R plane
    data[41] = 0;
    data[42] = 0; // G plane
    data[43] = 0;
    data[44] = 0; // B plane
    data[45] = 0;
    return data;
}

std::vector<BYTE> MakeDds2x1()
{
    std::vector<BYTE> data(128 + 8, 0);
    memcpy(data.data(), "DDS ", 4);
    data[4] = 124;
    data[12] = 1; // height
    data[16] = 2; // width
    data[76] = 32; // pf size
    data[80] = 0x41; // RGB + alpha
    data[88] = 32; // bit count
    data[92] = 0x00;
    data[93] = 0x00;
    data[94] = 0xFF;
    data[95] = 0x00; // R
    data[96] = 0x00;
    data[97] = 0xFF;
    data[98] = 0x00;
    data[99] = 0x00; // G
    data[100] = 0xFF;
    data[101] = 0x00;
    data[102] = 0x00;
    data[103] = 0x00; // B
    data[104] = 0x00;
    data[105] = 0x00;
    data[106] = 0x00;
    data[107] = 0xFF; // A
    // two BGRA pixels
    data[128] = 0;
    data[129] = 0;
    data[130] = 255;
    data[131] = 255;
    data[132] = 0;
    data[133] = 255;
    data[134] = 0;
    data[135] = 255;
    return data;
}

std::vector<BYTE> MakeEpsPreview()
{
    const char* eps =
        "%!PS-Adobe-3.0 EPSF-3.0\n"
        "%%BeginPreview: 2 2 1 2\n"
        "C0\n"
        "30\n"
        "%%EndPreview\n"
        "showpage\n";
    return std::vector<BYTE>(eps, eps + strlen(eps));
}

std::vector<BYTE> MakeJpegBlob()
{
    // Minimal JPEG SOI/APP0/SOF/SOS/EOI is large; a truncated SOI is enough
    // for FindEmbeddedPreview to reject, so wrap a recognizable SOF.
    return {
        0x00, 0x00, 0xFF, 0xD8, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0x02, 0x00, 0x02, 0x01, 0x01, 0x11,
        0x00, 0xFF, 0xD9};
}

void PutBE32(std::vector<BYTE>& data, UINT32 value)
{
    data.push_back(static_cast<BYTE>(value >> 24));
    data.push_back(static_cast<BYTE>(value >> 16));
    data.push_back(static_cast<BYTE>(value >> 8));
    data.push_back(static_cast<BYTE>(value));
}

void PatchBE32(std::vector<BYTE>& data, size_t at, UINT32 value)
{
    data[at] = static_cast<BYTE>(value >> 24);
    data[at + 1] = static_cast<BYTE>(value >> 16);
    data[at + 2] = static_cast<BYTE>(value >> 8);
    data[at + 3] = static_cast<BYTE>(value);
}

std::vector<BYTE> MakePng1x1Red()
{
    const BYTE zlib[] = {0x78, 0x01, 0x01, 0x04, 0x00, 0xFB, 0xFF, 0x00, 0xFF, 0x00, 0x00, 0x03, 0x01, 0x01, 0x00};
    std::vector<BYTE> png = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    auto chunk = [&](const char* type, const BYTE* payload, UINT32 length) {
        PutBE32(png, length);
        png.insert(png.end(), type, type + 4);
        if (length > 0 && payload != nullptr)
        {
            png.insert(png.end(), payload, payload + length);
        }
        PutBE32(png, 0);
    };
    const BYTE ihdr[] = {0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x02, 0x00, 0x00, 0x00};
    chunk("IHDR", ihdr, 13);
    chunk("IDAT", zlib, static_cast<UINT32>(sizeof(zlib)));
    chunk("IEND", nullptr, 0);
    return png;
}

std::vector<BYTE> MakeDdsDx10Bc1()
{
    std::vector<BYTE> data(148 + 8, 0);
    memcpy(data.data(), "DDS ", 4);
    data[4] = 124;
    data[12] = 4;
    data[16] = 4;
    data[76] = 32;
    data[80] = 0x04;
    data[84] = 'D';
    data[85] = 'X';
    data[86] = '1';
    data[87] = '0';
    data[128] = 71;
    data[140] = 1;
    data[148] = 0x00;
    data[149] = 0xF8;
    data[150] = 0x00;
    data[151] = 0x00;
    return data;
}

std::vector<BYTE> MakePdfJpeg()
{
    const char* header = "%PDF-1.4\n1 0 obj<</Filter/DCTDecode>>\nstream\n";
    const char* footer = "\nendstream\nendobj\n";
    const auto jpeg = MakeJpegBlob();
    std::vector<BYTE> data(header, header + strlen(header));
    data.insert(data.end(), jpeg.begin(), jpeg.end());
    data.insert(data.end(), footer, footer + strlen(footer));
    return data;
}

std::vector<BYTE> MakePdnThumb()
{
    const auto png = MakePng1x1Red();
    static const char kTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string b64;
    int val = 0;
    int valb = -6;
    for (BYTE c : png)
    {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0)
        {
            b64.push_back(kTable[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        b64.push_back(kTable[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (b64.size() % 4 != 0)
    {
        b64.push_back('=');
    }
    std::string xml = "<pdnImage width=\"1\" height=\"1\" layers=\"1\"><custom><thumb png=\"" + b64 + "\" /></custom></pdnImage>";
    std::vector<BYTE> data = {'P', 'D', 'N', '3', 0, 0, 0};
    const UINT32 xmlLen = static_cast<UINT32>(xml.size());
    data[4] = static_cast<BYTE>(xmlLen);
    data[5] = static_cast<BYTE>(xmlLen >> 8);
    data[6] = static_cast<BYTE>(xmlLen >> 16);
    data.insert(data.end(), xml.begin(), xml.end());
    return data;
}

std::vector<BYTE> MakeXcf2x1()
{
    std::vector<BYTE> data;
    const char magic[] = "gimp xcf file";
    data.insert(data.end(), magic, magic + 14);
    PutBE32(data, 2);
    PutBE32(data, 1);
    PutBE32(data, 0);
    PutBE32(data, 17);
    PutBE32(data, 1);
    data.push_back(0);
    PutBE32(data, 0);
    PutBE32(data, 0);
    const size_t layerPtr = data.size();
    PutBE32(data, 0);
    PutBE32(data, 0);
    PutBE32(data, 0);
    const UINT32 layerAt = static_cast<UINT32>(data.size());
    PatchBE32(data, layerPtr, layerAt);
    PutBE32(data, 2);
    PutBE32(data, 1);
    PutBE32(data, 0);
    PutBE32(data, 1);
    data.push_back(0);
    PutBE32(data, 0);
    PutBE32(data, 0);
    const size_t hierPtr = data.size();
    PutBE32(data, 0);
    PutBE32(data, 0);
    const UINT32 hierAt = static_cast<UINT32>(data.size());
    PatchBE32(data, hierPtr, hierAt);
    PutBE32(data, 2);
    PutBE32(data, 1);
    PutBE32(data, 3);
    const size_t levelPtr = data.size();
    PutBE32(data, 0);
    PutBE32(data, 0);
    const UINT32 levelAt = static_cast<UINT32>(data.size());
    PatchBE32(data, levelPtr, levelAt);
    PutBE32(data, 2);
    PutBE32(data, 1);
    const size_t tilePtr = data.size();
    PutBE32(data, 0);
    PutBE32(data, 0);
    const UINT32 tileAt = static_cast<UINT32>(data.size());
    PatchBE32(data, tilePtr, tileAt);
    data.insert(data.end(), {255, 0, 0, 0, 255, 0});
    return data;
}

std::vector<BYTE> Make3dmWithBmp()
{
    std::vector<BYTE> data(32, ' ');
    memcpy(data.data(), "3D Geometry File Format", 23);
    data[24] = '3';
    std::vector<BYTE> bmp(14 + 40 + 8, 0);
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = 62;
    bmp[10] = 54;
    bmp[14] = 40;
    bmp[18] = 2;
    bmp[22] = 1;
    bmp[26] = 1;
    bmp[28] = 24;
    bmp[54] = 0;
    bmp[55] = 0;
    bmp[56] = 255;
    bmp[57] = 0;
    bmp[58] = 255;
    bmp[59] = 0;
    data.insert(data.end(), bmp.begin(), bmp.end());
    return data;
}

void PutLE16(std::vector<BYTE>& data, UINT16 value)
{
    data.push_back(static_cast<BYTE>(value));
    data.push_back(static_cast<BYTE>(value >> 8));
}

void PutLE32(std::vector<BYTE>& data, UINT32 value)
{
    PutLE16(data, static_cast<UINT16>(value));
    PutLE16(data, static_cast<UINT16>(value >> 16));
}

void PutLE64(std::vector<BYTE>& data, UINT64 value)
{
    PutLE32(data, static_cast<UINT32>(value));
    PutLE32(data, static_cast<UINT32>(value >> 32));
}

void Append3dmChunk(std::vector<BYTE>& data, UINT32 tcode, const std::vector<BYTE>& payload, bool crc)
{
    PutLE32(data, tcode);
    PutLE32(data, static_cast<UINT32>(payload.size() + (crc ? 4u : 0u)));
    data.insert(data.end(), payload.begin(), payload.end());
    if (crc)
    {
        PutLE32(data, 0);
    }
}

void Append3dmChunk64(std::vector<BYTE>& data, UINT32 tcode, const std::vector<BYTE>& payload, bool crc)
{
    PutLE32(data, tcode);
    PutLE64(data, payload.size() + (crc ? 4u : 0u));
    data.insert(data.end(), payload.begin(), payload.end());
    if (crc)
    {
        PutLE32(data, 0);
    }
}

std::vector<BYTE> Make3dm32bppAlpha0()
{
    std::vector<BYTE> data(32, ' ');
    memcpy(data.data(), "3D Geometry File Format", 23);
    data[24] = '3';
    std::vector<BYTE> bmp(14 + 40 + 8, 0);
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = 62;
    bmp[10] = 54;
    bmp[14] = 40;
    bmp[18] = 2;
    bmp[22] = 1;
    bmp[26] = 1;
    bmp[28] = 32;
    bmp[54] = 0;
    bmp[55] = 255;
    bmp[56] = 0;
    bmp[57] = 0;
    bmp[58] = 0;
    bmp[59] = 255;
    bmp[60] = 0;
    bmp[61] = 0;
    data.insert(data.end(), bmp.begin(), bmp.end());
    return data;
}

std::vector<BYTE> Make3dmCompressedPreview()
{
    std::vector<BYTE> header(32, ' ');
    memcpy(header.data(), "3D Geometry File Format", 23);
    header[24] = '8';

    std::vector<BYTE> bitmap;
    PutLE32(bitmap, 40);
    PutLE32(bitmap, 2);
    PutLE32(bitmap, 1);
    PutLE16(bitmap, 1);
    PutLE16(bitmap, 24);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 8);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 8); // uncompressed palette+bits
    PutLE32(bitmap, 0); // crc
    bitmap.push_back(0); // method 0 = stored
    bitmap.insert(bitmap.end(), {0, 0, 255, 0, 255, 0, 0, 0});

    std::vector<BYTE> table;
    Append3dmChunk(table, 0x20008025u, bitmap, true);
    PutLE32(table, 0xFFFFFFFFu);
    PutLE32(table, 0);

    std::vector<BYTE> data = header;
    Append3dmChunk(data, 0x10000014u, table, false);
    return data;
}

std::vector<BYTE> Make3dmV50CompressedPreview(bool zlibWrapped)
{
    std::vector<BYTE> header(32, ' ');
    memcpy(header.data(), "3D Geometry File Format", 23);
    memcpy(header.data() + 24, "      50", 8);

    std::vector<BYTE> bitmap;
    PutLE32(bitmap, 40);
    PutLE32(bitmap, 2);
    PutLE32(bitmap, 1);
    PutLE16(bitmap, 1);
    PutLE16(bitmap, 24);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 8);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 0);
    PutLE32(bitmap, 8); // uncompressed palette+bits
    PutLE32(bitmap, 0); // crc
    if (zlibWrapped)
    {
        bitmap.push_back(1); // method 1 = zlib
        const BYTE zlib[] = {0x78, 0xda, 0x63, 0x60, 0xf8, 0x0f, 0x84, 0x0c,
                             0x0c, 0x00, 0x09, 0xfe, 0x01, 0xff};
        PutLE32(bitmap, 0x40008000u); // TCODE_ANONYMOUS_CHUNK
        PutLE64(bitmap, sizeof(zlib) + 4);
        bitmap.insert(bitmap.end(), zlib, zlib + sizeof(zlib));
        PutLE32(bitmap, 0); // chunk CRC
    }
    else
    {
        bitmap.push_back(0); // method 0 = stored
        bitmap.insert(bitmap.end(), {0, 0, 255, 0, 255, 0, 0, 0});
    }

    std::vector<BYTE> table;
    Append3dmChunk64(table, 0x20008025u, bitmap, true);
    PutLE32(table, 0xFFFFFFFFu);
    PutLE64(table, 0);

    std::vector<BYTE> data = header;
    Append3dmChunk64(data, 0x10000014u, table, false);
    return data;
}

std::vector<BYTE> Make3dmWmfTrap()
{
    std::vector<BYTE> data(128, 0);
    memcpy(data.data(), "3D Geometry File Format", 23);
    data[24] = '3';
    data[64] = 0xD7;
    data[65] = 0xCD;
    data[66] = 0xC6;
    data[67] = 0x9A;
    return data;
}

std::vector<BYTE> MakePlaceableWmf()
{
    std::vector<BYTE> data;
    PutLE32(data, 0x9AC6CDD7);
    PutLE16(data, 0);
    PutLE16(data, 0);
    PutLE16(data, 0);
    PutLE16(data, 1440);
    PutLE16(data, 1440);
    PutLE16(data, 1440);
    PutLE32(data, 0);
    PutLE16(data, 0);
    UINT16 sum = 0;
    for (size_t i = 0; i < 20; i += 2)
    {
        sum ^= static_cast<UINT16>(data[i] | (data[i + 1] << 8));
    }
    data[20] = static_cast<BYTE>(sum);
    data[21] = static_cast<BYTE>(sum >> 8);
    PutLE16(data, 1);
    PutLE16(data, 9);
    PutLE16(data, 0x0300);
    PutLE32(data, 22);
    PutLE16(data, 0);
    PutLE32(data, 5);
    PutLE16(data, 0);
    PutLE32(data, 5);
    PutLE16(data, 0x020B);
    PutLE16(data, 0);
    PutLE16(data, 0);
    PutLE32(data, 5);
    PutLE16(data, 0x020C);
    PutLE16(data, 1440);
    PutLE16(data, 1440);
    PutLE32(data, 3);
    PutLE16(data, 0);
    return data;
}

void PutF32LE(std::vector<BYTE>& data, float value)
{
    UINT32 bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    PutLE32(data, bits);
}

void AppendBinaryStlTri(std::vector<BYTE>& data, float ax, float ay, float az, float bx, float by, float bz, float cx,
                        float cy, float cz)
{
    PutF32LE(data, 0);
    PutF32LE(data, 0);
    PutF32LE(data, 0);
    PutF32LE(data, ax);
    PutF32LE(data, ay);
    PutF32LE(data, az);
    PutF32LE(data, bx);
    PutF32LE(data, by);
    PutF32LE(data, bz);
    PutF32LE(data, cx);
    PutF32LE(data, cy);
    PutF32LE(data, cz);
    PutLE16(data, 0);
}

std::vector<BYTE> MakeBinaryStlTetrahedron()
{
    std::vector<BYTE> data(80, 0);
    PutLE32(data, 4);
    AppendBinaryStlTri(data, 1, 1, 1, 1, -1, -1, -1, 1, -1);
    AppendBinaryStlTri(data, 1, 1, 1, -1, -1, 1, 1, -1, -1);
    AppendBinaryStlTri(data, 1, 1, 1, -1, 1, -1, -1, -1, 1);
    AppendBinaryStlTri(data, 1, -1, -1, -1, -1, 1, -1, 1, -1);
    return data;
}

std::vector<BYTE> MakeAsciiStlTetrahedron()
{
    const char* ascii =
        "solid test\n"
        "facet normal 0 0 0\n outer loop\n"
        "  vertex 1 1 1\n  vertex 1 -1 -1\n  vertex -1 1 -1\n"
        " endloop\nendfacet\n"
        "facet normal 0 0 0\n outer loop\n"
        "  vertex 1 1 1\n  vertex -1 -1 1\n  vertex 1 -1 -1\n"
        " endloop\nendfacet\n"
        "facet normal 0 0 0\n outer loop\n"
        "  vertex 1 1 1\n  vertex -1 1 -1\n  vertex -1 -1 1\n"
        " endloop\nendfacet\n"
        "facet normal 0 0 0\n outer loop\n"
        "  vertex 1 -1 -1\n  vertex -1 -1 1\n  vertex -1 1 -1\n"
        " endloop\nendfacet\n"
        "endsolid test\n";
    return std::vector<BYTE>(ascii, ascii + strlen(ascii));
}

std::vector<BYTE> MakeStoredZip(const char* name, const std::vector<BYTE>& payload)
{
    std::vector<BYTE> data;
    const UINT16 nameLen = static_cast<UINT16>(strlen(name));
    PutLE32(data, 0x04034b50);
    PutLE16(data, 20);
    PutLE16(data, 0);
    PutLE16(data, 0);
    PutLE16(data, 0);
    PutLE16(data, 0);
    PutLE32(data, 0);
    PutLE32(data, static_cast<UINT32>(payload.size()));
    PutLE32(data, static_cast<UINT32>(payload.size()));
    PutLE16(data, nameLen);
    PutLE16(data, 0);
    data.insert(data.end(), name, name + nameLen);
    data.insert(data.end(), payload.begin(), payload.end());
    return data;
}

std::vector<BYTE> MakeBmp1x1Red()
{
    std::vector<BYTE> bmp(14 + 40 + 4, 0);
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = 58;
    bmp[10] = 54;
    bmp[14] = 40;
    bmp[18] = 1;
    bmp[22] = 1;
    bmp[26] = 1;
    bmp[28] = 32;
    bmp[54] = 0;
    bmp[55] = 0;
    bmp[56] = 255;
    bmp[57] = 255;
    return bmp;
}

std::vector<BYTE> MakeRiffCdrWithBmp()
{
    const auto bmp = MakeBmp1x1Red();
    const UINT32 chunkSize = static_cast<UINT32>(bmp.size());
    const UINT32 riffSize = 4u + 8u + chunkSize + (chunkSize & 1u);
    std::vector<BYTE> data(8 + riffSize, 0);
    memcpy(data.data(), "RIFF", 4);
    data[4] = static_cast<BYTE>(riffSize);
    data[5] = static_cast<BYTE>(riffSize >> 8);
    memcpy(data.data() + 8, "CDR8", 4);
    memcpy(data.data() + 12, "disp", 4);
    data[16] = static_cast<BYTE>(chunkSize);
    data[17] = static_cast<BYTE>(chunkSize >> 8);
    memcpy(data.data() + 20, bmp.data(), bmp.size());
    return data;
}

void PutU32At(std::vector<BYTE>& data, size_t at, UINT32 value)
{
    data[at] = static_cast<BYTE>(value);
    data[at + 1] = static_cast<BYTE>(value >> 8);
    data[at + 2] = static_cast<BYTE>(value >> 16);
    data[at + 3] = static_cast<BYTE>(value >> 24);
}

std::vector<BYTE> MakeDosEpsWmf()
{
    const auto wmf = MakePlaceableWmf();
    std::vector<BYTE> data(30 + wmf.size(), 0);
    data[0] = 0xC5;
    data[1] = 0xD0;
    data[2] = 0xD3;
    data[3] = 0xC6;
    PutU32At(data, 12, 30);
    PutU32At(data, 16, static_cast<UINT32>(wmf.size()));
    memcpy(data.data() + 30, wmf.data(), wmf.size());
    return data;
}

std::vector<BYTE> MakeDosEpsTiff()
{
    const BYTE tiff[] = {'I', 'I', 42, 0, 8, 0, 0, 0};
    std::vector<BYTE> data(30 + sizeof(tiff), 0);
    data[0] = 0xC5;
    data[1] = 0xD0;
    data[2] = 0xD3;
    data[3] = 0xC6;
    PutU32At(data, 20, 30);
    PutU32At(data, 24, static_cast<UINT32>(sizeof(tiff)));
    memcpy(data.data() + 30, tiff, sizeof(tiff));
    return data;
}

std::vector<BYTE> MakeDdsDx10Bc5()
{
    std::vector<BYTE> data(148 + 16, 0);
    memcpy(data.data(), "DDS ", 4);
    data[4] = 124;
    data[12] = 4;
    data[16] = 4;
    data[76] = 32;
    data[80] = 0x04;
    data[84] = 'D';
    data[85] = 'X';
    data[86] = '1';
    data[87] = '0';
    data[128] = 83; // DXGI_FORMAT_BC5_UNORM
    data[140] = 1;
    data[148] = 255;
    data[149] = 255;
    data[156] = 128;
    data[157] = 128;
    return data;
}

std::vector<BYTE> MakePdfFlateRgb()
{
    const BYTE zlib[] = {0x78, 0x01, 0x01, 0x06, 0x00, 0xF9, 0xFF, 0xFF, 0x00, 0x00,
                         0x00, 0xFF, 0x00, 0x07, 0xFE, 0x01, 0xFF};
    const char* header =
        "%PDF-1.4\n1 0 obj\n<< /Subtype /Image /Width 2 /Height 1 /ColorSpace /DeviceRGB "
        "/BitsPerComponent 8 /Filter /FlateDecode /Length 17 >>\nstream\n";
    const char* footer = "\nendstream\nendobj\n";
    std::vector<BYTE> data(header, header + strlen(header));
    data.insert(data.end(), zlib, zlib + sizeof(zlib));
    data.insert(data.end(), footer, footer + strlen(footer));
    return data;
}

std::vector<BYTE> MakeDwgWithBmp()
{
    const BYTE sentinel[16] = {0x1F, 0x25, 0x6D, 0x07, 0xD4, 0x36, 0x28, 0x28,
                               0x9D, 0x57, 0xCA, 0x3F, 0x9D, 0x44, 0x10, 0x2B};
    std::vector<BYTE> bmp(14 + 40 + 8, 0);
    bmp[0] = 'B';
    bmp[1] = 'M';
    bmp[2] = 62;
    bmp[10] = 54;
    bmp[14] = 40;
    bmp[18] = 2;
    bmp[22] = 1;
    bmp[26] = 1;
    bmp[28] = 24;
    bmp[54] = 0;
    bmp[55] = 0;
    bmp[56] = 255;
    bmp[57] = 0;
    bmp[58] = 255;
    bmp[59] = 0;
    const UINT32 previewAt = 64;
    const UINT32 bmpAt = previewAt + 16 + 4 + 1 + 9;
    std::vector<BYTE> data(static_cast<size_t>(bmpAt) + bmp.size(), 0);
    memcpy(data.data(), "AC1018", 6);
    data[0x0D] = static_cast<BYTE>(previewAt);
    memcpy(data.data() + previewAt, sentinel, sizeof(sentinel));
    size_t pos = previewAt + 16;
    data[pos] = 30; // overall size (unused beyond presence)
    pos += 4;
    data[pos++] = 1; // one preview image
    data[pos++] = 2; // type BMP
    data[pos] = static_cast<BYTE>(bmpAt);
    data[pos + 1] = static_cast<BYTE>(bmpAt >> 8);
    data[pos + 2] = static_cast<BYTE>(bmpAt >> 16);
    data[pos + 3] = static_cast<BYTE>(bmpAt >> 24);
    pos += 4;
    const UINT32 bmpBytes = static_cast<UINT32>(bmp.size());
    data[pos] = static_cast<BYTE>(bmpBytes);
    data[pos + 1] = static_cast<BYTE>(bmpBytes >> 8);
    data[pos + 2] = static_cast<BYTE>(bmpBytes >> 16);
    data[pos + 3] = static_cast<BYTE>(bmpBytes >> 24);
    memcpy(data.data() + bmpAt, bmp.data(), bmp.size());
    return data;
}

} // namespace

int main()
{
    std::vector<std::string> masks;
    PictView::Native::GetDecoderMasks(masks);
    const char* required[] = {"*.tga", "*.pcx", "*.pnm", "*.svg", "*.psd", "*.iff", "*.eps", "*.mov",
                              "*.dds", "*.xcf", "*.pdn", "*.3dm", "*.ai", "*.dwg", "*.wmf", "*.stl", "*.3mf"};
    for (const char* mask : required)
    {
        if (std::find(masks.begin(), masks.end(), mask) == masks.end())
        {
            return Fail("native decoder mask is missing");
        }
    }

    int failures = 0;
    failures += ExpectImage("tga", MakeTga2x1(), PVF_TGA, 2, 1);
    failures += ExpectImage("pnm", MakePnm2x1(), PVF_PNM, 2, 1);
    failures += ExpectImage("wbmp", MakeWbmp2x2(), PVF_WBMP, 2, 2);
    failures += ExpectImage("ras", MakeRas2x1(), PVF_RAS, 2, 1);
    failures += ExpectImage("sgi", MakeSgi2x1(), PVF_SGI, 2, 1);
    failures += ExpectImage("pcx", MakePcx2x1(), PVF_PCX, 2, 1);
    failures += ExpectImage("svg", MakeSvg2x2(), PVF_SVG, 2, 2);
    {
        PictView::Native::DecodedImage svgRed;
        const auto svgBytes = MakeSvg2x2();
        if (PictView::Native::DecodeMemory(svgBytes.data(), svgBytes.size(), svgRed) !=
                PictView::Native::Status::Ok ||
            svgRed.frames.empty() || svgRed.frames[0].bgra.size() < 4)
        {
            return Fail("SVG fill #ff0000 must decode");
        }
        const auto& px = svgRed.frames[0].bgra;
        bool foundRedBgra = false;
        for (size_t i = 0; i + 3 < px.size(); i += 4)
        {
            if (px[i] == 0 && px[i + 1] == 0 && px[i + 2] == 255 && px[i + 3] == 255)
            {
                foundRedBgra = true;
                break;
            }
        }
        if (!foundRedBgra)
        {
            return Fail("SVG fill #ff0000 must be stored as BGRA, not NanoSVG RGBA");
        }
    }
    failures += ExpectImage("psd", MakePsd2x1(), PVF_PSD, 2, 1);
    failures += ExpectImage("dds", MakeDds2x1(), PVF_DDS, 2, 1);
    failures += ExpectImage("dds-dx10", MakeDdsDx10Bc1(), PVF_DDS, 4, 4);
    failures += ExpectImage("eps", MakeEpsPreview(), PVF_EPS, 2, 2);
    failures += ExpectDecoded("eps-dos-wmf", MakeDosEpsWmf(), PVF_EPS);
    {
        PictView::Native::EmbeddedPreview dosTiff;
        const auto epsTiff = MakeDosEpsTiff();
        if (!PictView::Native::FindEmbeddedPreview(epsTiff.data(), epsTiff.size(), dosTiff) ||
            dosTiff.offset != 30 || dosTiff.size < 8)
        {
            return Fail("DOS EPS header must expose the TIFF preview at offset 20/24");
        }
    }
    failures += ExpectImage("3mf", MakeStoredZip("Metadata/thumbnail.png", MakePng1x1Red()), PVF_3MF, 1, 1);
    failures += ExpectImage("skp", MakeStoredZip("preview.png", MakePng1x1Red()), PVF_SKP, 1, 1);
    failures += ExpectImage("cdr-zip-bmp", MakeStoredZip("metadata/thumbnails/thumbnail.bmp", MakeBmp1x1Red()), PVF_CDR, 1, 1);
    failures += ExpectImage("cdr-riff-bmp", MakeRiffCdrWithBmp(), PVF_CDR, 1, 1);
    {
        const auto stlBin = MakeBinaryStlTetrahedron();
        PictView::Native::DecodedImage stolen;
        if (PictView::Native::DecodeMemory(stlBin.data(), stlBin.size(), stolen) == PictView::Native::Status::Ok)
        {
            return Fail("STL must not be decoded by DecodeMemory (viewer stays on IPreviewHandler)");
        }
        auto checkStl = [](const char* name, const std::vector<BYTE>& bytes) -> int {
            PictView::Native::Frame frame;
            const PictView::Native::Status st = PictView::Native::RasterizeStlMemory(
                bytes.data(), bytes.size(), 64, 64, RGB(200, 200, 200), frame);
            if (st != PictView::Native::Status::Ok || frame.width != 64 || frame.height != 64 ||
                frame.bgra.size() < 64u * 64u * 4u)
            {
                std::cerr << name << ": STL rasterize failed\n";
                return 1;
            }
            const size_t center = (32u * 64u + 32u) * 4u;
            if (frame.bgra[center + 3] != 255)
            {
                std::cerr << name << ": mesh center must be opaque\n";
                return 1;
            }
            if (frame.bgra[3] != 0)
            {
                std::cerr << name << ": corner must stay transparent\n";
                return 1;
            }
            std::cout << name << ": 64x64 checksum=" << Checksum(frame) << "\n";
            return 0;
        };
        failures += checkStl("stl-binary", stlBin);
        failures += checkStl("stl-ascii", MakeAsciiStlTetrahedron());
        auto stlPadded = stlBin;
        stlPadded.insert(stlPadded.end(), 64, 0);
        failures += checkStl("stl-binary-padded", stlPadded);
        PictView::Native::Frame huge;
        BYTE dummy = 0;
        if (PictView::Native::RasterizeStlMemory(&dummy, 33ull * 1024ull * 1024ull, 8, 8, RGB(255, 255, 255), huge) !=
            PictView::Native::Status::Unsupported)
        {
            return Fail("STL larger than 32 MB must be rejected");
        }
        std::vector<BYTE> tooMany(84, 0);
        tooMany[80] = 0xA9;
        tooMany[81] = 0xD0;
        tooMany[82] = 0x03; // 250001 little-endian, size does not match
        if (PictView::Native::RasterizeStlMemory(tooMany.data(), tooMany.size(), 8, 8, RGB(255, 255, 255), huge) !=
            PictView::Native::Status::Unsupported)
        {
            return Fail("STL with a huge triangle count must be rejected");
        }
    }
    failures += ExpectImage("pdn", MakePdnThumb(), PVF_PDN, 1, 1);
    failures += ExpectImage("xcf", MakeXcf2x1(), PVF_XCF, 2, 1);
    failures += ExpectImage("3dm", Make3dmWithBmp(), PVF_3DM, 2, 1);
    failures += ExpectImage("3dm-bgra0", Make3dm32bppAlpha0(), PVF_3DM, 2, 1);
    failures += ExpectImage("3dm-compressed", Make3dmCompressedPreview(), PVF_3DM, 2, 1);
    failures += ExpectImage("3dm-v50", Make3dmV50CompressedPreview(false), PVF_3DM, 2, 1);
    failures += ExpectImage("3dm-v50-zlib", Make3dmV50CompressedPreview(true), PVF_3DM, 2, 1);
    {
        PictView::Native::DecodedImage opaque;
        const auto alpha0 = Make3dm32bppAlpha0();
        if (PictView::Native::DecodeMemory(alpha0.data(), alpha0.size(), opaque) != PictView::Native::Status::Ok ||
            opaque.frames.empty() || opaque.frames[0].bgra.size() < 8 || opaque.frames[0].bgra[1] != 255 ||
            opaque.frames[0].bgra[3] != 255)
        {
            return Fail("32-bpp 3DM preview with unused alpha must stay visible");
        }
        PictView::Native::DecodedImage trap;
        const auto wmfTrap = Make3dmWmfTrap();
        if (PictView::Native::DecodeMemory(wmfTrap.data(), wmfTrap.size(), trap) == PictView::Native::Status::Ok)
        {
            return Fail("3DM must not treat an embedded WMF signature as the model preview");
        }
        const wchar_t* sample = L"H:\\_projects\\3d_models\\box_nozzles.3dm";
        if (GetFileAttributesW(sample) != INVALID_FILE_ATTRIBUTES)
        {
            PictView::Native::DecodedImage rhino;
            if (PictView::Native::DecodeFile(sample, rhino) != PictView::Native::Status::Ok ||
                rhino.frames.empty() || rhino.frames[0].width != 1024 || rhino.frames[0].height != 552)
            {
                return Fail("Rhino 5 3DM compressed preview must decode as 1024x552");
            }
            bool sawColor = false;
            for (size_t i = 0; i + 3 < rhino.frames[0].bgra.size(); i += 4)
            {
                if (rhino.frames[0].bgra[i + 3] != 255)
                {
                    return Fail("Rhino 5 3DM preview must be opaque");
                }
                if (rhino.frames[0].bgra[i] != 0 || rhino.frames[0].bgra[i + 1] != 0 ||
                    rhino.frames[0].bgra[i + 2] != 0)
                {
                    sawColor = true;
                }
            }
            if (!sawColor)
            {
                return Fail("Rhino 5 3DM preview must contain visible RGB");
            }
            std::cout << "3dm-box-nozzles: 1024x552 checksum=" << Checksum(rhino.frames[0]) << "\n";
        }
    }
    failures += ExpectDecoded("wmf", MakePlaceableWmf(), PVF_WMF);
    failures += ExpectImage("dds-bc5", MakeDdsDx10Bc5(), PVF_DDS, 4, 4);
    failures += ExpectImage("pdf-flate", MakePdfFlateRgb(), PVF_EPS, 2, 1);
    failures += ExpectImage("dwg", MakeDwgWithBmp(), PVF_DWG, 2, 1);

    PictView::Native::EmbeddedPreview preview;
    const auto jpeg = MakeJpegBlob();
    if (!PictView::Native::FindEmbeddedPreview(jpeg.data(), jpeg.size(), preview) || preview.size < 4)
    {
        return Fail("embedded JPEG preview was not found");
    }
    const auto pdf = MakePdfJpeg();
    if (!PictView::Native::FindEmbeddedPreview(pdf.data(), pdf.size(), preview) || preview.format != PVF_JPG)
    {
        return Fail("PDF DCTDecode JPEG preview was not found");
    }

    std::vector<BYTE> threeDmJpeg(32, ' ');
    memcpy(threeDmJpeg.data(), "3D Geometry File Format", 23);
    threeDmJpeg[24] = '8';
    const auto jpegIn3dm = MakeJpegBlob();
    threeDmJpeg.insert(threeDmJpeg.end(), jpegIn3dm.begin(), jpegIn3dm.end());
    const auto wmfIn3dm = MakePlaceableWmf();
    threeDmJpeg.insert(threeDmJpeg.end(), wmfIn3dm.begin(), wmfIn3dm.end());
    PictView::Native::EmbeddedPreview ranked;
    if (!PictView::Native::FindEmbeddedPreview(threeDmJpeg.data(), threeDmJpeg.size(), ranked) ||
        ranked.format != PVF_JPG)
    {
        return Fail("3DM preview scan must prefer an embedded JPEG over a later WMF signature");
    }

    std::vector<BYTE> noise(512u * 1024u, static_cast<BYTE>('x'));
    memcpy(noise.data(), "%PDF-1.4\n", 9);
    memcpy(noise.data() + 100, "/DCTDecode", 10);
    for (size_t i = 128; i + 3 < noise.size(); i += 64)
    {
        noise[i] = 0xFF;
        noise[i + 1] = 0xD8;
        noise[i + 2] = 0xFF;
        noise[i + 3] = 0xE0;
    }
    const auto started = std::chrono::steady_clock::now();
    PictView::Native::DecodedImage ignored;
    (void)PictView::Native::DecodeMemory(noise.data(), noise.size(), ignored);
    PictView::Native::EmbeddedPreview noisePreview;
    (void)PictView::Native::FindEmbeddedPreview(noise.data(), noise.size(), noisePreview);
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - started)
                               .count();
    if (elapsedMs > 3000)
    {
        return Fail("PDF/AI preview scan hung on noisy input");
    }
    if (ignored.format == PVF_SVG)
    {
        return Fail("PDF-like input must not be parsed as SVG");
    }

    std::vector<BYTE> pdfDicts(2u * 1024u * 1024u, static_cast<BYTE>(' '));
    memcpy(pdfDicts.data(), "%PDF-1.4\n", 9);
    const char* dict = "<< /Type /Page /MediaBox [0 0 1 1] >>\n";
    const size_t dictLen = strlen(dict);
    for (size_t i = 32; i + dictLen < pdfDicts.size(); i += dictLen)
    {
        memcpy(pdfDicts.data() + i, dict, dictLen);
    }
    PictView::Native::DecodedImage pdfSvgTrap;
    const auto svgTrapStarted = std::chrono::steady_clock::now();
    const auto pdfSvgStatus = PictView::Native::DecodeMemory(pdfDicts.data(), pdfDicts.size(), pdfSvgTrap);
    const auto svgTrapMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - svgTrapStarted)
                               .count();
    if (svgTrapMs > 3000)
    {
        return Fail("PDF dictionaries must not hang the SVG/NanoSVG path");
    }
    if (pdfSvgTrap.format == PVF_SVG)
    {
        return Fail("PDF dictionaries must not be treated as SVG");
    }
    (void)pdfSvgStatus;

    std::vector<BYTE> notSvg(256u * 1024u, static_cast<BYTE>('x'));
    memcpy(notSvg.data(), "<foo><bar>", 10);
    for (size_t i = 16; i + 8 < 4096 && i + 8 < notSvg.size(); i += 8)
    {
        memcpy(notSvg.data() + i, "<abc>", 5);
    }
    PictView::Native::DecodedImage xmlTrap;
    const auto xmlStarted = std::chrono::steady_clock::now();
    (void)PictView::Native::DecodeMemory(notSvg.data(), notSvg.size(), xmlTrap);
    const auto xmlMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - xmlStarted)
                           .count();
    if (xmlMs > 3000)
    {
        return Fail("non-SVG markup must not hang NanoSVG");
    }
    if (xmlTrap.format == PVF_SVG)
    {
        return Fail("markup without <svg must not decode as SVG");
    }

    if (failures != 0)
    {
        std::cerr << failures << " native decoder checks failed\n";
        return 1;
    }
    std::cout << "native_decoder_tests: ok\n";
    return 0;
}
