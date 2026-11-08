// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-07 | Share Find results and viewport with UI Automation. | Keep accessibility queries independent of the native list's exhaustive tree. | https://github.com/KRtkovo-eu-AI/salamander/issues/819 | GPT-6 (Codex) |

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CFoundFilesListView;
struct CFoundFilesData;
struct IRawElementProviderSimple;

constexpr int FindUiaColumnCount = 6;

struct CFindUiaView
{
    HWND Window = NULL;
    UINT_PTR Revision = 0;
    int Count = 0;
    int First = 0;
    int Last = 0; // exclusive
    int Focus = -1;
    int SelectedTotal = 0;
    BOOL HasFocus = FALSE;
    RECT Bounds = {};
    RECT FirstRow = {};
    int RowHeight = 0;
    RECT Columns[FindUiaColumnCount] = {};
    std::vector<bool> Selected;
    std::wstring Name;
};

enum class CFindUiaAction
{
    Focus,
    ScrollIntoView,
    Select,
    AddSelection,
    RemoveSelection,
    Invoke,
    SortColumn,
    ScrollHorizontal,
    ScrollVertical,
    QuerySelection,
    FindSelection
};

enum CFindUiaEvent : unsigned
{
    FindUiaStructureChanged = 1,
    FindUiaFocusChanged = 2,
    FindUiaSelectionChanged = 4
};

// The dialog publishes a small viewport snapshot. COM threads borrow result
// data only briefly and never keep the list alive after its window is closed.
class CFindUiaState
{
public:
    explicit CFindUiaState(CFoundFilesListView& list);
    void Detach();
    unsigned Refresh();
    std::shared_ptr<const CFindUiaView> ReadView() const;
    HRESULT ReadText(UINT_PTR revision, int row, int column, std::wstring& text) const;
    HRESULT FindName(UINT_PTR revision, int start, const wchar_t* name, int* index) const;
    HRESULT IsSelected(UINT_PTR revision, int row, BOOL* value) const;
    HRESULT ReadSelection(UINT_PTR revision, std::vector<int>& rows) const;
    HRESULT FindSelected(UINT_PTR revision, int start, int* index) const;
    HRESULT Act(UINT_PTR revision, CFindUiaAction action, int index) const;

    // Coalesce events on the UI Automation worker, independently of the dialog.
    std::atomic<unsigned> PendingEvents{0};

private:
    HRESULT SendAction(UINT_PTR revision, CFindUiaAction action, int index, DWORD_PTR& result) const;
    bool AcquireRows(UINT_PTR revision, int first, int count, CFoundFilesData** rows) const;

    mutable std::mutex Mutex;
    CFoundFilesListView* List;
    std::shared_ptr<const CFindUiaView> View;
    bool Refreshing = false; // accessed only on the window thread
};

IRawElementProviderSimple* CreateFindUiaProvider(const std::shared_ptr<CFindUiaState>& state);
void QueueFindUiaEvents(const std::shared_ptr<CFindUiaState>& state, unsigned flags);
