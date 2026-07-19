// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "hypervm.h"
#include "managed_bridge.h"
#include "../../darkmode.h"

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
};

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
    LoadStringW(HLanguage, id, buffer, 1024);
    return buffer;
}

void SetFont(HWND child, HFONT font)
{
    SendMessage(child, WM_SETFONT, (WPARAM)font, TRUE);
}

HWND AddControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int x, int y, int w, int h, int id, HFONT font)
{
    HWND h = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, parent, (HMENU)(INT_PTR)id, DLLInstance, nullptr);
    SetFont(h, font);
    return h;
}

void EnableOk(CVhdDialogState* s)
{
    wchar_t path[2];
    EnableWindow(s->Ok, GetWindowTextW(s->Path, path, 2) > 0);
}

bool Browse(HWND owner, bool save, std::wstring& path)
{
    std::vector<wchar_t> file(SAL_MAX_PATH, L'\0');
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
    std::vector<wchar_t> path(SAL_MAX_PATH, L'\0');
    wchar_t sizeText[64] = L"";
    GetWindowTextW(s->Path, path.data(), (int)path.size());
    GetWindowTextW(s->Size, sizeText, _countof(sizeText));
    unsigned long long value = _wtoi64(sizeText);
    int unit = (int)SendMessage(s->Unit, CB_GETCURSEL, 0, 0);
    unsigned long long multiplier = unit == 2 ? 1024ULL * 1024 * 1024 * 1024 : unit == 1 ? 1024ULL * 1024 * 1024 : 1024ULL * 1024;
    const wchar_t* format = IsDlgButtonChecked(s->Window, IDC_VHD_FORMAT_VHD) == BST_CHECKED ? L"VHD" : L"VHDX";
    const wchar_t* type = IsDlgButtonChecked(s->Window, IDC_VHD_TYPE_FIXED) == BST_CHECKED ? L"Fixed" : L"Dynamic";
    std::vector<wchar_t> payload(2 * SAL_MAX_PATH, L'\0');
    StringCchPrintfW(payload.data(), payload.size(), L"CreateVhd|%s|%llu|%s|%s", path.data(), value * multiplier, format, type);
    if (ManagedBridge_RunMenuCommandW(s->Parent, payload.data())) Finish(s, true);
}

