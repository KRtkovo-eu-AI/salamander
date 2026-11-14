// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Hide native file-list workspace details behind a host interface. | Avoid SDK identity and header-order requirements in panel consumers. | #731 | GPT-6.1 Sol |
// | 2026-11-11 | Isolate a generated file-list source. | Bind its eventual copy and cleanup to the created NTFS objects. | #731 | GPT-6.1 Sol |
#pragma once
#include <memory>
#include <string>
#include "uacbroker/f7pilot.h"

namespace UacFileList
{
class Workspace
{
public:
    using CleanupReporter = void (*)(void*, const std::wstring&, DWORD);
    using CleanupFallback = void (*)(void*, const std::wstring&, DWORD) noexcept;
    explicit Workspace(CleanupReporter reporter = nullptr, void* context = nullptr,
                       CleanupFallback fallback = nullptr);
    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;
    ~Workspace();
    DWORD Create();
    DWORD Seal();
    HANDLE Writer() const;
    const std::wstring& Path() const;
    const std::wstring& DirectoryName() const;
    ULONGLONG Size() const;
    UacF7::MoveFileIdentity Identity() const;
    UacF7::MoveFileIdentity ParentIdentity() const;
#ifdef SAM_UAC_FILELIST_TEST
    void ReleaseReaderForTest();
#endif
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
