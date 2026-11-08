/*
| Date | Summary | Reason | Ticket | Model |
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
enum class Operation : DWORD { Mkdir = 1, Rename = 2, Metadata = 3, Delete = 4, List = 5, Copy = 6, Move = 7, CopyTree = 8 };

constexpr ULONGLONG maxPilotCopyBytes = 16 * 1024 * 1024;
constexpr DWORD plainCopyAttributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE |
                                      FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
inline bool IsPlainCopyFile(DWORD attributes)
{
    return attributes && !(attributes & ~plainCopyAttributes);
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
    bool PrepareMetadata(const std::wstring& path, const MetadataChange& change);
    bool PrepareDelete(const std::wstring& path);
    DWORD ExecuteLocal() const;
    Result Execute(HWND owner) const;
    bool IsDirectoryMetadata() const
    {
        return operation == Operation::Metadata && (metadata.expectedAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

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
    bool Prepare(const std::wstring& sourcePath, const std::wstring& targetPath, ULONGLONG expectedSize, bool moving);
    bool PrepareTree(const std::wstring& sourcePath, std::wstring_view targetDirectory, std::wstring& destination);
    Result Execute(HWND owner, HANDLE cancel = nullptr) const;
    bool IsMove() const { return source.operation == Operation::Move; }
    bool IsTree() const { return source.operation == Operation::CopyTree; }

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
