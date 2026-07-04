#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string>

// Include the REAL functions from the application
#include "../titlebar_helpers.h"

// === Test infrastructure ===

static int gPassed = 0;
static int gFailed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { gPassed++; } \
    else { gFailed++; printf("  FAIL: %s\n", msg); } \
} while(0)

#define SECTION(name) printf("\n--- %s ---\n", name)

static bool EndsWide(const std::wstring& title, const std::wstring& suffix)
{
    if (suffix.length() > title.length()) return false;
    return title.compare(title.length() - suffix.length(), suffix.length(), suffix) == 0;
}

// === Tests ===

void Test_WCharVisualWidth()
{
    SECTION("WCharVisualWidth");

    // ASCII
    CHECK(WCharVisualWidth(L'A') == 1, "A = 1");
    CHECK(WCharVisualWidth(L'z') == 1, "z = 1");
    CHECK(WCharVisualWidth(L'0') == 1, "0 = 1");
    CHECK(WCharVisualWidth(L' ') == 1, "space = 1");
    CHECK(WCharVisualWidth(L'\\') == 1, "backslash = 1");
    CHECK(WCharVisualWidth(0x7F) == 1, "DEL = 1");

    // Latin extended (same width as ASCII in title bar)
    CHECK(WCharVisualWidth(L'\u00E1') == 1, "a-acute = 1");
    CHECK(WCharVisualWidth(L'\u00E9') == 1, "e-acute = 1");
    CHECK(WCharVisualWidth(L'\u010D') == 1, "c-caron = 1");
    CHECK(WCharVisualWidth(L'\u017E') == 1, "z-caron = 1");
    CHECK(WCharVisualWidth(L'\u00FC') == 1, "u-umlaut = 1");

    // Fullwidth ASCII forms
    CHECK(WCharVisualWidth(0xFF21) == 2, "fullwidth A = 2");
    CHECK(WCharVisualWidth(0xFF41) == 2, "fullwidth a = 2");
    CHECK(WCharVisualWidth(0xFF01) == 2, "fullwidth ! = 2");
    CHECK(WCharVisualWidth(0xFF5E) == 2, "fullwidth ~ = 2");

    // CJK
    CHECK(WCharVisualWidth(L'\u65E5') == 3, "日 = 3");
    CHECK(WCharVisualWidth(L'\u672C') == 3, "本 = 3");
    CHECK(WCharVisualWidth(L'\u8A9E') == 3, "語 = 3");
    CHECK(WCharVisualWidth(L'\u74F6') == 3, "瓶 = 3");
    CHECK(WCharVisualWidth(L'\u9577') == 3, "長 = 3");

    // CJK boundary ranges
    CHECK(WCharVisualWidth(0x2E80) == 3, "CJK Radical start = 3");
    CHECK(WCharVisualWidth(0x9FFF) == 3, "CJK Ideograph end = 3");
    CHECK(WCharVisualWidth(0xF900) == 3, "CJK Compat Ideograph = 3");
    CHECK(WCharVisualWidth(0xFE30) == 3, "CJK Compat Form = 3");

    // Halfwidth katakana (NOT in CJK range)
    CHECK(WCharVisualWidth(0xFF65) == 1, "halfwidth katakana = 1");

    printf("  Summary: ASCII=1, Latin=1, Fullwidth=2, CJK=3\n");
}

void Test_StringVisualWidth()
{
    SECTION("StringVisualWidth");

    CHECK(StringVisualWidth(L"") == 0, "empty = 0");
    CHECK(StringVisualWidth(L"abc") == 3, "abc = 3");
    CHECK(StringVisualWidth(L"Open Salamander 5.0 Samandarin 0.9 (x64)") == 40, "full suffix = 40");
    CHECK(StringVisualWidth(L" - ") == 3, "separator = 3");
    CHECK(StringVisualWidth(L"\u00E1\u00E1\u00E1") == 3, "3x á = 3");
    CHECK(StringVisualWidth(L"\u65E5\u672C\u8A9E") == 9, "日本語 = 9");
    CHECK(StringVisualWidth(L"C:\\\u74F6\u74F6") == 9, "C:\\瓶瓶 = 9");
    CHECK(StringVisualWidth(L"\u74F6\u74F6\u74F6...") == 12, "瓶瓶瓶... = 12");
}

