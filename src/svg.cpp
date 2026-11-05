// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-05 | Supersample SVG icon bitmaps and keep a single premul BGRA pass. | FS-panel SVG icons looked muddy and lost detail after a second premultiply, especially at 16px. | | Composer |
// CommentsTranslationProject: TRANSLATED

#include "precomp.h"

#include "svg.h"
#include "cfgdlg.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg\nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg\nanosvgrast.h"

CSVGSprite SVGArrowRight;
CSVGSprite SVGArrowRightSmall;
CSVGSprite SVGArrowMore;
CSVGSprite SVGArrowLess;
CSVGSprite SVGArrowDropDown;

// Alternative: http://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers
// (we could probably find one for shorter versions as well)
//
// The following solution has the advantage that constants are computed within the precompiler
// LOG2_k(n) returns floor(log2(n)) and is valid for values 0 <= n < 1 << k
#define LOG2_2(n) ((n) & 0x2 ? 1 : 0)
#define LOG2_4(n) ((n) & 0xC ? 2 + LOG2_2((n) >> 2) : LOG2_2(n))
#define LOG2_8(n) ((n) & 0xF0 ? 4 + LOG2_4((n) >> 4) : LOG2_4(n))
#define LOG2_16(n) ((n) & 0xFF00 ? 8 + LOG2_8((n) >> 8) : LOG2_8(n))
#define LOG2_32(n) ((n) & 0xFFFF0000 ? 16 + LOG2_16((n) >> 16) : LOG2_16(n))
#define LOG2_64(n) ((n) & 0xFFFFFFFF00000000 ? 32 + LOG2_32((n) >> 32) : LOG2_32(n))

//__popcnt16, __popcnt, __popcnt64
//https://msdn.microsoft.com/en-us/library/bb385231(v=vs.100).aspx

DWORD GetSVGSysColor(int index)
{
    DWORD color = GetSysColor(index);
    DWORD ret = 0xFF000000;
    ret |= GetBValue(color) << 16;
    ret |= GetGValue(color) << 8;
    ret |= GetRValue(color);
    return ret;
}

static DWORD ColorRefToARGB(COLORREF color)
{
    DWORD argb = 0xFF000000;
    argb |= GetBValue(color) << 16;
    argb |= GetGValue(color) << 8;
    argb |= GetRValue(color);
    return argb;
}

//*****************************************************************************
//
// RenderSVGImage
//

char* ReadSVGFile(const char* fileName)
{
    char* buff = NULL;
    HANDLE hFile = HANDLES_Q(CreateFile(fileName, GENERIC_READ,
                                        FILE_SHARE_READ, NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_SEQUENTIAL_SCAN,
                                        NULL));
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD size = GetFileSize(hFile, NULL);
        if (size != INVALID_FILE_SIZE)
        {
            buff = (char*)malloc(size + 1);
            DWORD read;
            if (ReadFile(hFile, buff, size, &read, NULL) && read == size)
            {
                buff[size] = 0;
            }
            else
            {
                TRACE_E("ReadSVGFile(): ReadFile() failed on " << fileName);
                free(buff);
                buff = NULL;
            }
        }
        else
        {
            TRACE_E("ReadSVGFile(): GetFileSize() failed on " << fileName);
        }
        HANDLES(CloseHandle(hFile));
    }
    else
    {
        TRACE_I("ReadSVGFile(): cannot open SVG file " << fileName);
    }
    return buff;
}

// Renders icons for which we have an SVG representation
static char* LoadToolbarSVG(const char* svgName)
{
    // Dynamic package contributions pass an absolute UTF-8 SVG path. Keep
    // the historic toolbar-name lookup for built-in icons, but allow the
    // shared GUI API to render package-owned artwork as well.
    if (svgName != NULL &&
        (strchr(svgName, ':') != NULL || svgName[0] == '\\' || svgName[0] == '/'))
    {
        return ReadSVGFile(svgName);
    }

    char svgFile[2 * MAX_PATH];
    GetModuleFileName(NULL, svgFile, _countof(svgFile));
    char* s = strrchr(svgFile, '\\');
    if (s != NULL)
    {
        if (Configuration.UseWindowsDarkMode)
        {
            sprintf(s + 1, "toolbars\\darkmode\\%s.svg", svgName);
            char* svg = ReadSVGFile(svgFile);
            if (svg != NULL)
                return svg;
        }

        sprintf(s + 1, "toolbars\\%s.svg", svgName);
    }
    char* svg = ReadSVGFile(svgFile);
    if (svg != NULL)
        return svg;

    return NULL;
}

