// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include "../panel_quick_search.h"
#include "../quick_search_match.h"
#include <cstdio>
#include <cstdlib>
#include <string>

using namespace Salamander::Panel;

namespace
{
void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        std::exit(1);
    }
}

void TestUnicodeInput()
{
    wchar_t pending = 0;
    std::wstring text;
    Check(AppendQuickSearchCodeUnit(pending, L'\x0161', text) && text == L"\x0161", "Czech s-caron is a Unicode character");
    Check(AppendQuickSearchCodeUnit(pending, L'\x00E9', text) && text == L"\x00E9", "Latin-1 character is not mistaken for UTF8 lead byte");
    Check(AppendQuickSearchCodeUnit(pending, L'\x0436', text) && text == L"\x0436", "Cyrillic input is preserved");
    Check(!AppendQuickSearchCodeUnit(pending, 0xD83D, text) && text.empty(), "high surrogate waits without modifying mask");
    Check(AppendQuickSearchCodeUnit(pending, 0xDE00, text) && text == L"\xD83D\xDE00" && pending == 0, "surrogate pair forms one complete input");
    Check(!AppendQuickSearchCodeUnit(pending, 0xDE00, text), "orphan low surrogate is ignored");
    AppendQuickSearchCodeUnit(pending, 0xD83D, text);
    Check(AppendQuickSearchCodeUnit(pending, L'a', text) && text == L"a", "ordinary character drops incomplete pair");
    AppendQuickSearchCodeUnit(pending, 0xD83D, text);
    Check(!AppendQuickSearchCodeUnit(pending, VK_ESCAPE, text) && pending == 0, "control input clears pending pair");
    Check(!AppendQuickSearchCodeUnit(pending, 0x110000, text), "out-of-range code unit is rejected");
    Check(QuickSearchCodePointText(0x1F600) == L"\xD83D\xDE00", "WM_UNICHAR supplementary scalar converts to UTF16");
    Check(QuickSearchCodePointText(0xD800).empty() && QuickSearchCodePointText(0x110000).empty(), "WM_UNICHAR rejects invalid scalars");
    Check(QuickSearchCodePointText(0x10FFFF) == L"\xDBFF\xDFFF", "highest valid Unicode scalar is accepted");
}

void TestIndependentPanels()
{
    wchar_t leftPending = 0, rightPending = 0;
    std::wstring text;
    AppendQuickSearchCodeUnit(leftPending, 0xD83D, text);
    Check(!AppendQuickSearchCodeUnit(rightPending, 0xDE00, text), "another panel cannot complete the first panel's character");
    leftPending = 0; // EndQuickSearch/OnKillFocus use this reset.
    Check(!AppendQuickSearchCodeUnit(leftPending, 0xDE00, text), "new session cannot reuse an unfinished pair");
    Check(AppendQuickSearchCodeUnit(rightPending, L'\x0161', text) && text == L"\x0161", "other panel remains usable");
}

void TestEditingAndMask()
{
    std::wstring prefix = L"\x0161k", mask = prefix;
    ShortenQuickSearchPrefix(prefix, mask);
    Check(prefix == L"\x0161" && mask == prefix, "Left updates both matching prefix and mask");
    mask += L"a";
    int offset = 0;
    Check(MatchQuickSearchWide(L"\x0160" L"atna", false, mask.c_str(), false, offset) && offset == 2,
          "typing after Left uses shortened mask, with case-insensitive Unicode matching");
    prefix = L"A\xD83D\xDE00";
    mask = prefix;
    ShortenQuickSearchPrefix(prefix, mask);
    Check(prefix == L"A" && mask == L"A", "Left removes both surrogate units");
    Check(ExtendQuickSearchPrefix(prefix, mask, L"A\xD83D\xDE00Z") &&
              prefix == L"A\xD83D\xDE00" && mask == prefix, "Right extends by a complete supplementary scalar");
    RemoveLastQuickSearchCharacter(mask);
    Check(mask == L"A", "Backspace removes both surrogate units");
    prefix = L"ae\x0301";
    mask = L"a\x00E9";
    ShortenQuickSearchPrefix(prefix, mask);
    if (!MatchQuickSearchWide(prefix.c_str(), true, mask.c_str(), true, offset))
        mask = prefix; // The host verifies this invariant after Left as well.
    Check(prefix == L"ae" && mask == prefix, "Left reconciles composed mask with decomposed filename prefix");
    ExtendQuickSearchPrefix(prefix, mask, L"ae\x0301Z");
    Check(MatchQuickSearchWide(L"a\x00E9Z", false, mask.c_str(), false, offset) && offset == 2,
          "Right after Left must restore the original canonical match");
    prefix = L"alphabet";
    mask = L"al/ph";
    ShortenQuickSearchPrefix(prefix, mask);
    Check(prefix == L"alphabe" && mask == L"al/p", "Left preserves an ordinary wildcard mask suffix");
    ShortenQuickSearchPrefix(prefix, mask);
    Check(prefix == L"alphab" && mask == prefix, "Left drops wildcard when crossing its boundary");
    prefix.clear(); mask.clear();
    Check(!ExtendQuickSearchPrefix(prefix, mask, L""), "Right at end of name is harmless");
    RemoveLastQuickSearchCharacter(mask);
    Check(mask.empty(), "Backspace on empty mask is harmless");
}

