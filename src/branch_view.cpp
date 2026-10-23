// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef BRANCH_VIEW_TEST
#include "precomp.h"
#endif
#include "branch_view.h"
#include "branch_view_text.h"
#include <new>
#include <algorithm>

namespace Salamander { namespace BranchView {
std::wstring Join(const std::wstring& directory, const std::wstring& name)
{
    return directory + (!directory.empty() && directory.back() != L'\\' ? L"\\" : L"") + name;
}
std::wstring Entry::FullPath() const { return Join(Directory, Data.cFileName); }
size_t InitialPreviewCount(size_t discovered, size_t published)
{
    const size_t limit = published == 0 ? 2048 : 8192;
    return (std::min)(discovered, (std::max)(published, limit));
}
int CompareEntryPaths(const Entry& first, const Entry& second)
{
    const int directory = CompareStringOrdinal(first.Directory.c_str(), -1,
        second.Directory.c_str(), -1, TRUE) - CSTR_EQUAL;
    if (directory != 0) return directory;
    // All entries share the same exact root, so relative and absolute paths
    // have identical ordinal tie-breaking without constructing either path.
    return first.RelativePath.compare(second.RelativePath);
}
static std::string Utf8Path(const std::wstring& path)
{
    const int length = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), NULL, 0, NULL, NULL);
    if (length <= 0) return std::string();
    std::string result((size_t)length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path.c_str(), (int)path.size(), &result[0], length, NULL, NULL);
    return result;
}

std::wstring ExtendedPath(const std::wstring& path)
{
    if (path.compare(0, 4, L"\\\\?\\") == 0 || path.compare(0, 4, L"\\\\.\\") == 0)
        return path;
    if (path.compare(0, 2, L"\\\\") == 0)
        return L"\\\\?\\UNC\\" + path.substr(2);
    return L"\\\\?\\" + path;
}
bool IsWithin(const std::wstring& root, const std::wstring& path)
{
    size_t length = root.size();
    while (length > 0 && root[length - 1] == L'\\') --length;
    return path.size() >= length &&
           CompareStringOrdinal(root.c_str(), (int)length, path.c_str(), (int)length, TRUE) == CSTR_EQUAL &&
           (path.size() == length || path[length] == L'\\');
}

static bool SameEntry(const Entry& first, const Entry& second)
{
    const WIN32_FIND_DATAW& a = first.Data;
    const WIN32_FIND_DATAW& b = second.Data;
    return first.Directory == second.Directory && wcscmp(a.cFileName, b.cFileName) == 0 &&
           wcscmp(a.cAlternateFileName, b.cAlternateFileName) == 0 &&
           a.dwFileAttributes == b.dwFileAttributes && a.dwReserved0 == b.dwReserved0 &&
           a.nFileSizeHigh == b.nFileSizeHigh && a.nFileSizeLow == b.nFileSizeLow &&
           CompareFileTime(&a.ftCreationTime, &b.ftCreationTime) == 0 &&
           CompareFileTime(&a.ftLastWriteTime, &b.ftLastWriteTime) == 0;
}
bool SameSnapshot(const std::vector<Entry>& first, const std::vector<Entry>& second)
{
    if (first.size() != second.size()) return false;
    if (std::equal(first.begin(), first.end(), second.begin(), SameEntry)) return true;
    // Enumeration order can change without any change to a visible item. Sort
    // pointers rather than copying long paths or rearranging the live snapshot.
    std::vector<const Entry*> a, b;
    a.reserve(first.size()); b.reserve(second.size());
    for (const auto& entry : first) a.push_back(&entry);
    for (const auto& entry : second) b.push_back(&entry);
    auto less = [](const Entry* left, const Entry* right) {
        if (left->Directory != right->Directory) return left->Directory < right->Directory;
        return wcscmp(left->Data.cFileName, right->Data.cFileName) < 0;
    };
    std::sort(a.begin(), a.end(), less);
    std::sort(b.begin(), b.end(), less);
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](const Entry* left, const Entry* right) { return SameEntry(*left, *right); });
}

struct Scan::State
{
    CRITICAL_SECTION Lock;
    volatile LONG Cancelled;
    std::wstring Root;
    bool HideHiddenSystem;
    std::vector<Entry> Pending;
    Progress Status;
    State() : Cancelled(0), HideHiddenSystem(false) { InitializeCriticalSection(&Lock); }
    ~State() { DeleteCriticalSection(&Lock); }
};

