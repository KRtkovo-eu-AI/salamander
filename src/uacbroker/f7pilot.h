/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Add a typed request for basic file metadata. | Share one operation between the ordinary token and the broker. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Capture a file identity for the shared F2 rename path. | Keep the source and parent pinned across consent. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Declare the one-leaf F7 elevation pilot. | Hold the verified parent across consent and distinguish uncertain outcomes. | | GPT-6.1 Sol (Codex) |
*/
#pragma once

#include <windows.h>
#include <string>

namespace UacF7
{
struct Result
{
    DWORD error = ERROR_GEN_FAILURE;
    bool unknown = false;
};

constexpr DWORD basicAttributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY |
                                  FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
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
    DWORD ExecuteLocalMetadata() const;
    Result Execute(HWND owner) const;

private:
    bool PrepareTarget(const std::wstring& path, const std::wstring& renameTo, const MetadataChange* change);
    HANDLE parent = nullptr;
    HANDLE source = nullptr;
    ULONGLONG serial = 0;
    BYTE fileId[16]{};
    BYTE sourceId[16]{};
    std::wstring volume, relative, leaf, sourceLeaf;
    MetadataChange metadata{};
    bool isMetadata = false;
};

struct MetadataPrompt
{
    const PreparedLeaf& request;
    const char* path;
    Result result{ERROR_CANCELLED, false};
};
}
