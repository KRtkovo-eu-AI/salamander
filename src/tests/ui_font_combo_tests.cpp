// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#define UNICODE
#define _UNICODE
#define NOMINMAX
#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "../common/winlibdpi.h"

namespace
{
int Checks = 0;
int Failures = 0;
const char* CurrentCase = "setup";

void Check(bool condition, const char* description)
{
    ++Checks;
    if (!condition)
    {
        ++Failures;
        std::fprintf(stderr, "FAIL [%s]: %s\n", CurrentCase, description);
    }
}

std::wstring ReadWide(HWND window)
{
    std::wstring text(GetWindowTextLengthW(window) + 1, L'\0');
    text.resize(GetWindowTextW(window, &text[0], static_cast<int>(text.size())));
    return text;
}

std::string ReadAnsi(HWND window)
{
    std::string text(GetWindowTextLengthA(window) + 1, '\0');
    text.resize(GetWindowTextA(window, &text[0], static_cast<int>(text.size())));
    return text;
}

struct Combo
{
    HWND Parent;
    HWND Window;
    HWND Edit;

    Combo(bool unicode, DWORD style, HFONT initialFont)
    {
        // These are private, hidden test windows. No desktop input or existing
        // application is accessed, and the regression is entirely synchronous.
        Parent = CreateWindowExW(0, L"STATIC", L"", WS_POPUP,
                                 0, 0, 500, 300, NULL, NULL, GetModuleHandleW(NULL), NULL);
        Window = unicode
                     ? CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | style | CBS_AUTOHSCROLL,
                                       0, 0, 450, 200, Parent, NULL, GetModuleHandleW(NULL), NULL)
                     : CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | style | CBS_AUTOHSCROLL,
                                       0, 0, 450, 200, Parent, NULL, GetModuleHandleW(NULL), NULL);
        COMBOBOXINFO info = {};
        info.cbSize = sizeof(info);
        Check(Parent != NULL && Window != NULL && GetComboBoxInfo(Window, &info), "create native combo");
        Edit = info.hwndItem;
        SendMessageW(Window, CB_LIMITTEXT, 262144, 0);
        // A dialog template installs a font before Transfer fills its controls.
        SendMessageW(Window, WM_SETFONT, reinterpret_cast<WPARAM>(initialFont), FALSE);
    }

    ~Combo()
    {
        DestroyWindow(Parent);
    }
};

void CheckSelection(HWND edit, DWORD start, DWORD end)
{
    DWORD actualStart = 0;
    DWORD actualEnd = 0;
    SendMessageW(edit, EM_GETSEL, reinterpret_cast<WPARAM>(&actualStart), reinterpret_cast<LPARAM>(&actualEnd));
    Check(actualStart == start && actualEnd == end, "edit selection is preserved");
}

void CheckHistoryAndFont(Combo& combo, HFONT font, LRESULT selected, LRESULT count)
{
    Check(SendMessageW(combo.Window, CB_GETCURSEL, 0, 0) == selected, "history selection is preserved");
    Check(SendMessageW(combo.Window, CB_GETCOUNT, 0, 0) == count, "history count is preserved");
    Check(reinterpret_cast<HFONT>(SendMessageW(combo.Window, WM_GETFONT, 0, 0)) == font, "combo receives configured font");
}

