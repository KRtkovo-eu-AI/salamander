// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Test identity-bound cleanup without content isolation. | Preserve native cancellation while rejecting replacements and wrong scopes. | #731 | GPT-6 |

// Actual dispatcher and NTFS with a same-token client on a real pipe.
// This does not run DoCopyFile, Windows consent or another administrator.
void CheckThinOwnedCleanup(const Directory& source, const Directory& target, const Directory& outside)
{
    const wchar_t* sourceLeaf = L"owned-source-東.txt";
    Check(WriteByte(source.File(sourceLeaf), 0x42), "prepare owned cleanup source");
    auto input = Adopt(CreateFileW(source.File(sourceLeaf).c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO sourceId{};
    if (!input || !GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId)))
    { Check(false, "identify owned cleanup source"); return; }
    Directory nested, deep;
    if (!nested.Open(target.File(std::wstring(120, L'x') + L"-東").c_str()) ||
        !deep.Open(nested.File(std::wstring(120, L'y') + L"-日").c_str()))
    { Check(false, "prepare long Unicode cleanup parent"); return; }
    Check(deep.path.size() > 260, "cleanup fixture uses an actual extended-length parent");

    for (const bool asynchronous : {false, true})
    {
        for (int scenario = 0; scenario < 6; ++scenario)
        {
            const std::wstring leaf = std::wstring(asynchronous ? L"async-" : L"sync-") +
                std::to_wstring(scenario) + L"-日.txt";
            const auto path = deep.File(leaf);
            const auto parked = deep.File(leaf + L".parked");
            JobPipe job;
            DWORD status = ERROR_GEN_FAILURE;
            ThinWireReply reply{};
            const bool created = job.Open() && job.Thin(source, sourceLeaf, sourceId, deep, leaf,
                Operation::ThinTargetCreate, nullptr, asynchronous, status, reply, input.get(),
                INVALID_HANDLE_VALUE, false, 28u) &&
                !status && reply.value;
            if (!created) {
                std::printf("create status=%lu\n", status);
                Check(false, "create owned target through actual dispatcher"); return;
            }
            const FILE_ID_INFO targetId = reply.target;
            auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            const BYTE first = 0x53;
            DWORD written = 0;
            OVERLAPPED io{};
            auto event = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
            io.hEvent = event.get();
            BOOL wrote = WriteFile(output.get(), &first, 1, &written, asynchronous ? &io : nullptr);
            if (!wrote && asynchronous && GetLastError() == ERROR_IO_PENDING)
                wrote = GetOverlappedResult(output.get(), &io, &written, TRUE);
            Check(wrote && written == 1, "write and drain transferred output before cleanup");
            if (scenario != 3) output.reset();

            if (scenario == 1)
            {
                auto other = Adopt(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr));
                const BYTE changed = 0x76;
                FILE_ID_INFO id{};
                Check(other && GetFileInformationByHandleEx(other.get(), FileIdInfo, &id, sizeof(id)) &&
                    UacRenameLeaf::Detail::SameSource(id, targetId) &&
                    WriteFile(other.get(), &changed, 1, &written, nullptr) && written == 1,
                    "independent writer changes the same owned object");
            }
            if (scenario == 2)
                Check(MoveFileW(path.c_str(), parked.c_str()) && WriteByte(path, 0x79),
                    "replace pathname while original identity anchor remains alive");
            if (scenario == 4)
                Check(SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_READONLY) != FALSE,
                    "prepare real cleanup access failure");
            const auto expected = targetId;
            const Directory& requestedParent = scenario == 5 ? outside : deep;
            if (scenario == 5)
                Check(WriteByte(outside.File(leaf), 0x7a), "prepare out-of-scope target");

            const bool sent = job.Thin(source, sourceLeaf, sourceId, requestedParent, leaf,
                Operation::ThinDiscardTarget, &expected, false, status, reply,
                INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, false, 28u);
            Check(sent && !reply.value, "cleanup returns no host file handle");
            if (scenario == 0 || scenario == 1)
                Check(!status && IsAbsent(path), scenario == 0 ?
                    "cleanup removes own completed partial object" :
                    "same-object content change does not prevent cleanup");
            else if (scenario == 2)
                Check(status == ERROR_FILE_INVALID && HasByte(path, 0x79) && HasByte(parked, first),
                    "replacement refused and both original and replacement preserved");
            else if (scenario == 3)
                Check(status == ERROR_SHARING_VIOLATION, "live exclusive writer reports real sharing failure");
            else if (scenario == 4)
                Check(status == ERROR_ACCESS_DENIED && HasByte(path, first),
                    "readonly failure preserves own partial object");
            else
                Check(status == ERROR_ACCESS_DENIED && HasByte(path, first) && HasByte(outside.File(leaf), 0x7a),
                    "scope mismatch preserves both objects");
            output.reset();
            job.Finish();
            if (scenario == 3) Check(HasByte(path, first), "failed cleanup leaves written data intact");
        }
    }
    for (const bool failAnchor : {false, true})
    {
        const wchar_t* leaf = failAnchor ? L"anchor-failed-東.txt" : L"transfer-failed-日.txt";
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const bool sent = job.Open(nullptr, nullptr, Operation::FindContent, nullptr, false,
                                   nullptr, !failAnchor, failAnchor) &&
            job.Thin(source, sourceLeaf, sourceId, deep, leaf, Operation::ThinTargetCreate,
                nullptr, false, status, reply, input.get(), INVALID_HANDLE_VALUE, false, 28u);
        Check(sent && status == ERROR_ACCESS_DENIED && !reply.value && IsAbsent(deep.File(leaf)),
            failAnchor ? "anchor preparation failure cleans identified creation" :
                         "failed duplication cleans identified creation before reply");
        job.Finish();
    }
    input.reset();
    Check(HasByte(source.File(sourceLeaf), 0x42), "cleanup never changes source content");
    // Remove only this fixture's known files. The Python runner removes its
    // fixed temporary target tree after all process/dispatcher handles close.
    for (const bool asynchronous : {false, true})
        for (int scenario = 2; scenario < 6; ++scenario)
        {
            const std::wstring leaf = std::wstring(asynchronous ? L"async-" : L"sync-") +
                std::to_wstring(scenario) + L"-日.txt";
            if (scenario == 4) SetFileAttributesW(deep.File(leaf).c_str(), FILE_ATTRIBUTE_NORMAL);
            Check(DeleteFileW(deep.File(leaf).c_str()) != FALSE, "remove preserved owned test target");
            if (scenario == 2) Check(DeleteFileW(deep.File(leaf + L".parked").c_str()) != FALSE,
                "remove parked fixture original");
            if (scenario == 5) Check(DeleteFileW(outside.File(leaf).c_str()) != FALSE,
                "remove preserved out-of-scope fixture");
        }
}
