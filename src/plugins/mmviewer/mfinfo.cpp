// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Include the shell header that declares the property-store call. | The Windows 7 plugin target does not see that call from propsys.h. | https://github.com/KRtkovo-eu-AI/salamander/issues/732 | Grok 4.7 |
// | 2026-11-04 | Read video and audio stream details with Media Foundation. | Multimedia Viewer showed only the audio part of video files. | https://github.com/KRtkovo-eu-AI/salamander/issues/732 | Grok 4.7 |

#include "precomp.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <shobjidl.h>
#include <propsys.h>
#include <propkey.h>

#include <string.h>

#include <string>
#include <vector>

#include "mfinfo.h"
#include "output.h"
#include "mmviewer.rh2"

extern char* LoadStr(int resID);

namespace
{
const unsigned int kH264FourCC = 0x34363248; // H264

// These subtypes are not FOURCC media-type GUIDs. Values match mfapi.h.
const GUID kSubtypeMpeg2 = {0xe06d8026, 0xdb46, 0x11cf, {0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea}};
const GUID kSubtypeH264Es = {0x3f40f4f0, 0x5622, 0x4ff8, {0xb6, 0xd8, 0xa1, 0x7a, 0x58, 0x4b, 0xee, 0x5e}};
const GUID kSubtypeAc3 = {0xe06d802c, 0xdb46, 0x11cf, {0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea}};
const GUID kSubtypeDdPlus = {0xa7fb87af, 0x2d02, 0x42fb, {0xa4, 0xd4, 0x05, 0xcd, 0x93, 0x84, 0x3b, 0xdd}};
const GUID kSubtypeVorbis = {0x8d2fd10b, 0x5841, 0x4a6b, {0x89, 0x05, 0x58, 0x8f, 0xec, 0x1a, 0xde, 0xd9}};
const GUID kSubtypeDtsRaw = {0xe06d8033, 0xdb46, 0x11cf, {0xb4, 0xd1, 0x00, 0x80, 0x5f, 0x6c, 0xbb, 0xea}};
const GUID kVideoRotation = {0xc380465d, 0x2271, 0x428c, {0x9b, 0x83, 0xec, 0xea, 0x3b, 0x4a, 0x85, 0xc1}};

bool SameGuid(const GUID& left, const GUID& right)
{
    return IsEqualGUID(left, right) == TRUE;
}

bool IsStandardMediaSubtype(const GUID& guid)
{
    static const unsigned char tail[8] = {0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
    return guid.Data2 == 0x0000 && guid.Data3 == 0x0010 && memcmp(guid.Data4, tail, 8) == 0;
}

char AsciiUpper(char value)
{
    if (value >= 'a' && value <= 'z')
        return (char)(value - 'a' + 'A');
    return value;
}

std::string FourCCText(unsigned int tag)
{
    char text[5];
    text[0] = (char)(tag & 0xFF);
    text[1] = (char)((tag >> 8) & 0xFF);
    text[2] = (char)((tag >> 16) & 0xFF);
    text[3] = (char)((tag >> 24) & 0xFF);
    text[4] = 0;
    for (int i = 0; i < 4; i++)
    {
        const unsigned char c = (unsigned char)text[i];
        if (c < 32 || c > 126)
            return std::string();
    }
    while (text[0] != 0 && text[strlen(text) - 1] == ' ')
        text[strlen(text) - 1] = 0;
    return std::string(text);
}

bool FourCCIs(unsigned int tag, const char* text)
{
    const std::string value = FourCCText(tag);
    if (value.size() != strlen(text))
        return false;
    for (size_t i = 0; i < value.size(); i++)
    {
        if (AsciiUpper(value[i]) != AsciiUpper(text[i]))
            return false;
    }
    return true;
}

bool IsH264Tag(unsigned int tag)
{
    return FourCCIs(tag, "H264") || FourCCIs(tag, "X264") || FourCCIs(tag, "AVC1") || FourCCIs(tag, "AVC3");
}

bool IsHevcTag(unsigned int tag)
{
    return FourCCIs(tag, "HEVC") || FourCCIs(tag, "H265") || FourCCIs(tag, "HEV1") || FourCCIs(tag, "HVC1") ||
           FourCCIs(tag, "HEVS");
}

const char* FriendlyVideoName(unsigned int tag)
{
    if (IsH264Tag(tag))
        return "H.264";
    if (IsHevcTag(tag))
        return "HEVC";
    if (FourCCIs(tag, "VP80"))
        return "VP8";
    if (FourCCIs(tag, "VP90"))
        return "VP9";
    if (FourCCIs(tag, "AV01"))
        return "AV1";
    if (FourCCIs(tag, "MP4V") || FourCCIs(tag, "FMP4") || FourCCIs(tag, "MP4S") || FourCCIs(tag, "M4S2") ||
        FourCCIs(tag, "MP42") || FourCCIs(tag, "MP43") || FourCCIs(tag, "DIVX") || FourCCIs(tag, "DX50") ||
        FourCCIs(tag, "XVID") || FourCCIs(tag, "DIV3"))
        return "MPEG-4";
    if (FourCCIs(tag, "MPG1") || FourCCIs(tag, "MP1V") || FourCCIs(tag, "MPEG"))
        return "MPEG-1";
    if (FourCCIs(tag, "MPG2") || FourCCIs(tag, "MP2V") || FourCCIs(tag, "H262"))
        return "MPEG-2";
    if (FourCCIs(tag, "MJPG") || FourCCIs(tag, "JPEG") || FourCCIs(tag, "DMB1"))
        return "MJPEG";
    if (FourCCIs(tag, "WMV1") || FourCCIs(tag, "WMV2") || FourCCIs(tag, "WMV3") || FourCCIs(tag, "WMVA") ||
        FourCCIs(tag, "WVC1") || FourCCIs(tag, "MSS1") || FourCCIs(tag, "MSS2"))
        return "WMV";
    if (FourCCIs(tag, "DVSD") || FourCCIs(tag, "DV25") || FourCCIs(tag, "DV50") || FourCCIs(tag, "DVH1") ||
        FourCCIs(tag, "DVHD") || FourCCIs(tag, "DVSL") || FourCCIs(tag, "DVC"))
        return "DV";
    return NULL;
}

std::string CodecWithFourCC(const char* friendly, const std::string& raw)
{
    if (friendly == NULL)
        return raw;
    if (raw.empty() || _stricmp(raw.c_str(), friendly) == 0)
        return std::string(friendly);
    return std::string(friendly) + " (" + raw + ")";
}

std::string VideoCodecName(const GUID& subtype, unsigned int* codecTag)
{
    *codecTag = 0;
    if (SameGuid(subtype, kSubtypeH264Es))
    {
        *codecTag = kH264FourCC;
        return "H.264";
    }
    if (SameGuid(subtype, kSubtypeMpeg2))
    {
        *codecTag = 0x3247504D; // MPG2
        return "MPEG-2";
    }
    if (!IsStandardMediaSubtype(subtype))
        return std::string();

    *codecTag = subtype.Data1;
    const std::string raw = FourCCText(subtype.Data1);
    const char* friendly = FriendlyVideoName(subtype.Data1);
    if (friendly != NULL)
        return CodecWithFourCC(friendly, raw);
    return raw;
}

const char* AudioNameFromTag(unsigned int tag)
{
    switch (tag)
    {
    case 0x0001:
        return "PCM";
    case 0x0003:
        return "IEEE Float";
    case 0x0008:
        return "DTS";
    case 0x000A:
        return "WMA Voice";
    case 0x0050:
        return "MPEG Audio";
    case 0x0055:
        return "MP3";
    case 0x0092:
        return "AC-3";
    case 0x00FF:
    case 0x1600:
    case 0x1610:
        return "AAC";
    case 0x0161:
    case 0x0162:
    case 0x0164:
        return "WMA";
    case 0x0163:
        return "WMA Lossless";
    case 0x7361:
        return "AMR-NB";
    case 0x7362:
        return "AMR-WB";
    case 0x7363:
        return "AMR-WP";
    case 0x6C61:
        return "ALAC";
    case 0x704F:
        return "Opus";
    case 0xF1AC:
        return "FLAC";
    default:
        return NULL;
    }
}

std::string AudioCodecName(const GUID& subtype)
{
    if (SameGuid(subtype, kSubtypeAc3))
        return "AC-3";
    if (SameGuid(subtype, kSubtypeDdPlus))
        return "E-AC-3";
    if (SameGuid(subtype, kSubtypeVorbis))
        return "Vorbis";
    if (SameGuid(subtype, kSubtypeDtsRaw))
        return "DTS";
    if (!IsStandardMediaSubtype(subtype))
        return std::string();

    const char* named = AudioNameFromTag(subtype.Data1);
    if (named != NULL)
        return named;
    return FourCCText(subtype.Data1);
}

std::string WideToUtf8(const wchar_t* text)
{
    if (text == NULL || text[0] == 0)
        return std::string();

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    if (required <= 1)
        return std::string();

    std::string result(static_cast<size_t>(required - 1), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, text, -1, &result[0], required, NULL, NULL) <= 0)
        return std::string();
    return result;
}

std::string AnsiToUtf8(const char* text)
{
    if (text == NULL || text[0] == 0)
        return std::string();

    const int wideCount = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
    if (wideCount <= 1)
        return std::string();

    std::wstring wide(static_cast<size_t>(wideCount), L'\0');
    if (MultiByteToWideChar(CP_ACP, 0, text, -1, &wide[0], wideCount) <= 0)
        return std::string();
    wide.resize(static_cast<size_t>(wideCount - 1));
    return WideToUtf8(wide.c_str());
}

std::string PropVariantToUtf8(const PROPVARIANT& value)
{
    if ((value.vt == VT_LPWSTR || value.vt == VT_BSTR) && value.pwszVal != NULL)
        return WideToUtf8(value.pwszVal);
    if (value.vt == VT_LPSTR && value.pszVal != NULL)
        return AnsiToUtf8(value.pszVal);
    if (value.vt == VT_UI4 || value.vt == VT_UINT)
    {
        if (value.ulVal == 0)
            return std::string();
        char text[32];
        _snprintf_s(text, _countof(text), _TRUNCATE, "%lu", value.ulVal);
        return std::string(text);
    }
    if (value.vt == VT_I4 || value.vt == VT_INT)
    {
        if (value.lVal == 0)
            return std::string();
        char text[32];
        _snprintf_s(text, _countof(text), _TRUNCATE, "%ld", value.lVal);
        return std::string(text);
    }
    if (value.vt == VT_UI2)
    {
        if (value.uiVal == 0)
            return std::string();
        char text[32];
        _snprintf_s(text, _countof(text), _TRUNCATE, "%u", (unsigned int)value.uiVal);
        return std::string(text);
    }
    return std::string();
}

bool HasTag(const CMfMediaInfo& info, int labelId)
{
    for (size_t i = 0; i < info.Tags.size(); i++)
    {
        if (info.Tags[i].LabelId == labelId)
            return true;
    }
    return false;
}

void AddTag(CMfMediaInfo* info, int labelId, const std::string& value)
{
    if (value.empty() || HasTag(*info, labelId))
        return;
    CMfTag tag;
    tag.LabelId = labelId;
    tag.Value = value;
    info->Tags.push_back(tag);
}

bool ReadShellTagsFromPath(const wchar_t* path, CMfMediaInfo* info)
{
    IPropertyStore* store = NULL;
    const HRESULT opened = SHGetPropertyStoreFromParsingName(
        path, NULL, (GETPROPERTYSTOREFLAGS)(GPS_BESTEFFORT | GPS_FASTPROPERTIESONLY), IID_PPV_ARGS(&store));
    if (FAILED(opened) || store == NULL)
        return false;

    const struct
    {
        const PROPERTYKEY* Key;
        int LabelId;
    } keys[] = {
        {&PKEY_Title, IDS_OGG_TITLE},
        {&PKEY_Music_Artist, IDS_OGG_AUTHOR},
        {&PKEY_Author, IDS_OGG_AUTHOR},
        {&PKEY_Music_AlbumTitle, IDS_OGG_ALBUM},
        {&PKEY_Music_Genre, IDS_OGG_GENRE},
        {&PKEY_Media_Year, IDS_MP3_YEAR},
        {&PKEY_Comment, IDS_OGG_COMMENTS},
        {&PKEY_Copyright, IDS_OGG_COPYRIGHT},
    };

    const size_t before = info->Tags.size();
    for (size_t i = 0; i < _countof(keys); i++)
    {
        PROPVARIANT value;
        PropVariantInit(&value);
        if (SUCCEEDED(store->GetValue(*keys[i].Key, &value)))
            AddTag(info, keys[i].LabelId, PropVariantToUtf8(value));
        PropVariantClear(&value);
    }
    store->Release();
    return info->Tags.size() != before;
}

std::wstring StripExtendedPrefix(const wchar_t* path)
{
    const std::wstring text(path);
    if (text.compare(0, 8, L"\\\\?\\UNC\\") == 0)
        return L"\\\\" + text.substr(8);
    if (text.compare(0, 4, L"\\\\?\\") == 0)
        return text.substr(4);
    return text;
}

void ReadShellTags(const wchar_t* path, CMfMediaInfo* info)
{
    if (ReadShellTagsFromPath(path, info))
        return;
    const std::wstring plain = StripExtendedPrefix(path);
    if (plain != path)
        ReadShellTagsFromPath(plain.c_str(), info);
}

void CollectStream(IMFMediaType* type, CMfMediaInfo* info)
{
    GUID major = GUID_NULL;
    GUID subtype = GUID_NULL;
    if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major)))
        return;
    type->GetGUID(MF_MT_SUBTYPE, &subtype);

    if (SameGuid(major, MFMediaType_Video))
    {
        CMfVideoStream stream = {};
        stream.Codec = VideoCodecName(subtype, &stream.CodecTag);

        UINT32 width = 0;
        UINT32 height = 0;
        if (SUCCEEDED(MFGetAttributeSize(type, MF_MT_FRAME_SIZE, &width, &height)))
        {
            stream.Width = width;
            stream.Height = height;
        }

        UINT32 rateNum = 0;
        UINT32 rateDen = 0;
        if (SUCCEEDED(MFGetAttributeRatio(type, MF_MT_FRAME_RATE, &rateNum, &rateDen)) && rateDen != 0 && rateNum != 0)
        {
            stream.HasFrameRate = true;
            stream.FrameRateNumerator = rateNum;
            stream.FrameRateDenominator = rateDen;
        }

        UINT32 bitrate = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_AVG_BITRATE, &bitrate)) && bitrate != 0)
        {
            stream.HasBitrate = true;
            stream.BitrateBps = bitrate;
        }

        UINT32 aspectNum = 0;
        UINT32 aspectDen = 0;
        if (SUCCEEDED(MFGetAttributeRatio(type, MF_MT_PIXEL_ASPECT_RATIO, &aspectNum, &aspectDen)) &&
            aspectNum != 0 && aspectDen != 0 && !(aspectNum == 1 && aspectDen == 1))
        {
            stream.HasPixelAspect = true;
            stream.PixelAspectNumerator = aspectNum;
            stream.PixelAspectDenominator = aspectDen;
        }

        UINT32 scan = MFVideoInterlace_Unknown;
        if (SUCCEEDED(type->GetUINT32(MF_MT_INTERLACE_MODE, &scan)))
        {
            if (scan == MFVideoInterlace_Progressive)
                stream.ScanMode = 1;
            else if (scan == MFVideoInterlace_FieldInterleavedUpperFirst ||
                     scan == MFVideoInterlace_FieldInterleavedLowerFirst ||
                     scan == MFVideoInterlace_FieldSingleUpper ||
                     scan == MFVideoInterlace_FieldSingleLower)
                stream.ScanMode = 2;
            else if (scan == MFVideoInterlace_MixedInterlaceOrProgressive)
                stream.ScanMode = 3;
        }

        UINT32 rotation = 0;
        if (SUCCEEDED(type->GetUINT32(kVideoRotation, &rotation)) &&
            (rotation == 90 || rotation == 180 || rotation == 270))
        {
            stream.HasRotation = true;
            stream.RotationDegrees = rotation;
        }

        UINT32 profile = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_MPEG2_PROFILE, &profile)) && profile != 0)
        {
            stream.HasProfile = true;
            stream.Profile = profile;
        }

        UINT32 level = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_MPEG2_LEVEL, &level)) && level != 0)
        {
            stream.HasLevel = true;
            stream.Level = level;
        }

        info->Video.push_back(stream);
        return;
    }

    if (SameGuid(major, MFMediaType_Audio))
    {
        CMfAudioStream stream = {};
        stream.Codec = AudioCodecName(subtype);

        UINT32 sampleRate = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate)))
            stream.SampleRate = sampleRate;

        UINT32 channels = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels)))
            stream.Channels = channels;

        UINT32 bits = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits)))
            stream.BitsPerSample = bits;

        UINT32 bitrate = 0;
        if (SUCCEEDED(type->GetUINT32(MF_MT_AVG_BITRATE, &bitrate)) && bitrate != 0)
        {
            stream.HasBitrate = true;
            stream.BitrateBps = bitrate;
        }
        info->Audio.push_back(stream);
    }
}

