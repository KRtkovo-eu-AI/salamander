// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "hypervm.h"
#include "managed_bridge.h"
#include "../../darkmode.h"
#include "../../common/winlibdpi.h"

#include <commdlg.h>
#include <strsafe.h>
#include <string>
#include <vector>

#define IDC_VHD_PATH 1001
#define IDC_VHD_BROWSE 1002
#define IDC_VHD_SIZE 1003
#define IDC_VHD_UNIT 1004
#define IDC_VHD_FORMAT_VHD 1005
#define IDC_VHD_FORMAT_VHDX 1006
#define IDC_VHD_TYPE_FIXED 1007
#define IDC_VHD_TYPE_DYNAMIC 1008
#define IDC_VHD_READONLY 1009

namespace
{
const int VHD_PATH_BUFFER = 32768;

struct CVhdDialogState
{
    bool Create;
    bool Done;
    bool Result;
    HWND Parent;
    HWND Window;
    HWND Path;
    HWND Ok;
    HWND Size;
    HWND Unit;
    HWND ReadOnly;
    bool Dark;
    HBRUSH BackgroundBrush;
    HFONT Font;
    UINT Dpi;
};

bool ShouldUseDarkMode(COLORREF background)
{
    int r = GetRValue(background);
    int g = GetGValue(background);
    int b = GetBValue(background);
    return (r * 30 + g * 59 + b * 11) / 100 < 128;
}

std::wstring Utf8ToWide(const char* text)
{
    if (text == nullptr) return std::wstring();
    int required = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
    if (required <= 0) return std::wstring();
    std::wstring out(required, L'\0');
    int converted = MultiByteToWideChar(CP_UTF8, 0, text, -1, out.data(), required);
    if (converted <= 0) return std::wstring();
    out.resize(converted - 1);
    return out;
}

const wchar_t* WStr(int id)
{
    static wchar_t buffers[16][1024];
    static int index = 0;
    wchar_t* buffer = buffers[index++ % 16];
    buffer[0] = L'\0';

    char textA[4096];
    textA[0] = '\0';
    LoadStringA(HLanguage, id, textA, _countof(textA));
    const char* text = textA[0] != '\0' ? textA : LoadStr(id);
    if (text == nullptr)
    {
        return buffer;
    }

    int converted = MultiByteToWideChar(CP_UTF8, 0, text, -1, buffer, 1024);
    if (converted <= 0)
    {
        MultiByteToWideChar(CP_ACP, 0, text, -1, buffer, 1024);
    }
    buffer[1023] = L'\0';
    return buffer;
}

std::string AStr(int id)
{
    const char* text = LoadStr(id);
    return text != nullptr ? std::string(text) : std::string();
}

void SetFont(HWND child, HFONT font)
{
    SendMessage(child, WM_SETFONT, (WPARAM)font, TRUE);
}

HWND AddControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int height, int id, HFONT font)
{
    HWND control = CreateWindowExW(
        0, cls, text, WS_CHILD | WS_VISIBLE | style,
        WinLibDPIFromLogical(parent, x), WinLibDPIFromLogical(parent, y),
        WinLibDPIFromLogical(parent, w), WinLibDPIFromLogical(parent, height),
        parent, (HMENU)(INT_PTR)id, DLLInstance, nullptr);
    SetFont(control, font);
    return control;
}

