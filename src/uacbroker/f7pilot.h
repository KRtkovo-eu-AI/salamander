/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Declare a distinct junction rename operation. | Keep the approved reparse type explicit in the protocol. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Distinguish directory symbolic-link rename on the wire. | Bind the approved source type to the operation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Declare a typed symbolic-link rename request. | Bind the approved object type to the broker operation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Define a typed compressed-file copy. | Bind preservation of NTFS compression to the consent request. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Recognize sparse files for the default F5 pilot. | Keep the plain-file check available for other operations. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Define a distinct copy command that skips named streams. | Keep the selected data policy bound to the authorized request. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Expose whether the metadata request includes attributes. | Keep time-only consent text specific. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Expose the directory timestamp request type. | Let the prompt describe the correct item. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Carry explicit tree consent and an optional cancellation event. | Let the worker execute while the dialog stays responsive. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Identify queued tree transfers in the consent dialog. | Reuse the tree-specific consent and result strings. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Declare the registered F5 tree pilot. | Bind a directory source to a fixed protected target. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Expose a bounded inventory for an approved directory. | Use the existing read-only listing session. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Share the prepared transfer with same-volume F6. | Bind source and destination to a typed move request. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Declare a bounded one-file copy request. | Hold both parent identities across UAC. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Expose the approved listing root to the panel. | Locate the protected suffix for display. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Declare one rooted read-only listing session. | Keep one authorization across navigation and refresh. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Declare the one-file permanent delete request. | Pin the original file across UAC consent and retries. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Add a typed request for basic file metadata. | Share one operation between the ordinary token and the broker. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Capture a file identity for the shared F2 rename path. | Keep the source and parent pinned across consent. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Declare the one-leaf F7 elevation pilot. | Hold the verified parent across consent and distinguish uncertain outcomes. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include <windows.h>
#include <string>
#include <string_view>
#include <vector>

namespace UacF7
{
enum class Operation : DWORD { Mkdir = 1, Rename = 2, Metadata = 3, Delete = 4, List = 5, Copy = 6, Move = 7, CopyTree = 8, CopyIgnoreAds = 9, CopyCompressed = 10, RenameSymlink = 11, RenameDirectorySymlink = 12, RenameJunction = 13 };

inline bool IsRename(Operation operation)
{
    return operation == Operation::Rename || operation == Operation::RenameSymlink ||
           operation == Operation::RenameDirectorySymlink || operation == Operation::RenameJunction;
}

inline bool IsFileCopy(Operation operation)
{
    return operation == Operation::Copy || operation == Operation::CopyIgnoreAds || operation == Operation::CopyCompressed;
}

constexpr ULONGLONG maxPilotCopyBytes = 16 * 1024 * 1024;
constexpr DWORD plainCopyAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                                      FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
inline bool IsPlainCopyFile(DWORD attributes)
{
    return attributes && !(attributes & ~plainCopyAttributes);
}

inline bool IsDefaultCopyFile(DWORD attributes)
{
    return attributes && !(attributes & ~(plainCopyAttributes | FILE_ATTRIBUTE_SPARSE_FILE));
}

constexpr DWORD compressedCopyAttributes = plainCopyAttributes | FILE_ATTRIBUTE_COMPRESSED;

inline bool IsCompressedCopyFile(DWORD attributes)
{
    return (attributes & FILE_ATTRIBUTE_COMPRESSED) && !(attributes & ~compressedCopyAttributes);
}

inline bool IsEligibleFileCopy(DWORD attributes, bool copyAttributes, bool ignoreAds)
{
    if (copyAttributes)
        return !ignoreAds && IsCompressedCopyFile(attributes);
    return ignoreAds ? IsPlainCopyFile(attributes) : IsDefaultCopyFile(attributes);
}

struct Result
{
    DWORD error = ERROR_GEN_FAILURE;
    bool unknown = false;
};

constexpr DWORD basicAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY |
                                  FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
inline bool IsPlainDeleteFile(DWORD attributes)
{
    return attributes == FILE_ATTRIBUTE_NORMAL || attributes == FILE_ATTRIBUTE_ARCHIVE;
}
struct MetadataChange
{
    LONGLONG created = 0;
    LONGLONG accessed = 0;
    LONGLONG modified = 0;
    DWORD expectedAttributes = 0;
    DWORD attributes = 0;
};
static_assert(sizeof(MetadataChange) == 32);

class PreparedLeaf
{
public:
    PreparedLeaf() = default;
    ~PreparedLeaf();
    PreparedLeaf(const PreparedLeaf&) = delete;
    PreparedLeaf& operator=(const PreparedLeaf&) = delete;

