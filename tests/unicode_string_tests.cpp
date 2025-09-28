// SPDX-FileCopyrightText: 2025 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/unicode.h"
#include "common/str.h"

#include <array>
#include <cassert>
#include <string>

using salamander::unicode::SalWideString;

void TestDuplicateAndRelease()
{
    const wchar_t* sample = L"Unicode 😀 string";
    SalWideString duplicated = SalWideString::Duplicate(sample);
    assert(duplicated);
    assert(std::wstring(sample) == duplicated.to_wstring());

    wchar_t* raw = duplicated.release();
    assert(raw != nullptr);
    std::wstring restored(raw);
    free(raw);
    assert(restored == sample);
}

void TestConcatenate()
{
    std::wstring left = L"Hello ";
    std::wstring right = L"世界";
    SalWideString combined = SalWideString::Concat({left, right});
    assert(combined);
    assert(combined.to_wstring() == left + right);
}

void TestSliceSurrogate()
{
    const std::wstring text = L"A\U0001F600B"; // includes surrogate pair for 😀
    SalWideString slice = SalWideString::Slice(text, 1, 1);
    assert(slice);
    assert(slice.length() == 2); // high + low surrogate preserved
    assert(slice.to_wstring() == L"\U0001F600");
}

void TestUtf8RoundTrip()
{
    const char* utf8Sample = u8"Encoding 😀 test";
    SalWideString fromUtf8 = SalWideString::FromUtf8(utf8Sample, -1);
    assert(fromUtf8);
    std::string utf8 = fromUtf8.ToUtf8();
    assert(utf8 == utf8Sample);
}

void TestStrNCatWide()
{
    std::array<wchar_t, 16> buffer{};
    lstrcpyW(buffer.data(), L"Hi");
    StrNCatW(buffer.data(), L" 😀", static_cast<int>(buffer.size()));
    assert(std::wstring(buffer.data()) == L"Hi 😀");
}

int main()
{
    TestDuplicateAndRelease();
    TestConcatenate();
    TestSliceSurrogate();
    TestUtf8RoundTrip();
    TestStrNCatWide();
    return 0;
}