void ApplyDialogDPI(CVhdDialogState* s, UINT newDpi, const RECT* suggestedRect)
{
    if (s == nullptr || s->Window == nullptr || newDpi == 0)
        return;

    UINT oldDpi = s->Dpi != 0 ? s->Dpi : USER_DEFAULT_SCREEN_DPI;
    if (suggestedRect != nullptr)
    {
        SetWindowPos(s->Window, nullptr, suggestedRect->left, suggestedRect->top,
                     suggestedRect->right - suggestedRect->left,
                     suggestedRect->bottom - suggestedRect->top,
                     SWP_NOACTIVATE | SWP_NOZORDER);
    }

    if (oldDpi != newDpi)
    {
        for (HWND child = GetWindow(s->Window, GW_CHILD); child != nullptr;
             child = GetWindow(child, GW_HWNDNEXT))
        {
            RECT rect;
            GetWindowRect(child, &rect);
            MapWindowPoints(nullptr, s->Window, reinterpret_cast<POINT*>(&rect), 2);
            SetWindowPos(child, nullptr,
                         MulDiv(rect.left, newDpi, oldDpi),
                         MulDiv(rect.top, newDpi, oldDpi),
                         MulDiv(rect.right - rect.left, newDpi, oldDpi),
                         MulDiv(rect.bottom - rect.top, newDpi, oldDpi),
                         SWP_NOACTIVATE | SWP_NOZORDER);
        }
    }

    HFONT newFont = WinLibDPICreateMessageFont(s->Window);
    if (newFont != nullptr)
    {
        for (HWND child = GetWindow(s->Window, GW_CHILD); child != nullptr;
             child = GetWindow(child, GW_HWNDNEXT))
        {
            SendMessage(child, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
        }
        HFONT oldFont = s->Font;
        s->Font = newFont;
        if (oldFont != nullptr)
            DeleteObject(oldFont);
    }
    s->Dpi = newDpi;
    RedrawWindow(s->Window, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
}

void EnableOk(CVhdDialogState* s)
{
    wchar_t path[2];
    EnableWindow(s->Ok, GetWindowTextW(s->Path, path, 2) > 0);
}

bool Browse(HWND owner, bool save, std::wstring& path)
{
    std::vector<wchar_t> file(VHD_PATH_BUFFER, L'\0');
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = file.data();
    ofn.nMaxFile = (DWORD)file.size();
    std::wstring filter = WStr(IDS_VHD_FILTER);
    for (wchar_t& ch : filter) if (ch == L'|') ch = L'\0';
    filter.push_back(L'\0');
    ofn.lpstrFilter = filter.c_str();
    ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    ofn.lpstrDefExt = L"vhdx";
    BOOL ok = save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return false;
    path = file.data();
    return true;
}

void Finish(CVhdDialogState* s, bool result)
{
    s->Result = result;
    s->Done = true;
    DestroyWindow(s->Window);
}

void ExecuteCreate(CVhdDialogState* s)
{
    std::vector<wchar_t> path(VHD_PATH_BUFFER, L'\0');
    wchar_t sizeText[64] = L"";
    GetWindowTextW(s->Path, path.data(), (int)path.size());
    GetWindowTextW(s->Size, sizeText, _countof(sizeText));
    unsigned long long value = _wtoi64(sizeText);
    int unit = (int)SendMessage(s->Unit, CB_GETCURSEL, 0, 0);
    unsigned long long multiplier = unit == 2 ? 1024ULL * 1024 * 1024 * 1024 : unit == 1 ? 1024ULL * 1024 * 1024 : 1024ULL * 1024;
    const wchar_t* format = IsDlgButtonChecked(s->Window, IDC_VHD_FORMAT_VHD) == BST_CHECKED ? L"VHD" : L"VHDX";
    const wchar_t* type = IsDlgButtonChecked(s->Window, IDC_VHD_TYPE_FIXED) == BST_CHECKED ? L"Fixed" : L"Dynamic";
    std::vector<wchar_t> payload(2 * VHD_PATH_BUFFER, L'\0');
    StringCchPrintfW(payload.data(), payload.size(), L"CreateVhd|%s|%llu|%s|%s", path.data(), value * multiplier, format, type);
    if (ManagedBridge_RunMenuCommandW(s->Parent, payload.data())) Finish(s, true);
}

void ExecuteAttach(CVhdDialogState* s)
{
    std::vector<wchar_t> path(VHD_PATH_BUFFER, L'\0');
    GetWindowTextW(s->Path, path.data(), (int)path.size());
    std::vector<wchar_t> payload(2 * VHD_PATH_BUFFER, L'\0');
    StringCchPrintfW(payload.data(), payload.size(), L"AttachVhd|%s|%d", path.data(), IsDlgButtonChecked(s->Window, IDC_VHD_READONLY) == BST_CHECKED ? 1 : 0);
    if (ManagedBridge_RunMenuCommandW(s->Parent, payload.data())) Finish(s, true);
}

LRESULT CALLBACK VhdWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CVhdDialogState* s = (CVhdDialogState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (msg)
    {
    case WM_CREATE:
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lp)->lpCreateParams);
        return 0;
    case WM_DPICHANGED:
        if (s != nullptr)
        {
            ApplyDialogDPI(s, LOWORD(wp), reinterpret_cast<const RECT*>(lp));
            return 0;
        }
        break;
    case WM_COMMAND:
        if (!s) break;
        if (LOWORD(wp) == IDC_VHD_PATH && HIWORD(wp) == EN_CHANGE) EnableOk(s);
        else if (LOWORD(wp) == IDC_VHD_BROWSE)
        {
            std::wstring path;
            if (Browse(hwnd, s->Create, path)) SetWindowTextW(s->Path, path.c_str());
        }
        else if (LOWORD(wp) == IDOK) s->Create ? ExecuteCreate(s) : ExecuteAttach(s);
        else if (LOWORD(wp) == IDCANCEL) Finish(s, false);
        return 0;
    case WM_CLOSE:
        if (s) Finish(s, false);
        return 0;
    case WM_DESTROY:
        if (s != nullptr && s->Font != nullptr)
        {
            DeleteObject(s->Font);
            s->Font = nullptr;
        }
        return 0;
    case WM_ERASEBKGND:
        if (s != nullptr && s->Dark && s->BackgroundBrush != nullptr)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect((HDC)wp, &rc, s->BackgroundBrush);
            return TRUE;
        }
        break;
    default:
        if (s != nullptr && s->Dark)
        {
            LRESULT result;
            if (DarkModeHandleCtlColor(msg, wp, lp, result)) return result;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

bool RunDialog(HWND parent, bool create)
{
    CVhdDialogState s = {0};
    s.Create = create;
    s.Parent = parent;
    COLORREF background = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_BK_NORMAL);
    COLORREF text = SalamanderGeneral->GetCurrentColor(SALCOL_ITEM_FG_NORMAL);
    bool dark = ShouldUseDarkMode(background);
    COLORREF fallbackText = GetSysColor(COLOR_BTNTEXT);
    COLORREF fallbackBackground = GetSysColor(COLOR_BTNFACE);
    if (!dark)
    {
        text = fallbackText;
        background = fallbackBackground;
    }
    s.Dark = dark;
    s.BackgroundBrush = CreateSolidBrush(background);
    DarkModeSetConfiguredColors(text, background, fallbackText, fallbackBackground);
    DarkModeConfigureDialogColors(DarkModeEnsureReadableForeground(text, background), background, s.BackgroundBrush);
    DarkModeSetEnabled(dark);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = VhdWndProc;
    wc.hInstance = DLLInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "HyperVMVhdDialog";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    UINT initialDpi = WinLibDPIGetWindowDPI(parent);
    int width = MulDiv(380, initialDpi, USER_DEFAULT_SCREEN_DPI);
    int height = MulDiv(create ? 440 : 157, initialDpi, USER_DEFAULT_SCREEN_DPI);
    RECT windowRect = {0, 0, width, height};
    typedef BOOL(WINAPI * FAdjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    FAdjustWindowRectExForDpi adjustForDpi =
        user32 != nullptr
            ? reinterpret_cast<FAdjustWindowRectExForDpi>(
                  GetProcAddress(user32, "AdjustWindowRectExForDpi"))
            : nullptr;
    if (adjustForDpi != nullptr)
        adjustForDpi(&windowRect, WS_CAPTION | WS_SYSMENU | WS_POPUP, FALSE,
                     WS_EX_DLGMODALFRAME, initialDpi);
    else
        AdjustWindowRectEx(&windowRect, WS_CAPTION | WS_SYSMENU | WS_POPUP,
                           FALSE, WS_EX_DLGMODALFRAME);
    int titleId = create ? IDS_CREATE_VHD_DLG_TITLE : IDS_ATTACH_VHD_DLG_TITLE;
    std::wstring title = WStr(titleId);
    std::string titleA = AStr(titleId);
    if (title.length() <= 1)
    {
        title = create ? L"Create and Attach Virtual Hard Disk" : L"Attach Virtual Hard Disk";
        titleA = create ? "Create and Attach Virtual Hard Disk" : "Attach Virtual Hard Disk";
    }
    HWND hwnd = nullptr;
    {
        // This window bypasses WinLib's normal top-level creation path. CLR
        // calls made by the plugin can leave the callback thread in a legacy
        // DPI context, which would permanently make this HWND SystemAware and
        // keep its controls at 96 DPI. Capture PMv2 explicitly at creation.
        CWinLibDPIContext dpiContext;
        hwnd = CreateWindowExA(WS_EX_DLGMODALFRAME, wc.lpszClassName, titleA.c_str(),
                               WS_CAPTION | WS_SYSMENU | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                               windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
                               parent, nullptr, DLLInstance, &s);
    }
    SetWindowTextW(hwnd, title.c_str());
    if (!titleA.empty())
    {
        SetWindowTextA(hwnd, titleA.c_str());
    }
    s.Window = hwnd;
    s.Dpi = WinLibDPIGetWindowDPI(hwnd);
    s.Font = WinLibDPICreateMessageFont(hwnd);
    HFONT font = s.Font != nullptr ? s.Font : (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    AddControl(hwnd, L"STATIC", WStr(create ? IDS_VHD_CREATE_INTRO : IDS_VHD_ATTACH_INTRO), 0, 12, 12, 350, 18, -1, font);
    AddControl(hwnd, L"STATIC", WStr(IDS_VHD_LOCATION), 0, 12, 43, 100, 18, -1, font);
    s.Path = AddControl(hwnd, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 12, 61, 275, 21, IDC_VHD_PATH, font);
    AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_BROWSE), WS_TABSTOP, 295, 59, 74, 24, IDC_VHD_BROWSE, font);
    if (create)
    {
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_SIZE), 0, 12, 105, 170, 18, -1, font);
        s.Size = AddControl(hwnd, L"EDIT", L"64", WS_TABSTOP | WS_BORDER | ES_NUMBER, 236, 98, 74, 21, IDC_VHD_SIZE, font);
        s.Unit = AddControl(hwnd, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 315, 98, 54, 200, IDC_VHD_UNIT, font);
        SendMessageW(s.Unit, CB_ADDSTRING, 0, (LPARAM)L"MB"); SendMessageW(s.Unit, CB_ADDSTRING, 0, (LPARAM)L"GB"); SendMessageW(s.Unit, CB_ADDSTRING, 0, (LPARAM)L"TB"); SendMessage(s.Unit, CB_SETCURSEL, 0, 0);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_FORMAT), BS_GROUPBOX, 12, 130, 356, 137, -1, font);
        AddControl(hwnd, L"BUTTON", L"VHD", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 21, 146, 80, 18, IDC_VHD_FORMAT_VHD, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_VHD_HELP), 0, 32, 166, 328, 18, -1, font);
        AddControl(hwnd, L"BUTTON", L"VHDX", BS_AUTORADIOBUTTON | WS_TABSTOP, 21, 185, 80, 18, IDC_VHD_FORMAT_VHDX, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_VHDX_HELP), 0, 32, 205, 328, 51, -1, font);
        CheckRadioButton(hwnd, IDC_VHD_FORMAT_VHD, IDC_VHD_FORMAT_VHDX, IDC_VHD_FORMAT_VHD);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_TYPE), BS_GROUPBOX, 12, 276, 356, 122, -1, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_FIXED), BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 21, 293, 220, 18, IDC_VHD_TYPE_FIXED, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_FIXED_HELP), 0, 32, 312, 328, 30, -1, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_DYNAMIC), BS_AUTORADIOBUTTON | WS_TABSTOP, 21, 343, 220, 18, IDC_VHD_TYPE_DYNAMIC, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_DYNAMIC_HELP), 0, 32, 363, 328, 30, -1, font);
        CheckRadioButton(hwnd, IDC_VHD_TYPE_FIXED, IDC_VHD_TYPE_DYNAMIC, IDC_VHD_TYPE_FIXED);
        s.Ok = AddControl(hwnd, L"BUTTON", WStr(IDS_OK), WS_TABSTOP | BS_DEFPUSHBUTTON, 215, 407, 72, 24, IDOK, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_CANCEL), WS_TABSTOP, 295, 407, 72, 24, IDCANCEL, font);
    }
    else
    {
        s.ReadOnly = AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_READONLY), BS_AUTOCHECKBOX | WS_TABSTOP, 12, 88, 120, 18, IDC_VHD_READONLY, font);
        s.Ok = AddControl(hwnd, L"BUTTON", WStr(IDS_OK), WS_TABSTOP | BS_DEFPUSHBUTTON, 215, 121, 72, 24, IDOK, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_CANCEL), WS_TABSTOP, 295, 121, 72, 24, IDCANCEL, font);
    }
    EnableWindow(s.Ok, FALSE);
    if (dark)
    {
        DarkModeApplyWindow(hwnd);
        DarkModeApplyTree(hwnd);
        DarkModeRefreshTitleBar(hwnd);
    }
    SalamanderGeneral->MultiMonCenterWindow(hwnd, parent, TRUE);
    EnableWindow(parent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    SetWindowTextW(hwnd, title.c_str());
    if (!titleA.empty())
    {
        SetWindowTextA(hwnd, titleA.c_str());
    }
    MSG msg;
    while (!s.Done && GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(hwnd, &msg)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
    EnableWindow(parent, TRUE);
    if (s.BackgroundBrush != nullptr)
    {
        DeleteObject(s.BackgroundBrush);
    }
    SetActiveWindow(parent);
    return s.Result;
}
} // namespace

bool ShowCreateVhdDialog(HWND parent) { return RunDialog(parent, true); }
bool ShowAttachVhdDialog(HWND parent) { return RunDialog(parent, false); }