void FormatDuration(unsigned long long duration100ns, char* text, size_t textSize)
{
    const unsigned long long totalSeconds = duration100ns / 10000000ULL;
    const unsigned long long hours = totalSeconds / 3600ULL;
    const unsigned long long minutes = (totalSeconds / 60ULL) % 60ULL;
    const unsigned long long seconds = totalSeconds % 60ULL;
    if (hours != 0)
        _snprintf_s(text, textSize, _TRUNCATE, "%02llu:%02llu:%02llu", hours, minutes, seconds);
    else
        _snprintf_s(text, textSize, _TRUNCATE, "%02llu:%02llu", minutes, seconds);
}

void FormatKbps(unsigned int bitsPerSecond, char* text, size_t textSize)
{
    unsigned int kbps = (bitsPerSecond + 500u) / 1000u;
    if (kbps == 0)
        kbps = 1;
    _snprintf_s(text, textSize, _TRUNCATE, "%u", kbps);
}

bool FormatFrameRate(unsigned int numerator, unsigned int denominator, char* text, size_t textSize)
{
    if (denominator == 0 || numerator == 0)
        return false;
    if (numerator % denominator == 0)
    {
        _snprintf_s(text, textSize, _TRUNCATE, "%u", numerator / denominator);
        return true;
    }

    const unsigned long long scaled =
        (static_cast<unsigned long long>(numerator) * 1000ULL + denominator / 2ULL) / denominator;
    const unsigned int whole = static_cast<unsigned int>(scaled / 1000ULL);
    const unsigned int frac = static_cast<unsigned int>(scaled % 1000ULL);
    if (frac == 0)
    {
        _snprintf_s(text, textSize, _TRUNCATE, "%u", whole);
        return true;
    }
    char fracText[4];
    _snprintf_s(fracText, _countof(fracText), _TRUNCATE, "%03u", frac);
    size_t length = 3;
    while (length > 0 && fracText[length - 1] == '0')
        length--;
    fracText[length] = 0;
    _snprintf_s(text, textSize, _TRUNCATE, "%u.%s", whole, fracText);
    return true;
}