void TestWide(const char* name, const std::wstring& initial, DWORD start, DWORD end,
              bool modified, bool selectedHistory, DWORD style, HFONT initialFont, HFONT finalFont)
{
    CurrentCase = name;
    Combo combo(true, style, initialFont);
    std::vector<std::wstring> history = {L"unrelated.txt", initial + L"-old-history.txt", L"other.txt"};
    for (const auto& item : history)
        SendMessageW(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    if (selectedHistory)
        SendMessageW(combo.Window, CB_SETCURSEL, 2, 0);
    SetWindowTextW(combo.Edit, initial.c_str());
    SendMessageW(combo.Edit, EM_SETSEL, start, end);
    SendMessageW(combo.Edit, EM_SETMODIFY, modified, 0);
    LRESULT selected = SendMessageW(combo.Window, CB_GETCURSEL, 0, 0);

    WinLibDPIApplyDialogFont(combo.Parent, finalFont);

    Check(ReadWide(combo.Edit) == initial, "Unicode edit keeps its exact initial text");
    Check(ReadWide(combo.Window) == initial, "Unicode combo keeps its exact initial text");
    CheckSelection(combo.Edit, start, end);
    Check((SendMessageW(combo.Edit, EM_GETMODIFY, 0, 0) != 0) == modified, "edit modified flag is preserved");
    CheckHistoryAndFont(combo, finalFont, selected, static_cast<LRESULT>(history.size()));
    for (size_t index = 0; index < history.size(); ++index)
    {
        std::vector<wchar_t> item(static_cast<size_t>(SendMessageW(combo.Window, CB_GETLBTEXTLEN, index, 0)) + 1);
        SendMessageW(combo.Window, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(item.data()));
        Check(item.data() == history[index], "Unicode history contents and order are preserved");
    }
    WinLibDPIApplyDialogFontChild(combo.Window, reinterpret_cast<LPARAM>(finalFont));
    Check(ReadWide(combo.Edit) == initial, "repeated font application keeps exact text");
    CheckSelection(combo.Edit, start, end);
    Check((SendMessageW(combo.Edit, EM_GETMODIFY, 0, 0) != 0) == modified, "repeated font application preserves modified flag");

    // Windows and custom DPI handlers can send this after WinLib initialization.
    // This deliberately bypasses the shared font-application callback.
    SendMessageW(combo.Window, WM_SETFONT, reinterpret_cast<WPARAM>(initialFont), TRUE);
    Check(ReadWide(combo.Edit) == initial, "later raw font message keeps Unicode text");
    CheckSelection(combo.Edit, start, end);
    Check((SendMessageW(combo.Edit, EM_GETMODIFY, 0, 0) != 0) == modified, "later raw font message preserves modified flag");
    CheckHistoryAndFont(combo, initialFont, selected, static_cast<LRESULT>(history.size()));
}

void TestAnsi(const char* name, const std::string& initial, DWORD start, DWORD end,
              bool modified, HFONT initialFont, HFONT finalFont, const std::string& suffix = "-old-history.txt")
{
    CurrentCase = name;
    Combo combo(false, CBS_DROPDOWN, initialFont);
    std::string history = initial + suffix;
    SendMessageA(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(history.c_str()));
    SetWindowTextA(combo.Edit, initial.c_str());
    SendMessageA(combo.Edit, EM_SETSEL, start, end);
    SendMessageA(combo.Edit, EM_SETMODIFY, modified, 0);

    WinLibDPIApplyDialogFont(combo.Parent, finalFont);

    Check(ReadAnsi(combo.Edit) == initial, "ANSI edit keeps its exact initial text");
    Check(ReadAnsi(combo.Window) == initial, "ANSI combo keeps its exact initial text");
    CheckSelection(combo.Edit, start, end);
    Check((SendMessageA(combo.Edit, EM_GETMODIFY, 0, 0) != 0) == modified, "ANSI modified flag is preserved");
    CheckHistoryAndFont(combo, finalFont, CB_ERR, 1);
    std::vector<char> item(static_cast<size_t>(SendMessageA(combo.Window, CB_GETLBTEXTLEN, 0, 0)) + 1);
    SendMessageA(combo.Window, CB_GETLBTEXT, 0, reinterpret_cast<LPARAM>(item.data()));
    Check(item.data() == history, "ANSI history text is preserved");

    SendMessageA(combo.Window, WM_SETFONT, reinterpret_cast<WPARAM>(initialFont), TRUE);
    Check(ReadAnsi(combo.Edit) == initial, "later raw font message keeps ANSI text");
    CheckSelection(combo.Edit, start, end);
    Check((SendMessageA(combo.Edit, EM_GETMODIFY, 0, 0) != 0) == modified, "later raw ANSI font preserves modified flag");
}

void TestDropDownList(HFONT font)
{
    CurrentCase = "noneditable dropdown list";
    Combo combo(true, CBS_DROPDOWNLIST, font);
    SendMessageW(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"First"));
    SendMessageW(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Selected"));
    SendMessageW(combo.Window, CB_SETCURSEL, 1, 0);

    WinLibDPIApplyDialogFont(combo.Parent, font);

    Check(ReadWide(combo.Window) == L"Selected", "noneditable list keeps selected text");
    CheckHistoryAndFont(combo, font, 1, 2);
}

void TestPlainEdit(HFONT font)
{
    CurrentCase = "ordinary edit control";
    HWND parent = CreateWindowExW(0, L"STATIC", L"", WS_POPUP, 0, 0, 300, 100,
                                  NULL, NULL, GetModuleHandleW(NULL), NULL);
    HWND edit = CreateWindowExW(0, L"EDIT", L"unchanged", WS_CHILD, 0, 0, 250, 25,
                                parent, NULL, GetModuleHandleW(NULL), NULL);
    SendMessageW(edit, EM_SETSEL, 2, 5);

    WinLibDPIApplyDialogFont(parent, font);

    Check(ReadWide(edit) == L"unchanged", "ordinary edit keeps text");
    CheckSelection(edit, 2, 5);
    Check(reinterpret_cast<HFONT>(SendMessageW(edit, WM_GETFONT, 0, 0)) == font, "ordinary edit receives configured font");
    DestroyWindow(parent);
}

LRESULT CALLBACK CountTextMessages(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                  UINT_PTR, DWORD_PTR data)
{
    if (message == WM_SETTEXT)
        ++*reinterpret_cast<int*>(data);
    return DefSubclassProc(window, message, wParam, lParam);
}

void TestUnchangedTextNotifications(HFONT font)
{
    CurrentCase = "unchanged text notifications";
    Combo combo(true, CBS_DROPDOWN, font);
    SendMessageW(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"unrelated"));
    SetWindowTextW(combo.Edit, L"folder");
    int messages = 0;
    SetWindowSubclass(combo.Edit, CountTextMessages, 1, reinterpret_cast<DWORD_PTR>(&messages));
    SendMessageW(combo.Window, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
    int nativeMessages = messages;
    messages = 0;

    WinLibDPIApplyDialogFontChild(combo.Window, reinterpret_cast<LPARAM>(font));

    Check(ReadWide(combo.Edit) == L"folder", "unmatched history leaves text unchanged");
    Check(messages == nativeMessages, "unchanged text does not cause additional WM_SETTEXT");
    RemoveWindowSubclass(combo.Edit, CountTextMessages, 1);
}

void TestMultibyteAnsiSuffix(HFONT font)
{
    CPINFO info = {};
    if (!GetCPInfo(CP_ACP, &info) || info.MaxCharSize <= 1)
    {
        std::printf("ANSI multibyte suffix case requires a multibyte system code page (current: %u)\n", GetACP());
        return;
    }
    const wchar_t* wideSuffix = L"漢";
    BOOL usedDefault = FALSE;
    BOOL* defaultFlag = GetACP() == CP_UTF8 ? NULL : &usedDefault;
    int length = WideCharToMultiByte(CP_ACP, 0, wideSuffix, -1, NULL, 0, NULL, defaultFlag);
    if (length <= 2 || usedDefault)
    {
        std::printf("ANSI multibyte suffix is not representable in code page %u\n", GetACP());
        return;
    }
    std::string suffix(length, '\0');
    WideCharToMultiByte(CP_ACP, 0, wideSuffix, -1, &suffix[0], length, NULL, NULL);
    suffix.resize(length - 1);
    TestAnsi("ANSI multibyte history suffix", "folder", 0, 6, false, font, font, suffix);
}

LRESULT CALLBACK CountTextReads(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                               UINT_PTR, DWORD_PTR data)
{
    if (message == WM_GETTEXT || message == WM_GETTEXTLENGTH)
        ++*reinterpret_cast<int*>(data);
    return DefSubclassProc(window, message, wParam, lParam);
}

void TestHookIdempotence(HFONT font)
{
    CurrentCase = "repeated hook installation";
    Combo combo(true, CBS_DROPDOWN, font);
    SendMessageW(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"folder-history"));
    SetWindowTextW(combo.Edit, L"folder");
    WinLibDPIApplyDialogFont(combo.Parent, font);

    int reads = 0;
    SetWindowSubclass(combo.Edit, CountTextReads, 1, reinterpret_cast<DWORD_PTR>(&reads));
    SendMessageW(combo.Window, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    int firstReads = reads;
    for (int repeat = 0; repeat < 3; ++repeat)
        WinLibDPIApplyDialogFontChild(combo.Window, reinterpret_cast<LPARAM>(font));
    reads = 0;

    SendMessageW(combo.Window, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

    Check(reads == firstReads, "reinstalling font protection does not multiply snapshots");
    Check(ReadWide(combo.Edit) == L"folder", "late raw font remains protected after repeated installation");
    RemoveWindowSubclass(combo.Edit, CountTextReads, 1);
}

void TestLaterLayout(bool unicode, HFONT font)
{
    CurrentCase = unicode ? "later Unicode combo layout" : "later ANSI combo layout";
    Combo combo(unicode, CBS_DROPDOWN, font);
    SendMessageW(combo.Window, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"folder-history"));
    SetWindowTextW(combo.Edit, L"folder");
    SendMessageW(combo.Edit, EM_SETSEL, 1, 5);
    SendMessageW(combo.Edit, EM_SETMODIFY, TRUE, 0);
    WinLibDPIApplyDialogFont(combo.Parent, font);

    // A native layout/DPI change also refreshes the combo's edit from its list,
    // independently of WM_SETFONT. Preserve the input without suppressing layout.
    Check(SetWindowPos(combo.Window, NULL, 13, 17, 510, 220,
                       SWP_NOZORDER | SWP_NOACTIVATE) != FALSE, "native combo layout succeeds");
    RECT actual = {};
    GetWindowRect(combo.Window, &actual);
    MapWindowPoints(NULL, combo.Parent, reinterpret_cast<POINT*>(&actual), 2);
    Check(actual.left == 13 && actual.top == 17, "requested combo position is applied");
    Check(actual.right - actual.left == 510, "requested combo width is applied");
    Check(ReadWide(combo.Edit) == L"folder", "layout preserves exact initial text");
    CheckSelection(combo.Edit, 1, 5);
    Check(SendMessageW(combo.Edit, EM_GETMODIFY, 0, 0) != 0, "layout preserves modified flag");
    Check(SendMessageW(combo.Window, CB_GETCURSEL, 0, 0) == CB_ERR, "layout preserves absent history selection");
}
}

