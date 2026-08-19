// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"
#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>
#include <cwctype>
#include <string>
#include <vector>
#undef PathIsPrefix // propsys/shlwapi can define this macro; plugins.h has a method with the same name

#include "filetags.h"

#include "cfgdlg.h"
#include "plugins.h"
#include "fileswnd.h"
#include "mainwnd.h"
#include "salinflt.h"

static int Utf8CharStart(const char* text, int index)
{
    while (index > 0 && ((unsigned char)text[index] & 0xC0) == 0x80)
        index--;
    return index;
}

static int Utf8PrevCharStart(const char* text, int index)
{
    if (index <= 0)
        return -1;
    index--;
    return Utf8CharStart(text, index);
}

static BOOL IsValidUtf8Text(const char* text, int textLen)
{
    return text != NULL && textLen >= 0 &&
           MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, textLen, NULL, 0) != 0;
}


void CopyStringTruncateUtf8(char* dst, int dstSize, const char* src)
{
    if (dst == NULL || dstSize <= 0)
        return;

    if (src == NULL)
    {
        dst[0] = 0;
        return;
    }

    int srcLen = (int)strlen(src);
    BOOL srcIsValidUtf8 = IsValidUtf8Text(src, srcLen);
    lstrcpyn(dst, src, dstSize);

    if (srcIsValidUtf8 && srcLen >= dstSize)
    {
        int len = (int)strlen(dst);
        while (len > 0 && !IsValidUtf8Text(dst, len))
        {
            len = Utf8PrevCharStart(dst, len);
            if (len < 0)
                len = 0;
            dst[len] = 0;
        }
    }
}

//****************************************************************************
//
// CTruncatedString
//
// documented in Hck
//

CTruncatedString::CTruncatedString()
{
    Text = NULL;
    TextW = NULL;
    SubStrIndex = -1;
    SubStrLen = 0;
    TruncatedText = NULL;
    TruncatedTextW = NULL;
    UseWideText = FALSE;
}

CTruncatedString::~CTruncatedString()
{
    if (Text != NULL)
        free(Text);
    if (TextW != NULL)
        free(TextW);
    if (TruncatedText != NULL)
        free(TruncatedText);
    if (TruncatedTextW != NULL)
        free(TruncatedTextW);
};

static char* DupWideAsUtf8(const wchar_t* text)
{
    if (text == NULL)
        text = L"";
    int len = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
        len = 1;
    char* ret = (char*)malloc(len);
    if (ret == NULL)
        return NULL;
    if (len == 1)
        ret[0] = 0;
    else
        WideCharToMultiByte(CP_UTF8, 0, text, -1, ret, len, NULL, NULL);
    return ret;
}

static wchar_t* DupWideString(const wchar_t* text, int chars = -1)
{
    if (text == NULL)
        text = L"";
    if (chars < 0)
        chars = (int)wcslen(text);
    wchar_t* ret = (wchar_t*)malloc((chars + 1) * sizeof(wchar_t));
    if (ret == NULL)
        return NULL;
    if (chars > 0)
        memcpy(ret, text, chars * sizeof(wchar_t));
    ret[chars] = 0;
    return ret;
}

