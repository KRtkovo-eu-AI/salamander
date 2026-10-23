// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_set>

namespace Salamander { namespace BranchView {
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

bool SameSnapshot(const std::vector<Entry>& first, const std::vector<Entry>& second);

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
