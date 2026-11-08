/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-08 | Exercise the verified EFS transfer end to end. | Validate the path shared with the elevated broker. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify EFS publication and inherited destination permissions. | Exercise cancellation, no-replace collision and workspace cleanup. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise EFS workspace recovery on NTFS. | Retain the original link and reject substitutions in live and deferred cleanup. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise bounded EFS Raw export and import. | Verify ciphertext preservation and measure the source-handle sharing boundary. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Check the exact-file Find protocol boundary. | Reject path selection and directory operations on the pinned file. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise the read-only Find property stream. | Check bounded reads, large offsets, cancellation and clone lifetime. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Cover scoped content opens used by Find. | Check long Unicode paths, ranges beyond snapshots and scope escapes. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Probe read-only and delete-pending source guards. | Exercise new and already open NTFS streams before designing retirement. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise the shared file exchange on NTFS. | Verify deadline and backup-name collisions leave the original intact. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Test ZIP journal cleanup through the named archive. | Keep coverage of preserving the original on an interrupted write. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise protected ZIP selection output on NTFS. | Verify nested names, empty directories and all-or-nothing manifest validation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Test restoring a missing ZIP by selecting its parent. | Match the recovery command after application restart. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise ZIP publication with a pinned original. | Match the broker-owned handle used throughout staging. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Adapt ZIP publication tests to the fixed archive revision. | Exercise the same metadata as the broker protocol. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Add ZIP publication and recovery cases to the native fixture. | Cover verified commit, a stale revision and recovery with a competing file. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify bounded archive ranges and opening. | Cover ZIP64 offsets without weakening viewer limits. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Check detailed protected inventory against ordinary NTFS size queries. | Cover empty, normal, compressed and sparse file sizes. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise read-only viewer snapshots on NTFS. | Verify exact bytes, empty files, identity checks and the size bound. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Check EFS copy error classification. | Keep EFS failures distinct from ordinary access and sharing errors. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise the protected security-copy option. | Verify exact owner, group and ACL and refuse unsupported inheritance. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise permanent junction deletion on real NTFS. | Verify type and identity checks and preserve the target. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise verified permanent directory symbolic-link deletion. | Check type, identity, dangling links and target preservation. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify permanent deletion of a file symbolic link. | Check the typed identity guard and preserve the linked file. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify junction rename and mount-point rejection. | Exercise real NTFS junction data and malformed or unsupported substitute names. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise verified directory symbolic-link rename. | Check type, collision, parent binding, and unchanged reparse data. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Check the scoped rename of an NTFS file symbolic link. | Verify the exact object and preserve the stored link target. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise explicit NTFS compression preservation. | Verify content, exact attributes, compression format, and ADS refusal. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise sparse F5 staging with real NTFS I/O. | Check contents, allocation gaps, and rejection of named streams. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise verified preservation of named NTFS streams. | Check exact data and refusal when staging protection is unavailable. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify that explicit Ignore ADS leaves the original streams intact. | Check the copied data and preserve the strict default. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify basic attributes and times on the same directory handle. | Cover setting and clearing flags without changing descendants. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Check creation and last-write times on the published directory tree. | Cover both the selected root and a nested Unicode directory. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Verify timestamp-only directory metadata. | Cover Unicode, stale identities and reparse rejection. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Probe stream creation before source retirement. | Test the precondition for deleting the source after copying. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Probe how NTFS replacement interacts with target handles. | Establish the identity boundary before adding protected overwrite. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise tree cancellation received through a pipe. | Distinguish the valid control byte from unrelated data. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Test exclusive journal recovery on NTFS. | Reap an owned orphan and preserve unrelated or substituted directories. | | GPT-6.1 Sol (Codex) |
| 2026-11-08 | Exercise mandatory integrity on staged tree objects. | Check Windows access decisions and preserve destination DACL inheritance. | | GPT-6.1 Sol (Codex) |
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
#include "../uacbroker/zipupdate.h"
#include "../uacbroker/zipselection.h"
#include "../uacbroker/copytree.h"
#include "../uacbroker/viewleaf.h"
#include "../findtagstream.h"
#include "../uacbroker/efsraw.h"
#include "../uacbroker/efscopy.h"
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

Handle OpenIdentity(const std::wstring& path, FILE_ID_INFO& identity, bool directory = false)
{
    const HANDLE file = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                    OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT |
                                                       (directory ? FILE_FLAG_BACKUP_SEMANTICS : 0), nullptr);
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

bool ReadDacl(const std::wstring& path, PACL& acl, PSECURITY_DESCRIPTOR& descriptor)
{
    return GetNamedSecurityInfoW(path.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION, nullptr, nullptr, &acl,
                                 nullptr, &descriptor) == ERROR_SUCCESS && acl;
}

bool SameInheritedDacl(const std::wstring& reference, const std::wstring& copied)
{
    PACL first = nullptr, second = nullptr;
    PSECURITY_DESCRIPTOR rawFirst = nullptr, rawSecond = nullptr;
    const bool haveFirst = ReadDacl(reference, first, rawFirst);
    const std::unique_ptr<void, decltype(&LocalFree)> firstDescriptor(rawFirst, LocalFree);
    const bool haveSecond = ReadDacl(copied, second, rawSecond);
    const std::unique_ptr<void, decltype(&LocalFree)> secondDescriptor(rawSecond, LocalFree);
    if (!haveFirst || !haveSecond || first->AceCount != second->AceCount)
        return false;
    SECURITY_DESCRIPTOR_CONTROL firstFlags{}, secondFlags{};
    DWORD revision = 0;
    if (!GetSecurityDescriptorControl(rawFirst, &firstFlags, &revision) ||
        !GetSecurityDescriptorControl(rawSecond, &secondFlags, &revision) ||
        ((firstFlags ^ secondFlags) & (SE_DACL_PROTECTED | SE_DACL_AUTO_INHERITED)))
        return false;
    bool inherited = false;
    for (DWORD i = 0; i < first->AceCount; ++i)
    {
        void* a = nullptr;
        void* b = nullptr;
        if (!GetAce(first, i, &a) || !GetAce(second, i, &b))
            return false;
        const auto* firstAce = static_cast<const ACE_HEADER*>(a);
        const auto* secondAce = static_cast<const ACE_HEADER*>(b);
        if (firstAce->AceSize != secondAce->AceSize || std::memcmp(a, b, firstAce->AceSize) != 0)
            return false;
        inherited |= (firstAce->AceFlags & INHERITED_ACE) != 0;
    }
    return inherited;
}

Handle LowerIntegrityToken(bool high)
{
    HANDLE raw = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &raw))
        return {};
    const Handle processToken(raw);
    if (high)
    {
        DWORD shellPid = 0;
        GetWindowThreadProcessId(GetShellWindow(), &shellPid);
        const Handle shell(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, shellPid));
        if (!shell || !OpenProcessToken(shell.get(), TOKEN_QUERY | TOKEN_DUPLICATE, &raw))
        {
            Check(false, "open the interactive shell token", GetLastError());
            return {};
        }
        const Handle filtered(raw);
        struct UserBuffer
        {
            TOKEN_USER user;
            BYTE sid[SECURITY_MAX_SID_SIZE];
        } elevated{}, medium{};
        struct LabelBuffer
        {
            TOKEN_MANDATORY_LABEL label;
            BYTE sid[SECURITY_MAX_SID_SIZE];
        } integrity{};
        TOKEN_ELEVATION elevation{};
        DWORD size = 0;
        if (!GetTokenInformation(processToken.get(), TokenUser, &elevated, sizeof(elevated), &size) ||
            !GetTokenInformation(filtered.get(), TokenUser, &medium, sizeof(medium), &size) ||
            !GetTokenInformation(filtered.get(), TokenIntegrityLevel, &integrity, sizeof(integrity), &size) ||
            !GetTokenInformation(filtered.get(), TokenElevation, &elevation, sizeof(elevation), &size))
        {
            Check(false, "read the interactive shell token identity and integrity", GetLastError());
            return {};
        }
        const PSID label = integrity.label.Label.Sid;
        if (!EqualSid(elevated.user.User.Sid, medium.user.User.Sid) || elevation.TokenIsElevated ||
            *GetSidSubAuthorityCount(label) != 1 || *GetSidSubAuthority(label, 0) != SECURITY_MANDATORY_MEDIUM_RID)
        {
            Check(false, "the interactive shell must be unelevated at medium integrity under the same user");
            return {};
        }
        if (!DuplicateTokenEx(filtered.get(), TOKEN_QUERY | TOKEN_IMPERSONATE, nullptr, SecurityImpersonation,
                             TokenImpersonation, &raw))
        {
            Check(false, "duplicate the interactive shell token", GetLastError());
            return {};
        }
        return Handle(raw);
    }
    if (!DuplicateTokenEx(processToken.get(), TOKEN_QUERY | TOKEN_IMPERSONATE | TOKEN_ADJUST_DEFAULT, nullptr,
                         SecurityImpersonation, TokenImpersonation, &raw))
        return {};
    Handle lower(raw);
    alignas(DWORD) BYTE sid[SECURITY_MAX_SID_SIZE]{};
    DWORD size = sizeof(sid);
    if (!CreateWellKnownSid(WinLowLabelSid, nullptr, sid, &size))
        return {};
    TOKEN_MANDATORY_LABEL label{{sid, SE_GROUP_INTEGRITY}};
    if (!SetTokenInformation(lower.get(), TokenIntegrityLevel, &label, sizeof(label) + size))
        return {};
    return lower;
}

DWORD ProbeAccess(const std::wstring& path, DWORD desired, DWORD flags)
{
    const HANDLE raw = CreateFileW(path.c_str(), desired, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   nullptr, OPEN_EXISTING, flags, nullptr);
    if (raw == INVALID_HANDLE_VALUE)
        return GetLastError();
    CloseHandle(raw);
    return ERROR_SUCCESS;
}

