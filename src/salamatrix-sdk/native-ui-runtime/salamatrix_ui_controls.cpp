// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>
#include "salamatrix_ui_controls.h"

namespace Salamatrix { namespace UI { namespace {

static std::wstring Wide(const char* text)
{
    if (text == NULL || text[0] == 0) return std::wstring();
    int length = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    std::wstring value(length > 0 ? static_cast<size_t>(length) : 0, L'\0');
    if (length > 1) MultiByteToWideChar(CP_UTF8, 0, text, -1, &value[0], length);
    if (!value.empty()) value.resize(value.size() - 1);
    return value;
}

static std::string Utf8(const wchar_t* text)
{
    if (text == NULL || text[0] == 0) return std::string();
    int length = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
    std::string value(length > 0 ? static_cast<size_t>(length) : 0, '\0');
    if (length > 1) WideCharToMultiByte(CP_UTF8, 0, text, -1, &value[0], length, NULL, NULL);
    if (!value.empty()) value.resize(value.size() - 1);
    return value;
}

static std::string WindowText(HWND window)
{
    int length = GetWindowTextLengthW(window);
    std::wstring text(static_cast<size_t>(length + 1), L'\0');
    GetWindowTextW(window, &text[0], length + 1);
    return Utf8(text.c_str());
}

class StaticTextControl : public CGUIStaticTextAbstract
{
protected:
    HWND Window;
    DWORD Flags;
    std::string Text;
    std::string ToolTip;
    char Separator;
    HFONT DerivedFont;

    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        StaticTextControl* self = reinterpret_cast<StaticTextControl*>(data);
        if (message == WM_SETTEXT && self != NULL && lParam != 0)
            self->Text = Utf8(reinterpret_cast<const wchar_t*>(lParam));
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Proc, id);
            delete self;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

public:
    StaticTextControl(HWND window, DWORD flags)
        : Window(window), Flags(flags), Text(WindowText(window)), Separator('\\'), DerivedFont(NULL)
    {
        SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this));
        if ((Flags & (STF_BOLD | STF_UNDERLINE)) != 0)
        {
            HFONT source = reinterpret_cast<HFONT>(SendMessage(Window, WM_GETFONT, 0, 0));
            LOGFONTW font;
            if (source != NULL && GetObjectW(source, sizeof(font), &font) == sizeof(font))
            {
                if ((Flags & STF_BOLD) != 0) font.lfWeight = FW_BOLD;
                if ((Flags & STF_UNDERLINE) != 0) font.lfUnderline = TRUE;
                DerivedFont = CreateFontIndirectW(&font);
                if (DerivedFont != NULL) SendMessage(Window, WM_SETFONT, reinterpret_cast<WPARAM>(DerivedFont), TRUE);
            }
        }
        LONG_PTR style = GetWindowLongPtr(Window, GWL_STYLE);
        if ((Flags & STF_END_ELLIPSIS) != 0) style |= SS_ENDELLIPSIS;
        if ((Flags & STF_PATH_ELLIPSIS) != 0) style |= SS_PATHELLIPSIS;
        if ((Flags & STF_HANDLEPREFIX) == 0) style |= SS_NOPREFIX;
        SetWindowLongPtr(Window, GWL_STYLE, style);
    }

    virtual ~StaticTextControl()
    {
        if (DerivedFont != NULL) DeleteObject(DerivedFont);
    }

    virtual BOOL WINAPI SetText(const char* text)
    {
        if (text == NULL) return FALSE;
        Text = text;
        std::wstring wide = Wide(text);
        return SetWindowTextW(Window, wide.c_str());
    }
    virtual const char* WINAPI GetText() { return Text.empty() ? NULL : Text.c_str(); }
    virtual void WINAPI SetPathSeparator(char separator) { Separator = separator; }
    virtual BOOL WINAPI SetToolTipText(const char* text) { ToolTip = text != NULL ? text : ""; return TRUE; }
    virtual void WINAPI SetToolTip(HWND, DWORD) {}
};

