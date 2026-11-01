// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "codetbl_utils.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

DWORD GetConversionAutoCodePage(DWORD activeCodePage, DWORD systemLocaleCodePage)
{
    if (activeCodePage == CP_UTF8 && systemLocaleCodePage != 0)
        return systemLocaleCodePage;
    return activeCodePage;
}

DWORD GetSystemLocaleAnsiCodePage()
{
    WCHAR value[16];
    int length = GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE,
                                value, ARRAYSIZE(value));
    if (length <= 1)
        return 0;

    WCHAR* end = NULL;
    unsigned long codePage = wcstoul(value, &end, 10);
    if (end == value || *end != L'\0' || codePage > MAXUINT)
        return 0;
    return (DWORD)codePage;
}

DWORD GetEffectiveConversionCodePage()
{
    DWORD activeCodePage = GetACP();
    return GetConversionAutoCodePage(
        activeCodePage,
        activeCodePage == CP_UTF8 ? GetSystemLocaleAnsiCodePage() : 0);
}

BOOL ParseConversionCodePageIdentifier(const char* text, size_t length, DWORD* identifier)
{
    if (text == NULL || identifier == NULL)
        return FALSE;

    size_t begin = 0;
    while (begin < length && (text[begin] == ' ' || text[begin] == '\t'))
        begin++;
    while (length > begin && (text[length - 1] == ' ' || text[length - 1] == '\t'))
        length--;
    if (begin == length)
        return FALSE;

    DWORD value = 0;
    for (size_t i = begin; i < length; i++)
    {
        if (text[i] < '0' || text[i] > '9')
            return FALSE;
        DWORD digit = (DWORD)(text[i] - '0');
        if (value > (MAXDWORD - digit) / 10)
            return FALSE;
        value = value * 10 + digit;
    }
    if (value == 0)
        return FALSE;

    *identifier = value;
    return TRUE;
}

BOOL ConvertConversionTableText(const char* source, UINT sourceCodePage,
                                UINT destinationCodePage, char* destination,
                                size_t destinationSize)
{
    if (destination == NULL || destinationSize == 0)
        return FALSE;
    destination[0] = 0;
    if (source == NULL || !IsValidCodePage(sourceCodePage) ||
        !IsValidCodePage(destinationCodePage))
    {
        return FALSE;
    }

    int wideLength = MultiByteToWideChar(sourceCodePage, 0, source, -1, NULL, 0);
    if (wideLength <= 0)
        return FALSE;

    WCHAR* wide = (WCHAR*)malloc((size_t)wideLength * sizeof(WCHAR));
    if (wide == NULL)
        return FALSE;
    BOOL result = FALSE;
    if (MultiByteToWideChar(sourceCodePage, 0, source, -1, wide, wideLength) != 0)
    {
        int convertedLength = WideCharToMultiByte(destinationCodePage, 0, wide, -1,
                                                  NULL, 0, NULL, NULL);
        if (convertedLength > 0 && (size_t)convertedLength <= destinationSize &&
            WideCharToMultiByte(destinationCodePage, 0, wide, -1, destination,
                                convertedLength, NULL, NULL) != 0)
        {
            result = TRUE;
        }
    }
    free(wide);
    if (!result)
        destination[0] = 0;
    return result;
}

BOOL ConvertLegacyViewerTextToWide(const char* source, int sourceLength,
                                   UINT sourceCodePage, std::wstring* destination)
{
    if (destination == NULL || sourceLength < 0 ||
        (source == NULL && sourceLength != 0))
    {
        return FALSE;
    }

    destination->clear();
    if (sourceLength == 0)
        return TRUE;

    int required = MultiByteToWideChar(sourceCodePage, 0, source, sourceLength,
                                       NULL, 0);
    if (required <= 0)
        return FALSE;

    destination->resize(required);
    int converted = MultiByteToWideChar(sourceCodePage, 0, source, sourceLength,
                                        &(*destination)[0], required);
    if (converted != required)
    {
        destination->clear();
        return FALSE;
    }
    return TRUE;
}

BOOL ConvertProcessTextToLegacyViewerText(const char* source, UINT processCodePage,
                                          UINT legacyCodePage, std::string* destination)
{
    if (destination == NULL)
        return FALSE;
    destination->clear();
    if (source == NULL || !IsValidCodePage(processCodePage) || !IsValidCodePage(legacyCodePage))
        return FALSE;
    if (source[0] == 0)
        return TRUE;

    int wideLength = MultiByteToWideChar(processCodePage, MB_ERR_INVALID_CHARS, source, -1, NULL, 0);
    if (wideLength <= 1)
        return FALSE;
    std::wstring wide((size_t)wideLength, L'\0');
    if (MultiByteToWideChar(processCodePage, MB_ERR_INVALID_CHARS, source, -1, &wide[0], wideLength) != wideLength)
        return FALSE;

    // UTF-8/UTF-7 targets reject the lpUsedDefaultChar argument; they are lossless anyway.
    BOOL lossless = legacyCodePage == CP_UTF8 || legacyCodePage == CP_UTF7;
    DWORD flags = lossless ? 0 : WC_NO_BEST_FIT_CHARS;
    BOOL usedDefault = FALSE;
    BOOL* usedDefaultPtr = lossless ? NULL : &usedDefault;
    int length = WideCharToMultiByte(legacyCodePage, flags, wide.c_str(), -1, NULL, 0, NULL, usedDefaultPtr);
    if (length <= 1 || usedDefault)
        return FALSE;
    destination->resize((size_t)length);
    if (WideCharToMultiByte(legacyCodePage, flags, wide.c_str(), -1, &(*destination)[0], length,
                            NULL, usedDefaultPtr) != length ||
        usedDefault)
    {
        destination->clear();
        return FALSE;
    }
    destination->resize((size_t)length - 1); // drop the converted terminator
    return TRUE;
}

