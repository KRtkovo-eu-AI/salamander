// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>

#include "../salamatrix_ui_layout.h"

namespace
{
int Failures = 0;

void Check(bool condition, const char* message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAILED: %s\n", message);
        ++Failures;
    }
}
}

int main()
{
    Salamatrix::UI::FilePickerLayoutMetrics metrics =
        Salamatrix::UI::ComputeFilePickerLayout(8, 120);
    Check(metrics.EditWidth == 92, "file picker edit width");
    Check(metrics.BrowseX == 104, "file picker browse position");
    Check(metrics.BrowseWidth == 24, "file picker browse width");

    metrics = Salamatrix::UI::ComputeFilePickerLayout(3, 1);
    Check(metrics.EditWidth == 1, "narrow picker keeps editable field");
    Check(metrics.BrowseWidth == 1, "narrow picker keeps browse button");

    Check(Salamatrix::UI::ScaleDialogMetric(96, 96, 96) == 96,
          "96 DPI metric remains unchanged");
    Check(Salamatrix::UI::ScaleDialogMetric(100, 96, 144) == 150,
          "DPI metric scales to 150 percent");
    Check(Salamatrix::UI::ScaleDialogMetric(150, 144, 96) == 100,
          "DPI metric scales back to 100 percent");
    Check(Salamatrix::UI::ScaleDialogMetric(10, 0, 144) == 15,
          "zero source DPI uses the Windows baseline");

    if (Failures != 0)
    {
        std::fprintf(stderr, "%d UI layout test(s) failed.\n", Failures);
        return 1;
    }
    std::printf("All Salamatrix UI layout tests passed.\n");
    return 0;
}
