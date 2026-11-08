// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Open a scoped content file relative to a verified directory. | Share read-only validation with the viewer and ZIP. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Share verified file opening with archive reads. | Keep the snapshot limit specific to the viewer. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Read one verified file into a bounded viewer snapshot. | Release the elevated file before the viewer opens. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "f7pilot.h"
#include "renameleaf.h"
#include <memory>
#include <new>

namespace UacViewLeaf
{
inline DWORD OpenRead(HANDLE parent, const FILE_ID_INFO* expectedSource, std::wstring_view leaf,
                              ULONGLONG deadline, HANDLE& opened, LARGE_INTEGER& size, FILE_BASIC_INFO& initial)
{
    opened = nullptr;
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;

    HANDLE raw = nullptr;
    DWORD error = UacRenameLeaf::Detail::OpenSource(parent, leaf, raw, FILE_READ_DATA, FILE_SHARE_READ);
    if (error)
        return error;
    const auto close = [](void* handle) { CloseHandle(handle); };
    std::unique_ptr<void, decltype(close)> file(raw, close);
    FILE_ID_INFO actualSource{};
    if ((error = UacRenameLeaf::Detail::ReadSourceIdentity(file.get(), actualSource)) != ERROR_SUCCESS)
        return error;
    if (expectedSource && !UacRenameLeaf::Detail::SameSource(actualSource, *expectedSource))
        return ERROR_FILE_INVALID;

    if (!GetFileInformationByHandleEx(file.get(), FileBasicInfo, &initial, sizeof(initial)) ||
        !GetFileSizeEx(file.get(), &size))
        return GetLastError();
    if (initial.FileAttributes & (FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_OFFLINE |
                                  FILE_ATTRIBUTE_RECALL_ON_OPEN | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS))
        return ERROR_NOT_SUPPORTED;
    if (size.QuadPart < 0)
        return ERROR_FILE_INVALID;
    opened = file.release();
    return ERROR_SUCCESS;
}

inline DWORD OpenVerifiedRead(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                              const FILE_ID_INFO& expectedSource, std::wstring_view leaf,
                              ULONGLONG deadline, HANDLE& opened, LARGE_INTEGER& size, FILE_BASIC_INFO& initial)
{
    opened = nullptr;
    UacDirectoryLeaf::ParentIdentity actualParent;
    const DWORD error = UacDirectoryLeaf::ReadParentIdentity(parent, actualParent);
    if (error)
        return error;
    if (!UacDirectoryLeaf::SameParent(actualParent, expectedParent))
        return ERROR_FILE_INVALID;
    return OpenRead(parent, &expectedSource, leaf, deadline, opened, size, initial);
}

inline DWORD OpenScopedRead(HANDLE root, std::wstring_view relative, ULONGLONG deadline,
                            HANDLE& opened, LARGE_INTEGER& size, FILE_BASIC_INFO& initial)
{
    opened = nullptr;
    const auto split = relative.find_last_of(L'\\');
    if (split == 0)
        return ERROR_INVALID_NAME;
    if (split == std::wstring_view::npos)
    {
        UacDirectoryLeaf::ParentIdentity identity;
        const DWORD error = UacDirectoryLeaf::ReadParentIdentity(root, identity);
        return error ? error : OpenRead(root, nullptr, relative, deadline, opened, size, initial);
    }
    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity;
    const DWORD error = UacDirectoryLeaf::OpenParentBelowRoot(root, relative.substr(0, split), raw, identity);
    if (error)
        return error;
    const std::unique_ptr<void, decltype(&CloseHandle)> parent(raw, CloseHandle);
    return OpenRead(parent.get(), nullptr, relative.substr(split + 1), deadline, opened, size, initial);
}

inline DWORD ReadVerifiedSnapshot(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                  const FILE_ID_INFO& expectedSource, std::wstring_view leaf,
                                  ULONGLONG deadline, std::vector<BYTE>& snapshot)
{
    snapshot.clear();
    HANDLE opened = nullptr;
    FILE_BASIC_INFO initial{};
    LARGE_INTEGER size{};
    const DWORD error = OpenVerifiedRead(parent, expectedParent, expectedSource, leaf, deadline, opened, size, initial);
    if (error)
        return error;
    const auto close = [](void* handle) { CloseHandle(handle); };
    const std::unique_ptr<void, decltype(close)> file(opened, close);
    if (static_cast<ULONGLONG>(size.QuadPart) > UacF7::maxViewerSnapshotBytes)
        return ERROR_FILE_TOO_LARGE;

    std::vector<BYTE> complete;
    try
    {
        complete.resize(static_cast<size_t>(size.QuadPart));
    }
    catch (const std::bad_alloc&)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    DWORD count = 0;
    if (!complete.empty() && !ReadFile(file.get(), complete.data(), static_cast<DWORD>(complete.size()), &count, nullptr))
        return GetLastError();
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    FILE_BASIC_INFO after{};
    LARGE_INTEGER afterSize{};
    if (!GetFileInformationByHandleEx(file.get(), FileBasicInfo, &after, sizeof(after)) ||
        !GetFileSizeEx(file.get(), &afterSize))
        return GetLastError();
    if (count != complete.size() || afterSize.QuadPart != size.QuadPart ||
        after.LastWriteTime.QuadPart != initial.LastWriteTime.QuadPart ||
        after.ChangeTime.QuadPart != initial.ChangeTime.QuadPart ||
        after.FileAttributes != initial.FileAttributes)
        return ERROR_FILE_INVALID;
    snapshot.swap(complete);
    return ERROR_SUCCESS;
}
}