bool CheckStagingProtection(HANDLE target, const std::wstring& sourcePath,
                            UacCopyTree::Detail::StageProtection& protection, bool high)
{
    using namespace UacCopyTree::Detail;
    const DWORD protectionError = protection.Prepare(target);
    if (protectionError)
    {
        Check(false, "prepare the protection probe", protectionError);
        return false;
    }
    UacListing::SubtreeManifest manifest;
    Stage stage(manifest);
    const DWORD stageError = stage.Create(target, protection);
    if (stageError)
    {
        Check(false, "create the protected staging probe", stageError);
        return false;
    }
    std::wstring root(32768, L'\0');
    const DWORD length = GetFinalPathNameByHandleW(stage.root.get(), root.data(), static_cast<DWORD>(root.size()), FILE_NAME_OPENED);
    if (!length || length >= root.size())
    {
        Check(false, "read the staging probe path", GetLastError());
        return false;
    }
    root.resize(length);
    std::unique_ptr<void, decltype(&CloseHandle)> child(nullptr, CloseHandle);
    const DWORD childError = OpenDirectory(stage.root.get(), L"child", child, READ_CONTROL | DELETE, FILE_CREATE,
                                            protection.Directory());
    if (childError)
    {
        Check(false, "create the nested staging probe", childError);
        return false;
    }
    const HANDLE raw = CreateFileW(sourcePath.c_str(), FILE_READ_DATA | FILE_READ_EA | FILE_READ_ATTRIBUTES, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (raw == INVALID_HANDLE_VALUE)
    {
        Check(false, "open the staging probe source", GetLastError());
        return false;
    }
    const ::Handle source(raw);
    FILE_BASIC_INFO basic{};
    const DWORD copyError = UacCopyLeaf::Detail::CopyOpenedFile(source.get(), child.get(), L"file.bin", nullptr, nullptr,
                                                                 MAXULONGLONG, 6, basic, nullptr, &protection);
    if (copyError)
    {
        Check(false, "copy a file into the staging probe", copyError);
        return false;
    }
    const auto file = root + L"\\child\\file.bin";
    const auto nested = root + L"\\child";
    const ::Handle low = LowerIntegrityToken(high);
    if (!low)
        return false;
    if (!SetThreadToken(nullptr, low.get()))
    {
        Check(false, "impersonate the lower-integrity token", GetLastError());
        return false;
    }
    const DWORD listing = ProbeAccess(root, FILE_LIST_DIRECTORY, FILE_FLAG_BACKUP_SEMANTICS);
    const DWORD childWrite = ProbeAccess(nested, FILE_ADD_FILE, FILE_FLAG_BACKUP_SEMANTICS);
    const DWORD fileRead = ProbeAccess(file, GENERIC_READ, 0);
    const DWORD fileWrite = ProbeAccess(file, GENERIC_WRITE, 0);
    const DWORD fileAcl = ProbeAccess(file, WRITE_DAC, 0);
    const DWORD deleteRoot = ProbeAccess(root, DELETE, FILE_FLAG_BACKUP_SEMANTICS);
    if (!RevertToSelf())
        return false;
    Check(listing == ERROR_ACCESS_DENIED && childWrite == ERROR_ACCESS_DENIED && deleteRoot == ERROR_ACCESS_DENIED,
          "lower-integrity token cannot list, add entries to, or delete the staging tree", childWrite);
    Check(fileRead == ERROR_ACCESS_DENIED && fileWrite == ERROR_ACCESS_DENIED && fileAcl == ERROR_ACCESS_DENIED,
          "lower-integrity token cannot read, write, or change the staged file DACL", fileAcl);
    if (!DeleteFileW(file.c_str()) || Remove(child.get()))
        return false;
    child.reset();
    return stage.Rollback() == ERROR_SUCCESS;
}

bool CheckTreeCopy(const std::wstring& fixture, HANDLE parent, bool high)
{
    using namespace UacDirectoryLeaf;
    const std::wstring source = fixture + L"\\tree-source";
    const std::wstring target = fixture + L"\\tree-target";
    const std::wstring nested = L"\\a-\u76ee\u9332";
    const std::wstring file = nested + L"\\\u65e5.bin";
    const std::wstring longChild = nested + L"\\" + std::wstring(170, L'\u76ee');
    const std::wstring longLeaf = longChild + L"\\" + std::wstring(150, L'\u7dd1') + L".bin";
    const std::wstring published = target + L"\\published";
    PSECURITY_DESCRIPTOR rawDescriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(high ? L"S:(ML;OICI;NWNR;;;HI)" : L"S:(ML;OICI;NWNR;;;ME)", SDDL_REVISION_1,
                                                               &rawDescriptor, nullptr))
        return false;
    const std::unique_ptr<void, decltype(&LocalFree)> targetDescriptor(rawDescriptor, LocalFree);
    SECURITY_ATTRIBUTES security{sizeof(security), rawDescriptor, FALSE};
    if (!CreateDirectoryW(source.c_str(), nullptr) || !CreateDirectoryW(target.c_str(), &security) ||
        !CreateDirectoryW((source + nested).c_str(), nullptr) ||
        !CreateDirectoryW((source + longChild).c_str(), nullptr) ||
        !CreateDirectoryW((source + L"\\empty").c_str(), nullptr) ||
        !WriteFixture(source + file, {4, 8, 15, 16, 23, 42}) ||
        !WriteFixture(source + longLeaf, {9}))
        return false;

    const FILETIME creation{0, 0x01D90000};
    const FILETIME rootModification{0, 0x01DA0000};
    const FILETIME nestedModification{0, 0x01DB0000};
    const auto setDirectoryTimes = [&](const std::wstring& path, const FILETIME& modification) {
        const HANDLE raw = CreateFileW(path.c_str(), FILE_WRITE_ATTRIBUTES, 0, nullptr,
                                       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        const Handle directory(raw);
        return SetFileTime(directory.get(), &creation, nullptr, &modification) != FALSE;
    };
    if (!setDirectoryTimes(source, rootModification) ||
        !setDirectoryTimes(source + nested, nestedModification))
        return false;

    HANDLE rawSource = nullptr, rawTarget = nullptr;
    ParentIdentity sourceId, targetId;
    if (OpenParentBelowRoot(parent, L"tree-source", rawSource, sourceId, FILE_LIST_DIRECTORY | FILE_READ_EA))
        return false;
    const Handle from(rawSource);
    if (OpenParentBelowRoot(parent, L"tree-target", rawTarget, targetId, FILE_LIST_DIRECTORY | READ_CONTROL))
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

    const auto journalPath = fixture + L"\\recovery.dat";
    const HANDLE rawJournal = CreateFileW(journalPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr);
    if (rawJournal == INVALID_HANDLE_VALUE)
        return false;
    const Handle journal(rawJournal);
    Check(ProbeAccess(journalPath, GENERIC_READ | GENERIC_WRITE, 0) == ERROR_SHARING_VIOLATION,
          "the active copy holds its recovery journal exclusively");
    const auto copyTree = [high, &journal](HANDLE from, const ParentIdentity& fromId, HANDLE to, const ParentIdentity& toId,
                             std::wstring_view name, HANDLE cancel = nullptr, HANDLE pipe = nullptr,
                             ULONGLONG deadline = MAXULONGLONG) {
        if (high)
            return UacCopyTree::CopyVerifiedTree(from, fromId, to, toId, name, cancel, pipe, deadline, journal.get());
        UacCopyTree::Detail::StageProtection protection(false);
        return UacCopyTree::Detail::CopyVerifiedTree(from, fromId, to, toId, name, protection, cancel, pipe, deadline, journal.get());
    };
    auto result = copyTree(from.get(), sourceId, to.get(), targetId, L"published");
    Check(!result.error && !result.cleanupError && noStaging() &&
              HasContents(published + file, {4, 8, 15, 16, 23, 42}) && DirectoryExists(published + L"\\empty") &&
              HasContents(source + file, {4, 8, 15, 16, 23, 42}) && HasContents(published + longLeaf, {9}),
          "publish a complete Unicode tree with an empty directory and preserve the source", result.error);
    const auto hasDirectoryTimes = [&](const std::wstring& path, const FILETIME& modification) {
        WIN32_FILE_ATTRIBUTE_DATA data{};
        return GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data) &&
               CompareFileTime(&data.ftCreationTime, &creation) == 0 &&
               CompareFileTime(&data.ftLastWriteTime, &modification) == 0;
    };
    Check(hasDirectoryTimes(published, rootModification),
          "preserve creation and last-write times of the published directory root");
    Check(hasDirectoryTimes(published + nested, nestedModification),
          "preserve creation and last-write times of a nested Unicode directory");
    const auto reference = target + L"\\reference";
    if (!CreateDirectoryW(reference.c_str(), nullptr) || !CreateDirectoryW((reference + nested).c_str(), nullptr) ||
        !WriteFixture(reference + file, {0}))
        return false;
    Check(SameInheritedDacl(reference, published) && SameInheritedDacl(reference + nested, published + nested) &&
              SameInheritedDacl(reference + file, published + file),
          "published directories and files retain the DACLs of ordinary destination children");
    UacCopyTree::Detail::StageProtection protection(high);
    const HANDLE rawReference = CreateFileW(reference.c_str(), READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                          nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (rawReference == INVALID_HANDLE_VALUE)
        return false;
    const Handle referenceDirectory(rawReference);
    const DWORD referenceLabel = protection.Check(referenceDirectory.get(), true);
    Check(referenceLabel == ERROR_SUCCESS, "ordinary destination directories inherit the same integrity policy", referenceLabel);
    const HANDLE rawReferenceFile = CreateFileW((reference + file).c_str(), READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                              nullptr, OPEN_EXISTING, 0, nullptr);
    if (rawReferenceFile == INVALID_HANDLE_VALUE)
    {
        Check(false, "open the ordinary reference file for label inspection", GetLastError());
        return false;
    }
    const Handle referenceFile(rawReferenceFile);
    const DWORD referenceFileLabel = protection.Check(referenceFile.get(), false);
    Check(referenceFileLabel == ERROR_SUCCESS, "ordinary destination files inherit the same integrity policy", referenceFileLabel);
    if (!CheckStagingProtection(to.get(), source + file, protection, high))
        return false;
    if (!high)
    {
        result = UacCopyTree::CopyVerifiedTree(from.get(), sourceId, to.get(), targetId, L"wrong-profile");
        Check(result.error == ERROR_NOT_SUPPORTED && !result.cleanupError && noStaging() &&
                  !Exists(target + L"\\wrong-profile"),
              "the production profile rejects a medium-integrity destination before staging", result.error);
    }

    result = copyTree(from.get(), sourceId, to.get(), targetId, L"published");
    Check((result.error == ERROR_ALREADY_EXISTS || result.error == ERROR_FILE_EXISTS) && !result.cleanupError &&
              noStaging() && HasContents(published + file, {4, 8, 15, 16, 23, 42}),
          "roll back the staged tree and preserve the existing destination on collision", result.error);

    const auto adsFile = source + L"\\z-unsupported.bin";
    if (!WriteFixture(adsFile, {0}) || !WriteFixture(adsFile + L":stream", {5}))
        return false;
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"unsupported");
    Check(result.error == ERROR_NOT_SUPPORTED && !result.cleanupError && noStaging() &&
              !Exists(target + L"\\unsupported") && HasContents(adsFile + L":stream", {5}),
          "unsupported file streams abort after staging earlier entries and remove the temporary tree", result.error);
    if (!DeleteFileW(adsFile.c_str()) || !WriteFixture(source + nested + L":stream", {7}))
        return false;
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"directory-stream");
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
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"oversized");
    Check(result.error == ERROR_FILE_TOO_LARGE && !result.cleanupError && noStaging(),
          "reject a tree above the total byte budget", result.error);
    if (!DeleteFileW(oversized.c_str()))
        return false;

    result = copyTree(from.get(), sourceId, to.get(), sourceId, L"wrong-parent");
    Check(result.error == ERROR_FILE_INVALID && !result.cleanupError && noStaging(),
          "reject a substituted destination parent before staging", result.error);
    result = copyTree(from.get(), targetId, to.get(), targetId, L"wrong-source");
    Check(result.error == ERROR_FILE_INVALID && !result.cleanupError && noStaging(),
          "reject a substituted source root before staging", result.error);
    result = copyTree(from.get(), sourceId, from.get(), sourceId, L"inside");
    Check(result.error == ERROR_INVALID_PARAMETER && !Exists(source + L"\\inside"),
          "reject copying a tree into itself", result.error);
    HANDLE rawNested = nullptr;
    ParentIdentity nestedId;
    if (OpenParentBelowRoot(from.get(), std::wstring_view(nested).substr(1), rawNested, nestedId))
        return false;
    const Handle descendant(rawNested);
    result = copyTree(from.get(), sourceId, descendant.get(), nestedId, L"inside");
    Check(result.error == ERROR_INVALID_PARAMETER && !Exists(source + nested + L"\\inside"),
          "reject a destination below the source root by identity", result.error);
    const Handle cancel(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (!cancel)
        return false;
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"cancelled", cancel.get());
    Check(result.error == ERROR_CANCELLED && !result.cleanupError && noStaging() && !Exists(target + L"\\cancelled"),
          "honor a signalled cancellation before staging a tree", result.error);
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"timed-out", nullptr, nullptr, GetTickCount64());
    Check(result.error == ERROR_TIMEOUT && !result.cleanupError && noStaging(),
          "honor an expired tree copy deadline before staging", result.error);
    HANDLE rawRead = nullptr, rawWrite = nullptr;
    if (!CreatePipe(&rawRead, &rawWrite, nullptr, 0))
        return false;
    const Handle read(rawRead);
    CloseHandle(rawWrite);
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"disconnected", nullptr, read.get());
    Check(result.error == ERROR_BROKEN_PIPE && !result.cleanupError && noStaging(),
          "refuse to stage a tree after the peer disconnects", result.error);

    for (const BYTE signal : {UacRenameLeaf::Detail::treeCancelSignal, BYTE{0}})
    {
        if (!CreatePipe(&rawRead, &rawWrite, nullptr, 0))
            return false;
        const Handle cancelRead(rawRead), cancelWrite(rawWrite);
        DWORD written = 0;
        if (!WriteFile(cancelWrite.get(), &signal, sizeof(signal), &written, nullptr) || written != sizeof(signal))
            return false;
        const DWORD expected = signal == UacRenameLeaf::Detail::treeCancelSignal ? ERROR_CANCELLED : ERROR_INVALID_DATA;
        result = copyTree(from.get(), sourceId, to.get(), targetId, L"pipe-cancel", nullptr, cancelRead.get());
        Check(result.error == expected && !result.cleanupError && noStaging() && !Exists(target + L"\\pipe-cancel"),
              "accept only the defined tree cancel signal before publication", result.error);
    }

    using UacCopyTree::Detail::Stage;
    using UacCopyTree::Detail::RecoveryRecord;
    UacListing::SubtreeManifest emptyManifest;
    if (protection.Prepare(to.get()))
        return false;
    {
        UacCopyTree::Detail::Handle empty(nullptr, CloseHandle);
        const DWORD error = UacCopyTree::Detail::OpenDirectory(to.get(), L"unrecorded-empty", empty, DELETE,
            FILE_CREATE, protection.Directory(), FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_DELETE_ON_CLOSE);
        if (error)
            return false;
    }
    Check(!Exists(target + L"\\unrecorded-empty"), "an empty stage is removed before its journal is committed");
    const auto orphan = [&]() -> std::wstring {
        Stage stage(emptyManifest);
        const DWORD error = stage.Create(to.get(), protection, journal.get(), targetId.file);
        if (error)
        {
            Check(false, "create a journaled stage", error);
            return {};
        }
        RecoveryRecord record;
        DWORD bytes = 0;
        if (!SetFilePointerEx(journal.get(), {}, nullptr, FILE_BEGIN) ||
            !ReadFile(journal.get(), &record, sizeof(record), &bytes, nullptr) || bytes != sizeof(record))
            return {};
        const auto path = target + L"\\" + UacCopyTree::Detail::StageName(record.nonce);
        if (!CreateDirectoryW((path + L"\\nested").c_str(), nullptr) ||
            !WriteFixture(path + L"\\nested\\first.txt", {1, 2, 3}))
            return {};
        stage.root.reset(); // Leave the disk state of a process that died after journaling.
        return path;
    };
    const auto abandoned = orphan();
    const auto unrelated = target + L"\\.samandarin-tree-{11111111-1111-1111-1111-111111111111}";
    if (abandoned.empty() || !CreateDirectoryW(unrelated.c_str(), nullptr) ||
        !WriteFixture(unrelated + L"\\keep.bin", {7}))
        return false;
    HANDLE rawPublished = nullptr;
    ParentIdentity publishedId;
    if (OpenParentBelowRoot(to.get(), L"published", rawPublished, publishedId, FILE_LIST_DIRECTORY | FILE_READ_EA))
        return false;
    const Handle publishedSource(rawPublished);
    result = copyTree(publishedSource.get(), publishedId, to.get(), targetId, L"recovered");
    Check(!result.error && !result.cleanupError && !Exists(abandoned) && HasContents(unrelated + L"\\keep.bin", {7}) &&
              HasContents(target + L"\\recovered" + file, {4, 8, 15, 16, 23, 42}) && HasContents(published + file, {4, 8, 15, 16, 23, 42}),
          "next copy reaps only the journaled orphan and preserves a published copy and a lookalike", result.error);

    const auto replaced = orphan();
    const auto original = target + L"\\held-original-stage";
    if (replaced.empty() || !MoveFileExW(replaced.c_str(), original.c_str(), 0) ||
        !CreateDirectoryW(replaced.c_str(), nullptr) || !WriteFixture(replaced + L"\\keep.bin", {8}))
        return false;
    result = copyTree(from.get(), sourceId, to.get(), targetId, L"refused");
    Check(result.error == ERROR_FILE_INVALID && !Exists(target + L"\\refused") &&
              HasContents(replaced + L"\\keep.bin", {8}) && HasContents(original + L"\\nested\\first.txt", {1, 2, 3}),
          "a substituted stage identity blocks cleanup and preserves both trees", result.error);

    const auto linked = orphan();
    if (linked.empty() || !CreateHardLinkW((linked + L"\\z-hardlink.bin").c_str(), (published + file).c_str(), nullptr))
        return false;
    result = copyTree(publishedSource.get(), publishedId, to.get(), targetId, L"refused-link");
    Check(result.error == ERROR_NOT_SUPPORTED && !Exists(target + L"\\refused-link") &&
              HasContents(linked + L"\\nested\\first.txt", {1, 2, 3}) &&
              HasContents(linked + L"\\z-hardlink.bin", {4, 8, 15, 16, 23, 42}) && HasContents(published + file, {4, 8, 15, 16, 23, 42}),
          "a hard link refuses the whole recovery before deleting any entry", result.error);
    return true;
}
DWORD ReplaceForProbe(HANDLE source, std::wstring_view target, bool posix)
{
    constexpr DWORD replaceExisting = 1;
    constexpr DWORD posixSemantics = 2;
    const auto bytes = static_cast<DWORD>(target.size() * sizeof(wchar_t));
    const auto length = static_cast<DWORD>(sizeof(FILE_RENAME_INFO)) + bytes;
    const auto buffer = std::make_unique<BYTE[]>(length);
    auto& info = *reinterpret_cast<FILE_RENAME_INFO*>(buffer.get());
    info.Flags = replaceExisting | (posix ? posixSemantics : 0);
    info.FileNameLength = bytes;
    std::memcpy(info.FileName, target.data(), bytes);
    return SetFileInformationByHandle(source, FileRenameInfoEx, &info, length) ? ERROR_SUCCESS : GetLastError();
}

