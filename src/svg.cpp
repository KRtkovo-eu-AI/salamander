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
struct CBuiltinToolbarSVG
{
    const char* Name;
    const char* Svg;
};

static char* DuplicateToolbarSVG(const char* svg)
{
    size_t len = strlen(svg);
    char* buffer = (char*)malloc(len + 1);
    if (buffer != NULL)
        memcpy(buffer, svg, len + 1);
    else
        TRACE_E("DuplicateToolbarSVG(): malloc() failed");
    return buffer;
}

static char* LoadToolbarSVG(const char* svgName)
{
    static const CBuiltinToolbarSVG BuiltinToolbarSVGs[] = {
        {"TabsClose",
         R"SVG(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" id="Icon" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
     width="16px" height="16px" viewBox="0 0 16 16" enable-background="new 0 0 16 16" xml:space="preserve">
  <g id="Icon_1_">
    <path fill="#414141" d="M2,5h4.8L8,7h6v5H2V5z"/>
    <path fill="#FFFFFF" d="M3,6h3.9L7.6,8H13v3H3V6z"/>
    <path fill="#C23A3A" d="M6.2,8l0.8-0.8L8,8.2l1-1l0.8,0.8L8.6,9.2l1.2,1.2L9,11.2l-1-1l-1,1l-0.8-0.8l1.2-1.2L6.2,8z"/>
  </g>
</svg>
)SVG"},
        {"TabsDuplicate",
         R"SVG(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" id="Icon" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
     width="16px" height="16px" viewBox="0 0 16 16" enable-background="new 0 0 16 16" xml:space="preserve">
  <g id="Icon_1_">
    <path fill="#6B7FA5" d="M4,4h4.5L9.5,6H14v4H4V4z"/>
    <path fill="#E6EFFB" d="M5,5h3.3L9,7h4v2H5V5z"/>
    <path fill="#414141" d="M2,7h4.8L8,9h6v4H2V7z"/>
    <path fill="#FFFFFF" d="M3,8h3.9L7.6,10H13v2H3V8z"/>
  </g>
</svg>
)SVG"},
        {"TabsNew",
         R"SVG(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" id="Icon" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
     width="16px" height="16px" viewBox="0 0 16 16" enable-background="new 0 0 16 16" xml:space="preserve">
  <g id="Icon_1_">
    <path fill="#414141" d="M2,5h4.8L8,7h6v5H2V5z"/>
    <path fill="#FFFFFF" d="M3,6h3.9L7.6,8H13v3H3V6z"/>
  </g>
</svg>
)SVG"},
        {"TabsNext",
         R"SVG(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" id="Icon" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
     width="16px" height="16px" viewBox="0 0 16 16" enable-background="new 0 0 16 16" xml:space="preserve">
  <g id="Icon_1_">
    <path fill="#414141" d="M2,5h4.8L8,7h6v5H2V5z"/>
    <path fill="#FFFFFF" d="M3,6h3.9L7.6,8H13v3H3V6z"/>
    <polygon fill="#2A5496" points="6.2,8.5 6.2,11.5 8.2,11.5 8.2,12.5 11,10 8.2,7.5 8.2,8.5"/>
  </g>
</svg>
)SVG"},
        {"CommandPrompt",
         R"SVG(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<!-- Creator: CorelDRAW Standard 2024 -->
<svg xmlns="http://www.w3.org/2000/svg" xml:space="preserve" width="16px" height="13px" version="1.1" style="shape-rendering:geometricPrecision; text-rendering:geometricPrecision; image-rendering:optimizeQuality; fill-rule:evenodd; clip-rule:evenodd" viewBox="0 0 1.58 1.284" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xodm="http://www.corel.com/coreldraw/odm/2003">
 <defs><style type="text/css"><![CDATA[.fil2 {fill:black}.fil1 {fill:#687991}.fil5 {fill:#8F9297}.fil3 {fill:#CACCCD}.fil0 {fill:#D3D9E0}.fil4 {fill:white}]]></style></defs>
 <g id="Layer_x0020_1"><metadata id="CorelCorpID_0Corel-Layer"/><g id="_2203928280400">
   <path fill="#D3D9E0" d="M0.099 0.099l0.098 0 0.099 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0 0.099 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 0 -0.099zm1.185 0l0.099 0 0 0.099 -0.099 0 0 -0.099zm-0.198 0l0.099 0 0 0.099 -0.099 0 0 -0.099z"/>
   <path fill="#687991" d="M1.383 0.099l0.098 0 0 0.099 -0.098 0 0 -0.099zm-0.395 0l0.098 0 0 0.099 -0.098 0 0 -0.099zm0.197 0l0.099 0 0 0.099 -0.099 0 0 -0.099z"/>
   <path fill="black" d="M0.099 0.296l0.098 0 0.099 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099zm1.284 0.692l0 -0.099 -0.099 0 -0.099 0 0 0.099 0.099 0 0.099 0zm-0.297 0l0 -0.099 0 -0.099 0 -0.099 -0.098 0 0 -0.098 0 -0.099 -0.099 0 0 0.099 0 0.098 0 0.099 0.099 0 0 0.099 0 0.099 0.098 0zm-0.592 0l0 -0.099 -0.099 0 -0.099 0 0 0.099 0.099 0 0.099 0zm0.296 -0.198l-0.099 0 0 0.099 0.099 0 0 -0.099zm-0.197 0l-0.099 0 0 0.099 0.099 0 0 -0.099zm-0.297 0l0 -0.099 0 -0.098 -0.099 0 0 0.098 0 0.099 0 0.099 0.099 0 0 -0.099zm0.494 -0.197l-0.099 0 0 0.098 0.099 0 0 -0.098zm-0.197 0l-0.099 0 0 0.098 0.099 0 0 -0.098zm-0.099 0l0 -0.099 -0.099 0 -0.099 0 0 0.099 0.099 0 0.099 0z"/>
   <polygon fill="#CACCCD" points="0.099,0.198 0.197,0.198 0.296,0.198 0.395,0.198 0.494,0.198 0.593,0.198 0.691,0.198 0.79,0.198 0.889,0.198 0.988,0.198 1.086,0.198 1.185,0.198 1.284,0.198 1.383,0.198 1.481,0.198 1.481,0.296 1.383,0.296 1.284,0.296 1.185,0.296 1.086,0.296 0.988,0.296 0.889,0.296 0.79,0.296 0.691,0.296 0.593,0.296 0.494,0.296 0.395,0.296 0.296,0.296 0.197,0.296 0.099,0.296 "/>
   <path fill="white" d="M0.296 0.494l0.099 0 0.099 0 0 0.099 0.099 0 0 0.098 -0.099 0 0 -0.098 -0.099 0 -0.099 0 0 0.098 0 0.099 0 0.099 0.099 0 0.099 0 0 -0.099 0.099 0 0 0.099 -0.099 0 0 0.099 -0.099 0 -0.099 0 0 -0.099 -0.099 0 0 -0.099 0 -0.099 0 -0.098 0.099 0 0 -0.099zm0.79 0.296l0 0.099 0 0.099 -0.098 0 0 -0.099 0 -0.099 -0.099 0 0 -0.099 0 -0.098 0 -0.099 0.099 0 0 0.099 0 0.098 0.098 0 0 0.099zm-0.395 0l0.099 0 0 0.099 -0.099 0 0 -0.099zm0.593 0.099l0.099 0 0 0.099 -0.099 0 -0.099 0 0 -0.099 0.099 0zm-0.593 -0.296l0.099 0 0 0.098 -0.099 0 0 -0.098z"/>
   <path fill="#8F9297" d="M1.58 1.086l0 0.099 0 0.099 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098zm-0.197 -0.987l-0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0.098 0 0.099 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0.099 0 0.099 0 0.099 0 0.098 0 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 0 -0.099 0 -0.099 0 -0.098 0 -0.099 -0.098 0z"/>
  </g></g>
</svg>)SVG"},
        {"WindowsTerminal",
         R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 23 23"><path fill="#f35325" d="M1 1h10v10H1z"/><path fill="#81bc06" d="M12 1h10v10H12z"/><path fill="#05a6f0" d="M1 12h10v10H1z"/><path fill="#ffba08" d="M12 12h10v10H12z"/></svg>)SVG"},
        {"WindowsPowerShell",
         R"SVG(<svg version="1.1" id="PowerShell" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 204.691 154.521">
<g><path fill="#E0EAF5" d="M120.14.032c23.011-.008 46.023-.078 69.034.019 13.68.056 17.537 4.627 14.588 18.137-8.636 39.566-17.466 79.092-26.415 118.589-2.83 12.484-9.332 17.598-22.465 17.637-46.023.137-92.046.152-138.068-.006-15.043-.053-19-5.148-15.759-19.404C9.849 96.287 18.69 57.582 27.602 18.892 30.997 4.148 36.099.1 51.104.057 74.116-.008 97.128.04 120.14.032z"/>
<path fill="#2671BE" d="M85.365 149.813c-23.014-.008-46.029.098-69.042-.053-11.67-.076-13.792-2.83-11.165-14.244 8.906-38.71 18.099-77.355 26.807-116.109C34.3 9.013 39.337 4.419 50.473 4.522c46.024.427 92.056.137 138.083.184 11.543.011 13.481 2.48 10.89 14.187-8.413 38.007-16.879 76.003-25.494 113.965-3.224 14.207-6.938 16.918-21.885 16.951-22.234.047-44.469.012-66.702.004z"/>
<path fill="#FDFDFE" d="M104.948 73.951c-1.543-1.81-3.237-3.894-5.031-5.886-10.173-11.3-20.256-22.684-30.61-33.815-4.738-5.094-6.248-10.041-.558-15.069 5.623-4.97 11.148-4.53 16.306 1.188 14.365 15.919 28.713 31.856 43.316 47.556 5.452 5.864 4.182 9.851-1.823 14.196-23.049 16.683-45.968 33.547-68.862 50.443-5.146 3.799-10.052 4.75-14.209-.861-4.586-6.189-.343-9.871 4.414-13.335 17.013-12.392 33.993-24.83 50.9-37.366 2.355-1.746 5.736-2.764 6.157-7.051zM112.235 133.819c-6.196 0-12.401.213-18.583-.068-4.932-.223-7.9-2.979-7.838-8.174.06-4.912 2.536-8.605 7.463-8.738 13.542-.363 27.104-.285 40.651-.02 4.305.084 7.483 2.889 7.457 7.375-.031 5.146-2.739 9.133-8.25 9.465-6.944.42-13.931.104-20.9.104v.056z"/></g>
</svg>
)SVG"},
        {"PowerShell",
         R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 128 128"><linearGradient id="a" x1="96.306" x2="25.454" y1="35.144" y2="98.431" gradientTransform="matrix(1 0 0 -1 0 128)" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#a9c8ff"/><stop offset="1" stop-color="#c7e6ff"/></linearGradient><path fill="url(#a)" fill-rule="evenodd" d="M7.2 110.5c-1.7 0-3.1-.7-4.1-1.9-1-1.2-1.3-2.9-.9-4.6l18.6-80.5c.8-3.4 4-6 7.4-6h92.6c1.7 0 3.1.7 4.1 1.9 1 1.2 1.3 2.9.9 4.6l-18.6 80.5c-.8 3.4-4 6-7.4 6H7.2z" opacity=".8"/><linearGradient id="b" x1="25.336" x2="94.569" y1="98.33" y2="36.847" gradientTransform="matrix(1 0 0 -1 0 128)" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#2d4664"/><stop offset=".169" stop-color="#29405b"/><stop offset=".445" stop-color="#1e2f43"/><stop offset=".79" stop-color="#0c131b"/><stop offset="1"/></linearGradient><path fill="url(#b)" fill-rule="evenodd" d="M120.3 18.5H28.5c-2.9 0-5.7 2.3-6.4 5.2L3.7 104.3c-.7 2.9 1.1 5.2 4 5.2h91.8c2.9 0 5.7-2.3 6.4-5.2l18.4-80.5c.7-2.9-1.1-5.3-4-5.3z"/><path fill="#2C5591" d="M64.2 88.3h22.3c2.6 0 4.7 2.2 4.7 4.9s-2.1 4.9-4.7 4.9H64.2c-2.6 0-4.7-2.2-4.7-4.9s2.1-4.9 4.7-4.9zM78.7 66.5c-.4.8-1.2 1.6-2.6 2.6L34.6 98.9c-2.3 1.6-5.5 1-7.3-1.4-1.7-2.4-1.3-5.7.9-7.3l37.4-27.1v-.6l-23.5-25c-1.9-2-1.7-5.3.4-7.4 2.2-2 5.5-2 7.4 0l28.2 30c1.7 1.9 1.8 4.5.6 6.4z"/><path fill="#FFF" d="M77.6 65.5c-.4.8-1.2 1.6-2.6 2.6L33.6 97.9c-2.3 1.6-5.5 1-7.3-1.4-1.7-2.4-1.3-5.7.9-7.3l37.4-27.1v-.6l-23.5-25c-1.9-2-1.7-5.3.4-7.4 2.2-2 5.5-2 7.4 0l28.2 30c1.7 1.8 1.8 4.4.5 6.4zM63.5 87.8h22.3c2.6 0 4.7 2.1 4.7 4.6 0 2.6-2.1 4.6-4.7 4.6H63.5c-2.6 0-4.7-2.1-4.7-4.6 0-2.6 2.1-4.6 4.7-4.6z"/></svg>
)SVG"},
        {"AzureCloudShell",
         R"SVG(<svg viewBox="0 -16.33 161.67 161.67" xmlns="http://www.w3.org/2000/svg"><path d="m88.33 0-47.66 41.33-40.67 73h36.67zm6.34 9.67-20.34 57.33 39 49-75.66 13h124z" fill="#0072c6"/></svg>
)SVG"},
        {"VisualStudio",
         R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 96 95.51"><path fill="#52218a" d="M13.87 75.15a4 4 0 0 1-4.14.65l-7.27-3A4 4 0 0 1 0 69.08V26.42a4 4 0 0 1 2.46-3.67l7.27-3a4 4 0 0 1 4.14.65l1.63 1.4A2.21 2.21 0 0 0 12 23.55v48.4a2.21 2.21 0 0 0 3.5 1.8z"/><path fill="#6c33af" d="M2.46 72.75A4 4 0 0 1 0 69.08v-.33a2.31 2.31 0 0 0 4 1.55L66 1.75A6 6 0 0 1 72.82.59l19.78 9.52a6 6 0 0 1 3.4 5.41v.23a3.79 3.79 0 0 0-6.19-2.93L15.5 73.75l-1.63 1.4a4 4 0 0 1-4.14.65z"/><path fill="#854cc7" d="M2.46 22.75A4 4 0 0 0 0 26.42v.33a2.31 2.31 0 0 1 4-1.55l62 68.55a6 6 0 0 0 6.82 1.16L92.6 85.39A6 6 0 0 0 96 79.98v-.23a3.79 3.79 0 0 1-6.19 2.93L15.5 21.75l-1.63-1.4a4 4 0 0 0-4.14-.65z"/><path fill="#b179f1" d="M72.82 94.91A6 6 0 0 1 66 93.75a3.52 3.52 0 0 0 6-2.49v-87a3.52 3.52 0 0 0-6-2.51A6 6 0 0 1 72.82.59l19.78 9.51a6 6 0 0 1 3.4 5.41v64.48a6 6 0 0 1-3.4 5.41z"/></svg>
)SVG"},
        {"TabsPrevious",
         R"SVG(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN" "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
<svg version="1.1" id="Icon" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" x="0px" y="0px"
     width="16px" height="16px" viewBox="0 0 16 16" enable-background="new 0 0 16 16" xml:space="preserve">
  <g id="Icon_1_">
    <path fill="#414141" d="M2,5h4.8L8,7h6v5H2V5z"/>
    <path fill="#FFFFFF" d="M3,6h3.9L7.6,8H13v3H3V6z"/>
    <polygon fill="#2A5496" points="9.8,8.5 9.8,11.5 7.8,11.5 7.8,12.5 5,10 7.8,7.5 7.8,8.5"/>
  </g>
</svg>
)SVG"},
    };

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

    for (int i = 0; i < _countof(BuiltinToolbarSVGs); i++)
    {
        if (strcmp(BuiltinToolbarSVGs[i].Name, svgName) == 0)
            return DuplicateToolbarSVG(BuiltinToolbarSVGs[i].Svg);
    }

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


BOOL RenderSVGIconBitmap(const char* svgName, int iconSize, BOOL enabled, HBITMAP* hBitmap)
{
    if (hBitmap == NULL)
        return FALSE;
    *hBitmap = NULL;
    char* svg = LoadToolbarSVG(svgName);
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
    free(svg);
    return *hBitmap != NULL;
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
