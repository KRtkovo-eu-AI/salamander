/*
| Date | Summary | Reason | Ticket | Model |
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

class PreparedLeaf
{
public:
    PreparedLeaf() = default;
    ~PreparedLeaf();
    PreparedLeaf(const PreparedLeaf&) = delete;
    PreparedLeaf& operator=(const PreparedLeaf&) = delete;

    bool Prepare(const std::wstring& path, const std::wstring& renameTo = {});
    Result Execute(HWND owner) const;

private:
    HANDLE parent = nullptr;
    HANDLE source = nullptr;
    ULONGLONG serial = 0;
    BYTE fileId[16]{};
    BYTE sourceId[16]{};
    std::wstring volume, relative, leaf, sourceLeaf;
};
}
