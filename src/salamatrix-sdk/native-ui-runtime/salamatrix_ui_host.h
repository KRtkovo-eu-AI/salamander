// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <windows.h>
#include <commctrl.h>
#include "../../plugins/shared/spl_com.h"
#include "../../plugins/shared/spl_gui.h"

namespace Salamatrix
{
namespace UI
{

// Narrow host boundary used by the shared NativeDialog implementation.
// Salamatrix.SPL and Salamatrix Studio provide different environment adapters,
// but both execute the same dialog and control code.
class INativeDialogHost
{
public:
    virtual UINT GetWindowDpi(HWND window) = 0;
    virtual void PrepareTheme() = 0;
    virtual BOOL HandleThemeChange(UINT message, LPARAM data) = 0;
    virtual BOOL IsDarkMode() const = 0;
    virtual void ApplyTheme(HWND window) = 0;
    virtual void SetDarkScrollbars(HWND window, BOOL enabled) = 0;
    virtual int ShowUtf8MessageBox(
        HWND parent, const char* message, const char* title, UINT flags) = 0;

    virtual CGUIStaticTextAbstract* AttachStaticText(
        HWND parent, int controlId, DWORD flags) = 0;
    virtual CGUIHyperLinkAbstract* AttachHyperLink(
        HWND parent, int controlId, DWORD flags) = 0;
    virtual CGUIProgressBarAbstract* AttachProgressBar(
        HWND parent, int controlId) = 0;
    virtual BOOL ChangeToArrowButton(HWND parent, int controlId) = 0;
    virtual CGUIButtonAbstract* AttachButton(
        HWND parent, int controlId, DWORD flags) = 0;
    virtual CGUIColorArrowButtonAbstract* AttachColorArrowButton(
        HWND parent, int controlId, BOOL showArrow) = 0;
    virtual CGUIToolbarHeaderAbstract* AttachToolbarHeader(
        HWND parent, int controlId, HWND alignWindow, DWORD buttonMask) = 0;

protected:
    virtual ~INativeDialogHost() {}
};

void WINAPI SetNativeDialogHost(INativeDialogHost* host);
INativeDialogHost* WINAPI GetNativeDialogHost();

} // namespace UI
} // namespace Salamatrix
