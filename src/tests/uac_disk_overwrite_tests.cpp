/*
| Date | Summary | Reason | Ticket | Model |
| 2026-11-11 | Keep legacy ADS backups after interrupted publication. | Do not discard security state that the old journal cannot prove. | #731 | GPT-6.1 Terra |
| 2026-11-11 | Exercise large attributed disk replacements and cancellable durable receipts. | Verify stream preservation, atomic read-only cleanup and recovery on real NTFS. | #731 | GPT-6.1 Sol |
*/
#define SAM_UAC_REPLACEMENT_FAULT_TEST
#define SAM_UAC_DISK_RECEIPT_TEST
#include "../uacbroker/zipupdate.h"
#include <cstdio>
#include <string>

namespace
{
using Handle = UacZipUpdate::Handle;
HANDLE cancelWriter = nullptr;
bool cancelAfterReceipt = false;
bool cancelDuringReceiptHash = false;
int crashPhase = -1;
int checks = 0, failures = 0;
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
bool Write(const std::wstring& path, ULONGLONG size, BYTE value)
{
    HANDLE raw = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    Handle file(raw == INVALID_HANDLE_VALUE ? nullptr : raw, CloseHandle);
    std::vector<BYTE> block(64 * 1024, value);
    while (file && size)
    {
        const DWORD count = static_cast<DWORD>((std::min)(size, static_cast<ULONGLONG>(block.size())));
        DWORD written = 0;
        if (!WriteFile(file.get(), block.data(), count, &written, nullptr) || written != count) return false;
        size -= count;
    }
    return file && FlushFileBuffers(file.get());
}
bool SetAttributes(HANDLE file, DWORD flags)
{
    FILE_BASIC_INFO basic{};
    basic.FileAttributes = flags;
    return SetFileInformationByHandle(file, FileBasicInfo, &basic, sizeof(basic)) != FALSE;
}
bool Attrs(HANDLE file, DWORD required)
{
    FILE_BASIC_INFO info{};
    return GetFileInformationByHandleEx(file, FileBasicInfo, &info, sizeof(info)) &&
        (info.FileAttributes & required) == required;
}
struct CancelWhileHashing
{
    HANDLE file;
    HANDLE writer;
    bool sent = false;
};
DWORD WINAPI CancelActiveHash(void* raw)
{
    auto& context = *static_cast<CancelWhileHashing*>(raw);
    const ULONGLONG until = GetTickCount64() + 3000;
    do
    {
        IO_STATUS_BLOCK io{};
        LARGE_INTEGER position{};
        constexpr auto positionClass = static_cast<FILE_INFORMATION_CLASS>(14);
        if (NtQueryInformationFile(context.file, &io, &position, sizeof(position), positionClass) >= 0 && position.QuadPart > 0)
        {
            DWORD written = 0;
            const BYTE signal = UacRenameLeaf::Detail::transferCancelSignal;
            context.sent = WriteFile(context.writer, &signal, 1, &written, nullptr) && written == 1;
            return 0;
        }
        Sleep(0);
    } while (GetTickCount64() < until);
    return 1;
}
bool Empty(HANDLE journal)
{
    LARGE_INTEGER size{};
    return GetFileSizeEx(journal, &size) && !size.QuadPart;
}
}
namespace UacRenameLeaf::Detail
{
void PauseReplacementAt(ReplacementFaultPoint point)
{
    const ReplacementFaultPoint crashPoints[] = {ReplacementFaultPoint::BackupRenamed, ReplacementFaultPoint::StageRenamed,
        ReplacementFaultPoint::StageKept, ReplacementFaultPoint::ReceiptFlushed};
    if (crashPhase >= 0 && point == crashPoints[crashPhase]) TerminateProcess(GetCurrentProcess(), 91);
    if (cancelAfterReceipt && point == ReplacementFaultPoint::ReceiptFlushed)
    {
        DWORD written = 0;
        const BYTE signal = transferCancelSignal;
        if (!WriteFile(cancelWriter, &signal, 1, &written, nullptr) || written != 1) std::abort();
    }
}
}

namespace UacZipUpdate::Detail
{
void DiskReceiptReadBlock()
{
    if (!cancelDuringReceiptHash) return;
    cancelDuringReceiptHash = false;
    const BYTE signal = UacRenameLeaf::Detail::transferCancelSignal;
    DWORD written = 0;
    if (!WriteFile(cancelWriter, &signal, 1, &written, nullptr) || written != 1) std::abort();
}
}

