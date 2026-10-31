// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../viewerhex.h"
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <vector>

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

void TestIssueRow()
{
    const unsigned char source[] = "[credential]\n\the";
    wchar_t glyphs[256], row[ViewerHexLineCapacity];
    BuildViewerHexGlyphTable(1250, glyphs);
    Check(FormatViewerHexBytes(source, source, 16, glyphs, row) == 68, "one cell per byte");
    Check(std::wstring(row) ==
              L"5B 63 72 65  64 65 6E 74  69 61 6C 5D  0A 09 68 65  [credential]\x00B7\x00B7he",
          "issue 737 row keeps both control-byte cells");
    Check(FormatViewerHexBytes(source, source, 3, glyphs, row) == 55, "short final row length");
    Check(std::wstring(row + 52) == L"[cr", "short final row text follows full hex column");
}

void TestAllBytesAndConversions()
{
    unsigned char raw[256], text[256];
    char table[256];
    wchar_t glyphs[256], before[ViewerHexLineCapacity], after[ViewerHexLineCapacity];
    BuildViewerHexGlyphTable(1250, glyphs);
    for (int i = 0; i < 256; ++i)
    {
        text[i] = (unsigned char)i;
        table[i] = 'X'; // Non-injective conversion cannot be reversed to recover raw bytes.
    }
    CaptureViewerBytes(raw, text, 256, NULL);
    for (int offset = 0; offset < 256; offset += 16)
    {
        FormatViewerHexBytes(raw + offset, text + offset, 16, glyphs, before);
        CaptureViewerBytes(raw + offset, text + offset, 16, table);
        FormatViewerHexBytes(raw + offset, text + offset, 16, glyphs, after);
        Check(std::wmemcmp(before, after, 52) == 0, "conversion must never change numeric hex cells");
        Check(std::wstring(after + 52) == L"XXXXXXXXXXXXXXXX", "conversion affects text cells");
    }
    for (int i = 0; i < 256; ++i)
        Check(raw[i] == i && text[i] == 'X', "original-byte and converted-text views stay separate");
    const unsigned char original[] = {0xE8, 0xFF, 0x80};
    std::memcpy(text, original, sizeof(original));
    table[0xE8] = 'c';
    CaptureViewerBytes(raw, text, sizeof(original), table);
    Check(raw[0] == 0xE8 && text[0] == 'c', "binary search/prefill and converted copy source differ");
    std::memcpy(text, original, sizeof(original));
    CaptureViewerBytes(raw, text, sizeof(original), NULL);
    Check(std::memcmp(raw, original, sizeof(original)) == 0 &&
              std::memcmp(text, original, sizeof(original)) == 0,
          "reloading after disabling conversion restores the original text");
}

void TestGlyphCells()
{
    for (int i = 0; i < 32; ++i)
        Check(ViewerHexGlyph((unsigned char)i, 1250) == 0xB7, "C0 controls use middle dots");
    Check(ViewerHexGlyph(0x7F, 1250) == 0xB7, "DEL uses a middle dot");
    Check(ViewerHexGlyph(0x81, 1250) == 0xB7, "undefined CP1250 C1 code uses a middle dot");
    Check(ViewerHexGlyph(0xAD, 1250) == 0xB7, "invisible soft hyphen uses a middle dot");
    Check(ViewerHexGlyph(0xE8, 1250) == L'\x010D', "CP1250 Czech letter is preserved");
    Check(ViewerHexGlyph(0xC0, 1251) == L'\x0410', "CP1251 Cyrillic letter is preserved");
    Check(ViewerHexGlyph(0x80, 1252) == L'\x20AC', "printable high-byte character is preserved");
    Check(ViewerHexGlyph(' ', 1250) == L' ', "ordinary spaces remain spaces");
    Check(ViewerHexGlyph(0x81, 932) == 0xB7, "DBCS lead byte must not consume the next cell");
    Check(ViewerHexGlyph(0xB6, 932) == L'\xFF76', "standalone DBCS halfwidth byte stays printable");
    Check(ViewerHexGlyph(0xCC, CP_UTF8) == 0xB7, "incomplete UTF8 byte must not consume next cell");
    Check(ViewerHexGlyph(0xFD, 1255) == 0xB7, "bidi format mark cannot move other byte cells");
    Check(ViewerHexGlyph(0xC0, 1255) == 0xB7, "combining mark cannot occupy another byte cell");
}

