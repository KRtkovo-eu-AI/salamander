/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-07 | Exercise the basic metadata executor on real NTFS files. | Verify independent fields, stale snapshots, and preservation of the verified source. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Probe NTFS native rename with a directory handle. | Verify the target cannot be redirected before adding broker rename. | | GPT-6.1 Sol (Codex) |
| 2026-11-07 | Test anchored mkdir against real NTFS substitutions. | Verify that a replacement path and reparse points cannot redirect a creation. | | GPT-6.1 Sol (Codex) |
*/
#include "../uacbroker/metadataleaf.h"
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
}

int wmain(int argc, wchar_t** argv)
{
    using namespace UacDirectoryLeaf;
    if (argc != 2)
        return 2;
    const std::wstring fixture = L"\\\\?\\" + std::wstring(argv[1]);
    const HANDLE rawRoot = CreateFileW(fixture.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                        nullptr);
    if (rawRoot == INVALID_HANDLE_VALUE)
        return 2;
    const Handle root(rawRoot);

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
    const BOOL removedFile = DeleteFileW((longRename == ERROR_SUCCESS ? longTarget : longSource).c_str());

    longParent.reset();
    // The Python runner may have a MAX_PATH-limited filesystem API.
    const BOOL removedLeaf = error == ERROR_SUCCESS ? RemoveDirectoryW((longPath + L"\\" + leaf).c_str()) : TRUE;
    const BOOL removedParent = RemoveDirectoryW(longPath.c_str());
    const BOOL removedFirst = RemoveDirectoryW((fixture + L"\\" + first).c_str());
    Check(removedFile && removedLeaf && removedParent && removedFirst, "remove only the owned long-path fixture");
    return failures ? 1 : 0;
}