unsigned int GreatestDivisor(unsigned int left, unsigned int right)
{
    while (right != 0)
    {
        const unsigned int next = left % right;
        left = right;
        right = next;
    }
    return left == 0 ? 1 : left;
}

std::string FormatProfile(unsigned int codecTag, unsigned int profile)
{
    const char* name = NULL;
    if (IsH264Tag(codecTag))
    {
        switch (profile)
        {
        case 66:
            name = "Baseline";
            break;
        case 77:
            name = "Main";
            break;
        case 88:
            name = "Extended";
            break;
        case 100:
            name = "High";
            break;
        case 110:
            name = "High 10";
            break;
        case 122:
            name = "High 4:2:2";
            break;
        case 244:
            name = "High 4:4:4 Predictive";
            break;
        }
    }
    else if (IsHevcTag(codecTag))
    {
        switch (profile)
        {
        case 1:
            name = "Main";
            break;
        case 2:
            name = "Main 10";
            break;
        case 3:
            name = "Main Still Picture";
            break;
        }
    }
    if (name != NULL)
        return std::string(name);

    char text[32];
    _snprintf_s(text, _countof(text), _TRUNCATE, "%u", profile);
    return std::string(text);
}

void FormatLevel(unsigned int codecTag, unsigned int level, char* text, size_t textSize)
{
    if ((IsH264Tag(codecTag) || IsHevcTag(codecTag)) && level >= 10 && level <= 255)
        _snprintf_s(text, textSize, _TRUNCATE, "%u.%u", level / 10, level % 10);
    else
        _snprintf_s(text, textSize, _TRUNCATE, "%u", level);
}