void TestSlidingCache()
{
    const size_t half = 30000, capacity = 2 * half;
    std::vector<unsigned char> file(5 * half), raw(capacity), text(capacity);
    char table[256];
    for (size_t i = 0; i < file.size(); ++i)
        file[i] = (unsigned char)((i * 37 + i / 251) & 255);
    for (int i = 0; i < 256; ++i)
        table[i] = (char)(i % 26 + 'a');
    auto read = [&](size_t destination, size_t offset, size_t count)
    {
        std::memcpy(text.data() + destination, file.data() + offset, count);
        CaptureViewerBytes(raw.data() + destination, text.data() + destination, count, table);
    };
    auto verify = [&](size_t offset)
    {
        for (size_t i = 0; i < capacity; ++i)
            Check(raw[i] == file[offset + i] && text[i] == (unsigned char)table[file[offset + i]],
                  "sliding raw and converted caches must share file offsets");
    };
    read(0, 0, half);
    read(half, half, half);
    verify(0);
    // LoadBehind discards the old first half, then appends the new half.
    MoveViewerBytes(raw.data(), text.data(), 0, half, half);
    read(half, capacity, half);
    verify(half);
    // LoadBefore shifts the retained half right, then prepends original bytes.
    MoveViewerBytes(raw.data(), text.data(), half, 0, half);
    read(0, 0, half);
    verify(0);
    // A partial load preserves a non-half-aligned cache boundary as well.
    const size_t prefix = 173;
    MoveViewerBytes(raw.data(), text.data(), prefix, 0, capacity - prefix);
    read(0, file.size() - prefix, prefix);
    for (size_t i = prefix; i < capacity; ++i)
        Check(raw[i] == file[i - prefix] && text[i] == (unsigned char)table[file[i - prefix]],
              "overlapping partial cache moves preserve both views");
}

void TestDrawCells()
{
    const int width = 256, height = 32, cell = 20;
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    HDC dc = CreateCompatibleDC(NULL);
    Check(dc != NULL, "create drawing test DC");
    void* pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &pixels, NULL, 0);
    Check(bitmap != NULL, "create drawing test bitmap");
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HFONT font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
                             FIXED_PITCH, L"Courier New");
    Check(font != NULL, "create drawing test font");
    HGDIOBJ oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    const wchar_t cells[] = {L'A', 0xB7, 0x05D0, 0x05D1, 0xB7, L'Z'};
    PatBlt(dc, 0, 0, width, height, WHITENESS);
    DrawViewerHexCells(dc, 0, 0, cells, 6, cell);
    GdiFlush();
    std::vector<unsigned char> whole(width * height * 4);
    std::memcpy(whole.data(), pixels, whole.size());
    PatBlt(dc, 0, 0, width, height, WHITENESS);
    for (int i = 5; i >= 0; --i)
        DrawViewerHexCells(dc, i * cell, 0, cells + i, 1, cell);
    GdiFlush();
    Check(std::memcmp(whole.data(), pixels, whole.size()) == 0,
          "whole row and independently positioned cells render identically, including bidi text");
    PatBlt(dc, 0, 0, width, height, WHITENESS);
    SetBkMode(dc, OPAQUE);
    SetBkColor(dc, RGB(0, 0, 255));
    DrawViewerHexCells(dc, cell, 0, cells + 1, 4, cell);
    GdiFlush();
    Check(GetPixel(dc, cell - 1, 0) == RGB(255, 255, 255) &&
              GetPixel(dc, cell, 0) == RGB(0, 0, 255) &&
              GetPixel(dc, 5 * cell, 0) == RGB(255, 255, 255),
          "selected text starts and ends at byte-cell boundaries");
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);
}
}

int main()
{
    TestIssueRow();
    TestAllBytesAndConversions();
    TestGlyphCells();
    TestSlidingCache();
    TestDrawCells();
    std::puts("Viewer hex tests passed: issue row, all 256 bytes/conversion invariance, glyph cells, sliding cache, GDI selection/bidi alignment.");
    return 0;
}
