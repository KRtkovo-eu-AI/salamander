// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Convert portable workspace identities for native tests. | Check the same 128-bit IDs through the isolated implementation. | #731 | GPT-6.1 Sol |
// | 2026-11-11 | Test a private generated file-list source. | Verify broker-compatible sharing and identity-safe publication. | #731 | GPT-6.1 Sol |
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <aclapi.h>
#include <cstdio>
#include <string>
#include <thread>
#include "../uacfilelist.h"
#include "../uacbroker/copyleaf.h"

namespace UacFileList
{
unsigned failCleanupPoint = 0;
unsigned failIdentityPoint = 0;
DWORD CleanupFaultForTest(unsigned point)
{
    return point == failCleanupPoint ? ERROR_ACCESS_DENIED : ERROR_SUCCESS;
}
DWORD IdentityFaultForTest(unsigned point)
{
    return point == failIdentityPoint ? ERROR_ACCESS_DENIED : ERROR_SUCCESS;
}
}

namespace
{
int failures = 0;
void Check(bool value, const char* message)
{
    std::printf("%s: %s\n", value ? "PASS" : "FAIL", message);
    if (!value) ++failures;
}
FILE_ID_INFO NativeIdentity(const UacF7::MoveFileIdentity& identity)
{
    FILE_ID_INFO native{};
    native.VolumeSerialNumber = identity.serial;
    memcpy(native.FileId.Identifier, identity.id, sizeof(identity.id));
    return native;
}
struct CleanupReport
{
    unsigned calls = 0;
    unsigned fallbackCalls = 0;
    DWORD error = 0;
    std::wstring path;
    bool fallbackPathMatches = false;
};
void Report(void* context, const std::wstring& path, DWORD error)
{
    auto& report = *static_cast<CleanupReport*>(context);
    ++report.calls; report.path = path; report.error = error;
}
void ThrowReport(void* context, const std::wstring&, DWORD)
{
    ++static_cast<CleanupReport*>(context)->calls;
    throw std::bad_alloc();
}
void FallbackReport(void* context, const std::wstring& path, DWORD error) noexcept
{
    auto& report = *static_cast<CleanupReport*>(context);
    ++report.fallbackCalls; report.error = error; report.fallbackPathMatches = path == report.path;
}
bool RemoveOwned(const std::wstring& path, const FILE_ID_INFO& expected, bool directory = false)
{
    const HANDLE file = CreateFileW(path.c_str(), DELETE | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | (directory ? FILE_FLAG_BACKUP_SEMANTICS : 0), nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    FILE_ID_INFO actual{};
    FILE_DISPOSITION_INFO remove{TRUE};
    const bool done = GetFileInformationByHandleEx(file, FileIdInfo, &actual, sizeof(actual)) &&
        actual.VolumeSerialNumber == expected.VolumeSerialNumber &&
        !memcmp(actual.FileId.Identifier, expected.FileId.Identifier, sizeof(actual.FileId.Identifier)) &&
        SetFileInformationByHandle(file, FileDispositionInfo, &remove, sizeof(remove));
    CloseHandle(file);
    return done;
}
bool PrivateAcl(HANDLE file)
{
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR descriptor = nullptr;
    const DWORD error = GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                        nullptr, nullptr, &dacl, nullptr, &descriptor);
    if (error || !dacl) { if (descriptor) LocalFree(descriptor); return false; }
    BYTE userBuffer[sizeof(TOKEN_USER) + SECURITY_MAX_SID_SIZE]{};
    BYTE systemBuffer[SECURITY_MAX_SID_SIZE]{}, adminsBuffer[SECURITY_MAX_SID_SIZE]{};
    DWORD used = 0, systemBytes = sizeof(systemBuffer), adminBytes = sizeof(adminsBuffer);
    HANDLE token = nullptr;
    bool user = false, system = false, admins = false;
    bool valid = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) &&
        GetTokenInformation(token, TokenUser, userBuffer, sizeof(userBuffer), &used) &&
        CreateWellKnownSid(WinLocalSystemSid, nullptr, systemBuffer, &systemBytes) &&
        CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, adminsBuffer, &adminBytes);
    if (token) CloseHandle(token);
    const PSID userSid = reinterpret_cast<TOKEN_USER*>(userBuffer)->User.Sid;
    const DWORD write = FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES |
                        DELETE | WRITE_DAC | WRITE_OWNER;
    for (DWORD index = 0; valid && index < dacl->AceCount; ++index)
    {
        void* raw = nullptr;
        if (!GetAce(dacl, index, &raw)) { valid = false; break; }
        const auto* header = static_cast<ACE_HEADER*>(raw);
        if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) { valid = false; break; }
        const auto* ace = static_cast<ACCESS_ALLOWED_ACE*>(raw);
        PSID sid = const_cast<DWORD*>(&ace->SidStart);
        if (EqualSid(sid, userSid)) user = (ace->Mask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS;
        else if (EqualSid(sid, systemBuffer)) system = (ace->Mask & FILE_ALL_ACCESS) == FILE_ALL_ACCESS;
        else if (EqualSid(sid, adminsBuffer)) admins = (ace->Mask & (FILE_READ_DATA | FILE_READ_EA)) ==
            (FILE_READ_DATA | FILE_READ_EA) && !(ace->Mask & write);
        else valid = false;
    }
    LocalFree(descriptor);
    return valid && user && system && admins;
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring target = argv[1];
    if (!CreateDirectoryW(target.c_str(), nullptr)) return 3;
    const HANDLE targetDir = CreateFileW(target.c_str(), FILE_READ_ATTRIBUTES | FILE_ADD_FILE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (targetDir == INVALID_HANDLE_VALUE) return 4;
    UacDirectoryLeaf::ParentIdentity targetId{};
    if (UacDirectoryLeaf::ReadParentIdentity(targetDir, targetId)) return 5;
    std::wstring workspacePath;
    {
        UacFileList::Workspace workspace;
        const DWORD created = workspace.Create();
        Check(!created, "create private workspace and source");
        if (!created)
        {
            workspacePath = workspace.Path();
            const char content[] = "first\r\nsecond\r\n";
            DWORD written = 0;
            Check(WriteFile(workspace.Writer(), content, sizeof(content) - 1, &written, nullptr) &&
                      written == sizeof(content) - 1, "write the generated content");
            Check(!workspace.Seal() && workspace.Size() == sizeof(content) - 1, "seal the exact generated content");
            HANDLE writer = CreateFileW(workspacePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            Check(writer == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION,
                  "sealed source denies an extra writer");
            if (writer != INVALID_HANDLE_VALUE) CloseHandle(writer);
            const std::wstring parent = workspacePath.substr(0, workspacePath.find_last_of(L'\\'));
            const HANDLE sourceDir = CreateFileW(parent.c_str(), FILE_READ_ATTRIBUTES | FILE_TRAVERSE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            Check(sourceDir != INVALID_HANDLE_VALUE, "open the source parent with broker-compatible sharing");
            if (sourceDir != INVALID_HANDLE_VALUE)
            {
                HANDLE readableAcl = CreateFileW(parent.c_str(), READ_CONTROL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
                Check(readableAcl != INVALID_HANDLE_VALUE && PrivateAcl(readableAcl),
                      "workspace ACL grants only caller and system full access and administrators read access");
                if (readableAcl != INVALID_HANDLE_VALUE) CloseHandle(readableAcl);
                UacDirectoryLeaf::ParentIdentity sourceId{};
                const DWORD parentStatus = UacDirectoryLeaf::ReadParentIdentity(sourceDir, sourceId);
                Check(!parentStatus && UacDirectoryLeaf::SameParent(sourceId,
                    UacDirectoryLeaf::ParentIdentity{NativeIdentity(workspace.ParentIdentity()), sourceId.volume}),
                    "source parent matches pinned identity");
                const DWORD copied = parentStatus ? parentStatus : UacCopyLeaf::CopyVerifiedFile(
                    sourceDir, sourceId, NativeIdentity(workspace.Identity()), L"list.tmp", targetDir, targetId,
                    L"result.txt", nullptr, nullptr, MAXULONGLONG, workspace.Size(),
                    UacCopyLeaf::StreamPolicy::IgnoreNamed);
                Check(!copied, "real broker copy primitive publishes the sealed source");
                const DWORD collision = UacCopyLeaf::CopyVerifiedFile(sourceDir, sourceId,
                    NativeIdentity(workspace.Identity()), L"list.tmp", targetDir, targetId, L"result.txt",
                    nullptr, nullptr, MAXULONGLONG, workspace.Size(), UacCopyLeaf::StreamPolicy::IgnoreNamed);
                Check(collision == ERROR_FILE_EXISTS || collision == ERROR_ALREADY_EXISTS,
                      "existing destination is not replaced");
                CloseHandle(sourceDir);
            }
            const HANDLE result = CreateFileW((target + L"\\result.txt").c_str(), GENERIC_READ, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            char bytes[sizeof(content)]{};
            DWORD read = 0;
            Check(result != INVALID_HANDLE_VALUE && ReadFile(result, bytes, sizeof(bytes), &read, nullptr) &&
                      read == sizeof(content) - 1 && !memcmp(bytes, content, read), "destination bytes match");
            if (result != INVALID_HANDLE_VALUE) CloseHandle(result);
        }
    }
    Check(!workspacePath.empty() && GetFileAttributesW(workspacePath.c_str()) == INVALID_FILE_ATTRIBUTES,
          "remove only the owned workspace source");
    {
        CleanupReport report;
        std::wstring path;
        std::thread release;
        {
            UacFileList::Workspace workspace(Report, &report);
            if (workspace.Create() || workspace.Seal()) return 7;
            path = workspace.Path();
            HANDLE held = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                      FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (held == INVALID_HANDLE_VALUE) return 8;
            release = std::thread([held] { Sleep(150); CloseHandle(held); });
        }
        release.join();
        Check(report.calls == 0 && GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES,
              "bounded cleanup succeeds when a broker-like reader releases the source");
    }
    {
        CleanupReport report;
        std::wstring path, directory;
        FILE_ID_INFO fileId{}, directoryId{};
        HANDLE held = INVALID_HANDLE_VALUE;
        {
            UacFileList::Workspace workspace(Report, &report);
            if (workspace.Create() || workspace.Seal()) return 9;
            path = workspace.Path(); directory = path.substr(0, path.find_last_of(L'\\'));
            fileId = NativeIdentity(workspace.Identity()); directoryId = NativeIdentity(workspace.ParentIdentity());
            held = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                               FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (held == INVALID_HANDLE_VALUE) return 10;
        }
        Check(report.calls == 1 && report.error == ERROR_SHARING_VIOLATION && report.path == directory &&
                  GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES,
              "persistent broker-like lock reports the retained workspace once");
        CloseHandle(held);
        Check(RemoveOwned(path, fileId) && RemoveOwned(directory, directoryId, true),
              "later cleanup removes only the verified retained identities");
    }
    {
        CleanupReport report;
        std::wstring directory;
        FILE_ID_INFO directoryId{};
        {
            UacFileList::Workspace workspace(Report, &report);
            if (workspace.Create() || workspace.Seal()) return 11;
            directory = workspace.Path().substr(0, workspace.Path().find_last_of(L'\\'));
            directoryId = NativeIdentity(workspace.ParentIdentity());
            UacFileList::failCleanupPoint = 3;
        }
        UacFileList::failCleanupPoint = 0;
        Check(report.calls == 1 && report.error == ERROR_ACCESS_DENIED && report.path == directory &&
                  GetFileAttributesW(directory.c_str()) != INVALID_FILE_ATTRIBUTES,
              "a failed directory disposition reports the retained directory");
        Check(RemoveOwned(directory, directoryId, true), "remove only the verified retained directory");
    }
    {
        CleanupReport report;
        std::wstring directory;
        FILE_ID_INFO directoryId{};
        {
            UacFileList::Workspace workspace(Report, &report);
            UacFileList::failIdentityPoint = 1;
            Check(workspace.Create() == ERROR_ACCESS_DENIED, "inject failure while reading created directory identity");
            UacFileList::failIdentityPoint = 0;
            directory = workspace.Path().substr(0, workspace.Path().find_last_of(L'\\'));
            HANDLE actual = CreateFileW(directory.c_str(), FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
                FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (actual == INVALID_HANDLE_VALUE ||
                !GetFileInformationByHandleEx(actual, FileIdInfo, &directoryId, sizeof(directoryId))) return 16;
            CloseHandle(actual);
            UacFileList::failCleanupPoint = 3;
        }
        UacFileList::failCleanupPoint = 0;
        Check(report.calls == 1 && report.error == ERROR_ACCESS_DENIED && report.path == directory &&
                  GetFileAttributesW(directory.c_str()) != INVALID_FILE_ATTRIBUTES,
              "directory identity and cleanup failures report its exact path");
        Check(RemoveOwned(directory, directoryId, true), "remove directory fixture only after checking its identity");
    }
    {
        CleanupReport report;
        std::wstring path, directory;
        FILE_ID_INFO fileId{}, directoryId{};
        {
            UacFileList::Workspace workspace(Report, &report);
            UacFileList::failIdentityPoint = 2;
            Check(workspace.Create() == ERROR_ACCESS_DENIED, "inject failure while reading created source identity");
            UacFileList::failIdentityPoint = 0;
            path = workspace.Path(); directory = path.substr(0, path.find_last_of(L'\\'));
            directoryId = NativeIdentity(workspace.ParentIdentity());
            if (!GetFileInformationByHandleEx(workspace.Writer(), FileIdInfo, &fileId, sizeof(fileId))) return 15;
            UacFileList::failCleanupPoint = 1;
        }
        UacFileList::failCleanupPoint = 0;
        Check(report.calls == 1 && report.error == ERROR_ACCESS_DENIED && report.path == directory &&
                  GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES,
              "failed FILE_CREATE handle cleanup reports the unverified retained source");
        Check(RemoveOwned(path, fileId) && RemoveOwned(directory, directoryId, true),
              "remove failed identity fixture only after checking actual IDs");
    }
    {
        CleanupReport report;
        FILE_ID_INFO directoryId{};
        {
            UacFileList::Workspace workspace(ThrowReport, &report, FallbackReport);
            if (workspace.Create() || workspace.Seal()) return 17;
            report.path = workspace.Path().substr(0, workspace.Path().find_last_of(L'\\'));
            directoryId = NativeIdentity(workspace.ParentIdentity());
            UacFileList::failCleanupPoint = 3;
        }
        UacFileList::failCleanupPoint = 0;
        Check(report.calls == 1 && report.fallbackCalls == 1 && report.fallbackPathMatches &&
                  report.error == ERROR_ACCESS_DENIED,
              "throwing reporter invokes exactly one nonthrowing fallback with the workspace path");
        Check(RemoveOwned(report.path, directoryId, true), "remove verified reporter-failure fixture");
    }
    {
        CleanupReport report;
        std::wstring path, directory, original;
        FILE_ID_INFO fileId{}, directoryId{}, replacementId{};
        {
            UacFileList::Workspace workspace(Report, &report);
            if (workspace.Create() || workspace.Seal()) return 12;
            path = workspace.Path(); directory = path.substr(0, path.find_last_of(L'\\'));
            original = directory + L"\\original.tmp";
            fileId = NativeIdentity(workspace.Identity()); directoryId = NativeIdentity(workspace.ParentIdentity());
            workspace.ReleaseReaderForTest(); // Force the race; production keeps this pin.
            if (!MoveFileW(path.c_str(), original.c_str())) return 13;
            HANDLE replacement = CreateFileW(path.c_str(), GENERIC_WRITE | FILE_READ_ATTRIBUTES, 0,
                                              nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (replacement == INVALID_HANDLE_VALUE ||
                !GetFileInformationByHandleEx(replacement, FileIdInfo, &replacementId, sizeof(replacementId))) return 14;
            CloseHandle(replacement);
        }
        Check(report.calls == 1 && report.error == ERROR_FILE_INVALID &&
                  GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES &&
                  GetFileAttributesW(original.c_str()) != INVALID_FILE_ATTRIBUTES,
              "cleanup reports and preserves a replacement with a different file ID");
        Check(RemoveOwned(path, replacementId) && RemoveOwned(original, fileId) &&
                  RemoveOwned(directory, directoryId, true), "remove only the verified test identities");
    }
    DeleteFileW((target + L"\\result.txt").c_str());
    CloseHandle(targetDir);
    RemoveDirectoryW(target.c_str());
    std::printf("SKIP: other-account elevated broker requires an interactive Windows test\n");
    return failures ? 1 : 0;
}
