// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "../codetbl_utils.h"
#include "../common/winlibdpi.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int Failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << "\n";
        Failures++;
    }
}

void TestAutomaticCodePageSelection()
{
    Check(GetConversionAutoCodePage(1250, 1252) == 1250,
          "A non-UTF-8 active code page must remain authoritative");
    Check(GetConversionAutoCodePage(CP_UTF8, 1250) == 1250,
          "UTF-8 plus a Central European system locale must resolve to CP1250");
    Check(GetConversionAutoCodePage(CP_UTF8, 1251) == 1251,
          "UTF-8 plus a Cyrillic system locale must resolve to CP1251");
    Check(GetConversionAutoCodePage(CP_UTF8, 1252) == 1252,
          "UTF-8 plus a Western European system locale must resolve to CP1252");
    Check(GetConversionAutoCodePage(CP_UTF8, 0) == CP_UTF8,
          "An unavailable system-locale code page must preserve the existing fallback path");
}

void TestIdentifierParsing()
{
    DWORD identifier = 0;
    Check(ParseConversionCodePageIdentifier("  1250\t ", 8, &identifier) && identifier == 1250,
          "The identifier parser must trim leading and trailing whitespace");
    Check(!ParseConversionCodePageIdentifier("1250x", 5, &identifier),
          "The identifier parser must reject trailing non-digits");
    Check(!ParseConversionCodePageIdentifier("   ", 3, &identifier),
          "The identifier parser must reject an empty value");
}

void TestRegionalTextConversion()
{
    const unsigned char kamenictiBytes[] = {'K', 'a', 'm', 'e', 'n', 'i', 0xE8, 't', 0xED, 0};
    const unsigned char koiCs2Bytes[] = {'K', 'O', 'I', '-', '8', ' ', 0xC8, 'S', '2', 0};
    const unsigned char cyrillicBytes[] = {0xCA, 0xE8, 0xF0, 0xE8, 0xEB,
                                           0xEB, 0xE8, 0xF6, 0xE0, 0};
    const char* kamenicti = reinterpret_cast<const char*>(kamenictiBytes);
    const char* koiCs2 = reinterpret_cast<const char*>(koiCs2Bytes);
    const char* cyrillic = reinterpret_cast<const char*>(cyrillicBytes);
    char converted[256];

    Check(ConvertConversionTableText(kamenicti, 1250, CP_UTF8, converted, sizeof(converted)) &&
              std::string(converted) == u8"Kameničtí",
          "CP1250 Kameničtí must become exact UTF-8");
    Check(ConvertConversionTableText(koiCs2, 1250, CP_UTF8, converted, sizeof(converted)) &&
              std::string(converted) == u8"KOI-8 ČS2",
          "CP1250 ČS2 must become exact UTF-8");
    Check(ConvertConversionTableText(cyrillic, 1251, CP_UTF8, converted, sizeof(converted)) &&
              std::string(converted) == u8"Кириллица",
          "Representative CP1251 text must become exact UTF-8");

    char tooSmall[4] = {'x', 'x', 'x', 0};
    Check(!ConvertConversionTableText(kamenicti, 1250, CP_UTF8, tooSmall, sizeof(tooSmall)) &&
              tooSmall[0] == 0,
          "A short destination buffer must fail safely");
    Check(!ConvertConversionTableText(kamenicti, 99999, CP_UTF8, converted, sizeof(converted)) &&
              converted[0] == 0,
          "An unsupported source code page must preserve the loader fallback path");

    const char utf8MenuText[] = u8"Kameničtí - ČS2";
    std::wstring wideMenuText;
    Check(ConvertLegacyViewerTextToWide(utf8MenuText, (int)strlen(utf8MenuText),
                                        CP_UTF8, &wideMenuText) &&
              wideMenuText == L"Kameničtí - ČS2",
          "UTF-8 conversion menu names must reach the Unicode Win32 menu API intact");
}

void TestLegacyViewerTextConversion()
{
    const unsigned char sampleBytes[] = {
        'H', 'e', 'l', 'l', 'o', ' ', 0xEC, 0x9A, 0xE8, 0xF8, 0x9E,
        0xFD, 0xE1, 0xED, 0xE9, 0x9D, 0xF2, 0xFA, 0xF9, ' ', 'H',
        'e', 'l', 'l', 'o'};
    std::wstring converted;
    Check(ConvertLegacyViewerTextToWide(
              reinterpret_cast<const char*>(sampleBytes),
              static_cast<int>(sizeof(sampleBytes)), 1250, &converted) &&
              converted == L"Hello ěščřžýáíéťňúů Hello",
          "The issue #649 CP1250 sample must reach Unicode GDI without losing diacritics");

    Check(ConvertLegacyViewerTextToWide(NULL, 0, 1250, &converted) && converted.empty(),
          "An empty legacy viewer run must convert safely");
    Check(!ConvertLegacyViewerTextToWide("x", 1, 99999, &converted) && converted.empty(),
          "An unsupported viewer code page must fail without stale output");
}

void TestInheritedViewerFontDpiScaling()
{
    LOGFONT font = {};
    font.lfHeight = -20;
    font.lfWidth = 10;
    WinLibDPIScaleLogFontBetweenDPI(&font, 144, 144);
    Check(font.lfHeight == -20 && font.lfWidth == 10,
          "A viewer font already at the monitor DPI must not be scaled twice");

    WinLibDPIScaleLogFontBetweenDPI(&font, 144, 96);
    Check(font.lfHeight == -13 && font.lfWidth == 7,
          "An inherited viewer font must scale relative to its source system DPI");
}

