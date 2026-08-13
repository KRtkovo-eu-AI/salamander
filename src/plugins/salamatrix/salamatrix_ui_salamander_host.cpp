// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_ui.h"
#include "../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_host.h"
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
        return SalamanderGUI != NULL
                   ? SalamanderGUI->AttachStaticText(parent, controlId, flags)
                   : NULL;
    }

    virtual CGUIHyperLinkAbstract* AttachHyperLink(
        HWND parent, int controlId, DWORD flags)
    {
        return SalamanderGUI != NULL
                   ? SalamanderGUI->AttachHyperLink(parent, controlId, flags)
                   : NULL;
    }

    virtual CGUIProgressBarAbstract* AttachProgressBar(
        HWND parent, int controlId)
    {
        return SalamanderGUI != NULL
                   ? SalamanderGUI->AttachProgressBar(parent, controlId)
                   : NULL;
    }

    virtual BOOL ChangeToArrowButton(HWND parent, int controlId)
    {
        return SalamanderGUI != NULL
                   ? SalamanderGUI->ChangeToArrowButton(parent, controlId)
                   : FALSE;
    }

    virtual CGUIButtonAbstract* AttachButton(
        HWND parent, int controlId, DWORD flags)
    {
        return SalamanderGUI != NULL
                   ? SalamanderGUI->AttachButton(parent, controlId, flags)
                   : NULL;
    }

    virtual CGUIColorArrowButtonAbstract* AttachColorArrowButton(
        HWND parent, int controlId, BOOL showArrow)
    {
        return SalamanderGUI != NULL
                   ? SalamanderGUI->AttachColorArrowButton(
                         parent, controlId, showArrow)
                   : NULL;
    }

    virtual CGUIToolbarHeaderAbstract* AttachToolbarHeader(
        HWND parent, int controlId, HWND alignWindow, DWORD buttonMask)
    {
        return SalamanderGUI != NULL
                   ? SalamanderGUI->AttachToolbarHeader(
                         parent, controlId, alignWindow, buttonMask)
                   : NULL;
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
