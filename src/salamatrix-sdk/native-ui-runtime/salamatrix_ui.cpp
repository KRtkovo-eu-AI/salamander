// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <CommDlg.h>
#include <shlobj.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#include "salamatrix_ui.h"
#include "salamatrix_ui_host.h"
#include "salamatrix_ui_layout.h"

#include <string>
#include <vector>

namespace Salamatrix
{
namespace UI
{
static INativeDialogHost* NativeDialogHost = NULL;

void WINAPI SetNativeDialogHost(INativeDialogHost* host)
{
    NativeDialogHost = host;
}

INativeDialogHost* WINAPI GetNativeDialogHost()
{
    return NativeDialogHost;
}

namespace
{
static const UINT WM_SALAMATRIX_APPLY_DARK_SCROLLBARS = WM_APP + 0x3A1;
static const UINT WM_SALAMATRIX_SPLITTER_MOVED = WM_APP + 0x3A2;
static std::vector<NativeDialog*> OpenNativeDialogs;
static std::vector<HWND> OpenNotificationWindows;
static BOOL ClosingAllNativeDialogs = FALSE;

static void RegisterNativeDialog(NativeDialog* dialog)
{
    if (dialog != NULL)
        OpenNativeDialogs.push_back(dialog);
}

static void UnregisterNativeDialog(NativeDialog* dialog)
{
    for (size_t index = 0; index < OpenNativeDialogs.size(); ++index)
    {
        if (OpenNativeDialogs[index] == dialog)
        {
            OpenNativeDialogs.erase(OpenNativeDialogs.begin() + index);
            return;
        }
    }
}

static void RegisterNotificationWindow(HWND window)
{
    if (window != NULL)
        OpenNotificationWindows.push_back(window);
}

static void UnregisterNotificationWindow(HWND window)
{
    for (size_t index = 0; index < OpenNotificationWindows.size(); ++index)
    {
        if (OpenNotificationWindows[index] == window)
        {
            OpenNotificationWindows.erase(
                OpenNotificationWindows.begin() + index);
            return;
        }
    }
}

static BOOL Utf8ToWide(const char* value, std::wstring& result);

static BOOL BuildFilePickerFilter(
    const char* filter,
    std::vector<wchar_t>& wideFilter)
{
    const char* source = filter != NULL && filter[0] != '\0'
                             ? filter
                             : "All files (*.*)|*.*";
    std::wstring wide;
    if (!Utf8ToWide(source, wide))
        return FALSE;
    for (size_t index = 0; index < wide.size(); ++index)
    {
        if (wide[index] == L'|')
            wide[index] = L'\0';
    }
    if (wide.empty() || wide[wide.size() - 1] != L'\0')
        wide.push_back(L'\0');
    wide.push_back(L'\0');
    wideFilter.assign(wide.begin(), wide.end());
    return TRUE;
}

static BOOL Utf8ToWide(const char* value, std::wstring& result)
{
    result.clear();
    if (value == NULL)
        return TRUE;
    int length = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, NULL, 0);
    if (length <= 0)
        return FALSE;
    std::vector<wchar_t> buffer(static_cast<size_t>(length));
    if (MultiByteToWideChar(
            CP_UTF8, MB_ERR_INVALID_CHARS, value, -1,
            &buffer[0], length) <= 0)
        return FALSE;
    result.assign(&buffer[0]);
    return TRUE;
}

static BOOL WideToUtf8(const wchar_t* value, std::string& result)
{
    result.clear();
    if (value == NULL)
        return TRUE;
    int length = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, NULL, 0, NULL, NULL);
    if (length <= 0)
        return FALSE;
    std::vector<char> buffer(static_cast<size_t>(length));
    if (WideCharToMultiByte(
            CP_UTF8, WC_ERR_INVALID_CHARS, value, -1,
            &buffer[0], length, NULL, NULL) <= 0)
        return FALSE;
    result.assign(&buffer[0]);
    return TRUE;
}

static void ApplyNativeDialogDarkMode(HWND hwnd)
{
    INativeDialogHost* host = GetNativeDialogHost();
    if (hwnd != NULL && host != NULL)
        host->ApplyTheme(hwnd);
}

static int ShowHostAwareMessageBox(
    HWND parent,
    const wchar_t* message,
    const wchar_t* title,
    UINT flags)
{
    std::string messageUtf8;
    std::string titleUtf8;
    if (!WideToUtf8(message, messageUtf8) || !WideToUtf8(title, titleUtf8))
        return 0;
    INativeDialogHost* host = GetNativeDialogHost();
    return host != NULL
               ? host->ShowUtf8MessageBox(
                     parent, messageUtf8.c_str(), titleUtf8.c_str(), flags)
               : MessageBoxW(parent, message, title, flags);
}

static BOOL PickFolderPath(HWND parent, const char* title, std::string& result)
{
    result.clear();
    std::wstring titleWide;
    if (!Utf8ToWide(title != NULL ? title : "Select folder", titleWide))
        return FALSE;
    BROWSEINFOW browse;
    memset(&browse, 0, sizeof(browse));
    browse.hwndOwner = parent;
    browse.lpszTitle = titleWide.c_str();
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST item = SHBrowseForFolderW(&browse);
    if (item == NULL)
        return FALSE;
    PWSTR path = NULL;
    HRESULT status = SHGetNameFromIDList(item, SIGDN_FILESYSPATH, &path);
    CoTaskMemFree(item);
    if (FAILED(status) || path == NULL)
        return FALSE;
    BOOL converted = WideToUtf8(path, result);
    CoTaskMemFree(path);
    return converted && !result.empty();
}

static BOOL PickEditableFilePath(
    HWND parent,
    const char* title,
    const char* initialPath,
    const char* fileFilter,
    BOOL fileSave,
    std::string& result)
{
    result.clear();
    std::wstring titleWide;
    std::wstring initialWide;
    if (!Utf8ToWide(title != NULL ? title : "Select file", titleWide) ||
        !Utf8ToWide(initialPath != NULL ? initialPath : "", initialWide))
        return FALSE;

    // Keep the caller-owned path storage heap-backed and sized for the
    // Win32 wide-path limit instead of introducing a MAX_PATH dependency.
    const size_t FileBufferCapacity = 32768;
    if (initialWide.size() >= FileBufferCapacity)
        return FALSE;
    std::vector<wchar_t> path(FileBufferCapacity, L'\0');
    if (!initialWide.empty())
        memcpy(&path[0], initialWide.c_str(),
               initialWide.size() * sizeof(wchar_t));

    std::vector<wchar_t> filter;
    if (!BuildFilePickerFilter(fileFilter, filter) ||
        filter.empty())
        return FALSE;
    OPENFILENAMEW dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = parent;
    dialog.lpstrFilter = &filter[0];
    dialog.lpstrFile = &path[0];
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrTitle = titleWide.c_str();
    dialog.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    if (fileSave)
    {
        dialog.Flags |= OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameW(&dialog))
            return FALSE;
    }
    else if (!GetOpenFileNameW(&dialog))
    {
        return FALSE;
    }
    return WideToUtf8(&path[0], result) && !result.empty();
}

struct NotificationData
{
    std::wstring Title;
    std::wstring Message;
};

static INIT_ONCE NotificationClassInit = INIT_ONCE_STATIC_INIT;
static HINSTANCE NotificationInstance = NULL;
static const wchar_t NotificationClassName[] =
    L"OpenSalamander.Salamatrix.Notification";

static LRESULT CALLBACK NotificationWindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NotificationData* data = reinterpret_cast<NotificationData*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    switch (message)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        data = create != NULL
                   ? static_cast<NotificationData*>(create->lpCreateParams)
                   : NULL;
        SetWindowLongPtrW(
            window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        return data != NULL ? TRUE : FALSE;
    }
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_TIMER:
        if (wParam == 1)
            DestroyWindow(window);
        return 0;
    case WM_LBUTTONDOWN:
        DestroyWindow(window);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        RECT client;
        GetClientRect(window, &client);
        HBRUSH background = CreateSolidBrush(
            GetSysColor(COLOR_WINDOW));
        FillRect(dc, &client, background);
        DeleteObject(background);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
        RECT titleRect = client;
        titleRect.left += 14;
        titleRect.top += 10;
        titleRect.right -= 14;
        titleRect.bottom = titleRect.top + 24;
        RECT messageRect = titleRect;
        messageRect.top += 28;
        messageRect.bottom = client.bottom - 10;
        if (data != NULL)
        {
            HFONT font = reinterpret_cast<HFONT>(GetStockObject(
                DEFAULT_GUI_FONT));
            HGDIOBJ oldFont = SelectObject(dc, font);
            DrawTextW(
                dc,
                data->Title.c_str(),
                -1,
                &titleRect,
                DT_SINGLELINE | DT_END_ELLIPSIS);
            DrawTextW(
                dc,
                data->Message.c_str(),
                -1,
                &messageRect,
                DT_WORDBREAK | DT_END_ELLIPSIS);
            SelectObject(dc, oldFont);
        }
        EndPaint(window, &paint);
        return 0;
    }
    case WM_NCDESTROY:
        UnregisterNotificationWindow(window);
        delete data;
        SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        break;
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

static BOOL CALLBACK RegisterNotificationClass(
    PINIT_ONCE,
    PVOID,
    PVOID*)
{
    HINSTANCE module = NULL;
    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ShowNativeNotification),
            &module))
        return FALSE;
    NotificationInstance = module;
    WNDCLASSEXW windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = NotificationInstance;
    windowClass.lpfnWndProc = NotificationWindowProc;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(
        COLOR_WINDOW + 1);
    windowClass.lpszClassName = NotificationClassName;
    ATOM registered = RegisterClassExW(&windowClass);
    return registered != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

static BOOL CALLBACK CountNotificationWindows(HWND window, LPARAM data)
{
    wchar_t className[128];
    if (GetClassNameW(window, className, _countof(className)) > 0 &&
        wcscmp(className, NotificationClassName) == 0)
        ++*reinterpret_cast<int*>(data);
    return TRUE;
}

static void AppendWord(std::vector<BYTE>& bytes, WORD value)
{
    bytes.push_back(static_cast<BYTE>(value & 0xff));
    bytes.push_back(static_cast<BYTE>((value >> 8) & 0xff));
}