void ExecuteAttach(CVhdDialogState* s)
{
    std::vector<wchar_t> path(SAL_MAX_PATH, L'\0');
    GetWindowTextW(s->Path, path.data(), (int)path.size());
    std::vector<wchar_t> payload(2 * SAL_MAX_PATH, L'\0');
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
    default:
        LRESULT result;
        if (DarkModeHandleCtlColor(msg, wp, lp, result)) return result;
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

bool RunDialog(HWND parent, bool create)
{
    CVhdDialogState s = {0};
    s.Create = create;
    s.Parent = parent;
    bool dark = SalamanderGeneral->GetCurrentColor(11) < 0x808080;
    DarkModeSetEnabled(dark);

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = VhdWndProc;
    wc.hInstance = DLLInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"HyperVMVhdDialog";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    int width = 380;
    int height = create ? 470 : 148;
    RECT rc = {0, 0, width, height};
    AdjustWindowRectEx(&rc, WS_CAPTION | WS_SYSMENU | WS_POPUP, FALSE, WS_EX_DLGMODALFRAME);
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, wc.lpszClassName, WStr(create ? IDS_VHD_CREATE_TITLE : IDS_VHD_ATTACH_TITLE),
                                WS_CAPTION | WS_SYSMENU | WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top, parent, nullptr, DLLInstance, &s);
    s.Window = hwnd;
    HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    AddControl(hwnd, L"STATIC", WStr(create ? IDS_VHD_CREATE_INTRO : IDS_VHD_ATTACH_INTRO), 0, 12, 12, 350, 18, -1, font);
    AddControl(hwnd, L"STATIC", WStr(IDS_VHD_LOCATION), 0, 12, 74, 100, 18, -1, font);
    s.Path = AddControl(hwnd, L"EDIT", L"", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 12, 94, 275, 21, IDC_VHD_PATH, font);
    AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_BROWSE), WS_TABSTOP, 295, 92, 74, 24, IDC_VHD_BROWSE, font);
    if (create)
    {
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_SIZE), 0, 12, 138, 170, 18, -1, font);
        s.Size = AddControl(hwnd, L"EDIT", L"64", WS_TABSTOP | WS_BORDER | ES_NUMBER, 236, 131, 74, 21, IDC_VHD_SIZE, font);
        s.Unit = AddControl(hwnd, L"COMBOBOX", L"", WS_TABSTOP | CBS_DROPDOWNLIST, 315, 130, 54, 200, IDC_VHD_UNIT, font);
        SendMessageW(s.Unit, CB_ADDSTRING, 0, (LPARAM)L"MB"); SendMessageW(s.Unit, CB_ADDSTRING, 0, (LPARAM)L"GB"); SendMessageW(s.Unit, CB_ADDSTRING, 0, (LPARAM)L"TB"); SendMessage(s.Unit, CB_SETCURSEL, 0, 0);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_FORMAT), BS_GROUPBOX, 12, 164, 356, 134, -1, font);
        AddControl(hwnd, L"BUTTON", L"VHD", BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 21, 181, 80, 18, IDC_VHD_FORMAT_VHD, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_VHD_HELP), 0, 33, 202, 315, 18, -1, font);
        AddControl(hwnd, L"BUTTON", L"VHDX", BS_AUTORADIOBUTTON | WS_TABSTOP, 21, 218, 80, 18, IDC_VHD_FORMAT_VHDX, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_VHDX_HELP), 0, 33, 239, 315, 48, -1, font);
        CheckRadioButton(hwnd, IDC_VHD_FORMAT_VHD, IDC_VHD_FORMAT_VHDX, IDC_VHD_FORMAT_VHD);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_TYPE), BS_GROUPBOX, 12, 306, 356, 118, -1, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_FIXED), BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 21, 323, 220, 18, IDC_VHD_TYPE_FIXED, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_FIXED_HELP), 0, 33, 344, 315, 30, -1, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_DYNAMIC), BS_AUTORADIOBUTTON | WS_TABSTOP, 21, 380, 220, 18, IDC_VHD_TYPE_DYNAMIC, font);
        AddControl(hwnd, L"STATIC", WStr(IDS_VHD_DYNAMIC_HELP), 0, 33, 401, 315, 30, -1, font);
        CheckRadioButton(hwnd, IDC_VHD_TYPE_FIXED, IDC_VHD_TYPE_DYNAMIC, IDC_VHD_TYPE_FIXED);
        s.Ok = AddControl(hwnd, L"BUTTON", WStr(IDS_OK), WS_TABSTOP | BS_DEFPUSHBUTTON, 215, 438, 72, 24, IDOK, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_CANCEL), WS_TABSTOP, 295, 438, 72, 24, IDCANCEL, font);
    }
    else
    {
        s.ReadOnly = AddControl(hwnd, L"BUTTON", WStr(IDS_VHD_READONLY), BS_AUTOCHECKBOX | WS_TABSTOP, 12, 121, 120, 18, IDC_VHD_READONLY, font);
        s.Ok = AddControl(hwnd, L"BUTTON", WStr(IDS_OK), WS_TABSTOP | BS_DEFPUSHBUTTON, 215, 116, 72, 24, IDOK, font);
        AddControl(hwnd, L"BUTTON", WStr(IDS_CANCEL), WS_TABSTOP, 295, 116, 72, 24, IDCANCEL, font);
    }
    EnableWindow(s.Ok, FALSE);
    DarkModeApplyTree(hwnd);
    SalamanderGeneral->MultiMonCenterWindow(hwnd, parent, TRUE);
    EnableWindow(parent, FALSE);
    ShowWindow(hwnd, SW_SHOW);
    MSG msg;
    while (!s.Done && GetMessage(&msg, nullptr, 0, 0))
    {
        if (!IsDialogMessage(hwnd, &msg)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    }
    EnableWindow(parent, TRUE);
    SetActiveWindow(parent);
    return s.Result;
}
} // namespace

bool ShowCreateVhdDialog(HWND parent) { return RunDialog(parent, true); }
bool ShowAttachVhdDialog(HWND parent) { return RunDialog(parent, false); }