bool CheckOverwrite(const std::wstring& fixture)
{
    constexpr DWORD shareAll = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const auto open = [](const std::wstring& path, DWORD access, DWORD share) {
        const HANDLE raw = CreateFileW(path.c_str(), access, share, nullptr, OPEN_EXISTING, 0, nullptr);
        return Handle(raw != INVALID_HANDLE_VALUE ? raw : nullptr);
    };
    struct TargetPin
    {
        const wchar_t* name;
        DWORD access;
        DWORD share;
    };
    const TargetPin pins[] = {
        {L"metadata", FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE},
        {L"read-no-delete-share", FILE_READ_DATA, FILE_SHARE_READ},
        {L"exclusive-delete", FILE_READ_DATA | DELETE, 0},
        {L"delete-sharing", FILE_READ_DATA, shareAll},
    };
    const std::vector<BYTE> original{1, 2, 3};
    const std::vector<BYTE> replacement{4, 5, 6};
    for (const auto& pin : pins)
    {
        for (const bool posix : {false, true})
        {
            const std::wstring tag = std::wstring(pin.name) + (posix ? L"-posix" : L"-legacy");
            const auto target = fixture + L"\\" + tag + L"-target";
            const auto source = fixture + L"\\" + tag + L"-source";
            if (!WriteFixture(target, original) || !WriteFixture(source, replacement))
                return false;

            auto held = open(target, pin.access, pin.share);
            auto staged = open(source, DELETE, shareAll);
            if (!held || !staged)
                return false;
            FILE_ID_INFO approvedId{}, stagedId{}, heldId{}, currentId{};
            if (!GetFileInformationByHandleEx(held.get(), FileIdInfo, &approvedId, sizeof(approvedId)) ||
                !GetFileInformationByHandleEx(staged.get(), FileIdInfo, &stagedId, sizeof(stagedId)))
                return false;

            const HANDLE competing = CreateFileW(target.c_str(), DELETE, shareAll, nullptr, OPEN_EXISTING, 0, nullptr);
            const DWORD deleteOpen = competing == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
            if (competing != INVALID_HANDLE_VALUE)
                CloseHandle(competing);
            const DWORD rename = ReplaceForProbe(staged.get(), target, posix);
            const bool originalHandleStayed = GetFileInformationByHandleEx(held.get(), FileIdInfo, &heldId, sizeof(heldId)) &&
                UacRenameLeaf::Detail::SameSource(approvedId, heldId);
            held.reset();
            staged.reset();
            const auto current = OpenIdentity(target, currentId);
            const bool changed = current && UacRenameLeaf::Detail::SameSource(stagedId, currentId);
            const bool oldNameStayed = current && UacRenameLeaf::Detail::SameSource(approvedId, currentId);
            std::printf("PROBE: %ls delete-open=%lu rename=%lu old-handle-stayed=%d name-replaced=%d\n",
                         tag.c_str(), deleteOpen, rename, originalHandleStayed, changed);
            Check(originalHandleStayed && (rename == ERROR_SUCCESS
                      ? changed && !Exists(source) && HasContents(target, replacement)
                      : oldNameStayed && HasContents(source, replacement) && HasContents(target, original)),
                  "replacement result matches the identities and bytes on disk", rename);
        }
    }
    // Interleave an independent replacement after the identity snapshot and
    // before the publisher's syscall. Both operate inside the same NTFS parent.
    const auto target = fixture + L"\\approved-\u6771\u4eac.txt";
    const auto rival = fixture + L"\\competing-copy.txt";
    const auto source = fixture + L"\\staged-copy.txt";
    const std::vector<BYTE> rivalContents{7, 8, 9};
    if (!WriteFixture(target, original) || !WriteFixture(rival, rivalContents) || !WriteFixture(source, replacement))
        return false;
    FILE_ID_INFO approvedId{}, intermediateId{}, finalId{}, heldId{};
    const auto approved = OpenIdentity(target, approvedId);
    auto competing = open(rival, DELETE, shareAll);
    auto staged = open(source, DELETE, shareAll);
    if (!approved || !competing || !staged)
        return false;
    const DWORD first = ReplaceForProbe(competing.get(), target, true);
    competing.reset();
    if (first != ERROR_SUCCESS)
    {
        std::printf("PROBE: interleaving unavailable on this Windows build (%lu)\n", first);
        return failures == 0;
    }
    const auto intermediate = OpenIdentity(target, intermediateId);
    if (!intermediate || !HasContents(target, rivalContents) ||
        UacRenameLeaf::Detail::SameSource(approvedId, intermediateId))
        return false;
    const DWORD second = ReplaceForProbe(staged.get(), target, true);
    staged.reset();
    const auto final = OpenIdentity(target, finalId);
    if (!final)
        return false;
    const bool approvedHandleStayed = GetFileInformationByHandleEx(approved.get(), FileIdInfo, &heldId, sizeof(heldId)) &&
        UacRenameLeaf::Detail::SameSource(approvedId, heldId);
    const bool intermediateNameChanged = !UacRenameLeaf::Detail::SameSource(intermediateId, finalId);
    std::printf("PROBE: interleaved rename=%lu old-handle-stayed=%d substituted-name-replaced=%d\n",
                second, approvedHandleStayed, intermediateNameChanged);
    Check(approvedHandleStayed && (second == ERROR_SUCCESS
              ? intermediateNameChanged && HasContents(target, replacement)
              : !intermediateNameChanged && HasContents(target, rivalContents) && HasContents(source, replacement)),
          "a stale approval handle does not identify the pathname used by replacement", second);
    return failures == 0;
}
bool CheckMoveSource(const std::wstring& fixture, HANDLE root)
{
    using namespace UacDirectoryLeaf;
    HANDLE rawParent = nullptr;
    ParentIdentity targetId;
    if (OpenParentBelowRoot(root, L"destination", rawParent, targetId))
        return false;
    const Handle target(rawParent);
    const std::vector<BYTE> mainData{1, 2, 3};
    const std::vector<BYTE> addedData{4, 5, 6};
    constexpr DWORD shareAll = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;

    for (const bool simulateDeletion : {false, true})
    {
        const std::wstring leaf = simulateDeletion ? L"discard-probe" : L"preserve-probe";
        const auto original = fixture + L"\\" + leaf;
        const auto copied = fixture + L"\\destination\\" + leaf;
        const auto alternate = original + L":late-\u65e5";
        if (!WriteFixture(original, mainData))
            return false;
        HANDLE rawSource = nullptr;
        if (UacRenameLeaf::Detail::OpenSource(root, leaf, rawSource, FILE_READ_DATA | FILE_READ_EA | DELETE, 0))
            return false;
        Handle source(rawSource);
        FILE_ID_INFO originalId{};
        if (UacRenameLeaf::Detail::ReadSourceIdentity(source.get(), originalId))
            return false;
        FILE_BASIC_INFO snapshot{};
        const DWORD copyError = UacCopyLeaf::Detail::CopyOpenedFile(source.get(), target.get(), leaf,
            nullptr, nullptr, MAXULONGLONG, mainData.size(), snapshot);
        Check(copyError == ERROR_SUCCESS && HasContents(copied, mainData),
              "publish the main-stream copy while holding an exclusive source handle", copyError);
        if (copyError)
            return false;

        const HANDLE mainWriter = CreateFileW(original.c_str(), FILE_WRITE_DATA, shareAll, nullptr, OPEN_EXISTING, 0, nullptr);
        const DWORD mainOpen = mainWriter == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
        if (mainWriter != INVALID_HANDLE_VALUE)
            CloseHandle(mainWriter);
        Check(mainOpen == ERROR_SHARING_VIOLATION, "refuse a competing main-stream writer", mainOpen);

        // Model the last recheck before the eventual delete request. The
        // competing stream arrives after it, using the same source name.
        FILE_BASIC_INFO beforeLateWrite{};
        const DWORD before = UacCopyLeaf::Detail::ReadPlainMetadata(source.get(), beforeLateWrite);
        Check(before == ERROR_SUCCESS && UacCopyLeaf::Detail::SameBasicMetadata(snapshot, beforeLateWrite),
              "observe an unchanged, ordinary source at the last pre-delete check", before);
        if (before)
            return false;
        const HANDLE rawStream = CreateFileW(alternate.c_str(), GENERIC_WRITE, shareAll, nullptr, CREATE_NEW, 0, nullptr);
        const DWORD streamOpen = rawStream == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
        std::printf("PROBE: %ls main-write-open=%lu late-stream-open=%lu\n", leaf.c_str(), mainOpen, streamOpen);
        if (streamOpen != ERROR_SUCCESS)
        {
            Check(streamOpen == ERROR_SHARING_VIOLATION || streamOpen == ERROR_ACCESS_DENIED,
                  "report a source guard that also rejects the alternate-stream writer", streamOpen);
            continue;
        }
        {
            const Handle stream(rawStream);
            DWORD written = 0;
            if (!WriteFile(stream.get(), addedData.data(), static_cast<DWORD>(addedData.size()), &written, nullptr) ||
                written != addedData.size())
                return false;
        }
        FILE_ID_INFO currentId{};
        FILE_BASIC_INFO afterLateWrite{};
        const DWORD after = UacCopyLeaf::Detail::ReadPlainMetadata(source.get(), afterLateWrite);
        Check(!UacRenameLeaf::Detail::ReadSourceIdentity(source.get(), currentId) &&
                  UacRenameLeaf::Detail::SameSource(originalId, currentId) && after == ERROR_NOT_SUPPORTED,
              "identify the same file with a stream the earlier copy never read", after);

        DWORD dispositionError = ERROR_SUCCESS;
        if (simulateDeletion)
        {
            FILE_DISPOSITION_INFO_EX disposition{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS |
                                                 FILE_DISPOSITION_FLAG_FORCE_IMAGE_SECTION_CHECK};
            if (!SetFileInformationByHandle(source.get(), FileDispositionInfoEx, &disposition, sizeof(disposition)))
                dispositionError = GetLastError();
        }
        source.reset();
        const bool removed = simulateDeletion && dispositionError == ERROR_SUCCESS;
        std::printf("PROBE: %ls delete-status=%lu source-removed=%d\n", leaf.c_str(), dispositionError, removed);
        Check(HasContents(copied, mainData) && !Exists(copied + L":late-\u65e5") &&
                  (removed ? !Exists(original) && !Exists(alternate)
                           : HasContents(original, mainData) && HasContents(alternate, addedData)),
              "observe whether the late bytes survive the chosen source-retirement decision", dispositionError);
    }
    return failures == 0;
}
bool CheckMoveSourceGuards(const std::wstring& fixture, HANDLE root)
{
    constexpr DWORD shareAll = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const auto path = fixture + L"\\guard-probe";
    const auto existingStream = path + L":existing";
    const auto newStream = path + L":new";
    const std::vector<BYTE> original{1};
    if (!WriteFixture(path, original))
        return false;

    HANDLE raw = nullptr;
    const DWORD open = UacRenameLeaf::Detail::OpenSource(root, L"guard-probe", raw,
        FILE_READ_DATA | FILE_READ_EA | FILE_WRITE_ATTRIBUTES | DELETE, 0);
    if (open)
        return false;
    Handle source(raw);
    auto openStream = [&](const std::wstring& name, DWORD access, DWORD disposition, DWORD& error) -> Handle {
        const HANDLE file = CreateFileW(name.c_str(), access, shareAll, nullptr, disposition, 0, nullptr);
        error = file == INVALID_HANDLE_VALUE ? GetLastError() : ERROR_SUCCESS;
        return Handle(error ? nullptr : file);
    };
    auto setAttributes = [](HANDLE file, DWORD attributes) {
        FILE_BASIC_INFO basic{};
        basic.FileAttributes = attributes;
        return SetFileInformationByHandle(file, FileBasicInfo, &basic, sizeof(basic)) != FALSE;
    };

    if (!setAttributes(source.get(), FILE_ATTRIBUTE_READONLY))
        return false;
    DWORD readonlyStreamError = ERROR_SUCCESS;
    auto readonlyStream = openStream(newStream, FILE_WRITE_DATA, CREATE_NEW, readonlyStreamError);
    readonlyStream.reset();
    DWORD attributeOpenError = ERROR_SUCCESS;
    const auto attributeWriter = openStream(path, FILE_WRITE_ATTRIBUTES, OPEN_EXISTING, attributeOpenError);
    const bool clearedByPeer = attributeWriter && setAttributes(attributeWriter.get(), FILE_ATTRIBUTE_NORMAL);
    if (!clearedByPeer && !setAttributes(source.get(), FILE_ATTRIBUTE_NORMAL))
        return false;
    std::printf("PROBE: readonly-new-stream=%lu attribute-writer=%lu peer-cleared-readonly=%d\n",
                readonlyStreamError, attributeOpenError, clearedByPeer);

    DWORD existingError = ERROR_SUCCESS;
    auto writer = openStream(existingStream, FILE_WRITE_DATA, CREATE_NEW, existingError);
    if (!writer)
        return false;
    FILE_DISPOSITION_INFO disposition{TRUE};
    const BOOL pending = SetFileInformationByHandle(source.get(), FileDispositionInfo, &disposition, sizeof(disposition));
    const DWORD pendingError = pending ? ERROR_SUCCESS : GetLastError();
    if (!pending)
    {
        std::printf("PROBE: delete-pending=%lu\\n", pendingError);
        return false;
    }
    DWORD pendingStreamError = ERROR_SUCCESS;
    const auto pendingStream = openStream(path + L":after-delete-pending", FILE_WRITE_DATA, CREATE_NEW, pendingStreamError);
    const BYTE added = 9;
    DWORD written = 0;
    const BOOL writtenWhilePending = WriteFile(writer.get(), &added, sizeof(added), &written, nullptr);
    const DWORD pendingWriteError = writtenWhilePending ? ERROR_SUCCESS : GetLastError();
    disposition.DeleteFile = FALSE;
    const BOOL cleared = SetFileInformationByHandle(source.get(), FileDispositionInfo, &disposition, sizeof(disposition));
    const DWORD clearError = cleared ? ERROR_SUCCESS : GetLastError();
    writer.reset();
    source.reset();
    std::printf("PROBE: delete-pending-new-stream=%lu existing-write=%lu clear-pending=%lu\n",
                pendingStreamError, pendingWriteError, clearError);
    Check(cleared && HasContents(path, original), "clear delete-pending and keep the original main stream", clearError);
    Check(!writtenWhilePending || (written == sizeof(added) && HasContents(existingStream, {added})),
          "detect bytes written through an existing alternate stream while delete-pending", pendingWriteError);
    return failures == 0;
}

bool CheckDirectoryMetadata(const std::wstring& fixture, HANDLE root)
{
    using namespace UacDirectoryLeaf;
    constexpr std::wstring_view leaf = L"times-\u6771\u4eac";
    const auto path = fixture + L"\\" + std::wstring(leaf);
    if (!CreateDirectoryW(path.c_str(), nullptr))
        return false;
    const auto child = path + L"\\child.txt";
    if (!WriteFixture(child, {1, 2, 3}))
        return false;
    WIN32_FILE_ATTRIBUTE_DATA childBefore{};
    if (!GetFileAttributesExW(child.c_str(), GetFileExInfoStandard, &childBefore))
        return false;
    HANDLE raw = nullptr;
    ParentIdentity rootId, directoryId;
    if (ReadParentIdentity(root, rootId) || OpenParentBelowRoot(root, leaf, raw, directoryId))
        return false;
    Handle directory(raw);
    FILE_BASIC_INFO before{}, after{};
    if (!GetFileInformationByHandleEx(directory.get(), FileBasicInfo, &before, sizeof(before)))
        return false;

    UacF7::MetadataChange change{};
    change.expectedAttributes = before.FileAttributes;
    change.created = 132000000000000000LL;
    change.modified = 133000000000000000LL;
    const auto set = [&](const FILE_ID_INFO& expected, std::wstring_view name) {
        return UacMetadataLeaf::SetVerifiedMetadata(root, rootId, expected, name, change);
    };
    DWORD error = set(directoryId.file, leaf);
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(directory.get(), FileBasicInfo, &after, sizeof(after)) &&
              after.CreationTime.QuadPart == change.created && after.LastWriteTime.QuadPart == change.modified &&
              after.LastAccessTime.QuadPart == before.LastAccessTime.QuadPart && after.FileAttributes == before.FileAttributes,
          "set only requested times on a verified Unicode directory", error);
    change.created = 0;
    change.modified = 0;
    change.accessed = 134000000000000000LL;
    error = set(directoryId.file, leaf);
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(directory.get(), FileBasicInfo, &after, sizeof(after)) &&
              after.CreationTime.QuadPart == 132000000000000000LL && after.LastWriteTime.QuadPart == 133000000000000000LL &&
              after.LastAccessTime.QuadPart == change.accessed && after.FileAttributes == before.FileAttributes,
          "set directory access time independently", error);

    change.accessed = 0;
    change.modified = 131000000000000000LL;
    change.attributes = UacF7::basicAttributes;
    error = set(directoryId.file, leaf);
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(directory.get(), FileBasicInfo, &after, sizeof(after)) &&
              after.FileAttributes == (FILE_ATTRIBUTE_DIRECTORY | UacF7::basicAttributes) &&
              after.LastWriteTime.QuadPart == change.modified && after.CreationTime.QuadPart == 132000000000000000LL,
          "set all basic directory flags and the requested time", error);
    change.expectedAttributes = FILE_ATTRIBUTE_DIRECTORY | UacF7::basicAttributes;
    change.attributes = FILE_ATTRIBUTE_NORMAL;
    change.modified = 0;
    error = set(directoryId.file, leaf);
    Check(error == ERROR_SUCCESS &&
              GetFileInformationByHandleEx(directory.get(), FileBasicInfo, &after, sizeof(after)) &&
              after.FileAttributes == before.FileAttributes && after.LastWriteTime.QuadPart == 131000000000000000LL,
          "clear the basic flags without removing the directory bit or changing its time", error);
    WIN32_FILE_ATTRIBUTE_DATA childAfter{};
    Check(GetFileAttributesExW(child.c_str(), GetFileExInfoStandard, &childAfter) &&
              childAfter.dwFileAttributes == childBefore.dwFileAttributes &&
              CompareFileTime(&childBefore.ftLastWriteTime, &childAfter.ftLastWriteTime) == 0,
          "leave the contents of the verified directory untouched");
    change.expectedAttributes = before.FileAttributes;
    change.attributes = FILE_ATTRIBUTE_COMPRESSED;
    error = set(directoryId.file, leaf);
    Check(error == ERROR_INVALID_DATA && GetFileAttributesW(path.c_str()) == before.FileAttributes,
          "refuse a non-basic attribute in a directory metadata request", error);
    change.attributes = 0;
    change.accessed = 134000000000000000LL;
    change.expectedAttributes ^= FILE_ATTRIBUTE_HIDDEN;
    error = set(directoryId.file, leaf);
    Check(error == ERROR_FILE_INVALID, "refuse a stale directory attribute snapshot", error);
    change.expectedAttributes = before.FileAttributes;
    error = set(directoryId.file, L"junction");
    Check(error != ERROR_SUCCESS, "refuse a directory junction before writing timestamps", error);

    const auto moved = fixture + L"\\displaced-times";
    directory.reset();
    if (!MoveFileW(path.c_str(), moved.c_str()) || !CreateDirectoryW(path.c_str(), nullptr))
        return false;
    error = set(directoryId.file, leaf);
    Check(error == ERROR_FILE_INVALID, "refuse a substituted directory at the same name", error);
    return true;
}
}

