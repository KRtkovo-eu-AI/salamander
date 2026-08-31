// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "../../audio_metadata_legacy.h"

#include <string>
#include <vector>

#include <fileref.h>
#include <tpropertymap.h>

#include "taglibparser.h"
#include "output.h"
char* LoadStr(int resID);
#include "mmviewer.rh2"

namespace
{
std::wstring PathToWide(const char* path)
{
    if (path == NULL || path[0] == 0)
        return std::wstring();

    UINT codePage = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int required = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    if (required <= 0)
    {
        codePage = CP_ACP;
        flags = 0;
        required = MultiByteToWideChar(codePage, flags, path, -1, NULL, 0);
    }
    if (required <= 0)
        return std::wstring();

    std::wstring result(static_cast<size_t>(required), L'\0');
    if (MultiByteToWideChar(codePage, flags, path, -1, &result[0], required) <= 0)
        return std::wstring();
    result.resize(static_cast<size_t>(required - 1));
    return result;
}

std::wstring MakeExtendedPath(const wchar_t* path)
{
    if (path == NULL)
        return std::wstring();

    std::wstring result(path);
    if (result.compare(0, 4, L"\\\\?\\") == 0 || result.length() < MAX_PATH)
        return result;
    if (result.compare(0, 2, L"\\\\") == 0)
        return L"\\\\?\\UNC\\" + result.substr(2);
    return L"\\\\?\\" + result;
}

std::string ToUtf8(const TagLib::String& value)
{
    return value.to8Bit(true);
}

std::string JoinValues(const TagLib::StringList& values)
{
    return ToUtf8(values.toString(TagLib::String("; ", TagLib::String::UTF8)));
}

int PropertyLabel(const TagLib::String& key)
{
    if (key == "TITLE") return IDS_OGG_TITLE;
    if (key == "ARTIST") return IDS_OGG_AUTHOR;
    if (key == "ALBUM") return IDS_OGG_ALBUM;
    if (key == "TRACKNUMBER") return IDS_OGG_TRACK;
    if (key == "DATE" || key == "ORIGINALDATE") return IDS_OGG_DATE;
    if (key == "GENRE") return IDS_OGG_GENRE;
    if (key == "COMMENT") return IDS_OGG_COMMENTS;
    if (key == "COPYRIGHT") return IDS_OGG_COPYRIGHT;
    if (key == "PERFORMER") return IDS_OGG_PERFORMER;
    if (key == "DESCRIPTION") return IDS_OGG_DESCRIPTION;
    if (key == "ISRC") return IDS_OGG_ISRC;
    if (key == "COMPOSER") return IDS_MP3_COMPOSER;
    if (key == "ENCODEDBY") return IDS_MP3_ENCODEDBY;
    return 0;
}

bool ContainsKey(const std::vector<std::string>& keys, const std::string& key)
{
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}

void AddProperty(COutputInterface* output, const std::string& key, const std::string& value)
{
    const TagLib::String tagKey(key, TagLib::String::UTF8);
    const int label = PropertyLabel(tagKey);
    if (label != 0)
        output->AddItem(LoadStr(label), value.c_str());
    else
        output->AddItem(key.c_str(), value.c_str());
}

void FormatDuration(int milliseconds, char* text, size_t textSize)
{
    const int totalSeconds = milliseconds / 1000;
    if (totalSeconds / 3600 != 0)
        _snprintf_s(text, textSize, _TRUNCATE, "%02d:%02d:%02d", totalSeconds / 3600,
                    totalSeconds / 60 % 60, totalSeconds % 60);
    else
        _snprintf_s(text, textSize, _TRUNCATE, "%02d:%02d", totalSeconds / 60, totalSeconds % 60);
}
}

CParserTagLib::CParserTagLib()
{
}

CParserResultEnum CParserTagLib::OpenFile(const char* fileName)
{
    const std::wstring wideName = PathToWide(fileName);
    FileName = MakeExtendedPath(wideName.c_str());
    if (FileName.empty())
        return preOpenError;

    TagLib::FileRef file(FileName.c_str(), true, TagLib::AudioProperties::Average);
    if (file.isNull() || file.file() == NULL)
    {
        FileName.clear();
        return preUnknownFile;
    }
    return preOK;
}

CParserResultEnum CParserTagLib::CloseFile()
{
    FileName.clear();
    return preOK;
}

CParserResultEnum CParserTagLib::GetFileInfo(COutputInterface* output)
{
    if (FileName.empty())
        return preOpenError;

    TagLib::FileRef file(FileName.c_str(), true, TagLib::AudioProperties::Average);
    if (file.isNull() || file.file() == NULL)
        return preReadError;

    output->AddHeader(LoadStr(IDS_OGG_INFO));
    TagLib::AudioProperties* audio = file.audioProperties();
    if (audio != NULL)
    {
        char value[64];
        FormatDuration(audio->lengthInMilliseconds(), value, _countof(value));
        output->AddItem(LoadStr(IDS_OGG_LENGTH), value);
        _snprintf_s(value, _countof(value), _TRUNCATE, "%d", audio->bitrate());
        output->AddItem(LoadStr(IDS_OGG_BITRATE), value);
        _snprintf_s(value, _countof(value), _TRUNCATE, "%d", audio->sampleRate());
        output->AddItem(LoadStr(IDS_OGG_FREQUENCY), value);
        _snprintf_s(value, _countof(value), _TRUNCATE, "%d", audio->channels());
        output->AddItem(LoadStr(IDS_OGG_CHANNELS), value);
    }

    const TagLib::PropertyMap properties = file.properties();
    SalLegacyAudioMetadata::CTextProperties legacyProperties;
    SalLegacyAudioMetadata::ReadOggTextProperties(FileName.c_str(), legacyProperties);
    if (!properties.isEmpty() || !legacyProperties.empty())
    {
        output->AddSeparator();
        output->AddHeader(LoadStr(IDS_OGG_STDTAGS));
        std::vector<std::string> emittedKeys;
        for (TagLib::PropertyMap::ConstIterator it = properties.begin(); it != properties.end(); ++it)
        {
            const std::string value = JoinValues(it->second);
            if (value.empty())
                continue;
            const std::string key = ToUtf8(it->first);
            AddProperty(output, key, value);
            emittedKeys.push_back(key);
        }
        for (SalLegacyAudioMetadata::CTextProperties::const_iterator it = legacyProperties.begin();
             it != legacyProperties.end(); ++it)
        {
            if (!ContainsKey(emittedKeys, it->first))
                AddProperty(output, it->first, it->second);
        }
    }
    return preOK;
}
