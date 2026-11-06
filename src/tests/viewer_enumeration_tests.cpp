// SPDX-FileCopyrightText: 2026 Samandarin Contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../viewerpath.h"
#include "../plugins/pictview/unicodetitle.h"

static int Failed = 0;
static int Passed = 0;
static void Check(bool ok, const char* name)
{
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    if (ok) ++Passed; else ++Failed;
}
static std::string Utf8(const std::wstring& text)
{
    int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], length, NULL, NULL);
    result.resize(length - 1);
    return result;
}
int main()
{
    using namespace Salamander::ViewerPaths;
    std::vector<std::wstring> files = {L"C:\\root\\pic.jpg", L"C:\\root\\A\\pic.jpg", L"C:\\root\\B\\pic.jpg"};
    auto path = [&files](int i) { return files[i]; };
    Check(FindIndex(1, 3, files[1], path) == 1 && FindIndex(0, 3, files[2], path) == 2,
          "duplicate basenames use complete directory identity");
    std::wstring current = files[2];
    std::reverse(files.begin(), files.end());
    Check(FindIndex(2, 3, current, path) == 0, "reordering relocates the current full path");
    files.erase(files.begin());
    Check(FindIndex(0, 2, current, path) == -1, "removed item cannot bind to another equal basename");
    files = {L"C:\\root\\A.jpg", L"C:\\root\\a.jpg", L"C:\\root\\u00e9.jpg", L"C:\\root\\e\u0301.jpg"};
    Check(FindIndex(0, 4, files[1], path) == 1 && FindIndex(2, 4, files[3], path) == 3,
          "case and canonical Unicode variants remain distinct disk identities");
    std::wstring unicode = L"\\server\\share\\u65e5\u672c\\u010desk\u00fd\\U0001f600.png";
    Check(Decode(Utf8(unicode).c_str()) == unicode, "UNC path preserves all Unicode components");
    std::wstring utf8Long = L"C:\\" + std::wstring(120, L'\u65e5') + L"\\image.png";
    Check(utf8Long.size() < MAX_PATH && Utf8(utf8Long).size() > MAX_PATH && Decode(Utf8(utf8Long).c_str()) == utf8Long,
          "UTF-8 byte length beyond MAX_PATH is not truncated");
    std::wstring longName = L"C:\\";
    for (int i = 0; i < 160; ++i) longName += std::wstring(100, L'\u65e5') + L"\\";
    longName += L"image.png";
    Check(longName.size() > MAX_PATH && Utf8(longName).size() > 32768 &&
          Decode(Utf8(longName).c_str()) == longName && FullPath(longName.c_str()) == longName,
          "long UTF-16 and larger UTF-8 representations stay complete");
    std::vector<wchar_t> output(unicode.size() + 2, L'!');
    Check(CopyResult(unicode, &output[0], (int)unicode.size() + 1) &&
          std::wstring(&output[0]) == unicode && output.back() == L'!',
          "exact output capacity includes terminator without overwriting canary");
    output.assign(output.size(), L'!');
    Check(!CopyResult(unicode, &output[0], (int)unicode.size()) &&
          GetLastError() == ERROR_INSUFFICIENT_BUFFER && output[0] == 0 && output.back() == L'!',
          "short output fails empty without returning a partial path");
    Check(!CopyResult(unicode, NULL, 0) && GetLastError() == ERROR_INVALID_PARAMETER,
          "invalid capacity is rejected");
    HWND window = CreateWindowExA(0, "STATIC", "", WS_POPUP, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (window != NULL)
    {
        BOOL installed = PictViewUnicodeTitle::Install(window);
        SetWindowTextW(window, unicode.c_str());
        std::vector<wchar_t> title(unicode.size() + 1);
        GetWindowTextW(window, &title[0], (int)title.size());
        Check(installed && IsWindowUnicode(window) && std::wstring(&title[0]) == unicode, "wide native title survives the legacy ANSI window class");
        DestroyWindow(window);
    }
    else Check(false, "create hidden title test window");
    std::printf("%d passed, %d failed\n", Passed, Failed);
    return Failed == 0 ? 0 : 1;
}
