// SPDX-License-Identifier: GPL-2.0-or-later
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "../../../salamatrix-sdk/native-ui-runtime/salamatrix_ui_layout.h"

struct Control
{
    std::wstring Kind, Id, Text;
    int X = 0, Y = 0, Width = 0, Height = 0;
    unsigned Style = 0;
    bool Checked = false;
};

struct DialogModel
{
    std::wstring Title;
    int Width = 320, Height = 180;
    std::vector<Control> Controls;
};

static std::wstring Utf8ToWide(const std::string& value)
{
    if (value.empty()) return std::wstring();
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), NULL, 0);
    if (size <= 0) return std::wstring();
    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), &result[0], size);
    return result;
}

static bool Hex(const std::string& value, std::wstring& result)
{
    if ((value.size() & 1) != 0) return false;
    std::string decoded;
    decoded.reserve(value.size() / 2);
    for (size_t i = 0; i < value.size(); i += 2)
    {
        char* end = NULL;
        const std::string pair = value.substr(i, 2);
        const long byte = strtol(pair.c_str(), &end, 16);
        if (end == NULL || *end != '\0' || byte < 0 || byte > 255) return false;
        decoded.push_back(static_cast<char>(byte));
    }
    result = Utf8ToWide(decoded);
    return decoded.empty() || !result.empty();
}

static std::vector<std::string> Split(const std::string& line)
{
    std::vector<std::string> fields;
    size_t start = 0;
    for (;;)
    {
        const size_t tab = line.find('\t', start);
        fields.push_back(line.substr(start, tab == std::string::npos ? tab : tab - start));
        if (tab == std::string::npos) return fields;
        start = tab + 1;
    }
}

static bool ReadLine(std::ifstream& input, std::string& line)
{
    if (!std::getline(input, line)) return false;
    if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
    return true;
}

static bool LoadModel(const wchar_t* path, DialogModel& model)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::string line;
    if (!ReadLine(input, line) || line != "SMXPREVIEW1") return false;
    if (!ReadLine(input, line) || !Hex(line, model.Title)) return false;
    if (!ReadLine(input, line)) return false;
    std::vector<std::string> size = Split(line);
    if (size.size() != 2) return false;
    model.Width = atoi(size[0].c_str()); model.Height = atoi(size[1].c_str());
    while (ReadLine(input, line))
    {
        const std::vector<std::string> field = Split(line);
        if (field.size() != 10) return false;
        Control control;
        if (!Hex(field[0], control.Kind) || !Hex(field[1], control.Id) || !Hex(field[2], control.Text)) return false;
        control.X = atoi(field[3].c_str()); control.Y = atoi(field[4].c_str());
        control.Width = atoi(field[5].c_str()); control.Height = atoi(field[6].c_str());
        control.Style = static_cast<unsigned>(strtoul(field[7].c_str(), NULL, 10));
        control.Checked = field[8] == "1";
        model.Controls.push_back(control);
    }
    return model.Width > 0 && model.Height > 0;
}

static int DluX(int value) { return MulDiv(value, LOWORD(GetDialogBaseUnits()), 4); }
static int DluY(int value) { return MulDiv(value, HIWORD(GetDialogBaseUnits()), 8); }

static HWND AddWindow(HWND parent, const wchar_t* cls, const std::wstring& text, DWORD style, DWORD exStyle, const Control& control)
{
    HWND window = CreateWindowExW(exStyle, cls, text.c_str(), WS_CHILD | WS_VISIBLE | style,
        DluX(control.X), DluY(control.Y), DluX(control.Width), DluY(control.Height), parent, NULL, GetModuleHandleW(NULL), NULL);
    if (window != NULL) SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    return window;
}

