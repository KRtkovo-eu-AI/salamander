// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-04 | Read video and audio stream details with Media Foundation. | Multimedia Viewer showed only the audio part of video files. | https://github.com/KRtkovo-eu-AI/salamander/issues/732 | Grok 4.7 |

#pragma once

#include "parser.h"

#include <string>
#include <vector>

class COutputInterface;

// UTF-8 first, ACP when that conversion fails. Paths that reach MAX_PATH get the
// Win32 extended prefix so later file opens keep the full name.
std::wstring MmViewerPathToWide(const char* path);
std::wstring MmViewerMakeExtendedPath(const wchar_t* path);

struct CMfVideoStream
{
    std::string Codec;
    unsigned int CodecTag;
    unsigned int Width;
    unsigned int Height;
    bool HasFrameRate;
    unsigned int FrameRateNumerator;
    unsigned int FrameRateDenominator;
    bool HasBitrate;
    unsigned int BitrateBps;
    bool HasPixelAspect;
    unsigned int PixelAspectNumerator;
    unsigned int PixelAspectDenominator;
    int ScanMode; // 0 omitted, 1 progressive, 2 interlaced, 3 mixed
    bool HasRotation;
    unsigned int RotationDegrees;
    bool HasProfile;
    unsigned int Profile;
    bool HasLevel;
    unsigned int Level;
};

struct CMfAudioStream
{
    std::string Codec;
    unsigned int SampleRate;
    unsigned int Channels;
    unsigned int BitsPerSample;
    bool HasBitrate;
    unsigned int BitrateBps;
};

struct CMfTag
{
    int LabelId;
    std::string Value;
};

struct CMfMediaInfo
{
    bool Opened;
    bool HasDuration;
    unsigned long long Duration100ns;
    std::vector<CMfVideoStream> Video;
    std::vector<CMfAudioStream> Audio;
    std::vector<CMfTag> Tags;
};

// True for containers that can carry a video track. Pure audio extensions skip Media Foundation.
bool MmViewerMayContainVideo(const wchar_t* path);

// Reads container metadata only. Returns false when the file has no audio or video stream.
bool ReadMediaFoundationInfo(const wchar_t* path, CMfMediaInfo* info, bool readTags);

bool AppendMfDurationSection(COutputInterface* output, const CMfMediaInfo& info);
bool AppendMfVideoSection(COutputInterface* output, const CMfMediaInfo& info);
bool AppendMfAudioSection(COutputInterface* output, const CMfMediaInfo& info);
bool AppendMfTagSection(COutputInterface* output, const CMfMediaInfo& info);

class CParserMF : public CParserInterface
{
public:
    CParserMF();

    virtual CParserResultEnum OpenFile(const char* fileName);
    virtual CParserResultEnum CloseFile();
    virtual CParserResultEnum GetFileInfo(COutputInterface* output);

private:
    std::wstring FileName;
    CMfMediaInfo Info;
    bool HasInfo;
};
