// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <algorithm>
#include <string>
#include "salamatrix_ui_controls.h"
#include "salamatrix_ui_host.h"

namespace Salamatrix { namespace UI { namespace {

static BOOL IsDarkMode()
{
    INativeDialogHost* host = GetNativeDialogHost();
    return host != NULL && host->IsDarkMode();
}

static void FillSolid(HDC dc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);
}

static void FrameSolid(HDC dc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FrameRect(dc, &rect, brush);
    DeleteObject(brush);
}

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

class NativeToolTip
{
    HWND Target;
    HWND Window;
    std::wstring Text;

public:
    explicit NativeToolTip(HWND target) : Target(target), Window(NULL) {}
    ~NativeToolTip() { if (Window != NULL) DestroyWindow(Window); }

    BOOL SetText(const char* text)
    {
        Text = Wide(text != NULL ? text : "");
        if (Text.empty()) return TRUE;
        if (Window == NULL)
        {
            Window = CreateWindowExW(
                WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
                WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                GetParent(Target), NULL, GetModuleHandleW(NULL), NULL);
            if (Window == NULL) return FALSE;
            SetWindowPos(Window, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            SendMessageW(Window, TTM_SETMAXTIPWIDTH, 0, 600);
            SetWindowTheme(Window, IsDarkMode() ? L"DarkMode_Explorer" : L"Explorer", NULL);
        }
        TOOLINFOW tool;
        memset(&tool, 0, sizeof(tool));
        tool.cbSize = sizeof(tool);
        tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        tool.hwnd = GetParent(Target);
        tool.uId = reinterpret_cast<UINT_PTR>(Target);
        tool.lpszText = const_cast<wchar_t*>(Text.c_str());
        SendMessageW(Window, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        return SendMessageW(Window, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool)) != FALSE;
    }

    void ShowAtTarget()
    {
        if (Window == NULL || Text.empty()) return;
        TOOLINFOW tool;
        memset(&tool, 0, sizeof(tool));
        tool.cbSize = sizeof(tool);
        tool.uFlags = TTF_TRACK | TTF_ABSOLUTE;
        tool.hwnd = GetParent(Target);
        tool.uId = reinterpret_cast<UINT_PTR>(Target);
        tool.lpszText = const_cast<wchar_t*>(Text.c_str());
        SendMessageW(Window, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        SendMessageW(Window, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        RECT targetRect;
        GetWindowRect(Target, &targetRect);
        SendMessageW(Window, TTM_TRACKPOSITION, 0, MAKELPARAM(targetRect.left, targetRect.bottom + 2));
        SendMessageW(Window, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&tool));
    }
};

class StaticTextControl : public CGUIStaticTextAbstract
{
protected:
    HWND Window;
    DWORD Flags;
    std::string Text;
    std::string ToolTip;
    NativeToolTip ToolTipControl;
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
        : Window(window), Flags(flags), Text(WindowText(window)), ToolTipControl(window), Separator('\\'), DerivedFont(NULL)
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
    virtual BOOL WINAPI SetToolTipText(const char* text) { ToolTip = text != NULL ? text : ""; return ToolTipControl.SetText(ToolTip.c_str()); }
    virtual void WINAPI SetToolTip(HWND, DWORD) {}
};

class HyperLinkControl : public CGUIHyperLinkAbstract
{
    HWND Window;
    std::string Text, Target, Hint, ToolTip;
    WORD Command;
    DWORD Flags;
    HFONT Font;
    NativeToolTip ToolTipControl;

    void Paint()
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(Window, &paint);
        RECT rect;
        GetClientRect(Window, &rect);
        DrawThemeParentBackground(Window, dc, &rect);
        HFONT font = Font != NULL ? Font : reinterpret_cast<HFONT>(SendMessage(Window, WM_GETFONT, 0, 0));
        HFONT oldFont = font != NULL ? reinterpret_cast<HFONT>(SelectObject(dc, font)) : NULL;
        SetBkMode(dc, TRANSPARENT);
        const COLORREF normal = IsDarkMode() ? RGB(240, 240, 240) : GetSysColor(COLOR_WINDOWTEXT);
        const COLORREF link = IsDarkMode() ? RGB(79, 177, 255) : GetSysColor(COLOR_HOTLIGHT);
        SetTextColor(dc, (Flags & STF_HYPERLINK_COLOR) != 0 ? link : normal);
        std::wstring wide = Wide(Text.c_str());
        RECT textRect = rect;
        DrawTextW(dc, wide.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if ((Flags & STF_DOTUNDERLINE) != 0)
        {
            SIZE size = {0, 0};
            GetTextExtentPoint32W(dc, wide.c_str(), static_cast<int>(wide.size()), &size);
            TEXTMETRICW metric;
            GetTextMetricsW(dc, &metric);
            const int y = (std::min)(rect.bottom - 1, (rect.bottom - rect.top - metric.tmHeight) / 2 + metric.tmAscent + 1);
            HPEN pen = CreatePen(PS_DOT, 1, (Flags & STF_HYPERLINK_COLOR) != 0 ? link : normal);
            HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(dc, pen));
            MoveToEx(dc, rect.left, y, NULL);
            LineTo(dc, (std::min)(rect.right, rect.left + size.cx), y);
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        if (GetFocus() == Window)
        {
            RECT focus = rect;
            focus.right = (std::min)(focus.right, focus.left + textRect.right - textRect.left);
            DrawFocusRect(dc, &focus);
        }
        if (oldFont != NULL) SelectObject(dc, oldFont);
        EndPaint(Window, &paint);
    }

    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        HyperLinkControl* self = reinterpret_cast<HyperLinkControl*>(data);
        if (self != NULL && (message == WM_LBUTTONUP || (message == WM_KEYUP && wParam == VK_SPACE)))
        {
            if (!self->Hint.empty()) self->ToolTipControl.ShowAtTarget();
            else if (!self->Target.empty()) ShellExecuteW(window, L"open", Wide(self->Target.c_str()).c_str(), NULL, NULL, SW_SHOWNORMAL);
            else if (self->Command != 0) PostMessage(GetParent(window), WM_COMMAND, self->Command, 0);
            return 0;
        }
        if (message == WM_PAINT && self != NULL) { self->Paint(); return 0; }
        if (message == WM_ERASEBKGND) return 1;
        if ((message == WM_SETFOCUS || message == WM_KILLFOCUS) && self != NULL) InvalidateRect(window, NULL, TRUE);
        if (message == WM_SETCURSOR) { SetCursor(LoadCursor(NULL, IDC_HAND)); return TRUE; }
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Proc, id);
            delete self;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    HyperLinkControl(HWND window, DWORD flags)
        : Window(window), Text(WindowText(window)), Command(0), Flags(flags), Font(NULL), ToolTipControl(window)
    {
        SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this));
        LONG_PTR style = GetWindowLongPtr(Window, GWL_STYLE);
        SetWindowLongPtr(Window, GWL_STYLE, style | SS_NOTIFY | WS_TABSTOP);
        HFONT source = reinterpret_cast<HFONT>(SendMessage(Window, WM_GETFONT, 0, 0));
        LOGFONTW font;
        if ((Flags & STF_UNDERLINE) != 0 && source != NULL && GetObjectW(source, sizeof(font), &font) == sizeof(font))
        {
            font.lfUnderline = TRUE; Font = CreateFontIndirectW(&font);
        }
    }
    virtual ~HyperLinkControl() { if (Font != NULL) DeleteObject(Font); }
    virtual BOOL WINAPI SetText(const char* text) { if (text == NULL) return FALSE; Text = text; SetWindowTextW(Window, Wide(text).c_str()); InvalidateRect(Window, NULL, TRUE); return TRUE; }
    virtual const char* WINAPI GetText() { return Text.empty() ? NULL : Text.c_str(); }
    virtual void WINAPI SetActionOpen(const char* file) { Target = file != NULL ? file : ""; Command = 0; }
    virtual void WINAPI SetActionPostCommand(WORD command) { Command = command; Target.clear(); }
    virtual BOOL WINAPI SetActionShowHint(const char* text) { Hint = text != NULL ? text : ""; return ToolTipControl.SetText(Hint.c_str()); }
    virtual BOOL WINAPI SetToolTipText(const char* text) { ToolTip = text != NULL ? text : ""; return ToolTipControl.SetText(ToolTip.c_str()); }
    virtual void WINAPI SetToolTip(HWND, DWORD) {}
};

class ProgressControl : public CGUIProgressBarAbstract
{
    HWND Window;
    DWORD Value, MoveTime, MoveSpeed;
    DWORD MoveStarted;
    int BarX;
    BOOL MoveRight;
    BOOL Indeterminate;
    std::string Text;

