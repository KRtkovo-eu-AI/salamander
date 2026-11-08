// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Carry ordinary attributes through EFS publication. | Keep source flags without changing its encryption. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Publish verified EFS payloads with destination permissions. | Keep deletion armed until the published metadata has been verified. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "efsworkspace.h"

namespace UacEfsWorkspace
{
namespace Detail
{
class FileSecurity
{
public:
    DWORD Read(HANDLE file)
    {
        PSECURITY_DESCRIPTOR raw = nullptr;
        const DWORD error = GetSecurityInfo(file, SE_FILE_OBJECT, information | LABEL_SECURITY_INFORMATION,
                                            &owner, &group, &dacl, &label, &raw);
        descriptor.reset(raw);
        if (error)
            return error;
        DWORD revision = 0;
        if (!owner || !group || !dacl || !GetSecurityDescriptorControl(raw, &control, &revision))
            return ERROR_INVALID_SECURITY_DESCR;
        return ERROR_SUCCESS;
    }

    DWORD Apply(HANDLE file) const
    {
        const auto inheritance = control & SE_DACL_PROTECTED ? PROTECTED_DACL_SECURITY_INFORMATION :
                                                                 UNPROTECTED_DACL_SECURITY_INFORMATION;
        return SetSecurityInfo(file, SE_FILE_OBJECT, information | inheritance, owner, group, dacl, nullptr);
    }

    bool Matches(const FileSecurity& other) const
    {
        return EqualSid(owner, other.owner) && EqualSid(group, other.group) &&
               EqualAcl(dacl, other.dacl) && EqualLabel(other) &&
               (control & SE_DACL_PROTECTED) == (other.control & SE_DACL_PROTECTED);
    }

    bool EqualLabel(const FileSecurity& other) const
    {
        return EqualAcl(label, other.label);
    }

private:
    static bool EqualAcl(PACL a, PACL b)
    {
        if (!a || !b)
            return a == b;
        return a->AclSize == b->AclSize && std::memcmp(a, b, a->AclSize) == 0;
    }

    static constexpr SECURITY_INFORMATION information = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                                                         DACL_SECURITY_INFORMATION;
    std::unique_ptr<void, decltype(&LocalFree)> descriptor{nullptr, LocalFree};
    PSID owner = nullptr;
    PSID group = nullptr;
    PACL dacl = nullptr;
    PACL label = nullptr;
    SECURITY_DESCRIPTOR_CONTROL control{};
};

inline DWORD OpenPublicationPayload(const Workspace& workspace, Handle& payload)
{
    HANDLE raw = nullptr;
    DWORD error = UacRenameLeaf::Detail::OpenSource(workspace.Directory(), payloadName, raw, 0, FILE_SHARE_DELETE);
    if (error)
        return error;
    const Handle verified(raw, CloseHandle);
    if ((error = CheckFile(verified.get(), workspace.PayloadId(), false)) != ERROR_SUCCESS)
        return error;

    // Reopen the verified object itself. Do not arm deletion on a name
    // that could now refer to a different object.
    raw = ReOpenFile(verified.get(), DELETE | READ_CONTROL | WRITE_DAC | WRITE_OWNER |
                                        FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                     0, FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_OPEN_REPARSE_POINT);
    if (raw == INVALID_HANDLE_VALUE)
        return GetLastError();
    payload.reset(raw);
    return ERROR_SUCCESS;
}

}

// The caller retains the exclusive workspace journal, and keeps the verified
// destination parent open for the duration of this operation. Its token must
// be able to create an ordinary child and set that child's security.
inline DWORD PublishPayload(const Workspace& workspace, HANDLE target,
                            const UacDirectoryLeaf::ParentIdentity& expectedTarget, std::wstring_view leaf,
                            ULONGLONG expectedSize, const FILE_BASIC_INFO& sourceTimes,
                            HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    using namespace Detail;
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;
    DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline);
    if (error)
        return error;

