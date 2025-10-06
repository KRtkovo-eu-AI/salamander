#include "precomp.h"
#include "eventviewerwindow.h"

namespace
{
const int kMaxEventsToDisplay = 512;

struct TreeDefinition
{
    int ParentIndex;
    int TextResourceId;
    const wchar_t* LogName;
};

const TreeDefinition kTreeItems[] = {
    {-1, IDS_TREE_WINDOWS_LOGS, NULL},
    {0, IDS_LOG_APPLICATION, L"Application"},
    {0, IDS_LOG_SECURITY, L"Security"},
    {0, IDS_LOG_SETUP, L"Setup"},
    {0, IDS_LOG_SYSTEM, L"System"},
    {0, IDS_LOG_FORWARD, L"ForwardedEvents"},
};
}

CEventViewerWindow::CEventViewerWindow()
    : HWindow(NULL)
    , TreeView(NULL)
    , ListView(NULL)
    , DetailsEdit(NULL)
    , StatusBar(NULL)
{
}

CEventViewerWindow::~CEventViewerWindow()
{
    Close();
}

bool CEventViewerWindow::Create(HWND parent)
{
    if (IsCreated())
        return true;

    HWND window = CreateDialogParam(GetLanguageResourceHandle(), MAKEINTRESOURCE(IDD_EVENT_VIEWER), parent, DialogProc,
                                    reinterpret_cast<LPARAM>(this));
    if (window == NULL)
        return false;

    HWindow = window;
    return true;
}

void CEventViewerWindow::Show()
{
    if (!IsCreated())
        return;

    ShowWindow(HWindow, SW_SHOWNORMAL);
    SetForegroundWindow(HWindow);
}

void CEventViewerWindow::Close()
{
    if (HWindow != NULL)
    {
        DestroyWindow(HWindow);
        HWindow = NULL;
        TreeView = NULL;
        ListView = NULL;
        DetailsEdit = NULL;
        StatusBar = NULL;
        Records.clear();
        ActiveLog.clear();
    }
}

INT_PTR CALLBACK CEventViewerWindow::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CEventViewerWindow* self = nullptr;
    if (msg == WM_INITDIALOG)
    {
        self = reinterpret_cast<CEventViewerWindow*>(lParam);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self)
        {
            self->HWindow = hwnd;
            return self->HandleMessage(msg, wParam, lParam);
        }
        return FALSE;
    }

    self = reinterpret_cast<CEventViewerWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!self)
    {
        return FALSE;
    }

    return self->HandleMessage(msg, wParam, lParam);
}

INT_PTR CEventViewerWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        InitializeControls();
        InitializeTree();
        return TRUE;

    case WM_SIZE:
        UpdateLayout(LOWORD(lParam), HIWORD(lParam));
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDCANCEL)
        {
            Close();
            return TRUE;
        }
        break;

    case WM_CLOSE:
        Close();
        return TRUE;

    case WM_DESTROY:
        HWindow = NULL;
        TreeView = NULL;
        ListView = NULL;
        DetailsEdit = NULL;
        StatusBar = NULL;
        Records.clear();
        ActiveLog.clear();
        TreeItemStorage.clear();
        return TRUE;

    case WM_NOTIFY:
    {
        NMHDR* hdr = reinterpret_cast<NMHDR*>(lParam);
        if (hdr->hwndFrom == TreeView && hdr->code == TVN_SELCHANGED)
        {
            NMTREEVIEW* tv = reinterpret_cast<NMTREEVIEW*>(lParam);
            const std::wstring* data = reinterpret_cast<std::wstring*>(tv->itemNew.lParam);
            if (data != NULL)
                RefreshLog(*data);
            else
                RefreshLog(std::wstring());
            return TRUE;
        }
        if (hdr->hwndFrom == ListView && hdr->code == LVN_ITEMCHANGED)
        {
            NMLISTVIEW* lv = reinterpret_cast<NMLISTVIEW*>(lParam);
            if ((lv->uChanged & LVIF_STATE) != 0 && (lv->uNewState & LVIS_SELECTED) != 0)
            {
                DisplayRecordDetails(lv->iItem);
            }
            return TRUE;
        }
        break;
    }
    }

    return FALSE;
}

