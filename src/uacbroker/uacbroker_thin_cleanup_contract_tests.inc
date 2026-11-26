// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-13 | Reject old peers and exercise retained-target cleanup. | Preserve content across failed requests without relying on an unpinned target. | #731 | GPT-6 |
// | 2026-11-13 | Exercise thin cleanup and incompatible peers. | Prevent deletion of changed files and reject unsafe wire contracts. | #731 | GPT-6 |

void CheckThinCleanupContract(const Directory& source, const Directory& target)
{
    constexpr auto sourceLeaf = L"thin-contract-source.txt";
    const auto sourcePath = source.File(sourceLeaf);
    Check(WriteByte(sourcePath, 0x44), "prepare thin contract source");
    auto input = Adopt(CreateFileW(sourcePath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO sourceId{};
    Check(input && GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId)),
          "record contract source identity");
    for (const auto operation : {Operation::ThinOpenSource, Operation::ThinTargetCreate,
             Operation::ThinTargetRetry, Operation::ThinTargetProbe, Operation::ThinDiscardTarget,
             Operation::ThinTargetRelease})
    {
      for (DWORD contract : {0u, 4u, 8u, 28u})
      {
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const auto ordinaryCopy = target.File(sourceLeaf);
        const auto deniedTarget = target.File(L"thin-contract-matrix-target.txt");
        const bool ready = job.Open() && job.Request(Operation::Copy, source, sourceLeaf, status, &target) && !status;
        Check(ready && HasByte(ordinaryCopy, 0x44), "ordinary copy succeeds before incompatible thin request");
#ifdef SAM_UAC_THIN_COPY_TEST
        const bool sent = ready && job.Thin(source, sourceLeaf, sourceId, target,
            L"thin-contract-matrix-target.txt", operation, &sourceId, false, status, reply,
            input.get(), INVALID_HANDLE_VALUE, false, contract);
#else
        const bool sent = ready && job.Thin(source, sourceLeaf, sourceId, target,
            L"thin-contract-matrix-target.txt", operation, &sourceId, false, status, reply,
            input.get(), INVALID_HANDLE_VALUE, contract);
#endif
        const DWORD dispatchError = job.Finish();
#ifdef SAM_UAC_THIN_COPY_TEST
        Check(ready && !sent && dispatchError == ERROR_INVALID_DATA && HasByte(ordinaryCopy, 0x44) &&
              GetFileAttributesW(deniedTarget.c_str()) == INVALID_FILE_ATTRIBUTES,
              "incompatible experimental thin contract is rejected without mutation");
#else
        Check(ready && !sent && dispatchError == ERROR_INVALID_DATA && HasByte(ordinaryCopy, 0x44) &&
              GetFileAttributesW(deniedTarget.c_str()) == INVALID_FILE_ATTRIBUTES,
              "nonexperimental dispatcher rejects thin operation before reading its contract");
#endif
        Check(DeleteFileW(ordinaryCopy.c_str()) != FALSE, "remove ordinary-copy matrix fixture");
      }
    }
    for (const DWORD contract : {0u, 4u, 8u, 28u})
    {
#ifndef SAM_UAC_THIN_COPY_TEST
        if (contract != 0u) continue;
#endif
        for (const bool occupied : {false, true})
        {
            JobPipe job;
            DWORD status = ERROR_GEN_FAILURE;
            ThinWireReply reply{};
            const auto path = target.File(L"thin-contract-refused-create.txt");
            FILE_ID_INFO originalId{};
            if (occupied)
            {
                Check(WriteByte(path, 0x6b), "prepare existing incompatible-peer target");
                const auto original = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
                Check(original && GetFileInformationByHandleEx(original.get(), FileIdInfo, &originalId, sizeof(originalId)),
                      "capture existing incompatible-peer target identity");
            }
            const bool opened = job.Open();
#ifdef SAM_UAC_THIN_COPY_TEST
            const bool sent = opened && job.Thin(source, sourceLeaf, sourceId, target,
                L"thin-contract-refused-create.txt", Operation::ThinTargetCreate, nullptr, false, status, reply,
                input.get(), INVALID_HANDLE_VALUE, false, contract);
#else
            const bool sent = opened && job.Thin(source, sourceLeaf, sourceId, target,
                L"thin-contract-refused-create.txt", Operation::ThinTargetCreate, nullptr, false, status, reply,
                input.get(), INVALID_HANDLE_VALUE, contract);
#endif
            const DWORD dispatchError = job.Finish();
#ifdef SAM_UAC_THIN_COPY_TEST
            Check(opened && !sent && dispatchError == ERROR_INVALID_DATA,
                  "old thin contract is rejected by the real dispatcher as invalid data");
#else
            Check(opened && !sent, "old thin contract is rejected by the real dispatcher");
            (void)dispatchError;
#endif
            if (occupied)
            {
                auto current = Adopt(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
                FILE_ID_INFO currentId{};
                LARGE_INTEGER size{};
                Check(current && GetFileInformationByHandleEx(current.get(), FileIdInfo, &currentId, sizeof(currentId)) &&
                      UacRenameLeaf::Detail::SameSource(originalId, currentId) && GetFileSizeEx(current.get(), &size) &&
                      size.QuadPart == 1 && HasByte(path, 0x6b),
                      "incompatible peer preserves existing target identity, size and contents");
                current.reset();
                Check(DeleteFileW(path.c_str()) != FALSE, "remove existing incompatible-peer fixture");
            }
            else
                Check(GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND,
                      "incompatible peer creates no target");
        }
    }
#ifdef SAM_UAC_THIN_COPY_TEST
    {
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const auto path = target.File(L"thin-contract-failed-transfer.txt");
        const bool replied = job.Open(nullptr, nullptr, Operation::FindContent, nullptr, false, nullptr, true) &&
            job.Thin(source, sourceLeaf, sourceId, target, L"thin-contract-failed-transfer.txt",
                Operation::ThinTargetCreate, nullptr, false, status, reply, input.get());
        Check(replied && status == ERROR_PARTIAL_COPY && !reply.value &&
              GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES &&
              UacF7::TransferReply(Operation::ThinTargetCreate, status, true).unknown,
              "failure after creation preserves target and reports an uncertain partial result");
        job.Finish();
    }
    for (int scenario = 0; scenario != 3; ++scenario)
    {
        const wchar_t* leaf = scenario == 0 ? L"thin-contract-refused-discard.txt" :
                              scenario == 1 ? L"thin-contract-foreign-write.txt" : L"thin-contract-replaced.txt";
        const auto path = target.File(leaf);
        const auto parked = target.File(L"thin-contract-original-parked.txt");
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const bool created = job.Open() && job.Thin(source, sourceLeaf, sourceId, target, leaf,
            Operation::ThinTargetCreate, nullptr, false, status, reply, input.get()) && !status && reply.value;
        Check(created, "create target with matching thin contract");
        if (!created) { job.Finish(); continue; }
        const FILE_ID_INFO targetId = reply.target;
        auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
        const BYTE initial = 0x45;
        DWORD count = 0;
        Check(output && WriteFile(output.get(), &initial, 1, &count, nullptr) && count == 1,
              "write target through returned handle");
        Handle foreign;
        if (scenario == 1)
        {
            HANDLE duplicate = nullptr;
            Check(output && DuplicateHandle(GetCurrentProcess(), output.get(), GetCurrentProcess(), &duplicate,
                  0, FALSE, DUPLICATE_SAME_ACCESS), "duplicate a previously granted target writer");
            foreign = Adopt(duplicate);
        }
        output.reset();
        if (scenario == 1)
        {
            FILE_ID_INFO actual{};
            const BYTE changed = 0x7a;
            LARGE_INTEGER zero{};
            count = 0;
            Check(foreign && GetFileInformationByHandleEx(foreign.get(), FileIdInfo, &actual, sizeof(actual)) &&
                  UacRenameLeaf::Detail::SameSource(actual, targetId) &&
                  SetFilePointerEx(foreign.get(), zero, nullptr, FILE_BEGIN) &&
                  WriteFile(foreign.get(), &changed, 1, &count, nullptr) && count == 1,
                  "already granted duplicate changes the retained target");
        }
        if (scenario == 2)
            Check(!MoveFileW(path.c_str(), parked.c_str()) && GetLastError() == ERROR_SHARING_VIOLATION,
                  "retained target prevents this pathname replacement");
        const bool sent = job.Thin(source, sourceLeaf, sourceId, target, leaf, Operation::ThinDiscardTarget,
            &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, scenario == 0 ? 4u : 12u);
        if (scenario == 0)
            Check(!sent && HasByte(path, initial), "old host cannot discard using new broker");
        if (scenario == 1)
        {
            Check(sent && status == ERROR_PARTIAL_COPY && !reply.value && foreign &&
                  GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES,
                  "uncertain discard preserves a live previously granted duplicate");
            foreign.reset();
            Check(HasByte(path, 0x7a), "previously granted duplicate's bytes remain readable");
        }
        if (scenario == 2)
            Check(sent && status == ERROR_PARTIAL_COPY && HasByte(path, initial) &&
                  GetFileAttributesW(parked.c_str()) == INVALID_FILE_ATTRIBUTES,
                  "uncertain discard preserves the original retained target");
        if (scenario != 0)
            Check(UacF7::TransferReply(Operation::ThinDiscardTarget, status, true).unknown &&
                  !UacF7::CanContinueTransferJob(status), "partial cleanup remains unknown and stops job");
        job.Finish();
    }
#endif
    input.reset();
    Check(DeleteFileW(sourcePath.c_str()) != FALSE, "remove thin contract source fixture");
}