static void AppendString(std::vector<BYTE>& bytes, const std::wstring& value)
{
    for (size_t index = 0; index < value.size(); ++index)
        AppendWord(bytes, static_cast<WORD>(value[index]));
    AppendWord(bytes, 0);
}

static void AlignTemplate(std::vector<BYTE>& bytes)
{
    while ((bytes.size() & 3) != 0)
        bytes.push_back(0);
}

static void AppendItem(
    std::vector<BYTE>& bytes,
    short x,
    short y,
    short width,
    short height,
    WORD id,
    DWORD style,
    WORD classOrdinal,
    const std::wstring& text,
    const wchar_t* className)
{
    AlignTemplate(bytes);
    size_t offset = bytes.size();
    bytes.resize(offset + sizeof(DLGITEMTEMPLATE), 0);
    DLGITEMTEMPLATE* item =
        reinterpret_cast<DLGITEMTEMPLATE*>(&bytes[offset]);
    item->x = x;
    item->y = y;
    item->cx = width;
    item->cy = height;
    item->id = id;
    item->style = style;
    item->dwExtendedStyle = 0;
    if (className != NULL)
        AppendString(bytes, std::wstring(className));
    else
    {
        AppendWord(bytes, 0xffff);
        AppendWord(bytes, classOrdinal);
    }
    AppendString(bytes, text);
    AppendWord(bytes, 0);
}

static short ClampDialogCoordinate(int value)
{
    if (value < -32768)
        return -32768;
    if (value > 32767)
        return 32767;
    return static_cast<short>(value);
}

static void CopyEventText(
    char* destination,
    size_t capacity,
    const std::string& value)
{
    if (destination == NULL || capacity == 0)
        return;
    size_t length = value.size();
    if (length >= capacity)
        length = capacity - 1;
    // DialogEvent keeps its historical bounded UTF-8 field. Never cut a
    // multibyte path value in the middle of a code point when it is copied
    // into that compatibility payload.
    while (length > 0 && length < value.size() &&
           (static_cast<unsigned char>(value[length]) & 0xc0) == 0x80)
        --length;
    if (length != 0)
        memcpy(destination, value.data(), length);
    destination[length] = '\0';
}
} // namespace

BOOL WINAPI ShowNativeNotification(
    HWND parent,
    const char* title,
    const char* message,
    DWORD timeoutMs)
{
    std::wstring titleWide;
    std::wstring messageWide;
    if (!Utf8ToWide(
            title != NULL && title[0] != '\0' ? title : "Salamander",
            titleWide) ||
        !Utf8ToWide(message != NULL ? message : "", messageWide))
        return FALSE;
    if (!InitOnceExecuteOnce(
            &NotificationClassInit,
            RegisterNotificationClass,
            NULL,
            NULL) ||
        NotificationInstance == NULL)
        return FALSE;

    NotificationData* data = new NotificationData;
    if (data == NULL)
        return FALSE;
    data->Title = titleWide;
    data->Message = messageWide;

    HWND owner = parent != NULL && IsWindow(parent)
                     ? parent
                     : GetForegroundWindow();
    HMONITOR monitor = MonitorFromWindow(
        owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo;
    memset(&monitorInfo, 0, sizeof(monitorInfo));
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (monitor == NULL || !GetMonitorInfoW(monitor, &monitorInfo))
    {
        delete data;
        return FALSE;
    }

    const int width = 400;
    const int height = 112;
    const int margin = 16;
    int existing = 0;
    EnumWindows(CountNotificationWindows, reinterpret_cast<LPARAM>(&existing));
    RECT work = monitorInfo.rcWork;
    int x = work.right - width - margin;
    int y = work.bottom - height - margin - existing * (height + 8);
    if (y < work.top + margin)
        y = work.top + margin;

    HWND window = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        NotificationClassName,
        titleWide.c_str(),
        WS_POPUP | WS_BORDER,
        x,
        y,
        width,
        height,
        owner,
        NULL,
        NotificationInstance,
        data);
    if (window == NULL)
    {
        delete data;
        return FALSE;
    }
    RegisterNotificationWindow(window);
    DWORD duration = timeoutMs == 0 ? 5000 : timeoutMs;
    if (duration > 600000)
        duration = 600000;
    if (SetTimer(window, 1, duration, NULL) == 0)
    {
        DestroyWindow(window);
        return FALSE;
    }
    ShowWindow(window, SW_SHOWNOACTIVATE);
    UpdateWindow(window);
    return TRUE;
}

struct NativeDialog::Impl
{
    struct Control : public IControl
    {
        ControlKind Kind;
        std::string Id;
        std::string Text;
        BOOL ReadOnly;
        BOOL Checked;
        int DialogResult;
        BOOL KeepOpen;
        BOOL Multiline;
        std::string FileFilter;
        BOOL FileSave;
        std::string AccessibleName;
        std::string AccessibleDescription;
        std::wstring AccessibleTooltipText;
        BOOL Enabled;
        HWND WindowHandle;
        HWND BrowseWindowHandle;
        WORD BrowseNumericId;
        BOOL Required;
        std::string ValidationMessage;
        BOOL HasBounds;
        int X;
        int Y;
        int Width;
        int Height;
        int SelectedIndex;
        WORD NumericId;
        std::vector<std::string> Items;
        std::vector<int> ItemParents;
        std::vector<std::string> ColumnTitles;
        std::vector<int> ColumnWidths;
        DWORD StyleFlags;
        char PathSeparator;
        std::string ToolTipText;
        std::string ActionTarget;
        std::string ActionHint;
        WORD ActionCommand;
        int ProgressValue;
        ULONGLONG ProgressCurrent;
        ULONGLONG ProgressTotal;
        BOOL HasProgressValues;
        std::string ProgressText;
        DWORD IndeterminateDuration;
        DWORD IndeterminateInterval;
        COLORREF TextColor;
        COLORREF BackgroundColor;
        std::string ToolbarAlignControlId;
        DWORD ToolbarButtonMask;
        CGUIStaticTextAbstract* StaticText;
        CGUIHyperLinkAbstract* HyperLink;
        CGUIProgressBarAbstract* ProgressBar;
        CGUIButtonAbstract* TextArrowButton;
        CGUIColorArrowButtonAbstract* ColorArrowButton;
        CGUIToolbarHeaderAbstract* ToolbarHeader;

        Control(
            ControlKind kind,
            const ControlOptions& options,
            const ControlLayout& layout,
            WORD numericId)
            : Kind(kind),
              Id(options.Id != NULL ? options.Id : ""),
              Text(options.Text != NULL ? options.Text : ""),
              ReadOnly(options.ReadOnly),
              Checked(options.Checked),
              DialogResult(options.DialogResult),
              KeepOpen(options.KeepOpen),
              Multiline(options.Multiline),
              FileFilter(options.FileFilter != NULL ? options.FileFilter : ""),
              FileSave(options.FileSave),
              AccessibleName(options.AccessibleName != NULL ? options.AccessibleName : ""),
              AccessibleDescription(options.AccessibleDescription != NULL ? options.AccessibleDescription : ""),
              Enabled(TRUE),
              WindowHandle(NULL),
              BrowseWindowHandle(NULL),
              BrowseNumericId(0),
              Required(FALSE),
              ValidationMessage(),
              HasBounds(layout.HasBounds),
              X(layout.X),
              Y(layout.Y),
              Width(layout.Width),
              Height(layout.Height),
              SelectedIndex(-1),
              NumericId(numericId),
              StyleFlags(0),
              PathSeparator('\\'),
              ActionCommand(0),
              ProgressValue(0),
              ProgressCurrent(0),
              ProgressTotal(0),
              HasProgressValues(FALSE),
              IndeterminateDuration(0xFFFFFFFF),
              IndeterminateInterval(50),
              TextColor(RGB(0, 0, 0)),
              BackgroundColor(RGB(255, 255, 255)),
              ToolbarButtonMask(0),
              StaticText(NULL),
              HyperLink(NULL),
              ProgressBar(NULL),
              TextArrowButton(NULL),
              ColorArrowButton(NULL),
              ToolbarHeader(NULL)
        {
        }

        virtual ControlKind WINAPI GetKind() const { return Kind; }
        virtual const char* WINAPI GetId() const { return Id.c_str(); }

        virtual BOOL WINAPI GetText(char* buffer, DWORD capacity) const
        {
            if (buffer == NULL || capacity == 0 || Text.size() >= capacity)
            {
                if (buffer != NULL && capacity != 0)
                    buffer[0] = '\0';
                return FALSE;
            }
            memcpy(buffer, Text.c_str(), Text.size() + 1);
            return TRUE;
        }

        virtual BOOL WINAPI SetText(const char* value)
        {
            if (value == NULL)
                return FALSE;
            Text.assign(value);
            if (StaticText != NULL)
                return StaticText->SetText(value);
            if (HyperLink != NULL)
                return HyperLink->SetText(value);
            if (WindowHandle != NULL)
            {
                std::wstring wide;
                if (Utf8ToWide(Text.c_str(), wide))
                {
                    SetWindowTextW(WindowHandle, wide.c_str());
                    if (Multiline)
                    {
                        SendMessage(WindowHandle, EM_SETSEL,
                                    static_cast<WPARAM>(-1),
                                    static_cast<LPARAM>(-1));
                        SendMessage(WindowHandle, EM_SCROLLCARET, 0, 0);
                    }
                    UpdateWindow(WindowHandle);
                }
            }
            return TRUE;
        }

        virtual BOOL WINAPI GetChecked() const { return Checked; }

        virtual BOOL WINAPI SetChecked(BOOL checked)
        {
            Checked = checked;
            if (WindowHandle != NULL)
                SendMessage(WindowHandle, BM_SETCHECK,
                            checked ? BST_CHECKED : BST_UNCHECKED, 0);
            return TRUE;
        }

        virtual int WINAPI GetDialogResult() const { return DialogResult; }

        virtual const char* WINAPI GetAccessibleName() const
        {
            return AccessibleName.c_str();
        }

        virtual const char* WINAPI GetAccessibleDescription() const
        {
            return AccessibleDescription.c_str();
        }

