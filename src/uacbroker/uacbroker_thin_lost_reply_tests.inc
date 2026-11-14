// SPDX-FileCopyrightText: 2026 Open Samandarin Authors
// SPDX-License-Identifier: GPL-2.0-or-later
// | Date | Summary | Reason | Ticket | Model |
// | 2026-11-14 | Measure lost replies after duplicate delivery. | Expose the lifetime of unclaimed host handles across repeated jobs. | #731 | gpt-6.1-terra |

// Same-process, same-token test: the dispatcher uses a real pipe and the
// actual DuplicateHandle call. The seam records the otherwise unknown
// numeric handle solely so this isolated test can inspect and close it.
void CheckThinLostReply(const Directory& source, const Directory& target)
{
    constexpr wchar_t sourceLeaf[] = L"lost-reply-source-東.txt";
    Check(WriteByte(source.File(sourceLeaf), 0x42), "prepare lost-reply source");
    auto input = Adopt(CreateFileW(source.File(sourceLeaf).c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
    FILE_ID_INFO sourceId{};
    if (!input || !GetFileInformationByHandleEx(input.get(), FileIdInfo, &sourceId, sizeof(sourceId)))
    { Check(false, "identify lost-reply source"); return; }

    std::vector<HANDLE> unclaimed;
    for (int iteration = 0; iteration != 3; ++iteration)
    {
        const std::wstring leaf = L"lost-reply-target-" + std::to_wstring(iteration) + L"-日.txt";
        ULONGLONG recorded = 0;
        DWORD status = ERROR_GEN_FAILURE;
        ThinWireReply reply{};
        JobPipe job;
        const bool started = job.Open(nullptr, nullptr, Operation::FindContent, nullptr, false,
                                      nullptr, false, false, nullptr, &recorded);
        const bool gotReply = started && job.Thin(source, sourceLeaf, sourceId, target, leaf,
            Operation::ThinTargetCreate, nullptr, false, status, reply, input.get(),
            INVALID_HANDLE_VALUE, false, 28u);
        const DWORD finished = job.Finish(); // Joins the dispatcher before reading its test marker.
        HANDLE remote = reinterpret_cast<HANDLE>(static_cast<ULONG_PTR>(recorded));
        LARGE_INTEGER size{};
        Check(started && !gotReply && finished == ERROR_BROKEN_PIPE && !reply.value && remote &&
              GetFileType(remote) == FILE_TYPE_DISK && GetFileSizeEx(remote, &size) && size.QuadPart == 0,
              "reply lost after duplication leaves host handle open");
        Check(remote && std::find(unclaimed.begin(), unclaimed.end(), remote) == unclaimed.end(),
              "independent lost job produces a distinct unclaimed handle");
        if (remote) unclaimed.push_back(remote);
    }
    Check(unclaimed.size() == 3 && HasByte(source.File(sourceLeaf), 0x42),
          "three successive lost replies leave three handles while source stays intact");
    for (HANDLE file : unclaimed)
        Check(CloseHandle(file) != FALSE, "test closes its recorded unclaimed handle");
    for (int iteration = 0; iteration != 3; ++iteration)
    {
        const std::wstring leaf = L"lost-reply-target-" + std::to_wstring(iteration) + L"-日.txt";
        Check(DeleteFileW(target.File(leaf).c_str()) != FALSE, "remove only dedicated lost-reply fixture");
    }
}
