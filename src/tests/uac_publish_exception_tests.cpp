// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-12 | Exercise allocation exceptions throughout journalled publication. | Preserve the original or its backup and report uncertain outcomes. | #731 | GPT-6 (Codex) |
#define SAM_UAC_REPLACEMENT_FAULT_TEST
#include "../uacbroker/zipupdate.h"
#include <sddl.h>
#include <cstdio>

using Handle = UacZipUpdate::Handle;
using UacRenameLeaf::Detail::ReplacementFaultPoint;
static int checks = 0, failures = 0;
static bool inject = false, collisionCreated = false;
static ReplacementFaultPoint pointToThrow{};
static std::wstring collisionPath;
static const std::string sourceBytes(131073, 'S'), targetBytes = "original destination";

static void Check(bool condition, const char* label)
{
    ++checks;
    std::printf("%s: %s (%lu)\n", condition ? "PASS" : "FAIL", label, GetLastError());
    if (!condition) ++failures;
}
static bool WriteFixture(const std::wstring& path, const std::string& bytes)
{
    Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, 0, nullptr), CloseHandle);
    if (file.get() == INVALID_HANDLE_VALUE) { file.release(); return false; }
    DWORD written = 0;
    return WriteFile(file.get(), bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
}
namespace UacRenameLeaf::Detail
{
void PauseReplacementAt(ReplacementFaultPoint point)
{
    if (!inject) return;
    if (pointToThrow == ReplacementFaultPoint::BeforeRollback && point == ReplacementFaultPoint::BackupRenamed)
        collisionCreated = WriteFixture(collisionPath, "independent collision");
    if (point == pointToThrow) throw std::bad_alloc{};
}
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

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring root = argv[1], sourcePath = root + L"\\source-東.txt";
    if (!WriteFixture(sourcePath, sourceBytes)) return 3;
    Handle parent(CreateFileW(root.c_str(), FILE_LIST_DIRECTORY | FILE_ADD_FILE | FILE_READ_ATTRIBUTES,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr), CloseHandle);
    if (parent.get() == INVALID_HANDLE_VALUE) { parent.release(); return 4; }
    UacDirectoryLeaf::ParentIdentity parentIdentity{};
    if (UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentIdentity)) return 5;
    PSECURITY_DESCRIPTOR rawSecurity = nullptr;
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;FA;;;OW)(A;;FA;;;SY)(A;;FA;;;BA)",
                                                               SDDL_REVISION_1, &rawSecurity, nullptr)) return 13;
    std::unique_ptr<void, decltype(&LocalFree)> security(rawSecurity, LocalFree);
    SECURITY_ATTRIBUTES journalAttributes{sizeof(journalAttributes), security.get(), FALSE};
    struct Scenario
    {
        ReplacementFaultPoint point;
        const char* label;
        bool journalExpected, confirmed, backupExpected, finalOriginal, finalReplacement, finalCollision;
    };
    const Scenario scenarios[] = {
        {ReplacementFaultPoint::BeforeJournal, "before journal", false, false, false, true, false, false},
        {ReplacementFaultPoint::JournalFlushed, "after journal", true, false, false, true, false, false},
        {ReplacementFaultPoint::BackupRenamed, "after backup rename", true, false, true, false, false, false},
        {ReplacementFaultPoint::StageRenamed, "after stage rename", true, false, true, false, false, false},
        {ReplacementFaultPoint::StageKept, "after keeping stage", true, false, true, false, true, false},
        {ReplacementFaultPoint::ReceiptFlushed, "after durable receipt", true, true, true, false, true, false},
        {ReplacementFaultPoint::BackupRetired, "after retiring backup", true, true, false, false, true, false},
        {ReplacementFaultPoint::BeforeRollback, "before rollback", true, false, true, false, false, true},
    };
    for (size_t i = 0; i != sizeof(scenarios) / sizeof(scenarios[0]); ++i)
    {
        const auto& scenario = scenarios[i];
        const std::wstring leaf = L"target-日-" + std::to_wstring(i) + L".txt";
        const std::wstring targetPath = root + L"\\" + leaf;
        const std::wstring journalPath = root + L"\\journal-" + std::to_wstring(i);
        if (!WriteFixture(targetPath, targetBytes)) return 6;
        Handle journal(CreateFileW(journalPath.c_str(), GENERIC_READ | GENERIC_WRITE | DELETE, 0, &journalAttributes,
                                   CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, nullptr), CloseHandle);
        if (journal.get() == INVALID_HANDLE_VALUE) { journal.release(); return 7; }
        std::wstring backupPath;
        std::string durableJournal;
        UacZipUpdate::PublishResult result{};
        bool escaped = false, journalOK = false, bytesOK = false, outputIdOK = false;
        {
            HANDLE rawOriginal = nullptr, rawStage = nullptr;
            if (UacZipUpdate::Detail::Open(parent.get(), leaf, rawOriginal, DELETE)) return 8;
            Handle original(rawOriginal, CloseHandle);
            UacZipUpdate::Revision revision{};
            if (UacZipUpdate::ReadRevision(original.get(), revision)) return 9;
            std::wstring stageName;
            if (UacCopyLeaf::Detail::StageFile(parent.get(), rawStage, stageName, false, nullptr, true)) return 10;
            Handle stage(rawStage, CloseHandle);
            DWORD written = 0;
            if (!WriteFile(stage.get(), sourceBytes.data(), static_cast<DWORD>(sourceBytes.size()), &written, nullptr) ||
                written != sourceBytes.size()) return 11;
            FILE_ID_INFO stageIdentity{}, returnedIdentity{};
            if (!GetFileInformationByHandleEx(stage.get(), FileIdInfo, &stageIdentity, sizeof(stageIdentity))) return 12;
            std::memset(&returnedIdentity, 0xa5, sizeof(returnedIdentity));
            const FILE_ID_INFO sentinel = returnedIdentity;
            pointToThrow = scenario.point;
            collisionPath = targetPath;
            collisionCreated = false;
            inject = true;
            try
            {
                result = UacZipUpdate::Publish(parent.get(), parentIdentity, leaf, revision, original, stage.get(),
                                               journal.get(), nullptr, MAXULONGLONG, &returnedIdentity,
                                               UacZipUpdate::ReplacementKind::DiskCopy);
            }
            catch (...) { escaped = true; }
            inject = false;
            bytesOK = ReadContent(stage.get()) == sourceBytes && FileHasContent(sourcePath, sourceBytes);
            outputIdOK = scenario.confirmed ? UacRenameLeaf::Detail::SameSource(returnedIdentity, stageIdentity) :
                std::memcmp(&returnedIdentity, &sentinel, sizeof(sentinel)) == 0;
            LARGE_INTEGER size{};
            journalOK = GetFileSizeEx(journal.get(), &size) &&
                        (scenario.journalExpected ? size.QuadPart > 0 : size.QuadPart == 0);
            if (scenario.journalExpected && journalOK)
            {
                LARGE_INTEGER zero{}; DWORD read = 0;
                UacZipUpdate::Detail::RecoveryRecord record{};
                journalOK = SetFilePointerEx(journal.get(), zero, nullptr, FILE_BEGIN) &&
                    ReadFile(journal.get(), &record, sizeof(record), &read, nullptr) && read == sizeof(record) &&
                    std::wstring(record.leaf) == leaf;
                if (journalOK)
                    backupPath = root + L"\\" + UacZipUpdate::Detail::BackupName(record.nonce, UacZipUpdate::ReplacementKind::DiskCopy);
            }
            durableJournal = ReadContent(journal.get());
            journalOK = journalOK && durableJournal.size() == static_cast<size_t>(size.QuadPart);
        }
        Check(!escaped && result.error == ERROR_NOT_ENOUGH_MEMORY &&
              result.recoveryError == (scenario.journalExpected ? static_cast<DWORD>(ERROR_PARTIAL_COPY) : ERROR_SUCCESS) &&
              result.published == scenario.confirmed, scenario.label);
        Check(journalOK && bytesOK && outputIdOK, "fault retains journal, source bytes and confirmed-ID semantics");
        Check(scenario.backupExpected ? FileHasContent(backupPath, targetBytes) :
              (backupPath.empty() || !FileExists(backupPath)), "fault retains original backup when required");
        Check(scenario.finalOriginal ? FileHasContent(targetPath, targetBytes) :
              scenario.finalReplacement ? FileHasContent(targetPath, sourceBytes) :
              scenario.finalCollision ? collisionCreated && FileHasContent(targetPath, "independent collision") :
              !FileExists(targetPath), "fault leaves final name consistent with its uncertain phase");
        journal.reset();
        Check(FileHasContent(journalPath, durableJournal), "journal bytes persist after the exclusive handle closes");
    }
    Check(FileHasContent(sourcePath, sourceBytes), "all injected failures leave source data intact");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
