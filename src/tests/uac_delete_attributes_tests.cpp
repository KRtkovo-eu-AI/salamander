/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-11 | Exercise permanent deletion of attributed NTFS objects. | Verify atomic ReadOnly handling, no-follow links and preserved failure state. | #731 | GPT-6.1 Sol |
*/
#include "../uacbroker/deleteleaf.h"
#include <sddl.h>
#include <cstdio>
#include <string>
#include <vector>

namespace
{
struct Close
{
    void operator()(void* handle) const { if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
};
using Handle = std::unique_ptr<void, Close>;
int failures = 0, checks = 0;

void Check(bool result, const char* description, DWORD error = ERROR_SUCCESS)
{
    ++checks;
    if (!result) ++failures;
    std::printf("%s: %s (%lu)\n", result ? "PASS" : "FAIL", description, error);
}

Handle Open(const std::wstring& path, DWORD access = FILE_READ_ATTRIBUTES)
{
    HANDLE handle = CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(handle == INVALID_HANDLE_VALUE ? nullptr : handle);
}

bool Attributes(HANDLE file, DWORD& attributes)
{
    FILE_ATTRIBUTE_TAG_INFO info{};
    if (!GetFileInformationByHandleEx(file, FileAttributeTagInfo, &info, sizeof(info))) return false;
    attributes = info.FileAttributes;
    return true;
}

bool SetAttributes(HANDLE file, DWORD attributes)
{
    FILE_BASIC_INFO info{};
    info.FileAttributes = attributes;
    return SetFileInformationByHandle(file, FileBasicInfo, &info, sizeof(info)) != FALSE;
}

bool Create(const std::wstring& path, DWORD attributes, bool directory = false)
{
    if (directory && !CreateDirectoryW(path.c_str(), nullptr)) return false;
    HANDLE raw = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                             directory ? OPEN_EXISTING : CREATE_NEW,
                             FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    Handle file(raw == INVALID_HANDLE_VALUE ? nullptr : raw);
    if (!file) return false;
    if (!directory)
    {
        const char contents[] = "preserve-the-target";
        DWORD written = 0;
        if (!WriteFile(file.get(), contents, sizeof(contents), &written, nullptr) || written != sizeof(contents)) return false;
    }
    DWORD returned = 0;
    if ((attributes & FILE_ATTRIBUTE_SPARSE_FILE) &&
        !DeviceIoControl(file.get(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &returned, nullptr)) return false;
    if (attributes & FILE_ATTRIBUTE_COMPRESSED)
    {
        USHORT compression = COMPRESSION_FORMAT_LZNT1;
        if (!DeviceIoControl(file.get(), FSCTL_SET_COMPRESSION, &compression, sizeof(compression),
                             nullptr, 0, &returned, nullptr)) return false;
    }
    const DWORD writable = attributes & UacF7::basicAttributes;
    DWORD actual = 0;
    return SetAttributes(file.get(), writable ? writable : FILE_ATTRIBUTE_NORMAL) && Attributes(file.get(), actual) &&
           (actual & attributes) == attributes && ((actual & FILE_ATTRIBUTE_DIRECTORY) != 0) == directory;
}

bool SetDacl(const std::wstring& path, const wchar_t* sddl)
{
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &descriptor, nullptr)) return false;
    const BOOL result = SetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, descriptor);
    LocalFree(descriptor);
    return result != FALSE;
}

bool SameAttributes(HANDLE file, DWORD expected)
{
    DWORD actual = 0;
    return Attributes(file, actual) && actual == expected;
}

bool CreateJunction(const std::wstring& path, const std::wstring& target)
{
    if (!CreateDirectoryW(path.c_str(), nullptr)) return false;
    const auto directory = Open(path, GENERIC_WRITE);
    if (!directory) return false;
    const auto substitute = L"\\??\\" + target.substr(4); // fixture paths use the Win32 extended prefix
    const size_t bytes = sizeof(UacRenameLeaf::Detail::MountPointHeader) + (substitute.size() + 2) * sizeof(wchar_t);
    std::vector<BYTE> buffer(bytes);
    UacRenameLeaf::Detail::MountPointHeader header{};
    header.tag = IO_REPARSE_TAG_MOUNT_POINT;
    header.dataLength = static_cast<USHORT>(bytes - offsetof(decltype(header), substituteOffset));
    header.substituteLength = static_cast<USHORT>(substitute.size() * sizeof(wchar_t));
    header.printOffset = header.substituteLength + sizeof(wchar_t);
    std::memcpy(buffer.data(), &header, sizeof(header));
    std::memcpy(buffer.data() + sizeof(header), substitute.data(), header.substituteLength);
    DWORD returned = 0;
    return DeviceIoControl(directory.get(), FSCTL_SET_REPARSE_POINT, buffer.data(), static_cast<DWORD>(bytes),
                           nullptr, 0, &returned, nullptr) != FALSE;
}

bool Exists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }

bool Unlink(HANDLE parent, const UacDirectoryLeaf::ParentIdentity& parentId, const std::wstring& path,
            const std::wstring& leaf, UacF7::Operation operation, const char* description)
{
    const auto pin = Open(path);
    FILE_ID_INFO identity{};
    DWORD original = 0, after = 0;
    if (!pin || !Attributes(pin.get(), original) ||
        UacRenameLeaf::Detail::ReadSourceIdentity(pin.get(), identity, UacDeleteLeaf::SourceKind(operation))) return false;
    const DWORD error = UacDeleteLeaf::DeleteVerifiedLeaf(parent, parentId, identity, leaf, operation);
    Check(error == ERROR_SUCCESS && !Exists(path) && Attributes(pin.get(), after) && after == original,
          description, error);
    return error == ERROR_SUCCESS;
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root = argv[1];
    auto parent = Open(root, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES);
    UacDirectoryLeaf::ParentIdentity parentId;
    if (!parent || UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentId)) return 2;

    constexpr DWORD common = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY;
    const DWORD fileAttributes[] = {FILE_ATTRIBUTE_NORMAL, FILE_ATTRIBUTE_HIDDEN, FILE_ATTRIBUTE_SYSTEM,
        FILE_ATTRIBUTE_READONLY, common | FILE_ATTRIBUTE_ARCHIVE,
        FILE_ATTRIBUTE_SPARSE_FILE | common, FILE_ATTRIBUTE_COMPRESSED | common};
    unsigned index = 0;
    for (DWORD attributes : fileAttributes)
    {
        const auto leaf = L"file-" + std::to_wstring(++index) + L"-東";
        const auto path = root + L"\\" + leaf;
        if (!Create(path, attributes) || !Unlink(parent.get(), parentId, path, leaf, UacF7::Operation::Delete,
            "delete the exact attributed file without changing its attributes")) return 2;
    }
    for (DWORD attributes : {common, common | FILE_ATTRIBUTE_COMPRESSED})
    {
        const auto leaf = L"directory-" + std::to_wstring(++index) + L"-日";
        const auto path = root + L"\\" + leaf;
        if (!Create(path, attributes, true) || !Unlink(parent.get(), parentId, path, leaf,
            UacF7::Operation::DeleteDirectory, "delete an empty attributed directory without clearing ReadOnly")) return 2;
    }

    const auto negative = root + L"\\negative-東";
    if (!Create(negative, common)) return 2;
    auto negativePin = Open(negative);
    FILE_ID_INFO identity{};
    DWORD original = 0;
    if (!negativePin || !Attributes(negativePin.get(), original) ||
        UacRenameLeaf::Detail::ReadSourceIdentity(negativePin.get(), identity)) return 2;
    FILE_ID_INFO stale = identity;
    stale.FileId.Identifier[0] ^= 1;
    DWORD error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), parentId, stale, L"negative-東");
    Check(error == ERROR_FILE_INVALID && Exists(negative) && SameAttributes(negativePin.get(), original),
          "stale identity leaves all ReadOnly, Hidden and System attributes intact", error);
    auto wrongParent = parentId;
    wrongParent.file.FileId.Identifier[0] ^= 1;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), wrongParent, identity, L"negative-東");
    Check(error == ERROR_FILE_INVALID && Exists(negative) && SameAttributes(negativePin.get(), original),
          "wrong parent cannot change the attributed object", error);
    error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), parentId, identity, L"negative-東:stream");
    Check(error == ERROR_INVALID_NAME && Exists(negative) && SameAttributes(negativePin.get(), original),
          "an alternate stream name never becomes a delete target", error);
    {
        const auto reader = Open(negative, FILE_READ_DATA | FILE_READ_ATTRIBUTES);
        if (!reader) return 2;
        error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), parentId, identity, L"negative-東");
        Check(error == ERROR_SHARING_VIOLATION && Exists(negative) && SameAttributes(negativePin.get(), original),
              "an active data reader leaves the attributed file unchanged", error);
    }
    const auto alias = root + L"\\hardlink";
    if (!CreateHardLinkW(alias.c_str(), negative.c_str(), nullptr) || !Attributes(negativePin.get(), original)) return 2;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), parentId, identity, L"negative-東");
    Check(error == ERROR_NOT_SUPPORTED && Exists(alias) && Exists(negative) && SameAttributes(negativePin.get(), original),
          "multiple hard links retain the original exclusion", error);
    if (!SetFileAttributesW(negative.c_str(), FILE_ATTRIBUTE_NORMAL) || !DeleteFileW(alias.c_str()) ||
        !SetFileAttributesW(negative.c_str(), original)) return 2;

    // The first DELETE-only open succeeds. The scoped ReadOnly reopen must fail without mutation.
    if (!SetDacl(negative, L"D:P(D;;0x100;;;WD)(A;;FA;;;WD)") || !Attributes(negativePin.get(), original)) return 2;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), parentId, identity, L"negative-東");
    Check(error == ERROR_ACCESS_DENIED && Exists(negative) && SameAttributes(negativePin.get(), original),
          "denied WRITE_ATTRIBUTES preserves ReadOnly instead of clearing it", error);
    if (!SetDacl(negative, L"D:P(A;;FA;;;WD)")) return 2;
    if (!Unlink(parent.get(), parentId, negative, L"negative-東", UacF7::Operation::Delete,
                "retry after a failed ReadOnly open removes only the original identity")) return 2;

    const auto deleteOnly = root + L"\\delete-only";
    if (!Create(deleteOnly, FILE_ATTRIBUTE_HIDDEN) ||
        !SetDacl(deleteOnly, L"D:P(D;;0x101;;;WD)(A;;FA;;;WD)")) return 2;
    const auto deniedRead = Open(deleteOnly, FILE_READ_DATA);
    const DWORD readError = GetLastError();
    Check(!deniedRead && readError == ERROR_ACCESS_DENIED, "fixture denies data reads and attribute writes", readError);
    if (!Unlink(parent.get(), parentId, deleteOnly, L"delete-only", UacF7::Operation::Delete,
                "ordinary deletion needs neither data access nor WRITE_ATTRIBUTES")) return 2;

    const auto directory = root + L"\\nonempty";
    if (!Create(directory, common, true) || !Create(directory + L"\\child", FILE_ATTRIBUTE_NORMAL)) return 2;
    auto directoryPin = Open(directory);
    if (!directoryPin || UacRenameLeaf::Detail::ReadSourceIdentity(directoryPin.get(), identity,
        UacRenameLeaf::MoveKind::RenameDirectory) || !Attributes(directoryPin.get(), original)) return 2;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(parent.get(), parentId, identity, L"nonempty", UacF7::Operation::DeleteDirectory);
    Check(error == ERROR_DIR_NOT_EMPTY && Exists(directory + L"\\child") && GetFileAttributesW(directory.c_str()) == original,
          "nonempty directory refusal preserves its child and attributes", error);

    const auto target = root + L"\\target";
    const DWORD targetAttributes = GetFileAttributesW(target.c_str());
    const auto junction = root + L"\\junction";
    if (!CreateJunction(junction, target)) return 2;
    {
        const auto writable = Open(junction, FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES);
        if (!writable || !SetAttributes(writable.get(), common)) return 2;
    }
    if (!Unlink(parent.get(), parentId, junction, L"junction", UacF7::Operation::DeleteJunction,
                "detach a ReadOnly Hidden System junction without following it")) return 2;
    Check(Exists(target + L"\\sentinel") && GetFileAttributesW(target.c_str()) == targetAttributes,
          "junction target and its metadata survive unlink");
    for (bool isDirectory : {false, true})
    {
        const auto leaf = isDirectory ? L"directory-link" : L"file-link";
        const auto path = root + L"\\" + leaf;
        const auto linked = isDirectory ? target : target + L"\\sentinel";
        const DWORD linkedAttributes = GetFileAttributesW(linked.c_str());
        const DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE | (isDirectory ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0);
        if (!CreateSymbolicLinkW(path.c_str(), linked.c_str(), flags))
        {
            if (GetLastError() != ERROR_PRIVILEGE_NOT_HELD) return 2;
            std::printf("SKIP: symbolic-link fixture requires Developer Mode or SeCreateSymbolicLinkPrivilege\n");
            continue;
        }
        {
            const auto writable = Open(path, FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES);
            if (!writable || !SetAttributes(writable.get(), common)) return 2;
        }
        if (!Unlink(parent.get(), parentId, path, leaf,
            isDirectory ? UacF7::Operation::DeleteDirectorySymlink : UacF7::Operation::DeleteSymlink,
            "delete only the attributed symbolic link")) return 2;
        Check(Exists(linked) && GetFileAttributesW(linked.c_str()) == linkedAttributes,
              "the symbolic link target and its attributes remain");
    }

    const auto copied = root + L"\\copied-source";
    const auto destination = root + L"\\copied-target";
    if (!Create(copied, FILE_ATTRIBUTE_READONLY) || !Create(destination, FILE_ATTRIBUTE_NORMAL)) return 2;
    FILE_ID_INFO sourceId{}, targetId{};
    const auto sourcePin = Open(copied), targetPin = Open(destination);
    if (!sourcePin || !targetPin || UacRenameLeaf::Detail::ReadSourceIdentity(sourcePin.get(), sourceId) ||
        UacRenameLeaf::Detail::ReadSourceIdentity(targetPin.get(), targetId)) return 2;
    error = UacDeleteLeaf::DeleteCopiedSource(copied, destination, sourceId, targetId);
    Check(error == ERROR_NOT_SUPPORTED && Exists(copied) && Exists(destination),
          "copied-source retirement retains its strict original contract", error);

    Check(!UacF7::IsEligibleDeleteFile(FILE_ATTRIBUTE_REPARSE_POINT) &&
              !UacF7::IsEligibleDeleteFile(FILE_ATTRIBUTE_OFFLINE) && !UacF7::IsEligibleDeleteFile(FILE_ATTRIBUTE_DEVICE) &&
              !UacF7::IsEligibleDeleteLink(FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_OFFLINE) &&
              !UacF7::IsEligibleDeleteDirectory(FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT),
          "unknown, offline and mismatched object types stay ineligible");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