class HyperLinkControl : public CGUIHyperLinkAbstract
{
    HWND Window;
    std::string Text, Target, Hint, ToolTip;
    WORD Command;
    HFONT Font;
    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        HyperLinkControl* self = reinterpret_cast<HyperLinkControl*>(data);
        if (self != NULL && (message == WM_LBUTTONUP || (message == WM_KEYUP && wParam == VK_SPACE)))
        {
            if (!self->Target.empty()) ShellExecuteW(window, L"open", Wide(self->Target.c_str()).c_str(), NULL, NULL, SW_SHOWNORMAL);
            else if (self->Command != 0) PostMessage(GetParent(window), WM_COMMAND, self->Command, 0);
            return 0;
        }
        if (message == WM_SETCURSOR) { SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE; }
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Proc, id);
            delete self;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    HyperLinkControl(HWND window, DWORD) : Window(window), Text(WindowText(window)), Command(0), Font(NULL)
    {
        SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this));
        LONG_PTR style = GetWindowLongPtr(Window, GWL_STYLE);
        SetWindowLongPtr(Window, GWL_STYLE, style | SS_NOTIFY | WS_TABSTOP);
        HFONT source = reinterpret_cast<HFONT>(SendMessage(Window, WM_GETFONT, 0, 0));
        LOGFONTW font;
        if (source != NULL && GetObjectW(source, sizeof(font), &font) == sizeof(font))
        {
            font.lfUnderline = TRUE; Font = CreateFontIndirectW(&font);
            if (Font != NULL) SendMessage(Window, WM_SETFONT, reinterpret_cast<WPARAM>(Font), TRUE);
        }
    }
    virtual ~HyperLinkControl() { if (Font != NULL) DeleteObject(Font); }
    virtual BOOL WINAPI SetText(const char* text) { if (text == NULL) return FALSE; Text = text; return SetWindowTextW(Window, Wide(text).c_str()); }
    virtual const char* WINAPI GetText() { return Text.empty() ? NULL : Text.c_str(); }
    virtual void WINAPI SetActionOpen(const char* file) { Target = file != NULL ? file : ""; Command = 0; }
    virtual void WINAPI SetActionPostCommand(WORD command) { Command = command; Target.clear(); }
    virtual BOOL WINAPI SetActionShowHint(const char* text) { Hint = text != NULL ? text : ""; return TRUE; }
    virtual BOOL WINAPI SetToolTipText(const char* text) { ToolTip = text != NULL ? text : ""; return TRUE; }
    virtual void WINAPI SetToolTip(HWND, DWORD) {}
};

class ProgressControl : public CGUIProgressBarAbstract
{
    HWND Window;
    DWORD Value, MoveTime, MoveSpeed;
    std::string Text;
    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        ProgressControl* self = reinterpret_cast<ProgressControl*>(data);
        if (message == WM_PAINT && self != NULL)
        {
            PAINTSTRUCT paint; HDC dc = BeginPaint(window, &paint); RECT rect; GetClientRect(window, &rect);
            FillRect(dc, &rect, GetSysColorBrush(COLOR_3DFACE)); FrameRect(dc, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            RECT fill = rect; InflateRect(&fill, -1, -1); fill.right = fill.left + MulDiv(fill.right - fill.left, self->Value > 1000 ? 1000 : self->Value, 1000);
            FillRect(dc, &fill, GetSysColorBrush(COLOR_HIGHLIGHT));
            std::wstring text = self->Text.empty() ? std::to_wstring(self->Value / 10) + L" %" : Wide(self->Text.c_str());
            SetBkMode(dc, TRANSPARENT); DrawTextW(dc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            EndPaint(window, &paint); return 0;
        }
        if (message == WM_NCDESTROY) { RemoveWindowSubclass(window, Proc, id); delete self; }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ProgressControl(HWND window) : Window(window), Value(0), MoveTime(0xFFFFFFFF), MoveSpeed(50) { SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this)); }
    virtual void WINAPI SetProgress(DWORD progress, const char* text) { Value = progress == static_cast<DWORD>(-1) ? 500 : progress; Text = text != NULL ? text : ""; InvalidateRect(Window, NULL, TRUE); UpdateWindow(Window); }
    virtual void WINAPI SetSelfMoveTime(DWORD time) { MoveTime = time; }
    virtual void WINAPI SetSelfMoveSpeed(DWORD time) { MoveSpeed = time; }
    virtual void WINAPI Stop() { KillTimer(Window, 1); }
    virtual void WINAPI SetProgress2(const CQuadWord& current, const CQuadWord& total, const char* text) { SetProgress(total.Value == 0 ? 0 : static_cast<DWORD>((current.Value >= total.Value ? 1000 : current.Value * 1000 / total.Value)), text); }
};

class ButtonControl : public CGUIButtonAbstract
{
    HWND Window; std::string ToolTip;
    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        if (message == WM_NCDESTROY) { ButtonControl* self = reinterpret_cast<ButtonControl*>(data); RemoveWindowSubclass(window, Proc, id); delete self; }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ButtonControl(HWND window, DWORD flags) : Window(window) { SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this)); if ((flags & BTF_MORE) != 0) SetWindowTextW(Window, (Wide(WindowText(Window).c_str()) + L"  »").c_str()); else if ((flags & (BTF_DROPDOWN | BTF_RIGHTARROW)) != 0) SetWindowTextW(Window, (Wide(WindowText(Window).c_str()) + L"  ▾").c_str()); }
    virtual BOOL WINAPI SetToolTipText(const char* text) { ToolTip = text != NULL ? text : ""; return TRUE; }
    virtual void WINAPI SetToolTip(HWND, DWORD) {}
};

