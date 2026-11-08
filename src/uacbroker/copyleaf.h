/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Publish verified ZIP output from a staged file. | Commit into a vacant name only and discard partial writes. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Return a distinct failure for EFS sources and staging. | Refuse a copy that could silently decrypt an encrypted file. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Preserve a verified protected DACL and owner and group at file creation. | Require exact readback and avoid elevating security privileges. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Preserve LZNT1 compression on a verified staged file. | Validate compression and basic attributes before publishing the copy. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Preserve sparse allocation in the default one-file F5 pilot. | Copy allocated ranges into a sparse stage before publishing. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Preserve bounded NTFS data streams on a verified staged file. | Publish the file and its named streams together. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Allow explicitly omitting named data streams from one file copy. | Retain extended-attribute and file identity checks. | | GPT-6.1 Sol (Codex) |
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
#include <winioctl.h>
#include <algorithm>
#include <memory>
#include <vector>

extern "C" NTSYSAPI NTSTATUS NTAPI NtQueryInformationFile(
    HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

namespace UacCopyLeaf
{
enum class StreamPolicy { RequireDefault, IgnoreNamed, PreserveNamed };
using StageVerifier = bool (*)(HANDLE, bool);

namespace Detail
{
inline DWORD CheckPlainStreamsAndEa(HANDLE file, bool directory, StreamPolicy policy)
{
    if (policy == StreamPolicy::RequireDefault)
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
    }

    IO_STATUS_BLOCK io{};
    ULONG eaSize = 0;
    constexpr auto eaClass = static_cast<FILE_INFORMATION_CLASS>(7); // FileEaInformation
    const NTSTATUS status = NtQueryInformationFile(file, &io, &eaSize, sizeof(eaSize), eaClass);
    if (status < 0)
        return RtlNtStatusToDosError(status);
    return eaSize ? ERROR_NOT_SUPPORTED : ERROR_SUCCESS;
}

inline DWORD ReadPlainMetadata(HANDLE file, FILE_BASIC_INFO& basic, bool directory = false,
                              StreamPolicy policy = StreamPolicy::RequireDefault,
                              DWORD allowedAttributes = UacF7::plainCopyAttributes)
{
    if (!GetFileInformationByHandleEx(file, FileBasicInfo, &basic, sizeof(basic)))
        return GetLastError();
    if (basic.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
        return ERROR_FILE_ENCRYPTED;
    const bool validFile = basic.FileAttributes && !(basic.FileAttributes & ~allowedAttributes);
    if (directory ? basic.FileAttributes != FILE_ATTRIBUTE_DIRECTORY : !validFile)
        return ERROR_NOT_SUPPORTED;
    return CheckPlainStreamsAndEa(file, directory, policy);
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
    return (attributes & ~(FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_SPARSE_FILE)) | FILE_ATTRIBUTE_ARCHIVE;
}

inline DWORD StageFile(HANDLE parent, HANDLE& file, std::wstring& name, bool readSecurity,
                       PSECURITY_DESCRIPTOR security = nullptr, bool readData = false)
{
    GUID id{};
    WCHAR guid[39]{};
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, ARRAYSIZE(guid)))
        return ERROR_GEN_FAILURE;
    name = std::wstring(L".samandarin-copy-") + guid + L".tmp";
    const ACCESS_MASK access = FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | DELETE |
                               (readSecurity || security ? READ_CONTROL : 0) | (readData ? FILE_READ_DATA : 0);
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

inline DWORD ReadCompression(HANDLE file, USHORT& format)
{
    DWORD bytes = 0;
    if (!DeviceIoControl(file, FSCTL_GET_COMPRESSION, nullptr, 0, &format, sizeof(format), &bytes, nullptr))
        return GetLastError();
    return bytes == sizeof(format) ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}

inline DWORD CopySparseData(HANDLE source, HANDLE target, LONGLONG size,
                            HANDLE cancel, HANDLE connection, ULONGLONG deadline)
{
    DWORD bytes = 0;
    if (!DeviceIoControl(target, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytes, nullptr))
        return GetLastError();
    LARGE_INTEGER eof{};
    eof.QuadPart = size;
    if (!SetFilePointerEx(target, eof, nullptr, FILE_BEGIN) || !SetEndOfFile(target))
        return GetLastError();
    if (!size)
        return ERROR_SUCCESS;

    FILE_ALLOCATED_RANGE_BUFFER request{};
    request.Length = eof;
    std::vector<FILE_ALLOCATED_RANGE_BUFFER> ranges(4096);
    const auto capacity = static_cast<DWORD>(ranges.size() * sizeof(ranges[0]));
    if (!DeviceIoControl(source, FSCTL_QUERY_ALLOCATED_RANGES, &request, sizeof(request),
                         ranges.data(), capacity, &bytes, nullptr))
        return GetLastError();
    if (bytes > capacity || bytes % sizeof(ranges[0]))
        return ERROR_INVALID_DATA;

    LONGLONG previousEnd = 0;
    for (size_t index = 0; index < bytes / sizeof(ranges[0]); ++index)
    {
        const auto& range = ranges[index];
        const LONGLONG start = range.FileOffset.QuadPart;
        if (start < previousEnd || start >= size || range.Length.QuadPart <= 0)
            return ERROR_INVALID_DATA;
        const LONGLONG length = (std::min)(range.Length.QuadPart, size - start);
        if (!SetFilePointerEx(source, range.FileOffset, nullptr, FILE_BEGIN) ||
            !SetFilePointerEx(target, range.FileOffset, nullptr, FILE_BEGIN))
            return GetLastError();
        if (const DWORD error = CopyData(source, target, length, cancel, connection, deadline, false))
            return error;
        previousEnd = start + length;
    }
    return ERROR_SUCCESS;
}

struct DataStream
{
    std::wstring suffix;
    ULONGLONG size;

    bool operator==(const DataStream& other) const { return suffix == other.suffix && size == other.size; }
};

inline DWORD ReadDataStreams(HANDLE file, std::vector<DataStream>& streams)
{
    constexpr size_t maxStreams = 64;
    constexpr size_t headerSize = offsetof(FILE_STREAM_INFO, StreamName);
    constexpr std::wstring_view type = L":$DATA";
    std::vector<ULONGLONG> buffer(64 * 1024 / sizeof(ULONGLONG));
    const auto bytes = buffer.size() * sizeof(ULONGLONG);
    if (!GetFileInformationByHandleEx(file, FileStreamInfo, buffer.data(), static_cast<DWORD>(bytes)))
        return GetLastError();

    streams.clear();
    ULONGLONG total = 0;
    bool mainFound = false;
    for (size_t offset = 0; ; )
    {
        if (bytes - offset < headerSize || streams.size() == maxStreams)
            return ERROR_NOT_SUPPORTED;
        const auto& entry = *reinterpret_cast<const FILE_STREAM_INFO*>(reinterpret_cast<const BYTE*>(buffer.data()) + offset);
        const size_t next = entry.NextEntryOffset;
        const size_t available = next ? next : bytes - offset;
        if ((next && (next % alignof(ULONGLONG) || next > bytes - offset)) || available < headerSize ||
            entry.StreamNameLength % sizeof(wchar_t) || entry.StreamNameLength > available - headerSize ||
            entry.StreamSize.QuadPart < 0)
            return ERROR_INVALID_DATA;

        const auto size = static_cast<ULONGLONG>(entry.StreamSize.QuadPart);
        if (size > UacF7::maxPilotCopyBytes - total)
            return ERROR_FILE_TOO_LARGE;
        total += size;
        const std::wstring_view suffix(entry.StreamName, entry.StreamNameLength / sizeof(wchar_t));
        if (suffix == L"::$DATA")
        {
            if (mainFound)
                return ERROR_INVALID_DATA;
            mainFound = true;
        }
        else if (suffix.size() <= type.size() + 1 || suffix.front() != L':' ||
                 suffix.substr(suffix.size() - type.size()) != type ||
                 !UacDirectoryLeaf::IsComponent(suffix.substr(1, suffix.size() - 1 - type.size())))
            return ERROR_NOT_SUPPORTED;
        streams.push_back({std::wstring(suffix), size});
        if (!next)
            break;
        offset += next;
    }
    std::sort(streams.begin(), streams.end(), [](const DataStream& a, const DataStream& b) { return a.suffix < b.suffix; });
    return mainFound ? ERROR_SUCCESS : ERROR_INVALID_DATA;
}

using StreamHandle = std::unique_ptr<void, decltype(&CloseHandle)>;

inline DWORD OpenDataStream(HANDLE parent, std::wstring_view leaf, const DataStream& stream, const FILE_ID_INFO& identity,
                            bool create, StreamHandle& opened)
{
    // The suffix came from FileStreamInfo and contains one validated component.
    // The base name is pinned by the caller. Open relative to its verified parent.
    const auto name = std::wstring(leaf) + stream.suffix;
    HANDLE raw = nullptr;
    DWORD error = UacRenameLeaf::Detail::OpenFile(parent, name, raw, create ? FILE_WRITE_DATA : FILE_READ_DATA,
                                                 create ? FILE_SHARE_DELETE : FILE_SHARE_READ,
                                                 create ? FILE_CREATE : FILE_OPEN, 0);
    if (error)
        return error;
    opened.reset(raw);
    FILE_ID_INFO actual{};
    FILE_BASIC_INFO basic{};
    LARGE_INTEGER size{};
    if (!GetFileInformationByHandleEx(raw, FileIdInfo, &actual, sizeof(actual)) ||
        !GetFileInformationByHandleEx(raw, FileBasicInfo, &basic, sizeof(basic)) || !GetFileSizeEx(raw, &size))
        return GetLastError();
    if (!UacRenameLeaf::Detail::SameSource(actual, identity) ||
        size.QuadPart < 0 || static_cast<ULONGLONG>(size.QuadPart) != (create ? 0 : stream.size))
        return ERROR_FILE_INVALID;
    return UacF7::IsPlainCopyFile(basic.FileAttributes) ? ERROR_SUCCESS : ERROR_NOT_SUPPORTED;
}

class CopySecuritySnapshot
{
public:
    DWORD Read(HANDLE file)
    {
        // The pilot copies discretionary access only. Refuse explicit security
        // policy that could constrain access beyond the DACL.
        PSECURITY_DESCRIPTOR rawPolicy = nullptr;
        PACL policy = nullptr;
        constexpr SECURITY_INFORMATION policyInfo = LABEL_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION |
                                                    SCOPE_SECURITY_INFORMATION;
        const DWORD policyError = GetSecurityInfo(file, SE_FILE_OBJECT, policyInfo,
                                                  nullptr, nullptr, nullptr, &policy, &rawPolicy);
        const std::unique_ptr<void, decltype(&LocalFree)> policyDescriptor(rawPolicy, LocalFree);
        if (policyError)
            return policyError;
        if (policy && policy->AceCount)
            return ERROR_NOT_SUPPORTED;

        PSECURITY_DESCRIPTOR raw = nullptr;
        constexpr SECURITY_INFORMATION info = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
        const DWORD error = GetSecurityInfo(file, SE_FILE_OBJECT, info, &owner, &group, &dacl, nullptr, &raw);
        descriptor.reset(raw);
        if (error)
            return error;
        SECURITY_DESCRIPTOR_CONTROL control{};
        DWORD revision = 0;
        if (!GetSecurityDescriptorControl(raw, &control, &revision))
            return GetLastError();
        if (!owner || !group || !dacl || !(control & SE_DACL_PROTECTED) || !IsValidAcl(dacl))
            return ERROR_NOT_SUPPORTED;
        if (!GetAclInformation(dacl, &size, sizeof(size), AclSizeInformation))
            return GetLastError();
        if (size.AclBytesInUse < sizeof(ACL) || size.AclBytesInUse > dacl->AclSize)
            return ERROR_INVALID_ACL;
        for (DWORD index = 0; index < size.AceCount; ++index)
        {
            void* ace = nullptr;
            if (!GetAce(dacl, index, &ace))
                return GetLastError();
            if (static_cast<const ACE_HEADER*>(ace)->AceFlags & INHERITED_ACE)
                return ERROR_NOT_SUPPORTED;
        }
        return ERROR_SUCCESS;
    }

    PSECURITY_DESCRIPTOR Descriptor() const { return descriptor.get(); }

    DWORD Verify(HANDLE file) const
    {
        CopySecuritySnapshot current;
        if (const DWORD error = current.Read(file))
            return error;
        if (!EqualSid(owner, current.owner) || !EqualSid(group, current.group) ||
            dacl->AclRevision != current.dacl->AclRevision || size.AceCount != current.size.AceCount ||
            size.AclBytesInUse != current.size.AclBytesInUse ||
            std::memcmp(dacl + 1, current.dacl + 1, size.AclBytesInUse - sizeof(ACL)) != 0)
            return ERROR_FILE_INVALID;
        return ERROR_SUCCESS;
    }

private:
    std::unique_ptr<void, decltype(&LocalFree)> descriptor{nullptr, LocalFree};
    PSID owner = nullptr;
    PSID group = nullptr;
    PACL dacl = nullptr;
    ACL_SIZE_INFORMATION size{};
};

struct CopySourceLocation
{
    HANDLE parent;
    std::wstring_view leaf;
    const FILE_ID_INFO& identity;
    StageVerifier verifyStage;
};

inline DWORD CopyOpenedFile(HANDLE source, HANDLE targetParent, std::wstring_view targetName,
                            HANDLE cancel, HANDLE connection, ULONGLONG deadline, ULONGLONG expectedSize,
                            FILE_BASIC_INFO& basic, FILE_ID_INFO* created = nullptr,
                            const UacCopyTree::Detail::StageProtection* protection = nullptr,
                            StreamPolicy policy = StreamPolicy::RequireDefault,
                            const CopySourceLocation* location = nullptr, bool preserveCompression = false,
                            const CopySecuritySnapshot* copySecurity = nullptr)
{
    if ((preserveCompression || copySecurity) && (!location || policy != StreamPolicy::RequireDefault))
        return ERROR_INVALID_PARAMETER;
    DWORD allowedAttributes = UacF7::plainCopyAttributes;
    if (preserveCompression)
        allowedAttributes = UacF7::compressedCopyAttributes;
    else if (location && policy == StreamPolicy::PreserveNamed)
        allowedAttributes |= FILE_ATTRIBUTE_SPARSE_FILE;
    DWORD error = ReadPlainMetadata(source, basic, false, policy, allowedAttributes);
    if (error)
        return error;
    USHORT format = COMPRESSION_FORMAT_NONE;
    if (preserveCompression)
    {
        if (!UacF7::IsCompressedCopyFile(basic.FileAttributes))
            return ERROR_NOT_SUPPORTED;
        if ((error = ReadCompression(source, format)) != ERROR_SUCCESS)
            return error;
        if (format != COMPRESSION_FORMAT_LZNT1)
            return ERROR_NOT_SUPPORTED;
    }
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(source, &size))
        return GetLastError();
    if (size.QuadPart < 0 || static_cast<ULONGLONG>(size.QuadPart) > UacF7::maxPilotCopyBytes)
        return ERROR_FILE_TOO_LARGE;
    if (expectedSize != MAXULONGLONG && static_cast<ULONGLONG>(size.QuadPart) != expectedSize)
        return ERROR_FILE_INVALID;

    const bool sparse = (basic.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) != 0;
    std::vector<DataStream> streams;
    std::vector<StreamHandle> inputs, outputs;
    if (policy == StreamPolicy::PreserveNamed)
    {
        if (!location)
            return ERROR_INVALID_PARAMETER;
        if ((error = ReadDataStreams(source, streams)) != ERROR_SUCCESS)
            return error;
        if (sparse && streams.size() != 1)
            return ERROR_NOT_SUPPORTED;
        for (const auto& stream : streams)
        {
            if (stream.suffix == L"::$DATA")
            {
                if (stream.size != static_cast<ULONGLONG>(size.QuadPart))
                    return ERROR_FILE_INVALID;
                continue;
            }
            StreamHandle input(nullptr, CloseHandle);
            error = OpenDataStream(location->parent, location->leaf, stream, location->identity, false, input);
            if (error)
                return error;
            inputs.push_back(std::move(input));
        }
    }

    std::wstring stageName;
    HANDLE rawTarget = nullptr;
    PSECURITY_DESCRIPTOR stageDescriptor = protection ? protection->File() : nullptr;
    if (copySecurity)
        stageDescriptor = copySecurity->Descriptor();
    if ((error = StageFile(targetParent, rawTarget, stageName, !inputs.empty(),
                           stageDescriptor, preserveCompression)) != ERROR_SUCCESS)
        return error;
    const std::unique_ptr<void, decltype(&CloseHandle)> target(rawTarget, CloseHandle);
    if (protection && (error = protection->Check(target.get(), false)) != ERROR_SUCCESS)
        return error;
    if ((copySecurity || !inputs.empty()) && (!location->verifyStage || !location->verifyStage(target.get(), false)))
        return ERROR_ACCESS_DENIED;
    if (copySecurity && (error = copySecurity->Verify(target.get())) != ERROR_SUCCESS)
        return error;
    FILE_BASIC_INFO targetBasic{};
    if (!GetFileInformationByHandleEx(target.get(), FileBasicInfo, &targetBasic, sizeof(targetBasic)))
        return GetLastError();
    if (targetBasic.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
        return ERROR_FILE_ENCRYPTED;
    if (!UacF7::IsPlainCopyFile(targetBasic.FileAttributes) &&
        !(preserveCompression && UacF7::IsCompressedCopyFile(targetBasic.FileAttributes)))
        return ERROR_NOT_SUPPORTED;
    if (preserveCompression)
    {
        DWORD bytes = 0;
        if (!DeviceIoControl(target.get(), FSCTL_SET_COMPRESSION, &format, sizeof(format), nullptr, 0, &bytes, nullptr))
            return GetLastError();
    }
    error = sparse ? CopySparseData(source, target.get(), size.QuadPart, cancel, connection, deadline)
                   : CopyData(source, target.get(), size.QuadPart, cancel, connection, deadline, protection != nullptr);
    if (error)
        return error;

    if (!inputs.empty())
    {
        FILE_ID_INFO targetId{};
        if (!GetFileInformationByHandleEx(target.get(), FileIdInfo, &targetId, sizeof(targetId)))
            return GetLastError();
        for (const auto& stream : streams)
        {
            if (stream.suffix == L"::$DATA")
                continue;
            StreamHandle output(nullptr, CloseHandle);
            error = OpenDataStream(targetParent, stageName, stream, targetId, true, output);
            if (error)
                return error;
            error = CopyData(inputs[outputs.size()].get(), output.get(), stream.size, cancel, connection, deadline, false);
            if (error)
                return error;
            if (!FlushFileBuffers(output.get()))
                return GetLastError();
            outputs.push_back(std::move(output));
        }
    }

    FILE_BASIC_INFO current{};
    if ((error = ReadPlainMetadata(source, current, false, policy, allowedAttributes)) != ERROR_SUCCESS)
        return error;
    if (!SameBasicMetadata(basic, current))
        return ERROR_FILE_INVALID;

    // Follow the ordinary copy behavior when the exact-attribute option is off.
    targetBasic = basic;
    targetBasic.ChangeTime.QuadPart = 0;
    targetBasic.FileAttributes = preserveCompression ? basic.FileAttributes & ~FILE_ATTRIBUTE_COMPRESSED
                                                     : CopiedAttributes(basic.FileAttributes);
    if (!targetBasic.FileAttributes)
        targetBasic.FileAttributes = FILE_ATTRIBUTE_NORMAL;
    if (!SetFileInformationByHandle(target.get(), FileBasicInfo, &targetBasic, sizeof(targetBasic)) || !FlushFileBuffers(target.get()))
        return GetLastError();
    if (sparse)
    {
        FILE_BASIC_INFO actual{};
        if (!GetFileInformationByHandleEx(target.get(), FileBasicInfo, &actual, sizeof(actual)))
            return GetLastError();
        if (!(actual.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE))
            return ERROR_FILE_INVALID;
    }
    if (preserveCompression)
    {
        USHORT actualFormat = COMPRESSION_FORMAT_NONE;
        if ((error = ReadCompression(target.get(), actualFormat)) != ERROR_SUCCESS)
            return error;
        if (actualFormat != format)
            return ERROR_FILE_INVALID;
        if ((error = CheckPlainStreamsAndEa(target.get(), false, policy)) != ERROR_SUCCESS)
            return error;
    }
    if (created && !GetFileInformationByHandleEx(target.get(), FileIdInfo, created, sizeof(*created)))
        return GetLastError();
    if (policy == StreamPolicy::PreserveNamed)
    {
        std::vector<DataStream> currentStreams;
        if ((error = ReadDataStreams(source, currentStreams)) != ERROR_SUCCESS || currentStreams != streams)
            return error ? error : ERROR_FILE_INVALID;
        if ((error = ReadDataStreams(target.get(), currentStreams)) != ERROR_SUCCESS || currentStreams != streams)
            return error ? error : ERROR_FILE_INVALID;
    }
    if (copySecurity)
    {
        if ((error = copySecurity->Verify(source)) != ERROR_SUCCESS ||
            (error = copySecurity->Verify(target.get())) != ERROR_SUCCESS ||
            (error = CheckPlainStreamsAndEa(target.get(), false, policy)) != ERROR_SUCCESS)
            return error;
    }
    if ((error = CheckCancelled(cancel, connection, deadline, protection != nullptr)) != ERROR_SUCCESS)
        return error;
    if ((error = UacRenameLeaf::Detail::NativeRename(target.get(), targetParent, targetName)) != ERROR_SUCCESS)
        return error;

    if (preserveCompression)
    {
        // NTFS can set Archive during rename. The exclusive handle still owns the
        // delete-on-close file, so a failure here discards the incomplete result.
        FILE_BASIC_INFO actual{};
        if (!SetFileInformationByHandle(target.get(), FileBasicInfo, &targetBasic, sizeof(targetBasic)) ||
            !GetFileInformationByHandleEx(target.get(), FileBasicInfo, &actual, sizeof(actual)) ||
            !FlushFileBuffers(target.get()))
            return GetLastError();
        if (actual.FileAttributes != basic.FileAttributes)
            return ERROR_FILE_INVALID;
    }

    if (copySecurity && (error = copySecurity->Verify(target.get())) != ERROR_SUCCESS)
        return error;

    // A failure or process exit before the commit discards the staged file.
    FILE_DISPOSITION_INFO_EX commit{FILE_DISPOSITION_FLAG_ON_CLOSE};
    if (!SetFileInformationByHandle(target.get(), FileDispositionInfoEx, &commit, sizeof(commit)))
        return GetLastError();
    return ERROR_SUCCESS;
}
}

// Publish already decoded, verified host bytes into a new regular file.
inline DWORD PublishZipEntry(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expected,
                             std::wstring_view name, const std::vector<BYTE>& bytes, FILETIME modified,
                             HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    if (!UacDirectoryLeaf::IsComponent(name) || bytes.size() > UacF7::maxPilotCopyBytes)
        return ERROR_INVALID_PARAMETER;
    UacDirectoryLeaf::ParentIdentity actual;
    DWORD error = UacDirectoryLeaf::ReadParentIdentity(parent, actual);
    if (error)
        return error;
    if (!UacDirectoryLeaf::SameParent(expected, actual))
        return ERROR_FILE_INVALID;
    if ((error = Detail::CheckCancelled(nullptr, connection, deadline)) != ERROR_SUCCESS)
        return error;

    HANDLE raw = nullptr;
    std::wstring stageName;
    if ((error = Detail::StageFile(parent, raw, stageName, false)) != ERROR_SUCCESS)
        return error;
    const std::unique_ptr<void, decltype(&CloseHandle)> stage(raw, CloseHandle);
    FILE_BASIC_INFO basic{};
    if (!GetFileInformationByHandleEx(stage.get(), FileBasicInfo, &basic, sizeof(basic)))
        return GetLastError();
    if (basic.FileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
        return ERROR_FILE_ENCRYPTED;
    if (!UacF7::IsPlainCopyFile(basic.FileAttributes))
        return ERROR_NOT_SUPPORTED;
    DWORD written = 0;
    if (!bytes.empty() && !WriteFile(stage.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr))
        return GetLastError();
    if (written != bytes.size())
        return ERROR_WRITE_FAULT;
    if (!SetFileTime(stage.get(), nullptr, nullptr, &modified) || !FlushFileBuffers(stage.get()))
        return GetLastError();
    if ((error = Detail::CheckCancelled(nullptr, connection, deadline)) != ERROR_SUCCESS)
        return error;
    if ((error = UacRenameLeaf::Detail::NativeRename(stage.get(), parent, name)) != ERROR_SUCCESS)
        return error;

    FILE_DISPOSITION_INFO_EX commit{FILE_DISPOSITION_FLAG_ON_CLOSE};
    return SetFileInformationByHandle(stage.get(), FileDispositionInfoEx, &commit, sizeof(commit))
        ? ERROR_SUCCESS : GetLastError();
}

// Copies the unnamed stream, supported basic metadata and the selected named
// streams. The new file inherits destination security unless an explicit descriptor is requested.
// The source is kept and the final name cannot replace an existing entry.
inline DWORD CopyVerifiedFile(HANDLE sourceParent, const UacDirectoryLeaf::ParentIdentity& expectedSourceParent,
                              const FILE_ID_INFO& expectedSource, std::wstring_view sourceName,
                              HANDLE targetParent, const UacDirectoryLeaf::ParentIdentity& expectedTargetParent,
                              std::wstring_view targetName, HANDLE cancel = nullptr,
                              HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG,
                              ULONGLONG expectedSize = MAXULONGLONG,
                              StreamPolicy policy = StreamPolicy::RequireDefault,
                              StageVerifier verifyStage = nullptr, bool preserveCompression = false, bool preserveSecurity = false)
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

    if (preserveSecurity && (preserveCompression || policy != StreamPolicy::RequireDefault || !verifyStage))
        return ERROR_INVALID_PARAMETER;
    HANDLE rawSource = nullptr;
    const ACCESS_MASK sourceAccess = FILE_READ_DATA | FILE_READ_EA | (preserveSecurity ? READ_CONTROL : 0);
    error = UacRenameLeaf::Detail::OpenSource(sourceParent, sourceName, rawSource, sourceAccess, FILE_SHARE_READ);
    if (error)
        return error;
    const std::unique_ptr<void, decltype(&CloseHandle)> source(rawSource, CloseHandle);
    FILE_ID_INFO actualSource{};
    if ((error = UacRenameLeaf::Detail::ReadSourceIdentity(source.get(), actualSource)) != ERROR_SUCCESS)
        return error;
    if (!UacRenameLeaf::Detail::SameSource(actualSource, expectedSource))
        return ERROR_FILE_INVALID;
    Detail::CopySecuritySnapshot security;
    if (preserveSecurity)
    {
        if (!verifyStage(source.get(), false))
            return ERROR_ACCESS_DENIED;
        if ((error = security.Read(source.get())) != ERROR_SUCCESS)
            return error;
    }
    FILE_BASIC_INFO basic{};
    const Detail::CopySourceLocation location{sourceParent, sourceName, actualSource, verifyStage};
    return Detail::CopyOpenedFile(source.get(), targetParent, targetName, cancel, connection, deadline,
                                  expectedSize, basic, nullptr, nullptr, policy, &location, preserveCompression, preserveSecurity ? &security : nullptr);
}
}
