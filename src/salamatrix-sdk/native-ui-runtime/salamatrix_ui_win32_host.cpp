// SPDX-License-Identifier: GPL-2.0-or-later

#include "salamatrix_ui_win32_host.h"
#include "salamatrix_ui_controls.h"
#include "../../third_party/darkmodelib/include/Darkmodelib.h"
#include <string>

namespace Salamatrix { namespace UI { namespace {
static BOOL CALLBACK ConfigureChildFont(HWND window, LPARAM data)
{
    HFONT font = reinterpret_cast<HFONT>(data);
    if (window != NULL && font != NULL)
    {
        SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        wchar_t className[32];
        if (GetClassNameW(window, className, ARRAYSIZE(className)) != 0 && wcscmp(className, WC_BUTTON) == 0)
            SetPropW(window, L"Darkmodelib.Button.UseConfiguredFont", reinterpret_cast<HANDLE>(1));
    }
    return TRUE;
}

static std::wstring Wide(const char* text)
{
    if (text == NULL) return std::wstring();
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    std::wstring result(length > 0 ? static_cast<size_t>(length) : 0, L'\0');
    if (length > 0) MultiByteToWideChar(CP_UTF8, 0, text, -1, &result[0], length);
    return result;
}

class Win32NativeDialogHost : public INativeDialogHost
{
public:
    Win32NativeDialogHost() : DarkMode(FALSE), ThemePrepared(FALSE) {}

    void SetDarkMode(BOOL enabled)
    {
        DarkMode = enabled;
        PrepareTheme();
    }

    virtual UINT GetWindowDpi(HWND window)
    {
        HMODULE user = GetModuleHandleW(L"user32.dll");
        typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
        GetDpiForWindowProc getDpi = user != NULL ? reinterpret_cast<GetDpiForWindowProc>(GetProcAddress(user, "GetDpiForWindow")) : NULL;
        return getDpi != NULL ? getDpi(window) : 96;
    }
    virtual void PrepareTheme()
    {
        if (!ThemePrepared)
        {
            dmlib::initDarkMode();
            ThemePrepared = TRUE;
        }
        dmlib::setDarkModeConfigEx(static_cast<UINT>(DarkMode ? dmlib::DarkModeType::dark : dmlib::DarkModeType::light));
        dmlib::setDefaultColors(true);
    }
    virtual BOOL HandleThemeChange(UINT, LPARAM lParam)
    {
        if (lParam != 0) dmlib::handleSettingChange(lParam);
        PrepareTheme();
        return TRUE;
    }
    virtual BOOL IsDarkMode() const { return DarkMode; }
    virtual void ApplyTheme(HWND window)
    {
        if (window == NULL) return;
        PrepareTheme();
        if (DarkMode)
        {
            dmlib::setDarkWndNotifySafe(window);
            dmlib::setChildCtrlsSubclassAndTheme(window);
        }
        else
            dmlib::setChildCtrlsTheme(window);
        HFONT dialogFont = reinterpret_cast<HFONT>(SendMessageW(window, WM_GETFONT, 0, 0));
        if (dialogFont != NULL)
            EnumChildWindows(window, ConfigureChildFont, reinterpret_cast<LPARAM>(dialogFont));
        dmlib::setDarkTitleBar(window);
        RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_FRAME);
    }
    virtual void SetDarkScrollbars(HWND window, BOOL enabled)
    {
        if (enabled && DarkMode) dmlib::enableDarkScrollBarForWindowAndChildren(window);
        else dmlib::disableDarkScrollBarForWindowAndChildren(window);
    }
    virtual int ShowUtf8MessageBox(HWND parent, const char* message, const char* title, UINT flags) { return MessageBoxW(parent, Wide(message).c_str(), Wide(title).c_str(), flags); }
    virtual CGUIStaticTextAbstract* AttachStaticText(HWND parent, int id, DWORD flags) { return AttachNativeStaticText(parent, id, flags); }
    virtual CGUIHyperLinkAbstract* AttachHyperLink(HWND parent, int id, DWORD flags) { return AttachNativeHyperLink(parent, id, flags); }
    virtual CGUIProgressBarAbstract* AttachProgressBar(HWND parent, int id) { return AttachNativeProgressBar(parent, id); }
    virtual BOOL ChangeToArrowButton(HWND parent, int id) { return ChangeNativeArrowButton(parent, id); }
    virtual CGUIButtonAbstract* AttachButton(HWND parent, int id, DWORD flags) { return AttachNativeButton(parent, id, flags); }
    virtual CGUIColorArrowButtonAbstract* AttachColorArrowButton(HWND parent, int id, BOOL arrow) { return AttachNativeColorArrowButton(parent, id, arrow); }
    virtual CGUIToolbarHeaderAbstract* AttachToolbarHeader(HWND parent, int id, HWND align, DWORD mask) { return AttachNativeToolbarHeader(parent, id, align, mask); }

private:
    BOOL DarkMode;
    BOOL ThemePrepared;
};

Win32NativeDialogHost Host;
struct Registration { Registration() { SetNativeDialogHost(&Host); } } RegistrationInstance;
} // namespace

INativeDialogHost* WINAPI GetWin32NativeDialogHost() { return &Host; }
void WINAPI SetWin32NativeDialogDarkMode(BOOL enabled) { Host.SetDarkMode(enabled); }
} }