BOOL CTruncatedString::CopyFrom(const CTruncatedString* src)
{
    if (Text != NULL)
    {
        free(Text);
        Text = NULL;
    }
    if (src->Text != NULL)
    {
        Text = DupStr(src->Text);
        if (Text == NULL)
        {
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    if (TextW != NULL)
    {
        free(TextW);
        TextW = NULL;
    }
    if (src->TextW != NULL)
    {
        TextW = DupWideString(src->TextW);
        if (TextW == NULL)
        {
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    SubStrIndex = src->SubStrIndex;
    SubStrLen = src->SubStrLen;
    if (TruncatedText != NULL)
    {
        free(TruncatedText);
        TruncatedText = NULL;
    }
    if (src->TruncatedText != NULL)
    {
        TruncatedText = DupStr(src->TruncatedText);
        if (TruncatedText == NULL)
        {
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    if (TruncatedTextW != NULL)
    {
        free(TruncatedTextW);
        TruncatedTextW = NULL;
    }
    if (src->TruncatedTextW != NULL)
    {
        TruncatedTextW = DupWideString(src->TruncatedTextW);
        if (TruncatedTextW == NULL)
        {
            TRACE_E(LOW_MEMORY);
            return FALSE;
        }
    }
    UseWideText = src->UseWideText;
    return TRUE;
}

BOOL CTruncatedString::Set(const char* str, const char* subStr)
{
    UseWideText = FALSE;
    if (TextW != NULL)
    {
        free(TextW);
        TextW = NULL;
    }
    if (TruncatedTextW != NULL)
    {
        free(TruncatedTextW);
        TruncatedTextW = NULL;
    }
    if (TruncatedText != NULL)
    {
        free(TruncatedText);
        TruncatedText = NULL;
    }
    int len = (int)strlen(str);
    int subStrIndex = -1;
    int subStrLen = 0;
    if (subStr != NULL)
    {
        const char* p = str;
        int doubles = 0;
        while (*p != 0)
        {
            if (*p == '%')
            {
                if (*(p + 1) == '%')
                {
                    p++;
                    doubles++; // "%%" will be shortened by sprintf to "%"
                }
                else
                {
                    if (*(p + 1) == 's')
                    {
                        subStrIndex = (int)(p - str - doubles);
                        break;
                    }
                    else
                    {
                        TRACE_E("CTruncatedString::Set: unknown format specifier in str:" << str);
                        break;
                    }
                }
            }
            p++;
        }
        if (subStrIndex == -1)
        {
            TRACE_E("CTruncatedString::Set: %s was not found in str:" << str);
        }
        else
        {
            len -= 2; // subtract the %s that will be removed
            subStrLen = (int)strlen(subStr);
            len += subStrLen;
        }
    }
    char* text = (char*)malloc(len + 1);
    if (text == NULL)
        return FALSE;
    if (Text != NULL)
        free(Text);
    Text = text;
    size_t textCapacity = (size_t)len + 1;
    if (subStrIndex != -1)
    {
        int prefixLen = subStrIndex;
        memcpy(Text, str, prefixLen);
        memcpy(Text + prefixLen, subStr, subStrLen);
        const char* suffix = str + prefixLen + 2; // skip "%s"
        size_t suffixOffset = (size_t)(prefixLen + subStrLen);
        _snprintf_s(Text + suffixOffset, textCapacity - suffixOffset, _TRUNCATE, "%s", suffix);
        SubStrIndex = subStrIndex;
        SubStrLen = subStrLen;
    }
    else
    {
        _snprintf_s(Text, textCapacity, _TRUNCATE, "%s", str);
        SubStrIndex = -1;
        SubStrLen = 0;
    }

    return TRUE;
}

BOOL CTruncatedString::SetW(const wchar_t* str, const wchar_t* subStr)
{
    UseWideText = TRUE;
    if (Text != NULL)
    {
        free(Text);
        Text = NULL;
    }
    if (TextW != NULL)
    {
        free(TextW);
        TextW = NULL;
    }
    if (TruncatedText != NULL)
    {
        free(TruncatedText);
        TruncatedText = NULL;
    }
    if (TruncatedTextW != NULL)
    {
        free(TruncatedTextW);
        TruncatedTextW = NULL;
    }

    if (str == NULL)
        str = L"";

    int subStrIndex = -1;
    int subStrLen = 0;
    const wchar_t* insertPos = NULL;
    if (subStr != NULL)
    {
        const wchar_t* p = str;
        int doubles = 0;
        while (*p != 0)
        {
            if (*p == L'%')
            {
                if (*(p + 1) == L'%')
                {
                    p++;
                    doubles++;
                }
                else
                {
                    if (*(p + 1) == L's')
                    {
                        insertPos = p;
                        subStrIndex = (int)(p - str - doubles);
                        break;
                    }
                    else
                    {
                        TRACE_E("CTruncatedString::SetW: unknown format specifier");
                        break;
                    }
                }
            }
            p++;
        }
        if (subStrIndex == -1)
            TRACE_E("CTruncatedString::SetW: %s was not found");
        else
            subStrLen = (int)wcslen(subStr);
    }

    if (insertPos != NULL)
    {
        int prefixLen = (int)(insertPos - str);
        int suffixLen = (int)wcslen(insertPos + 2);
        TextW = (wchar_t*)malloc((prefixLen + subStrLen + suffixLen + 1) * sizeof(wchar_t));
        if (TextW == NULL)
            return FALSE;
        memcpy(TextW, str, prefixLen * sizeof(wchar_t));
        if (subStrLen > 0)
            memcpy(TextW + prefixLen, subStr, subStrLen * sizeof(wchar_t));
        memcpy(TextW + prefixLen + subStrLen, insertPos + 2, (suffixLen + 1) * sizeof(wchar_t));
        SubStrIndex = subStrIndex;
        SubStrLen = subStrLen;
    }
    else
    {
        TextW = DupWideString(str);
        if (TextW == NULL)
            return FALSE;
        SubStrIndex = -1;
        SubStrLen = 0;
    }

    Text = DupWideAsUtf8(TextW);
    return Text != NULL;
}

const char*
CTruncatedString::Get()
{
    if (SubStrIndex == -1 || TruncatedText == NULL)
    {
        if (Text == NULL)
        {
            TRACE_E("Text == NULL");
            return "";
        }
        else
            return Text;
    }
    else
    {
        return TruncatedText;
    }
}

const wchar_t*
CTruncatedString::GetW()
{
    if (!UseWideText)
        return L"";
    if (SubStrIndex == -1 || TruncatedTextW == NULL)
        return TextW != NULL ? TextW : L"";
    return TruncatedTextW;
}

BOOL CTruncatedString::TruncateText(HWND hWindow, BOOL forMessageBox)
{
    // if there is nothing to truncate, exit
    if (SubStrIndex == -1)
        return TRUE;

    BOOL ret = TRUE;

    HDC hDC = HANDLES(GetDC(hWindow));
    HFONT hFont = (HFONT)SendMessage(hWindow, WM_GETFONT, 0, 0);
    HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

    int fitChars;
    int alpDx[8000]; // for measuring widths

    if (UseWideText && TextW != NULL)
    {
        int textLen = (int)wcslen(TextW);
        wchar_t* truncated = (wchar_t*)malloc((textLen + 1 + 3) * sizeof(wchar_t));
        if (truncated == NULL)
        {
            TRACE_E(LOW_MEMORY);
            ret = FALSE;
        }
        else
        {
            if (TruncatedTextW != NULL)
                free(TruncatedTextW);
            TruncatedTextW = truncated;

            if (forMessageBox)
            {
                int maxWidth = 400;
                SIZE sz;
                GetTextExtentExPointW(hDC, TextW + SubStrIndex, SubStrLen, maxWidth, &fitChars, alpDx, &sz);
                if (fitChars < SubStrLen)
                {
                    memcpy(TruncatedTextW, TextW, (SubStrIndex + fitChars) * sizeof(wchar_t));
                    memcpy(TruncatedTextW + SubStrIndex + fitChars, L"...", 3 * sizeof(wchar_t));
                    wcscpy(TruncatedTextW + SubStrIndex + fitChars + 3, TextW + SubStrIndex + SubStrLen);
                }
                else
                    memcpy(TruncatedTextW, TextW, (textLen + 1) * sizeof(wchar_t));
            }
            else
            {
                RECT r;
                GetClientRect(hWindow, &r);
                int maxWidth = r.right;

                SIZE sz;
                if (textLen > 8000)
                {
                    TRACE_E("Text was truncated (to 7999 characters)");
                    TextW[7999] = 0;
                    textLen = 7999;
                }
                GetTextExtentExPointW(hDC, TextW, textLen, 0, NULL, alpDx, &sz);
                if (sz.cx > maxWidth)
                {
                    int width = sz.cx;
                    GetTextExtentPoint32W(hDC, L"...", 3, &sz);
                    int ellipsisWidth = sz.cx;
                    int index = SubStrIndex + SubStrLen - 1;
                    maxWidth -= ellipsisWidth;
                    while (width > maxWidth && index >= SubStrIndex)
                    {
                        int prev = index > 0 ? alpDx[index - 1] : 0;
                        width -= (alpDx[index] - prev);
                        index--;
                        // don't split a surrogate pair: if index now points to a
                        // high surrogate (D800-DBFF) followed by a low surrogate
                        // (DC00-DFFF), retreat one more to remove the whole pair
                        if (index >= SubStrIndex &&
                            TextW[index] >= 0xD800 && TextW[index] <= 0xDBFF &&
                            index + 1 < textLen &&
                            TextW[index + 1] >= 0xDC00 && TextW[index + 1] <= 0xDFFF)
                        {
                            prev = index > 0 ? alpDx[index - 1] : 0;
                            width -= (alpDx[index] - prev);
                            index--;
                        }
                    }
                    int keep = index + 1;
                    memcpy(TruncatedTextW, TextW, keep * sizeof(wchar_t));
                    memcpy(TruncatedTextW + keep, L"...", 3 * sizeof(wchar_t));
                    wcscpy(TruncatedTextW + keep + 3, TextW + SubStrIndex + SubStrLen);
                }
                else
                    memcpy(TruncatedTextW, TextW, (textLen + 1) * sizeof(wchar_t));
            }
            if (TruncatedText != NULL)
                free(TruncatedText);
            TruncatedText = DupWideAsUtf8(TruncatedTextW);
        }
        SelectObject(hDC, hOldFont);
        HANDLES(ReleaseDC(hWindow, hDC));
        return ret;
    }

    int textLen = (int)strlen(Text);
    char* truncated = (char*)malloc(textLen + 1 + 3); // 3: reserve for an ellipsis in the extreme case
    if (truncated == NULL)
    {
        TRACE_E(LOW_MEMORY);
        ret = FALSE;
    }
    else
    {
        if (TruncatedText != NULL)
            free(TruncatedText);
        TruncatedText = truncated;

        if (forMessageBox)
        {
            // for message boxes -- we just ensure that the substring is not larger than 400 points (so it fits even on 640x480)
            int chars = SubStrLen;
            int maxWidth = 400;
            SIZE sz;
            GetTextExtentExPoint(hDC, Text + SubStrIndex, SubStrLen, maxWidth, &fitChars, alpDx, &sz);
            if (fitChars < SubStrLen)
            {
                // first part with the truncated substring
                memcpy(TruncatedText, Text, SubStrIndex + fitChars);
                // ellipsis
                memcpy(TruncatedText + SubStrIndex + fitChars, "...", 3);
                // the rest
                strcpy(TruncatedText + SubStrIndex + fitChars + 3, Text + SubStrIndex + SubStrLen);
            }
            else
                memcpy(TruncatedText, Text, textLen + 1); // just copy -— we still fit
        }
        else
        {
            // single-line layout for dialogs
            // determine the maximum width we can afford
            RECT r;
            GetClientRect(hWindow, &r);
            int maxWidth = r.right;

            SIZE sz;
            if (textLen > 8000)
            {
                TRACE_E("Text was truncated (to 7999 characters)");
                Text[7999] = 0;
                textLen = 7999;
            }
            GetTextExtentExPoint(hDC, Text, textLen, 0, NULL, alpDx, &sz);
            if (sz.cx > maxWidth)
            {
                int width = sz.cx;

                GetTextExtentPoint32(hDC, "...", 3, &sz);
                int ellipsisWidth = sz.cx;

                // we will subtract from the part that can be shortened
                int index = SubStrIndex + SubStrLen - 1;
                maxWidth -= ellipsisWidth;
                BOOL textIsUtf8 = IsValidUtf8Text(Text, textLen);
                while (width > maxWidth && index >= SubStrIndex)
                {
                    if (textIsUtf8)
                    {
                        // retreat by one whole UTF-8 character
                        int charStart = Utf8CharStart(Text, index);
                        if (charStart < SubStrIndex)
                            charStart = SubStrIndex;
                        int prevCumul = charStart > 0 ? alpDx[charStart - 1] : 0;
                        width -= (alpDx[index] - prevCumul);
                        index = Utf8PrevCharStart(Text, charStart);
                    }
                    else
                    {
                        int prevCumul = index > 0 ? alpDx[index - 1] : 0;
                        width -= (alpDx[index] - prevCumul);
                        index--;
                    }
                }
                // the first part with the shortened substring
                int keep = index + 1;
                memcpy(TruncatedText, Text, keep);
                // ellipsis
                memcpy(TruncatedText + keep, "...", 3);
                // the rest
                strcpy(TruncatedText + keep + 3, Text + SubStrIndex + SubStrLen);
            }
            else
                memcpy(TruncatedText, Text, textLen + 1); // just copy -— we still fit
        }
    }

    SelectObject(hDC, hOldFont);
    HANDLES(ReleaseDC(hWindow, hDC));
    return ret;
}

//****************************************************************************
//
// StrToUInt64
//
// Converts a number (it may begin with a '+' character) to unsigned __int64.
// The len variable specifies the maximum count of processed characters.
// If 'isNum' is not NULL, it returns TRUE when the entire string
// 'str' represents a number.
//

unsigned __int64
StrToUInt64(const char* str, int len, BOOL* isNum)
{
    const char* end = str + len;
    const char* s = str;
    while (s < end && *s <= ' ')
        s++;
    if (s < end && *s == '+')
        s++;

    unsigned __int64 total = 0;
    const char* begNum = s;
    while (s < end && *s >= '0' && *s <= '9')
    {
        unsigned __int64 new_total = total * 10 + (*s - '0');
        if (new_total >= total)
        {
            total = total * 10 + (*s - '0');
            s++;
        }
        else
        {
            total = 0xffffffffffffffff;
            while (s < end && *s >= '0' && *s <= '9')
                s++;
            break;
        }
    }
    BOOL hasDigits = begNum != s;
    while (s < end && *s <= ' ')
        s++;
    if (isNum != NULL)
        *isNum = (hasDigits && s == end);
    return total;
}

//****************************************************************************
//
// ExpandPluralString
//

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// WARNING: whenever ExpandPluralString is modified it is also necessary to update
//          ValidatePluralStrings in the TRANSLATOR project
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

int ExpandPluralString(char* lpOut, int nOutMax, const char* lpFmt, int nParCount,
                       const CQuadWord* lpParArray)
{
    const char* input = lpFmt;
    char* output = lpOut;
    char* outputNullTerm = lpOut + nOutMax - 1;
    int actParIndex = 0;

    struct CAuxParUsed
    {
        BOOL StackArr[20];
        BOOL* Arr;
        CAuxParUsed(int nParCount)
        {
            Arr = nParCount <= sizeof(StackArr) / sizeof(StackArr[0]) ? StackArr : new BOOL[nParCount];
            memset(Arr, 0, nParCount * sizeof(BOOL));
        }
        ~CAuxParUsed()
        {
            if (Arr != StackArr)
                delete[] (Arr);
        }
    } parUsedArr(max(0, nParCount));

    if (nOutMax > 0 && lpOut != NULL)
        *lpOut = 0;

    // check and skip the {!} signature
    if (input != NULL && *input++ == '{' && *input++ == '!' && *input++ == '}' && nOutMax > 0)
    {
        while (*input != 0)
        {
            if (*input == '\\' &&
                (*(input + 1) == '|' || *(input + 1) == '\\' || *(input + 1) == ':' ||
                 *(input + 1) == '{' || *(input + 1) == '}')) // escape sequence
            {
                input++;
                if (output >= outputNullTerm) // the buffer must also fit the terminating zero
                {
                    lpOut[nOutMax - 1] = 0;
                    TRACE_E("ExpandPluralString: truncated output string.");
                    return nOutMax - 1;
                }
                *output++ = *input++;
            }
            else
            {
                if (*input == '{') // perform expansion of the curly brace
                {
                    input++;

                    // fetch the corresponding parameter value from the array
                    unsigned __int64 arg;
                    const char* parInd = input;
                    int parIndVal = 0;
                    while (*parInd >= '0' && *parInd <= '9')
                        parIndVal = 10 * parIndVal + *parInd++ - '0';
                    if (*parInd == ':' && parInd > input) // an index was assigned, use it
                    {
                        if (parIndVal >= 1 && parIndVal <= nParCount)
                        {
                            input = parInd + 1;
                            parUsedArr.Arr[parIndVal - 1] = TRUE;
                            arg = lpParArray[parIndVal - 1].Value;
                        }
                        else
                        {
                            TRACE_E("ExpandPluralString: specified index of parameter is out of range: " << parIndVal);
                            *output = 0;
                            return (int)(output - lpOut);
                        }
                    }
                    else // use the next parameter in order
                    {
                        if (actParIndex < nParCount)
                        {
                            parUsedArr.Arr[actParIndex] = TRUE;
                            arg = lpParArray[actParIndex++].Value;
                        }
                        else
                        {
                            TRACE_E("ExpandPluralString: few parameters in array.");
                            *output = 0;
                            return (int)(output - lpOut);
                        }
                    }

                    while (*input != '}' && *input != 0)
                    {
                        const char* subStr = input;
                        int subStrLen = 0;

                        while (*input != '}' && *input != 0 && *input != '|')
                        {
                            if (*input == '\\' &&
                                (*(input + 1) == '|' || *(input + 1) == '\\' || *(input + 1) == ':' ||
                                 *(input + 1) == '{' || *(input + 1) == '}')) // escape sequence
                                input++;
                            subStrLen++;
                            input++;
                        }

                        if (*input == '|')
                            input++;

                        const char* numStr = input;
                        int numStrLen = 0;

                        while (*input != '}' && *input != 0 && *input != '|')
                        {
                            if (*input == '\\' &&
                                (*(input + 1) == '|' || *(input + 1) == '\\' || *(input + 1) == ':' ||
                                 *(input + 1) == '{' || *(input + 1) == '}')) // escape sequence
                                input++;
                            numStrLen++;
                            input++;
                        }

                        if (*input == '|')
                            input++;

                        if (numStrLen == 0 && *input != '}')
                        {
                            TRACE_E("ExpandPluralString: syntax error: " << lpFmt);
                        }

                        unsigned __int64 num = 0;
                        if (numStrLen > 0)
                        {
                            BOOL isNum;
                            num = StrToUInt64(numStr, numStrLen, &isNum);
                            if (!isNum)
                                TRACE_E("ExpandPluralString: contains limit that is not a number: " << lpFmt);
                        }

                        // if this is the last string without interval limitation,
                        // or the value of arg is less than or equal to the interval boundary
                        if (numStrLen == 0 || arg <= num)
                        {
                            // insert the relevant substring into the output string
                            int i;
                            for (i = 0; i < subStrLen; i++)
                            {
                                if (*subStr == '\\' &&
                                    (*(subStr + 1) == '|' || *(subStr + 1) == '\\' || *(subStr + 1) == ':' ||
                                     *(subStr + 1) == '{' || *(subStr + 1) == '}')) // escape sequence
                                    subStr++;
                                if (output >= outputNullTerm) // the buffer must also fit the terminating zero
                                {
                                    lpOut[nOutMax - 1] = 0;
                                    TRACE_E("ExpandPluralString: truncated output string.");
                                    return nOutMax - 1;
                                }
                                *output++ = *subStr++;
                            }

                            // and stop searching
                            while (*input != '}' && *input != 0)
                            {
                                if (*input == '\\' &&
                                    (*(input + 1) == '|' || *(input + 1) == '\\' || *(input + 1) == ':' ||
                                     *(input + 1) == '{' || *(input + 1) == '}')) // escape sequence
                                    input++;
                                input++;
                            }
                        }
                    }
                    if (*input == '}')
                        input++;
                }
                else
                {
                    if (output >= outputNullTerm) // the buffer must also fit the terminating zero
                    {
                        lpOut[nOutMax - 1] = 0;
                        TRACE_E("ExpandPluralString: truncated output string.");
                        return nOutMax - 1;
                    }
                    *output++ = *input++;
                }
            }
        }
        *output = 0; // insert the terminator
    }
    else
        TRACE_E("ExpandPluralString: format string does not contain {!} signature or output buffer is too short.");

    int i;
    for (i = 0; i < nParCount; i++)
        if (!parUsedArr.Arr[i])
        {
            TRACE_E("ExpandPluralString: warning: some parameters from array were not used, zero-based index "
                    "of first unused parameter is "
                    << i);
            break;
        }

    return (int)(output - lpOut);
}

//****************************************************************************
//
// ExpandPluralFilesDirs
//
// Writes a string to lpOut depending on the values of the variables 'files' and 'dirs':
// files > 0 && dirs == 0  ->  XXX (selected) files
// files == 0 && dirs > 0  ->  YYY (selected) directories
// files > 0 && dirs > 0   ->  XXX (selected) files and YYY directories
//
// where XXX and YYY correspond to the values of the files and dirs variables.
// The selectedForm variable controls inserting the word selected.
//
// forDlgCaption is TRUE/FALSE if the text is/is not meant for a dialog caption
// (initial capital letters are required in English).
//
// Returns the number of copied characters without the terminator.
//

int ExpandPluralFilesDirs(char* lpOut, int nOutMax, int files, int dirs, int mode, BOOL forDlgCaption)
{
    static int form[2][3][3] =
        {
            {{IDS_PLURAL_X_FILES, IDS_PLURAL_X_DIRS, IDS_PLURAL_X_FILES_Y_DIRS},
             {IDS_PLURAL_X_SEL_FILES, IDS_PLURAL_X_SEL_DIRS, IDS_PLURAL_X_SEL_FILES_Y_SEL_DIRS},
             {IDS_PLURAL_X_HID_FILES, IDS_PLURAL_X_HID_DIRS, IDS_PLURAL_X_HID_FILES_Y_HID_DIRS}},

            {{IDS_DLG_PLURAL_X_FILES, IDS_DLG_PLURAL_X_DIRS, IDS_DLG_PLURAL_X_FILES_Y_DIRS},
             {IDS_DLG_PLURAL_X_SEL_FILES, IDS_DLG_PLURAL_X_SEL_DIRS, IDS_DLG_PLURAL_X_SEL_FILES_Y_SEL_DIRS},
             {IDS_DLG_PLURAL_X_HID_FILES, IDS_DLG_PLURAL_X_HID_DIRS, IDS_DLG_PLURAL_X_HID_FILES_Y_HID_DIRS}},
        };
    int indDlgCaption = forDlgCaption ? 1 : 0;
    char expanded[200];
    if (nOutMax > 200)
        nOutMax = 200;
    nOutMax -= 20; // make room for the numbers of files and dirs

    int ret;

    if (files > 0 && dirs == 0)
    {
        CQuadWord qwFiles(files, 0);
        ExpandPluralString(expanded, nOutMax, LoadStr(form[indDlgCaption][mode][0]), 1, &qwFiles);
        ret = sprintf(lpOut, expanded, files);
    }
    else
    {
        if (files == 0 && dirs > 0)
        {
            CQuadWord qwDirs(dirs, 0);
            ExpandPluralString(expanded, nOutMax, LoadStr(form[indDlgCaption][mode][1]), 1, &qwDirs);
            ret = sprintf(lpOut, expanded, dirs);
        }
        else
        {
            CQuadWord qwPars[2] = {CQuadWord(files, 0), CQuadWord(dirs, 0)};
            ExpandPluralString(expanded, nOutMax, LoadStr(form[indDlgCaption][mode][2]), 2, qwPars);
            ret = sprintf(lpOut, expanded, files, dirs);
        }
    }
    return ret;
}

int ExpandPluralBytesFilesDirs(char* lpOut, int nOutMax, const CQuadWord& selectedBytes, int files, int dirs, BOOL useSubTexts)
{
    char expanded[200];
    char number[50];
    if (nOutMax > 200)
        nOutMax = 200;
    nOutMax -= 30; // make room for the numbers of files and dirs

    int ret;

    if (files > 0 && dirs == 0)
    {
        CQuadWord qwPars[2] = {selectedBytes, CQuadWord(files, 0)};
        ExpandPluralString(expanded, nOutMax,
                           LoadStr(useSubTexts ? IDS_PLURAL_X_BYTES_Y_SEL_FILES2 : IDS_PLURAL_X_BYTES_Y_SEL_FILES),
                           2, qwPars);
        ret = sprintf(lpOut, expanded, NumberToStr(number, selectedBytes), files);
    }
    else
    {
        if (files == 0 && dirs > 0)
        {
            CQuadWord qwPars[2] = {selectedBytes, CQuadWord(dirs, 0)};
            ExpandPluralString(expanded, nOutMax,
                               LoadStr(useSubTexts ? IDS_PLURAL_X_BYTES_Y_SEL_DIRS2 : IDS_PLURAL_X_BYTES_Y_SEL_DIRS),
                               2, qwPars);
            ret = sprintf(lpOut, expanded, NumberToStr(number, selectedBytes), dirs);
        }
        else
        {
            CQuadWord qwPars[3] = {selectedBytes, CQuadWord(files, 0), CQuadWord(dirs, 0)};
            ExpandPluralString(expanded, nOutMax,
                               LoadStr(useSubTexts ? IDS_PLURAL_X_BYTES_Y_SEL_FILES_Z_SEL_DIRS2 : IDS_PLURAL_X_BYTES_Y_SEL_FILES_Z_SEL_DIRS),
                               3, qwPars);
            ret = sprintf(lpOut, expanded, NumberToStr(number, selectedBytes), files, dirs);
        }
    }
    return ret;
}

BOOL LookForSubTexts(char* text, DWORD* varPlacements, int* varPlacementsCount)
{
    int maxVars = *varPlacementsCount;
    *varPlacementsCount = 0;

    const char* src = text; // we read characters from this pointer
    char* dst = text;       // we write the result to this pointer
    char* var = NULL;       // pointer to the first character of the variable

    while (*src != 0)
    {
        switch (*src)
        {
        case '\\':
        {
            if (*(src + 1) == '<' || *(src + 1) == '>' || *(src + 1) == '\\')
                src++; // escape sequence
            break;
        }

        case '<':
        {
            if (var != NULL)
            {
                TRACE_E("LookForSubTexts: syntax error in (already changed): " << text);
                return FALSE;
            }
            src++;
            var = dst;
            continue;
        }

        case '>':
        {
            if (var == NULL)
            {
                TRACE_E("LookForSubTexts: syntax error in (already changed): " << text);
                return FALSE;
            }
            if (*varPlacementsCount >= maxVars)
            {
                TRACE_E("LookForSubTexts: too many variables in: " << text);
                return FALSE;
            }
            *varPlacements = MAKELPARAM(var - text, dst - var);
            varPlacements++;
            (*varPlacementsCount)++;
            src++;
            var = NULL;
            continue;
        }
        }

        *dst = *src;
        src++;
        dst++;
    }
    // write the terminator
    *dst = 0;
    if (var != NULL)
    {
        TRACE_E("LookForSubTexts: syntax error in (already changed): " << text);
        return FALSE;
    }

    return TRUE;
}

//****************************************************************************
//
// CViewTemplates
//

const char* SALAMANDER_VIEWTEMPLATE_NAME = "Name";
const char* SALAMANDER_VIEWTEMPLATE_FLAGS = "Flags";
const char* SALAMANDER_VIEWTEMPLATE_COLUMNS = "Columns";
const char* SALAMANDER_VIEWTEMPLATE_COLUMNORDER = "Column Order";
const char* SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNS = "Explorer Columns";
const char* SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNVISIBLE = "Explorer Column Visible";
const char* SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNORDER = "Explorer Column Order";
const char* SALAMANDER_VIEWTEMPLATE_LEFTSMARTMODE = "Left Smart Mode";
const char* SALAMANDER_VIEWTEMPLATE_RIGHTSMARTMODE = "Right Smart Mode";
const char* SALAMANDER_VIEWTEMPLATE_AVAILABLEEXPLORERCOLUMNS = "Available Explorer Columns";
const char* SALAMANDER_VIEWTEMPLATE_EXPLORERAVAILABILITYVERSION = "Explorer Availability Version";
const char* SALAMANDER_VIEWTEMPLATE_EXTRACOUNT = "Extra View Count";
const char* SALAMANDER_VIEWTEMPLATE_NEXTID = "Next View ID";
const char* SALAMANDER_VIEWTEMPLATE_ID = "ID";
const int EXPLORER_COLUMN_CANONICAL_NAME_MAX = 256;
const DWORD EXPLORER_COLUMN_AVAILABILITY_VERSION = 1;

CViewTemplates::CViewTemplates()
    : ExtraItems(3, 3)
{
    // default values
    Set(0, VIEW_MODE_TREE, LoadStr(IDS_TREE_VIEW), 0, TRUE, TRUE);
    Set(1, VIEW_MODE_BRIEF, LoadStr(IDS_BRIEF_VIEW), 0, TRUE, TRUE);
    Set(2, VIEW_MODE_DETAILED, LoadStr(IDS_DETAILED_VIEW), VIEW_SHOW_SIZE | VIEW_SHOW_DATE | VIEW_SHOW_TIME | VIEW_SHOW_ATTRIBUTES, TRUE, TRUE);
    Set(3, VIEW_MODE_ICONS, LoadStr(IDS_ICONS_VIEW), 0, TRUE, TRUE);
    Set(4, VIEW_MODE_THUMBNAILS, LoadStr(IDS_THUMBNAILS_VIEW), 0, TRUE, TRUE);
    Set(5, VIEW_MODE_TILES, LoadStr(IDS_TILES_VIEW), 0, TRUE, TRUE);
    Set(6, VIEW_MODE_DETAILED, LoadStr(IDS_TYPES_VIEW), VIEW_SHOW_SIZE | VIEW_SHOW_TYPE | VIEW_SHOW_DATE | VIEW_SHOW_TIME | VIEW_SHOW_ATTRIBUTES, TRUE, TRUE);
    //  Set(4, VIEW_MODE_DETAILED, LoadStr(IDS_DESCRIPTIONS_VIEW), VIEW_SHOW_SIZE | VIEW_SHOW_DESCRIPTION, TRUE, TRUE);
    int i;
    for (i = 7; i < VIEW_TEMPLATES_COUNT; i++)
        Set(i, VIEW_MODE_DETAILED, "", 0, TRUE, TRUE);
    for (i = 0; i < VIEW_TEMPLATES_COUNT; i++)
    {
        Items[i].ID = i + 1;
        ZeroMemory(Items[i].Columns, sizeof(Items[i].Columns));
        ZeroMemory(Items[i].ExplorerColumns, sizeof(Items[i].ExplorerColumns));
        ZeroMemory(Items[i].ExplorerColumnAvailable, sizeof(Items[i].ExplorerColumnAvailable));
        ZeroMemory(Items[i].ExplorerColumnVisible, sizeof(Items[i].ExplorerColumnVisible));
        for (int j = 0; j < EXPLORER_COLUMNS_COUNT; j++)
            Items[i].ExplorerColumnOrder[j] = (WORD)j;
        for (int j = 0; j < STANDARD_COLUMNS_COUNT; j++)
            Items[i].ColumnOrder[j] = (BYTE)j;
    }
    ZeroMemory(ExplorerColumnAvailable, sizeof(ExplorerColumnAvailable));
    NextID = VIEW_TEMPLATES_COUNT + 1;
}

CViewTemplate* CViewTemplates::Get(int index)
{
    if (index < 0 || index >= GetCount())
        return NULL;
    return index < VIEW_TEMPLATES_COUNT ? &Items[index] : ExtraItems[index - VIEW_TEMPLATES_COUNT];
}

const CViewTemplate* CViewTemplates::Get(int index) const
{
    if (index < 0 || index >= GetCount())
        return NULL;
    return index < VIEW_TEMPLATES_COUNT
               ? &Items[index]
               : const_cast<CViewTemplates*>(this)->ExtraItems[index - VIEW_TEMPLATES_COUNT];
}

int CViewTemplates::GetIndex(const CViewTemplate* item) const
{
    for (int i = 0; i < GetCount(); i++)
        if (Get(i) == item)
            return i;
    return -1;
}

int CViewTemplates::AddDetailedView(const char* name)
{
    if (ExtraItems.Count >= CM_ACTIVEEXTRAMODE_MAX - CM_ACTIVEEXTRAMODE_MIN + 1)
        return -1;
#ifdef new
#undef new
#define RESTORE_ADD_VIEW_TEMPLATE_DEBUG_NEW_MACRO
#endif
    CViewTemplate* item = new (std::nothrow) CViewTemplate;
#ifdef RESTORE_ADD_VIEW_TEMPLATE_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_ADD_VIEW_TEMPLATE_DEBUG_NEW_MACRO
#endif
    if (item == NULL)
        return -1;
    memcpy(item, &Items[2], sizeof(*item));
    item->ID = NextID++;
    CopyStringTruncateUtf8(item->Name, VIEW_NAME_MAX, name != NULL ? name : "");
    ExtraItems.Add(item);
    if (!ExtraItems.IsGood())
    {
        ExtraItems.ResetState();
        delete item;
        return -1;
    }
    return GetCount() - 1;
}

BOOL CViewTemplates::DeleteView(int index)
{
    if (index < 7 || index >= GetCount())
        return FALSE;
    if (index < VIEW_TEMPLATES_COUNT)
    {
        Items[index].Name[0] = 0;
        return TRUE;
    }
    ExtraItems.Delete(index - VIEW_TEMPLATES_COUNT);
    return TRUE;
}

void CViewTemplates::Load(CViewTemplates& source)
{
    memcpy(Items, source.Items, sizeof(Items));
    memcpy(ExplorerColumnAvailable, source.ExplorerColumnAvailable, sizeof(ExplorerColumnAvailable));
    NextID = source.NextID;
    ExtraItems.DestroyMembers();
    for (int i = 0; i < source.ExtraItems.Count; i++)
    {
#ifdef new
#undef new
#define RESTORE_COPY_VIEW_TEMPLATE_DEBUG_NEW_MACRO
#endif
        CViewTemplate* item = new (std::nothrow) CViewTemplate;
#ifdef RESTORE_COPY_VIEW_TEMPLATE_DEBUG_NEW_MACRO
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef RESTORE_COPY_VIEW_TEMPLATE_DEBUG_NEW_MACRO
#endif
        if (item == NULL)
            break;
        memcpy(item, source.ExtraItems[i], sizeof(*item));
        ExtraItems.Add(item);
        if (!ExtraItems.IsGood())
        {
            ExtraItems.ResetState();
            delete item;
            break;
        }
    }
}

BOOL CViewTemplates::IsExplorerColumnAvailable(int index) const
{
    return index >= 0 && index < EXPLORER_COLUMNS_COUNT && ExplorerColumnAvailable[index] != 0;
}

void CViewTemplates::SetExplorerColumnAvailable(int index, BOOL available)
{
    if (index >= 0 && index < EXPLORER_COLUMNS_COUNT)
        ExplorerColumnAvailable[index] = available ? TRUE : FALSE;
}

void CViewTemplates::RebuildExplorerColumnAvailable()
{
    ZeroMemory(ExplorerColumnAvailable, sizeof(ExplorerColumnAvailable));
    for (int i = 0; i < GetCount(); i++)
    {
        CViewTemplate* view = Get(i);
        for (int j = 0; j < EXPLORER_COLUMNS_COUNT; j++)
            if (view->ExplorerColumnAvailable[j] || view->ExplorerColumnVisible[j])
                ExplorerColumnAvailable[j] = TRUE;
    }
}

void CViewTemplates::Set(DWORD index, const char* name, DWORD flags, BOOL leftSmartMode, BOOL rightSmartMode)
{
    if (lstrlen(name) >= VIEW_NAME_MAX)
        TRACE_E("String is too long");
    CopyStringTruncateUtf8(Items[index].Name, VIEW_NAME_MAX, name);
    Items[index].Flags = flags;
    Items[index].LeftSmartMode = leftSmartMode;
    Items[index].RightSmartMode = rightSmartMode;
}

void CViewTemplates::Set(DWORD index, DWORD viewMode, const char* name, DWORD flags, BOOL leftSmartMode, BOOL rightSmartMode)
{
    Items[index].Mode = viewMode;
    Set(index, name, flags, leftSmartMode, rightSmartMode);
}

BOOL CViewTemplates::SwapItems(int index1, int index2)
{
    if (index1 < 2 || index2 < 2)
    {
        TRACE_E("It is not possible to move first nor second item.");
        return FALSE;
    }

    if (index1 >= VIEW_TEMPLATES_COUNT || index2 >= VIEW_TEMPLATES_COUNT)
    {
        TRACE_E("Index is out of range");
        return FALSE;
    }

    CViewTemplate tmp = Items[index1];
    Items[index1] = Items[index2];
    Items[index2] = tmp;
    return TRUE;
}

BOOL CViewTemplates::CleanName(char* name)
{
    char* start = name;
    char* end = name + strlen(name) - 1;
    while (*start != 0 && *start == ' ')
        start++;
    while (end >= name && *end == ' ')
        end--;
    end++;
    *end = 0;
    if (start > name && start < end)
        memmove(name, start, end - start + 1);
    return strlen(name) > 0;
}

int CViewTemplates::SaveColumns(CColumnConfig* columns, char* buffer, int count)
{
    char* s = buffer;
    int i;
    for (i = 0; i < count; i++)
    {
        CColumnConfig* column = &columns[i];
        if (i > 0)
        {
            *s = ',';
            s++;
        }
        DWORD data = column->LeftWidth | column->LeftFixedWidth << 16;
        s += sprintf(s, "%lx", data);
    }
    *s++ = ',';
    for (i = 0; i < count; i++)
    {
        CColumnConfig* column = &columns[i];
        if (i > 0)
        {
            *s = ',';
            s++;
        }
        DWORD data = column->RightWidth | column->RightFixedWidth << 16;
        s += sprintf(s, "%lx", data);
    }
    *s = 0;
    return (int)(s - buffer);
}

void CViewTemplates::LoadColumns(CColumnConfig* columns, char* buffer, int count)
{
    CColumnConfig* firstColumn = columns;
    char* p = strtok(buffer, ",");
    while (p != NULL)
    {
        DWORD data;
        int i = sscanf(p, "%lx", &data);
        columns->LeftWidth = data & 0x0000ffff;
        columns->LeftFixedWidth = (data & 0x00010000) >> 16;
        if (columns->LeftWidth > 2000)
            columns->LeftWidth = 2000;
        columns->RightWidth = columns->LeftWidth;
        columns->RightFixedWidth = columns->LeftFixedWidth;
        p = strtok(NULL, ",");
        columns++;
        if (columns - firstColumn >= count)
            break;
    }
    columns = firstColumn;
    while (p != NULL)
    {
        DWORD data;
        int i = sscanf(p, "%lx", &data);
        columns->RightWidth = data & 0x0000ffff;
        columns->RightFixedWidth = (data & 0x00010000) >> 16;
        if (columns->RightWidth > 2000)
            columns->RightWidth = 2000;
        p = strtok(NULL, ",");
        columns++;
        if (columns - firstColumn >= count)
            break;
    }
}

int CViewTemplates::SaveColumnOrder(BYTE* order, char* buffer, int count)
{
    char* s = buffer;
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
            *s++ = ',';
        s += sprintf(s, "%u", (unsigned)order[i]);
    }
    *s = 0;
    return (int)(s - buffer);
}


int CViewTemplates::SaveColumnOrder(WORD* order, char* buffer, int count)
{
    char* s = buffer;
    for (int i = 0; i < count; i++)
    {
        if (i > 0)
            *s++ = ',';
        s += sprintf(s, "%u", (unsigned)order[i]);
    }
    *s = 0;
    return (int)(s - buffer);
}

void CViewTemplates::LoadColumnOrder(BYTE* order, char* buffer, int count)
{
    BOOL used[EXPLORER_COLUMNS_COUNT];
    ZeroMemory(used, sizeof(used));
    int pos = 0;
    char* p = strtok(buffer, ",");
    while (p != NULL && pos < count)
    {
        unsigned int value;
        if (sscanf(p, "%u", &value) == 1 && value < (unsigned int)count && !used[value])
        {
            order[pos++] = (BYTE)value;
            used[value] = TRUE;
        }
        p = strtok(NULL, ",");
    }
    for (int value = 0; pos < count && value < count; value++)
    {
        if (!used[value])
            order[pos++] = (BYTE)value;
    }
}


void CViewTemplates::LoadColumnOrder(WORD* order, char* buffer, int count)
{
    BOOL* used = (BOOL*)calloc(count, sizeof(BOOL));
    if (used == NULL)
        return;
    int pos = 0;
    char* p = strtok(buffer, ",");
    while (p != NULL && pos < count)
    {
        unsigned int value;
        if (sscanf(p, "%u", &value) == 1 && value < (unsigned int)count && !used[value])
        {
            order[pos++] = (WORD)value;
            used[value] = TRUE;
        }
        p = strtok(NULL, ",");
    }
    for (int value = 0; pos < count && value < count; value++)
    {
        if (!used[value])
            order[pos++] = (WORD)value;
    }
    free(used);
}

int CViewTemplates::SaveExplorerColumnVisible(BYTE* visible, char* buffer)
{
    char* s = buffer;
    for (int i = 0; i < EXPLORER_COLUMNS_COUNT; i++)
    {
        if (i > 0)
            *s++ = ',';
        s += sprintf(s, "%u", visible[i] ? 1 : 0);
    }
    *s = 0;
    return (int)(s - buffer);
}

void CViewTemplates::LoadExplorerColumnVisible(BYTE* visible, char* buffer)
{
    ZeroMemory(visible, EXPLORER_COLUMNS_COUNT * sizeof(BYTE));
    int pos = 0;
    char* p = strtok(buffer, ",");
    while (p != NULL && pos < EXPLORER_COLUMNS_COUNT)
    {
        unsigned int value;
        if (sscanf(p, "%u", &value) == 1 && value != 0)
            visible[pos] = TRUE;
        pos++;
        p = strtok(NULL, ",");
    }
}

static void SaveExplorerColumnAvailable(HKEY hKey, const BYTE* available,
                                        char* buffer, int bufferSize)
{
    char* out = buffer;
    int remaining = bufferSize;
    int explorerCount = GetExplorerColumnCount();
    for (int i = 0; i < explorerCount; i++)
    {
        if (!available[i])
            continue;
        const char* canonicalName = GetExplorerColumnCanonicalName(i);
        int len = lstrlen(canonicalName);
        if (len == 0 || len + 2 > remaining)
            continue;
        if (out != buffer)
        {
            *out++ = '\n';
            remaining--;
        }
        memcpy(out, canonicalName, len);
        out += len;
        remaining -= len;
    }
    *out = 0;
    SetValue(hKey, SALAMANDER_VIEWTEMPLATE_AVAILABLEEXPLORERCOLUMNS, REG_SZ,
             buffer, (int)(out - buffer) + 1);
}

static BOOL LoadExplorerColumnAvailable(HKEY hKey, BYTE* available,
                                        char* buffer, int bufferSize)
{
    if (!GetValue(hKey, SALAMANDER_VIEWTEMPLATE_AVAILABLEEXPLORERCOLUMNS, REG_SZ,
                  buffer, bufferSize))
        return FALSE;
    ZeroMemory(available, EXPLORER_COLUMNS_COUNT * sizeof(BYTE));
    char* context = NULL;
    char* canonicalName = strtok_s(buffer, "\n", &context);
    while (canonicalName != NULL)
    {
        int explorerCount = GetExplorerColumnCount();
        for (int explorerIndex = 0; explorerIndex < explorerCount; explorerIndex++)
        {
            if (_stricmp(canonicalName, GetExplorerColumnCanonicalName(explorerIndex)) == 0)
            {
                available[explorerIndex] = TRUE;
                break;
            }
        }
        canonicalName = strtok_s(NULL, "\n", &context);
    }
    return TRUE;
}

BOOL CViewTemplates::Save(HKEY hKey)
{
    char buff[12 * EXPLORER_COLUMNS_COUNT + 1];
    char keyName[32];
    int i;
    DWORD extraCount = ExtraItems.Count;
    RebuildExplorerColumnAvailable();
    SetValue(hKey, SALAMANDER_VIEWTEMPLATE_EXTRACOUNT, REG_DWORD, &extraCount, sizeof(extraCount));
    SetValue(hKey, SALAMANDER_VIEWTEMPLATE_NEXTID, REG_DWORD, &NextID, sizeof(NextID));
    for (i = 0; i < GetCount(); i++)
    {
        if (i < VIEW_TEMPLATES_COUNT)
            itoa(i < VIEW_TEMPLATES_COUNT - 1 ? i + 1 : 0, keyName, 10);
        else
            _snprintf_s(keyName, _TRUNCATE, "Extra %d", i - VIEW_TEMPLATES_COUNT + 1);
        CViewTemplate* view = Get(i);
        HKEY actKey;
        if (CreateKey(hKey, keyName, actKey))
        {
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_ID, REG_DWORD, &view->ID, sizeof(view->ID));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_NAME, REG_SZ, view->Name, -1);
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_FLAGS, REG_DWORD, &view->Flags, sizeof(DWORD));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_COLUMNS, REG_SZ, buff, SaveColumns(view->Columns, buff));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_COLUMNORDER, REG_SZ, buff, SaveColumnOrder(view->ColumnOrder, buff));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNS, REG_SZ, buff,
                     SaveColumns(view->ExplorerColumns, buff, EXPLORER_COLUMNS_COUNT));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNVISIBLE, REG_SZ, buff, SaveExplorerColumnVisible(view->ExplorerColumnVisible, buff));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNORDER, REG_SZ, buff, SaveColumnOrder(view->ExplorerColumnOrder, buff, EXPLORER_COLUMNS_COUNT));
            const int availableBufferSize = EXPLORER_COLUMNS_COUNT * EXPLORER_COLUMN_CANONICAL_NAME_MAX + 1;
            char* availableBuffer = (char*)malloc(availableBufferSize);
            if (availableBuffer != NULL)
            {
                SaveExplorerColumnAvailable(actKey, view->ExplorerColumnAvailable,
                                            availableBuffer, availableBufferSize);
                free(availableBuffer);
            }
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_LEFTSMARTMODE, REG_DWORD, &view->LeftSmartMode, sizeof(DWORD));
            SetValue(actKey, SALAMANDER_VIEWTEMPLATE_RIGHTSMARTMODE, REG_DWORD, &view->RightSmartMode, sizeof(DWORD));
            CloseKey(actKey);
        }
    }
    // Write the marker only after all per-view values. An interrupted save is
    // therefore migrated again instead of trusting a partial set of subsets.
    DWORD explorerAvailabilityVersion = EXPLORER_COLUMN_AVAILABILITY_VERSION;
    SetValue(hKey, SALAMANDER_VIEWTEMPLATE_EXPLORERAVAILABILITYVERSION, REG_DWORD,
             &explorerAvailabilityVersion, sizeof(explorerAvailabilityVersion));
    const int availableBufferSize = EXPLORER_COLUMNS_COUNT * EXPLORER_COLUMN_CANONICAL_NAME_MAX + 1;
    char* availableBuffer = (char*)malloc(availableBufferSize);
    if (availableBuffer != NULL)
    {
        SaveExplorerColumnAvailable(hKey, ExplorerColumnAvailable,
                                    availableBuffer, availableBufferSize);
        free(availableBuffer);
    }
    return TRUE;
}

