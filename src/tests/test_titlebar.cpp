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
    if (suffix.length() > title.length()) return false;
    return title.compare(title.length() - suffix.length(), suffix.length(), suffix) == 0;
}

static std::wstring BuildTitleForTest(const std::wstring& pathText,
                                      const std::wstring& prefix,
                                      const std::wstring& appSuffix,
                                      bool showPath = true,
                                      int maxPrefixVW = 220,
                                      int maxUnicodePrefixVW = 24)
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
        CHECK(title == L"\u00E1\u00E1\u00E1 - " + suffix, "short Latin-extended path is not compacted");
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

int main()
{
    printf("=== Salamander Title Bar Unit Tests ===\n");
    Test_VisualWidth();
    Test_TitleAssemblyScenarios();
    Test_UnicodeCompactionPreservesSuffix();
    printf("\n=== Results: %d passed, %d failed ===\n", gPassed, gFailed);
    return gFailed > 0 ? 1 : 0;
}
