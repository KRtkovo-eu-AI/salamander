// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Clarify scope and NTFS probe test coverage. | Keep direct checks distinct from pipe and UI acceptance. | #731 | GPT-6 (Codex) |
// | 2026-11-11 | Test the proposed one-shot overwrite probe scope. | Reject changed identities, revisions and replay before broker integration. | #731 | GPT-6.1 Sol |
// This directly tests PendingOverwriteProbe and the NTFS metadata probe.
// It does not exercise the pipe, dispatcher, elevation or UI.

#define SAM_UAC_BROKER
#define SAM_UAC_BROKER_JOB_TEST
#define wmain InstalledBrokerMain
#include "uacbroker.cpp"
#undef wmain

#include <cstdio>

namespace
{
int failures = 0;
int checks = 0;

void Check(bool value, const char* label)
{
    ++checks;
    if (!value)
    {
        ++failures;
        std::printf("FAIL: %s\n", label);
    }
}

PendingOverwriteProbe ValidProbe()
{
    PendingOverwriteProbe p;
    p.present = true;
    p.operation = Operation::CopyOverwrite;
    p.nonce.Data1 = 42;
    p.nonce.Data4[7] = 13;
    p.sourceParentSerial = 10;
    p.targetParentSerial = 20;
    p.sourceParent.Identifier[0] = 1;
    p.sourceParent.Identifier[15] = 2;
    p.targetParent.Identifier[0] = 3;
    p.targetParent.Identifier[15] = 4;
    p.source.serial = 10;
    p.source.fileId[0] = 5;
    p.source.fileId[15] = 6;
    p.source.size = 123;
    p.source.created = 101;
    p.source.modified = 102;
    p.source.changed = 103;
    p.source.attributes = FILE_ATTRIBUTE_ARCHIVE;
    p.target.serial = 20;
    p.target.fileId[0] = 7;
    p.target.fileId[15] = 8;
    p.target.size = 456;
    p.target.created = 201;
    p.target.modified = 202;
    p.target.changed = 203;
    p.target.attributes = FILE_ATTRIBUTE_ARCHIVE;
    p.sourceVolume = L"\\\\?\\Volume{source}\\";
    p.sourceRelative = L"parent\\branch";
    p.sourceLeaf = L"source-\u6771.txt";
    p.targetVolume = L"\\\\?\\Volume{target}\\";
    p.targetRelative = L"destination\\branch";
    p.targetLeaf = L"target-\u65e5.txt";
    return p;
}

template <typename Mutate>
void RejectChanged(const char* label, Mutate mutate)
{
    auto pending = ValidProbe();
    auto request = pending;
    mutate(request);
    Check(!pending.Consume(request), label);
    Check(!pending.present, "mismatch consumes pending authorization");
    Check(!pending.Consume(ValidProbe()), "mismatch cannot be retried");
}

struct ProbeDirectory
{
    UacDirectoryLeaf::ParentIdentity identity;
    std::wstring path, relative;

