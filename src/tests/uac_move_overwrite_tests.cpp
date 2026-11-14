/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-11 | Exercise a deletable target that explicitly denies attribute writes. | Keep normal publication and hard-exit recovery within necessary access rights. | #731 | GPT-6.1 Sol |
| 2026-11-11 | Exercise NORMAL-to-Archive recovery after a hard process exit. | Verify the shared NTFS backup attribute normalization on real files. | #731 | GPT-6.1 Sol |
| 2026-11-11 | Verify live-source moves and interrupted overwrite recovery on NTFS. | Preserve identity, security and streams and reject stale or changed objects. | #731 | GPT-6.1 Sol |
*/
#define SAM_UAC_MOVE_OVERWRITE_FAULT_TEST
#include "../uacbroker/moveoverwrite.h"
#include <cstdio>
#include <sddl.h>

namespace
{
using Handle = UacMoveOverwrite::Handle;
using Revision = UacZipUpdate::Revision;
int checks = 0, failures = 0, interruptAt = -1, cancelAt = -1;
bool hardExit = false;
HANDLE cancelWriter = nullptr;
std::wstring collisionPath;
constexpr DWORD attributes = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
void Check(bool success, const char* message, DWORD error = 0)
{
    ++checks;
    if (!success) ++failures;
    std::printf("%s: %s (%lu)\n", success ? "PASS" : "FAIL", message, error);
}
bool TrustedFixture(HANDLE, bool) { return true; }
bool Exists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }
Handle Open(const std::wstring& path, DWORD access, DWORD share = FILE_SHARE_READ)
{
    HANDLE file = CreateFileW(path.c_str(), access, share, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(file == INVALID_HANDLE_VALUE ? nullptr : file, CloseHandle);
}
bool Write(const std::wstring& path, DWORD size, BYTE value)
{
    HANDLE raw = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    Handle file(raw == INVALID_HANDLE_VALUE ? nullptr : raw, CloseHandle);
    std::vector<BYTE> block(64 * 1024, value);
    while (file && size)
    {
        const DWORD count = (std::min)(size, static_cast<DWORD>(block.size()));
        DWORD written = 0;
        if (!WriteFile(file.get(), block.data(), count, &written, nullptr) || written != count) return false;
        size -= count;
    }
    return file && FlushFileBuffers(file.get());
}
bool Empty(HANDLE journal)
{
    LARGE_INTEGER size{};
    return GetFileSizeEx(journal, &size) && !size.QuadPart;
}
std::vector<BYTE> Security(HANDLE file)
{
    DWORD size = 0;
    GetKernelObjectSecurity(file, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                            nullptr, 0, &size);
    std::vector<BYTE> value(size);
    if (!size || !GetKernelObjectSecurity(file, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                         value.data(), size, &size)) value.clear();
    return value;
}
bool SetPrivateDacl(const std::wstring& path, bool denyAttributeWrites = false)
{
    PSECURITY_DESCRIPTOR raw = nullptr;
    const auto sddl = denyAttributeWrites ? L"D:P(D;;0x100;;;WD)(A;;FA;;;WD)" : L"D:P(A;;FA;;;WD)";
    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, SDDL_REVISION_1, &raw, nullptr)) return false;
    const std::unique_ptr<void, decltype(&LocalFree)> descriptor(raw, LocalFree);
    return SetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, raw) != FALSE;
}
bool CrashChild(const std::wstring& folder, const std::wstring& sourceLeaf, const std::wstring& targetLeaf,
                const std::wstring& journal, int point)
{
    std::vector<WCHAR> image(32768);
    const DWORD length = GetModuleFileNameW(nullptr, image.data(), static_cast<DWORD>(image.size()));
    if (!length || length >= image.size()) return false;
    std::wstring command = L"\"" + std::wstring(image.data(), length) + L"\" --crash \"" + folder + L"\" \"" +
        sourceLeaf + L"\" \"" + targetLeaf + L"\" \"" + journal + L"\" " + std::to_wstring(point);
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        return false;
    Handle child(process.hProcess, CloseHandle), thread(process.hThread, CloseHandle);
    const DWORD wait = WaitForSingleObject(child.get(), 8000);
    if (wait != WAIT_OBJECT_0)
    {
        TerminateProcess(child.get(), 99);
        WaitForSingleObject(child.get(), 1000);
        return false;
    }
    DWORD exit = 0;
    return GetExitCodeProcess(child.get(), &exit) && exit == 77;
}
}
namespace UacMoveOverwrite
{
void PauseAt(FaultPoint point)
{
    if (static_cast<int>(point) == interruptAt)
    {
        if (hardExit) TerminateProcess(GetCurrentProcess(), 77);
        throw point;
    }
    if (static_cast<int>(point) == cancelAt)
    {
        DWORD written = 0;
        const BYTE signal = UacRenameLeaf::Detail::transferCancelSignal;
        if (!WriteFile(cancelWriter, &signal, 1, &written, nullptr) || written != 1) std::abort();
    }
    if (point == FaultPoint::BackupRenamed && !collisionPath.empty() && !Write(collisionPath, 5, 0x77)) std::abort();
}
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 7 && std::wstring_view(argv[1]) == L"--crash")
    {
        const std::wstring folder = argv[2], sourceLeaf = argv[3], targetLeaf = argv[4];
        const auto sourceParent = Open(folder + L"\\source-東", FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        const auto targetParent = Open(folder + L"\\target-日", FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        UacMoveOverwrite::ParentIdentity sourceParentId, targetParentId;
        if (!sourceParent || !targetParent || UacDirectoryLeaf::ReadParentIdentity(sourceParent.get(), sourceParentId) ||
            UacDirectoryLeaf::ReadParentIdentity(targetParent.get(), targetParentId)) return 3;
        auto source = Open(folder + L"\\source-東\\" + sourceLeaf, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA);
        auto target = Open(folder + L"\\target-日\\" + targetLeaf, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA);
        Revision sourceRevision, targetRevision;
        if (!source || !target || UacZipUpdate::ReadRevision(source.get(), sourceRevision) ||
            UacZipUpdate::ReadRevision(target.get(), targetRevision)) return 3;
        source.reset(); target.reset();
        const auto journal = Open(folder + L"\\" + argv[5], GENERIC_READ | GENERIC_WRITE, 0);
        if (!journal) return 3;
        interruptAt = _wtoi(argv[6]);
        hardExit = true;
        UacMoveOverwrite::MoveSameVolume(sourceParent.get(), sourceParentId, UacZipUpdate::RevisionIdentity(sourceRevision),
            sourceLeaf, targetParent.get(), targetParentId, targetLeaf, targetRevision, journal.get(), nullptr,
            MAXULONGLONG, TrustedFixture);
        return 3;
    }
    if (argc != 2) return 2;
    using namespace UacMoveOverwrite;
    const std::wstring folder = argv[1];
    const auto sourceFolder = folder + L"\\source-東", targetFolder = folder + L"\\target-日";
    if (!CreateDirectoryW(sourceFolder.c_str(), nullptr) || !CreateDirectoryW(targetFolder.c_str(), nullptr)) return 2;
    const auto sourceParent = Open(sourceFolder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    const auto targetParent = Open(targetFolder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    ParentIdentity sourceParentId, targetParentId;
    if (!sourceParent || !targetParent || UacDirectoryLeaf::ReadParentIdentity(sourceParent.get(), sourceParentId) ||
        UacDirectoryLeaf::ReadParentIdentity(targetParent.get(), targetParentId)) return 2;
    for (int scenario = 0; scenario < 14; ++scenario)
    {
        const auto sourceLeaf = L"zdroj-東-" + std::to_wstring(scenario) + L".bin";
        const auto targetLeaf = L"cíl-日-" + std::to_wstring(scenario) + L".bin";
        const auto sourcePath = sourceFolder + L"\\" + sourceLeaf;
        const auto targetPath = targetFolder + L"\\" + targetLeaf;
        if (!Write(sourcePath, scenario == 0 ? 20 * 1024 * 1024 + 7 : 67, 0x55) ||
            !Write(sourcePath + L":data-東", 53, 0xAA) || !SetPrivateDacl(sourcePath) ||
            !SetFileAttributesW(sourcePath.c_str(), attributes) || !Write(targetPath, 43, 0x11) ||
            !Write(targetPath + L":old-data", 47, 0x22) ||
            !SetFileAttributesW(targetPath.c_str(), scenario == 5 ? FILE_ATTRIBUTE_NORMAL :
                scenario >= 12 ? FILE_ATTRIBUTE_ARCHIVE : attributes)) return 2;
        if (scenario >= 12 && !SetPrivateDacl(targetPath, true)) return 2;
        auto source = Open(sourcePath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | READ_CONTROL);
        auto target = Open(targetPath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA);
        Revision sourceRevision, targetRevision;
        if (!source || !target || UacZipUpdate::ReadRevision(source.get(), sourceRevision) ||
            UacZipUpdate::ReadRevision(target.get(), targetRevision)) return 2;
        const auto sourceId = UacZipUpdate::RevisionIdentity(sourceRevision);
        const auto security = Security(source.get());
        BYTE digest[32]{};
        if (security.empty() || UacZipUpdate::Detail::DigestReplacement(sourceParent.get(), sourceLeaf, source.get(),
                sourceRevision, true, digest)) return 2;
        source.reset(); target.reset();
        if (scenario >= 12)
        {
            const auto refused = Open(targetPath, FILE_WRITE_ATTRIBUTES);
            const DWORD refusal = GetLastError();
            Check(!refused && refusal == ERROR_ACCESS_DENIED, "target ACL explicitly rejects FILE_WRITE_ATTRIBUTES", refusal);
            const auto allowed = Open(targetPath, DELETE | FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | READ_CONTROL);
            Check(allowed != nullptr, "same target permits the read and delete access needed by an ordinary overwrite");
        }
        const auto journalName = JournalName(sourceParentId, sourceId, targetParentId, targetLeaf);
        Check(!journalName.empty() && journalName != JournalName(sourceParentId, sourceId, targetParentId, L"other.bin"),
              "journal key isolates each source object and target name");
        HANDLE raw = CreateFileW((folder + L"\\" + journalName).c_str(), GENERIC_READ | GENERIC_WRITE,
            0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw == INVALID_HANDLE_VALUE) return 2;
        Handle journal(raw, CloseHandle);
        HANDLE readPipe = nullptr, writePipe = nullptr;
        if (!CreatePipe(&readPipe, &writePipe, nullptr, 0)) return 2;
        Handle connection(readPipe, CloseHandle), writer(writePipe, CloseHandle);
        cancelWriter = writer.get();
        if (scenario == 2) ++targetRevision.size;
        interruptAt = scenario >= 3 && scenario <= 6 ? scenario - 3 : scenario == 7 || scenario == 8 ? 2 : -1;
        cancelAt = scenario == 9 ? 1 : scenario == 10 ? 2 : -1;
        collisionPath = scenario == 11 ? targetPath : L"";
        Result result{ERROR_SUCCESS};
        bool interrupted = false;
        if ((scenario >= 3 && scenario <= 6) || scenario == 13)
        {
            journal.reset();
            interrupted = CrashChild(folder, sourceLeaf, targetLeaf, journalName, scenario == 13 ? 2 : scenario - 3);
            Check(interrupted, "terminate the child process at the durable move overwrite checkpoint");
            journal = Open(folder + L"\\" + journalName, GENERIC_READ | GENERIC_WRITE, 0);
            if (!interrupted || !journal) return 2;
        }
        else
        {
            try
            {
                result = MoveSameVolume(sourceParent.get(), sourceParentId, sourceId, sourceLeaf,
                    targetParent.get(), targetParentId, targetLeaf, targetRevision, journal.get(), connection.get(),
                    scenario == 1 ? 0 : MAXULONGLONG, TrustedFixture);
            }
            catch (FaultPoint) { interrupted = true; }
        }
        interruptAt = -1;
        cancelAt = -1;
        collisionPath.clear();
        if (scenario == 9 || scenario == 10)
        {
            BYTE signal = 0;
            DWORD read = 0;
            if (!ReadFile(connection.get(), &signal, 1, &read, nullptr) || read != 1) return 2;
            if (scenario == 9)
            {
                Check(result.error == ERROR_CANCELLED && !result.moved && !result.recoveryError &&
                      Empty(journal.get()) && Exists(sourcePath) && Exists(targetPath),
                      "cancel after backup rename restores both original names", result.error);
                continue;
            }
            Check(!result.error && result.moved && result.recoveryError == ERROR_CANCELLED && !Empty(journal.get()),
                  "cancel after source rename preserves moved source and recoverable old target", result.recoveryError);
            bool completed = false;
            const DWORD recovered = RecoverSameVolume(sourceParent.get(), sourceParentId, sourceId, sourceLeaf,
                targetParent.get(), targetParentId, targetLeaf, journal.get(), nullptr, MAXULONGLONG, TrustedFixture, &completed);
            Check(!recovered && completed && Empty(journal.get()), "recover the exact move after late cancellation", recovered);
            result.recoveryError = recovered;
        }
        if (scenario == 11)
        {
            Check(result.error == ERROR_ALREADY_EXISTS && result.recoveryError == ERROR_ALREADY_EXISTS &&
                  !result.moved && !Empty(journal.get()) && Exists(sourcePath) && Exists(targetPath),
                  "late target collision preserves source, foreign target and recorded old backup", result.error);
            const DWORD refused = RecoverSameVolume(sourceParent.get(), sourceParentId, sourceId, sourceLeaf,
                targetParent.get(), targetParentId, targetLeaf, journal.get(), nullptr, MAXULONGLONG, TrustedFixture);
            Check(refused == ERROR_FILE_INVALID && !Empty(journal.get()), "recovery never overwrites a foreign target", refused);
            if (!DeleteFileW(targetPath.c_str())) return 2;
            const DWORD recovered = RecoverSameVolume(sourceParent.get(), sourceParentId, sourceId, sourceLeaf,
                targetParent.get(), targetParentId, targetLeaf, journal.get(), nullptr, MAXULONGLONG, TrustedFixture);
            Check(!recovered && Exists(sourcePath) && Exists(targetPath) && Empty(journal.get()),
                  "restore the exact old target once the independent collision is removed", recovered);
            continue;
        }
        if (scenario == 1 || scenario == 2)
        {
            Check(result.error == static_cast<DWORD>(scenario == 1 ? ERROR_TIMEOUT : ERROR_FILE_INVALID) && !result.moved &&
                  Empty(journal.get()) && Exists(sourcePath) && Exists(targetPath),
                  "timeout and stale target preserve both original names and leave no journal", result.error);
            continue;
        }
        std::wstring backupPath;
        if (interrupted)
        {
            Detail::Record record;
            DWORD read = 0;
            if (!SetFilePointerEx(journal.get(), {}, nullptr, FILE_BEGIN) ||
                !ReadFile(journal.get(), &record, sizeof(record), &read, nullptr) || read != sizeof(record)) return 2;
            backupPath = targetFolder + L"\\" + Detail::BackupName(record.nonce);
            if (scenario == 5)
                Check(targetRevision.attributes == FILE_ATTRIBUTE_NORMAL &&
                      GetFileAttributesW(backupPath.c_str()) == FILE_ATTRIBUTE_ARCHIVE,
                      "hard exit changes a renamed NORMAL target backup to Archive on NTFS");
            if (scenario == 7)
            {
                if (!SetFileAttributesW(backupPath.c_str(), FILE_ATTRIBUTE_NORMAL) || !Write(backupPath + L":old-data", 47, 0x33) ||
                    !SetFileAttributesW(backupPath.c_str(), attributes)) return 2;
            }
            if (scenario == 8)
            {
                auto wrongSource = sourceId;
                wrongSource.FileId.Identifier[0] ^= 1;
                const DWORD rejected = RecoverSameVolume(sourceParent.get(), sourceParentId, wrongSource, sourceLeaf,
                    targetParent.get(), targetParentId, targetLeaf, journal.get(), nullptr, MAXULONGLONG, TrustedFixture);
                Check(rejected == ERROR_INVALID_DATA && Exists(backupPath), "recovery rejects a different source identity", rejected);
            }
            bool completed = true;
            const DWORD error = RecoverSameVolume(sourceParent.get(), sourceParentId, sourceId, sourceLeaf,
                targetParent.get(), targetParentId, targetLeaf, journal.get(), nullptr, MAXULONGLONG, TrustedFixture, &completed);
            if (scenario == 7)
            {
                Check(error == ERROR_FILE_INVALID && !completed && Exists(backupPath) && !Empty(journal.get()),
                      "changed backup ADS prevents destructive recovery", error);
                continue;
            }
            Check(!error && completed == (scenario >= 5) && Empty(journal.get()),
                  "recovery reports completion only after the original source object reached its final name", error);
            if (error) continue;
            if (scenario == 3 || scenario == 4)
            {
                Check(Exists(sourcePath) && Exists(targetPath) && !Exists(backupPath),
                      "interruption before source publication restores the original target and source");
                continue;
            }
        }
        else Check(!result.error && !result.recoveryError && result.moved && Empty(journal.get()),
                   "publish an attributed same-volume move and retire its original target", result.error ? result.error : result.recoveryError);
        source = Open(targetPath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA | READ_CONTROL);
        Revision finalRevision;
        BYTE actual[32]{};
        const DWORD error = !source ? GetLastError() : UacZipUpdate::ReadRevision(source.get(), finalRevision);
        Check(!error && UacRenameLeaf::Detail::SameSource(sourceId, UacZipUpdate::RevisionIdentity(finalRevision)) &&
              !Exists(sourcePath), "same-volume replacement keeps the original source NTFS identity", error);
        Check(source && Security(source.get()) == security && (finalRevision.attributes & attributes) == attributes,
              "same-volume replacement preserves source DACL and read-only hidden system attributes");
        const DWORD hashed = source ? UacZipUpdate::Detail::DigestReplacement(targetParent.get(), targetLeaf, source.get(),
            finalRevision, true, actual) : ERROR_INVALID_HANDLE;
        Check(!hashed && !std::memcmp(digest, actual, sizeof(digest)), "default and named source streams survive the move unchanged", hashed);
    }

    // This standard-token composition verifies owner/group/DACL only. The
    // production adapter requests ACCESS_SYSTEM_SECURITY and verifies SACL too;
    // that additional privilege boundary remains a separate elevated VM check.
    for (int scenario = 0; scenario < 3; ++scenario)
    {
        const auto sourceLeaf = L"guard-source-東-" + std::to_wstring(scenario);
        const auto targetLeaf = L"guard-target-日-" + std::to_wstring(scenario);
        const auto aliasLeaf = L"guard-alias-" + std::to_wstring(scenario);
        const auto sourcePath = sourceFolder + L"\\" + sourceLeaf;
        const auto targetPath = targetFolder + L"\\" + targetLeaf;
        const DWORD sourceSize = 20 * 1024 * 1024 + 7;
        if (!Write(sourcePath, sourceSize, 0x55) || !Write(sourcePath + L":data-東", 79, 0xAA) ||
            !SetFileAttributesW(sourcePath.c_str(), attributes) || !Write(targetPath, 43, 0x11) ||
            !Write(targetPath + L":old-data", 47, 0x22) || !SetPrivateDacl(targetPath) ||
            !SetFileAttributesW(targetPath.c_str(), attributes)) return 2;
        auto source = Open(sourcePath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA);
        Revision sourceRevision;
        BYTE digest[32]{};
        if (!source || UacZipUpdate::ReadRevision(source.get(), sourceRevision) ||
            UacZipUpdate::Detail::DigestReplacement(sourceParent.get(), sourceLeaf, source.get(), sourceRevision, true, digest)) return 2;
        const auto sourceId = UacZipUpdate::RevisionIdentity(sourceRevision);
        source.reset();
        UacMoveSourceJournal::SourceGuard guard(attributes, TrustedFixture);
        DWORD error = guard.Open(sourceParent.get(), sourceLeaf, sourceId, 1);
        if (!error) error = guard.AddRecoveryLink(sourceParent.get(), aliasLeaf);
        if (!error) error = guard.LockStreams(sourceParent.get(), aliasLeaf);
        Check(!error, "guard original and named source streams for overwrite publication", error);
        if (error) return 2;
        Handle original(nullptr, CloseHandle);
        error = Detail::Open(targetParent.get(), targetLeaf, original, DELETE | FILE_WRITE_ATTRIBUTES | READ_CONTROL);
        Revision targetRevision;
        if (error || UacZipUpdate::ReadRevision(original.get(), targetRevision)) return 2;
        auto security = Security(original.get());
        if (security.empty()) return 2;
        HANDLE raw = CreateFileW((folder + L"\\guard-journal-" + std::to_wstring(scenario)).c_str(),
            GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw == INVALID_HANDLE_VALUE) return 2;
        Handle journal(raw, CloseHandle), published(nullptr, CloseHandle);
        FILE_ID_INFO publishedId{};
        const SecurityVerifier securityVerifier = [](HANDLE file, void* context) -> DWORD {
            const auto& expected = *static_cast<std::vector<BYTE>*>(context);
            return Security(file) == expected ? ERROR_SUCCESS : ERROR_INVALID_SECURITY_DESCR;
        };
        const auto publication = PublishAcrossVolume(guard, targetParent.get(), targetParentId, targetLeaf,
            targetRevision, original, security.data(), journal.get(), TrustedFixture, sourceSize,
            nullptr, scenario == 1 ? 0 : MAXULONGLONG, published, publishedId, nullptr,
            scenario == 2 ? nullptr : securityVerifier, &security, false);
        if (scenario)
        {
            Check(publication.error == static_cast<DWORD>(scenario == 1 ? ERROR_TIMEOUT : ERROR_INVALID_PARAMETER) &&
                  !publication.published && !published && Empty(journal.get()) && original &&
                  !UacZipUpdate::CheckRevision(original.get(), targetRevision) && !guard.Check(2),
                  "cancelled or unverified guarded publication preserves original source and target", publication.error);
            continue;
        }
        Check(!publication.error && !publication.recoveryError && publication.published && published && Empty(journal.get()),
              "publish a complete guarded overwrite through the actual shared disk transaction", publication.error ? publication.error : publication.recoveryError);
        if (!published) return 2;
        Revision finalRevision;
        BYTE actual[32]{};
        error = UacZipUpdate::ReadRevision(published.get(), finalRevision);
        if (!error) error = UacZipUpdate::Detail::DigestReplacement(targetParent.get(), targetLeaf, published.get(), finalRevision, true, actual);
        Check(!error && !std::memcmp(actual, digest, sizeof(digest)) && !guard.Check(2) && Exists(sourcePath),
              "guarded copy keeps source until exact retirement and preserves every stream", error);
        Check(Security(published.get()) == security && finalRevision.created == targetRevision.created &&
              (finalRevision.attributes & attributes) == attributes,
              "guarded overwrite retains target DACL and creation time with source attributes");
    }
    std::printf("Checks=%d Failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