// vykresli ikony pro ktere mame SVG reprezentaci
void RenderSVGImage(NSVGrasterizer* rast, HDC hDC, int x, int y, const char* svgName, int iconSize, COLORREF bkColor, BOOL enabled)
{
    char* svg = LoadToolbarSVG(svgName);
    if (svg != NULL)
    {
        HDC hMemDC = HANDLES(CreateCompatibleDC(NULL));
        BITMAPINFOHEADER bmhdr;
        memset(&bmhdr, 0, sizeof(bmhdr));
        bmhdr.biSize = sizeof(bmhdr);
        bmhdr.biWidth = iconSize;
        bmhdr.biHeight = -iconSize;
        if (bmhdr.biHeight == 0)
            bmhdr.biHeight = -1;
        bmhdr.biPlanes = 1;
        bmhdr.biBitCount = 32;
        bmhdr.biCompression = BI_RGB;
        void* lpMemBits = NULL;
        HBITMAP hMemBmp = HANDLES(CreateDIBSection(hMemDC, (CONST BITMAPINFO*)&bmhdr, DIB_RGB_COLORS, &lpMemBits, NULL, 0));
        SelectObject(hMemDC, hMemBmp);

        RECT r;
        r.left = x;
        r.top = y;
        r.right = x + iconSize;
        r.bottom = y + iconSize;
        SetBkColor(hDC, bkColor);
        ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &r, "", 0, NULL);

        float sysDPIScale = (float)GetScaleForSystemDPI();
        NSVGimage* image = nsvgParse(svg, "px", sysDPIScale);

        if (!enabled)
        {
            DWORD disabledColor = GetSVGSysColor(COLOR_BTNSHADOW); // JRYFIXME - initial draft: where will we take the disabled color from?
            NSVGshape* shape = image->shapes;
            while (shape != NULL)
            {
                if ((shape->fill.color & 0x00FFFFFF) != 0x00FFFFFF)
                    shape->fill.color = disabledColor;
                shape = shape->next;
            }
        }

        float scale = 1.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        if (image->width > 0.0f && image->height > 0.0f)
        {
            scale = min((float)iconSize / image->width, (float)iconSize / image->height);
            offsetX = ((float)iconSize - image->width * scale) / 2.0f;
            offsetY = ((float)iconSize - image->height * scale) / 2.0f;
        }
        nsvgRasterize(rast, image, offsetX, offsetY, scale, (BYTE*)lpMemBits, iconSize, iconSize, iconSize * 4);
        nsvgDelete(image);

        BLENDFUNCTION bf;
        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 0;
        bf.SourceConstantAlpha = 0xff; // want to use per-pixel alpha values
        bf.AlphaFormat = AC_SRC_ALPHA;
        AlphaBlend(hDC, x, y, iconSize, iconSize, hMemDC, 0, 0, iconSize, iconSize, bf);

        HANDLES(DeleteObject(hMemBmp));
        HANDLES(DeleteDC(hMemDC));

        free(svg);
    }
}


// Downsample a 2x premultiplied BGRA buffer into iconSize x iconSize with a
// box filter. Averaging premultiplied channels preserves coverage correctly.
static void DownsamplePremultipliedBGRA2x(const BYTE* src, int iconSize, BYTE* dst)
{
    const int srcSize = iconSize * 2;
    for (int y = 0; y < iconSize; y++)
    {
        const BYTE* row0 = src + (size_t)(y * 2) * srcSize * 4;
        const BYTE* row1 = row0 + srcSize * 4;
        BYTE* out = dst + (size_t)y * iconSize * 4;
        for (int x = 0; x < iconSize; x++)
        {
            const BYTE* p00 = row0 + x * 8;
            const BYTE* p01 = p00 + 4;
            const BYTE* p10 = row1 + x * 8;
            const BYTE* p11 = p10 + 4;
            out[0] = (BYTE)(((unsigned)p00[0] + p01[0] + p10[0] + p11[0] + 2) / 4);
            out[1] = (BYTE)(((unsigned)p00[1] + p01[1] + p10[1] + p11[1] + 2) / 4);
            out[2] = (BYTE)(((unsigned)p00[2] + p01[2] + p10[2] + p11[2] + 2) / 4);
            out[3] = (BYTE)(((unsigned)p00[3] + p01[3] + p10[3] + p11[3] + 2) / 4);
            out += 4;
        }
    }
}