    bool Open(const std::wstring& name)
    {
        const auto file = Adopt(CreateFileW(name.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        if (!file || UacDirectoryLeaf::ReadParentIdentity(file.get(), identity)) return false;
        constexpr DWORD flags = VOLUME_NAME_GUID | FILE_NAME_OPENED;
        const DWORD capacity = GetFinalPathNameByHandleW(file.get(), nullptr, 0, flags);
        if (!capacity) return false;
        path.resize(capacity);
        const DWORD length = GetFinalPathNameByHandleW(file.get(), path.data(), capacity, flags);
        if (length <= identity.volume.size() || length >= capacity) return false;
        path.resize(length);
        relative = path.substr(identity.volume.size() + 1);
        return true;
    }
};

bool WriteByte(const std::wstring& path, BYTE value)
{
    const auto file = Adopt(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr));
    DWORD written = 0;
    return file && WriteFile(file.get(), &value, 1, &written, nullptr) && written == 1;
}

bool ReadByte(const std::wstring& path, BYTE expected)
{
    const auto file = Adopt(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
    BYTE value = 0;
    DWORD bytes = 0;
    return file && ReadFile(file.get(), &value, 1, &bytes, nullptr) && bytes == 1 && value == expected;
}

void TestRealProbe()
{
    wchar_t temp[MAX_PATH]{};
    GUID guid{};
    WCHAR text[39]{};
    if (!GetTempPathW(ARRAYSIZE(temp), temp) || FAILED(CoCreateGuid(&guid)) ||
        !StringFromGUID2(guid, text, ARRAYSIZE(text)))
    {
        Check(false, "create unique probe fixture name");
        return;
    }
    const std::wstring root = std::wstring(temp) + L"UacOverwriteProbe-" + text;
    const std::wstring src = root + L"\\source", dst = root + L"\\target", moved = root + L"\\source-moved";
    const std::wstring source = src + L"\\source-\u6771.txt", target = dst + L"\\target-\u65e5.txt";
    if (!CreateDirectoryW(root.c_str(), nullptr)) { Check(false, "create unique probe fixture"); return; }
    if (!CreateDirectoryW(src.c_str(), nullptr) || !CreateDirectoryW(dst.c_str(), nullptr) ||
        !WriteByte(source, 19) || !WriteByte(target, 27))
    {
        Check(false, "create probe source and target");
    }
    else
    {
        ProbeDirectory s, t;
        FILE_ID_INFO sourceId{};
        UacF7::ArchiveRevision sourceBefore{}, targetBefore{};
        auto sourceHandle = Adopt(CreateFileW(source.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        auto targetHandle = Adopt(CreateFileW(target.c_str(), FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
        if (!s.Open(src) || !t.Open(dst) || !sourceHandle || !targetHandle ||
            UacRenameLeaf::Detail::ReadSourceIdentity(sourceHandle.get(), sourceId) ||
            UacZipUpdate::ReadRevision(sourceHandle.get(), sourceBefore) ||
            UacZipUpdate::ReadRevision(targetHandle.get(), targetBefore))
        {
            Check(false, "capture initial NTFS probe fixture identities");
        }
        else
        {
            auto capture = [&](const UacDirectoryLeaf::ParentIdentity& si, const FILE_ID_INFO& id,
                               const UacDirectoryLeaf::ParentIdentity& ti, std::wstring_view targetLeaf,
                               PendingOverwriteProbe& out) {
                return CaptureOverwriteProbe(Operation::CopyOverwrite, si, s.relative, L"source-\u6771.txt", id, 1,
                                             ti, t.relative, targetLeaf, out);
            };
            PendingOverwriteProbe out{};
            DWORD status = capture(s.identity, sourceId, t.identity, L"target-\u65e5.txt", out);
            Check(status == ERROR_SUCCESS && out.present && !IsEqualGUID(out.nonce, GUID_NULL) &&
                  UacF7::SameFileRevision(out.source, sourceBefore) && UacF7::SameFileRevision(out.target, targetBefore),
                  "real capture returns both full revisions and nonce");
            UacF7::ArchiveRevision sourceAfter{}, targetAfter{};
            Check(!UacZipUpdate::ReadRevision(sourceHandle.get(), sourceAfter) &&
                  !UacZipUpdate::ReadRevision(targetHandle.get(), targetAfter) &&
                  UacF7::SameFileRevision(sourceBefore, sourceAfter) && UacF7::SameFileRevision(targetBefore, targetAfter) &&
                  ReadByte(source, 19) && ReadByte(target, 27), "capture preserves both files and their revisions");
            sourceHandle.reset();
            targetHandle.reset();

            auto badId = sourceId;
            badId.FileId.Identifier[15] ^= 1;
            Check(capture(s.identity, badId, t.identity, L"target-\u65e5.txt", out) == ERROR_FILE_INVALID && !out.present,
                  "capture rejects different source ID");
            auto badParent = s.identity;
            badParent.file.FileId.Identifier[15] ^= 1;
            Check(capture(badParent, sourceId, t.identity, L"target-\u65e5.txt", out) == ERROR_FILE_INVALID && !out.present,
                  "capture rejects different source parent");
            badParent = t.identity;
            badParent.file.FileId.Identifier[15] ^= 1;
            Check(capture(s.identity, sourceId, badParent, L"target-\u65e5.txt", out) == ERROR_FILE_INVALID && !out.present,
                  "capture rejects different target parent");

            // Keep a preopened WRITE_DAC handle to this one owned fixture so
            // restoration does not depend on the temporary restricted DACL.
            const auto control = Adopt(CreateFileW(target.c_str(), READ_CONTROL | WRITE_DAC,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
            PSECURITY_DESCRIPTOR savedRaw = nullptr, restrictedRaw = nullptr;
            PACL savedAcl = nullptr, restrictedAcl = nullptr;
            SECURITY_DESCRIPTOR_CONTROL savedControl{};
            DWORD securityRevision = 0;
            BOOL present = FALSE, defaulted = FALSE;
            const bool securityReady = control &&
                !GetSecurityInfo(control.get(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr,
                                 &savedAcl, nullptr, &savedRaw) &&
                GetSecurityDescriptorControl(savedRaw, &savedControl, &securityRevision) &&
                ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(D;;0x1;;;WD)(A;;FA;;;WD)",
                    SDDL_REVISION_1, &restrictedRaw, nullptr) &&
                GetSecurityDescriptorDacl(restrictedRaw, &present, &restrictedAcl, &defaulted) && present;
            const std::unique_ptr<void, decltype(&LocalFree)> savedSecurity(savedRaw, LocalFree), restricted(restrictedRaw, LocalFree);
            Check(securityReady, "capture original DACL and create read-data denial fixture");
            if (securityReady)
            {
                const DWORD apply = SetSecurityInfo(control.get(), SE_FILE_OBJECT,
                    DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, restrictedAcl, nullptr);
                DWORD readError = ERROR_SUCCESS, metadataError = ERROR_SUCCESS, probeError = ERROR_GEN_FAILURE;
                bool validProbe = false;
                if (!apply)
                {
                    const auto denied = Adopt(CreateFileW(target.c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
                    if (!denied) readError = GetLastError();
                    const auto metadata = Adopt(CreateFileW(target.c_str(), FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                        FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
                    if (!metadata) metadataError = GetLastError();
                    UacF7::ArchiveRevision restrictedRevision{};
                    if (metadata && !UacZipUpdate::ReadRevision(metadata.get(), restrictedRevision))
                    {
                        probeError = capture(s.identity, sourceId, t.identity, L"target-\u65e5.txt", out);
                        validProbe = !probeError && out.present && UacF7::SameFileRevision(out.target, restrictedRevision);
                    }
                }
                const DWORD restore = apply ? ERROR_SUCCESS : SetSecurityInfo(control.get(), SE_FILE_OBJECT,
                    DACL_SECURITY_INFORMATION | ((savedControl & SE_DACL_PROTECTED) ?
                        PROTECTED_DACL_SECURITY_INFORMATION : UNPROTECTED_DACL_SECURITY_INFORMATION),
                    nullptr, nullptr, savedAcl, nullptr);
                PSECURITY_DESCRIPTOR checkedRaw = nullptr;
                PACL checkedAcl = nullptr;
                SECURITY_DESCRIPTOR_CONTROL checkedControl{};
                DWORD checkedRevision = 0;
                const bool restoredExact = !apply && !restore &&
                    !GetSecurityInfo(control.get(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                     nullptr, nullptr, &checkedAcl, nullptr, &checkedRaw) &&
                    GetSecurityDescriptorControl(checkedRaw, &checkedControl, &checkedRevision) &&
                    ((savedControl ^ checkedControl) & SE_DACL_PROTECTED) == 0 &&
                    ((!savedAcl && !checkedAcl) || (savedAcl && checkedAcl && savedAcl->AclSize == checkedAcl->AclSize &&
                        !std::memcmp(savedAcl, checkedAcl, savedAcl->AclSize)));
                const std::unique_ptr<void, decltype(&LocalFree)> checkedSecurity(checkedRaw, LocalFree);
                if (apply || restore || readError != ERROR_ACCESS_DENIED || metadataError || !validProbe)
                    std::printf("DACL probe: apply=%lu read=%lu metadata=%lu probe=%lu restored=%lu\n",
                                apply, readError, metadataError, probeError, restore);
                Check(!apply && readError == ERROR_ACCESS_DENIED && !metadataError && validProbe,
                      "probe succeeds on metadata-only target while READ_DATA is denied");
                Check(restoredExact, "restore the exact original target DACL and inheritance protection");
                Check(!restore && ReadByte(target, 27), "restored target data remains unchanged and readable");
            }

            const std::wstring hardlink = dst + L"\\hardlink";
            if (CreateHardLinkW(hardlink.c_str(), source.c_str(), nullptr))
            {
                status = capture(s.identity, sourceId, t.identity, L"hardlink", out);
                Check(status == ERROR_NOT_SUPPORTED && !out.present,
                      "capture rejects a multiply linked source through the existing identity guard");
                Check(DeleteFileW(hardlink.c_str()) != FALSE, "clean up hardlink fixture");
            }
            else
            {
                const DWORD error = GetLastError();
                if (error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION)
                    std::printf("SKIP: filesystem does not support hardlinks (error %lu)\n", error);
                else Check(false, "create hardlink fixture");
            }

            const std::wstring link = dst + L"\\symlink";
            if (CreateSymbolicLinkW(link.c_str(), source.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
            {
                Check(capture(s.identity, sourceId, t.identity, L"symlink", out) != ERROR_SUCCESS && !out.present &&
                      ReadByte(source, 19), "capture rejects target symlink without following it");
                Check(DeleteFileW(link.c_str()) != FALSE, "clean up symlink fixture");
            }
            else
            {
                const DWORD error = GetLastError();
                if (error == ERROR_PRIVILEGE_NOT_HELD || error == ERROR_NOT_SUPPORTED || error == ERROR_INVALID_FUNCTION)
                    std::printf("SKIP: symlink privilege or filesystem support unavailable (error %lu)\n", error);
                else Check(false, "create symlink fixture");
            }

            const BOOL movedOut = MoveFileW(src.c_str(), moved.c_str());
            const DWORD moveError = movedOut ? ERROR_SUCCESS : GetLastError();
            const BOOL movedBack = movedOut ? MoveFileW(moved.c_str(), src.c_str()) : FALSE;
            if (!movedOut || !movedBack) std::printf("parent move error: %lu, restore error: %lu\n", moveError, GetLastError());
            Check(movedOut && movedBack, "probe releases source parent handle before returning");
        }
    }
    Check(DeleteFileW(source.c_str()) != FALSE, "clean up source fixture");
    Check(DeleteFileW(target.c_str()) != FALSE, "clean up target fixture");
    Check(RemoveDirectoryW(src.c_str()) != FALSE && RemoveDirectoryW(dst.c_str()) != FALSE &&
          RemoveDirectoryW(root.c_str()) != FALSE, "clean up unique probe directories");
}
}

int main()
{
    auto pending = ValidProbe();
    const auto original = pending;
    Check(pending.Consume(original), "exact proposal is accepted once");
    Check(!pending.present && !pending.Consume(original), "successful proposal cannot be replayed");

    pending = ValidProbe();
    pending.operation = Operation::CopyOverwriteIgnoreAds;
    Check(pending.Consume(pending), "IgnoreAds is accepted for its own proposal");

    RejectChanged("absent request", [](auto& p) { p.present = false; });
    RejectChanged("different operation", [](auto& p) { p.operation = Operation::CopyOverwriteIgnoreAds; });
    RejectChanged("other operation", [](auto& p) { p.operation = Operation::MoveOverwrite; });
    RejectChanged("different nonce", [](auto& p) { p.nonce.Data4[7] ^= 1; });
    RejectChanged("null nonce", [](auto& p) { p.nonce = {}; });
    RejectChanged("source volume", [](auto& p) { p.sourceVolume += L"x"; });
    RejectChanged("source relative", [](auto& p) { p.sourceRelative += L"x"; });
    RejectChanged("source leaf", [](auto& p) { p.sourceLeaf += L"x"; });
    RejectChanged("target volume", [](auto& p) { p.targetVolume += L"x"; });
    RejectChanged("target relative", [](auto& p) { p.targetRelative += L"x"; });
    RejectChanged("target leaf", [](auto& p) { p.targetLeaf += L"x"; });
    RejectChanged("source parent serial", [](auto& p) { ++p.sourceParentSerial; });
    RejectChanged("source parent final ID byte", [](auto& p) { p.sourceParent.Identifier[15] ^= 1; });
    RejectChanged("target parent serial", [](auto& p) { ++p.targetParentSerial; });
    RejectChanged("target parent final ID byte", [](auto& p) { p.targetParent.Identifier[15] ^= 1; });
    RejectChanged("source revision serial", [](auto& p) { ++p.source.serial; });
    RejectChanged("source revision final ID byte", [](auto& p) { p.source.fileId[15] ^= 1; });
    RejectChanged("source revision size", [](auto& p) { ++p.source.size; });
    RejectChanged("source revision creation time", [](auto& p) { ++p.source.created; });
    RejectChanged("source revision write time", [](auto& p) { ++p.source.modified; });
    RejectChanged("source revision change time", [](auto& p) { ++p.source.changed; });
    RejectChanged("source revision attributes", [](auto& p) { p.source.attributes ^= FILE_ATTRIBUTE_HIDDEN; });
    RejectChanged("source revision reserved", [](auto& p) { p.source.reserved = 1; });
    RejectChanged("target revision serial", [](auto& p) { ++p.target.serial; });
    RejectChanged("target revision final ID byte", [](auto& p) { p.target.fileId[15] ^= 1; });
    RejectChanged("target revision size", [](auto& p) { ++p.target.size; });
    RejectChanged("target revision creation time", [](auto& p) { ++p.target.created; });
    RejectChanged("target revision write time", [](auto& p) { ++p.target.modified; });
    RejectChanged("target revision change time", [](auto& p) { ++p.target.changed; });
    RejectChanged("target revision attributes", [](auto& p) { p.target.attributes ^= FILE_ATTRIBUTE_HIDDEN; });
    RejectChanged("target revision reserved", [](auto& p) { p.target.reserved = 1; });

    auto invalid = ValidProbe();
    invalid.nonce = {};
    Check(!invalid.Consume(invalid) && !invalid.present, "a pending null nonce cannot authorize a commit");
    invalid = ValidProbe();
    invalid.source.size = -1;
    Check(!invalid.Consume(invalid) && !invalid.present, "negative size cannot authorize a commit");
    invalid = ValidProbe();
    invalid.sourceParent = {};
    Check(!invalid.Consume(invalid) && !invalid.present, "zero parent ID cannot authorize a commit");
    invalid = ValidProbe();
    invalid.target.fileId[0] = invalid.source.fileId[0];
    invalid.target.fileId[15] = invalid.source.fileId[15];
    invalid.target.serial = invalid.source.serial;
    invalid.targetParentSerial = invalid.sourceParentSerial;
    Check(!invalid.Consume(invalid) && !invalid.present, "same source and target identity is rejected");

    TestRealProbe();
    std::printf("overwrite scope and NTFS probe only: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
