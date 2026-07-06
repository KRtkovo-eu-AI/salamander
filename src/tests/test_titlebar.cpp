#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string>

#include "../titlebar_helpers.h"

static int gPassed = 0;
static int gFailed = 0;

#define CHECK(cond, msg) do { \
    if (cond) { gPassed++; } \
    else { gFailed++; printf("  FAIL: %s\n", msg); } \
} while(0)

#define SECTION(name) printf("\n--- %s ---\n", name)

static bool EndsWide(const std::wstring& title, const std::wstring& suffix)
{
    std::wstring visible = title;
    while (!visible.empty() && visible.back() == L' ')
        visible.pop_back();
    if (suffix.length() > visible.length()) return false;
    return visible.compare(visible.length() - suffix.length(), suffix.length(), suffix) == 0;
}

static std::wstring BuildTitleForTest(const std::wstring& pathText,
                                      const std::wstring& prefix,
                                      const std::wstring& appSuffix,
                                      bool showPath = true,
                                      int maxPrefixVW = 80,
                                      int maxUnicodePrefixVW = 12)
{
    std::wstring title;
    if (!prefix.empty())
        title += prefix + L" - ";
    if (showPath && !pathText.empty())
        title += pathText + L" - ";
    title += appSuffix;
    EnsureAppNameSuffixInTitle(title, appSuffix, maxPrefixVW, maxUnicodePrefixVW);
    return title;
}

static void Test_VisualWidth()
{
    SECTION("Visual width helpers");
    CHECK(WCharVisualWidth(L'a') == 1, "ASCII = 1");
    CHECK(WCharVisualWidth(L'\u00E1') == 1, "Latin accent = 1");
    CHECK(WCharVisualWidth(L'\u65E5') == 3, "CJK = 3");
    CHECK(WCharVisualWidth(0xD852) == 2, "surrogate code unit is conservative");
    CHECK(StringVisualWidth(L"Open Salamander 5.0 Samandarin 0.9 (x64)") == 40, "suffix width");
    CHECK(StringHasNonAscii(L"aaa") == false, "ASCII detection");
    CHECK(StringHasNonAscii(L"\u00E1\u00E1\u00E1") == true, "Unicode detection");
}

static void Test_TitleAssemblyScenarios()
{
    SECTION("Title assembly scenarios");
    const std::wstring suffix = L"Open Salamander 5.0 Samandarin 0.9 (x64)";
    const std::wstring adminSuffix = L"Open Salamander 5.0 Samandarin 0.9 (x64) (Administrator)";

    {
        std::wstring title = BuildTitleForTest(L"Program Files (x86)", L"", suffix);
        CHECK(title == L"Program Files (x86) - " + suffix, "directory mode ASCII is not compacted");
    }
    {
        std::wstring title = BuildTitleForTest(L"C:\\Program Files (x86)", L"", suffix);
        CHECK(title == L"C:\\Program Files (x86) - " + suffix, "full path ASCII is not compacted");
    }
    {
        std::wstring title = BuildTitleForTest(L"C:\\...\\Program Files (x86)", L"", suffix);
        CHECK(title == L"C:\\...\\Program Files (x86) - " + suffix, "shortened path ASCII is not compacted");
    }
    {
        std::wstring title = BuildTitleForTest(L"Program Files (x86)", L"ADMIN", adminSuffix);
        CHECK(title == L"ADMIN - Program Files (x86) - " + adminSuffix, "prefix and Administrator suffix preserved");
    }
    {
        std::wstring title = BuildTitleForTest(L"Program Files (x86)", L"", suffix, false);
        CHECK(title == suffix, "hidden path uses only suffix");
    }
}

static void Test_UnicodeCompactionPreservesSuffix()
{
    SECTION("Unicode compaction preserves app suffix");
    const std::wstring suffix = L"Open Salamander 5.0 Samandarin 0.9 (x64)";

    {
        std::wstring title = BuildTitleForTest(L"\u00E1\u00E1\u00E1", L"", suffix);
        CHECK(EndsWide(title, suffix), "short Latin-extended suffix preserved");
    }
    {
        std::wstring title = BuildTitleForTest(L"\u65E5\u672C\u8A9E \u65E5\u672C\u8A9E \u65E5\u672C\u8A9E", L"", suffix);
        CHECK(EndsWide(title, suffix), "Japanese suffix preserved");
        CHECK(title.find(L"...") != std::wstring::npos, "Japanese prefix compacted before app is clipped");
    }
    {
        std::wstring path = L"C:\\temp\\test\\";
        for (int i = 0; i < 25; i++)
            path += L"\U00024B62";
        path += L"\\\u00E1\u00E1\u00E1";
        std::wstring title = BuildTitleForTest(path, L"", suffix);
        CHECK(EndsWide(title, suffix), "supplementary-plane path suffix preserved");
        CHECK(title.find(L"...") != std::wstring::npos, "supplementary-plane path compacted");
    }
    {
        std::wstring title = BuildTitleForTest(std::wstring(240, L'a'), L"", suffix);
        CHECK(EndsWide(title, suffix), "very long ASCII suffix preserved");
        CHECK(title.find(L"...") != std::wstring::npos, "very long ASCII compacted only past large budget");
    }
}