std::string CodecText(const std::string& codec)
{
    if (!codec.empty())
        return codec;
    const char* unknown = LoadStr(IDS_MEDIA_UNKNOWN);
    return unknown != NULL ? std::string(unknown) : std::string();
}

std::string FormatIndexedLabel(const char* pattern, unsigned int index)
{
    if (pattern == NULL)
        return std::string();

    const char* marker = strstr(pattern, "%u");
    char number[16];
    _snprintf_s(number, _countof(number), _TRUNCATE, "%u", index);
    if (marker == NULL)
        return std::string(pattern) + " " + number;

    std::string result(pattern, static_cast<size_t>(marker - pattern));
    result += number;
    result += marker + 2;
    return result;
}

void AddStreamHeader(COutputInterface* output, size_t count, size_t index, int singleId, int indexedId)
{
    if (count <= 1)
    {
        output->AddHeader(LoadStr(singleId));
        return;
    }

    const std::string label = FormatIndexedLabel(LoadStr(indexedId), (unsigned int)(index + 1));
    output->AddHeader(label.c_str());
}

void AddLoadedItem(COutputInterface* output, int labelId, int valueId)
{
    const std::string value = LoadStr(valueId);
    output->AddItem(LoadStr(labelId), value.c_str());
}

void AddNumberItem(COutputInterface* output, int labelId, unsigned int value)
{
    if (value == 0)
        return;
    char text[32];
    _snprintf_s(text, _countof(text), _TRUNCATE, "%u", value);
    output->AddItem(LoadStr(labelId), text);
}
}