BOOL CViewTemplates::Load(HKEY hKey)
{
    ExtraItems.DestroyMembers();
    NextID = VIEW_TEMPLATES_COUNT + 1;
    char buff[12 * EXPLORER_COLUMNS_COUNT + 1];
    char keyName[32];
    int i;
    DWORD explorerAvailabilityVersion = 0;
    GetValue(hKey, SALAMANDER_VIEWTEMPLATE_EXPLORERAVAILABILITYVERSION, REG_DWORD,
             &explorerAvailabilityVersion, sizeof(explorerAvailabilityVersion));
    BOOL availableLoaded = FALSE;
    const int availableBufferSize = EXPLORER_COLUMNS_COUNT * EXPLORER_COLUMN_CANONICAL_NAME_MAX + 1;
    char* availableBuffer = (char*)malloc(availableBufferSize);
    if (availableBuffer != NULL)
        availableLoaded = LoadExplorerColumnAvailable(hKey, ExplorerColumnAvailable,
                                                      availableBuffer, availableBufferSize);
    DWORD extraCount = 0;
    if (GetValue(hKey, SALAMANDER_VIEWTEMPLATE_EXTRACOUNT, REG_DWORD, &extraCount, sizeof(extraCount)))
    {
        if (extraCount > 1000)
            extraCount = 1000;
        for (DWORD extra = 0; extra < extraCount; extra++)
            if (AddDetailedView("") < 0)
                break;
    }
    DWORD loadedNextID = NextID;
    GetValue(hKey, SALAMANDER_VIEWTEMPLATE_NEXTID, REG_DWORD, &loadedNextID, sizeof(loadedNextID));
    if (loadedNextID > NextID)
        NextID = loadedNextID;

    for (i = 0; i < GetCount(); i++)
        ZeroMemory(Get(i)->ExplorerColumnAvailable,
                   sizeof(Get(i)->ExplorerColumnAvailable));

    // Configurations without the version marker include both the original
    // global availability format and builds which incorrectly cloned that
    // global subset into every view. There is no reliable provenance for
    // unchecked properties in those files. Keep the legacy global subset on
    // the primary Detailed view and reconstruct every other view from its
    // independently persisted visibility below.
    if (explorerAvailabilityVersion < EXPLORER_COLUMN_AVAILABILITY_VERSION &&
        availableLoaded && GetCount() > 2)
        memcpy(Get(2)->ExplorerColumnAvailable, ExplorerColumnAvailable,
               sizeof(Get(2)->ExplorerColumnAvailable));

    for (i = 0; i < GetCount(); i++)
    {
        if (i < VIEW_TEMPLATES_COUNT)
            itoa(i < VIEW_TEMPLATES_COUNT - 1 ? i + 1 : 0, keyName, 10);
        else
            _snprintf_s(keyName, _TRUNCATE, "Extra %d", i - VIEW_TEMPLATES_COUNT + 1);
        if (i == 6 && Configuration.ConfigVersion < 23)
            continue; // for the IDS_TYPES_VIEW view we want default columns
        CViewTemplate* view = Get(i);
        HKEY actKey;
        if (OpenKey(hKey, keyName, actKey))
        {
            if (explorerAvailabilityVersion >= EXPLORER_COLUMN_AVAILABILITY_VERSION &&
                availableBuffer != NULL)
                LoadExplorerColumnAvailable(actKey, view->ExplorerColumnAvailable,
                                            availableBuffer, availableBufferSize);
            char name[SAL_MAX_PATH];
            DWORD flags;
            name[0] = 0;
            flags = 0;
            buff[0] = 0;
            DWORD leftSM = TRUE;
            DWORD rightSM = TRUE;
            DWORD id = view->ID;
            GetValue(actKey, SALAMANDER_VIEWTEMPLATE_ID, REG_DWORD, &id, sizeof(id));
            GetValue(actKey, SALAMANDER_VIEWTEMPLATE_LEFTSMARTMODE, REG_DWORD, &leftSM, sizeof(DWORD));
            GetValue(actKey, SALAMANDER_VIEWTEMPLATE_RIGHTSMARTMODE, REG_DWORD, &rightSM, sizeof(DWORD));
            if (GetValue(actKey, SALAMANDER_VIEWTEMPLATE_NAME, REG_SZ, name, SAL_MAX_PATH) &&
                GetValue(actKey, SALAMANDER_VIEWTEMPLATE_FLAGS, REG_DWORD, &flags, sizeof(DWORD)) &&
                GetValue(actKey, SALAMANDER_VIEWTEMPLATE_COLUMNS, REG_SZ, buff, sizeof(buff)))
            {
                LoadColumns(view->Columns, buff);
                if (GetValue(actKey, SALAMANDER_VIEWTEMPLATE_COLUMNORDER, REG_SZ, buff, sizeof(buff)))
                    LoadColumnOrder(view->ColumnOrder, buff);
                if (GetValue(actKey, SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNS, REG_SZ, buff, sizeof(buff)))
                    LoadColumns(view->ExplorerColumns, buff, EXPLORER_COLUMNS_COUNT);
                if (GetValue(actKey, SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNVISIBLE, REG_SZ, buff, sizeof(buff)))
                    LoadExplorerColumnVisible(view->ExplorerColumnVisible, buff);
                if (GetValue(actKey, SALAMANDER_VIEWTEMPLATE_EXPLORERCOLUMNORDER, REG_SZ, buff, sizeof(buff)))
                    LoadColumnOrder(view->ExplorerColumnOrder, buff, EXPLORER_COLUMNS_COUNT);
                CleanName(name);

                // overwrite file names the user could not change anyway
                int resID = -1;
                switch (i)
                {
                case 0:
                    resID = IDS_TREE_VIEW;
                    break;
                case 1:
                    resID = IDS_BRIEF_VIEW;
                    break;
                case 2:
                    resID = IDS_DETAILED_VIEW;
                    break;
                case 3:
                    resID = IDS_ICONS_VIEW;
                    break;
                case 4:
                    resID = IDS_THUMBNAILS_VIEW;
                    break;
                case 5:
                    resID = IDS_TILES_VIEW;
                    break;
                case 6:
                    resID = IDS_TYPES_VIEW;
                    break;
                }
                if (resID != -1)
                    strcpy(name, LoadStr(resID));

                if (i < VIEW_TEMPLATES_COUNT)
                    Set(i, name, flags, leftSM, rightSM);
                else
                {
                    view->Mode = VIEW_MODE_DETAILED;
                    CopyStringTruncateUtf8(view->Name, VIEW_NAME_MAX, name);
                    view->Flags = flags;
                    view->LeftSmartMode = leftSM;
                    view->RightSmartMode = rightSM;
                }
                view->ID = id;
                if (id >= NextID)
                    NextID = id + 1;
            }
            CloseKey(actKey);
        }
    }
    if (availableBuffer != NULL)
        free(availableBuffer);
    if (!availableLoaded)
        ZeroMemory(ExplorerColumnAvailable, sizeof(ExplorerColumnAvailable));
    // A visible property must always remain available for its own view. This
    // also reconstructs independent subsets while migrating old configurations.
    for (i = 0; i < GetCount(); i++)
        for (int j = 0; j < EXPLORER_COLUMNS_COUNT; j++)
            if (Get(i)->ExplorerColumnVisible[j])
                Get(i)->ExplorerColumnAvailable[j] = TRUE;
    RebuildExplorerColumnAvailable();
    return TRUE;
}

