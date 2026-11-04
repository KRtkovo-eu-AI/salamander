// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Stop on both sides of space runs. | Spaces were still skipped while punctuation already stopped before and after. | | Composer |
// | 2026-11-04 | Add word boundaries for edit controls. | Ctrl+arrow must stop around punctuation and symbols. | | Grok 4.7 |

#ifndef _EDIT_WORD_BREAK_H
#define _EDIT_WORD_BREAK_H

#ifndef _WINDOWS_
#include <windows.h>
#endif

// Word movement for single-line edit controls.
//
// A word is a run of letters and digits. Diacritics and other nonspacing marks
// stay with the preceding character. Characters outside the BMP, including a
// surrogate pair, are one word and are never split.
//
// Every other character is a boundary: punctuation, symbols, '_', spaces, and
// line breaks. A consecutive run of the same class is one step, so the caret
// stops before and after the run.

enum EditWordClass
{
    EditWordClassWord,
    EditWordClassSpace,
    EditWordClassLineBreak,
    EditWordClassPunct
};

struct EditWordSelection
{
    int Anchor;
    int Caret;
};

inline BOOL EditWordIsCombiningMark(wchar_t ch)
{
    WORD type1;
    WORD type3;

    if (ch < 128 || (ch >= 0xD800 && ch <= 0xDFFF))
        return FALSE;
    type1 = 0;
    type3 = 0;
    if (!GetStringTypeW(CT_CTYPE1, &ch, 1, &type1))
        return FALSE;
    if ((type1 & (C1_ALPHA | C1_DIGIT)) != 0)
        return FALSE;
    if (!GetStringTypeW(CT_CTYPE3, &ch, 1, &type3))
        return FALSE;
    return (type3 & (C3_NONSPACING | C3_DIACRITIC)) != 0;
}

inline EditWordClass EditWordClassOfBase(wchar_t ch)
{
    WORD type1;

    if (ch == L'\n' || ch == L'\r' || ch == 0x0085 || ch == 0x2028 || ch == 0x2029)
        return EditWordClassLineBreak;
    if (ch == L' ' || ch == L'\t' || ch == L'\f' || ch == L'\v')
        return EditWordClassSpace;
    if ((ch >= L'0' && ch <= L'9') ||
        (ch >= L'A' && ch <= L'Z') ||
        (ch >= L'a' && ch <= L'z'))
        return EditWordClassWord;
    if (ch < 128)
        return EditWordClassPunct;
    if (ch >= 0xD800 && ch <= 0xDFFF)
        return EditWordClassWord;

    type1 = 0;
    if (!GetStringTypeW(CT_CTYPE1, &ch, 1, &type1))
        return EditWordClassWord;
    if ((type1 & (C1_ALPHA | C1_DIGIT)) != 0)
        return EditWordClassWord;
    if ((type1 & C1_SPACE) != 0)
        return EditWordClassSpace;
    return EditWordClassPunct;
}

inline int EditWordNextUnit(const wchar_t* text, int length, int index)
{
    if (index < 0)
        return 0;
    if (text == NULL || index >= length)
        return length;
    if (index + 1 < length &&
        IS_HIGH_SURROGATE(text[index]) &&
        IS_LOW_SURROGATE(text[index + 1]))
        return index + 2;
    return index + 1;
}

inline int EditWordPreviousUnit(const wchar_t* text, int index)
{
    if (text == NULL || index <= 1)
        return 0;
    if (IS_LOW_SURROGATE(text[index - 1]) && IS_HIGH_SURROGATE(text[index - 2]))
        return index - 2;
    return index - 1;
}

inline int EditWordNormalizeCaret(const wchar_t* text, int length, int caret)
{
    if (caret < 0)
        return 0;
    if (text == NULL || length <= 0)
        return 0;
    if (caret > length)
        return length;
    if (caret > 0 && caret < length &&
        IS_LOW_SURROGATE(text[caret]) &&
        IS_HIGH_SURROGATE(text[caret - 1]))
        return caret - 1;
    return caret;
}

inline EditWordClass EditWordClassAt(const wchar_t* text, int length, int index)
{
    wchar_t ch;
    int guard;

    if (text == NULL || index < 0 || index >= length)
        return EditWordClassPunct;
    if (IS_HIGH_SURROGATE(text[index]) || IS_LOW_SURROGATE(text[index]))
        return EditWordClassWord;

    ch = text[index];
    guard = 0;
    while (EditWordIsCombiningMark(ch) && index > 0 && guard < length)
    {
        index = EditWordPreviousUnit(text, index);
        ch = text[index];
        if (IS_HIGH_SURROGATE(ch) || IS_LOW_SURROGATE(ch))
            return EditWordClassWord;
        guard++;
    }
    if (EditWordIsCombiningMark(ch))
        return EditWordClassWord;
    return EditWordClassOfBase(ch);
}