std::wstring MmViewerPathToWide(const char* path)
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

std::wstring MmViewerMakeExtendedPath(const wchar_t* path)
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

bool MmViewerMayContainVideo(const wchar_t* path)
{
    if (path == NULL || path[0] == 0)
        return false;

    const wchar_t* extension = wcsrchr(path, L'.');
    if (extension == NULL)
        return false;

    static const wchar_t* videoExtensions[] = {
        L".m4a", L".mp4", L".m4b", L".mka", L".webm", L".mkv", L".m4v",
        L".mov", L".qt", L".3gp", L".3g2", L".asf", L".avi", L".divx",
        L".wmv", L".mpg", L".mpeg", L".m2ts", L".mts"};
    for (size_t i = 0; i < _countof(videoExtensions); i++)
    {
        if (_wcsicmp(extension, videoExtensions[i]) == 0)
            return true;
    }
    return false;
}

bool ReadMediaFoundationInfo(const wchar_t* path, CMfMediaInfo* info, bool readTags)
{
    if (info == NULL)
        return false;

    info->Opened = false;
    info->HasDuration = false;
    info->Duration100ns = 0;
    info->Video.clear();
    info->Audio.clear();
    info->Tags.clear();
    if (path == NULL || path[0] == 0)
        return false;

    // The viewer thread owns a window, so prefer the apartment model when this
    // call is the first COM user on the thread. An existing apartment is left as-is.
    const HRESULT com = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    const bool uninitializeCom = (com == S_OK || com == S_FALSE);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE)
        return false;

    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET)))
    {
        if (uninitializeCom)
            CoUninitialize();
        return false;
    }

    IMFAttributes* attributes = NULL;
    IMFSourceReader* reader = NULL;
    bool opened = false;
    if (SUCCEEDED(MFCreateAttributes(&attributes, 1)) && attributes != NULL)
    {
        attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, FALSE);
        if (SUCCEEDED(MFCreateSourceReaderFromURL(path, attributes, &reader)) && reader != NULL)
        {
            PROPVARIANT duration;
            PropVariantInit(&duration);
            if (SUCCEEDED(reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &duration)) &&
                duration.vt == VT_UI8)
            {
                info->HasDuration = true;
                info->Duration100ns = duration.uhVal.QuadPart;
            }
            PropVariantClear(&duration);

            for (DWORD streamIndex = 0; streamIndex < 64; streamIndex++)
            {
                IMFMediaType* type = NULL;
                const HRESULT typeResult = reader->GetNativeMediaType(streamIndex, 0, &type);
                if (typeResult == MF_E_INVALIDSTREAMNUMBER)
                {
                    if (type != NULL)
                        type->Release();
                    break;
                }
                if (SUCCEEDED(typeResult) && type != NULL)
                    CollectStream(type, info);
                if (type != NULL)
                    type->Release();
            }
            opened = !info->Video.empty() || !info->Audio.empty();
        }
    }

    if (reader != NULL)
        reader->Release();
    if (attributes != NULL)
        attributes->Release();
    // Shell property handlers need COM, so read tags before this call shuts it down.
    // The source reader is already released and no longer holds the file.
    if (opened && readTags)
        ReadShellTags(path, info);
    MFShutdown();
    if (uninitializeCom)
        CoUninitialize();

    if (!opened)
        return false;

    info->Opened = true;
    return true;
}