        virtual BOOL WINAPI AddItem(const char* value, int parentIndex)
        {
            if (value == NULL ||
                 (Kind != ControlKindComboBox &&
                  Kind != ControlKindListView &&
                 Kind != ControlKindTreeView &&
                Kind != ControlKindTabControl) ||
                Items.size() >= 256 ||
                parentIndex < -1 ||
                (Kind == ControlKindTreeView &&
                 parentIndex >= static_cast<int>(Items.size())))
                return FALSE;
            Items.push_back(value);
            ItemParents.push_back(parentIndex);
            if (WindowHandle != NULL && Kind == ControlKindListView)
            {
                std::wstring wide;
                if (Utf8ToWide(value, wide))
                {
                    LVITEMW item;
                    memset(&item, 0, sizeof(item));
                    item.mask = LVIF_TEXT;
                    item.iItem = static_cast<int>(Items.size()) - 1;
                    item.pszText = const_cast<wchar_t*>(wide.c_str());
                    SendMessageW(WindowHandle, LVM_INSERTITEMW, 0,
                                 reinterpret_cast<LPARAM>(&item));
                }
            }
            return TRUE;
        }

        virtual BOOL WINAPI ClearItems()
        {
            Items.clear();
            ItemParents.clear();
            if (WindowHandle != NULL && Kind == ControlKindListView)
                SendMessage(WindowHandle, LVM_DELETEALLITEMS, 0, 0);
            return TRUE;
        }

        virtual int WINAPI GetItemCount() const
        {
            return static_cast<int>(Items.size());
        }

        virtual BOOL WINAPI AddColumn(const char* title, int width)
        {
            if (Kind != ControlKindListView || title == NULL ||
                title[0] == '\0' || width <= 0 ||
                ColumnTitles.size() >= 64)
                return FALSE;
            ColumnTitles.push_back(title);
            ColumnWidths.push_back(width);
            return TRUE;
        }

        virtual int WINAPI GetSelectedIndex() const
        {
            return SelectedIndex;
        }

        virtual BOOL WINAPI SetSelectedIndex(int index)
        {
            if ((Kind != ControlKindListView &&
                 Kind != ControlKindComboBox &&
                 Kind != ControlKindTabControl) ||
                index < -1 || index >= static_cast<int>(Items.size()))
                return FALSE;
            SelectedIndex = index;
            if (index >= 0 && index < static_cast<int>(Items.size()))
            {
                Text = Items[index];
                if (WindowHandle != NULL && Kind == ControlKindComboBox)
                    SendMessage(WindowHandle, CB_SETCURSEL, index, 0);
            }
            return TRUE;
        }

        virtual BOOL WINAPI SetRequired(BOOL required)
        {
            Required = required;
            return TRUE;
        }

        virtual BOOL WINAPI IsRequired() const
        {
            return Required;
        }

        virtual BOOL WINAPI SetValidationMessage(const char* message)
        {
            ValidationMessage.assign(message != NULL ? message : "");
            return TRUE;
        }

        virtual BOOL WINAPI GetValidationMessage(
            char* buffer,
            DWORD capacity) const
        {
            if (buffer == NULL || capacity == 0 ||
                ValidationMessage.size() >= capacity)
            {
                if (buffer != NULL && capacity != 0)
                    buffer[0] = '\0';
                return FALSE;
            }
            memcpy(buffer, ValidationMessage.c_str(),
                   ValidationMessage.size() + 1);
            return TRUE;
        }

        virtual BOOL WINAPI SetBounds(int x, int y, int width, int height)
        {
            if (WindowHandle == NULL || width <= 0 || height <= 0)
                return FALSE;
            X = x; Y = y; Width = width; Height = height; HasBounds = TRUE;
            SetWindowPos(WindowHandle, NULL, x, y, width, height,
                         SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
            if (Kind == ControlKindListView)
            {
                HWND header = ListView_GetHeader(WindowHandle);
                if (header != NULL && Header_GetItemCount(header) == 1)
                {
                    const int columnWidth = width - GetSystemMetrics(SM_CXVSCROLL) - 4;
                    ListView_SetColumnWidth(WindowHandle, 0,
                                            columnWidth > 32 ? columnWidth : 32);
                }
            }
            if (BrowseWindowHandle != NULL)
            {
                FilePickerLayoutMetrics metrics =
                    ComputeFilePickerLayout(x, width);
                SetWindowPos(BrowseWindowHandle, NULL,
                             metrics.BrowseX, y, metrics.BrowseWidth, height,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
            }
            return TRUE;
        }

        virtual BOOL WINAPI SetEnabled(BOOL enabled)
        {
            Enabled = enabled;
            if (WindowHandle != NULL)
                EnableWindow(WindowHandle, enabled);
            if (BrowseWindowHandle != NULL)
                EnableWindow(BrowseWindowHandle, enabled);
            return TRUE;
        }

        virtual BOOL WINAPI IsEnabled() const
        {
            return Enabled;
        }

        virtual BOOL WINAPI SetStyleFlags(DWORD flags)
        {
            if (WindowHandle != NULL)
                return FALSE;
            StyleFlags = flags;
            return TRUE;
        }

        virtual BOOL WINAPI SetPathSeparator(char separator)
        {
            if (Kind != ControlKindStaticText || separator == '\0')
                return FALSE;
            PathSeparator = separator;
            if (StaticText != NULL)
                StaticText->SetPathSeparator(separator);
            return TRUE;
        }

        virtual BOOL WINAPI SetToolTipText(const char* text)
        {
            ToolTipText.assign(text != NULL ? text : "");
            if (StaticText != NULL)
                return StaticText->SetToolTipText(ToolTipText.c_str());
            if (HyperLink != NULL)
                return HyperLink->SetToolTipText(ToolTipText.c_str());
            if (TextArrowButton != NULL)
                return TextArrowButton->SetToolTipText(ToolTipText.c_str());
            return WindowHandle == NULL;
        }

        virtual BOOL WINAPI SetActionOpen(const char* target)
        {
            if (Kind != ControlKindHyperLink || target == NULL)
                return FALSE;
            ActionTarget.assign(target);
            if (HyperLink != NULL)
                HyperLink->SetActionOpen(ActionTarget.c_str());
            return TRUE;
        }

        virtual BOOL WINAPI SetActionPostCommand(WORD command)
        {
            if (Kind != ControlKindHyperLink)
                return FALSE;
            ActionCommand = command;
            if (HyperLink != NULL)
                HyperLink->SetActionPostCommand(command);
            return TRUE;
        }

        virtual BOOL WINAPI SetActionShowHint(const char* text)
        {
            if (Kind != ControlKindHyperLink)
                return FALSE;
            ActionHint.assign(text != NULL ? text : "");
            return HyperLink != NULL
                       ? HyperLink->SetActionShowHint(
                             text != NULL ? ActionHint.c_str() : NULL)
                       : TRUE;
        }

        virtual BOOL WINAPI SetProgress(int progress, const char* text)
        {
            if (Kind != ControlKindProgressBar || progress < -1 || progress > 1000)
                return FALSE;
            ProgressValue = progress;
            HasProgressValues = FALSE;
            ProgressText.assign(text != NULL ? text : "");
            if (ProgressBar != NULL)
                ProgressBar->SetProgress(
                    static_cast<DWORD>(progress),
                    text != NULL ? ProgressText.c_str() : NULL);
            return TRUE;
        }

        virtual BOOL WINAPI SetProgressValues(
            ULONGLONG current, ULONGLONG total, const char* text)
        {
            if (Kind != ControlKindProgressBar)
                return FALSE;
            ProgressCurrent = current;
            ProgressTotal = total;
            HasProgressValues = TRUE;
            ProgressText.assign(text != NULL ? text : "");
            if (ProgressBar != NULL)
            {
                CQuadWord currentValue;
                CQuadWord totalValue;
                currentValue.SetUI64(current);
                totalValue.SetUI64(total);
                ProgressBar->SetProgress2(
                    currentValue, totalValue,
                    text != NULL ? ProgressText.c_str() : NULL);
            }
            return TRUE;
        }

        virtual BOOL WINAPI SetIndeterminateTiming(DWORD duration, DWORD interval)
        {
            if (Kind != ControlKindProgressBar || interval == 0)
                return FALSE;
            IndeterminateDuration = duration;
            IndeterminateInterval = interval;
            if (ProgressBar != NULL)
            {
                ProgressBar->SetSelfMoveTime(duration);
                ProgressBar->SetSelfMoveSpeed(interval);
            }
            return TRUE;
        }

        virtual BOOL WINAPI SetColor(
            COLORREF textColor, COLORREF backgroundColor)
        {
            if (Kind != ControlKindColorArrowButton)
                return FALSE;
            TextColor = textColor;
            BackgroundColor = backgroundColor;
            if (ColorArrowButton != NULL)
                ColorArrowButton->SetColor(textColor, backgroundColor);
            return TRUE;
        }

        virtual BOOL WINAPI SetToolbarHeader(
            const char* alignControlId, DWORD buttonMask)
        {
            if (Kind != ControlKindToolbarHeader || alignControlId == NULL ||
                alignControlId[0] == '\0' || WindowHandle != NULL)
                return FALSE;
            ToolbarAlignControlId.assign(alignControlId);
            ToolbarButtonMask = buttonMask;
            return TRUE;
        }
    };

    static LRESULT CALLBACK SplitterSubclassProc(
        HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
        UINT_PTR subclassId, DWORD_PTR reference)
    {
        Control* control = reinterpret_cast<Control*>(reference);
        switch (message)
        {
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            return TRUE;
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            SetCursor(LoadCursor(NULL, IDC_SIZENS));
            return 0;
        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd && (wParam & MK_LBUTTON) != 0 &&
                control != NULL)
            {
                POINT point = {
                    static_cast<short>(LOWORD(lParam)),
                    static_cast<short>(HIWORD(lParam))};
                MapWindowPoints(hwnd, GetParent(hwnd), &point, 1);
                SendMessage(GetParent(hwnd), WM_SALAMATRIX_SPLITTER_MOVED,
                            control->NumericId, point.y);
                return 0;
            }
            break;
        case WM_LBUTTONUP:
            if (GetCapture() == hwnd)
                ReleaseCapture();
            return 0;
        case WM_NCDESTROY:
            RemoveWindowSubclass(hwnd, SplitterSubclassProc, subclassId);
            break;
        }
        return DefSubclassProc(hwnd, message, wParam, lParam);
    }

