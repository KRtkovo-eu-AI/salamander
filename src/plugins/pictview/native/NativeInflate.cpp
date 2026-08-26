// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// Deflate inflater used by XCF zlib tiles, PNG IDAT, SKP ZIP, and compressed
// 3DM previews. The bit-reader and Huffman construction follow Mark Adler's
// puff (zlib license): Copyright (C) 2002-2013 Mark Adler.

#include "NativeInternal.h"

#include <algorithm>
#include <vector>

namespace PictView::Native
{
namespace Detail
{
namespace
{

constexpr size_t kMaxInflateBytes = 256ull * 1024ull * 1024ull;

struct BitReader
{
    const BYTE* data = nullptr;
    size_t size = 0;
    size_t pos = 0;
    UINT32 hold = 0;
    int bits = 0;
    bool failed = false;

    int Need(int n)
    {
        while (bits < n)
        {
            if (pos >= size)
            {
                failed = true;
                return 0;
            }
            hold |= static_cast<UINT32>(data[pos++]) << bits;
            bits += 8;
        }
        return n;
    }

    UINT32 Bits(int n)
    {
        if (Need(n), failed)
        {
            return 0;
        }
        const UINT32 value = hold & ((1u << n) - 1u);
        hold >>= n;
        bits -= n;
        return value;
    }