static bool CheckFindTagStream()
{
    using UacF7::Operation;
    using UacListing::ValidRequestScope;
    Check(ValidRequestScope(Operation::FindFile, 0, false, true) &&
              !ValidRequestScope(Operation::FindFile, 1, false, true),
          "an exact-file Find grant accepts only a content request with no path");
    Check(!ValidRequestScope(Operation::FindFile, 0, false, false) &&
              !ValidRequestScope(Operation::FindFile, 0, true, false) &&
              !ValidRequestScope(Operation::FindFile, 0, true, true),
          "an exact-file Find grant refuses directory listing and inventory");
    Check(ValidRequestScope(Operation::FindContent, 1, false, true) &&
              !ValidRequestScope(Operation::FindContent, 0, false, true) &&
              !ValidRequestScope(Operation::List, 1, false, true),
          "directory content reads require a named file and an explicit content grant");
    struct Fixture
    {
        DWORD calls = 0;
        ULONGLONG last = 0;
        DWORD error = ERROR_SUCCESS;
    } fixture;
    auto read = [](void* context, ULONGLONG offset, void* output, DWORD bytes) -> DWORD {
        auto& fixture = *static_cast<Fixture*>(context);
        ++fixture.calls;
        fixture.last = offset;
        if (fixture.error)
            return fixture.error;
        for (DWORD i = 0; i < bytes; ++i)
            static_cast<BYTE*>(output)[i] = static_cast<BYTE>(offset + i);
        return ERROR_SUCCESS;
    };
    constexpr ULONGLONG length = (5ULL << 30) + 9;
    auto* stream = CFindTagStream::Create(length, 3, read, &fixture);
    if (!stream)
        return false;

    STATSTG stat{};
    HRESULT hr = stream->Stat(&stat, STATFLAG_NONAME);
    Check(hr == S_OK && stat.cbSize.QuadPart == length && stat.grfMode == STGM_READ,
          "advertise a large read-only tag stream", hr);
    BYTE bytes[8]{};
    ULONG count = 0;
    hr = stream->Read(bytes, sizeof(bytes), &count);
    Check(hr == S_OK && count == sizeof(bytes) && bytes[7] == 7 && fixture.calls == 3,
          "split property reads into bounded broker blocks", hr);

    LARGE_INTEGER move{};
    move.QuadPart = -2;
    ULARGE_INTEGER position{};
    hr = stream->Seek(move, STREAM_SEEK_END, &position);
    Check(hr == S_OK && position.QuadPart == length - 2, "seek to a 64-bit property offset", hr);
    IStream* clone = nullptr;
    if (FAILED(stream->Clone(&clone)))
    {
        stream->Release();
        return false;
    }
    hr = stream->Read(bytes, sizeof(bytes), &count);
    Check(hr == S_FALSE && count == 2 && fixture.last == length - 2,
          "return a partial property read at end of file", hr);
    hr = clone->Read(bytes, 1, &count);
    Check(hr == S_OK && count == 1 && fixture.last == length - 2,
          "keep an independent seek position for a property stream clone", hr);
    hr = clone->Write(bytes, 1, &count);
    Check(hr == STG_E_ACCESSDENIED && count == 0, "reject property stream writes", hr);

    fixture.error = ERROR_BROKEN_PIPE;
    hr = clone->Read(bytes, 1, &count);
    const auto error = stream->Detach();
    Check(hr == HRESULT_FROM_WIN32(ERROR_BROKEN_PIPE) && !count && error == ERROR_BROKEN_PIPE,
          "retain a broker failure even if a property handler ignores it", error);
    hr = clone->Read(bytes, 1, &count);
    Check(hr == STG_E_REVERTED && !count, "revoke outstanding property stream clones after reading", hr);
    clone->Release();
    stream->Release();
    return true;
}

static bool CheckViewerSnapshot(const std::wstring& fixture, HANDLE root)
{
    const auto component = std::wstring(160, L'\u65e5');
    const auto path = fixture + L"\\" + component;
    const auto leaf = std::wstring(110, L'\u6771') + L".txt";
    const auto name = path + L"\\" + leaf;
    const std::vector<BYTE> expected{0xef, 0xbb, 0xbf, 'A', 0, 'Z'};
    if (!CreateDirectoryW(path.c_str(), nullptr) || !WriteFixture(name, expected) ||
        !WriteFixture(path + L"\\empty.txt", {}) || !WriteFixture(path + L"\\large.txt", {}))
        return false;
    UacDirectoryLeaf::ParentIdentity parentIdentity;
    HANDLE rawParent = nullptr;
    if (UacDirectoryLeaf::OpenParentBelowRoot(root, component, rawParent, parentIdentity))
        return false;
    const Handle parent(rawParent);
    FILE_ID_INFO identity{}, emptyId{}, largeId{};
    const Handle original = OpenIdentity(name, identity);
    const Handle empty = OpenIdentity(path + L"\\empty.txt", emptyId);
    const Handle large = OpenIdentity(path + L"\\large.txt", largeId);
    if (!original || !empty || !large)
        return false;
    const HANDLE writerRaw = CreateFileW((path + L"\\large.txt").c_str(), GENERIC_WRITE,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         nullptr, OPEN_EXISTING, 0, nullptr);
    if (writerRaw == INVALID_HANDLE_VALUE)
        return false;
    Handle writer(writerRaw);
    LARGE_INTEGER limit{};
    limit.QuadPart = UacF7::maxViewerSnapshotBytes + 1;
    if (!SetFilePointerEx(writer.get(), limit, nullptr, FILE_BEGIN) || !SetEndOfFile(writer.get()))
        return false;
    writer.reset();

    HANDLE archiveRaw = nullptr;
    LARGE_INTEGER archiveSize{};
    FILE_BASIC_INFO archiveInfo{};
    const DWORD archiveError = UacViewLeaf::OpenVerifiedRead(parent.get(), parentIdentity, largeId, L"large.txt",
                                                            MAXULONGLONG, archiveRaw, archiveSize, archiveInfo);
    const Handle archive(archiveRaw);
    Check(archiveError == ERROR_SUCCESS && archive && archiveSize.QuadPart == limit.QuadPart,
          "open a verified archive above the viewer snapshot limit", archiveError);
    const ULONGLONG zip64Size = 5ULL * 1024 * 1024 * 1024;
    const DWORD block = UacF7::archiveReadBlock;
    Check(UacF7::ValidArchiveRange({zip64Size - block, block, 0}, zip64Size, block),
          "accept an archive range beyond the 32-bit offset limit", 0);
    Check(!UacF7::ValidArchiveRange({MAXULONGLONG - 5, 8, 0}, MAXULONGLONG, block) &&
              !UacF7::ValidArchiveRange({zip64Size, 1, 0}, zip64Size, block),
          "reject archive range overflow and reads beyond EOF", 0);
    Check(!UacF7::ValidArchiveRange({0, block + 1, 0}, zip64Size, block + 1) &&
              !UacF7::ValidArchiveRange({0, block, 0}, zip64Size, block - 1) &&
              !UacF7::ValidArchiveRange({0, 1, 1}, zip64Size, block) &&
              !UacF7::ValidArchiveRange({0, 0, 0}, zip64Size, block),
          "reject oversized, over-budget and malformed archive ranges", 0);

    std::vector<BYTE> snapshot;
    DWORD error = UacViewLeaf::ReadVerifiedSnapshot(parent.get(), parentIdentity, identity, leaf, MAXULONGLONG, snapshot);
    Check(error == ERROR_SUCCESS && snapshot == expected && HasContents(name, expected),
          "read exact bytes of a verified long Unicode viewer file", error);
    error = UacViewLeaf::ReadVerifiedSnapshot(parent.get(), parentIdentity, emptyId, L"empty.txt", MAXULONGLONG, snapshot);
    Check(error == ERROR_SUCCESS && snapshot.empty(), "open an empty protected viewer file", error);
    error = UacViewLeaf::ReadVerifiedSnapshot(parent.get(), parentIdentity, largeId, L"large.txt", MAXULONGLONG, snapshot);
    Check(error == ERROR_FILE_TOO_LARGE && snapshot.empty(), "refuse an oversized viewer file before transferring bytes", error);
    error = UacViewLeaf::ReadVerifiedSnapshot(parent.get(), parentIdentity, identity, leaf, GetTickCount64(), snapshot);
    Check(error == ERROR_TIMEOUT && snapshot.empty(), "refuse a viewer read after its deadline", error);
    error = UacViewLeaf::ReadVerifiedSnapshot(parent.get(), parentIdentity, identity, L"empty.txt", MAXULONGLONG, snapshot);
    Check(error == ERROR_FILE_INVALID && snapshot.empty(), "reject a different file identity for the viewer", error);
    error = UacViewLeaf::ReadVerifiedSnapshot(parent.get(), parentIdentity, identity, L"..\\empty.txt", MAXULONGLONG, snapshot);
    Check(error == ERROR_INVALID_NAME && snapshot.empty(), "reject a path in a single viewer leaf request", error);

    HANDLE scopedRaw = nullptr;
    FILE_BASIC_INFO scopedInfo{};
    LARGE_INTEGER scopedSize{};
    error = UacViewLeaf::OpenScopedRead(root, component + L"\\" + leaf, MAXULONGLONG,
                                        scopedRaw, scopedSize, scopedInfo);
    {
        const Handle scoped(scopedRaw);
        BYTE bytes[6]{};
        DWORD read = 0;
        Check(!error && scopedSize.QuadPart == sizeof(bytes) &&
                  ReadFile(scoped.get(), bytes, sizeof(bytes), &read, nullptr) &&
                  read == sizeof(bytes) && std::equal(expected.begin(), expected.end(), bytes),
              "read a nested Unicode Find candidate beyond MAX_PATH through its root", error);
    }
    error = UacViewLeaf::OpenScopedRead(root, component + L"\\large.txt", MAXULONGLONG,
                                        scopedRaw, scopedSize, scopedInfo);
    {
        const Handle scoped(scopedRaw);
        LARGE_INTEGER last{};
        last.QuadPart = limit.QuadPart - 1;
        BYTE value = 1;
        DWORD read = 0;
        Check(!error && scopedSize.QuadPart == limit.QuadPart &&
                  SetFilePointerEx(scoped.get(), last, nullptr, FILE_BEGIN) &&
                  ReadFile(scoped.get(), &value, 1, &read, nullptr) && read == 1 && value == 0,
              "read a Find range past the viewer snapshot limit", error);
    }
    error = UacViewLeaf::OpenScopedRead(parent.get(), L"..\\guard-probe", MAXULONGLONG,
                                        scopedRaw, scopedSize, scopedInfo);
    Check(error == ERROR_INVALID_NAME && !scopedRaw, "reject escaping the Find root", error);
    error = UacViewLeaf::OpenScopedRead(root, component + L"\\empty.txt:stream", MAXULONGLONG,
                                        scopedRaw, scopedSize, scopedInfo);
    Check(error == ERROR_INVALID_NAME && !scopedRaw, "reject a named stream in Find", error);

    const auto alias = path + L"\\hardlink.txt";
    if (!CreateHardLinkW(alias.c_str(), name.c_str(), nullptr))
        return false;
    error = UacViewLeaf::OpenScopedRead(parent.get(), L"hardlink.txt", MAXULONGLONG,
                                        scopedRaw, scopedSize, scopedInfo);
    Check(error == ERROR_NOT_SUPPORTED && !scopedRaw, "reject a multiply linked Find candidate", error);
    return DeleteFileW(alias.c_str()) != FALSE;
}

static bool CheckDetailedInventory(const std::wstring& fixture, HANDLE root)
{
    const auto path = fixture + L"\\size-details";
    if (!CreateDirectoryW(path.c_str(), nullptr) ||
        !WriteFixture(path + L"\\empty.txt", {}) || !WriteFixture(path + L"\\ordinary.txt", {1, 2, 3}))
        return false;

    const auto compressed = path + L"\\\u6771-compressed.txt";
    const auto sparse = path + L"\\sparse.bin";
    const auto createSpecial = [](const std::wstring& name, bool compress) {
        const HANDLE raw = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        const Handle file(raw);
        DWORD count = 0;
        USHORT format = COMPRESSION_FORMAT_LZNT1;
        if (!compress && !DeviceIoControl(raw, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &count, nullptr))
            return false;
        const std::vector<BYTE> data(compress ? 128 * 1024 : 1, 'A');
        if (!WriteFile(raw, data.data(), static_cast<DWORD>(data.size()), &count, nullptr) || count != data.size())
            return false;
        if (compress)
            return DeviceIoControl(raw, FSCTL_SET_COMPRESSION, &format, sizeof(format), nullptr, 0, &count, nullptr) != FALSE;
        LARGE_INTEGER end{};
        end.QuadPart = 1024 * 1024;
        return SetFilePointerEx(raw, end, nullptr, FILE_BEGIN) && SetEndOfFile(raw);
    };
    if (!createSpecial(compressed, true) || !createSpecial(sparse, false))
        return false;

    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity;
    if (UacDirectoryLeaf::OpenParentBelowRoot(root, L"size-details", raw, identity, FILE_LIST_DIRECTORY))
        return false;
    const Handle folder(raw);
    DWORD sectors, bytes, freeClusters, totalClusters;
    if (!GetDiskFreeSpaceW((identity.volume + L"\\").c_str(), &sectors, &bytes, &freeClusters, &totalClusters))
        return false;
    const ULONGLONG cluster = sectors * bytes;
    UacF7::DirectoryInventory inventory;
    UacF7::DirectorySizeDetails details;
    const DWORD error = UacListing::Inventory(folder.get(), MAXULONGLONG, inventory, &details);
    std::sort(details.fileSizes.begin(), details.fileSizes.end());
    Check(error == ERROR_SUCCESS && inventory.files == 4 && inventory.directories == 0 &&
              details.fileSizes == std::vector<ULONGLONG>{0, 3, 128 * 1024, 1024 * 1024},
          "return complete per-file logical sizes for protected estimates", error);
    const auto storedSize = [](const std::wstring& name) {
        DWORD high = 0;
        const DWORD low = GetCompressedFileSizeW(name.c_str(), &high);
        return (static_cast<ULONGLONG>(high) << 32) | low;
    };
    const auto compressedBytes = storedSize(compressed);
    const auto sparseBytes = storedSize(sparse);
    const auto rounded = [cluster](ULONGLONG size) { return size + (cluster - size % cluster) % cluster; };
    Check(!error && compressedBytes < 128 * 1024 && sparseBytes < 1024 * 1024 &&
              details.disk.compressed == 3 + compressedBytes + sparseBytes,
          "measure sparse and compressed files like ordinary Ctrl+Q", error);
    Check(!error && details.disk.occupied == rounded(3) + rounded(compressedBytes) + rounded(sparseBytes),
          "round the measured size separately for each protected file", error);
    return true;
}