    DialogOptions Options;
    std::string Title;
    std::vector<Control*> Controls;
    HWND Window;
    HWND AccessibilityTooltip;
    int Result;
    BOOL Running;
    UINT CurrentDpi;
    DialogEventCallback EventCallback;
    void* EventContext;
    DialogResizeCallback ResizeCallback;
    void* ResizeContext;
    DialogCloseCallback CloseCallback;
    void* CloseContext;

    void ApplyDarkScrollbarScopes(BOOL dark)
    {
        INativeDialogHost* host = GetNativeDialogHost();
        if (host == NULL)
            return;
        for (size_t index = 0; index < Controls.size(); ++index)
        {
            Control* control = Controls[index];
            if (control == NULL || control->WindowHandle == NULL)
                continue;
            const bool hasScrollbar =
                (control->Kind == ControlKindTextBox && control->Multiline) ||
                control->Kind == ControlKindListView ||
                control->Kind == ControlKindTreeView ||
                control->Kind == ControlKindTabControl;
            if (!hasScrollbar)
                continue;
            host->SetDarkScrollbars(control->WindowHandle, dark);
        }
    }

    explicit Impl(const DialogOptions& options)
        : Options(options),
          Title(options.Title != NULL ? options.Title : "Salamander"),
          Window(NULL),
          AccessibilityTooltip(NULL),
          Result(0),
          Running(FALSE),
          CurrentDpi(96),
          EventCallback(NULL),
          EventContext(NULL),
          ResizeCallback(NULL),
          ResizeContext(NULL),
          CloseCallback(NULL),
          CloseContext(NULL)
    {
    }

    ~Impl()
    {
        if (AccessibilityTooltip != NULL)
            DestroyWindow(AccessibilityTooltip);
        for (size_t index = 0; index < Controls.size(); ++index)
            delete Controls[index];
    }

    Control* Find(const char* id) const
    {
        if (id == NULL)
            return NULL;
        for (size_t index = 0; index < Controls.size(); ++index)
        {
            if (_stricmp(Controls[index]->Id.c_str(), id) == 0)
                return Controls[index];
        }
        return NULL;
    }

    Control* Find(WORD numericId) const
    {
        for (size_t index = 0; index < Controls.size(); ++index)
        {
            if (Controls[index]->NumericId == numericId ||
                Controls[index]->BrowseNumericId == numericId)
                return Controls[index];
        }
        return NULL;
    }

    Control* FindInvalid() const
    {
        for (size_t index = 0; index < Controls.size(); ++index)
        {
            Control* control = Controls[index];
            if (control->Required &&
                (control->Kind == ControlKindTextBox ||
                 control->Kind == ControlKindFolderPicker ||
                 control->Kind == ControlKindFilePicker ||
                 control->Kind == ControlKindComboBox) &&
                control->Text.empty())
                return control;
        }
        return NULL;
    }

    void NotifyChanged(Control* control)
    {
        if (control == NULL || EventCallback == NULL)
            return;
        DialogEvent event;
        event.Control = control->Kind;
        CopyEventText(
            event.ControlId, _countof(event.ControlId), control->Id);
        CopyEventText(event.Text, _countof(event.Text), control->Text);
        event.Checked = control->Checked;
        event.SelectedIndex = control->SelectedIndex;
        EventCallback(EventContext, &event);
    }