    void StopTimer()
    {
        KillTimer(Window, 1);
    }

    void StartTimer()
    {
        StopTimer();
        MoveStarted = GetTickCount();
        if (MoveTime != 0) SetTimer(Window, 1, (std::max<DWORD>)(10, MoveSpeed), NULL);
    }

    void MoveBlock()
    {
        RECT rect;
        GetClientRect(Window, &rect);
        const int height = static_cast<int>(rect.bottom - rect.top);
        const int half = (std::max)(4, height * 2);
        const int step = (std::max)(1, height / 6);
        BarX += MoveRight ? step : -step;
        if (BarX + half >= rect.right - 1) { BarX = rect.right - 1 - half; MoveRight = FALSE; }
        if (BarX - half <= rect.left + 1) { BarX = rect.left + 1 + half; MoveRight = TRUE; }
        InvalidateRect(Window, NULL, FALSE);
    }

    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        ProgressControl* self = reinterpret_cast<ProgressControl*>(data);
        if (message == WM_PAINT && self != NULL)
        {
            PAINTSTRUCT paint; HDC dc = BeginPaint(window, &paint); RECT rect; GetClientRect(window, &rect);
            const BOOL dark = IsDarkMode();
            FillSolid(dc, rect, dark ? RGB(45, 45, 48) : GetSysColor(COLOR_3DFACE));
            FrameSolid(dc, rect, dark ? RGB(16, 16, 16) : GetSysColor(COLOR_3DSHADOW));
            RECT fill = rect;
            InflateRect(&fill, -1, -1);
            if (self->Indeterminate)
            {
                const int half = (std::max)(4, static_cast<int>(rect.bottom - rect.top) * 2);
                fill.left = (std::max<LONG>)(fill.left, static_cast<LONG>(self->BarX - half));
                fill.right = (std::min<LONG>)(fill.right, static_cast<LONG>(self->BarX + half));
                if (fill.right > fill.left) FillSolid(dc, fill, dark ? RGB(0, 128, 220) : GetSysColor(COLOR_HIGHLIGHT));
            }
            else
            {
                fill.right = fill.left + MulDiv(fill.right - fill.left, self->Value > 1000 ? 1000 : self->Value, 1000);
                FillSolid(dc, fill, dark ? RGB(0, 128, 220) : GetSysColor(COLOR_HIGHLIGHT));
                std::wstring text = self->Text.empty() ? std::to_wstring(self->Value / 10) + L" %" : Wide(self->Text.c_str());
                HFONT font = reinterpret_cast<HFONT>(SendMessage(window, WM_GETFONT, 0, 0));
                HFONT oldFont = font != NULL ? reinterpret_cast<HFONT>(SelectObject(dc, font)) : NULL;
                SetBkMode(dc, TRANSPARENT); ::SetTextColor(dc, dark ? RGB(240, 240, 240) : GetSysColor(COLOR_BTNTEXT));
                DrawTextW(dc, text.c_str(), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                if (oldFont != NULL) SelectObject(dc, oldFont);
            }
            EndPaint(window, &paint); return 0;
        }
        if (message == WM_TIMER && self != NULL && wParam == 1)
        {
            if (self->MoveTime != 0xFFFFFFFF && GetTickCount() - self->MoveStarted >= self->MoveTime)
                self->StopTimer();
            else
                self->MoveBlock();
            return 0;
        }
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_NCDESTROY) { if (self != NULL) self->StopTimer(); RemoveWindowSubclass(window, Proc, id); delete self; }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ProgressControl(HWND window)
        : Window(window), Value(0), MoveTime(0xFFFFFFFF), MoveSpeed(50), MoveStarted(0),
          BarX(1), MoveRight(TRUE), Indeterminate(FALSE)
    {
        SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this));
    }
    virtual void WINAPI SetProgress(DWORD progress, const char* text)
    {
        Indeterminate = progress == static_cast<DWORD>(-1);
        Text = text != NULL ? text : "";
        if (Indeterminate)
        {
            RECT rect; GetClientRect(Window, &rect);
            BarX = static_cast<int>(rect.left) + (std::max)(4, static_cast<int>(rect.bottom - rect.top) * 2) + 1;
            MoveRight = TRUE;
            StartTimer();
            if (MoveTime == 0) MoveBlock();
        }
        else
        {
            StopTimer();
            Value = progress;
        }
        InvalidateRect(Window, NULL, TRUE);
        UpdateWindow(Window);
    }
    virtual void WINAPI SetSelfMoveTime(DWORD time) { MoveTime = time; }
    virtual void WINAPI SetSelfMoveSpeed(DWORD time)
    {
        MoveSpeed = (std::max<DWORD>)(10, time);
        if (Indeterminate) StartTimer();
    }
    virtual void WINAPI Stop() { StopTimer(); }
    virtual void WINAPI SetProgress2(const CQuadWord& current, const CQuadWord& total, const char* text) { SetProgress(total.Value == 0 ? 0 : static_cast<DWORD>((current.Value >= total.Value ? 1000 : current.Value * 1000 / total.Value)), text); }
};

