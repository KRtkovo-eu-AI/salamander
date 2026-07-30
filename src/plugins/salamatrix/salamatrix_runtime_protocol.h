// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

/*
    Salamatrix Framework for Open Salamander

    salamatrix_runtime_protocol.h
    Versioned line framing for out-of-process runtime workers.
*/

#pragma once

#include <windows.h>

#include <errno.h>
#include <stdlib.h>
#include <string>

namespace Salamatrix
{
namespace Runtime
{
namespace Protocol
{

static const DWORD ProtocolVersion1 = 1;
static const size_t MaxFrameBytes = 1024 * 1024;

namespace Json
{
inline void SkipWhitespace(const std::string& json, size_t* position)
{
    while (*position < json.size() &&
           (json[*position] == ' ' || json[*position] == '\t' ||
            json[*position] == '\r' || json[*position] == '\n'))
    {
        ++*position;
    }
}

inline BOOL ReadString(
    const std::string& json,
    size_t* position,
    std::string* value)
{
    if (position == NULL || value == NULL || *position >= json.size() ||
        json[*position] != '"')
        return FALSE;
    ++*position;
    value->clear();
    while (*position < json.size())
    {
        unsigned char character =
            static_cast<unsigned char>(json[*position]);
        ++*position;
        if (character == '"')
            return TRUE;
        if (character < 0x20)
            return FALSE;
        if (character != '\\')
        {
            value->push_back(static_cast<char>(character));
            continue;
        }
        if (*position >= json.size())
            return FALSE;
        char escaped = json[*position];
        ++*position;
        switch (escaped)
        {
        case '"':
        case '\\':
        case '/':
            value->push_back(escaped);
            break;
        case 'b':
            value->push_back('\b');
            break;
        case 'f':
            value->push_back('\f');
            break;
        case 'n':
            value->push_back('\n');
            break;
        case 'r':
            value->push_back('\r');
            break;
        case 't':
            value->push_back('\t');
            break;
        case 'u':
            // The dispatcher only extracts ASCII protocol keys/values. Reject
            // escaped Unicode here instead of silently corrupting UTF-16.
            return FALSE;
        default:
            return FALSE;
        }
    }
    return FALSE;
}

inline BOOL SkipValue(const std::string& json, size_t* position)
{
    SkipWhitespace(json, position);
    if (*position >= json.size())
        return FALSE;
    if (json[*position] == '"')
    {
        std::string ignored;
        return ReadString(json, position, &ignored);
    }
    if (json[*position] == '{' || json[*position] == '[')
    {
        char opening = json[*position];
        char closing = opening == '{' ? '}' : ']';
        ++*position;
        SkipWhitespace(json, position);
        if (*position < json.size() && json[*position] == closing)
        {
            ++*position;
            return TRUE;
        }
        for (;;)
        {
            if (opening == '{')
            {
                std::string ignoredKey;
                if (!ReadString(json, position, &ignoredKey))
                    return FALSE;
                SkipWhitespace(json, position);
                if (*position >= json.size() || json[*position] != ':')
                    return FALSE;
                ++*position;
            }
            if (!SkipValue(json, position))
                return FALSE;
            SkipWhitespace(json, position);
            if (*position >= json.size())
                return FALSE;
            if (json[*position] == closing)
            {
                ++*position;
                return TRUE;
            }
            if (json[*position] != ',')
                return FALSE;
            ++*position;
            SkipWhitespace(json, position);
        }
    }

    size_t start = *position;
    while (*position < json.size() && json[*position] != ',' &&
           json[*position] != '}' && json[*position] != ']')
    {
        ++*position;
    }
    return *position > start;
}

inline BOOL FindStringMember(
    const char* jsonText,
    const char* member,
    std::string* value)
{
    if (jsonText == NULL || member == NULL || value == NULL)
        return FALSE;
    std::string json(jsonText);
    size_t position = 0;
    SkipWhitespace(json, &position);
    if (position >= json.size() || json[position] != '{')
        return FALSE;
    ++position;
    SkipWhitespace(json, &position);
    if (position < json.size() && json[position] == '}')
        return FALSE;
    for (;;)
    {
        std::string key;
        if (!ReadString(json, &position, &key))
            return FALSE;
        SkipWhitespace(json, &position);
        if (position >= json.size() || json[position] != ':')
            return FALSE;
        ++position;
        SkipWhitespace(json, &position);
        if (key == member)
        {
            return ReadString(json, &position, value);
        }
        if (!SkipValue(json, &position))
            return FALSE;
        SkipWhitespace(json, &position);
        if (position >= json.size())
            return FALSE;
        if (json[position] == '}')
            return FALSE;
        if (json[position] != ',')
            return FALSE;
        ++position;
        SkipWhitespace(json, &position);
    }
}

/// Returns the raw JSON value for an object member. This is intentionally
/// small and bounded like FindStringMember, but preserves nested objects and
/// arrays so host calls can carry runtime-neutral context without converting
/// it through a language-specific string representation.
inline BOOL FindRawMember(
    const char* jsonText,
    const char* member,
    std::string* value)
{
    if (jsonText == NULL || member == NULL || value == NULL)
        return FALSE;
    std::string json(jsonText);
    size_t position = 0;
    SkipWhitespace(json, &position);
    if (position >= json.size() || json[position] != '{')
        return FALSE;
    ++position;
    SkipWhitespace(json, &position);
    if (position < json.size() && json[position] == '}')
        return FALSE;
    for (;;)
    {
        std::string key;
        if (!ReadString(json, &position, &key))
            return FALSE;
        SkipWhitespace(json, &position);
        if (position >= json.size() || json[position] != ':')
            return FALSE;
        ++position;
        SkipWhitespace(json, &position);
        size_t valueStart = position;
        if (!SkipValue(json, &position))
            return FALSE;
        if (key == member)
        {
            size_t valueEnd = position;
            while (valueEnd > valueStart &&
                   (json[valueEnd - 1] == ' ' ||
                    json[valueEnd - 1] == '\t' ||
                    json[valueEnd - 1] == '\r' ||
                    json[valueEnd - 1] == '\n'))
                --valueEnd;
            value->assign(json, valueStart, valueEnd - valueStart);
            return TRUE;
        }
        SkipWhitespace(json, &position);
        if (position >= json.size() || json[position] == '}' ||
            json[position] != ',')
            return FALSE;
        ++position;
        SkipWhitespace(json, &position);
    }
}

inline BOOL FindBoolMember(
    const char* jsonText,
    const char* member,
    BOOL* value)
{
    if (jsonText == NULL || member == NULL || value == NULL)
        return FALSE;
    std::string raw;
    if (!FindRawMember(jsonText, member, &raw))
        return FALSE;
    if (raw == "true")
    {
        *value = TRUE;
        return TRUE;
    }
    if (raw == "false")
    {
        *value = FALSE;
        return TRUE;
    }
    return FALSE;
}

inline BOOL FindIntegerMember(
    const char* jsonText,
    const char* member,
    int* value)
{
    if (jsonText == NULL || member == NULL || value == NULL)
        return FALSE;
    std::string raw;
    if (!FindRawMember(jsonText, member, &raw) || raw.empty())
        return FALSE;
    size_t first = 0;
    while (first < raw.size() &&
           (raw[first] == ' ' || raw[first] == '\t' ||
            raw[first] == '\r' || raw[first] == '\n'))
        ++first;
    size_t last = raw.size();
    while (last > first &&
           (raw[last - 1] == ' ' || raw[last - 1] == '\t' ||
            raw[last - 1] == '\r' || raw[last - 1] == '\n'))
        --last;
    if (first != 0 || last != raw.size())
        raw = raw.substr(first, last - first);
    if (raw.empty())
        return FALSE;
    size_t index = 0;
    BOOL negative = FALSE;
    if (raw[index] == '-')
    {
        negative = TRUE;
        ++index;
    }
    if (index >= raw.size())
        return FALSE;
    long long parsed = 0;
    const long long limit = negative ? 2147483648LL : 2147483647LL;
    for (; index < raw.size(); ++index)
    {
        if (raw[index] < '0' || raw[index] > '9')
            return FALSE;
        const long long digit = raw[index] - '0';
        if (parsed > (limit - digit) / 10)
            return FALSE;
        parsed = parsed * 10 + digit;
    }
    if (negative && parsed == 2147483648LL)
        *value = (-2147483647 - 1);
    else
        *value = negative ? -static_cast<int>(parsed)
                          : static_cast<int>(parsed);
    return TRUE;
}

// Runtime storage values use signed 64-bit integers.  Keep this parser
// separate from FindIntegerMember, whose historic contract intentionally
// limits results to a Win32 int for UI/control parameters.
inline BOOL FindInteger64Member(
    const char* jsonText,
    const char* member,
    LONGLONG* value)
{
    if (jsonText == NULL || member == NULL || value == NULL)
        return FALSE;
    std::string raw;
    if (!FindRawMember(jsonText, member, &raw) || raw.empty())
        return FALSE;

    size_t first = 0;
    while (first < raw.size() &&
           (raw[first] == ' ' || raw[first] == '\t' ||
            raw[first] == '\r' || raw[first] == '\n'))
        ++first;
    size_t last = raw.size();
    while (last > first &&
           (raw[last - 1] == ' ' || raw[last - 1] == '\t' ||
            raw[last - 1] == '\r' || raw[last - 1] == '\n'))
        --last;
    if (first != 0 || last != raw.size())
        raw = raw.substr(first, last - first);
    if (raw.empty())
        return FALSE;

    errno = 0;
    char* end = NULL;
    long long parsed = _strtoi64(raw.c_str(), &end, 10);
    if (end == raw.c_str() || *end != '\0' || errno == ERANGE)
        return FALSE;
    *value = static_cast<LONGLONG>(parsed);
    return TRUE;
}
} // namespace Json

enum MessageType
{
    MessageHello,
    MessageReady,
    MessageCall,
    MessageResult,
    MessageEvent,
    MessageShutdown,
    MessageError
};

struct Frame
{
    MessageType Type;
    ULONGLONG Id;
    std::string PayloadJson;

