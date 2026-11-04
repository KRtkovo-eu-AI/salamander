// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../filenamecase.h"
#include "../panel_quick_search.h"
#include "../quick_search_match.h"
#include <clocale>

#include <stdio.h>
#include <string>

static BYTE LowerCaseTable[256];
static BYTE UpperCaseTable[256];
static int Failures = 0;

#define CHECK_EQUAL(expected, actual)                                      \
    do                                                                     \
    {                                                                      \
        if ((expected) != (actual))                                        \
        {                                                                  \
            fprintf(stderr, "%s(%d): check failed: %s == %s\n",           \
                    __FILE__, __LINE__, #expected, #actual);               \
            ++Failures;                                                    \
        }                                                                  \
    } while (0)

static void InitializeCaseTables()
{
    for (int i = 0; i < 256; i++)
        LowerCaseTable[i] = UpperCaseTable[i] = (BYTE)i;
    for (int i = 'A'; i <= 'Z'; i++)
        LowerCaseTable[i] = (BYTE)(i - 'A' + 'a');
    for (int i = 'a'; i <= 'z'; i++)
        UpperCaseTable[i] = (BYTE)(i - 'a' + 'A');
}

static std::string ChangeCase(const std::string& input, BOOL upper)
{
    std::string source = input;
    std::string result(input.size() + 1, '\0');
    char* sourcePos = &source[0];
    char* targetPos = &result[0];
    while (*sourcePos != 0)
    {
        CopyFileNameCharWithCase(targetPos, sourcePos, upper, TRUE,
                                 LowerCaseTable, UpperCaseTable);
    }
    *targetPos = 0;
    result.resize((size_t)(targetPos - &result[0]));
    return result;
}

static void TestCzechUtf8CaseConversion()
{
    CHECK_EQUAL(std::string(u8"ÁÁÁ"), ChangeCase(u8"ááá", TRUE));
    CHECK_EQUAL(std::string(u8"ÁÁÁ"), ChangeCase(u8"ááá", TRUE));
    CHECK_EQUAL(std::string(u8"ŽLUŤOUČKÝ KŮŇ"),
                ChangeCase(u8"žluťoučký kůň", TRUE));
    CHECK_EQUAL(std::string(u8"žluťoučký kůň"),
                ChangeCase(u8"ŽLUŤOUČKÝ KŮŇ", FALSE));

    std::string longLower;
    std::string longUpper;
    for (int i = 0; i < 100; i++)
    {
        longLower += u8"ž";
        longUpper += u8"Ž";
    }
    CHECK_EQUAL(longUpper, ChangeCase(longLower, TRUE));
}

static void TestQuickSearchCaretTextRange()
{
    using Salamander::Panel::GetQuickSearchCaretTextRange;

    Salamander::Panel::QuickSearchCaretTextRange range =
        GetQuickSearchCaretTextRange(L"cap.cap", 3, true);
    CHECK_EQUAL((size_t)0, range.Start);
    CHECK_EQUAL((size_t)3, range.Length);

    range = GetQuickSearchCaretTextRange(L"cap.cap", 4, true);
    CHECK_EQUAL((size_t)4, range.Start);
    CHECK_EQUAL((size_t)0, range.Length);

    range = GetQuickSearchCaretTextRange(L"cap.cap", 7, true);
    CHECK_EQUAL((size_t)4, range.Start);
    CHECK_EQUAL((size_t)3, range.Length);

    range = GetQuickSearchCaretTextRange(L"archive.tar.gz", 14, true);
    CHECK_EQUAL((size_t)12, range.Start);
    CHECK_EQUAL((size_t)2, range.Length);

    range = GetQuickSearchCaretTextRange(L"žluťoučký.kůň", 13, true);
    CHECK_EQUAL((size_t)10, range.Start);
    CHECK_EQUAL((size_t)3, range.Length);

    range = GetQuickSearchCaretTextRange(L".htaccess", 9, false);
    CHECK_EQUAL((size_t)0, range.Start);
    CHECK_EQUAL((size_t)9, range.Length);
}


static void CheckQuickSearchMatch(const wchar_t* name, const wchar_t* mask,
                                 bool hasExtension, bool wholeString, int expectedOffset)
{
    int offset = -1;
    bool matched = Salamander::Panel::MatchQuickSearchWide(name, hasExtension, mask, wholeString, offset);
    CHECK_EQUAL(expectedOffset >= 0, matched);
    CHECK_EQUAL(expectedOffset >= 0 ? expectedOffset : 0, offset);
}

static void TestUnicodeQuickSearchMatching()
{
    // The application does not select a CRT character locale. Matching must not
    // depend on towlower's ASCII-only behavior in the default C locale.
    std::setlocale(LC_CTYPE, "C");
    CheckQuickSearchMatch(L"\u0160\u00cd\u0158KA.txt", L"\u0161\u00ed\u0159", true, false, 3);
    CheckQuickSearchMatch(L"\u017elut\u00fd.txt", L"\u017dLUT\u00dd", true, false, 5);
    CheckQuickSearchMatch(L"\u0394\u039f\u039a.txt", L"\u03b4\u03bf\u03ba", true, false, 3);
    CheckQuickSearchMatch(L"\u041f\u0420\u0418.txt", L"\u043f\u0440\u0438", true, false, 3);
    CheckQuickSearchMatch(L"e\u0301clair.txt", L"\u00c9", true, false, 2);
    CheckQuickSearchMatch(L"\u00c9clair.txt", L"e\u0301", true, false, 1);
    CheckQuickSearchMatch(L"e\u0301cole.txt", L"\u00e9c", true, false, 3);
    CheckQuickSearchMatch(L"a\u0315\u0300.txt", L"\u00e0", true, false, 3);
    CheckQuickSearchMatch(L"\u1100\u1161.txt", L"\uac00", true, false, 2);
    CheckQuickSearchMatch(L"\U0001f600\u0160.txt", L"\U0001f600\u0161", true, false, 3);
    CheckQuickSearchMatch(L"before\U0001f600\u0160.txt", L"/\U0001f600\u0161", true, false, 9);
    CheckQuickSearchMatch(L"e\u0301.txt", L"/\u00c9.txt", true, true, 6);

    CheckQuickSearchMatch(L"plain.txt", L"PLAIN", true, false, 5);
    CheckQuickSearchMatch(L"plain.txt", L"plain", true, true, -1);
    CheckQuickSearchMatch(L"flag", L"/g.", false, false, 4);
    CheckQuickSearchMatch(L"flag.txt", L"/g.", true, false, 5);
    CheckQuickSearchMatch(L"pic.jpg", L"/g.", true, false, -1);
    CheckQuickSearchMatch(L"plain", L"plain.", false, true, 5);
    CheckQuickSearchMatch(L"a*b", L"a*b", false, true, 3);
    CheckQuickSearchMatch(L"axb", L"a*b", false, false, -1);
    CheckQuickSearchMatch(L"", L"", false, true, 0);
    CheckQuickSearchMatch(L"name", L"", false, false, 0);
    CheckQuickSearchMatch(NULL, L"", false, false, -1);
    CheckQuickSearchMatch(L"name", NULL, false, false, -1);
    std::wstring longName(200, L'\u0160');
    longName += L".txt";
    CheckQuickSearchMatch(longName.c_str(), std::wstring(200, L'\u0161').c_str(), true, false, 200);
}

int main()
{
    InitializeCaseTables();
    TestCzechUtf8CaseConversion();
    TestQuickSearchCaretTextRange();
    TestUnicodeQuickSearchMatching();
    if (Failures == 0)
        printf("All filename case tests passed.\n");
    return Failures == 0 ? 0 : 1;
}
