// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_ui.h"
#include "salamatrix_ui_layout.h"
#include "../../third_party/darkmodelib/src/DmlibDpi.h"

#include <vector>

namespace Salamatrix
{
namespace UI
{
namespace
{
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

    const wchar_t filter[] = L"All files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = parent;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = &path[0];
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrTitle = titleWide.c_str();
    dialog.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog))
        return FALSE;
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
              NumericId(numericId)
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
            if (WindowHandle != NULL)
            {
                std::wstring wide;
                if (Utf8ToWide(Text.c_str(), wide))
                    SetWindowTextW(WindowHandle, wide.c_str());
            }
            return TRUE;
        }

        virtual BOOL WINAPI GetChecked() const { return Checked; }

        virtual BOOL WINAPI SetChecked(BOOL checked)
        {
            Checked = checked;
            return TRUE;
        }

        virtual int WINAPI GetDialogResult() const { return DialogResult; }

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
    };

    DialogOptions Options;
    std::string Title;
    std::vector<Control*> Controls;
    HWND Window;
    int Result;
    BOOL Running;
    UINT CurrentDpi;
    DialogEventCallback EventCallback;
    void* EventContext;

    explicit Impl(const DialogOptions& options)
        : Options(options),
          Title(options.Title != NULL ? options.Title : "Salamander"),
          Window(NULL),
          Result(0),
          Running(FALSE),
          CurrentDpi(96),
          EventCallback(NULL),
          EventContext(NULL)
    {
    }

    ~Impl()
    {
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
    return SALAMATRIX_UI_VERSION_1_0;
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
                    DS_MODALFRAME | DS_SETFONT;
    // Let dialog-manager keyboard navigation traverse child controls,
    // including the two controls composing an editable file picker.
    header->dwExtendedStyle = WS_EX_CONTROLPARENT;
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
        if (control->Kind != ControlKindLabel)
            style |= WS_TABSTOP;
        WORD classOrdinal = 0x0082; // STATIC
        short height = 14;
        short width = static_cast<short>(m_pImpl->Options.Width - 16);
        if (control->Kind == ControlKindTextBox ||
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
            style |= BS_PUSHBUTTON;
            width = 70;
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
    DialogBoxIndirectParamW(
        GetModuleHandle(NULL),
        reinterpret_cast<DLGTEMPLATE*>(&dialog[0]),
        m_pImpl->Options.Parent,
        DialogProc,
        reinterpret_cast<LPARAM>(this));
    m_pImpl->Window = NULL;
    m_pImpl->Running = FALSE;
    return m_pImpl->Result;
}

void WINAPI NativeDialog::Close()
{
    if (m_pImpl != NULL && m_pImpl->Window != NULL)
        EndDialog(m_pImpl->Window, 0);
}

void WINAPI NativeDialog::Release()
{
    delete this;
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
        dialog->m_pImpl->CurrentDpi = dmlib_dpi::GetDpiForWindow(hwnd);
        if (dialog->m_pImpl->CurrentDpi == 0)
            dialog->m_pImpl->CurrentDpi = 96;
        DarkModeApplyTree(hwnd);
        DarkModeRefreshTitleBar(hwnd);
        DarkModeApplyStaticTextColors(hwnd, NULL);
        HWND initialFocus = NULL;
        for (size_t index = 0; index < dialog->m_pImpl->Controls.size(); ++index)
        {
            Impl::Control* control = dialog->m_pImpl->Controls[index];
            HWND child = GetDlgItem(hwnd, control->NumericId);
            if (child == NULL)
                continue;
            control->WindowHandle = child;
            if (initialFocus == NULL && control->Kind != ControlKindLabel)
                initialFocus = child;
            if (control->Kind == ControlKindFilePicker)
                control->BrowseWindowHandle =
                    GetDlgItem(hwnd, control->BrowseNumericId);
            std::wstring text;
            if (Utf8ToWide(control->Text.c_str(), text))
                SetWindowTextW(child, text.c_str());
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
            }
            else if (control->Kind == ControlKindListView)
            {
                const size_t columnCount = control->ColumnTitles.empty()
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
        EndDialog(hwnd, 0);
        return TRUE;
    }
    if (message == WM_SETTINGCHANGE)
    {
        if (DarkModeHandleSettingChange(message, lParam))
        {
            DarkModeApplyTree(hwnd);
            DarkModeRefreshTitleBar(hwnd);
            DarkModeApplyStaticTextColors(hwnd, NULL);
        }
        return FALSE;
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
                hwnd, dialog->m_pImpl->Title.c_str(), control->Text.c_str(),
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
                std::string message = invalid->ValidationMessage.empty()
                                          ? "This field is required."
                                          : invalid->ValidationMessage;
                std::wstring messageWide;
                std::wstring titleWide;
                if (!Utf8ToWide(message.c_str(), messageWide))
                    messageWide = L"This field is required.";
                if (!Utf8ToWide(dialog->m_pImpl->Title.c_str(), titleWide))
                    titleWide = L"Salamander";
                MessageBoxW(
                    hwnd,
                    messageWide.c_str(),
                    titleWide.c_str(),
                    MB_OK | MB_ICONWARNING);
                SetFocus(GetDlgItem(hwnd, invalid->NumericId));
                return TRUE;
            }
        }
        dialog->m_pImpl->NotifyChanged(control);
        if (control->KeepOpen)
            return TRUE;
        dialog->m_pImpl->Result = control->DialogResult != 0
                                      ? control->DialogResult
                                      : IDOK;
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
