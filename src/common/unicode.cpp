// SPDX-FileCopyrightText: 2025 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include "unicode.h"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace salamander::unicode
{
namespace
{
constexpr size_t kMaxSize = static_cast<size_t>(std::numeric_limits<int>::max());

[[nodiscard]] bool ShouldIncludeTerminator(int length)
{
    return length == -1;
}

[[nodiscard]] bool HasRoomFor(size_t value)
{
    return value <= kMaxSize;
}
} // namespace

SalWideString::SalWideString() noexcept
    : buffer_(nullptr), length_(0), valid_(true)
{
}

SalWideString::SalWideString(size_t length)
    : buffer_(nullptr), length_(0), valid_(true)
{
    if (!HasRoomFor(length))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        Invalidate();
        return;
    }
    buffer_ = Allocate(length);
    if (!buffer_)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        Invalidate();
        return;
    }
    length_ = length;
    buffer_[length_] = L'\0';
}

SalWideString::SalWideString(std::wstring_view text)
    : buffer_(nullptr), length_(0), valid_(true)
{
    Assign(text);
}

SalWideString::SalWideString(const SalWideString& other)
    : buffer_(nullptr), length_(0), valid_(other.valid_)
{
    if (!other.valid_)
    {
        Invalidate();
        return;
    }

    if (other.buffer_)
        Assign(std::wstring_view(other.buffer_, other.length_));
    else
        Assign(std::wstring_view());
}

SalWideString::SalWideString(SalWideString&& other) noexcept
    : buffer_(other.buffer_), length_(other.length_), valid_(other.valid_)
{
    other.buffer_ = nullptr;
    other.length_ = 0;
    other.valid_ = true;
}

SalWideString::~SalWideString()
{
    ReleaseStorage();
}

SalWideString& SalWideString::operator=(const SalWideString& other)
{
    if (this != &other)
    {
        if (!other.valid_)
        {
            Invalidate();
        }
        else if (other.buffer_)
        {
            Assign(std::wstring_view(other.buffer_, other.length_));
        }
        else
        {
            Assign(std::wstring_view());
        }
    }
    return *this;
}

SalWideString& SalWideString::operator=(SalWideString&& other) noexcept
{
    if (this != &other)
    {
        ReleaseStorage();
        buffer_ = other.buffer_;
        length_ = other.length_;
        valid_ = other.valid_;
        other.buffer_ = nullptr;
        other.length_ = 0;
        other.valid_ = true;
    }
    return *this;
}

void SalWideString::clear() noexcept
{
    ReleaseStorage();
    valid_ = true;
}

void SalWideString::swap(SalWideString& other) noexcept
{
    std::swap(buffer_, other.buffer_);
    std::swap(length_, other.length_);
    std::swap(valid_, other.valid_);
}

std::wstring SalWideString::to_wstring() const
{
    if (!valid_ || !buffer_)
        return {};
    return std::wstring(buffer_, length_);
}

wchar_t* SalWideString::release() noexcept
{
    if (!valid_)
        return nullptr;
    wchar_t* tmp = buffer_;
    buffer_ = nullptr;
    length_ = 0;
    valid_ = true;
    return tmp;
}

SalWideString SalWideString::Duplicate(std::wstring_view text)
{
    return SalWideString(text);
}

SalWideString SalWideString::Concat(std::wstring_view first, std::wstring_view second)
{
    return Concat(std::vector<std::wstring_view>{first, second});
}

SalWideString SalWideString::Concat(const std::vector<std::wstring_view>& parts)
{
    size_t total = 0;
    for (const std::wstring_view part : parts)
    {
        if (!HasRoomFor(part.size()) || part.size() > kMaxSize - total)
        {
            SalWideString failure;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            failure.Invalidate();
            return failure;
        }
        total += part.size();
    }

    SalWideString result(total);
    if (!result)
        return result;

    size_t offset = 0;
    for (const std::wstring_view part : parts)
    {
        if (!part.empty())
        {
            memcpy(result.buffer_ + offset, part.data(), part.size() * sizeof(wchar_t));
            offset += part.size();
        }
    }
    result.buffer_[result.length_] = L'\0';
    return result;
}

SalWideString SalWideString::Slice(std::wstring_view source, size_t start, size_t length)
{
    if (source.empty())
        return SalWideString(0);

    size_t safeStart = AdjustSliceStart(source, start);
    size_t safeEnd = AdjustSliceEnd(source, safeStart, length);
    if (safeStart >= safeEnd)
        return SalWideString(0);
    return SalWideString(std::wstring_view(source.data() + safeStart, safeEnd - safeStart));
}