bool CheckZipSelection(const std::wstring& fixture, HANDLE root)
{
    const std::wstring folder = fixture + L"\\zip-extract";
    if (!CreateDirectoryW(folder.c_str(), nullptr))
        return false;
    HANDLE raw = nullptr;
    UacDirectoryLeaf::ParentIdentity identity;
    if (UacDirectoryLeaf::OpenParentBelowRoot(root, L"zip-extract", raw, identity))
        return false;
    const Handle parent(raw);
    std::vector<BYTE> manifest;
    const auto append = [&](DWORD index, std::wstring_view name, bool directory, const std::vector<BYTE>& contents) {
        FILETIME time{};
        GetSystemTimeAsFileTime(&time);
        const UacF7::ZipSelectionEntry entry{index, static_cast<DWORD>(name.size()),
                                            static_cast<DWORD>(contents.size()), directory ? 1U : 0U, time};
        const auto* header = reinterpret_cast<const BYTE*>(&entry);
        manifest.insert(manifest.end(), header, header + sizeof(entry));
        const auto* bytes = reinterpret_cast<const BYTE*>(name.data());
        manifest.insert(manifest.end(), bytes, bytes + name.size() * sizeof(wchar_t));
        manifest.insert(manifest.end(), contents.begin(), contents.end());
    };

    constexpr DWORD top = UacF7::zipSelectionRoot;
    append(top, L"root", true, {});
    append(0, L"\u6771.txt", false, {1, 2, 3});
    append(0, L"empty", true, {});
    auto error = UacZipSelection::Publish(parent.get(), identity, manifest);
    Check(!error && HasContents(folder + L"\\root\\\u6771.txt", {1, 2, 3}) &&
          DirectoryExists(folder + L"\\root\\empty"), "extract nested Unicode and empty ZIP entries", error);

    error = UacZipSelection::Publish(parent.get(), identity, manifest);
    Check(error == ERROR_ALREADY_EXISTS && HasContents(folder + L"\\root\\\u6771.txt", {1, 2, 3}),
          "reject an existing ZIP extraction root without changing its files", error);

    manifest.clear();
    append(top, L"unpublished.txt", false, {4});
    append(top, L"..", true, {});
    error = UacZipSelection::Publish(parent.get(), identity, manifest);
    Check(error == ERROR_INVALID_NAME && !Exists(folder + L"\\unpublished.txt"),
          "reject an invalid ZIP component before any output is created", error);

    manifest.clear();
    append(top, L"duplicate.txt", false, {5});
    append(top, L"DUPLICATE.TXT", false, {6});
    error = UacZipSelection::Publish(parent.get(), identity, manifest);
    Check(error == ERROR_ALREADY_EXISTS && !Exists(folder + L"\\duplicate.txt"),
          "reject case-insensitive duplicate output names before extraction", error);
    return true;
}

