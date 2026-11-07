// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-07 | Bridge Find's native list and its virtualized UI Automation view. | Read data without walking native controls, and marshal only explicit actions to the dialog. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | GPT-6 (Codex) |

#include "precomp.h"
#include "cfgdlg.h"
#include "find.h"
#include "finduia.h"

#include <oleacc.h>
#include <UIAutomation.h>
#include <algorithm>

#pragma comment(lib, "uiautomationcore.lib")

namespace
{
// The native owner-data list supports up to 100,000,000 items. Pack an
// operation and an index into WPARAM without passing any address to a window.
constexpr UINT FindUiaActionMessage = WM_APP + 216;
constexpr UINT FindUiaInvokeMessage = WM_APP + 217;
constexpr unsigned ActionIndexBits = 27;
constexpr WPARAM ActionIndexMask = (WPARAM(1) << ActionIndexBits) - 1;
constexpr int ActionSignedLimit = 1 << (ActionIndexBits - 1);
constexpr LRESULT ActionDisabled = -1;
std::atomic<UINT_PTR> NextRevision{1};
constexpr UINT ColumnNames[] = {IDS_FOUNDFILESCOLUMN1, IDS_FOUNDFILESCOLUMN2, IDS_FOUNDFILESCOLUMN3,
                                IDS_FOUNDFILESCOLUMN4, IDS_FOUNDFILESCOLUMN5, IDS_FOUNDFILESCOLUMN6};
static_assert(_countof(ColumnNames) == FindUiaColumnCount);

std::wstring LoadUiaName(UINT resource)
{
    wchar_t text[128] = {};
    const int length = LoadStringW(HLanguage, resource, text, _countof(text));
    return std::wstring(text, length);
}

std::wstring ReadUiaSize(const CFoundFilesData& row)
{
    if (row.IsDir)
        return LoadUiaName(IDS_DIRCOLUMN);

    auto text = std::to_wstring(row.Size.Value);
    wchar_t separator[16] = {};
    if (GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, separator, _countof(separator)) <= 1)
        separator[0] = L' ';
    for (size_t pos = text.size(); pos > 3;)
    {
        pos -= 3;
        text.insert(pos, separator);
    }
    return text;
}

void ScreenRect(HWND window, RECT& rect)
{
    MapWindowPoints(window, NULL, reinterpret_cast<POINT*>(&rect), 2);
}

class RetainedRows
{
public:
    CFoundFilesData* Items[128] = {};
    ~RetainedRows()
    {
        for (auto* row : Items)
        {
            if (row == NULL)
                break;
            row->Release();
        }
    }
};
}

void CFoundFilesListView::InvalidateUiaRows()
{
    UiaRevision = NextRevision.fetch_add(1, std::memory_order_relaxed);
}

void CFoundFilesListView::RefreshUiaView()
{
    const auto state = UiaState;
    if (state)
        QueueFindUiaEvents(state, state->Refresh());
}

void CFoundFilesListView::ReleaseUiaView()
{
    if (UiaState)
        UiaState->Detach();
    if (UiaProvider != NULL)
    {
        IRawElementProviderSimple* provider = UiaProvider;
        UiaProvider = NULL;
        provider->Release();
    }
    UiaState.reset();
}