SalWideString SalWideString::FromAnsi(const char* src, int srcLen, unsigned int codepage)
{
    if (!src)
    {
        SalWideString failure;
        SetLastError(ERROR_INVALID_PARAMETER);
        failure.Invalidate();
        return failure;
    }

    if (srcLen != -1 && srcLen < 0)
    {
        SalWideString failure;
        SetLastError(ERROR_INVALID_PARAMETER);
        failure.Invalidate();
        return failure;
    }

    if (srcLen == 0)
        return SalWideString(0);

    const bool includeTerminator = ShouldIncludeTerminator(srcLen);
    int required = MultiByteToWideChar(codepage, 0, src, srcLen, nullptr, 0);
    if (required <= 0)
    {
        SalWideString failure;
        DWORD err = GetLastError();
        failure.Invalidate();
        SetLastError(err);
        return failure;
    }

    if (includeTerminator)
        --required;

    if (!HasRoomFor(static_cast<size_t>(required)))
    {
        SalWideString failure;
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        failure.Invalidate();
        return failure;
    }

    SalWideString result(static_cast<size_t>(required));
    if (!result)
        return result;

    int copy = MultiByteToWideChar(codepage, 0, src, srcLen, result.buffer_, required + (includeTerminator ? 1 : 0));
    if (copy <= 0)
    {
        DWORD err = GetLastError();
        result.Invalidate();
        SetLastError(err);
        return result;
    }

    if (includeTerminator)
    {
        result.length_ = static_cast<size_t>(copy > 0 ? copy - 1 : 0);
    }
    else
    {
        result.length_ = static_cast<size_t>(copy);
        result.buffer_[result.length_] = L'\0';
    }

    return result;
}

SalWideString SalWideString::FromUtf8(const char* src, int srcLen)
{
    return FromAnsi(src, srcLen, CP_UTF8);
}

std::string SalWideString::ToAnsi(bool compositeCheck, unsigned int codepage) const
{
    if (!valid_ || !buffer_)
        return std::string();

    DWORD flags = compositeCheck ? WC_COMPOSITECHECK : 0;
    int required = WideCharToMultiByte(codepage, flags, buffer_, static_cast<int>(length_), nullptr, 0, nullptr, nullptr);
    if (required <= 0 && compositeCheck)
    {
        flags = 0;
        required = WideCharToMultiByte(codepage, flags, buffer_, static_cast<int>(length_), nullptr, 0, nullptr, nullptr);
    }

    if (required <= 0)
    {
        DWORD err = GetLastError();
        SetLastError(err);
        return std::string();
    }

    std::string result(static_cast<size_t>(required), '\0');
    int converted = WideCharToMultiByte(codepage, flags, buffer_, static_cast<int>(length_), result.data(), required, nullptr, nullptr);
    if (converted <= 0)
    {
        DWORD err = GetLastError();
        SetLastError(err);
        return std::string();
    }

    if (static_cast<size_t>(converted) < result.size())
        result.resize(static_cast<size_t>(converted));

    return result;
}

std::string SalWideString::ToUtf8() const
{
    return ToAnsi(FALSE, CP_UTF8);
}

wchar_t* SalWideString::Allocate(size_t length) noexcept
{
    size_t bytes = (length + 1) * sizeof(wchar_t);
    auto* buffer = static_cast<wchar_t*>(malloc(bytes));
    if (!buffer)
        return nullptr;
    buffer[length] = L'\0';
    return buffer;
}

void SalWideString::Assign(std::wstring_view text)
{
    ReleaseStorage();
    valid_ = true;

    if (!HasRoomFor(text.size()))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        Invalidate();
        return;
    }

    buffer_ = Allocate(text.size());
    if (!buffer_)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        Invalidate();
        return;
    }

    length_ = text.size();
    if (length_ != 0)
        memcpy(buffer_, text.data(), length_ * sizeof(wchar_t));
    buffer_[length_] = L'\0';
}

void SalWideString::Invalidate() noexcept
{
    ReleaseStorage();
    valid_ = false;
}

void SalWideString::ReleaseStorage() noexcept
{
    if (buffer_)
        free(buffer_);
    buffer_ = nullptr;
    length_ = 0;
}

bool IsHighSurrogate(wchar_t ch) noexcept
{
    return ch >= 0xD800 && ch <= 0xDBFF;
}

bool IsLowSurrogate(wchar_t ch) noexcept
{
    return ch >= 0xDC00 && ch <= 0xDFFF;
}

size_t AdjustSliceStart(std::wstring_view text, size_t start) noexcept
{
    if (start >= text.size())
        return text.size();
    if (start > 0 && IsLowSurrogate(text[start]) && IsHighSurrogate(text[start - 1]))
        return start - 1;
    return start;
}

size_t AdjustSliceEnd(std::wstring_view text, size_t start, size_t length) noexcept
{
    if (start >= text.size())
        return text.size();
    size_t end = start + length;
    if (end > text.size())
        end = text.size();
    if (end < text.size() && IsLowSurrogate(text[end]) && IsHighSurrogate(text[end - 1]))
        ++end;
    return end;
}

} // namespace salamander::unicode