    Frame()
        : Type(MessageError),
          Id(0)
    {
    }
};

/// A deliberately dumb envelope around JSON payloads. Keeping framing
/// independent from JSON parsing lets Python, PowerShell, PHP, and a future
/// JavaScript worker use their native JSON libraries while the host enforces
/// protocol version, message type, request id, line boundaries, and a hard
/// memory limit. The wire format is:
///
///   SMX1<TAB><type><TAB><decimal-id><TAB><compact-json><LF>
///
/// Payloads may not contain literal CR/LF; JSON strings must use escapes.
class LineCodec
{
private:
    std::string Pending;

    static const char* TypeName(MessageType type)
    {
        switch (type)
        {
        case MessageHello:
            return "hello";
        case MessageReady:
            return "ready";
        case MessageCall:
            return "call";
        case MessageResult:
            return "result";
        case MessageEvent:
            return "event";
        case MessageShutdown:
            return "shutdown";
        default:
            return "error";
        }
    }

    static BOOL ParseType(const std::string& name, MessageType* type)
    {
        if (name == "hello")
            *type = MessageHello;
        else if (name == "ready")
            *type = MessageReady;
        else if (name == "call")
            *type = MessageCall;
        else if (name == "result")
            *type = MessageResult;
        else if (name == "event")
            *type = MessageEvent;
        else if (name == "shutdown")
            *type = MessageShutdown;
        else if (name == "error")
            *type = MessageError;
        else
            return FALSE;
        return TRUE;
    }

