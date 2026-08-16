// SPDX-FileCopyrightText: 2026 Sally Authors (0xeb)
// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <cwchar>
#include <string>
#include <vector>

namespace Salamander::Unicode
{

enum class BomEncoding
{
    LegacyBytes,
    Utf8,
    Utf16Le,
    Utf16Be,
};

struct BomInfo
{
    BomEncoding Encoding = BomEncoding::LegacyBytes;
    std::int64_t TextOffset = 0;
};

struct DecodedRun
{
    std::wstring Text;
    std::vector<std::size_t> CellTextIndex;
    std::vector<std::int64_t> RawStart;
    std::vector<std::int64_t> RawEnd;
    std::vector<std::uint32_t> Scalars;
    std::size_t RawBytesConsumed = 0;

    void Clear();
    std::size_t CellCount() const { return RawStart.size(); }
    std::size_t TextIndexForCellEnd(std::size_t cell) const;
    void AppendCell(std::uint32_t scalar, std::int64_t rawStart, std::int64_t rawEnd);
    void AppendRun(const DecodedRun& other);
};

// Maps user-visible text elements (caret stops / grapheme clusters) back to
// the scalar cells and raw file offsets kept by DecodedRun.  CellBoundary
// always contains zero and, for a non-empty run, the final CellCount().
struct TextElementMap
{
    std::vector<std::size_t> CellBoundary;

    std::size_t Count() const { return CellBoundary.empty() ? 0 : CellBoundary.size() - 1; }
    std::size_t CellStart(std::size_t element) const { return CellBoundary[element]; }
    std::size_t CellEnd(std::size_t element) const { return CellBoundary[element + 1]; }
};

struct LiteralMatch
{
    std::size_t CellIndex = 0;
    std::size_t CellCount = 0;
    std::int64_t RawStart = 0;
    std::int64_t RawEnd = 0;
};

BomInfo DetectBom(const std::uint8_t* data, std::size_t size);
BomInfo DetectTextEncoding(const std::uint8_t* data, std::size_t size);
bool IsLikelyUtf8NoBom(const std::uint8_t* data, std::size_t size);
bool IsDecodedEncoding(BomEncoding encoding);
const char* EncodingDisplayName(BomEncoding encoding, std::int64_t textOffset = 0);
std::int64_t AlignToCodeUnit(BomEncoding encoding, std::int64_t offset, std::int64_t textOffset);

DecodedRun DecodeBytes(BomEncoding encoding, const std::uint8_t* data, std::size_t size,
                       std::int64_t rawOffset, bool flush);
TextElementMap BuildTextElementMap(const DecodedRun& run);

std::size_t CountPatternCells(const std::wstring& pattern);
bool FindLiteralForward(const DecodedRun& run, const std::wstring& pattern, bool caseSensitive,
                        bool wholeWords, std::size_t startCell, LiteralMatch& match);
bool FindLiteralBackward(const DecodedRun& run, const std::wstring& pattern, bool caseSensitive,
                         bool wholeWords, std::size_t cellLimit, LiteralMatch& match);

} // namespace Salamander::Unicode