bool CFoundFilesListView::HandleUiaMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (message == WM_GETOBJECT && static_cast<LONG>(lParam) == UiaRootObjectId)
    {
        if (UiaClosing)
            return false;
        const HWND header = ListView_GetHeader(HWindow);
        if (header == NULL || Header_GetItemCount(header) != FindUiaColumnCount)
            return false;
        if (!UiaState)
        {
            try
            {
                UiaState = std::make_shared<CFindUiaState>(*this);
            }
            catch (const std::bad_alloc&)
            {
                return false;
            }
            const auto state = UiaState;
            state->Refresh();
        }
        if (UiaProvider == NULL)
            UiaProvider = CreateFindUiaProvider(UiaState);
        if (UiaProvider == NULL)
            return false;
        result = UiaReturnRawElementProvider(HWindow, wParam, lParam, UiaProvider);
        return result != 0;
    }
    if (message == FindUiaActionMessage || message == FindUiaInvokeMessage)
    {
        const bool queuedInvoke = message == FindUiaInvokeMessage;
        result = 0;
        if (!UiaState || UiaRevision != static_cast<UINT_PTR>(lParam))
            return true;

        const auto action = static_cast<CFindUiaAction>(wParam >> ActionIndexBits);
        if (queuedInvoke && action != CFindUiaAction::Invoke)
            return true;
        if (action != CFindUiaAction::QuerySelection && action != CFindUiaAction::FindSelection &&
            (!IsWindowEnabled(HWindow) || !IsWindowEnabled(FindDialog->HWindow)))
        {
            result = ActionDisabled;
            return true;
        }
        const WPARAM raw = wParam & ActionIndexMask;
        int index = raw == ActionIndexMask ? -1 : static_cast<int>(raw);
        if (action == CFindUiaAction::ScrollHorizontal || action == CFindUiaAction::ScrollVertical)
        {
            index = static_cast<int>(raw);
            if (index >= ActionSignedLimit)
                index -= ActionSignedLimit * 2;
        }
        const int count = ListView_GetItemCount(HWindow);
        switch (action)
        {
        case CFindUiaAction::SortColumn:
            if (index < 0 || index >= FindUiaColumnCount - 1)
                return true;
            SortItems(index);
            break;
        case CFindUiaAction::ScrollHorizontal:
            ListView_Scroll(HWindow, index, 0);
            break;
        case CFindUiaAction::ScrollVertical:
            ListView_Scroll(HWindow, 0, index);
            break;
        case CFindUiaAction::FindSelection:
            if (index < -1 || index >= count)
                return true;
            result = ListView_GetNextItem(HWindow, index, LVNI_SELECTED) + 2;
            return true;
        case CFindUiaAction::Focus:
            if (index == -1)
            {
                SetFocus(HWindow);
                break;
            }
            [[fallthrough]];
        case CFindUiaAction::ScrollIntoView:
        case CFindUiaAction::Select:
        case CFindUiaAction::AddSelection:
        case CFindUiaAction::RemoveSelection:
        case CFindUiaAction::Invoke:
        case CFindUiaAction::QuerySelection:
            if (index < 0 || index >= count)
                return true;
            if (action == CFindUiaAction::QuerySelection)
            {
                result = ListView_GetItemState(HWindow, index, LVIS_SELECTED) != 0 ? 2 : 1;
                return true;
            }
            if (action == CFindUiaAction::Invoke && !queuedInvoke)
            {
                // Invoke must return without waiting for the shell. Keep the
                // validated row and its revision with the queued operation.
                result = PostMessage(HWindow, FindUiaInvokeMessage, wParam, lParam);
                return true;
            }
            if (action == CFindUiaAction::Focus || action == CFindUiaAction::Invoke)
            {
                SetFocus(HWindow);
                ListView_SetItemState(HWindow, index, LVIS_FOCUSED, LVIS_FOCUSED);
            }
            if (action == CFindUiaAction::Select || action == CFindUiaAction::Invoke)
                ListView_SetItemState(HWindow, -1, 0, LVIS_SELECTED);
            if (action == CFindUiaAction::Select || action == CFindUiaAction::AddSelection ||
                action == CFindUiaAction::Invoke)
                ListView_SetItemState(HWindow, index, LVIS_SELECTED, LVIS_SELECTED);
            if (action == CFindUiaAction::RemoveSelection)
            {
                ListView_SetItemState(HWindow, index, 0, LVIS_SELECTED);
            }
            else
                ListView_EnsureVisible(HWindow, index, FALSE);
            if (queuedInvoke)
            {
                if (UiaRevision == static_cast<UINT_PTR>(lParam) &&
                    ListView_GetNextItem(HWindow, -1, LVNI_FOCUSED) == index)
                {
                    RetainedRows retained;
                    retained.Items[0] = At(index);
                    retained.Items[0]->Retain();
                    if (FindDialog->InitializeOle())
                    {
                        result = 1;
                        FindDialog->OpenResult(*retained.Items[0]);
                    }
                }
                return true;
            }
            break;
        default:
            return true;
        }
        RefreshUiaView();
        result = 1;
        return true;
    }
    if (message == WM_DESTROY || message == WM_NCDESTROY)
    {
        UiaClosing = true;
        if (UiaState)
        {
            ReleaseUiaView();
            UiaReturnRawElementProvider(HWindow, 0, 0, NULL);
        }
    }
    return false;
}

CFindUiaState::CFindUiaState(CFoundFilesListView& list) : List(&list)
{
}

void CFindUiaState::Detach()
{
    const std::lock_guard<std::mutex> guard(Mutex);
    List = NULL;
    View.reset();
}

std::shared_ptr<const CFindUiaView> CFindUiaState::ReadView() const
{
    const std::lock_guard<std::mutex> guard(Mutex);
    return View;
}