bool AppendMfDurationSection(COutputInterface* output, const CMfMediaInfo& info)
{
    if (!info.HasDuration)
        return false;

    char text[64];
    FormatDuration(info.Duration100ns, text, _countof(text));
    output->AddHeader(LoadStr(IDS_MF_INFO));
    output->AddItem(LoadStr(IDS_OGG_LENGTH), text);
    return true;
}

bool AppendMfVideoSection(COutputInterface* output, const CMfMediaInfo& info)
{
    if (info.Video.empty())
        return false;

    for (size_t i = 0; i < info.Video.size(); i++)
    {
        if (i != 0)
            output->AddSeparator();
        AddStreamHeader(output, info.Video.size(), i, IDS_VIDEO_INFO, IDS_VIDEO_INFO_INDEX);

        const CMfVideoStream& stream = info.Video[i];
        const std::string codec = CodecText(stream.Codec);
        output->AddItem(LoadStr(IDS_VIDEO_CODEC), codec.c_str());

        if (stream.Width != 0 && stream.Height != 0)
        {
            char text[64];
            _snprintf_s(text, _countof(text), _TRUNCATE, "%u x %u", stream.Width, stream.Height);
            output->AddItem(LoadStr(IDS_VIDEO_RESOLUTION), text);
        }
        if (stream.HasFrameRate)
        {
            char text[64];
            if (FormatFrameRate(stream.FrameRateNumerator, stream.FrameRateDenominator, text, _countof(text)))
                output->AddItem(LoadStr(IDS_VIDEO_FRAMERATE), text);
        }
        if (stream.HasBitrate)
        {
            char text[32];
            FormatKbps(stream.BitrateBps, text, _countof(text));
            output->AddItem(LoadStr(IDS_VIDEO_BITRATE), text);
        }
        if (stream.HasPixelAspect)
        {
            const unsigned int divisor = GreatestDivisor(stream.PixelAspectNumerator, stream.PixelAspectDenominator);
            char text[64];
            _snprintf_s(text, _countof(text), _TRUNCATE, "%u:%u", stream.PixelAspectNumerator / divisor,
                        stream.PixelAspectDenominator / divisor);
            output->AddItem(LoadStr(IDS_VIDEO_PIXEL_ASPECT), text);
        }
        if (stream.ScanMode == 1)
            AddLoadedItem(output, IDS_VIDEO_SCAN, IDS_VIDEO_PROGRESSIVE);
        else if (stream.ScanMode == 2)
            AddLoadedItem(output, IDS_VIDEO_SCAN, IDS_VIDEO_INTERLACED);
        else if (stream.ScanMode == 3)
            AddLoadedItem(output, IDS_VIDEO_SCAN, IDS_VIDEO_MIXED);
        if (stream.HasRotation)
        {
            char text[32];
            _snprintf_s(text, _countof(text), _TRUNCATE, "%u deg", stream.RotationDegrees);
            output->AddItem(LoadStr(IDS_VIDEO_ROTATION), text);
        }
        if (stream.HasProfile)
        {
            const std::string profile = FormatProfile(stream.CodecTag, stream.Profile);
            output->AddItem(LoadStr(IDS_VIDEO_PROFILE), profile.c_str());
        }
        if (stream.HasLevel)
        {
            char text[32];
            FormatLevel(stream.CodecTag, stream.Level, text, _countof(text));
            output->AddItem(LoadStr(IDS_VIDEO_LEVEL), text);
        }
    }
    return true;
}

