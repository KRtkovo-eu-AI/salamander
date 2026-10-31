// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <cstring>

// Largest row: 19 offset characters, colon + space, 52 hex characters,
// 16 byte cells, and a terminator. Text-mode rendering does not use this path.
const int ViewerHexLineCapacity = 90;

inline void CaptureViewerBytes(unsigned char* raw, unsigned char* text,
                               size_t count, const char* conversion)
{
    memcpy(raw, text, count);
    if (conversion != NULL)
        for (size_t i = 0; i < count; ++i)
            text[i] = (unsigned char)conversion[raw[i]];
}

inline void MoveViewerBytes(unsigned char* raw, unsigned char* text,
                            size_t destination, size_t source, size_t count)
{
    memmove(raw + destination, raw + source, count);
    memmove(text + destination, text + source, count);
}

inline wchar_t ViewerHexGlyph(unsigned char value, UINT codePage)
{
    const wchar_t dot = L'\x00B7';
    if (value < 0x20 || value == 0x7F)
        return dot;

    // Decode each byte separately. A hex cell must never consume the next byte
    // as a DBCS/UTF-8 continuation, even when a regional code page uses DBCS.
    const char byte = (char)value;
    wchar_t glyph;
    if (MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, &byte, 1, &glyph, 1) != 1)
        return dot;
    WORD type1 = 0, type3 = 0;
    if (!GetStringTypeW(CT_CTYPE1, &glyph, 1, &type1) ||
        !GetStringTypeW(CT_CTYPE3, &glyph, 1, &type3) ||
        (type1 & C1_CNTRL) != 0 || (type3 & C3_NONSPACING) != 0 ||
        glyph == 0x00AD || (glyph >= 0x200B && glyph <= 0x200F) ||
        glyph == 0xFFFD)
        return dot;
    return glyph;
}

inline void BuildViewerHexGlyphTable(UINT codePage, wchar_t (&glyphs)[256])
{
    for (int i = 0; i < 256; ++i)
        glyphs[i] = ViewerHexGlyph((unsigned char)i, codePage);
}

// Writes only the numeric-byte and character columns (at most 68 cells).
// The text bytes may be converted; the numeric column always reads raw bytes.
inline int FormatViewerHexBytes(const unsigned char* raw, const unsigned char* text,
                                int count, const wchar_t (&glyphs)[256], wchar_t* output)
{
    const wchar_t digits[] = L"0123456789ABCDEF";
    wchar_t* dest = output;
    for (int i = 0; i < 16; ++i)
    {
        *dest++ = i < count ? digits[raw[i] >> 4] : L' ';
        *dest++ = i < count ? digits[raw[i] & 15] : L' ';
        *dest++ = L' ';
        if ((i % 4) == 3)
            *dest++ = L' ';
    }
    for (int i = 0; i < count; ++i)
        *dest++ = glyphs[text[i]];
    *dest = 0;
    return (int)(dest - output);
}

inline void DrawViewerHexCells(HDC dc, int x, int y, const wchar_t* text,
                              int count, int cellWidth)
{
    // Keep every byte at its own position, including selected segments and
    // right-to-left scripts. Shaping is appropriate for text mode, not a byte grid.
    int advances[ViewerHexLineCapacity];
    for (int i = 0; i < count; ++i)
        advances[i] = cellWidth;
    ExtTextOutW(dc, x, y, ETO_IGNORELANGUAGE, NULL, text, count, advances);
}