unsigned CFindUiaState::Refresh()
{
    // Only the window thread calls Refresh and Detach. Never hold Mutex
    // around SendMessage: a common control may re-enter WM_GETOBJECT.
    if (Refreshing || List == NULL)
        return 0;
    Refreshing = true;
    struct EndRefresh
    {
        bool& Active;
        ~EndRefresh() { Active = false; }
    } endRefresh{Refreshing};

    try
    {
        const auto before = ReadView();
        auto next = std::make_shared<CFindUiaView>();
        const HWND window = List->HWindow;
        next->Window = window;
        next->Revision = List->UiaRevision;
        next->Count = ListView_GetItemCount(window);
        next->Focus = ListView_GetNextItem(window, -1, LVNI_FOCUSED);
        next->SelectedTotal = ListView_GetSelectedCount(window);
        next->HasFocus = GetFocus() == window;
        next->Name = before ? before->Name : LoadUiaName(IDS_FF_NAME);
        GetClientRect(window, &next->Bounds);
        ScreenRect(window, next->Bounds);

        const HWND header = ListView_GetHeader(window);
        if (header != NULL)
        {
            for (int i = 0; i < FindUiaColumnCount; ++i)
                if (Header_GetItemRect(header, i, &next->Columns[i]))
                    ScreenRect(header, next->Columns[i]);
        }

        if (next->Count != 0 && IsWindowVisible(window))
        {
            next->First = ListView_GetTopIndex(window);
            if (next->First >= 0 && next->First < next->Count &&
                ListView_GetItemRect(window, next->First, &next->FirstRow, LVIR_BOUNDS))
            {
                ScreenRect(window, next->FirstRow);
                next->RowHeight = next->FirstRow.bottom - next->FirstRow.top;
                if (next->RowHeight > 0 && next->Bounds.bottom > next->FirstRow.top)
                {
                    const int height = next->Bounds.bottom - next->FirstRow.top;
                    next->Last = (std::min)(next->Count, next->First + (height - 1) / next->RowHeight + 1);
                    for (int row = next->First; row < next->Last; ++row)
                        next->Selected.push_back(ListView_GetItemState(window, row, LVIS_SELECTED) != 0);
                }
            }
            if (next->Last == 0)
                next->First = 0;
        }

        unsigned flags = 0;
        if (before)
        {
            if (before->Revision != next->Revision || before->Count != next->Count ||
                before->First != next->First || before->Last != next->Last)
                flags |= FindUiaStructureChanged;
            if (next->HasFocus && (!before->HasFocus || before->Focus != next->Focus))
                flags |= FindUiaFocusChanged;
            if (before->SelectedTotal != next->SelectedTotal ||
                (before->Revision == next->Revision && before->First == next->First &&
                 before->Last == next->Last && before->Selected != next->Selected))
                flags |= FindUiaSelectionChanged;
        }
        {
            const std::lock_guard<std::mutex> guard(Mutex);
            if (List != NULL)
                View = std::move(next);
        }
        return flags;
    }
    catch (const std::bad_alloc&)
    {
        return 0;
    }
}

bool CFindUiaState::AcquireRows(UINT_PTR revision, int first, int count, CFoundFilesData** rows) const
{
    const std::lock_guard<std::mutex> guard(Mutex);
    if (List == NULL)
        return false;
    HANDLES(EnterCriticalSection(&List->DataCriticalSection));
    const bool valid = revision == List->UiaRevision && first >= 0 &&
                       first <= List->Data.Count && count <= List->Data.Count - first;
    if (valid)
    {
        for (int i = 0; i < count; ++i)
        {
            rows[i] = List->Data[first + i];
            rows[i]->Retain();
        }
    }
    HANDLES(LeaveCriticalSection(&List->DataCriticalSection));
    return valid;
}

