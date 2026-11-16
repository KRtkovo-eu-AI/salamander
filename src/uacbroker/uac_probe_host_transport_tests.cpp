// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-11 | Test host overwrite probe framing against a local fake peer. | Keep read-only failures separate from uncertain commits. | #731 | GPT-6.1 Sol |
// Executes the host transport on a real local pipe. The peer is a test double;
// installation checks, broker dispatcher, authentication, UAC and UI are not exercised.

#define SAM_UAC_F7_HOST
#include "uacbroker.cpp"
#include <thread>

namespace
{
enum class Scenario { DropStatus, DropReply, ShortReply, BadVersion, BadIdentity, Success, Failure, CommitDrop };

struct Outcome
{
    UacF7::Result result{};
    UacF7::OverwriteProbeReply reply{};
    DWORD peerError = ERROR_SUCCESS;
    bool receivedRequest = false;
    bool receivedNonce = false;
    bool open = false;
};

bool Run(Scenario scenario, Outcome& outcome)
{
    GUID pipeId{};
    if (FAILED(CoCreateGuid(&pipeId))) return false;
    const auto name = PipeName(pipeId);
    auto server = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 4096, 4096, 0, nullptr));
    auto peer = server ? Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING,
                                            FILE_FLAG_OVERLAPPED, nullptr)) : Handle{};
    if (!peer || Connect(server.get(), GetTickCount64() + timeoutMs)) return false;

    RequestHeader header{};
    header.operation = scenario == Scenario::CommitDrop ? Operation::CopyOverwrite : Operation::ProbeOverwrite;
    header.serial = 7; header.targetSerial = 9; header.copySize = 3;
    header.parentId.Identifier[0] = 1; header.sourceId.Identifier[0] = 2; header.targetParentId.Identifier[0] = 3;
    std::wstring payload = L"read-only-fixture";
    UacF7::ArchiveRevision target{};
    target.serial = 9; target.fileId[0] = 4; target.size = 8; target.attributes = FILE_ATTRIBUTE_ARCHIVE;
    GUID nonce{}; nonce.Data1 = 11;

    std::thread fakePeer([&] {
        const auto deadline = GetTickCount64() + 3000;
        RequestHeader request{};
        std::wstring body(payload.size(), L'\0');
        outcome.peerError = TransferRequest(peer.get(), &request, sizeof(request), false, deadline);
        if (!outcome.peerError) outcome.peerError = TransferRequest(peer.get(), body.data(),
            static_cast<DWORD>(body.size() * sizeof(wchar_t)), false, deadline);
        if (outcome.peerError) { peer.reset(); return; }
        outcome.receivedRequest = request.operation == header.operation && body == payload;
        if (scenario == Scenario::CommitDrop)
        {
            UacF7::ArchiveRevision received{};
            GUID receivedNonce{};
            outcome.peerError = TransferRequest(peer.get(), &received, sizeof(received), false, deadline);
            if (!outcome.peerError) outcome.peerError = TransferRequest(peer.get(), &receivedNonce, sizeof(receivedNonce), false, deadline);
            outcome.receivedNonce = !outcome.peerError && IsEqualGUID(receivedNonce, nonce);
            peer.reset();
            return;
        }
        if (scenario == Scenario::DropStatus) { peer.reset(); return; }
        DWORD status = scenario == Scenario::Failure ? ERROR_ACCESS_DENIED : ERROR_SUCCESS;
        outcome.peerError = TransferRequest(peer.get(), &status, sizeof(status), true, deadline);
        if (outcome.peerError || scenario == Scenario::DropReply) { peer.reset(); return; }
        UacF7::OverwriteProbeReply reply{};
        if (status == ERROR_SUCCESS)
        {
            reply.present = 1; reply.nonce = nonce;
            reply.sourceParentSerial = header.serial; reply.targetParentSerial = header.targetSerial;
            std::memcpy(reply.sourceParentId, header.parentId.Identifier, sizeof(reply.sourceParentId));
            std::memcpy(reply.targetParentId, header.targetParentId.Identifier, sizeof(reply.targetParentId));
            reply.source.serial = header.serial;
            std::memcpy(reply.source.fileId, header.sourceId.Identifier, sizeof(reply.source.fileId));
            reply.source.size = static_cast<LONGLONG>(header.copySize);
            reply.source.attributes = FILE_ATTRIBUTE_ARCHIVE;
            reply.target = target;
        }
        if (scenario == Scenario::BadVersion) ++reply.version;
        if (scenario == Scenario::BadIdentity) reply.sourceParentId[15] ^= 1;
        const DWORD bytes = scenario == Scenario::ShortReply ? sizeof(DWORD) : sizeof(reply);
        outcome.peerError = TransferRequest(peer.get(), &reply, bytes, true, deadline);
        peer.reset();
    });
    HANDLE connection = server.release();
    if (scenario == Scenario::CommitDrop)
        outcome.result = UacF7::RepeatTransferJob(connection, header, payload, &target, sizeof(target),
                                                   nullptr, nullptr, nullptr, nullptr, nullptr, &nonce);
    else
        outcome.result = UacF7::RepeatTransferJob(connection, header, payload, nullptr, 0,
                                                   nullptr, nullptr, nullptr, nullptr, &outcome.reply);
    outcome.open = connection != nullptr;
    if (connection) CloseHandle(connection);
    fakePeer.join();
    return outcome.peerError == ERROR_SUCCESS && outcome.receivedRequest;
}