void CEventViewerWindow::InitializeControls()
{
    if (HWindow != NULL)
        SetWindowTextA(HWindow, LoadStr(IDS_PLUGINNAME));

    TreeView = GetDlgItem(HWindow, IDC_EVENT_TREE);
    ListView = GetDlgItem(HWindow, IDC_EVENT_LIST);
    DetailsEdit = GetDlgItem(HWindow, IDC_EVENT_DETAILS);
    StatusBar = GetDlgItem(HWindow, IDC_EVENT_STATUS);

    if (ListView != NULL)
    {
        ListView_SetExtendedListViewStyle(ListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;

        column.cx = 110;
        column.pszText = LoadStr(IDS_COLUMN_LEVEL);
        column.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(ListView, 0, &column);

        column.cx = 160;
        column.pszText = LoadStr(IDS_COLUMN_TIME);
        ListView_InsertColumn(ListView, 1, &column);

        column.cx = 160;
        column.pszText = LoadStr(IDS_COLUMN_SOURCE);
        ListView_InsertColumn(ListView, 2, &column);

        column.cx = 80;
        column.pszText = LoadStr(IDS_COLUMN_EVENTID);
        ListView_InsertColumn(ListView, 3, &column);

        column.cx = 120;
        column.pszText = LoadStr(IDS_COLUMN_TASK);
        ListView_InsertColumn(ListView, 4, &column);
    }

    if (DetailsEdit != NULL)
    {
        SendMessage(DetailsEdit, EM_SETREADONLY, TRUE, 0);
    }

    if (StatusBar != NULL)
    {
        UpdateStatus(LoadStr(IDS_STATUS_READY));
    }
}

void CEventViewerWindow::InitializeTree()
{
    if (TreeView == NULL)
        return;

    TVINSERTSTRUCT insert = {};
    std::vector<HTREEITEM> handles(_countof(kTreeItems));
    TreeItemStorage.clear();
    TreeItemStorage.reserve(_countof(kTreeItems));
    HTREEITEM firstLeaf = NULL;

    for (size_t i = 0; i < _countof(kTreeItems); ++i)
    {
        const TreeDefinition& def = kTreeItems[i];
        insert.hParent = def.ParentIndex >= 0 ? handles[def.ParentIndex] : TVI_ROOT;
        insert.hInsertAfter = TVI_LAST;
        insert.item.mask = TVIF_TEXT | TVIF_PARAM;
        insert.item.pszText = LoadStr(def.TextResourceId);
        insert.item.lParam = 0;

        if (def.LogName != NULL)
        {
            auto storage = std::make_unique<std::wstring>(def.LogName);
            insert.item.lParam = reinterpret_cast<LPARAM>(storage.get());
            TreeItemStorage.push_back(std::move(storage));
        }

        handles[i] = TreeView_InsertItem(TreeView, &insert);
        if (firstLeaf == NULL && def.LogName != NULL)
            firstLeaf = handles[i];
    }

    if (!ActiveLog.empty())
    {
        RefreshLog(ActiveLog);
    }
    else if (firstLeaf != NULL)
    {
        TreeView_SelectItem(TreeView, firstLeaf);
    }
}

void CEventViewerWindow::PopulateList()
{
    if (ListView == NULL)
        return;

    ListView_DeleteAllItems(ListView);

    for (size_t i = 0; i < Records.size(); ++i)
    {
        const EventLogRecord& rec = Records[i];
        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = static_cast<int>(i);
        item.pszText = const_cast<char*>(rec.Level.c_str());
        item.lParam = static_cast<LPARAM>(i);
        int index = ListView_InsertItem(ListView, &item);
        if (index >= 0)
        {
            ListView_SetItemText(ListView, index, 1, const_cast<char*>(rec.TimeCreated.c_str()));
            ListView_SetItemText(ListView, index, 2, const_cast<char*>(rec.Source.c_str()));
            ListView_SetItemText(ListView, index, 3, const_cast<char*>(rec.EventId.c_str()));
            ListView_SetItemText(ListView, index, 4, const_cast<char*>(rec.TaskCategory.c_str()));
        }
    }

    if (!Records.empty())
    {
        ListView_SetItemState(ListView, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        DisplayRecordDetails(0);
    }
    else
    {
        DisplayRecordDetails(-1);
    }
}

void CEventViewerWindow::RefreshLog(const std::wstring& logName)
{
    ActiveLog = logName;
    Records.clear();

    if (ActiveLog.empty())
    {
        PopulateList();
        UpdateStatus(LoadStr(IDS_STATUS_SELECT_LOG));
        return;
    }

    std::string error;
    if (Reader.Query(ActiveLog.c_str(), kMaxEventsToDisplay, Records, error))
    {
        PopulateList();
        UpdateStatus(LoadStr(IDS_STATUS_UPDATED));
    }
    else
    {
        PopulateList();
        std::string message = LoadStr(IDS_STATUS_ERROR);
        if (!error.empty())
        {
            message += " ";
            message += error;
        }
        UpdateStatus(message);
    }
}

void CEventViewerWindow::DisplayRecordDetails(int index)
{
    if (DetailsEdit == NULL)
        return;

    if (index < 0 || index >= static_cast<int>(Records.size()))
    {
        SetWindowTextA(DetailsEdit, "");
        return;
    }

    const EventLogRecord& record = Records[index];
    SetWindowTextA(DetailsEdit, record.Details.c_str());
}

void CEventViewerWindow::UpdateLayout(int width, int height)
{
    if (!IsCreated())
        return;

    RECT rect;
    GetClientRect(HWindow, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    const int margin = 6;
    const int statusHeight = 22;
    int treeWidth = max(220, width / 4);

    if (TreeView != NULL)
        MoveWindow(TreeView, margin, margin, treeWidth - margin, height - statusHeight - 2 * margin, TRUE);

    int rightX = treeWidth + margin;
    int rightWidth = width - rightX - margin;
    int rightHeight = height - statusHeight - 2 * margin;

    int listHeight = (rightHeight * 3) / 5;
    int detailsY = margin + listHeight + margin / 2;
    int detailsHeight = rightHeight - listHeight - margin / 2;

    if (ListView != NULL)
        MoveWindow(ListView, rightX, margin, rightWidth, listHeight, TRUE);

    if (DetailsEdit != NULL)
        MoveWindow(DetailsEdit, rightX, detailsY, rightWidth, max(detailsHeight, 60), TRUE);

    if (StatusBar != NULL)
        MoveWindow(StatusBar, margin, height - statusHeight, width - 2 * margin, statusHeight, TRUE);
}

void CEventViewerWindow::UpdateStatus(const std::string& text)
{
    if (StatusBar != NULL)
        SetWindowTextA(StatusBar, text.c_str());
}
