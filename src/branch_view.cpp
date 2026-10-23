// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef BRANCH_VIEW_TEST
#include "precomp.h"
#endif
#include "branch_view.h"
#include <new>
#include <algorithm>

namespace Salamander { namespace BranchView {
std::wstring Join(const std::wstring& directory, const std::wstring& name)
{
    return directory + (!directory.empty() && directory.back() != L'\\' ? L"\\" : L"") + name;
}
std::wstring Entry::FullPath() const { return Join(Directory, Data.cFileName); }
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