// ****************************************************************************

DWORD AddUnicodeToClipboard(const char* str, int textLen)
{
    DWORD err = ERROR_SUCCESS;
    int unicodeLen = 0;
    if (textLen > 0)
    {
        unicodeLen = MultiByteToWideChar(CP_ACP, 0, str, textLen, NULL, 0);
        if (unicodeLen == 0)
            err = GetLastError();
    }
    if (err == ERROR_SUCCESS)
    {
        HGLOBAL unicode = NOHANDLES(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(WCHAR) * (unicodeLen + 1)));
        if (unicode != NULL)
        {
            WCHAR* unicodeStr = (WCHAR*)HANDLES(GlobalLock(unicode));
            if (unicodeStr != NULL)
            {
                if (textLen > 0 && MultiByteToWideChar(CP_ACP, 0, str, textLen, unicodeStr, unicodeLen + 1) == 0)
                    err = GetLastError();
                unicodeStr[unicodeLen] = 0; // terminating zero
                HANDLES(GlobalUnlock(unicode));
                if (err == ERROR_SUCCESS && SetClipboardData(CF_UNICODETEXT, unicode) == NULL)
                    err = GetLastError();
            }
            else
                err = GetLastError();
            if (err != ERROR_SUCCESS)
                NOHANDLES(GlobalFree(unicode));
        }
        else
            err = GetLastError();
    }
    if (err != ERROR_SUCCESS)
        TRACE_E("SetClipboardData failed for Unicode version of text. Error: " << GetErrorText(err));
    return err;
}