void Test_TotalVWCalculation()
{
    SECTION("VW budget calculation");

    int suffixVW = StringVisualWidth(L"Open Salamander 5.0 Samandarin 0.9 (x64)");
    int sepVW = StringVisualWidth(L" - ");
    int ellipsisVW = StringVisualWidth(L"...");
    int totalLimit = 58;
    int maxPrefixVW = totalLimit - suffixVW - sepVW - ellipsisVW;

    printf("    Suffix VW:      %d\n", suffixVW);
    printf("    Separator VW:   %d\n", sepVW);
    printf("    Ellipsis VW:    %d\n", ellipsisVW);
    printf("    Total limit:    %d\n", totalLimit);
    printf("    Max prefix VW:  %d\n", maxPrefixVW);
    printf("    Max ASCII chars: %d\n", maxPrefixVW);
    printf("    Max CJK chars:  %d (at 3vw each)\n", maxPrefixVW / 3);

    CHECK(suffixVW == 40, "suffix = 40 VW");
    CHECK(sepVW == 3, "separator = 3 VW");
    CHECK(ellipsisVW == 3, "ellipsis = 3 VW");
    CHECK(maxPrefixVW == 12, "maxPrefixVW = 12");
}

void Test_EnsureAppNameSuffixInTitle()
{
    SECTION("EnsureAppNameSuffixInTitle");

    const std::wstring suffix = L"Open Salamander 5.0 Samandarin 0.9 (x64)";

    // Short ASCII path - no truncation
    {
        std::wstring title = L"C:\\temp\\test - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(EndsWide(title, suffix), "short ASCII: suffix preserved");
        CHECK(title.find(L"C:\\temp\\test") != std::wstring::npos, "short ASCII: path preserved");
        printf("    Result: %ls\n", title.c_str());
    }

    // Long ASCII path - truncation with ...
    {
        std::wstring prefix(80, L'a');
        std::wstring title = prefix + L" - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(EndsWide(title, suffix), "long ASCII: suffix preserved");
        CHECK(title.find(L"...") != std::wstring::npos, "long ASCII: has ...");
        printf("    Result: %ls\n", title.c_str());
    }

    // 10x瓶 - from screenshot 1
    {
        std::wstring prefix;
        for (int i = 0; i < 10; i++) prefix += L'\u74F6';
        std::wstring title = prefix + L" - " + suffix;
        int vwBefore = StringVisualWidth(title);
        EnsureAppNameSuffixInTitle(title, suffix);
        int vwAfter = StringVisualWidth(title);
        printf("    10x瓶 BEFORE: VW=%d  AFTER: VW=%d\n", vwBefore, vwAfter);
        printf("    Result: %ls\n", title.c_str());
        CHECK(EndsWide(title, suffix), "10x瓶: suffix preserved");
        CHECK(vwAfter <= 58, "10x瓶: fits in title bar");
    }

    // Japanese long path - from screenshot 3/4
    {
        std::wstring prefix = L"\u65E5\u672C\u8A9E \u3053\u306E\u30C7\u30A3\u30EC\u30AF\u30C8\u30EA\u306F\u3068\u3066\u3082\u9577\u3044\u540D\u524D\u3067\u3059\u74F6\u74F6\u74F6";
        std::wstring title = prefix + L" - " + suffix;
        int vwBefore = StringVisualWidth(title);
        EnsureAppNameSuffixInTitle(title, suffix);
        int vwAfter = StringVisualWidth(title);
        printf("    Japanese BEFORE: VW=%d  AFTER: VW=%d\n", vwBefore, vwAfter);
        printf("    Result: %ls\n", title.c_str());
        CHECK(EndsWide(title, suffix), "Japanese: suffix preserved");
        CHECK(vwAfter <= 58, "Japanese: fits in title bar");
    }

    // 日本語 日本語 日本語 - from screenshot 5
    {
        std::wstring prefix = L"\u65E5\u672C\u8A9E \u65E5\u672C\u8A9E \u65E5\u672C\u8A9E";
        std::wstring title = prefix + L" - " + suffix;
        int vwBefore = StringVisualWidth(title);
        EnsureAppNameSuffixInTitle(title, suffix);
        int vwAfter = StringVisualWidth(title);
        printf("    3x日本語 BEFORE: VW=%d  AFTER: VW=%d\n", vwBefore, vwAfter);
        printf("    Result: %ls\n", title.c_str());
        CHECK(EndsWide(title, suffix), "3x日本語: suffix preserved");
        CHECK(vwAfter <= 58, "3x日本語: fits in title bar");
    }

    // Mixed CJK + ASCII path
    {
        std::wstring prefix;
        for (int i = 0; i < 15; i++) prefix += L'\u65E5';
        prefix += L"\\test";
        std::wstring title = prefix + L" - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(EndsWide(title, suffix), "mixed: suffix preserved");
        CHECK(StringVisualWidth(title) <= 58, "mixed: fits in title bar");
        printf("    Mixed: VW=%d  Result: %ls\n", StringVisualWidth(title), title.c_str());
    }

    // Edge cases
    {
        std::wstring title = L"";
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(title.empty(), "empty: stays empty");
    }
    {
        std::wstring title = L"short";
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(title == L"short", "shorter than suffix: unchanged");
    }
    {
        std::wstring title = L"something else";
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(title == L"something else", "no suffix match: unchanged");
    }
    {
        std::wstring title = suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(title == suffix, "only suffix: unchanged");
    }

    // Exactly at limit - no truncation
    {
        // maxPrefixVW=12, 12 ASCII = 12 VW
        std::wstring prefix(12, L'a');
        std::wstring title = prefix + L" - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(title == prefix + L" - " + suffix, "at limit: no truncation");
        printf("    At limit (12): %ls\n", title.c_str());
    }

    // 1 over limit - truncation
    {
        std::wstring prefix(13, L'a');
        std::wstring title = prefix + L" - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(EndsWide(title, suffix), "1 over: suffix preserved");
        CHECK(title.find(L"...") != std::wstring::npos, "1 over: has ...");
        printf("    1 over (13): %ls\n", title.c_str());
    }

    // 4 CJK (12VW) - fits
    {
        std::wstring prefix;
        for (int i = 0; i < 4; i++) prefix += L'\u65E5';
        std::wstring title = prefix + L" - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(title == prefix + L" - " + suffix, "4 CJK: no truncation");
        printf("    4 CJK (12VW): %ls\n", title.c_str());
    }

    // 5 CJK (15VW) - over limit, truncated
    {
        std::wstring prefix;
        for (int i = 0; i < 5; i++) prefix += L'\u65E5';
        std::wstring title = prefix + L" - " + suffix;
        EnsureAppNameSuffixInTitle(title, suffix);
        CHECK(EndsWide(title, suffix), "5 CJK: suffix preserved");
        CHECK(title.find(L"...") != std::wstring::npos, "5 CJK: has ...");
        printf("    5 CJK (15VW): %ls\n", title.c_str());
    }
}

int main()
{
    printf("=== Salamander Title Bar Unit Tests ===\n");
    printf("Testing REAL functions from titlebar_helpers.h\n\n");

    Test_WCharVisualWidth();
    Test_StringVisualWidth();
    Test_TotalVWCalculation();
    Test_EnsureAppNameSuffixInTitle();

    printf("\n=== Results: %d passed, %d failed ===\n", gPassed, gFailed);
    return gFailed > 0 ? 1 : 0;
}
