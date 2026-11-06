// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <windows.h>
#include <algorithm>
#include <vector>

// At most one small viewport batch is copied on the UI thread. The predicate
// is invoked only for visible rows; explicit sorting does not use this helper.
template<class Missing>
std::vector<int> ExplorerPropertyDisplayRows(int total, int first, int count, Missing missing)
{
    const int batchLimit = 128;
    first = (std::max)(0, (std::min)(first, total));
    count = (std::max)(0, (std::min)(count, total - first));
    std::vector<int> rows;
    rows.reserve((std::min)(batchLimit, count));
    for (int row = first; row < first + count && rows.size() < batchLimit; ++row)
        if (missing(row)) rows.push_back(row);
    return rows;
}

// -1 means viewport display metadata; nonnegative values identify the exact
// Explorer sort column. Cancellation remains superseded even if the user later
// switches back to the original column before that worker completes.
inline bool ExplorerPropertyRequestNeedsReplacement(ULONGLONG jobGeneration, int jobColumn,
    ULONGLONG currentGeneration, int currentColumn, bool cancelled)
{
    return cancelled || jobGeneration != currentGeneration || jobColumn != currentColumn;
}

// The panel and worker each own a reference. Cancellation disconnects the HWND
// while it still belongs to the panel, and serializes with posting completion.
// Thus the panel can close without waiting for a blocked shell property handler.
class CExplorerPropertyTaskLifetime
{
    CRITICAL_SECTION PostGate;
    LONG References;
    HWND Target;
protected:
    virtual ~CExplorerPropertyTaskLifetime() { DeleteCriticalSection(&PostGate); }
public:
    volatile LONG Cancelled;
    explicit CExplorerPropertyTaskLifetime(HWND target) : References(1), Target(target), Cancelled(0)
    { InitializeCriticalSection(&PostGate); }
    void AddRef() { InterlockedIncrement(&References); }
    void Release() { if (InterlockedDecrement(&References) == 0) delete this; }
    void PostCompletion(UINT message, LPARAM payload)
    {
        EnterCriticalSection(&PostGate);
        // A superseded generation still reports completion so the live panel
        // can schedule its replacement. Teardown alone disconnects Target.
        if (Target != NULL)
            PostMessageW(Target, message, 0, payload);
        LeaveCriticalSection(&PostGate);
    }
    void CancelCompletion(UINT message)
    {
        EnterCriticalSection(&PostGate);
        InterlockedExchange(&Cancelled, 1);
        const HWND target = Target;
        Target = NULL;
        if (target != NULL)
        {
            MSG queued;
            while (PeekMessageW(&queued, target, message, message, PM_REMOVE)) {}
        }
        LeaveCriticalSection(&PostGate);
    }
    CExplorerPropertyTaskLifetime(const CExplorerPropertyTaskLifetime&) = delete;
    CExplorerPropertyTaskLifetime& operator=(const CExplorerPropertyTaskLifetime&) = delete;
};
