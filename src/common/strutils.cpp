// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <windows.h>

#pragma warning(3 : 4706) // warning C4706: assignment within conditional expression

#include "strutils.h"
#include "unicode.h"

using salamander::unicode::SalWideString;

int ConvertU2A(const WCHAR* src, int srcLen, char* buf, int bufSize, BOOL compositeCheck, UINT codepage)
{
    if (buf == NULL || bufSize <= 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    buf[0] = 0;
    if (src == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (srcLen != -1 && srcLen < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (srcLen == 0)
        return 1;

    size_t length = (srcLen == -1) ? wcslen(src) : static_cast<size_t>(srcLen);
    SalWideString view(std::wstring_view(src, length));
    if (!view)
        return 0;

    std::string converted = view.ToAnsi(compositeCheck, codepage);
    if (converted.empty() && view.length() > 0)
        return 0;

    size_t required = converted.size() + 1;
    if (required > static_cast<size_t>(bufSize))
    {
        size_t copyCount = static_cast<size_t>(bufSize - 1);
        if (copyCount > 0)
            memcpy(buf, converted.data(), copyCount);
        buf[bufSize - 1] = 0;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    if (!converted.empty())
        memcpy(buf, converted.data(), converted.size());
    buf[converted.size()] = 0;
    return static_cast<int>(required);
}

char* ConvertAllocU2A(const WCHAR* src, int srcLen, BOOL compositeCheck, UINT codepage)
{
    if (src == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (srcLen != -1 && srcLen < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (srcLen == 0)
    {
        char* txt = static_cast<char*>(malloc(1));
        if (txt == NULL)
            SetLastError(ERROR_OUTOFMEMORY);
        else
            txt[0] = 0;
        return txt;
    }

    size_t length = (srcLen == -1) ? wcslen(src) : static_cast<size_t>(srcLen);
    SalWideString view(std::wstring_view(src, length));
    if (!view)
        return NULL;

    std::string converted = view.ToAnsi(compositeCheck, codepage);
    if (converted.empty() && view.length() > 0)
        return NULL;

    size_t bytes = converted.size() + 1;
    char* txt = static_cast<char*>(malloc(bytes));
    if (txt == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return NULL;
    }
    if (!converted.empty())
        memcpy(txt, converted.data(), converted.size());
    txt[converted.size()] = 0;
    return txt;
}

int ConvertA2U(const char* src, int srcLen, WCHAR* buf, int bufSizeInChars, UINT codepage)
{
    if (buf == NULL || bufSizeInChars <= 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    buf[0] = 0;
    if (src == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (srcLen != -1 && srcLen < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (srcLen == 0)
        return 1;

    SalWideString wide = SalWideString::FromAnsi(src, srcLen, codepage);
    if (!wide)
        return 0;

    size_t required = wide.length() + 1;
    if (required > static_cast<size_t>(bufSizeInChars))
    {
        size_t copyCount = static_cast<size_t>(bufSizeInChars - 1);
        if (copyCount > 0)
            memcpy(buf, wide.c_str(), copyCount * sizeof(WCHAR));
        buf[bufSizeInChars - 1] = 0;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    memcpy(buf, wide.c_str(), required * sizeof(WCHAR));
    return static_cast<int>(required);
}

WCHAR* ConvertAllocA2U(const char* src, int srcLen, UINT codepage)
{
    if (src == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (srcLen != -1 && srcLen < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    if (srcLen == 0)
    {
        WCHAR* txt = static_cast<WCHAR*>(malloc(sizeof(WCHAR)));
        if (txt == NULL)
            SetLastError(ERROR_OUTOFMEMORY);
        else
            txt[0] = 0;
        return txt;
    }

    SalWideString wide = SalWideString::FromAnsi(src, srcLen, codepage);
    if (!wide)
        return NULL;

    return wide.release();
}

WCHAR* DupStr(const WCHAR* txt)
{
    if (txt == NULL)
        return NULL;
    int len = lstrlenW(txt) + 1;
    WCHAR* ret = (WCHAR*)malloc(len * sizeof(WCHAR));
#ifndef SAFE_ALLOC
    if (ret == NULL)
        return NULL;
#endif // SAFE_ALLOC
    return (WCHAR*)memcpy(ret, txt, len * sizeof(WCHAR));
}

/*
LPTSTR FindString( // Return value: pointer to matched substring of text, or null pointer
                  LCID Locale, // locale identifier for CompareString
                  DWORD dwCmpFlags, // CompareString flags
                  LPCTSTR text, // text to be searched
                  int ltext, // number of TCHARs in text, or negative number if null terminated
                  LPCTSTR str, // string to look for
                  int lstr, // number of TCHARs in string, or negative number if null terminated
                  int *lsubstr // pointer to int that receives the number of TCHARs in the substring
                  )
{
  int i, j; // start/end index of substring being analyzed

  i = 0;
  // for i=0..end, compare text.SubString(i, end) with str
  do {
    switch (CompareString(Locale, dwCmpFlags, &text[i],
                          ltext < 0 ? ltext : ltext-i, str, lstr))
    {
      case 0:
        return 0;

      case CSTR_LESS_THAN:
        continue;
    }
    j = i;
    // if greater: for j=i..end, compare text.SubString(i, j) with str
    do {
      switch (CompareString(Locale, dwCmpFlags, &text[i],
                            j-i, str, lstr))
      {
        case 0:
          return 0;

        case CSTR_LESS_THAN:
          continue;

        case CSTR_EQUAL:
          if (lsubstr) *lsubstr = j-i;
          return (LPTSTR)&text[i];
      }
      // if greater: break to outer loop
      break;
    } while (ltext < 0 ? text[j++] : j++ < ltext);
  } while (ltext < 0 ? text[i++] : i++ < ltext);

  SetLastError(ERROR_SUCCESS);
  return 0;
} 



  int res;
  char buf[10];
  WCHAR *s1, *s2, *s3 = L"D:\\Á";
  res = ConvertU2A(s1 = L"", -1, buf, 10, TRUE);
  res = ConvertU2A(s1 = L"ahoj", 0, buf, 10, TRUE);
  res = ConvertU2A(s1 = L"ahoj", -1, buf, 5, TRUE);
  res = ConvertU2A(s1 = L"ahoj", -1, buf, 4, TRUE);
  res = ConvertU2A(s1 = L"ahoj", 4, buf, 5, TRUE);
  res = ConvertU2A(s1 = L"ahoj", 4, buf, 4, TRUE);
  res = ConvertU2A(s1 = L"ahoj", 4, buf, 3, TRUE);
  res = ConvertU2A(s1 = L"D:\\\x0061\x0308", -1, buf, 10, TRUE);  // L"D:\\\x00e4"
  res = ConvertU2A(s2 = L"D:\\á", -1, buf, 4);
  res = ConvertU2A(s1 = L"D:\\\xfb01-\x0061\x0308-\x00e4.txt", -1, buf, 10, TRUE);
  res = ConvertU2A(s2 = L"fi", -1, buf, 4);

  int fLen;
  WCHAR *f = FindString(LOCALE_USER_DEFAULT, 0, s1, -1, L"fi", -1, &fLen);
  f = FindString(LOCALE_USER_DEFAULT, 0, s1, -1, L"f", -1, &fLen);
  f = FindString(LOCALE_USER_DEFAULT, 0, s1, -1, L"\x00e4", -1, &fLen);
  f = FindString(LOCALE_USER_DEFAULT, 0, s1, -1, L"a", -1, &fLen);  // NEFUNGUJE !!!
  WCHAR *ss = wcsstr(s1, L"\x00e4");
/ *  
procist X:\ZUMPA\!\unicode\ch05.pdf - jak vubec ma vypadat to hledani v Unicode ???

Nekam odswapnout + casem proverit + odladit:
Some time ago I implemented a FindString function which in most cases takes only O(n) time (around 1.5*n CompareString
calls most of which return immediately). In the end I didn't use it because I wasn't sure whether the relevant statement
in the CompareString documentation can be relied on in a strict sense: "If the two strings are of different lengths,
they are compared up to the length of the shortest one. If they are equal to that point, then the return value will
indicate that the longer string is greater." More specifically, the function fails for TCHAR strings that are lexically
before any of their substrings (from the beginning). For example, when looking for "á" = {U+00E1}, the function will not
find the "a?" = {U+0061, U+0301} representation, if it sorts before "a" = {U+0061} in the specified locale. In other
words: The function assumes that CompareString(lcid, flags, string, m, string, n never returns CSTR_GREATER_THAN if
m <= n and the strings agree in the first m TCHARs.

Mozna by se hodilo pouzit "StringInfo Class", ktery umi rozebrat retezec
na zobrazitelne znaky (sekvence WCHARu odpovidajici jednomu zobrazenemu znaku).

* /

  WCHAR wbuf[50];
  WCHAR wbuf2[50];
  res = FoldString(MAP_FOLDCZONE | MAP_EXPAND_LIGATURES, s1, -1, wbuf, 50);
  res = FoldString(MAP_FOLDCZONE | MAP_PRECOMPOSED, wbuf, -1, wbuf2, 50);

  HANDLE file = CreateFile(s1, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file != INVALID_HANDLE_VALUE) CloseHandle(file);

  res = CompareString(LOCALE_USER_DEFAULT, 0, s1, -1, s2, -1);

  res = ConvertA2U("Âëŕäčěčđ", -1, wbuf, 10, 1251);
  res = ConvertA2U("ahoj", 0, wbuf, 10);
  res = ConvertA2U("ahoj", -1, wbuf, 5);
  res = ConvertA2U("ahoj", -1, wbuf, 4);
  res = ConvertA2U("ahoj", 4, wbuf, 5);
  res = ConvertA2U("ahoj", 4, wbuf, 4);
  res = ConvertA2U("ahoj", 4, wbuf, 3);


  res = CompareString(LOCALE_USER_DEFAULT, 0, s2, -1, s3, -1);

  {
    char *res;
    res = ConvertU2A(L"", -1, TRUE);
    res = ConvertU2A(L"ahoj", 0, TRUE);
    res = ConvertU2A(L"ahoj", 2, TRUE);
    res = ConvertU2A(L"ahoj", -1, TRUE);
    res = ConvertU2A(L"D:\\\x0061\x0308", -1, TRUE);
    res = ConvertU2A(L"D:\\\x0061\x0308", -1);
    res = ConvertU2A(L"D:\\\xfb01-\x0061\x0308-\x00e4.txt", -1, TRUE);

    WCHAR *wres;
    wres = ConvertA2U("Âëŕäčěčđ", -1, 1251);
    wres = ConvertA2U("", -1);
    wres = ConvertA2U("ahoj čěšťíňká", 0);
    wres = ConvertA2U("ahoj čěšťíňká", 2);
    wres = ConvertA2U("ahoj čěšťíňká", -1);
  }
*/