    void AddAccessibilityTooltip(HWND tooltip, HWND target, Control* control)
    {
        if (tooltip == NULL || target == NULL || control == NULL ||
            (control->AccessibleName.empty() &&
             control->AccessibleDescription.empty()))
            return;
        const std::string& text = control->AccessibleDescription.empty()
                                      ? control->AccessibleName
                                      : control->AccessibleDescription;
        if (!Utf8ToWide(text.c_str(), control->AccessibleTooltipText) ||
            control->AccessibleTooltipText.empty())
            return;
        TOOLINFOW tool;
        memset(&tool, 0, sizeof(tool));
        tool.cbSize = sizeof(tool);
        tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        tool.hwnd = GetParent(target);
        tool.uId = reinterpret_cast<UINT_PTR>(target);
        tool.lpszText = const_cast<wchar_t*>(
            control->AccessibleTooltipText.c_str());
        SendMessageW(tooltip, TTM_DELTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
        SendMessageW(tooltip, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
    }
};

NativeDialog::NativeDialog(const DialogOptions& options)
    : m_pImpl(new Impl(options))
{
}

NativeDialog::~NativeDialog()
{
    Close();
    delete m_pImpl;
    m_pImpl = NULL;
}

DWORD WINAPI NativeDialog::GetVersion() const
{
    return SALAMATRIX_UI_VERSION_1_4;
}

IControl* WINAPI NativeDialog::AddControl(
    ControlKind kind,
    const ControlOptions& options)
{
    ControlLayout layout;
    return AddControlEx(kind, options, layout);
}

IControl* WINAPI NativeDialog::AddControlEx(
    ControlKind kind,
    const ControlOptions& options,
    const ControlLayout& layout)
{
    if (m_pImpl == NULL || m_pImpl->Running || m_pImpl->Controls.size() >= 64 ||
        (options.Id != NULL && m_pImpl->Find(options.Id) != NULL))
        return NULL;
    WORD numericId = static_cast<WORD>(2000 + m_pImpl->Controls.size());
    Impl::Control* control = new Impl::Control(kind, options, layout, numericId);
    if (kind == ControlKindFilePicker)
        control->BrowseNumericId = static_cast<WORD>(4000 + m_pImpl->Controls.size());
    m_pImpl->Controls.push_back(control);
    return control;
}

BOOL WINAPI NativeDialog::SetEventCallback(
    DialogEventCallback callback,
    void* context)
{
    if (m_pImpl == NULL)
        return FALSE;
    m_pImpl->EventCallback = callback;
    m_pImpl->EventContext = context;
    return TRUE;
}

BOOL WINAPI NativeDialog::SetResizeCallback(
    DialogResizeCallback callback,
    void* context)
{
    if (m_pImpl == NULL)
        return FALSE;
    m_pImpl->ResizeCallback = callback;
    m_pImpl->ResizeContext = context;
    return TRUE;
}

BOOL WINAPI NativeDialog::SetCloseCallback(
    DialogCloseCallback callback,
    void* context)
{
    if (m_pImpl == NULL)
        return FALSE;
    m_pImpl->CloseCallback = callback;
    m_pImpl->CloseContext = context;
    return TRUE;
}

IControl* WINAPI NativeDialog::FindControl(const char* id)
{
    return m_pImpl != NULL ? m_pImpl->Find(id) : NULL;
}

int WINAPI NativeDialog::ShowModal()
{
    if (m_pImpl == NULL || m_pImpl->Running)
        return 0;

    std::wstring title;
    if (!Utf8ToWide(m_pImpl->Title.c_str(), title))
        return 0;
    std::vector<BYTE> dialog;
    dialog.resize(sizeof(DLGTEMPLATE), 0);
    DLGTEMPLATE* header = reinterpret_cast<DLGTEMPLATE*>(&dialog[0]);
    header->style = WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION |
                    DS_SETFONT;
    if (!m_pImpl->Options.Modeless)
        header->style |= DS_MODALFRAME;
    if (m_pImpl->Options.Resizable)
        header->style |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    // Let dialog-manager keyboard navigation traverse child controls,
    // including the two controls composing an editable file picker.
    header->dwExtendedStyle = WS_EX_CONTROLPARENT |
                              (m_pImpl->Options.Taskbar ? WS_EX_APPWINDOW : 0);
    size_t dialogItemCount = m_pImpl->Controls.size();
    for (size_t index = 0; index < m_pImpl->Controls.size(); ++index)
    {
        if (m_pImpl->Controls[index]->Kind == ControlKindFilePicker)
            ++dialogItemCount;
    }
    header->cdit = static_cast<WORD>(dialogItemCount);
    header->x = 10;
    header->y = 10;
    header->cx = m_pImpl->Options.Width;
    header->cy = m_pImpl->Options.Height;
    AppendWord(dialog, 0);
    AppendWord(dialog, 0);
    AppendString(dialog, title);
    AppendWord(dialog, 8);
    AppendString(dialog, L"MS Shell Dlg");

    short y = 8;
    for (size_t index = 0; index < m_pImpl->Controls.size(); ++index)
    {
        Impl::Control* control = m_pImpl->Controls[index];
        std::wstring text;
        if (!Utf8ToWide(control->Text.c_str(), text))
            text.clear();
        DWORD style = WS_CHILD | WS_VISIBLE;
        if (control->Kind != ControlKindLabel &&
            control->Kind != ControlKindStaticText &&
            control->Kind != ControlKindProgressBar &&
            control->Kind != ControlKindToolbarHeader &&
            control->Kind != ControlKindGroupBox &&
            control->Kind != ControlKindSplitter)
            style |= WS_TABSTOP;
        WORD classOrdinal = 0x0082; // STATIC
        short height = 14;
        short width = static_cast<short>(m_pImpl->Options.Width - 16);
        if (control->Kind == ControlKindSplitter)
        {
            style |= SS_NOTIFY | SS_ETCHEDHORZ;
            height = 4;
        }
        else if (control->Kind == ControlKindGroupBox)
        {
            classOrdinal = 0x0080;
            style |= BS_GROUPBOX;
        }
        else if (control->Kind == ControlKindStaticText ||
                 control->Kind == ControlKindHyperLink ||
                 control->Kind == ControlKindToolbarHeader)
        {
            classOrdinal = 0x0082;
            if ((control->StyleFlags & StaticTextAlignCenter) != 0)
                style |= SS_CENTER;
            else if ((control->StyleFlags & StaticTextAlignRight) != 0)
                style |= SS_RIGHT;
            else
                style |= SS_LEFT;
            if ((control->StyleFlags & StaticTextNotify) != 0)
                style |= SS_NOTIFY;
        }
        else if (control->Kind == ControlKindProgressBar)
        {
            // AttachProgressBar replaces the rendering of this placeholder.
            classOrdinal = 0x0082;
        }
        else if (control->Kind == ControlKindTextBox ||
            control->Kind == ControlKindFilePicker)
        {
            classOrdinal = 0x0081; // EDIT
            style |= WS_BORDER;
            if (control->Multiline)
                style |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL;
            else
                style |= ES_AUTOHSCROLL;
            if (control->ReadOnly)
                style |= ES_READONLY;
            height = 18;
        }
        else if (control->Kind == ControlKindFolderPicker)
        {
            classOrdinal = 0x0080; // BUTTON
            style |= BS_PUSHBUTTON | BS_LEFT;
            height = 18;
        }
        else if (control->Kind == ControlKindCheckBox)
        {
            classOrdinal = 0x0080; // BUTTON
            style |= BS_AUTOCHECKBOX;
        }
        else if (control->Kind == ControlKindRadioButton)
        {
            classOrdinal = 0x0080;
            style |= BS_AUTORADIOBUTTON;
        }
        else if (control->Kind == ControlKindButton)
        {
            classOrdinal = 0x0080;
            style |= (control->StyleFlags & ButtonDefault) != 0
                         ? BS_DEFPUSHBUTTON
                         : BS_PUSHBUTTON;
            width = 70;
        }
        else if (control->Kind == ControlKindArrowButton ||
                 control->Kind == ControlKindTextArrowButton ||
                 control->Kind == ControlKindColorArrowButton)
        {
            classOrdinal = 0x0080;
            style |= BS_PUSHBUTTON;
        }
        else if (control->Kind == ControlKindComboBox)
        {
            classOrdinal = 0x0085; // COMBOBOX
            style |= CBS_DROPDOWNLIST | WS_VSCROLL;
            height = 80;
        }
        else if (control->Kind == ControlKindListView ||
                 control->Kind == ControlKindTreeView ||
                 control->Kind == ControlKindTabControl)
        {
            classOrdinal = 0;
            style |= WS_BORDER;
            height = static_cast<short>(m_pImpl->Options.Height > 64
                                            ? m_pImpl->Options.Height - 48
                                            : 64);
        }
        const wchar_t* className = NULL;
        if (control->Kind == ControlKindListView)
        {
            className = L"SysListView32";
            style |= LVS_REPORT | LVS_SINGLESEL;
            if ((control->StyleFlags & ListViewShowSelectionAlways) != 0)
                style |= LVS_SHOWSELALWAYS;
            if ((control->StyleFlags & ListViewEditLabels) != 0)
                style |= LVS_EDITLABELS;
            if ((control->StyleFlags & ListViewNoSortHeader) != 0)
                style |= LVS_NOSORTHEADER;
        }
        else if (control->Kind == ControlKindTreeView)
        {
            className = L"SysTreeView32";
            style |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;
        }
        else if (control->Kind == ControlKindTabControl)
        {
            className = L"SysTabControl32";
            style |= TCS_TABS | TCS_SINGLELINE;
        }
        short x = 8;
        short itemY = y;
        if (control->Kind == ControlKindButton)
            x = static_cast<short>(m_pImpl->Options.Width - 78);
        if (control->HasBounds)
        {
            x = ClampDialogCoordinate(control->X);
            itemY = ClampDialogCoordinate(control->Y);
            if (control->Width > 0)
                width = ClampDialogCoordinate(control->Width);
            if (control->Height > 0)
                height = ClampDialogCoordinate(control->Height);
        }
        if (control->Kind == ControlKindFilePicker)
        {
            FilePickerLayoutMetrics metrics =
                ComputeFilePickerLayout(x, width);
            AppendItem(
                dialog, x, itemY, ClampDialogCoordinate(metrics.EditWidth), height,
                control->NumericId, style, classOrdinal, text, className);
            AppendItem(
                dialog, ClampDialogCoordinate(metrics.BrowseX), itemY,
                ClampDialogCoordinate(metrics.BrowseWidth), height,
                control->BrowseNumericId,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                0x0080, L"...", NULL);
        }
        else
        {
            AppendItem(dialog, x, itemY, width, height, control->NumericId,
                       style, classOrdinal, text, className);
        }
        if (!control->HasBounds)
            y = static_cast<short>(y + (control->Kind == ControlKindComboBox ? 24 : 22));
    }

    m_pImpl->Running = TRUE;
    m_pImpl->Result = 0;
    if (m_pImpl->Options.Modeless)
    {
        HWND owner = m_pImpl->Options.Taskbar ? NULL : m_pImpl->Options.Parent;
        HWND window = CreateDialogIndirectParamW(
            GetModuleHandle(NULL), reinterpret_cast<DLGTEMPLATE*>(&dialog[0]),
            owner, DialogProc, reinterpret_cast<LPARAM>(this));
        if (window != NULL)
        {
            if (m_pImpl->Options.SmallIcon != NULL)
                SendMessage(window, WM_SETICON, ICON_SMALL,
                            reinterpret_cast<LPARAM>(m_pImpl->Options.SmallIcon));
            if (m_pImpl->Options.LargeIcon != NULL)
                SendMessage(window, WM_SETICON, ICON_BIG,
                            reinterpret_cast<LPARAM>(m_pImpl->Options.LargeIcon));
            RECT clientRect;
            if (GetClientRect(window, &clientRect))
                SendMessage(window, WM_SIZE, SIZE_RESTORED,
                            MAKELPARAM(clientRect.right - clientRect.left,
                                      clientRect.bottom - clientRect.top));
            ShowWindow(window, SW_SHOWNORMAL);
            UpdateWindow(window);
        }
    }
    else
    {
        DialogBoxIndirectParamW(
            GetModuleHandle(NULL),
            reinterpret_cast<DLGTEMPLATE*>(&dialog[0]),
            m_pImpl->Options.Parent,
            DialogProc,
            reinterpret_cast<LPARAM>(this));
    }
    if (!m_pImpl->Options.Modeless)
    {
        m_pImpl->Window = NULL;
        m_pImpl->Running = FALSE;
    }
    return m_pImpl->Result;
}

void WINAPI NativeDialog::Close()
{
    if (m_pImpl != NULL && m_pImpl->Window != NULL)
    {
        if (m_pImpl->Options.Modeless || ClosingAllNativeDialogs)
            DestroyWindow(m_pImpl->Window);
        else
            EndDialog(m_pImpl->Window, 0);
    }
}

void WINAPI NativeDialog::Release()
{
    delete this;
}

static IControl* AddControlsShowcaseControl(
    IDialog* dialog,
    ControlKind kind,
    const char* id,
    const char* text,
    int x,
    int y,
    int width,
    int height,
    BOOL readOnly = FALSE,
    BOOL checked = FALSE,
    int dialogResult = 0,
    BOOL multiline = FALSE,
    const char* fileFilter = NULL,
    BOOL keepOpen = FALSE)
{
    ControlOptions options;
    options.Id = id;
    options.Text = text;
    options.ReadOnly = readOnly;
    options.Checked = checked;
    options.DialogResult = dialogResult;
    options.Multiline = multiline;
    options.FileFilter = fileFilter;
    options.KeepOpen = keepOpen;

    ControlLayout layout;
    layout.HasBounds = TRUE;
    layout.X = x;
    layout.Y = y;
    layout.Width = width;
    layout.Height = height;
    return dialog->AddControlEx(kind, options, layout);
}

BOOL WINAPI ShowNativeControlsShowcase(HWND parent)
{
    DialogOptions options;
    options.Title = "Salamatrix UI capabilities";
    options.Parent = parent;
    options.Width = 463;
    options.Height = 236;
    NativeDialog dialog(options);

    bool complete = true;
    char uptime[96];
    _snprintf_s(
        uptime, _countof(uptime), _TRUNCATE,
        "System was started %lu ms ago.",
        static_cast<unsigned long>(GetTickCount()));
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindGroupBox, "static-group",
        "CGUIStaticTextAbstract", 6, 4, 254, 108) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, "not-attached-label",
        "Not attached static text", 14, 17, 80, 8) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, "uptime-plain",
        uptime, 102, 17, 152, 8) != NULL && complete;

