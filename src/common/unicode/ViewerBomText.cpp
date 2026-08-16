// SPDX-FileCopyrightText: 2026 Sally Authors (0xeb)
// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ViewerBomText.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <usp10.h>

#include <algorithm>
#include <cwctype>
#include <limits>

#pragma comment(lib, "usp10.lib")

namespace Salamander::Unicode
{
namespace
{

constexpr std::uint32_t Replacement = 0xFFFD;

bool IsHighSurrogate(std::uint16_t value)
{
    return value >= 0xD800 && value <= 0xDBFF;
}

bool IsLowSurrogate(std::uint16_t value)
{
    return value >= 0xDC00 && value <= 0xDFFF;
}

std::uint32_t DecodeSurrogatePair(std::uint16_t high, std::uint16_t low)
{
    return 0x10000 + (((std::uint32_t)high - 0xD800) << 10) + ((std::uint32_t)low - 0xDC00);
}

std::uint16_t ReadUtf16(const std::uint8_t* data, BomEncoding encoding)
{
    if (encoding == BomEncoding::Utf16Le)
        return (std::uint16_t)data[0] | ((std::uint16_t)data[1] << 8);
    return ((std::uint16_t)data[0] << 8) | (std::uint16_t)data[1];
}

bool IsContinuation(std::uint8_t value)
{
    return (value & 0xC0) == 0x80;
}

bool IsFallbackContinuation(std::uint32_t scalar, std::uint32_t previous)
{
    if (scalar == 0x200C || scalar == 0x200D || previous == 0x200D ||
        (scalar >= 0xFE00 && scalar <= 0xFE0F) ||
        (scalar >= 0xE0100 && scalar <= 0xE01EF))
        return true;

    if (scalar <= 0xFFFF)
    {
        WORD type = 0;
        wchar_t ch = (wchar_t)scalar;
        if (GetStringTypeW(CT_CTYPE3, &ch, 1, &type) &&
            (type & (C3_NONSPACING | C3_DIACRITIC | C3_VOWELMARK)) != 0)
            return true;
    }
    return false;
}

std::uint32_t FoldScalar(std::uint32_t scalar, bool caseSensitive)
{
    if (caseSensitive)
        return scalar;
    if (scalar <= std::numeric_limits<wchar_t>::max())
        return (std::uint32_t)towlower((wchar_t)scalar);
    return scalar;
}

bool IsWordScalar(std::uint32_t scalar)
{
    if (scalar == L'_')
        return true;
    if (scalar <= std::numeric_limits<wchar_t>::max())
        return iswalnum((wchar_t)scalar) != 0;
    return true;
}

std::vector<std::uint32_t> PatternScalars(const std::wstring& pattern)
{
    std::vector<std::uint32_t> scalars;
    for (std::size_t i = 0; i < pattern.size();)
    {
        std::uint32_t scalar = (std::uint32_t)pattern[i++];
        if (sizeof(wchar_t) == 2 && IsHighSurrogate((std::uint16_t)scalar) && i < pattern.size())
        {
            std::uint16_t low = (std::uint16_t)pattern[i];
            if (IsLowSurrogate(low))
            {
                scalar = DecodeSurrogatePair((std::uint16_t)scalar, low);
                i++;
            }
        }
        scalars.push_back(scalar);
    }
    return scalars;
}

bool MatchAt(const DecodedRun& run, const std::vector<std::uint32_t>& pattern, bool caseSensitive,
             bool wholeWords, std::size_t cell)
{
    if (cell + pattern.size() > run.CellCount())
        return false;
    for (std::size_t i = 0; i < pattern.size(); ++i)
    {
        if (FoldScalar(run.Scalars[cell + i], caseSensitive) != FoldScalar(pattern[i], caseSensitive))
            return false;
    }
    if (wholeWords)
    {
        if (cell > 0 && IsWordScalar(run.Scalars[cell - 1]))
            return false;
        if (cell + pattern.size() < run.CellCount() && IsWordScalar(run.Scalars[cell + pattern.size()]))
            return false;
    }
    return true;
}

} // namespace

void DecodedRun::Clear()
{
    Text.clear();
    CellTextIndex.clear();
    RawStart.clear();
    RawEnd.clear();
    Scalars.clear();
    RawBytesConsumed = 0;
}

std::size_t DecodedRun::TextIndexForCellEnd(std::size_t cell) const
{
    if (cell >= CellTextIndex.size())
        return Text.size();
    return CellTextIndex[cell];
}

void DecodedRun::AppendCell(std::uint32_t scalar, std::int64_t rawStart, std::int64_t rawEnd)
{
    CellTextIndex.push_back(Text.size());
    RawStart.push_back(rawStart);
    RawEnd.push_back(rawEnd);
    Scalars.push_back(scalar);

    if (scalar > 0x10FFFF)
        scalar = Replacement;

    if (sizeof(wchar_t) == 2 && scalar > 0xFFFF)
    {
        scalar -= 0x10000;
        Text.push_back((wchar_t)(0xD800 + (scalar >> 10)));
        Text.push_back((wchar_t)(0xDC00 + (scalar & 0x3FF)));
    }
    else
    {
        Text.push_back((wchar_t)scalar);
    }
}

void DecodedRun::AppendRun(const DecodedRun& other)
{
    for (std::size_t i = 0; i < other.CellCount(); ++i)
        AppendCell(other.Scalars[i], other.RawStart[i], other.RawEnd[i]);
    RawBytesConsumed += other.RawBytesConsumed;
}

BomInfo DetectBom(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr)
        return {};
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
        return {BomEncoding::Utf8, 3};
    if (size >= 2 && data[0] == 0xFF && data[1] == 0xFE)
        return {BomEncoding::Utf16Le, 2};
    if (size >= 2 && data[0] == 0xFE && data[1] == 0xFF)
        return {BomEncoding::Utf16Be, 2};
    return {};
}

