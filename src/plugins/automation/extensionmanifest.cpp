// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Automation Plugin for Open Salamander

    extensionmanifest.cpp
    Strict JSON parser and validated model for Salamatrix extension manifests.
*/

#include <windows.h>
#include "extensionmanifest.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <utility>

namespace
{
    enum JsonType
    {
        JsonNull,
        JsonBoolean,
        JsonNumber,
        JsonString,
        JsonArray,
        JsonObject
    };

    struct JsonValue
    {
        JsonType Type;
        bool Boolean;
        double Number;
        std::string String;
        std::vector<JsonValue> Array;
        std::vector<std::pair<std::string, JsonValue>> Object;

        JsonValue()
            : Type(JsonNull),
              Boolean(false),
              Number(0)
        {
        }

        const JsonValue* Find(const char* name) const
        {
            for (size_t i = 0; i < Object.size(); ++i)
            {
                if (Object[i].first == name)
                    return &Object[i].second;
            }
            return NULL;
        }
    };

    class JsonParser
    {
    private:
        const char* m_begin;
        const char* m_current;
        const char* m_end;
        CExtensionManifestError& m_error;
        bool m_failed;

        void SetError(const char* message)
        {
            if (m_failed)
                return;

            m_failed = true;
            m_error.Offset = static_cast<size_t>(m_current - m_begin);
            m_error.Line = 1;
            m_error.Column = 1;
            for (const char* p = m_begin; p < m_current; ++p)
            {
                if (*p == '\n')
                {
                    ++m_error.Line;
                    m_error.Column = 1;
                }
                else
                {
                    ++m_error.Column;
                }
            }
            m_error.Message = message;
        }

        void SkipWhitespace()
        {
            while (m_current < m_end &&
                   (*m_current == ' ' || *m_current == '\t' ||
                    *m_current == '\r' || *m_current == '\n'))
            {
                ++m_current;
            }
        }

        bool Consume(char expected)
        {
            if (m_current >= m_end || *m_current != expected)
                return false;
            ++m_current;
            return true;
        }

        static int HexDigit(char value)
        {
            if (value >= '0' && value <= '9')
                return value - '0';
            if (value >= 'a' && value <= 'f')
                return value - 'a' + 10;
            if (value >= 'A' && value <= 'F')
                return value - 'A' + 10;
            return -1;
        }

        bool ParseHexQuad(unsigned int& value)
        {
            if (m_end - m_current < 4)
            {
                SetError("Incomplete Unicode escape");
                return false;
            }

            value = 0;
            for (int i = 0; i < 4; ++i)
            {
                int digit = HexDigit(*m_current++);
                if (digit < 0)
                {
                    SetError("Invalid Unicode escape");
                    return false;
                }
                value = (value << 4) | static_cast<unsigned int>(digit);
            }
            return true;
        }

