// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Validate and publish a bounded ZIP selection. | Bind every output component to a verified destination handle. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "copyleaf.h"
#include <set>

namespace UacZipSelection
{
inline constexpr wchar_t marker[] = L".samandarin-zip-selection";

struct Entry
{
    UacF7::ZipSelectionEntry header;
    std::wstring name;
    size_t offset;
    DWORD depth;
};

struct EntryNameLess
{
    bool operator()(const std::pair<DWORD, std::wstring>& left,
                    const std::pair<DWORD, std::wstring>& right) const
    {
        return left.first != right.first ? left.first < right.first :
            CompareStringOrdinal(left.second.c_str(), -1, right.second.c_str(), -1, TRUE) == CSTR_LESS_THAN;
    }
};

inline DWORD Decode(const std::vector<BYTE>& bytes, std::vector<Entry>& entries)
{
    if (bytes.empty() || bytes.size() > UacF7::maxZipSelectionBytes)
        return ERROR_INVALID_DATA;
    std::set<std::pair<DWORD, std::wstring>, EntryNameLess> names;
    size_t offset = 0;
    while (offset < bytes.size())
    {
        UacF7::ZipSelectionEntry header{};
        if (entries.size() == UacF7::maxZipSelectionEntries || bytes.size() - offset < sizeof(header))
            return ERROR_INVALID_DATA;
        std::memcpy(&header, bytes.data() + offset, sizeof(header));
        offset += sizeof(header);
        if (!header.nameLength || header.nameLength > UacDirectoryLeaf::maxComponent ||
            header.directory > 1 || header.size > UacF7::maxPilotCopyBytes ||
            (header.directory && header.size))
            return ERROR_INVALID_DATA;

        DWORD depth = 1;
        if (header.parent != UacF7::zipSelectionRoot)
        {
            if (header.parent >= entries.size() || !entries[header.parent].header.directory)
                return ERROR_INVALID_DATA;
            depth = entries[header.parent].depth + 1;
            if (depth > UacF7::maxZipSelectionDepth)
                return ERROR_INVALID_DATA;
        }

        const size_t nameBytes = header.nameLength * sizeof(wchar_t);
        if (nameBytes > bytes.size() - offset || header.size > bytes.size() - offset - nameBytes)
            return ERROR_INVALID_DATA;
        std::wstring name(header.nameLength, L'\0');
        std::memcpy(name.data(), bytes.data() + offset, nameBytes);
        offset += nameBytes;
        if (!UacDirectoryLeaf::IsComponent(name))
            return ERROR_INVALID_NAME;
        if (!names.emplace(header.parent, name).second)
            return ERROR_ALREADY_EXISTS;
        entries.push_back({header, std::move(name), offset, depth});
        offset += header.size;
    }
    return ERROR_SUCCESS;
}

inline DWORD Publish(HANDLE root, const UacDirectoryLeaf::ParentIdentity& expected, const std::vector<BYTE>& bytes,
                     HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    bool changed = false;
    try
    {
        std::vector<Entry> entries;
        DWORD error = Decode(bytes, entries);
        if (error)
            return error;
        UacDirectoryLeaf::ParentIdentity actual;
        if ((error = UacDirectoryLeaf::ReadParentIdentity(root, actual)) != ERROR_SUCCESS)
            return error;
        if (!UacDirectoryLeaf::SameParent(expected, actual))
            return ERROR_FILE_INVALID;

        // Refuse existing roots before publishing anything. Native create and
        // rename still refuse late collisions, including in the new subtrees.
        for (const auto& entry : entries)
        {
            if (entry.header.parent != UacF7::zipSelectionRoot)
                continue;
            if ((error = UacCopyLeaf::Detail::CheckCancelled(nullptr, connection, deadline)) != ERROR_SUCCESS)
                return error;
            HANDLE raw = nullptr;
            error = UacRenameLeaf::Detail::OpenFile(root, entry.name, raw, 0,
                                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                     FILE_OPEN, 0, nullptr, true);
            if (!error)
            {
                CloseHandle(raw);
                return ERROR_ALREADY_EXISTS;
            }
            if (error != ERROR_FILE_NOT_FOUND)
                return error;
        }

        using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;
        std::vector<Handle> parents;
        parents.reserve(entries.size());
        for (const auto& entry : entries)
        {
            HANDLE parent = entry.header.parent == UacF7::zipSelectionRoot ? root : parents[entry.header.parent].get();
            error = UacCopyLeaf::Detail::CheckCancelled(nullptr, connection, deadline);
            HANDLE directory = nullptr;
            if (!error && entry.header.directory)
                error = UacDirectoryLeaf::Detail::NativeDirectory(parent, entry.name, FILE_CREATE, directory,
                                                                   FILE_WRITE_ATTRIBUTES, nullptr,
                                                                   FILE_SHARE_READ | FILE_SHARE_WRITE);
            if (!error && !entry.header.directory)
            {
                UacDirectoryLeaf::ParentIdentity identity;
                error = UacDirectoryLeaf::ReadParentIdentity(parent, identity);
                if (!error)
                    error = UacCopyLeaf::PublishZipEntry(parent, identity, entry.name, bytes.data() + entry.offset,
                                                        entry.header.size, entry.header.modified, connection, deadline);
            }
            if (error)
                return changed ? ERROR_PARTIAL_COPY : error;
            changed = true;
            parents.emplace_back(directory, CloseHandle);
        }

        for (size_t i = entries.size(); i > 0; --i)
            if (parents[i - 1] && !SetFileTime(parents[i - 1].get(), nullptr, nullptr, &entries[i - 1].header.modified))
                return ERROR_PARTIAL_COPY;
        return ERROR_SUCCESS;
    }
    catch (const std::bad_alloc&)
    {
        return changed ? ERROR_PARTIAL_COPY : ERROR_NOT_ENOUGH_MEMORY;
    }
}
}
