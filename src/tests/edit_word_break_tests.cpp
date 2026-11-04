// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Expect stops on both sides of spaces. | Spaces must match punctuation: stop before and after the run. | | Composer |
// | 2026-11-04 | Cover edit word-boundary movement. | Ctrl+arrow must stop around punctuation and symbols. | | Grok 4.7 |

#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#include "../edit_word_break.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace
{
int gCallCount = 0;
int gCallCode[32];
int gCallCurrent[32];
int gCallResult[32];

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        std::exit(1);
    }
}

void ExpectAt(const wchar_t* text, int caret, int expected, bool forward, const char* message)
{
    const int length = (int)std::wcslen(text);
    const int actual = forward ? NextEditWordBoundary(text, length, caret) : PreviousEditWordBoundary(text, length, caret);
    if (actual != expected)
    {
        std::fprintf(stderr, "FAILED: %s (caret %d expected %d actual %d)\n", message, caret, expected, actual);
        std::exit(1);
    }
}

void ExpectNext(const wchar_t* text, int caret, int expected, const char* message)
{
    ExpectAt(text, caret, expected, true, message);
}

void ExpectPrevious(const wchar_t* text, int caret, int expected, const char* message)
{
    ExpectAt(text, caret, expected, false, message);
}

void TestPath()
{
    const wchar_t* text = L"C:\\foo-bar_baz.txt";
    ExpectNext(text, 0, 1, "stop after the drive letter");
    ExpectNext(text, 1, 3, "colon and backslash are one punctuation run");
    ExpectNext(text, 3, 6, "stop before the hyphen");
    ExpectNext(text, 4, 6, "middle of foo still ends before the hyphen");
    ExpectNext(text, 6, 7, "hyphen is its own stop");
    ExpectNext(text, 7, 10, "stop before the underscore");
    ExpectNext(text, 10, 11, "underscore is its own stop");
    ExpectNext(text, 11, 14, "stop before the dot");
    ExpectNext(text, 14, 15, "dot is its own stop");
    ExpectNext(text, 15, 18, "extension is the last word");
    ExpectNext(text, 18, 18, "forward at the end stays put");

    ExpectPrevious(text, 18, 15, "back from the end to the extension");
    ExpectPrevious(text, 15, 14, "back from the extension to the dot");
    ExpectPrevious(text, 14, 11, "back from the dot to baz");
    ExpectPrevious(text, 12, 11, "middle of baz returns to its start");
    ExpectPrevious(text, 11, 10, "back from baz to the underscore");
    ExpectPrevious(text, 10, 7, "back from the underscore to bar");
    ExpectPrevious(text, 7, 6, "back from bar to the hyphen");
    ExpectPrevious(text, 6, 3, "back from the hyphen to foo");
    ExpectPrevious(text, 3, 1, "back from foo to the colon");
    ExpectPrevious(text, 1, 0, "back from the colon to the drive letter");
    ExpectPrevious(text, 0, 0, "back at the start stays put");
}

void TestSpaces()
{
    ExpectNext(L"hello world", 0, 5, "stop before the space");
    ExpectNext(L"hello world", 2, 5, "middle of the first word still stops before the space");
    ExpectNext(L"hello world", 5, 6, "a space run is its own stop");
    ExpectNext(L"hello world", 6, 11, "second word runs to the end");
    ExpectPrevious(L"hello world", 11, 6, "back from the end to the second word");
    ExpectPrevious(L"hello world", 8, 6, "middle of the second word returns to its start");
    ExpectPrevious(L"hello world", 6, 5, "back from the second word stops on the space");
    ExpectPrevious(L"hello world", 5, 0, "back from the space returns to the first word");

    ExpectNext(L"hello  world", 0, 5, "stop before a repeated space run");
    ExpectNext(L"hello  world", 5, 7, "repeated spaces are one step");
    ExpectNext(L"hello  world", 6, 7, "middle of repeated spaces jumps to their end");
    ExpectPrevious(L"hello  world", 7, 5, "back from the second word to the space run");
    ExpectPrevious(L"hello  world", 6, 5, "middle of repeated spaces returns to their start");
    ExpectPrevious(L"hello  world", 5, 0, "back from the space run to the first word");
}