class StateLock
{
    CRITICAL_SECTION* Lock;
public:
    explicit StateLock(CRITICAL_SECTION& lock) : Lock(&lock) { EnterCriticalSection(Lock); }
    ~StateLock() { LeaveCriticalSection(Lock); }
};

DWORD WINAPI Scan::Run(void* context)
{
    std::unique_ptr<std::shared_ptr<State>> reference(static_cast<std::shared_ptr<State>*>(context));
    std::shared_ptr<State> state = *reference;
    std::vector<std::pair<std::wstring, std::wstring>> pending;
    std::vector<Entry> batch;
    Progress progress;
    progress.Running = true;
    HANDLE search = INVALID_HANDLE_VALUE;
    DWORD oldMode = 0;
    SetThreadErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX, &oldMode);
    try
    {
        pending.emplace_back(state->Root, L"");
        while (!pending.empty() && !InterlockedCompareExchange(&state->Cancelled, 0, 0))
        {
            auto directory = std::move(pending.back());
            pending.pop_back();
            WIN32_FIND_DATAW data;
            std::wstring pattern = ExtendedPath(Join(directory.first, L"*"));
            search = FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &data, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);
            if (search == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER)
                search = FindFirstFileExW(pattern.c_str(), FindExInfoStandard, &data, FindExSearchNameMatch, NULL, 0);
            ++progress.Directories;
            if (search == INVALID_HANDLE_VALUE)
            {
                DWORD error = GetLastError();
                if (error != ERROR_FILE_NOT_FOUND && error != ERROR_NO_MORE_FILES)
                {
                    ++progress.Errors;
                    progress.LastError = error;
                }
            }
            else
            {
                do
                {
                    if (InterlockedCompareExchange(&state->Cancelled, 0, 0)) break;
                    if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
                    const bool isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                    if (isDirectory)
                    {
                        if (state->HideHiddenSystem && (data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM))) continue;
                        // Follow cloud/other ordinary reparse directories, but not
                        // name-surrogate links (junctions and symbolic links).
                        if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) && IsReparseTagNameSurrogate(data.dwReserved0))
                        {
                            ++progress.SkippedLinks;
                            continue;
                        }
                        pending.emplace_back(Join(directory.first, data.cFileName), Join(directory.second, data.cFileName));
                    }
                    else
                    {
                        Entry entry;
                        entry.Directory = directory.first;
                        entry.RelativePath = Join(directory.second, data.cFileName);
                        entry.Data = data;
                        entry.CacheKey = Utf8Path(entry.FullPath());
                        batch.push_back(std::move(entry));
                        ++progress.Files;
                    }
                    if (batch.size() >= 256)
                    {
                        StateLock lock(state->Lock);
                        state->Pending.insert(state->Pending.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
                        batch.clear();
                        state->Status = progress;
                    }
                } while (FindNextFileW(search, &data));
                DWORD error = GetLastError();
                FindClose(search);
                search = INVALID_HANDLE_VALUE;
                if (!InterlockedCompareExchange(&state->Cancelled, 0, 0) && error != ERROR_NO_MORE_FILES)
                {
                    ++progress.Errors;
                    progress.LastError = error;
                }
            }
            // Small directories publish without waiting for a full batch.
            StateLock lock(state->Lock);
            state->Pending.insert(state->Pending.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
            batch.clear();
            state->Status = progress;
        }
    }
    catch (const std::bad_alloc&)
    {
        ++progress.Errors;
        progress.LastError = ERROR_NOT_ENOUGH_MEMORY;
    }
    if (search != INVALID_HANDLE_VALUE) FindClose(search);
    progress.Cancelled = InterlockedCompareExchange(&state->Cancelled, 0, 0) != 0;
    progress.Running = false;
    {
        StateLock lock(state->Lock);
        state->Status = progress;
    }
    SetThreadErrorMode(oldMode, NULL);
    return 0;
}

