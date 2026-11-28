// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-28 | Add a Unicode-aware native JSON reader and writer. | Replace managed catalog and receipt serialization. | | openrouter/openai/gpt-6-luna-pro |

#include "precomp.h"
#include "native_json.h"

#include <algorithm>
#include <limits>

namespace SamandarinJson
{
namespace
{
constexpr size_t kMaximumDepth = 64;
constexpr size_t kMaximumNodes = 500000;

void SetError(std::wstring* error, const wchar_t* text)
{
    if (error != nullptr)
    {
        *error = text;
    }
}

bool AppendCodePoint(std::wstring& output, unsigned int codePoint)
{
    if (codePoint > 0x10FFFF || (codePoint >= 0xD800 && codePoint <= 0xDFFF))
    {
        return false;
    }
    if (codePoint <= 0xFFFF)
    {
        output.push_back(static_cast<wchar_t>(codePoint));
    }
    else
    {
        codePoint -= 0x10000;
        output.push_back(static_cast<wchar_t>(0xD800 + (codePoint >> 10)));
        output.push_back(static_cast<wchar_t>(0xDC00 + (codePoint & 0x3FF)));
    }
    return true;
}

class Parser
{
public:
    explicit Parser(const std::wstring& text) : Text(text) {}

    bool Parse(Value* value, std::wstring* error)
    {
        if (value == nullptr)
        {
            SetError(error, L"JSON output is unavailable.");
            return false;
        }
        SkipWhitespace();
        if (!ParseValue(value, 0))
        {
            SetError(error, Error.empty() ? L"Invalid JSON document." : Error.c_str());
            return false;
        }
        SkipWhitespace();
        if (Position != Text.size())
        {
            SetError(error, L"Unexpected data after the JSON value.");
            return false;
        }
        return true;
    }

private:
    bool ParseValue(Value* value, size_t depth)
    {
        if (depth > kMaximumDepth || ++NodeCount > kMaximumNodes)
        {
            Error = L"The JSON document exceeds the supported size.";
            return false;
        }
        SkipWhitespace();
        if (Position >= Text.size())
        {
            Error = L"Unexpected end of the JSON document.";
            return false;
        }

        const wchar_t current = Text[Position];
        if (current == L'{')
        {
            return ParseObject(value, depth + 1);
        }
        if (current == L'[')
        {
            return ParseArray(value, depth + 1);
        }
        if (current == L'"')
        {
            value->Kind = Value::Type::String;
            return ParseString(&value->Text);
        }
        if (current == L't')
        {
            if (!ConsumeLiteral(L"true")) return false;
            value->Kind = Value::Type::Boolean;
            value->Boolean = true;
            return true;
        }
        if (current == L'f')
        {
            if (!ConsumeLiteral(L"false")) return false;
            value->Kind = Value::Type::Boolean;
            value->Boolean = false;
            return true;
        }
        if (current == L'n')
        {
            if (!ConsumeLiteral(L"null")) return false;
            value->Kind = Value::Type::Null;
            return true;
        }
        if (current == L'-' || (current >= L'0' && current <= L'9'))
        {
            return ParseNumber(value);
        }
        Error = L"Unexpected token in the JSON document.";
        return false;
    }

    bool ParseObject(Value* value, size_t depth)
    {
        value->Kind = Value::Type::Object;
        ++Position;
        SkipWhitespace();
        if (Consume(L'}')) return true;
        for (;;)
        {
            SkipWhitespace();
            if (Position >= Text.size() || Text[Position] != L'"')
            {
                Error = L"A JSON object key must be a string.";
                return false;
            }
            std::wstring key;
            if (!ParseString(&key)) return false;
            SkipWhitespace();
            if (!Consume(L':'))
            {
                Error = L"A colon is missing after a JSON object key.";
                return false;
            }
            Value child;
            if (!ParseValue(&child, depth)) return false;
            value->Object[key] = std::move(child);
            SkipWhitespace();
            if (Consume(L'}')) return true;
            if (!Consume(L','))
            {
                Error = L"A comma is missing between JSON object members.";
                return false;
            }
        }
    }

    bool ParseArray(Value* value, size_t depth)
    {
        value->Kind = Value::Type::Array;
        ++Position;
        SkipWhitespace();
        if (Consume(L']')) return true;
        for (;;)
        {
            Value child;
            if (!ParseValue(&child, depth)) return false;
            value->Array.push_back(std::move(child));
            SkipWhitespace();
            if (Consume(L']')) return true;
            if (!Consume(L','))
            {
                Error = L"A comma is missing between JSON array values.";
                return false;
            }
        }
    }

