// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"
#include "salamatrix_ui.h"

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
} // namespace

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
        WORD NumericId;
        std::vector<std::string> Items;
        std::vector<int> ItemParents;

        Control(ControlKind kind, const ControlOptions& options, WORD numericId)
            : Kind(kind),
              Id(options.Id != NULL ? options.Id : ""),
              Text(options.Text != NULL ? options.Text : ""),
              ReadOnly(options.ReadOnly),
              Checked(options.Checked),
              DialogResult(options.DialogResult),
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
                (Kind == ControlKindTreeView &&
                 parentIndex >= static_cast<int>(Items.size())))
                return FALSE;
            Items.push_back(value);
            ItemParents.push_back(parentIndex);
            return TRUE;
        }

        virtual BOOL WINAPI ClearItems()
        {
            Items.clear();
            ItemParents.clear();
            return TRUE;
        }

        virtual int WINAPI GetItemCount() const
        {
            return static_cast<int>(Items.size());
        }
    };

    DialogOptions Options;
    std::string Title;
    std::vector<Control*> Controls;
    HWND Window;
    int Result;
    BOOL Running;

    explicit Impl(const DialogOptions& options)
        : Options(options),
          Title(options.Title != NULL ? options.Title : "Salamander"),
          Window(NULL),
          Result(0),
          Running(FALSE)
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
            if (Controls[index]->NumericId == numericId)
                return Controls[index];
        }
        return NULL;
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
    if (m_pImpl == NULL || m_pImpl->Running || m_pImpl->Controls.size() >= 64 ||
        (options.Id != NULL && m_pImpl->Find(options.Id) != NULL))
        return NULL;
    WORD numericId = static_cast<WORD>(2000 + m_pImpl->Controls.size());
    Impl::Control* control = new Impl::Control(kind, options, numericId);
    m_pImpl->Controls.push_back(control);
    return control;
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
    header->dwExtendedStyle = 0;
    header->cdit = static_cast<WORD>(m_pImpl->Controls.size());
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
        WORD classOrdinal = 0x0082; // STATIC
        short height = 14;
        short width = static_cast<short>(m_pImpl->Options.Width - 16);
        if (control->Kind == ControlKindTextBox)
        {
            classOrdinal = 0x0081; // EDIT
            style |= WS_BORDER | ES_AUTOHSCROLL;
            if (control->ReadOnly)
                style |= ES_READONLY;
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
        if (control->Kind == ControlKindButton)
            x = static_cast<short>(m_pImpl->Options.Width - 78);
        AppendItem(dialog, x, y, width, height, control->NumericId,
                   style, classOrdinal, text, className);
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
        for (size_t index = 0; index < dialog->m_pImpl->Controls.size(); ++index)
        {
            Impl::Control* control = dialog->m_pImpl->Controls[index];
            HWND child = GetDlgItem(hwnd, control->NumericId);
            if (child == NULL)
                continue;
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
                LVCOLUMNW column;
                memset(&column, 0, sizeof(column));
                column.mask = LVCF_TEXT | LVCF_WIDTH;
                column.cx = 220;
                column.pszText = const_cast<wchar_t*>(L"Item");
                SendMessageW(child, LVM_INSERTCOLUMNW, 0,
                             reinterpret_cast<LPARAM>(&column));
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
    if (message != WM_COMMAND)
        return FALSE;
    Impl::Control* control = dialog->m_pImpl->Find(LOWORD(wParam));
    if (control == NULL)
        return FALSE;
    if (control->Kind == ControlKindButton)
    {
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
    }
    if (control->Kind == ControlKindTextBox &&
        HIWORD(wParam) == EN_CHANGE)
    {
        wchar_t value[4096];
        GetWindowTextW(GetDlgItem(hwnd, control->NumericId), value, _countof(value));
        WideToUtf8(value, control->Text);
    }
    return TRUE;
}

} // namespace UI
} // namespace Salamatrix