bool IsLikelyUtf8NoBom(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr || size == 0)
        return false;

    bool sawMultibyte = false;
    for (std::size_t i = 0; i < size;)
    {
        std::uint8_t lead = data[i];
        if (lead == 0)
            return false;
        if (lead < 0x80)
        {
            i++;
            continue;
        }

        std::size_t need = 0;
        std::uint32_t scalar = 0;
        std::uint32_t minScalar = 0;
        if (lead >= 0xC2 && lead <= 0xDF)
        {
            need = 2;
            scalar = lead & 0x1F;
            minScalar = 0x80;
        }
        else if (lead >= 0xE0 && lead <= 0xEF)
        {
            need = 3;
            scalar = lead & 0x0F;
            minScalar = 0x800;
        }
        else if (lead >= 0xF0 && lead <= 0xF4)
        {
            need = 4;
            scalar = lead & 0x07;
            minScalar = 0x10000;
        }
        else
            return false;

        if (i + need > size)
            break;
        for (std::size_t j = 1; j < need; ++j)
        {
            if (!IsContinuation(data[i + j]))
                return false;
            scalar = (scalar << 6) | (data[i + j] & 0x3F);
        }
        if (scalar < minScalar || scalar > 0x10FFFF || (scalar >= 0xD800 && scalar <= 0xDFFF))
            return false;
        sawMultibyte = true;
        i += need;
    }
    return sawMultibyte;
}

BomInfo DetectTextEncoding(const std::uint8_t* data, std::size_t size)
{
    BomInfo bom = DetectBom(data, size);
    if (bom.Encoding != BomEncoding::LegacyBytes)
        return bom;
    if (IsLikelyUtf8NoBom(data, size))
        return {BomEncoding::Utf8, 0};
    return bom;
}

bool IsDecodedEncoding(BomEncoding encoding)
{
    return encoding == BomEncoding::Utf8 || encoding == BomEncoding::Utf16Le ||
           encoding == BomEncoding::Utf16Be;
}

