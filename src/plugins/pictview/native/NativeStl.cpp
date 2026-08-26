// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "NativeInternal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace PictView::Native
{
namespace
{

constexpr size_t kMaxStlBytes = 32ull * 1024ull * 1024ull;
constexpr UINT32 kMaxStlTriangles = 250000;
constexpr float kPi = 3.14159265f;

struct Vec3
{
    float x = 0;
    float y = 0;
    float z = 0;
};

struct Triangle
{
    Vec3 v[3];
};

float ReadF32LE(const BYTE* p)
{
    UINT32 u = static_cast<UINT32>(p[0]) | (static_cast<UINT32>(p[1]) << 8) |
               (static_cast<UINT32>(p[2]) << 16) | (static_cast<UINT32>(p[3]) << 24);
    float f = 0;
    memcpy(&f, &u, sizeof(f));
    return f;
}

bool FiniteVec(const Vec3& v)
{
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

Vec3 Sub(const Vec3& a, const Vec3& b)
{
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float Dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Length(const Vec3& v)
{
    return std::sqrt(Dot(v, v));
}

Vec3 Normalize(const Vec3& v)
{
    const float len = Length(v);
    if (!(len > 1.0e-12f))
    {
        return Vec3{0, 0, 1};
    }
    return Vec3{v.x / len, v.y / len, v.z / len};
}

Vec3 RotateIsometric(const Vec3& v)
{
    // STL is Z-up (print bed in XY). The isometric camera treats Y as up, so
    // rotate -90 degrees around X before the yaw/pitch view.
    const Vec3 yUp{v.x, v.z, -v.y};
    const float yaw = kPi / 4.0f;
    const float pitch = std::atan(1.0f / std::sqrt(2.0f));
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float x1 = yUp.x * cy - yUp.z * sy;
    const float z1 = yUp.x * sy + yUp.z * cy;
    const float y2 = yUp.y * cp - z1 * sp;
    const float z2 = yUp.y * sp + z1 * cp;
    return Vec3{x1, y2, z2};
}

bool LooksLikeBinaryStl(const BYTE* data, size_t size)
{
    if (data == nullptr || size < 84)
    {
        return false;
    }
    const UINT32 count = static_cast<UINT32>(data[80]) | (static_cast<UINT32>(data[81]) << 8) |
                         (static_cast<UINT32>(data[82]) << 16) | (static_cast<UINT32>(data[83]) << 24);
    if (count == 0 || count > kMaxStlTriangles)
    {
        return false;
    }
    const ULONGLONG expected = 84ull + static_cast<ULONGLONG>(count) * 50ull;
    return expected <= static_cast<ULONGLONG>(size) && (static_cast<ULONGLONG>(size) - expected) <= 4096ull;
}

bool ParseBinaryStl(const BYTE* data, size_t size, std::vector<Triangle>& tris)
{
    const UINT32 count = static_cast<UINT32>(data[80]) | (static_cast<UINT32>(data[81]) << 8) |
                         (static_cast<UINT32>(data[82]) << 16) | (static_cast<UINT32>(data[83]) << 24);
    try
    {
        tris.reserve(count);
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
    size_t pos = 84;
    for (UINT32 i = 0; i < count; ++i)
    {
        if (pos + 50 > size)
        {
            return false;
        }
        Triangle t{};
        for (int v = 0; v < 3; ++v)
        {
            const BYTE* p = data + pos + 12u + static_cast<size_t>(v) * 12u;
            t.v[v].x = ReadF32LE(p);
            t.v[v].y = ReadF32LE(p + 4);
            t.v[v].z = ReadF32LE(p + 8);
            if (!FiniteVec(t.v[v]))
            {
                return false;
            }
        }
        tris.push_back(t);
        pos += 50;
    }
    return !tris.empty();
}

bool SkipAsciiSpace(const BYTE* data, size_t size, size_t& pos)
{
    while (pos < size)
    {
        const BYTE c = data[pos];
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
        {
            return true;
        }
        ++pos;
    }
    return false;
}

bool MatchAsciiToken(const BYTE* data, size_t size, size_t& pos, const char* token)
{
    if (!SkipAsciiSpace(data, size, pos))
    {
        return false;
    }
    const size_t len = strlen(token);
    if (pos + len > size)
    {
        return false;
    }
    for (size_t i = 0; i < len; ++i)
    {
        BYTE c = data[pos + i];
        if (c >= 'A' && c <= 'Z')
        {
            c = static_cast<BYTE>(c - 'A' + 'a');
        }
        if (c != static_cast<BYTE>(token[i]))
        {
            return false;
        }
    }
    pos += len;
    return true;
}

bool ParseAsciiFloat(const BYTE* data, size_t size, size_t& pos, float& value)
{
    if (!SkipAsciiSpace(data, size, pos))
    {
        return false;
    }
    char buf[64];
    size_t n = 0;
    while (pos < size && n + 1 < sizeof(buf))
    {
        const char c = static_cast<char>(data[pos]);
        if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E')
        {
            buf[n++] = c;
            ++pos;
            continue;
        }
        break;
    }
    if (n == 0)
    {
        return false;
    }
    buf[n] = 0;
    char* end = nullptr;
    value = static_cast<float>(strtod(buf, &end));
    return end != buf && std::isfinite(value);
}

bool ParseAsciiStl(const BYTE* data, size_t size, std::vector<Triangle>& tris)
{
    size_t pos = 0;
    if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF)
    {
        pos = 3;
    }
    if (!MatchAsciiToken(data, size, pos, "solid"))
    {
        return false;
    }
    while (pos < size && data[pos] != '\n' && data[pos] != '\r')
    {
        ++pos;
    }
    try
    {
        tris.reserve(64);
    }
    catch (const std::bad_alloc&)
    {
        return false;
    }
    while (pos < size)
    {
        if (MatchAsciiToken(data, size, pos, "endsolid"))
        {
            break;
        }
        if (!MatchAsciiToken(data, size, pos, "facet"))
        {
            return false;
        }
        MatchAsciiToken(data, size, pos, "normal");
        float ignore = 0;
        ParseAsciiFloat(data, size, pos, ignore);
        ParseAsciiFloat(data, size, pos, ignore);
        ParseAsciiFloat(data, size, pos, ignore);
        if (!MatchAsciiToken(data, size, pos, "outer") || !MatchAsciiToken(data, size, pos, "loop"))
        {
            return false;
        }
        Triangle t{};
        for (int v = 0; v < 3; ++v)
        {
            if (!MatchAsciiToken(data, size, pos, "vertex") || !ParseAsciiFloat(data, size, pos, t.v[v].x) ||
                !ParseAsciiFloat(data, size, pos, t.v[v].y) || !ParseAsciiFloat(data, size, pos, t.v[v].z))
            {
                return false;
            }
        }
        if (!MatchAsciiToken(data, size, pos, "endloop") || !MatchAsciiToken(data, size, pos, "endfacet"))
        {
            return false;
        }
        if (tris.size() >= kMaxStlTriangles)
        {
            return false;
        }
        tris.push_back(t);
    }
    return !tris.empty();
}

float Edge(float ax, float ay, float bx, float by, float px, float py)
{
    return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void FillTriangle(Frame& frame, std::vector<float>& zbuf, const Vec3& a, const Vec3& b, const Vec3& c, BYTE blue,
                  BYTE green, BYTE red)
{
    const int width = static_cast<int>(frame.width);
    const int height = static_cast<int>(frame.height);
    const int minX = (std::max)(0, static_cast<int>(std::floor((std::min)({a.x, b.x, c.x}))));
    const int maxX = (std::min)(width - 1, static_cast<int>(std::ceil((std::max)({a.x, b.x, c.x}))));
    const int minY = (std::max)(0, static_cast<int>(std::floor((std::min)({a.y, b.y, c.y}))));
    const int maxY = (std::min)(height - 1, static_cast<int>(std::ceil((std::max)({a.y, b.y, c.y}))));
    const float area = Edge(a.x, a.y, b.x, b.y, c.x, c.y);
    if (std::fabs(area) < 1.0e-8f)
    {
        return;
    }
    for (int y = minY; y <= maxY; ++y)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float w0 = Edge(b.x, b.y, c.x, c.y, px, py) / area;
            const float w1 = Edge(c.x, c.y, a.x, a.y, px, py) / area;
            const float w2 = Edge(a.x, a.y, b.x, b.y, px, py) / area;
            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
            {
                continue;
            }
            const float z = w0 * a.z + w1 * b.z + w2 * c.z;
            const size_t index = static_cast<size_t>(y) * frame.width + static_cast<size_t>(x);
            if (z <= zbuf[index])
            {
                continue;
            }
            zbuf[index] = z;
            Detail::SetPixel(frame, static_cast<UINT>(x), static_cast<UINT>(y), blue, green, red, 255);
        }
    }
}

Status RasterizeTriangles(const std::vector<Triangle>& tris, UINT width, UINT height, COLORREF albedo, Frame& frame,
                          bool (*cancel)(void*), void* cancelCtx)
{
    Status st = Detail::MakeFrame(frame, width, height, 32, PV_COLOR_TC32);
    if (st != Status::Ok)
    {
        return st;
    }
    frame.hasTransparency = true;

    std::vector<Vec3> projected;
    try
    {
        projected.resize(tris.size() * 3u);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }
    float minX = 1.0e30f;
    float minY = 1.0e30f;
    float maxX = -1.0e30f;
    float maxY = -1.0e30f;
    for (size_t i = 0; i < tris.size(); ++i)
    {
        for (int v = 0; v < 3; ++v)
        {
            const Vec3 p = RotateIsometric(tris[i].v[v]);
            projected[i * 3u + static_cast<size_t>(v)] = p;
            minX = (std::min)(minX, p.x);
            minY = (std::min)(minY, p.y);
            maxX = (std::max)(maxX, p.x);
            maxY = (std::max)(maxY, p.y);
        }
    }
    const float spanX = (std::max)(1.0e-6f, maxX - minX);
    const float spanY = (std::max)(1.0e-6f, maxY - minY);
    const float margin = 0.08f;
    const float usableW = static_cast<float>(width) * (1.0f - 2.0f * margin);
    const float usableH = static_cast<float>(height) * (1.0f - 2.0f * margin);
    const float scale = (std::min)(usableW / spanX, usableH / spanY);
    const float ox = static_cast<float>(width) * 0.5f - (minX + maxX) * 0.5f * scale;
    const float oy = static_cast<float>(height) * 0.5f + (minY + maxY) * 0.5f * scale;

    std::vector<float> zbuf;
    try
    {
        zbuf.assign(static_cast<size_t>(width) * height, -1.0e30f);
    }
    catch (const std::bad_alloc&)
    {
        return Status::OutOfMemory;
    }

    const Vec3 light = Normalize(Vec3{0.35f, 0.85f, 0.45f});
    const float ar = static_cast<float>(GetRValue(albedo));
    const float ag = static_cast<float>(GetGValue(albedo));
    const float ab = static_cast<float>(GetBValue(albedo));

    for (size_t i = 0; i < tris.size(); ++i)
    {
        if (cancel != nullptr && (i % 1024u) == 0 && cancel(cancelCtx))
        {
            return Status::Unsupported;
        }
        Vec3 p0 = projected[i * 3u];
        Vec3 p1 = projected[i * 3u + 1u];
        Vec3 p2 = projected[i * 3u + 2u];
        const Vec3 e1 = Sub(p1, p0);
        const Vec3 e2 = Sub(p2, p0);
        Vec3 n = Normalize(Cross(e1, e2));
        if (n.z < 0.0f)
        {
            n.x = -n.x;
            n.y = -n.y;
            n.z = -n.z;
        }
        p0.x = p0.x * scale + ox;
        p0.y = oy - p0.y * scale;
        p1.x = p1.x * scale + ox;
        p1.y = oy - p1.y * scale;
        p2.x = p2.x * scale + ox;
        p2.y = oy - p2.y * scale;
        const float ndotl = (std::max)(0.18f, Dot(n, light));
        const BYTE r = static_cast<BYTE>((std::min)(255.0f, ar * ndotl + 0.5f));
        const BYTE g = static_cast<BYTE>((std::min)(255.0f, ag * ndotl + 0.5f));
        const BYTE b = static_cast<BYTE>((std::min)(255.0f, ab * ndotl + 0.5f));
        FillTriangle(frame, zbuf, p0, p1, p2, b, g, r);
    }
    return Status::Ok;
}

} // namespace

Status RasterizeStlMemory(const BYTE* data, size_t size, UINT width, UINT height, COLORREF albedo, Frame& frame,
                          bool (*cancel)(void*), void* cancelCtx)
{
    frame = {};
    if (data == nullptr || size < 15 || width == 0 || height == 0 || width > Detail::kMaxDimension ||
        height > Detail::kMaxDimension)
    {
        return Status::Unsupported;
    }
    if (size > kMaxStlBytes)
    {
        return Status::Unsupported;
    }
    if (cancel != nullptr && cancel(cancelCtx))
    {
        return Status::Unsupported;
    }
    std::vector<Triangle> tris;
    bool parsed = false;
    if (LooksLikeBinaryStl(data, size))
    {
        parsed = ParseBinaryStl(data, size, tris);
    }
    else
    {
        parsed = ParseAsciiStl(data, size, tris);
    }
    if (!parsed || tris.empty())
    {
        return Status::Unsupported;
    }
    return RasterizeTriangles(tris, width, height, albedo, frame, cancel, cancelCtx);
}

Status RasterizeStlFile(const wchar_t* path, UINT width, UINT height, COLORREF albedo, Frame& frame,
                        bool (*cancel)(void*), void* cancelCtx)
{
    frame = {};
    if (path == nullptr || path[0] == 0)
    {
        return Status::CannotOpen;
    }
    if (cancel != nullptr && cancel(cancelCtx))
    {
        return Status::Unsupported;
    }
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return Status::CannotOpen;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || static_cast<ULONGLONG>(size.QuadPart) > kMaxStlBytes)
    {
        CloseHandle(file);
        return Status::Unsupported;
    }
    std::vector<BYTE> bytes;
    try
    {
        bytes.resize(static_cast<size_t>(size.QuadPart));
    }
    catch (const std::bad_alloc&)
    {
        CloseHandle(file);
        return Status::OutOfMemory;
    }
    DWORD read = 0;
    size_t offset = 0;
    while (offset < bytes.size())
    {
        if (cancel != nullptr && cancel(cancelCtx))
        {
            CloseHandle(file);
            return Status::Unsupported;
        }
        const DWORD chunk = static_cast<DWORD>((std::min)(bytes.size() - offset, static_cast<size_t>(1024 * 1024)));
        if (!ReadFile(file, bytes.data() + offset, chunk, &read, nullptr) || read == 0)
        {
            CloseHandle(file);
            return Status::CannotOpen;
        }
        offset += read;
    }
    CloseHandle(file);
    if (cancel != nullptr && cancel(cancelCtx))
    {
        return Status::Unsupported;
    }
    return RasterizeStlMemory(bytes.data(), bytes.size(), width, height, albedo, frame, cancel, cancelCtx);
}

} // namespace PictView::Native