// Converts NanoSVG's premultiplied BGRA into straight BGRA so CreateIconIndirect /
// CIconList::DrawALPHA can blend without dark fringe artifacts. Also defringes
// fully transparent neighbours the same way NanoSVG's unpremultiply helper does.
BOOL UnpremultiplySVGIconBitmap(HBITMAP hBitmap)
{
    if (hBitmap == NULL)
        return FALSE;

    DIBSECTION section;
    memset(&section, 0, sizeof(section));
    if (GetObject(hBitmap, sizeof(section), &section) != sizeof(section) ||
        section.dsBm.bmBits == NULL || section.dsBm.bmBitsPixel != 32 ||
        section.dsBm.bmWidth <= 0 || section.dsBm.bmHeight == 0)
        return FALSE;

    const int width = section.dsBm.bmWidth;
    const int height = abs(section.dsBm.bmHeight);
    const int stride = section.dsBm.bmWidthBytes;
    BYTE* bits = (BYTE*)section.dsBm.bmBits;

    for (int y = 0; y < height; y++)
    {
        BYTE* row = bits + (size_t)y * stride;
        for (int x = 0; x < width; x++)
        {
            BYTE* p = row + x * 4;
            const unsigned a = p[3];
            if (a == 0)
            {
                p[0] = p[1] = p[2] = 0;
            }
            else if (a < 255)
            {
                p[0] = (BYTE)min(255u, (p[0] * 255u + a / 2) / a);
                p[1] = (BYTE)min(255u, (p[1] * 255u + a / 2) / a);
                p[2] = (BYTE)min(255u, (p[2] * 255u + a / 2) / a);
            }
        }
    }

    // Defringe: fill RGB of fully transparent pixels from opaque neighbours so
    // bilinear upscales (Tiles/Thumbnails/DPI) do not pick up black fringes.
    for (int y = 0; y < height; y++)
    {
        BYTE* row = bits + (size_t)y * stride;
        for (int x = 0; x < width; x++)
        {
            BYTE* p = row + x * 4;
            if (p[3] != 0)
                continue;

            unsigned r = 0, g = 0, b = 0, n = 0;
            if (x > 0 && row[(x - 1) * 4 + 3] != 0)
            {
                b += row[(x - 1) * 4 + 0];
                g += row[(x - 1) * 4 + 1];
                r += row[(x - 1) * 4 + 2];
                n++;
            }
            if (x + 1 < width && row[(x + 1) * 4 + 3] != 0)
            {
                b += row[(x + 1) * 4 + 0];
                g += row[(x + 1) * 4 + 1];
                r += row[(x + 1) * 4 + 2];
                n++;
            }
            if (y > 0)
            {
                BYTE* above = bits + (size_t)(y - 1) * stride + x * 4;
                if (above[3] != 0)
                {
                    b += above[0];
                    g += above[1];
                    r += above[2];
                    n++;
                }
            }
            if (y + 1 < height)
            {
                BYTE* below = bits + (size_t)(y + 1) * stride + x * 4;
                if (below[3] != 0)
                {
                    b += below[0];
                    g += below[1];
                    r += below[2];
                    n++;
                }
            }
            if (n > 0)
            {
                p[0] = (BYTE)(b / n);
                p[1] = (BYTE)(g / n);
                p[2] = (BYTE)(r / n);
            }
        }
    }
    return TRUE;
}

