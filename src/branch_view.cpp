// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
//
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Preserve ordinary traversal of non-name-surrogate directories. | Keep readable descendants without extending the verified elevation scope. | #731 | GPT-6.1 Terra |
// | 2026-11-12 | Detect identity and metadata changes during partial retries. | Retain previous rows without merging same-name replacements. | #731 | GPT-6.1 Terra |
// | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
// | 2026-11-11 | Publish completed initial Branch batches on explicit Stop. | Keep accessible results without replacing an older refresh snapshot. | #731 | GPT-6.1 Terra |
// | 2026-11-10 | Enumerate a protected Branch tree through its approved panel session. | Keep anchored listing requests on the panel thread and share row traversal. | #731 | GPT-6.1 Sol |
// | 2026-11-07 | Skip hidden and system files in Branch scans, except cloud placeholders. | Match the ordinary panel hide option. | https://github.com/KRtkovo-eu-AI/salamander/pull/818 | Grok 4.7 |
#ifndef BRANCH_VIEW_TEST
#include "precomp.h"
#endif
#include "branch_view.h"
#include "branch_view_text.h"
#include <new>
#include <algorithm>

#ifndef IO_REPARSE_TAG_FILE_PLACEHOLDER
#define IO_REPARSE_TAG_FILE_PLACEHOLDER (0x80000015L)
#endif