Scan::Scan() : Thread(NULL) {}
Scan::~Scan() { Cancel(); }
void Scan::Cancel()
{
    if (Shared) InterlockedExchange(&Shared->Cancelled, 1);
    if (Thread != NULL)
    {
        CancelSynchronousIo(Thread);
        CloseHandle(Thread); // worker retains its own shared state until return
        Thread = NULL;
    }
}
bool Scan::Start(const std::wstring& root, bool hideHiddenSystem)
{
    Cancel();
    try
    {
        Shared = std::make_shared<State>();
        Shared->Root = root;
        Shared->HideHiddenSystem = hideHiddenSystem;
        Shared->Status.Running = true;
        auto reference = std::make_unique<std::shared_ptr<State>>(Shared);
        Thread = CreateThread(NULL, 0, Run, reference.get(), 0, NULL);
        if (Thread == NULL)
        {
            Shared->Status.Running = false;
            Shared->Status.Errors = 1;
            Shared->Status.LastError = GetLastError();
            return false;
        }
        reference.release();
        return true;
    }
    catch (const std::bad_alloc&) { Shared.reset(); return false; }
}
void Scan::Drain(std::vector<Entry>& entries, Progress& progress)
{
    if (!Shared) { progress = Progress(); return; }
    StateLock lock(Shared->Lock);
    entries.swap(Shared->Pending);
    progress = Shared->Status;
}
} }

#ifndef BRANCH_VIEW_TEST
#include "plugins.h"
#include "fileswnd.h"
#include "filesbox.h"
#include "mainwnd.h"
#include "stswnd.h"
#include "snooper.h"
#include "common/widepath.h"
#include <set>

BOOL CFilesWindow::IsBranchView() const { return BranchView != NULL && BranchView->Enabled; }
BOOL CFilesWindow::IsBranchViewScanning() const { return IsBranchView() && (BranchView->Status.Running || BranchView->Preparing); }
ULONGLONG CFilesWindow::GetBranchViewGeneration() const { return BranchView != NULL ? BranchView->Generation : 0; }