static BOOL RenderSVGIconBitmapText(char* svg, int iconSize, BOOL enabled, HBITMAP* hBitmap)
{
    if (hBitmap == NULL)
        return FALSE;
    *hBitmap = NULL;
    if (svg == NULL || iconSize <= 0 || iconSize > 512)
        return FALSE;

    // 2x supersampling keeps thin strokes and logo detail readable at the
    // panel's 16/32/48 buckets (and their DPI-scaled sizes up to 64).
    const int renderSize = (iconSize <= 64) ? iconSize * 2 : iconSize;

    HDC hMemDC = HANDLES(CreateCompatibleDC(NULL));
    if (hMemDC == NULL)
        return FALSE;

    BITMAPINFOHEADER bmhdr;
    memset(&bmhdr, 0, sizeof(bmhdr));
    bmhdr.biSize = sizeof(bmhdr);
    bmhdr.biWidth = iconSize;
    bmhdr.biHeight = -iconSize;
    if (bmhdr.biHeight == 0)
        bmhdr.biHeight = -1;
    bmhdr.biPlanes = 1;
    bmhdr.biBitCount = 32;
    bmhdr.biCompression = BI_RGB;
    void* lpMemBits = NULL;
    HBITMAP hMemBmp = HANDLES(CreateDIBSection(hMemDC, (CONST BITMAPINFO*)&bmhdr, DIB_RGB_COLORS, &lpMemBits, NULL, 0));
    BYTE* renderBits = NULL;
    if (hMemBmp != NULL && lpMemBits != NULL)
    {
        memset(lpMemBits, 0, static_cast<size_t>(iconSize) * iconSize * 4);
        // Parse with the CSS/SVG reference DPI. Callers already pass the target
        // size in device pixels (including system-DPI toolbar sizes).
        NSVGimage* image = nsvgParse(svg, "px", 96.0f);
        if (image != NULL)
        {
            if (!enabled)
            {
                DWORD disabledColor = GetSVGSysColor(COLOR_BTNSHADOW);
                NSVGshape* shape = image->shapes;
                while (shape != NULL)
                {
                    if ((shape->fill.color & 0x00FFFFFF) != 0x00FFFFFF)
                        shape->fill.color = disabledColor;
                    shape = shape->next;
                }
            }
            NSVGrasterizer* rast = nsvgCreateRasterizer();
            if (rast != NULL)
            {
                BYTE* rasterTarget = (BYTE*)lpMemBits;
                int rasterSize = iconSize;
                if (renderSize != iconSize)
                {
                    renderBits = (BYTE*)malloc((size_t)renderSize * renderSize * 4);
                    if (renderBits != NULL)
                    {
                        memset(renderBits, 0, (size_t)renderSize * renderSize * 4);
                        rasterTarget = renderBits;
                        rasterSize = renderSize;
                    }
                }

                float scale = 1.0f;
                float offsetX = 0.0f;
                float offsetY = 0.0f;
                if (image->width > 0.0f && image->height > 0.0f)
                {
                    scale = min((float)rasterSize / image->width, (float)rasterSize / image->height);
                    offsetX = ((float)rasterSize - image->width * scale) / 2.0f;
                    offsetY = ((float)rasterSize - image->height * scale) / 2.0f;
                }
                // nsvgRasterize (with the local NanoSVG patch) emits premultiplied
                // BGRA. Keep that single premul pass — a second multiply here used
                // to darken anti-aliased edges and muddy detailed logos.
                nsvgRasterize(rast, image, offsetX, offsetY, scale, rasterTarget,
                              rasterSize, rasterSize, rasterSize * 4);
                if (renderBits != NULL)
                    DownsamplePremultipliedBGRA2x(renderBits, iconSize, (BYTE*)lpMemBits);

                nsvgDeleteRasterizer(rast);
                *hBitmap = hMemBmp;
                hMemBmp = NULL;
            }
            nsvgDelete(image);
        }
    }
    free(renderBits);
    if (hMemBmp != NULL)
        HANDLES(DeleteObject(hMemBmp));
    HANDLES(DeleteDC(hMemDC));
    return *hBitmap != NULL;
}

BOOL RenderSVGIconBitmap(const char* svgName, int iconSize, BOOL enabled, HBITMAP* hBitmap)
{
    char* svg = LoadToolbarSVG(svgName);
    if (svg == NULL)
    {
        if (hBitmap != NULL)
            *hBitmap = NULL;
        return FALSE;
    }
    BOOL result = RenderSVGIconBitmapText(svg, iconSize, enabled, hBitmap);
    free(svg);
    return result;
}

BOOL RenderSVGIconBitmapFromFile(const char* svgFile, int iconSize, BOOL enabled, HBITMAP* hBitmap)
{
    char* svg = svgFile != NULL ? ReadSVGFile(svgFile) : NULL;
    if (svg == NULL)
    {
        if (hBitmap != NULL)
            *hBitmap = NULL;
        return FALSE;
    }
    BOOL result = RenderSVGIconBitmapText(svg, iconSize, enabled, hBitmap);
    free(svg);
    return result;
}