inline int EditWordEndOfRun(const wchar_t* text, int length, int index)
{
    EditWordClass kind;

    kind = EditWordClassAt(text, length, index);
    while (index < length && EditWordClassAt(text, length, index) == kind)
        index = EditWordNextUnit(text, length, index);
    return index;
}

inline int EditWordStartOfRun(const wchar_t* text, int length, int index)
{
    EditWordClass kind;
    int previous;

    if (index <= 0)
        return 0;
    if (index > length)
        index = length;
    previous = EditWordPreviousUnit(text, index);
    kind = EditWordClassAt(text, length, previous);
    while (previous > 0)
    {
        int before;

        before = EditWordPreviousUnit(text, previous);
        if (EditWordClassAt(text, length, before) != kind)
            break;
        previous = before;
    }
    return previous;
}

inline int NextEditWordBoundary(const wchar_t* text, int length, int caret)
{
    if (text == NULL || length <= 0)
        return 0;
    caret = EditWordNormalizeCaret(text, length, caret);
    if (caret >= length)
        return length;
    return EditWordEndOfRun(text, length, caret);
}

inline int PreviousEditWordBoundary(const wchar_t* text, int length, int caret)
{
    if (text == NULL || length <= 0)
        return 0;
    caret = EditWordNormalizeCaret(text, length, caret);
    if (caret <= 0)
        return 0;
    return EditWordStartOfRun(text, length, caret);
}

inline int EditWordTokenStart(const wchar_t* text, int length, int index)
{
    EditWordClass kind;
    int previous;

    if (text == NULL || length <= 0 || index <= 0)
        return 0;
    index = EditWordNormalizeCaret(text, length, index);
    if (index >= length)
        return PreviousEditWordBoundary(text, length, length);

    kind = EditWordClassAt(text, length, index);
    while (index > 0)
    {
        previous = EditWordPreviousUnit(text, index);
        if (EditWordClassAt(text, length, previous) != kind)
            break;
        index = previous;
    }
    return index;
}

inline int EditWordTokenEnd(const wchar_t* text, int length, int index)
{
    if (text == NULL || length <= 0)
        return 0;
    if (index < 0)
        index = 0;
    index = EditWordNormalizeCaret(text, length, index);
    if (index >= length)
        return length;
    return EditWordEndOfRun(text, length, index);
}

inline int EditWordIsDelimiter(const wchar_t* text, int length, int index)
{
    if (text == NULL || index < 0 || index >= length)
        return FALSE;
    if (index > 0 && IS_LOW_SURROGATE(text[index]) && IS_HIGH_SURROGATE(text[index - 1]))
        return FALSE;
    return EditWordClassAt(text, length, index) != EditWordClassWord;
}

inline int EditWordBreakBoundary(const wchar_t* text, int length, int current, int code)
{
    if (text == NULL || length < 0)
        return 0;
    if (length == 0)
        return 0;
    if (current < 0)
        current = 0;
    if (current > length)
        current = length;

    if (code == WB_LEFT)
        return EditWordTokenStart(text, length, current);
    if (code == WB_RIGHT)
    {
        // comctl32 v6 selects [WB_LEFT(index), WB_RIGHT(WB_LEFT(index) + 1)].
        // A one-character token would otherwise swallow the following run, so
        // WB_RIGHT ends the run that contains the preceding unit.
        const int contained = current > 0 ? current - 1 : 0;
        return EditWordTokenEnd(text, length, contained);
    }
    if (code == WB_ISDELIMITER)
        return EditWordIsDelimiter(text, length, current);
    return length;
}

inline EditWordSelection MoveEditWordCaret(const wchar_t* text, int length, int anchor, int caret, BOOL extend, BOOL forward)
{
    EditWordSelection result;
    int next;

    next = forward ? NextEditWordBoundary(text, length, caret) : PreviousEditWordBoundary(text, length, caret);
    result.Anchor = extend ? anchor : next;
    result.Caret = next;
    return result;
}

// The returned anchor is the first deleted unit and the caret is the unit after
// the deleted span. A non-empty selection is deleted as it stands.
inline EditWordSelection EditWordDeleteRange(const wchar_t* text, int length, int anchor, int caret, BOOL forward)
{
    EditWordSelection result;
    int other;

    if (anchor != caret)
    {
        result.Anchor = anchor < caret ? anchor : caret;
        result.Caret = anchor < caret ? caret : anchor;
        return result;
    }

    other = forward ? NextEditWordBoundary(text, length, caret) : PreviousEditWordBoundary(text, length, caret);
    result.Anchor = other < caret ? other : caret;
    result.Caret = other < caret ? caret : other;
    return result;
}

#endif // _EDIT_WORD_BREAK_H
