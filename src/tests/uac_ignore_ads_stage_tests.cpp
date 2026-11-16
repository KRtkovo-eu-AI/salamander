// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Exercise ignored ADS staging and overwrite. | Validate the exact default-stream result and protected backup. | #731 | GPT-6.1 Terra |
#define SAM_UAC_REPLACEMENT_FAULT_TEST
#include "../uacbroker/zipupdate.h"
#include <cstdio>

using Handle = UacZipUpdate::Handle;
static int failures = 0;
static void Check(bool ok, const char* text, DWORD error = 0)
{
    std::printf("%s: %s (%lu)\n", ok ? "PASS" : "FAIL", text, error);
    failures += !ok;
}
static bool Trusted(HANDLE, bool) { return true; }
static bool Untrusted(HANDLE, bool) { return false; }
static HANDLE cancelWriter = nullptr;
static bool cancelAfterBackup = false;
namespace UacRenameLeaf::Detail
{
void PauseReplacementAt(ReplacementFaultPoint point)
{
    if (cancelWriter && (point == ReplacementFaultPoint::ReceiptFlushed ||
                         (cancelAfterBackup && point == ReplacementFaultPoint::BackupRenamed)))
    {
        const BYTE cancel = transferCancelSignal;
        DWORD written = 0;
        if (!WriteFile(cancelWriter, &cancel, 1, &written, nullptr) || written != 1) std::abort();
    }
}
}
static Handle Open(const std::wstring& path, DWORD access, DWORD sharing, DWORD disposition)
{
    HANDLE h = CreateFileW(path.c_str(), access, sharing, nullptr, disposition,
                           FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    return Handle(h == INVALID_HANDLE_VALUE ? nullptr : h, CloseHandle);
}
static bool Write(const std::wstring& path, const char* data)
{
    auto file = Open(path, GENERIC_WRITE, 0, CREATE_ALWAYS);
    DWORD written = 0;
    const auto size = static_cast<DWORD>(std::strlen(data));
    return file && WriteFile(file.get(), data, size, &written, nullptr) && written == size;
}
static bool Contains(HANDLE file, const char* text)
{
    const LARGE_INTEGER start{};
    char data[64]{};
    DWORD read = 0;
    return SetFilePointerEx(file, start, nullptr, FILE_BEGIN) && ReadFile(file, data, sizeof(data), &read, nullptr) &&
           read == std::strlen(text) && !std::memcmp(data, text, read);
}
int wmain(int argc, wchar_t** argv)
{
    if (argc != 2) return 2;
    const std::wstring folder = argv[1];
    const std::wstring sourcePath = folder + L"\\source-東.txt", targetPath = folder + L"\\target-日.txt";
    if (!Write(sourcePath, "new content") || !Write(sourcePath + L":source", "source stream") ||
        !Write(targetPath, "old content") || !Write(targetPath + L":target", "target stream")) return 2;
    auto parent = Open(folder, FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, OPEN_EXISTING);
    auto source = Open(sourcePath, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA, FILE_SHARE_READ, OPEN_EXISTING);
    UacDirectoryLeaf::ParentIdentity parentId{};
    UacZipUpdate::Revision sourceRevision{};
    if (!parent || !source || UacDirectoryLeaf::ReadParentIdentity(parent.get(), parentId) ||
        UacZipUpdate::ReadRevision(source.get(), sourceRevision)) return 2;
    HANDLE raw = nullptr;
    if (UacZipUpdate::Detail::Open(parent.get(), L"target-日.txt", raw, DELETE | READ_CONTROL)) return 2;
    Handle original(raw, CloseHandle);
    UacZipUpdate::Revision targetRevision{};
    if (UacZipUpdate::ReadRevision(original.get(), targetRevision)) return 2;
    std::wstring stageName;
    if (UacCopyLeaf::Detail::StageFile(parent.get(), raw, stageName, true, nullptr, true)) return 2;
    Handle stage(raw, CloseHandle);
    auto result = UacZipUpdate::CopyIgnoredNamedStreamsToStage(source.get(), sourceRevision, stage.get(), Untrusted);
    Check(result == ERROR_ACCESS_DENIED, "reject an untrusted stage", result);
    auto wrong = sourceRevision;
    ++wrong.size;
    result = UacZipUpdate::CopyIgnoredNamedStreamsToStage(source.get(), wrong, stage.get(), Trusted);
    Check(result == ERROR_FILE_INVALID, "reject a changed source revision", result);
    result = UacZipUpdate::CopyIgnoredNamedStreamsToStage(source.get(), sourceRevision, stage.get(), Trusted);
    Check(!result && Contains(stage.get(), "new content"), "copy only correct default content to the stage", result);
    std::vector<UacCopyLeaf::Detail::DataStream> streams;
    result = UacCopyLeaf::Detail::ReadDataStreams(stage.get(), streams, UacF7::maxNativeCopyBytes);
    Check(!result && streams.size() == 1 && streams[0].suffix == L"::$DATA" && streams[0].size == 11,
          "stage excludes named source streams", result);
    auto journal = Open(folder + L"\\journal", GENERIC_READ | GENERIC_WRITE, 0, CREATE_NEW);
    if (!journal) return 2;
    BYTE before[32]{}, after[32]{};
    result = UacZipUpdate::Detail::DigestReplacement(parent.get(), L"target-日.txt", original.get(),
                                                     targetRevision, true, before);
    if (result) return 2;
    const auto cancelled = UacZipUpdate::Publish(parent.get(), parentId, L"target-日.txt", targetRevision,
        original, stage.get(), journal.get(), nullptr, 0, nullptr,
        UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
    result = UacZipUpdate::Detail::DigestReplacement(parent.get(), L"target-日.txt", original.get(),
                                                     targetRevision, true, after);
    Check(cancelled.error == ERROR_TIMEOUT && !cancelled.published && !result &&
          !std::memcmp(before, after, sizeof(before)),
          "timeout leaves original default data and named streams untouched", result);
    auto wrongTarget = targetRevision;
    ++wrongTarget.size;
    const auto stale = UacZipUpdate::Publish(parent.get(), parentId, L"target-日.txt", wrongTarget,
        original, stage.get(), journal.get(), nullptr, MAXULONGLONG, nullptr,
        UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
    Check(stale.error == ERROR_FILE_INVALID && !stale.published &&
          !UacZipUpdate::CheckRevision(original.get(), targetRevision),
          "changed target revision requires a new overwrite decision", stale.error);
    HANDLE cancelReaderRaw = nullptr, cancelWriterRaw = nullptr;
    if (!CreatePipe(&cancelReaderRaw, &cancelWriterRaw, nullptr, 0)) return 2;
    Handle cancelReader(cancelReaderRaw, CloseHandle), cancelPipeWriter(cancelWriterRaw, CloseHandle);
    cancelWriter = cancelPipeWriter.get();
    cancelAfterBackup = true;
    const auto liveCancel = UacZipUpdate::Publish(parent.get(), parentId, L"target-日.txt", targetRevision,
        original, stage.get(), journal.get(), cancelReader.get(), MAXULONGLONG, nullptr,
        UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
    cancelAfterBackup = false;
    cancelWriter = nullptr;
    BYTE cancelSignal = 0;
    DWORD signalRead = 0;
    if (!ReadFile(cancelReader.get(), &cancelSignal, 1, &signalRead, nullptr) || signalRead != 1 ||
        UacZipUpdate::ReadRevision(original.get(), targetRevision)) return 2;
    result = UacZipUpdate::Detail::DigestReplacement(parent.get(), L"target-日.txt", original.get(),
                                                     targetRevision, true, after);
    LARGE_INTEGER journalLength{};
    Check(liveCancel.error == ERROR_CANCELLED && !liveCancel.published && !liveCancel.recoveryError &&
          !result && !std::memcmp(before, after, sizeof(before)) &&
          GetFileAttributesW(targetPath.c_str()) != INVALID_FILE_ATTRIBUTES &&
          GetFileSizeEx(journal.get(), &journalLength) && !journalLength.QuadPart,
          "live cancellation after backup rename restores original name and ADS", liveCancel.error);
    const auto published = UacZipUpdate::Publish(parent.get(), parentId, L"target-日.txt", targetRevision,
        original, stage.get(), journal.get(), nullptr, MAXULONGLONG, nullptr,
        UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
    Check(!published.error && !published.recoveryError && published.published,
          "journalled overwrite verifies and retires the original target", published.error);
    streams.clear();
    result = UacCopyLeaf::Detail::ReadDataStreams(stage.get(), streams, UacF7::maxNativeCopyBytes);
    Check(!result && streams.size() == 1 && streams[0].suffix == L"::$DATA" && Contains(stage.get(), "new content"),
          "replacement excludes both source and old target named streams", result);

    const std::wstring recoveryPath = folder + L"\\recovery.txt";
    if (!Write(recoveryPath, "old recovery") || !Write(recoveryPath + L":prior", "original prior stream")) return 2;
    raw = nullptr;
    if (UacZipUpdate::Detail::Open(parent.get(), L"recovery.txt", raw, DELETE | READ_CONTROL)) return 2;
    Handle recoveryOriginal(raw, CloseHandle);
    UacZipUpdate::Revision recoveryRevision{};
    BYTE recoveryDigest[32]{};
    if (UacZipUpdate::ReadRevision(recoveryOriginal.get(), recoveryRevision) ||
        UacZipUpdate::Detail::DigestReplacement(parent.get(), L"recovery.txt", recoveryOriginal.get(),
                                               recoveryRevision, true, recoveryDigest)) return 2;
    std::wstring recoveryStageName;
    if (UacCopyLeaf::Detail::StageFile(parent.get(), raw, recoveryStageName, true, nullptr, true)) return 2;
    Handle recoveryStage(raw, CloseHandle);
    if (UacZipUpdate::CopyIgnoredNamedStreamsToStage(source.get(), sourceRevision, recoveryStage.get(), Trusted)) return 2;
    auto recoveryJournal = Open(folder + L"\\recovery-journal", GENERIC_READ | GENERIC_WRITE, 0, CREATE_NEW);
    HANDLE readerRaw = nullptr, writerRaw = nullptr;
    if (!recoveryJournal || !CreatePipe(&readerRaw, &writerRaw, nullptr, 0)) return 2;
    Handle reader(readerRaw, CloseHandle), writer(writerRaw, CloseHandle);
    cancelWriter = writer.get();
    const auto interrupted = UacZipUpdate::Publish(parent.get(), parentId, L"recovery.txt", recoveryRevision,
        recoveryOriginal, recoveryStage.get(), recoveryJournal.get(), reader.get(), MAXULONGLONG, nullptr,
        UacZipUpdate::ReplacementKind::DiskCopy, Trusted);
    cancelWriter = nullptr;
    Check(interrupted.published && interrupted.error == ERROR_SUCCESS && interrupted.recoveryError == ERROR_CANCELLED,
          "cancellation after receipt retains the original backup for recovery", interrupted.recoveryError);
    UacZipUpdate::Detail::NamedBackupRecord record{};
    DWORD read = 0;
    const LARGE_INTEGER start{};
    if (!SetFilePointerEx(recoveryJournal.get(), start, nullptr, FILE_BEGIN) ||
        !ReadFile(recoveryJournal.get(), &record, sizeof(record), &read, nullptr) || read != sizeof(record)) return 2;
    const std::wstring backupLeaf = UacZipUpdate::Detail::BackupName(record.disk.exchange.nonce,
                                                                      UacZipUpdate::ReplacementKind::DiskCopy);
    auto backup = Open(folder + L"\\" + backupLeaf, FILE_READ_DATA | FILE_READ_ATTRIBUTES | FILE_READ_EA,
                       FILE_SHARE_READ | FILE_SHARE_DELETE, OPEN_EXISTING);
    BYTE backupDigest[32]{};
    result = backup ? UacZipUpdate::Detail::DigestReplacement(parent.get(), backupLeaf, backup.get(),
                                                               recoveryRevision, true, backupDigest) : ERROR_FILE_NOT_FOUND;
    Check(!result && !std::memcmp(recoveryDigest, backupDigest, sizeof(recoveryDigest)),
          "retained backup preserves the exact original including its ADS", result);
    backup.reset();
    recoveryStage.reset();
    BYTE cancel = 0;
    if (!ReadFile(reader.get(), &cancel, 1, &read, nullptr) || read != 1) return 2;
    result = UacZipUpdate::Recover(parent.get(), parentId, L"recovery.txt", recoveryJournal.get(),
        UacZipUpdate::ReplacementKind::DiskCopy, Trusted, UacZipUpdate::RecoveryIntent::Replace, reader.get());
    Check(result == ERROR_PARTIAL_COPY && GetFileAttributesW((folder + L"\\" + backupLeaf).c_str()) != INVALID_FILE_ATTRIBUTES,
          "legacy recovery retains its ADS backup without durable security proof", result);
    return failures ? 1 : 0;
}