class ButtonControl : public CGUIButtonAbstract
{
    HWND Window;
    DWORD Flags;
    std::string ToolTip;
    NativeToolTip ToolTipControl;

    static void DrawArrow(HDC dc, const RECT& rect, BOOL down, COLORREF color)
    {
        const int cx = (rect.left + rect.right) / 2;
        const int cy = (rect.top + rect.bottom) / 2;
        POINT points[3];
        if (down)
        {
            points[0].x = cx - 3; points[0].y = cy - 1;
            points[1].x = cx + 3; points[1].y = cy - 1;
            points[2].x = cx; points[2].y = cy + 2;
        }
        else
        {
            points[0].x = cx - 1; points[0].y = cy - 3;
            points[1].x = cx - 1; points[1].y = cy + 3;
            points[2].x = cx + 2; points[2].y = cy;
        }
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HBRUSH brush = CreateSolidBrush(color);
        HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(dc, pen));
        HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(dc, brush));
        Polygon(dc, points, 3);
        SelectObject(dc, oldBrush); SelectObject(dc, oldPen);
        DeleteObject(brush); DeleteObject(pen);
    }

    static void DrawFrame(HWND window, HDC dc, const RECT& rect)
    {
        const BOOL enabled = IsWindowEnabled(window);
        const BOOL pressed = (SendMessage(window, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;
        if (!IsDarkMode())
        {
            HTHEME theme = OpenThemeData(window, L"Button");
            if (theme != NULL)
            {
                int state = !enabled ? PBS_DISABLED : pressed ? PBS_PRESSED : PBS_NORMAL;
                DrawThemeBackground(theme, dc, BP_PUSHBUTTON, state, &rect, NULL);
                CloseThemeData(theme);
                return;
            }
        }
        FillSolid(dc, rect, pressed ? RGB(65, 65, 68) : RGB(51, 51, 55));
        FrameSolid(dc, rect, pressed ? RGB(0, 122, 204) : RGB(112, 112, 112));
    }

    void Paint()
    {
        PAINTSTRUCT paint; HDC dc = BeginPaint(Window, &paint); RECT rect; GetClientRect(Window, &rect);
        DrawFrame(Window, dc, rect);
        RECT textRect = rect;
        const BOOL hasArrow = (Flags & (BTF_DROPDOWN | BTF_RIGHTARROW)) != 0;
        INativeDialogHost* host = GetNativeDialogHost();
        const UINT dpi = host != NULL ? host->GetWindowDpi(Window) : 96;
        const int arrowWidth = hasArrow ? (std::max)(13, MulDiv(14, dpi, 96)) : 0;
        if (hasArrow) textRect.right -= arrowWidth;
        std::wstring text = Wide(WindowText(Window).c_str());
        HFONT font = reinterpret_cast<HFONT>(SendMessage(Window, WM_GETFONT, 0, 0));
        HFONT oldFont = font != NULL ? reinterpret_cast<HFONT>(SelectObject(dc, font)) : NULL;
        SetBkMode(dc, TRANSPARENT);
        const COLORREF foreground = IsDarkMode() ? RGB(240, 240, 240) : GetSysColor(COLOR_BTNTEXT);
        SetTextColor(dc, foreground);
        if (!text.empty()) DrawTextW(dc, text.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (hasArrow)
        {
            RECT arrowRect = rect; arrowRect.left = textRect.right;
            if ((Flags & BTF_DROPDOWN) != 0)
            {
                HPEN line = CreatePen(PS_SOLID, 1, IsDarkMode() ? RGB(90, 90, 90) : GetSysColor(COLOR_3DSHADOW));
                HPEN old = reinterpret_cast<HPEN>(SelectObject(dc, line));
                MoveToEx(dc, arrowRect.left, arrowRect.top + 3, NULL); LineTo(dc, arrowRect.left, arrowRect.bottom - 3);
                SelectObject(dc, old); DeleteObject(line);
            }
            DrawArrow(dc, arrowRect, (Flags & BTF_DROPDOWN) != 0, foreground);
        }
        if (oldFont != NULL) SelectObject(dc, oldFont);
        EndPaint(Window, &paint);
    }

    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        ButtonControl* self = reinterpret_cast<ButtonControl*>(data);
        if (message == WM_PAINT && self != NULL) { self->Paint(); return 0; }
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_ENABLE || message == WM_SETFOCUS || message == WM_KILLFOCUS) InvalidateRect(window, NULL, TRUE);
        if (message == WM_NCDESTROY) { RemoveWindowSubclass(window, Proc, id); delete self; }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ButtonControl(HWND window, DWORD flags) : Window(window), Flags(flags), ToolTipControl(window) { SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this)); }
    virtual BOOL WINAPI SetToolTipText(const char* text) { ToolTip = text != NULL ? text : ""; return ToolTipControl.SetText(ToolTip.c_str()); }
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
            PAINTSTRUCT paint; HDC dc = BeginPaint(window, &paint); RECT r; GetClientRect(window, &r);
            const BOOL dark = IsDarkMode();
            if (!dark)
            {
                HTHEME theme = OpenThemeData(window, L"Button");
                if (theme != NULL)
                {
                    const BOOL pressed = (SendMessage(window, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;
                    DrawThemeBackground(theme, dc, BP_PUSHBUTTON, pressed ? PBS_PRESSED : PBS_NORMAL, &r, NULL);
                    CloseThemeData(theme);
                }
                else
                    DrawFrameControl(dc, &r, DFC_BUTTON, DFCS_BUTTONPUSH);
            }
            else
            {
                FillSolid(dc, r, RGB(51, 51, 55));
                FrameSolid(dc, r, RGB(112, 112, 112));
            }
            RECT content = r;
            InflateRect(&content, -5, -4);
            RECT arrow = content;
            if (self->Arrow)
            {
                arrow.left = arrow.right - 11;
                content.right = arrow.left - 3;
            }
            FillSolid(dc, content, self->Background);
            FrameSolid(dc, content, dark ? RGB(15, 15, 15) : GetSysColor(COLOR_BTNSHADOW));
            HFONT font = reinterpret_cast<HFONT>(SendMessage(window, WM_GETFONT, 0, 0));
            HFONT oldFont = font != NULL ? reinterpret_cast<HFONT>(SelectObject(dc, font)) : NULL;
            std::wstring text = Wide(WindowText(window).c_str()); SetBkMode(dc, TRANSPARENT); ::SetTextColor(dc, self->TextColor); DrawTextW(dc, text.c_str(), -1, &content, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            if (self->Arrow)
            {
                const COLORREF foreground = dark ? RGB(240, 240, 240) : GetSysColor(COLOR_BTNTEXT);
                const int cx = (arrow.left + arrow.right) / 2;
                const int cy = (arrow.top + arrow.bottom) / 2;
                POINT points[3] = {{cx - 3, cy - 1}, {cx + 3, cy - 1}, {cx, cy + 2}};
                HPEN pen = CreatePen(PS_SOLID, 1, foreground); HBRUSH brush = CreateSolidBrush(foreground);
                HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(dc, pen)); HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(dc, brush));
                Polygon(dc, points, 3);
                SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(brush); DeleteObject(pen);
            }
            if (oldFont != NULL) SelectObject(dc, oldFont);
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
    struct ButtonInfo { DWORD Mask; WORD Command; const wchar_t* Glyph; };
    HWND Window, Notify;
    DWORD Mask, Enabled, Checked;
    std::wstring Caption;

    static const ButtonInfo* Buttons(size_t& count)
    {
        static const ButtonInfo buttons[] = {
            {TLBHDRMASK_SEARCH, TLBHDR_SEARCH, L"⌕"},
            {TLBHDRMASK_MODIFY, TLBHDR_MODIFY, L"✎"},
            {TLBHDRMASK_NEW, TLBHDR_NEW, L"+"},
            {TLBHDRMASK_DELETE, TLBHDR_DELETE, L"×"},
            {TLBHDRMASK_SORT, TLBHDR_SORT, L"↕"},
            {TLBHDRMASK_TOP, TLBHDR_TOP, L"⇈"},
            {TLBHDRMASK_UP, TLBHDR_UP, L"↑"},
            {TLBHDRMASK_DOWN, TLBHDR_DOWN, L"↓"},
            {TLBHDRMASK_BOTTOM, TLBHDR_BOTTOM, L"⇊"},
            {TLBHDRMASK_FILTER, TLBHDR_FILTER, L"⏷"},
        };
        count = _countof(buttons);
        return buttons;
    }

    int ButtonWidth() const
    {
        INativeDialogHost* host = GetNativeDialogHost();
        const UINT dpi = host != NULL ? host->GetWindowDpi(Window) : 96;
        return MulDiv(18, dpi, 96);
    }

    int VisibleButtonCount() const
    {
        size_t count = 0; const ButtonInfo* buttons = Buttons(count); int visible = 0;
        for (size_t index = 0; index < count; ++index) if ((Mask & buttons[index].Mask) != 0) ++visible;
        return visible;
    }

    BOOL ButtonRect(DWORD mask, RECT& result) const
    {
        RECT client; GetClientRect(Window, &client);
        const int width = ButtonWidth();
        int position = 0;
        size_t count = 0; const ButtonInfo* buttons = Buttons(count);
        for (size_t index = 0; index < count; ++index)
        {
            if ((Mask & buttons[index].Mask) == 0) continue;
            if (buttons[index].Mask == mask)
            {
                result = client;
                result.left = client.right - (VisibleButtonCount() - position) * width;
                result.right = result.left + width;
                return TRUE;
            }
            ++position;
        }
        return FALSE;
    }

    void Paint()
    {
        PAINTSTRUCT paint; HDC dc = BeginPaint(Window, &paint); RECT rect; GetClientRect(Window, &rect);
        const BOOL dark = IsDarkMode();
        FillSolid(dc, rect, dark ? RGB(32, 32, 32) : GetSysColor(COLOR_BTNFACE));
        FrameSolid(dc, rect, dark ? RGB(80, 80, 80) : GetSysColor(COLOR_3DSHADOW));
        HFONT font = reinterpret_cast<HFONT>(SendMessage(Window, WM_GETFONT, 0, 0));
        HFONT oldFont = font != NULL ? reinterpret_cast<HFONT>(SelectObject(dc, font)) : NULL;
        SetBkMode(dc, TRANSPARENT);
        const COLORREF foreground = dark ? RGB(240, 240, 240) : GetSysColor(COLOR_BTNTEXT);
        SetTextColor(dc, foreground);
        RECT captionRect = rect; captionRect.left += 5; captionRect.right -= VisibleButtonCount() * ButtonWidth() + 2;
        DrawTextW(dc, Caption.c_str(), -1, &captionRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        size_t count = 0; const ButtonInfo* buttons = Buttons(count);
        for (size_t index = 0; index < count; ++index)
        {
            RECT button;
            if (!ButtonRect(buttons[index].Mask, button)) continue;
            const BOOL enabled = (Enabled & buttons[index].Mask) != 0;
            const COLORREF iconColor = !enabled ? (dark ? RGB(105, 105, 105) : GetSysColor(COLOR_GRAYTEXT)) :
                ((buttons[index].Mask & (TLBHDRMASK_UP | TLBHDRMASK_DOWN | TLBHDRMASK_TOP | TLBHDRMASK_BOTTOM)) != 0
                    ? (dark ? RGB(64, 169, 255) : RGB(0, 102, 204)) : foreground);
            SetTextColor(dc, iconColor);
            if ((Checked & buttons[index].Mask) != 0) FillSolid(dc, button, dark ? RGB(55, 85, 110) : GetSysColor(COLOR_HIGHLIGHT));
            DrawTextW(dc, buttons[index].Glyph, -1, &button, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        if (oldFont != NULL) SelectObject(dc, oldFont);
        EndPaint(Window, &paint);
    }

    static LRESULT CALLBACK Proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR data)
    {
        ToolbarHeaderControl* self = reinterpret_cast<ToolbarHeaderControl*>(data);
        if (message == WM_PAINT && self != NULL) { self->Paint(); return 0; }
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_LBUTTONUP && self != NULL)
        {
            POINT point = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            size_t count = 0; const ButtonInfo* buttons = Buttons(count);
            for (size_t index = 0; index < count; ++index)
            {
                RECT button;
                if (self->ButtonRect(buttons[index].Mask, button) && PtInRect(&button, point) && (self->Enabled & buttons[index].Mask) != 0)
                {
                    PostMessage(self->Notify, WM_COMMAND, MAKEWPARAM(buttons[index].Command, BN_CLICKED), reinterpret_cast<LPARAM>(window));
                    break;
                }
            }
            return 0;
        }
        if (message == WM_THEMECHANGED && self != NULL) { InvalidateRect(window, NULL, TRUE); return 0; }
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Proc, id);
            delete self;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }
public:
    ToolbarHeaderControl(HWND window, HWND align, DWORD mask)
        : Window(window), Notify(GetParent(window)), Mask(mask), Enabled(mask), Checked(0), Caption(Wide(WindowText(window).c_str()))
    {
        SetWindowSubclass(Window, Proc, 1, reinterpret_cast<DWORD_PTR>(this));
        RECT aligned; GetWindowRect(align, &aligned);
        MapWindowPoints(NULL, GetParent(window), reinterpret_cast<POINT*>(&aligned), 2);
        INativeDialogHost* host = GetNativeDialogHost();
        const UINT dpi = host != NULL ? host->GetWindowDpi(window) : 96;
        const int height = MulDiv(22, dpi, 96);
        SetWindowPos(window, NULL, aligned.left, aligned.top - height, aligned.right - aligned.left, height, SWP_NOZORDER);
    }
    virtual void WINAPI EnableToolbar(DWORD mask) { Enabled = mask; InvalidateRect(Window, NULL, TRUE); }
    virtual void WINAPI CheckToolbar(DWORD mask) { Checked = mask; InvalidateRect(Window, NULL, TRUE); }
    virtual void WINAPI SetNotifyWindow(HWND window) { Notify = window; }
};

} // namespace

