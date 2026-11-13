// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-13 | Check metadata-probe mutation rights. | Verify the broker does not grant time changes or deletion. | #731 | GPT-6 |
// | 2026-11-13 | Verify overwrite-probe metadata access. | Preserve native conflict details without granting file content access. | #731 | GPT-6 |
// | 2026-11-13 | Exercise anchor-open failure after target creation. | Verify no target handle escapes and the uncertain file is retained. | #731 | GPT-6 |
// | 2026-11-13 | Check cancellation before target create and reopen. | Verify the real dispatcher pipe preserves data at both boundaries. | #731 | GPT-6 |
// | 2026-11-13 | Exercise cancellation handle validation on the real client pipe. | Refuse invalid waits without consuming the approved session. | #731 | GPT-6 |
// | 2026-11-13 | Exercise a real client and dispatcher reopening an anchored target. | Check retry identity, rights and I/O mode on one test pipe. | #731 | GPT-6 |

// The client and dispatcher run in the same process with the same token.
// This does not test the worker, peer authentication, UAC or another account.
void CheckThinAnchorFailure(const Directory& source, const Directory& target)
{
    constexpr wchar_t sourceLeaf[] = L"anchor-failure-source.txt";
    constexpr wchar_t targetLeaf[] = L"anchor-failure-target.txt";
    if (!WriteByte(source.File(sourceLeaf), 0x54) || !IsAbsent(target.File(targetLeaf)))
    { Check(false, "prepare isolated anchor failure fixture"); return; }
    auto input = Adopt(CreateFileW(source.File(sourceLeaf).c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO sourceId{};
    JobPipe pipe;
    if (!input || !GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId)) ||
        !pipe.Open(nullptr, nullptr, Operation::FindContent, nullptr, false, nullptr, false, true))
    { Check(false, "start isolated anchor failure dispatcher"); return; }
    DWORD status = ERROR_GEN_FAILURE;
    ThinWireReply reply{};
    const bool exchanged = pipe.Thin(source, sourceLeaf, sourceId, target, targetLeaf,
        Operation::ThinTargetCreate, nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, true, 28u);
    auto preserved = Adopt(CreateFileW(target.File(targetLeaf).c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    LARGE_INTEGER size{};
    Check(exchanged && status == ERROR_PARTIAL_COPY && !reply.value && preserved &&
          GetFileSizeEx(preserved.get(), &size) && size.QuadPart == 0 && HasByte(source.File(sourceLeaf), 0x54),
          "anchor-open failure returns no handle and preserves the uncertain target and source");
}

void CheckThinClientReopen(const Directory& source, const Directory& target, bool asynchronous)
{
    const wchar_t* root = asynchronous ? L"reopen-async-\u6771" : L"reopen-sync-\u6771";
    constexpr wchar_t leaf[] = L"payload-\u65e5.txt";
    std::wstring cwd(32768, L'\0');
    const DWORD length = GetCurrentDirectoryW(static_cast<DWORD>(cwd.size()), cwd.data());
    if (!length || length >= cwd.size()) { Check(false, "get isolated reopen fixture directory"); return; }
    cwd.resize(length);
    if (!SetEnvironmentVariableW(L"SAM_UAC_THIN_UNELEVATED_TEST", L"1") ||
        !SetEnvironmentVariableW(L"SAM_UAC_THIN_TEST_INSTALL", cwd.c_str()))
    { Check(false, "configure existing unelevated client test mode"); return; }
    const std::wstring sourceRoot = cwd + L"\\source\\" + root;
    const std::wstring targetRoot = cwd + L"\\target\\" + root;
    const std::wstring sourcePath = sourceRoot + L"\\" + leaf;
    const std::wstring targetPath = targetRoot + L"\\" + leaf;
    const std::wstring parkedPath = targetRoot + L"\\parked-original.txt";
    Directory from;
    if (!from.Open(source.File(root).c_str()) || !WriteByte(from.File(leaf), 0x67))
    { Check(false, "prepare isolated Unicode source for reopen"); return; }
    UacF7::PreparedTransfer treePlan;
    if (!treePlan.PrepareTreePlan(sourceRoot, targetRoot))
    { Check(false, "prepare the actual planned tree scope"); return; }
    JobPipe pipe;
    DWORD status = ERROR_GEN_FAILURE;
    ListingReply listing{};
    std::vector<UacF7::TreeCopySourceEntry> rows;
    const bool planned = pipe.Open() &&
        pipe.Request(Operation::PlanCopyTree, source, root, status, &target, nullptr, nullptr,
                     false, nullptr, nullptr, 0) && !status &&
        pipe.ListTreeSource(from, {}, from.identity.file, listing, rows) &&
        !listing.error && rows.size() == 1 && std::wstring_view(rows.front().data.cFileName) == leaf;
    Check(planned, "plan and list actual source on the dispatcher pipe for reopen");
    if (!planned) return;
    auto handle = pipe.DetachClient();
    auto job = UacF7::ThinClientPipeTestAccess::Adopt(handle.get(), treePlan);
    (void)handle.release();
    UacF7::MoveFileIdentity rootIdentity{};
    rootIdentity.serial = from.identity.file.VolumeSerialNumber;
    std::memcpy(rootIdentity.id, from.identity.file.FileId.Identifier, sizeof(rootIdentity.id));
    UacF7::PreparedLeaf listed;
    UacF7::PreparedTransfer createRoot;
    if (!job->PrepareTreeSource({}, rootIdentity, rows.front(), listed) ||
        !createRoot.Prepare(sourceRoot, targetRoot, 0, Operation::CopyDirectoryEntry))
    { Check(false, "prepare listed source and target root for reopen"); return; }
    const auto createdRoot = createRoot.Execute(nullptr, nullptr, job.get());
    if (createdRoot.error || createdRoot.unknown || job->NeedsTreeRoot())
    { Check(false, "create planned root before reopening a target"); return; }
    UacF7::PreparedTransfer request;
    UacF7::ThinCopySession thin;
    if (!request.Prepare(sourcePath, targetPath, 1, Operation::Copy, &listed) ||
        !thin.TakePlanned(job, request))
    { Check(false, "take actual planned client session for reopen"); return; }
    auto cancelledEvent = Adopt(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (!cancelledEvent) { Check(false, "create cancellation fixture"); return; }
    const auto cancelled = thin.Open(nullptr, request, UacF7::ThinOpenKind::Source, false, false, nullptr,
                                     UacF7::ThinOpenedSource{}, UacF7::ThinCancelEvent{cancelledEvent.get()});
    Check(cancelled.error == ERROR_CANCELLED && !cancelled.unknown && thin.IsOpen(),
          "signalled cancellation stops before the broker request");
    HANDLE invalidEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!invalidEvent || !CloseHandle(invalidEvent)) { Check(false, "create invalid cancellation fixture"); return; }
    const auto invalid = thin.Open(nullptr, request, UacF7::ThinOpenKind::Source, false, false, nullptr,
                                   UacF7::ThinOpenedSource{}, UacF7::ThinCancelEvent{invalidEvent});
    Check(invalid.error == ERROR_INVALID_HANDLE && !invalid.unknown && thin.IsOpen(),
          "invalid cancellation handle stops before the broker request");
    const auto opened = thin.Open(nullptr, request, UacF7::ThinOpenKind::Source, false, false, nullptr);
    auto input = Adopt(opened.file);
    if (opened.error || opened.unknown || !input)
    { Check(false, "open verified source through actual client"); return; }
    const auto cancelledCreate = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetCreate, asynchronous, false,
                                           nullptr, UacF7::ThinOpenedSource{input.get()},
                                           UacF7::ThinCancelEvent{cancelledEvent.get()});
    Check(cancelledCreate.error == ERROR_CANCELLED && !cancelledCreate.unknown &&
          cancelledCreate.file == INVALID_HANDLE_VALUE && IsAbsent(targetPath) && thin.IsOpen(),
          "cancelled target create leaves no target and preserves the job");
    const auto created = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetCreate, asynchronous, false,
                                   nullptr, UacF7::ThinOpenedSource{input.get()});
    auto output = Adopt(created.file);
    if (created.error || created.unknown || !output)
    { Check(false, "create anchored target through actual client"); return; }

    auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    OVERLAPPED io{};
    io.hEvent = event.get();
    BYTE value = 0;
    DWORD count = 0;
    BOOL read = ReadFile(output.get(), &value, 1, &count, asynchronous ? &io : nullptr);
    DWORD readError = read ? ERROR_SUCCESS : GetLastError();
    if (!read && readError == ERROR_IO_PENDING)
    {
        read = GetOverlappedResult(output.get(), &io, &count, TRUE);
        readError = read ? ERROR_SUCCESS : GetLastError();
    }
    Check(!read && readError == ERROR_ACCESS_DENIED, "initial target handle denies read access");
    value = 0x67; count = 0;
    OVERLAPPED writeIo{};
    writeIo.hEvent = event.get();
    if (asynchronous && event) ResetEvent(event.get());
    BOOL written = WriteFile(output.get(), &value, 1, &count, asynchronous ? &writeIo : nullptr);
    if (!written && GetLastError() == ERROR_IO_PENDING)
        written = GetOverlappedResult(output.get(), &writeIo, &count, TRUE);
    Check(written && count == 1, "initial target handle writes the expected byte");
    if (!written || count != 1) return;
    if (!asynchronous)
    {
        LARGE_INTEGER offset{}; offset.QuadPart = 11;
        Check(SetFilePointerEx(output.get(), offset, nullptr, FILE_BEGIN) != FALSE,
              "set original synchronous file object's position");
    }
    const auto busy = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetRetry, asynchronous, false,
                                &created.targetIdentity, UacF7::ThinOpenedSource{input.get()});
    auto busyHandle = Adopt(busy.file);
    Check(busy.error == ERROR_SHARING_VIOLATION && !busy.unknown && !busyHandle,
          "retry refuses while the original exclusive writer is still open");
    output.reset();
    const auto metadata = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetProbe, false, false,
                                    nullptr, UacF7::ThinOpenedSource{input.get()});
    auto metadataHandle = Adopt(metadata.file);
    FILETIME modified{};
    FILE_ATTRIBUTE_TAG_INFO tag{};
    BY_HANDLE_FILE_INFORMATION nativeInfo{};
    LARGE_INTEGER metadataSize{};
    Check(!metadata.error && !metadata.unknown && metadataHandle &&
          GetFileTime(metadataHandle.get(), nullptr, nullptr, &modified) &&
          GetFileInformationByHandle(metadataHandle.get(), &nativeInfo) &&
          GetFileSizeEx(metadataHandle.get(), &metadataSize) && metadataSize.QuadPart == 1 &&
          GetFileInformationByHandleEx(metadataHandle.get(), FileAttributeTagInfo, &tag, sizeof(tag)),
          "target probe supplies timestamp and attributes for the native overwrite dialog");
    BYTE metadataByte = 0;
    DWORD metadataCount = 0;
    Check(metadataHandle && !ReadFile(metadataHandle.get(), &metadataByte, 1, &metadataCount, nullptr) &&
          GetLastError() == ERROR_ACCESS_DENIED &&
          !WriteFile(metadataHandle.get(), &metadataByte, 1, &metadataCount, nullptr) &&
          GetLastError() == ERROR_ACCESS_DENIED,
          "target probe grants neither content read nor write");
    FILETIME changed = modified;
    changed.dwLowDateTime ^= 1;
    Check(metadataHandle && !SetFileTime(metadataHandle.get(), nullptr, nullptr, &changed) &&
          GetLastError() == ERROR_ACCESS_DENIED,
          "target probe cannot change timestamps");
    FILE_DISPOSITION_INFO disposition{TRUE};
    Check(metadataHandle && !SetFileInformationByHandle(metadataHandle.get(), FileDispositionInfo,
                                                       &disposition, sizeof(disposition)) &&
          GetLastError() == ERROR_ACCESS_DENIED,
          "target probe cannot delete the existing file");
    metadataHandle.reset();
    const auto cancelledRetry = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetRetry, asynchronous, false,
                                          &created.targetIdentity, UacF7::ThinOpenedSource{input.get()},
                                          UacF7::ThinCancelEvent{cancelledEvent.get()});
    Check(cancelledRetry.error == ERROR_CANCELLED && !cancelledRetry.unknown &&
          cancelledRetry.file == INVALID_HANDLE_VALUE && HasByte(targetPath, 0x67) && thin.IsOpen(),
          "cancelled target retry preserves existing content and the job");
    UacF7::MoveFileIdentity wrong = created.targetIdentity;
    wrong.id[0] ^= 1;
    const auto refused = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetRetry, asynchronous, false,
                                   &wrong, UacF7::ThinOpenedSource{input.get()});
    auto refusedHandle = Adopt(refused.file);
    Check(refused.error == ERROR_FILE_INVALID && !refused.unknown && !refusedHandle,
          "retry refuses an incorrect expected target identity");
    const auto reopened = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetRetry, asynchronous, false,
                                    &created.targetIdentity, UacF7::ThinOpenedSource{input.get()});
    auto retried = Adopt(reopened.file);
    Check(!reopened.error && !reopened.unknown && retried, "retry opens the same verified target");
    if (reopened.error || reopened.unknown || !retried) return;
    if (!asynchronous)
    {
        LARGE_INTEGER zero{}, current{};
        Check(SetFilePointerEx(retried.get(), zero, &current, FILE_CURRENT) && current.QuadPart == 0,
              "reopened synchronous file object has an independent initial position");
    }
    OVERLAPPED readIo{};
    readIo.hEvent = event.get();
    if (asynchronous && event) ResetEvent(event.get());
    value = 0; count = 0;
    read = ReadFile(retried.get(), &value, 1, &count, asynchronous ? &readIo : nullptr);
    if (!read && GetLastError() == ERROR_IO_PENDING)
        read = GetOverlappedResult(retried.get(), &readIo, &count, TRUE);
    Check(read && count == 1 && value == 0x67, "reopened target supports native tail verification");
    retried.reset();

    if (!MoveFileW(targetPath.c_str(), parkedPath.c_str()))
    { Check(false, "park the anchored target without removing its original identity"); return; }
    const auto missing = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetRetry, asynchronous, false,
                                   &created.targetIdentity, UacF7::ThinOpenedSource{input.get()});
    auto missingHandle = Adopt(missing.file);
    Check(missing.error == ERROR_FILE_NOT_FOUND && !missing.unknown && !missingHandle &&
          IsAbsent(targetPath) && HasByte(parkedPath, 0x67), "retry does not recreate a missing target");
    if (!WriteByte(targetPath, 0x72)) { Check(false, "create replacement fixture"); return; }
    const auto replaced = thin.Open(nullptr, request, UacF7::ThinOpenKind::TargetRetry, asynchronous, false,
                                    &created.targetIdentity, UacF7::ThinOpenedSource{input.get()});
    auto replacedHandle = Adopt(replaced.file);
    Check(replaced.error == ERROR_FILE_INVALID && !replaced.unknown && !replacedHandle &&
          HasByte(targetPath, 0x72) && HasByte(parkedPath, 0x67),
          "retry refuses a different object without changing either file");
    if (!DeleteFileW(targetPath.c_str()) || !MoveFileW(parkedPath.c_str(), targetPath.c_str()))
    { Check(false, "restore the verified target fixture"); return; }
    Check(thin.ReleaseTarget(request, created.targetIdentity) == ERROR_SUCCESS && HasByte(targetPath, 0x67),
          "release closes the anchor and leaves the verified target intact");
}
