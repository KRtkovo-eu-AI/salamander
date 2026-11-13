// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
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
             Operation::ThinTargetRetry, Operation::ThinTargetProbe, Operation::ThinDiscardTarget})
    {
      for (DWORD contract : {0u, 4u})
      {
#ifdef SAM_UAC_THIN_COPY_TEST
        if (contract == 4u) continue; // This combination is the compatible peer.
#endif
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
        Check(ready && !sent && HasByte(ordinaryCopy, 0x44) &&
              GetFileAttributesW(deniedTarget.c_str()) == INVALID_FILE_ATTRIBUTES,
              "incompatible thin operation is rejected in a live ordinary-copy job without mutation");
        job.Finish();
        Check(DeleteFileW(ordinaryCopy.c_str()) != FALSE, "remove ordinary-copy matrix fixture");
      }
    }
    {
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const auto path = target.File(L"thin-contract-refused-create.txt");
        const bool opened = job.Open();
#ifdef SAM_UAC_THIN_COPY_TEST
        const bool sent = opened && job.Thin(source, sourceLeaf, sourceId, target,
            L"thin-contract-refused-create.txt", Operation::ThinTargetCreate, nullptr, false, status, reply,
            input.get(), INVALID_HANDLE_VALUE, false, 0u);
#else
        const bool sent = opened && job.Thin(source, sourceLeaf, sourceId, target,
            L"thin-contract-refused-create.txt", Operation::ThinTargetCreate, nullptr, false, status, reply,
            input.get(), INVALID_HANDLE_VALUE, 0u);
#endif
        Check(opened && !sent && GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES,
              "old thin contract is refused before target creation");
        job.Finish();
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
        output.reset();
        Handle foreign;
        if (scenario == 1)
        {
            foreign = Adopt(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
            FILE_ID_INFO actual{};
            FILE_BASIC_INFO original{};
            const BYTE changed = 0x7a;
            LARGE_INTEGER zero{};
            count = 0;
            Check(foreign && GetFileInformationByHandleEx(foreign.get(), FileIdInfo, &actual, sizeof(actual)) &&
                  UacRenameLeaf::Detail::SameSource(actual, targetId) &&
                  GetFileInformationByHandleEx(foreign.get(), FileBasicInfo, &original, sizeof(original)) &&
                  WriteFile(foreign.get(), &changed, 1, &count, nullptr) && count == 1 &&
                  SetFileInformationByHandle(foreign.get(), FileBasicInfo, &original, sizeof(original)) &&
                  SetFilePointerEx(foreign.get(), zero, nullptr, FILE_BEGIN),
                  "independent writer alters same file ID and restores timestamps");
        }
        if (scenario == 2)
            Check(MoveFileW(path.c_str(), parked.c_str()) && WriteByte(path, 0x72),
                  "replace target path and retain original object");
        const bool sent = job.Thin(source, sourceLeaf, sourceId, target, leaf, Operation::ThinDiscardTarget,
            &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, scenario == 0 ? 0u : 4u);
        if (scenario == 0)
            Check(!sent && HasByte(path, initial), "old host cannot discard using new broker");
        if (scenario == 1)
        {
            BYTE observed = 0;
            count = 0;
            Check(sent && status == ERROR_PARTIAL_COPY && !reply.value && foreign &&
                  ReadFile(foreign.get(), &observed, 1, &count, nullptr) && count == 1 && observed == 0x7a &&
                  GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES,
                  "uncertain discard preserves a live same-object foreign write");
            foreign.reset();
            Check(HasByte(path, 0x7a), "foreign bytes remain readable");
        }
        if (scenario == 2)
            Check(sent && status == ERROR_PARTIAL_COPY && HasByte(path, 0x72) && HasByte(parked, initial),
                  "uncertain discard preserves replacement and parked original");
        if (scenario != 0)
            Check(UacF7::TransferReply(Operation::ThinDiscardTarget, status, true).unknown &&
                  !UacF7::CanContinueTransferJob(status), "partial cleanup remains unknown and stops job");
        job.Finish();
    }
#endif
    input.reset();
    Check(DeleteFileW(sourcePath.c_str()) != FALSE, "remove thin contract source fixture");
}