    bool Prepare(const std::wstring& path, const std::wstring& renameTo = {});
    bool PrepareRenameLink(const std::wstring& path, const std::wstring& renameTo, bool directory);
    bool IsJunctionRename() const { return operation == Operation::RenameJunction; }
    bool PrepareMetadata(const std::wstring& path, const MetadataChange& change);
    bool PrepareDelete(const std::wstring& path);
    DWORD ExecuteLocal() const;
    Result Execute(HWND owner) const;
    bool IsDirectoryMetadata() const
    {
        return operation == Operation::Metadata && (metadata.expectedAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    bool ChangesAttributes() const { return metadata.attributes != 0; }

private:
    friend class ListingSession;
    friend class PreparedTransfer;
    bool PrepareTarget(const std::wstring& path, Operation requested, const std::wstring& renameTo,
                       const MetadataChange* change);
    HANDLE parent = nullptr;
    HANDLE source = nullptr;
    ULONGLONG serial = 0;
    BYTE fileId[16]{};
    BYTE sourceId[16]{};
    std::wstring volume, relative, leaf, sourceLeaf;
    MetadataChange metadata{};
    Operation operation = Operation::Mkdir;
};

class PreparedTransfer
{
public:
    bool Prepare(const std::wstring& sourcePath, const std::wstring& targetPath, ULONGLONG expectedSize, Operation operation);
    bool PrepareTree(const std::wstring& sourcePath, std::wstring_view targetDirectory, std::wstring& destination);
    Result Execute(HWND owner, HANDLE cancel = nullptr) const;
    bool IsMove() const { return source.operation == Operation::Move; }
    bool IsTree() const { return source.operation == Operation::CopyTree; }
    bool PreservesCompression() const { return source.operation == Operation::CopyCompressed; }
    bool IgnoresAds() const { return source.operation == Operation::CopyIgnoreAds; }

private:
    PreparedLeaf source;
    PreparedLeaf target;
    ULONGLONG size = 0;
};

struct TransferPrompt
{
    const PreparedTransfer& request;
    const char* sourcePath;
    const char* targetPath;
    Result result{ERROR_CANCELLED, false};
    bool approved = false;
};

struct DirectoryInventory
{
    ULONGLONG bytes = 0;
    DWORD files = 0;
    DWORD directories = 0;
};
static_assert(sizeof(DirectoryInventory) == 16);

class ListingSession
{
public:
    ListingSession() = default;
    ~ListingSession();
    ListingSession(const ListingSession&) = delete;
    ListingSession& operator=(const ListingSession&) = delete;

    bool Prepare(const std::wstring& path);
    DWORD Start(HWND owner);
    bool Contains(const std::wstring& path) const;
    const std::wstring& GetRoot() const { return root; }
    DWORD List(const std::wstring& path, std::vector<WIN32_FIND_DATAW>& entries);
    DWORD Inventory(const std::wstring& path, DirectoryInventory& inventory);
    bool IsOpen() const { return pipe != nullptr; }

private:
    DWORD SendPath(const std::wstring& path, DWORD flags, ULONGLONG deadline);
    void Close();
    PreparedLeaf prepared;
    std::wstring root;
    HANDLE pipe = nullptr;
};

struct LeafPrompt
{
    const PreparedLeaf& request;
    const char* path;
    Result result{ERROR_CANCELLED, false};
};
}