BOOL CreateDarkModeIconBitmap(HBITMAP hSource, HBITMAP* hBitmap)
{
    if (hBitmap == NULL)
        return FALSE;
    *hBitmap = NULL;
    if (hSource == NULL)
        return FALSE;

    BITMAP sourceInfo;
    if (GetObject(hSource, sizeof(sourceInfo), &sourceInfo) != sizeof(sourceInfo) ||
        sourceInfo.bmWidth <= 0 || sourceInfo.bmHeight <= 0)
        return FALSE;

    const size_t pixelCount = static_cast<size_t>(sourceInfo.bmWidth) *
                              static_cast<size_t>(sourceInfo.bmHeight);
    if (pixelCount > static_cast<size_t>(0xFFFFFFFF) / 4)
        return FALSE;
    const size_t bufferSize = pixelCount * 4;
    DWORD* pixels = static_cast<DWORD*>(malloc(bufferSize));
    if (pixels == NULL)
        return FALSE;

    HDC hDC = HANDLES(GetDC(NULL));
    BITMAPINFO info;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = sourceInfo.bmWidth;
    info.bmiHeader.biHeight = -sourceInfo.bmHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    BOOL result = FALSE;
    if (hDC != NULL && GetDIBits(hDC, hSource, 0, sourceInfo.bmHeight,
                                 pixels, &info, DIB_RGB_COLORS) != 0)
    {
        for (size_t i = 0; i < pixelCount; ++i)
        {
            DWORD pixel = pixels[i];
            BYTE alpha = static_cast<BYTE>(pixel >> 24);
            if (alpha == 0)
                continue;
            BYTE red = static_cast<BYTE>(pixel & 0xff);
            BYTE green = static_cast<BYTE>((pixel >> 8) & 0xff);
            BYTE blue = static_cast<BYTE>((pixel >> 16) & 0xff);
            int unpremultipliedRed = min(255, (red * 255 + alpha / 2) / alpha);
            int unpremultipliedGreen = min(255, (green * 255 + alpha / 2) / alpha);
            int unpremultipliedBlue = min(255, (blue * 255 + alpha / 2) / alpha);
            int luminance = (unpremultipliedRed * 54 +
                             unpremultipliedGreen * 183 +
                             unpremultipliedBlue * 19) / 256;
            if (luminance < 160)
            {
                const int targetLuminance = 230;
                if (luminance == 0)
                {
                    unpremultipliedRed = targetLuminance;
                    unpremultipliedGreen = targetLuminance;
                    unpremultipliedBlue = targetLuminance;
                }
                else
                {
                    unpremultipliedRed = min(255, unpremultipliedRed * targetLuminance / luminance);
                    unpremultipliedGreen = min(255, unpremultipliedGreen * targetLuminance / luminance);
                    unpremultipliedBlue = min(255, unpremultipliedBlue * targetLuminance / luminance);
                }
                red = static_cast<BYTE>((unpremultipliedRed * alpha + 127) / 255);
                green = static_cast<BYTE>((unpremultipliedGreen * alpha + 127) / 255);
                blue = static_cast<BYTE>((unpremultipliedBlue * alpha + 127) / 255);
                pixels[i] = (static_cast<DWORD>(alpha) << 24) |
                            (static_cast<DWORD>(blue) << 16) |
                            (static_cast<DWORD>(green) << 8) | red;
            }
        }

        HDC destinationDC = HANDLES(CreateCompatibleDC(NULL));
        if (destinationDC != NULL)
        {
            void* destinationBits = NULL;
            HBITMAP resultBitmap = HANDLES(CreateDIBSection(destinationDC, &info,
                                                              DIB_RGB_COLORS,
                                                              &destinationBits, NULL, 0));
            if (resultBitmap != NULL && destinationBits != NULL)
            {
                memcpy(destinationBits, pixels, bufferSize);
                *hBitmap = resultBitmap;
                result = TRUE;
            }
            if (!result && resultBitmap != NULL)
                HANDLES(DeleteObject(resultBitmap));
            HANDLES(DeleteDC(destinationDC));
        }
    }
    if (hDC != NULL)
        HANDLES(ReleaseDC(NULL, hDC));
    free(pixels);
    return result;
}

//*****************************************************************************
//
// CSVGSprite
//

