// SPDX-License-Identifier: GPL-2.0-or-later

#include "salamatrix_ui_win32_host.h"
#include "salamatrix_ui_controls.h"
#include <string>

namespace Salamatrix { namespace UI { namespace {
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
    virtual UINT GetWindowDpi(HWND window)
    {
        HMODULE user = GetModuleHandleW(L"user32.dll");
        typedef UINT(WINAPI* GetDpiForWindowProc)(HWND);
        GetDpiForWindowProc getDpi = user != NULL ? reinterpret_cast<GetDpiForWindowProc>(GetProcAddress(user, "GetDpiForWindow")) : NULL;
        return getDpi != NULL ? getDpi(window) : 96;
    }
    virtual void PrepareTheme() {}
    virtual BOOL HandleThemeChange(UINT, LPARAM) { return TRUE; }
    virtual BOOL IsDarkMode() const { return FALSE; }
    virtual void ApplyTheme(HWND window) { RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN); }
    virtual void SetDarkScrollbars(HWND, BOOL) {}
    virtual int ShowUtf8MessageBox(HWND parent, const char* message, const char* title, UINT flags) { return MessageBoxW(parent, Wide(message).c_str(), Wide(title).c_str(), flags); }
    virtual CGUIStaticTextAbstract* AttachStaticText(HWND parent, int id, DWORD flags) { return AttachNativeStaticText(parent, id, flags); }
    virtual CGUIHyperLinkAbstract* AttachHyperLink(HWND parent, int id, DWORD flags) { return AttachNativeHyperLink(parent, id, flags); }
    virtual CGUIProgressBarAbstract* AttachProgressBar(HWND parent, int id) { return AttachNativeProgressBar(parent, id); }
    virtual BOOL ChangeToArrowButton(HWND parent, int id) { return ChangeNativeArrowButton(parent, id); }
    virtual CGUIButtonAbstract* AttachButton(HWND parent, int id, DWORD flags) { return AttachNativeButton(parent, id, flags); }
    virtual CGUIColorArrowButtonAbstract* AttachColorArrowButton(HWND parent, int id, BOOL arrow) { return AttachNativeColorArrowButton(parent, id, arrow); }
    virtual CGUIToolbarHeaderAbstract* AttachToolbarHeader(HWND parent, int id, HWND align, DWORD mask) { return AttachNativeToolbarHeader(parent, id, align, mask); }
};

Win32NativeDialogHost Host;
struct Registration { Registration() { SetNativeDialogHost(&Host); } } RegistrationInstance;
} // namespace

INativeDialogHost* WINAPI GetWin32NativeDialogHost() { return &Host; }
} }
