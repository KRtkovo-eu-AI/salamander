// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#define SALAMANDER_COLOR_DISABLE_OVERRIDES
#include "precomp.h"
#include "color.h"
#undef SALAMANDER_COLOR_DISABLE_OVERRIDES

// Konverze barevnych prostoru RGB<->HSL
// HSL prostor viz http://en.wikipedia.org/wiki/HSL_color_space
// Rutiny viz "How To Converting Colors Between RGB and HLS (HBS)"
//            http://support.microsoft.com/kb/q29240/

// A point of reference for the algorithms is Foley and Van Dam,
// "Fundamentals of Interactive Computer Graphics," Pages 618-19.
// Their algorithm is in floating point. CHART implements a less
// general (hardwired ranges) integral algorithm.

#define HLSMAX 240 // H,L, and S vary over 0-HLSMAX
#define RGBMAX 255 // R,G, and B vary over 0-RGBMAX \
                   // HLSMAX BEST IF DIVISIBLE BY 6 \
                   // RGBMAX, HLSMAX must each fit in a byte.

// There are potential round-off errors throughout this sample.
// ((0.5 + x)/y) without floating point is phrased ((x + (y/2))/y),
// yielding a very small round-off error. This makes many of the
// following divisions look strange.

// Hue is undefined if Saturation is 0 (grey-scale)
// This value determines where the Hue scrollbar is
// initially set for achromatic colors
#define UNDEFINED (HLSMAX * 2 / 3)

void ColorRGBToHLS(COLORREF clrRGB, WORD* pwHue, WORD* pwLuminance, WORD* pwSaturation)
{
    int r, g, b;     // input RGB values
    int h, l, s;     // output HLS values
    WORD cMax, cMin; // max and min RGB values
    WORD cSum, cDif;
    int rDelta, gDelta, bDelta; // intermediate value: % of spread from max

    // get R, G, and B out of DWORD
    r = GetRValue(clrRGB);
    g = GetGValue(clrRGB);
    b = GetBValue(clrRGB);

    // calculate lightness
    cMax = max(max(r, g), b);
    cMin = min(min(r, g), b);
    cSum = cMax + cMin;
    l = (WORD)(((cSum * (DWORD)HLSMAX) + RGBMAX) / (2 * RGBMAX));

    cDif = cMax - cMin;
    if (!cDif)
    {                  // r=g=b --> achromatic case
        s = 0;         // saturation
        h = UNDEFINED; // hue
    }
    else
    { // chromatic case
        // saturation
        if (l <= (HLSMAX / 2))
            s = (WORD)(((cDif * (DWORD)HLSMAX) + (cSum / 2)) / cSum);
        else
            s = (WORD)((DWORD)((cDif * (DWORD)HLSMAX) + (DWORD)((2 * RGBMAX - cSum) / 2)) / (2 * RGBMAX - cSum));

        // hue
        rDelta = (int)((((cMax - r) * (DWORD)(HLSMAX / 6)) + (cDif / 2)) / cDif);
        gDelta = (int)((((cMax - g) * (DWORD)(HLSMAX / 6)) + (cDif / 2)) / cDif);
        bDelta = (int)((((cMax - b) * (DWORD)(HLSMAX / 6)) + (cDif / 2)) / cDif);

        if ((WORD)r == cMax)
            h = bDelta - gDelta;
        else if ((WORD)g == cMax)
            h = (HLSMAX / 3) + rDelta - bDelta;
        else // B == cMax
            h = ((2 * HLSMAX) / 3) + gDelta - rDelta;

        if (h < 0)
            h += HLSMAX;

        if (h > HLSMAX)
            h -= HLSMAX;
    }

    *pwHue = (WORD)h;
    *pwLuminance = (WORD)l;
    *pwSaturation = (WORD)s;
}