void TestPunctuationRuns()
{
    ExpectNext(L"abc...def", 0, 3, "stop before a dot run");
    ExpectNext(L"abc...def", 3, 6, "a dot run is one step");
    ExpectNext(L"abc...def", 4, 6, "middle of a dot run jumps to its end");
    ExpectNext(L"abc...def", 5, 6, "last dot still ends the run");
    ExpectNext(L"abc...def", 6, 9, "word after the run goes to the end");
    ExpectPrevious(L"abc...def", 6, 3, "back from the following word to the start of the run");
    ExpectPrevious(L"abc...def", 5, 3, "middle of the run returns to its start");
    ExpectPrevious(L"abc...def", 3, 0, "back from the run to the word");

    ExpectNext(L"abc ((())) def", 0, 3, "stop before the space before parentheses");
    ExpectNext(L"abc ((())) def", 3, 4, "space before parentheses is its own stop");
    ExpectNext(L"abc ((())) def", 4, 10, "parenthesis run stops before the following space");
    ExpectNext(L"abc ((())) def", 10, 11, "space after parentheses is its own stop");
    ExpectPrevious(L"abc ((())) def", 11, 10, "back from def to the trailing space");
    ExpectPrevious(L"abc ((())) def", 10, 4, "back from that space to the parenthesis run");
    ExpectPrevious(L"abc ((())) def", 4, 3, "back from the parenthesis run to the leading space");
    ExpectPrevious(L"abc ((())) def", 3, 0, "back from that space to abc");

    ExpectNext(L"32.3", 0, 2, "a dot splits a number");
    ExpectNext(L"32.3", 2, 3, "the dot is its own stop");
    ExpectNext(L"32.3", 3, 4, "the last digit is a word");
    ExpectPrevious(L"32.3", 4, 3, "back from the last digit to the dot");
    ExpectPrevious(L"32.3", 2, 0, "back from the dot to the first digits");

    ExpectNext(L"don't", 0, 3, "an apostrophe splits a word");
    ExpectNext(L"don't", 3, 4, "the apostrophe is its own stop");
    ExpectNext(L"don't", 4, 5, "the letter after the apostrophe is a word");
    ExpectPrevious(L"don't", 4, 3, "back from t to the apostrophe");
    ExpectPrevious(L"don't", 3, 0, "back from the apostrophe to don");
}

void TestParenthesesAndWords()
{
    const wchar_t* text = L" (1) abc";
    ExpectNext(text, 0, 1, "leading space is its own stop");
    ExpectNext(text, 1, 2, "opening parenthesis stops before the digit");
    ExpectNext(text, 2, 3, "digit stops before the closing parenthesis");
    ExpectNext(text, 3, 4, "closing parenthesis stops before the following space");
    ExpectNext(text, 4, 5, "space after the closing parenthesis is its own stop");
    ExpectNext(text, 5, 8, "abc runs to the end");
    ExpectPrevious(text, 8, 5, "back from the end to abc");
    ExpectPrevious(text, 5, 4, "back from abc to the trailing space");
    ExpectPrevious(text, 4, 3, "back from that space to the closing parenthesis");
    ExpectPrevious(text, 3, 2, "back from the closing parenthesis to the digit");
    ExpectPrevious(text, 2, 1, "back from the digit to the opening parenthesis");
    ExpectPrevious(text, 1, 0, "back from the opening parenthesis to the leading space");
}

