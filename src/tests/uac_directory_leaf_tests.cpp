/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Exercise bounded NTFS tree copying. | Verify whole-tree publication and rollback of owned temporary entries. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise source manifest revalidation on NTFS. | Detect replacements and new entries even when a size summary would miss them. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Check strict recursive inventory on NTFS. | Verify Unicode contents and suppress results on timeout or reparse entries. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise a verified same-volume move. | Check identity, collisions, cancellation and peer loss. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Test the broker copy guard conditions. | Reject a stale size, an expired deadline and a lost connection before publishing. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify a staged copy on real NTFS. | Exercise file content, collisions, cancellation and unsupported source metadata. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Check repeated rooted directory reads on real NTFS. | Verify refresh returns fresh wide entries and reparse traversal fails. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Test verified permanent file deletion on NTFS. | Cover replacement, locks, special attributes and long Unicode names. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Exercise the basic metadata executor on real NTFS files. | Verify independent fields, stale snapshots, and preservation of the verified source. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Probe NTFS native rename with a directory handle. | Verify the target cannot be redirected before adding broker rename. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Test anchored mkdir against real NTFS substitutions. | Verify that a replacement path and reparse points cannot redirect a creation. | | GPT-6.1 Sol (Codex) |
*/
#include "../uacbroker/metadataleaf.h"
#include "../uacbroker/deleteleaf.h"
#include "../uacbroker/listing.h"
#include "../uacbroker/copyleaf.h"
#include "../uacbroker/copytree.h"
#include <winioctl.h>
#include <cstdio>
#include <memory>

