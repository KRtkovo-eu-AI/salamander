// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Check the Find paging protocol version. | Refuse mismatched broker wire semantics. | #731 | GPT-6.1 Terra |
// | 2026-11-12 | Retry denied Branch listings after scoped consent. | Keep partial results and verify listed directory identities. | #731 | GPT-6.1 Terra |
#include "../uacbroker/listing.h"
#include "../uacbroker/uacprotocol.h"
#include <cstdio>
#include <string>
static int failures = 0;
static void Check(bool ok, const char* label) { std::printf("%s: %s\n", ok ? "PASS" : "FAIL", label); if (!ok) ++failures; }
int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    using UacF7::Operation;
    Check(SAM_UAC_PROTOCOL_VERSION == 44, "protocol version is 44");
    Check(UacListing::ValidVerifiedListingScope(Operation::CompareList, 0), "verified metadata comparison accepts root");
    Check(!UacListing::ValidVerifiedListingScope(Operation::CompareContent, 0) &&
          !UacListing::ValidVerifiedListingScope(Operation::List, 0) &&
          !UacListing::ValidVerifiedListingScope(Operation::Inventory, 0), "other scopes cannot request verified root");
    const std::wstring root = std::wstring(argv[1]) + L"\\branch-identity-" + std::to_wstring(GetCurrentProcessId());
    const std::wstring moved = root + L"-moved";
    if (!CreateDirectoryW(root.c_str(), nullptr)) return 3;
    HANDLE first = CreateFileW(root.c_str(), FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    UacDirectoryLeaf::ParentIdentity before{};
    if (first == INVALID_HANDLE_VALUE || UacDirectoryLeaf::ReadParentIdentity(first, before)) return 4;
    UacF7::MoveFileIdentity expected{}; expected.serial = before.file.VolumeSerialNumber;
    memcpy(expected.id, before.file.FileId.Identifier, sizeof(expected.id));
    Check(UacListing::MatchesListingIdentity(before, expected), "original opened root matches captured ID");
    CloseHandle(first);
    if (!MoveFileW(root.c_str(), moved.c_str()) || !CreateDirectoryW(root.c_str(), nullptr)) return 5;
    HANDLE replacement = CreateFileW(root.c_str(), FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    UacDirectoryLeaf::ParentIdentity after{};
    if (replacement == INVALID_HANDLE_VALUE || UacDirectoryLeaf::ReadParentIdentity(replacement, after)) return 6;
    Check(!UacListing::MatchesListingIdentity(after, expected), "replacement at same path rejects old ID");
    auto wrongVolume = expected; ++wrongVolume.serial;
    Check(!UacListing::MatchesListingIdentity(before, wrongVolume), "different volume identity rejected");
    CloseHandle(replacement);
    if (!RemoveDirectoryW(root.c_str()) || !RemoveDirectoryW(moved.c_str())) return 7;
    return failures ? 1 : 0;
}