// utility routine for HLStoRGB
WORD HueToRGB(WORD n1, WORD n2, WORD hue)
{
    // range check: note values passed add/subtract thirds of range

    if (hue > HLSMAX)
        hue -= HLSMAX;

    // return r,g, or b value from this tridrant
    if (hue < (HLSMAX / 6))
        return (n1 + (((n2 - n1) * hue + (HLSMAX / 12)) / (HLSMAX / 6)));
    if (hue < (HLSMAX / 2))
        return (n2);
    if (hue < ((HLSMAX * 2) / 3))
        return (n1 + (((n2 - n1) * (((HLSMAX * 2) / 3) - hue) + (HLSMAX / 12)) / (HLSMAX / 6)));
    else
        return (n1);
}

COLORREF ColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation)
{
    WORD r, g, b;        // RGB component values
    WORD magic1, magic2; // calculated magic numbers (really!)

    if (wSaturation == 0)
    { // achromatic case
        r = g = b = (wLuminance * RGBMAX) / HLSMAX;
        if (wHue != UNDEFINED)
        {
            r = g = b = 0;
        }
    }
    else
    { // chromatic case
        // set up magic numbers
        if (wLuminance <= (HLSMAX / 2))
            magic2 = (WORD)((wLuminance * (HLSMAX + wSaturation) + (HLSMAX / 2)) / HLSMAX);
        else
            magic2 = wLuminance + wSaturation - ((wLuminance * wSaturation) + (HLSMAX / 2)) / HLSMAX;
        magic1 = 2 * wLuminance - magic2;

        // get RGB, change units from HLSMAX to RGBMAX */
        r = (WORD)((HueToRGB(magic1, magic2, (WORD)(wHue + (WORD)(HLSMAX / 3))) * (DWORD)RGBMAX + (HLSMAX / 2))) / (WORD)HLSMAX;
        g = (WORD)((HueToRGB(magic1, magic2, wHue) * (DWORD)RGBMAX + (HLSMAX / 2))) / HLSMAX;
        b = (WORD)((HueToRGB(magic1, magic2, (WORD)(wHue - (WORD)(HLSMAX / 3))) * (DWORD)RGBMAX + (HLSMAX / 2))) / (WORD)HLSMAX;
    }
    return RGB(min(r, RGBMAX), min(g, RGBMAX), min(b, RGBMAX));
}

struct SysColorEntry
{
    int Index;
    COLORREF DarkColor;
    HBRUSH DarkBrush;
};

static SysColorEntry DarkModeColors[] = {
    {COLOR_SCROLLBAR, RGB(73, 73, 73), NULL},
    {COLOR_BACKGROUND, RGB(0, 0, 0), NULL},
    {COLOR_ACTIVECAPTION, RGB(153, 180, 209), NULL},
    {COLOR_INACTIVECAPTION, RGB(191, 205, 219), NULL},
    {COLOR_MENU, RGB(73, 73, 73), NULL},
    {COLOR_WINDOW, RGB(255, 255, 255), NULL},
    {COLOR_WINDOWFRAME, RGB(100, 100, 100), NULL},
    {COLOR_MENUTEXT, RGB(255, 255, 255), NULL},
    {COLOR_WINDOWTEXT, RGB(0, 0, 0), NULL},
    {COLOR_CAPTIONTEXT, RGB(0, 0, 0), NULL},
    {COLOR_ACTIVEBORDER, RGB(73, 73, 73), NULL},
    {COLOR_INACTIVEBORDER, RGB(153, 153, 153), NULL},
    {COLOR_APPWORKSPACE, RGB(171, 171, 171), NULL},
    {COLOR_HIGHLIGHT, RGB(0, 120, 215), NULL},
    {COLOR_HIGHLIGHTTEXT, RGB(255, 255, 255), NULL},
    {COLOR_BTNFACE, RGB(73, 73, 73), NULL},
    {COLOR_BTNSHADOW, RGB(127, 127, 127), NULL},
    {COLOR_GRAYTEXT, RGB(142, 142, 142), NULL},
    {COLOR_BTNTEXT, RGB(204, 204, 204), NULL},
    {COLOR_INACTIVECAPTIONTEXT, RGB(0, 0, 0), NULL},
    {COLOR_BTNHIGHLIGHT, RGB(73, 73, 73), NULL},
    {COLOR_3DDKSHADOW, RGB(100, 100, 100), NULL},
    {COLOR_3DLIGHT, RGB(127, 127, 127), NULL},
    {COLOR_INFOTEXT, RGB(0, 0, 0), NULL},
    {COLOR_INFOBK, RGB(255, 255, 225), NULL},
    {COLOR_GRADIENTACTIVECAPTION, RGB(185, 209, 234), NULL},
    {COLOR_GRADIENTINACTIVECAPTION, RGB(215, 228, 242), NULL},
#ifdef COLOR_BTNALTERNATE
    {COLOR_BTNALTERNATE, RGB(0, 0, 0), NULL},
#endif
    {COLOR_HOTLIGHT, RGB(0, 102, 204), NULL},
    {COLOR_MENUHILIGHT, RGB(0, 120, 215), NULL},
    {COLOR_MENUBAR, RGB(73, 73, 73), NULL},
    {COLOR_DESKTOP, RGB(0, 0, 0), NULL},
};