bool AppendMfAudioSection(COutputInterface* output, const CMfMediaInfo& info)
{
    if (info.Audio.empty())
        return false;

    for (size_t i = 0; i < info.Audio.size(); i++)
    {
        if (i != 0)
            output->AddSeparator();
        AddStreamHeader(output, info.Audio.size(), i, IDS_OGG_INFO, IDS_AUDIO_INFO_INDEX);

        const CMfAudioStream& stream = info.Audio[i];
        const std::string codec = CodecText(stream.Codec);
        output->AddItem(LoadStr(IDS_AUDIO_CODEC), codec.c_str());
        AddNumberItem(output, IDS_OGG_FREQUENCY, stream.SampleRate);
        AddNumberItem(output, IDS_OGG_CHANNELS, stream.Channels);
        AddNumberItem(output, IDS_AUDIO_BITDEPTH, stream.BitsPerSample);
        if (stream.HasBitrate)
        {
            char text[32];
            FormatKbps(stream.BitrateBps, text, _countof(text));
            output->AddItem(LoadStr(IDS_VIDEO_BITRATE), text);
        }
    }
    return true;
}

bool AppendMfTagSection(COutputInterface* output, const CMfMediaInfo& info)
{
    if (info.Tags.empty())
        return false;

    output->AddHeader(LoadStr(IDS_OGG_STDTAGS));
    for (size_t i = 0; i < info.Tags.size(); i++)
        output->AddItem(LoadStr(info.Tags[i].LabelId), info.Tags[i].Value.c_str());
    return true;
}

