// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>

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
