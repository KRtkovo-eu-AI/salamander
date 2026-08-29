// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <cstdint>

namespace PictViewPreviewProtocol
{
constexpr std::uint32_t Magic = 0x48565053; // "SPVH"
constexpr std::uint16_t Version = 1;
constexpr std::uint32_t MaximumPayloadBytes = 1024 * 1024;

enum class Command : std::uint16_t
{
    Open = 1,
    Attach = 2,
    Resize = 3,
    Focus = 4,
    Close = 5,
};

#pragma pack(push, 1)
struct RequestHeader
{
    std::uint32_t magic;
    std::uint16_t version;
    Command command;
    std::uint32_t payloadBytes;
    std::uint32_t requestId;
};

struct Response
{
    std::uint32_t magic;
    std::uint16_t version;
    std::uint16_t reserved;
    std::uint32_t requestId;
    HRESULT result;
};

struct AttachPayload
{
    std::uint64_t parentWindow;
    RECT rectangle;
    COLORREF background;
};

struct ResizePayload
{
    RECT rectangle;
};
#pragma pack(pop)

static_assert(sizeof(RequestHeader) == 16, "Protocol header layout changed");
static_assert(sizeof(Response) == 16, "Protocol response layout changed");
} // namespace PictViewPreviewProtocol
