/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Check tree cancellation while copying staged data. | Stop before publishing the staged tree. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Apply tree staging integrity before copying bytes. | Keep the one-file path unchanged and verify protection on staged files. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share copying from a held file handle. | Let tree staging pin source handles and journal created file identities. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share the transfer connection guard. | Check for disconnect before the native operation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Bound copy size and monitor the broker connection. | Abort staging on timeout, disconnect or unexpected data. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Copy one ordinary file between verified NTFS directory handles. | Stage and publish a complete copy without replacing a destination. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "renameleaf.h"
#include "f7pilot.h"
#include "copytreeacl.h"
#include <objbase.h>
#include <memory>
#include <vector>

extern "C" NTSYSAPI NTSTATUS NTAPI NtQueryInformationFile(
    HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

namespace UacCopyLeaf
{
namespace Detail
{
inline DWORD CheckPlainStreamsAndEa(HANDLE file, bool directory)
{
    alignas(LONGLONG) BYTE streamBuffer[1024]{};
    if (GetFileInformationByHandleEx(file, FileStreamInfo, streamBuffer, sizeof(streamBuffer)))
    {
        const auto& stream = *reinterpret_cast<const FILE_STREAM_INFO*>(streamBuffer);
        constexpr std::wstring_view defaultStream = L"::$DATA";
        if (directory || stream.NextEntryOffset || stream.StreamNameLength != defaultStream.size() * sizeof(wchar_t) ||
            std::wstring_view(stream.StreamName, defaultStream.size()) != defaultStream)
            return ERROR_NOT_SUPPORTED;
    }
    else
    {
        const DWORD error = GetLastError();
        if (!directory || error != ERROR_HANDLE_EOF)
            return error == ERROR_MORE_DATA || error == ERROR_INSUFFICIENT_BUFFER ? ERROR_NOT_SUPPORTED : error;
    }

    IO_STATUS_BLOCK io{};
    ULONG eaSize = 0;
    constexpr auto eaClass = static_cast<FILE_INFORMATION_CLASS>(7); // FileEaInformation
    const NTSTATUS status = NtQueryInformationFile(file, &io, &eaSize, sizeof(eaSize), eaClass);
    if (status < 0)
        return RtlNtStatusToDosError(status);
    return eaSize ? ERROR_NOT_SUPPORTED : ERROR_SUCCESS;
}

inline DWORD ReadPlainMetadata(HANDLE file, FILE_BASIC_INFO& basic, bool directory = false)
{
    if (!GetFileInformationByHandleEx(file, FileBasicInfo, &basic, sizeof(basic)))
        return GetLastError();
    if (directory ? basic.FileAttributes != FILE_ATTRIBUTE_DIRECTORY : !UacF7::IsPlainCopyFile(basic.FileAttributes))
        return ERROR_NOT_SUPPORTED;
    return CheckPlainStreamsAndEa(file, directory);
}

inline bool SameBasicMetadata(const FILE_BASIC_INFO& first, const FILE_BASIC_INFO& second)
{
    return first.ChangeTime.QuadPart == second.ChangeTime.QuadPart &&
           first.CreationTime.QuadPart == second.CreationTime.QuadPart &&
           first.LastWriteTime.QuadPart == second.LastWriteTime.QuadPart &&
           first.FileAttributes == second.FileAttributes;
}

inline DWORD CopiedAttributes(DWORD attributes)
{
    return (attributes & ~FILE_ATTRIBUTE_NORMAL) | FILE_ATTRIBUTE_ARCHIVE;
}

inline DWORD StageFile(HANDLE parent, HANDLE& file, PSECURITY_DESCRIPTOR security = nullptr)
{
    GUID id{};
    WCHAR guid[39]{};
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, ARRAYSIZE(guid)))
        return ERROR_GEN_FAILURE;
    const std::wstring name = std::wstring(L".samandarin-copy-") + guid + L".tmp";
    const ACCESS_MASK access = FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | DELETE | (security ? READ_CONTROL : 0);
    return UacRenameLeaf::Detail::OpenFile(parent, name, file, access, 0, FILE_CREATE, FILE_DELETE_ON_CLOSE, security);
}

inline DWORD CheckCancelled(HANDLE cancel, HANDLE connection, ULONGLONG deadline, bool treeCopy = false)
{
    if (const DWORD error = UacRenameLeaf::Detail::CheckTransferConnection(connection, deadline, treeCopy))
        return error;
    if (!cancel)
        return ERROR_SUCCESS;
    const DWORD result = WaitForSingleObject(cancel, 0);
    if (result == WAIT_TIMEOUT)
        return ERROR_SUCCESS;
    if (result == WAIT_FAILED)
        return GetLastError();
    return result == WAIT_OBJECT_0 ? ERROR_CANCELLED : ERROR_INVALID_HANDLE;
}

inline DWORD CopyData(HANDLE source, HANDLE target, ULONGLONG remaining,
                       HANDLE cancel, HANDLE connection, ULONGLONG deadline, bool treeCopy)
{
    std::vector<BYTE> buffer(64 * 1024);
    const DWORD bufferSize = static_cast<DWORD>(buffer.size());
    while (remaining)
    {
        if (const DWORD error = CheckCancelled(cancel, connection, deadline, treeCopy))
            return error;
        DWORD read = 0;
        const DWORD requested = remaining < bufferSize ? static_cast<DWORD>(remaining) : bufferSize;
        if (!ReadFile(source, buffer.data(), requested, &read, nullptr))
            return GetLastError();
        if (!read)
            return ERROR_HANDLE_EOF;
        remaining -= read;

        for (DWORD offset = 0; offset < read;)
        {
            DWORD written = 0;
            if (!WriteFile(target, buffer.data() + offset, read - offset, &written, nullptr))
                return GetLastError();
            if (!written)
                return ERROR_WRITE_FAULT;
            offset += written;
        }
    }
    return ERROR_SUCCESS;
}

inline DWORD CopyOpenedFile(HANDLE source, HANDLE targetParent, std::wstring_view targetName,
                            HANDLE cancel, HANDLE connection, ULONGLONG deadline, ULONGLONG expectedSize,
                            FILE_BASIC_INFO& basic, FILE_ID_INFO* created = nullptr,
                            const UacCopyTree::Detail::StageProtection* protection = nullptr)
{
    DWORD error = ReadPlainMetadata(source, basic);
    if (error)
        return error;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(source, &size))
        return GetLastError();
    if (size.QuadPart < 0 || static_cast<ULONGLONG>(size.QuadPart) > UacF7::maxPilotCopyBytes)
        return ERROR_FILE_TOO_LARGE;
    if (expectedSize != MAXULONGLONG && static_cast<ULONGLONG>(size.QuadPart) != expectedSize)
        return ERROR_FILE_INVALID;

    HANDLE rawTarget = nullptr;
    if ((error = StageFile(targetParent, rawTarget, protection ? protection->File() : nullptr)) != ERROR_SUCCESS)
        return error;
    const std::unique_ptr<void, decltype(&CloseHandle)> target(rawTarget, CloseHandle);
    if (protection && (error = protection->Check(target.get(), false)) != ERROR_SUCCESS)
        return error;
    FILE_BASIC_INFO targetBasic{};
    if (!GetFileInformationByHandleEx(target.get(), FileBasicInfo, &targetBasic, sizeof(targetBasic)))
        return GetLastError();
    if (!UacF7::IsPlainCopyFile(targetBasic.FileAttributes))
        return ERROR_NOT_SUPPORTED;
    if ((error = CopyData(source, target.get(), static_cast<ULONGLONG>(size.QuadPart), cancel, connection, deadline, protection != nullptr)) != ERROR_SUCCESS)
        return error;

    FILE_BASIC_INFO current{};
    if ((error = ReadPlainMetadata(source, current)) != ERROR_SUCCESS)
        return error;
    if (!SameBasicMetadata(basic, current))
        return ERROR_FILE_INVALID;

    // Follow the ordinary copy behavior when the exact-attribute option is off.
    targetBasic = basic;
    targetBasic.ChangeTime.QuadPart = 0;
    targetBasic.FileAttributes = CopiedAttributes(basic.FileAttributes);
    if (!SetFileInformationByHandle(target.get(), FileBasicInfo, &targetBasic, sizeof(targetBasic)) || !FlushFileBuffers(target.get()))
        return GetLastError();
    if (created && !GetFileInformationByHandleEx(target.get(), FileIdInfo, created, sizeof(*created)))
        return GetLastError();
    if ((error = CheckCancelled(cancel, connection, deadline, protection != nullptr)) != ERROR_SUCCESS)
        return error;
    if ((error = UacRenameLeaf::Detail::NativeRename(target.get(), targetParent, targetName)) != ERROR_SUCCESS)
        return error;

    // A failure or process exit before the commit discards the staged file.
    FILE_DISPOSITION_INFO_EX commit{FILE_DISPOSITION_FLAG_ON_CLOSE};
    if (!SetFileInformationByHandle(target.get(), FileDispositionInfoEx, &commit, sizeof(commit)))
        return GetLastError();
    return ERROR_SUCCESS;
}
}

