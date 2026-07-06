#include <stdio.h>
#include <string>

#include "../titlebar_builder.h"

static int gPassed = 0;
static int gFailed = 0;

#define CHECK(cond, msg)             \
    do                               \
    {                                \
        if (cond)                    \
            gPassed++;               \
        else                         \
        {                            \
            gFailed++;               \
            printf("FAIL: %s\n", msg); \
        }                            \
    } while (0)

static bool EndsWith(const std::wstring& value, const std::wstring& suffix)
{
    return value.length() >= suffix.length() &&
           value.compare(value.length() - suffix.length(), suffix.length(), suffix) == 0;
}

int main()
{
    const std::wstring suffix = L"Open Salamander 5.0 Samandarin 0.9 (x64)";
    const std::wstring adminSuffix = L"Open Salamander 5.0 Samandarin 0.9 (x64) (Administrator)";

    CHECK(BuildMainWindowTitleText(L"", L"C:\\temp\\test", suffix) == L"C:\\temp\\test - " + suffix,
          "full path title keeps suffix");
    CHECK(BuildMainWindowTitleText(L"", L"Windows Photo Viewer", suffix) == L"Windows Photo Viewer - " + suffix,
          "directory-only ASCII title keeps suffix");
    CHECK(BuildMainWindowTitleText(L"ADMIN", L"Windows Photo Viewer", adminSuffix) == L"ADMIN - Windows Photo Viewer - " + adminSuffix,
          "prefix and Administrator suffix are preserved");
    CHECK(BuildMainWindowTitleText(L"", L"", suffix) == suffix,
          "empty path uses suffix only");

    std::wstring supplementaryPath = L"C:\\temp\\test\\";
    for (int i = 0; i < 25; i++)
        supplementaryPath += L"\U00024B62";
    CHECK(EndsWith(BuildMainWindowTitleText(L"", supplementaryPath, suffix), suffix),
          "supplementary-plane path keeps full suffix in title string");

    CHECK(EndsWith(BuildMainWindowTitleText(L"", L"\u65E5\u672C\u8A9E \u3053\u306E\u30C7\u30A3\u30EC\u30AF\u30C8\u30EA\u306F\u3068\u3066\u3082\u9577\u3044\u540D\u524D\u3067\u3059", suffix), suffix),
          "Japanese path keeps full suffix in title string");

    printf("Titlebar tests: %d passed, %d failed\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}
