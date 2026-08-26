// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdint>
#include <string>
#include <vector>

#include "../lib/PVW32DLL.h"

namespace PictView::Native
{

enum class Status
{
    Ok = 0,
    Unsupported,
    CannotOpen,
    OutOfMemory,
    Invalid
};

struct Frame
{
    UINT width = 0;
    UINT height = 0;
    UINT bitDepth = 32;
    DWORD colors = PV_COLOR_TC32;
    DWORD delayMs = 0;
    bool hasTransparency = false;
    std::vector<BYTE> bgra;
};

struct DecodedImage
{
    DWORD format = 0;
    const char* formatLabel = "Native";
    std::vector<Frame> frames;
};

void GetDecoderMasks(std::vector<std::string>& masks);
Status DecodeMemory(const BYTE* data, size_t size, DecodedImage& image);
Status DecodeFile(const wchar_t* path, DecodedImage& image);

struct EmbeddedPreview
{
    size_t offset = 0;
    size_t size = 0;
    DWORD format = 0;
    const char* formatLabel = "Preview";
};

bool FindEmbeddedPreview(const BYTE* data, size_t size, EmbeddedPreview& preview);

} // namespace PictView::Native