    void Align()
    {
        hold >>= (bits % 8);
        bits -= bits % 8;
    }
};

struct Huffman
{
    int count[16]{};
    int symbol[288]{};
    int first[16]{};
    int index[16]{};
    bool ok = false;
};

bool BuildHuffman(const BYTE* lengths, int n, Huffman& table, int maxSymbol)
{
    table = {};
    if (n <= 0 || n > maxSymbol)
    {
        return false;
    }
    for (int i = 0; i < n; ++i)
    {
        if (lengths[i] > 15)
        {
            return false;
        }
        table.count[lengths[i]]++;
    }
    if (table.count[0] == n)
    {
        table.ok = true;
        return true;
    }
    int left = 1;
    for (int len = 1; len <= 15; ++len)
    {
        left <<= 1;
        left -= table.count[len];
        if (left < 0)
        {
            return false;
        }
    }
    int offs[16]{};
    offs[1] = 0;
    for (int len = 1; len < 15; ++len)
    {
        offs[len + 1] = offs[len] + table.count[len];
    }
    int nextCode = 0;
    for (int len = 1; len <= 15; ++len)
    {
        table.first[len] = nextCode;
        table.index[len] = offs[len];
        nextCode = (nextCode + table.count[len]) << 1;
    }
    for (int symbol = 0; symbol < n; ++symbol)
    {
        const int len = lengths[symbol];
        if (len != 0)
        {
            table.symbol[offs[len]++] = symbol;
        }
    }
    table.ok = true;
    return true;
}

int DecodeHuffman(BitReader& br, const Huffman& table)
{
    int code = 0;
    for (int len = 1; len <= 15; ++len)
    {
        code = (code << 1) | static_cast<int>(br.Bits(1));
        if (br.failed)
        {
            return -1;
        }
        const int count = table.count[len];
        const int first = table.first[len];
        if (code - first >= 0 && code - first < count)
        {
            return table.symbol[table.index[len] + (code - first)];
        }
    }
    return -1;
}

constexpr int kLengthBase[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                                 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
constexpr int kLengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                                  3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr int kDistBase[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
                               257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
constexpr int kDistExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                                7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
constexpr BYTE kOrder[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

bool InflateCodes(BitReader& br, const Huffman& lit, const Huffman& dist, std::vector<BYTE>& dest, size_t maxOut)
{
    for (;;)
    {
        const int symbol = DecodeHuffman(br, lit);
        if (symbol < 0 || br.failed)
        {
            return false;
        }
        if (symbol < 256)
        {
            if (dest.size() >= maxOut)
            {
                return false;
            }
            dest.push_back(static_cast<BYTE>(symbol));
            continue;
        }
        if (symbol == 256)
        {
            return true;
        }
        if (symbol > 285)
        {
            return false;
        }
        const int lenIndex = symbol - 257;
        int length = kLengthBase[lenIndex] + static_cast<int>(br.Bits(kLengthExtra[lenIndex]));
        const int distSymbol = DecodeHuffman(br, dist);
        if (distSymbol < 0 || distSymbol > 29 || br.failed)
        {
            return false;
        }
        const int distance = kDistBase[distSymbol] + static_cast<int>(br.Bits(kDistExtra[distSymbol]));
        if (distance <= 0 || static_cast<size_t>(distance) > dest.size())
        {
            return false;
        }
        if (dest.size() + static_cast<size_t>(length) > maxOut)
        {
            return false;
        }
        for (int i = 0; i < length; ++i)
        {
            dest.push_back(dest[dest.size() - static_cast<size_t>(distance)]);
        }
    }
}

bool InflateDynamic(BitReader& br, Huffman& lit, Huffman& dist)
{
    const int nlit = static_cast<int>(br.Bits(5)) + 257;
    const int ndist = static_cast<int>(br.Bits(5)) + 1;
    const int ncode = static_cast<int>(br.Bits(4)) + 4;
    if (nlit > 286 || ndist > 32 || br.failed)
    {
        return false;
    }
    BYTE codeLen[19]{};
    for (int i = 0; i < ncode; ++i)
    {
        codeLen[kOrder[i]] = static_cast<BYTE>(br.Bits(3));
    }
    Huffman codes;
    if (!BuildHuffman(codeLen, 19, codes, 19))
    {
        return false;
    }
    BYTE lengths[318]{};
    int filled = 0;
    const int total = nlit + ndist;
    while (filled < total)
    {
        const int symbol = DecodeHuffman(br, codes);
        if (symbol < 0 || br.failed)
        {
            return false;
        }
        if (symbol < 16)
        {
            lengths[filled++] = static_cast<BYTE>(symbol);
        }
        else
        {
            int repeat = 0;
            BYTE value = 0;
            if (symbol == 16)
            {
                if (filled == 0)
                {
                    return false;
                }
                value = lengths[filled - 1];
                repeat = static_cast<int>(br.Bits(2)) + 3;
            }
            else if (symbol == 17)
            {
                repeat = static_cast<int>(br.Bits(3)) + 3;
            }
            else
            {
                repeat = static_cast<int>(br.Bits(7)) + 11;
            }
            if (filled + repeat > total)
            {
                return false;
            }
            while (repeat-- > 0)
            {
                lengths[filled++] = value;
            }
        }
    }
    return BuildHuffman(lengths, nlit, lit, 288) && BuildHuffman(lengths + nlit, ndist, dist, 32);
}

bool InflateFixed(Huffman& lit, Huffman& dist)
{
    BYTE lengths[288];
    for (int i = 0; i < 144; ++i)
    {
        lengths[i] = 8;
    }
    for (int i = 144; i < 256; ++i)
    {
        lengths[i] = 9;
    }
    for (int i = 256; i < 280; ++i)
    {
        lengths[i] = 7;
    }
    for (int i = 280; i < 288; ++i)
    {
        lengths[i] = 8;
    }
    BYTE distLen[32];
    for (int i = 0; i < 32; ++i)
    {
        distLen[i] = 5;
    }
    return BuildHuffman(lengths, 288, lit, 288) && BuildHuffman(distLen, 32, dist, 32);
}

bool InflateBlocks(BitReader& br, std::vector<BYTE>& dest, size_t maxOut)
{
    for (;;)
    {
        const int bfinal = static_cast<int>(br.Bits(1));
        const int btype = static_cast<int>(br.Bits(2));
        if (br.failed)
        {
            return false;
        }
        if (btype == 0)
        {
            br.Align();
            const UINT32 len = br.Bits(16);
            const UINT32 nlen = br.Bits(16);
            if (br.failed || (len ^ 0xFFFFu) != nlen)
            {
                return false;
            }
            if (br.pos + len > br.size || dest.size() + len > maxOut)
            {
                return false;
            }
            dest.insert(dest.end(), br.data + br.pos, br.data + br.pos + len);
            br.pos += len;
        }
        else if (btype == 1 || btype == 2)
        {
            Huffman lit;
            Huffman dist;
            if (btype == 1)
            {
                if (!InflateFixed(lit, dist))
                {
                    return false;
                }
            }
            else if (!InflateDynamic(br, lit, dist))
            {
                return false;
            }
            if (!InflateCodes(br, lit, dist, dest, maxOut))
            {
                return false;
            }
        }
        else
        {
            return false;
        }
        if (bfinal)
        {
            return !br.failed;
        }
    }
}

} // namespace

bool InflateRaw(const BYTE* src, size_t srcLen, std::vector<BYTE>& dest, size_t maxOut)
{
    dest.clear();
    if (src == nullptr || srcLen == 0)
    {
        return false;
    }
    if (maxOut == 0 || maxOut > kMaxInflateBytes)
    {
        maxOut = kMaxInflateBytes;
    }
    try
    {
        dest.reserve(std::min(srcLen * 2, maxOut));
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
    BitReader br;
    br.data = src;
    br.size = srcLen;
    return InflateBlocks(br, dest, maxOut);
}

bool InflateZlib(const BYTE* src, size_t srcLen, std::vector<BYTE>& dest, size_t maxOut)
{
    dest.clear();
    if (src == nullptr || srcLen < 2)
    {
        return false;
    }
    if (src[0] == 0x1F && src[1] == 0x8B)
    {
        if (srcLen < 10 || (src[3] & 0xE0) != 0)
        {
            return false;
        }
        size_t pos = 10;
        if (src[3] & 0x04)
        {
            if (pos + 2 > srcLen)
            {
                return false;
            }
            const UINT16 extra = static_cast<UINT16>(src[pos] | (src[pos + 1] << 8));
            pos += 2u + extra;
        }
        if (src[3] & 0x08)
        {
            while (pos < srcLen && src[pos] != 0)
            {
                ++pos;
            }
            ++pos;
        }
        if (src[3] & 0x10)
        {
            while (pos < srcLen && src[pos] != 0)
            {
                ++pos;
            }
            ++pos;
        }
        if (src[3] & 0x02)
        {
            pos += 2;
        }
        if (pos >= srcLen)
        {
            return false;
        }
        const size_t payload = srcLen - pos >= 8 ? srcLen - pos - 8 : srcLen - pos;
        return InflateRaw(src + pos, payload, dest, maxOut);
    }

    const BYTE cmf = src[0];
    const BYTE flg = src[1];
    if ((cmf & 0x0F) != 8 || ((static_cast<UINT32>(cmf) * 256u + flg) % 31u) != 0)
    {
        return false;
    }
    size_t pos = 2;
    if (flg & 0x20)
    {
        pos += 4;
    }
    if (pos >= srcLen)
    {
        return false;
    }
    const size_t payload = srcLen - pos >= 4 ? srcLen - pos - 4 : srcLen - pos;
    return InflateRaw(src + pos, payload, dest, maxOut);
}

} // namespace Detail
} // namespace PictView::Native
