// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_ui.h"
#include "../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_host.h"
#include "../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_controls.h"
#include "../../third_party/darkmodelib/src/DmlibDpi.h"

namespace
{
class SalamanderNativeDialogHost : public Salamatrix::UI::INativeDialogHost
{
public:
    virtual UINT GetWindowDpi(HWND window)
    {
        return dmlib_dpi::GetDpiForWindow(window);
    }

    virtual void PrepareTheme()
    {
        Salamatrix::Runtime::ApplyHostDarkModePolicy(SalamanderGeneral, NULL);
    }

    virtual BOOL HandleThemeChange(UINT message, LPARAM data)
    {
        return DarkModeHandleSettingChange(message, data);
    }

    virtual BOOL IsDarkMode() const
    {
        return DarkModeShouldUseDarkColors() ? TRUE : FALSE;
    }

    virtual void ApplyTheme(HWND window)
    {
        if (window == NULL)
            return;
        if (DarkModeShouldUseDarkColors())
            DarkModeFixScrollbars();
        DarkModeApplyTree(window);
        DarkModeRefreshTitleBar(window);
        DarkModeApplyStaticTextColors(window, NULL);
    }

    virtual void SetDarkScrollbars(HWND window, BOOL enabled)
    {
        if (enabled)
            DarkModeAllowDarkScrollbars(window);
        else
            DarkModeDisallowDarkScrollbars(window);
    }

    virtual int ShowUtf8MessageBox(
        HWND parent, const char* message, const char* title, UINT flags)
    {
        return Salamatrix::Runtime::ShowUtf8MessageBox(
            parent, SalamanderGeneral, message, title, flags);
    }

    virtual CGUIStaticTextAbstract* AttachStaticText(
        HWND parent, int controlId, DWORD flags)
    {
        return Salamatrix::UI::AttachNativeStaticText(parent, controlId, flags);
    }

    virtual CGUIHyperLinkAbstract* AttachHyperLink(
        HWND parent, int controlId, DWORD flags)
    {
        return Salamatrix::UI::AttachNativeHyperLink(parent, controlId, flags);
    }

    virtual CGUIProgressBarAbstract* AttachProgressBar(
        HWND parent, int controlId)
    {
        return Salamatrix::UI::AttachNativeProgressBar(parent, controlId);
    }

    virtual BOOL ChangeToArrowButton(HWND parent, int controlId)
    {
        return Salamatrix::UI::ChangeNativeArrowButton(parent, controlId);
    }

    virtual CGUIButtonAbstract* AttachButton(
        HWND parent, int controlId, DWORD flags)
    {
        return Salamatrix::UI::AttachNativeButton(parent, controlId, flags);
    }

    virtual CGUIColorArrowButtonAbstract* AttachColorArrowButton(
        HWND parent, int controlId, BOOL showArrow)
    {
        return Salamatrix::UI::AttachNativeColorArrowButton(
            parent, controlId, showArrow);
    }

    virtual CGUIToolbarHeaderAbstract* AttachToolbarHeader(
        HWND parent, int controlId, HWND alignWindow, DWORD buttonMask)
    {
        return Salamatrix::UI::AttachNativeToolbarHeader(
            parent, controlId, alignWindow, buttonMask);
    }
};

SalamanderNativeDialogHost NativeDialogHost;

struct NativeDialogHostRegistration
{
    NativeDialogHostRegistration()
    {
        Salamatrix::UI::SetNativeDialogHost(&NativeDialogHost);
    }

    ~NativeDialogHostRegistration()
    {
        if (Salamatrix::UI::GetNativeDialogHost() == &NativeDialogHost)
            Salamatrix::UI::SetNativeDialogHost(NULL);
    }
} NativeDialogHostRegistrationInstance;
} // namespace
