/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-12 | Apply exact case-only lookup to verified links. | Rename reparse entries in sensitive directories without following their targets. | #731 | GPT-6 (Codex) |
| 2026-11-12 | Handle exact case-only renames in case-sensitive directories. | Preserve the anchored identity and no-replace operation without changing case-insensitive behavior. | #731 | GPT-6 (Codex) |
| 2026-11-11 | Verify case-only renames against the same anchored object. | Preserve identity and no-replace semantics for native rename operations. | #731 | GPT-6.1 Terra |
| 2026-11-11 | Move a verified read-only file without changing its attributes. | Extend same-volume F6 while retaining exact identity and no-replace publication. | | GPT-6.1 Sol |
| 2026-11-11 | Accept hidden and system attributes for an ordinary same-volume move. | Match native F6 eligibility without widening rename or reparse operations. | | GPT-6.1 Sol |
| 2026-11-10 | Allow a verified plain directory to move between same-volume parents. | Keep directory rename and link-only operations on their existing scope. | #731 | GPT-6.1 Sol |
| 2026-11-10 | Validate plain directories in the verified rename path. | Preserve no-replace and reparse safety for the selected directory. | #731 | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Expose exchange checkpoints to the native fault fixture. | Exercise real handle teardown after each rename and commit. | | GPT-6.1 Sol (Codex) |
| 2026-11-09 | Allow an overlapped relative file open for F6. | Request an exclusive oplock on the exact source name. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Cancel active protected transfers. | Keep the progress dialog responsive and wait for the actual broker outcome. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share anchored hard link creation. | Preserve the file identity without opening its original path again. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share the no-replace exchange for staged file updates. | Preserve the verified original when a replacement collides or is interrupted. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Validate a junction before renaming it in place. | Accept a drive-letter junction without resolving it or accepting volume GUID mounts. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Rename a verified directory symbolic link in place. | Open the reparse object without traversing it and enforce the approved type. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Rename a verified file symbolic link in its current parent. | Keep the link target and all other reparse types out of the operation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Recognize one cancel control for tree transfers. | Keep the other transfer commands strict. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Allow a creation descriptor for native files. | Apply mandatory integrity before the staged file is visible. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Move one verified file across directories on the same volume. | Reuse the no-replace native rename without changing ordinary F6. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share anchored leaf opens with the staging file. | Keep temporary file creation relative to the verified target. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Share the anchored source open with basic metadata changes. | Keep access and sharing specific to each operation. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Share the anchored source open and identity check with the panel host. | Capture the file identity before requesting elevation. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add an NTFS rename primitive for a single file under a verified parent. | Establish a no-overwrite command boundary before broker and panel integration. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include "directoryleaf.h"
#include <cstddef>
#include <vector>
#include <winioctl.h>

extern "C" NTSYSAPI NTSTATUS NTAPI NtSetInformationFile(
    HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);

