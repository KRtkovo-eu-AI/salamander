// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <set>

static const DWORD BRANCH_VIEW_PATH_COLUMN = 0x42565041;

// No CFileData/plug-in ABI changes: the panel owns these paths separately.
namespace Salamander { namespace BranchView {
// S26: process-only explicit experiment; all normal configurations compile the code.
inline bool ExperimentEnabled()
{
    wchar_t value[2] = {};
    return GetEnvironmentVariableW(L"SAMANDARIN_EXPERIMENTAL_BRANCH_VIEW", value, 2) == 1 && value[0] == L'1';
}
struct Entry
{
    std::wstring Directory;
    std::wstring RelativePath;
    std::string CacheKey;
    WIN32_FIND_DATAW Data;
    std::wstring FullPath() const;
};

struct Progress
{
    bool Running = false;
    bool Cancelled = false;
    unsigned Errors = 0;
    unsigned SkippedLinks = 0;
    ULONGLONG Directories = 0;
    ULONGLONG Files = 0;
    DWORD LastError = ERROR_SUCCESS;
};

// Automatic notifications never interrupt an active scan. Keep a completed
// snapshot visible for a bounded cooldown, then debounce bursts without letting
// a continuously changing tree postpone the next refresh forever.
class RefreshSchedule
{
    bool Active = false;
    bool Pending = false;
    bool Manual = false;
    bool Paused = false;
    ULONGLONG StartedAt = 0;
    ULONGLONG NotBefore = 0;
    ULONGLONG FirstChange = 0;
    ULONGLONG LastChange = 0;
public:
    void Request(ULONGLONG now, bool automatic)
    {
        if (automatic && Paused) return;
        if (!automatic) { Paused = false; Manual = true; }
        if (!Pending) FirstChange = now;
        LastChange = now;
        Pending = true;
    }
    bool Due(ULONGLONG now) const
    {
        if (Active || !Pending) return false;
        if (Manual) return true;
        ULONGLONG due = LastChange + 1000;
        if (due > FirstChange + 5000) due = FirstChange + 5000;
        if (due < NotBefore) due = NotBefore;
        return now >= due;
    }
    void Started(ULONGLONG now)
    {
        Active = true; Pending = false; Manual = false; StartedAt = now;
    }
    void Finished(ULONGLONG now)
    {
        Active = false;
        ULONGLONG cooldown = now - StartedAt;
        if (cooldown < 5000) cooldown = 5000;
        if (cooldown > 30000) cooldown = 30000;
        NotBefore = now + cooldown;
    }
    void Cancel()
    {
        Active = false; Pending = false; Manual = false; Paused = true;
    }
    bool HasPending() const { return Pending; }
    bool IsManual() const { return Manual; }
};

// Last-access timestamps are deliberately not a content change: enumeration,
// icons and viewers must not rebuild an otherwise identical Branch listing.
bool SameSnapshot(const std::vector<Entry>& first, const std::vector<Entry>& second);

// Initial enumeration offers a small usable preview, then publishes the complete
// snapshot. Rebuilding every growing prefix turns a large scan into repeated UI
// stalls. Progress still reports every discovered file, independently of preview.
size_t InitialPreviewCount(size_t discovered, size_t published);
int CompareEntryPaths(const Entry& first, const Entry& second);

// Each call returns to the ordinary UI loop. The budget is checked between
// shell queries; a single third-party shell handler cannot be preempted here.
class AssociationPreparation
{
    std::vector<std::string> Extensions;
    std::unordered_set<std::string> Seen;
    size_t Next = 0;
    ULONGLONG Revision = 0;
public:
    void Add(const std::string& extension)
    {
        if (!extension.empty() && Seen.insert(extension).second) Extensions.push_back(extension);
    }
    void Restart() { Next = 0; ++Revision; }
    void Clear() { Extensions.clear(); Seen.clear(); Next = 0; ++Revision; }
    bool HasPending() const { return Next < Extensions.size(); }
    template<class Clock, class Resolve>
    size_t RunSlice(Clock clock, Resolve resolve, ULONGLONG budgetMs = 8, size_t maxCount = 64)
    {
        const ULONGLONG started = clock();
        const ULONGLONG revision = Revision;
        size_t completed = 0;
        while (HasPending() && completed < maxCount)
        {
            if (completed != 0 && clock() - started >= budgetMs) break;
            // Resolve may enter a shell message loop: Clear/Restart must not
            // invalidate the argument or advance a replaced work queue.
            const std::string extension = Extensions[Next];
            if (!resolve(extension) || revision != Revision) break;
            ++Next;
            ++completed;
        }
        return completed;
    }
};
struct PreparationLifetime
{
    bool Alive = true;
    bool Busy = false;
};
struct PreparationSession
{
    AssociationPreparation Queue;
    ULONGLONG AssociationEpoch = 0;
    bool Alive = true;
};

// The worker owns a shared state, never a window/panel. A disconnected SMB
// request may return after the tab was closed; its last reference cleans up.
class Scan
{
    struct State;
    std::shared_ptr<State> Shared;
    HANDLE Thread;
    static DWORD WINAPI Run(void* context);
public:
    Scan();
    ~Scan();
    bool Start(const std::wstring& root, bool hideHiddenSystem);
    void Cancel();
    void Drain(std::vector<Entry>& entries, Progress& progress);
    Scan(const Scan&) = delete;
    Scan& operator=(const Scan&) = delete;
};

std::wstring Join(const std::wstring& directory, const std::wstring& name);
std::wstring ExtendedPath(const std::wstring& path);
bool IsWithin(const std::wstring& root, const std::wstring& path);
} }

struct CBranchViewRestoreState
{
    bool Enabled = false;
    std::wstring Focus;
    std::vector<std::wstring> Selected;
    int TopIndex = 0;
    int XOffset = 0;
    int PathColumnWidth = 280;
    int ViewTemplate = 2;
    int OriginalViewTemplate = 2;
    int OriginalSort = 0;
    DWORD OriginalSortCustomData = 0;
    BOOL OriginalReverseSort = FALSE;
};
struct CFileData;
class CBranchViewState
{
public:
    bool Enabled = false;
    bool Applying = false;
    bool Dirty = false;
    bool PreserveListing = false;
    bool ForcePublish = false;
    bool Preparing = false;
    bool CompletionPending = false;
    std::shared_ptr<Salamander::BranchView::PreparationSession> Preparation;
    std::shared_ptr<Salamander::BranchView::PreparationLifetime> PreparationLifetime;
    ULONGLONG Generation = 0;
    DWORD LastPublish = 0;
    Salamander::BranchView::RefreshSchedule Refresh;
    std::wstring Root;
    std::wstring PendingFocus;
    int PathColumnWidth = 280;
    int OriginalViewTemplate = 2;
    int OriginalSort = 0;
    DWORD OriginalSortCustomData = 0;
    BOOL OriginalReverseSort = FALSE;
    std::shared_ptr<CBranchViewRestoreState> RestoreState;
    std::set<std::wstring> RestoreSelected;
    int OriginalTop = 0;
    int OriginalOffset = 0;
    std::wstring OriginalFocus;
    Salamander::BranchView::Scan Worker;
    Salamander::BranchView::Progress Status;
    std::vector<Salamander::BranchView::Entry> Entries;
    std::vector<Salamander::BranchView::Entry> IncomingEntries;
    // Name allocations stay stable across shallow CFilesArray sorts. Entries
    // for the old listing remain until refresh has released that listing.
    std::unordered_map<std::string, const CFileData*> CacheFiles;
    std::mutex ItemsMutex;
    std::unordered_map<const char*, Salamander::BranchView::Entry> Items;
};