namespace Salamander { namespace BranchView {
bool OmitHiddenSystemEntry(DWORD attributes, DWORD reparseTag)
{
    if ((attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0)
        return false;
    return !((attributes & FILE_ATTRIBUTE_REPARSE_POINT) && reparseTag == IO_REPARSE_TAG_FILE_PLACEHOLDER);
}
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

static bool ValidPartialRowName(const WIN32_FIND_DATAW& data)
{
    const size_t length = wcsnlen_s(data.cFileName, ARRAYSIZE(data.cFileName));
    if (!length || length == ARRAYSIZE(data.cFileName) ||
        wcsnlen_s(data.cAlternateFileName, ARRAYSIZE(data.cAlternateFileName)) == ARRAYSIZE(data.cAlternateFileName) ||
        (length == 1 && data.cFileName[0] == L'.') ||
        (length == 2 && data.cFileName[0] == L'.' && data.cFileName[1] == L'.')) return false;
    for (size_t i = 0; i != length; ++i)
        if (data.cFileName[i] == L'\\' || data.cFileName[i] == L'/' || data.cFileName[i] == L':') return false;
    return true;
}
PartialDirectoryRows::Row PartialDirectoryRows::MakeRow(const WIN32_FIND_DATAW& data, const BYTE fileId[16])
{
    Row row;
    CopyMemory(row.FileId, fileId, sizeof(row.FileId));
    row.Attributes = data.dwFileAttributes;
    row.ReparseTag = data.dwReserved0;
    row.SizeHigh = data.nFileSizeHigh;
    row.SizeLow = data.nFileSizeLow;
    row.Created = data.ftCreationTime;
    row.Written = data.ftLastWriteTime;
    static_assert(sizeof(row.AlternateName) == sizeof(data.cAlternateFileName));
    CopyMemory(row.AlternateName, data.cAlternateFileName, sizeof(row.AlternateName));
    return row;
}
bool PartialDirectoryRows::SameRow(const Row& a, const Row& b)
{
    return memcmp(a.FileId, b.FileId, sizeof(a.FileId)) == 0 &&
           a.Attributes == b.Attributes && a.ReparseTag == b.ReparseTag &&
           a.SizeHigh == b.SizeHigh && a.SizeLow == b.SizeLow &&
           wcscmp(a.AlternateName, b.AlternateName) == 0 && CompareFileTime(&a.Created, &b.Created) == 0 && CompareFileTime(&a.Written, &b.Written) == 0;
}
PartialDirectoryRows::PartialDirectoryRows(const DeniedDirectory& parent) :
    Volume(parent.Volume), ParentVerified(parent.Verified)
{
    CopyMemory(ParentId, parent.FileId, sizeof(ParentId));
}
bool PartialDirectoryRows::ObserveOriginal(const WIN32_FIND_DATAW& data, const BYTE fileId[16])
{
    if (!ParentVerified || Started || Changed || !fileId || !ValidPartialRowName(data)) { Changed = true; return false; }
    Row row = MakeRow(data, fileId);
    row.NameOffset = Names.size();
    row.NameLength = wcslen(data.cFileName);
    Names.insert(Names.end(), data.cFileName, data.cFileName + row.NameLength);
    try { Rows.push_back(row); }
    catch (...) { Names.resize(row.NameOffset); throw; }
    return true;
}
bool PartialDirectoryRows::BeginRetry(ULONGLONG volume, const BYTE parentId[16])
{
    if (!ParentVerified || Started || Changed || !parentId || Volume != volume ||
        memcmp(ParentId, parentId, sizeof(ParentId)) != 0)
    {
        Changed = true;
        return false;
    }
    Index.reserve(Rows.size());
    for (size_t i = 0; i < Rows.size(); ++i) Index.push_back(i);
    std::sort(Index.begin(), Index.end(), [&](size_t a, size_t b) { return Name(Rows[a]) < Name(Rows[b]); });
    for (size_t i = 1; i < Index.size(); ++i)
        if (Name(Rows[Index[i - 1]]) == Name(Rows[Index[i]])) { Changed = true; return false; }
    Started = true;
    return true;
}
RetryRowDecision PartialDirectoryRows::ObserveRetry(const WIN32_FIND_DATAW& data, const BYTE fileId[16])
{
    if (!Started || !fileId || !ValidPartialRowName(data)) { Changed = true; return RetryRowDecision::Conflict; }
    const Row current = MakeRow(data, fileId);
    const std::wstring_view name(data.cFileName);
    const auto found = std::lower_bound(Index.begin(), Index.end(), name,
        [&](size_t index, std::wstring_view key) { return Name(Rows[index]) < key; });
    if (found == Index.end() || Name(Rows[*found]) != name)
    {
        Row added = current;
        added.Retried = true;
        if (!Added.emplace(std::wstring(name), added).second) { Changed = true; return RetryRowDecision::Conflict; }
        return RetryRowDecision::Add;
    }
    Row& previous = Rows[*found];
    if (previous.Retried || !SameRow(previous, current))
    {
        Changed = true;
        return RetryRowDecision::Conflict;
    }
    previous.Retried = true;
    return RetryRowDecision::KeepOriginal;
}
bool PartialDirectoryRows::Complete() const
{
    if (!Started || Changed) return false;
    for (const auto& row : Rows)
        if (!row.Retried) return false;
    return true;
}

struct Scan::State
{
    CRITICAL_SECTION Lock;
    volatile LONG Cancelled;
    std::wstring Root;
    bool HideHiddenSystem;
    bool Incremental = false;
    struct Directory
    {
        std::wstring first, second;
        DeniedDirectory Expected;
        bool UnverifiedTraversal = false;
        Directory(std::wstring path, std::wstring relative) : first(std::move(path)), second(std::move(relative)) {}
    };
    std::vector<Directory> PendingDirectories;
    DirectoryPager Pager;
    std::vector<Entry> Pending;
    std::vector<DeniedDirectory> Denied;
    std::wstring FailurePath;
    DWORD FailureError = ERROR_SUCCESS;
    CaptureDenied Capture = nullptr;
    ListDirectory List = nullptr;
    std::shared_ptr<void> ListContext;
    HANDLE StopEvent = NULL; // owned by ListContext
    Progress Status;
    State() : Cancelled(0), HideHiddenSystem(false) { InitializeCriticalSection(&Lock); }
    ~State() { DeleteCriticalSection(&Lock); }
    void Accept(const Directory& directory, const WIN32_FIND_DATAW& data,
                std::vector<Directory>& remaining, std::vector<Entry>& batch, Progress& progress,
                const BYTE* fileId = nullptr, ULONGLONG volume = 0);
    void Publish(std::vector<Entry>& batch, const Progress& progress);
    DWORD ReadLocalPaged(const Directory& directory, std::vector<Directory>& remaining,
                         std::vector<Entry>& batch, Progress& progress, bool& fallback,
                         bool& fallbackWithoutIdentity, DeniedDirectory& denied);
};

class StateLock
{
    CRITICAL_SECTION* Lock;
public:
    explicit StateLock(CRITICAL_SECTION& lock) : Lock(&lock) { EnterCriticalSection(Lock); }
    ~StateLock() { LeaveCriticalSection(Lock); }
};

void Scan::State::Accept(const Directory& directory, const WIN32_FIND_DATAW& data,
                         std::vector<Directory>& remaining, std::vector<Entry>& batch, Progress& progress,
                         const BYTE* fileId, ULONGLONG volume)
{
    if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0 ||
        (HideHiddenSystem && OmitHiddenSystemEntry(data.dwFileAttributes, data.dwReserved0)))
        return;
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        // The approved listing cannot descend through any reparse directory.
        // Ordinary scans retain their handling of non-name-surrogate directories.
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
            (Incremental || IsReparseTagNameSurrogate(data.dwReserved0)))
        {
            ++progress.SkippedLinks;
            return;
        }
        remaining.emplace_back(Join(directory.first, data.cFileName), Join(directory.second, data.cFileName));
        remaining.back().UnverifiedTraversal = directory.UnverifiedTraversal ||
            ((data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
        if (fileId && !remaining.back().UnverifiedTraversal)
        {
            auto& expected = remaining.back().Expected;
            expected.Path = remaining.back().first;
            expected.Relative = remaining.back().second;
            expected.Volume = volume;
            CopyMemory(expected.FileId, fileId, sizeof(expected.FileId));
            expected.Verified = true;
        }
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
}

void Scan::State::Publish(std::vector<Entry>& batch, const Progress& progress)
{
    StateLock lock(Lock);
    Pending.insert(Pending.end(), std::make_move_iterator(batch.begin()), std::make_move_iterator(batch.end()));
    batch.clear();
    Status = progress;
}

DWORD Scan::State::ReadLocalPaged(const Directory& directory, std::vector<Directory>& remaining,
                                   std::vector<Entry>& batch, Progress& progress, bool& fallback,
                                   bool& fallbackWithoutIdentity, DeniedDirectory& denied)
{
    fallback = false;
    fallbackWithoutIdentity = false;
    void* cursor = nullptr;
    DeniedDirectory actual;
    const DeniedDirectory* expected = directory.Expected.Verified ? &directory.Expected : nullptr;
    DWORD error = Pager.Open(nullptr, directory.first, expected, actual, cursor);
    if (error)
    {
        // A no-follow cursor may need more access to pin ancestors than the
        // ordinary pathname enumerator. Preserve readable non-elevated work at
        // an unbound root; the legacy denial path still requests consent if
        // FindFirstFileExW itself also fails. Never discard an expected ID.
        const bool unsupported = error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION ||
                                 error == ERROR_INVALID_PARAMETER;
        fallback = unsupported || error == ERROR_SHARING_VIOLATION ||
                   (!expected && (error == ERROR_ACCESS_DENIED || error == ERROR_REPARSE_TAG_INVALID));
        fallbackWithoutIdentity = fallback && (expected != nullptr || error != ERROR_ACCESS_DENIED);
        if (error == ERROR_ACCESS_DENIED && expected) denied = *expected;
        return error;
    }
    struct CursorGuard
    {
        DirectoryPager& Pager;
        void* Cursor;
        ~CursorGuard() { if (Cursor) Pager.Close(nullptr, Cursor); }
        std::shared_ptr<void> Retain()
        {
            const auto close = Pager.Close;
            void* held = Cursor;
            Cursor = nullptr; // shared_ptr invokes the deleter itself if allocation fails
            return std::shared_ptr<void>(held, [close](void* value) { close(nullptr, value); });
        }
    } guard{Pager, cursor};
    if (!actual.Verified || (expected && (actual.Volume != expected->Volume ||
        memcmp(actual.FileId, expected->FileId, sizeof(actual.FileId)) != 0)))
        return ERROR_FILE_INVALID;
    actual.Path = directory.first;
    actual.Relative = directory.second;
    auto partial = std::make_shared<PartialDirectoryRows>(actual);
    std::vector<IdentifiedPageRow> page;
    bool done = false, publishedPage = false;
    while (!done && !InterlockedCompareExchange(&Cancelled, 0, 0))
    {
        page.clear();
        error = Pager.Next(nullptr, cursor, page, done);
        if (error)
        {
            const bool unsupported = error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION ||
                                     error == ERROR_INVALID_PARAMETER;
            fallback = !publishedPage && unsupported;
            fallbackWithoutIdentity = fallback;
            if (error == ERROR_ACCESS_DENIED)
            {
                denied = actual;
                denied.Anchor = guard.Retain();
                if (publishedPage) denied.Partial = std::move(partial);
            }
            return error;
        }
        if (page.empty() && !done) return ERROR_INVALID_DATA;
        for (const auto& row : page)
        {
            if (InterlockedCompareExchange(&Cancelled, 0, 0)) break;
            if (!partial->ObserveOriginal(row.Data, row.FileId)) return ERROR_INVALID_DATA;
            Accept(directory, row.Data, remaining, batch, progress, row.FileId, actual.Volume);
            if (batch.size() >= 256) Publish(batch, progress);
        }
        publishedPage = publishedPage || !page.empty();
    }
    return ERROR_SUCCESS;
}

DWORD WINAPI Scan::Run(void* context)
{
    std::unique_ptr<std::shared_ptr<State>> reference(static_cast<std::shared_ptr<State>*>(context));
    std::shared_ptr<State> state = *reference;
    std::vector<State::Directory> pending;
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
            // The ordinary scanner historically traverses non-name-surrogate
            // reparse directories. Keep that path and its descendants strictly
            // unprivileged; pathname enumeration cannot bind the observed ID.
            bool fallbackWithoutIdentity = directory.UnverifiedTraversal;
            bool usedPagedFallback = false;
            if (state->Pager.Valid() && !directory.UnverifiedTraversal)
            {
                bool fallback = false;
                DeniedDirectory denied;
                const DWORD error = state->ReadLocalPaged(directory, pending, batch, progress, fallback,
                                                          fallbackWithoutIdentity, denied);
                if (!fallback)
                {
                    ++progress.Directories;
                    if (error && !InterlockedCompareExchange(&state->Cancelled, 0, 0))
                    {
                        if (error == ERROR_ACCESS_DENIED && state->Capture)
                        {
                            if (!denied.Anchor)
                            {
                                denied.Path = directory.first;
                                denied.Relative = directory.second;
                                // Even a row ID from a readable parent needs an
                                // actual child handle to prevent ID recycling.
                                denied.Verified = state->Capture(directory.first, denied);
                            }
                            StateLock lock(state->Lock);
                            if (!InterlockedCompareExchange(&state->Cancelled, 0, 0))
                                state->Denied.push_back(std::move(denied));
                        }
                        else { ++progress.Errors; progress.LastError = error; }
                    }
                    state->Publish(batch, progress);
                    continue;
                }
                // Descendants of an unbound fallback cannot establish an
                // elevated scope merely by recapturing their current path.
                usedPagedFallback = true;
                directory.UnverifiedTraversal = fallbackWithoutIdentity;
            }
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
                    if (error == ERROR_ACCESS_DENIED && state->Capture && !fallbackWithoutIdentity)
                    {
                        DeniedDirectory denied;
                        denied.Path = directory.first;
                        denied.Relative = directory.second;
                        denied.Verified = state->Capture(directory.first, denied);
                        StateLock lock(state->Lock);
                        if (!InterlockedCompareExchange(&state->Cancelled, 0, 0))
                            state->Denied.push_back(std::move(denied));
                    }
                    else
                    {
                        ++progress.Errors;
                        progress.LastError = error;
                        if (error == ERROR_ACCESS_DENIED && fallbackWithoutIdentity)
                        {
                            progress.IdentityUnavailable = true;
                            StateLock lock(state->Lock);
                            if (state->FailurePath.empty()) { state->FailurePath = directory.first; state->FailureError = error; }
                        }
                    }
                }
            }
            else
            {
                // The access-denied root fallback may safely capture the root
                // if FindFirst itself fails. If it succeeds, however, those
                // pathname-enumerated children have no verified parent binding.
                if (usedPagedFallback) directory.UnverifiedTraversal = true;
                do
                {
                    if (InterlockedCompareExchange(&state->Cancelled, 0, 0)) break;
                    state->Accept(directory, data, pending, batch, progress);
                    if (batch.size() >= 256)
                        state->Publish(batch, progress);
                } while (FindNextFileW(search, &data));
                DWORD error = GetLastError();
                FindClose(search);
                search = INVALID_HANDLE_VALUE;
                if (!InterlockedCompareExchange(&state->Cancelled, 0, 0) && error != ERROR_NO_MORE_FILES)
                {
                    ++progress.Errors;
                    progress.LastError = error;
                    if (error == ERROR_ACCESS_DENIED)
                    {
                        // FindFirst/FindNext does not expose the opened parent
                        // or row IDs: never merge this partial listing by name.
                        progress.IdentityUnavailable = true;
                        StateLock lock(state->Lock);
                        if (state->FailurePath.empty()) { state->FailurePath = directory.first; state->FailureError = error; }
                    }
                }
            }
            // Small directories publish without waiting for a full batch.
            state->Publish(batch, progress);
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
    if (Shared)
    {
        InterlockedExchange(&Shared->Cancelled, 1);
        if (Shared->StopEvent) SetEvent(Shared->StopEvent);
        std::vector<DeniedDirectory> abandoned;
        {
            StateLock lock(Shared->Lock);
            abandoned.swap(Shared->Denied);
        } // Retained directory anchors are released outside the state lock.
    }
    if (Thread != NULL)
    {
        CancelSynchronousIo(Thread);
        CloseHandle(Thread); // worker retains its own shared state until return
        Thread = NULL;
    }
}
bool Scan::Start(const std::wstring& root, bool hideHiddenSystem, bool incremental)
{
    Cancel();
    try
    {
        Shared = std::make_shared<State>();
        Shared->Root = root;
        Shared->Capture = incremental ? nullptr : Capture;
        Shared->Pager = incremental ? DirectoryPager{} : LocalPager;
        Shared->HideHiddenSystem = hideHiddenSystem;
        Shared->Status.Running = true;
        if (incremental)
        {
            Shared->Incremental = true;
            Shared->PendingDirectories.emplace_back(root, L"");
            return true;
        }
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
DWORD WINAPI Scan::RunProtected(void* context)
{
    std::unique_ptr<std::shared_ptr<State>> reference(static_cast<std::shared_ptr<State>*>(context));
    auto state = *reference;
    auto& pending = state->PendingDirectories;
    Progress progress = state->Status;
    std::vector<Entry> batch;
    std::vector<WIN32_FIND_DATAW> entries;
    try
    {
        while (!pending.empty() && !InterlockedCompareExchange(&state->Cancelled, 0, 0))
        {
            auto directory = std::move(pending.back());
            pending.pop_back();
            entries.clear();
            const DWORD error = state->List(state->ListContext.get(), directory.first, entries);
            if (InterlockedCompareExchange(&state->Cancelled, 0, 0)) break;
            ++progress.Directories;
            if (error == ERROR_SUCCESS)
                for (const auto& entry : entries)
                    state->Accept(directory, entry, pending, batch, progress);
            else
            {
                ++progress.Errors;
                progress.LastError = error;
                if (state->FailurePath.empty())
                {
                    StateLock lock(state->Lock);
                    state->FailurePath = directory.first;
                    state->FailureError = error;
                }
                if (error == ERROR_BUFFER_OVERFLOW) progress.LimitExceeded = true;
                if (error == ERROR_BROKEN_PIPE)
                {
                    // Every queued directory is now known to be unprocessed.
                    const size_t available = static_cast<size_t>(UINT_MAX - progress.Errors);
                    progress.Errors += static_cast<unsigned>((std::min)(pending.size(), available));
                    pending.clear();
                }
            }
            state->Publish(batch, progress);
        }
    }
    catch (const std::bad_alloc&)
    {
        ++progress.Errors;
        progress.LastError = ERROR_NOT_ENOUGH_MEMORY;
    }
    progress.Cancelled = InterlockedCompareExchange(&state->Cancelled, 0, 0) != 0;
    progress.Running = false;
    try { state->Publish(batch, progress); }
    catch (const std::bad_alloc&)
    {
        ++progress.Errors;
        progress.LastError = ERROR_NOT_ENOUGH_MEMORY;
        StateLock lock(state->Lock);
        state->Status = progress;
    }
    return 0;
}

bool Scan::StartProtected(const std::vector<DeniedDirectory>& denied, const Progress& initial,
                          ListDirectory list, std::shared_ptr<void> context, HANDLE stop)
{
    if (!Shared || !list || !context || !stop || denied.empty()) return false;
    try
    {
        auto state = std::make_shared<State>();
        state->Root = Shared->Root;
        state->HideHiddenSystem = Shared->HideHiddenSystem;
        state->Incremental = true;
        state->Status = initial;
        state->Status.Running = true;
        state->Status.Cancelled = false;
        state->List = list;
        state->ListContext = std::move(context);
        state->StopEvent = stop;
        for (auto it = denied.rbegin(); it != denied.rend(); ++it)
        {
            if (!it->Verified || !IsWithin(state->Root, it->Path) ||
                (it->Relative.empty() && it->Path != state->Root))
                return false;
            state->PendingDirectories.emplace_back(it->Path, it->Relative);
        }
        auto reference = std::make_unique<std::shared_ptr<State>>(state);
        HANDLE thread = CreateThread(NULL, 0, RunProtected, reference.get(), 0, NULL);
        if (!thread) return false;
        reference.release();
        Cancel();
        Shared = std::move(state);
        Thread = thread;
        return true;
    }
    catch (const std::bad_alloc&) { return false; }
}

DWORD WINAPI Scan::RunProtectedPaged(void* context)
{
    std::unique_ptr<std::shared_ptr<State>> reference(static_cast<std::shared_ptr<State>*>(context));
    auto state = *reference;
    auto& pending = state->PendingDirectories;
    Progress progress = state->Status;
    std::vector<Entry> batch;
    std::vector<IdentifiedPageRow> page;
    try
    {
        while (!pending.empty() && !InterlockedCompareExchange(&state->Cancelled, 0, 0))
        {
            auto directory = std::move(pending.back());
            pending.pop_back();
            auto& expected = directory.Expected;
            DeniedDirectory actual;
            void* cursor = nullptr;
            DWORD error = state->Pager.Open(state->ListContext.get(), directory.first,
                                            expected.Verified ? &expected : nullptr, actual, cursor);
            ++progress.Directories;
            if (!error)
            {
                struct CursorGuard
                {
                    State& state;
                    void* cursor;
                    ~CursorGuard() { state.Pager.Close(state.ListContext.get(), cursor); }
                } guard{*state, cursor};
                if (!actual.Verified || (expected.Verified && (actual.Volume != expected.Volume ||
                    memcmp(actual.FileId, expected.FileId, sizeof(actual.FileId)) != 0)) ||
                    (expected.Partial && !expected.Partial->BeginRetry(actual.Volume, actual.FileId)))
                    error = ERROR_FILE_INVALID;
                // A directory handle pins the parent, but not every previously
                // observed child. An ID may be recycled and filesystem changes
                // during two passes do not form an atomic snapshot.
                if (!error && expected.Partial) progress.UncertainMerge = true;
                bool done = false, changed = false;
                while (!error && !done && !InterlockedCompareExchange(&state->Cancelled, 0, 0))
                {
                    page.clear();
                    error = state->Pager.Next(state->ListContext.get(), cursor, page, done);
                    if (error || InterlockedCompareExchange(&state->Cancelled, 0, 0)) break;
                    if (page.empty() && !done) { error = ERROR_INVALID_DATA; break; }
                    for (const auto& row : page)
                    {
                        if (InterlockedCompareExchange(&state->Cancelled, 0, 0)) break;
                        if (expected.Partial)
                        {
                            const auto decision = expected.Partial->ObserveRetry(row.Data, row.FileId);
                            if (decision == RetryRowDecision::KeepOriginal) continue;
                            if (decision == RetryRowDecision::Conflict) { changed = true; continue; }
                        }
                        state->Accept(directory, row.Data, pending, batch, progress, row.FileId, actual.Volume);
                        if (batch.size() >= 256) state->Publish(batch, progress);
                    }
                    state->Publish(batch, progress);
                }
                if (!error && !InterlockedCompareExchange(&state->Cancelled, 0, 0) &&
                    (changed || (expected.Partial && !expected.Partial->Complete())))
                    error = ERROR_FILE_INVALID;
            }
            if (InterlockedCompareExchange(&state->Cancelled, 0, 0)) break;
            if (error)
            {
                ++progress.Errors;
                progress.LastError = error;
                if (state->FailurePath.empty())
                {
                    StateLock lock(state->Lock);
                    state->FailurePath = directory.first;
                    state->FailureError = error;
                }
                if (error == ERROR_BUFFER_OVERFLOW) progress.LimitExceeded = true;
                if (error == ERROR_BROKEN_PIPE)
                {
                    const size_t available = static_cast<size_t>(UINT_MAX - progress.Errors);
                    progress.Errors += static_cast<unsigned>((std::min)(pending.size(), available));
                    pending.clear();
                }
            }
            state->Publish(batch, progress);
        }
    }
    catch (const std::bad_alloc&)
    {
        ++progress.Errors;
        progress.LastError = ERROR_NOT_ENOUGH_MEMORY;
    }
    progress.Cancelled = InterlockedCompareExchange(&state->Cancelled, 0, 0) != 0;
    progress.Running = false;
    try { state->Publish(batch, progress); }
    catch (const std::bad_alloc&)
    {
        ++progress.Errors;
        progress.LastError = ERROR_NOT_ENOUGH_MEMORY;
        StateLock lock(state->Lock);
        state->Status = progress;
    }
    return 0;
}

bool Scan::StartProtectedPaged(const std::vector<DeniedDirectory>& denied, const Progress& initial,
                               const DirectoryPager& pager, std::shared_ptr<void> context, HANDLE stop)
{
    if (!Shared || !pager.Valid() || !context || !stop || denied.empty()) return false;
    try
    {
        auto state = std::make_shared<State>();
        state->Root = Shared->Root;
        state->HideHiddenSystem = Shared->HideHiddenSystem;
        state->Incremental = true;
        state->Status = initial;
        state->Status.Running = true;
        state->Status.Cancelled = false;
        state->Pager = pager;
        state->ListContext = std::move(context);
        state->StopEvent = stop;
        for (auto it = denied.rbegin(); it != denied.rend(); ++it)
        {
            if (!it->Verified || !IsWithin(state->Root, it->Path) ||
                (it->Relative.empty() && it->Path != state->Root)) return false;
            state->PendingDirectories.emplace_back(it->Path, it->Relative);
            state->PendingDirectories.back().Expected = *it;
        }
        auto reference = std::make_unique<std::shared_ptr<State>>(state);
        HANDLE thread = CreateThread(NULL, 0, RunProtectedPaged, reference.get(), 0, NULL);
        if (!thread) return false;
        reference.release();
        Cancel();
        Shared = std::move(state);
        Thread = thread;
        return true;
    }
    catch (const std::bad_alloc&) { return false; }
}

void Scan::DrainDenied(std::vector<DeniedDirectory>& denied)
{
    if (!Shared) return;
    StateLock lock(Shared->Lock);
    denied.swap(Shared->Denied);
}

void Scan::DrainFailure(std::wstring& path, DWORD& error)
{
    if (!Shared) return;
    StateLock lock(Shared->Lock);
    path.swap(Shared->FailurePath);
    error = Shared->FailureError;
    Shared->FailureError = ERROR_SUCCESS;
}

void Scan::Advance(ListDirectory list, void* context)
{
    if (!Shared || !Shared->Incremental || !Shared->Status.Running || Shared->Cancelled)
        return;

    auto& state = *Shared;
    auto& pending = state.PendingDirectories;
    Progress progress = state.Status;
    std::vector<Entry> batch;
    std::vector<WIN32_FIND_DATAW> entries;
    const ULONGLONG started = GetTickCount64();
    try
    {
        for (unsigned count = 0; !pending.empty() && count < 32; ++count)
        {
            if (count && GetTickCount64() - started >= 8)
                break;
            auto directory = std::move(pending.back());
            pending.pop_back();
            entries.clear();
            const DWORD error = list(context, directory.first, entries);
            ++progress.Directories;
            if (!error)
            {
                for (const auto& entry : entries)
                    state.Accept(directory, entry, pending, batch, progress);
            }
            else
            {
                ++progress.Errors;
                progress.LastError = error;
                if (error == ERROR_BROKEN_PIPE)
                    pending.clear();
            }
        }
        progress.Running = !pending.empty();
        state.Publish(batch, progress);
    }
    catch (const std::bad_alloc&)
    {
        pending.clear();
        progress.Running = false;
        ++progress.Errors;
        progress.LastError = ERROR_NOT_ENOUGH_MEMORY;
        state.Status = progress;
    }
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
#include "dialogs.h"
#include "operationrecycle.h"
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
std::wstring CFilesWindow::GetBranchViewDisplayDirectoryW(const CFileData& file) const
{
    if (!IsBranchView()) return GetItemDirectoryW(file);
    std::lock_guard<std::mutex> lock(BranchView->ItemsMutex);
    auto item = BranchView->Items.find(file.Name);
    if (item == BranchView->Items.end()) return std::wstring();
    return Salamander::BranchViewText::DisplayDirectory(item->second.Directory,
        item->second.RelativePath, Configuration.BranchViewRelativePaths != FALSE);
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
             BranchView->Status.Errors != 0 ? IDS_BRANCH_ERRORS :
             BranchView->Status.UncertainMerge ? IDS_BRANCH_MERGE_UNCERTAIN : IDS_BRANCH_READY;
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
    BranchView->StoppingInitial = false;
    BranchView->Status.Running = false;
    BranchView->Status.Cancelled = true;
    BranchView->Refresh.Cancel();
    BranchView->IncomingEntries.clear();
    BranchView->StoppedBatch.clear();
    BranchView->StoppedBatchNext = 0;
    BranchView->Dirty = false;
    if (HWindow != NULL) KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
    DirectoryLineSetText();
    IdleRefreshStates = TRUE;
}
void CFilesWindow::StopBranchViewScan()
{
    // A stopped worker can still publish after a blocked I/O returns. Never
    // drain it a second time while the first stopped snapshot is preparing.
    if (IsBranchView() && BranchView->StoppingInitial) return;
    if (!IsBranchView() || BranchView->Applying ||
        (BranchView->PreserveListing && !BranchView->RetryingProtected) ||
        (!BranchView->Status.Running && !BranchView->CompletionPending))
    {
        CancelBranchViewScan();
        return;
    }
    // Detach the worker before consuming its last published batch. A worker
    // still returning from I/O owns its old state and cannot publish into a
    // subsequent generation. Reentrant Poll holds the old preparation session.
    try
    {
        const bool hadPreparation = BranchView->Preparation != NULL;
        auto preparation = hadPreparation ?
            std::make_shared<Salamander::BranchView::PreparationSession>(*BranchView->Preparation) :
            std::make_shared<Salamander::BranchView::PreparationSession>();
        BranchView->Worker.Cancel();
        Salamander::BranchView::Progress progress;
        BranchView->Worker.Drain(BranchView->StoppedBatch, progress);
        BranchView->StoppedBatchNext = 0;
        if (BranchView->Preparation != NULL) BranchView->Preparation->Alive = false;
        if (!hadPreparation) preparation->AssociationEpoch = Associations.GetShellAssociationEpoch();
        BranchView->Preparation = std::move(preparation);
        BranchView->Status = progress;
        BranchView->Status.Running = false;
        BranchView->Status.Cancelled = true;
        BranchView->Refresh.Cancel();
        BranchView->Preparing = true;
        BranchView->CompletionPending = true;
        BranchView->StoppingInitial = true;
        BranchView->Dirty = true;
        SetTimer(HWindow, IDT_BRANCHVIEW_POLL, 15, NULL);
        DirectoryLineSetText();
        IdleRefreshStates = TRUE;
    }
    catch (const std::bad_alloc&)
    {
        BranchView->Status.LastError = ERROR_NOT_ENOUGH_MEMORY;
        ++BranchView->Status.Errors;
        CancelBranchViewScan();
        SalMessageBox(HWindow, GetErrorText(ERROR_NOT_ENOUGH_MEMORY), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
    }
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
// The ordinary worker captures a no-follow identity before any elevation UI.
// WIN32_FIND_DATAW contains no file ID, so a replacement before this capture
// cannot be attributed to the earlier enumeration.
static bool CaptureBranchDenied(const std::wstring& path, Salamander::BranchView::DeniedDirectory& result)
{
    UacF7::MoveFileIdentity listed{};
    const std::wstring plain = SalPathRemoveExtendedPrefixW(path.c_str());
    const bool isVolumeRoot = plain.size() == 3 && plain[1] == L':' && plain[2] == L'\\';
    if (plain.size() < 3 || plain[1] != L':' || GetDriveTypeW(plain.substr(0, 3).c_str()) != DRIVE_FIXED)
        return false;
    const bool listedOk = UacF7::ListingSession::CaptureDirectoryIdentity(plain, listed);
    if (!listedOk && !isVolumeRoot) return false;
    if (result.Verified && (!listedOk || result.Volume != listed.serial ||
        memcmp(result.FileId, listed.id, sizeof(result.FileId)) != 0)) return false;

    // A zero-access, no-follow handle can pin the original NTFS object even
    // when LIST and READ_ATTRIBUTES are denied. It does not lock the path:
    // after consent the broker still opens by name and checks this exact ID.
    const std::wstring native = SalPathAddExtendedPrefixW(plain.c_str());
    HANDLE dir = CreateFileW(native.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_OVERLAPPED, nullptr);
    if (dir == INVALID_HANDLE_VALUE) return false;
    OperationRecycleDetail::RecycleFileId id{};
    FILE_ATTRIBUTE_TAG_INFO attr{};
    wchar_t fs[16]{};
    const bool ok = GetFileInformationByHandleEx(dir, OperationRecycleDetail::recycleFileIdInfo, &id, sizeof(id)) &&
        GetFileInformationByHandleEx(dir, FileAttributeTagInfo, &attr, sizeof(attr)) &&
        (attr.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(attr.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
        GetVolumeInformationByHandleW(dir, nullptr, 0, nullptr, nullptr, nullptr, fs, ARRAYSIZE(fs)) &&
        _wcsicmp(fs, L"NTFS") == 0 && (!listedOk || (listed.serial == id.VolumeSerialNumber &&
        memcmp(listed.id, id.FileId, sizeof(listed.id)) == 0));
    if (!ok) { CloseHandle(dir); return false; }
    result.Volume = id.VolumeSerialNumber;
    CopyMemory(result.FileId, id.FileId, sizeof(result.FileId));
    result.Anchor = std::shared_ptr<void>(dir, [](void* handle) { CloseHandle(handle); });
    return true;
}

struct BranchRetryContext
{
    HANDLE Stop = NULL;
    std::shared_ptr<UacF7::ListingSession> Session;
    ~BranchRetryContext() { Session.reset(); if (Stop) CloseHandle(Stop); }
};
struct BrokerBranchCursor { ULONGLONG Lease = 0; };

static void CopyPagedRows(const std::vector<UacF7::PagedListingEntry>& source,
                          std::vector<Salamander::BranchView::IdentifiedPageRow>& target)
{
    target.clear();
    target.reserve(source.size());
    for (const auto& row : source)
    {
        Salamander::BranchView::IdentifiedPageRow converted;
        converted.Data = row.data;
        CopyMemory(converted.FileId, row.id, sizeof(converted.FileId));
        target.push_back(std::move(converted));
    }
}

static DWORD OpenLocalBranchPage(void*, const std::wstring& path,
                                 const Salamander::BranchView::DeniedDirectory* expected,
                                 Salamander::BranchView::DeniedDirectory& actual, void*& opaque)
{
    auto cursor = std::make_unique<UacF7::LocalDirectoryCursor>();
    UacF7::MoveFileIdentity required{}, found{};
    if (expected)
    {
        required.serial = expected->Volume;
        CopyMemory(required.id, expected->FileId, sizeof(required.id));
    }
    const DWORD error = cursor->Open(SalPathRemoveExtendedPrefixW(path.c_str()), expected ? &required : nullptr, found);
    if (error) return error;
    actual.Volume = found.serial;
    CopyMemory(actual.FileId, found.id, sizeof(actual.FileId));
    actual.Verified = true;
    opaque = cursor.release();
    return ERROR_SUCCESS;
}
static DWORD NextLocalBranchPage(void*, void* opaque,
                                 std::vector<Salamander::BranchView::IdentifiedPageRow>& rows, bool& done)
{
    std::vector<UacF7::PagedListingEntry> page;
    const DWORD error = static_cast<UacF7::LocalDirectoryCursor*>(opaque)->Next(page, done);
    if (!error) CopyPagedRows(page, rows);
    return error;
}
static void CloseLocalBranchPage(void*, void* opaque)
{
    delete static_cast<UacF7::LocalDirectoryCursor*>(opaque);
}
static DWORD OpenBrokerBranchPage(void* owner, const std::wstring& path,
                                  const Salamander::BranchView::DeniedDirectory* expected,
                                  Salamander::BranchView::DeniedDirectory& actual, void*& opaque)
{
    auto* context = static_cast<BranchRetryContext*>(owner);
    if (!context || !context->Session) return ERROR_ACCESS_DENIED;
    const std::wstring plain = SalPathRemoveExtendedPrefixW(path.c_str());
    UacF7::MoveFileIdentity required{}, found{};
    if (expected && expected->Verified)
    {
        required.serial = expected->Volume;
        CopyMemory(required.id, expected->FileId, sizeof(required.id));
    }
    else
    {
        UacF7::FindRootIdentity root;
        const DWORD error = context->Session->CaptureFindRoot(plain, root, context->Stop);
        if (error) return error;
        required.serial = root.serial;
        CopyMemory(required.id, root.id.Identifier, sizeof(required.id));
    }
    auto cursor = std::make_unique<BrokerBranchCursor>();
    // The old worker can still be retiring a cancelled lease. Wait only on
    // this worker, never on the panel UI thread, and honour its Stop event.
    DWORD error = ERROR_SUCCESS;
    const ULONGLONG deadline = GetTickCount64() + 10000;
    do
    {
        error = context->Session->OpenDirectoryCursor(plain, required, found, cursor->Lease, context->Stop);
        if (error != ERROR_BUSY) break;
        if (WaitForSingleObject(context->Stop, 50) == WAIT_OBJECT_0) return ERROR_CANCELLED;
    } while (GetTickCount64() < deadline);
    if (error) return error;
    actual.Volume = found.serial;
    CopyMemory(actual.FileId, found.id, sizeof(actual.FileId));
    actual.Verified = true;
    opaque = cursor.release();
    return ERROR_SUCCESS;
}
static DWORD NextBrokerBranchPage(void* owner, void* opaque,
                                  std::vector<Salamander::BranchView::IdentifiedPageRow>& rows, bool& done)
{
    auto* context = static_cast<BranchRetryContext*>(owner);
    auto* cursor = static_cast<BrokerBranchCursor*>(opaque);
    if (!context || !context->Session || !cursor) return ERROR_INVALID_HANDLE;
    std::vector<UacF7::PagedListingEntry> page;
    const DWORD error = context->Session->NextDirectoryPage(cursor->Lease, page, done, context->Stop);
    if (error) return error;
    CopyPagedRows(page, rows);
    return ERROR_SUCCESS;
}
static void CloseBrokerBranchPage(void* owner, void* opaque)
{
    auto* context = static_cast<BranchRetryContext*>(owner);
    std::unique_ptr<BrokerBranchCursor> cursor(static_cast<BrokerBranchCursor*>(opaque));
    if (context && context->Session && cursor)
        context->Session->CloseDirectoryCursor(cursor->Lease); // cleanup must not reuse the signalled Stop
}

// Called only after the complete ordinary snapshot is visible. The session is
// owned by the retry worker, never borrowed from another panel or UI thread.
static void RetryBranchDenied(CFilesWindow* panel)
{
    using namespace Salamander::BranchView;
    CBranchViewState* const state = panel->BranchView;
    std::vector<DeniedDirectory> denied;
    state->Worker.DrainDenied(denied);
    if (denied.empty()) return;
    const auto lifetime = state->PreparationLifetime;
    const auto preparation = state->Preparation;
    const ULONGLONG tab = panel->GetPanelTabId();
    const ULONGLONG generation = state->Generation;
    const std::wstring root = state->Root;
    auto current = [&]() {
        return lifetime->Alive && preparation->Alive && panel->BranchView == state &&
               panel->IsBranchView() && panel->GetPanelTabId() == tab &&
               state->Generation == generation && state->Root == root;
    };
    std::vector<DeniedDirectory> verified;
    std::wstring firstUnverified;
    for (auto& item : denied)
    {
        if (item.Verified) verified.push_back(std::move(item));
        else
        {
            if (firstUnverified.empty()) firstUnverified = item.Path;
            ++state->Status.Errors;
            state->Status.LastError = ERROR_ACCESS_DENIED;
        }
    }
    if (!firstUnverified.empty())
    {
        const std::string text = std::string(LoadStr(IDS_BRANCH_IDENTITY_UNAVAILABLE)) +
            "\n\n" + SalWideToMultiBytePath(firstUnverified.c_str(), CP_UTF8) +
            "\n\n" + GetErrorText(ERROR_ACCESS_DENIED);
        SalMessageBox(panel->HWindow, text.c_str(), LoadStr(IDS_BRANCH_VIEW), MB_OK | MB_ICONEXCLAMATION);
        if (!current()) return;
    }
    if (verified.empty()) return;
    auto session = std::make_shared<UacF7::ListingSession>();
    const std::wstring scope = SalPathRemoveExtendedPrefixW(root.c_str());
    if (!session->PrepareComparison(scope, false))
    {
        state->Status.Errors += (unsigned)verified.size();
        state->Status.LastError = ERROR_ACCESS_DENIED;
        const std::string text = std::string(LoadStr(IDS_BRANCH_IDENTITY_UNAVAILABLE)) + "\n\n" +
                                 SalWideToMultiBytePath(scope.c_str(), CP_UTF8);
        SalMessageBox(panel->HWindow, text.c_str(), LoadStr(IDS_BRANCH_VIEW), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    const std::string aliases = std::to_string(IDYES) + "\t" + LoadStr(IDS_MSGBOXBTN_SKIP) +
        "\t" + std::to_string(IDNO) + "\t" + LoadStr(IDS_MSGBOXBTN_SKIPALL) +
        "\t" + std::to_string(IDOK) + "\t" + LoadStr(IDS_AS_ADMIN_TITLE);
    size_t next = 0;
    while (next < verified.size())
    {
        const std::string message = std::string(LoadStr(IDS_UAC_LIST_CONFIRM)) + "\n\n" +
            SalWideToMultiBytePath(verified[next].Path.c_str(), CP_UTF8) + "\n\n" +
            LoadStr(IDS_UAC_SIZE_CONSENT_SCOPE) + "\n" + SalWideToMultiBytePath(scope.c_str(), CP_UTF8);
        MSGBOXEX_PARAMS consent{};
        consent.HParent = panel->HWindow;
        consent.Text = message.c_str();
        consent.Caption = LoadStr(IDS_BRANCH_VIEW);
        consent.Flags = MSGBOXEX_YESNOOKCANCEL | MSGBOXEX_ICONQUESTION | MSGBOXEX_DEFBUTTON4 | MSGBOXEX_SHIELDONOKBTN;
        consent.AliasBtnNames = aliases.c_str();
        const int choice = SalMessageBoxEx(&consent);
        if (!current()) return;
        if (choice == IDYES) { ++next; ++state->Status.Errors; state->Status.LastError = ERROR_ACCESS_DENIED; continue; }
        if (choice != IDOK)
        {
            state->Status.Errors += (unsigned)(verified.size() - next);
            state->Status.LastError = ERROR_ACCESS_DENIED;
            if (choice == IDCANCEL) state->Refresh.Cancel();
            return;
        }
        break;
    }
    if (next == verified.size()) return;
    verified.erase(verified.begin(), verified.begin() + next);
    // Build the replacement snapshot before elevation, keeping ordinary rows
    // visible throughout a long or cancelled retry.
    std::vector<Entry> accessible = state->Entries;
    auto context = std::make_shared<BranchRetryContext>();
    context->Stop = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!context->Stop)
    {
        state->Status.Errors += (unsigned)verified.size();
        state->Status.LastError = GetLastError();
        return;
    }
    const DWORD started = session->Start(panel->HWindow, context->Stop);
    if (!current()) return;
    if (started != ERROR_SUCCESS)
    {
        state->Status.Errors += (unsigned)verified.size();
        state->Status.LastError = started;
        if (started == ERROR_CANCELLED) state->Refresh.Cancel();
        return;
    }
    context->Session = std::move(session);
    const DirectoryPager pager{OpenBrokerBranchPage, NextBrokerBranchPage, CloseBrokerBranchPage};
    if (!state->Worker.StartProtectedPaged(verified, state->Status, pager, context, context->Stop))
    {
        state->Status.Errors += (unsigned)verified.size();
        state->Status.LastError = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }
    state->IncomingEntries = std::move(accessible);
    state->RetryingProtected = true;
    state->PreserveListing = true;
    state->ForcePublish = true;
    state->Status.Running = true;
    state->Status.Cancelled = false;
    state->Refresh.Continue(GetTickCount64());
}

static void StartBranchViewScan(CFilesWindow* panel)
{
    CBranchViewState& state = *panel->BranchView;
    if (state.PanelScopePending)
    {
        bool contains = false;
        if (panel->ProtectedListing &&
            !panel->ProtectedListing->TryContains(SalPathRemoveExtendedPrefixW(state.Root.c_str()), contains))
        {
            SetTimer(panel->HWindow, IDT_BRANCHVIEW_POLL, 200, NULL);
            return; // another session operation owns the pipe; never block the UI
        }
        state.UseProtectedListing = contains;
        state.PanelScopePending = false;
    }
    // An explicit refresh may apply new file filters, hidden names or property
    // columns even when the filesystem snapshot itself has not changed.
    state.RetryingProtected = false;
    if (state.UseProtectedListing && !panel->ProtectedListing) state.UseProtectedListing = false;
    state.Worker.SetCaptureDenied(state.UseProtectedListing ? nullptr : CaptureBranchDenied);
    state.Worker.SetLocalPager(state.UseProtectedListing ? Salamander::BranchView::DirectoryPager{} :
        Salamander::BranchView::DirectoryPager{OpenLocalBranchPage, NextLocalBranchPage, CloseLocalBranchPage});
    state.ForcePublish = state.Refresh.IsManual();
    state.Refresh.Started(GetTickCount64());
    state.PreserveListing = state.HasCompletedSnapshot || !state.Entries.empty();
    state.IncomingEntries.clear();
    state.StoppedBatch.clear();
    state.StoppedBatchNext = 0;
    if (state.Preparation != NULL) state.Preparation->Alive = false;
    state.Preparing = false;
    state.CompletionPending = false;
    state.StoppingInitial = false;
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
    // Until all foreground calls can defer without blocking, keep the
    // existing panel session on its original UI-owned path. A retry that
    // needs its own consent still has a separate background session.
    const bool started = state.Worker.Start(state.Root, Configuration.NotHiddenSystemFiles != FALSE,
                                            state.UseProtectedListing);
    state.Status = Salamander::BranchView::Progress();
    state.Status.Running = started;
    if (!started)
    {
        state.Status.Errors = 1;
        state.Status.LastError = ERROR_NOT_ENOUGH_MEMORY;
        state.Refresh.Cancel();
    }
    state.Dirty = false;
    SetTimer(panel->HWindow, IDT_BRANCHVIEW_POLL, 200, NULL);
    panel->DirectoryLineSetText();
    IdleRefreshStates = TRUE;
}
void CFilesWindow::RefreshBranchView(BOOL automatic)
{
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
    if (!state.Enabled || !Is(ptDisk)) return;
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
    if (StopRefresh || SnooperSuspended || FilesActionInProgress) return; // No mode switch in a reentrant operation callback.
    if (!Is(ptDisk)) return;
    if (IsBranchView())
    {
        const int top = BranchView->OriginalTop;
        const int offset = BranchView->OriginalOffset;
        const std::wstring focus = BranchView->OriginalFocus;
        LeaveBranchView();
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
    BranchView->UseProtectedListing = false;
    BranchView->PanelScopePending = ProtectedListing != nullptr;
    BranchView->Enabled = true;
    ++BranchView->Generation; // mode change invalidates ordinary-panel property jobs
    BranchView->RestoreState.reset();
    BranchView->RestoreSelected.clear();
    // Keep Icons, Thumbnails and custom Detailed templates intact.
    BuildColumnsTemplate();
    BranchView->Entries.clear();
    BranchView->HasCompletedSnapshot = false;
    // Clear the ordinary directory rows immediately; a Branch collection only
    // contains files and the navigation row, even before its first batch.
    BranchView->Applying = true;
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
    if (BranchView->PanelScopePending && BranchView->Refresh.Due(GetTickCount64()))
    {
        StartBranchViewScan(this);
        if (BranchView->PanelScopePending) return;
    }
    // Stop Scan is persistent until an explicit refresh. A cancelled worker may
    // still finish a network request, but must never publish its late batch.
    if (BranchView->Status.Cancelled && !BranchView->StoppingInitial)
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
    bool statusChanged = false, stoppedBatchPending = false, finished = false;
    try
    {
    std::vector<Salamander::BranchView::Entry> batch;
    Salamander::BranchView::Progress progress;
    if (BranchView->UseProtectedListing && !BranchView->StoppingInitial)
    {
        BranchView->Worker.Advance([](void* context, const std::wstring& directory,
                                     std::vector<WIN32_FIND_DATAW>& entries) -> DWORD {
            auto* session = static_cast<UacF7::ListingSession*>(context);
            const auto path = SalPathRemoveExtendedPrefixW(directory.c_str());
            if (!session || !session->Contains(path)) return ERROR_BROKEN_PIPE;
            const DWORD error = session->List(path, entries);
            return session->IsOpen() ? error : ERROR_BROKEN_PIPE;
        }, ProtectedListing.get());
    }
    if (BranchView->StoppingInitial)
    {
        progress = BranchView->Status;
        const size_t end = (std::min)(BranchView->StoppedBatchNext + 256, BranchView->StoppedBatch.size());
        batch.insert(batch.end(),
                     std::make_move_iterator(BranchView->StoppedBatch.begin() + BranchView->StoppedBatchNext),
                     std::make_move_iterator(BranchView->StoppedBatch.begin() + end));
        BranchView->StoppedBatchNext = end;
    }
    else
        BranchView->Worker.Drain(batch, progress);
    const bool scanFinished = BranchView->Status.Running && !progress.Running;
    statusChanged = scanFinished || progress.Files != BranchView->Status.Files ||
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
    stoppedBatchPending = BranchView->StoppingInitial &&
        BranchView->StoppedBatchNext < BranchView->StoppedBatch.size();
    const bool preparing = BranchView->CompletionPending && (preparation->Queue.HasPending() || stoppedBatchPending);
    statusChanged = statusChanged || preparing != BranchView->Preparing;
    BranchView->Preparing = preparing;
    finished = BranchView->CompletionPending && !preparation->Queue.HasPending() && !stoppedBatchPending;
    }
    catch (const std::bad_alloc&)
    {
        if (!lifetime->Alive || !preparation->Alive) return;
        if (!BranchView->StoppingInitial) throw;
        BranchView->Status.LastError = ERROR_NOT_ENOUGH_MEMORY;
        ++BranchView->Status.Errors;
        CancelBranchViewScan();
        SalMessageBox(HWindow, GetErrorText(ERROR_NOT_ENOUGH_MEMORY), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
        return;
    }
    const size_t previewCount = Salamander::BranchView::InitialPreviewCount(
        BranchView->IncomingEntries.size(), BranchView->Entries.size());
    const bool publish = BranchView->Dirty && !preparation->Queue.HasPending() && !stoppedBatchPending &&
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
            BranchView->HasCompletedSnapshot = true;
            BranchView->IncomingEntries.clear();
            BranchView->CompletionPending = false;
            BranchView->StoppingInitial = false;
            BranchView->StoppedBatch.clear();
            BranchView->StoppedBatchNext = 0;
            // Pending notifications may start a new scan only after this
            // snapshot has been published (or confirmed unchanged).
            BranchView->Refresh.Finished(GetTickCount64());
            statusChanged = true;
        }
    }
    bool mergeExplained = false;
    if (finished && (BranchView->RetryingProtected || BranchView->UseProtectedListing ||
                     BranchView->Status.IdentityUnavailable))
    {
        std::wstring failurePath;
        DWORD failure = ERROR_SUCCESS;
        BranchView->Worker.DrainFailure(failurePath, failure);
        if (failure)
        {
            std::string text = std::string(LoadStr(BranchView->Status.IdentityUnavailable ?
                IDS_BRANCH_IDENTITY_UNAVAILABLE : failure == ERROR_BUFFER_OVERFLOW ?
                IDS_BRANCH_LIST_LIMIT : IDS_FIND_INCOMPLETE_PROTECTED_FAILURE)) + "\n\n" +
                SalWideToMultiBytePath(failurePath.c_str(), CP_UTF8) + "\n\n" + GetErrorText(failure);
            if (failure != ERROR_BUFFER_OVERFLOW && BranchView->Status.LimitExceeded)
                text += "\n\n" + std::string(LoadStr(IDS_BRANCH_LIST_LIMIT));
            if (BranchView->RetryingProtected && BranchView->Status.UncertainMerge)
            {
                text += "\n\n" + std::string(LoadStr(IDS_BRANCH_MERGE_UNCERTAIN_DETAIL));
                mergeExplained = true;
            }
            SalMessageBox(HWindow, text.c_str(), LoadStr(IDS_BRANCH_VIEW), MB_OK | MB_ICONEXCLAMATION);
            if (!lifetime->Alive || !preparation->Alive) return;
        }
    }
    if (finished && BranchView->RetryingProtected && BranchView->Status.UncertainMerge && !mergeExplained)
    {
        SalMessageBox(HWindow, LoadStr(IDS_BRANCH_MERGE_UNCERTAIN_DETAIL), LoadStr(IDS_BRANCH_VIEW), MB_OK | MB_ICONINFORMATION);
        if (!lifetime->Alive || !preparation->Alive) return;
    }
    if (finished && !BranchView->UseProtectedListing && !BranchView->RetryingProtected &&
        !BranchView->Status.Cancelled)
    {
        try { RetryBranchDenied(this); }
        catch (const std::bad_alloc&)
        {
            if (!lifetime->Alive || !preparation->Alive) return;
            ++BranchView->Status.Errors;
            BranchView->Status.LastError = ERROR_NOT_ENOUGH_MEMORY;
            SalMessageBox(HWindow, GetErrorText(ERROR_NOT_ENOUGH_MEMORY), LoadStr(IDS_BRANCH_VIEW), MB_OK | MB_ICONEXCLAMATION);
        }
        if (!lifetime->Alive || !preparation->Alive) return;
        statusChanged = true;
    }
    if (BranchView->Refresh.Due(GetTickCount64())) StartBranchViewScan(this);
    if (statusChanged) { DirectoryLineSetText(); IdleRefreshStates = TRUE; }
    if (!BranchView->Status.Running && !BranchView->Preparing &&
        !BranchView->Refresh.HasPending() && !BranchView->Dirty)
        KillTimer(HWindow, IDT_BRANCHVIEW_POLL);
    else
        SetTimer(HWindow, IDT_BRANCHVIEW_POLL,
            preparation->Queue.HasPending() || BranchView->StoppingInitial ||
                (BranchView->UseProtectedListing && BranchView->Status.Running) ? 15 : 200,
            NULL);
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
void CFilesWindow::FocusBranchShortcutTarget(CFilesWindow* panel, const CFileData& file)
{
    if (panel == NULL || !Is(ptDisk) || !IsBranchView() ||
        GetBranchViewFileByCacheKey(GetItemCacheKeyPtr(file)) != &file)
        return;
    const std::wstring source = GetItemFullPathW(file);
    if (source.empty()) return;
    const DWORD sourceAttributes = file.Attr;
    const CFileData* const original = &file;
    const std::string key = GetItemCacheKeyPtr(file);
    const std::wstring root = GetPathW();
    const ULONGLONG generation = GetBranchViewGeneration();
    const auto sourceTab = PanelTabId;
    const auto targetTab = panel->PanelTabId;
    const CPanelType targetType = panel->GetPanelType();
    const std::wstring targetRoot = panel->GetPathW();
    const std::string targetLegacyPath = panel->GetPath();
    const BOOL targetBranch = panel->IsBranchView();
    const ULONGLONG targetGeneration = panel->GetBranchViewGeneration();
    // Load/Resolve may dispatch messages. Only owned strings survive those calls;
    // never navigate from a replacement listing or overwrite a changed target tab.
    const auto stillCurrent = [&]() {
        return Is(ptDisk) && IsBranchView() && PanelTabId == sourceTab &&
            root == GetPathW() && generation == GetBranchViewGeneration() &&
            GetBranchViewFileByCacheKey(key.c_str()) == original &&
            panel->PanelTabId == targetTab && panel->GetPanelType() == targetType &&
            targetRoot == panel->GetPathW() && targetLegacyPath == panel->GetPath() &&
            panel->IsBranchView() == targetBranch &&
            panel->GetBranchViewGeneration() == targetGeneration;
    };
    std::vector<wchar_t> target(SAL_MAX_PATH, 0);
    bool resolved = false;
    bool cancelled = false;
    if ((sourceAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    {
        const std::wstring extended = Salamander::BranchView::ExtendedPath(source);
        HANDLE handle = CreateFileW(extended.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (handle != INVALID_HANDLE_VALUE)
        {
            DWORD length = GetFinalPathNameByHandleW(handle, target.data(), (DWORD)target.size(), FILE_NAME_NORMALIZED);
            resolved = length != 0 && length < target.size();
            CloseHandle(handle);
        }
    }
    else
    {
        IShellLinkW* link = NULL;
        if (CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&link) == S_OK)
        {
            IPersistFile* persist = NULL;
            if (link->QueryInterface(IID_IPersistFile, (void**)&persist) == S_OK)
            {
                const std::wstring extended = Salamander::BranchView::ExtendedPath(source);
                HRESULT result = persist->Load(extended.c_str(), STGM_READ);
                if (result == S_OK && stillCurrent())
                {
                    result = link->Resolve(HWindow, SLR_ANY_MATCH | SLR_UPDATE);
                    cancelled = result == S_FALSE;
                    if (result == S_OK && stillCurrent())
                    {
                        PIDLIST_ABSOLUTE pidl = NULL;
                        if (link->GetIDList(&pidl) == S_OK && pidl != NULL)
                        {
                            // GetPath may return a short alias for a long target.
                            // Ask the resolved filesystem PIDL for the complete wide path.
                            resolved = SHGetPathFromIDListEx(pidl, target.data(), (DWORD)target.size(), GPFIDL_DEFAULT) != FALSE;
                        }
                        else
                            resolved = link->GetPath(target.data(), (int)target.size(), NULL, SLGP_UNCPRIORITY) == S_OK;
                        if (pidl != NULL) CoTaskMemFree(pidl);
                        resolved = resolved && target[0] != 0 &&
                                   std::find(target.begin(), target.end(), L'\0') != target.end();
                    }
                }
                persist->Release();
            }
            link->Release();
        }
    }
    if (cancelled || !stillCurrent()) return;
    if (resolved)
    {
        std::wstring destination = SalPathRemoveExtendedPrefixW(target.data());
        const bool drivePath = destination.size() >= 3 && destination[1] == L':' && destination[2] == L'\\' &&
            ((destination[0] >= L'A' && destination[0] <= L'Z') || (destination[0] >= L'a' && destination[0] <= L'z'));
        const bool uncPath = destination.size() > 2 && destination.compare(0, 2, L"\\\\") == 0 && destination[2] != L'.';
        if (drivePath || uncPath)
        {
            const std::wstring extended = Salamander::BranchView::ExtendedPath(destination);
            const DWORD length = GetLongPathNameW(extended.c_str(), target.data(), (DWORD)target.size());
            if (length != 0 && length < target.size())
                destination = SalPathRemoveExtendedPrefixW(target.data());
        }
        const DWORD attributes = drivePath || uncPath ?
            GetFileAttributesW(Salamander::BranchView::ExtendedPath(destination).c_str()) : INVALID_FILE_ATTRIBUTES;
        if (attributes != INVALID_FILE_ATTRIBUTES)
        {
            if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                panel->ChangePathToDiskW(panel->HWindow, destination.c_str());
            else
            {
                const size_t slash = destination.find_last_of(L'\\');
                if (slash != std::wstring::npos)
                {
                    std::wstring directory = destination.substr(0, slash);
                    if (slash == 2) directory += L'\\'; // retain a drive root, never use drive-relative C:
                    const std::wstring name = destination.substr(slash + 1);
                    const std::string focus = SalWideToMultiBytePath(name.c_str(), CP_UTF8);
                    if (panel->ChangePathToDiskW(panel->HWindow, directory.c_str(), -1, focus.c_str()) &&
                        panel->Is(ptDisk) && !panel->IsBranchView() &&
                        CompareStringOrdinal(panel->GetPathW(), -1, directory.c_str(), -1, TRUE) == CSTR_EQUAL)
                    {
                        for (int i = 0; i < panel->Files->Count; ++i)
                        {
                            const CFileData& current = panel->Files->At(i);
                            const std::wstring currentName = current.UseWideName() ? current.NameW :
                                SalMultiByteToWidePath(current.Name, IsValidPathUtf8Text(current.Name) ? CP_UTF8 : CP_ACP);
                            if (currentName == name)
                            {
                                panel->SetCaretIndex(panel->Dirs->Count + i, TRUE);
                                break;
                            }
                        }
                    }
                }
            }
            return;
        }
    }
    SalMessageBox(HWindow, LoadStr(IDS_SHORTCUT_WRONG_PATH), LoadStr(IDS_ERRORTITLE), MB_OK | MB_ICONEXCLAMATION);
}

#endif