    UacDirectoryLeaf::ParentIdentity actual;
    error = UacDirectoryLeaf::ReadParentIdentity(target, actual);
    if (error)
        return error;
    if (!UacDirectoryLeaf::SameParent(actual, expectedTarget))
        return ERROR_FILE_INVALID;
    if (workspace.PayloadId().VolumeSerialNumber != actual.file.VolumeSerialNumber)
        return ERROR_NOT_SAME_DEVICE;

    FILE_ATTRIBUTE_TAG_INFO attributes{};
    if (!GetFileInformationByHandleEx(target, FileAttributeTagInfo, &attributes, sizeof(attributes)))
        return GetLastError();
    if (attributes.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
        return ERROR_NOT_SUPPORTED;

    Handle payload(nullptr, CloseHandle);
    if ((error = OpenPublicationPayload(workspace, payload)) != ERROR_SUCCESS)
        return error;
    LARGE_INTEGER size{};
    if (!GetFileInformationByHandleEx(payload.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileSizeEx(payload.get(), &size))
        return GetLastError();
    if (!(attributes.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED) || size.QuadPart < 0 ||
        static_cast<ULONGLONG>(size.QuadPart) != expectedSize)
        return ERROR_FILE_INVALID;

    FileSecurity destinationSecurity;
    {
        std::wstring templateName;
        HANDLE raw = nullptr;
        error = UacCopyLeaf::Detail::StageFile(target, raw, templateName, true);
        if (error)
            return error;
        const Handle securityTemplate(raw, CloseHandle);
        if ((error = destinationSecurity.Read(securityTemplate.get())) != ERROR_SUCCESS)
            return error;
    }

    FileSecurity stagedSecurity;
    if ((error = stagedSecurity.Read(payload.get())) != ERROR_SUCCESS)
        return error;
    if (!destinationSecurity.EqualLabel(stagedSecurity))
        return ERROR_NOT_SUPPORTED;

    if ((error = UacDirectoryLeaf::ReadParentIdentity(target, actual)) != ERROR_SUCCESS)
        return error;
    if (!UacDirectoryLeaf::SameParent(actual, expectedTarget))
        return ERROR_FILE_INVALID;

    if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) != ERROR_SUCCESS)
        return error;
    if ((error = UacRenameLeaf::Detail::NativeRename(payload.get(), target, leaf)) != ERROR_SUCCESS)
        return error;
    // SetSecurityInfo computes inheritance from the current parent. The
    // exclusive handle still owns deletion until the final check passes.
    if ((error = destinationSecurity.Apply(payload.get())) != ERROR_SUCCESS)
        return error;
    FileSecurity finalSecurity;
    if ((error = finalSecurity.Read(payload.get())) != ERROR_SUCCESS)
        return error;
    if (!destinationSecurity.Matches(finalSecurity))
        return ERROR_FILE_INVALID;

    FILE_BASIC_INFO metadata{};
    metadata.CreationTime = sourceTimes.CreationTime;
    metadata.LastWriteTime = sourceTimes.LastWriteTime;
    metadata.LastAccessTime = sourceTimes.LastAccessTime;
    if (sourceTimes.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
        metadata.FileAttributes = UacCopyLeaf::Detail::CopiedAttributes(sourceTimes.FileAttributes & ~FILE_ATTRIBUTE_ENCRYPTED);
    if (!SetFileInformationByHandle(payload.get(), FileBasicInfo, &metadata, sizeof(metadata)))
        return GetLastError();
    if ((error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline)) != ERROR_SUCCESS)
        return error;

    FILE_DISPOSITION_INFO_EX keep{FILE_DISPOSITION_FLAG_ON_CLOSE};
    if (!SetFileInformationByHandle(payload.get(), FileDispositionInfoEx, &keep, sizeof(keep)))
        return GetLastError();
    return ERROR_SUCCESS;
}
}
