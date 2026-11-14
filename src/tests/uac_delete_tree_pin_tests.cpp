// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Probe anchored directory pins for a protected delete tree. | Require physical NTFS evidence before broadening a destructive scope. | #731 | GPT-6.1 Terra |
#include <windows.h>
#include <winioctl.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "../uacbroker/directoryleaf.h"

using Handle = std::unique_ptr<void, decltype(&CloseHandle)>;
static int checks = 0, failures = 0;
static void Check(bool ok, const char* text)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s: %s\n", ok ? "PASS" : "FAIL", text);
}

static Handle OpenDirectory(const std::wstring& name, DWORD access, DWORD sharing)
{
    HANDLE raw = CreateFileW(name.c_str(), access, sharing, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(raw == INVALID_HANDLE_VALUE ? nullptr : raw, CloseHandle);
}

static DWORD PinChild(HANDLE parent, const wchar_t* name, Handle& result, bool deleting = false,
                      DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE)
{
    HANDLE raw = nullptr;
    const DWORD error = UacDirectoryLeaf::Detail::NativeDirectory(parent, name, FILE_OPEN, raw,
        FILE_LIST_DIRECTORY | (deleting ? DELETE : 0), nullptr, sharing, 0, false);
    if (!error)
        result.reset(raw);
    return error;
}

static DWORD SetTestJunction(HANDLE file, const std::wstring& destination)
{
    struct MountHeader
    {
        DWORD tag;
        USHORT dataLength, reserved, substituteOffset, substituteLength, printOffset, printLength;
    };
    const std::wstring substitute = L"\\??\\" + destination;
    const size_t substituteBytes = substitute.size() * sizeof(wchar_t);
    const size_t printBytes = destination.size() * sizeof(wchar_t);
    std::vector<BYTE> buffer(sizeof(MountHeader) + substituteBytes + sizeof(wchar_t) + printBytes + sizeof(wchar_t));
    auto* header = reinterpret_cast<MountHeader*>(buffer.data());
    header->tag = IO_REPARSE_TAG_MOUNT_POINT;
    header->dataLength = static_cast<USHORT>(buffer.size() - 8);
    header->substituteLength = static_cast<USHORT>(substituteBytes);
    header->printOffset = static_cast<USHORT>(substituteBytes + sizeof(wchar_t));
    header->printLength = static_cast<USHORT>(printBytes);
    auto* path = buffer.data() + sizeof(MountHeader);
    std::memcpy(path, substitute.data(), substituteBytes);
    std::memcpy(path + header->printOffset, destination.data(), printBytes);
    DWORD returned = 0;
    return DeviceIoControl(file, FSCTL_SET_REPARSE_POINT, buffer.data(), static_cast<DWORD>(buffer.size()),
                           nullptr, 0, &returned, nullptr) ? ERROR_SUCCESS : GetLastError();
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring rootPath = argv[1];
    const std::wstring ancestorPath = rootPath + L"\\ancestor";
    const std::wstring childPath = ancestorPath + L"\\child-\u6771";
    const std::wstring movedAncestor = rootPath + L"\\moved-ancestor";
    const std::wstring movedChild = rootPath + L"\\moved-child";
    if (!CreateDirectoryW(ancestorPath.c_str(), nullptr) || !CreateDirectoryW(childPath.c_str(), nullptr))
        return 2;
    const DWORD all = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    auto root = OpenDirectory(rootPath, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!root) return 2;
    Handle ancestor(nullptr, CloseHandle), child(nullptr, CloseHandle);
    Check(PinChild(root.get(), L"ancestor", ancestor) == ERROR_SUCCESS && ancestor,
          "open the actual ancestor with directory-list access and no delete sharing");
    Check(ancestor && PinChild(ancestor.get(), L"child-\u6771", child, true) == ERROR_SUCCESS && child,
          "open the selected child with list and delete access relative to its pinned ancestor");
    UacDirectoryLeaf::ParentIdentity rootId{}, ancestorId{}, childId{};
    Check(!UacDirectoryLeaf::ReadParentIdentity(root.get(), rootId) &&
          ancestor && !UacDirectoryLeaf::ReadParentIdentity(ancestor.get(), ancestorId) &&
          child && !UacDirectoryLeaf::ReadParentIdentity(child.get(), childId) &&
          rootId.file.VolumeSerialNumber == ancestorId.file.VolumeSerialNumber &&
          ancestorId.file.VolumeSerialNumber == childId.file.VolumeSerialNumber,
          "all pinned components are non-reparse directories on one NTFS volume");
    Check(!MoveFileW(ancestorPath.c_str(), movedAncestor.c_str()) && GetLastError() == ERROR_SHARING_VIOLATION,
          "a pinned ancestor cannot be renamed out of the selected path");
    Check(!MoveFileW(childPath.c_str(), movedChild.c_str()) && GetLastError() == ERROR_SHARING_VIOLATION,
          "a pinned child cannot be renamed out of the selected root");
    FILE_DISPOSITION_INFO disposition{TRUE};
    Check(child && SetFileInformationByHandle(child.get(), FileDispositionInfo, &disposition, sizeof(disposition)),
          "mark the same pinned empty child for deletion without reopening by name");
    child.reset();
    Check(GetFileAttributesW(childPath.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND,
          "closing the pinned delete handle removes only the verified child");
    Check(CreateDirectoryW(childPath.c_str(), nullptr) != FALSE,
          "prepare a new owned child for the ancestor pin checks");
    ancestor.reset();
    auto priorDelete = OpenDirectory(ancestorPath, DELETE | FILE_READ_ATTRIBUTES, all);
    Check(priorDelete != nullptr, "open a DELETE-capable handle before attempting the ancestor pin");
    const DWORD blocked = PinChild(root.get(), L"ancestor", ancestor);
    Check(priorDelete && blocked == ERROR_SHARING_VIOLATION && !ancestor,
          "a pre-existing DELETE handle prevents the proposed ancestor pin");
    priorDelete.reset();
    auto priorWriter = OpenDirectory(ancestorPath, FILE_ADD_FILE | FILE_READ_ATTRIBUTES, all);
    Check(priorWriter != nullptr, "open a writable directory handle before attempting a read-only-sharing pin");
    const DWORD writerBlocked = PinChild(root.get(), L"ancestor", ancestor, true, FILE_SHARE_READ);
    Check(priorWriter && writerBlocked == ERROR_SHARING_VIOLATION && !ancestor,
          "a pre-existing directory data writer prevents a no-write-sharing pin");
    priorWriter.reset();
    Check(PinChild(root.get(), L"ancestor", ancestor, true, FILE_SHARE_READ) == ERROR_SUCCESS && ancestor,
          "the selected directory can be pinned without write sharing after the data writer closes");
    Check(RemoveDirectoryW(childPath.c_str()) != FALSE,
          "an existing child can be removed while its parent denies write sharing");
    const BOOL addedWhilePinned = CreateDirectoryW(childPath.c_str(), nullptr);
    const DWORD addError = addedWhilePinned ? ERROR_SUCCESS : GetLastError();
    std::printf("PROBE: add child while parent pinned=%lu\n", addError);
    if (!addedWhilePinned)
    {
        ancestor.reset();
        Check(CreateDirectoryW(childPath.c_str(), nullptr) != FALSE || GetLastError() == ERROR_ALREADY_EXISTS,
              "prepare child for subsequent finalization checks");
    }
    ancestor.reset();
    Check(PinChild(root.get(), L"ancestor", ancestor, true) == ERROR_SUCCESS && ancestor,
          "the same ancestor can be pinned for deletion after the earlier DELETE handle closes");
    Check(!SetFileInformationByHandle(ancestor.get(), FileDispositionInfo, &disposition, sizeof(disposition)) &&
          GetLastError() == ERROR_DIR_NOT_EMPTY, "the pinned ancestor cannot be finalized while a child remains");
    Check(RemoveDirectoryW(childPath.c_str()) != FALSE,
          "a child can be removed while its selected parent remains pinned");
    Check(SetFileInformationByHandle(ancestor.get(), FileDispositionInfo, &disposition, sizeof(disposition)) != FALSE,
          "the same pinned ancestor can be finalized only after it is empty");
    ancestor.reset();
    Check(GetFileAttributesW(ancestorPath.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND,
          "closing the verified ancestor handle removes the selected empty directory");

    const std::wstring aliasPath = rootPath + L"\\alias";
    const std::wstring outsidePath = rootPath + L"\\outside";
    const std::wstring outsideChild = outsidePath + L"\\inside";
    Check(CreateDirectoryW(aliasPath.c_str(), nullptr) && CreateDirectoryW(outsidePath.c_str(), nullptr) &&
          CreateDirectoryW(outsideChild.c_str(), nullptr), "prepare an owned junction-swap fixture");
    auto priorAttributes = OpenDirectory(aliasPath, FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES, all);
    Check(priorAttributes != nullptr, "open a metadata writer before pinning an empty directory");
    Handle alias(nullptr, CloseHandle);
    const DWORD aliasPin = PinChild(root.get(), L"alias", alias, true, FILE_SHARE_READ);
    std::printf("PROBE: metadata-only prior handle pin=%lu\n", aliasPin);
    if (!aliasPin && priorAttributes && alias)
    {
        FILE_BASIC_INFO basic{};
        FILE_ATTRIBUTE_TAG_INFO changed{};
        if (GetFileInformationByHandleEx(priorAttributes.get(), FileBasicInfo, &basic, sizeof(basic)))
        {
            const auto original = basic;
            basic.FileAttributes |= FILE_ATTRIBUTE_HIDDEN;
            const BOOL changedHidden = SetFileInformationByHandle(priorAttributes.get(), FileBasicInfo, &basic, sizeof(basic));
            std::printf("PROBE: change hidden attribute via the prior metadata handle=%lu\n",
                        changedHidden ? ERROR_SUCCESS : GetLastError());
            if (changedHidden)
            {
                Check(GetFileInformationByHandleEx(alias.get(), FileAttributeTagInfo, &changed, sizeof(changed)) &&
                      (changed.FileAttributes & FILE_ATTRIBUTE_HIDDEN),
                      "observe a hidden attribute added after the directory pin");
                Check(SetFileInformationByHandle(priorAttributes.get(), FileBasicInfo,
                                                 const_cast<FILE_BASIC_INFO*>(&original), sizeof(original)) != FALSE,
                      "restore the owned test directory attributes");
            }
            else
                std::printf("SKIP: this environment refused the owned hidden-attribute probe\n");
        }
        else
            std::printf("SKIP: this environment refused to read the owned directory attributes (%lu)\n", GetLastError());
        const DWORD junction = SetTestJunction(priorAttributes.get(), outsidePath);
        std::printf("PROBE: set junction through the prior metadata handle=%lu\n", junction);
        if (!junction)
        {
            FILE_ATTRIBUTE_TAG_INFO attributes{};
            Check(GetFileInformationByHandleEx(alias.get(), FileAttributeTagInfo, &attributes, sizeof(attributes)) &&
                  (attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT),
                  "observe the in-place junction on the same pinned directory");
            Handle aliasLeaf(nullptr, CloseHandle);
            const DWORD leafOpen = PinChild(root.get(), L"alias", aliasLeaf, false, all);
            std::printf("PROBE: no-follow open of the junction leaf=%lu\n", leafOpen);
            if (!leafOpen && aliasLeaf)
            {
                UacDirectoryLeaf::ParentIdentity nonReparse{};
                Check(UacDirectoryLeaf::ReadParentIdentity(aliasLeaf.get(), nonReparse) != ERROR_SUCCESS,
                      "a junction leaf cannot be accepted as a verified ordinary directory");
                aliasLeaf.reset();
            }
            Handle escaped(nullptr, CloseHandle);
            const DWORD opened = PinChild(alias.get(), L"inside", escaped);
            std::printf("PROBE: relative no-follow open below the converted junction=%lu\n", opened);
            Check(opened != ERROR_SUCCESS && !escaped,
                  "a no-follow relative open does not cross the in-place junction");
            const BOOL marked = SetFileInformationByHandle(alias.get(), FileDispositionInfo, &disposition, sizeof(disposition));
            const DWORD markError = marked ? ERROR_SUCCESS : GetLastError();
            std::printf("PROBE: dispose converted pinned directory=%lu\n", markError);
            Check(GetFileAttributesW(outsideChild.c_str()) != INVALID_FILE_ATTRIBUTES,
                  "disposition on a converted pinned directory leaves the junction target intact");
            if (escaped)
            {
                UacDirectoryLeaf::ParentIdentity actual{}, outside{};
                auto outsideHandle = OpenDirectory(outsideChild, FILE_READ_ATTRIBUTES, all);
                if (outsideHandle && !UacDirectoryLeaf::ReadParentIdentity(escaped.get(), actual) &&
                    !UacDirectoryLeaf::ReadParentIdentity(outsideHandle.get(), outside))
                    std::printf("PROBE: relative open reached outside identity=%d\n",
                                UacDirectoryLeaf::SameParent(actual, outside) ? 1 : 0);
            }
        }
        else
            std::printf("SKIP: this environment refused the owned junction-conversion probe (%lu)\n", junction);
    }
    else
        std::printf("SKIP: a pre-existing metadata handle could not coexist with the pin (%lu)\n", aliasPin);
    alias.reset();
    priorAttributes.reset();
    const DWORD aliasAttributes = GetFileAttributesW(aliasPath.c_str());
    const bool aliasGone = aliasAttributes == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
    Check((aliasGone || RemoveDirectoryW(aliasPath.c_str())) && RemoveDirectoryW(outsideChild.c_str()) &&
          RemoveDirectoryW(outsidePath.c_str()), "remove only the owned junction-swap fixture and referent");

    const std::wstring stalePath = rootPath + L"\\stale";
    const std::wstring displacedPath = rootPath + L"\\displaced";
    Check(CreateDirectoryW(stalePath.c_str(), nullptr) != FALSE,
          "prepare an owned directory for the identity-substitution probe");
    Handle before(nullptr, CloseHandle);
    UacDirectoryLeaf::ParentIdentity beforeId{}, replacementId{};
    const DWORD beforeError = PinChild(root.get(), L"stale", before);
    const DWORD idError = before ? UacDirectoryLeaf::ReadParentIdentity(before.get(), beforeId) : ERROR_INVALID_HANDLE;
    before.reset();
    Check(!beforeError && !idError && MoveFileW(stalePath.c_str(), displacedPath.c_str()) &&
          CreateDirectoryW(stalePath.c_str(), nullptr),
          "replace the unpinned directory after recording its original identity");
    Handle replacement(nullptr, CloseHandle);
    const DWORD replacementError = PinChild(root.get(), L"stale", replacement);
    Check(!replacementError && replacement &&
          !UacDirectoryLeaf::ReadParentIdentity(replacement.get(), replacementId) &&
          !UacDirectoryLeaf::SameParent(beforeId, replacementId),
          "a same-name replacement does not match the previously recorded identity");
    replacement.reset();
    Check(RemoveDirectoryW(stalePath.c_str()) && RemoveDirectoryW(displacedPath.c_str()),
          "remove only the owned identity-substitution fixture");
    root.reset();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
