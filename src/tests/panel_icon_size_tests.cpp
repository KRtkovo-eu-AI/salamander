// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <windows.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
struct IconFixture
{
    std::vector<BYTE> bytes;
    std::map<int, size_t> offsets;
};

void Put16(std::vector<BYTE>& b, size_t p, WORD v)
{
    b[p] = static_cast<BYTE>(v);
    b[p + 1] = static_cast<BYTE>(v >> 8);
}

void Put32(std::vector<BYTE>& b, size_t p, DWORD v)
{
    Put16(b, p, static_cast<WORD>(v));
    Put16(b, p + 2, static_cast<WORD>(v >> 16));
}

IconFixture MakeFixture()
{
    const int sizes[] = {16, 24, 32};
    const COLORREF colors[] = {RGB(220, 40, 40), RGB(40, 200, 60), RGB(40, 80, 220)};
    IconFixture fixture;
    fixture.bytes.resize(6 + 16 * 3);
    Put16(fixture.bytes, 0, 0);
    Put16(fixture.bytes, 2, 1);
    Put16(fixture.bytes, 4, 3);
    size_t cursor = fixture.bytes.size();
    for (int i = 0; i < 3; ++i)
    {
        const int size = sizes[i];
        const DWORD xorBytes = static_cast<DWORD>(size * size * 4);
        const DWORD maskBytes = static_cast<DWORD>(((size + 31) / 32) * 4 * size);
        const DWORD imageBytes = 40 + xorBytes + maskBytes;
        fixture.offsets[size] = cursor;
        const size_t entry = 6 + 16 * i;
        fixture.bytes[entry] = static_cast<BYTE>(size);
        fixture.bytes[entry + 1] = fixture.bytes[entry];
        Put16(fixture.bytes, entry + 4, 1);
        Put16(fixture.bytes, entry + 6, 32);
        Put32(fixture.bytes, entry + 8, imageBytes);
        Put32(fixture.bytes, entry + 12, static_cast<DWORD>(cursor));
        fixture.bytes.resize(cursor + imageBytes);
        Put32(fixture.bytes, cursor, 40);
        Put32(fixture.bytes, cursor + 4, size);
        Put32(fixture.bytes, cursor + 8, size * 2);
        Put16(fixture.bytes, cursor + 12, 1);
        Put16(fixture.bytes, cursor + 14, 32);
        Put32(fixture.bytes, cursor + 16, 0);
        Put32(fixture.bytes, cursor + 20, xorBytes);
        const DWORD pixel = static_cast<DWORD>(GetBValue(colors[i])) | (static_cast<DWORD>(GetGValue(colors[i])) << 8) | (static_cast<DWORD>(GetRValue(colors[i])) << 16) | 0xff000000u;
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x)
                Put32(fixture.bytes, cursor + 40 + static_cast<size_t>(y * size + x) * 4, pixel);
        cursor += imageBytes;
    }
    return fixture;
}

HICON LoadExact(const IconFixture& fixture, int pixelSize)
{
    const auto it = fixture.offsets.find(pixelSize);
    if (it == fixture.offsets.end())
        return NULL;
    const BYTE* image = fixture.bytes.data() + it->second;
    const DWORD imageSize = static_cast<DWORD>(fixture.bytes.size() - it->second);
    return CreateIconFromResourceEx(const_cast<BYTE*>(image), imageSize, TRUE, 0x00030000,
                                    pixelSize, pixelSize, LR_CREATEDIBSECTION);
}

COLORREF Sample(HICON icon, int size)
{
    HDC screen = GetDC(NULL);
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, size, size);
    ReleaseDC(NULL, screen);
    HGDIOBJ old = SelectObject(dc, bitmap);
    PatBlt(dc, 0, 0, size, size, BLACKNESS);
    DrawIconEx(dc, 0, 0, icon, size, size, 0, NULL, DI_NORMAL);
    COLORREF color = GetPixel(dc, size / 2, size / 2);
    SelectObject(dc, old);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return color;
}

int Fail(const char* message)
{
    std::cerr << "FAIL: " << message << "\n";
    return 1;
}
}

int main()
{
    const IconFixture fixture = MakeFixture();
    const COLORREF expected[] = {RGB(220, 40, 40), RGB(40, 200, 60), RGB(40, 80, 220)};
    const int sequence[] = {16, 32, 24, 16};
    std::map<int, HICON> cache;
    std::mutex cacheMutex;
    for (int size : sequence)
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (cache.find(size) == cache.end())
            cache[size] = LoadExact(fixture, size);
        if (cache[size] == NULL || Sample(cache[size], size) != expected[size == 16 ? 0 : size == 24 ? 1 : 2])
            return Fail("pixel cache returned artwork from another icon size");
    }
    if (cache.size() != 3)
        return Fail("16/24/32 requests did not retain independent cache entries");

    std::thread panelA([&] {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (cache[16] == NULL || Sample(cache[16], 16) != expected[0])
            std::abort();
    });
    std::thread panelB([&] {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (cache[32] == NULL || Sample(cache[32], 32) != expected[2])
            std::abort();
    });
    panelA.join();
    panelB.join();
    for (const auto& item : cache)
        DestroyIcon(item.second);
    std::cout << "panel_icon_size_tests: ok\n";
    return 0;
}
