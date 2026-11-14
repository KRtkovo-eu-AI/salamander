// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-13 | Exercise retained thin targets through the dispatcher. | Verify identity, mode and pin lifetime without assuming revocation. | #731 | GPT-6 |

void CheckThinRetainedTarget(const Directory& source, const Directory& target, const Directory& outside)
{
    constexpr auto sourceLeaf = L"retained-source-東.txt";
    constexpr auto targetLeaf = L"retained-target-日.txt";
    constexpr DWORD contract = 12;
    const auto sourcePath = source.File(sourceLeaf);
    const auto targetPath = target.File(targetLeaf);
    Check(WriteByte(sourcePath, 0x51), "retained test prepares source");
    auto input = Adopt(CreateFileW(sourcePath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO sourceId{};
    Check(input && GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId)),
          "retained test identifies source");
    JobPipe job;
    Check(job.Open(), "retained test opens dispatcher");
    DWORD status = ERROR_GEN_FAILURE;
    ThinWireReply reply{};
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetCreate,
        nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "retained target created over real job pipe");
    const FILE_ID_INFO targetId = reply.target;
    auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    BYTE byte = 0x51; DWORD count = 0;
    Check(output && WriteFile(output.get(), &byte, 1, &count, nullptr) && count == 1,
        "host writes through retained target duplicate");
    byte = 0; count = 0;
    Check(output && !ReadFile(output.get(), &byte, 1, &count, nullptr) && GetLastError() == ERROR_ACCESS_DENIED,
        "initial target duplicate grants write but not read");
    output.reset();
    auto competing = Adopt(CreateFileW(targetPath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    Check(!competing && GetLastError() == ERROR_SHARING_VIOLATION, "broker pins target between host opens");
    FILE_ID_INFO wrong = targetId;
    wrong.FileId.Identifier[0] ^= 1;
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetRetry,
        &wrong, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && status == ERROR_FILE_INVALID,
        "retained retry rejects changed target ID");
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetRetry,
        &targetId, true, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && status == ERROR_INVALID_DATA,
        "retained retry rejects changed I/O mode");
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetRetry,
        &targetId, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "retained retry duplicates verified pinned object");
    output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    LARGE_INTEGER zero{};
    Check(output && SetFilePointerEx(output.get(), zero, nullptr, FILE_BEGIN) &&
        ReadFile(output.get(), &byte, 1, &count, nullptr) && count == 1 && byte == 0x51,
        "retry duplicate supports original worker verification");
    Check(job.Thin(source, sourceLeaf, sourceId, outside, targetLeaf, Operation::ThinTargetRelease,
        &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) && status == ERROR_ACCESS_DENIED,
        "release rejects another target parent");
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetRelease,
        &wrong, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) && status == ERROR_FILE_INVALID,
        "release rejects changed identity");
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetRelease,
        &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) && !status && !reply.value,
        "release drops broker reference only");
    byte = 0; count = 0;
    Check(output && SetFilePointerEx(output.get(), zero, nullptr, FILE_BEGIN) &&
        ReadFile(output.get(), &byte, 1, &count, nullptr) && count == 1 && byte == 0x51,
        "release does not revoke a host duplicate");
    output.reset();
    Check(HasByte(targetPath, 0x51), "released target can be reopened normally");
    Check(job.Thin(source, sourceLeaf, sourceId, target, targetLeaf, Operation::ThinTargetRelease,
        &targetId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) && status == ERROR_FILE_INVALID,
        "duplicate release is rejected");

    constexpr auto asyncLeaf = L"retained-async.txt";
    const auto asyncPath = target.File(asyncLeaf);
    Check(job.Thin(source, sourceLeaf, sourceId, target, asyncLeaf, Operation::ThinTargetCreate,
        nullptr, true, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "create retained overlapped target");
    const FILE_ID_INFO asyncId = reply.target;
    output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    output.reset();
    Check(job.Thin(source, sourceLeaf, sourceId, target, asyncLeaf, Operation::ThinTargetRetry,
        &asyncId, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && status == ERROR_INVALID_DATA,
        "overlapped target refuses synchronous retry");
    Check(job.Thin(source, sourceLeaf, sourceId, target, asyncLeaf, Operation::ThinTargetRetry,
        &asyncId, true, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "overlapped target retains its mode on retry");
    output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    OVERLAPPED overlapped{};
    auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    overlapped.hEvent = event.get();
    byte = 0x51; count = 0;
    BOOL written = output && WriteFile(output.get(), &byte, 1, &count, &overlapped);
    if (!written && GetLastError() == ERROR_IO_PENDING)
        written = GetOverlappedResult(output.get(), &overlapped, &count, TRUE);
    Check(written && count == 1, "retried overlapped target completes real asynchronous I/O");
    output.reset();
    Check(job.Thin(source, sourceLeaf, sourceId, target, asyncLeaf, Operation::ThinTargetRelease,
        &asyncId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) && !status,
        "release unlocks completed overlapped target");
    Check(HasByte(asyncPath, 0x51), "overlapped target remains intact after release");

    constexpr auto cancelLeaf = L"retained-cancel.txt";
    const auto cancelPath = target.File(cancelLeaf);
    Check(job.Thin(source, sourceLeaf, sourceId, target, cancelLeaf, Operation::ThinTargetCreate,
        nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "cancel test creates retained target");
    const FILE_ID_INFO cancelId = reply.target;
    output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    byte = 0x51; count = 0;
    Check(output && WriteFile(output.get(), &byte, 1, &count, nullptr), "cancel test writes partial output");
    output.reset();
    Check(job.Thin(source, sourceLeaf, sourceId, target, cancelLeaf, Operation::ThinDiscardTarget,
        &cancelId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) && status == ERROR_PARTIAL_COPY,
        "uncertain cancel fails closed instead of deleting by pathname");
    Check(HasByte(cancelPath, 0x51), "cancel releases pin while preserving partial target");
    const bool continued = job.Thin(source, sourceLeaf, sourceId, target, cancelLeaf, Operation::ThinTargetRetry,
        &cancelId, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract);
    Check(!continued, "uncertain cancel terminates the job without a stale retry");
    job.Finish();

    constexpr auto badLeaf = L"retained-bad-peer.txt";
    const auto badPath = target.File(badLeaf);
    JobPipe badPeer;
    Check(badPeer.Open() && badPeer.Thin(source, sourceLeaf, sourceId, target, badLeaf, Operation::ThinTargetCreate,
        nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "bad peer test creates retained target");
    const FILE_ID_INFO badId = reply.target;
    output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    byte = 0x51; count = 0;
    Check(output && WriteFile(output.get(), &byte, 1, &count, nullptr), "bad peer test writes target");
    output.reset();
    const bool badReply = badPeer.Thin(source, sourceLeaf, sourceId, target, badLeaf, Operation::ThinTargetRelease,
        &badId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 4u);
    Check(!badReply || status != ERROR_SUCCESS, "mismatched release contract is rejected");
    badPeer.Finish();
    Check(HasByte(badPath, 0x51), "bad peer disconnect releases pin and preserves target");

    constexpr auto refusedLeaf = L"retained-refused.txt";
    const auto refusedPath = target.File(refusedLeaf);
    JobPipe refused;
    Check(refused.Open() && refused.Thin(source, sourceLeaf, sourceId, target, refusedLeaf, Operation::ThinTargetCreate,
        nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, contract) && !status && reply.value,
        "refused cleanup test creates retained target");
    FILE_ID_INFO refusedId = reply.target;
    output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
    byte = 0x51; count = 0;
    Check(output && WriteFile(output.get(), &byte, 1, &count, nullptr), "refused cleanup test writes target");
    output.reset();
    refusedId.FileId.Identifier[0] ^= 1;
    Check(refused.Thin(source, sourceLeaf, sourceId, target, refusedLeaf, Operation::ThinDiscardTarget,
        &refusedId, false, status, reply, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, contract) &&
        status == ERROR_FILE_INVALID, "wrong identity refuses discard before releasing target pin");
    competing = Adopt(CreateFileW(refusedPath.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr));
    Check(!competing && GetLastError() == ERROR_SHARING_VIOLATION, "refused discard still pins target until job closes");
    refused.Finish();
    Check(HasByte(refusedPath, 0x51), "closing job after refused discard releases pin without deletion");
    input.reset();
    Check(DeleteFileW(targetPath.c_str()) && DeleteFileW(asyncPath.c_str()) &&
        DeleteFileW(cancelPath.c_str()) && DeleteFileW(badPath.c_str()) && DeleteFileW(refusedPath.c_str()) &&
        DeleteFileW(sourcePath.c_str()),
        "retained fixtures removed");
}