const char* EncodingDisplayName(BomEncoding encoding, std::int64_t textOffset)
{
    switch (encoding)
    {
    case BomEncoding::Utf8:
        return textOffset > 0 ? "UTF-8 BOM" : "UTF-8 no BOM";
    case BomEncoding::Utf16Le:
        return "UTF-16 LE";
    case BomEncoding::Utf16Be:
        return "UTF-16 BE";
    default:
        return nullptr;
    }
}

std::int64_t AlignToCodeUnit(BomEncoding encoding, std::int64_t offset, std::int64_t textOffset)
{
    if (offset < textOffset)
        return textOffset;
    if (encoding == BomEncoding::Utf16Le || encoding == BomEncoding::Utf16Be)
        return textOffset + ((offset - textOffset) & ~1ll);
    return offset;
}

DecodedRun DecodeBytes(BomEncoding encoding, const std::uint8_t* data, std::size_t size,
                       std::int64_t rawOffset, bool flush)
{
    DecodedRun run;
    if (data == nullptr || size == 0 || !IsDecodedEncoding(encoding))
        return run;

    std::size_t i = 0;
    if (encoding == BomEncoding::Utf8)
    {
        while (i < size)
        {
            std::uint8_t lead = data[i];
            std::uint32_t scalar = 0;
            std::size_t need = 0;
            std::uint32_t minScalar = 0;

            if (lead < 0x80)
            {
                scalar = lead;
                need = 1;
            }
            else if (lead >= 0xC2 && lead <= 0xDF)
            {
                scalar = lead & 0x1F;
                need = 2;
                minScalar = 0x80;
            }
            else if (lead >= 0xE0 && lead <= 0xEF)
            {
                scalar = lead & 0x0F;
                need = 3;
                minScalar = 0x800;
            }
            else if (lead >= 0xF0 && lead <= 0xF4)
            {
                scalar = lead & 0x07;
                need = 4;
                minScalar = 0x10000;
            }
            else
            {
                run.AppendCell(Replacement, rawOffset + (std::int64_t)i, rawOffset + (std::int64_t)i + 1);
                ++i;
                continue;
            }

            if (i + need > size)
            {
                if (!flush)
                    break;
                run.AppendCell(Replacement, rawOffset + (std::int64_t)i, rawOffset + (std::int64_t)size);
                i = size;
                break;
            }

            bool ok = true;
            for (std::size_t j = 1; j < need; ++j)
            {
                if (!IsContinuation(data[i + j]))
                {
                    ok = false;
                    break;
                }
                scalar = (scalar << 6) | (data[i + j] & 0x3F);
            }

            if (!ok)
            {
                run.AppendCell(Replacement, rawOffset + (std::int64_t)i, rawOffset + (std::int64_t)i + 1);
                ++i;
                continue;
            }

            if (scalar < minScalar || scalar > 0x10FFFF ||
                (scalar >= 0xD800 && scalar <= 0xDFFF))
            {
                run.AppendCell(Replacement, rawOffset + (std::int64_t)i, rawOffset + (std::int64_t)i + (std::int64_t)need);
            }
            else
            {
                run.AppendCell(scalar, rawOffset + (std::int64_t)i, rawOffset + (std::int64_t)i + (std::int64_t)need);
            }
            i += need;
        }
    }
    else
    {
        while (i + 1 < size)
        {
            std::int64_t start = rawOffset + (std::int64_t)i;
            std::uint16_t first = ReadUtf16(data + i, encoding);
            if (IsHighSurrogate(first))
            {
                if (i + 3 >= size)
                {
                    if (!flush)
                        break;
                    run.AppendCell(Replacement, start, start + 2);
                    i += 2;
                    continue;
                }
                std::uint16_t second = ReadUtf16(data + i + 2, encoding);
                if (IsLowSurrogate(second))
                {
                    run.AppendCell(DecodeSurrogatePair(first, second), start, start + 4);
                    i += 4;
                }
                else
                {
                    run.AppendCell(Replacement, start, start + 2);
                    i += 2;
                }
            }
            else if (IsLowSurrogate(first))
            {
                run.AppendCell(Replacement, start, start + 2);
                i += 2;
            }
            else
            {
                run.AppendCell(first, start, start + 2);
                i += 2;
            }
        }
        if (i < size && flush)
        {
            run.AppendCell(Replacement, rawOffset + (std::int64_t)i, rawOffset + (std::int64_t)size);
            i = size;
        }
    }
    run.RawBytesConsumed = i;
    return run;
}