CParserMF::CParserMF()
    : HasInfo(false)
{
}

CParserResultEnum CParserMF::OpenFile(const char* fileName)
{
    CloseFile();

    const std::wstring wideName = MmViewerPathToWide(fileName);
    FileName = MmViewerMakeExtendedPath(wideName.c_str());
    if (FileName.empty())
        return preOpenError;

    if (!ReadMediaFoundationInfo(FileName.c_str(), &Info, true))
    {
        FileName.clear();
        return preUnknownFile;
    }
    HasInfo = true;
    return preOK;
}

CParserResultEnum CParserMF::CloseFile()
{
    FileName.clear();
    Info = CMfMediaInfo();
    HasInfo = false;
    return preOK;
}

CParserResultEnum CParserMF::GetFileInfo(COutputInterface* output)
{
    if (!HasInfo)
        return preOpenError;

    bool any = false;
    if (AppendMfDurationSection(output, Info))
        any = true;
    if (!Info.Video.empty())
    {
        if (any)
            output->AddSeparator();
        AppendMfVideoSection(output, Info);
        any = true;
    }
    if (!Info.Audio.empty())
    {
        if (any)
            output->AddSeparator();
        AppendMfAudioSection(output, Info);
        any = true;
    }
    if (!Info.Tags.empty())
    {
        if (any)
            output->AddSeparator();
        AppendMfTagSection(output, Info);
    }
    return preOK;
}