class ColorButtonControl : public CGUIColorArrowButtonAbstract
{
    HWND Window; COLORREF TextColor, Background; BOOL Arrow;
    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        ColorButtonControl* self = reinterpret_cast<ColorButtonControl*>(data);
        if (message == WM_PAINT && self != NULL)
        {
            PAINTSTRUCT paint; HDC dc = BeginPaint(window, &paint); RECT r; GetClientRect(window, &r); DrawFrameControl(dc, &r, DFC_BUTTON, DFCS_BUTTONPUSH);
            InflateRect(&r, -5, -4); if (self->Arrow) r.right -= 13; HBRUSH brush = CreateSolidBrush(self->Background); FillRect(dc, &r, brush); DeleteObject(brush); FrameRect(dc, &r, GetSysColorBrush(COLOR_BTNSHADOW));
            std::wstring text = Wide(WindowText(window).c_str()); SetBkMode(dc, TRANSPARENT); ::SetTextColor(dc, self->TextColor); DrawTextW(dc, text.c_str(), -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (self->Arrow) { RECT arrow = r; arrow.left = r.right + 4; arrow.right += 13; DrawTextW(dc, L"▼", -1, &arrow, DT_CENTER | DT_VCENTER | DT_SINGLELINE); }
            EndPaint(window, &paint); return 0;
        }
        if (message == WM_NCDESTROY) { RemoveWindowSubclass(window, Proc, id); delete self; }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ColorButtonControl(HWND window, BOOL arrow) : Window(window), TextColor(RGB(0,0,0)), Background(RGB(255,255,255)), Arrow(arrow) { SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this)); }
    virtual void WINAPI SetColor(COLORREF text, COLORREF background) { TextColor = text; Background = background; InvalidateRect(Window, NULL, TRUE); }
    virtual void WINAPI SetTextColor(COLORREF color) { SetColor(color, Background); }
    virtual void WINAPI SetBkgndColor(COLORREF color) { SetColor(TextColor, color); }
    virtual COLORREF WINAPI GetTextColor() { return TextColor; }
    virtual COLORREF WINAPI GetBkgndColor() { return Background; }
};

class ToolbarHeaderControl : public CGUIToolbarHeaderAbstract
{
    HWND Window, Notify; DWORD Mask, Enabled, Checked;
    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        if (message == WM_NCDESTROY)
        {
            ToolbarHeaderControl* self = reinterpret_cast<ToolbarHeaderControl*>(data);
            RemoveWindowSubclass(window, Proc, id);
            delete self;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ToolbarHeaderControl(HWND window, HWND align, DWORD mask) : Window(window), Notify(GetParent(window)), Mask(mask), Enabled(mask), Checked(0) { SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this)); RECT a; GetWindowRect(align, &a); MapWindowPoints(NULL, GetParent(window), reinterpret_cast<POINT*>(&a), 2); RECT r; GetWindowRect(window, &r); MapWindowPoints(NULL, GetParent(window), reinterpret_cast<POINT*>(&r), 2); SetWindowPos(window, NULL, a.left, r.top, a.right-a.left, r.bottom-r.top, SWP_NOZORDER); SetWindowTextW(window, L"✎  ＋  ×  ↕  ↑  ↓  ⇈  ⌕"); }
    virtual void WINAPI EnableToolbar(DWORD mask) { Enabled = mask; EnableWindow(Window, mask != 0); }
    virtual void WINAPI CheckToolbar(DWORD mask) { Checked = mask; }
    virtual void WINAPI SetNotifyWindow(HWND window) { Notify = window; }
};

} // namespace

CGUIStaticTextAbstract* AttachNativeStaticText(HWND parent, int id, DWORD flags) { HWND w = GetDlgItem(parent, id); return w != NULL ? new StaticTextControl(w, flags) : NULL; }
CGUIHyperLinkAbstract* AttachNativeHyperLink(HWND parent, int id, DWORD flags) { HWND w = GetDlgItem(parent, id); return w != NULL ? new HyperLinkControl(w, flags) : NULL; }
CGUIProgressBarAbstract* AttachNativeProgressBar(HWND parent, int id) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ProgressControl(w) : NULL; }
BOOL ChangeNativeArrowButton(HWND parent, int id) { HWND w = GetDlgItem(parent, id); if (w == NULL) return FALSE; SetWindowTextW(w, L"▶"); return TRUE; }
CGUIButtonAbstract* AttachNativeButton(HWND parent, int id, DWORD flags) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ButtonControl(w, flags) : NULL; }
CGUIColorArrowButtonAbstract* AttachNativeColorArrowButton(HWND parent, int id, BOOL showArrow) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ColorButtonControl(w, showArrow) : NULL; }
CGUIToolbarHeaderAbstract* AttachNativeToolbarHeader(HWND parent, int id, HWND align, DWORD mask) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ToolbarHeaderControl(w, align, mask) : NULL; }

} }