// ****************************************************************************

DWORD AddMultibyteToClipboard(const wchar_t* str, int textLen)
{
    DWORD err = ERROR_SUCCESS;
    int mbLen = textLen == 0 ? 0 : WideCharToMultiByte(CP_ACP, 0, str, textLen, NULL, 0, NULL, NULL);
    if (mbLen > 0 || textLen == 0)
    {
        HGLOBAL multibyte = NOHANDLES(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, mbLen + 1));
        if (multibyte != NULL)
        {
            char* multibyteStr = (char*)HANDLES(GlobalLock(multibyte));
            if (multibyteStr != NULL)
            {
                if (textLen > 0 && WideCharToMultiByte(CP_ACP, 0, str, textLen, multibyteStr, mbLen + 1, NULL, NULL) == 0)
                    err = GetLastError();
                multibyteStr[mbLen] = 0; // terminating zero
                HANDLES(GlobalUnlock(multibyte));
                if (err == ERROR_SUCCESS && SetClipboardData(CF_TEXT, multibyte) == NULL)
                    err = GetLastError();
            }
            else
                err = GetLastError();
            if (err != ERROR_SUCCESS)
                NOHANDLES(GlobalFree(multibyte));
        }
        else
            err = GetLastError();
    }
    else
        err = GetLastError();

    if (err != ERROR_SUCCESS)
        TRACE_E("SetClipboardData failed for multibyte version of text. Error: " << GetErrorText(err));
    return err;
}

// ****************************************************************************

BOOL CopyHTextToClipboardW(HGLOBAL hGlobalText, int textLen)
{
    if (hGlobalText == NULL)
    {
        TRACE_E("hGlobalText == NULL");
        return FALSE;
    }

    DWORD err = ERROR_SUCCESS;

    if (OpenClipboard(NULL))
    {
        if (EmptyClipboard())
        {
            wchar_t* text = (wchar_t*)HANDLES(GlobalLock(hGlobalText));
            if (text != NULL)
            {
                if (textLen == -1)
                    textLen = (int)wcslen(text);
                err = AddMultibyteToClipboard(text, textLen); // store the text in multibyte first
                HANDLES(GlobalUnlock(hGlobalText));
            }
            else
                err = GetLastError();

            if (SetClipboardData(CF_UNICODETEXT, hGlobalText) == NULL) // then store the multibyte text
                err = GetLastError();
        }
        else
            err = GetLastError();
        CloseClipboard();

        IdleRefreshStates = TRUE;  // on the next idle force a check of the status variables
        IdleCheckClipboard = TRUE; // also let it check the clipboard
    }
    else
    {
        err = GetLastError();
        TRACE_E("OpenClipboard() has failed!");
    }

    return err == ERROR_SUCCESS;
}

// ****************************************************************************

BOOL CopyTextToClipboardW(const wchar_t* text, int textLen, BOOL showEcho, HWND hEchoParent)
{
    if (text == NULL)
    {
        TRACE_E("text == NULL");
        return FALSE;
    }

    DWORD err = ERROR_SUCCESS;

    if (textLen == -1)
        textLen = (int)wcslen(text);

    HGLOBAL hglbCopy = NOHANDLES(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, sizeof(wchar_t) * (textLen + 1)));
    if (hglbCopy != NULL)
    {
        wchar_t* lptstrCopy = (wchar_t*)HANDLES(GlobalLock(hglbCopy));
        if (lptstrCopy != NULL)
        {
            memcpy(lptstrCopy, text, sizeof(wchar_t) * textLen);
            lptstrCopy[textLen] = 0;
            HANDLES(GlobalUnlock(hglbCopy));

            if (!CopyHTextToClipboardW(hglbCopy, textLen))
                return FALSE;
        }
        else
            err = GetLastError();
    }
    else
        err = GetLastError();

    if (showEcho)
    {
        if (err != ERROR_SUCCESS)
            SalMessageBox(hEchoParent, GetErrorText(err), LoadStr(IDS_COPYTOCLIPBOARD),
                          MB_OK | MB_ICONEXCLAMATION);
        else
            SalMessageBox(hEchoParent, LoadStr(IDS_TEXTCOPIED), LoadStr(IDS_INFOTITLE),
                          MB_OK | MB_ICONINFORMATION);
    }
    return err == ERROR_SUCCESS;
}

// ****************************************************************************

BOOL CopyTextToClipboard(const char* text, int textLen, BOOL showEcho, HWND hEchoParent)
{
    if (text == NULL)
    {
        TRACE_E("text == NULL");
        return FALSE;
    }

    DWORD err = ERROR_SUCCESS;

    if (textLen == -1)
        textLen = lstrlen(text);

    HGLOBAL hglbCopy = NOHANDLES(GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, textLen + 1));
    if (hglbCopy != NULL)
    {
        char* lptstrCopy = (char*)HANDLES(GlobalLock(hglbCopy));
        if (lptstrCopy != NULL)
        {
            memcpy(lptstrCopy, text, textLen);
            lptstrCopy[textLen] = 0;
            HANDLES(GlobalUnlock(hglbCopy));
            if (!CopyHTextToClipboard(hglbCopy, textLen, NULL, FALSE))
                return FALSE;
        }
        else
            err = GetLastError();
    }
    else
        err = GetLastError();

    if (showEcho)
    {
        if (err != ERROR_SUCCESS)
            SalMessageBox(hEchoParent, GetErrorText(err), LoadStr(IDS_COPYTOCLIPBOARD),
                          MB_OK | MB_ICONEXCLAMATION);
        else
            SalMessageBox(hEchoParent, LoadStr(IDS_TEXTCOPIED), LoadStr(IDS_INFOTITLE),
                          MB_OK | MB_ICONINFORMATION);
    }
    return err == ERROR_SUCCESS;
}

// ****************************************************************************

BOOL CopyHTextToClipboard(HGLOBAL hGlobalText, int textLen, BOOL showEcho, HWND hEchoParent)
{
    if (hGlobalText == NULL)
    {
        TRACE_E("hGlobalText == NULL");
        return FALSE;
    }

    DWORD err = ERROR_SUCCESS;

    if (OpenClipboard(NULL))
    {
        if (EmptyClipboard())
        {
            char* text = (char*)HANDLES(GlobalLock(hGlobalText));
            if (text != NULL)
            {
                if (textLen == -1)
                    textLen = lstrlen(text);
                err = AddUnicodeToClipboard(text, textLen); // store the text in Unicode first
                HANDLES(GlobalUnlock(hGlobalText));
            }
            else
                err = GetLastError();

            if (SetClipboardData(CF_TEXT, hGlobalText) == NULL) // then store the multibyte text
                err = GetLastError();
        }
        else
            err = GetLastError();
        CloseClipboard();

        IdleRefreshStates = TRUE;  // on the next idle force a check of the status variables
        IdleCheckClipboard = TRUE; // also let it check the clipboard
    }
    else
    {
        err = GetLastError();
        TRACE_E("OpenClipboard() has failed!");
    }

    if (showEcho)
    {
        if (err != ERROR_SUCCESS)
            SalMessageBox(hEchoParent, GetErrorText(err), LoadStr(IDS_COPYTOCLIPBOARD),
                          MB_OK | MB_ICONEXCLAMATION);
        else
            SalMessageBox(hEchoParent, LoadStr(IDS_TEXTCOPIED), LoadStr(IDS_INFOTITLE),
                          MB_OK | MB_ICONINFORMATION);
    }
    return err == ERROR_SUCCESS;
}


// Windows Explorer property columns discovered through the Property System.
static char ExplorerColumnNames[EXPLORER_COLUMNS_COUNT][COLUMN_DESCRIPTION_MAX];
static char ExplorerColumnCanonicalNames[EXPLORER_COLUMNS_COUNT][EXPLORER_COLUMN_CANONICAL_NAME_MAX];
static char ExplorerColumnDescriptions[EXPLORER_COLUMNS_COUNT][COLUMN_DESCRIPTION_MAX];
static VARTYPE ExplorerColumnTypes[EXPLORER_COLUMNS_COUNT];
static PROPERTYKEY ExplorerColumnKeys[EXPLORER_COLUMNS_COUNT];
static int ExplorerColumnsCount = -1;

static void LoadExplorerColumns()
{
    if (ExplorerColumnsCount >= 0)
        return;

    ExplorerColumnsCount = 0;
    IPropertyDescriptionList* propList = NULL;
    if (FAILED(PSEnumeratePropertyDescriptions(PDEF_ALL, IID_IPropertyDescriptionList, (void**)&propList)) || propList == NULL)
        return;

    UINT count = 0;
    if (SUCCEEDED(propList->GetCount(&count)))
    {
        for (UINT i = 0; i < count && ExplorerColumnsCount < EXPLORER_COLUMNS_COUNT; i++)
        {
            IPropertyDescription* propDesc = NULL;
            if (SUCCEEDED(propList->GetAt(i, IID_IPropertyDescription, (void**)&propDesc)) && propDesc != NULL)
            {
                LPWSTR displayName = NULL;
                PROPERTYKEY key;
                if (SUCCEEDED(propDesc->GetDisplayName(&displayName)) && displayName != NULL && displayName[0] != 0 &&
                    SUCCEEDED(propDesc->GetPropertyKey(&key)))
                {
                    char name[COLUMN_DESCRIPTION_MAX];
                    if (WideCharToMultiByte(CP_ACP, 0, displayName, -1, name, COLUMN_DESCRIPTION_MAX, NULL, NULL) > 0 && name[0] != 0)
                    {
                        BOOL duplicate = FALSE;
                        for (int j = 0; j < ExplorerColumnsCount; j++)
                        {
                            if (key.pid == ExplorerColumnKeys[j].pid &&
                                IsEqualGUID(key.fmtid, ExplorerColumnKeys[j].fmtid))
                            {
                                duplicate = TRUE;
                                break;
                            }
                        }
                        if (!duplicate)
                        {
                            lstrcpyn(ExplorerColumnNames[ExplorerColumnsCount], name, COLUMN_DESCRIPTION_MAX);
                            ExplorerColumnKeys[ExplorerColumnsCount] = key;
                            ExplorerColumnCanonicalNames[ExplorerColumnsCount][0] = 0;
                            ExplorerColumnDescriptions[ExplorerColumnsCount][0] = 0;
                            ExplorerColumnTypes[ExplorerColumnsCount] = VT_EMPTY;
                            PWSTR canonicalName = NULL;
                            if (SUCCEEDED(PSGetNameFromPropertyKey(key, &canonicalName)) && canonicalName != NULL)
                            {
                                WideCharToMultiByte(CP_UTF8, 0, canonicalName, -1,
                                                    ExplorerColumnCanonicalNames[ExplorerColumnsCount],
                                                    EXPLORER_COLUMN_CANONICAL_NAME_MAX, NULL, NULL);
                                CoTaskMemFree(canonicalName);
                            }
                            LPWSTR invitation = NULL;
                            if (SUCCEEDED(propDesc->GetEditInvitation(&invitation)) && invitation != NULL)
                            {
                                WideCharToMultiByte(CP_ACP, 0, invitation, -1,
                                                    ExplorerColumnDescriptions[ExplorerColumnsCount],
                                                    COLUMN_DESCRIPTION_MAX, NULL, NULL);
                                CoTaskMemFree(invitation);
                            }
                            propDesc->GetPropertyType(&ExplorerColumnTypes[ExplorerColumnsCount]);
                            ExplorerColumnsCount++;
                        }
                    }
                }
                if (displayName != NULL)
                    CoTaskMemFree(displayName);
                propDesc->Release();
            }
        }
        if (count > EXPLORER_COLUMNS_COUNT && ExplorerColumnsCount >= EXPLORER_COLUMNS_COUNT)
            TRACE_E("Explorer property column list was truncated at " << EXPLORER_COLUMNS_COUNT << " entries (Windows reported " << count << ").");
    }
    propList->Release();
}

int GetExplorerColumnCount()
{
    LoadExplorerColumns();
    return ExplorerColumnsCount;
}

const char* GetExplorerColumnName(int index)
{
    LoadExplorerColumns();
    return index >= 0 && index < ExplorerColumnsCount ? ExplorerColumnNames[index] : "";
}

