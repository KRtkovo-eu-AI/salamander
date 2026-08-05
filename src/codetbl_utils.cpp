// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "codetbl_utils.h"

#include <limits.h>
#include <stdlib.h>
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