CSVGSprite::CSVGSprite()
{
    for (int i = 0; i < SVGSTATE_COUNT; i++)
        HBitmaps[i] = NULL;
    Clean();
}

CSVGSprite::~CSVGSprite()
{
    Clean();
}

void CSVGSprite::Clean()
{
    for (int i = 0; i < SVGSTATE_COUNT; i++)
    {
        if (HBitmaps[i] != NULL)
        {
            HANDLES(DeleteObject(HBitmaps[i]));
            HBitmaps[i] = NULL;
        }
    }
    Width = -1;
    Height = -1;
}

char* CSVGSprite::LoadSVGResource(int resID)
{
    char* ret = NULL;
    HRSRC hRsrc = FindResource(HInstance, MAKEINTRESOURCE(resID), RT_RCDATA);
    if (hRsrc != NULL)
    {
        char* rawSVG = (char*)LoadResource(HInstance, hRsrc);
        if (rawSVG != NULL)
        {
            DWORD size = SizeofResource(HInstance, hRsrc);
            if (size > 0)
            {
                NSVGimage* image = NULL;
                NSVGrasterizer* rast = NULL;

                char* terminatedSVG = (char*)malloc(size + 1);
                memcpy(terminatedSVG, rawSVG, size);
                terminatedSVG[size] = 0;
                ret = terminatedSVG;
            }
            else
            {
                TRACE_E("LoadSVGResource() Invalid resource data! resID=" << resID);
            }
        }
        else
        {
            TRACE_E("LoadSVGResource() Cannot load resource! resID=" << resID);
        }
    }
    else
    {
        TRACE_E("LoadSVGResource() Resource not found! resID=" << resID);
    }
    return ret;
}

void CSVGSprite::GetScaleAndSize(const NSVGimage* image, const SIZE* sz, float* scale, int* width, int* height)
{
    if (sz->cx != -1 || sz->cy != -1)
    {
        float scaleX, scaleY;
        if (sz->cx != -1)
            scaleX = sz->cx / image->width;
        if (sz->cy != -1)
            scaleY = sz->cy / image->height;
        if (sz->cx == -1)
        {
            *scale = scaleY;
            *height = sz->cy;
            *width = (int)(image->width * *scale);
        }
        else
        {
            if (sz->cy == -1)
            {
                *scale = scaleX;
                *width = sz->cx;
                *height = (int)(image->height * *scale);
            }
            else
            {
                *scale = min(scaleX, scaleY);
                *width = (int)(image->width * *scale);
                *height = (int)(image->height * *scale);
            }
        }
    }
    else
    {
        *scale = (float)GetScaleForSystemDPI() / 100;
        *width = (int)(image->width * *scale);
        *height = (int)(image->height * *scale);
    }
}
/*
HBITMAP
CSVGSprite::LoadSVGToBitmap(int resID, SIZE *sz)
{
  if (sz == NULL)
    TRACE_C("LoadSVGToBitmap(): invalid parameters!");

  HBITMAP hMemBmp = NULL;

  char *terminatedSVG = LoadSVGResource(resID);
  if (terminatedSVG != NULL)
  {
    NSVGimage *image = NULL;
    image = nsvgParse(terminatedSVG, "px", (float)GetSystemDPI());
    free(terminatedSVG);

    float scale;
    int w, h;
    GetScaleAndSize(image, sz, &scale, &w, &h);

    NSVGrasterizer *rast = NULL;
    rast = nsvgCreateRasterizer();

    HDC hMemDC = HANDLES(CreateCompatibleDC(NULL));
    BITMAPINFOHEADER bmhdr;
    memset(&bmhdr, 0, sizeof(bmhdr));
    bmhdr.biSize = sizeof(bmhdr);
    bmhdr.biWidth = w;
    bmhdr.biHeight = -h;
    if (bmhdr.biHeight == 0) bmhdr.biHeight = -1;
    bmhdr.biPlanes = 1;
    bmhdr.biBitCount = 32;
    bmhdr.biCompression = BI_RGB;
    void *lpMemBits = NULL;
    hMemBmp = HANDLES(CreateDIBSection(hMemDC, (CONST BITMAPINFO *)&bmhdr, DIB_RGB_COLORS, &lpMemBits, NULL, 0));
    HANDLES(DeleteDC(hMemDC));

    nsvgRasterize(rast, image, 0, 0, scale, (BYTE*)lpMemBits, w, h, w * 4);

    sz->cx = w;
    sz->cy = h;

    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
  }
  return hMemBmp;
}
*/
void CSVGSprite::CreateDIB(int width, int height, HBITMAP* hMemBmp, void** lpMemBits)
{
    HDC hMemDC = HANDLES(CreateCompatibleDC(NULL));
    BITMAPINFOHEADER bmhdr;
    memset(&bmhdr, 0, sizeof(bmhdr));
    bmhdr.biSize = sizeof(bmhdr);
    bmhdr.biWidth = width;
    bmhdr.biHeight = -height;
    if (bmhdr.biHeight == 0)
        bmhdr.biHeight = -1;
    bmhdr.biPlanes = 1;
    bmhdr.biBitCount = 32;
    bmhdr.biCompression = BI_RGB;
    *hMemBmp = HANDLES(CreateDIBSection(hMemDC, (CONST BITMAPINFO*)&bmhdr, DIB_RGB_COLORS, lpMemBits, NULL, 0));
    HANDLES(DeleteDC(hMemDC));
}

