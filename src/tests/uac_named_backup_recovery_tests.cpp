// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Exercise recovery before stage publication. | Detect modified data and untrusted named backups after a broker crash. | #731 | GPT-6.1 Terra |
#define SAM_UAC_REPLACEMENT_FAULT_TEST
#include "../uacbroker/zipupdate.h"
#include <cstdio>
#include <sddl.h>

using Handle = UacZipUpdate::Handle;
static bool crash = false;
static bool crashBeforeRename = false;
namespace UacRenameLeaf::Detail
{
void PauseReplacementAt(ReplacementFaultPoint point)
{
    if (crash && point == (crashBeforeRename ? ReplacementFaultPoint::JournalFlushed : ReplacementFaultPoint::BackupRenamed))
        TerminateProcess(GetCurrentProcess(), 91);
}
}
static bool Trusted(HANDLE, bool) { return true; }
static bool RejectWorldWritable(HANDLE file, bool)
{
    PACL dacl = nullptr;
    PSECURITY_DESCRIPTOR raw = nullptr;
    const DWORD error = GetSecurityInfo(file, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
                                        nullptr, nullptr, &dacl, nullptr, &raw);
    const std::unique_ptr<void, decltype(&LocalFree)> descriptor(raw, LocalFree);
    if (error || !dacl) return false;
    for (DWORD i = 0; i < dacl->AceCount; ++i)
    {
        void* entry = nullptr;
        if (!GetAce(dacl, i, &entry)) return false;
        const auto* header = static_cast<const ACE_HEADER*>(entry);
        if (header->AceType != ACCESS_ALLOWED_ACE_TYPE) continue;
        const auto* allow = static_cast<const ACCESS_ALLOWED_ACE*>(entry);
        if (IsWellKnownSid(const_cast<DWORD*>(&allow->SidStart), WinWorldSid) &&
            (allow->Mask & (GENERIC_ALL | GENERIC_WRITE | FILE_WRITE_DATA | FILE_APPEND_DATA | WRITE_DAC)))
            return false;
    }
    return true;
}
static Handle Open(const std::wstring& path, DWORD access, DWORD share, DWORD disposition)
{
    const HANDLE raw = CreateFileW(path.c_str(), access, share, nullptr, disposition,
                                   FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(raw == INVALID_HANDLE_VALUE ? nullptr : raw, CloseHandle);
}
static bool Write(const std::wstring& path, const char* text, bool truncate)
{
    auto file = Open(path, GENERIC_WRITE, 0, truncate ? CREATE_ALWAYS : OPEN_EXISTING);
    DWORD written = 0;
    const auto size = static_cast<DWORD>(std::strlen(text));
    return file && WriteFile(file.get(), text, size, &written, nullptr) && written == size && FlushFileBuffers(file.get());
}
static int Child(const std::wstring& folder)
{
    const std::wstring target = folder + L"\\target-東.txt";
    if (!Write(target, "original content", true) || !Write(target + L":東", "original ADS", true)) return 2;
    auto parent = Open(folder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING);
    UacDirectoryLeaf::ParentIdentity parentId{};
    if (!parent || UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentId)) return 2;
    HANDLE raw = nullptr;
    if (UacZipUpdate::Detail::Open(parent.get(), L"target-東.txt", raw, DELETE | READ_CONTROL)) return 2;
    Handle original(raw, CloseHandle);
    UacZipUpdate::Revision revision{};
    if (UacZipUpdate::ReadRevision(original.get(), revision)) return 2;
    std::wstring name;
    if (UacCopyLeaf::Detail::StageFile(parent.get(), raw, name, true, nullptr, true)) return 2;
    Handle stage(raw, CloseHandle);
    DWORD written = 0;
    if (!WriteFile(stage.get(), "replacement", 11, &written, nullptr) || written != 11) return 2;
    auto journal = Open(folder + L"\\journal", GENERIC_READ | GENERIC_WRITE, 0, CREATE_NEW);
    if (!journal) return 2;
    crash = true;
    UacZipUpdate::Publish(parent.get(), parentId, L"target-東.txt", revision, original, stage.get(),
                          journal.get(), nullptr, MAXULONGLONG, nullptr, UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
    return 3;
}
static bool Exists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }
static bool FileIdentity(const std::wstring& path, FILE_ID_INFO& identity)
{
    const auto file = Open(path, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING);
    return file && GetFileInformationByHandleEx(file.get(), FileIdInfo, &identity, sizeof(identity));
}
int wmain(int argc, wchar_t** argv)
{
    if (argc == 3)
    {
        crashBeforeRename = std::wcscmp(argv[2], L"--before-rename") == 0;
        return Child(argv[1]);
    }
    if (argc != 2) return 2;
    int failures = 0;
    const std::wstring base = argv[1];
    for (int scenario = 0; scenario != 5; ++scenario)
    {
        const auto folder = base + L"\\case-" + std::to_wstring(scenario);
        if (!CreateDirectoryW(folder.c_str(), nullptr)) return 2;
        std::wstring command = L"\"" + std::wstring(argv[0]) + L"\" \"" + folder +
                               (scenario == 4 ? L"\" --before-rename" : L"\" --child");
        std::vector<wchar_t> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back(0);
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(argv[0], mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                            nullptr, nullptr, &startup, &process)) return 2;
        Handle child(process.hProcess, CloseHandle), thread(process.hThread, CloseHandle);
        if (WaitForSingleObject(child.get(), 10000) != WAIT_OBJECT_0) { TerminateProcess(child.get(), 92); return 2; }
        DWORD exit = 0;
        if (!GetExitCodeProcess(child.get(), &exit) || exit != 91) return 2;
        auto parent = Open(folder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING);
        auto journal = Open(folder + L"\\journal", GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING);
        UacDirectoryLeaf::ParentIdentity parentId{};
        UacZipUpdate::Detail::NamedBackupRecord record{};
        DWORD read = 0;
        if (!parent || !journal || UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentId) ||
            !ReadFile(journal.get(), &record, sizeof(record), &read, nullptr) || read != sizeof(record)) return 2;
        const auto backup = folder + L"\\" + UacZipUpdate::Detail::BackupName(record.disk.exchange.nonce,
                                                                                UacZipUpdate::ReplacementKind::DiskCopy);
        const auto target = folder + L"\\target-東.txt";
        if (scenario == 4 ? (Exists(backup) || !Exists(target)) : (!Exists(backup) || Exists(target))) return 2;
        if ((scenario == 1 && !Write(backup + L":東", "changed  ADS", false)) ||
            (scenario == 2 && !Write(backup, "changed  content", false))) return 2;
        if (scenario == 3)
        {
            PSECURITY_DESCRIPTOR raw = nullptr;
            if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;FA;;;WD)", SDDL_REVISION_1,
                                                                       &raw, nullptr)) return 2;
            const std::unique_ptr<void, decltype(&LocalFree)> descriptor(raw, LocalFree);
            if (!SetFileSecurityW(backup.c_str(), DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
                                  raw)) return 2;
            auto probe = Open(backup, READ_CONTROL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING);
            if (!probe || RejectWorldWritable(probe.get(), false)) return 2;
        }
        const DWORD result = UacZipUpdate::Recover(parent.get(), parentId, L"target-東.txt", journal.get(),
            UacZipUpdate::ReplacementKind::DiskCopy, scenario == 3 ? RejectWorldWritable : Trusted);
        LARGE_INTEGER journalSize{};
        bool exactOriginal = true;
        if (scenario == 4)
        {
            const auto original = Open(target, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA, FILE_SHARE_READ, OPEN_EXISTING);
            UacZipUpdate::Revision revision{};
            BYTE digest[32]{};
            exactOriginal = original && !UacZipUpdate::Detail::CheckIdentity(original.get(), record.disk.exchange.original) &&
                !UacZipUpdate::ReadRevision(original.get(), revision) &&
                !UacZipUpdate::Detail::DigestReplacement(parent.get(), L"target-東.txt", original.get(), revision, true, digest) &&
                !std::memcmp(digest, record.originalDigest, sizeof(digest));
        }
        const bool correct = scenario == 4 ? result == ERROR_SUCCESS && Exists(target) && !Exists(backup) &&
                                             exactOriginal && GetFileSizeEx(journal.get(), &journalSize) && !journalSize.QuadPart :
            result == ERROR_PARTIAL_COPY && !Exists(target) && Exists(backup);
        std::printf("%s: %s (%lu)\n", correct ? "PASS" : "FAIL",
                    scenario == 0 ? "retain legacy backup without security proof" : scenario == 1 ? "retain backup after ADS mutation" :
                    scenario == 2 ? "retain backup after default-stream mutation" : scenario == 3 ? "retain an untrusted named backup" :
                    "clear pre-rename journal without changing the original", result);
        failures += !correct;
    }
    for (int version = 1; version <= 4; ++version)
    {
        for (int phase = 0; phase != 4; ++phase)
        {
            const auto folder = base + L"\\v" + std::to_wstring(version) + L"-phase" + std::to_wstring(phase);
            if (!CreateDirectoryW(folder.c_str(), nullptr)) return 2;
            auto parent = Open(folder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING);
            UacDirectoryLeaf::ParentIdentity parentId{};
            UacZipUpdate::Detail::NamedBackupRecord record{};
            auto& exchange = record.disk.exchange;
            exchange.version = version;
            if (!parent || UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentId) ||
                FAILED(CoCreateGuid(&exchange.nonce))) return 2;
            exchange.parent = parentId.file;
            constexpr wchar_t leaf[] = L"legacy-東.txt";
            std::copy(std::begin(leaf), std::end(leaf), exchange.leaf);
            const auto originalSeed = folder + L"\\original-seed", replacementSeed = folder + L"\\replacement-seed";
            if (!Write(originalSeed, "original", true) || !Write(replacementSeed, "replacement", true) ||
                !FileIdentity(originalSeed, exchange.original) || !FileIdentity(replacementSeed, exchange.replacement)) return 2;
            const auto backup = folder + L"\\" + UacZipUpdate::Detail::BackupName(exchange.nonce,
                                                                                     UacZipUpdate::ReplacementKind::DiskCopy);
            const auto target = folder + L"\\" + leaf;
            if (phase == 0)
            {
                if (!MoveFileW(originalSeed.c_str(), target.c_str()) || !DeleteFileW(replacementSeed.c_str())) return 2;
            }
            else
            {
                if (!MoveFileW(originalSeed.c_str(), backup.c_str())) return 2;
                if (phase == 1)
                {
                    if (!DeleteFileW(replacementSeed.c_str())) return 2;
                }
                else if (!MoveFileW(replacementSeed.c_str(), target.c_str())) return 2;
                if (phase == 3 && !DeleteFileW(backup.c_str())) return 2;
            }
            auto journal = Open(folder + L"\\journal", GENERIC_READ | GENERIC_WRITE, 0, CREATE_NEW);
            const DWORD size = version == 1 ? sizeof(UacZipUpdate::Detail::RecoveryRecord) :
                               version == 4 ? sizeof(record) : sizeof(UacZipUpdate::Detail::DiskRecoveryRecord);
            DWORD written = 0;
            if (!journal || !WriteFile(journal.get(), &record, size, &written, nullptr) || written != size ||
                !FlushFileBuffers(journal.get())) return 2;
            const bool targetBefore = Exists(target), backupBefore = Exists(backup);
            const DWORD result = UacZipUpdate::Recover(parent.get(), parentId, leaf, journal.get(),
                UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
            LARGE_INTEGER after{};
            const DWORD expected = phase == 0 ? ERROR_SUCCESS : ERROR_PARTIAL_COPY;
            const bool correct = result == expected &&
                Exists(target) == targetBefore && Exists(backup) == backupBefore &&
                GetFileSizeEx(journal.get(), &after) && after.QuadPart == (phase == 0 ? 0LL : static_cast<LONGLONG>(size));
            std::printf("%s: check legacy disk journal v%d phase %d (%lu)\n", correct ? "PASS" : "FAIL",
                        version, phase, result);
            failures += !correct;
        }
    }
    return failures ? 1 : 0;
}