CGUIStaticTextAbstract* AttachNativeStaticText(HWND parent, int id, DWORD flags) { HWND w = GetDlgItem(parent, id); return w != NULL ? new StaticTextControl(w, flags) : NULL; }
CGUIHyperLinkAbstract* AttachNativeHyperLink(HWND parent, int id, DWORD flags) { HWND w = GetDlgItem(parent, id); return w != NULL ? new HyperLinkControl(w, flags) : NULL; }
CGUIProgressBarAbstract* AttachNativeProgressBar(HWND parent, int id) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ProgressControl(w) : NULL; }
BOOL ChangeNativeArrowButton(HWND parent, int id) { HWND w = GetDlgItem(parent, id); if (w == NULL) return FALSE; SetWindowTextW(w, L""); new ButtonControl(w, BTF_RIGHTARROW); return TRUE; }
CGUIButtonAbstract* AttachNativeButton(HWND parent, int id, DWORD flags) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ButtonControl(w, flags) : NULL; }
CGUIColorArrowButtonAbstract* AttachNativeColorArrowButton(HWND parent, int id, BOOL showArrow) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ColorButtonControl(w, showArrow) : NULL; }
CGUIToolbarHeaderAbstract* AttachNativeToolbarHeader(HWND parent, int id, HWND align, DWORD mask) { HWND w = GetDlgItem(parent, id); return w != NULL ? new ToolbarHeaderControl(w, align, mask) : NULL; }

} }