int main()
{
    INITCOMMONCONTROLSEX controls = {sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    HFONT font = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                             DEFAULT_PITCH, L"MS Shell Dlg");
    HFONT largerFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                   DEFAULT_PITCH, L"MS Shell Dlg");
    Check(font != NULL && largerFont != NULL, "create dialog fonts");

    TestWide("folder matching rename history", L"folder", 0, 6, false, false, CBS_DROPDOWN, font, font);
    TestWide("copy target matching history prefix", L"C:\\target\\", 0, 10, false, false, CBS_DROPDOWN, font, font);
    TestWide("Unicode filename and extension selection", L"složka-日本.txt", 0, 9, true, false, CBS_DROPDOWN, font, largerFont);
    TestWide("empty edit", L"", 0, 0, false, false, CBS_DROPDOWN, font, font);
    TestWide("manually edited selected history entry", L"folder", 2, 4, true, true, CBS_DROPDOWN, font, font);
    TestWide("simple editable combo", L"folder", 1, 5, true, false, CBS_SIMPLE, font, largerFont);
    std::wstring longPath = L"C:\\日本\\" + std::wstring(700, L'ř') + L"\\file.txt";
    TestWide("long Unicode path", longPath, 300, 650, false, false, CBS_DROPDOWN, font, largerFont);
    TestWide("selection beyond 16-bit positions", std::wstring(70000, L'x'), 66000, 69999,
             true, false, CBS_DROPDOWN, font, font);
    TestAnsi("ANSI folder matching history", "folder", 0, 6, false, font, font);
    TestAnsi("empty ANSI edit", "", 0, 0, false, font, font);
    TestAnsi("long ANSI path", "C:\\" + std::string(700, 'x') + "\\file.txt", 300, 650, true, font, largerFont);
    TestDropDownList(font);
    TestPlainEdit(font);
    TestUnchangedTextNotifications(font);
    TestMultibyteAnsiSuffix(font);
    TestHookIdempotence(font);
    TestLaterLayout(true, font);
    TestLaterLayout(false, font);

    DeleteObject(largerFont);
    DeleteObject(font);
    std::printf("%d checks, %d failures\n", Checks, Failures);
    return Failures == 0 ? 0 : 1;
}
