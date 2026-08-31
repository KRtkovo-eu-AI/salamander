// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace SalLegacyAudioMetadata
{
typedef std::vector<std::pair<std::string, std::string> > CTextProperties;

static bool ReadExact(HANDLE file, void* buffer, DWORD size)
{
    BYTE* output = static_cast<BYTE*>(buffer);
    while (size != 0)
    {
        DWORD read = 0;
        if (!::ReadFile(file, output, size, &read, NULL) || read == 0)
            return false;
        output += read;
        size -= read;
    }
    return true;
}

static DWORD ReadLittleEndian32(const BYTE* data)
{
    return static_cast<DWORD>(data[0]) |
           (static_cast<DWORD>(data[1]) << 8) |
           (static_cast<DWORD>(data[2]) << 16) |
           (static_cast<DWORD>(data[3]) << 24);
}

static bool IsValidUtf8(const char* text, int length)
{
    return length == 0 || MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, length, NULL, 0) > 0;
}

static bool ConvertToUtf8(const char* text, int length, std::string& utf8)
{
    utf8.clear();
    if (length < 0)
        return false;
    if (IsValidUtf8(text, length))
    {
        utf8.assign(text, static_cast<size_t>(length));
        return true;
    }

    const int wideLength = MultiByteToWideChar(1250, 0, text, length, NULL, 0);
    if (wideLength <= 0)
        return false;
    std::vector<wchar_t> wide(static_cast<size_t>(wideLength));
    if (MultiByteToWideChar(1250, 0, text, length, &wide[0], wideLength) != wideLength)
        return false;
    const int utf8Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wide[0], wideLength,
                                               NULL, 0, NULL, NULL);
    if (utf8Length <= 0)
        return false;
    utf8.resize(static_cast<size_t>(utf8Length));
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wide[0], wideLength,
                               &utf8[0], utf8Length, NULL, NULL) == utf8Length;
}

static bool ParseCommentPacket(const std::vector<BYTE>& packet, CTextProperties& properties)
{
    size_t position = 0;
    if (packet.size() >= 7 && packet[0] == 3 && memcmp(&packet[1], "vorbis", 6) == 0)
        position = 7;
    else if (packet.size() >= 8 && memcmp(&packet[0], "OpusTags", 8) == 0)
        position = 8;
    else
        return false;

    if (position + 4 > packet.size())
        return false;
    const DWORD vendorLength = ReadLittleEndian32(&packet[position]);
    position += 4;
    if (vendorLength > packet.size() - position)
        return false;
    position += vendorLength;
    if (position + 4 > packet.size())
        return false;
    const DWORD count = ReadLittleEndian32(&packet[position]);
    position += 4;
    if (count > 100000)
        return false;

    for (DWORD i = 0; i < count; ++i)
    {
        if (position + 4 > packet.size())
            return false;
        const DWORD length = ReadLittleEndian32(&packet[position]);
        position += 4;
        if (length > packet.size() - position)
            return false;
        const char* comment = reinterpret_cast<const char*>(&packet[position]);
        const char* separator = static_cast<const char*>(memchr(comment, '=', length));
        if (separator != NULL && separator != comment)
        {
            std::string key(comment, separator);
            bool validKey = true;
            for (size_t j = 0; j < key.size(); ++j)
            {
                const unsigned char ch = static_cast<unsigned char>(key[j]);
                if (ch < 0x20 || ch > 0x7e)
                {
                    validKey = false;
                    break;
                }
                key[j] = static_cast<char>(toupper(ch));
            }
            std::string value;
            const int valueLength = static_cast<int>(length - (separator - comment) - 1);
            if (validKey && ConvertToUtf8(separator + 1, valueLength, value) && !value.empty())
                properties.push_back(std::make_pair(key, value));
        }
        position += length;
    }
    return true;
}

static bool ReadOggTextProperties(const wchar_t* fileName, CTextProperties& properties)
{
    properties.clear();
    if (fileName == NULL || fileName[0] == 0)
        return false;
    std::wstring extendedName(fileName);
    if (extendedName.compare(0, 4, L"\\\\?\\") != 0 && extendedName.length() >= MAX_PATH)
    {
        if (extendedName.compare(0, 2, L"\\\\") == 0)
            extendedName = L"\\\\?\\UNC\\" + extendedName.substr(2);
        else
            extendedName = L"\\\\?\\" + extendedName;
    }
    HANDLE file = CreateFileW(extendedName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;

    bool result = false;
    std::vector<BYTE> packet;
    for (unsigned int page = 0; page < 4096 && !result; ++page)
    {
        BYTE header[27];
        if (!ReadExact(file, header, sizeof(header)) || memcmp(header, "OggS", 4) != 0 || header[4] != 0)
            break;
        const BYTE segmentCount = header[26];
        std::vector<BYTE> segments(segmentCount);
        if (segmentCount != 0 && !ReadExact(file, &segments[0], segmentCount))
            break;
        size_t bodySize = 0;
        for (size_t i = 0; i < segments.size(); ++i)
            bodySize += segments[i];
        std::vector<BYTE> body(bodySize);
        if (bodySize != 0 && !ReadExact(file, &body[0], static_cast<DWORD>(bodySize)))
            break;

        size_t bodyPosition = 0;
        for (size_t i = 0; i < segments.size(); ++i)
        {
            const size_t segmentLength = segments[i];
            if (packet.size() + segmentLength > 16 * 1024 * 1024)
            {
                packet.clear();
                break;
            }
            packet.insert(packet.end(), body.begin() + bodyPosition, body.begin() + bodyPosition + segmentLength);
            bodyPosition += segmentLength;
            if (segmentLength < 255)
            {
                result = ParseCommentPacket(packet, properties);
                packet.clear();
                if (result)
                    break;
            }
        }
    }
    CloseHandle(file);
    return result;
}

static const std::string* FindProperty(const CTextProperties& properties, const char* key)
{
    for (CTextProperties::const_iterator it = properties.begin(); it != properties.end(); ++it)
        if (_stricmp(it->first.c_str(), key) == 0)
            return &it->second;
    return NULL;
}
}