void TestUnicode()
{
    const wchar_t* czech = L"P\u0159\u00edli\u0161 \u017elu\u0165ou\u010dk\u00fd";
    Check((int)std::wcslen(czech) == 16, "Czech sample has the expected length");
    ExpectNext(czech, 0, 6, "stop before the space after the first Czech word");
    ExpectNext(czech, 3, 6, "middle of a Czech word still stops before the space");
    ExpectNext(czech, 6, 7, "space between Czech words is its own stop");
    ExpectNext(czech, 7, 16, "second Czech word runs to the end");
    ExpectPrevious(czech, 16, 7, "back to the second Czech word");
    ExpectPrevious(czech, 10, 7, "middle of the second Czech word returns to its start");
    ExpectPrevious(czech, 7, 6, "back from the second Czech word stops on the space");
    ExpectPrevious(czech, 6, 0, "back from the space to the first Czech word");

    const wchar_t decomposed[] = {L'e', 0x0301, L'x', 0};
    ExpectNext(decomposed, 0, 3, "a combining mark stays inside the word");
    ExpectPrevious(decomposed, 2, 0, "back through a combining mark returns to the base");
    Check(!EditWordIsDelimiter(decomposed, 3, 1), "combining mark is not a delimiter");

    ExpectNext(L"a\u00a0b", 0, 1, "stop before a no-break space");
    ExpectNext(L"a\u00a0b", 1, 2, "no-break space is its own stop");
    ExpectPrevious(L"a\u00a0b", 2, 1, "back from b stops on the no-break space");
    ExpectPrevious(L"a\u00a0b", 1, 0, "back from the no-break space reaches a");

    const wchar_t line[] = {L'h', L'e', L'l', L'l', L'o', L'\n', L'w', L'o', L'r', L'l', L'd', 0};
    ExpectNext(line, 0, 5, "forward stops before a line break");
    ExpectNext(line, 5, 6, "a line break moves to the next word");
    ExpectPrevious(line, 6, 5, "back from the next line stops on the line break");
    ExpectPrevious(line, 5, 0, "back from the line break returns to the word");

    wchar_t pair[] = {L'a', L'b', 0xD83D, 0xDE00, L'c', L'd', 0};
    ExpectNext(pair, 0, 6, "a surrogate pair stays inside the surrounding word");
    ExpectNext(pair, 2, 6, "forward from the high surrogate does not split the pair");
    ExpectPrevious(pair, 3, 0, "back from the low surrogate does not split the pair");
    ExpectPrevious(pair, 4, 0, "back from the letter after the pair returns to the start");
    Check(!EditWordIsDelimiter(pair, 6, 3), "low surrogate is not a delimiter");

    wchar_t split[] = {L'a', 0xD83D, 0xDE00, L'-', L'b', 0};
    ExpectNext(split, 0, 3, "the pair is part of the word before the hyphen");
    ExpectNext(split, 1, 3, "forward from the high surrogate stops at the hyphen");
    ExpectNext(split, 3, 4, "hyphen after a pair is its own stop");
    ExpectPrevious(split, 3, 0, "back from the hyphen includes the whole pair");
}

void TestCaretMovementAndDeletion()
{
    const wchar_t* text = L"C:\\foo-bar_baz.txt";
    const int length = (int)std::wcslen(text);
    EditWordSelection selection = MoveEditWordCaret(text, length, 0, 0, TRUE, TRUE);
    Check(selection.Anchor == 0 && selection.Caret == 1, "Ctrl+Shift+Right keeps the anchor");
    selection = MoveEditWordCaret(text, length, selection.Anchor, selection.Caret, TRUE, TRUE);
    Check(selection.Anchor == 0 && selection.Caret == 3, "a second extend reaches the end of the punctuation run");
    selection = MoveEditWordCaret(text, length, 4, 9, FALSE, TRUE);
    Check(selection.Anchor == 10 && selection.Caret == 10, "Ctrl+Right without Shift collapses onto the new caret");

    selection = EditWordDeleteRange(text, length, 6, 6, FALSE);
    Check(selection.Anchor == 3 && selection.Caret == 6, "Ctrl+Backspace deletes back to the start of foo");
    selection = EditWordDeleteRange(text, length, 1, 6, TRUE);
    Check(selection.Anchor == 1 && selection.Caret == 6, "a selection is deleted as it stands");
    selection = EditWordDeleteRange(L"hello world", 11, 0, 0, TRUE);
    Check(selection.Anchor == 0 && selection.Caret == 5, "Ctrl+Delete removes the first word and stops before the space");
    selection = EditWordDeleteRange(L"hello world", 11, 5, 5, TRUE);
    Check(selection.Anchor == 5 && selection.Caret == 6, "Ctrl+Delete on a space removes only the space");
}