const char* GetExplorerColumnCanonicalName(int index)
{
    LoadExplorerColumns();
    return index >= 0 && index < ExplorerColumnsCount ? ExplorerColumnCanonicalNames[index] : "";
}

const char* GetExplorerColumnDescription(int index)
{
    LoadExplorerColumns();
    return index >= 0 && index < ExplorerColumnsCount ? ExplorerColumnDescriptions[index] : "";
}

VARTYPE GetExplorerColumnType(int index)
{
    LoadExplorerColumns();
    return index >= 0 && index < ExplorerColumnsCount ? ExplorerColumnTypes[index] : VT_EMPTY;
}

const PROPERTYKEY* GetExplorerColumnPropertyKey(int index)
{
    LoadExplorerColumns();
    return index >= 0 && index < ExplorerColumnsCount ? &ExplorerColumnKeys[index] : NULL;
}

static void AddPanelTipPropertyKey(PROPERTYKEY* keys, int maxKeys, int* count, REFPROPERTYKEY key)
{
    if (*count < maxKeys)
        keys[(*count)++] = key;
}

int GetPanelTipPropertyKeys(CPanelTipCategory category, PROPERTYKEY* keys, int maxKeys)
{
    if (keys == NULL || maxKeys <= 0)
        return 0;
    int count = 0;
    switch (category)
    {
    case ptcExecutable:
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_FileDescription);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Company);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_FileVersion);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_DateCreated);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Size);
        break;
    case ptcImage:
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Image_Dimensions);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Image_BitDepth);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Size);
        break;
    case ptcAudio:
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Title);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Music_Artist);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Music_AlbumTitle);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Media_Duration);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Audio_EncodingBitrate);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Size);
        break;
    case ptcVideo:
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Video_FrameWidth);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Video_FrameHeight);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Media_Duration);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Video_FrameRate);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Size);
        break;
    case ptcDocument:
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Title);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Author);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Document_PageCount);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_DateModified);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Size);
        break;
    case ptcArchive:
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_ItemTypeText);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_DateModified);
        AddPanelTipPropertyKey(keys, maxKeys, &count, PKEY_Size);
        break;
    }
    return count;
}

BOOL IsExplorerColumnInPanelTipCategory(int index, CPanelTipCategory category)
{
    const PROPERTYKEY* columnKey = GetExplorerColumnPropertyKey(index);
    if (columnKey == NULL)
        return FALSE;
    PROPERTYKEY keys[10];
    int count = GetPanelTipPropertyKeys(category, keys, _countof(keys));
    for (int i = 0; i < count; i++)
        if (IsEqualPropertyKey(*columnKey, keys[i]))
            return TRUE;
    return FALSE;
}

CExplorerColumnCategory GetExplorerColumnCategory(int index)
{
    const char* name = GetExplorerColumnCanonicalName(index);
    if (_strnicmp(name, "System.Document.", 16) == 0)
        return eccDocument;
    if (_strnicmp(name, "System.Image.", 13) == 0 || _strnicmp(name, "System.Photo.", 13) == 0)
        return eccImage;
    if (_strnicmp(name, "System.Audio.", 13) == 0 || _strnicmp(name, "System.Music.", 13) == 0)
        return eccAudio;
    if (_strnicmp(name, "System.Video.", 13) == 0 || _strnicmp(name, "System.Media.", 13) == 0)
        return eccVideo;
    if (_strnicmp(name, "System.File", 11) == 0 || _strnicmp(name, "System.Item", 11) == 0 ||
        _strnicmp(name, "System.Size", 11) == 0 || _strnicmp(name, "System.Date", 11) == 0)
        return eccFileSystem;
    return eccOther;
}

//****************************************************************************
//
// Internal functions for retrieving column content
//

// initialized in the drawing routine before calling the callback
const CFileData* TransferFileData;
int TransferIsDir;
char TransferBuffer[TRANSFER_BUFFER_MAX];
char TransferPanelPath[SAL_MAX_PATH];
WCHAR TransferPanelPathW[SAL_MAX_PATH];
CFilesWindow* TransferPanelWindow = NULL;
int TransferLen;
DWORD TransferRowData;
CPluginDataInterfaceAbstract* TransferPluginDataIface;
DWORD TransferActCustomData;

int TransferAssocIndex;

void WINAPI InternalGetDosName()
{
    if (TransferFileData->DosName != NULL)
    {
        TransferLen = lstrlen(TransferFileData->DosName);
        CopyMemory(TransferBuffer, TransferFileData->DosName, TransferLen);
    }
    else
        TransferLen = 0;
}

void WINAPI InternalGetSize()
{
    if (TransferIsDir && !TransferFileData->SizeValid) // only directories without a known size
    {
        memmove(TransferBuffer, DirColumnStr, DirColumnStrLen);
        TransferLen = DirColumnStrLen;
    }
    else
    {
        switch (Configuration.SizeFormat)
        {
        case SIZE_FORMAT_BYTES:
        {
            TransferLen = NumberToStr2(TransferBuffer, TransferFileData->Size);
            break;
        }

        case SIZE_FORMAT_KB: // WARNING: the same code is elsewhere, search for this constant
        {
            PrintDiskSize(TransferBuffer, TransferFileData->Size, 3);
            TransferLen = (int)strlen(TransferBuffer);
            break;
        }

        case SIZE_FORMAT_MIXED:
        {
            PrintDiskSize(TransferBuffer, TransferFileData->Size, 0);
            TransferLen = (int)strlen(TransferBuffer);
            break;
        }
        }
    }
}

// helper globals for InternalGetType()
char* InternalGetTypeAux1;
char* InternalGetTypeAux2;
char InternalGetTypeAux3[MAX_PATH + 4]; // extension in lowercase, aligned to DWORDs

void WINAPI InternalGetType()
{
    if (TransferIsDir) // we will have to handle directories differently
    {
        TransferLen = TransferIsDir == 1 ? FolderTypeNameLen : UpDirTypeNameLen;
        memcpy(TransferBuffer, TransferIsDir == 1 ? FolderTypeName : UpDirTypeName, TransferLen);
    }
    else
    {
        if (TransferAssocIndex == -2) // the extension lookup has not run yet
        {
            if (TransferFileData->Ext[0] != 0)
            {
                InternalGetTypeAux1 = InternalGetTypeAux3;
                InternalGetTypeAux2 = TransferFileData->Ext;
                while (*InternalGetTypeAux2 != 0)
                    *InternalGetTypeAux1++ = LowerCase[*InternalGetTypeAux2++];
                *((DWORD*)InternalGetTypeAux1) = 0;
                if (!Associations.GetIndex(InternalGetTypeAux3, TransferAssocIndex))
                    TransferAssocIndex = -1; // not found
            }
            else
                TransferAssocIndex = -1; // without an extension -> cannot be in Associations
        }

        if (TransferAssocIndex == -1)
            GetCommonFileTypeStr(TransferBuffer, &TransferLen, TransferFileData->Ext);
        else
        {
            InternalGetTypeAux1 = Associations[TransferAssocIndex].Type;
            if (InternalGetTypeAux1 != NULL) // valid file type
            {
                TransferLen = (int)strlen(InternalGetTypeAux1);
                memcpy(TransferBuffer, InternalGetTypeAux1, TransferLen);
            }
            else
                GetCommonFileTypeStr(TransferBuffer, &TransferLen, TransferFileData->Ext);
        }
    }
}

// we can afford this optimization because we are not called from multiple threads simultaneously
static SYSTEMTIME InternalColumnST;
static FILETIME InternalColumnFT;

void WINAPI InternalGetDate()
{
    if ((TransferRowData & 0x00000001) == 0)
    {
        if (!FileTimeToLocalFileTime(&TransferFileData->LastWrite, &InternalColumnFT) ||
            !FileTimeToSystemTime(&InternalColumnFT, &InternalColumnST))
        {
            TransferLen = sprintf(TransferBuffer, LoadStr(IDS_INVALID_DATEORTIME));
            return;
        }
        TransferRowData |= 0x00000001;
    }
    TransferLen = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &InternalColumnST, NULL, TransferBuffer, TRANSFER_BUFFER_MAX) - 1;
    if (TransferLen < 0)
        TransferLen = sprintf(TransferBuffer, "%u.%u.%u", InternalColumnST.wDay, InternalColumnST.wMonth, InternalColumnST.wYear);
}

void WINAPI InternalGetDateOnlyForDisk()
{
    if ((TransferRowData & 0x00000001) == 0)
    {
        if (!FileTimeToLocalFileTime(&TransferFileData->LastWrite, &InternalColumnFT) ||
            !FileTimeToSystemTime(&InternalColumnFT, &InternalColumnST))
        {
            TransferLen = sprintf(TransferBuffer, LoadStr(IDS_INVALID_DATEORTIME));
            return;
        }
        TransferRowData |= 0x00000001;
    }
    if (TransferIsDir == 2 /* UP-DIR */ &&
        InternalColumnST.wYear == 1602 && InternalColumnST.wMonth == 1 && InternalColumnST.wDay == 1 &&
        InternalColumnST.wHour == 0 && InternalColumnST.wMinute == 0 && InternalColumnST.wSecond == 0 &&
        InternalColumnST.wMilliseconds == 0)
    {
        TransferLen = 0;
        return;
    }
    TransferLen = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &InternalColumnST, NULL, TransferBuffer, TRANSFER_BUFFER_MAX) - 1;
    if (TransferLen < 0)
        TransferLen = sprintf(TransferBuffer, "%u.%u.%u", InternalColumnST.wDay, InternalColumnST.wMonth, InternalColumnST.wYear);
}

void WINAPI InternalGetTime()
{
    if ((TransferRowData & 0x00000001) == 0)
    {
        if (!FileTimeToLocalFileTime(&TransferFileData->LastWrite, &InternalColumnFT) ||
            !FileTimeToSystemTime(&InternalColumnFT, &InternalColumnST))
        {
            TransferLen = sprintf(TransferBuffer, LoadStr(IDS_INVALID_DATEORTIME));
            return;
        }
        TransferRowData |= 0x00000001;
    }
    TransferLen = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &InternalColumnST, NULL, TransferBuffer, TRANSFER_BUFFER_MAX) - 1;
    if (TransferLen < 0)
        TransferLen = sprintf(TransferBuffer, "%u:%02u:%02u", InternalColumnST.wHour, InternalColumnST.wMinute, InternalColumnST.wSecond);
}

void WINAPI InternalGetTimeOnlyForDisk()
{
    if ((TransferRowData & 0x00000001) == 0)
    {
        if (!FileTimeToLocalFileTime(&TransferFileData->LastWrite, &InternalColumnFT) ||
            !FileTimeToSystemTime(&InternalColumnFT, &InternalColumnST))
        {
            TransferLen = sprintf(TransferBuffer, LoadStr(IDS_INVALID_DATEORTIME));
            return;
        }
        TransferRowData |= 0x00000001;
    }
    if (TransferIsDir == 2 /* UP-DIR */ &&
        InternalColumnST.wYear == 1602 && InternalColumnST.wMonth == 1 && InternalColumnST.wDay == 1 &&
        InternalColumnST.wHour == 0 && InternalColumnST.wMinute == 0 && InternalColumnST.wSecond == 0 &&
        InternalColumnST.wMilliseconds == 0)
    {
        TransferLen = 0;
        return;
    }
    TransferLen = GetTimeFormat(LOCALE_USER_DEFAULT, 0, &InternalColumnST, NULL, TransferBuffer, TRANSFER_BUFFER_MAX) - 1;
    if (TransferLen < 0)
        TransferLen = sprintf(TransferBuffer, "%u:%02u:%02u", InternalColumnST.wHour, InternalColumnST.wMinute, InternalColumnST.wSecond);
}

void WINAPI InternalGetAttr()
{
    TransferLen = 0;
    // WARNING: if we want to display more attributes, we must rework GetAttrsString() and the DISPLAYED_ATTRIBUTES mask!!!
    if (TransferFileData->Attr & FILE_ATTRIBUTE_READONLY)
        TransferBuffer[TransferLen++] = 'R';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_HIDDEN)
        TransferBuffer[TransferLen++] = 'H';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_SYSTEM)
        TransferBuffer[TransferLen++] = 'S';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_ARCHIVE)
        TransferBuffer[TransferLen++] = 'A';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_TEMPORARY)
        TransferBuffer[TransferLen++] = 'T';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_COMPRESSED)
        TransferBuffer[TransferLen++] = 'C';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_ENCRYPTED)
        TransferBuffer[TransferLen++] = 'E';
    if (TransferFileData->Attr & FILE_ATTRIBUTE_OFFLINE)
        TransferBuffer[TransferLen++] = 'O';
}

void WINAPI InternalGetDescr()
{
    if (TransferIsDir)
    {
        TransferLen = TransferIsDir == 1 ? FolderTypeNameLen : UpDirTypeNameLen;
        memcpy(TransferBuffer, TransferIsDir == 1 ? FolderTypeName : UpDirTypeName, TransferLen);
    }
    else
    {
        InternalGetType();
    }
}

static BOOL BuildExplorerColumnPathW(const char* panelPath, const WCHAR* panelPathW,
                                     const CFileData* fileData, WCHAR* pathW, int pathSize)
{
    if (pathW == NULL || pathSize <= 0 || fileData == NULL)
        return FALSE;

    pathW[0] = 0;
    if (panelPathW != NULL && panelPathW[0] != 0)
        lstrcpynW(pathW, panelPathW, pathSize);
    else if (panelPath == NULL || panelPath[0] == 0 ||
             MultiByteToWideChar(CP_ACP, 0, panelPath, -1, pathW, pathSize) <= 0)
    {
        return FALSE;
    }

    int pathLen = lstrlenW(pathW);
    if (pathLen == 0)
        return FALSE;
    if (pathW[pathLen - 1] != L'\\')
    {
        if (pathLen + 1 >= pathSize)
            return FALSE;
        pathW[pathLen++] = L'\\';
        pathW[pathLen] = 0;
    }

    if (fileData->UseWideName())
    {
        const WCHAR* nameW = (const WCHAR*)fileData->NameW;
        int nameLen = lstrlenW(nameW);
        if (pathLen + nameLen >= pathSize)
            return FALSE;
        memcpy(pathW + pathLen, nameW, (nameLen + 1) * sizeof(WCHAR));
    }
    else if (MultiByteToWideChar(CP_ACP, 0, fileData->Name, -1,
                                 pathW + pathLen, pathSize - pathLen) <= 0)
    {
        return FALSE;
    }
    return TRUE;
}