namespace UacRenameLeaf
{
enum class MoveKind { OrdinaryFile, RenameFile, RenameSymlink, RenameDirectorySymlink, RenameJunction, RenameDirectory, OrdinaryDirectory };

inline bool IsDirectory(MoveKind kind)
{
    return kind == MoveKind::OrdinaryDirectory || kind == MoveKind::RenameDirectory || kind == MoveKind::RenameDirectorySymlink ||
           kind == MoveKind::RenameJunction;
}

inline bool ValidRenameNames(std::wstring_view from, std::wstring_view to)
{
    return UacDirectoryLeaf::IsComponent(from) && UacDirectoryLeaf::IsComponent(to) && from != to;
}

namespace Detail
{
inline DWORD NativeRename(HANDLE source, HANDLE targetParent, std::wstring_view leaf)
{
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;

    // FILE_RENAME_INFORMATION from ntifs.h. Its variable-length tail is
    // backed by storage for one NTFS component.
    struct
    {
        BOOLEAN replaceIfExists;
        HANDLE rootDirectory;
        ULONG fileNameLength;
        WCHAR fileName[UacDirectoryLeaf::maxComponent + 1];
    } info{};
    static_assert(offsetof(decltype(info), rootDirectory) == offsetof(FILE_RENAME_INFO, RootDirectory));
    static_assert(offsetof(decltype(info), fileNameLength) == offsetof(FILE_RENAME_INFO, FileNameLength));
    static_assert(offsetof(decltype(info), fileName) == offsetof(FILE_RENAME_INFO, FileName));
    info.rootDirectory = targetParent;
    info.fileNameLength = static_cast<ULONG>(leaf.size() * sizeof(wchar_t));
    std::memcpy(info.fileName, leaf.data(), info.fileNameLength);
    IO_STATUS_BLOCK io{};
    constexpr auto renameClass = static_cast<FILE_INFORMATION_CLASS>(10); // FileRenameInformation
    const NTSTATUS status = NtSetInformationFile(source, &io, &info, sizeof(info), renameClass);
    return status >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(status);
}

inline DWORD NativeHardLink(HANDLE source, HANDLE targetParent, std::wstring_view leaf)
{
    if (!UacDirectoryLeaf::IsComponent(leaf))
        return ERROR_INVALID_NAME;

    // FILE_LINK_INFORMATION with a single, parent-relative component.
    struct
    {
        BOOLEAN replaceIfExists;
        HANDLE rootDirectory;
        ULONG fileNameLength;
        WCHAR fileName[UacDirectoryLeaf::maxComponent + 1];
    } info{};
    static_assert(offsetof(decltype(info), fileName) == offsetof(FILE_RENAME_INFO, FileName));
    info.rootDirectory = targetParent;
    info.fileNameLength = static_cast<ULONG>(leaf.size() * sizeof(wchar_t));
    std::memcpy(info.fileName, leaf.data(), info.fileNameLength);
    IO_STATUS_BLOCK io{};
    constexpr auto linkClass = static_cast<FILE_INFORMATION_CLASS>(11); // FileLinkInformation
    const NTSTATUS status = NtSetInformationFile(source, &io, &info, sizeof(info), linkClass);
    return status >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(status);
}

inline DWORD OpenFile(HANDLE parent, std::wstring_view leaf, HANDLE& source, ACCESS_MASK extraAccess, ULONG sharing,
                      ULONG disposition, ULONG options, PSECURITY_DESCRIPTOR security = nullptr,
                      bool allowDirectory = false, bool overlapped = false, bool caseSensitive = false)
{
    const auto bytes = static_cast<USHORT>(leaf.size() * sizeof(wchar_t));
    UNICODE_STRING objectName{bytes, bytes, const_cast<wchar_t*>(leaf.data())};
    OBJECT_ATTRIBUTES attributes{};
    InitializeObjectAttributes(&attributes, &objectName,
                               OBJ_DONT_REPARSE | (caseSensitive ? 0 : OBJ_CASE_INSENSITIVE), parent, security);
    IO_STATUS_BLOCK io{};
    const ACCESS_MASK access = FILE_READ_ATTRIBUTES | (overlapped ? 0 : SYNCHRONIZE) | extraAccess;
    // Leave the type unrestricted for directory operations. The opened object's
    // attributes are verified before any mutation.
    const ULONG typeOption = allowDirectory ? 0 : FILE_NON_DIRECTORY_FILE;
    const ULONG ioOption = overlapped ? 0 : FILE_SYNCHRONOUS_IO_NONALERT;
    const NTSTATUS status = NtCreateFile(&source, access, &attributes, &io, nullptr, FILE_ATTRIBUTE_NORMAL, sharing, disposition,
                                         typeOption | ioOption | FILE_OPEN_REPARSE_POINT | options,
                                         nullptr, 0);
    return status >= 0 ? ERROR_SUCCESS : RtlNtStatusToDosError(status);
}

inline DWORD OpenSource(HANDLE parent, std::wstring_view leaf, HANDLE& source, ACCESS_MASK extraAccess, ULONG sharing,
                        MoveKind kind = MoveKind::RenameFile, bool caseSensitive = false)
{
    return OpenFile(parent, leaf, source, extraAccess, sharing, FILE_OPEN, 0, nullptr,
                    IsDirectory(kind), false, caseSensitive);
}

// Fixed fields of MS-FSCC Mount Point Reparse Data Buffer.
struct MountPointHeader
{
    DWORD tag;
    USHORT dataLength, reserved, substituteOffset, substituteLength, printOffset, printLength;
};
static_assert(sizeof(MountPointHeader) == 16);

inline DWORD CheckJunctionData(const WCHAR* buffer, size_t bytes)
{
    if (bytes < sizeof(MountPointHeader))
        return ERROR_INVALID_REPARSE_DATA;
    MountPointHeader header{};
    std::memcpy(&header, buffer, sizeof(header));
    constexpr size_t commonHeaderSize = offsetof(MountPointHeader, substituteOffset);
    const size_t pathBytes = bytes - sizeof(header);
    const auto validRange = [pathBytes](size_t offset, size_t length) {
        return offset % sizeof(WCHAR) == 0 && length % sizeof(WCHAR) == 0 &&
               offset <= pathBytes && length <= pathBytes - offset;
    };
    if (header.tag != IO_REPARSE_TAG_MOUNT_POINT)
        return ERROR_REPARSE_TAG_INVALID;
    if (header.dataLength != bytes - commonHeaderSize ||
        !validRange(header.substituteOffset, header.substituteLength) ||
        !validRange(header.printOffset, header.printLength))
        return ERROR_INVALID_REPARSE_DATA;

    const std::wstring_view target(buffer + (sizeof(header) + header.substituteOffset) / sizeof(WCHAR),
                                   header.substituteLength / sizeof(WCHAR));
    constexpr std::wstring_view prefix = L"\\??\\";
    if (target.size() < prefix.size() + 3 || target.substr(0, prefix.size()) != prefix)
        return ERROR_NOT_SUPPORTED;
    const auto path = target.substr(prefix.size());
    const WCHAR drive = path.front();
    if (!((drive >= L'A' && drive <= L'Z') || (drive >= L'a' && drive <= L'z')) ||
        path[1] != L':' || path[2] != L'\\' || path.find(L'\0') != std::wstring_view::npos ||
        path.find_first_of(L":/", 3) != std::wstring_view::npos)
        return ERROR_NOT_SUPPORTED;
    return ERROR_SUCCESS;
}

inline DWORD CheckJunction(HANDLE source)
{
    std::vector<WCHAR> data(MAXIMUM_REPARSE_DATA_BUFFER_SIZE / sizeof(WCHAR));
    const DWORD capacity = static_cast<DWORD>(data.size() * sizeof(WCHAR));
    DWORD bytes = 0;
    if (!DeviceIoControl(source, FSCTL_GET_REPARSE_POINT, nullptr, 0, data.data(), capacity, &bytes, nullptr))
        return GetLastError();
    return bytes <= capacity ? CheckJunctionData(data.data(), bytes) : ERROR_INVALID_REPARSE_DATA;
}

inline DWORD ReadSourceIdentity(HANDLE source, FILE_ID_INFO& identity, MoveKind kind = MoveKind::RenameFile)
{
    FILE_ATTRIBUTE_TAG_INFO attributes{};
    FILE_STANDARD_INFO standard{};
    if (!GetFileInformationByHandleEx(source, FileAttributeTagInfo, &attributes, sizeof(attributes)) ||
        !GetFileInformationByHandleEx(source, FileStandardInfo, &standard, sizeof(standard)) ||
        !GetFileInformationByHandleEx(source, FileIdInfo, &identity, sizeof(identity)))
        return GetLastError();
    const bool directory = IsDirectory(kind);
    const bool junction = kind == MoveKind::RenameJunction;
    DWORD expectedTag = 0;
    if (junction)
        expectedTag = IO_REPARSE_TAG_MOUNT_POINT;
    else if (kind == MoveKind::RenameDirectorySymlink || kind == MoveKind::RenameSymlink)
        expectedTag = IO_REPARSE_TAG_SYMLINK;
    const DWORD tag = (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ? attributes.ReparseTag : 0;
    if (tag != expectedTag || (tag == 0 && (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)))
        return ERROR_REPARSE_TAG_INVALID;
    if ((standard.Directory != FALSE) != directory || standard.NumberOfLinks != 1 || standard.DeletePending)
        return ERROR_NOT_SUPPORTED;
    return junction ? CheckJunction(source) : ERROR_SUCCESS;
}

inline constexpr BYTE transferCancelSignal = 1;

inline DWORD CheckTransferConnection(HANDLE connection, ULONGLONG deadline)
{
    if (GetTickCount64() >= deadline)
        return ERROR_TIMEOUT;
    if (!connection)
        return ERROR_SUCCESS;
    BYTE signal = 0;
    DWORD read = 0, pending = 0;
    if (!PeekNamedPipe(connection, &signal, sizeof(signal), &read, &pending, nullptr))
        return GetLastError();
    if (!pending)
        return ERROR_SUCCESS;
    return pending == sizeof(signal) && read == sizeof(signal) && signal == transferCancelSignal
               ? ERROR_CANCELLED : ERROR_INVALID_DATA;
}

#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
enum class ReplacementFaultPoint { JournalFlushed, BackupRenamed, StageRenamed, StageKept, ReceiptFlushed, BackupRetired };
void PauseReplacementAt(ReplacementFaultPoint point);
#endif

struct StagedReplacement
{
    DWORD error;
    DWORD rollbackError = ERROR_SUCCESS;
};

// The caller verifies the original and holds it against writes and deletes.
// The stage is exclusive and delete-on-close. The caller records the backup
// before invoking this exchange and retires it after successful publication.
inline StagedReplacement ReplaceStagedFile(HANDLE parent, HANDLE original, HANDLE stage,
                                           std::wstring_view leaf, std::wstring_view backup,
                                           HANDLE connection, ULONGLONG deadline)
{
    if (!UacDirectoryLeaf::IsComponent(leaf) || !UacDirectoryLeaf::IsComponent(backup) ||
        UacDirectoryLeaf::Equal(leaf, backup))
        return {ERROR_INVALID_NAME};

    DWORD error = CheckTransferConnection(connection, deadline);
    if (!error)
        error = NativeRename(original, parent, backup);
    if (error)
        return {error};
#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
    PauseReplacementAt(ReplacementFaultPoint::BackupRenamed);
#endif

    error = CheckTransferConnection(connection, deadline);
    if (!error)
        error = NativeRename(stage, parent, leaf);
    if (error)
        return {error, NativeRename(original, parent, leaf)};
#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
    PauseReplacementAt(ReplacementFaultPoint::StageRenamed);
#endif

    FILE_DISPOSITION_INFO_EX keep{FILE_DISPOSITION_FLAG_ON_CLOSE};
    if (!SetFileInformationByHandle(stage, FileDispositionInfoEx, &keep, sizeof(keep)))
    {
        // The stage still owns the name until it closes. Leave the backup
        // and its journal so the caller can handle the interrupted exchange.
        return {GetLastError(), ERROR_PARTIAL_COPY};
    }
#ifdef SAM_UAC_REPLACEMENT_FAULT_TEST
    PauseReplacementAt(ReplacementFaultPoint::StageKept);
#endif
    return {ERROR_SUCCESS};
}

inline bool SameSource(const FILE_ID_INFO& actual, const FILE_ID_INFO& expected)
{
    return actual.VolumeSerialNumber == expected.VolumeSerialNumber &&
           std::memcmp(actual.FileId.Identifier, expected.FileId.Identifier, sizeof(actual.FileId.Identifier)) == 0;
}
}

// The caller keeps the original parent and source identity handles alive
// until completion. A cross-directory move also pins its destination parent.
inline DWORD MoveVerifiedFile(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                              const FILE_ID_INFO& expectedSource, std::wstring_view from,
                              HANDLE targetParent, const UacDirectoryLeaf::ParentIdentity& expectedTarget,
                              std::wstring_view to, MoveKind kind = MoveKind::OrdinaryFile,
                              HANDLE connection = nullptr, ULONGLONG deadline = MAXULONGLONG)
{
    using namespace UacDirectoryLeaf;
    if (!IsComponent(from) || !IsComponent(to))
        return ERROR_INVALID_NAME;
    if (!Equal(expectedParent.volume, expectedTarget.volume) ||
        expectedParent.file.VolumeSerialNumber != expectedTarget.file.VolumeSerialNumber)
        return ERROR_NOT_SAME_DEVICE;
    const bool sameParent = SameParent(expectedParent, expectedTarget);
    const bool renameOnly = kind != MoveKind::OrdinaryFile && kind != MoveKind::OrdinaryDirectory;
    const bool caseOnly = sameParent && from != to && Equal(from, to);
    if ((sameParent && from == to) || (sameParent && Equal(from, to) && (!caseOnly || !renameOnly)) ||
        (renameOnly && !sameParent))
        return ERROR_NOT_SUPPORTED;

    ParentIdentity actual;
    DWORD error = ReadParentIdentity(parent, actual);
    if (error != ERROR_SUCCESS)
        return error;
    if (!SameParent(expectedParent, actual) || expectedSource.VolumeSerialNumber != actual.file.VolumeSerialNumber)
        return ERROR_FILE_INVALID;

    if (parent != targetParent)
    {
        ParentIdentity actualTarget;
        error = ReadParentIdentity(targetParent, actualTarget);
        if (error)
            return error;
        if (!SameParent(expectedTarget, actualTarget))
            return ERROR_FILE_INVALID;
    }
    else if (!SameParent(expectedTarget, actual))
        return ERROR_FILE_INVALID;

    // An exact source open is needed only for a case-only rename when the
    // pinned parent explicitly reports case-sensitive lookup. If the flag is
    // unavailable, retain the existing case-insensitive path unchanged.
    FILE_CASE_SENSITIVE_INFO sensitivity{};
    const bool exactCaseKind = kind == MoveKind::RenameFile || kind == MoveKind::RenameDirectory ||
                               kind == MoveKind::RenameSymlink || kind == MoveKind::RenameDirectorySymlink ||
                               kind == MoveKind::RenameJunction;
    const bool exactCase = caseOnly && exactCaseKind &&
        GetFileInformationByHandleEx(parent, FileCaseSensitiveInfo, &sensitivity, sizeof(sensitivity)) &&
        (sensitivity.Flags & FILE_CS_FLAG_CASE_SENSITIVE_DIR) != 0;
    HANDLE source = nullptr;
    error = Detail::OpenSource(parent, from, source, DELETE, FILE_SHARE_READ, kind, exactCase);
    if (error != ERROR_SUCCESS)
        return error;
    FILE_ID_INFO actualSource{};
    error = Detail::ReadSourceIdentity(source, actualSource, kind);
    if (error == ERROR_SUCCESS && !Detail::SameSource(actualSource, expectedSource))
        error = ERROR_FILE_INVALID;
    if (error == ERROR_SUCCESS && caseOnly)
    {
        // A case-insensitive match alone does not establish that both names
        // designate the approved object, especially in case-sensitive folders.
        // Keep the source pinned while verifying the exact destination alias.
        HANDLE alias = nullptr;
        error = Detail::OpenSource(targetParent, to, alias, 0, FILE_SHARE_READ | FILE_SHARE_DELETE, kind, exactCase);
        if (!error)
        {
            FILE_ID_INFO aliasIdentity{};
            error = Detail::ReadSourceIdentity(alias, aliasIdentity, kind);
            if (!error && !Detail::SameSource(aliasIdentity, actualSource))
                error = ERROR_FILE_INVALID;
            CloseHandle(alias);
        }
        else if (exactCase && error == ERROR_FILE_NOT_FOUND)
            error = ERROR_SUCCESS; // The exact name is free; NativeRename never replaces a collision.
    }
    if (error == ERROR_SUCCESS && kind == MoveKind::OrdinaryFile)
    {
        FILE_ATTRIBUTE_TAG_INFO attributes{};
        constexpr DWORD allowed = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY |
                                  FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
        if (!GetFileInformationByHandleEx(source, FileAttributeTagInfo, &attributes, sizeof(attributes)))
            error = GetLastError();
        else if (!attributes.FileAttributes || (attributes.FileAttributes & ~allowed))
            error = ERROR_NOT_SUPPORTED;
    }
    if (error == ERROR_SUCCESS)
        error = Detail::CheckTransferConnection(connection, deadline);
    if (error == ERROR_SUCCESS)
        error = Detail::NativeRename(source, targetParent, to);
    CloseHandle(source);
    return error;
}

inline DWORD RenameVerifiedFile(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& expectedParent,
                                const FILE_ID_INFO& expectedSource, std::wstring_view from, std::wstring_view to,
                                MoveKind kind = MoveKind::RenameFile)
{
    return MoveVerifiedFile(parent, expectedParent, expectedSource, from, parent, expectedParent, to, kind);
}
}
