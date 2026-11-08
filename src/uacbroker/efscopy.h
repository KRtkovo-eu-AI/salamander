// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-08 | Pass the audit verification mode to publication. | Enforce full destination checks for elevated copies. | | GPT-6.1 Sol (Codex) |
// | 2026-11-08 | Connect EFS Raw, staging and publication. | Copy the verified encrypted object with no plaintext fallback. | | GPT-6.1 Sol (Codex) |
#pragma once

#include "efspublish.h"
#include "efsraw.h"

namespace UacEfsWorkspace
{
inline DWORD CopyVerifiedFile(HANDLE sourceParent, const UacDirectoryLeaf::ParentIdentity& expectedSourceParent,
                              const FILE_ID_INFO& expectedSource, std::wstring_view sourceLeaf,
                              HANDLE target, const UacDirectoryLeaf::ParentIdentity& expectedTarget,
                              std::wstring_view targetLeaf, ULONGLONG size,
                              HANDLE privateParent, const FILE_ID_INFO& privateId, HANDLE journal,
                              PSECURITY_DESCRIPTOR security, HANDLE connection, ULONGLONG deadline, bool readAudit = false)
{
    using namespace Detail;
    if (!UacDirectoryLeaf::IsComponent(sourceLeaf) || !UacDirectoryLeaf::IsComponent(targetLeaf))
        return ERROR_INVALID_NAME;
    if (size > UacF7::maxPilotCopyBytes)
        return ERROR_FILE_TOO_LARGE;
    if (expectedSource.VolumeSerialNumber != privateId.VolumeSerialNumber ||
        expectedTarget.file.VolumeSerialNumber != privateId.VolumeSerialNumber)
        return ERROR_NOT_SAME_DEVICE;
    DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline);
    if (error)
        return error;
    UacDirectoryLeaf::ParentIdentity actual;
    if ((error = UacDirectoryLeaf::ReadParentIdentity(sourceParent, actual)) != ERROR_SUCCESS)
        return error;
    if (!UacDirectoryLeaf::SameParent(actual, expectedSourceParent))
        return ERROR_FILE_INVALID;
    if ((error = UacDirectoryLeaf::ReadParentIdentity(target, actual)) != ERROR_SUCCESS)
        return error;
    if (!UacDirectoryLeaf::SameParent(actual, expectedTarget))
        return ERROR_FILE_INVALID;

    Workspace workspace;
    auto copy = [&]() -> DWORD {
        HANDLE raw = nullptr;
        // Keep a metadata handle while opening the raw export on the private alias.
        // The raw export must still be able to open the verified private alias.
        error = UacRenameLeaf::Detail::OpenSource(sourceParent, sourceLeaf, raw, FILE_READ_EA, FILE_SHARE_READ);
        if (error)
            return error;
        const Handle source(raw, CloseHandle);
        if ((error = CheckFile(source.get(), expectedSource, false)) != ERROR_SUCCESS)
            return error;
        LARGE_INTEGER bytes{};
        if (!GetFileSizeEx(source.get(), &bytes))
            return GetLastError();
        if (bytes.QuadPart < 0 || static_cast<ULONGLONG>(bytes.QuadPart) != size)
            return ERROR_FILE_INVALID;

        if ((error = workspace.Create(privateParent, privateId, journal, source.get(), expectedSource, security)) != ERROR_SUCCESS ||
            (error = workspace.CreatePayload(security)) != ERROR_SUCCESS)
            return error;

        constexpr DWORD pathFlags = VOLUME_NAME_GUID | FILE_NAME_OPENED;
        const DWORD capacity = GetFinalPathNameByHandleW(workspace.Directory(), nullptr, 0, pathFlags);
        if (!capacity)
            return GetLastError();
        if (capacity > UacDirectoryLeaf::maxNativeName)
            return ERROR_FILENAME_EXCED_RANGE;
        std::wstring path(capacity, L'\0');
        const DWORD length = GetFinalPathNameByHandleW(workspace.Directory(), path.data(), capacity, pathFlags);
        if (!length)
            return GetLastError();
        if (length >= capacity)
            return ERROR_FILE_INVALID;
        path.resize(length);
        path += L'\\';

        // EFS Raw emits framing and encryption metadata in addition to the file.
        constexpr size_t maxRawBytes = 2 * UacF7::maxPilotCopyBytes;
        FILE_BASIC_INFO basic{};
        std::vector<BYTE> encrypted;
        {
            UacEfsRaw::Context input;
            error = input.Open((path + sourceName).c_str(), 0);
            if (error)
                return error;
            if (!GetFileInformationByHandleEx(source.get(), FileBasicInfo, &basic, sizeof(basic)) ||
                !GetFileSizeEx(source.get(), &bytes))
                return GetLastError();
            if (!UacF7::IsEncryptedCopyFile(basic.FileAttributes) ||
                bytes.QuadPart < 0 || static_cast<ULONGLONG>(bytes.QuadPart) != size)
                return ERROR_FILE_INVALID;
            if ((error = UacCopyLeaf::Detail::CheckPlainStreamsAndEa(source.get(), false,
                                                                    UacCopyLeaf::StreamPolicy::RequireDefault)) != ERROR_SUCCESS)
                return error;
            error = input.Read(encrypted, maxRawBytes, deadline, connection);
            if (error)
                return error;
        }
        {
            UacEfsRaw::Context output;
            error = output.Open((path + payloadName).c_str(), CREATE_FOR_IMPORT);
            if (!error)
                error = output.Write(encrypted, deadline, connection);
            if (error)
                return error;
        }
        return PublishPayload(workspace, target, expectedTarget, targetLeaf, size, basic, connection, deadline, readAudit);
    };
    error = copy();
    return workspace.Cleanup() ? ERROR_PARTIAL_COPY : error;
}
}