int CrashChild(const std::wstring& folder, int phase)
{
    using namespace UacZipUpdate;
    const auto parent = Open(folder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    UacDirectoryLeaf::ParentIdentity identity;
    if (!parent || UacDirectoryLeaf::ReadParentIdentity(parent.get(), identity)) return 2;
    const std::wstring leaf = L"crash-\u6771.bin", path = folder + L"\\" + leaf;
    if (!Write(path, 43, 0x11) || !Write(path + L":old-data", 47, 0x22) ||
        !SetFileAttributesW(path.c_str(), attributes)) return 2;
    HANDLE raw = nullptr;
    if (Detail::Open(parent.get(), leaf, raw, DELETE | FILE_WRITE_ATTRIBUTES | READ_CONTROL)) return 2;
    Handle original(raw, CloseHandle);
    Revision revision;
    if (ReadRevision(original.get(), revision)) return 2;
    std::wstring name;
    if (UacCopyLeaf::Detail::StageFile(parent.get(), raw, name, true, nullptr, true)) return 2;
    Handle stage(raw, CloseHandle);
    DWORD written = 0;
    const BYTE bytes[] = {0x55, 0xAA};
    FILE_DISPOSITION_INFO_EX rollback{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_ON_CLOSE |
        FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE};
    if (!WriteFile(stage.get(), bytes, sizeof(bytes), &written, nullptr) || written != sizeof(bytes) ||
        !SetFileInformationByHandle(stage.get(), FileDispositionInfoEx, &rollback, sizeof(rollback)) ||
        !SetAttributes(stage.get(), attributes | FILE_ATTRIBUTE_ARCHIVE)) return 2;
    raw = CreateFileW((folder + L"\\journal").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (raw == INVALID_HANDLE_VALUE) return 2;
    Handle journal(raw, CloseHandle);
    crashPhase = phase;
    Publish(parent.get(), identity, leaf, revision, original, stage.get(), journal.get(), nullptr, MAXULONGLONG,
        nullptr, ReplacementKind::DiskCopy, TrustedFixture);
    return 3; // Every child must terminate at its requested transaction checkpoint.
}

int wmain(int argc, wchar_t** argv)
{
    if (argc == 4) return CrashChild(argv[1], _wtoi(argv[3]));
    if (argc != 2) return 2;
    using namespace UacZipUpdate;
    using namespace UacZipUpdate::Detail;
    const std::wstring folder = argv[1];
    const auto parent = Open(folder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
    UacDirectoryLeaf::ParentIdentity parentId;
    if (!parent || UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentId)) return 2;
    const std::wstring sourceLeaf = L"zdroj-\u6771.bin", sourcePath = folder + L"\\" + sourceLeaf;
    constexpr ULONGLONG mainSize = 20ull * 1024 * 1024 + 7, streamSize = 17ull * 1024 * 1024 + 3;
    if (!Write(sourcePath, mainSize, 0x55) || !Write(sourcePath + L":data-\u65e5", streamSize, 0xAA) ||
        !Write(sourcePath + L":empty", 0, 0) || !SetFileAttributesW(sourcePath.c_str(), attributes)) return 2;
    auto source = Open(sourcePath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA);
    Revision sourceRevision;
    if (!source || ReadRevision(source.get(), sourceRevision)) return 2;
    const auto sourceId = RevisionIdentity(sourceRevision);
    BYTE expected[32]{}, actual[32]{};
    DWORD error = DigestReplacement(parent.get(), sourceLeaf, source.get(), sourceRevision, true, expected);
    Check(!error, "hash a read-only hidden system source with default and named streams above 16 MiB", error);
    if (error) return 2;
    error = DigestReplacement(parent.get(), sourceLeaf, source.get(), sourceRevision, true, actual, nullptr, 0);
    Check(error == ERROR_TIMEOUT, "stop receipt hashing at an expired deadline", error);
    HANDLE readPipe = nullptr, writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, nullptr, 0)) return 2;
    Handle connection(readPipe, CloseHandle), writer(writePipe, CloseHandle);
    cancelWriter = writer.get();
    BYTE cancel = UacRenameLeaf::Detail::transferCancelSignal;
    DWORD count = 0;
    if (!WriteFile(writer.get(), &cancel, 1, &count, nullptr)) return 2;
    error = DigestReplacement(parent.get(), sourceLeaf, source.get(), sourceRevision, true, actual, connection.get());
    Check(error == ERROR_CANCELLED, "stop receipt hashing on the operation cancellation channel", error);
    if (!ReadFile(connection.get(), &cancel, 1, &count, nullptr)) return 2;
    if (!SetFilePointerEx(source.get(), {}, nullptr, FILE_BEGIN)) return 2;
    CancelWhileHashing active{source.get(), writer.get()};
    Handle watcher(CreateThread(nullptr, 0, CancelActiveHash, &active, 0, nullptr), CloseHandle);
    if (!watcher) return 2;
    error = DigestReplacement(parent.get(), sourceLeaf, source.get(), sourceRevision, true, actual, connection.get());
    if (WaitForSingleObject(watcher.get(), 5000) != WAIT_OBJECT_0) std::abort();
    Check(active.sent && error == ERROR_CANCELLED, "cancel an already active hash between bounded read blocks", error);
    if (active.sent && !ReadFile(connection.get(), &cancel, 1, &count, nullptr)) return 2;

    for (int scenario = 0; scenario < 5; ++scenario)
    {
        const std::wstring leaf = L"target-\u65e5-" + std::to_wstring(scenario) + L".bin";
        const std::wstring path = folder + L"\\" + leaf;
        const DWORD originalAttributes = scenario == 4 ? FILE_ATTRIBUTE_NORMAL : attributes;
        if (!Write(path, 43, 0x11) || !Write(path + L":old-data", 47, 0x22) ||
            !SetFileAttributesW(path.c_str(), originalAttributes)) return 2;
        HANDLE raw = nullptr;
        if (Open(parent.get(), leaf, raw, DELETE | FILE_WRITE_ATTRIBUTES | READ_CONTROL)) return 2;
        Handle original(raw, CloseHandle);
        Revision oldRevision;
        if (ReadRevision(original.get(), oldRevision)) return 2;
        std::wstring stageName;
        if (UacCopyLeaf::Detail::StageFile(parent.get(), raw, stageName, true, nullptr, true)) return 2;
        Handle stage(raw, CloseHandle);
        const auto stagePath = folder + L"\\" + stageName;
        UacCopyLeaf::Detail::CopySourceLocation location{parent.get(), sourceLeaf, sourceId, TrustedFixture};
        if (!SetFilePointerEx(source.get(), {}, nullptr, FILE_BEGIN)) return 2;
        error = UacCopyLeaf::Detail::CopyOverwriteStreams(source.get(), location, parent.get(), stageName,
            stage.get(), mainSize, connection.get(), MAXULONGLONG);
        Check(!error, "stream a large attributed default stream and ADS into the bounded-memory stage", error);
        if (error) return 2;
        FILE_DISPOSITION_INFO_EX cleanup{FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_ON_CLOSE |
            FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE};
        if (!SetFileInformationByHandle(stage.get(), FileDispositionInfoEx, &cleanup, sizeof(cleanup)) ||
            !SetAttributes(stage.get(), attributes | FILE_ATTRIBUTE_ARCHIVE)) return 2;
        const auto journalPath = path + L".journal";
        raw = CreateFileW(journalPath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (raw == INVALID_HANDLE_VALUE) return 2;
        Handle journal(raw, CloseHandle);
        cancelAfterReceipt = scenario >= 2;
        auto publication = Publish(parent.get(), parentId, leaf, oldRevision, original, stage.get(), journal.get(),
            connection.get(), scenario == 1 ? 0 : MAXULONGLONG, nullptr, ReplacementKind::DiskCopy, TrustedFixture);
        cancelAfterReceipt = false;
        if (scenario == 1)
        {
            Check(publication.error == ERROR_TIMEOUT && !publication.published && Empty(journal.get()) &&
                !CheckRevision(original.get(), oldRevision), "cancellation preserves the original revision and empty journal", publication.error);
            stage.reset();
            Check(!Exists(stagePath) && Exists(path), "read-only cancelled stage is removed without changing the original");
            continue;
        }
        Check(!publication.error && publication.published && Attrs(stage.get(), attributes),
            "publish the complete read-only hidden system replacement", publication.error);
        if (!publication.published) return 2;
        Revision replacement;
        if (ReadRevision(stage.get(), replacement)) return 2;
        error = DigestReplacement(parent.get(), leaf, stage.get(), replacement, true, actual);
        Check(!error && !std::memcmp(actual, expected, sizeof(actual)), "published default and named streams match the original source", error);
        Check(!CheckRevision(source.get(), sourceRevision), "copy leaves the read-only source revision unchanged");
        stage.reset();
        if (scenario == 0)
        {
            Check(!publication.recoveryError && Empty(journal.get()), "retire a read-only ADS backup after durable receipt verification", publication.recoveryError);
            continue;
        }
        Check(publication.recoveryError == ERROR_CANCELLED && !Empty(journal.get()),
            "cancellation after the durable receipt retains the original backup", publication.recoveryError);
        NamedBackupRecord record;
        if (!SetFilePointerEx(journal.get(), {}, nullptr, FILE_BEGIN) ||
            !ReadFile(journal.get(), &record, sizeof(record), &count, nullptr) || count != sizeof(record)) return 2;
        const auto backupPath = folder + L"\\" + BackupName(record.disk.exchange.nonce, ReplacementKind::DiskCopy);
        const DWORD requiredBackupAttributes = scenario == 4 ? 0 : attributes;
        Check(Exists(backupPath) && (GetFileAttributesW(backupPath.c_str()) & attributes) == requiredBackupAttributes,
            "retained backup preserves the original storage attributes");
        error = Recover(parent.get(), parentId, leaf, journal.get(), ReplacementKind::DiskCopy, TrustedFixture,
            RecoveryIntent::Replace, connection.get());
        Check(error == ERROR_CANCELLED && Exists(backupPath), "cancelled recovery leaves both verified names intact", error);
        if (!ReadFile(connection.get(), &cancel, 1, &count, nullptr)) return 2;
        if (scenario == 2)
        {
            cancelDuringReceiptHash = true;
            error = Recover(parent.get(), parentId, leaf, journal.get(), ReplacementKind::DiskCopy, TrustedFixture,
                RecoveryIntent::Replace, connection.get());
            Check(cancelDuringReceiptHash && error == ERROR_PARTIAL_COPY && Exists(backupPath) && !Empty(journal.get()),
                "legacy named recovery blocks before losing unverified security state", error);
            cancelDuringReceiptHash = false;
        }
        if (scenario == 3)
        {
            if (!SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM) ||
                !Write(path + L":data-\u65e5", streamSize, 0xCC) || !SetFileAttributesW(path.c_str(), attributes)) return 2;
        }
        error = Recover(parent.get(), parentId, leaf, journal.get(), ReplacementKind::DiskCopy, TrustedFixture,
            RecoveryIntent::Replace, connection.get());
        Check(error == ERROR_PARTIAL_COPY && Exists(backupPath) && !Empty(journal.get()),
            "legacy receipt retains its original ADS backup and recovery record", error);
    }
    for (int phase = 0; phase < 4; ++phase)
    {
        const auto childFolder = folder + L"\\crash-" + std::to_wstring(phase);
        if (!CreateDirectoryW(childFolder.c_str(), nullptr)) return 2;
        const std::wstring command = L"\"" + std::wstring(argv[0]) + L"\" \"" + childFolder + L"\" --crash " + std::to_wstring(phase);
        std::vector<wchar_t> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back(0);
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(argv[0], mutableCommand.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                nullptr, nullptr, &startup, &process)) return 2;
        Handle child(process.hProcess, CloseHandle), thread(process.hThread, CloseHandle);
        if (WaitForSingleObject(child.get(), 10000) != WAIT_OBJECT_0) { TerminateProcess(child.get(), 92); return 2; }
        DWORD exitCode = 0;
        if (!GetExitCodeProcess(child.get(), &exitCode) || exitCode != 91) return 2;
        auto childParent = Open(childFolder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE);
        UacDirectoryLeaf::ParentIdentity identity;
        if (!childParent || UacDirectoryLeaf::ReadParentIdentity(childParent.get(), identity)) return 2;
        auto journal = Open(childFolder + L"\\journal", GENERIC_READ | GENERIC_WRITE, 0);
        NamedBackupRecord record;
        if (!journal || !ReadFile(journal.get(), &record, sizeof(record), &count, nullptr) || count != sizeof(record)) return 2;
        const auto backupPath = childFolder + L"\\" + BackupName(record.disk.exchange.nonce, ReplacementKind::DiskCopy);
        Check(Exists(backupPath) && (GetFileAttributesW(backupPath.c_str()) & attributes) == attributes,
            "hard process termination retains the attributed original backup");
        const auto leaf = L"crash-\u6771.bin";
        error = Recover(childParent.get(), identity, leaf, journal.get(), ReplacementKind::DiskCopy, TrustedFixture);
        Check(error == ERROR_PARTIAL_COPY && Exists(backupPath) && !Empty(journal.get()),
            "retain legacy backup and journal after interrupted replacement", error);
    }
    std::printf("Checks=%d Failures=%d\n", checks, failures);
    return failures ? 1 : 0;
}
