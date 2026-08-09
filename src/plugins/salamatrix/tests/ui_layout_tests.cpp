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
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_controls.h"
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_host.h"

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

BOOL CALLBACK CountNotificationWindow(HWND window, LPARAM context)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    wchar_t className[128];
    if (processId == GetCurrentProcessId() &&
        GetClassNameW(window, className, _countof(className)) > 0 &&
        wcscmp(
            className,
            L"OpenSalamander.Salamatrix.Notification") == 0)
        ++*reinterpret_cast<int*>(context);
    return TRUE;
}

int GetNotificationWindowCount()
{
    int count = 0;
    EnumWindows(CountNotificationWindow, reinterpret_cast<LPARAM>(&count));
    return count;
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
    INITCOMMONCONTROLSEX common = {sizeof(common), ICC_WIN95_CLASSES};
    InitCommonControlsEx(&common);
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

    HWND parent = CreateWindowExW(
        0, L"STATIC", L"SDK controls test", WS_OVERLAPPED,
        0, 0, 480, 320, NULL, NULL, GetModuleHandleW(NULL), NULL);
    Check(parent != NULL, "hidden SDK control host window is created");
    if (parent != NULL)
    {
        Check(
            Salamatrix::UI::ShowNativeNotification(
                parent, "Shutdown test", "Tracked notification", 600000) != FALSE,
            "native notification window is created");
        Check(GetNotificationWindowCount() == 1,
              "native notification window is tracked before shutdown");
        Salamatrix::UI::CloseAllNativeDialogs();
        Check(GetNotificationWindowCount() == 0,
              "native notification window is destroyed before provider unload");

        CreateWindowExW(0, L"STATIC", L"Original", WS_CHILD | WS_VISIBLE,
                        4, 4, 180, 20, parent, reinterpret_cast<HMENU>(1001), NULL, NULL);
        CGUIStaticTextAbstract* text =
            Salamatrix::UI::AttachNativeStaticText(parent, 1001, STF_BOLD | STF_END_ELLIPSIS);
        Check(text != NULL, "shared static-text implementation attaches");
        Check(text != NULL && text->SetText("Shared SDK"), "shared static-text value changes");
        Check(text != NULL && std::strcmp(text->GetText(), "Shared SDK") == 0,
              "shared static-text value is retained");

        CreateWindowExW(0, L"STATIC", L"Link", WS_CHILD | WS_VISIBLE,
                        4, 28, 180, 20, parent, reinterpret_cast<HMENU>(1002), NULL, NULL);
        CGUIHyperLinkAbstract* link =
            Salamatrix::UI::AttachNativeHyperLink(parent, 1002, STF_UNDERLINE);
        Check(link != NULL && link->SetActionShowHint("Hint"),
              "shared hyperlink implementation attaches");

        CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                        4, 52, 180, 18, parent, reinterpret_cast<HMENU>(1003), NULL, NULL);
        CGUIProgressBarAbstract* progress =
            Salamatrix::UI::AttachNativeProgressBar(parent, 1003);
        Check(progress != NULL, "shared progress implementation attaches");
        if (progress != NULL) progress->SetProgress(500, "Half");

        CreateWindowExW(0, L"BUTTON", L"Color", WS_CHILD | WS_VISIBLE,
                        4, 76, 120, 24, parent, reinterpret_cast<HMENU>(1004), NULL, NULL);
        CGUIColorArrowButtonAbstract* color =
            Salamatrix::UI::AttachNativeColorArrowButton(parent, 1004, TRUE);
        Check(color != NULL, "shared color-arrow implementation attaches");
        if (color != NULL)
        {
            color->SetColor(RGB(1, 2, 3), RGB(4, 5, 6));
            Check(color->GetTextColor() == RGB(1, 2, 3),
                  "shared color-arrow text color is retained");
            Check(color->GetBkgndColor() == RGB(4, 5, 6),
                  "shared color-arrow background is retained");
        }

        CreateWindowExW(0, L"BUTTON", L"More", WS_CHILD | WS_VISIBLE,
                        4, 104, 120, 24, parent, reinterpret_cast<HMENU>(1005), NULL, NULL);
        CGUIButtonAbstract* button =
            Salamatrix::UI::AttachNativeButton(parent, 1005, BTF_DROPDOWN);
        Check(button != NULL && button->SetToolTipText("Menu"),
              "shared text-arrow implementation attaches");

        CreateWindowExW(0, L"BUTTON", L"", WS_CHILD | WS_VISIBLE,
                        130, 104, 24, 24, parent, reinterpret_cast<HMENU>(1006), NULL, NULL);
        Check(Salamatrix::UI::ChangeNativeArrowButton(parent, 1006),
              "shared arrow-button implementation attaches");

        CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                        4, 132, 220, 20, parent, reinterpret_cast<HMENU>(1007), NULL, NULL);
        HWND align = CreateWindowExW(0, WC_LISTVIEWW, L"", WS_CHILD | WS_VISIBLE,
                                     4, 154, 220, 100, parent,
                                     reinterpret_cast<HMENU>(1008), NULL, NULL);
        CGUIToolbarHeaderAbstract* toolbar =
            Salamatrix::UI::AttachNativeToolbarHeader(
                parent, 1007, align, TLBHDRMASK_NEW | TLBHDRMASK_DELETE);
        Check(toolbar != NULL, "shared toolbar-header implementation attaches");
        DestroyWindow(parent);
    }

    if (Failures != 0)
    {
        std::fprintf(stderr, "%d UI layout test(s) failed.\n", Failures);
        return 1;
    }
    std::printf("All Salamatrix UI SDK tests passed.\n");
    return 0;
}
