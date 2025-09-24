// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Converts colors from RGB to hue-luminance-saturation (HLS) format.
void SalamanderColorRGBToHLS(COLORREF clrRGB, WORD* pwHue, WORD* pwLuminance, WORD* pwSaturation);

// Converts colors from hue-luminance-saturation (HLS) to RGB format.
COLORREF SalamanderColorHLSToRGB(WORD wHue, WORD wLuminance, WORD wSaturation);

// Applies or restores the application-wide system color overrides used for the
// legacy WinAPI dark mode simulation.
void ApplyDarkModeTheme(BOOL enable);

// Returns TRUE when the dark mode overrides are active.
BOOL IsDarkModeThemeActive();

// Retrieves the Salamander-specific replacements for system colors and
// brushes used when the legacy dark mode simulation is active.
COLORREF GetSalamanderSysColor(int index);
HBRUSH GetSalamanderSysColorBrush(int index);

#ifndef SALAMANDER_COLOR_DISABLE_OVERRIDES
#ifdef GetSysColor
#undef GetSysColor
#endif
#define GetSysColor(index) GetSalamanderSysColor(index)

#ifdef GetSysColorBrush
#undef GetSysColorBrush
#endif
#define GetSysColorBrush(index) GetSalamanderSysColorBrush(index)
#endif
