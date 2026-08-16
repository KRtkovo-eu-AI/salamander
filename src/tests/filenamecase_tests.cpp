// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../filenamecase.h"
#include "../panel_quick_search.h"

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

int main()
{
    InitializeCaseTables();
    TestCzechUtf8CaseConversion();
    TestQuickSearchCaretTextRange();
    if (Failures == 0)
        printf("All filename case tests passed.\n");
    return Failures == 0 ? 0 : 1;
}