void TestLongUnicodePrefix()
{
    const std::wstring name = std::wstring(240, L'\x0161') + L".\x017E";
    const int utf8Bytes = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), (int)name.length(), NULL, 0, NULL, NULL);
    Check(utf8Bytes > 260 && name.length() < 255, "fixture exceeds old UTF8 byte limit but is a valid filename");
    std::wstring prefix, mask;
    while (ExtendQuickSearchPrefix(prefix, mask, name))
        ;
    Check(prefix == name && mask == name, "right-arrow completion never truncates UTF8 bytes");
    int offset = 0;
    Check(MatchQuickSearchWide(name.c_str(), true, mask.c_str(), false, offset) && offset == (int)name.length(),
          "long Unicode prefix still matches full name");
    const auto range = GetQuickSearchCaretTextRange(name, prefix.length(), true);
    Check(range.Start == 241 && range.Length == 1, "extension caret uses UTF16 offsets after long Unicode basename");
}

struct InputWindow
{
    wchar_t Pending = 0;
    std::wstring Text;
};

LRESULT CALLBACK InputProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(window, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }
    auto input = reinterpret_cast<InputWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (input != NULL && (message == WM_CHAR || message == WM_SYSCHAR))
    {
        std::wstring complete;
        if (AppendQuickSearchCodeUnit(input->Pending, (std::uint32_t)wParam, complete))
            input->Text += complete;
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK AnsiInputProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTA*>(lParam);
        SetWindowLongPtrA(window, GWLP_USERDATA, (LONG_PTR)create->lpCreateParams);
    }
    auto text = reinterpret_cast<std::string*>(GetWindowLongPtrA(window, GWLP_USERDATA));
    if (text != NULL && message == WM_CHAR)
    {
        text->push_back((char)wParam);
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

void TestAnsiWindowCompatibility()
{
    WNDCLASSA klass = {};
    klass.hInstance = GetModuleHandleW(NULL);
    klass.lpszClassName = "QuickSearchAnsiCompatibilityTest";
    klass.lpfnWndProc = AnsiInputProc;
    Check(RegisterClassA(&klass) != 0, "register legacy ANSI window");
    std::string received;
    HWND window = CreateWindowExA(0, klass.lpszClassName, "", 0, 0, 0, 0, 0,
                                   HWND_MESSAGE, NULL, klass.hInstance, &received);
    Check(window != NULL && !IsWindowUnicode(window), "legacy test window remains ANSI");
    const wchar_t czech = L'\x0161';
    char encoded[4];
    int count = WideCharToMultiByte(GetACP(), 0, &czech, 1, encoded, 4, NULL, NULL);
    std::string expected = "A";
    if (count == 1)
        expected += encoded[0];
    else
        expected += 'Z';
    for (unsigned char byte : expected)
        PostMessageA(window, WM_CHAR, byte, 0);
    MSG message;
    while (PeekMessageW(&message, window, WM_CHAR, WM_CHAR, PM_REMOVE))
        DispatchMessageW(&message);
    Check(received == expected, "Unicode message pump preserves ANSI window byte semantics through Windows thunk");
    DestroyWindow(window);
    UnregisterClassA(klass.lpszClassName, klass.hInstance);
}

void TestWin32UnicodeWindow()
{
    WNDCLASSW klass = {};
    klass.hInstance = GetModuleHandleW(NULL);
    klass.lpszClassName = L"QuickSearchUnicodeInputTest";
    klass.lpfnWndProc = InputProc;
    Check(RegisterClassW(&klass) != 0, "register Unicode input window");
    InputWindow input;
    HWND window = CreateWindowExW(0, klass.lpszClassName, L"", 0, 0, 0, 0, 0,
                                   HWND_MESSAGE, NULL, klass.hInstance, &input);
    Check(window != NULL && IsWindowUnicode(window), "created input window uses UTF16 messages");
    SendMessageW(window, WM_CHAR, 0x0161, 0);
    SendMessageW(window, WM_SYSCHAR, 0x010D, 0);
    SendMessageW(window, WM_CHAR, 0xD83D, 0);
    Check(input.Text == L"\x0161\x010D", "pending surrogate never reaches search text");
    SendMessageW(window, WM_CHAR, 0xDE00, 0);
    Check(input.Text == L"\x0161\x010D\xD83D\xDE00", "real Win32 WM_CHAR and Alt WM_SYSCHAR preserve all Unicode units");
    input.Text.clear();
    const wchar_t queuedText[] = L"\x0161\x011B\x4E2D\xD83D\xDE00";
    for (wchar_t unit : std::wstring(queuedText))
        PostMessageW(window, WM_CHAR, unit, 0);
    MSG message;
    while (PeekMessageW(&message, window, WM_CHAR, WM_SYSCHAR, PM_REMOVE))
        DispatchMessageW(&message);
    Check(input.Text == queuedText, "Unicode host pump preserves Czech, CJK and supplementary characters independently of ACP");
    DestroyWindow(window);
    UnregisterClassW(klass.lpszClassName, klass.hInstance);
}
}

int main()
{
    TestUnicodeInput();
    TestIndependentPanels();
    TestEditingAndMask();
    TestLongUnicodePrefix();
    TestWin32UnicodeWindow();
    TestAnsiWindowCompatibility();
    std::printf("Quick-search Unicode input tests passed (6 scenarios; process ACP %u).\n", GetACP());
    return 0;
}