    static BOOL ParseUnsigned(const std::string& value, ULONGLONG* result)
    {
        if (value.empty() || result == NULL)
            return FALSE;
        ULONGLONG parsed = 0;
        for (size_t index = 0; index < value.size(); ++index)
        {
            char digit = value[index];
            if (digit < '0' || digit > '9')
                return FALSE;
            ULONGLONG next = parsed * 10 +
                             static_cast<ULONGLONG>(digit - '0');
            if (next < parsed)
                return FALSE;
            parsed = next;
        }
        *result = parsed;
        return TRUE;
    }

public:
    static BOOL Encode(
        MessageType type,
        ULONGLONG id,
        const std::string& payloadJson,
        std::string* line)
    {
        if (line == NULL || payloadJson.empty() ||
            payloadJson.find('\r') != std::string::npos ||
            payloadJson.find('\n') != std::string::npos ||
            payloadJson.size() > MaxFrameBytes)
        {
            return FALSE;
        }

        char idText[32];
        _ui64toa_s(id, idText, _countof(idText), 10);
        line->assign("SMX1\t");
        line->append(TypeName(type));
        line->push_back('\t');
        line->append(idText);
        line->push_back('\t');
        line->append(payloadJson);
        line->push_back('\n');
        return line->size() <= MaxFrameBytes;
    }