bool CheckReplacement(const std::wstring& fixture, HANDLE root, UacZipUpdate::ReplacementKind kind)
{
    using UacZipUpdate::Publish;
    using UacZipUpdate::ReadRevision;
    using UacZipUpdate::Recover;
    using UacZipUpdate::Revision;
    using UacRenameLeaf::Detail::NativeRename;
    const bool zip = kind == UacZipUpdate::ReplacementKind::Zip;
    const wchar_t* component = zip ? L"zip-update" : L"disk-overwrite";
    const std::wstring folder = fixture + L"\\" + component;
    if (!CreateDirectoryW(folder.c_str(), nullptr))
        return false;
    HANDLE rawParent = nullptr;
    UacDirectoryLeaf::ParentIdentity identity;
    if (UacDirectoryLeaf::OpenParentBelowRoot(root, component, rawParent, identity))
        return false;
    const Handle parent(rawParent);
    const std::wstring_view leaf = zip ? L"\u6771.zip" : L"\u6771.txt";
    const std::wstring path = folder + L"\\" + std::wstring(leaf);
    const HANDLE rawJournal = CreateFileW((folder + L"\\journal").c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                          nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (rawJournal == INVALID_HANDLE_VALUE)
        return false;
    const Handle journal(rawJournal);
    if (!WriteFixture(path, {1, 2}))
        return false;

    auto open = [&](ACCESS_MASK access) {
        HANDLE file = nullptr;
        if (UacZipUpdate::Detail::Open(parent.get(), leaf, file, access))
            return Handle{};
        return Handle(file);
    };
    Revision expected;
    {
        auto file = open(0);
        if (!file || ReadRevision(file.get(), expected))
            return false;
    }
    HANDLE rawStage = nullptr;
    std::wstring stageName;
    if (UacCopyLeaf::Detail::StageFile(parent.get(), rawStage, stageName, false))
        return false;
    Handle stage(rawStage);
    const BYTE data[] = {3, 4, 5};
    DWORD written = 0;
    if (!WriteFile(stage.get(), data, sizeof(data), &written, nullptr) || written != sizeof(data))
        return false;

    Revision stale = expected;
    stale.fileId[0] ^= 1;
    auto publish = [&](const Revision& revision) -> UacZipUpdate::PublishResult {
        HANDLE raw = nullptr;
        if (const DWORD error = UacZipUpdate::Detail::Open(parent.get(), leaf, raw, DELETE))
            return {error};
        UacZipUpdate::Handle original(raw, CloseHandle);
        return Publish(parent.get(), identity, leaf, revision, original, stage.get(), journal.get(), nullptr, MAXULONGLONG, nullptr, kind);
    };
    auto result = publish(stale);
    Check(result.error == ERROR_FILE_INVALID && !result.published && HasContents(path, {1, 2}),
          "reject a stale replacement identity and retain the original", result.error);
    const std::wstring occupiedBackup = folder + L"\\occupied.bak";
    if (!WriteFixture(occupiedBackup, {7}))
        return false;
    {
        auto original = open(DELETE);
        if (!original)
            return false;
        const auto cancelled = UacRenameLeaf::Detail::ReplaceStagedFile(
            parent.get(), original.get(), stage.get(), leaf, L"cancelled.bak", nullptr, 0);
        Check(cancelled.error == ERROR_TIMEOUT && !cancelled.rollbackError &&
              !Exists(folder + L"\\cancelled.bak"),
              "abort the shared file exchange before moving the original", cancelled.error);
        const auto collision = UacRenameLeaf::Detail::ReplaceStagedFile(
            parent.get(), original.get(), stage.get(), leaf, L"occupied.bak", nullptr, MAXULONGLONG);
        Check((collision.error == ERROR_ALREADY_EXISTS || collision.error == ERROR_FILE_EXISTS) &&
              !collision.rollbackError && HasContents(occupiedBackup, {7}),
              "preserve a competing backup name during staged replacement", collision.error);
    }
    Check(HasContents(path, {1, 2}), "failed exchanges leave the original at its name");
    result = publish(expected);
    stage.reset();
    Check(!result.error && !result.recoveryError && result.published && HasContents(path, {3, 4, 5}),
          "publish the complete verified replacement", result.error ? result.error : result.recoveryError);
    if (!result.published)
        return false;

    UacZipUpdate::Detail::RecoveryRecord record;
    record.parent = identity.file;
    record.replacement = UacZipUpdate::RevisionIdentity(expected);
    std::copy(leaf.begin(), leaf.end(), record.leaf);
    if (FAILED(CoCreateGuid(&record.nonce)))
        return false;
    const auto backup = UacZipUpdate::Detail::BackupName(record.nonce, kind);
    {
        auto file = open(DELETE);
        Revision current;
        if (!file || ReadRevision(file.get(), current))
            return false;
        record.original = UacZipUpdate::RevisionIdentity(current);
        if (UacZipUpdate::Detail::WriteRecord(journal.get(), record) ||
            NativeRename(file.get(), parent.get(), backup))
            return false;
    }
    const auto backupPath = folder + L"\\" + backup;
    if (!WriteFixture(path, {6}))
        return false;
    DWORD error = Recover(parent.get(), identity, leaf, journal.get(), kind);
    Check(error == ERROR_FILE_INVALID && HasContents(path, {6}) && HasContents(backupPath, {3, 4, 5}),
          "keep both files when an unrelated item occupies the destination name", error);
    if (!DeleteFileW(path.c_str()))
        return false;
    error = Recover(parent.get(), identity, leaf, journal.get(), kind);
    Check(!error && HasContents(path, {3, 4, 5}) && !Exists(backupPath),
          "restore the exact original after an interrupted publication", error);
    Check(Recover(parent.get(), identity, leaf, journal.get(), kind) == ERROR_SUCCESS,
          "allow another replacement after clearing the completed publication journal");
    return true;
}

static bool CheckEfsWorkspace(const std::wstring& folder, HANDLE source, const FILE_ID_INFO& sourceId,
                              const std::vector<BYTE>& plaintext)
{
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken))
        return false;
    const Handle token(rawToken);
    DWORD length = 0;
    GetTokenInformation(token.get(), TokenUser, nullptr, 0, &length);
    std::vector<BYTE> tokenData(length);
    if (!length || !GetTokenInformation(token.get(), TokenUser, tokenData.data(), length, &length))
        return false;
    PWSTR sid = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<const TOKEN_USER*>(tokenData.data())->User.Sid, &sid))
        return false;
    const std::unique_ptr<void, decltype(&LocalFree)> sidText(sid, LocalFree);
    const std::wstring sddl = std::wstring(L"O:") + sid + L"D:P(A;;FA;;;" + sid + L")";
    PSECURITY_DESCRIPTOR rawSecurity = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &rawSecurity, nullptr))
        return false;
    const std::unique_ptr<void, decltype(&LocalFree)> security(rawSecurity, LocalFree);

    const auto base = folder + L"\\efs-private";
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), rawSecurity, FALSE};
    if (!CreateDirectoryW(base.c_str(), &attributes))
        return false;
    constexpr DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const HANDLE rawParent = CreateFileW(base.c_str(), FILE_LIST_DIRECTORY | FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | READ_CONTROL,
        share, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (rawParent == INVALID_HANDLE_VALUE)
        return false;
    const Handle parent(rawParent);
    FILE_ID_INFO parentId{};
    if (!GetFileInformationByHandleEx(parent.get(), FileIdInfo, &parentId, sizeof(parentId)))
        return false;
    const auto journalPath = base + L"\\journal";
    const HANDLE rawJournal = CreateFileW(journalPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, &attributes, CREATE_NEW, 0, nullptr);
    if (rawJournal == INVALID_HANDLE_VALUE)
        return false;
    const Handle journal(rawJournal);
    auto recover = [&] { return UacEfsWorkspace::Recover(parent.get(), parentId, journal.get(), rawSecurity); };

    const auto targetPath = folder + L"\\destination";
    const HANDLE rawTarget = CreateFileW(targetPath.c_str(), FILE_TRAVERSE | FILE_ADD_FILE | READ_CONTROL, share, nullptr,
                                          OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (rawTarget == INVALID_HANDLE_VALUE)
        return false;
    const Handle target(rawTarget);
    UacDirectoryLeaf::ParentIdentity targetId;
    if (UacDirectoryLeaf::ReadParentIdentity(target.get(), targetId))
        return false;
    const auto referencePath = targetPath + L"\\ordinary-reference.txt";
    if (!WriteFixture(referencePath, {}))
        return false;
    const HANDLE rawReference = CreateFileW(referencePath.c_str(), READ_CONTROL, share, nullptr, OPEN_EXISTING, 0, nullptr);
    if (rawReference == INVALID_HANDLE_VALUE)
        return false;
    const Handle reference(rawReference);
    UacEfsWorkspace::Detail::FileSecurity referenceSecurity;
    if (referenceSecurity.Read(reference.get()))
        return false;

    FILE_BASIC_INFO sourceTimes{};
    if (!GetFileInformationByHandleEx(source, FileBasicInfo, &sourceTimes, sizeof(sourceTimes)))
        return false;
    enum class Scenario { Locked, Substitute, Collision, GuardedClose, Publish };
    for (const auto scenario : {Scenario::Locked, Scenario::Substitute, Scenario::Collision,
                                Scenario::GuardedClose, Scenario::Publish})
    {
        UacEfsWorkspace::Workspace workspace;
        DWORD error = workspace.Create(parent.get(), parentId, journal.get(), source, sourceId, rawSecurity);
        Check(!error, "create and journal a private EFS workspace with a verified source link", error);
        if (error)
            return false;
        error = workspace.CreatePayload(rawSecurity);
        Check(!error, "durably record the private payload before allowing raw import", error);
        if (error)
            return false;
        std::wstring stage(32768, L'\0');
        length = GetFinalPathNameByHandleW(workspace.Directory(), stage.data(), static_cast<DWORD>(stage.size()), FILE_NAME_OPENED);
        if (!length || length >= stage.size())
            return false;
        stage.resize(length);
        const auto sourceLink = stage + L"\\" + UacEfsWorkspace::sourceName;
        const auto payload = stage + L"\\" + UacEfsWorkspace::payloadName;
        const auto held = base + L"\\displaced.efs";
        std::wstring guidStage(32768, L'\0');
        length = GetFinalPathNameByHandleW(workspace.Directory(), guidStage.data(), static_cast<DWORD>(guidStage.size()),
                                          VOLUME_NAME_GUID | FILE_NAME_OPENED);
        if (!length || length >= guidStage.size())
            return false;
        guidStage.resize(length);
        std::vector<BYTE> raw;
        {
            UacEfsRaw::Context input;
            error = input.Open((guidStage + L"\\" + UacEfsWorkspace::sourceName).c_str(), 0);
            if (!error)
                error = input.Read(raw, 32 * 1024 * 1024, MAXULONGLONG);
        }
        if (error)
            return false;
        {
            UacEfsRaw::Context output;
            error = output.Open((guidStage + L"\\" + UacEfsWorkspace::payloadName).c_str(), CREATE_FOR_IMPORT);
            if (!error)
                error = output.Write(raw, MAXULONGLONG);
        }
        const DWORD payloadAttributes = GetFileAttributesW(payload.c_str());
        Check(!error && payloadAttributes != INVALID_FILE_ATTRIBUTES &&
                  (payloadAttributes & FILE_ATTRIBUTE_ENCRYPTED) && HasContents(payload, plaintext),
              "transfer ciphertext between the verified workspace link and payload", error);
        if (error)
            return false;
        if (scenario == Scenario::Publish || scenario == Scenario::Collision || scenario == Scenario::GuardedClose)
        {
            constexpr wchar_t leaf[] = L"\u6771-efs-copy.txt";
            const auto final = targetPath + L"\\" + leaf;
            auto publish = [&](const UacDirectoryLeaf::ParentIdentity& identity, ULONGLONG deadline = MAXULONGLONG) {
                return UacEfsWorkspace::PublishPayload(workspace, target.get(), identity, leaf, plaintext.size(),
                                                        sourceTimes, nullptr, deadline);
            };
            auto wrong = targetId;
            wrong.file.FileId.Identifier[0] ^= 1;
            error = publish(wrong);
            Check(error == ERROR_FILE_INVALID && Exists(payload) && !Exists(final),
                  "refuse publishing EFS data into a different destination parent", error);
            error = publish(targetId, GetTickCount64());
            Check(error == ERROR_TIMEOUT && Exists(payload) && !Exists(final),
                  "cancel an expired EFS publication before changing the destination", error);
            if (scenario == Scenario::Collision)
            {
                if (!WriteFixture(final, {9}))
                    return false;
                error = publish(targetId);
                Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && HasContents(final, {9}),
                      "leave an existing destination unchanged on EFS publication collision", error);
                if (!DeleteFileW(final.c_str()))
                    return false;
            }
            else if (scenario == Scenario::GuardedClose)
            {
                UacEfsWorkspace::Detail::Handle guarded(nullptr, CloseHandle);
                error = UacEfsWorkspace::Detail::OpenPublicationPayload(workspace, guarded);
                if (!error)
                    error = UacRenameLeaf::Detail::NativeRename(guarded.get(), target.get(), leaf);
                Check(!error && Exists(final), "rename the guarded EFS object before finalizing its permissions", error);
                guarded.reset();
                Check(!Exists(final) && !Exists(payload), "delete an uncommitted EFS object when its guarded handle closes");
            }
            else
            {
                error = publish(targetId);
                const HANDLE rawFinal = CreateFileW(final.c_str(), READ_CONTROL | FILE_READ_ATTRIBUTES, share, nullptr,
                                                    OPEN_EXISTING, 0, nullptr);
                const Handle finalHandle(rawFinal == INVALID_HANDLE_VALUE ? nullptr : rawFinal);
                FILE_BASIC_INFO finalTimes{};
                Check(finalHandle && GetFileInformationByHandleEx(finalHandle.get(), FileBasicInfo, &finalTimes, sizeof(finalTimes)) &&
                          finalTimes.CreationTime.QuadPart == sourceTimes.CreationTime.QuadPart &&
                          finalTimes.LastWriteTime.QuadPart == sourceTimes.LastWriteTime.QuadPart &&
                          finalTimes.LastAccessTime.QuadPart == sourceTimes.LastAccessTime.QuadPart,
                      "preserve all three EFS timestamps before the copy is opened");
                const DWORD finalAttrs = GetFileAttributesW(final.c_str());
                FILE_ID_INFO finalId{};
                const auto identity = OpenIdentity(final, finalId);
                Check(!error && identity && UacRenameLeaf::Detail::SameSource(workspace.PayloadId(), finalId) &&
                          finalAttrs != INVALID_FILE_ATTRIBUTES && (finalAttrs & FILE_ATTRIBUTE_ENCRYPTED) &&
                          HasContents(final, plaintext),
                      "publish only the verified encrypted payload and preserve the owner's data", error);
                UacEfsWorkspace::Detail::FileSecurity finalSecurity;
                Check(finalHandle && !finalSecurity.Read(finalHandle.get()) && referenceSecurity.Matches(finalSecurity),
                      "match the owner, group, inherited DACL and integrity label of an ordinary destination file");
            }
            error = workspace.Cleanup();
            Check(!error && !DirectoryExists(stage), "retire only the EFS workspace after publication or collision", error);
            if (scenario == Scenario::Publish)
                Check(HasContents(final, plaintext), "workspace cleanup leaves the published EFS copy intact");
            continue;
        }
        if (scenario == Scenario::Substitute)
        {
            if (!MoveFileW(payload.c_str(), held.c_str()) || !WriteFixture(payload, {9}))
                return false;
            UacEfsWorkspace::Detail::Handle guarded(nullptr, CloseHandle);
            error = UacEfsWorkspace::Detail::OpenPublicationPayload(workspace, guarded);
            Check(error == ERROR_FILE_INVALID && !guarded && HasContents(payload, {9}),
                  "refuse to arm deletion on a substituted EFS payload", error);
            error = workspace.Cleanup();
            Check(error == ERROR_FILE_INVALID && HasContents(payload, {9}) && Exists(sourceLink),
                  "refuse a substituted payload before deleting any recorded EFS object", error);
            error = recover();
            Check(error == ERROR_FILE_INVALID && HasContents(payload, {9}) && Exists(sourceLink),
                  "recovery also refuses a substitute with the recorded payload name", error);
            if (!DeleteFileW(payload.c_str()) || !MoveFileW(held.c_str(), payload.c_str()))
                return false;
        }
        else
        {
            const HANDLE rawLock = CreateFileW(payload.c_str(), FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
            if (rawLock == INVALID_HANDLE_VALUE)
                return false;
            const Handle lock(rawLock);
            error = workspace.Cleanup();
            Check(error == ERROR_SHARING_VIOLATION && Exists(payload) && Exists(sourceLink),
                  "leave a journaled EFS workspace when cleanup is interrupted", error);
        }
        error = recover();
        FILE_STANDARD_INFO info{};
        Check(!error && !DirectoryExists(stage) &&
                  GetFileInformationByHandleEx(source, FileStandardInfo, &info, sizeof(info)) && info.NumberOfLinks == 1,
              "recover only the recorded EFS workspace and keep the original link", error);
        if (error)
            return false;
    }
    constexpr wchar_t encryptedLeaf[] = L"\u6771-restored.txt";
    constexpr wchar_t copiedLeaf[] = L"\u6771-integrated-copy.txt";
    const auto copied = targetPath + L"\\" + copiedLeaf;
    const HANDLE rawSourceParent = CreateFileW(folder.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, share, nullptr,
                                              OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (rawSourceParent == INVALID_HANDLE_VALUE)
        return false;
    const Handle sourceParent(rawSourceParent);
    UacDirectoryLeaf::ParentIdentity sourceParentId;
    if (UacDirectoryLeaf::ReadParentIdentity(sourceParent.get(), sourceParentId))
        return false;
    auto copy = [&](ULONGLONG expectedSize) {
        return UacEfsWorkspace::CopyVerifiedFile(sourceParent.get(), sourceParentId, sourceId, encryptedLeaf,
                                                 target.get(), targetId, copiedLeaf, expectedSize, parent.get(), parentId,
                                                 journal.get(), rawSecurity, nullptr, MAXULONGLONG);
    };
    DWORD result = copy(plaintext.size() + 1);
    Check(result == ERROR_FILE_INVALID && !Exists(copied), "reject a stale EFS size before starting the raw transfer", result);
    const auto originalPath = folder + L"\\" + encryptedLeaf;
    const DWORD originalAttrs = GetFileAttributesW(originalPath.c_str());
    if (originalAttrs == INVALID_FILE_ATTRIBUTES ||
        !SetFileAttributesW(originalPath.c_str(), originalAttrs | FILE_ATTRIBUTE_HIDDEN))
        return false;
    result = copy(plaintext.size());
    const DWORD copiedAttrs = GetFileAttributesW(copied.c_str());
    Check(!result && copiedAttrs != INVALID_FILE_ATTRIBUTES &&
              (copiedAttrs & (FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_HIDDEN)) ==
                  (FILE_ATTRIBUTE_ENCRYPTED | FILE_ATTRIBUTE_HIDDEN) &&
              HasContents(copied, plaintext),
          "transfer and publish an encrypted file through the complete verified EFS path", result);

    result = copy(plaintext.size());
    Check((result == ERROR_FILE_EXISTS || result == ERROR_ALREADY_EXISTS) && HasContents(copied, plaintext),
          "the complete EFS transfer does not overwrite an existing file", result);
    LARGE_INTEGER size{};
    Check(GetFileSizeEx(journal.get(), &size) && !size.QuadPart, "clear the EFS journal after verified cleanup");
    return failures == 0;
}

static bool CheckEfsRaw(const std::wstring& folder, const wchar_t* fixturePath)
{
    const DWORD attrs = GetFileAttributesW(fixturePath);
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_ENCRYPTED) ||
        (attrs & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        return false;
    constexpr DWORD share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    auto open = [&](const wchar_t* path, DWORD access, DWORD sharing) {
        const HANDLE raw = CreateFileW(path, access, sharing, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        return Handle(raw == INVALID_HANDLE_VALUE ? nullptr : raw);
    };
    std::vector<BYTE> plaintext;
    {
        const auto original = open(fixturePath, GENERIC_READ, share);
        LARGE_INTEGER size{};
        if (!original || !GetFileSizeEx(original.get(), &size) || size.QuadPart < 1 || size.QuadPart > 1024 * 1024)
            return false;
        plaintext.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        if (!ReadFile(original.get(), plaintext.data(), static_cast<DWORD>(plaintext.size()), &read, nullptr) ||
            read != plaintext.size())
            return false;
    }

    std::vector<BYTE> raw;
    {
        UacEfsRaw::Context input;
        const DWORD openError = input.Open(fixturePath, 0);
        if (openError)
            return false;
        const DWORD error = input.Read(raw, 32 * 1024 * 1024, MAXULONGLONG);
        Check(!error && !raw.empty() && raw != plaintext, "export encrypted data without producing the plaintext", error);
        if (error)
            return false;
    }

    const auto restored = folder + L"\\\u6771-restored.txt";
    if (!WriteFixture(restored, {}))
        return false;
    FILE_ID_INFO prior{}, current{};
    {
        const auto stage = OpenIdentity(restored, prior);
        if (!stage)
            return false;
    }
    // Use the volume-qualified form accepted by EFS Raw instead of the
    // extended DOS path used by other native filesystem tests.
    std::wstring volumePath(32768, L'\0');
    {
        const auto stage = open(restored.c_str(), FILE_READ_ATTRIBUTES, share);
        const DWORD length = stage ? GetFinalPathNameByHandleW(stage.get(), volumePath.data(),
            static_cast<DWORD>(volumePath.size()), VOLUME_NAME_GUID) : 0;
        if (!length || length >= volumePath.size())
            return false;
        volumePath.resize(length);
    }
    {
        UacEfsRaw::Context output;
        DWORD error = output.Open(volumePath.c_str(), CREATE_FOR_IMPORT);
        if (!error)
            error = output.Write(raw, MAXULONGLONG);
        Check(!error, "import raw data to a volume-qualified staged filename", error);
        if (error)
            return false;
    }

    const auto restoredIdentity = OpenIdentity(restored, current);
    const DWORD restoredAttrs = GetFileAttributesW(restored.c_str());
    Check(restoredIdentity && UacRenameLeaf::Detail::SameSource(prior, current) &&
              restoredAttrs != INVALID_FILE_ATTRIBUTES && (restoredAttrs & FILE_ATTRIBUTE_ENCRYPTED),
          "retain the staged identity and EFS encryption after import");
    Check(HasContents(restored, plaintext), "the original EFS key holder reads the same bytes from the restored file");
    Check(HasContents(fixturePath, plaintext), "raw transfer leaves the original readable");
    if (!CheckEfsWorkspace(folder, restoredIdentity.get(), current, plaintext))
        return false;

    {
        const auto guard = open(restored.c_str(), DELETE, FILE_SHARE_READ);
        if (!guard)
            return false;
        UacEfsRaw::Context input;
        const DWORD error = input.Open(volumePath.c_str(), 0);
        Check(error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION,
              "EFS Raw cannot open a source pinned by a separate delete handle", error);
    }
    {
        const auto writer = open(restored.c_str(), FILE_WRITE_DATA, share);
        if (!writer)
            return false;
        UacEfsRaw::Context input;
        const DWORD error = input.Open(volumePath.c_str(), 0);
        Check(error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION,
              "EFS Raw cannot open while a writer already owns the encrypted file", error);
    }
    {
        UacEfsRaw::Context input;
        if (input.Open(volumePath.c_str(), 0))
            return false;
        const auto competitor = open(restored.c_str(), DELETE, share);
        const DWORD competingError = competitor ? ERROR_SUCCESS : GetLastError();
        const auto identity = open(restored.c_str(), FILE_READ_ATTRIBUTES, share);
        Check(!competitor && competingError == ERROR_SHARING_VIOLATION && identity,
              "an open EFS Raw context blocks deletion but allows reading identity", competingError);

        const auto writer = open(restored.c_str(), FILE_WRITE_DATA, share);
        const DWORD writerError = writer ? ERROR_SUCCESS : GetLastError();
        Check(!writer && writerError == ERROR_SHARING_VIOLATION,
              "an open EFS Raw export excludes a concurrent data writer", writerError);

        std::vector<BYTE> limited;
        const DWORD error = input.Read(limited, 1, MAXULONGLONG);
        Check(error == ERROR_FILE_TOO_LARGE && limited.empty(), "discard an oversized raw export", error);
    }
    {
        UacEfsRaw::Context input;
        if (input.Open(volumePath.c_str(), 0))
            return false;
        std::vector<BYTE> expired;
        const DWORD error = input.Read(expired, raw.size(), GetTickCount64());
        Check(error == ERROR_TIMEOUT && expired.empty(), "reject an expired raw export", error);
    }
    {
        HANDLE reader = nullptr, writer = nullptr;
        if (!CreatePipe(&reader, &writer, nullptr, 0))
            return false;
        const Handle connection(reader);
        CloseHandle(writer);
        UacEfsRaw::Context input;
        if (input.Open(volumePath.c_str(), 0))
            return false;
        std::vector<BYTE> interrupted;
        const DWORD error = input.Read(interrupted, raw.size(), MAXULONGLONG, connection.get());
        Check(error == ERROR_BROKEN_PIPE && interrupted.empty(), "discard EFS raw bytes when the host disconnects", error);
    }
    {
        const HANDLE rawParent = CreateFileW(folder.c_str(), FILE_ADD_FILE | FILE_READ_ATTRIBUTES,
            share, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (rawParent == INVALID_HANDLE_VALUE)
            return false;
        const Handle parent(rawParent);
        constexpr wchar_t name[] = L"raw-alias.txt";
        struct
        {
            BOOLEAN replaceIfExists;
            HANDLE rootDirectory;
            ULONG fileNameLength;
            WCHAR fileName[ARRAYSIZE(name)];
        } link{};
        link.rootDirectory = parent.get();
        link.fileNameLength = sizeof(name) - sizeof(wchar_t);
        std::memcpy(link.fileName, name, sizeof(name));
        IO_STATUS_BLOCK io{};
        const NTSTATUS status = NtSetInformationFile(restoredIdentity.get(), &io, &link, sizeof(link),
                                                     static_cast<FILE_INFORMATION_CLASS>(11));
        FILE_ID_INFO aliasId{};
        const auto alias = OpenIdentity(folder + L"\\" + name, aliasId);
        Check(status >= 0 && alias && UacRenameLeaf::Detail::SameSource(current, aliasId),
              "create a hard link to the EFS file from its verified identity handle",
              status < 0 ? RtlNtStatusToDosError(status) : ERROR_SUCCESS);
        if (status < 0 || !alias)
            return false;

        const auto moved = folder + L"\\moved-original.txt";
        if (!MoveFileExW(restored.c_str(), moved.c_str(), 0) || !WriteFixture(restored, {7}))
            return false;
        const auto volumeAlias = volumePath.substr(0, volumePath.find_last_of(L'\\') + 1) + name;
        UacEfsRaw::Context input;
        DWORD error = input.Open(volumeAlias.c_str(), 0);
        std::vector<BYTE> aliasData;
        if (!error)
            error = input.Read(aliasData, 32 * 1024 * 1024, MAXULONGLONG);
        Check(!error && aliasData == raw && HasContents(restored, {7}),
              "export the verified EFS alias after a different file takes the former name", error);
    }
    return failures == 0;
}

int wmain(int argc, wchar_t** argv)
{
    using namespace UacDirectoryLeaf;
    const bool high = argc == 3 && std::wstring_view(argv[2]) == L"--high-integrity";
    const bool overwrite = argc == 3 && std::wstring_view(argv[2]) == L"--overwrite-probe";
    const bool moveSource = argc == 3 && std::wstring_view(argv[2]) == L"--move-source-probe";
    const bool efsRaw = argc == 4 && std::wstring_view(argv[2]) == L"--efs-source";
    if (argc != 2 && !high && !overwrite && !moveSource && !efsRaw)
        return 2;
    const std::wstring fixture = L"\\\\?\\" + std::wstring(argv[1]);
    if (overwrite)
        return CheckOverwrite(fixture) ? 0 : 1;
    if (efsRaw)
        return CheckEfsRaw(fixture, argv[3]) ? 0 : 1;
    const HANDLE rawRoot = CreateFileW(fixture.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                        nullptr);
    if (rawRoot == INVALID_HANDLE_VALUE)
        return 2;
    const Handle root(rawRoot);
    if (moveSource)
        return CheckMoveSource(fixture, root.get()) && CheckMoveSourceGuards(fixture, root.get()) ? 0 : 1;
    if (!CheckFindTagStream() || !CheckDetailedInventory(fixture, root.get()) || !CheckViewerSnapshot(fixture, root.get()) || !CheckDirectoryMetadata(fixture, root.get()) || !CheckTreeCopy(fixture, root.get(), high) ||
        !CheckZipSelection(fixture, root.get()) || !CheckReplacement(fixture, root.get(), UacZipUpdate::ReplacementKind::Zip) ||
        !CheckReplacement(fixture, root.get(), UacZipUpdate::ReplacementKind::DiskCopy))
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
    std::wstring stagedName;
    copyError = UacCopyLeaf::Detail::StageFile(original.get(), rawUncommitted, stagedName, false);
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
    const auto mainOnly = moved + L"\\copy-main-only.bin";
    copyError = UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, copyId, L"copy-\u65e5.bin",
                                             original.get(), expected, L"copy-main-only.bin", nullptr, nullptr,
                                             MAXULONGLONG, content.size(), UacCopyLeaf::StreamPolicy::IgnoreNamed);
    Check(copyError == ERROR_SUCCESS && HasContents(mainOnly, content) &&
              ProbeAccess(mainOnly + L":named", GENERIC_READ, 0) == ERROR_FILE_NOT_FOUND &&
              HasContents(copySource, content) && HasContents(copySource + L":named", existing),
          "copy the main stream only after explicit selection and preserve both original streams", copyError);
    const auto preserved = moved + L"\\copy-preserved-streams.bin";
    const auto unicodeStream = L":data-\u6771\u4eac";
    if (!WriteFixture(copySource + unicodeStream, {}) || !WriteFixture(copySource + L":second", {7, 9}))
        return 2;
    const auto preserve = [&](UacCopyLeaf::StageVerifier verifier) {
        return UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, copyId, L"copy-\u65e5.bin",
            original.get(), expected, L"copy-preserved-streams.bin", nullptr, nullptr, MAXULONGLONG,
            content.size(), UacCopyLeaf::StreamPolicy::PreserveNamed, verifier);
    };
    copyError = preserve(nullptr);
    Check(copyError == ERROR_ACCESS_DENIED && !Exists(preserved),
          "refuse to copy named streams into an unverified stage", copyError);
    const auto trustedFixture = [](HANDLE, bool) { return true; }; // The native fixture is private to the test.
    {
        const HANDLE rawNamed = CreateFileW((copySource + L":named").c_str(), FILE_WRITE_DATA,
                                            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (rawNamed == INVALID_HANDLE_VALUE)
            return 2;
        const Handle writer(rawNamed);
        copyError = preserve(trustedFixture);
        Check(copyError == ERROR_SHARING_VIOLATION && !Exists(preserved),
              "refuse an active writer on a named stream before staging", copyError);
    }
    copyError = preserve(trustedFixture);
    Check(copyError == ERROR_SUCCESS && HasContents(preserved, content) &&
              HasContents(preserved + L":named", existing) && HasContents(preserved + unicodeStream, {}) &&
              HasContents(preserved + L":second", {7, 9}) && HasContents(copySource + L":named", existing),
          "publish multiple named streams including an empty Unicode stream", copyError);
    copyError = preserve(trustedFixture);
    Check(copyError == ERROR_ALREADY_EXISTS && HasContents(preserved + L":named", existing),
          "leave the existing file and its streams intact on collision", copyError);

    constexpr DWORD sparseLength = 4 * 1024 * 1024;
    const auto sparseSource = fixture + L"\\destination\\sparse-\u6771.bin";
    const auto sparseTarget = moved + L"\\sparse-\u6771.bin";
    std::vector<BYTE> sparseBytes(sparseLength);
    sparseBytes[0] = 17;
    sparseBytes[2 * 1024 * 1024] = 29;
    {
        const HANDLE raw = CreateFileW(sparseSource.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return 2;
        const Handle file(raw);
        DWORD count = 0;
        LARGE_INTEGER position{};
        if (!DeviceIoControl(raw, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &count, nullptr) ||
            !WriteFile(raw, sparseBytes.data(), 1, &count, nullptr) || count != 1)
            return 2;
        position.QuadPart = 2 * 1024 * 1024;
        if (!SetFilePointerEx(raw, position, nullptr, FILE_BEGIN) ||
            !WriteFile(raw, sparseBytes.data() + position.QuadPart, 1, &count, nullptr) || count != 1)
            return 2;
        position.QuadPart = sparseLength;
        if (!SetFilePointerEx(raw, position, nullptr, FILE_BEGIN) || !SetEndOfFile(raw))
            return 2;
    }
    FILE_ID_INFO sparseId{};
    if (!OpenIdentity(sparseSource, sparseId))
        return 2;
    const auto copySparse = [&](std::wstring_view leaf) {
        return UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, sparseId, L"sparse-\u6771.bin",
            original.get(), expected, leaf, nullptr, nullptr, MAXULONGLONG, sparseLength,
            UacCopyLeaf::StreamPolicy::PreserveNamed);
    };
    copyError = copySparse(L"sparse-\u6771.bin");
    Check(copyError == ERROR_SUCCESS && HasContents(sparseTarget, sparseBytes) && HasContents(sparseSource, sparseBytes),
          "preserve sparse data and logical length including the trailing hole", copyError);
    const auto hasHole = [](const std::wstring& path) {
        const HANDLE raw = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return false;
        const Handle file(raw);
        FILE_ALLOCATED_RANGE_BUFFER query{}, range{};
        query.FileOffset.QuadPart = 1024 * 1024;
        query.Length.QuadPart = 512 * 1024;
        DWORD count = 0;
        return DeviceIoControl(raw, FSCTL_QUERY_ALLOCATED_RANGES, &query, sizeof(query), &range, sizeof(range),
                               &count, nullptr) && count == 0;
    };
    const DWORD sparseAttributes = GetFileAttributesW(sparseTarget.c_str());
    Check(sparseAttributes != INVALID_FILE_ATTRIBUTES && (sparseAttributes & FILE_ATTRIBUTE_SPARSE_FILE) &&
              hasHole(sparseSource) && hasHole(sparseTarget),
          "preserve a genuinely unallocated NTFS range and the sparse attribute", copyError);
    if (!WriteFixture(sparseSource + L":named", existing))
        return 2;
    copyError = copySparse(L"sparse-with-ads.bin");
    Check(copyError == ERROR_NOT_SUPPORTED && !Exists(moved + L"\\sparse-with-ads.bin") &&
              HasContents(sparseSource + L":named", existing),
          "reject sparse files with named streams without publishing a partial copy", copyError);

    const auto compressedSource = fixture + L"\\destination\\compressed-\u6771.bin";
    const auto compressedTarget = moved + L"\\compressed-\u6771.bin";
    const std::vector<BYTE> compressedBytes(128 * 1024, 17);
    if (!WriteFixture(compressedSource, compressedBytes))
        return 2;
    {
        const HANDLE raw = CreateFileW(compressedSource.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                       OPEN_EXISTING, 0, nullptr);
        if (raw == INVALID_HANDLE_VALUE)
            return 2;
        const Handle file(raw);
        USHORT format = COMPRESSION_FORMAT_LZNT1;
        DWORD count = 0;
        if (!DeviceIoControl(raw, FSCTL_SET_COMPRESSION, &format, sizeof(format), nullptr, 0, &count, nullptr))
            return 2;
    }
    if (!SetFileAttributesW(compressedSource.c_str(), FILE_ATTRIBUTE_HIDDEN))
        return 2;
    FILE_ID_INFO compressedId{};
    if (!OpenIdentity(compressedSource, compressedId))
        return 2;
    const auto copyCompressed = [&](std::wstring_view leaf, bool preserveCompression) {
        return UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, compressedId, L"compressed-\u6771.bin",
            original.get(), expected, leaf, nullptr, nullptr, MAXULONGLONG, compressedBytes.size(),
            preserveCompression ? UacCopyLeaf::StreamPolicy::RequireDefault : UacCopyLeaf::StreamPolicy::PreserveNamed,
            nullptr, preserveCompression);
    };
    copyError = copyCompressed(L"compressed-\u6771.bin", false);
    Check(copyError == ERROR_NOT_SUPPORTED && !Exists(compressedTarget),
          "refuse compressed files outside the explicit attributes request", copyError);
    copyError = copyCompressed(L"compressed-\u6771.bin", true);
    USHORT copiedFormat = COMPRESSION_FORMAT_NONE;
    {
        const HANDLE raw = CreateFileW(compressedTarget.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                       OPEN_EXISTING, 0, nullptr);
        if (raw != INVALID_HANDLE_VALUE)
        {
            const Handle file(raw);
            if (UacCopyLeaf::Detail::ReadCompression(raw, copiedFormat) != ERROR_SUCCESS)
                return 2;
        }
    }
    Check(copyError == ERROR_SUCCESS && copiedFormat == COMPRESSION_FORMAT_LZNT1 &&
              GetFileAttributesW(compressedTarget.c_str()) == (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_HIDDEN) &&
              HasContents(compressedTarget, compressedBytes) && HasContents(compressedSource, compressedBytes),
          "copy an explicitly selected LZNT1 file with exact attributes and contents", copyError);
    copyError = copyCompressed(L"compressed-\u6771.bin", true);
    Check(copyError == ERROR_ALREADY_EXISTS && HasContents(compressedTarget, compressedBytes),
          "preserve an existing compressed destination on collision", copyError);
    if (!WriteFixture(compressedSource + L":named", existing))
        return 2;
    copyError = copyCompressed(L"compressed-with-ads.bin", true);
    Check(copyError == ERROR_NOT_SUPPORTED && !Exists(moved + L"\\compressed-with-ads.bin") &&
              HasContents(compressedSource + L":named", existing),
          "refuse compressed files with named streams without publishing a partial copy", copyError);

    Check(UacF7::IsEfsCopyFailure(FILE_ATTRIBUTE_ENCRYPTED, ERROR_ACCESS_DENIED) &&
              UacF7::IsEfsCopyFailure(0, ERROR_NO_USER_KEYS) &&
              !UacF7::IsEfsCopyFailure(FILE_ATTRIBUTE_NORMAL, ERROR_ACCESS_DENIED) &&
              !UacF7::IsEfsCopyFailure(FILE_ATTRIBUTE_ENCRYPTED, ERROR_SHARING_VIOLATION),
          "classify protected EFS failures without masking ordinary access and sharing errors");

    const auto securitySource = fixture + L"\\destination\\security-\u6771.bin";
    const auto securityTarget = moved + L"\\security-\u4eac.bin";
    FILE_ID_INFO securityId{};
    if (!WriteFixture(securitySource, content) || !OpenIdentity(securitySource, securityId))
        return 2;
    const auto copySecurity = [&](UacCopyLeaf::StageVerifier verify) {
        return UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, securityId, L"security-\u6771.bin",
            original.get(), expected, L"security-\u4eac.bin", nullptr, nullptr, MAXULONGLONG,
            content.size(), UacCopyLeaf::StreamPolicy::RequireDefault, verify, false, true);
    };
    copyError = copySecurity(trustedFixture);
    Check(copyError == ERROR_NOT_SUPPORTED && !Exists(securityTarget),
          "security copy rejects an inherited DACL", copyError);
    PSECURITY_DESCRIPTOR rawSecurity = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"D:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;FR;;;BU)(A;;FA;;;OW)", SDDL_REVISION_1, &rawSecurity, nullptr))
        return 2;
    const std::unique_ptr<void, decltype(&LocalFree)> explicitSecurity(rawSecurity, LocalFree);
    PACL explicitDacl = nullptr;
    BOOL present = FALSE, defaulted = FALSE;
    if (!GetSecurityDescriptorDacl(rawSecurity, &present, &explicitDacl, &defaulted) || !present || !explicitDacl ||
        SetNamedSecurityInfoW(const_cast<wchar_t*>(securitySource.c_str()), SE_FILE_OBJECT,
                              DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                              nullptr, nullptr, explicitDacl, nullptr) != ERROR_SUCCESS)
        return 2;
    copyError = copySecurity([](HANDLE, bool) { return false; });
    Check(copyError == ERROR_ACCESS_DENIED && !Exists(securityTarget),
          "security copy rejects a file that fails the protection check", copyError);
    copyError = copySecurity(trustedFixture);
    const auto openSecurity = [](const std::wstring& path) {
        const HANDLE raw = CreateFileW(path.c_str(), READ_CONTROL, FILE_SHARE_READ, nullptr,
                                       OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        return Handle(raw == INVALID_HANDLE_VALUE ? nullptr : raw);
    };
    const auto originalSecurity = openSecurity(securitySource);
    const auto copiedSecurity = openSecurity(securityTarget);
    UacCopyLeaf::Detail::CopySecuritySnapshot securitySnapshot;
    Check(copyError == ERROR_SUCCESS && originalSecurity && copiedSecurity &&
              securitySnapshot.Read(originalSecurity.get()) == ERROR_SUCCESS &&
              securitySnapshot.Verify(copiedSecurity.get()) == ERROR_SUCCESS && HasContents(securityTarget, content),
          "copy the exact protected ACL, owner and primary group with the file data", copyError);
    copyError = copySecurity(trustedFixture);
    Check(copyError == ERROR_ALREADY_EXISTS && HasContents(securityTarget, content),
          "security copy refuses to replace an existing target", copyError);

    PSECURITY_DESCRIPTOR labelDescriptor = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            L"S:(ML;;NRNW;;;ME)", SDDL_REVISION_1, &labelDescriptor, nullptr))
        return 2;
    const std::unique_ptr<void, decltype(&LocalFree)> ownedLabel(labelDescriptor, LocalFree);
    PACL label = nullptr;
    if (!GetSecurityDescriptorSacl(labelDescriptor, &present, &label, &defaulted) || !present || !label ||
        SetNamedSecurityInfoW(const_cast<wchar_t*>(securitySource.c_str()), SE_FILE_OBJECT,
                              LABEL_SECURITY_INFORMATION, nullptr, nullptr, nullptr, label) != ERROR_SUCCESS)
        return 2;
    copyError = UacCopyLeaf::CopyVerifiedFile(listing.get(), listingIdentity, securityId, L"security-\u6771.bin",
        original.get(), expected, L"labeled-copy.bin", nullptr, nullptr, MAXULONGLONG,
        content.size(), UacCopyLeaf::StreamPolicy::RequireDefault, trustedFixture, false, true);
    Check(copyError == ERROR_NOT_SUPPORTED && !Exists(moved + L"\\labeled-copy.bin"),
          "security copy refuses an explicit mandatory integrity label", copyError);

    const auto moveSourcePath = fixture + L"\\destination\\move-\u65e5.bin";
    const auto moveTargetPath = moved + L"\\move-\u672c.bin";
    FILE_ID_INFO moveId{};
    if (!WriteFixture(moveSourcePath, content) || !OpenIdentity(moveSourcePath, moveId))
        return 2;
    const auto move = [&](HANDLE directory, const ParentIdentity& identity, std::wstring_view name, HANDLE pipe = nullptr)
    {
        return UacRenameLeaf::MoveVerifiedFile(listing.get(), listingIdentity, moveId, L"move-\u65e5.bin",
                                               directory, identity, name, UacRenameLeaf::MoveKind::OrdinaryFile, pipe);
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

    error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, verifiedId,
                                               L"verified-\u6771\u4eac", L"ordinary-as-link", UacRenameLeaf::MoveKind::RenameSymlink);
    Check(error == ERROR_REPARSE_TAG_INVALID && !Exists(moved + L"\\ordinary-as-link"),
          "the symbolic-link operation rejects an ordinary file", error);

    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, verifiedId, L"verified-\u6771\u4eac", UacF7::Operation::DeleteSymlink);
    Check(error == ERROR_REPARSE_TAG_INVALID && Exists(moved + L"\\verified-\u6771\u4eac"),
          "typed symbolic-link deletion rejects an ordinary file", error);

    const auto linkSource = moved + L"\\symbolic-\u6771";
    const auto linkTarget = moved + L"\\symbolic-\u4eac";
    const auto referent = moved + L"\\symbolic-referent";
    if (!WriteFixture(referent, content))
        return 2;
    if (CreateSymbolicLinkW(linkSource.c_str(), L"symbolic-referent", SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        FILE_ID_INFO linkId{};
        const auto pinnedLink = OpenIdentity(linkSource, linkId);
        if (!pinnedLink)
            return 2;
        const auto readLink = [](HANDLE file, std::vector<BYTE>& data) {
            data.resize(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
            DWORD bytes = 0;
            if (!DeviceIoControl(file, FSCTL_GET_REPARSE_POINT, nullptr, 0,
                                 data.data(), static_cast<DWORD>(data.size()), &bytes, nullptr))
                return false;
            data.resize(bytes);
            return true;
        };
        std::vector<BYTE> beforeLink, afterLink;
        if (!readLink(pinnedLink.get(), beforeLink))
            return 2;
        error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, linkId, L"symbolic-\u6771", L"symbolic-\u4eac");
        Check(error == ERROR_REPARSE_TAG_INVALID && Exists(linkSource) && !Exists(linkTarget),
              "ordinary rename still rejects a symbolic link", error);
        error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, linkId, L"symbolic-\u6771", L"symbolic-referent", UacRenameLeaf::MoveKind::RenameSymlink);
        Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) && Exists(linkSource) && HasContents(referent, content),
              "symbolic-link rename preserves both entries on collision", error);
        error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, verifiedId, L"symbolic-\u6771", L"symbolic-\u4eac", UacRenameLeaf::MoveKind::RenameSymlink);
        Check(error == ERROR_FILE_INVALID && Exists(linkSource) && !Exists(linkTarget),
              "symbolic-link rename rejects a different source identity", error);
        error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, linkId, L"symbolic-\u6771", L"symbolic-\u4eac", UacRenameLeaf::MoveKind::RenameSymlink);
        FILE_ID_INFO renamedLinkId{};
        const auto renamedLink = OpenIdentity(linkTarget, renamedLinkId);
        Check(error == ERROR_SUCCESS && !Exists(linkSource) && renamedLink &&
                  UacRenameLeaf::Detail::SameSource(linkId, renamedLinkId) &&
                  readLink(renamedLink.get(), afterLink) && beforeLink == afterLink && HasContents(referent, content),
              "rename the exact Unicode link and preserve its raw reparse data and referent", error);
        const auto dangling = moved + L"\\dangling-source";
        if (!CreateSymbolicLinkW(dangling.c_str(), L"missing-referent", SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
            return 2;
        FILE_ID_INFO danglingId{}, renamedDanglingId{};
        const auto pinnedDangling = OpenIdentity(dangling, danglingId);
        if (!pinnedDangling)
            return 2;
        error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, danglingId,
                                                   L"dangling-source", L"dangling-target", UacRenameLeaf::MoveKind::RenameSymlink);
        Check(error == ERROR_SUCCESS && !Exists(dangling) &&
                  OpenIdentity(moved + L"\\dangling-target", renamedDanglingId) &&
                  UacRenameLeaf::Detail::SameSource(danglingId, renamedDanglingId) &&
                  !Exists(moved + L"\\missing-referent"),
              "rename a dangling link without opening or creating its target", error);

        const auto dirSource = moved + L"\\directory-link-\u6771";
        const auto dirTarget = moved + L"\\directory-link-\u4eac";
        const auto dirReferent = moved + L"\\directory-referent";
        const auto sentinel = dirReferent + L"\\sentinel.txt";
        if (!CreateDirectoryW(dirReferent.c_str(), nullptr) || !WriteFixture(sentinel, content) ||
            !CreateSymbolicLinkW(dirSource.c_str(), L"directory-referent",
                                 SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
            return 2;
        FILE_ID_INFO directoryLinkId{}, renamedDirectoryLinkId{};
        const auto directoryLink = OpenIdentity(dirSource, directoryLinkId, true);
        if (!directoryLink || !readLink(directoryLink.get(), beforeLink))
            return 2;
        const auto dirKind = UacRenameLeaf::MoveKind::RenameDirectorySymlink;
        const auto renameDirectory = [&](const FILE_ID_INFO& id, std::wstring_view from, std::wstring_view to) {
            return UacRenameLeaf::RenameVerifiedFile(original.get(), expected, id, from, to, dirKind);
        };
        error = renameDirectory(renamedLinkId, L"symbolic-\u4eac", L"wrong-link-type");
        Check(error == ERROR_NOT_SUPPORTED && !Exists(moved + L"\\wrong-link-type"),
              "directory link rename rejects a file link", error);
        error = UacRenameLeaf::RenameVerifiedFile(original.get(), expected, directoryLinkId,
                                                   L"directory-link-\u6771", L"wrong-link-type",
                                                   UacRenameLeaf::MoveKind::RenameSymlink);
        Check(error != ERROR_SUCCESS && !Exists(moved + L"\\wrong-link-type"),
              "file link rename rejects a directory link", error);
        error = renameDirectory(directoryLinkId, L"directory-link-\u6771", L"directory-referent");
        Check((error == ERROR_ALREADY_EXISTS || error == ERROR_FILE_EXISTS) &&
                  DirectoryExists(dirSource) && HasContents(sentinel, content),
              "directory link rename cannot replace an existing directory", error);
        error = UacRenameLeaf::MoveVerifiedFile(original.get(), expected, directoryLinkId, L"directory-link-\u6771",
                                                 listing.get(), listingIdentity, L"moved-directory-link", dirKind);
        Check(error == ERROR_NOT_SUPPORTED && DirectoryExists(dirSource),
              "directory link cannot move to a different parent through rename", error);
        error = renameDirectory(directoryLinkId, L"directory-link-\u6771", L"directory-link-\u4eac");
        const auto renamedDirectory = OpenIdentity(dirTarget, renamedDirectoryLinkId, true);
        Check(error == ERROR_SUCCESS && !Exists(dirSource) && renamedDirectory &&
                  UacRenameLeaf::Detail::SameSource(directoryLinkId, renamedDirectoryLinkId) &&
                  readLink(renamedDirectory.get(), afterLink) && beforeLink == afterLink &&
                  HasContents(sentinel, content) && HasContents(dirTarget + L"\\sentinel.txt", content),
              "rename a Unicode directory link and preserve the link and its referent", error);

        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedDirectoryLinkId, L"directory-link-\u4eac", UacF7::Operation::DeleteSymlink);
        Check(error != ERROR_SUCCESS && DirectoryExists(dirTarget) && HasContents(sentinel, content),
              "typed file-link deletion refuses a directory symbolic link", error);
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedDirectoryLinkId, L"directory-link-\u4eac",
                                                UacF7::Operation::DeleteJunction);
        Check(error == ERROR_REPARSE_TAG_INVALID && DirectoryExists(dirTarget) && HasContents(sentinel, content),
              "typed junction deletion rejects a directory symbolic link", error);
        constexpr auto deleteDirectoryLink = UacF7::Operation::DeleteDirectorySymlink;
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedLinkId, L"symbolic-\u4eac", deleteDirectoryLink);
        Check(error == ERROR_NOT_SUPPORTED && Exists(linkTarget),
              "typed directory-link deletion refuses a file symbolic link", error);
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedLinkId, L"directory-link-\u4eac", deleteDirectoryLink);
        Check(error == ERROR_FILE_INVALID && DirectoryExists(dirTarget) && HasContents(sentinel, content),
              "typed directory-link deletion rejects a stale source identity", error);
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedDirectoryLinkId, L"directory-link-\u4eac", deleteDirectoryLink);
        Check(error == ERROR_SUCCESS && !Exists(dirTarget) && DirectoryExists(dirReferent) && HasContents(sentinel, content),
              "delete the Unicode directory link while its identity is held and preserve its contents at the target", error);
        const auto danglingDirectory = moved + L"\\dangling-directory-link";
        FILE_ID_INFO danglingDirectoryId{};
        if (!CreateSymbolicLinkW(danglingDirectory.c_str(), L"missing-directory",
                                 SYMBOLIC_LINK_FLAG_DIRECTORY | SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) ||
            !OpenIdentity(danglingDirectory, danglingDirectoryId, true))
            return 2;
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, danglingDirectoryId, L"dangling-directory-link", deleteDirectoryLink);
        Check(error == ERROR_SUCCESS && !Exists(danglingDirectory) && !Exists(moved + L"\\missing-directory"),
              "delete a dangling directory link without requiring its target", error);

        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedLinkId, L"symbolic-\u4eac");
        Check(error == ERROR_REPARSE_TAG_INVALID && Exists(linkTarget),
              "ordinary deletion refuses a file symbolic link", error);
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, verifiedId, L"symbolic-\u4eac", UacF7::Operation::DeleteSymlink);
        Check(error == ERROR_FILE_INVALID && Exists(linkTarget),
              "typed symbolic-link deletion rejects a stale source identity", error);
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedLinkId, L"symbolic-\u4eac", UacF7::Operation::DeleteSymlink);
        Check(error == ERROR_SUCCESS && !Exists(linkTarget) && HasContents(referent, content),
              "delete the Unicode link while its identity is held and preserve the linked file", error);
        error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, renamedDanglingId, L"dangling-target", UacF7::Operation::DeleteSymlink);
        Check(error == ERROR_SUCCESS && !Exists(moved + L"\\dangling-target") && !Exists(moved + L"\\missing-referent"),
              "delete a dangling file link without requiring its target", error);
    }
    else if (GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
        std::puts("SKIP: creating the symbolic-link fixture requires Developer Mode or SeCreateSymbolicLinkPrivilege");
    else
        return 2;

    ParentIdentity junctionParent;
    FILE_ID_INFO junctionId{};
    const auto pinnedJunction = OpenIdentity(fixture + L"\\junction", junctionId, true);
    if (!pinnedJunction || ReadParentIdentity(root.get(), junctionParent))
        return 2;
    error = UacRenameLeaf::RenameVerifiedFile(root.get(), junctionParent, junctionId, L"junction", L"junction-renamed",
                                               UacRenameLeaf::MoveKind::RenameDirectorySymlink);
    Check(error == ERROR_REPARSE_TAG_INVALID && DirectoryExists(fixture + L"\\junction") &&
              !Exists(fixture + L"\\junction-renamed"),
          "directory link rename rejects the junction tag", error);
    error = UacDeleteLeaf::DeleteVerifiedLeaf(root.get(), junctionParent, junctionId, L"junction",
                                            UacF7::Operation::DeleteDirectorySymlink);
    Check(error == ERROR_REPARSE_TAG_INVALID && DirectoryExists(fixture + L"\\junction"),
          "typed directory-link deletion refuses a junction", error);

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
    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, deleteId, L"delete-source");
    Check(error == ERROR_FILE_INVALID && Exists(deletePath) && Exists(relocatedDelete),
          "delete rejects a replacement source and preserves both files", error);
    error = UacDeleteLeaf::DeleteVerifiedLeaf(substitute.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_FILE_INVALID && Exists(relocatedDelete), "delete rejects a replacement parent", error);
    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, deleteId, L"delete-original:stream");
    Check(error == ERROR_INVALID_NAME && Exists(relocatedDelete), "delete refuses an alternate stream name", error);

    const auto deleteAlias = moved + L"\\delete-alias";
    if (!CreateHardLinkW(deleteAlias.c_str(), relocatedDelete.c_str(), nullptr))
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_NOT_SUPPORTED && Exists(relocatedDelete) && Exists(deleteAlias),
          "delete refuses an object with another hardlink", error);
    if (!DeleteFileW(deleteAlias.c_str()))
        return 2;
    if (!SetFileAttributesW(relocatedDelete.c_str(), FILE_ATTRIBUTE_READONLY))
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_NOT_SUPPORTED && GetFileAttributesW(relocatedDelete.c_str()) == FILE_ATTRIBUTE_READONLY,
          "delete leaves a read-only file and its attributes intact", error);
    if (!SetFileAttributesW(relocatedDelete.c_str(), FILE_ATTRIBUTE_NORMAL))
        return 2;
    const HANDLE reader = CreateFileW(relocatedDelete.c_str(), GENERIC_READ,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (reader == INVALID_HANDLE_VALUE)
        return 2;
    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, deleteId, L"delete-original");
    Check(error == ERROR_SHARING_VIOLATION && Exists(relocatedDelete), "delete refuses an open data reader", error);
    CloseHandle(reader);
    error = UacDeleteLeaf::DeleteVerifiedLeaf(original.get(), expected, deleteId, L"delete-original");
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
    using UacRenameLeaf::Detail::MountPointHeader;
    const auto mountData = [](std::wstring_view substitute) {
        std::vector<WCHAR> buffer(sizeof(MountPointHeader) / sizeof(WCHAR) + substitute.size());
        MountPointHeader info{};
        info.tag = IO_REPARSE_TAG_MOUNT_POINT;
        info.substituteLength = static_cast<USHORT>(substitute.size() * sizeof(WCHAR));
        info.dataLength = static_cast<USHORT>(sizeof(info) - offsetof(MountPointHeader, substituteOffset) +
                                              info.substituteLength);
        std::memcpy(buffer.data(), &info, sizeof(info));
        std::copy(substitute.begin(), substitute.end(), buffer.begin() + sizeof(info) / sizeof(WCHAR));
        return buffer;
    };
    const auto checkMountData = [](const std::vector<WCHAR>& data) {
        return UacRenameLeaf::Detail::CheckJunctionData(data.data(), data.size() * sizeof(WCHAR));
    };
    const auto volumeData = mountData(L"\\??\\Volume{01234567-89ab-cdef-0123-456789abcdef}\\");
    const DWORD volumeError = checkMountData(volumeData);
    Check(volumeError == ERROR_NOT_SUPPORTED, "reject the volume GUID namespace despite the matching mount-point tag", volumeError);
    const DWORD uncError = checkMountData(mountData(L"\\??\\UNC\\server\\share"));
    Check(uncError == ERROR_NOT_SUPPORTED, "reject an unsupported junction target namespace", uncError);
    auto malformed = mountData(L"\\??\\C:\\fixture");
    malformed[offsetof(MountPointHeader, substituteOffset) / sizeof(WCHAR)] = 1;
    const DWORD malformedError = checkMountData(malformed);
    Check(malformedError == ERROR_INVALID_REPARSE_DATA, "reject a misaligned substitute name before reading it", malformedError);
    malformed.pop_back();
    const DWORD truncated = checkMountData(malformed);
    Check(truncated == ERROR_INVALID_REPARSE_DATA, "reject a truncated mount-point buffer", truncated);

    const auto longJunction = longPath + L"\\junction-\u65e5";
    const auto renamedJunction = longPath + L"\\junction-\u672c";
    const auto junctionSentinel = fixture + L"\\destination\\junction-sentinel.txt";
    if (!CreateDirectoryW(longJunction.c_str(), nullptr) || !WriteFixture(junctionSentinel, content))
        return 2;
    DWORD donorBytes = 0;
    {
        const auto newJunction = openReparse(longJunction, GENERIC_WRITE);
        DWORD written = 0;
        if (!newJunction ||
            !DeviceIoControl(donor.get(), FSCTL_GET_REPARSE_POINT, nullptr, 0, tag.get(),
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE, &donorBytes, nullptr) ||
            !DeviceIoControl(newJunction.get(), FSCTL_SET_REPARSE_POINT, tag.get(), donorBytes, nullptr, 0, &written, nullptr))
            return 2;
    }
    DWORD junctionRename = ERROR_GEN_FAILURE;
    {
        FILE_ID_INFO beforeId{}, afterId{};
        const auto beforeJunction = OpenIdentity(longJunction, beforeId, true);
        if (!beforeJunction)
            return 2;
        junctionRename = UacRenameLeaf::RenameVerifiedFile(longParent.get(), expected, beforeId,
            L"junction-\u65e5", leaf, UacRenameLeaf::MoveKind::RenameJunction);
        Check((junctionRename == ERROR_ALREADY_EXISTS || junctionRename == ERROR_FILE_EXISTS) &&
                  DirectoryExists(longJunction) && DirectoryExists(longPath + L"\\" + leaf),
              "junction rename keeps both entries on collision", junctionRename);
        junctionRename = UacRenameLeaf::RenameVerifiedFile(longParent.get(), expected, beforeId,
            L"junction-\u65e5", L"junction-\u672c", UacRenameLeaf::MoveKind::RenameJunction);
        const auto afterJunction = OpenIdentity(renamedJunction, afterId, true);
        std::vector<BYTE> afterData(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        DWORD actualBytes = 0;
        Check(junctionRename == ERROR_SUCCESS && !Exists(longJunction) && afterJunction &&
                  UacRenameLeaf::Detail::SameSource(beforeId, afterId) &&
                  DeviceIoControl(afterJunction.get(), FSCTL_GET_REPARSE_POINT, nullptr, 0, afterData.data(),
                                  static_cast<DWORD>(afterData.size()), &actualBytes, nullptr) &&
                  actualBytes == donorBytes && std::memcmp(tag.get(), afterData.data(), donorBytes) == 0 &&
                  HasContents(junctionSentinel, content) && HasContents(renamedJunction + L"\\junction-sentinel.txt", content),
              "rename a Unicode junction below a long path without changing its identity, data or target", junctionRename);
        if (junctionRename == ERROR_SUCCESS && afterJunction)
        {
            constexpr auto deleteJunction = UacF7::Operation::DeleteJunction;
            const DWORD stale = UacDeleteLeaf::DeleteVerifiedLeaf(longParent.get(), expected, expected.file,
                                                                 L"junction-\u672c", deleteJunction);
            Check(stale == ERROR_FILE_INVALID && DirectoryExists(renamedJunction),
                  "typed junction deletion rejects a stale identity", stale);
            const DWORD deleted = UacDeleteLeaf::DeleteVerifiedLeaf(longParent.get(), expected, afterId,
                                                                   L"junction-\u672c", deleteJunction);
            Check(deleted == ERROR_SUCCESS && !Exists(renamedJunction) && HasContents(junctionSentinel, content),
                  "delete a Unicode junction below a long path and preserve the linked folder", deleted);
        }
    }
    const auto remainingJunction = junctionRename == ERROR_SUCCESS ? renamedJunction : longJunction;
    if (Exists(remainingJunction) && !RemoveDirectoryW(remainingJunction.c_str()))
        return 2;

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
    const DWORD longDelete = UacDeleteLeaf::DeleteVerifiedLeaf(longParent.get(), expected, longId, L"target-\u672c");
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
