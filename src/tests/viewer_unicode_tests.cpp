// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../common/unicode/ViewerBomText.h"

#include <cstdio>
#include <cstdlib>
#include <string>

using Salamander::Unicode::BomEncoding;
using Salamander::Unicode::BuildTextElementMap;
using Salamander::Unicode::DecodeBytes;

namespace
{

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        std::exit(1);
    }
}

Salamander::Unicode::DecodedRun DecodeUtf8(const std::string& text)
{
    return DecodeBytes(BomEncoding::Utf8,
                       reinterpret_cast<const std::uint8_t*>(text.data()),
                       text.size(), 0, true);
}

void CheckElementCount(const std::string& text, std::size_t expected, const char* message)
{
    Salamander::Unicode::DecodedRun run = DecodeUtf8(text);
    Check(BuildTextElementMap(run).Count() == expected, message);
}

} // namespace

int main()
{
    // Latin NFD: A + COMBINING ACUTE ACCENT.
    CheckElementCount("A\xCC\x81", 1, "decomposed Latin letter must be one text element");

    // Hebrew SHIN + QAMATS (section 4 of the issue sample).
    CheckElementCount("\xD7\xA9\xD6\xB8", 1, "Hebrew base and niqqud must be one text element");

    // Devanagari KA + VIRAMA + SSA (section 5).
    CheckElementCount("\xE0\xA4\x95\xE0\xA5\x8D\xE0\xA4\xB7", 1,
                      "Indic conjunct must expose one caret stop");

    // Thai KO KAI + MAI EK combining tone mark (section 7).
    CheckElementCount("\xE0\xB8\x81\xE0\xB9\x88", 1,
                      "Thai base and tone mark must be one text element");

    // A compact Zalgo sequence: U plus several stacked combining marks.
    CheckElementCount("U\xCC\xB4\xCC\xA2\xCC\x9B\xCC\xBA\xCC\xB0", 1,
                      "stacked Zalgo marks must stay with their base character");

    CheckElementCount("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", 3,
                      "three CJK ideographs must count as three text elements");
    CheckElementCount("\xF0\x9F\x98\x80", 1,
                      "supplementary Unicode scalar must be one text element");
    CheckElementCount("\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB", 1,
                      "emoji ZWJ sequence must be one text element");

    Salamander::Unicode::DecodedRun mapped = DecodeUtf8("A\xCC\x81" "B");
    Salamander::Unicode::TextElementMap elements = BuildTextElementMap(mapped);
    Check(elements.Count() == 2, "combined letter followed by ASCII must have two elements");
    Check(elements.CellStart(0) == 0 && elements.CellEnd(0) == 2,
          "first element must cover the base and combining scalar");
    Check(mapped.RawStart[elements.CellStart(0)] == 0 && mapped.RawEnd[elements.CellEnd(0) - 1] == 3,
          "first element must map to all three UTF-8 bytes");
    Check(mapped.RawStart[elements.CellStart(1)] == 3 && mapped.RawEnd[elements.CellEnd(1) - 1] == 4,
          "second element raw range must start after the combined letter");

    Check(std::string(Salamander::Unicode::EncodingDisplayName(BomEncoding::Utf8, 3)) == "UTF-8 BOM",
          "UTF-8 BOM caption name");
    Check(std::string(Salamander::Unicode::EncodingDisplayName(BomEncoding::Utf8, 0)) == "UTF-8 no BOM",
          "UTF-8 no-BOM caption name");
    Check(std::string(Salamander::Unicode::EncodingDisplayName(BomEncoding::Utf16Le)) == "UTF-16 LE",
          "UTF-16 LE caption name");
    Check(std::string(Salamander::Unicode::EncodingDisplayName(BomEncoding::Utf16Be)) == "UTF-16 BE",
          "UTF-16 BE caption name");
    Check(Salamander::Unicode::EncodingDisplayName(BomEncoding::LegacyBytes) == nullptr,
          "legacy encoding keeps the conversion-table caption path");

    std::puts("viewer_unicode_tests passed");
    return 0;
}
