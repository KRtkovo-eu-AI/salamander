// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

// The generated fragment contains the actual production class, painter and
// converter. Only window/paint plumbing is replaced; GDI and path compaction run
// against real memory device contexts, without creating or operating UI windows.
#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{
const int SurfaceWidth = 24000;
const int SurfaceHeight = 32;
HDC PaintDC;
RECT PaintRect;
std::wstring PaintedText;
bool InvalidAnsiText = false;
int Checks = 0;
int Failures = 0;
const char* CurrentCase = "setup";

void Check(bool condition, const char* description)
{
    ++Checks;
    if (!condition)
    {
        ++Failures;
        if (Failures <= 30)
            std::fprintf(stderr, "FAIL [%s]: %s\n", CurrentCase, description);
    }
}

std::string Utf8(const std::wstring& text)
{
    int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, NULL, 0, NULL, NULL);
    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &result[0], length, NULL, NULL);
    result.resize(length - 1);
    return result;
}

bool ValidSurrogates(const std::wstring& text)
{
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] >= 0xD800 && text[i] <= 0xDBFF)
        {
            if (++i >= text.size() || text[i] < 0xDC00 || text[i] > 0xDFFF)
                return false;
        }
        else if (text[i] >= 0xDC00 && text[i] <= 0xDFFF)
            return false;
    }
    return true;
}

struct Surface
{
    HDC DC;
    HBITMAP Bitmap;
    HGDIOBJ OldBitmap;
    void* Pixels;

    Surface()
    {
        DC = CreateCompatibleDC(NULL);
        BITMAPINFO info = {};
        info.bmiHeader.biSize = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth = SurfaceWidth;
        info.bmiHeader.biHeight = -SurfaceHeight;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        Bitmap = CreateDIBSection(DC, &info, DIB_RGB_COLORS, &Pixels, NULL, 0);
        OldBitmap = SelectObject(DC, Bitmap);
        Check(DC != NULL && Bitmap != NULL, "create native memory drawing surface");
    }

    ~Surface()
    {
        SelectObject(DC, OldBitmap);
        DeleteObject(Bitmap);
        DeleteDC(DC);
    }
};

HDC TestBeginPaint(HWND, PAINTSTRUCT* paint)
{
    paint->hdc = PaintDC;
    return PaintDC;
}
BOOL TestEndPaint(HWND, const PAINTSTRUCT*) { return TRUE; }
BOOL TestGetClientRect(HWND, RECT* rect) { *rect = PaintRect; return TRUE; }
BOOL TestInvalidateRect(HWND, const RECT*, BOOL) { return TRUE; }
BOOL TestUpdateWindow(HWND) { return TRUE; }

[[maybe_unused]] int TestDrawTextW(HDC dc, LPCWSTR text, int length, RECT* rect, UINT format)
{
    PaintedText.assign(text, length < 0 ? wcslen(text) : static_cast<size_t>(length));
    return DrawTextW(dc, text, length, rect, format);
}

[[maybe_unused]] int TestDrawTextA(HDC dc, LPCSTR text, int length, RECT* rect, UINT format)
{
    int bytes = length < 0 ? static_cast<int>(strlen(text)) : length;
    DWORD flags = GetACP() == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0;
    int count = MultiByteToWideChar(CP_ACP, flags, text, bytes, NULL, 0);
    InvalidAnsiText = bytes != 0 && count == 0;
    PaintedText.assign(count, L'\0');
    if (count != 0)
        MultiByteToWideChar(CP_ACP, flags, text, bytes, &PaintedText[0], count);
    return DrawTextA(dc, text, length, rect, format);
}
}

class CWindow
{
public:
    HWND HWindow = NULL;
    virtual ~CWindow() {}
    virtual LRESULT WindowProc(UINT, WPARAM, LPARAM) { return 0; }
};
HFONT EnvFont;
const int SALCOL_INACTIVE_CAPTION_BK = 0;
const int SALCOL_INACTIVE_CAPTION_FG = 1;
COLORREF FileCompGetHostColor(int, COLORREF fallback) { return fallback; }
#define SAL_MAX_PATH 32768
#define CALL_STACK_MESSAGE1(...)
#define CALL_STACK_MESSAGE2(...)
#define CALL_STACK_MESSAGE4(...)
#define BeginPaint TestBeginPaint
#define EndPaint TestEndPaint
#define GetClientRect TestGetClientRect
#define InvalidateRect TestInvalidateRect
#define UpdateWindow TestUpdateWindow
#define DrawTextA TestDrawTextA
#define DrawTextW TestDrawTextW
#pragma warning(push)
#pragma warning(disable : 4505) // The old baseline does not use its path converter.
#include "filecomp_header_generated.h"
#pragma warning(pop)
#undef BeginPaint
#undef EndPaint
#undef GetClientRect
#undef InvalidateRect
#undef UpdateWindow
#undef DrawTextA
#undef DrawTextW