std::wstring CFilesWindow::GetItemDirectoryW(const CFileData& file) const
{
    if (BranchView != NULL)
    {
        std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
        auto item = BranchView->Items.find(file.Name);
        if (item != BranchView->Items.end()) return item->second.Directory;
        // An active Branch row without its identity must never be treated as a
        // file in the root. The up-directory row is the only synthetic item.
        if (BranchView->Enabled && (file.Name == NULL || strcmp(file.Name, "..") != 0))
            return std::wstring();
    }
    return GetPathW() != NULL && GetPathW()[0] != 0 ? std::wstring(GetPathW()) : SalMultiByteToWidePath(GetPath(), IsValidPathUtf8Text(GetPath()) ? CP_UTF8 : CP_ACP);
}
std::wstring CFilesWindow::GetItemFullPathW(const CFileData& file) const
{
    if (BranchView != NULL)
    {
        std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
        auto item = BranchView->Items.find(file.Name);
        if (item != BranchView->Items.end()) return item->second.FullPath();
        if (BranchView->Enabled && (file.Name == NULL || strcmp(file.Name, "..") != 0))
            return std::wstring();
    }
    std::wstring name = file.UseWideName() ? file.NameW : SalMultiByteToWidePath(file.Name, IsValidPathUtf8Text(file.Name) ? CP_UTF8 : CP_ACP);
    std::wstring path = GetItemDirectoryW(file);
    if (!SalPathAppendW(path, name.c_str())) return std::wstring(); // Preserve ordinary separators and fail closed at the path limit.
    return path;
}
std::wstring CFilesWindow::GetItemIdentityW(const CFileData& file) const { return GetItemFullPathW(file); }
std::wstring CFilesWindow::GetItemRelativePathW(const CFileData& file) const
{
    if (BranchView != NULL)
    {
        std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
        auto item = BranchView->Items.find(file.Name);
        if (item != BranchView->Items.end()) return item->second.RelativePath;
        if (BranchView->Enabled && (file.Name == NULL || strcmp(file.Name, "..") != 0))
            return std::wstring();
    }
    return file.UseWideName() ? file.NameW : SalMultiByteToWidePath(file.Name, IsValidPathUtf8Text(file.Name) ? CP_UTF8 : CP_ACP);
}
const char* CFilesWindow::GetItemCacheKeyPtr(const CFileData& file) const
{
    if (IsBranchView())
    {
        std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
        auto item = BranchView->Items.find(file.Name);
        if (item != BranchView->Items.end()) return item->second.CacheKey.c_str();
    }
    return file.Name;
}
std::string CFilesWindow::GetItemCacheKey(const CFileData& file) const { return GetItemCacheKeyPtr(file); }
const CFileData* CFilesWindow::GetBranchViewFileByCacheKey(const char* key) const
{
    if (!IsBranchView() || key == NULL) return NULL;
    std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
    auto file = BranchView->CacheFiles.find(key);
    return file == BranchView->CacheFiles.end() ? NULL : file->second;
}
void CFilesWindow::GetBranchViewProgress(Salamander::BranchView::Progress& progress) const
{
    progress = BranchView != NULL ? BranchView->Status : Salamander::BranchView::Progress();
}
std::wstring CFilesWindow::GetBranchViewStatusW() const
{
    if (!IsBranchView()) return std::wstring();
    const bool retainingSnapshot = BranchView->PreserveListing &&
        (BranchView->Status.Running || BranchView->Status.Cancelled);
    int id = BranchView->Preparing ? IDS_BRANCH_PREPARING : retainingSnapshot ?
                 (BranchView->Status.Running ? IDS_BRANCH_REFRESHING : IDS_BRANCH_REFRESH_STOPPED) :
             BranchView->Status.Running ? IDS_BRANCH_SCANNING :
             BranchView->Status.Cancelled ? IDS_BRANCH_STOPPED :
             BranchView->Status.Errors != 0 ? IDS_BRANCH_ERRORS : IDS_BRANCH_READY;
    // Background progress belongs to IncomingEntries. The user still sees the
    // previous snapshot, so do not describe it as a new/incomplete small list.
    const ULONGLONG files = (retainingSnapshot || BranchView->Status.Cancelled) ? (ULONGLONG)BranchView->Entries.size() : BranchView->Status.Files;
    const std::string text = Salamander::BranchViewText::FormatStatus(LoadStr(id),
                BranchView->Preparing ? BranchView->Status.Files : files,
                BranchView->Preparing ? (ULONGLONG)BranchView->Entries.size() : (ULONGLONG)BranchView->Status.Errors);
    return SalMultiByteToWidePath(text.c_str(), CP_UTF8);
}
std::string CFilesWindow::GetBranchViewStatusText() const
{
    return SalWideToMultiBytePath(GetBranchViewStatusW().c_str(), CP_UTF8);
}
int CFilesWindow::GetBranchViewPathColumnWidth() const { return BranchView != NULL ? BranchView->PathColumnWidth : 280; }
void CFilesWindow::SetBranchViewPathColumnWidth(int width) { if (BranchView != NULL) BranchView->PathColumnWidth = max(40, width); }
void CFilesWindow::CancelBranchViewScan()
{
    if (BranchView == NULL) return;
    BranchView->Worker.Cancel();
    if (BranchView->Preparation != NULL)
    {
        BranchView->Preparation->Alive = false;
        BranchView->Preparation->Queue.Clear();
    }
    BranchView->Preparing = false;
    BranchView->CompletionPending = false;
    BranchView->Status.Running = false;
    BranchView->Status.Cancelled = true;
    BranchView->Refresh.Cancel();
    BranchView->IncomingEntries.clear();
    BranchView->Dirty = false;
    if (HWindow != NULL) KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
    DirectoryLineSetText();
    IdleRefreshStates = TRUE;
}
void CFilesWindow::ShutdownBranchView()
{
    if (BranchView != NULL)
    {
        if (BranchView->PreparationLifetime != NULL) BranchView->PreparationLifetime->Alive = false;
        if (BranchView->Preparation != NULL) BranchView->Preparation->Alive = false;
    }
    if (HWindow != NULL) KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
    delete BranchView;
    BranchView = NULL;
}
void CFilesWindow::LeaveBranchView()
{
    if (!IsBranchView()) return;
    CancelBranchViewScan();
    BranchView->Enabled = false;
    SortType = (CSortType)BranchView->OriginalSort;
    SortCustomData = BranchView->OriginalSortCustomData;
    ReverseSort = BranchView->OriginalReverseSort;
    // Branch View changes listing scope, not the user-selected layout.
    BuildColumnsTemplate();
    ++BranchView->Generation;
    if (HWindow != NULL) KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
    // Metadata is retained until the old icon reader/listing is released.
    // A subsequent ReadDirectory prunes it before constructing ordinary rows.
}
static void StartBranchViewScan(CFilesWindow* panel)
{
    CBranchViewState& state = *panel->BranchView;
    // An explicit refresh may apply new file filters, hidden names or property
    // columns even when the filesystem snapshot itself has not changed.
    state.ForcePublish = state.Refresh.IsManual();
    state.Refresh.Started(GetTickCount64());
    state.PreserveListing = !state.Entries.empty();
    state.IncomingEntries.clear();
    if (state.Preparation != NULL) state.Preparation->Alive = false;
    state.Preparing = false;
    state.CompletionPending = false;
    try
    {
        if (state.PreparationLifetime == NULL)
            state.PreparationLifetime = std::make_shared<Salamander::BranchView::PreparationLifetime>();
        state.Preparation = std::make_shared<Salamander::BranchView::PreparationSession>();
    }
    catch (const std::bad_alloc&)
    {
        TRACE_E(LOW_MEMORY);
        state.Status = Salamander::BranchView::Progress();
        state.Status.Cancelled = true;
        state.Status.LastError = ERROR_NOT_ENOUGH_MEMORY;
        state.Refresh.Cancel();
        state.Dirty = false;
        KillTimer(panel->HWindow, IDT_BRANCHVIEW_POLL);
        panel->DirectoryLineSetText();
        IdleRefreshStates = TRUE;
        return;
    }
    state.Preparation->AssociationEpoch = Associations.GetShellAssociationEpoch();
    state.Worker.Start(state.Root, Configuration.NotHiddenSystemFiles != FALSE);
    state.Status = Salamander::BranchView::Progress();
    state.Status.Running = true;
    state.Dirty = false;
    SetTimer(panel->HWindow, IDT_BRANCHVIEW_POLL, 200, NULL);
    panel->DirectoryLineSetText();
    IdleRefreshStates = TRUE;
}
void CFilesWindow::RefreshBranchView(BOOL automatic)
{
    if (automatic) return; // S26 manual refresh only; automatic watcher policy is S30.
    if (!IsBranchView()) return;
    const ULONGLONG now = GetTickCount64();
    BranchView->Refresh.Request(now, automatic != FALSE);
    if (BranchView->Refresh.Due(now)) StartBranchViewScan(this);
    else if (BranchView->Refresh.HasPending())
        SetTimer(HWindow, IDT_BRANCHVIEW_POLL, BranchView->Preparing ? 15 : 200, NULL);
}
CBranchViewRestoreState CFilesWindow::CaptureBranchViewState()
{
    CBranchViewRestoreState state;
    state.Enabled = IsBranchView() != FALSE;
    if (!state.Enabled) return state;
    state.PathColumnWidth = BranchView->PathColumnWidth;
    state.ViewTemplate = GetViewTemplateIndex();
    state.OriginalViewTemplate = BranchView->OriginalViewTemplate;
    state.OriginalSort = BranchView->OriginalSort;
    state.OriginalSortCustomData = BranchView->OriginalSortCustomData;
    state.OriginalReverseSort = BranchView->OriginalReverseSort;
    state.TopIndex = ListBox->GetTopIndex();
    state.XOffset = ListBox->GetXOffset();
    int focus = GetCaretIndex() - Dirs->Count;
    for (int i = 0; i < Files->Count; ++i)
    {
        const std::wstring identity = GetItemIdentityW(Files->At(i));
        if (i == focus) state.Focus = identity;
        if (Files->At(i).Selected) state.Selected.push_back(identity);
    }
    return state;
}
void CFilesWindow::RestoreBranchViewState(const CBranchViewRestoreState& state)
{
    if (!Salamander::BranchView::ExperimentEnabled() || !state.Enabled || !Is(ptDisk)) return;
    if (!IsBranchView()) ToggleBranchView();
    if (!IsBranchView()) return;
    BranchView->PathColumnWidth = state.PathColumnWidth;
    BranchView->OriginalViewTemplate = state.OriginalViewTemplate;
    BranchView->OriginalSort = state.OriginalSort;
    BranchView->OriginalSortCustomData = state.OriginalSortCustomData;
    BranchView->OriginalReverseSort = state.OriginalReverseSort;
    BranchView->PendingFocus = state.Focus;
    BranchView->RestoreState = std::make_shared<CBranchViewRestoreState>(state);
    BranchView->RestoreSelected = std::set<std::wstring>(state.Selected.begin(), state.Selected.end());
    SelectViewTemplate(state.ViewTemplate, FALSE, FALSE);
    BuildColumnsTemplate();
}
void CFilesWindow::ToggleBranchView()
{
    if (!IsBranchView() && !Salamander::BranchView::ExperimentEnabled()) return;
    if (StopRefresh || SnooperSuspended || FilesActionInProgress) return; // No mode switch in a reentrant operation callback.
    if (!Is(ptDisk)) return;
    if (IsBranchView())
    {
        const int top = BranchView->OriginalTop;
        const int offset = BranchView->OriginalOffset;
        const std::wstring focus = BranchView->OriginalFocus;
        LeaveBranchView();
        SelectViewTemplate(BranchView->OriginalViewTemplate, FALSE, FALSE); // S26 restores layout after core-only experiment.
        DetachDirectory(this);
        RefreshDirectory();
        PruneBranchViewMetadata();
        int focusIndex = 0;
        for (int i = 0; i < Dirs->Count + Files->Count; ++i)
        {
            const CFileData& file = i < Dirs->Count ? Dirs->At(i) : Files->At(i - Dirs->Count);
            if (GetItemFullPathW(file) == focus) { focusIndex = i; break; }
        }
        RefreshListBox(offset, top, focusIndex, FALSE, FALSE);
        EnsureWatching(this, FALSE);
        RefreshPathHistoryData();
        return;
    }
    if (BranchView == NULL)
    {
#ifdef new
#undef new
#define BRANCH_RESTORE_DEBUG_NEW
#endif
        BranchView = new (std::nothrow) CBranchViewState;
#ifdef BRANCH_RESTORE_DEBUG_NEW
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#undef BRANCH_RESTORE_DEBUG_NEW
#endif
        if (BranchView == NULL) return;
    }
    BranchView->OriginalViewTemplate = GetViewTemplateIndex();
    BranchView->OriginalSort = (int)SortType;
    BranchView->OriginalSortCustomData = SortCustomData;
    BranchView->OriginalReverseSort = ReverseSort;
    BranchView->OriginalTop = ListBox->GetTopIndex();
    BranchView->OriginalOffset = ListBox->GetXOffset();
    int index = GetCaretIndex();
    BranchView->OriginalFocus.clear();
    if (index >= 0 && index < Dirs->Count + Files->Count)
        BranchView->OriginalFocus = GetItemFullPathW(index < Dirs->Count ? Dirs->At(index) : Files->At(index - Dirs->Count));
    BranchView->Root = GetPathW() != NULL && GetPathW()[0] != 0 ? GetPathW() : SalMultiByteToWidePath(GetPath(), IsValidPathUtf8Text(GetPath()) ? CP_UTF8 : CP_ACP);
    BranchView->Enabled = true;
    ++BranchView->Generation; // mode change invalidates ordinary-panel property jobs
    BranchView->RestoreState.reset();
    BranchView->RestoreSelected.clear();
    // S26 core-only read-only panel: arbitrary thumbnail/plugin layouts remain S28.
    BuildColumnsTemplate();
    BranchView->Entries.clear();
    // Clear the ordinary directory rows immediately; a Branch collection only
    // contains files and the navigation row, even before its first batch.
    BranchView->Applying = true;
    SelectViewTemplate(2, FALSE, FALSE); // Detailed; preserve original layout for exit.
    RefreshDirectory();
    BranchView->Applying = false;
    DetachDirectory(this);
    EnsureWatching(this, FALSE);
    RefreshBranchView();
    RefreshPathHistoryData();
}
void CFilesWindow::ReindexBranchViewFiles()
{
    if (BranchView == NULL) return;
    std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
    BranchView->CacheFiles.clear();
    BranchView->CacheFiles.reserve(Files->Count);
    for (int i = 0; i < Files->Count; ++i)
    {
        const CFileData& file = Files->At(i);
        auto item = BranchView->Items.find(file.Name);
        if (item != BranchView->Items.end()) BranchView->CacheFiles[item->second.CacheKey] = &file;
    }
}
void CFilesWindow::PruneBranchViewMetadata()
{
    if (BranchView == NULL) return;
    std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
    // Transfer existing nodes rather than copying paths or allocating one
    // tree/set node per row. References to retained Entry/CacheKey stay valid.
    decltype(BranchView->Items) live;
    live.reserve(Files->Count);
    for (int i = 0; i < Files->Count; ++i)
    {
        auto item = BranchView->Items.extract(Files->At(i).Name);
        if (!item.empty()) live.insert(std::move(item));
    }
    BranchView->Items.swap(live);
}
void CFilesWindow::PollBranchView()
{
    if (!IsBranchView() || BranchView->Applying || StopRefresh || SnooperSuspended) return;
    // Stop Scan is persistent until an explicit refresh. A cancelled worker may
    // still finish a network request, but must never publish its late batch.
    if (BranchView->Status.Cancelled)
    {
        KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
        return;
    }
    const auto preparation = BranchView->Preparation;
    const auto lifetime = BranchView->PreparationLifetime;
    if (preparation == NULL || lifetime->Busy) return;
    // Shell providers may pump messages. The shared session survives a closed
    // tab, and must be checked before accessing the panel after every query.
    struct PollScope
    {
        Salamander::BranchView::PreparationLifetime& Lifetime;
        PollScope(Salamander::BranchView::PreparationLifetime& lifetime) : Lifetime(lifetime) { Lifetime.Busy = true; }
        ~PollScope() { Lifetime.Busy = false; }
    } scope(*lifetime);
    std::vector<Salamander::BranchView::Entry> batch;
    Salamander::BranchView::Progress progress;
    BranchView->Worker.Drain(batch, progress);
    const bool scanFinished = BranchView->Status.Running && !progress.Running;
    bool statusChanged = scanFinished || progress.Files != BranchView->Status.Files ||
        progress.Errors != BranchView->Status.Errors || progress.Cancelled != BranchView->Status.Cancelled;
    BranchView->Status = progress;
    if (!batch.empty() || scanFinished)
    {
        // Match ReadDirectory's bounded UTF-8 basename mirror exactly. All file
        // operations continue using the original full wide paths.
        std::vector<char> name(sizeof(WIN32_FIND_DATAA().cFileName));
        for (const auto& entry : batch)
        {
            const size_t slash = entry.CacheKey.find_last_of('\\');
            const char* basename = entry.CacheKey.c_str() + (slash == std::string::npos ? 0 : slash + 1);
            CopyStringTruncateUtf8(name.data(), (int)name.size(), basename);
            const char* dot = strrchr(name.data(), '.');
            if (dot != NULL && dot[1] != 0)
            {
                std::string extension(dot + 1);
                for (char& character : extension) character = LowerCase[(unsigned char)character];
                preparation->Queue.Add(extension);
            }
        }
        BranchView->IncomingEntries.insert(BranchView->IncomingEntries.end(),
            std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
        BranchView->Dirty = true;
    }
    if (scanFinished) BranchView->CompletionPending = true;
    const ULONGLONG epoch = Associations.GetShellAssociationEpoch();
    if (preparation->AssociationEpoch != epoch)
    {
        preparation->AssociationEpoch = epoch;
        preparation->Queue.Restart();
    }
    preparation->Queue.RunSlice([] { return GetTickCount64(); },
        [&](const std::string& extension) {
            Associations.PrepareShellAssociation(extension);
            return preparation->Alive && Associations.GetShellAssociationEpoch() == epoch;
        });
    if (!preparation->Alive) return; // Cancel/Leave/Shutdown may have destroyed this panel.
    if (StopRefresh || SnooperSuspended) return;
    if (Associations.GetShellAssociationEpoch() != epoch)
    {
        preparation->AssociationEpoch = Associations.GetShellAssociationEpoch();
        preparation->Queue.Restart();
    }
    const bool preparing = BranchView->CompletionPending && preparation->Queue.HasPending();
    statusChanged = statusChanged || preparing != BranchView->Preparing;
    BranchView->Preparing = preparing;
    const bool finished = BranchView->CompletionPending && !preparation->Queue.HasPending();
    const size_t previewCount = Salamander::BranchView::InitialPreviewCount(
        BranchView->IncomingEntries.size(), BranchView->Entries.size());
    const bool publish = BranchView->Dirty && !preparation->Queue.HasPending() &&
        (finished || (!BranchView->PreserveListing && previewCount > BranchView->Entries.size() &&
                      GetTickCount() - BranchView->LastPublish >= 1000));
    if (publish)
    {
        if (!finished || BranchView->ForcePublish ||
            !Salamander::BranchView::SameSnapshot(BranchView->Entries, BranchView->IncomingEntries))
        {
            if (finished) BranchView->Entries.swap(BranchView->IncomingEntries);
            else BranchView->Entries.assign(BranchView->IncomingEntries.begin(),
                BranchView->IncomingEntries.begin() + previewCount);
            ++BranchView->Generation;
            BranchView->Applying = true;
            RefreshDirectory();
            if (!lifetime->Alive) return;
            BranchView->Applying = false;
            if (!preparation->Alive) return;
            // RefreshDirectory has released/pruned the previous listing. A
            // still-running property job restarts after its generation check.
            StartExplorerSortAsync(Files, Dirs,
                Dirs->Count > 0 && strcmp(Dirs->At(0).Name, "..") == 0 ? 1 : 0);
        }
        BranchView->LastPublish = GetTickCount();
        BranchView->Dirty = false;
        if (finished)
        {
            BranchView->IncomingEntries.clear();
            BranchView->CompletionPending = false;
            // Pending notifications may start a new scan only after this
            // snapshot has been published (or confirmed unchanged).
            BranchView->Refresh.Finished(GetTickCount64());
            statusChanged = true;
        }
    }
    if (BranchView->Refresh.Due(GetTickCount64())) StartBranchViewScan(this);
    if (statusChanged) { DirectoryLineSetText(); IdleRefreshStates = TRUE; }
    if (!BranchView->Status.Running && !BranchView->Preparing &&
        !BranchView->Refresh.HasPending() && !BranchView->Dirty)
        KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
    else
        SetTimer(HWindow, IDT_BRANCHVIEW_POLL,
            preparation->Queue.HasPending() ? 15 : 200, NULL);
}
void CFilesWindow::BranchItemRenamed(const char* oldNameKey, const CFileData& file, const std::wstring& oldFullPath)
{
    if (!IsBranchView()) return;
    {
        std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
        auto item = BranchView->Items.find(oldNameKey);
        if (item == BranchView->Items.end()) return;
        Salamander::BranchView::Entry entry = item->second;
        BranchView->Items.erase(item);
        std::wstring name = file.UseWideName() ? file.NameW : SalMultiByteToWidePath(file.Name);
        wcsncpy_s(entry.Data.cFileName, _countof(entry.Data.cFileName), name.c_str(), _TRUNCATE);
        size_t slash = entry.RelativePath.find_last_of(L'\\');
        entry.RelativePath = (slash == std::wstring::npos ? L"" : entry.RelativePath.substr(0, slash + 1)) + name;
        entry.CacheKey = SalWideToMultiBytePath(entry.FullPath().c_str(), CP_UTF8);
        BranchView->CacheFiles.erase(SalWideToMultiBytePath(oldFullPath.c_str(), CP_UTF8));
        BranchView->CacheFiles[entry.CacheKey] = &file;
        BranchView->Items[file.Name] = entry;
        for (auto& old : BranchView->Entries) if (old.FullPath() == oldFullPath) old = entry;
        BranchView->PendingFocus = entry.FullPath();
    } // No callbacks or refresh while holding the metadata lock.
    CancelBranchViewScan();
    RefreshBranchView();
}
BOOL CFilesWindow::OpenBranchItemDirectory()
{
    if (!IsBranchView()) return FALSE;
    int index = GetCaretIndex() - Dirs->Count;
    if (index < 0 || index >= Files->Count) return FALSE;
    const CFileData& file = Files->At(index);
    if (GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(file)) != &file) return FALSE;
    std::wstring path = GetItemDirectoryW(Files->At(index));
    std::wstring name = Files->At(index).UseWideName() ? Files->At(index).NameW : SalMultiByteToWidePath(Files->At(index).Name);
    if (path.empty() || name.empty()) return FALSE;
    // A vanished source directory must not enter ChangePathToDisk's fallback,
    // which closes Branch View before navigating to the nearest existing parent.
    std::wstring checkedPath = path;
    if (checkedPath.size() >= MAX_PATH && checkedPath.compare(0, 4, L"\\\\?\\") != 0)
        checkedPath = checkedPath.compare(0, 2, L"\\\\") == 0 ? L"\\\\?\\UNC\\" + checkedPath.substr(2) : L"\\\\?\\" + checkedPath;
    DWORD attributes = GetFileAttributesW(checkedPath.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) return FALSE;
    if (!ChangePathToDiskW(HWindow, path.c_str())) return FALSE;
    // CommonRefresh matches legacy Name. Use the owned exact wide basename
    // after the old Branch rows have been replaced by the real directory.
    for (int i = 0; i < Files->Count; ++i)
    {
        const CFileData& current = Files->At(i);
        const std::wstring currentName = current.UseWideName() ? current.NameW :
            SalMultiByteToWidePath(current.Name, IsValidPathUtf8Text(current.Name) ? CP_UTF8 : CP_ACP);
        if (currentName == name)
        {
            SetCaretIndex(Dirs->Count + i, TRUE);
            break;
        }
    }
    return TRUE;
}
#endif