void TestEdges()
{
    Check(NextEditWordBoundary(NULL, 4, 1) == 0, "missing text has no forward boundary");
    Check(PreviousEditWordBoundary(L"abc", 3, -4) == 0, "a negative caret moves back to the start");
    Check(NextEditWordBoundary(L"abc", 3, 9) == 3, "a caret past the end stays at the end");
    Check(EditWordTokenStart(L"foo-bar", 7, 1) == 0, "double-click start of foo");
    Check(EditWordTokenEnd(L"foo-bar", 7, 1) == 3, "double-click end of foo");
    Check(EditWordTokenStart(L"foo-bar", 7, 3) == 3, "double-click start of the hyphen");
    Check(EditWordTokenEnd(L"foo-bar", 7, 3) == 4, "double-click end of the hyphen");
    Check(EditWordIsDelimiter(L"foo_bar", 7, 3), "underscore is a delimiter");
    Check(!EditWordIsDelimiter(L"foo_bar", 7, 1), "a letter is not a delimiter");
    Check(EditWordBreakBoundary(L"foo-bar", 7, 0, WB_RIGHT) == 3, "WB_RIGHT at the start ends foo");
    Check(EditWordBreakBoundary(L"foo-bar", 7, 0, WB_LEFT) == 0, "WB_LEFT at the start stays on foo");
}

// The edit control selects [WB_LEFT(index), WB_RIGHT(WB_LEFT(index) + 1)].
void ExpectDoubleClick(const wchar_t* text, int index, int expectedStart, int expectedEnd, const char* message)
{
    const int length = (int)std::wcslen(text);
    const int start = EditWordBreakBoundary(text, length, index, WB_LEFT);
    const int end = EditWordBreakBoundary(text, length, start + 1, WB_RIGHT);
    if (start != expectedStart || end != expectedEnd)
    {
        std::fprintf(stderr, "FAILED: %s (index %d selection %d..%d expected %d..%d)\n",
                     message, index, start, end, expectedStart, expectedEnd);
        std::exit(1);
    }
}

void TestDoubleClickComposition()
{
    const wchar_t* path = L"C:\\foo-bar_baz.txt";
    ExpectDoubleClick(path, 0, 0, 1, "double-click on the drive letter");
    ExpectDoubleClick(path, 1, 1, 3, "double-click on the colon selects colon and backslash");
    ExpectDoubleClick(path, 2, 1, 3, "double-click on the backslash selects the same run");
    ExpectDoubleClick(path, 4, 3, 6, "double-click inside foo");
    ExpectDoubleClick(path, 6, 6, 7, "double-click on the hyphen");
    ExpectDoubleClick(path, 8, 7, 10, "double-click inside bar");
    ExpectDoubleClick(path, 10, 10, 11, "double-click on the underscore");
    ExpectDoubleClick(path, 12, 11, 14, "double-click inside baz");
    ExpectDoubleClick(path, 14, 14, 15, "double-click on the dot");
    ExpectDoubleClick(path, 16, 15, 18, "double-click inside the extension");

    ExpectDoubleClick(L"abc...def", 4, 3, 6, "double-click in a dot run selects the whole run");
    ExpectDoubleClick(L" (1) abc", 1, 1, 2, "double-click on an opening parenthesis");
    ExpectDoubleClick(L" (1) abc", 2, 2, 3, "double-click on a single digit");
    ExpectDoubleClick(L"foo-bar baz", 7, 7, 8, "double-click on the space");
    ExpectDoubleClick(L"foo-bar baz", 9, 8, 11, "double-click inside baz");
}

