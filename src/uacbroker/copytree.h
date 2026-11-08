/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Require protected tree staging from creation through publication. | Enforce the approved integrity profile without replacing inherited DACLs. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Stage and publish one bounded NTFS tree. | Exercise the native executor before enabling a broker request. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "copyleaf.h"
#include "listing.h"

namespace UacCopyTree
{
constexpr size_t maxTreeEntries = 128;
inline constexpr wchar_t stagePrefix[] = L".samandarin-tree-";

struct Result
{
    DWORD error = ERROR_SUCCESS;
    DWORD cleanupError = ERROR_SUCCESS;
};

namespace Detail
{
using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;

struct Entry
{
    Handle source{nullptr, CloseHandle};
    FILE_BASIC_INFO basic{};
    Handle stagedDirectory{nullptr, CloseHandle};
    FILE_ID_INFO stagedId{};
    bool created = false;
};

inline DWORD OpenDirectory(HANDLE parent, std::wstring_view name, Handle& directory,
                           ACCESS_MASK access, ULONG disposition, PSECURITY_DESCRIPTOR security = nullptr,
                           ULONG sharing = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE)
{
    HANDLE raw = nullptr;
    const DWORD error = UacDirectoryLeaf::Detail::NativeDirectory(parent, name, disposition, raw, access, security, sharing);
    if (!error)
        directory.reset(raw);
    return error;
}

inline DWORD CheckDirectory(HANDLE directory, const FILE_ID_INFO& expected)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    FILE_ID_INFO actual{};
    if (!GetFileInformationByHandleEx(directory, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileInformationByHandleEx(directory, FileIdInfo, &actual, sizeof(actual)))
        return GetLastError();
    if (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        return ERROR_REPARSE_TAG_INVALID;
    if (!(attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return ERROR_DIRECTORY;
    return UacRenameLeaf::Detail::SameSource(actual, expected) ? ERROR_SUCCESS : ERROR_FILE_INVALID;
}

inline DWORD Remove(HANDLE object)
{
    FILE_DISPOSITION_INFO remove{TRUE};
    return SetFileInformationByHandle(object, FileDispositionInfo, &remove, sizeof(remove)) ? ERROR_SUCCESS : GetLastError();
}

class Stage
{
public:
    explicit Stage(const UacListing::SubtreeManifest& manifest) : entries(manifest.entries.size()), manifest(manifest) {}
    ~Stage() { Rollback(); }

    HANDLE Parent(size_t index) const
    {
        return index == UacListing::manifestRoot ? root.get() : entries[index].stagedDirectory.get();
    }

    DWORD Create(HANDLE target, const StageProtection& protection)
    {
        GUID id{};
        wchar_t guid[39]{};
        if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, ARRAYSIZE(guid)))
            return ERROR_GEN_FAILURE;
        return OpenDirectory(target, std::wstring(stagePrefix) + guid, root,
                             DELETE | FILE_WRITE_ATTRIBUTES | FILE_READ_EA | FILE_LIST_DIRECTORY | READ_CONTROL,
                             FILE_CREATE, protection.Directory(), FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    DWORD Verify(ULONGLONG deadline) const
    {
        UacListing::SubtreeManifest copied;
        const DWORD error = UacListing::CaptureManifest(root.get(), deadline, copied);
        if (error)
            return error;
        if (copied.entries.size() != entries.size())
            return ERROR_FILE_INVALID;
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const auto& expected = manifest.entries[i];
            const auto& actual = copied.entries[i];
            const DWORD expectedAttributes = (expected.attributes & FILE_ATTRIBUTE_DIRECTORY)
                ? FILE_ATTRIBUTE_DIRECTORY
                : UacCopyLeaf::Detail::CopiedAttributes(expected.attributes);
            if (actual.parent != expected.parent || actual.name != expected.name || actual.bytes != expected.bytes ||
                actual.attributes != expectedAttributes ||
                !UacListing::Detail::SameId(actual.id, entries[i].stagedId.FileId))
                return ERROR_FILE_INVALID;
        }
        return ERROR_SUCCESS;
    }

    void CloseChildren()
    {
        for (auto& entry : entries)
            entry.stagedDirectory.reset();
    }

    DWORD Publish(HANDLE target, std::wstring_view leaf)
    {
        const DWORD error = UacRenameLeaf::Detail::NativeRename(root.get(), target, leaf);
        if (!error)
            root.reset();
        return error;
    }

    DWORD Rollback()
    {
        if (!root)
            return ERROR_SUCCESS;
        DWORD failure = ERROR_SUCCESS;
        // Reopen only directories created by this operation. Renaming the tree
        // may require closing descendant handles, even on a failed publication.
        for (size_t i = 0; i < entries.size(); ++i)
        {
            auto& state = entries[i];
            const auto& record = manifest.entries[i];
            if (!state.created || state.stagedDirectory || !(record.attributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;
            HANDLE parent = Parent(record.parent);
            if (!parent)
                continue;
            DWORD error = OpenDirectory(parent, record.name, state.stagedDirectory, DELETE, FILE_OPEN);
            if (!error)
                error = CheckDirectory(state.stagedDirectory.get(), state.stagedId);
            if (error)
            {
                state.stagedDirectory.reset();
                failure = error;
            }
        }
        for (size_t i = entries.size(); i > 0; --i)
        {
            auto& state = entries[i - 1];
            const auto& record = manifest.entries[i - 1];
            if (!state.created)
                continue;
            if (state.stagedDirectory)
            {
                const DWORD error = Remove(state.stagedDirectory.get());
                state.stagedDirectory.reset();
                if (error)
                    failure = error;
            }
            else if (!(record.attributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                HANDLE parent = Parent(record.parent);
                if (!parent)
                    continue;
                HANDLE raw = nullptr;
                DWORD error = UacRenameLeaf::Detail::OpenSource(parent, record.name, raw, DELETE, FILE_SHARE_READ);
                const Handle file(raw, CloseHandle);
                FILE_ID_INFO identity{};
                if (!error)
                    error = UacRenameLeaf::Detail::ReadSourceIdentity(file.get(), identity);
                if (!error)
                    error = UacRenameLeaf::Detail::SameSource(identity, state.stagedId) ? Remove(file.get()) : ERROR_FILE_INVALID;
                if (error)
                    failure = error;
            }
        }
        const DWORD error = Remove(root.get());
        root.reset();
        return failure ? failure : error;
    }

    Handle root{nullptr, CloseHandle};
    std::vector<Entry> entries;

private:
    const UacListing::SubtreeManifest& manifest;
};

inline DWORD RestoreDirectoryTimes(HANDLE target, FILE_BASIC_INFO basic)
{
    basic.ChangeTime.QuadPart = 0;
    basic.FileAttributes = 0;
    return SetFileInformationByHandle(target, FileBasicInfo, &basic, sizeof(basic)) ? ERROR_SUCCESS : GetLastError();
}
inline Result CopyVerifiedTree(HANDLE source, const UacDirectoryLeaf::ParentIdentity& expectedSource,
                               HANDLE target, const UacDirectoryLeaf::ParentIdentity& expectedTarget,
                               std::wstring_view leaf, StageProtection& protection, HANDLE cancel,
                               HANDLE connection, ULONGLONG deadline)
{
    using namespace UacCopyLeaf::Detail;
    using namespace UacDirectoryLeaf;
    if (!IsComponent(leaf))
        return {ERROR_INVALID_NAME};
    DWORD error = CheckCancelled(cancel, connection, deadline);
    if (error)
        return {error};
    ParentIdentity targetIdentity;
    if ((error = ReadParentIdentity(target, targetIdentity)) != ERROR_SUCCESS)
        return {error};
    if (!SameParent(targetIdentity, expectedTarget))
        return {ERROR_FILE_INVALID};
    ParentIdentity sourceIdentity;
    if ((error = ReadParentIdentity(source, sourceIdentity)) != ERROR_SUCCESS)
        return {error};
    if (!SameParent(sourceIdentity, expectedSource))
        return {ERROR_FILE_INVALID};
    if (SameParent(sourceIdentity, targetIdentity))
        return {ERROR_INVALID_PARAMETER};

    UacListing::SubtreeManifest manifest;
    if ((error = UacListing::CaptureManifest(source, deadline, manifest)) != ERROR_SUCCESS)
        return {error};
    if (!SameParent(manifest.root, expectedSource))
        return {ERROR_FILE_INVALID};
    if (manifest.entries.size() > maxTreeEntries || manifest.inventory.bytes > UacF7::maxPilotCopyBytes)
        return {ERROR_FILE_TOO_LARGE};
    if (Equal(manifest.root.volume, targetIdentity.volume))
        for (const auto& record : manifest.entries)
            if ((record.attributes & FILE_ATTRIBUTE_DIRECTORY) &&
                UacListing::Detail::SameId(record.id, targetIdentity.file.FileId))
                return {ERROR_INVALID_PARAMETER};
    if ((error = protection.Prepare(target)) != ERROR_SUCCESS)
        return {error};

    FILE_BASIC_INFO rootBasic{};
    if ((error = ReadPlainMetadata(source, rootBasic, true)) != ERROR_SUCCESS)
        return {error};
    if ((error = CheckCancelled(cancel, connection, deadline)) != ERROR_SUCCESS)
        return {error};

    Detail::Stage stage(manifest);
    if ((error = stage.Create(target, protection)) != ERROR_SUCCESS)
        return {error};

    const auto finish = [&]() -> DWORD {
        FILE_BASIC_INFO targetBasic{};
        if ((error = protection.Check(stage.root.get(), true)) != ERROR_SUCCESS)
            return error;
        if ((error = ReadPlainMetadata(stage.root.get(), targetBasic, true)) != ERROR_SUCCESS)
            return error;
        for (size_t i = 0; i < manifest.entries.size(); ++i)
        {
            if ((error = CheckCancelled(cancel, connection, deadline)) != ERROR_SUCCESS)
                return error;
            const auto& record = manifest.entries[i];
            const bool directory = (record.attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            const HANDLE sourceParent = record.parent == UacListing::manifestRoot ? source : stage.entries[record.parent].source.get();
            const HANDLE targetParent = stage.Parent(record.parent);
            const FILE_ID_INFO expected{manifest.root.file.VolumeSerialNumber, record.id};
            auto& state = stage.entries[i];
            if (directory)
            {
                if ((error = Detail::OpenDirectory(sourceParent, record.name, state.source, FILE_READ_EA, FILE_OPEN)) ||
                    (error = Detail::CheckDirectory(state.source.get(), expected)) ||
                    (error = ReadPlainMetadata(state.source.get(), state.basic, true)))
                    return error;
                error = Detail::OpenDirectory(targetParent, record.name, state.stagedDirectory,
                                              DELETE | FILE_WRITE_ATTRIBUTES | FILE_READ_EA | READ_CONTROL,
                                              FILE_CREATE, protection.Directory());
                if (error)
                    return error;
                state.created = true;
                if (!GetFileInformationByHandleEx(state.stagedDirectory.get(), FileIdInfo, &state.stagedId, sizeof(state.stagedId)))
                    return GetLastError();
                if ((error = protection.Check(state.stagedDirectory.get(), true)) != ERROR_SUCCESS)
                    return error;
                if ((error = ReadPlainMetadata(state.stagedDirectory.get(), targetBasic, true)) != ERROR_SUCCESS)
                    return error;
            }
            else
            {
                HANDLE raw = nullptr;
                error = UacRenameLeaf::Detail::OpenSource(sourceParent, record.name, raw, FILE_READ_DATA | FILE_READ_EA, FILE_SHARE_READ);
                if (error)
                    return error;
                state.source.reset(raw);
                FILE_ID_INFO actual{};
                if ((error = UacRenameLeaf::Detail::ReadSourceIdentity(raw, actual)) != ERROR_SUCCESS)
                    return error;
                if (!UacRenameLeaf::Detail::SameSource(actual, expected))
                    return ERROR_FILE_INVALID;
                error = CopyOpenedFile(raw, targetParent, record.name, cancel, connection, deadline,
                                       record.bytes, state.basic, &state.stagedId, &protection);
                if (error)
                    return error;
                state.created = true;
            }
        }

        if ((error = UacListing::VerifyManifest(source, deadline, manifest)) != ERROR_SUCCESS)
            return error;
        for (size_t i = manifest.entries.size(); i > 0; --i)
        {
            if ((error = CheckCancelled(cancel, connection, deadline)) != ERROR_SUCCESS)
                return error;
            auto& state = stage.entries[i - 1];
            const bool directory = state.stagedDirectory != nullptr;
            FILE_BASIC_INFO current{};
            if ((error = ReadPlainMetadata(state.source.get(), current, directory)) != ERROR_SUCCESS)
                return error;
            if (!SameBasicMetadata(state.basic, current))
                return ERROR_FILE_INVALID;
            if (directory && (error = Detail::RestoreDirectoryTimes(state.stagedDirectory.get(), state.basic)) != ERROR_SUCCESS)
                return error;
        }
        FILE_BASIC_INFO currentRoot{};
        if ((error = ReadPlainMetadata(source, currentRoot, true)) != ERROR_SUCCESS)
            return error;
        if (!SameBasicMetadata(rootBasic, currentRoot))
            return ERROR_FILE_INVALID;
        if ((error = Detail::RestoreDirectoryTimes(stage.root.get(), rootBasic)) != ERROR_SUCCESS)
            return error;
        if ((error = stage.Verify(deadline)) != ERROR_SUCCESS)
            return error;
        if ((error = ReadParentIdentity(target, targetIdentity)) != ERROR_SUCCESS)
            return error;
        if (!SameParent(targetIdentity, expectedTarget))
            return ERROR_FILE_INVALID;
        if ((error = protection.Check(stage.root.get(), true)) != ERROR_SUCCESS)
            return error;
        if ((error = protection.Check(target, true)) != ERROR_SUCCESS)
            return error;
        stage.CloseChildren();
        if ((error = CheckCancelled(cancel, connection, deadline)) != ERROR_SUCCESS)
            return error;
        return stage.Publish(target, leaf);
    };
    error = finish();
    return {error, stage.Rollback()};
}
}

// The first broker pilot supports only targets that already require high
// integrity on newly created children. The destination handle needs READ_CONTROL.
inline Result CopyVerifiedTree(HANDLE source, const UacDirectoryLeaf::ParentIdentity& expectedSource,
                               HANDLE target, const UacDirectoryLeaf::ParentIdentity& expectedTarget,
                               std::wstring_view leaf, HANDLE cancel = nullptr, HANDLE connection = nullptr,
                               ULONGLONG deadline = MAXULONGLONG)
{
    Detail::StageProtection protection(true);
    return Detail::CopyVerifiedTree(source, expectedSource, target, expectedTarget, leaf, protection,
                                    cancel, connection, deadline);
}
}