static void Test_ReportedRealScenarios()
{
    SECTION("Reported real scenarios keep full suffix");
    const std::wstring suffix = L"Open Salamander 5.0 Samandarin 0.9 (x64)";

    std::wstring twentyFiveSupplementary;
    for (int i = 0; i < 25; i++)
        twentyFiveSupplementary += L"\U00024B62";

    {
        std::wstring title = BuildTitleForTest(L"C:\\temp\\test\\" + twentyFiveSupplementary, L"", suffix);
        CHECK(EndsWide(title, suffix), "full path with supplementary chars keeps full suffix");
    }
    {
        std::wstring title = BuildTitleForTest(twentyFiveSupplementary, L"", suffix);
        CHECK(EndsWide(title, suffix), "directory-only supplementary chars keep full suffix");
    }
    {
        std::wstring title = BuildTitleForTest(twentyFiveSupplementary + L"\\u00E1\u00E1\u00E1", L"", suffix);
        CHECK(EndsWide(title, suffix), "reported directory-only supplementary-plus-á title keeps full suffix");
    }
    {
        std::wstring title = BuildTitleForTest(L"C:\\temp\\test\\" + twentyFiveSupplementary + L"\\\u00E1\u00E1\u00E1", L"", suffix);
        CHECK(EndsWide(title, suffix), "full path ending with ááá keeps full suffix");
    }
    {
        std::wstring title = BuildTitleForTest(L"\u00E1\u00E1\u00E1", L"", suffix);
        CHECK(EndsWide(title, suffix), "directory-only ááá keeps full suffix");
    }
    {
        std::wstring title = BuildTitleForTest(L"C:\\Program Files (x86)\\Windows Photo Viewer", L"", suffix);
        CHECK(title == L"C:\\Program Files (x86)\\Windows Photo Viewer - " + suffix, "ASCII full path remains unchanged");
    }
    {
        std::wstring title = BuildTitleForTest(L"Windows Photo Viewer", L"", suffix);
        CHECK(title == L"Windows Photo Viewer - " + suffix, "ASCII directory-only remains unchanged");
    }
    {
        std::wstring title = BuildTitleForTest(L"\u65E5\u672C\u8A9E \u3053\u306E\u30C7\u30A3\u30EC\u30AF\u30C8\u30EA\u306F\u3068\u3066\u3082\u9577\u3044\u540D\u524D\u3067\u3059\U00024B62\U00024B62\U00024B62", L"", suffix);
        CHECK(EndsWide(title, suffix), "long Japanese directory keeps full suffix");
    }
    {
        std::wstring title = BuildTitleForTest(L"\u65E5\u672C\u8A9E \u3053\u306E\u30C7\u30A3\u30EC\u30AF\u30C8\u30EA\u306F\u3068\u3066\u3082\u9577\u3044\u540D\u524D\u3067\u3059\U00024B62\U00024B62\U00024B62", L"", suffix);
        CHECK(title.find(L"\u65E5\u672C") == 0, "long Japanese directory keeps multiple path characters");
    }
    {
        std::wstring longAscii(220, L'a');
        std::wstring title = BuildTitleForTest(longAscii, L"", suffix);
        CHECK(EndsWide(title, suffix), "very long ASCII directory keeps full suffix");
        CHECK(title.find(L"...") != std::wstring::npos, "very long ASCII directory is compacted");
    }
    {
        std::wstring longZ(120, L'\u017E');
        std::wstring title = BuildTitleForTest(longZ, L"", suffix);
        CHECK(EndsWide(title, suffix), "long ž directory keeps full suffix including closing parenthesis");
        CHECK(title.rfind(std::wstring(6, L'\u017E'), 0) == 0, "long ž directory keeps multiple path characters");
    }
}

int main()
{
    printf("=== Salamander Title Bar Unit Tests ===\n");
    Test_VisualWidth();
    Test_TitleAssemblyScenarios();
    Test_UnicodeCompactionPreservesSuffix();
    Test_ReportedRealScenarios();
    printf("\n=== Results: %d passed, %d failed ===\n", gPassed, gFailed);
    return gFailed > 0 ? 1 : 0;
}