static BOOL ConvertExplorerColumnText(const WCHAR* text, char* buffer, int bufferSize)
{
    if (text == NULL || buffer == NULL || bufferSize <= 0)
        return FALSE;

    buffer[0] = 0;
    int textLen = lstrlenW(text);
    int low = 0;
    int high = textLen;
    while (low < high)
    {
        int count = low + (high - low + 1) / 2;
        int bytes = WideCharToMultiByte(CP_ACP, 0, text, count, NULL, 0, NULL, NULL);
        if (bytes > 0 && bytes < bufferSize)
            low = count;
        else
            high = count - 1;
    }
    if (low > 0 && text[low - 1] >= 0xd800 && text[low - 1] <= 0xdbff)
        low--;

    int bytes = 0;
    if (low > 0)
        bytes = WideCharToMultiByte(CP_ACP, 0, text, low, buffer, bufferSize - 1, NULL, NULL);
    if (low > 0 && bytes <= 0)
        return FALSE;
    buffer[bytes] = 0;
    return TRUE;
}

BOOL GetExplorerColumnTextForFile(const char* panelPath, const WCHAR* panelPathW, const CFileData* fileData,
                                  int columnIndex, char* buffer, int bufferSize)
{
    if (buffer == NULL || bufferSize <= 0)
        return FALSE;
    buffer[0] = 0;
    if (fileData == NULL)
        return FALSE;

    WCHAR pathW[SAL_MAX_PATH];
    if (!BuildExplorerColumnPathW(panelPath, panelPathW, fileData, pathW, SAL_MAX_PATH))
        return FALSE;

    return GetExplorerColumnTextForPathW(pathW, columnIndex, buffer, bufferSize);
}

BOOL GetExplorerColumnTextForPathW(const WCHAR* pathW, int columnIndex, char* buffer, int bufferSize)
{
    if (buffer == NULL || bufferSize <= 0)
        return FALSE;
    buffer[0] = 0;
    if (pathW == NULL || pathW[0] == 0)
        return FALSE;

    LoadExplorerColumns();
    if (columnIndex < 0 || columnIndex >= ExplorerColumnsCount)
        return FALSE;

    IPropertyStore* store = NULL;
    if (FAILED(SHGetPropertyStoreFromParsingName(pathW, NULL, GPS_DEFAULT, IID_IPropertyStore, (void**)&store)) || store == NULL)
        return FALSE;

    BOOL ret = FALSE;
    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(store->GetValue(ExplorerColumnKeys[columnIndex], &value)))
    {
        PWSTR display = NULL;
        if (SUCCEEDED(PSFormatForDisplayAlloc(ExplorerColumnKeys[columnIndex], value, PDFF_DEFAULT, &display)) && display != NULL)
        {
            if (ConvertExplorerColumnText(display, buffer, bufferSize))
                ret = TRUE;
            CoTaskMemFree(display);
        }
    }
    PropVariantClear(&value);
    store->Release();
    return ret;
}

static void TrimFileTag(std::wstring& tag)
{
    size_t first = 0;
    while (first < tag.size() && iswspace(tag[first]))
        first++;
    size_t last = tag.size();
    while (last > first && iswspace(tag[last - 1]))
        last--;
    tag = tag.substr(first, last - first);
}

static BOOL ContainsFileTag(const std::vector<std::wstring>& tags, const std::wstring& tag)
{
    for (size_t i = 0; i < tags.size(); i++)
    {
        if (_wcsicmp(tags[i].c_str(), tag.c_str()) == 0)
            return TRUE;
    }
    return FALSE;
}

void ParseFileTagsW(const wchar_t* text, std::vector<std::wstring>& tags)
{
    tags.clear();
    if (text == NULL)
        return;

    const wchar_t* start = text;
    for (const wchar_t* p = text;; p++)
    {
        if (*p == L';' || *p == 0)
        {
            std::wstring tag(start, p - start);
            TrimFileTag(tag);
            if (!tag.empty() && !ContainsFileTag(tags, tag))
                tags.push_back(tag);
            if (*p == 0)
                break;
            start = p + 1;
        }
    }
}

std::wstring FormatFileTagsW(const std::vector<std::wstring>& tags)
{
    std::wstring result;
    for (size_t i = 0; i < tags.size(); i++)
    {
        if (!result.empty())
            result += L"; ";
        result += tags[i];
    }
    return result;
}

HRESULT ReadFileTagsW(const wchar_t* path, std::vector<std::wstring>& tags)
{
    tags.clear();
    if (path == NULL || path[0] == 0)
        return E_INVALIDARG;

    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, NULL, GPS_DEFAULT,
                                                    IID_IPropertyStore, (void**)&store);
    if (FAILED(hr) || store == NULL)
        return hr;

    PROPVARIANT value;
    PropVariantInit(&value);
    hr = store->GetValue(PKEY_Keywords, &value);
    if (SUCCEEDED(hr) && value.vt != VT_EMPTY && value.vt != VT_NULL)
    {
        PWSTR* values = NULL;
        ULONG count = 0;
        hr = PropVariantToStringVectorAlloc(value, &values, &count);
        if (SUCCEEDED(hr))
        {
            for (ULONG i = 0; i < count; i++)
            {
                std::wstring tag = values[i] != NULL ? values[i] : L"";
                TrimFileTag(tag);
                if (!tag.empty() && !ContainsFileTag(tags, tag))
                    tags.push_back(tag);
                CoTaskMemFree(values[i]);
            }
            CoTaskMemFree(values);
        }
    }
    else if (SUCCEEDED(hr))
        hr = S_OK;

    PropVariantClear(&value);
    store->Release();
    return hr;
}

BOOL IsFilePropertyWritableW(const wchar_t* path, REFPROPERTYKEY key)
{
    if (path == NULL || path[0] == 0)
        return FALSE;

    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, NULL, GPS_READWRITE,
                                                    IID_IPropertyStore, (void**)&store);
    if (FAILED(hr) || store == NULL)
        return FALSE;

    BOOL writable = TRUE;
    IPropertyStoreCapabilities* capabilities = NULL;
    if (SUCCEEDED(store->QueryInterface(IID_PPV_ARGS(&capabilities))) && capabilities != NULL)
    {
        writable = capabilities->IsPropertyWritable(key) == S_OK;
        capabilities->Release();
    }
    store->Release();
    return writable;
}

HRESULT WriteFileStringVectorPropertyW(const wchar_t* path, REFPROPERTYKEY key,
                                       const std::vector<std::wstring>& tags)
{
    if (path == NULL || path[0] == 0)
        return E_INVALIDARG;

    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, NULL, GPS_READWRITE,
                                                    IID_IPropertyStore, (void**)&store);
    if (FAILED(hr) || store == NULL)
        return hr;

    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(hr) && !tags.empty())
    {
        std::vector<PCWSTR> values;
        for (size_t i = 0; i < tags.size(); i++)
            values.push_back(tags[i].c_str());
        hr = InitPropVariantFromStringVector(values.data(), (ULONG)values.size(), &value);
    }
    if (SUCCEEDED(hr))
        hr = store->SetValue(key, value);
    if (SUCCEEDED(hr))
        hr = store->Commit();

    PropVariantClear(&value);
    store->Release();
    return hr;
}

HRESULT WriteFileTagsW(const wchar_t* path, const std::vector<std::wstring>& tags)
{
    return WriteFileStringVectorPropertyW(path, PKEY_Keywords, tags);
}

HRESULT UpdateFileTagsW(const wchar_t* path, const std::vector<std::wstring>& tags,
                        CFileTagsOperation operation)
{
    if (operation == ftoReplace)
        return WriteFileTagsW(path, tags);

    std::vector<std::wstring> current;
    HRESULT hr = ReadFileTagsW(path, current);
    if (FAILED(hr))
        return hr;

    if (operation == ftoAdd)
    {
        for (size_t i = 0; i < tags.size(); i++)
        {
            if (!ContainsFileTag(current, tags[i]))
                current.push_back(tags[i]);
        }
    }
    else
    {
        for (std::vector<std::wstring>::iterator it = current.begin(); it != current.end();)
        {
            if (ContainsFileTag(tags, *it))
                it = current.erase(it);
            else
                ++it;
        }
    }
    return WriteFileTagsW(path, current);
}

BOOL FileTagsMatchW(const wchar_t* path, const std::vector<std::wstring>& tags,
                    CFileTagsMatchMode mode)
{
    if (tags.empty())
        return TRUE;

    std::vector<std::wstring> fileTags;
    if (FAILED(ReadFileTagsW(path, fileTags)))
        return mode == ftmmNone;

    int found = 0;
    for (size_t i = 0; i < tags.size(); i++)
    {
        if (ContainsFileTag(fileTags, tags[i]))
            found++;
    }
    if (mode == ftmmAny)
        return found > 0;
    if (mode == ftmmAll)
        return found == (int)tags.size();
    return found == 0;
}

HRESULT ReadFilePropertyTextW(const wchar_t* path, REFPROPERTYKEY key, std::wstring& text)
{
    text.clear();
    if (path == NULL || path[0] == 0)
        return E_INVALIDARG;

    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, NULL, GPS_DEFAULT,
                                                    IID_IPropertyStore, (void**)&store);
    if (FAILED(hr) || store == NULL)
        return hr;
    PROPVARIANT value;
    PropVariantInit(&value);
    hr = store->GetValue(key, &value);
    if (SUCCEEDED(hr) && value.vt != VT_EMPTY && value.vt != VT_NULL)
    {
        PWSTR display = NULL;
        hr = PSFormatForDisplayAlloc(key, value, PDFF_DEFAULT, &display);
        if (SUCCEEDED(hr) && display != NULL)
        {
            text = display;
            CoTaskMemFree(display);
        }
    }
    else if (SUCCEEDED(hr))
        hr = S_OK;
    PropVariantClear(&value);
    store->Release();
    return hr;
}

static HRESULT InitFilePropertyValueFromText(const wchar_t* text,
                                             IPropertyDescription* description,
                                             PROPVARIANT* value)
{
    IPropertyEnumTypeList* enumTypes = NULL;
    HRESULT hr = description->GetEnumTypeList(IID_IPropertyEnumTypeList,
                                               (void**)&enumTypes);
    if (SUCCEEDED(hr) && enumTypes != NULL)
    {
        UINT count = 0;
        if (SUCCEEDED(enumTypes->GetCount(&count)))
        {
            for (UINT i = 0; i < count; i++)
            {
                IPropertyEnumType* enumType = NULL;
                if (SUCCEEDED(enumTypes->GetAt(i, IID_IPropertyEnumType,
                                               (void**)&enumType)) &&
                    enumType != NULL)
                {
                    LPWSTR displayText = NULL;
                    HRESULT displayResult = enumType->GetDisplayText(&displayText);
                    BOOL matches = SUCCEEDED(displayResult) && displayText != NULL &&
                                   _wcsicmp(displayText, text != NULL ? text : L"") == 0;
                    CoTaskMemFree(displayText);
                    if (matches)
                        hr = enumType->GetValue(value);
                    enumType->Release();
                    if (matches)
                    {
                        enumTypes->Release();
                        return hr;
                    }
                }
            }
        }
        enumTypes->Release();
    }

    hr = InitPropVariantFromString(text != NULL ? text : L"", value);
    if (SUCCEEDED(hr))
        hr = description->CoerceToCanonicalValue(value);
    return hr;
}

HRESULT WriteFilePropertyTextW(const wchar_t* path, REFPROPERTYKEY key,
                               const wchar_t* text, BOOL clearValue)
{
    if (path == NULL || path[0] == 0)
        return E_INVALIDARG;

    IPropertyStore* store = NULL;
    HRESULT hr = SHGetPropertyStoreFromParsingName(path, NULL, GPS_READWRITE,
                                                    IID_IPropertyStore, (void**)&store);
    if (FAILED(hr) || store == NULL)
        return hr;

    PROPVARIANT value;
    PropVariantInit(&value);
    IPropertyDescription* description = NULL;
    if (SUCCEEDED(hr) && !clearValue)
    {
        hr = PSGetPropertyDescription(key, IID_IPropertyDescription, (void**)&description);
        if (SUCCEEDED(hr) && description != NULL)
            hr = InitFilePropertyValueFromText(text, description, &value);
    }
    if (SUCCEEDED(hr))
        hr = store->SetValue(key, value);
    if (SUCCEEDED(hr))
        hr = store->Commit();

    if (description != NULL)
        description->Release();
    PropVariantClear(&value);
    store->Release();
    return hr;
}

void WINAPI InternalGetExplorerColumn()
{
    TransferLen = 0;
    if (TransferIsDir == 2)
        return;

    char text[TRANSFER_BUFFER_MAX];
    BOOL hasText = TransferPanelWindow != NULL
                       ? TransferPanelWindow->GetCachedExplorerColumnText(
                             TransferFileData, (int)TransferActCustomData,
                             text, TRANSFER_BUFFER_MAX)
                       : GetExplorerColumnTextForFile(
                             TransferPanelPath, TransferPanelPathW, TransferFileData,
                             (int)TransferActCustomData, text, TRANSFER_BUFFER_MAX);
    if (hasText)
    {
        TransferLen = (int)strlen(text);
        if (TransferLen > TRANSFER_BUFFER_MAX)
            TransferLen = TRANSFER_BUFFER_MAX;
        memcpy(TransferBuffer, text, TransferLen);
    }
}


//****************************************************************************
//
// Internal function to obtain the index of simple icons for file systems with their own icons (pitFromPlugin)
//

int WINAPI InternalGetPluginIconIndex()
{
    return 0;
}

//*****************************************************************************
//
// CSalamanderView
//

CSalamanderView::CSalamanderView(CFilesWindow* panel)
{
    Panel = panel;
    Panel->GetPluginIconIndex = InternalGetPluginIconIndex;
}

DWORD
CSalamanderView::GetViewMode()
{
    return Panel->ViewTemplate->Mode;
}

void CSalamanderView::SetViewMode(DWORD viewMode, DWORD validFileData)
{
    if (Panel->ViewTemplate->Mode != viewMode)
    {
        int templateIndex;
        switch (viewMode)
        {
        case VIEW_MODE_TREE:
            templateIndex = 0;
            break;
        case VIEW_MODE_BRIEF:
            templateIndex = 1;
            break;
        case VIEW_MODE_DETAILED:
            templateIndex = 2;
            break;
        case VIEW_MODE_ICONS:
            templateIndex = 3;
            break;
        case VIEW_MODE_THUMBNAILS:
            templateIndex = 4;
            break;
        case VIEW_MODE_TILES:
            templateIndex = 5;
            break;
        default:
            TRACE_E("Unknown viewMode=" << viewMode);
            templateIndex = 2;
            break;
        }
        Panel->SelectViewTemplate(templateIndex, FALSE, TRUE, validFileData);
    }
    else
        Panel->SelectViewTemplate(Panel->GetViewTemplateIndex(), FALSE, TRUE, validFileData);
}

