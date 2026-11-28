// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Add a Unicode-aware native JSON reader and writer. | Replace managed catalog and receipt serialization. | | openrouter/openai/gpt-6-luna-pro |
#pragma once

#include <map>
#include <string>
#include <vector>

namespace SamandarinJson
{
struct CaseInsensitiveLess
{
    bool operator()(const std::wstring& left, const std::wstring& right) const;
};

struct Value
{
    enum class Type
    {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object,
    };

    Type Kind = Type::Null;
    bool Boolean = false;
    std::wstring Text;
    std::vector<Value> Array;
    std::map<std::wstring, Value, CaseInsensitiveLess> Object;

    const Value* Find(const wchar_t* name) const;
    const Value* Find(const std::wstring& name) const;
    std::wstring StringOrEmpty() const;
    bool IsNull() const { return Kind == Type::Null; }
};

bool ParseUtf8(const std::string& text, Value* value, std::wstring* error = nullptr);
bool ParseWide(const std::wstring& text, Value* value, std::wstring* error = nullptr);
std::string ToUtf8(const std::wstring& text);
std::string StringifyUtf8(const Value& value);
std::wstring Escape(const std::wstring& text);
} // namespace SamandarinJson