void TestLegacyAutoDetectionPreference()
{
    const unsigned char cp1250Bytes[] = {
        'H', 'e', 'l', 'l', 'o', ' ', 0xEC, 0x9A,
        0xE8, 0xF8, 0x9E, 0xFD,
        0xE1, 0xED, 0xE9, 0x9D,
        0xF2, 0xFA, 0xF9, ' ', 'H', 'e',
        'l', 'l', 'o'};
    const char* cp1250 = reinterpret_cast<const char*>(cp1250Bytes);
    Check(ShouldPreferWindowsCodePageText(
              cp1250, (int)sizeof(cp1250Bytes), 1250, "ISO-8859-1"),
          "CP1250 letters in the ISO C1 range must prevent a false ISO-8859-1 conversion");
    Check(!ShouldPreferWindowsCodePageText(
              "plain ASCII", 11, 1250, "ISO-8859-1"),
          "ASCII alone must not override automatic legacy-code-page recognition");
    Check(!ShouldPreferWindowsCodePageText(
              cp1250, (int)sizeof(cp1250Bytes), 1250, "CP852"),
          "The preference is specific to ISO code pages whose C1 range contains controls");
}

bool ReadIdentifier(const std::filesystem::path& cfgPath, DWORD& identifier)
{
    std::ifstream stream(cfgPath, std::ios::binary);
    std::string line;
    const std::string prefix = "WINDOWS_CODE_PAGE_IDENTIFIER=";
    while (std::getline(stream, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.compare(0, prefix.size(), prefix) == 0)
        {
            return ParseConversionCodePageIdentifier(line.data() + prefix.size(),
                                                     line.size() - prefix.size(), &identifier) != FALSE;
        }
    }
    return false;
}

void TestShippedConversionDirectories()
{
    struct ExpectedDirectory
    {
        const char* Name;
        DWORD Identifier;
    };
    const ExpectedDirectory directories[] = {
        {"centeuro", 1250},
        {"cyrillic", 1251},
        {"westeuro", 1252},
    };

    for (const ExpectedDirectory& expected : directories)
    {
        std::filesystem::path directory = std::filesystem::path("convert") / expected.Name;
        DWORD identifier = 0;
        Check(ReadIdentifier(directory / "convert.cfg", identifier) && identifier == expected.Identifier,
              (std::string("Unexpected convert.cfg identifier in ") + expected.Name).c_str());

        size_t tableCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".tab")
            {
                tableCount++;
                Check(entry.file_size() == 256,
                      (std::string("Conversion table is not 256 bytes: ") +
                       entry.path().string()).c_str());
            }
        }
        Check(tableCount > 0,
              (std::string("No conversion tables found in ") + expected.Name).c_str());
    }

    std::ifstream centralEuropeanConfig(
        std::filesystem::path("convert") / "centeuro" / "convert.cfg",
        std::ios::binary);
    std::string configBytes((std::istreambuf_iterator<char>(centralEuropeanConfig)),
                            std::istreambuf_iterator<char>());
    const unsigned char rawKamenicti[] = {
        'K', 'a', 'm', 'e', 'n', 'i', 0xE8, 't', 0xED, ' ', '-', ' ', 'C', 'P', '1', '2', '5', '0'};
    const std::string rawName(reinterpret_cast<const char*>(rawKamenicti), sizeof(rawKamenicti));
    const size_t rawNameOffset = configBytes.find(rawName);
    std::wstring menuName;
    Check(rawNameOffset != std::string::npos &&
              ConvertLegacyViewerTextToWide(configBytes.data() + rawNameOffset,
                                            (int)rawName.size(), 1250, &menuName) &&
              menuName == L"Kameničtí - CP1250",
          "The shipped CP1250 menu name must be decoded directly to exact UTF-16");

    const unsigned char rawKoiCs2[] = {
        'K', 'O', 'I', '-', '8', ' ', 0xC8, 'S', '2', ' ', '-', ' ', 'C', 'P', '1', '2', '5', '0'};
    const std::string rawKoiName(reinterpret_cast<const char*>(rawKoiCs2), sizeof(rawKoiCs2));
    const size_t rawKoiOffset = configBytes.find(rawKoiName);
    Check(rawKoiOffset != std::string::npos &&
              ConvertLegacyViewerTextToWide(configBytes.data() + rawKoiOffset,
                                            (int)rawKoiName.size(), 1250, &menuName) &&
              menuName == L"KOI-8 ČS2 - CP1250",
          "The shipped CP1250 KOI-8 ČS2 menu name must be decoded to exact UTF-16");
}
} // namespace

int main()
{
    TestAutomaticCodePageSelection();
    TestIdentifierParsing();
    TestRegionalTextConversion();
    TestLegacyViewerTextConversion();
    TestInheritedViewerFontDpiScaling();
    TestLegacyAutoDetectionPreference();
    TestShippedConversionDirectories();

    if (Failures != 0)
    {
        std::cerr << Failures << " conversion-table test(s) failed.\n";
        return 1;
    }
    std::cout << "All conversion-table tests passed.\n";
    return 0;
}