int ConvertLegacyViewerTextToProcessText(const char* source, int sourceLength,
                                         UINT legacyCodePage, UINT processCodePage,
                                         char* destination, int destinationSize)
{
    if (destination == NULL || destinationSize <= 0)
        return 0;
    destination[0] = 0;
    std::wstring wide;
    if (sourceLength <= 0 || !IsValidCodePage(processCodePage) ||
        !ConvertLegacyViewerTextToWide(source, sourceLength, legacyCodePage, &wide) || wide.empty())
    {
        return 0;
    }

    int wideLength = (int)wide.size();
    while (wideLength > 0)
    {
        int required = WideCharToMultiByte(processCodePage, 0, wide.c_str(), wideLength, NULL, 0, NULL, NULL);
        if (required > 0 && required <= destinationSize - 1)
        {
            int written = WideCharToMultiByte(processCodePage, 0, wide.c_str(), wideLength,
                                              destination, required, NULL, NULL);
            if (written != required)
                break;
            destination[written] = 0;
            return written;
        }
        // Too long: drop one character, keeping a surrogate pair together.
        wideLength--;
        if (wideLength > 0 && IS_LOW_SURROGATE(wide[wideLength]) && IS_HIGH_SURROGATE(wide[wideLength - 1]))
            wideLength--;
    }
    destination[0] = 0;
    return 0;
}

BOOL BuildLegacyLowerCaseTable(UINT legacyCodePage, BYTE* table)
{
    if (table == NULL)
        return FALSE;
    for (int i = 0; i < 256; i++)
        table[i] = (BYTE)(i >= 'A' && i <= 'Z' ? i - 'A' + 'a' : i);
    CPINFO info;
    if (!IsValidCodePage(legacyCodePage) || legacyCodePage == CP_UTF8 || legacyCodePage == CP_UTF7 ||
        !GetCPInfo(legacyCodePage, &info) || info.MaxCharSize != 1)
    {
        return FALSE; // multi-byte pages keep the ASCII-only map (bytes >= 0x80 are not whole characters)
    }
    for (int i = 0x80; i < 256; i++)
    {
        char source = (char)i;
        WCHAR wide;
        if (MultiByteToWideChar(legacyCodePage, MB_ERR_INVALID_CHARS, &source, 1, &wide, 1) != 1)
            continue;
        WCHAR lower = wide;
        if (LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_LOWERCASE, &wide, 1, &lower, 1, NULL, NULL, 0) != 1 ||
            lower == wide)
        {
            continue;
        }
        // Keep the byte when the lower-case letter has no exact single-byte form
        // (e.g. a best-fit '?' would make unrelated letters compare equal).
        char result;
        BOOL usedDefault = FALSE;
        if (WideCharToMultiByte(legacyCodePage, WC_NO_BEST_FIT_CHARS, &lower, 1, &result, 1, NULL, &usedDefault) == 1 &&
            !usedDefault)
        {
            table[i] = (BYTE)result;
        }
    }
    return TRUE;
}

BOOL ShouldPreferWindowsCodePageText(const char* source, int sourceLength,
                                     UINT windowsCodePage,
                                     const char* recognizedCodePage)
{
    if (source == NULL || sourceLength <= 0 || recognizedCodePage == NULL ||
        !IsValidCodePage(windowsCodePage))
    {
        return FALSE;
    }

    if (_strnicmp(recognizedCodePage, "ISO-8859-", 9) == 0)
    {
        for (int i = 0; i < sourceLength; ++i)
        {
            unsigned char byte = (unsigned char)source[i];
            if (byte < 0x80 || byte > 0x9F)
                continue;

            WCHAR character = 0;
            char encoded = (char)byte;
            if (MultiByteToWideChar(windowsCodePage, MB_ERR_INVALID_CHARS,
                                    &encoded, 1, &character, 1) != 1)
            {
                continue;
            }

            WORD type = 0;
            if (GetStringTypeW(CT_CTYPE1, &character, 1, &type) &&
                (type & C1_ALPHA) != 0)
            {
                return TRUE;
            }
        }
        return FALSE;
    }

    if (windowsCodePage != 1250 || _stricmp(recognizedCodePage, "CP852") != 0)
        return FALSE;

    int quotePairs = 0;
    int separators = 0;
    int openQuote = -1;
    for (int i = 0; i < sourceLength; ++i)
    {
        const unsigned char byte = (unsigned char)source[i];
        const BOOL leftBoundary = i == 0 ||
            (unsigned char)source[i - 1] <= ' ';
        const BOOL rightBoundary = i + 1 == sourceLength ||
            (unsigned char)source[i + 1] <= ' ';

        if (byte == 0x93 && leftBoundary && i + 1 < sourceLength &&
            (unsigned char)source[i + 1] > ' ')
        {
            openQuote = i;
        }
        else if (byte == 0x94 && openQuote >= 0 && i > openQuote + 1 &&
                 (unsigned char)source[i - 1] > ' ' && rightBoundary)
        {
            quotePairs++;
            openQuote = -1;
        }
        else if (byte == 0x9B && leftBoundary && rightBoundary)
        {
            separators++;
        }
    }

    return quotePairs >= 2 ||
           (quotePairs >= 1 && separators >= 2) ||
           separators >= 4;
}
