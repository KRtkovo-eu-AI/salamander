// SPDX-FileCopyrightText: 2025 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace salamander::unicode
{
class SalWideString
{
public:
    SalWideString() noexcept;
    explicit SalWideString(size_t length);
    explicit SalWideString(std::wstring_view text);
    SalWideString(const SalWideString& other);
    SalWideString(SalWideString&& other) noexcept;
    ~SalWideString();

    SalWideString& operator=(const SalWideString& other);
    SalWideString& operator=(SalWideString&& other) noexcept;

    [[nodiscard]] bool empty() const noexcept { return length_ == 0; }
    [[nodiscard]] size_t length() const noexcept { return length_; }
    [[nodiscard]] const wchar_t* c_str() const noexcept { return buffer_ ? buffer_ : L""; }
    [[nodiscard]] wchar_t* data() noexcept { return buffer_; }
    [[nodiscard]] explicit operator bool() const noexcept { return valid_; }

    void clear() noexcept;
    void swap(SalWideString& other) noexcept;

    [[nodiscard]] std::wstring to_wstring() const;
    [[nodiscard]] wchar_t* release() noexcept;

    static SalWideString Duplicate(std::wstring_view text);
    static SalWideString Concat(std::wstring_view first, std::wstring_view second);
    static SalWideString Concat(const std::vector<std::wstring_view>& parts);
    static SalWideString Slice(std::wstring_view source, size_t start, size_t length);

    static SalWideString FromAnsi(const char* src, int srcLen, unsigned int codepage);
    static SalWideString FromUtf8(const char* src, int srcLen);

    [[nodiscard]] std::string ToAnsi(bool compositeCheck, unsigned int codepage) const;
    [[nodiscard]] std::string ToUtf8() const;

private:
    wchar_t* buffer_;
    size_t length_;
    bool valid_;

    static wchar_t* Allocate(size_t length) noexcept;
    void Assign(std::wstring_view text);
    void Invalidate() noexcept;
    void ReleaseStorage() noexcept;
};

bool IsHighSurrogate(wchar_t ch) noexcept;
bool IsLowSurrogate(wchar_t ch) noexcept;
size_t AdjustSliceStart(std::wstring_view text, size_t start) noexcept;
size_t AdjustSliceEnd(std::wstring_view text, size_t start, size_t length) noexcept;

} // namespace salamander::unicode
