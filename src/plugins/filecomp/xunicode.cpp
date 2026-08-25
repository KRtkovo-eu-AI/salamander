// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

char* TCharSpecific<char>::LowerCase = ::LowerCase;
unsigned short* TCharSpecific<char>::CType = ::CType;

wchar_t TCharSpecific<wchar_t>::LowerCase[256 * 256];
unsigned short TCharSpecific<wchar_t>::CType[256 * 256];

static UINT FileCompLocaleInteger(LCTYPE type, UINT fallback)
{
    wchar_t buf[16] = {};
    if (GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT, type, buf, 16) <= 0 &&
        GetLocaleInfoW(LOCALE_USER_DEFAULT, type, buf, 16) <= 0)
        return fallback;
    UINT parsed = (UINT)_wtoi(buf);
    return (parsed != 0 && parsed != CP_UTF8) ? parsed : fallback;
}

UINT FileCompLegacyAnsiCodePage()
{
    UINT acp = GetACP();
    if (acp != 0 && acp != CP_UTF8)
        return acp;
    return FileCompLocaleInteger(LOCALE_IDEFAULTANSICODEPAGE, 1250);
}

static bool FileCompAnsi8ToWide(const char* src, int srcLen, std::wstring& dst)
{
    dst.clear();
    if (src == NULL || srcLen <= 0)
        return true;

    UINT cp = FileCompLegacyAnsiCodePage();
    int wideLen = MultiByteToWideChar(cp, MB_PRECOMPOSED, src, srcLen, NULL, 0);
    if (wideLen <= 0)
        return false;
    dst.assign((size_t)wideLen, L'\0');
    return MultiByteToWideChar(cp, MB_PRECOMPOSED, src, srcLen, &dst[0], wideLen) == wideLen;
}

wchar_t
TCharSpecific<wchar_t>::ConvertANSI8Char(char c)
{
    // Latin-1 fallback keeps 0xB7 as U+00B7 when conversion fails (UTF-8 ACP cannot decode it).
    wchar_t ret = (wchar_t)(unsigned char)c;
    wchar_t converted = 0;
    if (MultiByteToWideChar(FileCompLegacyAnsiCodePage(), MB_PRECOMPOSED, &c, 1, &converted, 1) == 1)
        return converted;
    return ret;
}

BOOL ExtTextOutX(HDC hdc, int X, int Y, UINT fuOptions, CONST RECT* lprc, LPCSTR lpString, UINT cbCount, CONST INT* lpDx)
{
    if (GetACP() != CP_UTF8)
        return ExtTextOutA(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);

    std::wstring wide;
    if (lpString == NULL || cbCount == 0)
        return ExtTextOutW(hdc, X, Y, fuOptions, lprc, L"", 0, lpDx);
    if (!FileCompAnsi8ToWide(lpString, (int)cbCount, wide))
        return ExtTextOutA(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
    return ExtTextOutW(hdc, X, Y, fuOptions, lprc, wide.c_str(), (UINT)wide.size(), lpDx);
}

int DrawTextExX(HDC hdc, LPSTR lpchText, int cchText, LPRECT lprc, UINT dwDTFormat,
                LPDRAWTEXTPARAMS lpDTParams)
{
    if (GetACP() != CP_UTF8)
        return DrawTextExA(hdc, lpchText, cchText, lprc, dwDTFormat, lpDTParams);

    if (lpchText == NULL)
        return DrawTextExA(hdc, lpchText, cchText, lprc, dwDTFormat, lpDTParams);

    int srcLen = cchText;
    if (srcLen < 0)
        srcLen = (int)strlen(lpchText);

    std::wstring wide;
    if (!FileCompAnsi8ToWide(lpchText, srcLen, wide))
        return DrawTextExA(hdc, lpchText, cchText, lprc, dwDTFormat, lpDTParams);
    wchar_t emptyBuf[1] = {0};
    return DrawTextExW(hdc, wide.empty() ? emptyBuf : &wide[0], (int)wide.size(),
                       lprc, dwDTFormat, lpDTParams);
}

BOOL PolyTextOutX(HDC hdc, CONST POLYTEXTA* pptxt, int cStrings)
{
    if (GetACP() != CP_UTF8)
        return PolyTextOutA(hdc, pptxt, cStrings);
    if (pptxt == NULL || cStrings <= 0)
        return PolyTextOutA(hdc, pptxt, cStrings);

    std::vector<POLYTEXTW> wide((size_t)cStrings);
    std::vector<std::wstring> strings((size_t)cStrings);
    for (int i = 0; i < cStrings; i++)
    {
        wide[i].x = pptxt[i].x;
        wide[i].y = pptxt[i].y;
        wide[i].uiFlags = pptxt[i].uiFlags;
        wide[i].rcl = pptxt[i].rcl;
        wide[i].pdx = pptxt[i].pdx;
        if (pptxt[i].lpstr != NULL && pptxt[i].n > 0)
        {
            if (!FileCompAnsi8ToWide(pptxt[i].lpstr, pptxt[i].n, strings[i]))
                return PolyTextOutA(hdc, pptxt, cStrings);
            wide[i].lpstr = strings[i].c_str();
            wide[i].n = (UINT)strings[i].size();
        }
        else
        {
            wide[i].lpstr = L"";
            wide[i].n = 0;
        }
    }
    return PolyTextOutW(hdc, &wide[0], cStrings);
}

void InitXUnicode()
{
    _ASSERT(TCharSpecific<char>::CType == ::CType);
    _ASSERT(TCharSpecific<char>::LowerCase == ::LowerCase);

    _ASSERT(sizeof(wchar_t) == 2);

    wchar_t charTable[256 * 256];
    uintptr_t i;
    for (i = 0; i < 256 * 256; i++)
    {
        // special handling for surrogates
        if (i >= 0xD800 && i <= 0xDFFF)
        {
            charTable[i] = 0;
            TCharSpecific<wchar_t>::LowerCase[i] = (wchar_t)i;
        }
        else
        {
            charTable[i] = wchar_t(i);
            TCharSpecific<wchar_t>::LowerCase[i] = (wchar_t)(UINT_PTR)CharLowerW((LPWSTR)i);
        }
    }

    if (!GetStringTypeW(CT_CTYPE1, charTable, 256 * 256, TCharSpecific<wchar_t>::CType))
    {
        TRACE_E("GetStringTypeW failed. Last Error = " << GetLastError());
    }

    // sanitize surrogates in CType table
    for (i = 0xD800; i <= 0xDFFF; i++)
        TCharSpecific<wchar_t>::CType[i] = 0;
}
