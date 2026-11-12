// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise allocation failure in sealed move recovery. | Check partial recovery keeps its durable journal and source. | #731 | GPT-6 (Codex) |
#define SAM_UAC_MOVE_RECOVERY_EXCEPTION_TEST
#include "../uacbroker/movesourcejournal.h"
#include <sddl.h>
#include <cstdio>

using Handle = UacMoveSourceJournal::Detail::Handle;
using UacMoveSourceJournal::RecoveryExceptionPoint;
static bool inject = false, reached = false;
static RecoveryExceptionPoint pointToThrow{};
static int checks = 0, failures = 0;
namespace UacMoveSourceJournal
{
void InjectRecoveryException(RecoveryExceptionPoint point)
{
    if (inject && point == pointToThrow) { reached = true; throw std::bad_alloc{}; }
}
}
static void Check(bool condition, const char* label)
{
    ++checks;
    std::printf("%s: %s (%lu)\n", condition ? "PASS" : "FAIL", label, GetLastError());
    if (!condition) ++failures;
}
static bool WriteFixture(const std::wstring& path, const std::string& bytes, SECURITY_ATTRIBUTES* attributes = nullptr)
{
    Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, attributes, CREATE_NEW, 0, nullptr), CloseHandle);
    if (file.get() == INVALID_HANDLE_VALUE) { file.release(); return false; }
    DWORD written = 0;
    return WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
}
static std::string ReadContent(HANDLE file)
{
    LARGE_INTEGER zero{}, size{};
    if (!file || !SetFilePointerEx(file, zero, nullptr, FILE_BEGIN) || !GetFileSizeEx(file, &size) ||
        size.QuadPart < 0 || size.QuadPart > 1024 * 1024) return {};
    std::string bytes(static_cast<size_t>(size.QuadPart), '\0');
    DWORD read = 0;
    if (!ReadFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) || read != bytes.size()) return {};
    return bytes;
}
static bool FileHasContent(const std::wstring& path, const std::string& expected)
{
    Handle file(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr), CloseHandle);
    if (file.get() == INVALID_HANDLE_VALUE) { file.release(); return false; }
    return ReadContent(file.get()) == expected;
}
static bool FileExists(const std::wstring& path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}
static bool Identity(const std::wstring& path, FILE_ID_INFO& id)
{
    Handle file(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr), CloseHandle);
    if (file.get() == INVALID_HANDLE_VALUE) { file.release(); return false; }
    return GetFileInformationByHandleEx(file.get(), FileIdInfo, &id, sizeof(id)) != FALSE;
}
static Handle Directory(const std::wstring& path)
{
    HANDLE raw = CreateFileW(path.c_str(), FILE_TRAVERSE | FILE_ADD_FILE | READ_CONTROL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(raw == INVALID_HANDLE_VALUE ? nullptr : raw, CloseHandle);
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root = argv[1];
    HANDLE rawToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &rawToken)) return 3;
    Handle token(rawToken, CloseHandle);
    DWORD length = 0;
    GetTokenInformation(token.get(), TokenUser, nullptr, 0, &length);
    std::vector<BYTE> tokenData(length);
    if (!length || !GetTokenInformation(token.get(), TokenUser, tokenData.data(), length, &length)) return 4;
    PWSTR sidRaw = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<const TOKEN_USER*>(tokenData.data())->User.Sid, &sidRaw)) return 5;
    std::unique_ptr<void, decltype(&LocalFree)> sidOwner(sidRaw, LocalFree);
    const std::wstring privateSddl = std::wstring(L"O:") + sidRaw + L"D:P(A;;FA;;;" + sidRaw + L")";
    const std::wstring sealSddl = std::wstring(L"D:P(A;;RC;;;OW)(A;;0x00170189;;;") + sidRaw + L")";
    PSECURITY_DESCRIPTOR privateRaw = nullptr, sealRaw = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(privateSddl.c_str(), SDDL_REVISION_1, &privateRaw, nullptr) ||
        !ConvertStringSecurityDescriptorToSecurityDescriptorW(sealSddl.c_str(), SDDL_REVISION_1, &sealRaw, nullptr)) return 6;
    std::unique_ptr<void, decltype(&LocalFree)> security(privateRaw, LocalFree), sealSecurity(sealRaw, LocalFree);
    SECURITY_ATTRIBUTES attributes{sizeof(attributes), security.get(), FALSE};
    PACL seal = nullptr; BOOL present = FALSE, defaulted = FALSE;
    if (!GetSecurityDescriptorDacl(sealSecurity.get(), &present, &seal, &defaulted) || !present || !seal) return 7;

    struct Scenario { RecoveryExceptionPoint point; const char* label; bool deleteOriginal, retainedAfter; };
    const Scenario cases[] = {
        {RecoveryExceptionPoint::BeforeRecord, "before reading the armed record", false, true},
        {RecoveryExceptionPoint::AfterRestoreName, "after restoring the source name", true, false},
        {RecoveryExceptionPoint::AfterAliasRetired, "after retiring the recovery alias", false, false},
    };
    for (size_t i = 0; i != sizeof(cases) / sizeof(cases[0]); ++i)
    {
        const auto& scenario = cases[i];
        const std::wstring originalPath = root + L"\\original-" + std::to_wstring(i);
        const std::wstring storagePath = root + L"\\private-" + std::to_wstring(i);
        const std::wstring sourcePath = originalPath + L"\\source-東.txt";
        const std::wstring streamPath = sourcePath + L":named-日";
        const std::wstring journalPath = storagePath + L"\\journal";
        if (!CreateDirectoryW(originalPath.c_str(), nullptr) || !CreateDirectoryW(storagePath.c_str(), &attributes) ||
            !WriteFixture(sourcePath, "source content") || !WriteFixture(streamPath, "named stream")) return 8;
        Handle parent = Directory(originalPath), storage = Directory(storagePath);
        Handle journal(CreateFileW(journalPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, &attributes,
                                   CREATE_NEW, FILE_FLAG_OPEN_REPARSE_POINT, nullptr), CloseHandle);
        if (!parent || !storage || journal.get() == INVALID_HANDLE_VALUE) { if (journal.get() == INVALID_HANDLE_VALUE) journal.release(); return 9; }
        HANDLE rawSource = nullptr;
        const DWORD open = UacRenameLeaf::Detail::OpenSource(parent.get(), L"source-東.txt", rawSource,
            DELETE | READ_CONTROL | WRITE_DAC | FILE_READ_DATA | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ);
        Handle source(rawSource, CloseHandle);
        FILE_ID_INFO expected{};
        auto originalDacl = std::make_unique<UacMoveSourceJournal::Detail::DaclSnapshot>();
        if (open || UacRenameLeaf::Detail::ReadSourceIdentity(source.get(), expected) || originalDacl->Read(source.get())) return 10;
        std::wstring alias;
        if (UacMoveSourceJournal::Arm(parent.get(), L"source-東.txt", source.get(), expected, storage.get(),
                                     journal.get(), security.get(), alias, seal) || alias.empty()) return 11;
        if (SetSecurityInfo(source.get(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                            nullptr, nullptr, seal, nullptr)) return 12;
        if (scenario.deleteOriginal)
        {
            FILE_DISPOSITION_INFO remove{TRUE};
            if (!SetFileInformationByHandle(source.get(), FileDispositionInfo, &remove, sizeof(remove))) return 14;
        }
        source.reset();
        const auto aliasPath = storagePath + L"\\" + alias;
        if (!FileExists(aliasPath) || FileExists(sourcePath) == scenario.deleteOriginal) return 15;
        const std::string beforeJournal = ReadContent(journal.get());
        if (beforeJournal.empty()) return 16;
        pointToThrow = scenario.point; reached = false; inject = true;
        DWORD error = ERROR_SUCCESS; bool escaped = false;
        try { error = UacMoveSourceJournal::Recover(parent.get(), storage.get(), journal.get(), security.get()); }
        catch (...) { escaped = true; }
        inject = false;
        FILE_ID_INFO current{};
        Check(reached && !escaped && error == ERROR_NOT_ENOUGH_MEMORY, scenario.label);
        Check(Identity(sourcePath, current) && UacRenameLeaf::Detail::SameSource(expected, current) &&
              FileHasContent(sourcePath, "source content") && FileHasContent(streamPath, "named stream"),
              "fault preserves the original source identity, data and named stream");
        Check(FileExists(aliasPath) == scenario.retainedAfter, "recovery alias reflects the completed partial step");
        Check(ReadContent(journal.get()) == beforeJournal, "recovery fault leaves the exact journal bytes unchanged");
        journal.reset();
        journal.reset(CreateFileW(journalPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                  OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        if (journal.get() == INVALID_HANDLE_VALUE) { journal.release(); return 17; }
        Check(ReadContent(journal.get()) == beforeJournal, "journal bytes survive closing and reopening the handle");
        error = UacMoveSourceJournal::Recover(parent.get(), storage.get(), journal.get(), security.get());
        LARGE_INTEGER size{};
        auto restoredDacl = std::make_unique<UacMoveSourceJournal::Detail::DaclSnapshot>();
        HANDLE rawRestored = CreateFileW(sourcePath.c_str(), READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                         nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        Handle restored(rawRestored == INVALID_HANDLE_VALUE ? nullptr : rawRestored, CloseHandle);
        Check(!error && Identity(sourcePath, current) && UacRenameLeaf::Detail::SameSource(expected, current) &&
              FileHasContent(sourcePath, "source content") && FileHasContent(streamPath, "named stream") &&
              !FileExists(aliasPath) && GetFileSizeEx(journal.get(), &size) && !size.QuadPart &&
              restored && !restoredDacl->Read(restored.get()) && restoredDacl->Matches(*originalDacl),
              "retry completes recovery with original identity, streams and owner/group/DACL");
    }
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
