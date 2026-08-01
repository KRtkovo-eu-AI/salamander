// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <string.h>

static void CopyFileNameCharWithCase(char*& target, char*& source, BOOL upper,
                                     BOOL utf8ACP, const BYTE* lowerCase,
                                     const BYTE* upperCase)
{
    unsigned char first = (unsigned char)*source;
    if (!utf8ACP || first < 0x80)
    {
        *target++ = upper ? upperCase[first] : lowerCase[first];
        source++;
        return;
    }

    int sourceBytes;
    if ((first & 0xE0) == 0xC0)
        sourceBytes = 2;
    else if ((first & 0xF0) == 0xE0)
        sourceBytes = 3;
    else if ((first & 0xF8) == 0xF0)
        sourceBytes = 4;
    else
        sourceBytes = 1;

    for (int i = 1; i < sourceBytes; i++)
    {
        unsigned char continuation = (unsigned char)source[i];
        if (continuation == 0 || (continuation & 0xC0) != 0x80)
        {
            sourceBytes = 1;
            break;
        }
    }

    WCHAR wide[2];
    int wideChars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, sourceBytes,
                                        wide, _countof(wide));
    if (wideChars > 0)
    {
        int convertedChars = upper ? CharUpperBuffW(wide, wideChars) :
                                     CharLowerBuffW(wide, wideChars);
        if (convertedChars == wideChars)
        {
            char converted[8];
            int convertedBytes = WideCharToMultiByte(CP_UTF8, 0, wide, wideChars,
                                                     converted, _countof(converted), NULL, NULL);
            // AlterFileName's target buffer is only guaranteed to be as large as
            // the source. Keep the original code point if its mapping would grow.
            if (convertedBytes > 0 && convertedBytes <= sourceBytes)
            {
                memcpy(target, converted, convertedBytes);
                target += convertedBytes;
                source += sourceBytes;
                return;
            }
        }
    }

    memcpy(target, source, sourceBytes);
    target += sourceBytes;
    source += sourceBytes;
}