static bool DarkModeActive = false;
static SysColorEntry* FindDarkModeEntry(int index)
{
    size_t count = sizeof(DarkModeColors) / sizeof(DarkModeColors[0]);
    for (size_t i = 0; i < count; i++)
    {
        if (DarkModeColors[i].Index == index)
            return &DarkModeColors[i];
    }
    return NULL;
}

static HBRUSH EnsureDarkBrush(SysColorEntry& entry)
{
    if (entry.DarkBrush == NULL)
        entry.DarkBrush = ::CreateSolidBrush(entry.DarkColor);
    return entry.DarkBrush;
}

static void DeleteDarkBrushes()
{
    size_t count = sizeof(DarkModeColors) / sizeof(DarkModeColors[0]);
    for (size_t i = 0; i < count; i++)
    {
        if (DarkModeColors[i].DarkBrush != NULL)
        {
            ::DeleteObject(DarkModeColors[i].DarkBrush);
            DarkModeColors[i].DarkBrush = NULL;
        }
    }
}

static BOOL CALLBACK SendSysColorChangeToProcessWindowsProc(HWND hwnd, LPARAM lParam)
{
    DWORD windowProcessId = 0;
    ::GetWindowThreadProcessId(hwnd, &windowProcessId);
    if (windowProcessId == static_cast<DWORD>(lParam))
        ::SendMessage(hwnd, WM_SYSCOLORCHANGE, 0, 0);
    return TRUE;
}

static void NotifyProcessWindowsOfColorChange()
{
    DWORD processId = ::GetCurrentProcessId();
    ::EnumWindows(SendSysColorChangeToProcessWindowsProc, static_cast<LPARAM>(processId));
}

void ApplyDarkModeTheme(BOOL enable)
{
    BOOL requestedState = enable ? TRUE : FALSE;
    if (DarkModeActive == (requestedState != FALSE))
        return;

    DarkModeActive = (requestedState != FALSE);

    if (!DarkModeActive)
        DeleteDarkBrushes();

    NotifyProcessWindowsOfColorChange();
}

BOOL IsDarkModeThemeActive()
{
    return DarkModeActive ? TRUE : FALSE;
}

COLORREF GetSalamanderSysColor(int index)
{
    if (DarkModeActive)
    {
        if (SysColorEntry* entry = FindDarkModeEntry(index))
            return entry->DarkColor;
    }
    return ::GetSysColor(index);
}

HBRUSH GetSalamanderSysColorBrush(int index)
{
    if (DarkModeActive)
    {
        if (SysColorEntry* entry = FindDarkModeEntry(index))
        {
            HBRUSH brush = EnsureDarkBrush(*entry);
            if (brush != NULL)
                return brush;
        }
    }
    return ::GetSysColorBrush(index);
}