static void CreateControl(HWND parent, const Control& control)
{
    HWND window = NULL;
    if (control.Kind == L"textbox") window = AddWindow(parent, L"EDIT", control.Text, WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, control);
    else if (control.Kind == L"checkbox") window = AddWindow(parent, L"BUTTON", control.Text, WS_TABSTOP | BS_AUTOCHECKBOX, 0, control);
    else if (control.Kind == L"radio") window = AddWindow(parent, L"BUTTON", control.Text, WS_TABSTOP | BS_AUTORADIOBUTTON, 0, control);
    else if (control.Kind == L"button" || control.Kind == L"arrowbutton" || control.Kind == L"textarrowbutton" || control.Kind == L"colorarrowbutton") window = AddWindow(parent, L"BUTTON", control.Text, WS_TABSTOP | BS_PUSHBUTTON | control.Style, 0, control);
    else if (control.Kind == L"combobox") window = AddWindow(parent, WC_COMBOBOXW, control.Text, WS_TABSTOP | CBS_DROPDOWNLIST, 0, control);
    else if (control.Kind == L"listview") window = AddWindow(parent, WC_LISTVIEWW, L"", WS_TABSTOP | LVS_REPORT | LVS_SHOWSELALWAYS, WS_EX_CLIENTEDGE, control);
    else if (control.Kind == L"treeview") window = AddWindow(parent, WC_TREEVIEWW, L"", WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS, WS_EX_CLIENTEDGE, control);
    else if (control.Kind == L"tabcontrol") window = AddWindow(parent, WC_TABCONTROLW, L"", WS_TABSTOP, 0, control);
    else if (control.Kind == L"progressbar") window = AddWindow(parent, PROGRESS_CLASSW, L"", 0, 0, control);
    else if (control.Kind == L"groupbox") window = AddWindow(parent, L"BUTTON", control.Text, BS_GROUPBOX, 0, control);
    else if (control.Kind == L"hyperlink") window = AddWindow(parent, WC_LINK, (L"<a>" + control.Text + L"</a>"), WS_TABSTOP, 0, control);
    else if (control.Kind == L"folderpicker" || control.Kind == L"filepicker")
    {
        const Salamatrix::UI::FilePickerLayoutMetrics metrics = Salamatrix::UI::ComputeFilePickerLayout(control.X, control.Width);
        Control edit = control; edit.Width = metrics.EditWidth;
        AddWindow(parent, L"EDIT", control.Text, WS_TABSTOP | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, edit);
        Control browse = control; browse.X = metrics.BrowseX; browse.Width = metrics.BrowseWidth;
        AddWindow(parent, L"BUTTON", L"...", WS_TABSTOP | BS_PUSHBUTTON, 0, browse);
    }
    else window = AddWindow(parent, L"STATIC", control.Text, control.Kind == L"statictext" ? control.Style : SS_LEFT, 0, control);
    if (window != NULL && control.Checked) SendMessageW(window, BM_SETCHECK, BST_CHECKED, 0);
    if (window != NULL && control.Kind == L"progressbar") SendMessageW(window, PBM_SETPOS, 50, 0);
}

static DialogModel g_model;
static LRESULT CALLBACK PreviewProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE)
    {
        for (size_t i = 0; i < g_model.Controls.size(); ++i) CreateControl(hwnd, g_model.Controls[i]);
        return 0;
    }
    if (message == WM_CLOSE) { DestroyWindow(hwnd); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
    int argumentCount = 0;
    wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &argumentCount);
    const bool validateOnly = arguments != NULL && argumentCount == 3 && wcscmp(arguments[1], L"--validate") == 0;
    const wchar_t* modelPath = arguments == NULL ? NULL : validateOnly ? arguments[2] : argumentCount == 2 ? arguments[1] : NULL;
    if (arguments == NULL || modelPath == NULL || !LoadModel(modelPath, g_model))
    {
        if (arguments != NULL) LocalFree(arguments);
        MessageBoxW(NULL, L"The Salamatrix Studio preview model is invalid.", L"Salamatrix Studio", MB_OK | MB_ICONERROR);
        return 2;
    }
    LocalFree(arguments);
    if (validateOnly) return 0;
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_LINK_CLASS };
    InitCommonControlsEx(&controls);
    WNDCLASSEXW cls = { sizeof(cls) };
    cls.hInstance = instance; cls.lpfnWndProc = PreviewProc; cls.lpszClassName = L"SalamatrixStudioPreview";
    cls.hCursor = LoadCursorW(NULL, IDC_ARROW); cls.hIcon = LoadIconW(NULL, IDI_APPLICATION); cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
    if (!RegisterClassExW(&cls)) return 3;
    RECT rect = { 0, 0, DluX(g_model.Width), DluY(g_model.Height) };
    AdjustWindowRectEx(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, WS_EX_DLGMODALFRAME);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME, cls.lpszClassName, g_model.Title.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, instance, NULL);
    if (window == NULL) return 4;
    ShowWindow(window, show); UpdateWindow(window);
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0) { TranslateMessage(&message); DispatchMessageW(&message); }
    return 0;
}