    /// Appends arbitrary bytes and extracts at most one complete frame. The
    /// caller can invoke it repeatedly until it returns FALSE with `complete`
    /// set to FALSE. A malformed/oversized frame clears the buffer and returns
    /// FALSE with `complete` set to TRUE, allowing the caller to fail closed.
    BOOL Append(
        const char* bytes,
        size_t count,
        Frame* frame,
        BOOL* complete)
    {
        if (complete != NULL)
            *complete = FALSE;
        if (bytes == NULL || count == 0 || frame == NULL || complete == NULL)
            return FALSE;
        if (Pending.size() + count > MaxFrameBytes)
        {
            Pending.clear();
            *complete = TRUE;
            return FALSE;
        }
        Pending.append(bytes, count);
        std::string::size_type newline = Pending.find('\n');
        if (newline == std::string::npos)
            return TRUE;

        std::string line = Pending.substr(0, newline);
        Pending.erase(0, newline + 1);
        *complete = TRUE;
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.resize(line.size() - 1);

        std::string::size_type firstTab = line.find('\t');
        std::string::size_type secondTab =
            firstTab == std::string::npos ? std::string::npos
                                          : line.find('\t', firstTab + 1);
        std::string::size_type thirdTab =
            secondTab == std::string::npos ? std::string::npos
                                           : line.find('\t', secondTab + 1);
        if (firstTab != 4 || secondTab == std::string::npos ||
            thirdTab == std::string::npos || line.compare(0, 4, "SMX1") != 0)
        {
            return FALSE;
        }

        MessageType type;
        ULONGLONG id = 0;
        if (!ParseType(line.substr(firstTab + 1, secondTab - firstTab - 1), &type) ||
            !ParseUnsigned(
                line.substr(secondTab + 1, thirdTab - secondTab - 1), &id))
        {
            return FALSE;
        }

        std::string payload = line.substr(thirdTab + 1);
        if (payload.empty() || payload.size() > MaxFrameBytes)
            return FALSE;
        frame->Type = type;
        frame->Id = id;
        frame->PayloadJson.swap(payload);
        return TRUE;
    }

    size_t PendingBytes() const
    {
        return Pending.size();
    }
};

// Stable method names used in payload JSON. They are intentionally namespaced
// strings rather than C++ class names so every runtime can implement them.
static const char HostHelloMethod[] = "host.hello";
static const char HostCallMethod[] = "host.call";
static const char HostEventMethod[] = "host.event";
static const char HostShutdownMethod[] = "host.shutdown";
static const char RuntimeReadyMethod[] = "runtime.ready";
static const char RuntimeResultMethod[] = "runtime.result";

} // namespace Protocol
} // namespace Runtime
} // namespace Salamatrix