void CSVGSprite::ColorizeSVG(NSVGimage* image, DWORD state)
{
    if (state == SVGSTATE_ORIGINAL)
        return;

    DWORD color;
    switch (state)
    {
    case SVGSTATE_ENABLED:
        if (DarkModeShouldUseDarkColors())
            color = ColorRefToARGB(GetCOLORREF(CurrentColors[ITEM_FG_NORMAL]));
        else
            color = GetSVGSysColor(COLOR_BTNTEXT);
        break;

    case SVGSTATE_DISABLED:
        color = GetSVGSysColor(COLOR_BTNSHADOW);
        break;

    default:
        color = GetSVGSysColor(COLOR_BTNTEXT);
        TRACE_E("CSVGSprite::ColorizeSVG() unknown state=" << state);
    }
    NSVGshape* shape = image->shapes;
    while (shape != NULL)
    {
        shape->fill.color = color;
        shape = shape->next;
    }
}

BOOL CSVGSprite::Load(int resID, int width, int height, DWORD states)
{
    if (states == 0 || states >= (1 << SVGSTATE_COUNT))
    {
        TRACE_E("CSVGSprite::Load() wrong states combination: " << states);
        states |= SVGSTATE_ORIGINAL;
    }
    Clean();

    char* terminatedSVG = LoadSVGResource(resID);
    if (terminatedSVG != NULL)
    {
        NSVGimage* image = NULL;
        image = nsvgParse(terminatedSVG, "px", (float)GetSystemDPI());
        free(terminatedSVG);

        float scale;
        SIZE sz = {width, height};
        GetScaleAndSize(image, &sz, &scale, &Width, &Height);

        NSVGrasterizer* rast = NULL;
        rast = nsvgCreateRasterizer();

        for (int i = 0; i < SVGSTATE_COUNT; i++)
        {
            DWORD state = 1 << i;
            if (states & state)
            {
                void* lpMemBits;
                CreateDIB(Width, Height, &HBitmaps[i], &lpMemBits);
                ColorizeSVG(image, state);
                nsvgRasterize(rast, image, 0, 0, scale, (BYTE*)lpMemBits, Width, Height, Width * 4);
            }
        }

        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
    }
    return TRUE;
}

void CSVGSprite::GetSize(SIZE* s)
{
    s->cx = Width;
    s->cy = Height;
}

int CSVGSprite::GetWidth()
{
    return Width;
}

int CSVGSprite::GetHeight()
{
    return Height;
}

void CSVGSprite::AlphaBlend(HDC hDC, int x, int y, int width, int height, DWORD state)
{
    HDC hMemTmpDC = HANDLES(CreateCompatibleDC(hDC));
    int index = LOG2_32(state);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemTmpDC, HBitmaps[index]);

    if (width == -1)
        width = Width;
    if (height == -1)
        height = Height;

    BLENDFUNCTION bf;
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0xff; // want to use per-pixel alpha values
    bf.AlphaFormat = AC_SRC_ALPHA;
    ::AlphaBlend(hDC, x, y, width, height, hMemTmpDC, 0, 0, Width, Height, bf);

    SelectObject(hMemTmpDC, hOldBitmap);
    HANDLES(DeleteDC(hMemTmpDC));
}