    bool ParseString(std::wstring* value)
    {
        if (!Consume(L'"')) return false;
        while (Position < Text.size())
        {
            const wchar_t current = Text[Position++];
            if (current == L'"') return true;
            if (current < 0x20)
            {
                Error = L"A JSON string contains an unescaped control character.";
                return false;
            }
            if (current != L'\\')
            {
                value->push_back(current);
                continue;
            }
            if (Position >= Text.size())
            {
                Error = L"A JSON escape is incomplete.";
                return false;
            }
            const wchar_t escaped = Text[Position++];
            switch (escaped)
            {
            case L'"': value->push_back(L'"'); break;
            case L'\\': value->push_back(L'\\'); break;
            case L'/': value->push_back(L'/'); break;
            case L'b': value->push_back(L'\b'); break;
            case L'f': value->push_back(L'\f'); break;
            case L'n': value->push_back(L'\n'); break;
            case L'r': value->push_back(L'\r'); break;
            case L't': value->push_back(L'\t'); break;
            case L'u':
            {
                unsigned int codePoint = 0;
                if (!ReadHexQuad(&codePoint)) return false;
                if (codePoint >= 0xD800 && codePoint <= 0xDBFF)
                {
                    if (Position + 2 > Text.size() || Text[Position] != L'\\' || Text[Position + 1] != L'u')
                    {
                        Error = L"A high surrogate is not followed by a low surrogate.";
                        return false;
                    }
                    Position += 2;
                    unsigned int low = 0;
                    if (!ReadHexQuad(&low) || low < 0xDC00 || low > 0xDFFF)
                    {
                        Error = L"A JSON surrogate pair is invalid.";
                        return false;
                    }
                    codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                }
                else if (codePoint >= 0xDC00 && codePoint <= 0xDFFF)
                {
                    Error = L"A low surrogate has no matching high surrogate.";
                    return false;
                }
                if (!AppendCodePoint(*value, codePoint))
                {
                    Error = L"A JSON Unicode escape is invalid.";
                    return false;
                }
                break;
            }
            default:
                Error = L"A JSON escape sequence is invalid.";
                return false;
            }
        }
        Error = L"A JSON string is not terminated.";
        return false;
    }

    bool ReadHexQuad(unsigned int* value)
    {
        if (Position + 4 > Text.size())
        {
            Error = L"A JSON Unicode escape is incomplete.";
            return false;
        }
        unsigned int result = 0;
        for (int index = 0; index < 4; ++index)
        {
            const wchar_t c = Text[Position++];
            unsigned int digit = 0;
            if (c >= L'0' && c <= L'9') digit = static_cast<unsigned int>(c - L'0');
            else if (c >= L'a' && c <= L'f') digit = static_cast<unsigned int>(c - L'a' + 10);
            else if (c >= L'A' && c <= L'F') digit = static_cast<unsigned int>(c - L'A' + 10);
            else
            {
                Error = L"A JSON Unicode escape contains a non-hexadecimal digit.";
                return false;
            }
            result = (result << 4) | digit;
        }
        *value = result;
        return true;
    }

    bool ParseNumber(Value* value)
    {
        const size_t start = Position;
        Consume(L'-');
        if (Consume(L'0'))
        {
            if (Position < Text.size() && Text[Position] >= L'0' && Text[Position] <= L'9')
            {
                Error = L"A JSON number contains a leading zero.";
                return false;
            }
        }
        else if (!ConsumeDigits())
        {
            Error = L"A JSON number is missing its integer part.";
            return false;
        }
        if (Consume(L'.') && !ConsumeDigits())
        {
            Error = L"A JSON fraction is missing its digits.";
            return false;
        }
        if (Consume(L'e') || Consume(L'E'))
        {
            if (!Consume(L'+')) Consume(L'-');
            if (!ConsumeDigits())
            {
                Error = L"A JSON exponent is missing its digits.";
                return false;
            }
        }
        value->Kind = Value::Type::Number;
        value->Text.assign(Text, start, Position - start);
        return true;
    }

    bool ConsumeDigits()
    {
        const size_t start = Position;
        while (Position < Text.size() && Text[Position] >= L'0' && Text[Position] <= L'9') ++Position;
        return Position != start;
    }

    bool ConsumeLiteral(const wchar_t* literal)
    {
        const size_t length = wcslen(literal);
        if (Text.compare(Position, length, literal) != 0)
        {
            Error = L"A JSON literal is invalid.";
            return false;
        }
        Position += length;
        return true;
    }

    void SkipWhitespace()
    {
        while (Position < Text.size())
        {
            const wchar_t c = Text[Position];
            if (c != L' ' && c != L'\t' && c != L'\r' && c != L'\n') break;
            ++Position;
        }
    }

    bool Consume(wchar_t character)
    {
        if (Position < Text.size() && Text[Position] == character)
        {
            ++Position;
            return true;
        }
        return false;
    }

