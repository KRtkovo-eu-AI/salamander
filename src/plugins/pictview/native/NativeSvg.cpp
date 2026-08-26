// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "../../../common/dep/nanosvg/nanosvg.h"
#include "../../../common/dep/nanosvg/nanosvgrast.h"

namespace PictView::Native
{
namespace Detail
{

Status DecodeSvg(Reader& reader, DecodedImage& image)
{
    if (reader.Size() < 4 || reader.Size() > 2ull * 1024ull * 1024ull)
    {
        return Status::Unsupported;
    }
    const BYTE* data = reader.Data();
    const size_t probe = (std::min)(reader.Size(), static_cast<size_t>(4096));
    bool looksLikeSvg = false;
    for (size_t i = 0; i + 4 <= probe; ++i)
    {
        if (data[i] != '<')
        {
            continue;
        }
        size_t j = i + 1;
        if (j < probe && (data[j] == '?' || data[j] == '!'))
        {
            continue;
        }
        while (j < probe && (data[j] == ' ' || data[j] == '\t' || data[j] == '\r' || data[j] == '\n'))
        {
            ++j;
        }
        if (j + 3 <= probe)
        {
            const BYTE a = static_cast<BYTE>(data[j] | 0x20);
            const BYTE b = static_cast<BYTE>(data[j + 1] | 0x20);
            const BYTE c = static_cast<BYTE>(data[j + 2] | 0x20);
            if (a == 's' && b == 'v' && c == 'g')
            {
                looksLikeSvg = true;
                break;
            }
        }
    }
    if (!looksLikeSvg)
    {
        return Status::Unsupported;
    }

    std::vector<char> text;
    try
    {
        text.assign(reinterpret_cast<const char*>(data), reinterpret_cast<const char*>(data) + reader.Size());
        text.push_back('\0');
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    NSVGimage* svg = nsvgParse(text.data(), "px", 96.0f);
    if (svg == nullptr)
    {
        return Status::Unsupported;
    }
    if (svg->shapes == nullptr)
    {
        nsvgDelete(svg);
        return Status::Unsupported;
    }

    float svgWidth = svg->width;
    float svgHeight = svg->height;
    if (!(svgWidth > 0.0f) || !(svgHeight > 0.0f))
    {
        svgWidth = 256.0f;
        svgHeight = 256.0f;
    }
    const float scale = std::min(1.0f, std::min(4096.0f / svgWidth, 4096.0f / svgHeight));
    const UINT width = std::max(1u, static_cast<UINT>(std::ceil(svgWidth * scale)));
    const UINT height = std::max(1u, static_cast<UINT>(std::ceil(svgHeight * scale)));

    Frame frame;
    Status st = MakeFrame(frame, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        nsvgDelete(svg);
        return st;
    }

    NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
    if (rasterizer == nullptr)
    {
        nsvgDelete(svg);
        return Status::OutOfMemory;
    }
    nsvgRasterize(rasterizer, svg, 0.0f, 0.0f, scale, frame.bgra.data(), static_cast<int>(width),
                  static_cast<int>(height), static_cast<int>(width * 4));
    nsvgDeleteRasterizer(rasterizer);
    nsvgDelete(svg);

    // nsvgRasterize already converts NanoSVG's RGBA buffer to BGRA (GDI / AlphaBlend).
    NoteAlpha(frame);
    image.format = PVF_SVG;
    image.formatLabel = "SVG";
    image.frames.clear();
    image.frames.push_back(std::move(frame));
    return Status::Ok;
}

} // namespace Detail
} // namespace PictView::Native