TextElementMap BuildTextElementMap(const DecodedRun& run)
{
    TextElementMap map;
    map.CellBoundary.push_back(0);
    if (run.CellCount() == 0)
        return map;

    const SCRIPT_LOGATTR* attributes = nullptr;
    SCRIPT_STRING_ANALYSIS analysis = nullptr;
    if (!run.Text.empty() && run.Text.size() <= (std::size_t)std::numeric_limits<int>::max())
    {
        int textLength = (int)run.Text.size();
        int glyphCapacity = textLength + textLength / 2 + 16;
        if (SUCCEEDED(ScriptStringAnalyse(nullptr, run.Text.data(), textLength, glyphCapacity, -1,
                                          SSA_BREAK, 0, nullptr, nullptr, nullptr, nullptr, nullptr,
                                          &analysis)))
            attributes = ScriptString_pLogAttr(analysis);
    }

    for (std::size_t cell = 1; cell < run.CellCount(); ++cell)
    {
        bool isBoundary = false;
        std::size_t textIndex = run.CellTextIndex[cell];
        if (attributes != nullptr && textIndex < run.Text.size())
            isBoundary = attributes[textIndex].fCharStop != 0;
        else
            isBoundary = !IsFallbackContinuation(run.Scalars[cell], run.Scalars[cell - 1]);
        if (isBoundary && IsFallbackContinuation(run.Scalars[cell], run.Scalars[cell - 1]))
            isBoundary = false;
        if (isBoundary)
            map.CellBoundary.push_back(cell);
    }
    map.CellBoundary.push_back(run.CellCount());

    if (analysis != nullptr)
        ScriptStringFree(&analysis);
    return map;
}

std::size_t CountPatternCells(const std::wstring& pattern)
{
    return PatternScalars(pattern).size();
}

bool FindLiteralForward(const DecodedRun& run, const std::wstring& pattern, bool caseSensitive,
                        bool wholeWords, std::size_t startCell, LiteralMatch& match)
{
    std::vector<std::uint32_t> scalars = PatternScalars(pattern);
    if (scalars.empty() || scalars.size() > run.CellCount())
        return false;

    for (std::size_t cell = std::min(startCell, run.CellCount()); cell + scalars.size() <= run.CellCount(); ++cell)
    {
        if (!MatchAt(run, scalars, caseSensitive, wholeWords, cell))
            continue;
        match.CellIndex = cell;
        match.CellCount = scalars.size();
        match.RawStart = run.RawStart[cell];
        match.RawEnd = run.RawEnd[cell + scalars.size() - 1];
        return true;
    }
    return false;
}

bool FindLiteralBackward(const DecodedRun& run, const std::wstring& pattern, bool caseSensitive,
                         bool wholeWords, std::size_t cellLimit, LiteralMatch& match)
{
    std::vector<std::uint32_t> scalars = PatternScalars(pattern);
    if (scalars.empty() || scalars.size() > run.CellCount())
        return false;

    cellLimit = std::min(cellLimit, run.CellCount());
    if (cellLimit < scalars.size())
        return false;

    for (std::size_t cell = cellLimit - scalars.size() + 1; cell-- > 0;)
    {
        if (!MatchAt(run, scalars, caseSensitive, wholeWords, cell))
            continue;
        match.CellIndex = cell;
        match.CellCount = scalars.size();
        match.RawStart = run.RawStart[cell];
        match.RawEnd = run.RawEnd[cell + scalars.size() - 1];
        return true;
    }
    return false;
}

} // namespace Salamander::Unicode