namespace
{
class Header : public CFileHeaderWindow
{
public:
    explicit Header(const char* text) : CFileHeaderWindow(text) { WindowProc(WM_CREATE, 0, 0); }
    void Paint() { WindowProc(WM_PAINT, 0, 0); }
};

void PaintAndCheck(Header& header, const std::wstring& expected, int width,
                   Surface& actual, Surface& reference)
{
    PaintDC = actual.DC;
    PaintRect = {0, 0, width, SurfaceHeight};
    PaintedText.clear();
    InvalidAnsiText = false;
    header.Paint();
    Check(!InvalidAnsiText, "rendered ANSI text is a valid sequence in the process code page");
    bool validSurrogates = ValidSurrogates(PaintedText);
    Check(validSurrogates, "compaction does not split a surrogate pair");
    if (!validSurrogates)
    {
        std::fprintf(stderr, "  width=%d; UTF-16:", width);
        for (wchar_t unit : PaintedText)
            std::fprintf(stderr, " %04X", static_cast<unsigned int>(unit));
        std::fprintf(stderr, "\n");
    }
    if (width == SurfaceWidth)
    {
        Check(PaintedText == expected, "wide header preserves the entire original Unicode path");
        HBRUSH background = CreateSolidBrush(GetSysColor(COLOR_INACTIVECAPTION));
        FillRect(reference.DC, &PaintRect, background);
        DeleteObject(background);
        HFONT oldFont = static_cast<HFONT>(SelectObject(reference.DC, EnvFont));
        SetBkColor(reference.DC, GetSysColor(COLOR_INACTIVECAPTION));
        SetTextColor(reference.DC, GetSysColor(COLOR_INACTIVECAPTIONTEXT));
        RECT textRect = {1, 1, width - 1, SurfaceHeight - 1};
        DrawTextW(reference.DC, expected.c_str(), -1, &textRect, DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(reference.DC, oldFont);
        GdiFlush();
        Check(memcmp(actual.Pixels, reference.Pixels,
                     static_cast<size_t>(SurfaceWidth) * SurfaceHeight * 4) == 0,
              "production header pixels match native Unicode rendering");
    }
}

void TestPath(const char* name, const std::wstring& expected, Surface& actual, Surface& reference,
              bool sweepSurrogateBoundaries = false)
{
    CurrentCase = name;
    std::string bytes = Utf8(expected);
    Header constructed(bytes.c_str());
    PaintAndCheck(constructed, expected, SurfaceWidth, actual, reference);
    Header changed("previous.txt");
    changed.SetText(bytes.c_str());
    PaintAndCheck(changed, expected, SurfaceWidth, actual, reference);
    for (int width : {2, 4, 8, 16, 32, 64, 120, 240, 480})
        PaintAndCheck(changed, expected, width, actual, reference);
    if (sweepSurrogateBoundaries)
        for (int width = 1; width <= 512; ++width)
            PaintAndCheck(changed, expected, width, actual, reference);
    PaintAndCheck(changed, expected, SurfaceWidth, actual, reference);
}
}

int main()
{
    EnvFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                          DEFAULT_PITCH, L"MS Shell Dlg");
    Check(EnvFont != NULL, "create header font");
    Surface actual;
    Surface reference;
    TestPath("ASCII path", L"C:\\data\\file.txt", actual, reference);
    TestPath("Unicode filename", L"C:\\data\\žluťoučký-日本-😀.txt", actual, reference);
    TestPath("Unicode directory", L"C:\\česká-日本\\file.txt", actual, reference);
    TestPath("multiple Unicode directories", L"C:\\česká\\日本\\документы\\soubor.txt", actual, reference);
    TestPath("UTF-8 length above 260", L"C:\\" + std::wstring(100, L'日') + L"\\soubor.txt", actual, reference);
    std::wstring longPath = L"C:\\";
    for (int component = 0; component < 5; ++component)
        longPath += std::wstring(70, L'ř') + L"\\";
    TestPath("UTF-16 length above 260", longPath + L"soubor.txt", actual, reference);
    TestPath("surrogate pair at compaction boundary", L"C:\\😀😀😀😀😀😀😀😀😀😀\\😀😀😀😀.txt", actual, reference, true);
    TestPath("short text", L"x", actual, reference);
    TestPath("empty text", L"", actual, reference);
    if (GetACP() == 1250)
    {
        CurrentCase = "legacy ACP1250 fallback";
        const char* legacy = "C:\\data\\p\xF8\xEDli\x9A.txt";
        Header header(legacy);
        PaintAndCheck(header, L"C:\\data\\příliš.txt", SurfaceWidth, actual, reference);
        header.SetText(legacy);
        PaintAndCheck(header, L"C:\\data\\příliš.txt", SurfaceWidth, actual, reference);
    }
    std::printf("ACP=%u; %d checks, %d failures\n", GetACP(), Checks, Failures);
    DeleteObject(EnvFont);
    return Failures == 0 ? 0 : 1;
}