bool RunPrecancelled()
{
    GUID id{};
    if (FAILED(CoCreateGuid(&id))) return false;
    const auto name = PipeName(id);
    auto server = Adopt(CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1, 4096, 4096, 0, nullptr));
    const auto peer = server ? Adopt(CreateFileW(name.c_str(), pipeAccess, 0, nullptr, OPEN_EXISTING,
                                                 FILE_FLAG_OVERLAPPED, nullptr)) : Handle{};
    const auto cancel = Adopt(CreateEventW(nullptr, TRUE, TRUE, nullptr));
    if (!peer || !cancel || Connect(server.get(), GetTickCount64() + timeoutMs)) return false;
    RequestHeader header{}; header.operation = Operation::CopyOverwrite;
    std::wstring payload = L"precancelled";
    UacF7::ArchiveRevision target{};
    GUID nonce{}; nonce.Data1 = 11;
    HANDLE connection = server.release();
    const auto result = UacF7::RepeatTransferJob(connection, header, payload, &target, sizeof(target),
                                                  nullptr, cancel.get(), nullptr, nullptr, nullptr, &nonce);
    DWORD available = 0;
    const BOOL peek = PeekNamedPipe(peer.get(), nullptr, 0, nullptr, &available, nullptr);
    const DWORD peekError = peek ? ERROR_SUCCESS : GetLastError();
    if (connection) CloseHandle(connection);
    return result.error == ERROR_CANCELLED && !result.unknown && !connection && !available &&
           (peek || peekError == ERROR_BROKEN_PIPE);
}
}

int main()
{
    int failures = 0, checks = 0;
    const auto check = [&](bool ok, const char* description) {
        ++checks;
        if (!ok) { ++failures; std::printf("FAIL: %s\n", description); }
    };
    for (const auto scenario : {Scenario::DropStatus, Scenario::DropReply, Scenario::ShortReply,
                                 Scenario::BadVersion, Scenario::BadIdentity})
    {
        Outcome result;
        const bool ran = Run(scenario, result);
        check(ran && result.result.error != ERROR_SUCCESS && !result.result.unknown && !result.open && !result.reply.present,
              "read-only probe transport failure closes session without unknown or usable reply");
        if (!ran || result.result.unknown)
            std::printf("scenario=%d peer=%lu error=%lu unknown=%d\n", static_cast<int>(scenario), result.peerError,
                        result.result.error, result.result.unknown);
    }
    {
        Outcome result;
        check(Run(Scenario::Success, result) && !result.result.error && !result.result.unknown && result.reply.present == 1,
              "complete valid probe reply is usable");
    }
    {
        Outcome result;
        check(Run(Scenario::Failure, result) && result.result.error == ERROR_ACCESS_DENIED &&
              !result.result.unknown && !result.reply.present, "valid failure is reported without mutation uncertainty");
    }
    {
        Outcome result;
        check(Run(Scenario::CommitDrop, result) && result.receivedNonce && result.result.error != ERROR_SUCCESS &&
              result.result.unknown && !result.open, "lost terminal reply after commit nonce remains unknown");
    }
    check(RunPrecancelled(), "pre-cancelled commit sends no pipe bytes and has no unknown result");
    std::printf("host probe transport: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