HRESULT CFindUiaState::ReadText(UINT_PTR revision, int row, int column, std::wstring& text) const
{
    if (column < 0 || column >= FindUiaColumnCount)
        return E_INVALIDARG;
    const auto view = ReadView();
    if (!view || revision != view->Revision || row < -1 || row >= view->Count)
        return UIA_E_ELEMENTNOTAVAILABLE;

    try
    {
        if (row == -1)
        {
            text = LoadUiaName(ColumnNames[column]);
            return S_OK;
        }
        RetainedRows retained;
        if (!AcquireRows(revision, row, 1, retained.Items))
            return UIA_E_ELEMENTNOTAVAILABLE;
        text = column == 2 ? ReadUiaSize(*retained.Items[0]) : retained.Items[0]->GetTextW(column, 0);
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
}

HRESULT CFindUiaState::FindName(UINT_PTR revision, int start, const wchar_t* name, int* index) const
{
    *index = -1;
    if (name == NULL || start < 0)
        return E_INVALIDARG;
    const auto view = ReadView();
    if (!view || revision != view->Revision)
        return UIA_E_ELEMENTNOTAVAILABLE;
    try
    {
        for (int first = start; first < view->Count;)
        {
            RetainedRows rows;
            const int n = (std::min)(view->Count - first, static_cast<int>(_countof(rows.Items)));
            if (!AcquireRows(revision, first, n, rows.Items))
                return UIA_E_ELEMENTNOTAVAILABLE;
            for (int i = 0; i < n; ++i)
            {
                const auto* row = rows.Items[i];
                const std::wstring fallback = row->NameW.empty() ? row->GetTextW(0, 0) : std::wstring();
                const auto& candidate = row->NameW.empty() ? fallback : row->NameW;
                if (CompareStringOrdinal(candidate.c_str(), -1, name, -1, TRUE) == CSTR_EQUAL)
                {
                    *index = first + i;
                    return S_OK;
                }
            }
            first += n;
        }
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
}

HRESULT CFindUiaState::SendAction(UINT_PTR revision, CFindUiaAction action, int index, DWORD_PTR& result) const
{
    const auto view = ReadView();
    if (!view || revision != view->Revision)
        return UIA_E_ELEMENTNOTAVAILABLE;
    const WPARAM packed = (static_cast<WPARAM>(action) << ActionIndexBits) |
                          (static_cast<WPARAM>(index) & ActionIndexMask);
    SetLastError(ERROR_SUCCESS);
    if (SendMessageTimeoutW(view->Window, FindUiaActionMessage, packed, static_cast<LPARAM>(revision),
                            SMTO_ABORTIFHUNG | SMTO_BLOCK | SMTO_ERRORONEXIT, 250, &result) == 0)
    {
        const DWORD error = GetLastError();
        return HRESULT_FROM_WIN32(error != ERROR_SUCCESS ? error : ERROR_TIMEOUT);
    }
    if (result == static_cast<DWORD_PTR>(ActionDisabled))
        return UIA_E_ELEMENTNOTENABLED;
    return result != 0 ? S_OK : UIA_E_ELEMENTNOTAVAILABLE;
}

HRESULT CFindUiaState::Act(UINT_PTR revision, CFindUiaAction action, int index) const
{
    if (action == CFindUiaAction::ScrollHorizontal || action == CFindUiaAction::ScrollVertical)
        index = (std::clamp)(index, -ActionSignedLimit, ActionSignedLimit - 1);
    DWORD_PTR result = 0;
    return SendAction(revision, action, index, result);
}

HRESULT CFindUiaState::IsSelected(UINT_PTR revision, int row, BOOL* value) const
{
    const auto view = ReadView();
    if (!view || revision != view->Revision || row < 0 || row >= view->Count)
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (row >= view->First && row < view->Last)
    {
        *value = view->Selected[row - view->First] ? TRUE : FALSE;
        return S_OK;
    }
    DWORD_PTR result = 0;
    const HRESULT hr = SendAction(revision, CFindUiaAction::QuerySelection, row, result);
    if (SUCCEEDED(hr))
        *value = result == 2 ? TRUE : FALSE;
    return hr;
}

HRESULT CFindUiaState::ReadSelection(UINT_PTR revision, std::vector<int>& rows) const
{
    const auto view = ReadView();
    if (!view || revision != view->Revision)
        return UIA_E_ELEMENTNOTAVAILABLE;
    try
    {
        rows.clear();
        for (int row = view->First; row < view->Last; ++row)
            if (view->Selected[row - view->First])
                rows.push_back(row);

        // Virtualized controls may report viewport selection for large sets.
        // FindItemByProperty(IsSelected) can still enumerate every selection.
        constexpr int CompleteSelectionLimit = 256;
        if (view->SelectedTotal <= CompleteSelectionLimit &&
            view->SelectedTotal != static_cast<int>(rows.size()))
        {
            rows.clear();
            for (int start = 0; static_cast<int>(rows.size()) < view->SelectedTotal;)
            {
                int index = -1;
                const HRESULT hr = FindSelected(revision, start, &index);
                if (FAILED(hr))
                    return hr;
                if (index < start)
                    break;
                rows.push_back(index);
                start = index + 1;
            }
        }
        return S_OK;
    }
    catch (const std::bad_alloc&)
    {
        return E_OUTOFMEMORY;
    }
}

HRESULT CFindUiaState::FindSelected(UINT_PTR revision, int start, int* index) const
{
    *index = -1;
    const auto view = ReadView();
    if (!view || revision != view->Revision)
        return UIA_E_ELEMENTNOTAVAILABLE;
    if (start < 0)
        return E_INVALIDARG;
    if (start >= view->Count)
        return S_OK;
    DWORD_PTR result = 0;
    const HRESULT hr = SendAction(revision, CFindUiaAction::FindSelection, start - 1, result);
    if (SUCCEEDED(hr))
        *index = static_cast<int>(result) - 2;
    return hr;
}