int CALLBACK LiveWordBreakProc(LPWSTR text, int current, int length, int code)
{
    const int result = EditWordBreakBoundary(text, length, current, code);
    if (gCallCount < 32)
    {
        gCallCode[gCallCount] = code;
        gCallCurrent[gCallCount] = current;
        gCallResult[gCallCount] = result;
        gCallCount++;
    }
    return result;
}

void FailLive(const char* message)
{
    std::fprintf(stderr, "FAILED: %s\n", message);
    for (int i = 0; i < gCallCount; i++)
    {
        std::fprintf(stderr, "  call code %d current %d result %d\n",
                     gCallCode[i], gCallCurrent[i], gCallResult[i]);
    }
    std::exit(1);
}

void LiveDoubleClick(HWND edit, int index, int expectedStart, int expectedEnd, const char* message)
{
    const LRESULT packed = SendMessageW(edit, EM_POSFROMCHAR, (WPARAM)index, 0);
    const LRESULT nextPacked = SendMessageW(edit, EM_POSFROMCHAR, (WPARAM)(index + 1), 0);
    int x = (short)LOWORD(packed);
    const int nextX = (short)LOWORD(nextPacked);
    if (nextPacked != -1 && nextX > x)
        x += (nextX - x) / 2;
    else
        x += 1;
    const int y = (short)HIWORD(packed) + 1;
    const LPARAM point = MAKELPARAM(x, y);

    gCallCount = 0;
    SendMessageW(edit, WM_LBUTTONDOWN, MK_LBUTTON, point);
    SendMessageW(edit, WM_LBUTTONUP, 0, point);
    SendMessageW(edit, WM_LBUTTONDBLCLK, MK_LBUTTON, point);
    SendMessageW(edit, WM_LBUTTONUP, 0, point);

    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(edit, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    if ((int)start != expectedStart || (int)end != expectedEnd)
    {
        std::fprintf(stderr, "selection %lu..%lu, expected %d..%d\n", start, end, expectedStart, expectedEnd);
        FailLive(message);
    }
}

void TestLiveEditControl()
{
    const HINSTANCE instance = GetModuleHandleW(NULL);
    HWND edit = CreateWindowExW(0, L"EDIT", L"", WS_POPUP | ES_AUTOHSCROLL,
                                -2000, -2000, 500, 24, NULL, NULL, instance, NULL);
    Check(edit != NULL, "create a Unicode edit control");
    SendMessageW(edit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);
    ShowWindow(edit, SW_SHOWNOACTIVATE);
    SetWindowTextW(edit, L"foo-bar baz");
    SendMessageW(edit, EM_SETWORDBREAKPROC, 0, (LPARAM)LiveWordBreakProc);
    Check((UINT_PTR)SendMessageW(edit, EM_GETWORDBREAKPROC, 0, 0) == (UINT_PTR)LiveWordBreakProc,
          "install the word-break callback");

    LiveDoubleClick(edit, 1, 0, 3, "double-click inside foo selects foo");
    LiveDoubleClick(edit, 3, 3, 4, "double-click on the hyphen selects only the hyphen");
    LiveDoubleClick(edit, 5, 4, 7, "double-click inside bar selects bar");
    LiveDoubleClick(edit, 8, 8, 11, "double-click inside baz selects baz");

    DestroyWindow(edit);
}
}

int main()
{
    TestPath();
    TestSpaces();
    TestPunctuationRuns();
    TestParenthesesAndWords();
    TestUnicode();
    TestCaretMovementAndDeletion();
    TestEdges();
    TestDoubleClickComposition();
    TestLiveEditControl();
    std::printf("Edit word-break tests passed.\n");
    return 0;
}
