// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
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


static BOOL RenderSVGIconBitmapText(char* svg, int iconSize, BOOL enabled, HBITMAP* hBitmap)
{
    if (hBitmap == NULL)
        return FALSE;
    *hBitmap = NULL;
    if (svg == NULL)
        return FALSE;

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
    if (hMemBmp != NULL && lpMemBits != NULL)
    {
        memset(lpMemBits, 0, static_cast<size_t>(iconSize) * iconSize * 4);
        float sysDPIScale = (float)GetScaleForSystemDPI();
        NSVGimage* image = nsvgParse(svg, "px", sysDPIScale);
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
                DWORD* pixels = (DWORD*)lpMemBits;
                for (int i = 0; i < iconSize * iconSize; i++)
                {
                    BYTE alpha = (BYTE)(pixels[i] >> 24);
                    BYTE red = (BYTE)(pixels[i] & 0xff);
                    BYTE green = (BYTE)((pixels[i] >> 8) & 0xff);
                    BYTE blue = (BYTE)((pixels[i] >> 16) & 0xff);
                    red = (BYTE)((red * alpha + 127) / 255);
                    green = (BYTE)((green * alpha + 127) / 255);
                    blue = (BYTE)((blue * alpha + 127) / 255);
                    pixels[i] = ((DWORD)alpha << 24) | ((DWORD)blue << 16) | ((DWORD)green << 8) | red;
                }
                nsvgDeleteRasterizer(rast);
                *hBitmap = hMemBmp;
                hMemBmp = NULL;
            }
            nsvgDelete(image);
        }
    }
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