    struct StaticRow
    {
        const char* Id;
        const char* Caption;
        const char* Value;
        int Y;
        DWORD Flags;
    };
    const StaticRow rows[] = {
        {"static-none", "0 (no flags)", uptime, 27, 0},
        {"static-cache", "STF_CACHED_PAINT", uptime, 37, StaticTextCachedPaint},
        {"static-bold", "STF_BOLD", "Bold &text", 47, StaticTextBold | StaticTextHandlePrefix | StaticTextAlignCenter},
        {"static-underline", "STF_UNDERLINE", "Underlined text", 56, StaticTextUnderline | StaticTextAlignRight},
        {"static-end", "STF_END_ELLIPSIS", "Long long long long long long long long long string.", 66, StaticTextEndEllipsis},
        {"static-path", "STF_PATH_ELLIPSIS", "C:\\Program Files\\Some Application With Long Path\\example.exe", 76, StaticTextPathEllipsis},
        {"static-path-url", "STF_PATH_ELLIPSIS", "ftp://ftp.altap.cz/pub/salamander/example.exe", 87, StaticTextPathEllipsis}};
    for (size_t index = 0; index < _countof(rows); ++index)
    {
        complete = AddControlsShowcaseControl(
            &dialog, ControlKindLabel, NULL, rows[index].Caption,
            14, rows[index].Y, 75, 8) != NULL && complete;
        IControl* text = AddControlsShowcaseControl(
            &dialog, ControlKindStaticText, rows[index].Id,
            rows[index].Value, 102, rows[index].Y, 152, 8);
        complete = text != NULL && text->SetStyleFlags(rows[index].Flags) && complete;
        if (text != NULL && index == _countof(rows) - 1)
            complete = text->SetPathSeparator('/') && complete;
    }
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, "drag-hint",
        "Drag texts to change their size.", 151, 97, 103, 8) != NULL && complete;

    complete = AddControlsShowcaseControl(
        &dialog, ControlKindGroupBox, "progress-group",
        "CGUIProgressBarAbstract", 6, 118, 254, 66) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, NULL, "Progress label",
        15, 129, 60, 8) != NULL && complete;
    IControl* progress = AddControlsShowcaseControl(
        &dialog, ControlKindProgressBar, "progress", "",
        15, 138, 235, 12);
    complete = progress != NULL && progress->SetProgress(120) && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, NULL, "Unknown progress",
        15, 154, 67, 8) != NULL && complete;
    IControl* unknownProgress = AddControlsShowcaseControl(
        &dialog, ControlKindProgressBar, "unknown-progress", "",
        15, 163, 235, 12);
    complete = unknownProgress != NULL &&
               unknownProgress->SetIndeterminateTiming(0xFFFFFFFF, 100) &&
               unknownProgress->SetProgress(-1) && complete;

    complete = AddControlsShowcaseControl(
        &dialog, ControlKindGroupBox, "buttons-group",
        "Button, CGUITextArrowButtonAbstract, CGUIColorArrowButtonAbstract",
        6, 188, 254, 40) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindButton, "more", "...",
        15, 204, 15, 14, FALSE, FALSE, 0, FALSE, NULL, TRUE) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindArrowButton, "arrow", "",
        37, 204, 15, 14) != NULL && complete;
    IControl* choose = AddControlsShowcaseControl(
        &dialog, ControlKindTextArrowButton, "choose", "&Choose",
        60, 204, 50, 14);
    complete = choose != NULL &&
               choose->SetStyleFlags(TextArrowButtonRightArrow) && complete;
    IControl* drop = AddControlsShowcaseControl(
        &dialog, ControlKindTextArrowButton, "drop", "&Drop",
        117, 204, 50, 14);
    complete = drop != NULL &&
               drop->SetStyleFlags(TextArrowButtonDropDown) && complete;
    IControl* color = AddControlsShowcaseControl(
        &dialog, ControlKindColorArrowButton, "color", "",
        174, 204, 33, 14);
    complete = color != NULL &&
               color->SetColor(RGB(0, 128, 255), RGB(0, 128, 255)) && complete;
    IControl* colorText = AddControlsShowcaseControl(
        &dialog, ControlKindColorArrowButton, "color-text", "ABC",
        215, 204, 33, 14);
    complete = colorText != NULL &&
               colorText->SetColor(RGB(0, 0, 0), RGB(255, 255, 0)) && complete;

    complete = AddControlsShowcaseControl(
        &dialog, ControlKindGroupBox, "hyperlink-group",
        "CGUIHyperLinkAbstract", 269, 4, 185, 48) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, NULL, "SetActionOpen",
        277, 17, 75, 8) != NULL && complete;
    IControl* open = AddControlsShowcaseControl(
        &dialog, ControlKindHyperLink, "open-link", "www.altap.cz",
        365, 17, 47, 8);
    complete = open != NULL &&
               open->SetStyleFlags(StaticTextUnderline | StaticTextHyperLinkColor) &&
               open->SetActionOpen("https://www.altap.cz") && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, NULL, "SetActionPostCommand",
        277, 27, 81, 8) != NULL && complete;
    IControl* command = AddControlsShowcaseControl(
        &dialog, ControlKindHyperLink, "command-link", "Say something!",
        365, 27, 55, 8);
    complete = command != NULL &&
               command->SetStyleFlags(StaticTextUnderline | StaticTextHyperLinkColor) &&
               command->SetActionPostCommand(0x7F01) && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, NULL, "SetActionPostCommand",
        277, 37, 81, 8) != NULL && complete;
    IControl* hint = AddControlsShowcaseControl(
        &dialog, ControlKindHyperLink, "hint-link", "mask hints",
        365, 37, 40, 8);
    complete = hint != NULL &&
               hint->SetStyleFlags(StaticTextDotUnderline) &&
               hint->SetActionShowHint(
                   "text 1 text 1 text 1 text 1\ntext 2 text 2 text 2 ") && complete;

    complete = AddControlsShowcaseControl(
        &dialog, ControlKindGroupBox, "tooltip-group",
        "SetCurrentToolTip", 269, 59, 185, 31) != NULL && complete;
    IControl* tooltip = AddControlsShowcaseControl(
        &dialog, ControlKindStaticText, "tooltip",
        "Pause the mouse pointer over this text.", 278, 73, 130, 8);
    complete = tooltip != NULL &&
               tooltip->SetStyleFlags(StaticTextNotify) &&
               tooltip->SetToolTipText("ToolTip") && complete;

    IControl* list = AddControlsShowcaseControl(
        &dialog, ControlKindListView, "header-list", "",
        269, 113, 185, 50);
    complete = list != NULL && list->SetStyleFlags(
        ListViewNoDefaultColumn | ListViewShowSelectionAlways |
        ListViewEditLabels | ListViewNoSortHeader) && complete;
    IControl* header = AddControlsShowcaseControl(
        &dialog, ControlKindToolbarHeader, "toolbar-header",
        "CGUIToolbarHeaderAbstract", 269, 102, 96, 8);
    complete = header != NULL && header->SetToolbarHeader(
        "header-list",
        ToolbarHeaderModify | ToolbarHeaderUp | ToolbarHeaderDown) && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindGroupBox, "origin-group",
        "Created by", 269, 169, 185, 38) != NULL && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, "runtime-label",
        "Runtime:", 277, 181, 42, 8) != NULL && complete;
    IControl* runtimeValue = AddControlsShowcaseControl(
        &dialog, ControlKindStaticText, "runtime-value",
        "Native", 323, 181, 122, 8);
    complete = runtimeValue != NULL &&
               runtimeValue->SetStyleFlags(StaticTextBold) && complete;
    complete = AddControlsShowcaseControl(
        &dialog, ControlKindLabel, "extension-label",
        "Extension:", 277, 192, 42, 8) != NULL && complete;
    IControl* extensionValue = AddControlsShowcaseControl(
        &dialog, ControlKindStaticText, "extension-value",
        "Salamatrix Framework", 323, 192, 122, 8);
    complete = extensionValue != NULL &&
               extensionValue->SetStyleFlags(StaticTextBold) && complete;
    IControl* close = AddControlsShowcaseControl(
                   &dialog, ControlKindButton,
                   "close", "Close", 403, 213, 50, 14,
                   FALSE, FALSE, 1);
    complete = close != NULL && close->SetStyleFlags(ButtonDefault) && complete;

    if (!complete)
        return FALSE;
    dialog.ShowModal();
    return TRUE;
}

void WINAPI CloseAllNativeDialogs()
{
    ClosingAllNativeDialogs = TRUE;
    while (!OpenNotificationWindows.empty())
    {
        HWND window = OpenNotificationWindows.back();
        const size_t previousCount = OpenNotificationWindows.size();
        if (IsWindow(window) && !DestroyWindow(window))
        {
            // The public UI contract is main-thread-only, so this is only a
            // defensive fallback. Detach the module-owned procedure rather
            // than allowing a live HWND to call into the unloaded provider.
            KillTimer(window, 1);
            NotificationData* data =
                reinterpret_cast<NotificationData*>(GetWindowLongPtrW(
                    window, GWLP_USERDATA));
            SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            SetWindowLongPtrW(
                window,
                GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(DefWindowProcW));
            delete data;
        }
        if (OpenNotificationWindows.size() == previousCount &&
            OpenNotificationWindows.back() == window)
        {
            // Either the handle was already gone or its window procedure was
            // detached by the defensive fallback above.
            OpenNotificationWindows.pop_back();
        }
    }
    if (NotificationInstance != NULL)
    {
        UnregisterClassW(NotificationClassName, NotificationInstance);
        NotificationInstance = NULL;
    }
    while (!OpenNativeDialogs.empty())
    {
        NativeDialog* dialog = OpenNativeDialogs.back();
        const size_t previousCount = OpenNativeDialogs.size();
        dialog->Close();
        if (OpenNativeDialogs.size() == previousCount &&
            OpenNativeDialogs.back() == dialog)
        {
            // Avoid retaining a window procedure from this DLL even if an
            // externally destroyed dialog failed to deliver WM_NCDESTROY.
            OpenNativeDialogs.pop_back();
        }
    }
    ClosingAllNativeDialogs = FALSE;
}

