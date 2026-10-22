// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#include "../tooltipcursor.h"
#include <cstdio>
#include <vector>

namespace
{
int Failures = 0;
void Check(bool condition, const char* description)
{
    if (!condition)
    {
        ++Failures;
        std::printf("FAIL: %s\n", description);
    }
}

HCURSOR MakeCursor(int width, int height, int hotspot, bool transparent)
{
    std::vector<BYTE> andBits(static_cast<size_t>(width / 8) * height, transparent ? 0xff : 0);
    std::vector<BYTE> xorBits(andBits.size(), 0);
    return CreateCursor(NULL, 0, hotspot, width, height, andBits.data(), xorBits.data());
}
}

int main()
{
    // The legacy helper returned zero here, placing the tooltip over the exact
    // hover point and making popup mouse-leave immediately hide it.
    Check(GetToolTipCursorHeight(NULL) > 0, "missing cursor cannot cover the hover point");
    Check(GetToolTipCursorHeight((HCURSOR)(ULONG_PTR)1) > 0, "invalid cursor has a safe positive fallback");
    HCURSOR transparent = MakeCursor(32, 32, 0, true);
    Check(transparent != NULL, "transparent cursor fixture created");
    if (transparent != NULL)
    {
        Check(GetToolTipCursorHeight(transparent) == 32,
              "fully transparent mask cannot produce the old zero offset");
        DestroyCursor(transparent);
    }
    HCURSOR large = MakeCursor(64, 96, 70, false);
    Check(large != NULL, "oversized cursor fixture created");
    if (large != NULL)
    {
        Check(GetToolTipCursorHeight(large) == 26,
              "96-pixel cursor and 70-pixel hotspot use the same unscaled coordinates");
        DestroyCursor(large);
    }
    HCURSOR bottom = MakeCursor(32, 32, 31, false);
    Check(bottom != NULL, "bottom-hotspot cursor fixture created");
    if (bottom != NULL)
    {
        Check(GetToolTipCursorHeight(bottom) == 1, "bottommost hotspot still leaves one pixel clearance");
        DestroyCursor(bottom);
    }
    HCURSOR arrow = LoadCursor(NULL, IDC_ARROW);
    int first = GetToolTipCursorHeight(arrow);
    Check(first > 0, "standard arrow has positive offset");
    bool stable = true;
    DWORD resources = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
    for (int i = 0; i < 1000; ++i)
        stable = stable && GetToolTipCursorHeight(arrow) == first;
    Check(stable, "one thousand arrow measurements are deterministic");
    Check(GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS) == resources,
          "repeated measurement releases every DC and bitmap");
    std::printf("Tooltip cursor tests: %s (%d failures), arrow offset %d pixels\n",
                Failures == 0 ? "PASS" : "FAIL", Failures, first);
    return Failures == 0 ? 0 : 1;
}