        static void AppendUtf8(std::string& output, unsigned int codePoint)
        {
            if (codePoint <= 0x7f)
            {
                output.push_back(static_cast<char>(codePoint));
            }
            else if (codePoint <= 0x7ff)
            {
                output.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
                output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
            else if (codePoint <= 0xffff)
            {
                output.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
                output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
                output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
            else
            {
                output.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
                output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
                output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
                output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
            }
        }

        bool AppendRawUtf8(std::string& output, unsigned char first)
        {
            int continuationCount;
            unsigned int codePoint;
            unsigned int minimum;
            if ((first & 0xe0) == 0xc0)
            {
                continuationCount = 1;
                codePoint = first & 0x1f;
                minimum = 0x80;
            }
            else if ((first & 0xf0) == 0xe0)
            {
                continuationCount = 2;
                codePoint = first & 0x0f;
                minimum = 0x800;
            }
            else if ((first & 0xf8) == 0xf0)
            {
                continuationCount = 3;
                codePoint = first & 0x07;
                minimum = 0x10000;
            }
            else
            {
                SetError("Invalid UTF-8 leading byte in string");
                return false;
            }

            if (m_end - m_current < continuationCount)
            {
                SetError("Incomplete UTF-8 sequence in string");
                return false;
            }

            output.push_back(static_cast<char>(first));
            for (int i = 0; i < continuationCount; ++i)
            {
                unsigned char continuation =
                    static_cast<unsigned char>(*m_current++);
                if ((continuation & 0xc0) != 0x80)
                {
                    SetError("Invalid UTF-8 continuation byte in string");
                    return false;
                }
                output.push_back(static_cast<char>(continuation));
                codePoint = (codePoint << 6) | (continuation & 0x3f);
            }

            if (codePoint < minimum || codePoint > 0x10ffff ||
                (codePoint >= 0xd800 && codePoint <= 0xdfff))
            {
                SetError("Invalid UTF-8 code point in string");
                return false;
            }
            return true;
        }

        bool ParseString(std::string& output)
        {
            if (!Consume('"'))
            {
                SetError("Expected a JSON string");
                return false;
            }

            output.clear();
            while (m_current < m_end)
            {
                unsigned char value = static_cast<unsigned char>(*m_current++);
                if (value == '"')
                    return true;
                if (value < 0x20)
                {
                    SetError("Unescaped control character in string");
                    return false;
                }
                if (value != '\\')
                {
                    if (value < 0x80)
                        output.push_back(static_cast<char>(value));
                    else if (!AppendRawUtf8(output, value))
                        return false;
                    continue;
                }

                if (m_current >= m_end)
                {
                    SetError("Incomplete string escape");
                    return false;
                }

                char escaped = *m_current++;
                switch (escaped)
                {
                case '"':
                case '\\':
                case '/':
                    output.push_back(escaped);
                    break;
                case 'b':
                    output.push_back('\b');
                    break;
                case 'f':
                    output.push_back('\f');
                    break;
                case 'n':
                    output.push_back('\n');
                    break;
                case 'r':
                    output.push_back('\r');
                    break;
                case 't':
                    output.push_back('\t');
                    break;
                case 'u':
                {
                    unsigned int codePoint;
                    if (!ParseHexQuad(codePoint))
                        return false;

                    if (codePoint >= 0xd800 && codePoint <= 0xdbff)
                    {
                        if (m_end - m_current < 6 || m_current[0] != '\\' || m_current[1] != 'u')
                        {
                            SetError("High surrogate is not followed by a low surrogate");
                            return false;
                        }
                        m_current += 2;
                        unsigned int lowSurrogate;
                        if (!ParseHexQuad(lowSurrogate))
                            return false;
                        if (lowSurrogate < 0xdc00 || lowSurrogate > 0xdfff)
                        {
                            SetError("Invalid low surrogate");
                            return false;
                        }
                        codePoint = 0x10000 +
                                    ((codePoint - 0xd800) << 10) +
                                    (lowSurrogate - 0xdc00);
                    }
                    else if (codePoint >= 0xdc00 && codePoint <= 0xdfff)
                    {
                        SetError("Unexpected low surrogate");
                        return false;
                    }

                    AppendUtf8(output, codePoint);
                    break;
                }
                default:
                    SetError("Unknown string escape");
                    return false;
                }
            }

            SetError("Unterminated JSON string");
            return false;
        }

        bool ParseNumber(JsonValue& value)
        {
            const char* start = m_current;
            if (Consume('-') && m_current == m_end)
            {
                SetError("Incomplete JSON number");
                return false;
            }

            if (Consume('0'))
            {
                if (m_current < m_end && isdigit(static_cast<unsigned char>(*m_current)))
                {
                    SetError("Leading zero in JSON number");
                    return false;
                }
            }
            else
            {
                if (m_current >= m_end || *m_current < '1' || *m_current > '9')
                {
                    SetError("Invalid JSON number");
                    return false;
                }
                while (m_current < m_end && isdigit(static_cast<unsigned char>(*m_current)))
                    ++m_current;
            }

            if (Consume('.'))
            {
                if (m_current >= m_end || !isdigit(static_cast<unsigned char>(*m_current)))
                {
                    SetError("Missing fractional digits");
                    return false;
                }
                while (m_current < m_end && isdigit(static_cast<unsigned char>(*m_current)))
                    ++m_current;
            }

            if (m_current < m_end && (*m_current == 'e' || *m_current == 'E'))
            {
                ++m_current;
                if (m_current < m_end && (*m_current == '+' || *m_current == '-'))
                    ++m_current;
                if (m_current >= m_end || !isdigit(static_cast<unsigned char>(*m_current)))
                {
                    SetError("Missing exponent digits");
                    return false;
                }
                while (m_current < m_end && isdigit(static_cast<unsigned char>(*m_current)))
                    ++m_current;
            }

            std::string numberText(start, m_current);
            errno = 0;
            char* conversionEnd = NULL;
            double number = strtod(numberText.c_str(), &conversionEnd);
            if (errno == ERANGE || conversionEnd == NULL || *conversionEnd != 0 || !_finite(number))
            {
                SetError("JSON number is outside the supported range");
                return false;
            }

            value.Type = JsonNumber;
            value.Number = number;
            return true;
        }

        bool ParseLiteral(const char* literal, JsonType type, bool boolean, JsonValue& value)
        {
            size_t length = strlen(literal);
            if (static_cast<size_t>(m_end - m_current) < length ||
                memcmp(m_current, literal, length) != 0)
            {
                SetError("Invalid JSON literal");
                return false;
            }
            m_current += length;
            value.Type = type;
            value.Boolean = boolean;
            return true;
        }

        bool ParseArray(JsonValue& value, int depth)
        {
            Consume('[');
            value.Type = JsonArray;
            SkipWhitespace();
            if (Consume(']'))
                return true;

            for (;;)
            {
                JsonValue item;
                if (!ParseValue(item, depth + 1))
                    return false;
                value.Array.push_back(item);

                SkipWhitespace();
                if (Consume(']'))
                    return true;
                if (!Consume(','))
                {
                    SetError("Expected ',' or ']' in array");
                    return false;
                }
                SkipWhitespace();
            }
        }

        bool ParseObject(JsonValue& value, int depth)
        {
            Consume('{');
            value.Type = JsonObject;
            SkipWhitespace();
            if (Consume('}'))
                return true;

            for (;;)
            {
                std::string name;
                if (!ParseString(name))
                    return false;
                for (size_t i = 0; i < value.Object.size(); ++i)
                {
                    if (value.Object[i].first == name)
                    {
                        SetError("Duplicate object member");
                        return false;
                    }
                }

                SkipWhitespace();
                if (!Consume(':'))
                {
                    SetError("Expected ':' after object member name");
                    return false;
                }
                SkipWhitespace();

                JsonValue member;
                if (!ParseValue(member, depth + 1))
                    return false;
                value.Object.push_back(std::make_pair(name, member));

                SkipWhitespace();
                if (Consume('}'))
                    return true;
                if (!Consume(','))
                {
                    SetError("Expected ',' or '}' in object");
                    return false;
                }
                SkipWhitespace();
            }
        }

        bool ParseValue(JsonValue& value, int depth)
        {
            if (depth > 64)
            {
                SetError("JSON nesting is too deep");
                return false;
            }
            if (m_current >= m_end)
            {
                SetError("Unexpected end of JSON input");
                return false;
            }

            switch (*m_current)
            {
            case '{':
                return ParseObject(value, depth);
            case '[':
                return ParseArray(value, depth);
            case '"':
                value.Type = JsonString;
                return ParseString(value.String);
            case 't':
                return ParseLiteral("true", JsonBoolean, true, value);
            case 'f':
                return ParseLiteral("false", JsonBoolean, false, value);
            case 'n':
                return ParseLiteral("null", JsonNull, false, value);
            default:
                if (*m_current == '-' || isdigit(static_cast<unsigned char>(*m_current)))
                    return ParseNumber(value);
                SetError("Unexpected character in JSON input");
                return false;
            }
        }

    public:
        JsonParser(const char* json, size_t length, CExtensionManifestError& error)
            : m_begin(json),
              m_current(json),
              m_end(json + length),
              m_error(error),
              m_failed(false)
        {
        }

        bool Parse(JsonValue& value)
        {
            if (static_cast<size_t>(m_end - m_current) >= 3 &&
                static_cast<unsigned char>(m_current[0]) == 0xef &&
                static_cast<unsigned char>(m_current[1]) == 0xbb &&
                static_cast<unsigned char>(m_current[2]) == 0xbf)
            {
                m_current += 3;
            }

            SkipWhitespace();
            if (!ParseValue(value, 0))
                return false;
            SkipWhitespace();
            if (m_current != m_end)
            {
                SetError("Unexpected data after the JSON document");
                return false;
            }
            return true;
        }
    };

    static bool SetValidationError(CExtensionManifestError& error, const char* message)
    {
        error.Offset = 0;
        error.Line = 1;
        error.Column = 1;
        error.Message = message;
        return false;
    }

    static bool ReadString(
        const JsonValue& object,
        const char* name,
        bool required,
        std::string& output,
        CExtensionManifestError& error)
    {
        const JsonValue* value = object.Find(name);
        if (value == NULL)
        {
            if (required)
            {
                std::string message("Missing required string member '");
                message += name;
                message += "'";
                return SetValidationError(error, message.c_str());
            }
            return true;
        }
        if (value->Type != JsonString)
        {
            std::string message("Manifest member '");
            message += name;
            message += "' must be a string";
            return SetValidationError(error, message.c_str());
        }
        output = value->String;
        return true;
    }

    static bool ReadBoolean(
        const JsonValue& object,
        const char* name,
        bool defaultValue,
        bool& output,
        CExtensionManifestError& error)
    {
        const JsonValue* value = object.Find(name);
        if (value == NULL)
        {
            output = defaultValue;
            return true;
        }
        if (value->Type != JsonBoolean)
        {
            std::string message("Manifest member '");
            message += name;
            message += "' must be a boolean";
            return SetValidationError(error, message.c_str());
        }
        output = value->Boolean;
        return true;
    }

    static bool ReadInteger(
        const JsonValue& object,
        const char* name,
        int defaultValue,
        int minimum,
        int maximum,
        int& output,
        CExtensionManifestError& error)
    {
        const JsonValue* value = object.Find(name);
        if (value == NULL)
        {
            output = defaultValue;
            return true;
        }
        if (value->Type != JsonNumber || !isfinite(value->Number) ||
            floor(value->Number) != value->Number ||
            value->Number < minimum || value->Number > maximum)
        {
            std::string message("Manifest member '");
            message += name;
            message += "' must be an integer in the supported range";
            return SetValidationError(error, message.c_str());
        }
        output = static_cast<int>(value->Number);
        return true;
    }

    static bool IsIdentifier(const std::string& identifier)
    {
        if (identifier.empty() || identifier.size() > 127)
            return false;

        for (size_t i = 0; i < identifier.size(); ++i)
        {
            unsigned char value = static_cast<unsigned char>(identifier[i]);
            if (!isalnum(value) && value != '.' && value != '_' && value != '-')
                return false;
        }
        return true;
    }

    static bool ParseRuntimeVersion(
        const JsonValue& value,
        unsigned long& version,
        CExtensionManifestError& error)
    {
        if (value.Type == JsonNumber)
        {
            if (value.Number < 0 || value.Number > 4294967295.0 ||
                floor(value.Number) != value.Number)
            {
                return SetValidationError(error, "Runtime minimumVersion number must be a 32-bit integer");
            }
            version = static_cast<unsigned long>(value.Number);
            return true;
        }
        if (value.Type != JsonString)
            return SetValidationError(error, "Runtime minimumVersion must be a number or 'major.minor' string");

        const char* text = value.String.c_str();
        char* end = NULL;
        errno = 0;
        unsigned long major = strtoul(text, &end, 10);
        if (errno != 0 || end == text || *end != '.' || major > 0xffff)
            return SetValidationError(error, "Invalid runtime minimumVersion");
        const char* minorText = end + 1;
        unsigned long minor = strtoul(minorText, &end, 10);
        if (errno != 0 || end == minorText || *end != 0 || minor > 0xffff)
            return SetValidationError(error, "Invalid runtime minimumVersion");

        version = (major << 16) | minor;
        return true;
    }

    static bool ValidateEnum(
        const std::string& value,
        const char* const* allowed,
        size_t allowedCount,
        const char* member,
        CExtensionManifestError& error)
    {
        for (size_t i = 0; i < allowedCount; ++i)
        {
            if (value == allowed[i])
                return true;
        }
        std::string message("Unsupported value for '");
        message += member;
        message += "'";
        return SetValidationError(error, message.c_str());
    }

    static bool IsSvgAssetPath(const std::string& path)
    {
        size_t dot = path.find_last_of('.');
        return dot != std::string::npos &&
               _stricmp(path.substr(dot).c_str(), ".svg") == 0;
    }

    static bool IsJsonAssetPath(const std::string& path)
    {
        size_t dot = path.find_last_of('.');
        return dot != std::string::npos &&
               _stricmp(path.substr(dot).c_str(), ".json") == 0;
    }

    static bool IsLocaleTag(const std::string& language)
    {
        if (language.empty() || language.size() > 31 ||
            language[0] == '-' || language[language.size() - 1] == '-')
        {
            return false;
        }
        for (size_t i = 0; i < language.size(); ++i)
        {
            unsigned char value = static_cast<unsigned char>(language[i]);
            if (!isalnum(value) && value != '-')
                return false;
        }
        return true;
    }
} // namespace

CExtensionManifest::CExtensionManifest()
{
    Clear();
}

void CExtensionManifest::Clear()
{
    SchemaVersion = 1;
    Id.clear();
    Name.clear();
    Version.clear();
    Description.clear();
    RuntimeId.clear();
    MinimumRuntimeVersion = 0;
    EntryPoint.clear();
    Icon.clear();
    IconDark.clear();
    Capabilities.clear();
    Dependencies.clear();
    Locales.clear();
    Settings.clear();
    EventsDeclared = false;
    Events.clear();
    Commands.clear();
}

bool CExtensionManifest::IsSafeRelativeEntryPoint(const std::string& entryPoint)
{
    if (entryPoint.empty() || entryPoint.size() >= SAL_MAX_PATH ||
        entryPoint[0] == '/' || entryPoint[0] == '\\' ||
        entryPoint.find(':') != std::string::npos)
    {
        return false;
    }

    size_t segmentStart = 0;
    while (segmentStart <= entryPoint.size())
    {
        size_t segmentEnd = entryPoint.find_first_of("/\\", segmentStart);
        if (segmentEnd == std::string::npos)
            segmentEnd = entryPoint.size();
        std::string segment = entryPoint.substr(segmentStart, segmentEnd - segmentStart);
        if (segment.empty() || segment == "." || segment == "..")
            return false;
        if (segmentEnd == entryPoint.size())
            break;
        segmentStart = segmentEnd + 1;
    }
    return true;
}

bool CExtensionManifest::Parse(
    const char* json,
    size_t length,
    CExtensionManifestError& error)
{
    Clear();
    error = CExtensionManifestError();

    if (json == NULL || length == 0)
        return SetValidationError(error, "Manifest is empty");
    if (length > 1024 * 1024)
        return SetValidationError(error, "Manifest is larger than 1 MiB");

    JsonValue root;
    JsonParser parser(json, length, error);
    if (!parser.Parse(root))
        return false;
    if (root.Type != JsonObject)
        return SetValidationError(error, "Manifest root must be a JSON object");

    const JsonValue* schemaVersion = root.Find("schemaVersion");
    if (schemaVersion != NULL)
    {
        if (schemaVersion->Type != JsonNumber ||
            schemaVersion->Number < 1 ||
            schemaVersion->Number > 0xffffffff ||
            floor(schemaVersion->Number) != schemaVersion->Number)
        {
            return SetValidationError(error, "schemaVersion must be a positive integer");
        }
        SchemaVersion = static_cast<unsigned int>(schemaVersion->Number);
    }
    if (SchemaVersion != 1)
        return SetValidationError(error, "Unsupported Salamatrix manifest schemaVersion");

    if (!ReadString(root, "id", true, Id, error) ||
        !ReadString(root, "name", false, Name, error) ||
        !ReadString(root, "version", false, Version, error) ||
        !ReadString(root, "description", false, Description, error) ||
        !ReadString(root, "entryPoint", true, EntryPoint, error))
    {
        return false;
    }

    if (!IsIdentifier(Id))
        return SetValidationError(error, "Manifest id contains unsupported characters or is too long");
    if (Name.empty())
    {
        if (!ReadString(root, "title", false, Name, error))
            return false;
        if (Name.empty())
            Name = Id;
    }
    if (!IsSafeRelativeEntryPoint(EntryPoint))
        return SetValidationError(error, "entryPoint must be a safe relative path inside the extension");

    if (!ReadString(root, "icon", false, Icon, error) ||
        !ReadString(root, "iconDark", false, IconDark, error))
    {
        return false;
    }
    if ((!Icon.empty() && !IsSafeRelativeEntryPoint(Icon)) ||
        (!IconDark.empty() && !IsSafeRelativeEntryPoint(IconDark)))
    {
        return SetValidationError(error, "icon and iconDark must be safe relative paths inside the extension");
    }
    if ((!Icon.empty() && !IsSvgAssetPath(Icon)) ||
        (!IconDark.empty() && !IsSvgAssetPath(IconDark)))
    {
        return SetValidationError(error, "icon and iconDark must point to SVG files");
    }

    const JsonValue* runtime = root.Find("runtime");
    if (runtime == NULL)
        return SetValidationError(error, "Missing required member 'runtime'");
    if (runtime->Type == JsonString)
    {
        RuntimeId = runtime->String;
    }
    else if (runtime->Type == JsonObject)
    {
        if (!ReadString(*runtime, "id", true, RuntimeId, error))
            return false;
        const JsonValue* minimumVersion = runtime->Find("minimumVersion");
        if (minimumVersion != NULL &&
            !ParseRuntimeVersion(*minimumVersion, MinimumRuntimeVersion, error))
        {
            return false;
        }
    }
    else
    {
        return SetValidationError(error, "runtime must be a string or object");
    }
    if (!IsIdentifier(RuntimeId))
        return SetValidationError(error, "Runtime id contains unsupported characters or is too long");

    const JsonValue* capabilities = root.Find("capabilities");
    if (capabilities != NULL)
    {
        if (capabilities->Type != JsonArray)
            return SetValidationError(error, "capabilities must be an array");
        for (size_t i = 0; i < capabilities->Array.size(); ++i)
        {
            if (capabilities->Array[i].Type != JsonString ||
                !IsIdentifier(capabilities->Array[i].String))
            {
                return SetValidationError(error, "Every capability must be a valid string identifier");
            }
            Capabilities.push_back(capabilities->Array[i].String);
        }
    }

    const JsonValue* dependencies = root.Find("dependencies");
    if (dependencies != NULL)
    {
        if (dependencies->Type != JsonArray)
            return SetValidationError(error, "dependencies must be an array");
        if (dependencies->Array.size() > 32)
            return SetValidationError(error, "Manifest contains more than 32 dependencies");
        for (size_t i = 0; i < dependencies->Array.size(); ++i)
        {
            if (dependencies->Array[i].Type != JsonString ||
                !IsIdentifier(dependencies->Array[i].String))
                return SetValidationError(
                    error, "Every dependency must be a valid extension id");
            for (size_t existing = 0; existing < Dependencies.size(); ++existing)
            {
                if (_stricmp(
                        Dependencies[existing].c_str(),
                        dependencies->Array[i].String.c_str()) == 0)
                    return SetValidationError(
                        error, "Dependency ids must be unique inside one manifest");
            }
            Dependencies.push_back(dependencies->Array[i].String);
        }
    }

    const JsonValue* locales = root.Find("locales");
    if (locales != NULL)
    {
        if (locales->Type != JsonObject)
            return SetValidationError(error, "locales must be an object mapping language tags to JSON files");
        if (locales->Object.size() > 32)
            return SetValidationError(error, "Manifest contains more than 32 locales");
        for (size_t i = 0; i < locales->Object.size(); ++i)
        {
            const std::string& language = locales->Object[i].first;
            const JsonValue& fileValue = locales->Object[i].second;
            if (!IsLocaleTag(language) || fileValue.Type != JsonString ||
                !IsSafeRelativeEntryPoint(fileValue.String) ||
                !IsJsonAssetPath(fileValue.String))
            {
                return SetValidationError(
                    error,
                    "Every locale must map a valid language tag to a safe JSON file");
            }
            for (size_t existing = 0; existing < Locales.size(); ++existing)
            {
                if (_stricmp(Locales[existing].Language.c_str(), language.c_str()) == 0)
                    return SetValidationError(
                        error, "Locale language tags must be unique inside one manifest");
            }
            CExtensionManifestLocale locale;
            locale.Language = language;
            locale.File = fileValue.String;
            Locales.push_back(locale);
        }
    }

    const JsonValue* settings = root.Find("settings");
    if (settings != NULL)
    {
        if (settings->Type != JsonArray)
            return SetValidationError(error, "settings must be an array");
        if (settings->Array.size() > 64)
            return SetValidationError(error, "Manifest contains more than 64 settings");

        static const char* const settingTypes[] = {
            "string", "integer", "boolean"};
        for (size_t i = 0; i < settings->Array.size(); ++i)
        {
            const JsonValue& settingValue = settings->Array[i];
            if (settingValue.Type != JsonObject)
                return SetValidationError(error, "Every settings entry must be an object");

            CExtensionManifestSetting setting;
            std::string type;
            if (!ReadString(settingValue, "key", true, setting.Key, error) ||
                !ReadString(settingValue, "type", true, type, error) ||
                !ReadString(settingValue, "label", false, setting.Label, error) ||
                !ReadString(settingValue, "description", false, setting.Description, error) ||
                !ReadString(settingValue, "group", false, setting.Group, error) ||
                !ReadInteger(settingValue, "order", 0, 0, 10000, setting.Order, error) ||
                !ReadInteger(settingValue, "width", 250, 120, 1000, setting.Width, error) ||
                !ReadBoolean(settingValue, "multiline", false, setting.Multiline, error))
                return false;
            if (!IsIdentifier(setting.Key))
                return SetValidationError(error, "Setting key contains unsupported characters or is too long");
            if (setting.Label.empty())
                setting.Label = setting.Key;
            if (setting.Label.size() > 255 || setting.Description.size() > 1023 ||
                setting.Group.size() > 127)
                return SetValidationError(error, "Setting presentation metadata is too long");
            if (!ValidateEnum(
                    type, settingTypes, _countof(settingTypes), "type", error))
                return false;
            if (type == "string")
                setting.Type = ExtensionManifestSettingString;
            else if (type == "integer")
                setting.Type = ExtensionManifestSettingInteger;
            else
                setting.Type = ExtensionManifestSettingBoolean;

            const JsonValue* defaultValue = settingValue.Find("default");
            if (defaultValue != NULL)
            {
                setting.HasDefault = true;
                if (setting.Type == ExtensionManifestSettingString)
                {
                    if (defaultValue->Type != JsonString)
                        return SetValidationError(error, "String setting default must be a string");
                    setting.StringDefault = defaultValue->String;
                }
                else if (setting.Type == ExtensionManifestSettingBoolean)
                {
                    if (defaultValue->Type != JsonBoolean)
                        return SetValidationError(error, "Boolean setting default must be a boolean");
                    setting.BooleanDefault = defaultValue->Boolean;
                }
                else
                {
                    // JSON numbers are parsed as doubles by the strict parser;
                    // accept only precisely representable integral defaults so
                    // the declared value cannot silently change on conversion.
                    if (defaultValue->Type != JsonNumber ||
                        !isfinite(defaultValue->Number) ||
                        floor(defaultValue->Number) != defaultValue->Number ||
                        defaultValue->Number < -9007199254740991.0 ||
                        defaultValue->Number > 9007199254740991.0)
                    {
                        return SetValidationError(
                            error,
                            "Integer setting default must be a precise signed integer");
                    }
                    setting.IntegerDefault =
                        static_cast<long long>(defaultValue->Number);
                }
            }

            for (size_t existing = 0; existing < Settings.size(); ++existing)
            {
                if (_stricmp(Settings[existing].Key.c_str(), setting.Key.c_str()) == 0)
                    return SetValidationError(
                        error, "Setting keys must be unique inside one manifest");
            }
            Settings.push_back(setting);
        }
    }

    const JsonValue* events = root.Find("events");
    if (events != NULL)
    {
        EventsDeclared = true;
        if (events->Type != JsonArray)
            return SetValidationError(error, "events must be an array");
        static const char* const eventNames[] = {
            "hostStartup", "hostShutdown", "settingsChanged",
            "configurationChanged", "colorsChanged", "panelsSwapped",
            "activePanelChanged", "sidePathChanged",
            "sideSelectionChanged", "sideTabChanged", "sideRefreshed",
            "pathChanged", "selectionChanged", "tabChanged",
            "fileChanged"};
        for (size_t i = 0; i < events->Array.size(); ++i)
        {
            if (events->Array[i].Type != JsonString)
                return SetValidationError(error, "Every event declaration must be a string");
            if (!ValidateEnum(events->Array[i].String,
                              eventNames, _countof(eventNames),
                              "events", error))
                return false;
            for (size_t existing = 0; existing < Events.size(); ++existing)
            {
                if (_stricmp(Events[existing].c_str(),
                             events->Array[i].String.c_str()) == 0)
                    return SetValidationError(error,
                                              "Event declarations must be unique");
            }
            Events.push_back(events->Array[i].String);
        }
    }

    const JsonValue* commands = root.Find("commands");
    if (commands != NULL)
    {
        if (commands->Type != JsonArray)
            return SetValidationError(error, "commands must be an array");
        if (commands->Array.size() > 64)
            return SetValidationError(error, "Manifest contains more than 64 commands");

        static const char* const menus[] = {"plugin", "context", "both", "none"};
        static const char* const requirements[] = {"any", "disk", "focused", "file", "selection"};
        for (size_t i = 0; i < commands->Array.size(); ++i)
        {
            const JsonValue& commandValue = commands->Array[i];
            if (commandValue.Type != JsonObject)
                return SetValidationError(error, "Every commands entry must be an object");

            CExtensionManifestCommand command;
            if (!ReadString(commandValue, "id", false, command.Id, error) ||
                !ReadString(commandValue, "title", false, command.Title, error) ||
                !ReadString(commandValue, "handler", false, command.Handler, error) ||
                !ReadString(commandValue, "menu", false, command.Menu, error) ||
                !ReadString(commandValue, "requires", false, command.Requires, error) ||
                !ReadString(commandValue, "icon", false, command.Icon, error) ||
                !ReadString(commandValue, "iconDark", false, command.IconDark, error) ||
                !ReadBoolean(commandValue, "contextMenu", false, command.ContextMenu, error) ||
                !ReadBoolean(commandValue, "toolbar", false, command.Toolbar, error))
            {
                return false;
            }

            std::string placement;
            if (!ReadString(commandValue, "placement", false, placement, error))
                return false;
            if (!placement.empty())
            {
                if (command.Menu != "plugin" && command.Menu != placement)
                    return SetValidationError(error, "Command cannot specify conflicting menu and placement values");
                command.Menu = placement;
            }

            if (command.Id.empty())
                command.Id = Id;
            if (command.Title.empty())
                command.Title = Name;
            if (!IsIdentifier(command.Id))
                return SetValidationError(error, "Command id contains unsupported characters or is too long");
            if ((!command.Icon.empty() && !IsSafeRelativeEntryPoint(command.Icon)) ||
                (!command.IconDark.empty() && !IsSafeRelativeEntryPoint(command.IconDark)) ||
                (!command.Icon.empty() && !IsSvgAssetPath(command.Icon)) ||
                (!command.IconDark.empty() && !IsSvgAssetPath(command.IconDark)))
            {
                return SetValidationError(error, "Command icon and iconDark must be safe relative SVG paths inside the extension");
            }
            if (!ValidateEnum(command.Menu, menus, _countof(menus), "menu", error) ||
                !ValidateEnum(command.Requires, requirements, _countof(requirements), "requires", error))
            {
                return false;
            }
            if (command.ContextMenu && command.Menu == "plugin")
                command.Menu = "both";

            for (size_t existing = 0; existing < Commands.size(); ++existing)
            {
                if (Commands[existing].Id == command.Id)
                    return SetValidationError(error, "Command ids must be unique inside one manifest");
            }
            Commands.push_back(command);
        }
    }

    if (Commands.empty())
    {
        CExtensionManifestCommand defaultCommand;
        defaultCommand.Id = Id;
        defaultCommand.Title = Name;
        Commands.push_back(defaultCommand);
    }
    return true;
}

bool CExtensionManifest::ParseLocaleText(
    const char* json,
    size_t length,
    CExtensionManifestLocaleText& localized,
    CExtensionManifestError& error)
{
    localized = CExtensionManifestLocaleText();
    error = CExtensionManifestError();
    if (json == NULL || length == 0)
        return SetValidationError(error, "Locale resource is empty");
    if (length > 1024 * 1024)
        return SetValidationError(error, "Locale resource is larger than 1 MiB");

    JsonValue root;
    JsonParser parser(json, length, error);
    if (!parser.Parse(root))
        return false;
    if (root.Type != JsonObject)
        return SetValidationError(error, "Locale resource root must be a JSON object");
    if (!ReadString(root, "name", false, localized.Name, error))
    {
        return false;
    }

    const JsonValue* settings = root.Find("settings");
    if (settings != NULL)
    {
        if (settings->Type != JsonObject)
            return SetValidationError(error, "Locale settings must be an object mapping setting keys to metadata");
        if (settings->Object.size() > 64)
            return SetValidationError(error, "Locale resource contains more than 64 setting labels");
        for (size_t i = 0; i < settings->Object.size(); ++i)
        {
            const std::string& key = settings->Object[i].first;
            const JsonValue& value = settings->Object[i].second;
            if (!IsIdentifier(key))
                return SetValidationError(error, "Locale setting keys must be valid identifiers");
            CExtensionManifestLocalizedSetting setting;
            setting.Key = key;
            if (value.Type == JsonString)
            {
                setting.Label = value.String;
            }
            else if (value.Type == JsonObject)
            {
                if (!ReadString(value, "label", false, setting.Label, error) ||
                    !ReadString(value, "description", false, setting.Description, error) ||
                    !ReadString(value, "group", false, setting.Group, error))
                    return false;
            }
            else
            {
                return SetValidationError(error, "Locale setting metadata must be a string or object");
            }
            if (setting.Label.empty() && setting.Description.empty() && setting.Group.empty())
                return SetValidationError(error, "Locale setting metadata cannot be empty");
            if (setting.Label.size() > 255 || setting.Description.size() > 1023 ||
                setting.Group.size() > 127)
                return SetValidationError(error, "Locale setting metadata is too long");
            for (size_t existing = 0; existing < localized.Settings.size(); ++existing)
            {
                if (_stricmp(localized.Settings[existing].Key.c_str(), key.c_str()) == 0)
                    return SetValidationError(error, "Locale setting keys must be unique");
            }
            localized.Settings.push_back(setting);
        }
    }

    const JsonValue* commands = root.Find("commands");
    if (commands == NULL)
        return true;
    if (commands->Type != JsonObject)
        return SetValidationError(error, "Locale commands must be an object mapping command ids to titles");
    if (commands->Object.size() > 64)
        return SetValidationError(error, "Locale resource contains more than 64 command titles");
    for (size_t i = 0; i < commands->Object.size(); ++i)
    {
        const std::string& id = commands->Object[i].first;
        const JsonValue& title = commands->Object[i].second;
        if (!IsIdentifier(id) || title.Type != JsonString)
            return SetValidationError(error, "Locale command ids must be valid and titles must be strings");
        for (size_t existing = 0; existing < localized.Commands.size(); ++existing)
        {
            if (_stricmp(localized.Commands[existing].Id.c_str(), id.c_str()) == 0)
                return SetValidationError(error, "Locale command ids must be unique");
        }
        CExtensionManifestLocalizedCommand command;
        command.Id = id;
        command.Title = title.String;
        localized.Commands.push_back(command);
    }
    return true;
}