namespace
{
struct Close
{
    void operator()(void* handle) const { CloseHandle(handle); }
};
using Handle = std::unique_ptr<void, Close>;

bool DirectoryExists(const std::wstring& path)
{
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

Handle CreateRenameSource(const std::wstring& path)
{
    const HANDLE file = CreateFileW(path.c_str(), DELETE, FILE_SHARE_READ, nullptr,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    return Handle(file == INVALID_HANDLE_VALUE ? nullptr : file);
}

Handle OpenIdentity(const std::wstring& path, FILE_ID_INFO& identity)
{
    const HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                    OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    Handle handle(file == INVALID_HANDLE_VALUE ? nullptr : file);
    if (handle && !GetFileInformationByHandleEx(handle.get(), FileIdInfo, &identity, sizeof(identity)))
        handle.reset();
    return handle;
}

bool WriteFixture(const std::wstring& path, const std::vector<BYTE>& data)
{
    const HANDLE raw = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (raw == INVALID_HANDLE_VALUE)
        return false;
    const Handle file(raw);
    DWORD written = 0;
    return WriteFile(file.get(), data.data(), static_cast<DWORD>(data.size()), &written, nullptr) && written == data.size();
}

bool HasContents(const std::wstring& path, const std::vector<BYTE>& expected)
{
    const HANDLE raw = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (raw == INVALID_HANDLE_VALUE)
        return false;
    const Handle file(raw);
    std::vector<BYTE> data(expected.size() + 1);
    DWORD read = 0;
    if (!ReadFile(file.get(), data.data(), static_cast<DWORD>(data.size()), &read, nullptr))
        return false;
    data.resize(read);
    return data == expected;
}

bool Exists(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

int failures = 0;
void Check(bool passed, const char* label, DWORD error = 0)
{
    std::printf("%s: %s (%lu)\n", passed ? "PASS" : "FAIL", label, error);
    if (!passed)
        ++failures;
}

bool CheckTreeCopy(const std::wstring& fixture, HANDLE parent)
{
    using namespace UacDirectoryLeaf;
    const std::wstring source = fixture + L"\\tree-source";
    const std::wstring target = fixture + L"\\tree-target";
    const std::wstring nested = L"\\a-\u76ee\u9332";
    const std::wstring file = nested + L"\\\u65e5.bin";
    const std::wstring longChild = nested + L"\\" + std::wstring(170, L'\u76ee');
    const std::wstring longLeaf = longChild + L"\\" + std::wstring(150, L'\u7dd1') + L".bin";
    const std::wstring published = target + L"\\published";
    if (!CreateDirectoryW(source.c_str(), nullptr) || !CreateDirectoryW(target.c_str(), nullptr) ||
        !CreateDirectoryW((source + nested).c_str(), nullptr) ||
        !CreateDirectoryW((source + longChild).c_str(), nullptr) ||
        !CreateDirectoryW((source + L"\\empty").c_str(), nullptr) ||
        !WriteFixture(source + file, {4, 8, 15, 16, 23, 42}) ||
        !WriteFixture(source + longLeaf, {9}))
        return false;

    const FILETIME modification{0, 0x01DA0000};
    {
        const HANDLE raw = CreateFileW((source + nested).c_str(), FILE_WRITE_ATTRIBUTES, 0, nullptr,
                                       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        const Handle directory(raw);
        if (!SetFileTime(directory.get(), nullptr, nullptr, &modification))
            return false;
    }

    HANDLE rawSource = nullptr, rawTarget = nullptr;
    ParentIdentity sourceId, targetId;
    if (OpenParentBelowRoot(parent, L"tree-source", rawSource, sourceId, FILE_LIST_DIRECTORY | FILE_READ_EA))
        return false;
    const Handle from(rawSource);
    if (OpenParentBelowRoot(parent, L"tree-target", rawTarget, targetId, FILE_LIST_DIRECTORY))
        return false;
    const Handle to(rawTarget);
    const auto noStaging = [&]() {
        std::vector<WIN32_FIND_DATAW> entries;
        if (UacListing::Enumerate(to.get(), entries))
            return false;
        for (const auto& entry : entries)
            if (std::wstring_view(entry.cFileName).substr(0, ARRAYSIZE(UacCopyTree::stagePrefix) - 1) == UacCopyTree::stagePrefix)
                return false;
        return true;
    };

    auto result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"published");
    Check(!result.error && !result.cleanupError && noStaging() &&
              HasContents(published + file, {4, 8, 15, 16, 23, 42}) && DirectoryExists(published + L"\\empty") &&
              HasContents(source + file, {4, 8, 15, 16, 23, 42}) && HasContents(published + longLeaf, {9}),
          "publish a complete Unicode tree with an empty directory and preserve the source", result.error);
    WIN32_FILE_ATTRIBUTE_DATA copiedDirectory{};
    Check(GetFileAttributesExW((published + nested).c_str(), GetFileExInfoStandard, &copiedDirectory) &&
              CompareFileTime(&copiedDirectory.ftLastWriteTime, &modification) == 0,
          "preserve the last-write time of an ordinary nested directory");

    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"published");
    Check((result.error == ERROR_ALREADY_EXISTS || result.error == ERROR_FILE_EXISTS) && !result.cleanupError &&
              noStaging() && HasContents(published + file, {4, 8, 15, 16, 23, 42}),
          "roll back the staged tree and preserve the existing destination on collision", result.error);

    const auto adsFile = source + L"\\z-unsupported.bin";
    if (!WriteFixture(adsFile, {0}) || !WriteFixture(adsFile + L":stream", {5}))
        return false;
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"unsupported");
    Check(result.error == ERROR_NOT_SUPPORTED && !result.cleanupError && noStaging() &&
              !Exists(target + L"\\unsupported") && HasContents(adsFile + L":stream", {5}),
          "unsupported file streams abort after staging earlier entries and remove the temporary tree", result.error);
    if (!DeleteFileW(adsFile.c_str()) || !WriteFixture(source + nested + L":stream", {7}))
        return false;
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"directory-stream");
    Check(result.error == ERROR_NOT_SUPPORTED && !result.cleanupError && noStaging() && !Exists(target + L"\\directory-stream"),
          "reject a directory with an alternate stream", result.error);
    if (!DeleteFileW((source + nested + L":stream").c_str()))
        return false;

    const auto oversized = source + L"\\too-big.bin";
    {
        const HANDLE raw = CreateFileW(oversized.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        const Handle fileHandle(raw);
        LARGE_INTEGER size{};
        size.QuadPart = UacF7::maxPilotCopyBytes + 1;
        if (!SetFilePointerEx(fileHandle.get(), size, nullptr, FILE_BEGIN) || !SetEndOfFile(fileHandle.get()))
            return false;
    }
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"oversized");
    Check(result.error == ERROR_FILE_TOO_LARGE && !result.cleanupError && noStaging(),
          "reject a tree above the total byte budget", result.error);
    if (!DeleteFileW(oversized.c_str()))
        return false;

    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), sourceId, L"wrong-parent");
    Check(result.error == ERROR_FILE_INVALID && !result.cleanupError && noStaging(),
          "reject a substituted destination parent before staging", result.error);
    result = UacCopyTree::CopyVerifiedTree(from.get(), targetId, to.get(), targetId, L"wrong-source");
    Check(result.error == ERROR_FILE_INVALID && !result.cleanupError && noStaging(),
          "reject a substituted source root before staging", result.error);
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, from.get(), sourceId, L"inside");
    Check(result.error == ERROR_INVALID_PARAMETER && !Exists(source + L"\\inside"),
          "reject copying a tree into itself", result.error);
    HANDLE rawNested = nullptr;
    ParentIdentity nestedId;
    if (OpenParentBelowRoot(from.get(), std::wstring_view(nested).substr(1), rawNested, nestedId))
        return false;
    const Handle descendant(rawNested);
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, descendant.get(), nestedId, L"inside");
    Check(result.error == ERROR_INVALID_PARAMETER && !Exists(source + nested + L"\\inside"),
          "reject a destination below the source root by identity", result.error);
    const Handle cancel(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (!cancel)
        return false;
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"cancelled", cancel.get());
    Check(result.error == ERROR_CANCELLED && !result.cleanupError && noStaging() && !Exists(target + L"\\cancelled"),
          "honor a signalled cancellation before staging a tree", result.error);
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"timed-out", nullptr, nullptr, GetTickCount64());
    Check(result.error == ERROR_TIMEOUT && !result.cleanupError && noStaging(),
          "honor an expired tree copy deadline before staging", result.error);
    HANDLE rawRead = nullptr, rawWrite = nullptr;
    if (!CreatePipe(&rawRead, &rawWrite, nullptr, 0))
        return false;
    const Handle read(rawRead);
    CloseHandle(rawWrite);
    result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"disconnected", nullptr, read.get());
    Check(result.error == ERROR_BROKEN_PIPE && !result.cleanupError && noStaging(),
          "refuse to stage a tree after the peer disconnects", result.error);
    return true;
}
}