// Copies only the unnamed data stream and supported basic metadata. The new
// file gets the archive bit and inherits security from its destination.
// The source is never
// removed and the final name cannot replace an existing entry.
inline DWORD CopyVerifiedFile(HANDLE sourceParent, const UacDirectoryLeaf::ParentIdentity& expectedSourceParent,
                              const FILE_ID_INFO& expectedSource, std::wstring_view sourceName,
                              HANDLE targetParent, const UacDirectoryLeaf::ParentIdentity& expectedTargetParent,
                              std::wstring_view targetName, HANDLE cancel = nullptr,
                              HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG,
                              ULONGLONG expectedSize = MAXULONGLONG)
{
    using namespace UacDirectoryLeaf;
    if (!IsComponent(sourceName) || !IsComponent(targetName))
        return ERROR_INVALID_NAME;

    ParentIdentity actualSourceParent;
    DWORD error = ReadParentIdentity(sourceParent, actualSourceParent);
    if (error)
        return error;
    ParentIdentity actualTargetParent;
    error = ReadParentIdentity(targetParent, actualTargetParent);
    if (error)
        return error;
    if (!SameParent(actualSourceParent, expectedSourceParent) ||
        !SameParent(actualTargetParent, expectedTargetParent) ||
        expectedSource.VolumeSerialNumber != actualSourceParent.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;

    if ((error = Detail::CheckCancelled(cancel, connection, deadline)) != ERROR_SUCCESS)
        return error;

    HANDLE rawSource = nullptr;
    error = UacRenameLeaf::Detail::OpenSource(sourceParent, sourceName, rawSource, FILE_READ_DATA | FILE_READ_EA, FILE_SHARE_READ);
    if (error)
        return error;
    const std::unique_ptr<void, decltype(&CloseHandle)> source(rawSource, CloseHandle);
    FILE_ID_INFO actualSource{};
    if ((error = UacRenameLeaf::Detail::ReadSourceIdentity(source.get(), actualSource)) != ERROR_SUCCESS)
        return error;
    if (!UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        return ERROR_FILE_INVALID;
    FILE_BASIC_INFO basic{};
    return Detail::CopyOpenedFile(source.get(), targetParent, targetName, cancel, connection, deadline, expectedSize, basic);
}
}