void CSalamanderView::SetPluginSimpleIconCallback(FGetPluginIconIndex callback)
{
    Panel->GetPluginIconIndex = callback;
}

int CSalamanderView::GetColumnsCount()
{
    return Panel->Columns.Count;
}

const CColumn*
CSalamanderView::GetColumn(int index)
{
    return (index >= 0 && index < Panel->Columns.Count) ? &Panel->Columns[index] : NULL;
}

BOOL CSalamanderView::InsertColumn(int index, const CColumn* column)
{
    int low = 1;
    if (Panel->Columns.Count > 1 && Panel->Columns[1].ID == COLUMN_ID_EXTENSION)
        low++; // must not let it wedge between Name and Ext
    if (index < low || index > Panel->Columns.Count)
    {
        TRACE_E("CSalamanderView::InsertColumn(): index=" << index << " is incorrect.");
        return FALSE;
    }
    if (column->ID != COLUMN_ID_CUSTOM)
    {
        TRACE_E("CSalamanderView::InsertColumn(): column->ID != COLUMN_ID_CUSTOM.");
        return FALSE;
    }
    Panel->Columns.Insert(index, *column);
    if (!Panel->Columns.IsGood())
    {
        TRACE_E("CSalamanderView::InsertColumn(): Columns.Insert() failed");
        Panel->Columns.ResetState();
        return FALSE;
    }
    return TRUE;
}

BOOL CSalamanderView::InsertStandardColumn(int index, DWORD id)
{
    int low = 1;
    if (Panel->Columns.Count > 1 && Panel->Columns[1].ID == COLUMN_ID_EXTENSION)
        low++; // must not let it wedge between Name and Ext
    if (index < low || index > Panel->Columns.Count ||
        index != 1 && id == COLUMN_ID_EXTENSION)
    {
        TRACE_E("CSalamanderView::InsertStandardColumn(): index=" << index << " is incorrect.");
        return FALSE;
    }
    if (id == COLUMN_ID_NAME || id == COLUMN_ID_CUSTOM)
    {
        TRACE_E("CSalamanderView::InsertStandardColumn(): column->ID == COLUMN_ID_CUSTOM or COLUMN_ID_NAME.");
        return FALSE;
    }

    CColumDataItem* item = NULL;
    int i;
    for (i = 0; i < STANDARD_COLUMNS_COUNT; i++)
    {
        if (GetStdColumn(i, FALSE)->ID == id) // found the requested standard column
        {
            item = GetStdColumn(i, FALSE);
            break;
        }
    }
    if (item != NULL)
    {
        CColumn column;
        column.CustomData = 0;
        lstrcpy(column.Name, LoadStr(item->NameResID));
        lstrcpy(column.Description, LoadStr(item->DescResID));
        column.GetText = item->GetText;
        column.SupportSorting = item->SupportSorting;
        column.LeftAlignment = item->LeftAlignment;
        column.ID = item->ID;
        CColumnConfig* colCfg = Panel->ViewTemplate->Columns;
        BOOL leftPanel = Panel == MainWindow->LeftPanel;
        column.Width = leftPanel ? colCfg[i].LeftWidth : colCfg[i].RightWidth;
        column.FixedWidth = leftPanel ? colCfg[i].LeftFixedWidth : colCfg[i].RightFixedWidth;
        column.MinWidth = 0; // dummy—will be overwritten when sizing HeaderLine

        Panel->Columns.Insert(index, column);
        if (!Panel->Columns.IsGood())
        {
            TRACE_E("CSalamanderView::InsertStandardColumn(): Columns.Insert() failed");
            Panel->Columns.ResetState();
            return FALSE;
        }
        return TRUE;
    }
    else
    {
        TRACE_E("CSalamanderView::InsertStandardColumn(): id=" << id << " is unknown.");
        return FALSE;
    }
}

BOOL CSalamanderView::SetColumnName(int index, const char* name, const char* description)
{
    if (index < 0 || index >= Panel->Columns.Count)
    {
        TRACE_E("CSalamanderView::SetColumnName(): index=" << index << " is out of columns array range.");
        return FALSE;
    }
    if (name == NULL || *name == 0 || description == NULL || *description == 0)
    {
        TRACE_E("CSalamanderView::SetColumnName(): name or description is NULL or empty string.");
        return FALSE;
    }
    if (index == 0 && !Panel->IsExtensionInSeparateColumn() && (Panel->ValidFileData & VALID_DATA_EXTENSION))
    { // check double (twice null-terminated) strings + the non-emptiness of the second one + and their copy
        const char* s = name + strlen(name) + 1;
        const char* beg = s;
        while (s < name + COLUMN_NAME_MAX && *s != 0)
            s++;
        if (s == name + COLUMN_NAME_MAX || beg == s)
        {
            TRACE_E("CSalamanderView::SetColumnName(): name is not double string (names of Name and Ext columns are expected) or second string is empty.");
            return FALSE;
        }
        const char* s2 = description + strlen(description) + 1;
        beg = s2;
        while (s2 < description + COLUMN_DESCRIPTION_MAX && *s2 != 0)
            s2++;
        if (s2 == description + COLUMN_DESCRIPTION_MAX || beg == s2)
        {
            TRACE_E("CSalamanderView::SetColumnName(): description is not double string (descriptions of Name and Ext columns are expected) or second string is empty.");
            return FALSE;
        }
        // copy the double strings
        int l = (int)(s - name);
        if (l >= COLUMN_NAME_MAX)
        {
            TRACE_E("CSalamanderView::SetColumnName(): name is too long! (index=" << index << ")");
            l = COLUMN_NAME_MAX - 1;
        }
        char* txt = Panel->Columns[index].Name;
        memcpy(txt, name, l);
        txt[l] = 0;
        l = (int)(s2 - description);
        if (l >= COLUMN_DESCRIPTION_MAX)
        {
            TRACE_E("CSalamanderView::SetColumnName(): desription is too long! (index=" << index << ")");
            l = COLUMN_DESCRIPTION_MAX - 1;
        }
        txt = Panel->Columns[index].Description;
        memcpy(txt, description, l);
        txt[l] = 0;
    }
    else
    {
        lstrcpyn(Panel->Columns[index].Name, name, COLUMN_NAME_MAX);
        lstrcpyn(Panel->Columns[index].Description, description, COLUMN_DESCRIPTION_MAX);
    }
    return TRUE;
}

BOOL CSalamanderView::DeleteColumn(int index)
{
    if (index < 0 || index >= Panel->Columns.Count)
    {
        TRACE_E("CSalamanderView::DeleteColumn(): index=" << index << " is out of columns array range.");
        return FALSE;
    }
    if (index == 0)
    {
        TRACE_E("CSalamanderView::DeleteColumn(): index=" << index << " Name column can't be deleted.");
        return FALSE;
    }
    Panel->Columns.Delete(index);
    if (!Panel->Columns.IsGood())
        Panel->Columns.ResetState(); // cannot fail; the array just was not shrunk
    return TRUE;
}

//*****************************************************************************
//
// CFileHistoryItem, CFileHistory
//
// Holds a list of files on which the user invoked View or Edit.
//

CFileHistoryItem::CFileHistoryItem(CFileHistoryItemTypeEnum type, DWORD handlerID, const char* fileName)
{
    Type = type;
    HandlerID = handlerID;
    FileName = DupStr(fileName);
    HIcon = NULL;
    if (FileName == NULL)
        return;

    // try to pull the icon from the system
    HIcon = GetFileOrPathIconAux(FileName, FALSE, FALSE);
}

CFileHistoryItem::~CFileHistoryItem()
{
    if (FileName != NULL)
        free(FileName);
    if (HIcon != NULL)
        HANDLES(DestroyIcon(HIcon));
}

BOOL CFileHistoryItem::Equal(CFileHistoryItemTypeEnum type, DWORD handlerID, const char* fileName)
{
    return (Type == type && HandlerID == handlerID && lstrcmp(FileName, fileName) == 0);
}

BOOL CFileHistoryItem::Execute()
{
    CALL_STACK_MESSAGE1("CFileHistoryItem::Execute()");
    CFilesWindow* panel = MainWindow->GetActivePanel();
    switch (Type)
    {
    case fhitView:
        panel->ViewFile(FileName, FALSE, HandlerID, -1, -1);
        break;
    case fhitEdit:
        panel->EditFile(FileName, HandlerID);
        break;
    case fhitOpen:
    {
        char buff[MAX_PATH];
        lstrcpy(buff, FileName);
        char* ptr = strrchr(buff, '\\');
        if (ptr != NULL)
        {
            *ptr = 0; // split the path into the path part and the file name
            HCURSOR hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
            MainWindow->SetDefaultDirectories(); // so the starting process inherits the correct current directories
            ExecuteAssociation(panel->GetListBoxHWND(), buff, ptr + 1);
            SetCursor(hOldCur);
        }
        break;
    }

    default:
        TRACE_E("Unknown Type=" << Type);
    }

    return TRUE;
}

//****************************************************************************
//
// A set of functions for opening associations using SalOpen.exe
//

BOOL SalOpenInit()
{
    // allocate shared space in pagefile.sys
    SalOpenFileMapping = HANDLES(CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, // FIXME_X64 are we passing x86/x64 incompatible data?
                                                   MAX_PATH + 200, NULL));
    if (SalOpenFileMapping != NULL)
    {
        SalOpenSharedMem = HANDLES(MapViewOfFile(SalOpenFileMapping, FILE_MAP_WRITE, 0, 0, 0)); // FIXME_X64 are we passing x86/x64 incompatible data?
        if (SalOpenSharedMem == NULL)
            TRACE_E("Unable to allocate shared memory (map view of file) for SalOpen.");
        else
            return TRUE;
    }
    else
        TRACE_E("Unable to allocate shared memory (create file mapping) for SalOpen.");
    return FALSE;
}

BOOL SalOpenExecute(HWND hWindow, const char* fileName)
{
    CALL_STACK_MESSAGE2("SalOpenExecute(, %s)", fileName);

    // initialization required for launching SalOpen
    static BOOL initCalled = FALSE;
    if (!initCalled)
    {
        initCalled = TRUE;
        SalOpenInit();
    }

    if (SalOpenSharedMem != NULL)
    {
        lstrcpyn((char*)SalOpenSharedMem, fileName, MAX_PATH + 200);

        char cmdline[SAL_MAX_PATH];
        cmdline[0] = '"';
        if (GetModuleFileName(NULL, cmdline + 1, SAL_MAX_PATH - 1) == 0)
            return FALSE;
        char* ptr = strrchr(cmdline, '\\');
        if (ptr == NULL)
            return FALSE;
        *ptr = 0;
        SalPathAppend(cmdline + 1, "utils\\salopen.exe", SAL_MAX_PATH - 1);
        char add[100];
        RECT r;
        MultiMonGetClipRectByWindow(GetTopVisibleParent(hWindow), &r, NULL);
        sprintf(add, "\" %u %Iu %u", GetCurrentProcessId(), (DWORD_PTR)SalOpenFileMapping, (DWORD)MAKELPARAM(r.left, r.top));
        if (strlen(cmdline) + strlen(add) >= SAL_MAX_PATH)
            return FALSE;
        strcat(cmdline, add);

        // start the salopen.exe process
        STARTUPINFO si;
        memset(&si, 0, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        PROCESS_INFORMATION pi;
        {
            CALL_STACK_MESSAGE1("SalOpenExecute::create-process");
            if (!HANDLES_Q(CreateProcess(NULL, cmdline, NULL, NULL, FALSE,
                                         CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
                                         NULL, NULL, &si, &pi)))
            {
                DWORD err = GetLastError();
                TRACE_E("SalOpenExecute failed: \"" << cmdline << "\", " << GetErrorText(err));
                return FALSE;
            }
            else
            {
                { // when waiting, DDE associations (.html, .h, .cpp, etc.) do not work
                    //          CALL_STACK_MESSAGE1("SalOpenExecute::wait-for-process");
                    //          WaitForSingleObject(pi.hProcess, INFINITE);
                }
                HANDLES(CloseHandle(pi.hProcess));
                HANDLES(CloseHandle(pi.hThread));
            }
        }

        return TRUE;
    }
    return FALSE;
}

void ReleaseSalOpen()
{
    if (SalOpenSharedMem != NULL)
        HANDLES(UnmapViewOfFile(SalOpenSharedMem));
    if (SalOpenFileMapping != NULL)
        HANDLES(CloseHandle(SalOpenFileMapping));
    SalOpenSharedMem = NULL;
    SalOpenFileMapping = NULL;
}

BOOL IsFileURLPath(const char* path)
{
    if (path == NULL)
        return FALSE;
    // skip whitespaces at the beginning of the string
    const char* s = path;
    while (*s != 0 && *s <= ' ')
        s++;
    // find the FS name
    const char* name = s;
    while (*s != 0 && *s != ':' && s - name < 4)
        s++;
    return *s == ':' && s - name == 4 && StrNICmp(name, "file", 4) == 0;
}

BOOL IsPluginFSPath(char* path, char* fsName, char** userPart)
{
    return IsPluginFSPath((const char*)path, fsName, (const char**)userPart);
}

BOOL IsPluginFSPath(const char* path, char* fsName, const char** userPart)
{
    CALL_STACK_MESSAGE2("IsPluginFSPath(%s, ,)", path);

    if (path == NULL)
        return FALSE;
    const char* start = path;
    // skip whitespaces at the beginning of the string
    while (*start >= 1 && *start <= ' ')
        start++;
    // find the FS name
    const char* name = start;
    while (LowerCase[*name] >= 'a' && LowerCase[*name] <= 'z' ||
           *name >= '0' && *name <= '9' || *name == '_' || *name == '-' || *name == '+')
        name++;
    // test whether the FS name meets all conditions (a ':' follows and length >= 2 characters)
    if (*name == ':' && name - start >= 2 && name - start < MAX_PATH)
    {
        // copy the FS name
        if (fsName != NULL)
        {
            memmove(fsName, start, name - start);
            fsName[name - start] = 0;
        }
        // pointer into 'path' to the first character of the plugin-defined path (after the first ':')
        if (userPart != NULL)
            *userPart = name + 1;
        return TRUE;
    }
    else
        return FALSE;
}