int wmain(int argc, wchar_t** argv)
{
    using namespace UacDirectoryLeaf;
    if (argc != 2)
        return 2;
    const std::wstring fixture = L"\\\\?\\" + std::wstring(argv[1]);
    const HANDLE rawRoot = CreateFileW(fixture.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                        nullptr);
    if (rawRoot == INVALID_HANDLE_VALUE)
        return 2;
    const Handle root(rawRoot);
    if (!CheckTreeCopy(fixture, root.get()))
        return 2;
    HANDLE rawListing = nullptr;
    ParentIdentity listingIdentity;
    DWORD listingError = OpenParentBelowRoot(root.get(), L"destination", rawListing, listingIdentity, FILE_LIST_DIRECTORY);
    if (listingError != ERROR_SUCCESS)
        return 2;
    const Handle listing(rawListing);
    std::vector<WIN32_FIND_DATAW> entries;
    listingError = UacListing::Enumerate(listing.get(), entries);
    Check(listingError == ERROR_SUCCESS && entries.size() == 1 &&
              std::wstring_view(entries.front().cFileName) == L"nested",
          "enumerate the approved directory by handle", listingError);
    if (!CreateRenameSource(fixture + L"\\destination\\\u6771\u4eac.txt"))
        return 2;
    listingError = UacListing::Enumerate(listing.get(), entries);
    bool foundUnicode = false;
    for (const auto& entry : entries)
        foundUnicode |= std::wstring_view(entry.cFileName) == L"\u6771\u4eac.txt";
    Check(listingError == ERROR_SUCCESS && entries.size() == 2 && foundUnicode,
          "refresh the same directory handle and observe new Unicode entries", listingError);
    listingError = OpenParentBelowRoot(listing.get(), L"..\\junction", rawListing, listingIdentity, FILE_LIST_DIRECTORY);
    Check(listingError == ERROR_INVALID_NAME && !rawListing, "reject escape from approved listing root", listingError);

    UacF7::DirectoryInventory inventory;
    const auto inventoryFile = fixture + L"\\destination\\nested\\\u65e5.bin";
    if (!WriteFixture(inventoryFile, {1, 2, 3}))
        return 2;
    DWORD inventoryError = UacListing::Inventory(listing.get(), MAXULONGLONG, inventory);
    Check(inventoryError == ERROR_SUCCESS && inventory.files == 2 && inventory.directories == 1 && inventory.bytes == 3,
          "strict inventory sums a nested Unicode file and an empty file", inventoryError);
    UacListing::SubtreeManifest manifest;
    DWORD manifestError = UacListing::CaptureManifest(listing.get(), MAXULONGLONG, manifest);
    FILE_ID_INFO nestedFileId{};
    const Handle nestedFile = OpenIdentity(inventoryFile, nestedFileId);
    Check(manifestError == ERROR_SUCCESS && nestedFile && manifest.entries.size() == 3 &&
              manifest.entries[0].parent == UacListing::manifestRoot && manifest.entries[0].name == L"nested" &&
              manifest.entries[1].parent == 0 && manifest.entries[1].name == L"\u65e5.bin" &&
              std::memcmp(manifest.entries[1].id.Identifier, nestedFileId.FileId.Identifier,
                          sizeof(nestedFileId.FileId.Identifier)) == 0,
          "record the NTFS file identity and its exact Unicode name and parent", manifestError);
    manifestError = UacListing::VerifyManifest(listing.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_SUCCESS, "unchanged subtree matches the source manifest", manifestError);
    const auto renamedFile = fixture + L"\\destination\\nested\\held-original.bin";
    if (!MoveFileExW(inventoryFile.c_str(), renamedFile.c_str(), 0) ||
        !WriteFixture(inventoryFile, {1, 2, 3}) || !DeleteFileW(renamedFile.c_str()))
        return 2;
    manifestError = UacListing::VerifyManifest(listing.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_FILE_INVALID, "a same-name same-size replacement has a different identity", manifestError);

    manifestError = UacListing::CaptureManifest(listing.get(), MAXULONGLONG, manifest);
    if (manifestError)
        return 2;
    const std::wstring longDirectory = fixture + L"\\destination\\nested\\" + std::wstring(180, L'\u76ee');
    const std::wstring longFile = longDirectory + L"\\" + std::wstring(160, L'\u7dd1') + L".bin";
    if (!CreateDirectoryW(longDirectory.c_str(), nullptr) || !WriteFixture(longFile, {4, 5}))
        return 2;
    manifestError = UacListing::VerifyManifest(listing.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_FILE_INVALID, "reject a newly added subtree with a long Unicode path", manifestError);
    manifestError = UacListing::CaptureManifest(listing.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_SUCCESS && manifest.entries.size() == 5 && manifest.inventory.bytes == 5,
          "capture identities through a long Unicode path", manifestError);
    if (!DeleteFileW(longFile.c_str()) || !RemoveDirectoryW(longDirectory.c_str()))
        return 2;
    manifestError = UacListing::VerifyManifest(listing.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_FILE_INVALID, "reject removed entries from the captured source", manifestError);

    const std::wstring empty = fixture + L"\\destination\\empty";
    const std::wstring originalEmpty = fixture + L"\\destination\\held-empty";
    if (!CreateDirectoryW(empty.c_str(), nullptr) ||
        UacListing::CaptureManifest(listing.get(), MAXULONGLONG, manifest) ||
        !MoveFileExW(empty.c_str(), originalEmpty.c_str(), 0) ||
        !CreateDirectoryW(empty.c_str(), nullptr) || !RemoveDirectoryW(originalEmpty.c_str()))
        return 2;
    manifestError = UacListing::VerifyManifest(listing.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_FILE_INVALID, "reject a same-name replacement of an empty directory", manifestError);
    if (!RemoveDirectoryW(empty.c_str()))
        return 2;

    manifestError = UacListing::CaptureManifest(root.get(), MAXULONGLONG, manifest);
    Check(manifestError == ERROR_REPARSE_TAG_INVALID && manifest.entries.empty() && manifest.root.volume.empty(),
          "publish no manifest when the scan encounters a junction", manifestError);
    inventoryError = UacListing::Inventory(root.get(), MAXULONGLONG, inventory);
    Check(inventoryError == ERROR_REPARSE_TAG_INVALID && !inventory.files && !inventory.directories && !inventory.bytes,
          "a junction aborts inventory without publishing partial counts", inventoryError);
    inventoryError = UacListing::Inventory(listing.get(), GetTickCount64(), inventory);
    Check(inventoryError == ERROR_TIMEOUT && !inventory.files && !inventory.directories && !inventory.bytes,
          "an expired inventory deadline publishes no counts", inventoryError);

    DWORD remaining = 0;
    inventoryError = UacListing::Detail::ScanDirectory(listing.get(), 0, UacListing::manifestRoot,
                                                        remaining, MAXULONGLONG, manifest);
    Check(inventoryError == ERROR_BUFFER_OVERFLOW && manifest.entries.empty(),
          "inventory rejects a directory exceeding its remaining entry budget", inventoryError);
    remaining = UacListing::maxEntries;
    inventoryError = UacListing::Detail::ScanDirectory(listing.get(), UacListing::maxInventoryDepth + 1,
                                                       UacListing::manifestRoot, remaining, MAXULONGLONG, manifest);
    Check(inventoryError == ERROR_BUFFER_OVERFLOW && manifest.entries.empty(),
          "inventory refuses to traverse beyond its depth limit", inventoryError);

    const auto ambiguousName = fixture + L"\\destination\\nested\\ambiguous.";
    if (!WriteFixture(ambiguousName, {4}))
        return 2;
    inventoryError = UacListing::Inventory(listing.get(), MAXULONGLONG, inventory);
    Check(inventoryError == ERROR_INVALID_NAME && !inventory.files && !inventory.directories && !inventory.bytes,
          "a filtered name prevents publishing an incomplete inventory", inventoryError);
    if (!DeleteFileW(ambiguousName.c_str()))
        return 2;

    HANDLE rawParent = nullptr;
    ParentIdentity expected;
    DWORD error = OpenParentBelowRoot(root.get(), L"ancestor\\parent", rawParent, expected);
    Check(error == ERROR_SUCCESS, "open verified NTFS parent below root", error);
    if (error)
        return 1;
    const Handle original(rawParent);
    const auto ancestor = fixture + L"\\ancestor";
    const auto moved = fixture + L"\\moved";
    if (!MoveFileExW((ancestor + L"\\parent").c_str(), moved.c_str(), 0) ||
        !CreateDirectoryW((ancestor + L"\\parent").c_str(), nullptr))
    {
        Check(false, "swap the parent while its original handle is held", GetLastError());
        return 1;
    }

    ParentIdentity replacement;
    error = OpenParentBelowRoot(root.get(), L"ancestor\\parent", rawParent, replacement);
    Check(error == ERROR_SUCCESS && !SameParent(expected, replacement), "replacement has a different identity", error);
    if (error)
        return 1;
    const Handle substitute(rawParent);
    const std::wstring leaf = L"\u017dlu\u0165ou\u010dk\u00fd-\u6771\u4eac";
    const auto unexpected = ancestor + L"\\parent\\" + leaf;
    const auto intended = moved + L"\\" + leaf;
    error = CreateVerifiedLeaf(substitute.get(), expected, leaf);
    Check(error == ERROR_FILE_INVALID && !DirectoryExists(unexpected), "reopened replacement rejected before mutation", error);

    error = CreateVerifiedLeaf(original.get(), expected, leaf);
    Check(error == ERROR_SUCCESS && DirectoryExists(intended) && !DirectoryExists(unexpected),
          "held handle creates only under the original parent after swap", error);
    error = CreateVerifiedLeaf(original.get(), expected, leaf);
    Check(error == ERROR_ALREADY_EXISTS && DirectoryExists(intended), "collision preserves existing folder", error);

    const auto copySource = fixture + L"\\destination\\copy-\u65e5.bin";
    const auto copyTarget = moved + L"\\copy-\u672c.bin";
    std::vector<BYTE> content(192 * 1024 + 17);
    for (size_t i = 0; i < content.size(); ++i)
        content[i] = static_cast<BYTE>(i % 251);
    FILE_ID_INFO copyId{};
    if (!WriteFixture(copySource, content) || !SetFileAttributesW(copySource.c_str(), FILE_ATTRIBUTE_HIDDEN) ||
        !OpenIdentity(copySource, copyId))
        return 2;
    const auto copy = [&](HANDLE target, std::wstring_view name, const FILE_ID_INFO& identity, HANDLE cancel = nullptr)
    {
        return UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, identity, L"copy-\u65e5.bin",
                                             target, expected, name, cancel);
    };
    DWORD copyError = copy(substitute.get(), L"copy-\u672c.bin", copyId);
    Check(copyError == ERROR_FILE_INVALID && !Exists(copyTarget), "reject a swapped copy target directory", copyError);
    FILE_ID_INFO staleCopyId = copyId;
    staleCopyId.FileId.Identifier[0] ^= 1;
    copyError = copy(original.get(), L"copy-\u672c.bin", staleCopyId);
    Check(copyError == ERROR_FILE_INVALID && !Exists(copyTarget), "reject a stale copy source identity", copyError);
    Handle cancelled(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (!cancelled)
        return 2;
    copyError = copy(original.get(), L"copy-\u672c.bin", copyId, cancelled.get());
    Check(copyError == ERROR_CANCELLED && !Exists(copyTarget), "cancel before staging a copy", copyError);

    const auto guardedCopy = [&](HANDLE pipe, ULONGLONG deadline, ULONGLONG size)
    {
        return UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, copyId, L"copy-\u65e5.bin",
                                             original.get(), expected, L"copy-\u672c.bin", nullptr, pipe, deadline, size);
    };
    copyError = guardedCopy(nullptr, MAXULONGLONG, content.size() - 1);
    Check(copyError == ERROR_FILE_INVALID && !Exists(copyTarget), "reject a stale source length", copyError);
    copyError = guardedCopy(nullptr, GetTickCount64(), content.size());
    Check(copyError == ERROR_TIMEOUT && !Exists(copyTarget), "expired copy deadline leaves no target", copyError);
    HANDLE rawPipeRead = nullptr, rawPipeWrite = nullptr;
    if (!CreatePipe(&rawPipeRead, &rawPipeWrite, nullptr, 0))
        return 2;
    const Handle pipeRead(rawPipeRead);
    CloseHandle(rawPipeWrite);
    copyError = guardedCopy(pipeRead.get(), MAXULONGLONG, content.size());
    Check(copyError == ERROR_BROKEN_PIPE && !Exists(copyTarget), "disconnected copy request leaves no target", copyError);

    copyError = copy(original.get(), L"copy-\u672c.bin", copyId);
    WIN32_FILE_ATTRIBUTE_DATA sourceData{}, targetData{};
    Check(copyError == ERROR_SUCCESS && HasContents(copyTarget, content), "publish complete copy at the original target", copyError);
    Check(HasContents(copySource, content), "source contents stay intact");
    const bool haveMetadata = GetFileAttributesExW(copySource.c_str(), GetFileExInfoStandard, &sourceData) &&
                              GetFileAttributesExW(copyTarget.c_str(), GetFileExInfoStandard, &targetData);
    Check(haveMetadata && (sourceData.dwFileAttributes | FILE_ATTRIBUTE_ARCHIVE) == targetData.dwFileAttributes, "preserve ordinary attributes and set archive on the copy", targetData.dwFileAttributes);
    Check(haveMetadata && CompareFileTime(&sourceData.ftCreationTime, &targetData.ftCreationTime) == 0,
          "preserve source creation time");
    Check(haveMetadata && CompareFileTime(&sourceData.ftLastWriteTime, &targetData.ftLastWriteTime) == 0,
          "preserve source modification time");

    const auto copyCollisionPath = moved + L"\\copy-collision.bin";
    const std::vector<BYTE> existing{13, 42};
    if (!WriteFixture(copyCollisionPath, existing))
        return 2;
    copyError = copy(original.get(), L"copy-collision.bin", copyId);
    Check(copyError == ERROR_ALREADY_EXISTS && HasContents(copyCollisionPath, existing),
          "commit refuses to replace an existing target", copyError);
    WIN32_FIND_DATAW stagedData{};
    const HANDLE staged = FindFirstFileW((moved + L"\\.samandarin-copy-*.tmp").c_str(), &stagedData);
    Check(staged == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND, "no staging files after success or collision");
    if (staged != INVALID_HANDLE_VALUE)
        FindClose(staged);

    HANDLE rawUncommitted = nullptr;
    copyError = UacCopyLeaf::Detail::StageFile(original.get(), rawUncommitted);
    if (copyError)
        return 2;
    Handle uncommitted(rawUncommitted);
    copyError = UacRenameLeaf::Detail::NativeRename(uncommitted.get(), original.get(), L"uncommitted-copy.bin");
    uncommitted.reset();
    Check(copyError == ERROR_SUCCESS && !Exists(moved + L"\\uncommitted-copy.bin"),
          "closing after rename but before commit removes the uncommitted target", copyError);

    const HANDLE rawWriter = CreateFileW(copySource.c_str(), FILE_WRITE_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (rawWriter == INVALID_HANDLE_VALUE)
        return 2;
    Handle copyWriter(rawWriter);
    copyError = copy(original.get(), L"copy-busy.bin", copyId);
    copyWriter.reset();
    Check(copyError == ERROR_SHARING_VIOLATION && !Exists(moved + L"\\copy-busy.bin"),
          "refuse a concurrent writer before copying", copyError);

    const auto emptySource = fixture + L"\\destination\\empty-copy-source";
    FILE_ID_INFO emptyId{};
    if (!WriteFixture(emptySource, {}) || !OpenIdentity(emptySource, emptyId))
        return 2;
    copyError = UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, emptyId, L"empty-copy-source",
                                             original.get(), expected, L"empty-copy-target");
    Check(copyError == ERROR_SUCCESS && HasContents(moved + L"\\empty-copy-target", {}),
          "publish an empty file", copyError);

    if (!WriteFixture(copySource + L":named", existing))
        return 2;
    copyError = copy(original.get(), L"copy-with-stream.bin", copyId);
    Check(copyError == ERROR_NOT_SUPPORTED && !Exists(moved + L"\\copy-with-stream.bin"),
          "reject alternate streams before publishing a copy", copyError);

    const auto moveSourcePath = fixture + L"\\destination\\move-\u65e5.bin";
    const auto moveTargetPath = moved + L"\\move-\u672c.bin";
    FILE_ID_INFO moveId{};
    if (!WriteFixture(moveSourcePath, content) || !OpenIdentity(moveSourcePath, moveId))
        return 2;
    const auto move = [&](HANDLE directory, const ParentIdentity& identity, std::wstring_view name, HANDLE pipe = nullptr)
    {
        return UacRenameLeaf::MoveVerifiedFile(listing.get(), listingIdentity, moveId, L"move-\u65e5.bin",
                                               directory, identity, name, true, pipe);
    };
    auto moveError = move(substitute.get(), expected, L"move-\u672c.bin");
    Check(moveError == ERROR_FILE_INVALID && Exists(moveSourcePath) && !Exists(moveTargetPath),
          "move rejects a substituted destination directory", moveError);
    auto foreign = expected;
    foreign.file.VolumeSerialNumber ^= 1;
    moveError = move(original.get(), foreign, L"move-\u672c.bin");
    Check(moveError == ERROR_NOT_SAME_DEVICE && Exists(moveSourcePath) && !Exists(moveTargetPath),
          "move rejects a different volume", moveError);
    moveError = move(original.get(), expected, L"copy-collision.bin");
    Check(moveError == ERROR_ALREADY_EXISTS && HasContents(copyCollisionPath, existing) && Exists(moveSourcePath),
          "move collision preserves source and existing destination", moveError);
    moveError = move(original.get(), expected, L"move-\u672c.bin", pipeRead.get());
    Check(moveError == ERROR_BROKEN_PIPE && Exists(moveSourcePath) && !Exists(moveTargetPath),
          "move aborts when its peer has disconnected before rename", moveError);
    moveError = move(original.get(), expected, L"move-\u672c.bin");
    FILE_ID_INFO movedId{};
    Check(moveError == ERROR_SUCCESS && !Exists(moveSourcePath) && HasContents(moveTargetPath, content) &&
              OpenIdentity(moveTargetPath, movedId) && UacRenameLeaf::Detail::SameSource(movedId, moveId),
          "cross-directory move preserves the file ID and bytes", moveError);

    const auto sourcePath = fixture + L"\\rename-source";
    const auto renamedPath = moved + L"\\rename-\u6771\u4eac";
    const auto replacedPath = ancestor + L"\\parent\\rename-\u6771\u4eac";
    auto source = CreateRenameSource(sourcePath);
    if (!source)
        return 2;
    error = UacRenameLeaf::Detail::NativeRename(source.get(), original.get(), L"rename-\u6771\u4eac");
    Check(error == ERROR_SUCCESS && Exists(renamedPath) && !Exists(sourcePath) && !Exists(replacedPath),
          "native rename stays on the held target after directory substitution", error);
    source.reset();

    const auto collisionPath = moved + L"\\collision-source";
    if (!CreateRenameSource(collisionPath))
        return 2;
    FILE_ID_INFO sourceId{};
    FILE_ID_INFO targetId{};
    const auto pinnedSource = OpenIdentity(collisionPath, sourceId);
    const auto pinnedTarget = OpenIdentity(renamedPath, targetId);
    if (!pinnedSource || !pinnedTarget)
        return 2;
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"collision-source", L"rename-\u6771\u4eac");
    FILE_ID_INFO targetAfter{};
    Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && Exists(collisionPath) &&
              OpenIdentity(renamedPath, targetAfter) &&
              std::memcmp(targetId.FileId.Identifier, targetAfter.FileId.Identifier, sizeof(targetId.FileId.Identifier)) == 0,
          "verified rename collision preserves source and target identity", error);

    error = UacRenameLeaf::RenameVerifiedFile(substitute.get(), expected, sourceId, L"collision-source", L"wrong-parent");
    Check(error == ERROR_FILE_INVALID && !Exists(ancestor + L"\\parent\\wrong-parent"),
          "rename rejects a replacement parent", error);

    const auto movedSource = moved + L"\\held-source";
    if (!MoveFileExW(collisionPath.c_str(), movedSource.c_str(), 0) || !CreateRenameSource(collisionPath))
        return 2;
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"collision-source", L"wrong-file");
    Check(error == ERROR_FILE_INVALID && Exists(collisionPath) && Exists(movedSource) && !Exists(moved + L"\\wrong-file"),
          "rename rejects a substituted source while the original ID is pinned", error);

    const auto alias = moved + L"\\hard-link";
    if (!CreateHardLinkW(alias.c_str(), movedSource.c_str(), nullptr))
        return 2;
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"held-source", L"wrong-file");
    Check(error == ERROR_NOT_SUPPORTED && Exists(alias) && Exists(movedSource) && !Exists(moved + L"\\wrong-file"),
          "rename rejects a multiply linked source", error);
    if (!DeleteFileW(alias.c_str()))
        return 2;
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"held-source", L"HELD-SOURCE");
    Check(error == ERROR_NOT_SUPPORTED && Exists(movedSource), "case-only rename stays outside the primitive", error);
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"held-source", L"new:stream");
    Check(error == ERROR_INVALID_NAME && Exists(movedSource), "rename rejects an alternate stream name", error);
    const HANDLE writer = CreateFileW(movedSource.c_str(), FILE_WRITE_DATA,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (writer == INVALID_HANDLE_VALUE)
        return 2;
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"held-source", L"wrong-file");
    Check(error == ERROR_SHARING_VIOLATION && Exists(movedSource) && !Exists(moved + L"\\wrong-file"),
          "verified rename refuses a concurrent writer", error);
    CloseHandle(writer);
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, leaf, L"wrong-file");
    Check(error == ERROR_ACCESS_DENIED && DirectoryExists(intended) && !Exists(moved + L"\\wrong-file"),
          "the file-only primitive leaves a directory source intact", error);
    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, sourceId, L"held-source", L"verified-\u6771\u4eac");
    FILE_ID_INFO verifiedId{};
    Check(error == ERROR_SUCCESS && !Exists(movedSource) && OpenIdentity(moved + L"\\verified-\u6771\u4eac", verifiedId) &&
              std::memcmp(sourceId.FileId.Identifier, verifiedId.FileId.Identifier, sizeof(sourceId.FileId.Identifier)) == 0 &&
              !Exists(ancestor + L"\\parent\\verified-\u6771\u4eac"),
          "verified rename preserves the source identity in the held parent", error);

    const auto metadataPath = moved + L"\\verified-\u6771\u4eac";
    const auto metadataFile = OpenIdentity(metadataPath, verifiedId);
    FILE_BASIC_INFO before{};
    if (!metadataFile || !GetFileInformationByHandleEx(metadataFile.get(), FileBasicInfo, &before, sizeof(before)))
        return 2;
    UacF7::MetadataChange metadata{};
    metadata.expectedAttributes = before.FileAttributes;
    metadata.attributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
    metadata.created = 132224078450000000;
    metadata.accessed = 132224078460000000;
    metadata.modified = 132224078470000000;
    const auto setMetadata = [&](const UacF7::MetadataChange& change, const FILE_ID_INFO& id) {
        return UacMetadataLeaf::SetVerifiedMetadata(original.get(), expected, id, L"verified-\u6771\u4eac", change);
    };
    error = setMetadata(metadata, targetId);
    Check(error == ERROR_FILE_INVALID && GetFileAttributesW(metadataPath.c_str()) == before.FileAttributes,
          "metadata rejects a different file ID before making any change", error);
    auto invalidMetadata = metadata;
    invalidMetadata.modified = -1;
    error = setMetadata(invalidMetadata, verifiedId);
    Check(error == ERROR_INVALID_DATA, "metadata refuses a special negative timestamp", error);
    invalidMetadata = metadata;
    invalidMetadata.attributes = FILE_ATTRIBUTE_REPARSE_POINT;
    error = setMetadata(invalidMetadata, verifiedId);
    Check(error == ERROR_INVALID_DATA, "metadata refuses special file attributes", error);
    error = setMetadata(metadata, verifiedId);
    FILE_BASIC_INFO applied{};
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(metadataFile.get(), FileBasicInfo, &applied, sizeof(applied)) &&
              applied.FileAttributes == metadata.attributes &&
              applied.CreationTime.QuadPart == metadata.created &&
              applied.LastAccessTime.QuadPart == metadata.accessed &&
              applied.LastWriteTime.QuadPart == metadata.modified,
          "one metadata request applies exact attributes and all three times", error);
    error = setMetadata(metadata, verifiedId);
    Check(error == ERROR_FILE_INVALID, "metadata rejects stale attributes instead of replacing them", error);
    UacF7::MetadataChange timesOnly{};
    timesOnly.expectedAttributes = applied.FileAttributes;
    timesOnly.modified = metadata.modified + 10000000;
    error = setMetadata(timesOnly, verifiedId);
    FILE_BASIC_INFO updated{};
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(metadataFile.get(), FileBasicInfo, &updated, sizeof(updated)) &&
              updated.FileAttributes == applied.FileAttributes &&
              updated.CreationTime.QuadPart == applied.CreationTime.QuadPart &&
              updated.LastAccessTime.QuadPart == applied.LastAccessTime.QuadPart &&
              updated.LastWriteTime.QuadPart == timesOnly.modified,
          "modify only last write time while keeping read-only and unspecified times", error);
    UacF7::MetadataChange attrsOnly{};
    attrsOnly.expectedAttributes = updated.FileAttributes;
    attrsOnly.attributes = FILE_ATTRIBUTE_NORMAL;
    error = setMetadata(attrsOnly, verifiedId);
    FILE_BASIC_INFO cleared{};
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(metadataFile.get(), FileBasicInfo, &cleared, sizeof(cleared)) &&
              cleared.FileAttributes == FILE_ATTRIBUTE_NORMAL &&
              cleared.CreationTime.QuadPart == updated.CreationTime.QuadPart &&
              cleared.LastAccessTime.QuadPart == updated.LastAccessTime.QuadPart &&
              cleared.LastWriteTime.QuadPart == updated.LastWriteTime.QuadPart,
          "clear the basic attributes without resetting any of the three times", error);
    SetFileAttributesW(metadataPath.c_str(), FILE_ATTRIBUTE_NORMAL); // keep failed fixtures removable

    const auto deletePath = moved + L"\\delete-source";
    const auto relocatedDelete = moved + L"\\delete-original";
    if (!CreateRenameSource(deletePath))
        return 2;
    FILE_ID_INFO deleteId{};
    const auto pinnedDelete = OpenIdentity(deletePath, deleteId);
    if (!pinnedDelete || !MoveFileExW(deletePath.c_str(), relocatedDelete.c_str(), 0) || !CreateRenameSource(deletePath))
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedFile(original.get(), expected, deleteId, L"delete-source");
    Check(error == ERROR_FILE_INVALID && Exists(deletePath) && Exists(relocatedDelete),
          "delete rejects a replacement source and preserves both files", error);
    error = UacDeleteLeaf::DeleteVerifiedFile(substitute.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_FILE_INVALID && Exists(relocatedDelete), "delete rejects a replacement parent", error);
    error = UacDeleteLeaf::DeleteVerifiedFile(original.get(), expected, deleteId, L"delete-original:stream");
    Check(error == ERROR_INVALID_NAME && Exists(relocatedDelete), "delete refuses an alternate stream name", error);

    const auto deleteAlias = moved + L"\\delete-alias";
    if (!CreateHardLinkW(deleteAlias.c_str(), relocatedDelete.c_str(), nullptr))
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedFile(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_NOT_SUPPORTED && Exists(relocatedDelete) && Exists(deleteAlias),
          "delete refuses an object with another hardlink", error);
    if (!DeleteFileW(deleteAlias.c_str()))
        return 2;
    if (!SetFileAttributesW(relocatedDelete.c_str(), FILE_ATTRIBUTE_READONLY))
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedFile(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_NOT_SUPPORTED && GetFileAttributesW(relocatedDelete.c_str()) == FILE_ATTRIBUTE_READONLY,
          "delete leaves a read-only file and its attributes intact", error);
    if (!SetFileAttributesW(relocatedDelete.c_str(), FILE_ATTRIBUTE_NORMAL))
        return 2;
    const HANDLE reader = CreateFileW(relocatedDelete.c_str(), GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (reader == INVALID_HANDLE_VALUE)
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedFile(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_SHARING_VIOLATION && Exists(relocatedDelete), "delete refuses an open data reader", error);
    CloseHandle(reader);
    error = UacDeleteLeaf::DeleteVerifiedFile(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_SUCCESS && !Exists(relocatedDelete) && Exists(deletePath),
          "delete detaches only the verified name while its identity handle is still open", error);

    for (const auto invalid : {L"..", L"parent\\escaped", L"parent/escaped", L"x:ads", L"NUL.txt", L"x.", L"x "})
    {
        error = CreateVerifiedLeaf(original.get(), expected, invalid);
        Check(error == ERROR_INVALID_NAME, "reject traversal, ADS, reserved or ambiguous leaf", error);
    }
    const wchar_t embedded[] = {L'x', L'\0', L'y'};
    error = CreateVerifiedLeaf(original.get(), expected, std::wstring_view(embedded, ARRAYSIZE(embedded)));
    Check(error == ERROR_INVALID_NAME, "reject embedded NUL", error);
    error = OpenParentBelowRoot(root.get(), L"..\\escape", rawParent, replacement);
    Check(error == ERROR_INVALID_NAME && !rawParent, "reject traversal in parent path", error);

    for (const auto reparse : {L"junction", L"junction\\nested"})
    {
        error = OpenParentBelowRoot(root.get(), reparse, rawParent, replacement);
        Check(error != ERROR_SUCCESS && !rawParent, "reject final or intermediate junction", error);
        if (rawParent)
            CloseHandle(rawParent);
    }

    // Convert an existing, empty and verified parent to a junction in place.
    const auto mutablePath = fixture + L"\\mutable";
    if (!CreateDirectoryW(mutablePath.c_str(), nullptr))
        return 2;
    error = OpenParentBelowRoot(root.get(), L"mutable", rawParent, expected);
    if (error)
        return 2;
    const Handle mutableParent(rawParent);
    const auto openReparse = [](const std::wstring& path, DWORD access) {
        const HANDLE h = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                     nullptr);
        return Handle(h != INVALID_HANDLE_VALUE ? h : nullptr);
    };
    const auto donor = openReparse(fixture + L"\\junction", FILE_READ_ATTRIBUTES);
    const auto writable = openReparse(mutablePath, GENERIC_WRITE);
    std::unique_ptr<BYTE[]> tag(new BYTE[MAXIMUM_REPARSE_DATA_BUFFER_SIZE]);
    DWORD bytes = 0;
    if (!donor || !writable ||
        !DeviceIoControl(donor.get(), FSCTL_GET_REPARSE_POINT, nullptr, 0, tag.get(),
                         MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &bytes, nullptr) ||
        !DeviceIoControl(writable.get(), FSCTL_SET_REPARSE_POINT, tag.get(), bytes, nullptr, 0, &bytes, nullptr))
    {
        Check(false, "convert the already verified parent into a junction", GetLastError());
        return 1;
    }
    const auto redirect = fixture + L"\\destination\\must-not-create";
    error = CreateVerifiedLeaf(mutableParent.get(), expected, L"must-not-create");
    Check(error == ERROR_REPARSE_TAG_INVALID && !DirectoryExists(redirect),
          "reject a reparse tag added to the verified parent in place", error);
    // Model a tag added after validation but before the syscall. A root-relative
    // create must either fail or stay on the original object.
    HANDLE raced = nullptr;
    const DWORD racedResult = Detail::NativeDirectory(mutableParent.get(), L"race-leaf", FILE_CREATE, raced);
    if (racedResult == ERROR_SUCCESS)
        CloseHandle(raced);
    Check(!DirectoryExists(fixture + L"\\destination\\race-leaf"),
          "native call cannot follow a parent tag added after validation", racedResult);
    const auto raceSourcePath = fixture + L"\\rename-race-source";
    source = CreateRenameSource(raceSourcePath);
    if (!source)
        return 2;
    const DWORD renameRace = UacRenameLeaf::Detail::NativeRename(source.get(), mutableParent.get(), L"rename-race-target");
    Check(!Exists(fixture + L"\\destination\\rename-race-target"),
          "native rename cannot follow a target tag added after validation", renameRace);
    source.reset();
    struct { DWORD tag; WORD length; WORD reserved; } remove{IO_REPARSE_TAG_MOUNT_POINT, 0, 0};
    if (!DeviceIoControl(writable.get(), FSCTL_DELETE_REPARSE_POINT, &remove, sizeof(remove), nullptr, 0, &bytes, nullptr))
    {
        Check(false, "remove the test junction tag", GetLastError());
        return 1;
    }

    Check(racedResult != ERROR_SUCCESS || DirectoryExists(mutablePath + L"\\race-leaf"),
          "successful raced creation stays in the original object", racedResult);
    Check(renameRace == ERROR_SUCCESS ? Exists(mutablePath + L"\\rename-race-target") : Exists(raceSourcePath),
          "raced rename either stays in the original object or preserves the source", renameRace);

    const std::wstring first = L"long-" + std::wstring(175, L'\u65e5');
    const std::wstring second(100, L'\u672c');
    const auto longPath = fixture + L"\\" + first + L"\\" + second;
    if (!CreateDirectoryW((fixture + L"\\" + first).c_str(), nullptr) || !CreateDirectoryW(longPath.c_str(), nullptr))
        return 2;
    error = OpenParentBelowRoot(root.get(), first + L"\\" + second, rawParent, expected);
    Check(error == ERROR_SUCCESS && longPath.size() > MAX_PATH, "open long Unicode parent", error);
    if (error)
        return 1;
    Handle longParent(rawParent);
    error = CreateVerifiedLeaf(longParent.get(), expected, leaf);
    Check(error == ERROR_SUCCESS && DirectoryExists(longPath + L"\\" + leaf), "create exact Unicode leaf on a long path", error);
    const auto longSource = longPath + L"\\source-\u65e5";
    const auto longTarget = longPath + L"\\target-\u672c";
    FILE_ID_INFO longId{};
    if (!CreateRenameSource(longSource) || !OpenIdentity(longSource, longId))
        return 2;
    const DWORD longRename = UacRenameLeaf::RenameVerifiedFile(longParent.get(), expected, longId, L"source-\u65e5", L"target-\u672c");
    Check(longRename == ERROR_SUCCESS && !Exists(longSource) && Exists(longTarget), "rename exact Unicode leaf on a long path", longRename);
    UacF7::MetadataChange longChange{};
    longChange.expectedAttributes = GetFileAttributesW(longTarget.c_str());
    longChange.attributes = FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_HIDDEN;
    const DWORD longMetadata = UacMetadataLeaf::SetVerifiedMetadata(longParent.get(), expected, longId, L"target-\u672c", longChange);
    Check(longMetadata == ERROR_SUCCESS && GetFileAttributesW(longTarget.c_str()) == longChange.attributes,
          "set attributes on an exact Unicode leaf below a long path", longMetadata);
    const auto longCopyPath = longPath + L"\\copy-\u6771\u4eac";
    const DWORD longCopy = UacCopyLeaf::CopyVerifiedFile(longParent.get(), expected, longId, L"target-\u672c",
                                                        longParent.get(), expected, L"copy-\u6771\u4eac");
    Check(longCopy == ERROR_SUCCESS && HasContents(longCopyPath, {}), "copy exact Unicode leaves below a long path", longCopy);
    if (longCopy == ERROR_SUCCESS && !DeleteFileW(longCopyPath.c_str()))
        return 2;
    if (longRename == ERROR_SUCCESS && !SetFileAttributesW(longTarget.c_str(), FILE_ATTRIBUTE_ARCHIVE))
        return 2;
    const DWORD longDelete = UacDeleteLeaf::DeleteVerifiedFile(longParent.get(), expected, longId, L"target-\u672c");
    Check(longDelete == ERROR_SUCCESS && !Exists(longTarget), "delete an exact Unicode leaf below a long path", longDelete);
    const BOOL removedFile = longDelete == ERROR_SUCCESS || DeleteFileW((longRename == ERROR_SUCCESS ? longTarget : longSource).c_str());

    longParent.reset();
    // The Python runner may have a MAX_PATH-limited filesystem API.
    const BOOL removedLeaf = error == ERROR_SUCCESS ? RemoveDirectoryW((longPath + L"\\" + leaf).c_str()) : TRUE;
    const BOOL removedParent = RemoveDirectoryW(longPath.c_str());
    const BOOL removedFirst = RemoveDirectoryW((fixture + L"\\" + first).c_str());
    Check(removedFile && removedLeaf && removedParent && removedFirst, "remove only the owned long-path fixture");
    return failures ? 1 : 0;
}