    const std::wstring& Text;
    size_t Position = 0;
    size_t NodeCount = 0;
    std::wstring Error;
};

void AppendEscaped(std::wstring& output, const std::wstring& text)
{
    static const wchar_t hex[] = L"0123456789abcdef";
    output.push_back(L'"');
    for (wchar_t c : text)
    {
        switch (c)
        {
        case L'"': output.append(L"\\\""); break;
        case L'\\': output.append(L"\\\\"); break;
        case L'\b': output.append(L"\\b"); break;
        case L'\f': output.append(L"\\f"); break;
        case L'\n': output.append(L"\\n"); break;
        case L'\r': output.append(L"\\r"); break;
        case L'\t': output.append(L"\\t"); break;
        default:
            if (c < 0x20)
            {
                output.append(L"\\u00");
                output.push_back(hex[(c >> 4) & 0xF]);
                output.push_back(hex[c & 0xF]);
            }
            else
            {
                output.push_back(c);
            }
            break;
        }
    }
    output.push_back(L'"');
}

void AppendValue(std::wstring& output, const Value& value)
{
    switch (value.Kind)
    {
    case Value::Type::Null: output.append(L"null"); break;
    case Value::Type::Boolean: output.append(value.Boolean ? L"true" : L"false"); break;
    case Value::Type::Number: output.append(value.Text); break;
    case Value::Type::String: AppendEscaped(output, value.Text); break;
    case Value::Type::Array:
        output.push_back(L'[');
        for (size_t index = 0; index < value.Array.size(); ++index)
        {
            if (index != 0) output.push_back(L',');
            AppendValue(output, value.Array[index]);
        }
        output.push_back(L']');
        break;
    case Value::Type::Object:
    {
        output.push_back(L'{');
        bool first = true;
        for (const auto& entry : value.Object)
        {
            if (!first) output.push_back(L',');
            first = false;
            AppendEscaped(output, entry.first);
            output.push_back(L':');
            AppendValue(output, entry.second);
        }
        output.push_back(L'}');
        break;
    }
    }
}
} // namespace

bool CaseInsensitiveLess::operator()(const std::wstring& left, const std::wstring& right) const
{
    const int result = CompareStringOrdinal(left.data(), static_cast<int>(left.size()),
                                            right.data(), static_cast<int>(right.size()), TRUE);
    return result == CSTR_LESS_THAN;
}

const Value* Value::Find(const wchar_t* name) const
{
    return name == nullptr ? nullptr : Find(std::wstring(name));
}

const Value* Value::Find(const std::wstring& name) const
{
    if (Kind != Type::Object) return nullptr;
    const auto found = Object.find(name);
    return found == Object.end() ? nullptr : &found->second;
}

std::wstring Value::StringOrEmpty() const
{
    if (Kind == Type::String || Kind == Type::Number) return Text;
    if (Kind == Type::Boolean) return Boolean ? L"true" : L"false";
    return std::wstring();
}

bool ParseUtf8(const std::string& text, Value* value, std::wstring* error)
{
    size_t offset = text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xEF &&
                            static_cast<unsigned char>(text[1]) == 0xBB &&
                            static_cast<unsigned char>(text[2]) == 0xBF
                        ? 3
                        : 0;
    if (text.size() - offset > static_cast<size_t>((std::numeric_limits<int>::max)()))
    {
        SetError(error, L"The JSON document exceeds the supported size.");
        return false;
    }
    const int length = static_cast<int>(text.size() - offset);
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data() + offset, length, nullptr, 0);
    if (required <= 0 && length != 0)
    {
        SetError(error, L"The JSON document is not valid UTF-8.");
        return false;
    }
    std::wstring wide(static_cast<size_t>(required), L'\0');
    if (required > 0 && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data() + offset, length,
                                            wide.data(), required) != required)
    {
        SetError(error, L"The JSON document is not valid UTF-8.");
        return false;
    }
    return ParseWide(wide, value, error);
}

bool ParseWide(const std::wstring& text, Value* value, std::wstring* error)
{
    Parser parser(text);
    return parser.Parse(value, error);
}

std::string ToUtf8(const std::wstring& text)
{
    if (text.size() > static_cast<size_t>((std::numeric_limits<int>::max)())) return std::string();
    const int length = static_cast<int>(text.size());
    if (length == 0) return std::string();
    const int required = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length,
                                             nullptr, 0, nullptr, nullptr);
    if (required <= 0) return std::string();
    std::string result(static_cast<size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), length,
                            &result[0], required, nullptr, nullptr) != required)
    {
        return std::string();
    }
    return result;
}

std::string StringifyUtf8(const Value& value)
{
    std::wstring wide;
    AppendValue(wide, value);
    return ToUtf8(wide);
}

std::wstring Escape(const std::wstring& text)
{
    std::wstring result;
    AppendEscaped(result, text);
    return result;
}
} // namespace SamandarinJson
