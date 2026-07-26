// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdio>
#include <cstring>
#include <windows.h>
#include <commctrl.h>

#pragma warning(push)
#pragma warning(disable:4201 4121 4245)
#include "../salamatrix_ui.h"
#pragma warning(pop)
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

class AccessibilityTestControl : public Salamatrix::UI::IControl
{
public:
    virtual Salamatrix::UI::ControlKind WINAPI GetKind() const
    {
        return Salamatrix::UI::ControlKindLabel;
    }

    virtual const char* WINAPI GetId() const
    {
        return "test";
    }

    virtual BOOL WINAPI GetText(char* buffer, DWORD capacity) const
    {
        if (buffer != NULL && capacity != 0)
            buffer[0] = '\0';
        return FALSE;
    }

    virtual BOOL WINAPI SetText(const char* value)
    {
        (void)value;
        return FALSE;
    }

    virtual BOOL WINAPI GetChecked() const
    {
        return FALSE;
    }

    virtual BOOL WINAPI SetChecked(BOOL checked)
    {
        (void)checked;
        return FALSE;
    }

    virtual int WINAPI GetDialogResult() const
    {
        return 0;
    }
};
}

int main()
{
    Salamatrix::UI::ControlOptions defaults;
    Check(defaults.AccessibleName == NULL,
          "default accessible name is null");
    Check(defaults.AccessibleDescription == NULL,
          "default accessible description is null");

    Salamatrix::UI::ControlOptions explicitValues;
    explicitValues.AccessibleName = "N\xc3\xa1" "zev ovl\xc3\xa1" "dac\xc3\xADho prvku";
    explicitValues.AccessibleDescription = "Popis \xc5\xbe\xc3\xa1" "dosti";
    Check(std::strcmp(explicitValues.AccessibleName,
                      "N\xc3\xa1" "zev ovl\xc3\xa1" "dac\xc3\xADho prvku") == 0,
          "explicit UTF-8 accessible name is retained");
    Check(std::strcmp(explicitValues.AccessibleDescription,
                      "Popis \xc5\xbe\xc3\xa1" "dosti") == 0,
          "explicit UTF-8 accessible description is retained");

    AccessibilityTestControl control;
    Check(std::strcmp(control.GetAccessibleName(), "") == 0,
          "default accessible name accessor is empty");
    Check(std::strcmp(control.GetAccessibleDescription(), "") == 0,
          "default accessible description accessor is empty");

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