INT_PTR CALLBACK NativeDialog::DialogProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    NativeDialog* dialog = reinterpret_cast<NativeDialog*>(
        GetWindowLongPtr(hwnd, DWLP_USER));
    if (message == WM_INITDIALOG)
    {
        dialog = reinterpret_cast<NativeDialog*>(lParam);
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        dialog->m_pImpl->Window = hwnd;
        RegisterNativeDialog(dialog);
        INativeDialogHost* host = GetNativeDialogHost();
        dialog->m_pImpl->CurrentDpi = host != NULL
                                         ? host->GetWindowDpi(hwnd)
                                         : 96;
        if (dialog->m_pImpl->CurrentDpi == 0)
            dialog->m_pImpl->CurrentDpi = 96;
        if (host != NULL)
            host->PrepareTheme();
        ApplyNativeDialogDarkMode(hwnd);
        dialog->m_pImpl->AccessibilityTooltip = CreateWindowExW(
            WS_EX_TRANSPARENT,
            TOOLTIPS_CLASSW,
            NULL,
            WS_POPUP | TTS_ALWAYSTIP,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            hwnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        HWND initialFocus = NULL;
        for (size_t index = 0; index < dialog->m_pImpl->Controls.size(); ++index)
        {
            Impl::Control* control = dialog->m_pImpl->Controls[index];
            HWND child = GetDlgItem(hwnd, control->NumericId);
            if (child == NULL)
                continue;
            control->WindowHandle = child;
            if (initialFocus == NULL &&
                control->Kind != ControlKindLabel &&
                control->Kind != ControlKindStaticText &&
                control->Kind != ControlKindProgressBar &&
                control->Kind != ControlKindToolbarHeader &&
                control->Kind != ControlKindGroupBox &&
                control->Kind != ControlKindSplitter)
                initialFocus = child;
            if (control->Kind == ControlKindSplitter)
                SetWindowSubclass(
                    child, Impl::SplitterSubclassProc,
                    control->NumericId,
                    reinterpret_cast<DWORD_PTR>(control));
            if (control->Kind == ControlKindFilePicker)
                control->BrowseWindowHandle =
                    GetDlgItem(hwnd, control->BrowseNumericId);
            control->SetEnabled(control->Enabled);
            dialog->m_pImpl->AddAccessibilityTooltip(
                dialog->m_pImpl->AccessibilityTooltip,
                child,
                control);
            if (control->Kind == ControlKindFilePicker)
                dialog->m_pImpl->AddAccessibilityTooltip(
                    dialog->m_pImpl->AccessibilityTooltip,
                    control->BrowseWindowHandle,
                    control);
            std::wstring text;
            if (Utf8ToWide(control->Text.c_str(), text))
                SetWindowTextW(child, text.c_str());
            if (host != NULL)
            {
                const DWORD hostFlags = control->StyleFlags & 0x0000FFFF;
                if (control->Kind == ControlKindStaticText)
                {
                    control->StaticText = host->AttachStaticText(
                        hwnd, control->NumericId, hostFlags);
                    if (control->StaticText != NULL)
                    {
                        control->StaticText->SetPathSeparator(
                            control->PathSeparator);
                        if (!control->ToolTipText.empty())
                            control->StaticText->SetToolTipText(
                                control->ToolTipText.c_str());
                    }
                }
                else if (control->Kind == ControlKindHyperLink)
                {
                    control->HyperLink = host->AttachHyperLink(
                        hwnd, control->NumericId, hostFlags);
                    if (control->HyperLink != NULL)
                    {
                        if (!control->ActionTarget.empty())
                            control->HyperLink->SetActionOpen(
                                control->ActionTarget.c_str());
                        else if (control->ActionCommand != 0)
                            control->HyperLink->SetActionPostCommand(
                                control->ActionCommand);
                        else if (!control->ActionHint.empty())
                            control->HyperLink->SetActionShowHint(
                                control->ActionHint.c_str());
                        if (!control->ToolTipText.empty())
                            control->HyperLink->SetToolTipText(
                                control->ToolTipText.c_str());
                    }
                }
                else if (control->Kind == ControlKindProgressBar)
                {
                    control->ProgressBar = host->AttachProgressBar(
                        hwnd, control->NumericId);
                    if (control->ProgressBar != NULL)
                    {
                        control->ProgressBar->SetSelfMoveTime(
                            control->IndeterminateDuration);
                        control->ProgressBar->SetSelfMoveSpeed(
                            control->IndeterminateInterval);
                        if (control->HasProgressValues)
                        {
                            CQuadWord currentValue;
                            CQuadWord totalValue;
                            currentValue.SetUI64(control->ProgressCurrent);
                            totalValue.SetUI64(control->ProgressTotal);
                            control->ProgressBar->SetProgress2(
                                currentValue, totalValue,
                                control->ProgressText.empty()
                                    ? NULL
                                    : control->ProgressText.c_str());
                        }
                        else
                        {
                            control->ProgressBar->SetProgress(
                                static_cast<DWORD>(control->ProgressValue),
                                control->ProgressText.empty()
                                    ? NULL
                                    : control->ProgressText.c_str());
                        }
                    }
                }
                else if (control->Kind == ControlKindArrowButton)
                {
                    host->ChangeToArrowButton(
                        hwnd, control->NumericId);
                }
                else if (control->Kind == ControlKindTextArrowButton)
                {
                    control->TextArrowButton = host->AttachButton(
                        hwnd, control->NumericId, hostFlags);
                    if (control->TextArrowButton != NULL &&
                        !control->ToolTipText.empty())
                    {
                        control->TextArrowButton->SetToolTipText(
                            control->ToolTipText.c_str());
                    }
                }
                else if (control->Kind == ControlKindColorArrowButton)
                {
                    control->ColorArrowButton =
                        host->AttachColorArrowButton(
                            hwnd, control->NumericId, TRUE);
                    if (control->ColorArrowButton != NULL)
                        control->ColorArrowButton->SetColor(
                            control->TextColor,
                            control->BackgroundColor);
                }
            }
            if (control->Kind == ControlKindCheckBox ||
                control->Kind == ControlKindRadioButton)
                SendMessage(child, BM_SETCHECK,
                            control->Checked ? BST_CHECKED : BST_UNCHECKED, 0);
            if (control->Kind == ControlKindComboBox)
            {
                for (size_t itemIndex = 0;
                     itemIndex < control->Items.size(); ++itemIndex)
                {
                    std::wstring itemText;
                    if (Utf8ToWide(control->Items[itemIndex].c_str(), itemText))
                        SendMessageW(child, CB_ADDSTRING, 0,
                                     reinterpret_cast<LPARAM>(itemText.c_str()));
                }
                if (control->SelectedIndex >= 0 &&
                    control->SelectedIndex < static_cast<int>(control->Items.size()))
                    SendMessage(child, CB_SETCURSEL,
                                static_cast<WPARAM>(control->SelectedIndex), 0);
            }
            else if (control->Kind == ControlKindListView)
            {
                const size_t columnCount =
                    control->ColumnTitles.empty() &&
                            (control->StyleFlags &
                             ListViewNoDefaultColumn) == 0
                        ? 1
                        : control->ColumnTitles.size();
                for (size_t columnIndex = 0;
                     columnIndex < columnCount; ++columnIndex)
                {
                    std::wstring columnText;
                    int columnWidth = 220;
                    if (!control->ColumnTitles.empty())
                    {
                        if (!Utf8ToWide(
                                control->ColumnTitles[columnIndex].c_str(),
                                columnText))
                            continue;
                        columnWidth = control->ColumnWidths[columnIndex];
                    }
                    LVCOLUMNW column;
                    memset(&column, 0, sizeof(column));
                    column.mask = LVCF_TEXT | LVCF_WIDTH;
                    column.cx = columnWidth;
                    column.pszText = control->ColumnTitles.empty()
                                         ? const_cast<wchar_t*>(L"Item")
                                         : const_cast<wchar_t*>(columnText.c_str());
                    SendMessageW(
                        child,
                        LVM_INSERTCOLUMNW,
                        static_cast<WPARAM>(columnIndex),
                        reinterpret_cast<LPARAM>(&column));
                }
                for (size_t itemIndex = 0;
                     itemIndex < control->Items.size(); ++itemIndex)
                {
                    std::wstring itemText;
                    if (!Utf8ToWide(control->Items[itemIndex].c_str(), itemText))
                        continue;
                    LVITEMW item;
                    memset(&item, 0, sizeof(item));
                    item.mask = LVIF_TEXT;
                    item.iItem = static_cast<int>(itemIndex);
                    item.pszText = const_cast<wchar_t*>(itemText.c_str());
                    SendMessageW(child, LVM_INSERTITEMW, 0,
                                 reinterpret_cast<LPARAM>(&item));
                }
                if (control->SelectedIndex >= 0 &&
                    control->SelectedIndex < static_cast<int>(control->Items.size()))
                {
                    LVITEMW selected;
                    memset(&selected, 0, sizeof(selected));
                    selected.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                    selected.state = LVIS_SELECTED | LVIS_FOCUSED;
                    SendMessageW(
                        child,
                        LVM_SETITEMSTATE,
                        static_cast<WPARAM>(control->SelectedIndex),
                        reinterpret_cast<LPARAM>(&selected));
                }
            }
            else if (control->Kind == ControlKindTreeView)
            {
                std::vector<HTREEITEM> treeItems;
                for (size_t itemIndex = 0;
                     itemIndex < control->Items.size(); ++itemIndex)
                {
                    std::wstring itemText;
                    if (!Utf8ToWide(control->Items[itemIndex].c_str(), itemText))
                        continue;
                    TVINSERTSTRUCTW item;
                    memset(&item, 0, sizeof(item));
                    int parentIndex = control->ItemParents[itemIndex];
                    item.hParent = parentIndex >= 0 &&
                                           parentIndex < static_cast<int>(treeItems.size())
                                       ? treeItems[parentIndex]
                                       : TVI_ROOT;
                    item.hInsertAfter = TVI_LAST;
                    item.item.mask = TVIF_TEXT;
                    item.item.pszText = const_cast<wchar_t*>(itemText.c_str());
                    HTREEITEM inserted = reinterpret_cast<HTREEITEM>(
                        SendMessageW(child, TVM_INSERTITEMW, 0,
                                     reinterpret_cast<LPARAM>(&item)));
                    treeItems.push_back(inserted);
                }
            }
            else if (control->Kind == ControlKindTabControl)
            {
                for (size_t itemIndex = 0;
                     itemIndex < control->Items.size(); ++itemIndex)
                {
                    std::wstring itemText;
                    if (!Utf8ToWide(control->Items[itemIndex].c_str(), itemText))
                        continue;
                    TCITEMW item;
                    memset(&item, 0, sizeof(item));
                    item.mask = TCIF_TEXT;
                    item.pszText = const_cast<wchar_t*>(itemText.c_str());
                    SendMessageW(child, TCM_INSERTITEMW,
                                 static_cast<WPARAM>(itemIndex),
                                 reinterpret_cast<LPARAM>(&item));
                }
            }
        }
        // Toolbar headers align to another control and therefore attach only
        // after every child handle has been collected.
        if (host != NULL)
        {
            for (size_t index = 0;
                 index < dialog->m_pImpl->Controls.size(); ++index)
            {
                Impl::Control* control = dialog->m_pImpl->Controls[index];
                if (control->Kind != ControlKindToolbarHeader ||
                    control->ToolbarAlignControlId.empty())
                    continue;
                Impl::Control* align = dialog->m_pImpl->Find(
                    control->ToolbarAlignControlId.c_str());
                if (align != NULL && align->WindowHandle != NULL)
                {
                    control->ToolbarHeader =
                        host->AttachToolbarHeader(
                            hwnd, control->NumericId,
                            align->WindowHandle,
                            control->ToolbarButtonMask);
                }
            }
        }
        // Host wrappers were attached after the initial dialog theme pass.
        // Re-apply the full tree so every newly subclassed control receives
        // the selected Windows Dark Mode palette immediately.
        ApplyNativeDialogDarkMode(hwnd);
        PostMessage(hwnd, WM_SALAMATRIX_APPLY_DARK_SCROLLBARS, 0, 0);
        if (initialFocus != NULL)
        {
            SetFocus(initialFocus);
            return FALSE;
        }
        return TRUE;
    }
    if (dialog == NULL || dialog->m_pImpl == NULL)
        return FALSE;
    if (message == WM_CLOSE)
    {
        dialog->m_pImpl->Result = 0;
        if (dialog->m_pImpl->AccessibilityTooltip != NULL)
        {
            DestroyWindow(dialog->m_pImpl->AccessibilityTooltip);
            dialog->m_pImpl->AccessibilityTooltip = NULL;
        }
        if (dialog->m_pImpl->Options.Modeless || ClosingAllNativeDialogs)
            DestroyWindow(hwnd);
        else
            EndDialog(hwnd, 0);
        return TRUE;
    }
    if (message == WM_NCDESTROY)
    {
        UnregisterNativeDialog(dialog);
        dialog->m_pImpl->ApplyDarkScrollbarScopes(FALSE);
        dialog->m_pImpl->Window = NULL;
        dialog->m_pImpl->Running = FALSE;
        DialogCloseCallback callback = dialog->m_pImpl->CloseCallback;
        void* context = dialog->m_pImpl->CloseContext;
        dialog->m_pImpl->CloseCallback = NULL;
        dialog->m_pImpl->CloseContext = NULL;
        if (callback != NULL)
            callback(context, dialog);
        return FALSE;
    }
    if (message == WM_SETTINGCHANGE || message == WM_THEMECHANGED)
    {
        INativeDialogHost* host = GetNativeDialogHost();
        if (host != NULL)
            host->PrepareTheme();
        if (host != NULL && host->HandleThemeChange(message, lParam))
        {
            ApplyNativeDialogDarkMode(hwnd);
            PostMessage(hwnd, WM_SALAMATRIX_APPLY_DARK_SCROLLBARS, 0, 0);
        }
        return FALSE;
    }
    if (message == WM_SALAMATRIX_APPLY_DARK_SCROLLBARS)
    {
        dialog->m_pImpl->ApplyDarkScrollbarScopes(
            GetNativeDialogHost() != NULL &&
                    GetNativeDialogHost()->IsDarkMode()
                ? TRUE
                : FALSE);
        return TRUE;
    }
    if (message == WM_SALAMATRIX_SPLITTER_MOVED)
    {
        Impl::Control* control =
            dialog->m_pImpl->Find(static_cast<WORD>(wParam));
        if (control != NULL && control->Kind == ControlKindSplitter)
        {
            char position[32];
            _snprintf_s(position, _countof(position), _TRUNCATE,
                        "%ld", static_cast<long>(lParam));
            control->Text.assign(position);
            dialog->m_pImpl->NotifyChanged(control);
        }
        return TRUE;
    }
    if (message == WM_GETMINMAXINFO && dialog->m_pImpl->Options.Resizable)
    {
        MINMAXINFO* limits = reinterpret_cast<MINMAXINFO*>(lParam);
        if (limits != NULL)
        {
            limits->ptMinTrackSize.x = 640;
            limits->ptMinTrackSize.y = 460;
        }
        return TRUE;
    }
    if (message == WM_SIZE && dialog->m_pImpl->ResizeCallback != NULL)
    {
        dialog->m_pImpl->ResizeCallback(
            dialog->m_pImpl->ResizeContext, dialog,
            LOWORD(lParam), HIWORD(lParam));
        RedrawWindow(hwnd, NULL, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        return TRUE;
    }
    if (message == WM_DPICHANGED)
    {
        UINT oldDpi = dialog->m_pImpl->CurrentDpi;
        UINT newDpi = HIWORD(wParam);
        if (newDpi == 0)
            newDpi = oldDpi;
        RECT* suggested = reinterpret_cast<RECT*>(lParam);
        if (suggested != NULL)
        {
            SetWindowPos(
                hwnd, NULL, suggested->left, suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        for (size_t index = 0; index < dialog->m_pImpl->Controls.size(); ++index)
        {
            Impl::Control* control = dialog->m_pImpl->Controls[index];
            HWND children[2] = {control->WindowHandle, control->BrowseWindowHandle};
            int childCount = control->BrowseWindowHandle != NULL ? 2 : 1;
            for (int childIndex = 0; childIndex < childCount; ++childIndex)
            {
                if (children[childIndex] == NULL)
                    continue;
                RECT childRect;
                if (!GetWindowRect(children[childIndex], &childRect))
                    continue;
                MapWindowPoints(
                    NULL, hwnd, reinterpret_cast<POINT*>(&childRect), 2);
                int x = ScaleDialogMetric(childRect.left, oldDpi, newDpi);
                int y = ScaleDialogMetric(childRect.top, oldDpi, newDpi);
                int width = ScaleDialogMetric(
                    childRect.right - childRect.left, oldDpi, newDpi);
                int height = ScaleDialogMetric(
                    childRect.bottom - childRect.top, oldDpi, newDpi);
                SetWindowPos(
                    children[childIndex], NULL, x, y, width, height,
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        dialog->m_pImpl->CurrentDpi = newDpi;
        return TRUE;
    }
    if (message == WM_NOTIFY)
    {
        NMHDR* header = reinterpret_cast<NMHDR*>(lParam);
        if (header == NULL)
            return FALSE;
        Impl::Control* notifyControl =
            dialog->m_pImpl->Find(static_cast<WORD>(header->idFrom));
        if (notifyControl == NULL)
            return FALSE;
        if (notifyControl->Kind == ControlKindListView &&
            header->code == LVN_ITEMCHANGED)
        {
            NMLISTVIEW* change = reinterpret_cast<NMLISTVIEW*>(lParam);
            if ((change->uNewState & LVIS_SELECTED) != 0)
            {
                notifyControl->SelectedIndex = change->iItem;
                dialog->m_pImpl->NotifyChanged(notifyControl);
            }
            else if ((change->uOldState & LVIS_SELECTED) != 0 &&
                     notifyControl->SelectedIndex == change->iItem)
            {
                notifyControl->SelectedIndex = -1;
                dialog->m_pImpl->NotifyChanged(notifyControl);
            }
        }
        else if (notifyControl->Kind == ControlKindTreeView &&
                 (header->code == TVN_SELCHANGEDW ||
                  header->code == TVN_SELCHANGEDA))
        {
            notifyControl->SelectedIndex = -1;
            dialog->m_pImpl->NotifyChanged(notifyControl);
        }
        else if (notifyControl->Kind == ControlKindTabControl &&
                 header->code == TCN_SELCHANGE)
        {
            notifyControl->SelectedIndex = TabCtrl_GetCurSel(header->hwndFrom);
            dialog->m_pImpl->NotifyChanged(notifyControl);
        }
        return TRUE;
    }
    if (message != WM_COMMAND)
        return FALSE;
    Impl::Control* control = dialog->m_pImpl->Find(LOWORD(wParam));
    if (control == NULL)
        return FALSE;
    if (control->Kind == ControlKindFolderPicker)
    {
        std::string selectedPath;
        if (PickFolderPath(
                hwnd, dialog->m_pImpl->Title.c_str(), selectedPath))
        {
            control->Text = selectedPath;
            std::wstring selectedWide;
            if (Utf8ToWide(control->Text.c_str(), selectedWide))
                SetWindowTextW(GetDlgItem(hwnd, control->NumericId), selectedWide.c_str());
            dialog->m_pImpl->NotifyChanged(control);
        }
        return TRUE;
    }
    if (control->Kind == ControlKindFilePicker &&
        LOWORD(wParam) == control->BrowseNumericId)
    {
        std::string selectedPath;
        if (PickEditableFilePath(
                hwnd, dialog->m_pImpl->Title.c_str(),
                control->Text.c_str(),
                control->FileFilter.c_str(),
                control->FileSave,
                selectedPath))
        {
            control->Text = selectedPath;
            std::wstring selectedWide;
            if (Utf8ToWide(control->Text.c_str(), selectedWide))
                SetWindowTextW(
                    GetDlgItem(hwnd, control->NumericId),
                    selectedWide.c_str());
            dialog->m_pImpl->NotifyChanged(control);
        }
        return TRUE;
    }
    if (control->Kind == ControlKindButton)
    {
        if (control->DialogResult != IDCANCEL)
        {
            Impl::Control* invalid = dialog->m_pImpl->FindInvalid();
            if (invalid != NULL)
            {
                std::string validationMessage =
                    invalid->ValidationMessage.empty()
                        ? "This field is required."
                        : invalid->ValidationMessage;
                std::wstring messageWide;
                std::wstring titleWide;
                if (!Utf8ToWide(validationMessage.c_str(), messageWide))
                    messageWide = L"This field is required.";
                if (!Utf8ToWide(dialog->m_pImpl->Title.c_str(), titleWide))
                    titleWide = L"Salamander";
                ShowHostAwareMessageBox(
                    hwnd, messageWide.c_str(), titleWide.c_str(), MB_OK | MB_ICONWARNING);
                SetFocus(GetDlgItem(hwnd, invalid->NumericId));
                return TRUE;
            }
        }
        dialog->m_pImpl->NotifyChanged(control);
        // A zero dialog result denotes an action button. This also preserves
        // keep-open behavior for the legacy Automation COM facade, whose
        // historical add() signature predates the explicit KeepOpen option.
        if (control->KeepOpen || control->DialogResult == 0)
            return TRUE;
        dialog->m_pImpl->Result = control->DialogResult != 0
                                      ? control->DialogResult
                                      : IDOK;
        if (dialog->m_pImpl->Options.Modeless)
            DestroyWindow(hwnd);
        else
            EndDialog(hwnd, dialog->m_pImpl->Result);
        return TRUE;
    }
    if (control->Kind == ControlKindCheckBox ||
        control->Kind == ControlKindRadioButton)
    {
        control->Checked = SendMessage(
            GetDlgItem(hwnd, control->NumericId), BM_GETCHECK, 0, 0) == BST_CHECKED;
        dialog->m_pImpl->NotifyChanged(control);
    }
    if ((control->Kind == ControlKindTextBox ||
         control->Kind == ControlKindFilePicker) &&
        HIWORD(wParam) == EN_CHANGE)
    {
        wchar_t value[4096];
        GetWindowTextW(GetDlgItem(hwnd, control->NumericId), value, _countof(value));
        WideToUtf8(value, control->Text);
        dialog->m_pImpl->NotifyChanged(control);
    }
    if (control->Kind == ControlKindComboBox &&
        HIWORD(wParam) == CBN_SELCHANGE)
    {
        HWND combo = GetDlgItem(hwnd, control->NumericId);
        control->SelectedIndex = static_cast<int>(
            SendMessage(combo, CB_GETCURSEL, 0, 0));
        if (control->SelectedIndex >= 0 &&
            control->SelectedIndex < static_cast<int>(control->Items.size()))
            control->Text = control->Items[control->SelectedIndex];
        dialog->m_pImpl->NotifyChanged(control);
    }
    return TRUE;
}

} // namespace UI
} // namespace Salamatrix
