// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <stdio.h>

#include "../common/str.h"
#include "../common/regexp.h"

#include <cstring>
#include <iostream>
#include <string>

// regexp.cpp only needs the case tables from str.cpp; build them like InitializeCase().
BYTE LowerCase[256];
BYTE UpperCase[256];

// The host (viewer2.cpp in Salamander) provides the localized error texts.
const char* RegExpErrorText(CRegExpErrors err)
{
    return err == reeNoError ? "No error." : "Regular expression error.";
}

namespace
{
int Failures = 0;

void InitCaseTables()
{
    for (int i = 0; i < 256; i++)
    {
        LowerCase[i] = (BYTE)((i >= 'A' && i <= 'Z') ? i - 'A' + 'a' : i);
        UpperCase[i] = (BYTE)((i >= 'a' && i <= 'z') ? i - 'a' + 'A' : i);
    }
}

void Check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << "\n";
        Failures++;
    }
}

struct Match
{
    bool compiled;
    int pos;
    int len;
};

// Searches 'line' for 'pattern'; forward from 'start' or backward over the first 'start' bytes
// (start < 0 = whole line).
Match Find(const char* pattern, const char* line, bool utf8, bool caseSensitive = true,
           bool forward = true, int start = -1)
{
    CRegularExpression re;
    re.SetUtf8(utf8 ? TRUE : FALSE);
    WORD flags = 0;
    if (caseSensitive)
        flags |= sfCaseSensitive;
    if (forward)
        flags |= sfForward;
    if (!re.Set(pattern, flags))
        return {false, -1, 0};
    int lineLen = (int)strlen(line);
    if (!re.SetLine(line, line + lineLen))
        return {false, -1, 0};
    int foundLen = 0;
    int pos;
    if (forward)
        pos = re.SearchForward(start < 0 ? 0 : start, foundLen);
    else
        pos = re.SearchBackward(start < 0 ? lineLen : start, foundLen);
    return {true, pos, pos >= 0 ? foundLen : 0};
}

void Expect(const char* name, const Match& m, int pos, int len)
{
    Check(m.compiled, std::string(name) + ": pattern must compile");
    Check(m.pos == pos && m.len == len,
          std::string(name) + ": expected pos " + std::to_string(pos) + " len " + std::to_string(len) +
              ", got pos " + std::to_string(m.pos) + " len " + std::to_string(m.len));
}

void ExpectNone(const char* name, const Match& m)
{
    Check(m.compiled, std::string(name) + ": pattern must compile");
    Check(m.pos == -1, std::string(name) + ": expected no match, got pos " + std::to_string(m.pos));
}

// UTF-8 byte lengths: ž/š/ř/č/Č = 2, € = 3, 😀 = 4.

void TestDot()
{
    Expect("dot takes a 2-byte char", Find("a.b", "ažb", true), 0, 4);
    Expect("dot takes a 3-byte char", Find("a.b", "a€b", true), 0, 5);
    Expect("dot takes a 4-byte char", Find("a.b", "a😀b", true), 0, 6);
    ExpectNone("dot is one char, not two", Find("a..b", "ažb", true));
    Expect("dot alone matches whole first char", Find(".", "žx", true), 0, 2);
}

void TestBrackets()
{
    Expect("range ř-ž contains š", Find("[ř-ž]", "aša", true), 1, 2);
    ExpectNone("range ř-ž excludes č", Find("[ř-ž]", "ačc", true));
    Expect("set with non-ASCII members", Find("[čž]+", "xžčžy", true), 1, 6);
    Expect("negated set skips non-ASCII char", Find("[^ ]", " ž x", true), 1, 2);
    Expect("negated non-ASCII set", Find("[^ž]", "žžax", true), 4, 1);
    Expect("mixed ASCII/non-ASCII range", Find("[a-cř-ž]+", "--ažb--", true), 2, 4);
}

void TestRepeats()
{
    Expect("star of a non-ASCII char", Find("ž*", "žžžx", true), 0, 6);
    Expect("plus of a non-ASCII char", Find("xž+", "xžžy", true), 0, 5);
    Expect("question of a non-ASCII char", Find("ab?ž?c", "ažc", true), 0, 4);
    Expect("literal back-off before star", Find("ažž*c", "ažc", true), 0, 4);
    Expect("dot star backtracks by chars", Find("a.*ž", "ašžčžb", true), 0, 9);
    Expect("plus of dot", Find("x.+y", "x€žy", true), 0, 7);
}

void TestEscapes()
{
    Expect("escaped non-ASCII char", Find("\\ž", "až", true), 1, 2);
    Expect("escaped char then star", Find("a\\ž*b", "ažžb", true), 0, 6);
}

void TestStartPositions()
{
    Expect("start offset after first char", Find(".", "žš", true, true, true, 2), 2, 2);
    Expect("anchored line start", Find("^ž", "žž", true), 0, 2);
    ExpectNone("anchored mismatch", Find("^š", "žš", true));
    Expect("end anchor", Find("ž$", "ažbž", true), 4, 2);
}

void TestCaseInsensitive()
{
    Expect("Č matches č", Find("Č", "xčx", true, false), 1, 2);
    Expect("č matches Č", Find("č", "xČx", true, false), 1, 2);
    Expect("word folds", Find("čau", "ahoj ČAU", true, false), 5, 4);
    Expect("range folds", Find("[ř-ž]+", "ŠŽx", true, false), 0, 4);
    ExpectNone("case-sensitive keeps Č != č", Find("Č", "xčx", true, true));
}

void TestBackward()
{
    Expect("backward finds last match", Find("ž.", "žažb", true, true, false), 3, 3);
    Expect("backward single char", Find("ž", "ažbž", true, true, false), 4, 2);
    Expect("backward limited length", Find("ž", "ažbž", true, true, false, 4), 1, 2);
    Expect("backward dot", Find("a.", "ačač", true, true, false), 3, 3);
    Expect("backward range plus", Find("[ř-ž]+", "šx žš", true, true, false), 4, 4);
    Expect("backward case-insensitive", Find("Ž", "žaž", true, false, false), 3, 2);
}

void TestMalformed()
{
    // Invalid or truncated sequences count as one character per byte and never run past the NUL.
    Expect("truncated lead byte at end", Find(".", "\xC5", true), 0, 1);
    Expect("lead byte without continuation", Find("a.b", "a\xC5" "b", true), 0, 3);
    Expect("lone continuation bytes", Find("..", "\xBE\xBE", true), 0, 2);
    Expect("backward over malformed text", Find("x.", "x\xC5x\xBE", true, true, false), 2, 2);
}

void TestByteModeUnchanged()
{
    ExpectNone("byte mode: dot is one byte", Find("a.b", "ažb", false));
    Expect("byte mode: two dots for a 2-byte char", Find("a..b", "ažb", false), 0, 4);
    Expect("byte mode: star repeats last byte", Find("ž*", "žžž", false), 0, 2);
    Expect("byte mode: ASCII case fold", Find("ABC", "xabc", false, false), 1, 3);
    Expect("byte mode: backward", Find("b", "abab", false, true, false), 3, 1);
}
} // namespace

int main()
{
    InitCaseTables();
    TestDot();
    TestBrackets();
    TestRepeats();
    TestEscapes();
    TestStartPositions();
    TestCaseInsensitive();
    TestBackward();
    TestMalformed();
    TestByteModeUnchanged();
    if (Failures != 0)
    {
        std::cerr << Failures << " regexp UTF-8 test(s) failed\n";
        return 1;
    }
    std::cout << "regexp UTF-8 tests passed\n";
    return 0;
}
