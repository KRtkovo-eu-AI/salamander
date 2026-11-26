// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-13 | Preserve uncertain large-parent targets. | Verify fail-closed cleanup over the dispatcher. | #731 | GPT-6 |
// | 2026-11-12 | Exercise large thin-copy parents through the real dispatcher. | Check cursor isolation and cancellation before target creation. | #731 | GPT-6 |

void CheckThinLargeParent(const Directory& source, const Directory& target)
{
    constexpr unsigned rows = 4104;
    const auto parentPath = source.File(L"thin-large-parent-東");
    const auto targetPath = target.File(L"thin-large-target.txt");
    const auto cancelledPath = target.File(L"thin-large-cancelled.txt");
    Directory parent;
    if (!parent.Open(parentPath.c_str())) { Check(false, "prepare large thin parent"); return; }
    unsigned created = 0;
    for (; created != rows; ++created)
    {
        wchar_t name[32]{};
        swprintf_s(name, L"row-%05u.txt", created);
        if (!WriteByte(parent.File(name), 0x5a)) break;
    }
    Check(created == rows, "prepare over 4096 real source-parent entries");
    auto scout = Adopt(CreateFileW(parentPath.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    auto hostParent = Adopt(CreateFileW(parentPath.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    std::vector<ULONGLONG> buffer(65536 / sizeof(ULONGLONG));
    const DWORD bytes = static_cast<DWORD>(buffer.size() * sizeof(ULONGLONG));
    bool foundSecond = created == rows && scout && hostParent &&
        GetFileInformationByHandleEx(scout.get(), FileIdExtdDirectoryRestartInfo, buffer.data(), bytes) &&
        GetFileInformationByHandleEx(scout.get(), FileIdExtdDirectoryInfo, buffer.data(), bytes);
    std::wstring leaf;
    if (foundSecond)
    {
        const auto* row = reinterpret_cast<const FILE_ID_EXTD_DIR_INFO*>(buffer.data());
        if (row->FileNameLength && !(row->FileNameLength % sizeof(wchar_t)) &&
            row->FileNameLength / sizeof(wchar_t) <= UacDirectoryLeaf::maxComponent)
            leaf.assign(row->FileName, row->FileNameLength / sizeof(wchar_t));
        foundSecond = UacDirectoryLeaf::IsComponent(leaf);
    }
    Check(foundSecond, "select an actual source name from the second NTFS listing page");
    auto input = foundSecond ? Adopt(CreateFileW(parent.File(leaf).c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr)) : Handle{};
    FILE_ID_INFO sourceId{};
    const bool prepared = input && GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId));
    Check(prepared, "capture the actual second-page source identity");
    if (prepared)
    {
        auto reached = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        auto resume = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        ThinIdentityScanPause pause{reached.get(), resume.get()};
        JobPipe job;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        const bool opened = reached && resume && job.Open(nullptr, nullptr, Operation::FindContent, nullptr, false, &pause);
        Check(opened, "open the paused real thin dispatcher");
        if (opened)
        {
            bool replied = false;
            std::thread request([&] {
                replied = job.Thin(parent, leaf, sourceId, target, L"thin-large-target.txt",
                    Operation::ThinTargetCreate, nullptr, false, status, reply, input.get(), hostParent.get());
            });
            const bool paused = WaitForSingleObject(reached.get(), 10000) == WAIT_OBJECT_0;
            bool hostAdvanced = false;
            if (paused)
            {
                // Advance the host's own file object through the second page
                // while the real broker scan is paused before its second page.
                hostAdvanced = GetFileInformationByHandleEx(hostParent.get(), FileIdExtdDirectoryRestartInfo,
                                                            buffer.data(), bytes) &&
                               GetFileInformationByHandleEx(hostParent.get(), FileIdExtdDirectoryInfo,
                                                            buffer.data(), bytes);
            }
            SetEvent(resume.get());
            request.join();
            Check(paused && hostAdvanced && replied && !status && reply.value,
                  "dispatcher finds second-page source despite host cursor advancement in a large parent");
            const auto createdId = reply.target;
            auto output = Adopt(reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(reply.value)));
            BYTE byte = 0x5a;
            DWORD count = 0;
            Check(output && WriteFile(output.get(), &byte, 1, &count, nullptr) && count == 1,
                  "large-parent target accepts the approved worker write");
            output.reset();
            Check(job.Thin(parent, leaf, sourceId, target, L"thin-large-target.txt", Operation::ThinDiscardTarget,
                &createdId, false, status, reply) && status == ERROR_PARTIAL_COPY && HasByte(targetPath, 0x5a),
                "large-parent target is preserved when cleanup continuity is uncertain");
        }
        job.Finish();

        auto cancelReached = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        auto cancelResume = Adopt(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        ThinIdentityScanPause cancelPause{cancelReached.get(), cancelResume.get()};
        JobPipe cancelJob;
        status = ERROR_GEN_FAILURE;
        reply = {};
        const bool cancelOpened = cancelReached && cancelResume &&
            cancelJob.Open(nullptr, nullptr, Operation::FindContent, nullptr, false, &cancelPause);
        Check(cancelOpened, "open the real dispatcher for a cancelled large scan");
        if (cancelOpened)
        {
            bool replied = false;
            std::thread request([&] {
                replied = cancelJob.Thin(parent, leaf, sourceId, target, L"thin-large-cancelled.txt",
                    Operation::ThinTargetCreate, nullptr, false, status, reply, input.get(), hostParent.get());
            });
            const bool paused = WaitForSingleObject(cancelReached.get(), 10000) == WAIT_OBJECT_0;
            const DWORD sent = paused ? cancelJob.CancelActive() : ERROR_TIMEOUT;
            SetEvent(cancelResume.get());
            request.join();
            Check(paused && !sent && replied && status == ERROR_CANCELLED && !reply.value &&
                  GetFileAttributesW(cancelledPath.c_str()) == INVALID_FILE_ATTRIBUTES,
                  "live cancellation during a large scan creates no target");
        }
        cancelJob.Finish();
    }
    input.reset();
    hostParent.reset();
    scout.reset();
    bool cleaned = true;
    for (unsigned i = 0; i != created; ++i)
    {
        wchar_t name[32]{};
        swprintf_s(name, L"row-%05u.txt", i);
        cleaned = DeleteFileW(parent.File(name).c_str()) != FALSE && cleaned;
    }
    parent.handle.reset();
    cleaned = RemoveDirectoryW(parentPath.c_str()) != FALSE && cleaned;
    Check(cleaned, "remove only the large-parent test fixture");
}
